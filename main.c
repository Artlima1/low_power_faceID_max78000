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

/**
 * @file    main.c
 * @brief   Image Capture Running on risc-v CPU
 *
 * @details
 *
 */

#define S_MODULE_NAME "MAIN"

/***** Includes *****/
#include <mxc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "MAXCAM_Debug.h"
#include "board.h"
#include "fcr_regs.h"
#include "img_capture.h"
#include "icc.h"
#include "lp.h"
#include "mxc_delay.h"
#include "rtc.h"
#include "sema_regs.h"
// #include "tft_ili9341.h"

/***** Definitions *****/
//----------------------------------------------------------
#define SYS_DIV 1       // 1,2,4,8,16,32,128    clock div
#define CLOCK_SOURCE 0  // 0: IPO,  1: ISO, 2: IBRO
//------------------------------------------------------------

extern volatile void const *__FlashStart_;  // Defined in linker file

void WakeISR(void) {
    MXC_SEMA->irq0 = MXC_F_SEMA_IRQ0_EN & ~MXC_F_SEMA_IRQ0_CM4_IRQ; //wake only RISC_V
}

void WUT_IRQHandler() {
    MXC_WUT_IntClear();
}

/********************************** Type Defines  *****************************/

/************************************ VARIABLES ******************************/

/********************************* Static Functions **************************/

/********************************* Public Functions **************************/

int main(void) {
    mxc_wut_cfg_t cfg;
    uint32_t ticks;

    MXC_ICC_Enable(MXC_ICC0); // Enable cache

    // Switch to 100 MHz clock
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    MXC_FCR->urvbootaddr = (uint32_t)&__FlashStart_; // Set RISC-V boot address
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_SMPHR); // Enable Sempahore clock
    MXC_NVIC_SetVector(RISCV_IRQn, WakeISR); // Set wakeup ISR
    NVIC_EnableIRQ(RISCV_IRQn);
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CPU1); // Enable RISC-V clock


    // Get ticks based off of milliseconds
    MXC_WUT_GetTicks(5000, MXC_WUT_UNIT_MILLISEC, &ticks);
    // config structure for one shot timer to trigger in a number of ticks
    cfg.mode    = MXC_WUT_MODE_ONESHOT;
    cfg.cmp_cnt = ticks;
    // Init WUT
    MXC_WUT_Init(MXC_WUT_PRES_1);
    //Config WUT
    MXC_WUT_Config(&cfg);
    MXC_LP_EnableWUTAlarmWakeup();
    NVIC_EnableIRQ(WUT_IRQn);

    MXC_Delay(300000);

    // MXC_WUT_Enable();
    int count;
    while(1){
        count++;
        // MXC_LP_EnterSleepMode();
        // MXC_WUT_Init(MXC_WUT_PRES_1);
    }

    return 0;

}
