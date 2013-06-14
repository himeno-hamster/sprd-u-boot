
#include <linux/types.h>
#include <common.h>
#include <part.h>
#include <mmc.h>
#include <fat.h>

//rootfs.cpio.gz
#define SD_RAMDISK   "ramdisk.img"

#define TEST_VERIFY_FILE ".sprdtest"

int load_sd_ramdisk(void* buf,int size)
{
	struct mmc *mmc;
	block_dev_desc_t *dev_desc=NULL;
	int ret = 0;
	char bufread[10];

	printf("%s: tick=%ld\n",__func__,get_ticks());

	mmc = find_mmc_device(0);
	if(mmc){
		ret = mmc_init(mmc);
		if(ret < 0){
			printf("mmc init failed %d,tick=%d\n", ret,get_ticks());
			return -1;
		}
	}else{
		printf("no mmc card found\n");
		return -1;
	}

	dev_desc = &mmc->block_dev;
	if(dev_desc==NULL){
		printf("no mmc block device found\n");
		return -1;
	}
	ret = fat_register_device(dev_desc, 1);
	if(ret < 0){
		printf("fat regist fail %d\n", ret);
		return -1;
	}
	ret = file_fat_detectfs();
	if(ret){
		printf("detect fs failed\n");
		return -1;
	}

	ret = file_fat_read(TEST_VERIFY_FILE, bufread, 10);
	if (ret<=0){
		printf("sd verity file read error %d\n", ret);
		return ret;
	}

	ret = file_fat_read(SD_RAMDISK, buf, size);
	if(ret <= 0){
		printf("sd file read error %d\n", ret);
	}
	printf("fat_read %d,tick=%ld\n", ret,get_ticks());
	return ret;
}

