#include <stdlib.h>
#include <stdint.h>
#include "mxc.h"
#include "gcfr_regs.h"
#include "fcr_regs.h"
#include "sema_regs.h"

/* 
    Basically, ARM doesn't do anything besides allowing RiscV to run. There's a 10s delay function during startup
    that allows the debugger to interrupt, so that it can flash a new code.
*/

extern volatile void const* __FlashStart_; // Defined in linker file

void WakeISR(void)
{
    MXC_SEMA->irq0 = MXC_F_SEMA_IRQ0_EN & ~MXC_F_SEMA_IRQ0_CM4_IRQ;
}

int main(void)
{
    MXC_ICC_Enable(MXC_ICC0); // Enable cache

    // Switch to 100 MHz clock
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    MXC_FCR->urvbootaddr = (uint32_t)&__FlashStart_; // Set RISC-V boot address
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_SMPHR); // Enable Sempahore clock
    MXC_NVIC_SetVector(RISCV_IRQn, WakeISR);         // Set wakeup ISR

    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CPU1); // Enable RISC-V clock

    // DO NOT DELETE THIS LINE:
    MXC_Delay(SEC(10)); // Let debugger interrupt if needed

    __WFI(); // Let RISC-V run

    return 0;
}