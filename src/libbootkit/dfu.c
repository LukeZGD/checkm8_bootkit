#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "dfu.h"

static size_t min(size_t first, size_t second) {
    if (first < second) {
        return first;
    } else {
        return second;
    }
}

int send_data(irecv_client_t client, unsigned char *data, size_t length) {
    size_t index = 0;

    while (index < length) {
        size_t amount = min(length - index, MAX_PACKET_SIZE);

        int ret = irecv_usb_control_transfer(client, 0x21, 1, 0, 0, data + index, amount, USB_TIMEOUT);

        if (ret != amount) {
            printf("DFU_DNLOAD ret = %d (%s)\n", ret, irecv_strerror(ret));
            return -1;
        }

        index += amount;
    }

    return 0;
}
