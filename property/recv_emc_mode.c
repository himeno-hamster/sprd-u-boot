#include <common.h>
#include <android_recovery.h>
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include "normal_mode.h"

static char buf[8192];

int get_recovery_message(struct recovery_message *out)
{
	block_dev_desc_t *p_block_dev = NULL;
	disk_partition_t info;
	int size=8192;
	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		return -1;
	}
	if (!get_partition_info_by_name(p_block_dev, L"misc", &info)) {
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (void *)buf)){
			debugf("function: %s emcc read error\n", __FUNCTION__);
			return -1;
		}
	}

	memcpy(out, buf, sizeof(*out));
	return 0;
}

int set_recovery_message(const struct recovery_message *in)
{
	block_dev_desc_t *p_block_dev = NULL;
	disk_partition_t info;
	int size=8192;
	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		return -1;
	}
	if (!get_partition_info_by_name(p_block_dev, L"misc", &info)) {
		memset(buf, 0, sizeof(buf));
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (void *)buf)){
			debugf("function: %s emcc read error\n", __FUNCTION__);
			return -1;
		}
		memcpy((void *)buf, in, sizeof(*in));
		if(TRUE !=  Emmc_Write(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (void *)buf)){
			debugf("function: %s emcc write error\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}
