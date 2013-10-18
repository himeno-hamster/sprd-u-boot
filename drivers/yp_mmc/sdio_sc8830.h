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
 ** 09/09/2013		ypxie				Create.
 ********************************************************************/
#ifndef __SDIO_CHIP_H_
#define __SDIO_CHIP_H_

/* sdio register base address */
#define SLOT0_BASE_ADDR					CTL_BASE_SDIO0
#define SLOT1_BASE_ADDR					(CTL_BASE_SDIO0 + 0x0100)
#define SLOT2_BASE_ADDR					0					/* not support */
#define SLOT3_BASE_ADDR					CTL_BASE_SDIO1
#define SLOT4_BASE_ADDR					(CTL_BASE_SDIO1 + 0x0100)
#define SLOT5_BASE_ADDR					(CTL_BASE_SDIO1 + 0x0200)
#define SLOT6_BASE_ADDR					CTL_BASE_SDIO2
#define SLOT7_BASE_ADDR					(CTL_BASE_SDIO2 + 0x0100)
#define SLOT8_BASE_ADDR					(CTL_BASE_SDIO2 + 0x0200)
#define SLOT9_BASE_ADDR					CTL_BASE_EMMC
#define SLOT10_BASE_ADDR				(CTL_BASE_SDIO2 + 0x0100)
#define SLOT11_BASE_ADDR				0					/* not support */

/* sdio controller version information */
#define SDIO_20							20
#define SDIO_30							30

#define SDIO0_VER						SDIO_30
#define SDIO1_VER						SDIO_20
#define SDIO2_VER						SDIO_20
#define SDIO3_VER						SDIO_30

/* sdio slot register */
#define AHB_SDIO_CTRL					(SPRD_AHB_PHYS + 0x0018)

#define SLOT0_SEL						& (~BIT_16)
#define SLOT1_SEL						| BIT_16
#define SLOT2_SEL						& 0					/* not support */
#define SLOT3_SEL						& (~(BIT_1 | BIT_0))
#define SLOT4_SEL						(REG32(AHB_SDIO_CTRL) & (~BIT_1)) | BIT_0
#define SLOT5_SEL						(REG32(AHB_SDIO_CTRL) & (~BIT_0)) | BIT_1
#define SLOT6_SEL						& (~(BIT_3 | BIT_2))
#define SLOT7_SEL						(REG32(AHB_SDIO_CTRL) & (~BIT_3)) | BIT_2
#define SLOT8_SEL						(REG32(AHB_SDIO_CTRL) & (~BIT_2)) | BIT_3
#define SLOT9_SEL						& (~BIT_17)
#define SLOT10_SEL						| BIT_17
#define SLOT11_SEL						& 0					/* not support */

/* sdio ahb clock enable : 0x20D00000*/
/* AHB_EB */
/* AHB_SOFT_RST */
/* sdio ahb base clock select */
/* REG_AP_CLK_SDIO0_CFG */
/* REG_AP_CLK_SDIO1_CFG */
/* REG_AP_CLK_SDIO2_CFG */
/* REG_AP_CLK_EMMC_CFG */
/* sdio ahb power on or off */
/* sdio ahb power sel */
typedef struct
{
	uint32								val;
	uint32								set_val;
} sel_info_t;

typedef struct
{
/* power enable info */
	uint32								pd_set;
	uint32								pd_set_bit;
	uint32								pd_clr;
	uint32								pd_clr_bit;
/* power select info */
	uint32								pwr_sel_reg;
	uint32								pwr_mask;
	uint32								pwr_shft;
	sel_info_t								pwr_sel[4];
} sdio_pwr_info_t;

typedef struct
{
	uint32								sdio_index;
/* enable clock reigster info */
	uint32								ahb_en;
	uint32								ahb_en_bit;
/* reset clock reigster info */
	uint32								ahb_rst;
	uint32								ahb_rst_bit;
/* base clock reigster info */
	uint32								clk_reg;
	uint32								clk_mask;
	uint32								clk_shft;
	sel_info_t								clk_sel[4];

	sdio_pwr_info_t						pwr_io;
	sdio_pwr_info_t						pwr_core;
} sdio_base_info_t;

LOCAL const sdio_base_info_t sdio_resource_detail[]=
{
/*
	{sdio_index, ahb_en, ahb_en_bit, ahb_rst, ahb_rst_bit, clk_reg, clk_mask
		, clk_shft, val, step, val, step, val, step, val, step
		, pd_set, pd_set_bit
		, pd_clr, pd_clr_bit
		, pwr_sel_reg, pwr_mask
		, pwr_shft, val, step, val, step, val, step, val, step
		, pd_set, pd_set_bit
		, pd_clr, pd_clr_bit
		, pwr_sel, pwr_mask
		, pwr_shft, val, step, val, step, val, step, val, step },
*/
	{0, AHB_EB, BIT_8, AHB_SOFT_RST, BIT_11,REG_AP_CLK_SDIO0_CFG, (BIT_1 | BIT_0)
		, 0, 26000000, 0x00, 192000000, 0x01, 256000000, 0x02, 312000000, 0x03
		, ANA_REG_GLB_LDO_PD_CTRL, BIT_1
		, 0, 0
		, ANA_REG_GLB_LDO_V_CTRL1, (BIT_3 | BIT_2)
		, 2, 2800, 0x00, 3000, 0x01, 2500, 0x02, 1800, 0x03
		, 0, 0
		, 0, 0
		, 0, 0
		, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

	{1, AHB_EB, BIT_9, AHB_SOFT_RST, BIT_12,REG_AP_CLK_SDIO1_CFG, (BIT_1 | BIT_0)
		, 0, 48000000, 0x00, 76800000, 0x01, 96000000, 0x02, 128000000, 0x03
		, ANA_REG_GLB_LDO_DCDC_PD_RTCSET, BIT_1			/* used vdd18 */
		, ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, BIT_1
		, ANA_REG_GLB_LDO_V_CTRL0, (BIT_1 | BIT_0)
		, 0x00, 1500, 0x00, 1800, 0x01, 1300, 0x02, 1200, 0x03
		, 0, 0
		, 0, 0
		, 0, 0
		, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{2, AHB_EB, BIT_10, AHB_SOFT_RST, BIT_13,REG_AP_CLK_SDIO2_CFG, (BIT_1 | BIT_0)
		, 0, 48000000, 0x00, 76800000, 0x01, 96000000, 0x02, 128000000, 0x03
		, ANA_REG_GLB_LDO_DCDC_PD_RTCSET, BIT_1			/* used vdd18 */
		, ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, BIT_1
		, ANA_REG_GLB_LDO_V_CTRL0, (BIT_1 | BIT_0)
		, 0x00, 1500, 0x00, 1800, 0x01, 1300, 0x02, 1200, 0x03
		, 0, 0
		, 0, 0
		, 0, 0
		, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{3, AHB_EB, BIT_11, AHB_SOFT_RST, BIT_14,REG_AP_CLK_EMMC_CFG, (BIT_1 | BIT_0)
		, 0, 26000000, 0x00, 192000000, 0x01, 256000000, 0x02, 312000000, 0x03
		, ANA_REG_GLB_LDO_DCDC_PD_RTCSET, BIT_7
		, ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, BIT_7
		, ANA_REG_GLB_LDO_V_CTRL0, (BIT_13 | BIT_12)
		, 12, 1500, 0x00, 1800, 0x01, 1300, 0x02, 1200, 0x03
		, ANA_REG_GLB_LDO_DCDC_PD_RTCSET, BIT_8
		, ANA_REG_GLB_LDO_DCDC_PD_RTCCLR, BIT_8
		, ANA_REG_GLB_LDO_V_CTRL0, (BIT_15 | BIT_14)
		, 14, 2800, 0x00, 3000, 0x01, 2500, 0x02, 1800, 0x03 },
};

#endif /* __SDIO_CHIP_H_ */