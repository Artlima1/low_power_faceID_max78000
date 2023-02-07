#define S_MODULE_NAME "MAIN-RISCV"
// #define PRINT_DEBUG
/***** Includes *****/
#include <stdint.h>
#include <stdio.h>

#include "MAXCAM_Debug.h"
#include "board.h"
#include "camera.h"
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
#include "MAXCAM_Debug.h"
#include "embedding_process.h"
#include "keypad.h"
#include "rtc.h"
#include "cnn.h"

#include "img_capture.h"
#include "faceID.h"


/***** Definitions *****/
#define OST_CLOCK_SOURCE MXC_TMR_32K_CLK // \ref mxc_tmr_clock_t
// Parameters for Continuous timer
#define OST_FREQ 10 // (Hz)
#define OST_TIMER MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5

#define COMPS_PER_BASE_PIC 100

#define PRINT_DEBUG

/***** Globals *****/
int timer_count = 0;
static int face_id_count = 0;
int key = 0;

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
	STATE_FACEID,
    NUM_STATES
}State_t;

typedef struct{
    State_t state;
    void (*state_function)(void);
} StateMachine_t;

void fn_INIT(void);
void fn_Pic1(void);
void fn_Compare(void);
void fn_FaceID(void);

State_t current_state = STATE_INIT;

StateMachine_t fsm[] = {
                      {STATE_INIT, fn_INIT},
                      {STATE_PIC1, fn_Pic1},
                      {STATE_COMPARE, fn_Compare},
                      {STATE_FACEID, fn_FaceID}
};

void fn_INIT(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State INIT\n");
	#endif
	current_state = STATE_PIC1;
}

void fn_Pic1(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State PIC1\n");
	#endif

	img_capture(IMAGE_CAPTURE_BASE);
	timer_count=0;

	current_state = STATE_COMPARE;
}

void fn_Compare(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State COMPARE: ");
	#endif
	
	uint8_t decision = img_capture(IMAGE_CAPTURE_COMPARE);

	if(decision == IMG_CAP_RET_ERROR){
		#ifdef PRINT_DEBUG
		printf("MAIN: Error in image comparison\n");
		#endif
	}
	else if(decision==IMG_CAP_RET_CHANGE){
		LED_On(LED_BLUE);
		current_state = STATE_FACEID;
	}
	else if(timer_count>=COMPS_PER_BASE_PIC){
		current_state = STATE_PIC1;
	}
}

void fn_FaceID(){
	#ifdef PRINT_DEBUG
	printf("MAIN: State CHANGE\n");
	printf("%d time face detection\n",face_id_count);
	#endif

	static faceID_decision_t faceID_decision;
	faceID_decision = faceid_run();

	face_id_count++;

	if(faceID_decision.decision >= 0 ){
		printf("Face %d detected, name %s\n", faceID_decision.decision, faceID_decision.name);
		face_id_count = 0;
		timer_count=0;
		current_state = STATE_COMPARE;
		/* TODO - Do something in case face detected*/
		LED_Off(LED_BLUE);
		LED_On(LED_GREEN);
		MXC_Delay(30000);
		LED_Off(LED_GREEN);
		change_img_size_to_compare();
	}
	else {
		if(face_id_count>3){
			printf("There's no face detected,changing back to comparing \n");
			face_id_count = 0;
			printf("i'm stuck here 1");
			timer_count=0;
			printf("i'm stuck here 2");
			current_state = STATE_COMPARE;
			printf("i'm stuck here 3");
			LED_Off(LED_BLUE);
			printf("i'm stuck here 4");
			LED_On(LED_RED);
			printf("i'm stuck here 5");
			MXC_Delay(30000);
			printf("i'm stuck here 6");
			LED_Off(LED_RED);
			printf("i'm stuck here 7");
			change_img_size_to_compare();
			printf("i'm stuck here 8");
		}
	}

}

int main(void) {

	// Wait for PMIC 1.8V to become available, about 180ms after power up.
    MXC_Delay(200000);

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC1);

    /* Initialize RTC */
    MXC_RTC_Init(0, 0);
    MXC_RTC_Start();
	
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


	__enable_irq();
	/* Enable PCIF wakeup event */
    NVIC_EnableEVENT(PCIF_IRQn);
    /* Enable wakeup timer interrupt */
    NVIC_EnableIRQ(WUT_IRQn);
    /* Enable wakeup timer event */
    NVIC_EnableEVENT(WUT_IRQn);

	img_capture_init();
	
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
