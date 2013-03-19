/******************************************************************************
 ** File Name:        pin_drv.c
 ** Author:           henry.he
 ** DATE:             11/03/2013
 ** Copyright:        2013 Spreatrum, Incoporated. All Rights Reserved.
 ** Description:
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History
 **-------------------------------------------------------------------------
 ** DATE          NAME            DESCRIPTION
 ** 11/03/2013                    Create.
 ******************************************************************************/

#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/arm_reg.h>
#include <asm/arch/sc_reg.h>
#include "asm/arch/chip_plf_export.h"
#include <asm/arch/mfp.h>


void set_cp_emc_pad(void)
{
    u32 dqs_drv = 0;
    u32 data_drv = 0;
    u32 ctl_drv = 1;
    u32 clk_drv = 0;

    u32 i = 0;

    for (i = 0; i < 2; i++) {// ckdp ckdm
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_CLKDMMEM_REG_OFFS) + i*4) &= (~0x300);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_CLKDMMEM_REG_OFFS) + i*4) |= (clk_drv<<8);
    }

    //addr
    for (i = 0; i<14; i++) {
        CHIP_REG_AND(((CHIPPIN_CTL_BEGIN + PIN_CP_EMA0_REG_OFFS) + i*4), ~0x300);
        CHIP_REG_OR(((CHIPPIN_CTL_BEGIN + PIN_CP_EMA0_REG_OFFS) + i*4), ctl_drv<<8);
    }

    for (i = 0; i < 5; i++) {//bank0 bank1 casn cke0 csn0
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMBA0_REG_OFFS) + i*4) &= (~0x300);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMBA0_REG_OFFS) + i*4) |= (ctl_drv<<8);
    }

    for (i = 0; i < 4; i++) {//dqm
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMDQM0_REG_OFFS) + i*4) &= (~0x300);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMDQM0_REG_OFFS) + i*4) |= (data_drv<<8);
    }

    for (i = 0; i < 4; i++) {//dqs
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMDQS0_REG_OFFS) + i*4) &= (~0x300);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMDQS0_REG_OFFS) + i*4) |= (dqs_drv<<8);
    }

    //data
    for (i = 0; i < 32; i++) {
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMD0_REG_OFFS) + i*4) &= (~0x300);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMD0_REG_OFFS) + i*4) |= (data_drv<<8);
    }

    for (i = 0; i < 4; i++) {//gpre_loop gpst_loop rasn wen
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMGPRE_LOOP_REG_OFFS) + i*4) &= (~0x300);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CP_EMGPRE_LOOP_REG_OFFS) + i*4) |= (ctl_drv<<8);
    }
}


void set_cp_jtag_pad(void)
{
    u32 i = 0;
    
    /*CP Jtag pin config*/
    CHIP_REG_OR(GR_GEN0, BIT_13);//pin eb
    
    CHIP_REG_SET((CHIPPIN_CTL_BEGIN + PIN_TRACEDAT3_REG_OFFS) , 0x10108);

    for (i = 0; i < 4; i++) {
        CHIP_REG_SET(((CHIPPIN_CTL_BEGIN + PIN_TRACEDAT4_REG_OFFS) + i*4), 0x10188);
    }
#if 0    
    CHIP_REG_SET(0x8C000588, 0x10108);
    CHIP_REG_SET(0x8C00058C, 0x10188);
    CHIP_REG_SET(0x8C000590, 0x10188);
    CHIP_REG_SET(0x8C000594, 0x10188);
    CHIP_REG_SET(0x8C000598, 0x10188);
#endif
}


