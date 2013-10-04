/******************************************************************************
 ** File Name:      ldo_drv.c                                             *
 ** Author:         Yi.Qiu                                                 *
 ** DATE:           01/09/2009                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic function for ldo management.  *
 ******************************************************************************/

/******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 01/09/2009     Yi.Qiu        Create.                                   *
 ******************************************************************************/

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include <common.h>
#include <asm/io.h>

#include <asm/arch/ldo.h>
#include <asm/arch/chip_drv_common_io.h>
#include <asm/arch/sprd_reg.h>

#define LDO_INVALID_REG	0xFFFFFFFF
#define LDO_INVALID_BIT	0xFFFFFFFF


#define CURRENT_STATUS_INIT	0x00000001
#define CURRENT_STATUS_ON	0x00000002
#define CURRENT_STATUS_OFF	0x00000004

#define LDO_INVALID_REG_ADDR	0x1

#define SCI_PASSERT(condition, format...)  \
	do {		\
		if(!(condition)) { \
			printf("function :%s\r\n", __FUNCTION__);\
		} \
	}while(0)

#if defined(CONFIG_SPX15)
struct {
	LDO_ID_E id;
	const char *name;
}__ldo_names[] = {
	{
		.id = LDO_LDO_USB, .name = "vddusb",
	},
	{
		.id = LDO_LDO_EMMCCORE, .name = "vddemmccore",
	},
	{
		.id = LDO_LDO_EMMCIO, .name = "vddemmcio",
	},
	{
		.id = LDO_LDO_SDIO3, .name = "vddsd",
	},
	{
		.id = LDO_LDO_SIM0, .name = "vddsim0",
	},
	{
		.id = LDO_LDO_SIM1, .name = "vddsim1",
	},
	{
		.id = LDO_LDO_SIM2, .name = "vddsim2",
	},
};

const char* __LDO_NAME(LDO_ID_E ldo_id)
{
	int i = 0;
	for(i = 0; i < ARRAY_SIZE(__ldo_names); i++) {
		if (ldo_id == __ldo_names[i].id)
			return __ldo_names[i].name;
	}
	return NULL;
}

int LDO_Init(void)
{
	return regulator_init();
}

void LDO_TurnOffAllLDO(void)
{
	regulator_disable_all();
}

LDO_ERR_E LDO_TurnOffLDO(LDO_ID_E ldo_id)
{
	return regulator_disable(__LDO_NAME(ldo_id));
}

LDO_ERR_E LDO_TurnOnLDO(LDO_ID_E ldo_id)
{
	return regulator_enable(__LDO_NAME(ldo_id));
}

#else

struct ldo_ctl_info {
	/**
	  need config area
	 */
	LDO_ID_E id;
	unsigned int bp_reg;
	unsigned int bp_bits;
	unsigned int bp_rst_reg;
	unsigned int bp_rst;//bits

	unsigned int level_reg_b0;
	unsigned int b0;
	unsigned int b0_rst;

	unsigned int level_reg_b1;
	unsigned int b1;
	unsigned int b1_rst;

	unsigned int init_level;
	/**
	  not need config area
	 */
	int ref;
	int current_status;
	int current_volt_level;
};

static struct ldo_ctl_info ldo_ctl_data[] =
{
	{
		.id           = LDO_LDO_EMMCCORE,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_8,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_8,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_EMMCIO,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_7,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_7,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_12,
		.b0_rst       = BIT_12,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_13,
		.b1_rst       = BIT_13,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_RF2,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_6,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_6,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_10,
		.b0_rst       = BIT_10,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_11,
		.b1_rst       = BIT_11,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_RF1,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_5,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_5,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_8,
		.b0_rst       = BIT_8,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_9,
		.b1_rst       = BIT_9,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_RF0,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_4,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_4,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_6,
		.b0_rst       = BIT_6,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_7,
		.b1_rst       = BIT_7,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_VDD25,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_3,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_3,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_4,
		.b0_rst       = BIT_4,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_5,
		.b1_rst       = BIT_5,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_VDD28,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_2,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_2,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_2,
		.b0_rst       = BIT_2,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_3,
		.b1_rst       = BIT_3,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_VDD18,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_1,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_1,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL0,
		.b0           = BIT_0,
		.b0_rst       = BIT_0,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL0,
		.b1           = BIT_1,
		.b1_rst       = BIT_1,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_SIM2,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_4,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_4,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL1,
		.b0           = BIT_8,
		.b0_rst       = BIT_8,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL1,
		.b1           = BIT_9,
		.b1_rst       = BIT_9,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_SIM1,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_3,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_3,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL1,
		.b0           = BIT_6,
		.b0_rst       = BIT_6,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL1,
		.b1           = BIT_7,
		.b1_rst       = BIT_7,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_SIM0,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_2,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_2,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL1,
		.b0           = BIT_4,
		.b0_rst       = BIT_4,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL1,
		.b1           = BIT_5,
		.b1_rst       = BIT_5,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_SDIO0,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_1,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_1,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL1,
		.b0           = BIT_2,
		.b0_rst       = BIT_2,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL1,
		.b1           = BIT_3,
		.b1_rst       = BIT_3,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_AVDD18,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_0,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_0,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL1,
		.b0           = BIT_0,
		.b0_rst       = BIT_0,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL1,
		.b1           = BIT_1,
		.b1_rst       = BIT_1,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_VBAT_RES,		//new
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_0,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_0,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_VBAT_V,			//new
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_0,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_0,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_12,
		.b0_rst       = BIT_12,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_13,
		.b1_rst       = BIT_13,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_CLSG,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_10,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_10,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_10,
		.b0_rst       = BIT_10,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_11,
		.b1_rst       = BIT_11,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_USB,
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_9,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_9,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_8,
		.b0_rst       = BIT_8,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_9,
		.b1_rst       = BIT_9,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_CAMM,  //LDO_LDO_CAMMOT
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_8,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_8,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_6,
		.b0_rst       = BIT_6,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_7,
		.b1_rst       = BIT_7,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_CAMD1,  //LDO_LDO_CAMIO
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_7,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_7,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_4,
		.b0_rst       = BIT_4,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_5,
		.b1_rst       = BIT_5,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_CAMD0,  //LDO_LDO_CAMCORE
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_6,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_6,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_2,
		.b0_rst       = BIT_2,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_3,
		.b1_rst       = BIT_3,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_CAMA,  //LDO_LDO_CAMA
		.bp_reg       = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_bits      = BIT_5,
		.bp_rst_reg   = ANA_REG_GLB_LDO_PD_CTRL,
		.bp_rst       = BIT_5,
		.level_reg_b0 = ANA_REG_GLB_LDO_V_CTRL2,
		.b0           = BIT_0,
		.b0_rst       = BIT_0,
		.level_reg_b1 = ANA_REG_GLB_LDO_V_CTRL2,
		.b1           = BIT_1,
		.b1_rst       = BIT_1,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
/**                config it later               **/
	{
		.id           = LDO_DCDCARM,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_ABB,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_MEM,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_11,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_11,
		.level_reg_b0 = ANA_REG_GLB_DCDC_MEM_ADI,
		.b0           = BIT_5,
		.b0_rst       = BIT_5,
		.level_reg_b1 = ANA_REG_GLB_DCDC_MEM_ADI,
		.b1           = BIT_6,
		.b1_rst       = BIT_6,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_DCDC,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_BG,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_AVB,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_WIF1,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_WIF0,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_SDIO1,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_RTC,
		.bp_reg       = ANA_REG_GLB_LDO_DCDC_PD_RTCSET,
		.bp_bits      = BIT_9,
		.bp_rst_reg   = ANA_REG_GLB_LDO_DCDC_PD_RTCCLR,
		.bp_rst       = BIT_9,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_USBD,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id           = LDO_LDO_VDD30,
		.bp_reg       = LDO_INVALID_REG_ADDR,
		.bp_bits      = 0,
		.bp_rst_reg   = LDO_INVALID_REG_ADDR,
		.bp_rst       = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0           = BIT_14,
		.b0_rst       = BIT_14,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1           = BIT_15,
		.b1_rst       = BIT_15,
		.init_level   = LDO_VOLT_LEVEL_FAULT_MAX,
	},
};

/**---------------------------------------------------------------------------*
 **                         Function Declaration                              *
 **---------------------------------------------------------------------------*/
static struct ldo_ctl_info* LDO_GetLdoCtl(LDO_ID_E ldo_id)
{
	int i = 0;
	struct ldo_ctl_info* ctl = NULL;

	for ( i = 0; i < ARRAY_SIZE(ldo_ctl_data); ++i)
	{
		if (ldo_ctl_data[i].id == ldo_id)
		{
			ctl = &ldo_ctl_data[i];
			break;
		}
	}

	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));
	return ctl;
}

LDO_ERR_E LDO_TurnOnLDO(LDO_ID_E ldo_id)
{
	struct ldo_ctl_info* ctl = NULL;

	if (ldo_id == LDO_LDO_SDIO3)
	{
		if (LDO_TurnOnLDO(LDO_LDO_EMMCCORE) != LDO_ERR_OK)
			return LDO_ERR_ERR;
		if (LDO_TurnOnLDO(LDO_LDO_EMMCIO)   != LDO_ERR_OK)
			return LDO_ERR_ERR;
		return LDO_ERR_OK;
	}

	ctl = LDO_GetLdoCtl(ldo_id);
	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));

	//if ((ctl->ref++) == 0)
	{
		if(ctl->bp_reg == LDO_INVALID_REG_ADDR)
		{
			//if (LDO_LDO_USBD == ldo_id)
			//	CHIP_REG_AND((~LDO_USB_PD), GR_CLK_GEN5);
			return LDO_ERR_ERR;
		}
		else
		{
			ANA_REG_OR (ctl->bp_rst_reg, ctl->bp_rst);
			ANA_REG_AND(ctl->bp_reg,     ~(ctl->bp_bits));
		}
		ctl->current_status = CURRENT_STATUS_ON;
	}
	return LDO_ERR_OK;
}

LDO_ERR_E LDO_TurnOffLDO(LDO_ID_E ldo_id)
{
	struct ldo_ctl_info* ctl = NULL;

	if (ldo_id == LDO_LDO_SDIO3)
	{
		if (LDO_TurnOffLDO(LDO_LDO_EMMCCORE) != LDO_ERR_OK)
			return LDO_ERR_ERR;
		if (LDO_TurnOffLDO(LDO_LDO_EMMCIO)   != LDO_ERR_OK)
			return LDO_ERR_ERR;
		return LDO_ERR_OK;
	}

	ctl = LDO_GetLdoCtl(ldo_id);
	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));

	//local_irq_save(flags);

	//if ((--ctl->ref) == 0)
	{
		if(ctl->bp_reg == LDO_INVALID_REG_ADDR)
		{
			//if (LDO_LDO_USBD == ldo_id)
			//	CHIP_REG_OR((LDO_USB_PD), GR_CLK_GEN5);
		}
		else
		{
			ANA_REG_AND(ctl->bp_rst_reg, (~(ctl->bp_rst)));
			ANA_REG_OR (ctl->bp_reg,     ctl->bp_bits);
		}
		ctl->current_status = CURRENT_STATUS_OFF;
	}

	//local_irq_restore(flags);

	return LDO_ERR_OK;
}

int LDO_IsLDOOn(LDO_ID_E ldo_id)
{
	unsigned int  masked_val = 0;
	struct ldo_ctl_info* ctl = NULL;

	ctl = LDO_GetLdoCtl(ldo_id);
	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));

	if (ctl->current_status == CURRENT_STATUS_INIT)
	{
		masked_val = (LDO_REG_GET(ctl->bp_reg) & ctl->bp_bits);
	}
	else
	{
		return (ctl->current_status == CURRENT_STATUS_OFF ? 0 : 1);
	}

	return (masked_val ? 0 : 1);
}

LDO_ERR_E LDO_SetVoltLevel(LDO_ID_E ldo_id, LDO_VOLT_LEVEL_E volt_level)
{
	unsigned short reg_data;
	struct ldo_ctl_info* ctl = NULL;

	ctl = LDO_GetLdoCtl(ldo_id);
	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));

	if (ctl->level_reg_b0 == LDO_INVALID_REG_ADDR)
	{
		goto Err_Exit;
	}

	if (ctl->level_reg_b0 != ctl->level_reg_b1)
	{
		printf("ldo_id:%d, level_reg_b0:%08x, level_reg_b1:%08x\r\n", ldo_id, ctl->level_reg_b0, ctl->level_reg_b1);
		goto Err_Exit;
	}

	switch (volt_level)
	{
		case LDO_VOLT_LEVEL0:
			ANA_REG_AND(ctl->level_reg_b0, (~(ctl->b0|ctl->b1)));
			break;

		case LDO_VOLT_LEVEL1:
			reg_data = ANA_REG_GET(ctl->level_reg_b0);
			reg_data &=~ctl->b1;
			reg_data |= ctl->b0;
			ANA_REG_SET(ctl->level_reg_b0, reg_data);
			break;

		case LDO_VOLT_LEVEL2:
			reg_data = ANA_REG_GET(ctl->level_reg_b0);
			reg_data &=~ctl->b0;
			reg_data |= ctl->b1;
			ANA_REG_SET(ctl->level_reg_b0, reg_data);
			break;

		case LDO_VOLT_LEVEL3:
			ANA_REG_OR (ctl->level_reg_b0, (ctl->b0|ctl->b1));
			break;

		default:
			goto Err_Exit;
	}

	ctl->current_volt_level = volt_level;
	return LDO_ERR_OK;
Err_Exit:
	return LDO_ERR_ERR;
}

LDO_VOLT_LEVEL_E LDO_GetVoltLevel(LDO_ID_E ldo_id)
{
	unsigned int level_ret = 0;
	struct ldo_ctl_info* ctl = NULL;

	ctl = LDO_GetLdoCtl(ldo_id);
	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));

	if (ctl->current_volt_level == LDO_VOLT_LEVEL_FAULT_MAX)
	{
		if(ctl->level_reg_b0 == ctl->level_reg_b1)
		{
			level_ret = 0;
		}
		else
		{
			level_ret = 0;
			SCI_PASSERT(0, ("shakr b0 must equal b1!"));
		}
	}
	else
	{
		level_ret = ctl->current_volt_level;
	}

	return level_ret;
}

int LDO_Init(void)
{
	int i;
	for ( i = 0; i < ARRAY_SIZE(ldo_ctl_data); ++i)
	{
		if( ldo_ctl_data[i].init_level != LDO_VOLT_LEVEL_FAULT_MAX)
		{
			LDO_SetVoltLevel(ldo_ctl_data[i].id, ldo_ctl_data[i].init_level);
		}
		ldo_ctl_data[i].ref = 0;
		ldo_ctl_data[i].current_status = CURRENT_STATUS_INIT;
		ldo_ctl_data[i].current_volt_level = ldo_ctl_data[i].init_level;
	}
	return LDO_ERR_OK;
}

static void LDO_TurnOffCoreLDO(void)
{
	ANA_REG_SET(ANA_REG_GLB_LDO_DCDC_PD_RTCSET, 0x7FFF);
}

static void LDO_TurnOffAllModuleLDO(void)
{
	ANA_REG_SET(ANA_REG_GLB_LDO_PD_CTRL, 0xFFF);
}

void LDO_TurnOffAllLDO(void)
{
	LDO_TurnOffAllModuleLDO();
	LDO_TurnOffCoreLDO();
}

#endif/*sc8830*/
