#ifndef __MAX78000_CMDS_H__
#define __MAX78000_CMDS_H__

#include <stdint.h>

enum {
    MSG_TYPE_NONE,
    MSG_TYPE_IMG
};

typedef struct {
    uint32_t img_len;
    uint16_t width;
    uint16_t height;
    uint8_t pixel_format;
} esp_msg_img_t;

/* typedef struct {
    uint8_t msg_type;
    union data {
        esp_msg_img_t img_msg;
    };
    union status {
        uint16_t packets_recv;
    };
} max_msg_t; */

typedef struct {
    uint8_t msg_type;
    esp_msg_img_t img_msg;
    uint16_t packets_recv;
} max_msg_t;

#endif // __MAX78000_CMDS_H__