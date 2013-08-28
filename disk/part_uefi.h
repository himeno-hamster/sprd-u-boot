#ifndef _DISK_PART_UEFI_H
#define _DISK_PART_UEFI_H

#include <common.h>

typedef enum _PARTITION_ATTR_TAG
{
	PARTITION_RAW = 0,
	PARTITION_EXT4 = 1,
	PARTITION_ATTR_COUNT
}PARTITION_ATTR_TAG;

//#define MAX_PARTITION_INFO	(((PARTITION_TOTAL_COUNT + 3) / 4) * 4)
#define MAX_PARTITION_INFO	128

#define MAX_SIZE_FLAG	0xFFFFFFFF

#define MAX_PARTITION_NAME_SIZE  (72)  //72byte  36 utf-16le code units
#define MAX_UTF_PARTITION_NAME_LEN  (36)

#define STARTING_LBA_OF_FIRST_PARTITION   (0x800)

typedef struct _PARTITION_CFG
{
	unsigned int partition_index;
	unsigned int partition_size;  //
	unsigned int partition_attr;
	unsigned int partition_offset;
/*
 In kernel #define PARTITION_META_INFO_VOLNAMELTH	64
 In u-boot disk_partition name 32
	   gpt entry efi_char16_t partition_name[72 / sizeof(efi_char16_t)];
*/
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN];
} __attribute__ ((packed)) PARTITION_CFG,*PPARTITION_CFG;


unsigned int write_uefi_partition_table(PARTITION_CFG *p_partition_cfg);

#endif	/* _DISK_PART_UEFI_H */
