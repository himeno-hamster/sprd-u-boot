/*
 * armboot - Startup Code for OMAP3530/ARM Cortex CPU-core
 *
 * Copyright (c) 2004	Texas Instruments <r-woodruff2@ti.com>
 *
 * Copyright (c) 2001	Marius Gröger <mag@sysgo.de>
 * Copyright (c) 2002	Alex Züpke <azu@sysgo.de>
 * Copyright (c) 2002	Gary Jennejohn <garyj@denx.de>
 * Copyright (c) 2003	Richard Woodruff <r-woodruff2@ti.com>
 * Copyright (c) 2003	Kshitij <kshitij@ti.com>
 * Copyright (c) 2006-2008 Syed Mohammed Khasim <x0khasim@ti.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <asm-offsets.h>
#include <config.h>
#include <common.h>
#include <version.h>

#define SVC_STACK_TEMP 0x5000CFF0 //for shark
#define SPL_STACK      0x808f0000

.globl _start
_start: b	reset
	ldr	pc, =0xFFFF0004 
	ldr	pc, =0xFFFF0008 
	ldr	pc, =0xFFFF000C 
	ldr	pc, =0xFFFF0010 
	ldr	pc, =0xFFFF0014 
	ldr	pc, =0xFFFF0018 
	ldr	pc, =0xFFFF001C 

/*************************************************************************
 *
 * 32 byte jump
 * 2048-32 byte
 *
 * 0x50000800
 *
 *************************************************************************/
#ifdef CONFIG_VOL_PARA
_vol_para:
	.word	vol_para
#endif

_hang:
	.word	do_hang

do_hang:
	b	do_hang

clear_bss:
	mov	r2, #0x00000000		/* clear                            */
clbss_l:str	r2, [r0]		/* clear loop...                    */
	add	r0, r0, #4
	cmp	r0, r1
	ble	clbss_l
	bx	r14

/*************************************************************************
 *
 * Startup Code (reset vector)
 *
 * do important init only if we don't start from memory!
 * setup Memory and board specific bits prior to relocation.
 * relocate armboot to ram
 * setup stack
 *
 *************************************************************************/

/*
 * These are defined in the board-specific linker script.
 */
.globl _bss_start_ofs
_bss_start_ofs:
	.word __bss_start

.globl _bss_end_ofs
_bss_end_ofs:
	.word __bss_end

/*
 * the actual reset code
 */

reset:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr,r0

	/*set up temp stack*/
	LDR 	sp, =SVC_STACK_TEMP

	/* the mask ROM code should have PLL and others stable */
	mrc 	p15, 0, r1, c1, c0, 0
	bic 	r1, #0x5
	mcr 	p15, 0, r1, c1, c0, 0

	bl	cpu_init_crit

	ldr 	r0, _bss_start_ofs
	ldr 	r1, _bss_end_ofs
	bl 	clear_bss

	bl	Chip_Init
	ldr     sp, =SPL_STACK 
	b	nand_boot

/*************************************************************************
 * void relocate_code (addr_sp, gd, addr_moni)
 *
 * This "function" does not return, instead it continues in RAM
 * after relocating the monitor code.
 *
 *************************************************************************/
	.globl	relocate_code
relocate_code:
	mov 	pc, lr

/*************************************************************************
 *
 * CPU_init_critical registers
 *
 * setup important registers
 * setup memory timing
 *
 *************************************************************************/
cpu_init_crit:
	/*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			@ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 12 (Z---) BTB
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Jump to board specific initialization...
	 * The Mask ROM will have already initialized
	 * basic memory. Go here to bump up clock rate and handle
	 * wake up conditions.
	 */
	mov	ip, lr			@ persevere link reg across call
	mov	lr, ip			@ restore link
	mov	pc, lr			@ back to my caller

