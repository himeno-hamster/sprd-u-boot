
#ifndef _NVITEM_COMMON_H_
#define _NVITEM_COMMON_H_

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef unsigned char		BOOLEAN;
typedef unsigned char 		uint8;
typedef unsigned short		uint16;
typedef unsigned  int		uint32;

typedef signed char		int8;
typedef signed short		int16;
typedef signed int			int32;

#else
#include <config.h>
#include <common.h>
#include "../drivers/sdio/sdio_api.h"
#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include <part.h>
#endif

//-------------------------------------------------
//				Const config: can not be changed
//-------------------------------------------------
#define RAMNV_NUM					15		// max number of ramdisk, can not >= 15.
#define RAMNV_SECT_SIZE			512		// sect size of ramdisk

//-------------------------------------------------
//				Config: can be changed if nessarry
//-------------------------------------------------
#define RAMNV_DIRTYTABLE_MAXSIZE	32		// max sect number is (RAMNV_DIRTYTABLE_MAXSIZE << 5), 32 means 512k ramdisk


#endif

