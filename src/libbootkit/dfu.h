#ifndef dfu_H
#define dfu_H

#include <stdbool.h>
#include <libirecovery.h>

#define MAX_PACKET_SIZE     0x800
#define USB_SMALL_TIMEOUT   100
#define USB_TIMEOUT         1000

int send_data(irecv_client_t client, unsigned char *data, size_t length);

#endif
