#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <image.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <android_recovery.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>

#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>
#include <fat.h>
#include <asm/byteorder.h>
#include <part.h>
#include <mmc.h>
#define msleep(a) udelay(a * 1000)
extern int prodinfo_read_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, int offset, char *buf, int len);


#ifdef dprintf
#undef dprintf
#endif
#define dprintf(fmt, args...) printf(fmt, ##args)

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
	if (!get_partition_info(p_block_dev, PARTITION_MISC, &info)) {
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (void *)buf)){
			printf("function: %s emcc read error\n", __FUNCTION__);
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
	if (!get_partition_info(p_block_dev, PARTITION_MISC, &info)) {
		memset(buf, 0, sizeof(buf));
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (void *)buf)){
			printf("function: %s emcc read error\n", __FUNCTION__);
			return -1;
		}
		memcpy((void *)buf, in, sizeof(*in));
		if(TRUE !=  Emmc_Write(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (void *)buf)){
			printf("function: %s emcc write error\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}
#ifdef CONFIG_GENERIC_MMC
#define BUF_ADDR 0x1000000
#define SD_NV_NAME "nvitem.bin"
#define NAND_NV_NAME " "
#define MODEM_PART "modem"
#define SD_MODEM_NAME "modem.bin"
#define DSP_PART "dsp"
#define SD_DSP_NAME "dsp.bin"
#define VM_PART "vmjaluna"
#define SD_VM_NAME "vmjaluna.bin"

void try_update_modem(void)
{
	struct mmc *mmc;
	block_dev_desc_t *dev_desc=NULL;
	block_dev_desc_t *p_block_dev = NULL;
	disk_partition_t info;

	int ret;
	int size;

	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		return -1;
	}

	mmc = find_mmc_device(0);
	if(mmc){
		ret = mmc_init(mmc);
		if(ret < 0){
			printf("mmc init failed %d\n", ret);
			return;
		}
	}else{
		printf("no mmc card found\n");
		return;
	}

	dev_desc = &mmc->block_dev;
	if(dev_desc==NULL){
		printf("no mmc block device found\n");
		return;
	}
	ret = fat_register_device(dev_desc, 1);
	if(ret < 0){
		printf("fat regist fail %d\n", ret);
		return;
	}
	ret = file_fat_detectfs();
	if(ret){
		printf("detect fs failed\n");
		return;
	}
	do{
		printf("reading %s\n", SD_NV_NAME);

		ret = do_fat_read(SD_NV_NAME, BUF_ADDR, 0, LS_NO);
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = FIXNV_SIZE + 4;
		nv_patch(BUF_ADDR, ret);
		nv_write_partition(p_block_dev, PARTITION_FIX_NV1, (char *)BUF_ADDR, size);
		nv_write_partition(p_block_dev, PARTITION_FIX_NV2, (char *)BUF_ADDR, size);

		file_fat_rm(SD_NV_NAME);
	}while(0);

	do{
		printf("reading %s\n", SD_MODEM_NAME);
		ret = do_fat_read(SD_MODEM_NAME, BUF_ADDR, 0, LS_NO);
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
		nv_write_partition(p_block_dev, PARTITION_MODEM, (char *)BUF_ADDR, size);

		file_fat_rm(SD_MODEM_NAME);
	}while(0);

	do{
		printf("reading %s\n", SD_DSP_NAME);
		ret = do_fat_read(SD_DSP_NAME, BUF_ADDR, 0, LS_NO);
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
		nv_write_partition(p_block_dev, PARTITION_DSP, (char *)BUF_ADDR, size);

		file_fat_rm(SD_DSP_NAME);
	}while(0);

	do{
		printf("reading %s\n", SD_VM_NAME);
		ret = do_fat_read(SD_VM_NAME, BUF_ADDR, 0, LS_NO);
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
		nv_write_partition(p_block_dev, PARTITION_VM, (char *)BUF_ADDR, size);

		file_fat_rm(SD_VM_NAME);
	}while(0);

	printf("update done\n");
	return;
}
#else
void try_update_modem(void)
{
}
#endif
