/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/regs_global.h>
#include "efuse_drv.h"

#ifdef EFUSE_DEBUG
static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}
#endif

//static void __iomem *ctl_efuse_base = 0;
void sci_efuse_poweron(void)
{
	SCI_D(GR_GEN0) |= GEN0_EFUSE_EN;
	SCI_D(REG_EFUSE_PGM_PARA) |= BIT_EFUSE_VDD_ON;
	SCI_D(REG_EFUSE_PGM_PARA) |= BIT_CLK_EFS_EN;
}

void sci_efuse_poweroff(void)
{
	SCI_D(REG_EFUSE_PGM_PARA) &= ~BIT_CLK_EFS_EN;
	SCI_D(REG_EFUSE_PGM_PARA) &= ~BIT_EFUSE_VDD_ON;
	SCI_D(GR_GEN0) &= ~GEN0_EFUSE_EN;
}

int sci_efuse_read(unsigned blk)
{
	int busy = 0;

	if (blk > (MASK_READ_INDEX >> SHIFT_READ_INDEX))
	{
		return 0;
	}

	SCI_D(REG_EFUSE_BLOCK_INDEX) = BITS_READ_INDEX(blk);
	SCI_D(REG_EFUSE_MODE_CTRL) |= BIT_RD_START;

	do
	{
		busy = SCI_D(REG_EFUSE_STATUS) & BIT_READ_BUSY;
	} while (busy);

	return SCI_D(REG_EFUSE_DATA_RD);
}

int sci_efuse_is_locked(unsigned blk)
{
	return 0;
}

int sci_efuse_lock(unsigned blk)
{
	return 0;
}

#ifdef EFUSE_DEBUG
int sci_efuse_program(unsigned blk, int data)
{
	int busy = 0;

	if (blk > (MASK_PGM_INDEX >> SHIFT_PGM_INDEX))
	{
		return 0;
	}

	SCI_D(REG_EFUSE_BLOCK_INDEX) = BITS_PGM_INDEX(blk);
	SCI_D(REG_EFUSE_DATA_WR) = data;
	SCI_D(REG_EFUSE_MODE_CTRL) |= BIT_PG_START;

	do
	{
		busy = SCI_D(REG_EFUSE_STATUS) & BIT_PGM_BUSY;
	} while(busy);

	return 1;
}

/* low level */
int sci_efuse_raw_write(unsigned blk, int data, u32 magic)
{
	int retVal;

	SCI_D(REG_EFUSE_PGM_PARA) |= BIT_PGM_EN;
	SCI_D(REG_EFUSE_MAGIC_NUMBER) = BITS_MAGIC_NUMBER(magic);
	ADI_Analogdie_reg_write(0x420006D4, 0xC686);
	ADI_Analogdie_reg_write(0x420006D8, 0x01);

	delay(10000);

	retVal = sci_efuse_program(blk, data);
	SCI_D(REG_EFUSE_PGM_PARA) &= ~BIT_PGM_EN;

	return retVal;
}
#else
int sci_efuse_program(unsigned blk, int data)
{
	return 0;
}

int sci_efuse_raw_write(unsigned blk, int data, u32 magic)
{
	return 0;
}
#endif

static int overclocking_flag = 0;
static int overclocking_data = 0;;

int sci_efuse_overclocking_get()
{
	int data;

	if (overclocking_flag == 0)
	{
		sci_efuse_poweron();
		data = sci_efuse_read(1);
		sci_efuse_poweroff();

		if ((data & BIT_29) != 0) {
			overclocking_data = 1;
		}

		overclocking_flag = 1;
	}

	return overclocking_data;
}


#if 0
#define CAL_DATA_BLK	7
#define BASE_ADC_P0   785   //3.6V
#define BASE_ADC_P1   917   //4.2V
#define VOL_P0        3600
#define VOL_P1        4200
#define ADC_DATA_OFFSET 128
int sci_efuse_calibration_get(unsigned int * p_cal_data)
{
	int data;
    unsigned int cal_temp;
    unsigned short adc_temp;

	sci_efuse_poweron();
	data = sci_efuse_read(CAL_DATA_BLK);
	sci_efuse_poweroff();

	data &= ~(1 << 31); 
    
//    data = (173 |(171 << 8));
    printf("sci_efuse_calibration data:%d\n",data);
	if((!data)||(p_cal_data == NULL))
	{
		return 0;
	}
    //adc 3.6V 
    adc_temp = ((data>>8) & 0x00FF) + BASE_ADC_P0 - ADC_DATA_OFFSET;
    p_cal_data[1] = (VOL_P0)|(adc_temp << 16);

    //adc 4.2V
	adc_temp = (data & 0x00FF) + BASE_ADC_P1 - ADC_DATA_OFFSET;
    p_cal_data[0] = (VOL_P1)|(adc_temp << 16);

	return 1;
}
#endif
