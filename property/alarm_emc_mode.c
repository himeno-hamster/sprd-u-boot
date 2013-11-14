#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>
#include <boot_mode.h>


#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>
#define PRODUCTINFO_SIZE	(3 * 1024)
extern int prodinfo_read_partition(block_dev_desc_t *p_block_dev, wchar_t *partition, int offset, char *buf, int len);
extern unsigned long char2u32(unsigned char *buf, int offset);


int move2timebuf(unsigned char *src, unsigned char *dst)
{
	int len = 0;
	if(src[len] == 0xff)
		return -1;
	while (src[len] != 0x0a)
		len ++;

	len ++;
	while (src[len] != 0x0a)
		len ++;
	len ++;
	memcpy(dst, src, len);
	return 1;
}

static wchar_t *alarm_partition = L"prodnv";
int alarm_file_check(char *time_buf)
{
	if(!do_fs_file_read(PROD_PART, "/alarm_flag", time_buf,200)){
		return 1;
	}else{
		return -1;
	}
}
int poweron_file_check(char *time_buf)
{
	if(!do_fs_file_read(PROD_PART, "/poweron_timeinmillis", time_buf,200)){
		return 1;
	}else{
		return -1;
	}
}
