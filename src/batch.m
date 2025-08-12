#import <Foundation/Foundation.h>
#import "libbootkit/libbootkit.h"
#import "batch.h"
#import "utils.h"

int batch_start(void *input, size_t size, void **ctx) {
    NSError *err = nil;
    NSArray *kbags = [NSJSONSerialization JSONObjectWithData:[NSData dataWithBytes:input length:size] options:NSJSONReadingMutableContainers error:&err];
    if (err) {
        printf("failed to parse JSON %s\n", [[err description] UTF8String]);
        return -1;
    }

    *ctx = kbags;

    return 0;
}

int batch_get_count(void *ctx) {
    return [(NSArray *)ctx count];
}

int batch_get_kbag(void *ctx, int idx, uint8_t *kbag, size_t *len) {
    NSString *kbagObj = [(NSArray *)ctx objectAtIndex:idx][@"kbag"];
    if (!kbagObj) {
        return -1;
    }

    NSUInteger kbagLen = [kbagObj length] / 2;

    if (kbagLen > KBAG_LEN_256) {
        printf("KBAG @ index %d is too long\n", idx);
        return -1;
    }

    str2hex([kbagObj length], kbag, [kbagObj UTF8String]);

    *len = kbagLen;

    return 0;
}

int batch_set_key(void *ctx, int idx, uint8_t *kbag, size_t len) {
    NSMutableDictionary *entry = [(NSArray *)ctx objectAtIndex:idx];

    char tmp[len * 2 + 1];
    hex2str(tmp, len, kbag);

    entry[@"key"] = [NSString stringWithUTF8String:tmp];

    return 0;
}

int batch_write(void *ctx, const char *path) {
    NSError *err = nil;
    NSData *d = [NSJSONSerialization dataWithJSONObject:ctx options:NSJSONWritingPrettyPrinted error:&err];
    if (err) {
        printf("failed to serialize JSON %s\n", [[err description] UTF8String]);
        return -1;
    }

    if (![d writeToFile:[NSString stringWithUTF8String:path] atomically:NO]) {
        printf("failed to write batch\n");
        return -1;
    }

    return 0;
}

void batch_quiesce(void **ctx) {
    [(NSArray *)*ctx dealloc];
    *ctx = NULL;
}
