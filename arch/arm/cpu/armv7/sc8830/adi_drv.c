/******************************************************************************
 ** File Name:      adi_phy_v3.c                                                 *
 ** Author:         tim.luo                                             *
 ** DATE:           2/25/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic operation interfaces of       *
 **                 Analog to Digital Module.                                       *
 **                                                                                             *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 2/25/2010     Tim Luo      Create.                                   *
 **                                                                                                *
 ******************************************************************************/


/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/ldo.h>
#include <ubi_uboot.h>
#include <asm/sizes.h>
#include <asm/arch/sprd_reg.h>

#include <asm/arch/chip_drv_common_io.h>
#include <asm/arch/adi_reg_v3.h>

#ifndef	__ADI_DEBUG__
#define	__ADI_DEBUG__
#endif

#define TIMEOUT_ADI	(200000)

/*****************************************************************************
 *  Description:    this function performs read operation to the analog die reg .   *
 *                      it will disable all interrupt and polling the ready bit,        *
 *              and return a half-word value after read complete.             *
 *  Global Resource Dependence:                                              *
 *  Author: Tim Luo                                                        *
 *  Note:   return register value                                               *
******************************************************************************/
unsigned short ADI_Analogdie_reg_read (unsigned int addr)

{
	unsigned int adi_rd_data;
	unsigned long flags;
	int timeout = TIMEOUT_ADI;

    //Set read command
    CHIP_REG_SET (ADI_ARM_RD_CMD, addr);

    //wait read operation complete, RD_data[31] will be cleared after the read operation complete
	do {
		adi_rd_data = CHIP_REG_GET (ADI_RD_DATA);		
		if (!timeout--)
		{
#if !(defined(CONFIG_NAND_SPL) || defined(CONFIG_FDL1))
			printf("[0x%s]: reg = 0x%x, value = 0x%x\n", __func__, addr, adi_rd_data);
#endif
			break;
		}
	} while (adi_rd_data & BIT_31);
	
    return ( (unsigned short) (adi_rd_data & 0x0000FFFF));

}

/*****************************************************************************
 *  Description:    this function performs write operation to the analog die reg .   *
 *                      it will write the analog die register if the fifo is not full       *
 *              It will polling the fifo full status til it is not full                  *
 *  Global Resource Dependence:                                              *
 *  Author: Tim Luo                                                        *
 *  Note:                                                                      *
******************************************************************************/
void ADI_Analogdie_reg_write (unsigned int addr, unsigned short data)
{

	int timeout = TIMEOUT_ADI;
	do {////ADI_wait_fifo_empty
		if ( ( (CHIP_REG_GET (ADI_FIFO_STS) & ( (unsigned int) ADI_FIFO_EMPTY)) != 0))
		{
			break;
		}
		if (!timeout--)
		{
#if !(defined(CONFIG_NAND_SPL) || defined(CONFIG_FDL1))
			printf("[0x%s]: reg = 0x%x, value = 0x%x\n", __func__, addr, data);
#endif
			return;
		}
	} while (1);/*lint !e506*/

	CHIP_REG_SET (addr, data);
}


/*****************************************************************************
 *  Description:    this function is used to init analog to digital module.   *
 *                      it will enable adi_acc and soft reset adi_module,        *
 *              and then config the priority of each channel.             *
 *  Global Resource Dependence:                                              *
 *  Author: Tim Luo                                                        *
 *  Note:                                                                                     *
******************************************************************************/
void ADI_init (void)
{
	CHIP_REG_OR (REG_AON_APB_APB_EB0, BIT_ADI_EB);

	//reset ADI module
	CHIP_REG_OR (REG_AON_APB_APB_RST0, BIT_ADI_SOFT_RST);
	{
		unsigned int wait = 50;

		while (wait--);
	}
	CHIP_REG_AND(REG_AON_APB_APB_RST0, (~BIT_ADI_SOFT_RST));
}

