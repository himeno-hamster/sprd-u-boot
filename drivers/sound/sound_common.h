/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 *  Definitions for Command Processor
 */
#ifndef __SOUND_COMMON_H
#define __SOUND_COMMON_H

#ifdef CONFIG_SC8825
#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_global.h>
#include <asm/arch/adi.h>
#endif

#ifdef CONFIG_SC8830
#include <asm/arch/sprd_reg.h>
#define ANA_REG_ADDR_START      (SPRD_ADI_PHYS + 0x40)
#define ANA_REG_ADDR_END        (SPRD_ADI_PHYS + 0xFFFF)
#endif

int snd_reg_read(int reg);
int snd_reg_write(int reg, int value, int mask);
int snd_reg_wirte_f(int reg, int value);

#ifdef CONFIG_SOUND_CODEC_SPRD
#include "codec/sprd-codec.h"
#endif
#ifdef CONFIG_SOUND_CODEC_SPRD_V3
#include "codec/sprd-codec-v3.h"
#endif
#ifdef CONFIG_SOUND_DAI_VBC
#include "dai/vbc.h"
#endif
#ifdef CONFIG_SOUND_DAI_I2S
#include "dai/i2s.h"
#endif

#endif /* __COMMAND_H */
