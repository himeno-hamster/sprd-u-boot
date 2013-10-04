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

int LDO_Init(void)
{
#if defined(CONFIG_SPX15)
	ANA_REG_BIC(ANA_REG_GLB_LDO_PD_CTRL, 0
		//|BIT_DCDC_GEN_PD
		//|BIT_DCDC_MEM_PD
		//|BIT_DCDC_ARM_PD
		//|BIT_DCDC_CORE_PD
		//|BIT_LDO_RF0_PD
		|BIT_LDO_EMMCCORE_PD
		|BIT_LDO_EMMCIO_PD
		//|BIT_LDO_DCXO_PD
		//|BIT_LDO_CON_PD
		//|BIT_LDO_VDD25_PD
		//|BIT_LDO_VDD28_PD
		//|BIT_LDO_VDD18_PD
		//|BIT_BG_PD
    );

	ANA_REG_BIC(ANA_REG_GLB_LDO_PD_CTRL, 0
		//|BIT_DCDC_WPA_PD
		//|BIT_LDO_CLSG_PD
		|BIT_LDO_USB_PD
		//|BIT_LDO_CAMMOT_PD
		//|BIT_LDO_CAMIO_PD
		//|BIT_LDO_CAMD_PD
		//|BIT_LDO_CAMA_PD
		//|BIT_LDO_SIM2_PD
		//|BIT_LDO_SIM1_PD
		//|BIT_LDO_SIM0_PD
		//|BIT_LDO_SD_PD
	);
#endif
	return 0;
}

LDO_ERR_E LDO_TurnOffLDO(LDO_ID_E ldo_id)
{
	return 0;
}

LDO_ERR_E LDO_TurnOnLDO(LDO_ID_E ldo_id)
{
	return 0;
}
