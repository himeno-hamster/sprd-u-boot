/******************************************************************************
 ** File Name:      chip_phy_v3.c                                             *
 ** Author:         Richard Yang                                              *
 ** DATE:           08/14/2002                                                *
 ** Copyright:      2002 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic information on chip.          *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 08/14/2002     Richard.Yang     Create.                                   *
 ** 09/16/2003     Xueliang.Wang    Modify CR4013                             *
 ** 08/23/2004     JImmy.Jia        Modify for SC6600D                        *
 ******************************************************************************/

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include <asm/io.h>
#include "asm/arch/sc_reg.h"
#include "asm/arch/adi_hal_internal.h"
#include "asm/arch/wdg_drvapi.h"
#include "asm/arch/sprd_reg.h"
#include "asm/arch/boot_drvapi.h"
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
/*****************************************************************************/
// Description :    This function is used to reset MCU.
// Global resource dependence :
// Author :         Xueliang.Wang
// Note :
/*****************************************************************************/
void CHIP_ResetMCU (void)  //reset interrupt disable??
{
    // This loop is very important to let the reset process work well on V3 board
    // @Richard
    uint32 i = 10000;

	WDG_ClockOn ();
    WDG_TimerInit ();
    
    while (i--);    

    WDG_ResetMCU ();
    
    {
        volatile uint32 tick1 = SCI_GetTickCount();
        volatile uint32 tick2 = SCI_GetTickCount();

        while ( (tick2 - tick1) < 500)
        {
            tick2 = SCI_GetTickCount();
        }
    }
}

/*****************************************************************************/
//  Description:    Returns the HW_RST register address.
//  Author:         Jeff.Li
//  Note :          Because there is no register which can restore information
//                  when watchdog resets the system, so we choose IRAM.
/*****************************************************************************/
LOCAL uint32 CHIP_PHY_GetHwRstAddr (void)
{
    // Returns a DWORD of IRAM shared with DCAM
    return ANA_REG_GLB_WDG_RST_MONITOR;
}

/*****************************************************************************/
//  Description:    Returns the reset mode register address.
//  Author:         Jeff.Li
//  Note:
/*****************************************************************************/
LOCAL uint32 CHIP_PHY_GetRstModeAddr (void)
{
    return ANA_REG_GLB_POR_RST_MONITOR;
}

/*****************************************************************************/
//  Description:    Gets the register in analog die to judge the reset mode. 
//  Author:         Jeff.Li
//  Note:           !It is called before __main now, so it can not call the adi
//                  interface because it contains SCI_DisableIRQ inside, below 
//                  writes the adi read interface individually. Because the la-
//                  ckless of SCI_DisableIRQ, so this function must be called 
//                  before system interrupt is turnned on!
/*****************************************************************************/
LOCAL uint32 CHIP_PHY_GetANAReg (void)
{
    return ANA_REG_GET(ANA_REG_GLB_POR_RST_MONITOR);
}

/*****************************************************************************/
//  Description:    This fuction returns the HW_RST value written before reset.
//  Author:         Jeff.Li
//  Note:           
/*****************************************************************************/
LOCAL uint32 CHIP_PHY_GetHWFlag (void)
{
    // Switch IRAM from DCAM to ARM
    return ANA_REG_GET (CHIP_PHY_GetHwRstAddr ());
}

/*****************************************************************************/
//  Description:    PHY layer realization of BOOT_SetRstMode.
//  Author:         Jeff.Li
//  Note:           The valid bit filed is from bit15 to bit0
/*****************************************************************************/
PUBLIC void CHIP_PHY_SetRstMode (uint32 val)
{
    ANA_REG_AND (CHIP_PHY_GetRstModeAddr (), ~0xFFFF);
    ANA_REG_OR (CHIP_PHY_GetRstModeAddr (), (val&0xFFFF));
}

/*****************************************************************************/
//  Description:    This fuction returns the reset mode value.
//  Author:         Jeff.Li
//  Note:
/*****************************************************************************/
PUBLIC uint32 CHIP_PHY_GetRstMode (void)
{
    return (ANA_REG_GET (CHIP_PHY_GetRstModeAddr ()) & 0xFFFF);
}

/*****************************************************************************/
//  Description:    PHY layer realization of BOOT_ResetHWFlag. It resets the HW
//                  reset register after system initialization.
//  Author:         Jeff.Li
//  Note:           The valid bit filed of analog register is from bit11 to bit0.
//                  | 11   10   9   8 |  7   6   5   4  |  3   2   1   0   |
//                  |ALL_HRST_MONITOR | POR_HRST_MONITOR| WDG_HRST_MONITOR |
//
//                  The valid bit filed of HW_RST is from bit11 to bit0.
/*****************************************************************************/
PUBLIC void CHIP_PHY_ResetHWFlag (uint32 val)
{
    // Reset the analog die register
    ANA_REG_AND(ANA_REG_GLB_POR_RST_MONITOR, ~0xFFF);
    ANA_REG_OR (ANA_REG_GLB_POR_RST_MONITOR, (val&0xFFF));

    // Reset the HW_RST
    ANA_REG_AND(CHIP_PHY_GetHwRstAddr (), ~0xFFFF);
    ANA_REG_OR (CHIP_PHY_GetHwRstAddr (), (val&0xFFFF));
}

/*****************************************************************************/
//  Description:    PHY layer realization of BOOT_SetWDGHWFlag. It Writes flag
//                  to the register which would not be reset by watchdog reset.
//  Author:         Jeff.Li
//  Note:           The valid bit filed is from bit15 to bit0
/*****************************************************************************/
PUBLIC void CHIP_PHY_SetWDGHWFlag (WDG_HW_FLAG_T type, uint32 val)
{
    if(TYPE_RESET == type)
    {        
        ANA_REG_AND(CHIP_PHY_GetHwRstAddr (), ~0xFFFF);
        ANA_REG_OR (CHIP_PHY_GetHwRstAddr (), (val&0xFFFF));
    }
    else
    {
        //wrong type, TODO
    }
}


/*****************************************************************************/
//  Description:    PHY layer realization of __BOOT_IRAM_EN.
//  Author:         Jeff.Li
//  Note:           Do nothing. There are 32KB internal ram dedicated for ARM.
/*****************************************************************************/
PUBLIC void CHIP_PHY_BootIramEn ()
{
}

/*****************************************************************************/
// Description :    This function returns whether the watchdog reset is caused
//                  by software reset or system halted.
// Author :         Jeff.Li
// Note :           The valid bit filed is from bit15 to bit0
/*****************************************************************************/
PUBLIC BOOLEAN CHIP_PHY_IsWDGRstByMCU (uint32 flag)
{
    // Copy the value of HW_RST register to the register specific to reset mode
    ANA_REG_SET (CHIP_PHY_GetRstModeAddr (),
                  (CHIP_PHY_GetHWFlag () & 0xFFFF));

    if ((CHIP_PHY_GetHWFlag () & 0xFFFF) == (flag & 0xFFFF))
    {
        return SCI_FALSE;
    }
    else
    {
        return SCI_TRUE;
    }
}

/*****************************************************************************/
// Description :    This function returns whether the reset is caused by power
//                  up.
// Author :         Jeff.Li
// Note :           | 11   10   9   8 |  7   6   5   4  |  3   2   1   0   |
//                  |ALL_HRST_MONITOR | POR_HRST_MONITOR| WDG_HRST_MONITOR |
/*****************************************************************************/
PUBLIC BOOLEAN CHIP_PHY_IsResetByPowerUp()
{
    if ((CHIP_PHY_GetANAReg () & 0xF0) == 0x0)
    {
        return SCI_TRUE;
    }
    else
    {
        return SCI_FALSE;
    }
}

/*****************************************************************************/
// Description :    This function returns whether the reset is caused by watch-
//                  dog reset.
// Author :         Jeff.Li
// Note :           | 11   10   9   8 |  7   6   5   4  |  3   2   1   0   |
//                  |ALL_HRST_MONITOR | POR_HRST_MONITOR| WDG_HRST_MONITOR |
/*****************************************************************************/
PUBLIC BOOLEAN CHIP_PHY_IsResetByWatchDog()
{
    if ((CHIP_PHY_GetANAReg () & 0xF) == 0x0)
    {
        return SCI_TRUE;
    }
    else
    {
        return SCI_FALSE;
    }
}

/************************************************************
*select TDPLL's reference crystal,
*(1)--RF0---------xtlbuf0-----------
*                               -?-tdpll_ref_sel-----TDPLL
*(2)--RF1---------xtlbuf1-----------
1)rf_id = 0,TDPLL will select (1), or select (2)
************************************************************/
PUBLIC uint32 TDPllRefConfig(TDPLL_REF_T rf_id)
{
    uint32 pll_reg;
/* before switch reference crystal, it must be sure that no module is using TDPLL */
    pll_reg = readl(REG_AP_CLK_AP_AHB_CFG);
    pll_reg &= ~AP_AHB_CLK_SEL_MASK;
    writel(pll_reg, REG_AP_CLK_AP_AHB_CFG);

    pll_reg = readl(REG_AON_CLK_PUB_AHB_CFG);
    pll_reg &= ~PUB_AHB_CLK_SEL_MASK;
    writel(pll_reg, REG_AON_CLK_PUB_AHB_CFG);

    pll_reg = readl(REG_AP_CLK_AP_APB_CFG);
    pll_reg &= ~AP_APB_CLK_SEL_MASK;
    writel(pll_reg, REG_AP_CLK_AP_APB_CFG);

    pll_reg = readl(REG_AON_CLK_AON_APB_CFG);
    pll_reg &= ~PUB_APB_CLK_SEL_MASK;
    writel(pll_reg, REG_AON_CLK_AON_APB_CFG);

    pll_reg = readl(REG_AON_APB_PLL_SOFT_CNT_DONE);
    pll_reg &= ~(BIT_TDPLL_SOFT_CNT_DONE);
    writel(pll_reg, REG_AON_APB_PLL_SOFT_CNT_DONE);
    udelay(1);

/* switch TDPLL reference crystal */
    if (rf_id == TDPLL_REF0)
    {
        pll_reg = readl(REG_PMU_APB_TDPLL_REL_CFG);
        pll_reg &= ~(0x1 << 4);
        writel(pll_reg, REG_PMU_APB_TDPLL_REL_CFG);

        pll_reg = readl(REG_PMU_APB_XTL0_REL_CFG);
        pll_reg |= BIT_XTL1_AP_SEL;
        writel(pll_reg, REG_PMU_APB_XTL0_REL_CFG);

        pll_reg = readl(REG_PMU_APB_XTLBUF0_REL_CFG);
        pll_reg |= BIT_XTLBUF1_AP_SEL;
        writel(pll_reg, REG_PMU_APB_XTLBUF0_REL_CFG);
    }
    else if(rf_id == TDPLL_REF1)
    {
        pll_reg = readl(REG_PMU_APB_TDPLL_REL_CFG);
        pll_reg |= (0x1 << 4);
        writel(pll_reg, REG_PMU_APB_TDPLL_REL_CFG);

        pll_reg = readl(REG_PMU_APB_XTL1_REL_CFG);
        pll_reg |= BIT_XTL1_AP_SEL;
        writel(pll_reg, REG_PMU_APB_XTL1_REL_CFG);

        pll_reg = readl(REG_PMU_APB_XTLBUF1_REL_CFG);
        pll_reg |= BIT_XTLBUF1_AP_SEL;
        writel(pll_reg, REG_PMU_APB_XTLBUF1_REL_CFG);
    }
    else
        return 1;

    pll_reg = readl(REG_AON_APB_PLL_SOFT_CNT_DONE);
    pll_reg |= (BIT_TDPLL_SOFT_CNT_DONE);
    writel(pll_reg, REG_AON_APB_PLL_SOFT_CNT_DONE);

    udelay(120);

/* after switch, up ahb clock to 128M, APB to 64M */
    pll_reg = readl(REG_AP_CLK_AP_AHB_CFG);
    pll_reg |= 0x3;
    writel(pll_reg, REG_AP_CLK_AP_AHB_CFG);

    pll_reg = readl(REG_AON_CLK_PUB_AHB_CFG);
    pll_reg |= 0x3;
    writel(pll_reg, REG_AON_CLK_PUB_AHB_CFG);
    
    pll_reg = readl(REG_AP_CLK_AP_APB_CFG);
    pll_reg |= 0x1;
    writel(pll_reg, REG_AP_CLK_AP_APB_CFG);

    pll_reg = readl(REG_AON_CLK_AON_APB_CFG);
    pll_reg |= 0x3;
    writel(pll_reg, REG_AON_CLK_AON_APB_CFG);
    return 0;
}
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
