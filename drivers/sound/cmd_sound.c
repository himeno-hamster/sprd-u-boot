/*
 * Copyright 2000-2009
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

#include <common.h>
#include <command.h>
#include <errno.h>
#include <ubi_uboot.h>
#include <asm/io.h>

#include "sound_common.h"

#define pr_fmt(fmt) "[sound: cmd ]" fmt
#ifdef	CMD_SND_DEBUG
#define	snd_dbg(fmt,args...)	pr_info(fmt ,##args)
#else
#define snd_dbg(fmt,args...)
#endif

#define SND_REG_MASK (0xFFFFFFFF)

static inline int snd_reg_lock(int reg)
{
	snd_dbg("register lock 0x%08x\n", reg);
	return 0;
}

static inline int snd_reg_unlock(int reg)
{
	snd_dbg("register unlock 0x%08x\n", reg);
	return 0;
}

static inline int snd_reg_is_adie_addr(int reg)
{
	return (reg >= ANA_REG_ADDR_START) && (reg <= ANA_REG_ADDR_END);
}

int snd_reg_read(int reg)
{
	if (snd_reg_is_adie_addr(reg)) {
		return sci_adi_read(reg);
	}
	return __raw_readl(reg);
}

EXPORT_SYMBOL(snd_reg_read);

int snd_reg_write(int reg, int value, int mask)
{
	int ret = 0;
	if (snd_reg_is_adie_addr(reg)) {
		ret = sci_adi_write(reg, value, mask);
	} else {
		int val;
		snd_reg_lock(reg);
		val = __raw_readl(reg) & ~mask;
		__raw_writel(val | (value & mask), reg);
		snd_reg_unlock(reg);
	}
	return ret;
}

EXPORT_SYMBOL(snd_reg_write);

int snd_reg_wirte_f(int reg, int value)
{
	return snd_reg_write(reg, value, SND_REG_MASK);
}

EXPORT_SYMBOL(snd_reg_wirte_f);

int sound_init(void)
{
	sprd_codec_init();
	vbc_init();
	return 0;
}

EXPORT_SYMBOL(sound_init);

int do_reg_write(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	int addr, value, mask = SND_REG_MASK;

	if (argc < 3)
		return cmd_usage(cmdtp);

	addr = simple_strtoul(argv[1], NULL, 16);
	value = simple_strtoul(argv[2], NULL, 16);
	if (argc > 3)
		mask = simple_strtoul(argv[3], NULL, 16);
	ret = snd_reg_write(addr, value, mask);
	printf("0x%08x | 0x%08x\n", addr, snd_reg_read(addr));
	return ret;
}

int do_reg_read(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int addr, lenght;

	if (argc < 2)
		return cmd_usage(cmdtp);

	addr = simple_strtoul(argv[1], NULL, 16);
	if (argc > 2) {
		int address;
		lenght = simple_strtoul(argv[2], NULL, 16);
		for (address = addr & ~0xF; address < (addr + lenght);
		     address += 0x4) {
			if (!(address & 0xF)) {
				printf("\n0x%08x | ", address);
			}
			if (address >= addr) {
				printf("0x%08x ", snd_reg_read(address));
			} else {
				printf("           ");
			}
		}
		printf("\n");
	} else {
		printf("0x%08x | 0x%08x\n", addr, snd_reg_read(addr));
	}
	return 0;
}

/* command define */

U_BOOT_CMD(regr, 3, 1, do_reg_read,
	   "get chip register value",
	   "\n"
	   "	- get chip register value, include adie register\n"
	   "regr reg_addr [range]\n"
	   "	reg_addr  - which register you want get\n"
	   "	range 	  - what range you want get\n");

U_BOOT_CMD(regw, 4, 1, do_reg_write,
	   "set chip register value",
	   "\n"
	   "	- set chip register value, include adie register\n"
	   "regw reg_addr value [mask]\n"
	   "	reg_addr  - which register you want set\n"
	   "	value 	  - what value you want set\n"
	   "	mask 	  - which bit mask you just want set\n");
