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


#ifndef _FACEID_H_
#define _FACEID_H_

#include <stdint.h>

#define CAMERA_FREQ (10 * 1000 * 1000)

#define CAPTURE_X 50
#define CAPTURE_Y 290
#define SKIP_X 60
#define SKIP_Y 290
#define RUN_X 160
#define RUN_Y 290
#define BACK_X 0
#define BACK_Y 280

#define HEIGHT 35
#define WIDTH 30
#define THICKNESS 1
#define IMAGE_H 40
#define IMAGE_W 40
#define FRAME_COLOR 0x535A

#define X_START 0
#define Y_START 0

#define BYTE_PER_PIXEL 2

#define IMG_SHIFT_ANALYSIS 4
#define FAST_FIFO           // if defined, it uses fast fifo instead of fifo

// Data input: HWC (little data): 160x120x3
#define DATA_SIZE_IN (160 * 120 * 3)

typedef struct {
    int8_t decision;
    char * name;
} faceID_decision_t;

faceID_decision_t faceid_run(void);

#endif // _FACEID_H_
