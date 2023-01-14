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
 * @file    main_riscv.c
 * @brief   FaceID EvKit Demo
 *
 * @details
 *
 */

#define S_MODULE_NAME "MAIN-RISCV"

/***** Includes *****/
#include <stdint.h>
#include <stdio.h>

#include "MAXCAM_Debug.h"
#include "board.h"
#include "camera.h"
#include "img_capture.h"
#include "dma.h"
#include "fcr_regs.h"
#include "icc.h"
#include "mxc.h"
#include "mxc_delay.h"
#include "mxc_sys.h"
#include "sema_regs.h"
#include "lp.h"
#include "gcfr_regs.h"


/***** Definitions *****/
#define SLEEP_MODE           // Select between SLEEP_MODE and LPM_MODE
#define OST_CLOCK_SOURCE MXC_TMR_8K_CLK // \ref mxc_tmr_clock_t
#define MILLISECONDS_WUT 5000
// Parameters for Continuous timer
#define OST_FREQ 0.2 // (Hz)
#define OST_TIMER MXC_TMR5 // Can be MXC_TMR0 through MXC_TMR5

/***** Globals *****/
int timerNumber = 1;

__attribute__((section(
    ".shared__at__mailbox"))) volatile uint32_t mail_box[ARM_MAILBOX_SIZE + RISCV_MAILBOX_SIZE];
volatile uint32_t *arm_mail_box = &mail_box[0];
volatile uint32_t *riscv_mail_box = &mail_box[ARM_MAILBOX_SIZE];

//extern int start_img_capture(void);

// *****************************************************************************
typedef enum {
    STATE_INIT,
    STATE_CAPTURING,
    STATE_ANALYSING,
    STATE_FINISH,
    NUM_STATES
}State_t;

typedef struct{
    State_t state;
    void (*state_function)(void);
} StateMachine_t;

void fn_INIT(void);
void fn_CAPTURING(void);
void fn_ANALYSING(void);
void fn_FINISH(void);

State_t current_state = STATE_INIT;

StateMachine_t fsm[] = {
                      {STATE_INIT, fn_INIT},
                      {STATE_CAPTURING, fn_CAPTURING},
                      {STATE_ANALYSING, fn_ANALYSING},
                      {STATE_FINISH, fn_FINISH}
};

void Timer_Calculation(){

    if(timerNumber < 5){
    	timerNumber+=1;
    }else{
    	timerNumber=1;
    }

}
void __attribute__((interrupt("machine")))TMR5_IRQHandler(void) {
    // Clear interrupt
    MXC_TMR_ClearFlags(OST_TIMER);

    // Clear interrupt
    if (MXC_TMR5->wkfl & MXC_F_TMR_WKFL_A) {
        MXC_TMR5->wkfl = MXC_F_TMR_WKFL_A;
        printf("Oneshot timer expired!\n");
    }
    Timer_Calculation();
    current_state = STATE_INIT;

}

void OneshotTimer(){
	for (int i = 0; i < 5000; i++) {}
	    //Button debounce

	    // Declare variables
	    mxc_tmr_cfg_t tmr;
	    uint32_t periodTicks = MXC_TMR_GetPeriod(OST_TIMER, OST_CLOCK_SOURCE, 1, OST_FREQ);

	    MXC_TMR_Shutdown(OST_TIMER);

	    tmr.pres = TMR_PRES_1;
	    tmr.mode = TMR_MODE_ONESHOT;
	    tmr.bitMode = TMR_BIT_MODE_32;
	    tmr.clock = OST_CLOCK_SOURCE;
	    //tmr.clock = MXC_SYS_PERIPH_CLOCK_CPU1;
	    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
	    tmr.pol = 0;

	    if (MXC_TMR_Init(OST_TIMER, &tmr, true) != E_NO_ERROR) {
	        printf("Failed one-shot timer Initialization.\n");
	        return;
	    }

	    MXC_TMR_EnableInt(OST_TIMER);

	    // Enable wkup source in Poower seq register
	    //MXC_LP_EnableTimerWakeup(OST_TIMER);
	    // Enable Timer wake-up source
	    MXC_TMR_EnableWakeup(OST_TIMER, &tmr);

	    printf("Oneshot timer started.\n\n");

	    MXC_TMR_Start(OST_TIMER);
}


void Transmission_Complete(int waitForTrigger)
{
    // Wait for serial transactions to complete.
    while (MXC_UART_ReadyForSleep(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {}
}



void Select_Pic(int timerNumber){
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

void fn_INIT(){
	__WFI();
	current_state = STATE_CAPTURING;
}

void fn_CAPTURING(){
	Select_Pic(timerNumber);
	current_state = STATE_ANALYSING;
}

void fn_ANALYSING(){
	printf("I'm Analyzing");
	current_state = STATE_FINISH;
}

void fn_FINISH(){
	Transmission_Complete(1);
	asm volatile("wfi");  // RISC-V sleeps and waits for command from ARM
	OneshotTimer();
}


int main(void) {

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC1);
    __enable_irq();
	NVIC_EnableIRQ(TMR5_IRQn);
	NVIC_EnableEVENT(TMR5_IRQn);

	while(1){
	    if(current_state < NUM_STATES){
	        (*fsm[current_state].state_function)();
	    }
	    else{
	        /* serious error */
	    }

	}

    return 0;
}
