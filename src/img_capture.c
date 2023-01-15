/******************************************************************************
 * Copyright (C) 2022 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/
#include <string.h>

#include "MAXCAM_Debug.h"
#include "camera.h"
#include "img_capture.h"
#include "led.h"
#include "mxc_delay.h"
#include "sema_regs.h"
#include "tmr.h"
#include "utils.h"

#define S_MODULE_NAME "img_capture"

/* **** Globals **** */

#define SAD_THRESHOLD 50

/********************************** Type Defines  *****************************/

typedef struct {
    uint16_t * img;
    uint32_t size;
    uint32_t h;
    uint32_t w;
} img_info_t;

enum {
    IMG_TP_BASE,
    IMG_TP_COMP, 
};

/************************************ VARIABLES ******************************/
static void store_img(uint8_t img_type);
static uint8_t img_compare(void);

static img_info_t base_img = {
    .img = NULL,
    .h = 0,
    .w = 0,
    .size = 0,
};
static img_info_t compare_img = {
    .img = NULL,
    .h = 0,
    .w = 0,
    .size = 0
};

uint8_t img_capture(uint8_t capture_mode) {
    uint8_t ret = IMG_CAP_RET_ERROR;

    camera_start_capture_image();

    while (1) {
        /* Check if camera image is ready to process */
        if (camera_is_image_rcv()) {
            switch (capture_mode) {
            case IMAGE_CAPTURE_BASE: {
                store_img(IMG_TP_BASE);
                ret = IMG_CAP_RET_SUCCESS;
                break;
            }
            case IMAGE_CAPTURE_COMPARE: {
                store_img(IMG_TP_COMP);
                ret = img_compare() ? IMG_CAP_RET_CHANGE : IMG_CAP_RET_NO_CHANGE;
                break;
            }
            default:
                break;
            }
            break;
        }
    }

    return 0;
}

static void store_img(uint8_t img_type){
    uint8_t *raw;
    img_info_t * dest;

    switch (img_type)
    {
        case IMG_TP_BASE:
            dest = &base_img;
            break;
        case IMG_TP_COMP:
            dest = &compare_img;
            break;
    }

    if(dest->size != 0){
        free(dest->img);
    }

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &dest->size, &dest->w, &dest->h);
    dest->img = (uint16_t * ) malloc(dest->size);
    memcpy(dest->img, raw, dest->size);

}

static uint8_t img_compare(void){
    uint64_t sad=0;
    for(uint32_t i=0; i<base_img.h; i++){
        for(uint32_t j=0; j<base_img.w; j++){
            sad += abs(base_img.img[i][j] - compare_img.img[i][j]);
        }
    }

    return sad>SAD_THRESHOLD;
}
