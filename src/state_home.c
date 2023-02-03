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
#include "board.h"

#include "keypad.h"
#include "state.h"
#include "utils.h"
#ifdef BOARD_FTHR_REVA
#include "tft_ili9341.h"
#endif
#ifdef BOARD_EVKIT_V1
#include "tft_ssd2119.h"
#include "bitmap.h"
#endif

#define X_START 56
#define Y_START 156
#define THICKNESS 4

/********************************** Type Defines  *****************************/
typedef void (*ScreenFunc)(void);

/************************************ VARIABLES ******************************/

/********************************* Static Functions **************************/


static int init(void)
{

    return 0;
}

static int key_process(int key)
{
    switch (key) {
    case KEY_1:
        state_set_current(get_faceID_state());
        break;
    default:
        break;
    }

    return 0;
}

static State g_state = { "faceID_home", init, key_process, NULL, 0 };

/********************************* Public Functions **************************/
State *get_home_state(void)
{
    return &g_state;
}
