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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "MAXCAM_Debug.h"
#include "camera.h"
#include "img_capture.h"
#include "led.h"
#include "mxc_delay.h"
#include "sema_regs.h"
#include "tmr.h"
#include "utils.h"
#include "dma.h"
#include "icc.h"
#include "linked_list.h"

#define S_MODULE_NAME "img_capture"

/* **** Globals **** */

#define POWER_ON 1

#define RED_MASK        0xF800  /* 1111 1000 0000 0000 */
#define RED_OFFSET      11
#define GREEN_MASK      0x7E0   /* 0000 0111 1110 0000 */
#define GREEN_OFFSET    5  
#define BLUE_MASK       0x1F    /* 0000 0000 0001 1111 */

#define IMG_SIZE IMAGE_XRES*IMAGE_YRES*2

/********************************** Type Defines  *****************************/

typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;
} rgb_t;

enum {
    IMG_TP_BASE,
    IMG_TP_COMP, 
};

/************************************ VARIABLES ******************************/
static uint8_t store_img(uint8_t img_type);
static uint8_t img_compare(void);

void * base_img=NULL;

uint8_t img_capture(uint8_t capture_mode) {
    uint8_t ret = IMG_CAP_RET_ERROR;

    camera_start_capture_image();

    while (1) {
        /* Check if camera image is ready to process */
        if (camera_is_image_rcv()) {
            switch (capture_mode) {
            case IMAGE_CAPTURE_BASE: {
                ret = store_img(IMG_TP_BASE);
                break;
            }
            case IMAGE_CAPTURE_COMPARE: {
                ret = img_compare();
                break;
            }
            default:
                break;
            }
            break;
        }
    }

    return ret;
}

static uint8_t store_img(uint8_t img_type){
    uint8_t *raw;
    uint32_t size, w, h;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &size, &w, &h);
    uint16_t * img = (uint16_t *) raw;

    if(base_img!=NULL){
        list_free(base_img);
        base_img = list_create();
    }

    if(list_insert_array(base_img, img, w*h)==0){
        printf("Not enough memory!\n");
        return IMG_CAP_RET_ERROR;
    }

    printf("Base pic size: %d stored\n", size);

    return IMG_CAP_RET_SUCCESS;

}

static uint8_t img_compare(void){

    uint8_t *raw;
    uint32_t size, w, h;
    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &size, &w, &h);
    uint16_t * comp_img = (uint16_t * ) raw;


    uint32_t SAD=0, i;
    // uint32_t MSE=0;
    uint16_t dif, *pixel_base, *pixel_comp;
    rgb_t rgb_base, rgb_comp, rgb_dif;
    for(i=0; i<(IMG_SIZE>>1); i++){
        if(i==0){
            if(list_get(base_img, 0, &pixel_base)==0){
                printf("Error to access first pixel of base\n");
                return IMG_CAP_RET_ERROR;
            }
        }
        else {
            if(list_get_next(&pixel_base)==0){
                printf("Error in getting pixel %d of base\n", i);
                return IMG_CAP_RET_ERROR;
            }
        }

        pixel_comp = &comp_img[i];

        /* Get color values from pixel */
        rgb_base.r = ((*pixel_base) & RED_MASK) >> RED_OFFSET;
        rgb_base.g = ((*pixel_base) & GREEN_MASK) >> GREEN_OFFSET;
        rgb_base.b = ((*pixel_base) & BLUE_MASK);

        rgb_comp.r = ((*pixel_comp) & RED_MASK) >> RED_OFFSET;
        rgb_comp.g = ((*pixel_comp) & GREEN_MASK) >> GREEN_OFFSET;
        rgb_comp.b = ((*pixel_comp) & BLUE_MASK);

        rgb_dif.r = (rgb_base.r > rgb_comp.r) ? (rgb_base.r - rgb_comp.r) : (rgb_comp.r - rgb_base.r);
        rgb_dif.g = (rgb_base.g > rgb_comp.g) ? (rgb_base.g - rgb_comp.g) : (rgb_comp.g - rgb_base.g);
        rgb_dif.b = (rgb_base.b > rgb_comp.b) ? (rgb_base.b - rgb_comp.b) : (rgb_comp.b - rgb_base.b);

        dif = rgb_dif.r + rgb_dif.g + rgb_dif.b;

        SAD = SAD + dif;

        // MSE = MSE + (dif * dif) / (IMG_SIZE>>1);
    }

    printf("%u\n", SAD);

    return (SAD<SAD_THRESHOLD) ? IMG_CAP_RET_NO_CHANGE : IMG_CAP_RET_CHANGE;
}

void img_capture_init(void) {
    int ret = 0;
    int slaveAddress;
    int id;
    int dma_channel;

    // Initialize DMA for camera interface
    MXC_DMA_Init();
    dma_channel = MXC_DMA_AcquireChannel();

    /* Enable camera power */
    Camera_Power(POWER_ON);
    MXC_Delay(300000);

    // Initialize the camera driver.
    camera_init(CAMERA_FREQ);

    printf("Init Camera");

    // Obtain the I2C slave address of the camera.
    slaveAddress = camera_get_slave_address();
    printf("Camera I2C slave address is %02x\n", slaveAddress);

    // Obtain the product ID of the camera.
    ret = camera_get_product_id(&id);

    if (ret != STATUS_OK) {
        printf("Error returned from reading camera id. Error %d\n", ret);
        return;
    }

    printf("Camera Product ID is %04x\n", id);

    // Obtain the manufacture ID of the camera.
    ret = camera_get_manufacture_id(&id);

    if (ret != STATUS_OK) {
        printf("Error returned from reading camera id. Error %d\n", ret);
        return;
    }

    printf("Camera Manufacture ID is %04x\n", id);

    // Setup the camera image dimensions, pixel format and data acquiring details.
    ret = camera_setup(IMAGE_XRES, IMAGE_YRES, PIXFORMAT_RGB565, FIFO_FOUR_BYTE, USE_DMA, dma_channel);

    if (ret != STATUS_OK) {
        printf("Error returned from setting up camera. Error %d\n", ret);
        return;
    }

    camera_write_reg(0x0c, 0x56); //camera vertical flip=0

    base_img = list_create();
    printf("Base img addr: %p", base_img);

}