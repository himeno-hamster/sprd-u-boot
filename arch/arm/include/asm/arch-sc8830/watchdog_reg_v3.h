/******************************************************************************
 ** File Name:    watchdog_reg_v3.h                                            *
 ** Author:       mingwei.zhang                                                 *
 ** DATE:         06/11/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/11/2010    mingwei.zhang   Create.                                     *
 ******************************************************************************/
#ifndef _WATCHDOG_REG_V3_H_
#define _WATCHDOG_REG_V3_H_
/*----------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **------------------------------------------------------------------------- */

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **--------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/**---------------------------------------------------------------------------*
**                               Micro Define                                **
**---------------------------------------------------------------------------*/
/*----------Watchdog Timer Counter Register----------*/
#include "sprd_reg.h"

#define WDG_BASE            (SPRD_ANA_WDG_PHYS)
#define WDG_LOAD_LOW        (WDG_BASE + 0x00)
#define WDG_LOAD_HIGH       (WDG_BASE + 0x04)
#define WDG_CTRL            (WDG_BASE + 0x08)
#define WDG_INT_CLR         (WDG_BASE + 0x0C)
#define WDG_INT_RAW         (WDG_BASE + 0x10)
#define WDG_INT_MSK         (WDG_BASE + 0x14)
#define WDG_CNT_LOW         (WDG_BASE + 0x18)
#define WDG_CNT_HIGH        (WDG_BASE + 0x1C)
#define WDG_LOCK            (WDG_BASE + 0x20)
#define WDG_CNT_RD_LOW      (WDG_BASE + 0x24)
#define WDG_CNT_RD_HIGH     (WDG_BASE + 0x28)
#define WDG_CNT_IRQV_LOW    (WDG_BASE + 0x2C)
#define WDG_CNT_IRQV_HIGH   (WDG_BASE + 0x30)

#define WDG_INT_EN_BIT          BIT_0
#define WDG_CNT_EN_BIT          BIT_1
#define WDG_RST_EN_BIT          BIT_3


#define WDG_INT_RST_BIT           BIT_3
#define WDG_INT_CLEAR_BIT       BIT_0

#define WDG_LD_BUSY_BIT         BIT_4

#define WDG_UNLOCK_KEY          0xE551

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/

#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif //_WATCHDOG_REG_V3_H_


