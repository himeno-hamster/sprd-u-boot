/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * (C) Copyright 2009
 * Ilya Yanok, Emcraft Systems Ltd, <yanok@emcraft.com>
 *
 * (C) Copyright 2009 DENX Software Engineering
 * Author: John Rigby <jrigby@gmail.com>
 * 	Add support for MX25
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
#include <div64.h>
#include <asm/io.h>
#include <linux/types.h>
#include <asm/arch/chip_drv_config_extern.h>
#include <asm/arch/bits.h>
#include "asm/arch/syscnt_drv.h"
#include "asm/arch/sci_types.h"

#define TIMER_MAX_VALUE 0xFFFFFFFF

typedef struct
{
	volatile unsigned int tmr_load;
	volatile unsigned int tmr_val;
	volatile unsigned int tmr_ctl;
	volatile unsigned int tmr_int;
}glb_tmr_phy_t;

typedef struct
{
	unsigned int tmr_base;
	unsigned int tmr_clk;
	unsigned int tmr_load;
	double       step;
}glb_tmr_info_t;

typedef struct
{
	glb_tmr_info_t*			tmr_info;
	volatile glb_tmr_phy_t* tmr_phy;
}glb_tmr_ctl_t;

static glb_tmr_info_t s_glb_tmr_info[]={
	{0x41000000, 32768,    0xffffffff},
	{0x41000020, 32768,    0xffffffff},
	{0x41000040, 26000000, 0xffffffff}
};

static void glb_tmr_init(glb_tmr_ctl_t* tmr_ctl, glb_tmr_info_t *tmr_info)
{
	REG32(GR_GEN0) |= 1<<2;  /* tmr enable */
	REG32(GR_GEN0) |= 1<<28; /* tmr rtc enable */

	tmr_ctl->tmr_info           = tmr_info;
	tmr_ctl->tmr_phy            = tmr_ctl->tmr_info->tmr_base;
	tmr_ctl->tmr_phy->tmr_ctl  &=~(1<<6); /* one time */
	tmr_ctl->tmr_phy->tmr_int  &=~(1<<0); /* disable int */
	tmr_ctl->tmr_phy->tmr_int  |= (3<<0); /* clear int */

	tmr_ctl->tmr_info->step	    =  1000*1000*1000;
	tmr_ctl->tmr_info->step	   /=  tmr_ctl->tmr_info->tmr_clk;

	printf("tmr_ctl->tmr_info->step :%f\r\n", tmr_ctl->tmr_info->step);
}

static inline void glb_tmr_load(glb_tmr_ctl_t* tmr_ctl, unsigned int value)
{
	while (tmr_ctl->tmr_phy->tmr_int & (1<<4)) /* load busy */
	{
		printf("load busy!\r\n");
	}

	tmr_ctl->tmr_phy->tmr_load = value;
}

static inline void glb_tmr_run(glb_tmr_ctl_t* tmr_ctl)
{
	tmr_ctl->tmr_phy->tmr_ctl |= 1<<7;
}

static inline void glb_tmr_stop(glb_tmr_ctl_t* tmr_ctl)
{
	tmr_ctl->tmr_phy->tmr_ctl &= ~(1<<7);
}

static inline unsigned int glb_tmr_value(glb_tmr_ctl_t* tmr_ctl)
{
	return tmr_ctl->tmr_phy->tmr_val;
}

static glb_tmr_ctl_t  s_glb_tmr_ctl[3];

int ____udelay(unsigned int __us)
{
	glb_tmr_ctl_t* tmr_ctl = &s_glb_tmr_ctl[2];

	glb_tmr_load(tmr_ctl, tmr_ctl->tmr_info->tmr_load);
	glb_tmr_run(tmr_ctl);

	__us = (unsigned int)(__us * 1000 / tmr_ctl->tmr_info->step + 0.5);

	//printf("tick cnt : %d\r\n", __us);

	__us = tmr_ctl->tmr_info->tmr_load - __us;

	while (__us < glb_tmr_value(tmr_ctl));

	glb_tmr_stop(tmr_ctl);
}

int global_timer_init()
{
	glb_tmr_init(&s_glb_tmr_ctl[0], &s_glb_tmr_info[0]);
	glb_tmr_init(&s_glb_tmr_ctl[1], &s_glb_tmr_info[1]);
	glb_tmr_init(&s_glb_tmr_ctl[2], &s_glb_tmr_info[2]);
}

static unsigned long long timestamp;
static ulong lastinc;
static ulong mul_value = 26;//default 26Mhz

/*
 * "time" is measured in 1 / CONFIG_SYS_HZ seconds,
 * "tick" is internal timer period
 */
static inline unsigned long long tick_to_time(unsigned long long tick)
{
	tick *= CONFIG_SYS_HZ;
	do_div(tick, CONFIG_SPRD_TIMER_CLK);
	return tick;
}

static inline unsigned long long time_to_tick(unsigned long long time)
{
	time *= CONFIG_SPRD_TIMER_CLK;
	do_div(time, CONFIG_SYS_HZ);
	return time;
}

static inline unsigned long long us_to_tick(unsigned long long us)
{
	us = us*CONFIG_SPRD_TIMER_CLK;
	do_div(us, CONFIG_SYS_HZ*1000);
	return us;
}

void calibrate_mul(void)
{
	ulong apb_clk, clk_div;
#define CLK_APB_SHIFT	(14)
	clk_div = readl(GR_CLK_DLY);
	apb_clk = (clk_div>>CLK_APB_SHIFT) & 0x3;
	if (apb_clk == 0)
		mul_value = 26;
	else if (apb_clk == 1)
		mul_value = 51;
	else
		mul_value = 77;
}

/* nothing really to do with interrupts, just starts up a counter. */
/* The 32KHz 23-bit timer overruns in 512 seconds */

int timer_init(void)
{
	REG32(GR_GEN0) |= BIT_19; //make sys cnt writable
	REG32(GR_GEN0) |= BIT_27; //enable rtc clk input

	REG32(GR_GEN0) |= BIT_2; //enable pclk timer
	REG32 (TM2_CLR) &=~ (BIT_0);//disable pclk timer interrupt
	REG32 (TM2_CTL) &=~ (BIT_7);//timer stops first
	REG32 (TM2_CTL) &=~ (BIT_6);//one-time mode

	calibrate_mul();

	//clear any hanging interrupts & disable interrupt
	REG32 (SYS_CTL) &=~ (BIT_0);
	REG32 (SYS_CTL) |= (BIT_3);

	global_timer_init();

	return 0;
}

void reset_timer(void)
{
	//capture current incrementer value time
	lastinc = readl(SYS_CNT0);
	timestamp = 0;
}
void reset_timer_masked(void)
{
	reset_timer();
}
unsigned long long get_ticks(void)
{
	ulong now = readl(SYS_CNT0);
	
	if(now >= lastinc) {
	/* not roll
	 * move stamp forward with absolut diff ticks
	 * */
		timestamp += (now - lastinc);
	}else{
		//timer roll over 
		timestamp += (TIMER_MAX_VALUE - lastinc) + now;
	}
	lastinc = now;
	return timestamp;
}

ulong get_timer_masked(void)
{
	return tick_to_time(get_ticks());
}

ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

void set_timer(ulong t)
{
	timestamp = time_to_tick(t);
}

#if 0
void __udelay (unsigned long usec)
{
	unsigned long long tmp;
	ulong tmo;

	if (usec<10000)
	{
		____udelay(usec);
		return;
	}

	tmo = us_to_tick(usec);
	tmp = get_ticks() + tmo; // get current timestamp
	
	while(get_ticks() < tmp) //loop till event
		/*NOP*/;
}
#endif
void __udelay (unsigned long usec)
{
	ulong delta_usec = usec * mul_value;
	writel(delta_usec, TM2_LOAD);
	REG32 (TM2_CTL) |= (BIT_7);//timer runs
	//printf("readl(TM2_VALUE) = 0x%x\n", readl(TM2_VALUE));
	while(readl(TM2_VALUE) > 0)//loop
		;
	REG32 (TM2_CTL) &=~ (BIT_7);//timer stops
}
