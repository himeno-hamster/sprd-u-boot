/***************************************************************************
 * Copyright (C) 2008 by Frank Gevaerts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************
 * Source file dumped from rockbox-3.1 distribution.
 * Copyright (C) 2008 by Frank Gevaerts
 ****************************************************************************
 * See file CREDITS for list of people who contributed to the U-boot 
 * project.
 *
 * 01-jan-2008 etienne.carriere@stnwireless.com - Port to U-boot from rockbox-3.1
 ***************************************************************************
 */
#ifndef __MV_H__
#define __MV_H__

/* FixMe: These macros are a bit nasty and perhaps misplaced here.
 *  We'll get rid of them once decided on how to proceed with multivolume. 
 * U-boot port: well do not support HAVE_MULTIVOLUME !!! 
 * that not U-boot config switch syntax: keep it like that to easy next upgrade...
 */
#ifdef HAVE_MULTIVOLUME
#define IF_MV(x) x /* optional volume/drive parameter */
#define IF_MV2(x,y) x,y /* same, for a list of arguments */
#define IF_MV_NONVOID(x) x /* for prototype with sole volume parameter */
#define NUM_VOLUMES 2
#else /* empty definitions if no multi-volume */
#define IF_MV(x)
#define IF_MV2(x,y)
#define IF_MV_NONVOID(x) void
#define NUM_VOLUMES 1
#endif

#endif
