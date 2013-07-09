/******************************************************************************
 ** File Name:      msic.c                                             *
 ** Author:         Yong.Li                                              *
 ** DATE:           04/19/2013                                                *
 ** Copyright:      2002 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic information on chip.          *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 04/19/2013     Richard.Yang     Create.                                   *
 ******************************************************************************/

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/

#include <asm/arch/sci_types.h>

#include <common.h>
#include <asm/io.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_ana.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sc8810_reg_ahb.h>
#include <asm/arch/sdram_sc7710g2.h>
#include <asm/arch/chip_plf_export.h>
#include <asm/arch/emc_config.h>
#include <asm/arch/misc_api.h>
#include <asm/arch/mfp.h>

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
 **                         Macro defines.
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Struct defines.
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Global variables                                  *
 **---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
 **                         Function Definitions                              *
 **---------------------------------------------------------------------------*/

unsigned int CHIP_PHY_GetChipID(void)
{
#if defined(CONFIG_SC7710G2)
	return CHIP_REG_GET(CHIP_ID);
#else
    return 0;
#endif
}

unsigned int CHIP_PHY_GetANAChipID(void)
{
#if defined(CONFIG_SC7710G2)
	return ((ADI_Analogdie_reg_read(ANA_CHIP_ID_HIGH) << 16) |
					ADI_Analogdie_reg_read(ANA_CHIP_ID_LOW));
#else
    return 0;
#endif
}

LOCAL void mcu_clock_select(MCU_CLK_SOURCE_E mcu_clk_sel)
{
    uint32 i;

    i = REG32(AHB_ARM_CLK);
    i &= ~(0x3 << 23);
    i |= ((mcu_clk_sel & 0x3) << 23);
    REG32(AHB_ARM_CLK) = i;
}

LOCAL void set_arm_bus_clk_div(uint32 arm_drv, uint32 axi_div, uint32 ahb_div, uint32 dbg_div)
{
    uint32 i;

    // A5 AXI DIV
    i = REG32(CA5_CFG);
    i &= (~(0x3 << 12));
    i |= ((axi_div & 0x3) << 11);
    REG32(CA5_CFG) = i;

    i = REG32(AHB_ARM_CLK);
    i &= (~(0x7 | (0x7 << 4) | (0x7 << 14)));
    i |= (arm_drv & 0x7) | ((ahb_div & 0x7) << 4) | ((dbg_div & 0x7) << 14);

    REG32(AHB_ARM_CLK) = i;

    for(i = 0; i < 50; i++);
}

LOCAL void set_gpu_clock_freq(void)
{
    // GPU AXI 256M
    REG32(GR_GEN2) &= ~(0x3);
}

LOCAL void set_mpll_clock_freq(uint32 clk_freq_hz)
{
    uint32 i;
    uint32 mpll_clk;

    mpll_clk = (clk_freq_hz / 1000000 / 4);

    //APB_GEN1_PCLK M_PLL_CTRL_WE
    REG32(GR_GEN1) |= (1 << 9);

    i = REG32(GR_MPLL_MN);
    i &= ~ 0x7FF;

	i |= (mpll_clk & 0x7FF);

    REG32(GR_MPLL_MN) = i;
    REG32(GR_GEN1) &= ~(1 << 9);
}

LOCAL void set_chip_clock_freq(void)
{
    uint32 arm_drv = 0;
    uint32 axi_div = 0;
    uint32 ahb_div = 0;
    uint32 dbg_div = 0;

    uint32 mpll_clk_freq = EMC_GetPara()->arm_clk;

    if (mpll_clk_freq == CHIP_CLK_26MHZ)
    {
        mcu_clock_select(MCU_CLK_XTL_SOURCE);
        return;
    }
    else if ((mpll_clk_freq >= CHIP_CLK_800MHZ) && (mpll_clk_freq <= CHIP_CLK_1200MHZ))
    {
        axi_div = 1;
        ahb_div = 3; // 1/4
        dbg_div = 7; // 1/8
    }
    else if ((mpll_clk_freq > CHIP_CLK_1200MHZ) && (mpll_clk_freq <= CHIP_CLK_1500MHZ))
    {
        axi_div = 3; // 1/4
        ahb_div = 7; // 1/8
        dbg_div = 7; // 1/8
    }
    else
    {
        SCI_ASSERT(0);
    }

    mcu_clock_select(MCU_CLK_XTL_SOURCE);

    set_gpu_clock_freq();
    set_arm_bus_clk_div(arm_drv, axi_div, ahb_div, dbg_div);
    set_mpll_clock_freq(mpll_clk_freq);

    mcu_clock_select(MCU_CLK_MPLL_SOURCE);
}

__inline LOCAL void set_XOSC32K_config(void)
{
#if defined(CONFIG_SC7710G2)
// from 13.8uA to 7.8uA
    uint16 reg_read;

    reg_read = ADI_Analogdie_reg_read(ANA_RTC_CTRL);

    reg_read &= (~0xFF);
    reg_read |= 0x95;

    ADI_Analogdie_reg_write(ANA_RTC_CTRL, reg_read);
#endif
}

__inline LOCAL void set_mem_volt(void)
{
#if defined(CONFIG_SC7710G2)

    uint16 reg_read;

    reg_read = ADI_Analogdie_reg_read(ANA_DCDC_MEM_CTL0);

    reg_read &= (~7);
    reg_read |= 0x6; // dcdc mem 1.8V

    ADI_Analogdie_reg_write(ANA_DCDC_MEM_CTL0, reg_read);

    reg_read = ADI_Analogdie_reg_read(ANA_LDO_TRIM9);

    reg_read &= (~0x1F);

    ADI_Analogdie_reg_write(ANA_LDO_TRIM9, reg_read);
#endif
}


__inline LOCAL void Chip_Workaround(void)
{
#if defined(CONFIG_SC7710G2)
	/* FIXME: disable otp for a-die internal bug */
	ANA_REG_OR(ANA_MIXED_CTRL, BIT_1/*BIT_OTP_EN_RST*/);
	ANA_REG_OR(ANA_DCDC_OPT_CTL, BIT_0/*BIT_DCDC_OTP_PD*/);

	/* FIXME: enable dcdc wpa current limit
	 * in order to prevent vbat drop when high load
	 */
	ANA_REG_OR(ANA_WPA_DCDC_AP_CTL2, BIT_6/*BIT_WPA_DCDC_CL_CTRL_AP*/);
#endif
}

PUBLIC void Chip_Init(void)
{
	Chip_Workaround();

    if (CHIP_PHY_GetANAChipID() == ANA_CHIP_ID_AA)
    {
        set_mem_volt();
        set_XOSC32K_config();
    }

    set_chip_clock_freq();

    sdram_init();

    return;
}


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
