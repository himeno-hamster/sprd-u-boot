/*********************************************************************
 ** File Name:		sdio_pal.c
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
#include "sdio_reg.h"
#include "sdio_phy.h"
#include "sdio_pal.h"

typedef struct
{
	sdio_pal_cmd_e					cmd;
	uint32							cmd_index;
	uint32							int_filter;
	uint32							response;
	uint32							err_filter;
	uint32							transmode;
} cmd_ctl_flg_t;

// response Name						cmd int filter				, rsp				, cmd error filter
#define CARD_SDIO_NO_RSP			NULL	| INT_CMD_CMPLT	, SDIO_NO_RSP	, NULL
#define CARD_SDIO_R1					NULL	| INT_CMD_CMPLT	, SDIO_R1		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
#define CARD_SDIO_R2					NULL	| INT_CMD_CMPLT	, SDIO_R2		, INT_TRGT_RESP					| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
#define CARD_SDIO_R3					NULL	| INT_CMD_CMPLT	, SDIO_R3		, INT_TRGT_RESP					| INT_CMD_END_BIT					| INT_CMD_TIMEOUT
#define CARD_SDIO_R4					NULL	| INT_CMD_CMPLT	, SDIO_R4		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
#define CARD_SDIO_R5					NULL	| INT_CMD_CMPLT	, SDIO_R5		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
#define CARD_SDIO_R6					NULL	| INT_CMD_CMPLT	, SDIO_R6		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
#define CARD_SDIO_R7					NULL	| INT_CMD_CMPLT	, SDIO_R7		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
#define CARD_SDIO_R1B				NULL	| INT_CMD_CMPLT	, SDIO_R1B		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT
//#define CARD_SDIO_R5B				NULL	| INT_CMD_CMPLT	, SDIO_R5B		, INT_TRGT_RESP	| INT_CMD_IND	| INT_CMD_END_BIT	| INT_CMD_CRC	| INT_CMD_TIMEOUT

LOCAL const cmd_ctl_flg_t sdio_cmd_detail[]=
{
	// cmdindex,rsp,transmode
	//#define CMDname						cmdindex	,data int filter	+	(cmd int filter+)rsp(+cmd error filter) +	,data error filter											, transmode
	{ CMD0_GO_IDLE_STATE					, 0	, NULL			| CARD_SDIO_NO_RSP	| NULL																		, NULL																			},
	{ CMD1_SEND_OP_COND					, 1	, NULL			| CARD_SDIO_R3		| NULL																		, NULL																			},
	{ CMD2_ALL_SEND_CID						, 2	, NULL			| CARD_SDIO_R2		| NULL																		, NULL																			},
	{ CMD3_SET_RELATIVE_ADDR_SD			, 3	, NULL			| CARD_SDIO_R6		| NULL																		, NULL																			},
	{ CMD3_SET_RELATIVE_ADDR				, 3	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD4_SET_DSR							, 4	, NULL			| CARD_SDIO_NO_RSP	| NULL																		, NULL																			},
//	{ CMD5_SLEEP_AWAKE						, 5	, NULL			| CARD_SDIO_R4												, NULL								, CMD_TYPE_NORMAL																},
	{ CMD6_SWITCH							, 6	, INT_TR_CMPLT 	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_DIR_READ	| SDIO_TRANS_DMA_EN			| SDIO_DATA_PRE			},
	{ CMD7_SELECT_CARD_SD					, 7	, NULL			| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ CMD7_SELECT_CARD						, 7	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD8_SEND_IF_COND_SD					, 8	, NULL			| CARD_SDIO_R7		| NULL																		, NULL																			},
	{ CMD8_SEND_IF_COND						, 8	, INT_TR_CMPLT	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_DIR_READ	| SDIO_TRANS_DMA_EN			| SDIO_DATA_PRE			},
	{ CMD9_SEND_CSD						, 9	, NULL			| CARD_SDIO_R2		| NULL																		, NULL																			},
	{ CMD10_SEND_CID 						, 10	, NULL			| CARD_SDIO_R2		| NULL																		, NULL																			},
	{ CMD11_READ_DAT_UNTIL_STOP			, 11	, INT_TR_CMPLT 	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_DIR_READ			| SDIO_TRANS_BLK_CNT_EN		| SDIO_TRANS_DMA_EN	| SDIO_DATA_PRE							},
	{ CMD11_READ_DAT_UNTIL_STOP_AUT12		, 11	, INT_TR_CMPLT 	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT	| INT_AUTO_CMD12	, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_DIR_READ			| SDIO_TRANS_AUTO_CMD12_EN| SDIO_TRANS_BLK_CNT_EN	| SDIO_TRANS_DMA_EN	| SDIO_DATA_PRE	},
	{ CMD12_STOP_TRANSMISSION				, 12	, INT_TR_CMPLT 	| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ CMD13_SEND_STATUS					, 13	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD15_GO_INACTIVE_STATE				, 15	, NULL			| CARD_SDIO_NO_RSP	| NULL																		, NULL																			},
	{ CMD16_SET_BLOCKLEN 					, 16	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD17_READ_SINGLE_BLOCK				, 17	, INT_TR_CMPLT 	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_DIR_READ	| SDIO_TRANS_DMA_EN 			| SDIO_DATA_PRE			},
	{ CMD18_READ_MULTIPLE_BLOCK				, 18	, INT_TR_CMPLT 	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_DIR_READ			| SDIO_TRANS_BLK_CNT_EN		| SDIO_TRANS_DMA_EN	| SDIO_DATA_PRE							},
	{ CMD18_READ_MULTIPLE_BLOCK_AUT12		, 18	, INT_TR_CMPLT 	| CARD_SDIO_R1		| INT_DATA_END_BIT	| INT_DATA_CRC	| INT_DATA_TIMEOUT	| INT_AUTO_CMD12	, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_DIR_READ			| SDIO_TRANS_AUTO_CMD12_EN| SDIO_TRANS_BLK_CNT_EN	| SDIO_TRANS_DMA_EN	| SDIO_DATA_PRE	},
	{ CMD20_WRITE_DAT_UNTIL_STOP			, 20	, INT_TR_CMPLT 	| CARD_SDIO_R1							| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_BLK_CNT_EN			| SDIO_TRANS_DMA_EN		| SDIO_DATA_PRE													},
	{ CMD20_WRITE_DAT_UNTIL_STOP_AUT12	, 20	, INT_TR_CMPLT	| CARD_SDIO_R1							| INT_DATA_CRC	| INT_DATA_TIMEOUT	| INT_AUTO_CMD12	, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_AUTO_CMD12_EN	| SDIO_TRANS_BLK_CNT_EN		| SDIO_TRANS_DMA_EN	| SDIO_DATA_PRE							},
	{ CMD23_SET_BLOCK_COUNT				, 23	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD24_WRITE_BLOCK					, 24	, INT_TR_CMPLT 	| CARD_SDIO_R1							| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_DMA_EN		| SDIO_DATA_PRE											},
	{ CMD25_WRITE_MULTIPLE_BLOCK			, 25	, INT_TR_CMPLT 	| CARD_SDIO_R1							| INT_DATA_CRC	| INT_DATA_TIMEOUT						, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_BLK_CNT_EN			| SDIO_TRANS_DMA_EN		| SDIO_DATA_PRE													},
	{ CMD25_WRITE_MULTIPLE_BLOCK_AUT12		, 25	, INT_TR_CMPLT 	| CARD_SDIO_R1							| INT_DATA_CRC	| INT_DATA_TIMEOUT	| INT_AUTO_CMD12	, SDIO_TRANS_MULTIBLK	| SDIO_TRANS_AUTO_CMD12_EN	| SDIO_TRANS_BLK_CNT_EN		| SDIO_TRANS_DMA_EN	| SDIO_DATA_PRE							},
	{ CMD26_PROGRAM_CID					, 26	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD27_PROGRAM_CSD					, 27	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD28_SET_WRITE_PROT					, 28	, INT_TR_CMPLT 	| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ CMD29_CLR_WRITE_PROT					, 29	, INT_TR_CMPLT 	| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ CMD30_SEND_WRITE_PROT				, 30	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD32_ERASE_WR_BLK_START			, 32	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD33_ERASE_WR_BLK_END				, 33	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD35_ERASE_GROUP_START				, 35	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD36_ERASE_GROUP_END				, 36	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD38_ERASE							, 38	, INT_CMD_CMPLT 	| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ CMD39_FAST_IO 						, 39	, NULL			| CARD_SDIO_R4		| NULL																		, NULL																			},
	{ CMD40_GO_IRQ_STATE			 		, 40	, NULL			| CARD_SDIO_R5		| NULL																		, NULL																			},
	{ CMD42_LOCK_UNLOCK_SD					, 42	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD42_LOCK_UNLOCK_MMC				, 42	, NULL			| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ CMD55_APP_CMD						, 55	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD56_GEN_CMD_SD 					, 56	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ CMD56_GEN_CMD_MMC					, 56	, NULL			| CARD_SDIO_R1B		| NULL																		, NULL																			},
	{ ACMD6_SET_BUS_WIDTH					, 6	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ ACMD13_SD_STATUS						, 13	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ ACMD22_SEND_NUM_WR_BLCOKS 			, 22	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ ACMD23_SET_WR_BLK_ERASE_COUNT		, 23	, NULL 			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ ACMD41_SD_SEND_OP_COND 				, 41	, NULL			| CARD_SDIO_R3		| NULL																		, NULL																			},
	{ ACMD42_SET_CLR_CARD_DETECT			, 42	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},
	{ ACMD51_SEND_SCR						, 51	, NULL			| CARD_SDIO_R1		| NULL																		, NULL																			},

	{ CMD_MAX								, 0	, NULL			| CARD_SDIO_NO_RSP	| NULL																		, NULL																			}
};

/* every sdio controller have one s_sdio_pal_hd table */
/* slot0,1,2 is sdio0; slot3,4,5 is sdio1; slot6,7,8 is sdio2; slot9,10,11 is sdio3; */
LOCAL sdio_pal_hd_t pal_hd[SLOT_MAX_NUM];

/*********************************************************************
**	Description :
**		This function used to init card event.
**	Param :
**		p_hd			: sdio pal handle.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/12			ypxie				create
*********************************************************************/
LOCAL void init_card_event (sdio_pal_hd_ptr p_hd)
{
	p_hd->card_event = 0;
} /* end of card_event_init */

LOCAL uint32 wait_card_event (sdio_pal_hd_ptr p_hd, uint32 event_id)
{
	if (event_id == (p_hd->card_event & event_id)) {
		return 0;
	}
	else {
		return 1;
	}
}

LOCAL void set_card_event (sdio_pal_hd_ptr p_hd, uint32 event_id)
{
	p_hd->card_event |= event_id;
}

/*********************************************************************
**	Description :
**		This function used to controller irq process function.
**	Param :
**		msg				: int status information.
**		slot_no			: slot no.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/12			ypxie				create
*********************************************************************/
LOCAL void  irq_handle (uint32 isr_num)
{
	uint32 msg;
	sdio_pal_hd_ptr p_hd;

	if(isr_num == SLOT_MAX_NUM){
		return ISR_DONE;
	}
	p_hd = &pal_hd[(isr_num/3)];

	msg = sdhost_get_int_st(p_hd->host_cfg);

	sdhost_int_st_sig_dis(p_hd->host_cfg, msg);
	sdhost_int_st_clr(p_hd->host_cfg, msg);

	if (0 != (msg & INT_TR_CMPLT)) {
		msg &= ~INT_DATA_TIMEOUT;
		set_card_event(p_hd, msg);
	}
	else if (0 != (msg & INT_DMA_INT)) {
		volatile uint32 next_addr;

		next_addr = sdhost_get_dma_addr(p_hd->host_cfg);
		sdhost_set_dma_addr(p_hd->host_cfg, next_addr);

		sdhost_int_st_sig_en(p_hd->host_cfg, INT_DMA_INT);
	}
	else {
		if (0 != (msg & INT_CMD_CMPLT)) {
			msg &= ~INT_CMD_TIMEOUT;
		}
		set_card_event(p_hd, msg);
	}
} /* end of irq_card_proc */

/*********************************************************************
**	Description :
**		This function used to register sdio.
**	Param :
**		slot_no			: slot no.
**		base_clk			: base clock must be set val in table.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/12			ypxie				create
*********************************************************************/
PUBLIC sdio_pal_hd_ptr sdio_pal_open (uint32 slot_no, uint32 base_clk)
{
	if (slot_no >= SLOT_MAX_NUM) {
		return 0;
	}

	/* step 1: init members */
	pal_hd[slot_no].sdio_type = (sdio_type_e)id;
	pal_hd[slot_no].card_event = 0;
	pal_hd[slot_no].slot_no = slot_no;

	/* step 2: get base address */
	pal_hd[slot_no].host_cfg = sdhost_get_base_addr(slot_no);
	if (pal_hd[slot_no].host_cfg == 0) {
		return 0;
	}

	/* step 3: select slot */
	sdhost_slot_select(slot_no);

	/* step 4: enable sdio ahb clock */
	sdhost_ahb_sdio_en(slot_no);

	/* step 5: disable sd clock for delete glitch in clock line. */
	sdhost_set_sd_clk(pal_hd[slot_no].host_cfg, SDIO_OFF);

	/* step 6: ahb soft reset. */
	sdhost_ahb_sdio_rst(slot_no);

	/* step 7: set base clock */
	pal_hd[slot_no].base_clock = sdhost_set_base_clk(slot_no, base_clk);
	if (pal_hd[slot_no].base_clock == 0) {
		return 0;
	}

	return &pal_hd[slot_no];
} /* end of sdio_pal_open */