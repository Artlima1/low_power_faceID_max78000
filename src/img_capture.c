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

/********************************** Type Defines  *****************************/

/************************************ VARIABLES ******************************/
static void process_img(void);

extern volatile uint32_t *arm_mail_box;
extern volatile uint32_t *riscv_mail_box;

int start_img_capture(void) {
    uint32_t run_count = 0;

    camera_start_capture_image();

    while (1) {  // Capture image and run CNN

        /* Check if camera image is ready to process */
        if (camera_is_image_rcv()) {
            process_img();
            break;
        }
    }

    return 0;
}

static void process_img(void) {
    uint32_t pass_time = 0;
    uint32_t imgLen;
    uint32_t w, h;
    int ret, lum;
    uint16_t *image;
    uint8_t *raw;

    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);
    // Send the image through the UART to the console.
    // "grab_image" python program will read from the console and write to an image file.
    utils_send_img_to_pc(raw, imgLen, w, h, camera_get_pixel_format());

    pass_time = utils_get_time_ms();

    image = (uint16_t *)raw;  // 2bytes per pixel RGB565

    // left line
    image += ((IMAGE_H - (WIDTH + 2 * THICKNESS)) / 2) * IMAGE_W;

    for (int i = 0; i < THICKNESS; i++) {
        image += ((IMAGE_W - (HEIGHT + 2 * THICKNESS)) / 2);

        for (int j = 0; j < HEIGHT + 2 * THICKNESS; j++) {
            *(image++) = FRAME_COLOR;  // color
        }

        image += ((IMAGE_W - (HEIGHT + 2 * THICKNESS)) / 2);
    }

    // right line
    image = ((uint16_t *)raw) +
            (((IMAGE_H - (WIDTH + 2 * THICKNESS)) / 2) + WIDTH + THICKNESS) * IMAGE_W;

    for (int i = 0; i < THICKNESS; i++) {
        image += ((IMAGE_W - (HEIGHT + 2 * THICKNESS)) / 2);

        for (int j = 0; j < HEIGHT + 2 * THICKNESS; j++) {
            *(image++) = FRAME_COLOR;  // color
        }

        image += ((IMAGE_W - (HEIGHT + 2 * THICKNESS)) / 2);
    }

    // top + bottom lines
    image = ((uint16_t *)raw) + ((IMAGE_H - (WIDTH + 2 * THICKNESS)) / 2) * IMAGE_W;

    for (int i = 0; i < WIDTH + 2 * THICKNESS; i++) {
        image += ((IMAGE_W - (HEIGHT + 2 * THICKNESS)) / 2);

        for (int j = 0; j < THICKNESS; j++) {
            *(image++) = FRAME_COLOR;  // color
        }

        image += HEIGHT;

        for (int j = 0; j < THICKNESS; j++) {
            *(image++) = FRAME_COLOR;  // color
        }

        image += ((IMAGE_W - (HEIGHT + 2 * THICKNESS)) / 2);
    }

    PR_INFO("Frame drawing time : %d", utils_get_time_ms() - pass_time);

    pass_time = utils_get_time_ms();

    // Post image info to RISC-V mailbox
    riscv_mail_box[1] = (uint32_t)raw;
    // PR_DEBUG("Image ptr: %x",riscv_mail_box[1]);
    riscv_mail_box[2] = h;
    // PR_DEBUG("Image heigth: %x",riscv_mail_box[2]);
    riscv_mail_box[3] = w;
    // PR_DEBUG("Image width: %x\n",riscv_mail_box[3]);
    riscv_mail_box[0] = IMAGE_READY;

    // Signal the Cortex-M4 to wake up
    //MXC_SEMA->irq0 = MXC_F_SEMA_IRQ0_EN | MXC_F_SEMA_IRQ0_CM4_IRQ;

    // Read luminance level from camera
    ret = camera_get_luminance_level(&lum);

    if (ret != STATUS_OK) {
        PR_ERR("Camera Error %d", ret);
    } else {
        PR_DEBUG("Lum = %d", lum);

        // Warn if luminance level is low
        if (lum < LOW_LIGHT_THRESHOLD) {
            PR_WARN("Low Light!");
        }
    }

    PR_INFO("Screen print time : %d", utils_get_time_ms() - pass_time);
}
