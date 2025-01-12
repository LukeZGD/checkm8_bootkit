#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

void hex2str(char *str, int buflen, const unsigned char *buf);
int str2hex(size_t buflen, uint8_t *buf, const char *str);

#endif
