/******************************************************************************
 ** File Name:      spi_phy_v0.c                                                 *
 ** Author:         liuhao                                                   *
 ** DATE:           06/28/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file define the physical layer of SPI device.      *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 06/28/2010     liuhao     Create.  
 ******************************************************************************/

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
//#define __DEBUG__
//#define __SPI_MODE__
#define mdelay(n)	udelay((n) * 1000)

extern int modem_status(void);

int	req_clk_status(void)
{
	if(modem_status() == 2)  {
		mdelay(10);
		return 0;
	} else {
		return gpio_get_value(CP_AP_LIV);
	}
}
/*********************************************************************************************************
** Function name: req_clk_init
** Descriptions:  
** input parameters:
**
**
**
** output parameters:
** Returned value:
*********************************************************************************************************/
void req_clk_init(void)
{
	//gpio_direction_output(CP_AP_LIV, 0); 
}

void dump_gpio_register(void)
{
//	GPIO_PRINT(("GPIO(0x%x --0x%x)      = 0x%8x\n",gpio_base_for_cs+0x4,gpio_base_for_cs,REG32(gpio_base_for_cs+0x4)));
//	GPIO_PRINT(("GPIO(0x%x -- %d)      = 0x%8x\n",gpio_base_for_cs+0x8,gpio_bit_num_for_cs,REG32(gpio_base_for_cs+0x8)));
}

