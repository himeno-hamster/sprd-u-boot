#ifndef __WDT_MONITOR_H__
#define __WDT_MONITOR_H__
#include <asm/arch/sci_types.h>
#include <command.h>
#include <common.h>

#define WDT_MON_BASE 			 (0xa0008000)

#define WDT_MON_R13          	 (WDT_MON_BASE+0x0200)
#define WDT_MON_R14          	 (WDT_MON_BASE+0x0204)
#define WDT_MON_R13_FIQ      	 (WDT_MON_BASE+0x0208)
#define WDT_MON_R14_FIQ      	 (WDT_MON_BASE+0x020C)
#define WDT_MON_R13_IRQ      	 (WDT_MON_BASE+0x0210)
#define WDT_MON_R14_IRQ      	 (WDT_MON_BASE+0x0214)
#define WDT_MON_R13_SVC      	 (WDT_MON_BASE+0x0218)
#define WDT_MON_R14_SVC      	 (WDT_MON_BASE+0x021C)
#define WDT_MON_R13_UND      	 (WDT_MON_BASE+0x0220)
#define WDT_MON_R14_UND      	 (WDT_MON_BASE+0x0224)
#define WDT_MON_R13_ABT      	 (WDT_MON_BASE+0x0228)
#define WDT_MON_R14_ABT      	 (WDT_MON_BASE+0x022C)
#define WDT_MON_R13_MON      	 (WDT_MON_BASE+0x0230)
#define WDT_MON_R14_MON      	 (WDT_MON_BASE+0x0234)
#define WDT_MON_PC           	 (WDT_MON_BASE+0x0238)
#define WDT_MON_CPSR         	 (WDT_MON_BASE+0x023C)
#define WDT_MON_SPSR_FIQ     	 (WDT_MON_BASE+0x0240)
#define WDT_MON_SPSR_IRQ     	 (WDT_MON_BASE+0x0244)
#define WDT_MON_SPSR_SVC     	 (WDT_MON_BASE+0x0248)
#define WDT_MON_SPSR_UND     	 (WDT_MON_BASE+0x024C)
#define WDT_MON_SPSR_ABT     	 (WDT_MON_BASE+0x0250)
#define WDT_MON_SPSR_MON     	 (WDT_MON_BASE+0x0254)
#define WDT_MON_R0           	 (WDT_MON_BASE+0x0258)
#define WDT_MON_R1           	 (WDT_MON_BASE+0x025C)
#define WDT_MON_R2           	 (WDT_MON_BASE+0x0260)
#define WDT_MON_R3           	 (WDT_MON_BASE+0x0264)
#define WDT_MON_R4           	 (WDT_MON_BASE+0x0268)
#define WDT_MON_R5           	 (WDT_MON_BASE+0x026C)
#define WDT_MON_R6           	 (WDT_MON_BASE+0x0270)
#define WDT_MON_R7           	 (WDT_MON_BASE+0x0274)
#define WDT_MON_R8           	 (WDT_MON_BASE+0x0278)
#define WDT_MON_R9           	 (WDT_MON_BASE+0x027C)
#define WDT_MON_R10          	 (WDT_MON_BASE+0x0280)
#define WDT_MON_R11          	 (WDT_MON_BASE+0x0284)
#define WDT_MON_R12          	 (WDT_MON_BASE+0x0288)
#define WDT_MON_R8_FIQ       	 (WDT_MON_BASE+0x028C)
#define WDT_MON_R9_FIQ       	 (WDT_MON_BASE+0x0290)
#define WDT_MON_R10_FIQ      	 (WDT_MON_BASE+0x0294)
#define WDT_MON_R11_FIQ      	 (WDT_MON_BASE+0x0298)
#define WDT_MON_R12_FIQ      	 (WDT_MON_BASE+0x029C)

#define WDT_MON_END WDT_MON_R12_FIQ

/*register name*/
#define WDT_MON_R13_NAME          	 "R13"
#define WDT_MON_R14_NAME          	 "R14"
#define WDT_MON_R13_FIQ_NAME      	 "R13_FIQ"
#define WDT_MON_R14_FIQ_NAME      	 "R14_FIQ"
#define WDT_MON_R13_IRQ_NAME      	 "R13_IRQ"
#define WDT_MON_R14_IRQ_NAME      	 "R14_IRQ"
#define WDT_MON_R13_SVC_NAME      	 "R13_SVC"
#define WDT_MON_R14_SVC_NAME      	 "R14_SVC"
#define WDT_MON_R13_UND_NAME      	 "R13_UND"
#define WDT_MON_R14_UND_NAME      	 "R14_UND"
#define WDT_MON_R13_ABT_NAME      	 "R13_ABT"
#define WDT_MON_R14_ABT_NAME     	 "R14_ABT"
#define WDT_MON_R13_MON_NAME      	 "R13_MON"
#define WDT_MON_R14_MON_NAME      	 "R14_MON"
#define WDT_MON_PC_NAME           	 "PC"
#define WDT_MON_CPSR_NAME         	 "CPSR"
#define WDT_MON_SPSR_FIQ_NAME     	 "SPSR_FIQ"
#define WDT_MON_SPSR_IRQ_NAME     	 "SPSR_IRQ"
#define WDT_MON_SPSR_SVC_NAME     	 "SPSR_SVC"
#define WDT_MON_SPSR_UND_NAME     	 "SPSR_UND"
#define WDT_MON_SPSR_ABT_NAME     	 "SPSR_ABT"
#define WDT_MON_SPSR_MON_NAME     	 "SPSR_MON"
#define WDT_MON_R0_NAME           	 "R0"
#define WDT_MON_R1_NAME           	 "R1"
#define WDT_MON_R2_NAME           	 "R2"
#define WDT_MON_R3_NAME           	 "R3"
#define WDT_MON_R4_NAME           	 "R4"
#define WDT_MON_R5_NAME           	 "R5"
#define WDT_MON_R6_NAME           	 "R6"
#define WDT_MON_R7_NAME           	 "R7"
#define WDT_MON_R8_NAME           	 "R8"
#define WDT_MON_R9_NAME           	 "R9"
#define WDT_MON_R10_NAME          	 "R10"
#define WDT_MON_R11_NAME          	 "R11"
#define WDT_MON_R12_NAME          	 "R12"
#define WDT_MON_R8_FIQ_NAME       	 "R8_FIQ"
#define WDT_MON_R9_FIQ_NAME       	 "R9_FIQ"
#define WDT_MON_R10_FIQ_NAME      	 "R10_FIQ"
#define WDT_MON_R11_FIQ_NAME      	 "R11_FIQ"
#define WDT_MON_R12_FIQ_NAME         "R12_FIQ"


struct wdt_monitor_struct{
	uint32 addr;
	char *name;
};

#define WDT_MONITOR_SET(ADDR) \
{\
	.addr = ADDR, \
	.name = ADDR##_NAME, \
\
}

#endif
