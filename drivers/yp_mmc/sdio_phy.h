/*********************************************************************
 ** File Name:		sdio_phy.h
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
#ifndef __SDIO_PHY_H_
#define __SDIO_PHY_H_

/********************************************************************/
/*****************      global information      ********************************/
/********************************************************************/

typedef enum sdio_onoff_e_tag
{
	SDIO_OFF,
	SDIO_ON
} sdio_onoff_e;

typedef enum sdio_buswidth_e_tag
{
	SDIO_BUS_1_BIT,
	SDIO_BUS_4_BIT,
	SDIO_BUS_8_BIT
} sdio_buswidth_e;

typedef enum sdio_speedmode_e_tag
{
	SDIO_LOWSPEED,
	SDIO_HIGHSPEED,
	SDIO_SDR13,
	SDIO_SDR26,
	SDIO_SDR52,
	SDIO_SDR104,
	SDIO_DDR52,
	SDIO_HS200
} sdio_speedmode_e;

typedef enum sdio_rst_type_e_tag
{
	RST_CMD_LINE,
	RST_DAT_LINE,
	RST_CMD_DAT_LINE,
	RST_ALL,
	RST_MODULE
} sdio_rst_type_e;

typedef enum sdio_dma_size_e_tag
{
	SDIO_DMA_4K,
	SDIO_DMA_8K,
	SDIO_DMA_16K,
	SDIO_DMA_32K,
	SDIO_DMA_64K,
	SDIO_DMA_128K,
	SDIO_DMA_256K,
	SDIO_DMA_512K
} sdio_dma_size_e;

//typedef sdio_reg_v20_t	sdio_reg_t, *sdio_reg_ptr;

#define SLOT_MAX_NUM					12






#endif /* __SDIO_PHY_H_ */
