#include <stdint.h>

#ifndef _IMG_CAPTURE_H_
#define _IMG_CAPTURE_H_

#define CAMERA_FREQ (10 * 1000 * 1000)

#define IMAGE_XRES 100
#define IMAGE_YRES 100
#define IMAGE_SIZE (IMAGE_XRES * IMAGE_YRES * 2)

#define BLOCK_PIXEL_W 10 /* Has to be a divisor of IMAGE_XRES */
#define BLOCK_PIXEL_H 10 /* Has to be a divisor of IMAGE_YRES */

#define SAD_THRESHOLD 60000
#define MIN_BLOCK_DIF 4000

enum {
    IMAGE_CAPTURE_BASE,
    IMAGE_CAPTURE_COMPARE
};

enum {
    IMG_CAP_RET_ERROR,
    IMG_CAP_RET_SUCCESS,
    IMG_CAP_RET_NO_CHANGE,
    IMG_CAP_RET_CHANGE,
};

typedef struct
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
} rgb_t;

uint8_t img_capture(uint8_t capture_mode);
void img_capture_send_img(void);
void img_capture_init(void);

#endif // _IMG_CAPTURE_H_