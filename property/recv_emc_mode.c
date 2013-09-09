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

#include "normal_mode.h"

#define msleep(a) udelay(a * 1000)
extern int prodinfo_read_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, int offset, char *buf, int len);


#ifdef dprintf
#undef dprintf
#endif
#define dprintf(fmt, args...) printf(fmt, ##args)
#define MAGIC_DATA_SAVE_OFFSET	(0x20/4)
#define CHECKSUM_SAVE_OFFSET	(0x24/4)
#define CHECKSUM_START_OFFSET	0x28
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
unsigned short eMMCCheckSum(const unsigned int *src, int len)
{
    unsigned int   sum = 0;
    unsigned short *src_short_ptr = PNULL;

    while (len > 3)
    {
        sum += *src++;
        len -= 4;
    }

    src_short_ptr = (unsigned short *) src;

    if (0 != (len&0x2))
    {
        sum += * (src_short_ptr);
        src_short_ptr++;
    }

    if (0 != (len&0x1))
    {
        sum += * ( (unsigned char *) (src_short_ptr));
    }

    sum  = (sum >> 16) + (sum & 0x0FFFF);
    sum += (sum >> 16);

    return (unsigned short) (~sum);
}
#if defined(CONFIG_TIGER) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
#define BOOTLOADER_HEADER_OFFSET 0x20
typedef struct{
	uint32 version;
	uint32 magicData;
	uint32 checkSum;
	uint32 hashLen;
}EMMC_BootHeader;
#endif
#define MAGIC_DATA	0xAA55A5A5
void splFillCheckData(unsigned int * splBuf,  int len)
{
#if defined(CONFIG_TIGER) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
	EMMC_BootHeader *header;
	header = (EMMC_BootHeader *)((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET);
	header->version  = 0;
	header->magicData= MAGIC_DATA;
	header->checkSum = (unsigned int)eMMCCheckSum((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET+sizeof(*header), CONFIG_SPL_LOAD_LEN-(BOOTLOADER_HEADER_OFFSET+sizeof(*header)));
	header->hashLen  = 0;
#else
	*(splBuf + MAGIC_DATA_SAVE_OFFSET) = MAGIC_DATA;
	*(splBuf + CHECKSUM_SAVE_OFFSET) = (unsigned int)eMMCCheckSum((unsigned int *)&splBuf[CHECKSUM_START_OFFSET/4], CONFIG_SPL_LOAD_LEN - CHECKSUM_START_OFFSET);
//	*(splBuf + CHECKSUM_SAVE_OFFSET) = splCheckSum(splBuf);
#endif
}
#if defined (CONFIG_SC8825) || defined (CONFIG_TIGER)
unsigned char *spl_eMMCBuf = (unsigned char*)0x82000000;
#else
unsigned char *spl_eMMCBuf = (unsigned char*)0x2000000;
#endif
#ifdef CONFIG_GENERIC_MMC
#define BUF_ADDR CONFIG_SYS_SDRAM_BASE+0x1000000
#define SD_NV_NAME "nvitem.bin"
#define NAND_NV_NAME " "
#define MODEM_PART "modem"
#define SD_MODEM_NAME "modem.bin"
#define SD_SPL_NAME "u-boot-spl-16k.bin"
#define DSP_PART "dsp"
#define SD_DSP_NAME "dsp.bin"
#define VM_PART "vmjaluna"
#define SD_VM_NAME "vmjaluna.bin"

//the following fixnv parameters's ids may need to be customized,please pay attention to this!!!
//calibration   0x2
#define FIXNV_CALIBRATION_ID        0x2
//IMEI 0x5,0x179,0x186,0x1e4,
#define FIXNV_IMEI1_ID        0x5
#define FIXNV_IMEI2_ID        0x179
#define FIXNV_IMEI3_ID        0x186
#define FIXNV_IMEI4_ID        0x1e4
//TD_calibration 0x516
#define FIXNV_TD_CALIBRATION_ID        0x516
//blue tooth 0x191
#define FIXNV_BT_ID        0x191
//band select 0xd
#define FIXNV_BAND_SELECT_ID        0xd
//WIFI 0x199
#define FIXNV_WIFI_ID        0x199
//MMITEST 0x19a
#define FIXNV_MMITEST_ID        0x19a

#define VLX_ADC_ID   2
#define VLX_RAND_TO_U32( _addr ) \
	if( (_addr) & 0x3 ){_addr += 0x4 -((_addr) & 0x3); }


extern unsigned short calc_checksum(unsigned char *dat, unsigned long len);
u32 Vlx_GetFixedBvitemAddr(u16 identifier, u32 search_start, u32 search_end)
{
	u32 start_addr, end_addr;
	u16 id, len;
	volatile u16 *flash_ptr;

	start_addr = search_start;
	end_addr   = search_end;
	start_addr += sizeof(u32); //skip update flag

	while(start_addr < end_addr)
	{
		flash_ptr = (volatile u16 *)(start_addr);
		id  = *flash_ptr++;
		len = *flash_ptr;
		if(0xFFFF == id)
		{
			return 0xffffffff;
		}
		if(identifier == id)
		{
			return (start_addr + 4);
		}
		else
		{
			start_addr += 4 + len +(len & 0x1);
			VLX_RAND_TO_U32( start_addr )
		}
	}
	return 0xffffffff;
}
void update_fixnv(unsigned char *old_addr,unsigned char*new_addr){
	unsigned short item_id = 0,i = 0,item_array[] = {FIXNV_CALIBRATION_ID,
		FIXNV_IMEI1_ID,
		FIXNV_IMEI2_ID,
		FIXNV_IMEI3_ID,
		FIXNV_IMEI4_ID,
		FIXNV_TD_CALIBRATION_ID,
		FIXNV_BT_ID,
		FIXNV_BAND_SELECT_ID,
		FIXNV_WIFI_ID,
		FIXNV_MMITEST_ID,
		0x0};
	unsigned int old_item_base = 0,new_item_base = 0,length = 0;
	unsigned short old_item_len=0, new_item_len=0;

	while(item_id = item_array[i++]){
		old_item_base = Vlx_GetFixedBvitemAddr(item_id, old_addr, (old_addr+FIXNV_SIZE));
		new_item_base = Vlx_GetFixedBvitemAddr(item_id, new_addr, (new_addr+FIXNV_SIZE));
		if(old_item_base == 0xffffffff || new_item_base == 0xffffffff){
			continue;
		}
		old_item_len =*(unsigned short*)(old_item_base-2);
		new_item_len =*(unsigned short*)(new_item_base-2);
		printf("item_id = 0x%x,old_item_len = %d,new_item_len = %d\n",item_id,new_item_len,old_item_len);
		if(old_item_len == new_item_len&&old_item_len != 0xffffffff){
			printf("copy from 0x%x to 0x%x and size = %d\n",old_item_base,new_item_base,old_item_len);
			memcpy(new_item_base,old_item_base,old_item_len);
		}
	}

	//length = Vlx_CalcFixnvLen(new_addr, new_addr+FIXNV_SIZE);
	//*((unsigned int*)&new_addr[FIXNV_SIZE-8])= length;//keep the real  fixnv file size.     
	//*(unsigned short*)&new_addr[FIXNV_SIZE-2] = crc16(0,(unsigned const char*)new_addr,FIXNV_SIZE-2);
	*(unsigned int*)&new_addr[FIXNV_SIZE] = 0x5a5a5a5a;
	memcpy(old_addr,new_addr,FIXNV_SIZE+4);
	*(unsigned short *)&new_addr[FIXNV_SIZE-4] = calc_checksum(new_addr, FIXNV_SIZE - 4);
	*(unsigned short *)&new_addr[FIXNV_SIZE-2] = 0;
}

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
#if !defined(CONFIG_ROCKBOX_FAT)
		ret = do_fat_read(SD_NV_NAME, BUF_ADDR, 0, LS_NO);
#else
		ret = file_fat_read(SD_NV_NAME, BUF_ADDR, 0);
#endif
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = FIXNV_SIZE + 4;
		nv_patch(BUF_ADDR, ret);
#ifdef CONFIG_SC8830
		//sc8830 do it later
#else
		memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
		if(0 == nv_read_partition(p_block_dev, PARTITION_FIX_NV1, (char *)FIXNV_ADR, FIXNV_SIZE + 4)){
			if (1 == fixnv_is_correct_endflag((unsigned char *)FIXNV_ADR, FIXNV_SIZE)){
				update_fixnv(FIXNV_ADR,BUF_ADDR);	
				goto SKIP;
			}else{
				goto NEXT;
			}
		}else{
			goto NEXT;
		}
#endif

NEXT:
#ifdef CONFIG_SC8830
			//sc8830 do it later
#else
		memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
		if(0 == nv_read_partition(p_block_dev, PARTITION_FIX_NV2, (char *)FIXNV_ADR, FIXNV_SIZE + 4)){
			if (1 == fixnv_is_correct_endflag((unsigned char *)FIXNV_ADR, FIXNV_SIZE)){
				update_fixnv(FIXNV_ADR,BUF_ADDR);	
			}else{
				printf("both of the fixnv files are not correct,skip parameter backup process!\n");
				break;
			}
		}else{
			printf("both of the fixnv files are not correct,skip parameter backup process!\n");
			break;
		}
#endif

SKIP:
#ifdef CONFIG_SC8830
				//sc8830 do it later
#else
		nv_write_partition(p_block_dev, PARTITION_FIX_NV1, (char *)BUF_ADDR, size);
		nv_write_partition(p_block_dev, PARTITION_FIX_NV2, (char *)BUF_ADDR, size);
#endif
		file_fat_rm(SD_NV_NAME);
	}while(0);
#ifndef CONFIG_SC8830
	do{
		printf("reading %s\n", SD_SPL_NAME);
#if !defined(CONFIG_ROCKBOX_FAT)
                ret = do_fat_read(SD_SPL_NAME, spl_eMMCBuf, 0, LS_NO);
#else
                ret = file_fat_read(SD_SPL_NAME, spl_eMMCBuf, 0);
#endif
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
		splFillCheckData((unsigned int *) spl_eMMCBuf, (int)size);
		//nv_write_partition(p_block_dev, PARTITION_MODEM, (char *)BUF_ADDR, size);
		if (TRUE !=  Emmc_Write(PARTITION_BOOT1, 0, CONFIG_SPL_LOAD_LEN / EMMC_SECTOR_SIZE, (uint8*)spl_eMMCBuf)) {
			printf("emmc image read error \n");
			ret = 1; /* fail */
		}
		printf("spl update done\n");
		file_fat_rm(SD_SPL_NAME);
	}while(0);
#endif
	do{
		printf("reading %s\n", SD_MODEM_NAME);
#if !defined(CONFIG_ROCKBOX_FAT)
		ret = do_fat_read(SD_MODEM_NAME, BUF_ADDR, 0, LS_NO);
#else
		ret = file_fat_read(SD_MODEM_NAME, BUF_ADDR, 0);
#endif
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
#ifdef CONFIG_SC8830
				//sc8830 do it later
#else
		nv_write_partition(p_block_dev, PARTITION_MODEM, (char *)BUF_ADDR, size);
#endif
		file_fat_rm(SD_MODEM_NAME);
	}while(0);

	do{
		printf("reading %s\n", SD_DSP_NAME);
#if !defined(CONFIG_ROCKBOX_FAT)
		ret = do_fat_read(SD_DSP_NAME, BUF_ADDR, 0, LS_NO);
#else
		ret = file_fat_read(SD_DSP_NAME, BUF_ADDR, 0);
#endif
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
#ifdef CONFIG_SC8830
				//sc8830 do it later
#else
		nv_write_partition(p_block_dev, PARTITION_DSP, (char *)BUF_ADDR, size);
#endif
		file_fat_rm(SD_DSP_NAME);
	}while(0);

	do{
		printf("reading %s\n", SD_VM_NAME);
#if !defined(CONFIG_ROCKBOX_FAT)
		ret = do_fat_read(SD_VM_NAME, BUF_ADDR, 0, LS_NO);
#else
		ret = file_fat_read(SD_VM_NAME, BUF_ADDR, 0);
#endif
		if(ret <= 0){
			printf("sd file read error %d\n", ret);
			break;
		}
		size = ret;
#ifndef CONFIG_SC8830
		nv_write_partition(p_block_dev, PARTITION_VM, (char *)BUF_ADDR, size);
#endif
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
