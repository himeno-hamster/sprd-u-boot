#ifndef _FDL_EMMC_OPERATE_H
#define _FDL_EMMC_OPERATE_H

#include <asm/arch/fdl_stdio.h>
#include <asm/arch/cmd_def.h>
#include <asm/arch/packet.h>
#ifdef CONFIG_EMMC_BOOT
#include "../../../disk/part_uefi.h"
#include "../../../include/part.h"

#define EMMC_SUCCESS                0
#define EMMC_SYSTEM_ERROR           1
#define EMMC_DEVICE_INIT_ERROR      2
#define EMMC_INVALID_DEVICE_SIZE    3
#define EMMC_INCOMPATIBLE_PART      4
#define EMMC_INVALID_ADDR           5
#define EMMC_INVALID_SIZE           6

static __inline DLSTATUS convert_err (int err)
{
    switch (err)
    {
        case EMMC_SUCCESS:
            return BSL_REP_ACK;
        case EMMC_INVALID_ADDR:
            return BSL_REP_DOWN_DEST_ERROR;
        case EMMC_INVALID_SIZE:
            return BSL_REP_DOWN_SIZE_ERROR;
        case EMMC_DEVICE_INIT_ERROR:
            return BSL_UNKNOWN_DEVICE;
        case EMMC_INVALID_DEVICE_SIZE:
            return BSL_INVALID_DEVICE_SIZE;
        case EMMC_INCOMPATIBLE_PART:
            return BSL_INCOMPATIBLE_PARTITION;
        default:
            return BSL_REP_OPERATION_FAILED;
    }
}

typedef enum _PARTITION_IMG_TYPE
{
	IMG_RAW = 0,
	IMG_WITH_SPARSE = 1,
	IMG_TYPE_MAX
}PARTITION_IMG_TYPE;

typedef enum _PARTITION_PURPOSE
{
	PARTITION_PURPOSE_NORMAL,
	PARTITION_PURPOSE_NV,
	PARTITION_PURPOSE_MAX
}PARTITION_PURPOSE;

typedef struct DL_EMMC_STATUS_TAG
{
	uint32 part_total_size ;
	uint32 base_sector;
	wchar_t *curUserPartitionName;
	PARTITION_PURPOSE partitionpurpose;
	uint8 curEMMCArea;
	PARTITION_IMG_TYPE curImgType; 
} DL_EMMC_STATUS;

typedef struct DL_FILE_STATUS_TAG
{
	unsigned long   total_size;
	unsigned long   total_recv_size;
	unsigned long   unsave_recv_size;
} DL_EMMC_FILE_STATUS;

typedef struct _SPECIAL_PARTITION_CFG
{
	wchar_t* partition;
	wchar_t* bak_partition;
	PARTITION_IMG_TYPE imgattr;
	PARTITION_PURPOSE purpose;
}SPECIAL_PARTITION_CFG;

#if defined(CONFIG_TIGER) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
#define BOOTLOADER_HEADER_OFFSET 0x20
typedef struct{
	uint32 version;
	uint32 magicData;
	uint32 checkSum;
	uint32 hashLen;
}EMMC_BootHeader;
#endif

PUBLIC int fdl2_download_start(wchar_t * partition_name, unsigned long size, unsigned long nv_checksum);
PUBLIC int fdl2_download_midst(unsigned short size, char *buf);
PUBLIC int fdl2_download_end(void);
PUBLIC int fdl2_read_start(wchar_t* partition_name, unsigned long size);
PUBLIC int fdl2_read_midst(unsigned long size, unsigned long off, unsigned char * buf);
PUBLIC int fdl2_read_end(void);
PUBLIC int fdl2_erase(wchar_t * partition_name, unsigned long size);
PUBLIC int fdl2_repartition(unsigned short * partition_cfg, unsigned short total_partition_num);

#endif  //CONFIG_EMMC_BOOT
#endif  //_FDL_EMMC_OPERATE_H
