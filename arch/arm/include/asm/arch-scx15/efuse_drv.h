/*****************************************************************************
 **  File Name:    effuse_drv.h                                                 *
 **  Author:       Jenny Deng                                                *
 **  Date:         20/10/2009                                                *
 **  Copyright:    2009 Spreadtrum, Incorporated. All Rights Reserved.       *
 **  Description:  This file defines the basic operation interfaces of       *
 **                EFuse initilize and operation. It provides read and         *
 **                writer interfaces of 0~5 efuse. Efuse 0 for Sn block.     *
 **                Efuse 1 to 4 for Hash blocks. Efuse 5 for control block.  *
 *****************************************************************************
 *****************************************************************************
 **  Edit History                                                            *
 **--------------------------------------------------------------------------*
 **  DATE               Author              Operation                        *
 **  20/10/2009         Jenny.Deng          Create.                          *
 **  26/10/2009         Yong.Li             Update.                          *
 **  30/10/2009         Yong.Li             Update after review.             *
 *****************************************************************************/

#ifndef _EFuse_DRV_H
#define _EFuse_DRV_H

#include "sci_types.h"

/***********************structure define**************************************/
#define EFUSE_DATA_RD						(SPRD_UIDEFUSE_PHYS + 0x0000)
#define EFUSE_DATA_WR						(SPRD_UIDEFUSE_PHYS + 0x0004)
#define EFUSE_BLOCK_INDEX					(SPRD_UIDEFUSE_PHYS + 0x0008)
#define EFUSE_MODE_CTRL						(SPRD_UIDEFUSE_PHYS + 0x000c)
#define EFUSE_PGM_PARA						(SPRD_UIDEFUSE_PHYS + 0x0010)
#define EFUSE_STATUS						(SPRD_UIDEFUSE_PHYS + 0x0014)
#define EUSE_MEM_BLOCK_FLAGS				(SPRD_UIDEFUSE_PHYS + 0x0018)
#define EUSE_MEM_BLOCK_FLAGS_CLR			(SPRD_UIDEFUSE_PHYS + 0x001c)
#define EFUSE_MAGIC_NUMBER					(SPRD_UIDEFUSE_PHYS + 0x0020)

/* bits definitions for register REG_EFUSE_BLOCK_INDEX */
#define BITS_READ_INDEX(_x_)					( (_x_) << 0 & ( BIT_0 | BIT_1 | BIT_2 ) )
#define BITS_PGM_INDEX(_x_)					( (_x_) << 16 & ( BIT_16 | BIT_17 | BIT_18 ) )

#define SHIFT_READ_INDEX						( 0 )
#define MASK_READ_INDEX						( BIT_0 | BIT_1 | BIT_2 )

#define SHIFT_PGM_INDEX						( 16 )
#define MASK_PGM_INDEX						( BIT_16 | BIT_17 | BIT_18 )

/* bits definitions for register REG_EFUSE_MODE_CTRL */
#define BIT_PG_START							( BIT_0 )
#define BIT_RD_START							( BIT_1 )
#define BIT_STANDBY_START					( BIT_2 )

/* bits definitions for register REG_EFUSE_PGM_PARA */
#define BITS_TPGM_TIME_CNT(_x_)				( (_x_) & 0x1FF )
#define BIT_CLK_EFS_EN						( BIT_28 )
#define BIT_EFS_VDD_ON						( BIT_29 )
#define BIT_PCLK_DIV_EN						( BIT_30 )
#define BIT_PGM_EN							( BIT_31 )

/* bits definitions for register REG_EFUSE_STATUS */
#define BIT_PGM_BUSY							( BIT_0 )
#define BIT_READ_BUSY						( BIT_1 )
#define BIT_STANDBY_BUSY						( BIT_2 )

/* bits definitions for register REG_EFUSE_BLK_FLAGS */
#define BIT_BLK0_PROT_FLAG					( BIT_0 )
#define BIT_BLK1_PROT_FLAG					( BIT_1 )
#define BIT_BLK2_PROT_FLAG					( BIT_2 )
#define BIT_BLK3_PROT_FLAG					( BIT_3 )
#define BIT_BLK4_PROT_FLAG					( BIT_4 )
#define BIT_BLK5_PROT_FLAG					( BIT_5 )
#define BIT_BLK6_PROT_FLAG					( BIT_6 )
#define BIT_BLK7_PROT_FLAG					( BIT_7 )

/* bits definitions for register REG_EFUSE_BLK_CLR */
#define BIT_BLK0_PROT_FLAG_CLR				( BIT_0 )
#define BIT_BLK1_PROT_FLAG_CLR				( BIT_1 )
#define BIT_BLK2_PROT_FLAG_CLR				( BIT_2 )
#define BIT_BLK3_PROT_FLAG_CLR				( BIT_3 )
#define BIT_BLK4_PROT_FLAG_CLR				( BIT_4 )
#define BIT_BLK5_PROT_FLAG_CLR				( BIT_5 )
#define BIT_BLK6_PROT_FLAG_CLR				( BIT_6 )
#define BIT_BLK7_PROT_FLAG_CLR				( BIT_7 )

/* bits definitions for register REG_EFUSE_MAGIC_NUMBER */
#define BITS_MAGIC_NUMBER(_x_)				( (_x_) & 0xFFFF )
/***********************function declaration**********************************/
int sci_efuse_calibration_get(unsigned int * p_cal_data);
#endif

