#include <stdlib.h>
#include <stdbool.h>

#define DEBUG_ENABLED_VAR   "LIBBOOTKIT_DEBUG"

bool libbootkit_debug_enabled = false;

__attribute__((constructor)) void libbootkit_log_init() {
    char *var = getenv(DEBUG_ENABLED_VAR);
    if (var) {
        char *stop = NULL;
        int val = strtol(var, &stop, 0);

        if (!*stop) {
            libbootkit_debug_enabled = (bool)val;
        }
    }
}
