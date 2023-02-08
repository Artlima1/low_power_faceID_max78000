#ifndef __ESP32_H__
#define __ESP32_H__

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

void esp32_init(void);
void esp32_send_img(uint8_t *img, uint32_t imgLen, uint16_t w, uint16_t h, uint8_t pixelformat);

#endif