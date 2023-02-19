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
#include "mxc_delay.h"
#include "esp32.h"
#include "faceID.h"
#include "embedding_process.h"

#define S_MODULE_NAME "MAIN-RISCV"
#define PRINT_DEBUG

/***** Definitions *****/
#define OST_CLOCK_SOURCE MXC_TMR_32K_CLK // \ref mxc_tmr_clock_t
// Parameters for Continuous timer
#define OST_FREQ 1	 // (Hz)
#define OST_TIMER MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5

#define COMPS_PER_BASE_PIC 1000

/***** Globals *****/
int timer_count = 0;

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
	STATE_FACEID,
	STATE_RECOGNIZED,
    NUM_STATES
}State_t;

typedef struct{
    State_t state;
    void (*state_function)(void);
} StateMachine_t;

void fn_Init(void);
void fn_Pic1(void);
void fn_Compare(void);
void fn_FaceID(void);
void fn_Recgnized(void);

State_t current_state = STATE_INIT;

StateMachine_t fsm[] = {
                      {STATE_INIT, fn_Init},
                      {STATE_PIC1, fn_Pic1},
                      {STATE_COMPARE, fn_Compare},
                      {STATE_FACEID, fn_FaceID},
					  {STATE_RECOGNIZED, fn_Recgnized}
};

void fn_Init(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State INIT\n");
	#endif

	esp32_init();

	if(img_capture_init() != IMG_CAP_RET_SUCCESS){
		printf("Could not initialize the image capture\n");
		while(1);
	}

	/* Camera need some exposition time after init */
	MXC_Delay(SEC(2));

	current_state = STATE_PIC1;
}

void fn_Pic1(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State PIC1\n");
	#endif

	if(img_capture(IMAGE_CAPTURE_BASE) != IMG_CAP_RET_SUCCESS){
		printf("Could not initialize the image capture\n");
		return;
	}
	timer_count=0;

	current_state = STATE_COMPARE;
	LED_On(LED_RED);
	LED_Off(LED_GREEN);
	LED_Off(LED_BLUE);
}

void fn_Compare(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State COMPARE\n");
	#endif
	
	uint8_t decision = img_capture(IMAGE_CAPTURE_COMPARE);

	if(decision == IMG_CAP_RET_ERROR){
		#ifdef PRINT_DEBUG
		printf("MAIN: Error in image comparison\n");
		#endif
	}
	else if(decision==IMG_CAP_RET_CHANGE){
		current_state = STATE_FACEID;
		LED_On(LED_BLUE);
		LED_Off(LED_GREEN);
		LED_Off(LED_RED);
	}
	else if(timer_count>=COMPS_PER_BASE_PIC){
		current_state = STATE_PIC1;
		LED_Off(LED_BLUE);
		LED_Off(LED_GREEN);
		LED_Off(LED_RED);
	}
}

void fn_FaceID(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State FACEID\n");
	#endif

	if(img_capture_free_base() != IMG_CAP_RET_SUCCESS){
		printf("Could Not store base!!\n");
		while(1){}
	}

	faceID_decision_t result = faceid_run();

	if(result.decision >= 0){
		printf("%s recognized!\n", result.name);
		current_state = STATE_RECOGNIZED;
		LED_On(LED_GREEN);
		LED_Off(LED_BLUE);
		LED_Off(LED_RED);
	}
	else {
		if(img_capture_rec_base() == IMG_CAP_RET_SUCCESS){
			timer_count=0;
			current_state = STATE_COMPARE;
			LED_Off(LED_BLUE);
			LED_Off(LED_GREEN);
			LED_On(LED_RED);
		}
		else {
			printf("Could not recuperate base\n");
			current_state = STATE_PIC1;
			LED_Off(LED_BLUE);
			LED_Off(LED_GREEN);
			LED_Off(LED_RED);
		}
	}

}

void fn_Recgnized(){
	#ifdef PRINT_DEBUG
	printf("MAIN: Face RECOGNIZED\n");
	#endif

	img_capture_send_img();
	MXC_Delay(SEC(3));

	if(img_capture_rec_base() == IMG_CAP_RET_SUCCESS){
		timer_count=0;
		current_state = STATE_COMPARE;
		LED_Off(LED_BLUE);
		LED_Off(LED_GREEN);
		LED_On(LED_RED);
	}
	else {
		printf("Could not recuperate base\n");
		current_state = STATE_PIC1;
		LED_Off(LED_BLUE);
		LED_Off(LED_GREEN);
		LED_Off(LED_RED);
	}
}

int main(void) {

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
		#ifdef PRINT_DEBUG
		printf("MAIN: Failed one-shot timer Initialization.\n");
		#endif
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
	
	#ifdef PRINT_DEBUG
	printf("Setup completed!\n");
	#endif
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
