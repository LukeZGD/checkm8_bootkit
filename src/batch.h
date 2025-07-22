#ifndef BATCH_H
#define BATCH_H

#include "stddef.h"
#include "stdint.h"

int batch_start(void *input, size_t size, void **ctx);
int batch_write(void *ctx, const char *path);
void batch_quiesce(void **ctx);

int batch_get_count(void *ctx);
int batch_get_kbag(void *ctx, int idx, uint8_t *kbag, size_t *len);
int batch_set_key(void *ctx, int idx, uint8_t *kbag, size_t len);

#endif
