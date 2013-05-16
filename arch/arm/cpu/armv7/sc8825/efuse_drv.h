/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 ************************************************
 * Automatically generated C config: don't edit *
 ************************************************
 */

#ifndef __EFUSE_H__
#define __EFUSE_H__

#define CTL_EFUSE_BASE						0x49000000
#define SCI_D(_addr_)							( *(volatile u32 *)(_addr_) )

/* registers definitions for controller CTL_EFUSE */
#define REG_EFUSE_DATA_RD					( CTL_EFUSE_BASE + 0x0000 )
#define REG_EFUSE_DATA_WR					( CTL_EFUSE_BASE + 0x0004 )
#define REG_EFUSE_BLOCK_INDEX				( CTL_EFUSE_BASE + 0x0008 )
#define REG_EFUSE_MODE_CTRL					( CTL_EFUSE_BASE + 0x000C )
#define REG_EFUSE_PGM_PARA					( CTL_EFUSE_BASE + 0x0010 )
#define REG_EFUSE_STATUS					( CTL_EFUSE_BASE + 0x0014 )
#define REG_EFUSE_BLK_FLAGS					( CTL_EFUSE_BASE + 0x0018 )
#define REG_EFUSE_BLK_CLR					( CTL_EFUSE_BASE + 0x001C )
#define REG_EFUSE_MAGIC_NUMBER				( CTL_EFUSE_BASE + 0x0020 )

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
#define BIT_EFUSE_VDD_ON						( BIT_29 )
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

/* vars definitions for controller CTL_EFUSE */
#define PROT_LOCK							( BIT_31 )

#endif //__EFUSE_H__
