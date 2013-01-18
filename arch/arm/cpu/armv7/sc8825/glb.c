/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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
 */

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch-sc8825/ldo.h>
#include <asm/arch/sc8810_reg_global.h>
#include <asm/arch/regs_global.h>
#include <asm/arch/regs_cpc.h>
#include <asm/arch/analog_reg_v3.h>
#include <ubi_uboot.h>
#include <asm/sizes.h>

#define rounddown(x, y) (				\
{							\
	typeof(x) __x = (x);				\
	__x - (__x % (y));				\
}							\
)

/* vars definitions for controller REGS_GLB */
#define REG_GLB_SET(A)                  ( A + 0x1000 )
#define REG_GLB_CLR(A)                  ( A + 0x2000 )

u32 sci_glb_read(u32 reg, u32 msk)
{
	return __raw_readl(reg) & msk;
}

int sci_glb_write(u32 reg, u32 val, u32 msk)
{
	unsigned long flags, hw_flags;
	__raw_writel((__raw_readl(reg) & ~msk) | val, reg);
	return 0;
}

static int __is_glb(u32 reg)
{
	return rounddown(reg, SZ_64K) == rounddown(GREG_BASE, SZ_64K) ||
	    rounddown(reg, SZ_64K) == rounddown(AHB_GEN_CTL_BEGIN, SZ_64K);
}

int sci_glb_set(u32 reg, u32 bit)
{
	if (__is_glb(reg))
		__raw_writel(bit, REG_GLB_SET(reg));
	else
		WARN_ON(1);
	return 0;
}

int sci_glb_clr(u32 reg, u32 bit)
{
	if (__is_glb(reg))
		__raw_writel(bit, REG_GLB_CLR(reg));
	else
		WARN_ON(1);
	return 0;
}

