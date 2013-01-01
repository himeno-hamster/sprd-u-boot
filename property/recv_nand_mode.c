#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <image.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>
#include <boot_mode.h>
#include <android_recovery.h>
#include <fat.h>
#include <asm/byteorder.h>
#include <part.h>
#include <mmc.h>

#ifdef dprintf
#undef dprintf
#endif
#define dprintf(fmt, args...) printf(fmt, ##args)



static const int MISC_COMMAND_PAGE = 1;		// bootloader command is this page
static char buf[8192];



int get_recovery_message(struct recovery_message *out)
{
	loff_t offset = 0;
	unsigned pagesize;
	size_t size;

	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;

	ret = mtdparts_init();
	if(ret != 0){
		dprintf("mtdparts init error %d\n", ret);
		return -1;
	}

	ret = find_dev_and_part("misc", &dev, &pnum, &part);
	if(ret){
		dprintf("No partiton named %s found\n", "misc");
		return -1;
	}else if(dev->id->type != MTD_DEV_TYPE_NAND){
        printf("Partition %s not a NAND device\n", "misc");
        return -1;
    }

	nand = &nand_info[dev->id->num];
	pagesize = nand->writesize;

	offset = pagesize * MISC_COMMAND_PAGE + part->offset;
	size = pagesize;
	ret = nand_read_skip_bad(nand, offset, &size, (void *)buf);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
		return -1;
	}

	memcpy(out, buf, sizeof(*out));
	return 0;
}

int set_recovery_message(const struct recovery_message *in)
{
	loff_t offset = 0;
	unsigned pagesize;
	size_t size;

	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;

	ret = mtdparts_init();
	if(ret != 0){
		dprintf("mtdparts init error %d\n", ret);
		return -1;
	}

	ret = find_dev_and_part("misc", &dev, &pnum, &part);
	if(ret){
		dprintf("No partiton named %s found\n", "misc");
		return -1;
	}else if(dev->id->type != MTD_DEV_TYPE_NAND){
		dprintf("Partition %s not a NAND device\n", "misc");
		return -1;
	}

	nand = &nand_info[dev->id->num];
	pagesize = nand->writesize;

	size = pagesize*(MISC_COMMAND_PAGE + 1);

	ret = nand_read_skip_bad(nand, part->offset, &size, (void *)SCRATCH_ADDR);
	if(ret != 0){
		dprintf("%s: nand read error %d\n", __FUNCTION__, ret);
		return -1;
	}


	offset = SCRATCH_ADDR;
	offset += (pagesize * MISC_COMMAND_PAGE);
	memcpy(offset, in, sizeof(*in));

	nand_erase_options_t opts;
	memset(&opts, 0, sizeof(opts));
	opts.offset = part->offset;
	opts.length = pagesize *(MISC_COMMAND_PAGE + 1);
	opts.jffs2 = 0;
	opts.scrub = 0;
	opts.quiet = 1;
	ret = nand_erase_opts(nand, &opts);
	if(ret != 0){
		dprintf("%s, nand erase error %d\n", __FUNCTION__, ret);
		return -1;
	}
	ret = nand_write_skip_bad(nand, part->offset, &size, (void *)SCRATCH_ADDR);
	if(ret != 0){
		dprintf("%s, nand erase error %d\n", __FUNCTION__, ret);
		return -1;
	}

}
#ifdef CONFIG_GENERIC_MMC
#define FIX_SIZE (64*1024)

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
	loff_t off, size;
	nand_info_t *nand;
	struct mtd_device *dev;
	u8 pnum;
	struct part_info *part;
	int ret;
	char *fixnvpoint = "/fixnv";
	//char *fixnvfilename = "/fixnv/fixnv.bin";
	char *fixnvfilename = "/fixnv/fixnvchange.bin";
	nand_erase_options_t opts;

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
		size = FIX_SIZE + 4;
		nv_patch(BUF_ADDR, ret);
		cmd_yaffs_mount(fixnvpoint);
		cmd_yaffs_mwrite_file(fixnvfilename, BUF_ADDR, size);
		cmd_yaffs_umount(fixnvpoint);

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
		ret = find_dev_and_part(MODEM_PART, &dev, &pnum, &part);
		if (ret) {
			printf("No partition named %s\n", MODEM_PART);
			break;
		} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
			printf("Partition %s not a NAND device\n", MODEM_PART);
			break;
		}
		off = part->offset;
		nand = &nand_info[dev->id->num];
		memset(&opts, 0, sizeof(opts));
		opts.offset = off;
		opts.length = part->size;
		opts.quiet  = 1;
		ret = nand_erase_opts(nand, &opts);
		if(ret){
			printf("nand erase bad %d\n", ret);
			break;
		}
		ret = nand_write_skip_bad(nand, off, &size, BUF_ADDR);
		if(ret){
			printf("nand write bad %d\n", ret);
			break;
		}

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
		ret = find_dev_and_part(DSP_PART, &dev, &pnum, &part);
		if (ret) {
			printf("No partition named %s\n", DSP_PART);
			break;
		} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
			printf("Partition %s not a NAND device\n", DSP_PART);
			break;
		}
		off = part->offset;
		nand = &nand_info[dev->id->num];
		memset(&opts, 0, sizeof(opts));
		opts.offset = off;
		opts.length = part->size;
		opts.quiet  = 1;
		ret = nand_erase_opts(nand, &opts);
		if(ret){
			printf("nand erase bad %d\n", ret);
			break;
		}
		ret = nand_write_skip_bad(nand, off, &size, BUF_ADDR);
		if(ret < 0){
			printf("nand write bad %d\n", ret);
			break;
		}

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
		ret = find_dev_and_part(VM_PART, &dev, &pnum, &part);
		if (ret) {
			printf("No partition named %s\n", VM_PART);
			break;
		} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
			printf("Partition %s not a NAND device\n", VM_PART);
			break;
		}
		off = part->offset;
		nand = &nand_info[dev->id->num];
		memset(&opts, 0, sizeof(opts));
		opts.offset = off;
		opts.length = part->size;
		opts.quiet  = 1;
		ret = nand_erase_opts(nand, &opts);
		if(ret){
			printf("nand erase bad %d\n", ret);
			break;
		}
		ret = nand_write_skip_bad(nand, off, &size, BUF_ADDR);
		if(ret < 0){
			printf("nand write bad %d\n", ret);
			break;
		}

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
