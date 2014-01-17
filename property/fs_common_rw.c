#include <common.h>
#include <linux/types.h>
#include <linux/string.h>
#include <nand.h>
#include <jffs2/jffs2.h>
#ifdef CONFIG_CMD_UBI
#include <ubi_uboot.h>
#endif
#ifdef  CONFIG_EMMC_BOOT
#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#endif

int do_fs_file_read(char *mpart, char *filenm, void *buf, int len)
{
	int ret=-1;
#ifdef CONFIG_FS_EXT4
	int mmc_dev=1;
	wchar_t *part=NULL;
	wchar_t *tmp;
	part = malloc(sizeof(wchar_t)*(strlen(mpart)+1));
	if(!part)
		return -1;
	tmp = part;
	while ((*tmp++ = *mpart++) != '\0')
		/* do nothing */;
	ret = ext4_read_content(mmc_dev,
			part,
			filenm,
			buf,
			0,/*not support non-zero offset*/
			len);
	free(part);
#elif defined(CONFIG_FS_UBIFS)
	static int is_ubifs_init = 0;
	if(is_ubifs_init == 0){
		ubifs_init();
		is_ubifs_init = 1;
	}

	ret = ubifs_mount(mpart);
	if(ret){
		printf("do_fs_file_read:mount %s failed!\n",mpart);
		return ret;
	}
	ret = ubifs_load(filenm, buf, len);
	if(ret)
		printf("do_fs_file_read:file %s not found!\n", filenm);
#endif
	return ret;
}

int do_fs_file_write(char *mpart, char *filenm, void *buf, int len)
{
	/*do not write in uboot now*/
	return -1;
}

int do_raw_data_read(char *part, u32 size, u32 off, char *buf)
{
	int ret =-1;
#ifdef  CONFIG_EMMC_BOOT
	u16 offp=0, len=0, left=0;
	u32 nsct, cursct;
	char *bufwp = buf;
	char *sctbuf=NULL;
	wchar_t *partition=NULL;
	wchar_t *tmp;
	disk_partition_t info;
	block_dev_desc_t *dev = NULL;

	if(NULL == buf)
		return -1;

	dev = get_dev("mmc", 1);
	if(NULL == dev){
		printf("get mmc dev failed!\n");
		return -1;
	}

	partition = malloc(sizeof(wchar_t)*(strlen(part)+1));
	if(!partition)
		return -1;
	tmp = partition;
	while ((*tmp++ = *part++) != '\0');

	offp = off%EMMC_SECTOR_SIZE;

	if(offp)
		len = EMMC_SECTOR_SIZE - offp;
	size = size - len;

	nsct = size/EMMC_SECTOR_SIZE;
	left = size%EMMC_SECTOR_SIZE;

	if(offp || left){
		sctbuf = malloc(EMMC_SECTOR_SIZE);
		if(!sctbuf) {
			free(partition);
			return -1;
		}
	}

	if(get_partition_info_by_name(dev, partition, &info))
		goto end;

	cursct = info.start + off/EMMC_SECTOR_SIZE;
	//read first unaligned data
	if(offp) {
		if(!Emmc_Read(PARTITION_USER, cursct, 1, sctbuf))
			goto end;
		cursct += 1;
		memcpy(bufwp,sctbuf+offp,len);
		bufwp += len;
	}
	//read sector aligned data
	if(nsct) {
		if(!Emmc_Read(PARTITION_USER, cursct, nsct, bufwp))
			goto end;
		cursct += nsct;
		bufwp += size-left;
	}
	//read last unaligned data
	if(left) {
		if(!Emmc_Read(PARTITION_USER, cursct, 1, sctbuf))
			goto end;
		memcpy(bufwp,sctbuf,left);
		bufwp += left;
	}
	ret = 0;
	//debug tmp
	printf("do_raw_data_read: wanted size :0x%x, real 0x%x\n",size+len,bufwp-buf);
end:
	free(partition);
	free(sctbuf);
#else
	int ubi_dev;
	struct ubi_volume_desc *vol;

	ubi_dev = nand_ubi_dev_init();
	if (ubi_dev<0) {
		printf("do_raw_data_read: ubi init failed.\n");
		return ret;
	}
	vol = ubi_open_volume_nm(ubi_dev, part, UBI_READONLY);
	if (IS_ERR(vol)) {
		printf("cannot open \"%s\", error %d",
			  part, (int)PTR_ERR(vol));
		return ret;
	}

	if(size != uboot_ubi_read(vol, buf, off, size)) 
		printf("%s: read vol %s failed!\n",__func__,part);
	else
		ret = 0;

	ubi_close_volume(vol);
#endif
	if (ret)
		printf("do_raw_data_read error.\n");
	return ret;
}

int do_raw_data_write(char *part, u32 updsz, u32 size, u32 off, char *buf)
{
	int ret =-1;
#ifdef  CONFIG_EMMC_BOOT
	//TODO
#else
	int i =0;
	int ubi_dev;
	u8 pnum;
	loff_t offset;
	size_t length,wlen=0;
	struct mtd_device *dev;
	struct mtd_info *nand;
	struct part_info *mtdpart;
	struct ubi_volume_desc *vol;
	nand_erase_options_t opts;

try_mtd:
	ret = find_dev_and_part(part, &dev, &pnum, &mtdpart);
	if (ret)
		goto try_ubi;
	else if (dev->id->type != MTD_DEV_TYPE_NAND)
		goto end;

	offset = mtdpart->offset+off;
	length = size;
	nand = &nand_info[dev->id->num];
	memset(&opts, 0x0, sizeof(opts));
	opts.offset = offset;
	opts.length = length;
	opts.quiet = 1;
	opts.spread = 1;

	ret = nand_erase_opts(nand, &opts);
	if (ret) {
		printf("erase %s failed.\n",part);
		goto end;
	}
	//write spl part with header
	if(strcmp(part, "spl")==0){
		ret = sprd_nand_write_spl(buf, nand);
		goto end;
	}

	while((size != wlen) && (i++<0xff)) {
		ret = nand_write_skip_bad(nand, offset, &length, buf);
		wlen += length;
		buf += length;
		offset += length;

		if(ret){
			//mark a block as badblock
			printf("nand write error %d, mark bad block 0x%llx\n",ret,offset&~(nand->erasesize-1));
			nand->block_markbad(nand,offset &~(nand->erasesize-1));
		}
	}
	goto end;

try_ubi:
	ubi_dev = nand_ubi_dev_init();
	if (ubi_dev<0) {
		printf("do_raw_data_write: ubi init failed.\n");
		return ret;
	}
	vol = ubi_open_volume_nm(ubi_dev, part, UBI_READWRITE);
	if (IS_ERR(vol)) {
		printf("cannot open \"%s\", error %d",
			  part, (int)PTR_ERR(vol));
		return ret;
	}

	//set total size to be updated in this volume
	if (updsz) {
		ret = ubi_start_update(vol->vol->ubi, vol->vol, updsz);
		if (ret < 0) {
			printf("Cannot start volume %s update\n",part);
			return ret;
		}
	}

	ret = ubi_more_update_data(vol->vol->ubi, vol->vol, buf, size);
	if (ret < 0) {
		printf("Couldnt write data in volume %s\n",part);
		return ret;
	}
	ret = 0;
	ubi_close_volume(vol);
#endif

end:
	if (ret)
		printf("do_raw_data_write error.\n");
	return ret;
}

