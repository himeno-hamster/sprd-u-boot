/******************************************************************************
 ** File Name:    sdram_cfg.h                                                 *
 ** Author:       Daniel.Ding                                                 *
 ** DATE:         6/25/2006                                                   *
 ** Copyright:    2005 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 6/25/2006      Daniel.Ding     Create.                                    *
 ******************************************************************************/
#ifndef _SDRAM_CFG_H_
#define _SDRAM_CFG_H_

#include "sci_types.h"
#include "sdram_drvapi.h"

struct sc8810_ddr_reset_para {
	unsigned char m_c;
	unsigned char d_c;
	unsigned char cyc_3;
	unsigned char cyc_4;
	unsigned char cyc_5;

    // clock
   u32 emc_clk;

	// driver strength
   u32 dqs_drv;
   u32 dat_drv;
   u32 ctl_drv;
   u32 clk_drv;
    
   // clk wr
   u32 clk_wr;
   u32 read_value;  // value from 0x20000174
};

#endif /* SDRAM_CFG_H */
