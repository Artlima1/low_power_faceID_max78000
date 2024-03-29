#include <stdint.h>

#ifndef _IMG_CAPTURE_H_
#define _IMG_CAPTURE_H_

#define CAMERA_FREQ (10 * 1000 * 1000)

#define IMG_X_RES 130
#define IMG_Y_RES 170
#define IMG_SIZE (IMG_X_RES * IMG_Y_RES * 2)

#define BLOCK_PIXEL_W 26 /* Has to be a divisor of IMAGE_XRES */
#define BLOCK_PIXEL_H 17 /* Has to be a divisor of IMAGE_YRES */

#define SAD_THRESHOLD 60000
#define MIN_BLOCK_DIF 4000

enum {
    IMAGE_CAPTURE_BASE,
    IMAGE_CAPTURE_COMPARE
};

enum {
    IMG_RES_COMPARE, 
    IMG_RES_FACEID,
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

uint8_t img_capture_init(void);
uint8_t img_capture(uint8_t capture_mode);
void img_capture_send_img(void);
uint8_t img_capture_free_base(void);
uint8_t img_capture_rec_base(void);

#endif // _IMG_CAPTURE_H_