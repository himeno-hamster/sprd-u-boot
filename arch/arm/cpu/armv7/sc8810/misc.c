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
#include <common.h>
#include <asm/io.h>

#include <common.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_ana.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sc8810_reg_ahb.h>
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
#define CHIP_REG_OR(reg_addr, value)    (*(volatile unsigned int *)(reg_addr) |= (unsigned int)(value))
#define CHIP_REG_AND(reg_addr, value)   (*(volatile unsigned int *)(reg_addr) &= (unsigned int)(value))
#define CHIP_REG_GET(reg_addr)          (*(volatile unsigned int *)(reg_addr))
#define CHIP_REG_SET(reg_addr, value)   (*(volatile unsigned int *)(reg_addr)  = (unsigned int)(value))

/**---------------------------------------------------------------------------*
 **                         Struct defines.
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                         Global variables                                  *
 **---------------------------------------------------------------------------*/
#if defined(CONFIG_SC7710G2) 
LOCAL unsigned int chip_id = 0;
LOCAL unsigned int ana_chip_id = 0;
#endif
/**---------------------------------------------------------------------------*
 **                         Function Definitions                              *
 **---------------------------------------------------------------------------*/

unsigned int CHIP_PHY_InitChipID(void)
{
#if defined(CONFIG_SC7710G2) 
	chip_id = CHIP_REG_GET(CHIP_ID);
	ana_chip_id = ((ADI_Analogdie_reg_read(ANA_CHIP_ID_HIGH) << 16) |
					ADI_Analogdie_reg_read(ANA_CHIP_ID_LOW));

#endif
	return SCI_TRUE;
}

unsigned int CHIP_PHY_GetChipID(void)
{
#if defined(CONFIG_SC7710G2) 
	return CHIP_REG_GET(CHIP_ID);
#endif
}

unsigned int CHIP_PHY_GetANAChipID(void)
{
#if defined(CONFIG_SC7710G2) 
	return ((ADI_Analogdie_reg_read(ANA_CHIP_ID_HIGH) << 16) |
					ADI_Analogdie_reg_read(ANA_CHIP_ID_LOW));
#endif
}


/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif