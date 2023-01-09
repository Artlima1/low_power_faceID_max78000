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
 * @brief   Hello World!
 * @details This example uses the UART to print to a terminal and flashes an LED.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc.h"

/***** Definitions *****/
#define SLEEP_MODE           // Select between SLEEP_MODE and LPM_MODE

#define MILLISECONDS_WUT 5000

/***** Globals *****/
int timerNumber = 0;

/***** Functions *****/

void setTrigger(int waitForTrigger)
{
    // Wait for serial transactions to complete.
    while (MXC_UART_ReadyForSleep(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {}
}

void Timer_Calculation(){

    if(timerNumber < 5){
    	timerNumber+=1;
    }else{
    	timerNumber=1;
    }
}

void WUT_IRQHandler()
{
    MXC_WUT_IntClear();

    Timer_Calculation();

    if (timerNumber==1){
    	LED_On(LED1);
    	MXC_Delay(500000);
    	LED_Off(LED1);
    	MXC_Delay(500000);
    	printf("\nCapturing Pic1\n");
    }else{
    	LED_On(LED2);
    	MXC_Delay(500000);
    	LED_Off(LED2);
    	MXC_Delay(500000);
    	printf("\nCapturing Pic2\n");
    }
}

// *****************************************************************************
int main(void)
{
    mxc_wut_cfg_t cfg;
    uint32_t ticks;

    printf("\n\n************** Wakeup timer example ********************\n");
    printf("This example is to show how the Wakeup timer is used and configured.\n");


    // Get ticks based off of milliseconds
    MXC_WUT_GetTicks(MILLISECONDS_WUT, MXC_WUT_UNIT_MILLISEC, &ticks);

    // config structure for one shot timer to trigger in a number of ticks
    cfg.mode = MXC_WUT_MODE_CONTINUOUS;
    cfg.cmp_cnt = ticks;

    // Init WUT
    MXC_WUT_Init(MXC_WUT_PRES_1);

    //Config WUT
    MXC_WUT_Config(&cfg);
    MXC_LP_EnableWUTAlarmWakeup();

    while (1) {
        setTrigger(1);

        NVIC_EnableIRQ(WUT_IRQn);
        MXC_WUT_Enable();

#ifdef SLEEP_MODE
        printf("Entering SLEEP mode.\n");
        MXC_LP_EnterSleepMode();
        printf("Waking up from SLEEP mode.\n");

#else
        printf("Entering LPM mode.\n");
        MXC_LP_EnterLowPowerMode();
        printf("Waking up from LPM mode.\n");
#endif

        printf("This is the %d timer",timerNumber);
        printf("\nEntering another sleep-wake cycle.\n");

    }
}
