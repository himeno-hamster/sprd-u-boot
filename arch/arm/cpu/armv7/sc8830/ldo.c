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

#include <asm/arch/regs_global.h>
#include <asm/arch/bits.h>
#include <asm/arch/ldo.h>
#include <asm/arch/regs_ana.h>

#define LDO_INVALID_REG	0xFFFFFFFF
#define LDO_INVALID_BIT		0xFFFFFFFF


#define CURRENT_STATUS_INIT	0x00000001
#define CURRENT_STATUS_ON	0x00000002
#define CURRENT_STATUS_OFF	0x00000004

#define LDO_INVALID_REG_ADDR		(0x1)

#define CHIP_REG_OR(reg_addr, value)    (*(volatile unsigned int *)(reg_addr) |= (unsigned int)(value))
#define CHIP_REG_AND(reg_addr, value)   (*(volatile unsigned int *)(reg_addr) &= (unsigned int)(value))
#define CHIP_REG_GET(reg_addr)          (*(volatile unsigned int *)(reg_addr))
#define CHIP_REG_SET(reg_addr, value)   (*(volatile unsigned int *)(reg_addr)  = (unsigned int)(value))
 
#define SCI_PASSERT(condition, format...)  \
	do {		\
		if(!(condition)) { \
			printf("function :%s\r\n", __FUNCTION__);\
			BUG();	\
		} \
	}while(0)

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

struct ldo_sleep_ctl_info {
	SLP_LDO_E id;
	unsigned int ldo_sleep_reg;
	unsigned int mask;
	unsigned  int value;
};


static struct ldo_ctl_info ldo_ctl_data[] =
{
#if 0
	{
		.id = LDO_DCDCARM,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_9,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_9,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0 = 0,
		.b0_rst = 0,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1 = 0,
		.b1_rst = 0,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_VDD25,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_8,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_8,
		.level_reg_b0 = ANA_LDO_VCTL3,
		.b0 = BIT_8,
		.b0_rst = BIT_9,
		.level_reg_b1 = ANA_LDO_VCTL3,
		.b1 = BIT_10,
		.b1_rst = BIT_11,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_VDD18,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_7,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_7,
		.level_reg_b0 = ANA_LDO_VCTL3,
		.b0 = BIT_4,
		.b0_rst = BIT_5,
		.level_reg_b1 = ANA_LDO_VCTL3,
		.b1 = BIT_6,
		.b1_rst = BIT_7,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_VDD28,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_6,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_6,
		.level_reg_b0 = ANA_LDO_VCTL3,
		.b0 = BIT_0,
		.b0_rst = BIT_1,
		.level_reg_b1 = ANA_LDO_VCTL3,
		.b1 = BIT_2,
		.b1_rst = BIT_3,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_ABB,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_5,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_5,
		.level_reg_b0 = ANA_LDO_VCTL0,
		.b0 = BIT_12,
		.b0_rst = BIT_13,
		.level_reg_b1 = ANA_LDO_VCTL0,
		.b1 = BIT_14,
		.b1_rst = BIT_15,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_RF0,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_3,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_3,
		.level_reg_b0 = ANA_LDO_VCTL0,
		.b0 = BIT_4,
		.b0_rst = BIT_5,
		.level_reg_b1 = ANA_LDO_VCTL0,
		.b1 = BIT_6,
		.b1_rst = BIT_7,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_RF1,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_4,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_4,
		.level_reg_b0 = ANA_LDO_VCTL0,
		.b0 = BIT_8,
		.b0_rst = BIT_9,
		.level_reg_b1 = ANA_LDO_VCTL0,
		.b1 = BIT_10,
		.b1_rst = BIT_11,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_MEM,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_2,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_2,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0 = 0,
		.b0_rst = 0,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1 = 0,
		.b1_rst = 0,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_DCDC,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_1,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_1,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0 = 0,
		.b0_rst = 0,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1 = 0,
		.b1_rst = 0,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_BG,
		.bp_reg = ANA_LDO_PD_SET,
		.bp_bits = BIT_0,
		.bp_rst_reg = ANA_LDO_PD_RST,
		.bp_rst = BIT_0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0 = 0,
		.b0_rst = 0,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1 = 0,
		.b1_rst = 0,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_AVB,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_14,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_15,
		.level_reg_b0 = ANA_LDO_VCTL1,
		.b0 = BIT_8,
		.b0_rst = BIT_9,
		.level_reg_b1 = ANA_LDO_VCTL1,
		.b1 = BIT_10,
		.b1_rst = BIT_11,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_CAMA,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_12,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_13,
		.level_reg_b0 = ANA_LDO_VCTL2,
		.b0 = BIT_8,
		.b0_rst = BIT_9,
		.level_reg_b1 = ANA_LDO_VCTL2,
		.b1 = BIT_10,
		.b1_rst = BIT_11,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_CAMD1,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_10,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_11,
		.level_reg_b0 = ANA_LDO_VCTL2,
		.b0 = BIT_4,
		.b0_rst = BIT_5,
		.level_reg_b1 = ANA_LDO_VCTL2,
		.b1 = BIT_6,
		.b1_rst = BIT_7,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_CAMD0,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_8,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_9,
		.level_reg_b0 = ANA_LDO_VCTL2,
		.b0 = BIT_0,
		.b0_rst = BIT_1,
		.level_reg_b1 = ANA_LDO_VCTL2,
		.b1 = BIT_2,
		.b1_rst = BIT_3,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_SIM1,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_6,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_7,
		.level_reg_b0 = ANA_LDO_VCTL1,
		.b0 = BIT_4,
		.b0_rst = BIT_5,
		.level_reg_b1 = ANA_LDO_VCTL1,
		.b1 = BIT_6,
		.b1_rst = BIT_7,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_SIM0,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_4,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_5,
		.level_reg_b0 = ANA_LDO_VCTL1,
		.b0 = BIT_0,
		.b0_rst = BIT_1,
		.level_reg_b1 = ANA_LDO_VCTL1,
		.b1 = BIT_2,
		.b1_rst = BIT_3,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_SDIO0,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_2,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_3,
		.level_reg_b0 = ANA_LDO_VCTL1,
		.b0 = BIT_12,
		.b0_rst = BIT_13,
		.level_reg_b1 = ANA_LDO_VCTL1,
		.b1 = BIT_14,
		.b1_rst = BIT_15,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_USB,
		.bp_reg = ANA_LDO_PD_CTL0,
		.bp_bits = BIT_0,
		.bp_rst_reg = ANA_LDO_PD_CTL0,
		.bp_rst = BIT_1,
		.level_reg_b0 = ANA_LDO_VCTL2,
		.b0 = BIT_12,
		.b0_rst = BIT_13,
		.level_reg_b1 = ANA_LDO_VCTL2,
		.b1 = BIT_14,
		.b1_rst = BIT_15,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_SIM3,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_8,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_9,
		.level_reg_b0 = ANA_LDO_VCTL4,
		.b0 = BIT_12,
		.b0_rst = BIT_13,
		.level_reg_b1 = ANA_LDO_VCTL4,
		.b1 = BIT_14,
		.b1_rst = BIT_15,
		.init_level = LDO_VOLT_LEVEL3,	//CMMB 1.2V
	},
	{
		.id = LDO_LDO_SIM2,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_6,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_7,
		.level_reg_b0 = ANA_LDO_VCTL4,
		.b0 = BIT_8,
		.b0_rst = BIT_9,
		.level_reg_b1 = ANA_LDO_VCTL4,
		.b1 = BIT_10,
		.b1_rst = BIT_11,
		.init_level = LDO_VOLT_LEVEL1,	//E-NAND 3.0V
	},
	{
		.id = LDO_LDO_WIF1,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_4,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_5,
		.level_reg_b0 = ANA_LDO_VCTL4,
		.b0 = BIT_4,
		.b0_rst = BIT_5,
		.level_reg_b1 = ANA_LDO_VCTL4,
		.b1 = BIT_6,
		.b1_rst = BIT_7,
		.init_level = LDO_VOLT_LEVEL2,	//WIFI 1.8V
	},
	{
		.id = LDO_LDO_WIF0,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_2,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_3,
		.level_reg_b0 = ANA_LDO_VCTL4,
		.b0 = BIT_0,
		.b0_rst = BIT_1,
		.level_reg_b1 = ANA_LDO_VCTL4,
		.b1 = BIT_2,
		.b1_rst = BIT_3,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_SDIO1,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_0,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_1,
		.level_reg_b0 = ANA_LDO_VCTL3,
		.b0 = BIT_12,
		.b0_rst = BIT_13,
		.level_reg_b1 = ANA_LDO_VCTL3,
		.b1 = BIT_14,
		.b1_rst = BIT_15,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_RTC,
		.bp_reg = LDO_INVALID_REG_ADDR,
		.bp_bits = 0,
		.bp_rst_reg = LDO_INVALID_REG_ADDR,
		.bp_rst = 0,
		.level_reg_b0 = ANA_LDO_VCTL0,
		.b0 = 0,
		.b0_rst = 1,
		.level_reg_b1 = ANA_LDO_VCTL0,
		.b1 = 2,
		.b1_rst = 3,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_USBD,
		.bp_reg = LDO_INVALID_REG_ADDR,
		.bp_bits = 0,
		.bp_rst_reg = LDO_INVALID_REG_ADDR,
		.bp_rst = 0,
		.level_reg_b0 = LDO_INVALID_REG_ADDR,
		.b0 = 0,
		.b0_rst = 0,
		.level_reg_b1 = LDO_INVALID_REG_ADDR,
		.b1 = 0,
		.b1_rst = 0,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_SDIO3,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_4,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_5,
		.level_reg_b0 = ANA_LDO_VCTL4,
		.b0 = BIT_4,
		.b0_rst = BIT_5,
		.level_reg_b1 = ANA_LDO_VCTL4,
		.b1_rst = BIT_6,
		.b1 = BIT_7,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
	{
		.id = LDO_LDO_VDD30,
		.bp_reg = ANA_LDO_PD_CTL1,
		.bp_bits = BIT_6,
		.bp_rst_reg = ANA_LDO_PD_CTL1,
		.bp_rst = BIT_7,
		.level_reg_b0 = ANA_LDO_VCTL1,
		.b0 = BIT_8,
		.b0_rst = BIT_9,
		.level_reg_b1 = ANA_LDO_VCTL1,
		.b1 = BIT_10,
		.b1_rst = BIT_11,
		.init_level = LDO_VOLT_LEVEL_FAULT_MAX,
	},
#endif
};

 /**---------------------------------------------------------------------------*
 **                         Function Declaration                              *
 **---------------------------------------------------------------------------*/
static struct ldo_ctl_info* LDO_GetLdoCtl(LDO_ID_E ldo_id)
{
	int i = 0;
	struct ldo_ctl_info* ctl = NULL;

	for ( i = 0; i < ARRAY_SIZE(ldo_ctl_data); ++i) {
		if (ldo_ctl_data[i].id == ldo_id) {
			ctl = &ldo_ctl_data[i];
			break;
		}
	}

	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));
	return ctl;
}

LDO_ERR_E LDO_TurnOnLDO(LDO_ID_E ldo_id)
{
	return LDO_ERR_OK;
}


 LDO_ERR_E LDO_TurnOffLDO(LDO_ID_E ldo_id)
{

	return LDO_ERR_OK;
}

 int LDO_IsLDOOn(LDO_ID_E ldo_id)
{
	unsigned int  masked_val = 0;
	struct ldo_ctl_info* ctl = NULL;

	ctl = LDO_GetLdoCtl(ldo_id);
	SCI_PASSERT(ctl != NULL, ("ldo_id = %d", ldo_id));

	if (ctl->current_status == CURRENT_STATUS_INIT)
		masked_val = (LDO_REG_GET(ctl->bp_reg) & ctl->bp_bits);
	else
		return (ctl->current_status == CURRENT_STATUS_OFF ? 0 : 1);

	return (masked_val ? 0 : 1);
}

 LDO_ERR_E LDO_SetVoltLevel(LDO_ID_E ldo_id, LDO_VOLT_LEVEL_E volt_level)
{
	return LDO_ERR_OK;
}


 LDO_VOLT_LEVEL_E LDO_GetVoltLevel(LDO_ID_E ldo_id)
{
	unsigned int level_ret = 0;

	return level_ret;
}

void LDO_DeepSleepInit(void)
{
}

int LDO_Init(void)
{
	return LDO_ERR_OK;
}

static void LDO_TurnOffCoreLDO(void)
{
}

static void LDO_TurnOffAllModuleLDO(void)
{

}

void LDO_TurnOffAllLDO(void)
{
}

