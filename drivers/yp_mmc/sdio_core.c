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
#include "sdio_core.h"

#ifdef SDIO_DEBUG
#define sdio_printf(x)			printf(x)
#endif

typedef struct
{
	sdio_cmd_e						cmd;
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

const cmd_ctl_flg_t sdio_cmd_detail[]=
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
sdio_pal_t pal_hd[SLOT_MAX_NUM] = {0,0,0, 0,0,0, 0,0,0, 0,0,0};

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
void init_card_event (sdio_pal_ptr p_hd)
{
	p_hd->card_event = 0;
} /* end of card_event_init */

uint32 wait_card_event (sdio_pal_ptr p_hd, uint32 event_id)
{
	if (event_id == (p_hd->card_event & event_id)) {
		return 0;
	}
	else {
		return 1;
	}
}

void set_card_event (sdio_pal_ptr p_hd, uint32 event_id)
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
void  irq_handle (uint32 isr_num)
{
	uint32 msg;
	sdio_pal_ptr p_hd;

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

sdio_pal_err_e get_cmd_err_info(uint32 card_event)
{
	uint32 err_st;

	err_st = ERR_NONE;
	if (INT_ERR_INT & card_event) {
		err_st &= (card_event & 0x1FFF0000) >> 15;
	}
	return err_st;
}

/*********************************************************************
**	Description :
**		This function used to register sdio.
**	Param :
**		slot_no			: slot no.
**		base_clk			: base clock must be set val in table.
**	Return :
**		p_hd			: return sdio pal handle
**	Date					Author				Operation
**	2013/09/12			ypxie				create
*********************************************************************/
sdio_pal_ptr sdio_pal_open (uint32 slot_no, uint32 base_clk)
{
	if (slot_no >= SLOT_MAX_NUM) {
		return 0;
	}

	/* step 1: init members */
	pal_hd[slot_no].slot_no = slot_no;
	pal_hd[slot_no].sdio_type = (sdio_type_e)(slot_no/3);
	pal_hd[slot_no].sdio_version = sdhost_get_sdio_version(slot_no);
	pal_hd[slot_no].card_event = 0;

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

/*********************************************************************
**	Description :
**		This function used to set power and clock, and set initialize freq, bus width.
**	Param :
**		p_hd			: sdio pal handle.
**		onoff			: power on or off
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/22			ypxie				create
**	2013/10/08			ypxie				alter return and added check handle.
*********************************************************************/
BOOLEAN sdio_pal_power (sdio_pal_ptr p_hd, sdio_onoff_e onoff)
{
	if (p_hd == NULL){
		return FALSE;
	}

	switch (onoff) {
	case SDIO_ON:
		/* reset controller */
		sdhost_reset(p_hd->host_cfg, RST_ALL);

		/* set power for sdio controller. */
		sdhost_set_voltage(p_hd->slot_no, 1800, 3000);
		sdhost_set_power(p_hd->slot_no, SDIO_ON);

		/* set base bus width and speed */
		if (p_hd->sdio_version == SDIO_20) {
			sdhost_set_speed_mode_v20(p_hd->host_cfg, SDIO_LOWSPEED);
			sdhost_set_clk_freq_v20(p_hd->host_cfg, p_hd->base_clock, 400000);
			sdhost_set_bus_width_v20(p_hd->host_cfg, SDIO_BUS_1_BIT);
		}
		else if (p_hd->sdio_version == SDIO_30) {
			sdhost_set_speed_mode_v30(p_hd->host_cfg, SDIO_SDR13);
			sdhost_set_clk_freq_v30(p_hd->host_cfg, p_hd->base_clock, 400000);
			sdhost_set_bus_width_v30(p_hd->host_cfg, SDIO_BUS_1_BIT);
		}

		/* open clock in sdio controller. */
		sdhost_set_internal_clk(p_hd->host_cfg, SDIO_ON);
		sdhost_set_sd_clk(p_hd->host_cfg, SDIO_ON);
		break;

	case SDIO_OFF:
		sdhost_set_sd_clk(p_hd->host_cfg, SDIO_OFF);
		sdhost_reset(p_hd->host_cfg, RST_ALL);
		sdhost_set_power(p_hd->slot_no, SDIO_OFF);
		break;

	default :
		return FALSE;
		break;
	}

	return TRUE;
} /* end of sdio_pal_power */

/*********************************************************************
**	Description :
**		This function used to set clock.
**	Param :
**		p_hd			: sdio pal handle.
**		clk				: 	SDIO_CLK_400K,
**							SDIO_CLK_13M,
**							SDIO_CLK_26M,
**							SDIO_CLK_52M,
**							SDIO_CLK_104M,
**							SDIO_CLK_200M,
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/22			ypxie				create
**	2013/10/08			ypxie				alter return and added check handle.
*********************************************************************/
BOOLEAN sdio_set_clk (sdio_pal_ptr p_hd, uint32 clk)
{
	if (p_hd == NULL){
		return FALSE;
	}

	sdhost_set_sd_clk(p_hd->host_cfg, SDIO_OFF);

	if (p_hd->sdio_version == SDIO_30) {
		p_hd->sd_clock = sdhost_set_clk_freq_v30(p_hd->host_cfg, p_hd->base_clock, clk);
	}
	else if (p_hd->sdio_version == SDIO_20) {
		p_hd->sd_clock = sdhost_set_clk_freq_v20(p_hd->host_cfg, p_hd->base_clock, clk);
	}
	else {
		return FALSE;
	}
	
	sdhost_set_internal_clk(p_hd->host_cfg, SDIO_ON);
	sdhost_set_sd_clk(p_hd->host_cfg, SDIO_ON);

	return TRUE;
} /* end of sdio_set_clk */

/*********************************************************************
**	Description :
**		This function used to set bus width.
**	Param :
**		p_hd			: sdio pal handle.
**		bus_type			: 	SDIO_BUS_1_BIT,
**							SDIO_BUS_4_BIT,
**							SDIO_BUS_8_BIT,
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/23			ypxie				create
**	2013/10/08			ypxie				alter return and added check handle.
*********************************************************************/
BOOLEAN sdio_set_bus_width (sdio_pal_ptr p_hd, sdio_buswidth_e bus_type)
{
	if ((p_hd == NULL) || (bus_type > SDIO_BUS_8_BIT)) {
		return FALSE;
	}

	if (p_hd->sdio_version == SDIO_30) {
		sdhost_set_bus_width_v30(p_hd->host_cfg, bus_type);
		p_hd->bus_width = bus_type;
	}
	else if (p_hd->sdio_version == SDIO_20) {
		sdhost_set_bus_width_v20(p_hd->host_cfg, bus_type);
		p_hd->bus_width = bus_type;
	}
	else {
		return FALSE;
	}

	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to set speed mode.
**	Param :
**		p_hd			: sdio pal handle.
**		onoff			: 	SDIO_LOWSPEED,
**							SDIO_HIGHSPEED,
**							SDIO_SDR13,
**							SDIO_SDR26,
**							SDIO_SDR52,
**							SDIO_SDR104,
**							SDIO_DDR52,
**							SDIO_HS200
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/23			ypxie				create
**	2013/10/08			ypxie				alter return and added check handle.
*********************************************************************/
BOOLEAN sdio_set_speed_mode (sdio_pal_ptr p_hd, sdio_speedmode_e spd_type)
{
	if ((p_hd == NULL) || (spd_type > SDIO_HS200)) {
		return FALSE;
	}

	if (p_hd->sdio_version == SDIO_30) {
		sdhost_set_speed_mode_v30(p_hd->host_cfg, spd_type);
		p_hd->spd_mode = spd_type;
	}
	else if (p_hd->sdio_version == SDIO_20) {
		sdhost_set_speed_mode_v20(p_hd->host_cfg, spd_type);
		p_hd->spd_mode = spd_type;
	}
	else {
		return FALSE;
	}

	return TRUE;
}

/*********************************************************************
**	Description :
**		This function used to send command to card
**	Param :
**		p_hd			: sdio pal handle.
**		cmd				: command index.
**		argu				: argument of card.
**		data_param		: data information.
**		rsp_buf			: used to store response data.
**	Return :
**		none
**	Date					Author				Operation
**	2013/10/08			ypxie				create
*********************************************************************/
PUBLIC sdio_pal_err_e sdio_send_cmd (sdio_pal_ptr p_hd,
								sdio_cmd_e cmd,
								uint32 argu,
								sdio_data_param_t *data_param,
								uint8 *rsp_buf )
{
	uint32 tmp_msg, ret_val;
	int time_out = 0;
	const cmd_ctl_flg_t* cmd_info = NULL;

	cmd_info = &s_cmdDetail[cmd];

	if ((cmd != cmd_info->cmd) || (p_hd == NULL)) {
		return ERR_BAD_PARAM;
	}

	sdio_printf("%s : cmd:%x, cmdIndex:%x, argument:%x\r\n", __FUNCTION__,
					cmd, cmd_info->cmd_index, argu);

	sdhost_int_st_clr(p_hd->host_cfg, INT_ALL);
	sdhost_int_st_sig_dis(p_hd->host_cfg, INT_ALL);

	sdhost_set_data_timeout_value(p_hd->host_cfg, 0x0E);

	tmp_msg = cmd_info->int_filter;
	if (NULL != cmd_info->err_filter) {
		tmp_msg |= cmd_info->err_filter;
	}

	if (NULL != data_param) {
		tmp_msg |= INT_DMA_INT;
	}

	sdhost_int_st_sig_en(p_hd->host_cfg, tmp_msg);

	init_card_event(p_hd);

	if (NULL != data_param) {
		uint32 buf_size = 0;

		buf_size = data_param->blk_len  *  (data_param->blk_num);

		/* enable mmu, must be update cache */
		Dcache_CleanRegion((unsigned int)(data_param->data_buf), buf_size);
		Dcache_InvalRegion((unsigned int)(data_param->data_buf), buf_size);

		sdhost_set_dma_addr(p_hd->host_cfg, (uint32)(data_param->data_buf));
		sdhost_set_data_param(p_hd->host_cfg, data_param->blk_len,
							data_param->blk_num, SDIO_DMA_512K);
	}

	sdhost_set_cmd_argu(p_hd->host_cfg, argu);
	sdhost_set_cmd(p_hd->host_cfg, cmd_info->cmd_index, cmd_info->transmode,
							SDIO_CMD_TYPE_NML, cmd_info->response);

	while (0 != wait_card_event(p_hd, cmd_info->int_filter)) {
		irq_handle(p_hd->slot_no);

		sdhost_delayus(100);
		time_out++;
		if (time_out > 100000) {
			break;
		}
	}

	sdhost_reset (p_hd->host_cfg, RST_CMD_DAT_LINE);

	if (0 != (p_hd->card_event & INT_DATA_TIMEOUT))
	{
		if ( CMD38_ERASE == cmd)
		{
			sdio_printf ("sdio may be erase R1b is too long");
			time_out = SCI_GetTickCount();

			/* BIT_20 is data[0]'s pin status */
			while(0 == (BIT_20 & sdhost_get_pin_state(p_hd->host_cfg))) {
				sdhost_delayus (1000);
				if ((time_out + 20000) < SCI_GetTickCount()) {
					sdhost_reset(p_hd->host_cfg, RST_CMD_DAT_LINE);
					return ERR_DATA_TIMEOUT;
				}
			}

			p_hd->card_event &= ~INT_DATA_TIMEOUT;
		}
		else
		{
			sdio_printf ("SDIO_Card error = 0x%x", p_hd->card_event);
			return p_hd->card_event;
		}
	}

	sdhost_get_response(p_hd->host_cfg, cmd_info->response, rsp_buf);

	sdio_printf(("resp[0-4]:%02X, %02X, %02X, %02X\r\n",
				rsp_buf[0], rsp_buf[1], rsp_buf[2], rsp_buf[3]));
	return get_cmd_err_info(p_hd->card_event);
} /* end of sdio_send_cmd */