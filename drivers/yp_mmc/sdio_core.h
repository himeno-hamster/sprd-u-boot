/*********************************************************************
 ** File Name:		sdio_pal.h
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
 #ifndef __SDIO_CORE_H_
#define __SDIO_CORE_H_

typedef enum sdio_slot_e_tag
{
	SDIO_SLOT_0							= 0,				/* SDIO0 : S1 */
	SDIO_SLOT_1							= 1,				/* SDIO0 : S2 */
	SDIO_SLOT_2							= 2,				/* SDIO0 : SLAVE */
	SDIO_SLOT_3							= 3,				/* SDIO1 : S1 */
	SDIO_SLOT_4							= 4,				/* SDIO1 : S2 */
	SDIO_SLOT_5							= 5,				/* SDIO1 : SLAVEv */
	SDIO_SLOT_6							= 6,				/* SDIO2 : S1 */
	SDIO_SLOT_7							= 7,				/* SDIO2 : S2 */
	SDIO_SLOT_8							= 8,				/* SDIO2 : SLAVE */
	SDIO_SLOT_9							= 9,				/* SDIO3 : S1 (emmc)*/
	SDIO_SLOT_10						= 10,			/* SDIO3 : S2 */
	SDIO_SLOT_11						= 11,			/* SDIO3 : SLAVE */
	SDIO_SLOT_MAX
} sdio_slot_e;

typedef enum sdio_volt_e_tag
{
	VOL_1_2								= 1200,
	VOL_1_3								= 1300,
	VOL_1_5								= 1500,
	VOL_1_8								= 1800,
	VOL_2_5								= 2500,
	VOL_2_8								= 2800,
	VOL_3_0								= 3000,
	VOL_RES
} sdio_vol_e;

typedef enum sdio_type_e_tag
{
	SDIO_TYPE_SD						= 0,
	SDIO_TYPE_WIFI						= 1,
	SDIO_TYPE_MASTER					= 2,
	SDIO_TYPE_MMC						= 3,
	SDIO_TYPE_UNKOWN
} sdio_type_e;

/* used for sd clock */
#define SDIO_CLK_400K					400000
#define SDIO_CLK_13M						13000000
#define SDIO_CLK_26M						26000000
#define SDIO_CLK_52M						52000000
#define SDIO_CLK_104M					104000000
#define SDIO_CLK_200M					200000000


typedef enum sdio_pal_cmd_tag
{
	// cmdindex,rsp,transmode , MS:master & slave
	CMD0_GO_IDLE_STATE,					/* MMC, SD, MS */
	CMD1_SEND_OP_COND,					/* MMC */
	CMD2_ALL_SEND_CID,					/* MMC, SD */
	CMD3_SET_RELATIVE_ADDR_SD,			/* SD */
	CMD3_SET_RELATIVE_ADDR,			/* MMC */
	CMD4_SET_DSR,
	CMD5_SLEEP_AWAKE,					/* MMC, MS */
	CMD6_SWITCH,						/* MMC */
	CMD7_SELECT_CARD_SD,				/* MMC, SD, MS */
	CMD7_SELECT_CARD,					/* MMC */
	CMD8_SEND_IF_COND_SD,				/* SD */
	CMD8_SEND_IF_COND,					/* MMC */
	CMD9_SEND_CSD,						/* MMC, SD */
	CMD10_SEND_CID,						/* MMC, SD */
	CMD11_READ_DAT_UNTIL_STOP,			/* SD */
	CMD11_READ_DAT_UNTIL_STOP_AUT12,	/* SD */
	CMD12_STOP_TRANSMISSION,			/* MMC, SD */
	CMD13_SEND_STATUS,					/* MMC, SD */
	CMD14_BUS_TEST_R,					/* MMC */
	CMD15_GO_INACTIVE_STATE,
	CMD16_SET_BLOCKLEN,					/* MMC, SD */
	CMD17_READ_SINGLE_BLOCK,			/* MMC, SD */
	CMD18_READ_MULTIPLE_BLOCK,			/* MMC, SD */
	CMD18_READ_MULTIPLE_BLOCK_AUT12,	/* MMC, SD */
	CMD19_SEND_TUNING_BLOCK,			/* MMC, SD */
	CMD20_WRITE_DAT_UNTIL_STOP,		/* SD */
	CMD20_WRITE_DAT_UNTIL_STOP_AUT12,	/* SD */
	CMD21_SEND_TUNING_BLOCK_HS200,		/* MMC */
	CMD23_SET_BLOCK_COUNT,				/* MMC, SD */
	CMD24_WRITE_BLOCK,					/* MMC, SD */
	CMD25_WRITE_MULTIPLE_BLOCK,		/* MMC, SD */
	CMD25_WRITE_MULTIPLE_BLOCK_AUT12,	/* MMC, SD */
	CMD26_PROGRAM_CID,					/* MMC */
	CMD27_PROGRAM_CSD,				/* MMC, SD */
	CMD28_SET_WRITE_PROT,				/* MMC, SD */
	CMD29_CLR_WRITE_PROT,				/* MMC, SD */
	CMD30_SEND_WRITE_PROT,				/* MMC, SD */
	CMD31_SEND_WRITE_PROT_TYPE,		/* MMC */
	CMD32_ERASE_WR_BLK_START,			/* SD */
	CMD33_ERASE_WR_BLK_END,			/* SD */
	CMD35_ERASE_GROUP_START,			/* MMC */
	CMD36_ERASE_GROUP_END,				/* MMC */
	CMD38_ERASE,						/* MMC, SD */
	CMD39_FAST_IO,						/* MMC */
	CMD40_GO_IRQ_STATE,				/* MMC */
	CMD42_LOCK_UNLOCK_SD,				/* SD */
	CMD42_LOCK_UNLOCK_MMC,			/* MMC */
	CMD55_APP_CMD,						/* MMC, SD */
	CMD56_GEN_CMD_SD,					/* SD */
	CMD56_GEN_CMD_MMC,				/* MMC */
	ACMD6_SET_BUS_WIDTH,				/* SD */
	ACMD13_SD_STATUS,					/* SD */
	ACMD22_SEND_NUM_WR_BLCOKS,		/* SD */
	ACMD23_SET_WR_BLK_ERASE_COUNT,	/* SD */
	ACMD41_SD_SEND_OP_COND,			/* SD */
	ACMD42_SET_CLR_CARD_DETECT,		/* SD */
	ACMD51_SEND_SCR,					/* SD */
	ACMD6_SET_EXT_CSD,

	CMD52_IO_RW_DIRECT,				/* MS */
	CMD53_READ_BYTES,					/* MS */
	CMD53_READ_BLOCKS,					/* MS */
	CMD53_WRITE_BYTES,					/* MS */
	CMD53_WRITE_BLOCKS,				/* MS */

	CMD_MAX
} sdio_cmd_e;

//typedef void (*sdio_call_back)(uint32 msg, uint32 slot_no);

typedef struct sdio_pal_handle_t_tag
{
	volatile sdio_reg_ptr					host_cfg;				/* base address */
	uint32								slot_no;				/* slot no */
	sdio_type_e							sdio_type;			/* sdio type */
	uint32								sdio_version;			/* sdio version */

//	sdio_call_back							sig_call_back;
	volatile uint32							card_event;
//	volatile uint32							card_err_code;

	uint32								base_clock;			/* base clock */
	uint32								sd_clock;				/* used clock */
	sdio_speedmode_e						spd_mode;
	sdio_buswidth_e						bus_width;			/* bus width */

	uint16								rca;					/* card RCA */
	uint32								block_len;			/* block length */
} sdio_pal_t, *sdio_pal_ptr;

typedef enum 
{
	SDIO_DMA_IN = 1,
	SDIO_DMA_OUT,
	SDIO_DMA_BIDIRECTIONAL
} sdio_dma_dir_e;

typedef struct sdio_data_param_t_tag
{
	uint8*								data_buf;
	uint32								blk_len;
	uint32								blk_num;
	sdio_dma_dir_e						direction;
} sdio_data_param_t;

typedef enum sdio_pal_err_e_tag
{
	ERR_NONE							= 0,
	ERR_CMD_TIMEOUT					= BIT_0,
	ERR_CMD_CRC						= BIT_1,
	ERR_CMD_END_BIT					= BIT_2,
	ERR_CMD_INDEX						= BIT_3,
	ERR_DATA_TIMEOUT					= BIT_4,
	ERR_DATA_CRC						= BIT_5,
	ERR_DATA_END_BIT					= BIT_6,
	ERR_CUR_LIMIT						= BIT_7,
	ERR_AUTO_CMD12						= BIT_8,
	ERR_TRGT_RSP						= BIT_9,
	ERR_BAD_PARAM						= BIT_10				/* software error */
} sdio_pal_err_e;

#endif /* __SDIO_CORE_H_ */