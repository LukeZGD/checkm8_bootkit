#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "libbootkit/libbootkit.h"
#include "batch.h"
#include "utils.h"

void print_usb_serial(irecv_client_t client) {
    const struct irecv_device_info *info = irecv_get_device_info(client);
    printf("found: %s\n", info->serial_string);
}

void print_usage(const char *program_name) {
    printf("usage: %s VERB [args]\n", program_name);
    printf("where VERB is one of the following:\n");
    printf("\tboot <bootloader>\n");
    printf("\tkbag <kbag>\n");
    printf("\tdemote\n");
    printf("\tbatch <input> <output>\n");

    printf("\nfor batch KBAG processing, you must input a text file in following format:\n\n");
    printf("\tFIRMWARE0 FILE0 KBAG\n");
    printf("\t...\n");
    printf("\tFIRMWAREn FILEn KBAG\n");

    printf("\nin return you'll get the same structure, but with IV+key pair appended to each entry\n");

    printf("\nsupported platforms:\n\t");

    size_t number_of_configs = sizeof(rom_configs) / sizeof(rom_config_t);

    for (int i = 0; i < number_of_configs; i++) {
        const rom_config_t *config = &rom_configs[i];
        (i != number_of_configs - 1) ? printf("%s, ", config->platform) : printf("%s", config->platform);
    }

    printf("\n");
}

static bool kbag_len_validate(size_t len, uint16_t cpid) {
    bool haywire = cpid == 0x8747;

    if (len == KBAG_LEN_128) {
        if (!haywire) {
            printf("128-bit KBAG provided for non-Haywire target\n");
            return false;
        }
    } else if (len == KBAG_LEN_256) {
        if (haywire) {
            printf("256-bit KBAG provided for Haywire target\n");
            return false;
        }
    } else {
        printf("KBAG is neither 256-bit nor 128?!\n");
        return false;
    }

    return true;
}

enum {
    VERB_NONE = -1,
    VERB_BOOT,
    VERB_KBAG,
    VERB_DEMOTE,
    VERB_BATCH
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        goto usage;
    }

    int ret = -1;
    int verb = VERB_NONE;
    bool debug = false;

    int fd = -1;
    int out_fd = -1;

    uint8_t *bootloader_buffer = NULL;
    size_t bootloader_size = 0;

    char *batch_input_buffer = NULL;
    size_t batch_input_size = 0;
    struct batch_entry *batch_entries = NULL;
    int batch_entry_count = 0;

    uint8_t kbag[KBAG_LEN_256] = { 0 };
    size_t kbag_len = 0;

    irecv_client_t client = NULL;

    if (strcmp(argv[1], "boot") == 0) {
        verb = VERB_BOOT;

        if (argc != 3) {
            goto missing_arg;
        }

        const char *bootloader = argv[2];

        fd = open(bootloader, O_RDONLY);
        if (fd < 0) {
            printf("failed to open bootloader\n");
            goto out;
        }

        bootloader_size = lseek(fd, 0, SEEK_END);
        if (!bootloader_size) {
            printf("bootloader is empty?!\n");
            goto out;
        }

        bootloader_buffer = malloc(bootloader_size);
        if (!bootloader_buffer) {
            printf("out of memory\n");
            goto out;
        }

        if (pread(fd, bootloader_buffer, bootloader_size, 0) != bootloader_size) {
            printf("failed to read bootloader\n");
            goto out;
        }

        close(fd);
        fd = 0;

    } else if (strcmp(argv[1], "kbag") == 0) {
        verb = VERB_KBAG;

        if (argc != 3) {
            goto missing_arg;
        }

        const char *raw_kbag = argv[2];

        size_t raw_kbag_len = strlen(raw_kbag);

        if (raw_kbag_len == KBAG_LEN_256 * 2) {
            kbag_len = KBAG_LEN_256;
        } else if (raw_kbag_len == KBAG_LEN_128 * 2) {
            kbag_len = KBAG_LEN_128;
        } else {
            printf("KBAG must be %d bytes in length (or %d bytes for Haywire)\n", KBAG_LEN_256, KBAG_LEN_128);
            return -1;
        }

        if (str2hex(kbag_len, kbag, raw_kbag) != kbag_len) {
            printf("failed to decode KBAG\n");
            return -1;
        }

    } else if (strcmp(argv[1], "demote") == 0) {
        verb = VERB_DEMOTE;
    } else if (strcmp(argv[1], "batch") == 0) {
        verb = VERB_BATCH;

        if (argc != 4) {
            goto missing_arg;
        }

        const char *input  = argv[2];

        fd = open(input, O_RDONLY);
        if (fd < 0) {
            printf("failed to open batch input\n");
            goto out;
        }

        batch_input_size = lseek(fd, 0, SEEK_END);
        if (!batch_input_size) {
            printf("batch input is empty?!\n");
            goto out;
        }

        batch_input_buffer = malloc(batch_input_size + 1);
        if (!batch_input_buffer) {
            printf("out of memory\n");
            goto out;
        }

        if (pread(fd, batch_input_buffer, batch_input_size, 0) != batch_input_size) {
            printf("failed to read batch input\n");
            goto out;
        }

        batch_input_buffer[batch_input_size] = '\0';

        close(fd);
        fd = -1;

        if (batch_parse(batch_input_buffer, &batch_entries, &batch_entry_count) != 0) {
            goto out;
        }

    }

    if (verb == VERB_NONE) {
        goto usage;
    }

    if (irecv_open_with_ecid(&client, 0) != IRECV_E_SUCCESS) {
        printf("ERROR: failed to open DFU-device\n");
        return -1;
    }

    print_usb_serial(client);

    const struct irecv_device_info *info = irecv_get_device_info(client);
    const rom_config_t *config = get_config(info->cpid);

    switch (verb) {
        case VERB_BOOT: {
            printf("loading iBoot...\n");

            if (!config->watch) {
                ret = dfu_boot(client, bootloader_buffer, bootloader_size, debug);
            } else {
                ret = dfu_boot_watch(client, bootloader_buffer, bootloader_size, debug);
            }

            goto out;
        }

        case VERB_KBAG: {
            printf("decrypting KBAG...\n");

            if (!kbag_len_validate(kbag_len, config->cpid)) {
                goto out;
            }

#define AES_IV_SIZE 0x10

            ret = aes_op(client, config, kbag, kbag_len);
            if (ret == 0) {
                printf("iv: ");
                for (size_t i = 0; i < AES_IV_SIZE; i++) {
                    printf("%02X", kbag[i]);
                }

                printf(" ");

                printf("key: ");
                for (size_t i = AES_IV_SIZE; i < kbag_len; i++) {
                    printf("%02X", kbag[i]);
                }

                printf("\n");
            }

            break;
        }

        case VERB_DEMOTE: {
            printf("demoting...\n");
            ret = demote_op(client, config, true);
            break;
        }

        case VERB_BATCH: {
            out_fd = open(argv[3], O_WRONLY | O_CREAT, 0644);
            if (out_fd < 0) {
                printf("failed to create output file\n");
                goto out;
            }

            printf("decrypting KBAG batch...\n");

            for (int i = 0; i < batch_entry_count; i++) {
                struct batch_entry *curr = &batch_entries[i];

                uint8_t kbag[KBAG_LEN_256] = { 0 };
                int kbag_len = curr->kbag_len;

                memcpy(kbag, curr->kbag, kbag_len);

                if (!kbag_len_validate(kbag_len, config->cpid)) {
                    goto out;
                }

                write(out_fd, curr->firmware, strlen(curr->firmware));
                write(out_fd, " ", 1);
                write(out_fd, curr->file, strlen(curr->file));
                write(out_fd, " ", 1);

                char kbag_str[KBAG_LEN_256 * 2 + 1] = { 0 };
                hex2str(kbag_str, kbag_len, curr->kbag);

                write(out_fd, kbag_str, kbag_len * 2);
                write(out_fd, " ", 1);

                ret = aes_op(client, config, kbag, kbag_len);
                if (ret != 0) {
                    goto out;
                }

                char key_str[KBAG_LEN_256 * 2 + 1] = { 0 };
                hex2str(key_str, kbag_len, kbag);

                write(out_fd, key_str, kbag_len * 2);
                write(out_fd, "\n", 1);

                printf("\rdecrypting: %d/%d", i + 1, batch_entry_count);
            }

            printf("\n");
        }
    }

out:
    if (fd != -1) {
        close(fd);
    }

    if (out_fd != -1) {
        close(out_fd);
    }

    if (bootloader_buffer) {
        free(bootloader_buffer);
    }

    if (batch_input_buffer) {
        free(batch_input_buffer);
    }

    if (batch_entries) {
        free(batch_entries);
    }

    if (client) {
        irecv_close(client);
    }

    if (ret == 0) {
        printf("DONE\n");
    }

    return ret;

missing_arg:
    printf("\"%s\" needs argument!\n", argv[1]);

usage:
    print_usage(argv[0]);
    return -1;
}
