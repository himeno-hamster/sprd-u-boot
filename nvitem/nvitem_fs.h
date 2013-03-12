
#include "nvitem_common.h"

#ifndef _NVITEM_FS_H_
#define _NVITEM_FS_H_

typedef struct
{
	uint32	partId;
#ifdef CONFIG_EMMC_BOOT
	int		image_path;
	int		imageBak_path;
#else
	char		image_path[100];
	char		imageBak_path[100];
#endif
	uint32	image_size;
}RAM_NV_CONFIG;


typedef uint32 RAMDISK_HANDLE;

const RAM_NV_CONFIG*	ramDisk_Init(void);

RAMDISK_HANDLE	ramDisk_Open(uint32 partId);

BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size);

BOOLEAN		ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size);

void			ramDisk_Close(RAMDISK_HANDLE handle);


#endif


