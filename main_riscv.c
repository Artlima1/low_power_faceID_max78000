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
/*
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
#include "led.h"

/***** Definitions *****/
#define OST_CLOCK_SOURCE MXC_TMR_32K_CLK // \ref mxc_tmr_clock_t
// Parameters for Continuous timer
#define OST_FREQ 10 // (Hz)
#define OST_TIMER MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5

#define COMPS_PER_BASE_PIC 100

/***** Globals *****/
int timer_count = 0;
static int blink_count = 0;

__attribute__((section(
    ".shared__at__mailbox"))) volatile uint32_t mail_box[ARM_MAILBOX_SIZE + RISCV_MAILBOX_SIZE];
volatile uint32_t *arm_mail_box = &mail_box[0];
volatile uint32_t *riscv_mail_box = &mail_box[ARM_MAILBOX_SIZE];

void __attribute__((interrupt("machine")))TMR1_IRQHandler(void) {
    // Clear interrupt
    MXC_TMR_ClearFlags(OST_TIMER);
	NVIC_ClearPendingIRQ(TMR1_IRQn);

	int i;
	for(i=0; i<0xFFFF; i++){
		__NOP();
	}
    timer_count+=1;

}

void __attribute__((interrupt("machine"))) WUT_IRQHandler(void)
{
    MXC_WUT_IntClear();
    NVIC_ClearPendingIRQ(WUT_IRQn);
    NVIC_ClearPendingEVENT(WUT_IRQn);
}

// ******************************************************************************
typedef enum {
    STATE_INIT,
    STATE_PIC1,
    STATE_COMPARE,
	STATE_CHANGE,
    NUM_STATES
}State_t;

typedef struct{
    State_t state;
    void (*state_function)(void);
} StateMachine_t;

void fn_INIT(void);
void fn_Pic1(void);
void fn_Compare(void);
void fn_Change(void);

State_t current_state = STATE_INIT;

StateMachine_t fsm[] = {
                      {STATE_INIT, fn_INIT},
                      {STATE_PIC1, fn_Pic1},
                      {STATE_COMPARE, fn_Compare},
                      {STATE_CHANGE, fn_Change}
};

void fn_INIT(){
	// printf("MAIN: State INIT\n");
	
	current_state = STATE_PIC1;
}

void fn_Pic1(){
	printf("MAIN: State PIC1\n");

	img_capture(IMAGE_CAPTURE_BASE);
	timer_count=0;

	current_state = STATE_COMPARE;
}

void fn_Compare(){
	printf("MAIN: State COMPARE: ");
	
	uint8_t decision = img_capture(IMAGE_CAPTURE_COMPARE);

	if(decision == IMG_CAP_RET_ERROR){
		printf("MAIN: Error in image comparison\n");
	}
	else if(decision==IMG_CAP_RET_CHANGE){
		current_state = STATE_CHANGE;
	}
	else if(timer_count>=COMPS_PER_BASE_PIC){
		current_state = STATE_PIC1;
	}
}

void fn_Change(){
	printf("MAIN: State CHANGE\n");

	blink_count++;
	if(blink_count>20){
		blink_count = 0;
		timer_count=0;
		current_state = STATE_PIC1;
		LED_Off(LED2);
	}
	else{
		LED_Toggle(LED2);
	}

}

int main(void) {

	LED_Off(LED2);

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC1);
	
	// Config Timer
	mxc_tmr_cfg_t tmr;
	uint32_t periodTicks = MXC_TMR_GetPeriod(OST_TIMER, OST_CLOCK_SOURCE, 32, OST_FREQ);
	MXC_TMR_Shutdown(OST_TIMER);
	tmr.pres = TMR_PRES_32;
	tmr.mode = TMR_MODE_ONESHOT;
	tmr.bitMode = TMR_BIT_MODE_32;
	tmr.clock = OST_CLOCK_SOURCE;
	tmr.cmp_cnt = periodTicks;
	tmr.pol = 0;
	if (MXC_TMR_Init(OST_TIMER, &tmr, false) != E_NO_ERROR) {
		printf("MAIN: Failed one-shot timer Initialization.\n");
	}
	MXC_TMR_EnableInt(OST_TIMER);
	MXC_TMR_EnableWakeup(OST_TIMER, &tmr);
	NVIC_EnableIRQ(TMR1_IRQn);

	/* Enable PCIF wakeup event */
    NVIC_EnableEVENT(PCIF_IRQn);
    /* Enable wakeup timer interrupt */
    NVIC_EnableIRQ(WUT_IRQn);
    /* Enable wakeup timer event */
    NVIC_EnableEVENT(WUT_IRQn);

	__enable_irq();

	img_capture_init();

	MXC_TMR_Start(OST_TIMER);
	
	while(1){
		asm volatile("wfi");
		if(current_state < NUM_STATES){
			(*fsm[current_state].state_function)();
			MXC_TMR_Start(OST_TIMER);
		}
		else{
			/* serious error */
		}
	}

    return 0;
}
