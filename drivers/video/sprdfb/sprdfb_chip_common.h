/******************************************************************************
 ** File Name:    sprdfb_chip_common.h                                     *
 ** Author:       congfu.zhao                                           *
 ** DATE:         30/04/2013                                        *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved. *
 ** Description:                                                    *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                               *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                       *

 ******************************************************************************/
#ifndef __DISPC_CHIP_COM_H_
#define __DISPC_CHIP_COM_H_

#include <common.h>
#include <asm/arch/dispc_reg.h>
#include <asm/arch/sprd_lcd.h>


#ifdef CONFIG_SPX15

#include "sprdfb_chip_7715.h"


#elif defined(CONFIG_SC8830)


#include "sprdfb_chip_8830.h"

#endif

#ifdef CONFIG_SC8825

#include "sprdfb_chip_8825.h"

#endif

#ifdef CONFIG_SC7710G2

#include "sprdfb_chip_7710.h"

#endif

void __raw_bits_or(unsigned int v, unsigned int a);

void __raw_bits_set_value(unsigned int reg, unsigned int value, unsigned int bit, unsigned int mask);

void dispc_pll_clk_set(unsigned int clk_src, unsigned int clk_div);

void dispc_dbi_clk_set(unsigned int clk_src, unsigned int clk_div);

void dispc_dpi_clk_set(unsigned int clk_src, unsigned int clk_div);



#endif
