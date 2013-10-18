/*********************************************************************
 ** File Name:		sdio_phy.c
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
#include <common.h>
#include "sdio_reg.h"
#include "sdio_phy.h"

PUBLIC void sdhost_delayus(uint32 usec)
{
	volatile uint32 i;
	/* current is 800MHZ, when usec =1, the mean is delay 1 us */
	for (i = 0; i < (usec << 1); i++);
}

PUBLIC uint32 sdhost_get_pin_state (sdio_reg_ptr p_cfg)
{
	return p_cfg->pres_state;
}

/*********************************************************************
**	Description :
**		This function used to enable pause function of host.
**	Param :
**		p_cfg			: sdio control reg.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_read_wait_ctl_en (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl1 |= BIT_18;
}

/*********************************************************************
**	Description :
**		This function used to set break point the transmition.
**	Param :
**		sdhost_handler	: sdio control reg.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_stop_at_blk_gap_req (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl1 |= BIT_16;
}

PUBLIC void sdhost_clear_at_blk_gap_req (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl1 &= ~BIT_16;
}

PUBLIC void sdhost_blk_gap_int_en(sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl1 |= BIT_19;
}

PUBLIC uint32 sdhost_get_transfer_status(sdio_reg_ptr p_cfg)
{
	return (p_cfg->pres_state & (BIT_8|BIT_9));
}

/*********************************************************************
**	Description :
**		This function used to when transmission is paused ,this function can resume the
**		transmission.
**	Param :
**		p_cfg			: sdio control reg.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_continue(sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl1 &= ~(BIT_16 | BIT_17);
	p_cfg->sd_ctrl1 |= BIT_17;
}

/*********************************************************************
**	Description :
**		This function used to set controller bus width.
**	Param :
**		p_cfg			: sdio control reg.
**		width			: data bus width,only 1bit ,4bit and 8bit canbe used.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_bus_width_v20(sdio_reg_ptr p_cfg, sdio_buswidth_e width)
{
	uint32 tmp_reg;
	tmp_reg =  p_cfg->sd_ctrl1;

	tmp_reg &= ~BIT_1;

	switch (width) {
	case SDIO_BUS_4_BIT:
		tmp_reg |= BIT_1;
		break;

	case SDIO_BUS_1_BIT:
	default:
		break;
	}

	p_cfg->sd_ctrl1 = tmp_reg;
} /* end of sdhost_set_bus_width */

PUBLIC void sdhost_set_bus_width_v30(sdio_reg_ptr p_cfg, sdio_buswidth_e width)
{
	uint32 tmp_reg;
	sdio_reg_v30_ptr p_tmp_cfg;

	p_tmp_cfg = (sdio_reg_v30_ptr) p_cfg;

	tmp_reg =  p_tmp_cfg->sd_ctrl1;

	tmp_reg &= ~BIT_5;
	tmp_reg &= ~BIT_1;

	switch (width) {
	case SDIO_BUS_4_BIT:
		tmp_reg |= BIT_1;
		break;

	case SDIO_BUS_8_BIT:
		tmp_reg |= BIT_5;
		break;

	case SDIO_BUS_1_BIT:
	default:
		break;
	}

	p_tmp_cfg->sd_ctrl1 = tmp_reg;
} /* end of sdhost_set_bus_width */

/*********************************************************************
**	Description :
**		This function used to set controller speed mode.
**	Param :
**		p_cfg			: sdio control reg.
**		speed			: data bus width, only sdr13, sdr26, sdr52, ddr52, hs200 used.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_speed_mode_v20 (sdio_reg_ptr p_cfg, sdio_speedmode_e speed_mode)
{
	uint32 tmp_reg;

	tmp_reg = p_cfg->sd_ctrl1;
	tmp_reg &= ~BIT_2;

	switch (speed_mode) {
	case SDIO_HIGHSPEED:
		tmp_reg |= BIT_2;
		break;

	case SDIO_LOWSPEED:
	default:
		break;
	}

	p_cfg->sd_ctrl1 = tmp_reg;
} /* end of sdhost_set_speed_mode */

PUBLIC void sdhost_set_speed_mode_v30 (sdio_reg_ptr p_cfg, sdio_speedmode_e speed_mode)
{
	uint32 tmp_reg;
	sdio_reg_v30_ptr p_tmp_cfg;

	p_tmp_cfg = (sdio_reg_v30_ptr) p_cfg;

	/* clear high speed bit in controller version3.0. */
	p_tmp_cfg->sd_ctrl1 &= ~BIT_2;

	tmp_reg = p_tmp_cfg->sd_ctrl3;
	tmp_reg &= ~( BIT_16 | BIT_17 | BIT_18 );

	switch (speed_mode) {
	case SDIO_SDR26:
	case SDIO_HIGHSPEED:
		tmp_reg |= BIT_16;
		break;

	case SDIO_SDR52:
		tmp_reg |= BIT_17;
		break;

	case SDIO_SDR104:
	case SDIO_HS200:
		tmp_reg |= (BIT_16 | BIT_17);
		break;

	case SDIO_DDR52:
		tmp_reg |= BIT_18;
		break;

	case SDIO_SDR13:
	case SDIO_LOWSPEED:
	default:
		break;
	}

	p_tmp_cfg->sd_ctrl3 = tmp_reg;
} /* end of sdhost_set_speed_mode */

/*********************************************************************
**	Description :
**		This function used to set internal clock on or off and wait set over.
**	Param :
**		p_cfg			: sdio control reg.
**		onoff			: enable or disable.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_internal_clk (sdio_reg_ptr p_cfg, sdio_onoff_e onoff)
{
	uint32 timeout, chk_val;

	switch (onoff) {
	case SDIO_ON:
		/* Enable internal clock */
		p_cfg->sd_ctrl2 |= BIT_0;
		chk_val = 0;
		break;

	case SDIO_OFF:
	default:
		p_cfg->sd_ctrl2 &= (~BIT_0);
		chk_val = BIT_1;
		break;
	}

	/* wait set over for bit_1 */
	timeout = 60;
	while ((chk_val == (p_cfg->sd_ctrl2 & BIT_1)) && timeout) {
		sdhost_delayus(50);
		timeout--;
	}
} /* end of sdhost_set_internal_clk */

/*********************************************************************
**	Description :
**		This function used to set sd clock on or off and wait set over.
**	Param :
**		p_cfg			: sdio control reg.
**		onoff			: enable or disable.
**	Return :
**		none.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_sd_clk (sdio_reg_ptr p_cfg, sdio_onoff_e onoff)
{
	switch (onoff) {
	case SDIO_ON:
		/* Enable internal clock */
		p_cfg->sd_ctrl2 |= BIT_2;
		break;

	case SDIO_OFF:
	default:
		if ((p_cfg->sd_ctrl2 & BIT_2) != 0) {
			p_cfg->sd_ctrl2 &= ~BIT_2;

			/* used to wait two clocks for clear glitch in clk line. */
			SDHOST_Delayus(200);
		}
		break;
	}

} /* end of sdhost_set_sd_clk */

/*********************************************************************
**	Description :
**		This function used to set sd clock.
**	Param :
**		p_cfg			: sdio control reg.
**		base_clk			: base clock.
**		clk				: want sdio controller runing clock.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_set_clk_freq_v20 (sdio_reg_ptr p_cfg, uint32 base_clk, uint32 clk)
{
	volatile uint32 tmp_reg;
	uint32 clk_div, sd_clk, i;

	/* SDCLK Frequency Select ,Configure SDCLK select */
	clk_div = base_clk / clk;
	if (0 != base_clk % clk) {
		clk_div++;
	}

	tmp_reg = p_cfg->sd_ctrl2;
	tmp_reg &= ~(0xff<<8);

	for (i = 0; i < 8; i++) {
		if (((clk_div > (1 << i)) && (clk_div <= (1 << (i + 1)))) || (i ==  7)) {
			clk_div = (1 << (i + 1));
			tmp_reg |= (i << 8);
			break;
		}
	}

	sd_clk = base_clk/clk_div;
	p_cfg->sd_ctrl2 = tmp_reg;

	return sd_clk;
} /* end of sdhost_set_clk_freq_v20 */

PUBLIC uint32 sdhost_set_clk_freq_v30 (sdio_reg_ptr p_cfg, uint32 base_clk, uint32 clk)
{
	volatile uint32 tmp_reg;
	uint32 clk_div, sd_clk;
	sdio_reg_v30_ptr p_tmp_cfg;

	p_tmp_cfg = (sdio_reg_v30_ptr) p_cfg;

	/* SDCLK Frequency Select ,Configure SDCLK select */
	clk_div = base_clk / clk;
	clk_div /= 2;
	if (0 != base_clk % clk) {
		clk_div++;
	}

	tmp_reg = p_tmp_cfg->sd_ctrl2;
	if (clk_div > 0) {
		clk_div--;
	}

	tmp_reg &= ~(0x3ff << 6);
	tmp_reg |= ((clk_div >> 8) & 0x3) << 6;
	tmp_reg |= (clk_div & 0xff) << 8;

	sd_clk = base_clk / (2 * (clk_div + 1));

	p_tmp_cfg->sd_ctrl2 = tmp_reg;

	return sd_clk;
} /* end of sdhost_set_clk_freq_v30 */

/*********************************************************************
**	Description :
**		This function used to set timeout value for transfer cmd.
**	Param :
**		p_cfg			: sdio control reg.
**		clk_cnt			: current clock freq num.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_data_timeout_value (sdio_reg_ptr p_cfg, uint32 clk_cnt)
{
	volatile uint32 tmp_reg, tmp_int_reg;

	tmp_int_reg = p_cfg->int_st_en;

	/* cfg the data timeout clk */
	p_cfg->int_st_en &= ~BIT_20;

	tmp_reg = p_cfg->sd_ctrl2;
	tmp_reg &= ~(BIT_16 | BIT_17 | BIT_18 | BIT_19);
	tmp_reg |= (clk_cnt & 0x0F) << 16;
	p_cfg->sd_ctrl2 = tmp_reg;

	p_cfg->int_st_en = tmp_int_reg;
}

/*********************************************************************
**	Description :
**		This function used to reset data line.
**	Param :
**		p_cfg			: sdio control reg.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
LOCAL void _reset_data_line (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl2 |= BIT_26;

	while (0 != (p_cfg->sd_ctrl2 & BIT_26));
}

/*********************************************************************
**	Description :
**		This function used to reset cmd line.
**	Param :
**		p_cfg			: sdio control reg.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
LOCAL void _reset_cmd_line (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl2 |= BIT_25;

	while (0 != (p_cfg->sd_ctrl2 & BIT_25));
}

LOCAL void _reset_data_cmd_line (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl2 |= (BIT_25 | BIT_26);

	while (0 != (p_cfg->sd_ctrl2 & (BIT_25 | BIT_26)));
}

/*********************************************************************
**	Description :
**		This function used to reset cmd line.
**	Param :
**		p_cfg			: sdio control reg.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
LOCAL void _reset_all (sdio_reg_ptr p_cfg)
{
	p_cfg->sd_ctrl2 |= BIT_24;

	while (0 != (p_cfg->sd_ctrl2 & BIT_24));
}

/*********************************************************************
**	Description :
**		This function used to reset the specify module of host.
**	Param :
**		p_cfg			: sdio control reg.
**		rst_type			: select reset type.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_reset (sdio_reg_ptr p_cfg, sdio_rst_type_e rst_type)
{
	switch (rst_type) {
	case RST_CMD_LINE:
		_reset_cmd_line(p_cfg);
		break;

	case RST_DAT_LINE:
		_reset_data_line(p_cfg);
		break;

	case RST_CMD_DAT_LINE:
		_reset_data_cmd_line(p_cfg);
		break;

	case RST_ALL:
		sdhost_set_sd_clk(p_cfg, SDIO_OFF);
		_reset_all(p_cfg);
		break;

	case RST_MODULE:
		break;
	default:
		break;
	}
} /* end of sdhost_reset */

/*********************************************************************
**	Description :
**		This function used to set dma transfer information.
**	Param :
**		p_cfg			: sdio control reg.
**		blk_size			: set block size.
**		blk_cnt			: block count.
**		dma_size			: select dma size.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_data_param (sdio_reg_ptr p_cfg,
								uint32 blk_size,
								uint32 blk_cnt,
								sdio_dma_size_e dma_size)
{
	volatile uint32 tmp_reg;

	tmp_reg = p_cfg->blk_size_cnt;

	/* Set Block Size */
	tmp_reg &= ~BIT_15;
	tmp_reg &= ~0xFFF;
	if (0x1000 >= blk_size) {
		tmp_reg |= BIT_15;
	}
	else {
		tmp_reg |= blk_size;
	}

	/* Set Block Cnt [31 : 16] */
	tmp_reg &= ~0xFFFF0000;
	tmp_reg |= blk_cnt << 16;

	/* Set DMA Buf Size [14 : 12] */
	tmp_reg &= ~(BIT_12 | BIT_13 | BIT_14);

	switch (dma_size) {
	case SDIO_DMA_8K:
		tmp_reg |= BIT_12;
		break;

	case SDIO_DMA_16K:
		tmp_reg |= BIT_13;
		break;

	case SDIO_DMA_32K:
		tmp_reg |= (BIT_12 | BIT_13);
		break;

	case SDIO_DMA_64K:
		tmp_reg |= BIT_14;
		break;

	case SDIO_DMA_128K:
		tmp_reg |= (BIT_12 | BIT_14);
		break;

	case SDIO_DMA_256K:
		tmp_reg |= (BIT_13 | BIT_14);
		break;

	case SDIO_DMA_512K:
		tmp_reg |= (BIT_12 | BIT_13 | BIT_14);
		break;

	case SDIO_DMA_4K:
	default:
		break;
	}

	p_cfg->blk_size_cnt = tmp_reg;
} /* end of sdhost_set_data_param */

/*********************************************************************
**	Description :
**		This function used to get dma transfer information.
**	Param :
**		p_cfg			: sdio control reg.
**		blk_size			: get block size.
**		blk_cnt			: get block count.
**		dma_addr		: get dma address.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_get_data_param (sdio_reg_ptr p_cfg,
								uint32 *blk_size,
								uint32 *blk_cnt,
								uint32 *dma_addr)
{
	uint32 size_cnt;

	size_cnt = p_cfg->blk_size_cnt;

	if ((size_cnt & BIT_15) != 0) {
		*blk_size = 0x1000;
	}
	else {
		*blk_size = size_cnt & 0xFFF;
	}

	*blk_cnt  = (size_cnt & 0xFFFF0000) >>16;
	*dma_addr = p_cfg->dma_addr;
}

/*********************************************************************
**	Description :
**		This function used to set start address of dma buffer.
**	Param :
**		p_cfg			: sdio control reg.
**		dma_addr		: set dma address.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_dma_addr (sdio_reg_ptr p_cfg, uint32 dma_addr)
{
	p_cfg->dma_addr = dma_addr;
}

/*********************************************************************
**	Description :
**		This function used to get start address of dma buffer.
**	Param :
**		p_cfg			: sdio control reg.
**	Return :
**		dma address.
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_get_dma_addr (sdio_reg_ptr p_cfg)
{
	return p_cfg->dma_addr;
}

/*********************************************************************
**	Description :
**		This function used to set the argument of command.
**	Param :
**		p_cfg			: sdio control reg.
**		argument			: send to card argument.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/05			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_cmd_argu (sdio_reg_ptr p_cfg, uint32 argument)
{
	p_cfg->cmd_argu = argument;
}

/*********************************************************************
**	Description :
**		This function used to set the mode of command.
**	Param :
**		p_cfg			: sdio control reg
**		cmd_index		: cmd index
**		transmode		: SDIO_BOOT_ACK
**						  SDIO_CMD_LINE_BOOT
**						  SDIO_DATA_PRE
**						  SDIO_TRANS_DIS_AUTO
**						  SDIO_TRANS_AUTO_CMD12_EN
**						  SDIO_TRANS_AUTO_CMD23_EN
**						  SDIO_TRANS_COMP_ATA
**						  SDIO_TRANS_MULTIBLK
**						  SDIO_TRANS_DIR_READ
**						  SDIO_TRANS_BLK_CNT_EN
**						  SDIO_TRANS_DMA_EN
**		cmd_type			: SDIO_CMD_TYPE_NML
**						  SDIO_CMD_TYPE_SUSPEND
**						  SDIO_CMD_TYPE_RESUME
**						  SDIO_CMD_TYPE_ABORT
**		response			: SDIO_NO_RSP
**						  SDIO_R1
**						  SDIO_R2
**						  SDIO_R3
**						  SDIO_R4
**						  SDIO_R5
**						  SDIO_R6
**						  SDIO_R7
**						  SDIO_R1B
**						  SDIO_R5B
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/06			ypxie				create
*********************************************************************/
PUBLIC void sdhost_set_cmd (sdio_reg_ptr p_cfg,
						uint32 cmd_index,
						uint32 transmode,
						uint32 cmd_type,
						uint32 response)
{
	volatile uint32 tmp_reg;

	/* tmp_reg &= (~(0x7F | 0x30000 | 0x3FF80000)); */
	tmp_reg &= ~(0x7F | 0x30000 | 0x3FF80700 | BIT_30 | BIT_31);

	/* [transmode] bit[6:0], bit[21], bit[30], bit[31] */
	/* bit[3], bit[30], bit[31] is used by v30 */
	tmp_reg |= transmode & (0x7F | BIT_21 | BIT_30 | BIT_31);

	/* [cmd_type] bit[23:22] */
	tmp_reg |= cmd_type & (BIT_22 | BIT_23);

	/* [response] bit[17:16], bit[20], bit[19] */
	tmp_reg |= response & (BIT_16 | BIT_17 | BIT_19 | BIT_20);

	/* [cmd_index] bit[29:24] */
	tmp_reg |= (cmd_index & 0x3F) << 24;

	p_cfg->tr_mode = tmp_reg;
} /* end of sdhost_set_cmd */

/*********************************************************************
**	Description :
**		This function used to get content from host response register.
**	Param :
**		p_cfg			: sdio control reg
**		response			: SDIO_NO_RSP
**						  SDIO_R1
**						  SDIO_R2
**						  SDIO_R3
**						  SDIO_R4
**						  SDIO_R5
**						  SDIO_R6
**						  SDIO_R7
**						  SDIO_R1B
**						  SDIO_R5B
**		rsp_buf			: store response buffer
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/06			ypxie				create
*********************************************************************/
PUBLIC void sdhost_get_response (sdio_reg_ptr p_cfg, uint32 response, uint8 *rsp_buf)
{
	uint32 tmp_buf[4];
	uint32 i;

	tmp_buf[0] = p_cfg->resp0;
	tmp_buf[1] = p_cfg->resp1;
	tmp_buf[2] = p_cfg->resp2;
	tmp_buf[3] = p_cfg->resp3;

	switch (response) {
	case SDIO_R1:
	case SDIO_R1B:
	case SDIO_R3:
	case SDIO_R4:
	case SDIO_R5:
	case SDIO_R6:
	case SDIO_R7:
	case SDIO_R5B:
		rsp_buf[0] = (uint8) ( (tmp_buf[0] >> 24) & 0xFF);
		rsp_buf[1] = (uint8) ( (tmp_buf[0] >> 16) & 0xFF);
		rsp_buf[2] = (uint8) ( (tmp_buf[0] >> 8) & 0xFF);
		rsp_buf[3] = (uint8) ( tmp_buf[0] & 0xFF);
		break;

	case SDIO_R2:
		rsp_buf[0] = (uint8) ( (tmp_buf[3] >> 16) & 0xFF);
		rsp_buf[1] = (uint8) ( (tmp_buf[3] >> 8) & 0xFF);
		rsp_buf[2] = (uint8) (tmp_buf[3] & 0xFF);

		rsp_buf[3] = (uint8) ( (tmp_buf[2] >> 24) & 0xFF);
		rsp_buf[4] = (uint8) ( (tmp_buf[2] >> 16) & 0xFF);
		rsp_buf[5] = (uint8) ( (tmp_buf[2] >> 8) & 0xFF);
		rsp_buf[6] = (uint8) (tmp_buf[2] & 0xFF);

		rsp_buf[7] = (uint8) ( (tmp_buf[1] >> 24) & 0xFF);
		rsp_buf[8] = (uint8) ( (tmp_buf[1] >> 16) & 0xFF);
		rsp_buf[9] = (uint8) ( (tmp_buf[1] >> 8) & 0xFF);
		rsp_buf[10] = (uint8) (tmp_buf[1] & 0xFF);

		rsp_buf[11] = (uint8) ( (tmp_buf[0] >> 24) & 0xFF);
		rsp_buf[12] = (uint8) ( (tmp_buf[0] >> 16) & 0xFF);
		rsp_buf[13] = (uint8) ( (tmp_buf[0] >> 8) & 0xFF);
		rsp_buf[14] = (uint8) (tmp_buf[0] & 0xFF);
		break;

	case SDIO_NO_RSP:
	default:
		break;
	}
}/* end of sdhost_get_response */

/*********************************************************************
**	Description :
**		This function used to get function that host can be support.
**	Param :
**		p_cfg			: sdio control reg
**		cap_buf			: size is 3*32bits.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/06			ypxie				create
*********************************************************************/
LOCAL void sdhost_get_capability (sdio_reg_ptr p_cfg, uint32 *cap_buf)
{
	cap_buf[0] = p_cfg->cap1;
	cap_buf[1] = p_cfg->cap2;
	cap_buf[2] = p_cfg->cur_cap1;
}

/*********************************************************************
**	Description :
**		This function used to clear Normal int Status register ,if event is happed ,host will set
**		status in register.
**	Param :
**		p_cfg			: sdio control reg
**		msg				: int status bit.
**						  INT_CMD_CMPLT					BIT_0
**						  INT_TR_CMPLT					BIT_1
**						  INT_CAP_EVENT					BIT_2
**						  INT_DMA_INT					BIT_3
**						  INT_BUF_WR_RDY				BIT_4
**						  INT_BUF_RD_RDY					BIT_5
**						  INT_CARD_INT					BIT_8
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/09			ypxie				create
*********************************************************************/
PUBLIC void sdhost_int_st_clr (sdio_reg_ptr p_cfg, uint32 msg)
{
	volatile uint32 tmp_reg;

	tmp_reg = 0;
	/* if err, clr all error bit. */
	if (0 != (msg & INT_ERR_INT)) {
		tmp_reg |= (INT_CMD_TIMEOUT | INT_CMD_CRC | INT_CMD_END_BIT
					| INT_CMD_IND | INT_DATA_TIMEOUT | INT_DATA_CRC
					| INT_DATA_END_BIT | INT_CUR_LMT | INT_AUTO_CMD12
					| INT_TRGT_RESP);
	}

	tmp_reg |= msg;
	p_cfg->int_st = tmp_reg;
} /* end of sdhost_int_st_clr */

/*********************************************************************
**	Description :
**		This function used to get interrupt status.
**	Param :
**		p_cfg			: sdio control reg
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/09			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_get_int_st (sdio_reg_ptr p_cfg)
{
	volatile uint32 tmp_reg;

	tmp_reg = p_cfg->int_st;

	return tmp_reg;
}

/*********************************************************************
**	Description :
**		This function used to enable interrupt.
**	Param :
**		p_cfg			: sdio control reg
**		err_msg			: int err bit status.
**						  INT_CMD_TIMEOUT				BIT_16
**						  INT_CMD_CRC					BIT_17
**						  INT_CMD_END_BIT				BIT_18
**						  INT_CMD_IND					BIT_19
**						  INT_DATA_TIMEOUT				BIT_20
**						  INT_DATA_CRC					BIT_21
**						  INT_DATA_END_BIT				BIT_22
**						  INT_CUR_LMT					BIT_23
**						  INT_AUTO_CMD12				BIT_24
**						  INT_TRGT_RESP					BIT_28
**		msg				: int status bit.
**						  INT_CMD_CMPLT					BIT_0
**						  INT_TR_CMPLT					BIT_1
**						  INT_CAP_EVENT					BIT_2
**						  INT_DMA_INT					BIT_3
**						  INT_BUF_WR_RDY				BIT_4
**						  INT_BUF_RD_RDY					BIT_5
**						  INT_CARD_INT					BIT_8
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/09			ypxie				create
*********************************************************************/
PUBLIC void sdhost_int_st_sig_en (sdio_reg_ptr p_cfg, uint32 msg)
{
	volatile uint32 tmp_reg;

	tmp_reg = 0;

	/* enable error bit. */
	/* enable status and err int bit*/
	tmp_reg |= msg;

	p_cfg->int_st_en |= tmp_reg;
	p_cfg->int_sig_en |= tmp_reg;
}

PUBLIC void sdhost_int_st_sig_dis (sdio_reg_ptr p_cfg, uint32 msg)
{
	volatile uint32 tmp_reg;

	tmp_reg = 0;

	/* clr all error bit. */
	if (0 != (msg & INT_ERR_INT)) {
		tmp_reg |= (INT_CMD_TIMEOUT | INT_CMD_CRC | INT_CMD_END_BIT
					| INT_CMD_IND | INT_DATA_TIMEOUT | INT_DATA_CRC
					| INT_DATA_END_BIT | INT_CUR_LMT | INT_AUTO_CMD12
					| INT_TRGT_RESP);
	}

	/* clr msg bit. */
	tmp_reg |= msg;

	p_cfg->int_st_en &= ~tmp_reg;
	p_cfg->int_sig_en &= ~tmp_reg;
}

/*********************************************************************
**	Description :
**		This function used to slot select.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/10			ypxie				create
*********************************************************************/
PUBLIC void sdhost_slot_select (uint32 slot_no)
{
	switch (slot_no) {
	case 0 :
		REG32(AHB_SDIO_CTRL) = SLOT0_SEL;
		break;
	case 1 :
		REG32(AHB_SDIO_CTRL) = SLOT1_SEL;
		break;
	case 2 :
		REG32(AHB_SDIO_CTRL) = SLOT2_SEL;
		break;
	case 3 :
		REG32(AHB_SDIO_CTRL) = SLOT3_SEL;
		break;
	case 4 :
		REG32(AHB_SDIO_CTRL) = SLOT4_SEL;
		break;
	case 5 :
		REG32(AHB_SDIO_CTRL) = SLOT5_SEL;
		break;
	case 6 :
		REG32(AHB_SDIO_CTRL) = SLOT6_SEL;
		break;
	case 7 :
		REG32(AHB_SDIO_CTRL) = SLOT7_SEL;
		break;
	case 8 :
		REG32(AHB_SDIO_CTRL) = SLOT8_SEL;
		break;
	case 9 :
		REG32(AHB_SDIO_CTRL) = SLOT9_SEL;
		break;
	case 10 :
		REG32(AHB_SDIO_CTRL) = SLOT10_SEL;
		break;
	case 11 :
		REG32(AHB_SDIO_CTRL) = SLOT11_SEL;
		break;
	default :
		break;
	}
} /* end of sdhost_slot_select */

/*********************************************************************
**	Description :
**		This function used to get emmc controller register base address.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**	Return :
**		reg_addr			: sdio base register address
**	Date					Author				Operation
**	2013/09/11			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_get_base_addr (uint32 slot_no)
{
	uint32 reg_addr = 0;

	switch (slot_no) {
	case 0 :
		reg_addr = SLOT0_BASE_ADDR;
		break;
	case 1 :
		reg_addr = SLOT1_BASE_ADDR;
		break;
	case 2 :
		reg_addr = SLOT2_BASE_ADDR;
		break;
	case 3 :
		reg_addr = SLOT3_BASE_ADDR;
		break;
	case 4 :
		reg_addr = SLOT4_BASE_ADDR;
		break;
	case 5 :
		reg_addr = SLOT5_BASE_ADDR;
		break;
	case 6 :
		reg_addr = SLOT6_BASE_ADDR;
		break;
	case 7 :
		reg_addr = SLOT7_BASE_ADDR;
		break;
	case 8 :
		reg_addr = SLOT8_BASE_ADDR;
		break;
	case 9 :
		reg_addr = SLOT9_BASE_ADDR;
		break;
	case 10 :
		reg_addr = SLOT10_BASE_ADDR;
		break;
	case 11 :
		reg_addr = SLOT11_BASE_ADDR;
		break;
	default :
		break;
	}

	return reg_addr;
} /* end of sdhost_get_base_addr */

/*********************************************************************
**	Description :
**		This function used to enable sdio clock in ahb.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/11			ypxie				create
*********************************************************************/
PUBLIC void sdhost_ahb_sdio_en (uint32 slot_no)
{
	uint32 id;
	sdio_base_info_t *p_tb;

	p_tb = sdio_resource_detail;
	id = slot_no / 3;

	REG32(p_tb[id].ahb_en) |= p_tb[id].ahb_en_bit;
}

/*********************************************************************
**	Description :
**		This function used to rst sdio controller in ahb.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/11			ypxie				create
*********************************************************************/
PUBLIC void sdhost_ahb_sdio_rst (uint32 slot_no)
{
	uint32 id;
	sdio_base_info_t *p_tb;

	p_tb = sdio_resource_detail;
	id = slot_no / 3;

	REG32(p_tb[id].ahb_rst) |= p_tb[id].ahb_rst_bit;
	sdhost_delayus(200);
	REG32(p_tb[id].ahb_rst) &= ~(p_tb[id].ahb_rst_bit);
}

/*********************************************************************
**	Description :
**		This function used to set sdio base clock.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**		base_clk			: base clock value. must the same as the value in table .
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/11			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_set_base_clk (uint32 slot_no, uint32 base_clk)
{
	uint32 i,id, clk;
	sdio_base_info_t *p_tb;

	p_tb = sdio_resource_detail;
	id = slot_no / 3;
	clk = 0;

	REG32(p_tb[id].clk_reg) &= ~(p_tb[id].clk_mask);

	for (i = 0; i < 4; i++) {
		if (base_clk == p_tb[id].clk_sel[i].val) {
			REG32(p_tb[id].clk_reg) |= p_tb[id].clk_sel[i].step << p_tb[id].clk_shft;
			clk = base_clk;
		}
	}

	return clk;
}

/*********************************************************************
**	Description :
**		This function used to set sdio base clock.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**		pwr_flg			: on or off flag.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/11			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_set_power (uint32 slot_no, sdio_onoff_e pwr_flg)
{
	uint32 i, id;
	sdio_base_info_t *p_tb;

	p_tb = sdio_resource_detail;
	id = slot_no / 3;

	if (pwr_flg == SDIO_ON) {
		if (p_tb[id].pwr_io.pd_set != 0) {
			REG32(p_tb[id].pwr_io.pd_set) &= ~(p_tb[id].pwr_io.pd_set_bit);

			if (p_tb[id].pwr_io.pd_clr != 0) {
				REG32(p_tb[id].pwr_io.pd_clr) |= (p_tb[id].pwr_io.pd_clr_bit);
			}
		}

		if (p_tb[id].pwr_core.pd_set != 0) {
			REG32(p_tb[id].pwr_core.pd_set) &= ~(p_tb[id].pwr_core.pd_set_bit);

			if (p_tb[id].pwr_core.pd_clr != 0) {
				REG32(p_tb[id].pwr_core.pd_clr) |= (p_tb[id].pwr_core.pd_clr_bit);
			}
		}
	}
	else if (pwr_flg == SDIO_OFF) {
		if (p_tb[id].pwr_io.pd_set != 0) {
			REG32(p_tb[id].pwr_io.pd_set) |= (p_tb[id].pwr_io.pd_set_bit);

			if (p_tb[id].pwr_io.pd_clr != 0) {
				REG32(p_tb[id].pwr_io.pd_clr) &= ~(p_tb[id].pwr_io.pd_clr_bit);
			}
		}

		if (p_tb[id].pwr_core.pd_set != 0) {
			REG32(p_tb[id].pwr_core.pd_set) |= (p_tb[id].pwr_core.pd_set_bit);

			if (p_tb[id].pwr_core.pd_clr != 0) {
				REG32(p_tb[id].pwr_core.pd_clr) &= ~(p_tb[id].pwr_core.pd_clr_bit);
			}
		}
	}
} /* end of sdhost_set_power */

/*********************************************************************
**	Description :
**		This function used to select sdio power_io, power_core voltage.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**		io_volt			: io voltage, unit: mV.
**		core_volt			: core voltage, unit: mV. if not, set 0.
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/11			ypxie				create
*********************************************************************/
PUBLIC uint32 sdhost_set_voltage (uint32 slot_no, uint32 io_volt, uint32 core_volt)
{
	uint32 i, id;
	sdio_base_info_t *p_tb;
	sdio_pwr_info_t *pwr;

	p_tb = sdio_resource_detail;
	id = slot_no / 3;

	pwr = &(p_tb[id].pwr_io);
	if ( pwr->pwr_sel_reg != 0 ) {
		REG32(pwr->pwr_sel_reg) &= ~(pwr->pwr_mask);

		for (i = 0; i < 4; i++) {
			if (io_volt == pwr->pwr_sel[i].val) {
				REG32(pwr->pwr_sel_reg) |= pwr->pwr_sel[i].step << pwr->pwr_shft;
			}
		}
	}

	pwr = &(p_tb[id].pwr_core);
	if ( pwr->pwr_sel_reg != 0 ) {
		REG32(pwr->pwr_sel_reg) &= ~(pwr->pwr_mask);

		for (i = 0; i < 4; i++) {
			if (core_volt == pwr->pwr_sel[i].val) {
				REG32(pwr->pwr_sel_reg) |= pwr->pwr_sel[i].step << pwr->pwr_shft;
			}
		}
	}
} /* end of sdhost_set_voltage */

/*********************************************************************
**	Description :
**		This function used to get sdio controller version.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**	Return :
**		sdio_ver			: sdio version.
**	Date					Author				Operation
**	2013/09/22			ypxie				create
*********************************************************************/
LOCAL uint32 sdhost_get_sdio_version(uint32 slot_no)
{
	uint32 id;

	id = slot_no/3;

	if (id == 0) {
		return SDIO0_VER;
	}
	else if (id == 1) {
		return SDIO1_VER;
	}
	else if (id == 2) {
		return SDIO2_VER;
	}
	else if (id == 3) {
		return SDIO3_VER;
	}
}

/*********************************************************************
**	Description :
**		This function used to select sdio power_io, power_core voltage.
**	Param :
**		slot_no			: select slot number.
**						  S1	,S2	,slave		sdio controller
**						  0	,1	,2			SDIO0 (SD)
**						  3	,4	,5			SDIO1 (WIFI)
**						  6	,7	,8			SDIO2 (MASTER)
**						  9	,10	,11			SDIO3 (EMMC)
**	Return :
**		none
**	Date					Author				Operation
**	2013/09/12			ypxie				create
*********************************************************************/
LOCAL void sdhost_pin_select(uint32 slot_no)
{
	uint32 id;

	id = slot_no / 3;
}