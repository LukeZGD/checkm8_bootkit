#ifndef BATCH_H
#define BATCH_H

#include "libbootkit/ops.h"

#define FIRMWARE_NAME_MAX_LEN   128
#define FILE_NAME_MAX_LEN       64

struct batch_entry {
    char firmware[FIRMWARE_NAME_MAX_LEN + 1];
    char file[FILE_NAME_MAX_LEN + 1];
    uint8_t kbag[KBAG_LEN_256];
    int kbag_len;
};

int batch_parse(char *input, struct batch_entry **output, int *count);

#endif
