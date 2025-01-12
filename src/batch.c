#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batch.h"
#include "utils.h"

#define MAX_BATCH_COUNT 16384

int entry_parse(char *input, struct batch_entry *output) {
    int idx = 0;

    char *curr = NULL;
    char *next = input;

    while ((curr = strsep(&next, " "))) {
        size_t len = strlen(curr);

        switch (idx) {
            case 0: {
                if (len > FIRMWARE_NAME_MAX_LEN) {
                    printf("firmware name \"%s\" is too long (%d max)\n", curr, FIRMWARE_NAME_MAX_LEN);
                    return -1;
                }

                strcpy(output->firmware, curr);
                break;
            }

            case 1: {
                if (len > FILE_NAME_MAX_LEN) {
                    printf("file name \"%s\" is too long (%d max)\n", curr, FILE_NAME_MAX_LEN);
                    return -1;
                }

                strcpy(output->file, curr);
                break;
            }

            case 2: {
                if (len != KBAG_LEN_256 * 2 && len != KBAG_LEN_128 * 2) {
                    printf("KBAG %s is neither AES 256 nor 128\n", curr);
                    return -1;
                }

                output->kbag_len = len / 2;

                if (str2hex(output->kbag_len, output->kbag, curr) != output->kbag_len) {
                    printf("failed to decode KBAG %s\n", curr);
                    return -1;
                }

                break;
            }

            default: {
                printf("entry has too many components\n");
                return -1;
            }
        }

        idx++;
    }

    if (idx == 0) {
        return -2;
    }

    if (idx != 3) {
        printf("entry has too few components\n");
        return -1;
    }

    return 0;
}

int batch_parse(char *input, struct batch_entry **output, int *count) {
    char *curr_ = input;
    int _count = 1;

    while ((curr_ = strchr(curr_, '\n'))) {
        if (_count > MAX_BATCH_COUNT) {
            printf("batch is too large (%d lines max)\n", MAX_BATCH_COUNT);
            return -1;
        }

        _count++;
        curr_++;
    }

    struct batch_entry *entries = calloc(sizeof(*entries), _count);
    if (!entries) {
        printf("out of memory?!\n");
        return -1;
    }

    int raw_idx = 0;
    int real_idx = 0;

    char *curr = NULL;
    char *next = input;

    while ((curr = strsep(&next, "\n"))) {
        int ret = entry_parse(curr, &entries[real_idx]);

        switch (ret) {
            case 0: {
                raw_idx++;
                real_idx++;
                break;
            }

            case -2: {
                // empty line
                raw_idx++;
                break;
            }

            case -1: {
                printf("failed to decode line %d\n", raw_idx);
                return -1;
            }
        }
    }

    *output = entries;
    *count = real_idx;

    return 0;
}
