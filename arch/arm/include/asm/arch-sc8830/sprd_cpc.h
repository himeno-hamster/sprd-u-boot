#ifndef _SPRD_CPC_H_
#define _SPRD_CPC_H_

#include <common.h>
#include <asm/arch/sprd_reg_base.h>

#define PIN_CTL_BASE CHIPPIN_CTL_BEGIN
#define PIN_CTL_REG CHIPPIN_CTL_BEGIN

#define ANA_CPC_BASE (ADI_CTL_BEGAIN + 0x180)
#define ANA_PIN_CTL_BASE (ANA_CPC_BASE + 0x8C)

#endif //_SPRD_CPC_H_
