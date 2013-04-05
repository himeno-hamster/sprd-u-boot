/******************************************************************************
 ** File Name:    adi_reg_v3.h                                                *
 ** Author:       Tim.Luo                                                     *
 ** DATE:         03/03/2010                                                  *
 ** Copyright:    2005 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 03/03/2010    Tim.Luo         Create.                                     *
 ******************************************************************************/

#ifndef _ADI_REG_V3_H_
#define _ADI_REG_V3_H_

#include <asm/arch/sprd_reg.h>

/*----------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **-------------------------------------------------------------------------- */

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/**----------------------------------------------------------------------------*
**                               Micro Define                                 **
**----------------------------------------------------------------------------*/

#define  ADI_CLK_DIV            (SPRD_MISC_PHYS + 0x0 )
#define  ADI_CTL_REG            (SPRD_MISC_PHYS + 0x4 )
#define  ADI_CHANNEL_PRI        (SPRD_MISC_PHYS + 0x8 )
#define  ADI_INT_EN             (SPRD_MISC_PHYS + 0xC )
#define  ADI_INT_RAW_STS        (SPRD_MISC_PHYS + 0x10)
#define  ADI_INT_MASK_STS       (SPRD_MISC_PHYS + 0x14)
#define  ADI_INT_CLR            (SPRD_MISC_PHYS_ADDR + 0x18)
//#define  RESERVED             (SPRD_MISC_PHYS_ADDR + 0x1C)
//#define  RESERVED             (SPRD_MISC_PHYS_ADDR + 0x20)
#define  ADI_ARM_RD_CMD         (SPRD_MISC_PHYS + 0x24)
#define  ADI_RD_DATA            (SPRD_MISC_PHYS + 0x28)
#define  ADI_FIFO_STS           (SPRD_MISC_PHYS + 0x2C)
#define  ADI_STS                (SPRD_MISC_PHYS + 0x30)
#define  ADI_REQ_STS            (SPRD_MISC_PHYS + 0x34)

//ADI_EIC
#define ADI_EIC_DATA		(SPRD_ANA_EIC_PHYS + 0x00)
#define ADI_EIC_MASK		(SPRD_ANA_EIC_PHYS + 0x04)

//ADI_CTL_REG
#define ANA_INT_STEAL_EN        BIT_0
#define ARM_SERCLK_EN           BIT_1
#define DSP_SERCLK_EN           BIT_2

//ADI_FIFO_STS
#define   ADI_FIFO_EMPTY        BIT_10
#define   ADI_FIFO_FULL         BIT_11



//ADI_CHANNEL_PRI bit define
#define    INT_STEAL_PRI        0
#define    STC_WR_PRI           2
#define    ARM_WR_PRI           4
#define    ARM_RD_PRI           6
#define    DSP_WR_PRI           8
#define    DSP_RD_PRI           10
#define    RFT_WR_PRI           12
#define    PD_WR_PRI            14

/**----------------------------------------------------------------------------*
**                             Data Prototype                                 **
**----------------------------------------------------------------------------*/

/**----------------------------------------------------------------------------*
**                         Local Function Prototype                           **
**----------------------------------------------------------------------------*/

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/


/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

#endif  //_ADI_REG_V3_H_


