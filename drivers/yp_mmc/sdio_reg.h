/*********************************************************************
 ** File Name:		sdio_reg.h
 ** Author:			yanping.xie
 ** DATE:			09/05/2013
 ** Copyright:			2004 Spreadtrum, Incoporated. All Rights Reserved.
 ** Description:		This file describe operation of sdio host.
 *********************************************************************

 *********************************************************************
 **                               Edit History                                                                                   **
 ** ------------------------------------------------------------------------- **
 ** DATE				NAME				DESCRIPTION
 ** 09/05/2013		ypxie				Create.
 ********************************************************************/
#ifndef __SDIO_REG_H_
#define __SDIO_REG_H_

/* REG[0x0040], REG[0x0044], REG[0x0048] : capability */
typedef struct sdhost_cap_v20_t_tag
{
	/* CAP[0x0040] */
	uint32 timeout_clk_freq		: 6;								///< [5:0]
	uint32 rsrvd0				: 1;								///< [6]
	uint32 timeout_lk_uint		: 1;								///< [7]
	uint32 base_clk_freq		: 6;								///< [13:8]
	uint32 rsrvd1				: 2;								///< [15:14]
	uint32 max_blk_size		: 2;								///< [17:16]
	uint32 rsrvd2				: 3;								///< [20:18]
	uint32 spprt_high_speed	: 1;								///< [21]
	uint32 spprt_dma			: 1;								///< [22]
	uint32 spprt_susp_res		: 1;								///< [23]
	uint32 spprt_volt_33		: 1;								///< [24]
	uint32 spprt_volt_30		: 1;								///< [25]
	uint32 spprt_volt_18		: 1;								///< [26]
	uint32 rsrvd3				: 5;								///< [31:27]

	/* CAP2[0x0044] */
	uint32 cap2;												///< reserved

	/* MAX_CUR_CAP[0x0048] */
	uint32 max_cur_volt_33		: 8;								///< [7:0]
	uint32 max_cur_volt_30		: 8;								///< [15:8]
	uint32 max_cur_volt_18		: 8;								///< [23:16]
	uint32 rsrvd4				: 8;
} sdhost_cap_v20_t;

typedef struct sdhost_cap_v30_t_tag
{
	/* CAP[0x0040] */
	uint32 timeout_clk_freq		: 6;								///< [5:0]
	uint32 rsrvd0				: 1;								///< [6]
	uint32 timeout_lk_uint		: 1;								///< [7]
	uint32 base_clk_freq		: 8;								///< [15:8]
	uint32 max_blk_size		: 2;								///< [17:16]
	uint32 spprt_8bit			: 1;								///< [18]
	uint32 spprt_adma2		: 1;								///< [19]
	uint32 rsrvd1				: 1;								///< [20]
	uint32 spprt_high_speed	: 1;								///< [21]
	uint32 spprt_dma			: 1;								///< [22]
	uint32 spprt_susp_res		: 1;								///< [23]
	uint32 spprt_volt_33		: 1;								///< [24]
	uint32 spprt_volt_30		: 1;								///< [25]
	uint32 spprt_volt_18		: 1;								///< [26]
	uint32 rsrvd2				: 1;								///< [27]
	uint32 spprt_64bit_sys		: 1;								///< [28]
	uint32 spprt_async_int		: 1;								///< [29]
	uint32 rsrvd3				: 2;								///< [31:30]

	/* CAP2[0x0044] */
	uint32 spprt_sdr52			: 1;								///< [0]
	uint32 spprt_sdr104		: 1;								///< [1]
	uint32 spprt_ddr52			: 1;								///< [2]
	uint32 rsrvd4				: 29;							///< [31:3]

	/* MAX_CUR_CAP[0x0048] */
	uint32 max_cur_volt_33		: 8;								///< [7:0]
	uint32 max_cur_volt_30		: 8;								///< [15:8]
	uint32 max_cur_volt_18		: 8;								///< [23:16]
} sdhost_cap_v30_t;

/* REG[0x000C] : cmd trans mode */
/* [cmd_index] bit[29:24] */
/* [cmd_type] bit[23:22] */
#define SDIO_CMD_TYPE_ABORT				( 3 << 22 )
#define SDIO_CMD_TYPE_RESUME			( 2 << 22 )
#define SDIO_CMD_TYPE_SUSPEND			( 1 << 22 )
#define SDIO_CMD_TYPE_NML				( 0 << 22 )

/* [transmode] bit[6:0], bit[21], bit[31], bit[30] */
#define SDIO_BOOT_ACK					BIT_31				///< v3.0
#define SDIO_CMD_LINE_BOOT				BIT_30				///< v3.0
#define SDIO_DATA_PRE					BIT_21

/* v3.0 auto cmd12 is bit[3:2] , v2.0 auto cmd12 is bit[2] */
#define SDIO_TRANS_DIS_AUTO				( 0x00 << 2 )			///< v3.0
#define SDIO_TRANS_AUTO_CMD12_EN		BIT_2				///< v2.0
#define SDIO_TRANS_AUTO_CMD23_EN		( 0x02 << 2 )			///< v3.0

#define SDIO_TRANS_COMP_ATA			BIT_6
#define SDIO_TRANS_MULTIBLK				BIT_5
#define SDIO_TRANS_DIR_READ				BIT_4
#define SDIO_TRANS_BLK_CNT_EN			BIT_1
#define SDIO_TRANS_DMA_EN				BIT_0

/* [response] bit[17:16], bit[20], bit[19]*/
#define SDIO_CMD_INDEX_CHK				BIT_20
#define SDIO_CMD_CRC_CHK				BIT_19

#define SDIO_CMD_NO_RSP					( 0x00 << 16 )
#define SDIO_CMD_RSP_136				( 0x01 << 16 )
#define SDIO_CMD_RSP_48					( 0x02 << 16 )
#define SDIO_CMD_RSP_48_BUSY			( 0x03 << 16 )

#define SDIO_NO_RSP	0x00
#define SDIO_R1		( SDIO_CMD_RSP_48 | SDIO_CMD_INDEX_CHK | SDIO_CMD_CRC_CHK )
#define SDIO_R2		( SDIO_CMD_RSP_136 | SDIO_CMD_CRC_CHK )
#define SDIO_R3		SDIO_CMD_RSP_48
#define SDIO_R4		SDIO_CMD_RSP_48
#define SDIO_R5		( SDIO_CMD_RSP_48 | SDIO_CMD_INDEX_CHK | SDIO_CMD_CRC_CHK )
#define SDIO_R6		( SDIO_CMD_RSP_48 | SDIO_CMD_INDEX_CHK | SDIO_CMD_CRC_CHK )
#define SDIO_R7		( SDIO_CMD_RSP_48 | SDIO_CMD_INDEX_CHK | SDIO_CMD_CRC_CHK )
#define SDIO_R1B		( SDIO_CMD_RSP_48_BUSY | SDIO_CMD_INDEX_CHK | SDIO_CMD_CRC_CHK )
#define SDIO_R5B		( SDIO_CMD_RSP_48_BUSY | SDIO_CMD_INDEX_CHK | SDIO_CMD_CRC_CHK )

/* REG[0x0030], REG[0x0034], REG[0x0038] : interrupt */
#define INT_CMD_CMPLT					BIT_0
#define INT_TR_CMPLT						BIT_1
#define INT_CAP_EVENT					BIT_2
#define INT_DMA_INT						BIT_3
#define INT_BUF_WR_RDY					BIT_4
#define INT_BUF_RD_RDY					BIT_5
#define INT_CARD_INT						BIT_8

#define INT_ERR_INT						BIT_15

#define INT_CMD_TIMEOUT					BIT_16
#define INT_CMD_CRC						BIT_17
#define INT_CMD_END_BIT					BIT_18
#define INT_CMD_IND						BIT_19
#define INT_DATA_TIMEOUT				BIT_20
#define INT_DATA_CRC						BIT_21
#define INT_DATA_END_BIT					BIT_22
#define INT_CUR_LMT						BIT_23
#define INT_AUTO_CMD12					BIT_24
#define INT_TRGT_RESP					BIT_28
#define INT_VNDR_ERR_ST					(BIT_29 | BIT_30 | BIT_31)

#define INT_ALL							(BIT_15 | BIT_8 | 0xFF)

typedef struct sdio_reg_v20_t_tag
{
	volatile uint32					dma_addr;					///< 0x0000
	volatile uint32					blk_size_cnt;					///< 0x0004
	volatile uint32					cmd_argu;					///< 0x0008
	volatile uint32					tr_mode;						///< 0x000C
	volatile uint32					resp0;						///< 0x0010
	volatile uint32					resp1;						///< 0x0014
	volatile uint32					resp2;						///< 0x0018
	volatile uint32					resp3;						///< 0x001C
	volatile uint32					buf_port;						///< 0x0020
	volatile uint32					pres_state;					///< 0x0024
	volatile uint32					sd_ctrl1;						///< 0x0028
	volatile uint32					sd_ctrl2;						///< 0x002C
	volatile uint32					int_st;						///< 0x0030
	volatile uint32					int_st_en;					///< 0x0034
	volatile uint32					int_sig_en;					///< 0x0038
	volatile uint32					cmd12_st;					///< 0x003C
	volatile uint32					cap1;						///< 0x0040
	volatile uint32					cap2;						///< 0x0044
	volatile uint32					cur_cap1;					///< 0x0048
	volatile uint32					cur_cap2;					///< 0x004C
} sdio_reg_v20_t, *sdio_reg_ptr;

typedef struct sdio_reg_v30_t_tag
{
	volatile uint32					dma_addr;					///< 0x0000
	volatile uint32					blk_size_cnt;					///< 0x0004
	volatile uint32					cmd_argu;					///< 0x0008
	volatile uint32					tr_mode;						///< 0x000C
	volatile uint32					resp0;						///< 0x0010
	volatile uint32					resp1;						///< 0x0014
	volatile uint32					resp2;						///< 0x0018
	volatile uint32					resp3;						///< 0x001C
	volatile uint32					buf_port;						///< 0x0020
	volatile uint32					pres_state;					///< 0x0024
	volatile uint32					sd_ctrl1;						///< 0x0028
	volatile uint32					sd_ctrl2;						///< 0x002C
	volatile uint32					int_st;						///< 0x0030
	volatile uint32					int_st_en;					///< 0x0034
	volatile uint32					int_sig_en;					///< 0x0038
	volatile uint32					sd_ctrl3;						///< 0x003C
	volatile uint32					cap1;						///< 0x0040
	volatile uint32					cap2;						///< 0x0044
	volatile uint32					cur_cap1;					///< 0x0048
	volatile uint32					cur_cap2;					///< 0x004C
	volatile uint32					force_evt;					///< 0x0050
	volatile uint32					reserved0;					///< 0x0054
	volatile uint32					reserved1;					///< 0x0058
	volatile uint32					reserved2;					///< 0x005C
	volatile uint32					pre_val_def;					///< 0x0060
	volatile uint32					pre_val_high;					///< 0x0064
	volatile uint32					pre_val_sdr52;				///< 0x0068
	volatile uint32					pre_val_ddr52;				///< 0x006C
	volatile uint32					reserved3;					///< 0x0070
	volatile uint32					reserved4;					///< 0x0074
	volatile uint32					reserved5;					///< 0x0078
	volatile uint32					reserved6;					///< 0x007C
	volatile uint32					wr_dly;						///< 0x0080
	volatile uint32					rd_pos_dly;					///< 0x0080
	volatile uint32					rd_neg_dly;					///< 0x0088
	/* slt_int_st   ...   0x00FC is slot interrupt status */
} sdio_reg_v30_t, *sdio_reg_v30_ptr;

typedef struct sdio_reg_slave_t_tag
{
	volatile uint32					dma_addr;					///< 0x0000
	volatile uint32					rst_dma_set;					///< 0x0004
	volatile uint32					reserved0;					///< 0x0008
	volatile uint32					blk_size_cnt;					///< 0x000C
	volatile uint32					tr_mode;						///< 0x0010
	volatile uint32					cur_st;						///< 0x0014
	volatile uint32					cmd_argu;					///< 0x0018
	volatile uint32					rsp_argu;					///< 0x001C
	volatile uint32					buf_port;						///< 0x0020
	volatile uint32					reserved1;					///< 0x0024
	volatile uint32					reserved2;					///< 0x0028
	volatile uint32					reserved3;					///< 0x002C
	volatile uint32					int_en;						///< 0x0030
	volatile uint32					int_clr;						///< 0x0034
	volatile uint32					int_raw_st;					///< 0x0038
	volatile uint32					int_mask;					///< 0x003C
	volatile uint32					supt_ocr;						///< 0x0040
	volatile uint32					cur_ocr;						///< 0x0044
} sdio_reg_slave_t, *sdio_reg_slave_ptr;

#endif /* __SDIO_REG_H_ */