/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *************************************************
 * Automatically generated C header: do not edit *
 *************************************************
 */

/*
 * Regulator Name[0], Regulator Type[1], Power Off Ctrl[2] and Bit[3], Power On Ctrl[4] and Bit[5], Slp Ctrl[6] and Bit[7],
 * Voltage Trimming Ctrl[8] and Bits[9], Voltage Ctrl[10] and Bits[11], Voltage Select Count[12] and List[13 ... ...]
 */
	SCI_REGU_REG(vddmem, 2, ANA_REG_GLB_LDO_PD_SET, 0x08,
				ANA_REG_GLB_LDO_PD_RST, 0x10, 0, 0,
				ANA_REG_GLB_DCDCMEM_CTRL_CAL, 0x1F,
				ANA_REG_GLB_DCDCMEM_CTRL0, 0x03,
				4, 1200, 1100, 1300, 1400);

	SCI_REGU_REG(vddarm, 2, ANA_REG_GLB_LDO_PD_SET, 0x04,
				ANA_REG_GLB_LDO_PD_RST, 0x02, 0, 0,
				ANA_REG_GLB_DCDCARM_CTRL_CAL, 0x1F,
				ANA_REG_GLB_DCDCARM_CTRL0, 0x07,
				8, 1100, 700, 800, 900, 1000, 650, 1200, 1300);

	SCI_REGU_REG(vddcore, 2, ANA_REG_GLB_LDO_PD_SET, 0x02,
				ANA_REG_GLB_LDO_PD_RST, 0x02, 0, 0,
				ANA_REG_GLB_DCDC_CTRL_CAL,  0x1F,
				ANA_REG_GLB_DCDC_CTRL0, 0x07,
				8, 1100, 700, 800, 900, 1000, 650, 1200, 1300);

	SCI_REGU_REG(vddsim0, 0, ANA_REG_GLB_LDO_PD_CTRL0, 0x10,
				ANA_REG_GLB_LDO_PD_CTRL0, 0x20, ANA_REG_GLB_LDO_SLP_CTRL0, 0x04,
				(ANA_REGS_GLB2_BASE+0x10), 0x1F00,
				ANA_REG_GLB_LDO_VCTRL1, 0x0F,
				4, 1800, 2900, 3000, 3100);