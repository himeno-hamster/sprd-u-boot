#include <config.h>
#include "normal_mode.h"

#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>
#ifdef BOOTING_BACKUP_NVCALIBRATION
#include "backupnvitem.h"
#endif

#define KERNL_PAGE_SIZE 2048
int get_partition_info (block_dev_desc_t *dev_desc, int part, disk_partition_t *info);

int nv_erase_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part)
{
	disk_partition_t info;
	int ret = 0; /* success */
#ifdef CONFIG_SC8830
	unsigned char *tmpbuf = (unsigned char *)TDMODEM_ADR;
#else
	unsigned char *tmpbuf = (unsigned char *)MODEM_ADR;
#endif
	if (!get_partition_info(p_block_dev, part, &info)) {
		memset(tmpbuf, 0xff, info.size * EMMC_SECTOR_SIZE);
		//printf("part = %d  info.start = 0x%08x  info.size = 0x%08x\n", part, info.start, info.size);
		if (TRUE !=  Emmc_Write(PARTITION_USER, info.start, info.size, (unsigned char *)tmpbuf)) {
			printf("emmc image erase error \n");
			ret = 1; /* fail */
		}
	}

	return ret;
}

int nv_read_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = (len +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	int ret = 0; /* success */

	if (!get_partition_info(p_block_dev, part, &info)) {
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, size / EMMC_SECTOR_SIZE, (uint8*)buf)) {
			printf("emmc image read error \n");
			ret = 1; /* fail */
		}
	}

	return ret;
}

int Calibration_read_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = (len +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	int ret = 0; /* success */

	if (!get_partition_info(p_block_dev, part, &info)) {
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, size / EMMC_SECTOR_SIZE, (uint8*)buf)) {
			printf("emmc image read error \n");
			ret = -1; /* fail */
		}
	}
	return ret;
}

unsigned long char2u32(unsigned char *buf, int offset)
{
	unsigned long ret = 0;

	ret = (buf[offset + 3] & 0xff) \
		| ((buf[offset + 2] & 0xff) << 8) \
		| ((buf[offset + 1] & 0xff) << 16) \
		| ((buf[offset] & 0xff) << 24);

	return ret;
}

int prodinfo_read_partition_flag(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, int offset, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = (len +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	int ret = 0; /* success */
	unsigned long crc;
	unsigned long offset_block = offset / EMMC_SECTOR_SIZE;

	if (!get_partition_info(p_block_dev, part, &info)) {
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start + offset_block, size / EMMC_SECTOR_SIZE, (uint8*)buf)) {
			printf("emmc image read error : %d\n", offset);
			ret = 1; /* fail */
		}
	} else
		ret = 1;

	if (ret == 1)
		return ret;

	crc = crc32b(0xffffffff, buf, len);

	if (offset == 0) {
		/* phasecheck */
		if ((buf[PRODUCTINFO_SIZE + 7] == (crc & 0xff)) \
			&& (buf[PRODUCTINFO_SIZE + 6] == ((crc & (0xff << 8)) >> 8)) \
			&& (buf[PRODUCTINFO_SIZE + 5] == ((crc & (0xff << 16)) >> 16)) \
			&& (buf[PRODUCTINFO_SIZE + 4] == ((crc & (0xff << 24)) >> 24))) {
			ret = 0;
		} else
			ret = 1;
	} else if ((offset == 4096) || (offset == 8192) || (offset == 12288)) {
		/* factorymode or alarm mode */
		if ((buf[PRODUCTINFO_SIZE + 11] == (crc & 0xff)) \
			&& (buf[PRODUCTINFO_SIZE + 10] == ((crc & (0xff << 8)) >> 8)) \
			&& (buf[PRODUCTINFO_SIZE + 9] == ((crc & (0xff << 16)) >> 16)) \
			&& (buf[PRODUCTINFO_SIZE + 8] == ((crc & (0xff << 24)) >> 24))) {
			ret = 0;
		} else
			ret = 1;
	} else
		ret = 1;

	return ret;
}

int prodinfo_read_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, int offset, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = (len +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	int ret = 0; /* success */
	unsigned long crc;
	unsigned long offset_block = offset / EMMC_SECTOR_SIZE;

	if (!get_partition_info(p_block_dev, part, &info)) {
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start + offset_block, size / EMMC_SECTOR_SIZE, (uint8*)buf)) {
			printf("emmc image read error : %d\n", offset);
			ret = 1; /* fail */
		}
	} else
		ret = 1;

	if (ret == 1)
		return ret;

	crc = crc32b(0xffffffff, buf, len);
	if (offset == 0) {
		/* phasecheck */
		if ((buf[PRODUCTINFO_SIZE + 7] == (crc & 0xff)) \
			&& (buf[PRODUCTINFO_SIZE + 6] == ((crc & (0xff << 8)) >> 8)) \
			&& (buf[PRODUCTINFO_SIZE + 5] == ((crc & (0xff << 16)) >> 16)) \
			&& (buf[PRODUCTINFO_SIZE + 4] == ((crc & (0xff << 24)) >> 24))) {
				buf[PRODUCTINFO_SIZE + 7] = 0xff;
				buf[PRODUCTINFO_SIZE + 6] = 0xff;
				buf[PRODUCTINFO_SIZE + 5] = 0xff;
				buf[PRODUCTINFO_SIZE + 4] = 0xff;
				/* clear 5a flag */
				buf[PRODUCTINFO_SIZE] = 0xff;
				buf[PRODUCTINFO_SIZE + 1] = 0xff;
				buf[PRODUCTINFO_SIZE + 2] = 0xff;
				buf[PRODUCTINFO_SIZE + 3] = 0xff;
		} else
			ret = 1;
	} else if ((offset == 4096) || (offset == 8192) || (offset == 12288)) {
		/* factorymode or alarm mode */
		if ((buf[PRODUCTINFO_SIZE + 11] == (crc & 0xff)) \
			&& (buf[PRODUCTINFO_SIZE + 10] == ((crc & (0xff << 8)) >> 8)) \
			&& (buf[PRODUCTINFO_SIZE + 9] == ((crc & (0xff << 16)) >> 16)) \
			&& (buf[PRODUCTINFO_SIZE + 8] == ((crc & (0xff << 24)) >> 24))) {
				buf[PRODUCTINFO_SIZE + 11] = 0xff;
				buf[PRODUCTINFO_SIZE + 10] = 0xff;
				buf[PRODUCTINFO_SIZE + 9] = 0xff;
				buf[PRODUCTINFO_SIZE + 8] = 0xff;
		} else
			ret = 1;
	} else
		ret = 1;

	return ret;
}

int nv_write_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = (len +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	int ret = 0; /* success */

	if (!get_partition_info(p_block_dev, part, &info)) {
		if (TRUE !=  Emmc_Write(PARTITION_USER, info.start, size / EMMC_SECTOR_SIZE, (uint8*)buf)) {
			printf("emmc image read error \n");
			ret = 1; /* fail */
		}
	}

	return ret;
}


#ifdef BOOTING_BACKUP_NVCALIBRATION
void get_nvitem_from_hostpc(unsigned char *Buffer)
{
	/* samsung implement : get nvitem from HOST PC */

}

int eMMC_nv_is_correct(unsigned char *array, unsigned long size)
{
	if ((array[size] == 0x5a) && (array[size + 1] == 0x5a) && (array[size + 2] == 0x5a) && (array[size + 3] == 0x5a)) {
		array[size] = 0xff; array[size + 1] = 0xff;
		array[size + 2] = 0xff; array[size + 3] = 0xff;
		return 1;
	} else
		return -1;
}

unsigned long Boot_GetPartBaseSec(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part)
{
	disk_partition_t info;

	if (!get_partition_info(p_block_dev, part, &info))
		return info.start;

	return 0;
}

void dump_nvitem(NV_BACKUP_ITEM_T *nvitem)
{
	int cnt2;

	printf("\nszItemName = %s  wIsBackup = %d  wIsUseFlag = %d  dwID = 0x%08x  dwFlagCount = %d\n", nvitem->szItemName, nvitem->wIsBackup, nvitem->wIsUseFlag, nvitem->dwID, nvitem->dwFlagCount);

	for (cnt2 = 0; cnt2 < nvitem->dwFlagCount; cnt2 ++) {
		printf("szFlagName = %s  dwCheck = %d\n", nvitem->nbftArray[cnt2].szFlagName, nvitem->nbftArray[cnt2].dwCheck);
	}
}

#endif

int read_logoimg(char *bmp_img,size_t size)
{
	block_dev_desc_t *p_block_dev = NULL;
	disk_partition_t info;

	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		return -1;
	}
	if (!get_partition_info(p_block_dev, PARTITION_LOGO, &info)) {
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, bmp_img)){
			printf("function: %s nand read error\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}

int is_factorymode()
{
	int ret = 0;
	block_dev_desc_t *p_block_dev = NULL;

	printf("Checking factorymode : ");
	p_block_dev = get_dev("mmc", 1);
	int factoryalarmret1, factoryalarmret2;
	unsigned long factoryalarmcnt1, factoryalarmcnt2;
	unsigned char factoryalarmarray1[PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE];
	unsigned char factoryalarmarray2[PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE];

	memset((unsigned char *)factoryalarmarray1, 0xff, PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE);
	factoryalarmret1 = prodinfo_read_partition(p_block_dev, PARTITION_PROD_INFO1, 4 * 1024, (char *)factoryalarmarray1, PRODUCTINFO_SIZE + 8);
	memset((unsigned char *)factoryalarmarray2, 0xff, PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE);
	factoryalarmret2 = prodinfo_read_partition(p_block_dev, PARTITION_PROD_INFO2, 4 * 1024, (char *)factoryalarmarray2, PRODUCTINFO_SIZE + 8);

	if ((factoryalarmret1 == 0) && (factoryalarmret2 == 0)) {
		factoryalarmcnt1 = char2u32(factoryalarmarray1, 3 * 1024 + 4);
		factoryalarmcnt2 = char2u32(factoryalarmarray2, 3 * 1024 + 4);
		if (factoryalarmcnt2 >= factoryalarmcnt1) {
			if (factoryalarmarray2[0] == 0x31)
				ret = 1;
		} else {
			if (factoryalarmarray1[0] == 0x31)
				ret = 1;
		}
	} else if ((factoryalarmret1 == 0) && (factoryalarmret2 == 1)) {
		if (factoryalarmarray1[0] == 0x31)
			ret = 1;
	} else if ((factoryalarmret1 == 1) && (factoryalarmret2 == 0)) {
		if (factoryalarmarray2[0] == 0x31)
			ret = 1;
	} else if ((factoryalarmret1 == 1) && (factoryalarmret2 == 1))
		printf("0\n");
	return ret;
}
void addbuf(char *buf)
{
}

void addcmdline(char *buf)
{
#if (!BOOT_NATIVE_LINUX) || BOOT_NATIVE_LINUX_MODEM
#if defined (CONFIG_SC8830)
	/* tdfixnv=0x????????,0x????????*/
	int str_len = strlen(buf);
	sprintf(&buf[str_len], " tdfixnv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", TDFIXNV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", FIXNV_SIZE);

	/* tdruntimenv=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " tdruntimenv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", TDRUNTIMENV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", RUNTIMENV_SIZE);

	/* wfixnv=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " wfixnv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", WFIXNV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", FIXNV_SIZE);

	/* wruntimenv=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " wruntimenv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", WRUNTIMENV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", RUNTIMENV_SIZE);

	/* productinfo=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " productinfo=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", PRODUCTINFO_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", PRODUCTINFO_SIZE);
#else
	/* fixnv=0x????????,0x????????*/
	int str_len = strlen(buf);
	sprintf(&buf[str_len], " fixnv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", FIXNV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", FIXNV_SIZE);

	/* productinfo=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " productinfo=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", PRODUCTINFO_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", PRODUCTINFO_SIZE);

	/* productinfo=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " runtimenv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", RUNTIMENV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", RUNTIMENV_SIZE);
#endif
#endif
#if BOOT_NATIVE_LINUX_MODEM
	str_len = strlen(buf);
	buf[str_len] = '\0';
	char* nv_infor = (char*)(((volatile u32*)CALIBRATION_FLAG));
	sprintf(nv_infor, buf);
	nv_infor[str_len] = '\0';
	printf("nv_infor:[%08x]%s \n", nv_infor, nv_infor);
#if defined (CONFIG_SC8830)
	nv_infor = (char*)(((volatile u32*)CALIBRATION_FLAG_WCDMA));
	sprintf(nv_infor, buf);
	nv_infor[str_len] = '\0';
	printf("nv_infor:[%08x]%s \n", nv_infor, nv_infor);
#endif
#endif
}

int read_spldata()
{
	int size = CONFIG_SPL_LOAD_LEN;
	if(TRUE !=  Emmc_Read(PARTITION_BOOT1, 0, size/EMMC_SECTOR_SIZE, (uint8*)spl_data)){
		printf("vmjaluna nand read error \n");
		return -1;
	}
	return 0;
}

/*The function is temporary, will move to the chip directory*/
#if BOOT_NATIVE_LINUX_MODEM
void modem_entry()
{
#ifdef CONFIG_SC8825
	//  *(volatile u32 *)0x4b00100c=0x100;
	u32 cpdata[3] = {0xe59f0000, 0xe12fff10, MODEM_ADR};
	*(volatile u32*)0x20900250 = 0;//disbale cp clock, cp iram select to ap
	//*(volatile u32*)0x20900254 = 0;// hold cp
	memcpy((volatile u32*)0x30000, cpdata, sizeof(cpdata));
	*(volatile u32*)0x20900250 =0xf;// 0x3;//enale cp clock, cp iram select to cp
	*(volatile u32*)0x20900254 = 1;// reset cp
#elif defined (CONFIG_SC8830)
	u32 state;
	u32 cp0data[3] = {0xe59f0000, 0xe12fff10, WMODEM_ADR};
	u32 cp1data[3] = {0xe59f0000, 0xe12fff10, TDMODEM_ADR};
	memcpy(0x50000000, cp0data, sizeof(cp0data));      /* copy cp0 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000001;       /* reset cp0 */
	*((volatile u32*)0x402B003C) &= ~0x02000000;       /* clear cp0 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00B8);
		if (!(state & (0xf<<28)))
			break;
	}
	*((volatile u32*)0x402B003C) &= ~0x10000000;       /* clear cp0 force deep sleep */

	memcpy(0x50001800, cp1data, sizeof(cp1data));      /* copy cp1 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000002;       /* reset cp1 */
	*((volatile u32*)0x402B0050) &= ~0x02000000;       /* clear cp1 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00BC);
		if (!(state & (0xf<<16)))
			break;
	}
	*((volatile u32*)0x402B0050) &= ~0x10000000;       /* clear cp1 force deep sleep */

	*((volatile u32*)0x402B00A8) &= ~0x00000003;       /* clear reset cp0 cp1 */
#endif
}

void sipc_addr_reset()
{
#ifdef CONFIG_SC8825
	memset((void *)SIPC_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#elif defined (CONFIG_SC8830)
	memset((void *)SIPC_TD_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
	memset((void *)SIPC_WCDMA_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#endif
}

#endif

void vlx_nand_boot(char * kernel_pname, char * cmdline, int backlight_set)
{
    boot_img_hdr *hdr = (void *)raw_header;
	block_dev_desc_t *p_block_dev = NULL;
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	size_t size;
	loff_t off = 0;
	int orginal_right, backupfile_right;
	unsigned long orginal_index, backupfile_index;
	nand_erase_options_t opts;
	char * mtdpart_def = NULL;
	disk_partition_t info;
	int filelen;
	ulong part_length;
	#if (defined CONFIG_SC8810) || (defined CONFIG_SC8825)
	MMU_Init(CONFIG_MMU_TABLE_ADDR);
	#endif
	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		return;
	}

#ifdef CONFIG_SPLASH_SCREEN
#define SPLASH_PART "boot_logo"
	//read boot image header
	#ifdef CONFIG_LCD_720P
	size = 1<<20;
	#else
	size = 1<<19;
	#endif
	uint8 * bmp_img = malloc(size);
	if(!bmp_img){
	    printf("not enough memory for splash image\n");
	    return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_LOGO, &info)) {
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, bmp_img)){
			printf("function: %s nand read error\n", __FUNCTION__);
			return;
		}
	}
	else{
              free(bmp_img);
		return;
	}

   lcd_display_logo(backlight_set,(ulong)bmp_img,size);
#endif
    set_vibrator(0);

#ifdef BOOTING_BACKUP_NVCALIBRATION
	/* nv backup example : nvitem.bin is from HOST PC, and saved into g_eMMCBuf */
	unsigned char g_eMMCBuf[FIXNV_SIZE + EMMC_SECTOR_SIZE];
	memset(g_eMMCBuf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	/* get nvitem.bin from HOST PC and save it into g_eMMCBuf */
	get_nvitem_from_hostpc(g_eMMCBuf);

#define FIX_NV_IS_OK		(1)
#define FIX_BACKUP_NV_IS_OK	(2)

	unsigned char *pDestCode = g_eMMCBuf;
	unsigned long dwCodeSize = FIXNV_SIZE;
	unsigned char g_fix_nv_buf[FIXNV_SIZE + EMMC_SECTOR_SIZE];
	unsigned char g_fixbucknv_buf[FIXNV_SIZE + EMMC_SECTOR_SIZE];

	/* check nv struction from HOST PC */
	/*dump_all_buffer(pDestCode, 1024);*/
	if (!XCheckNVStructEx(pDestCode, dwCodeSize, 0, 1)) {
		printf("NV data from HOST PC is wrong, Failed : Verify error.\n");
		return;
	}

	unsigned long backupnvitem_count = sizeof(backupnvitem) / sizeof(NV_BACKUP_ITEM_T);
	unsigned long cnt;
	unsigned long nSectorCount, base_sector, nSectorBase;
	unsigned char *lpReadBuffer = NULL;
	unsigned long dwReadSize = 0;
	int read_nv_check = 0;
	/* printf("backupnvitem_count = %d\n", backupnvitem_count); */
	NV_BACKUP_ITEM_T *pNvBkpItem;

	if (backupnvitem_count) {
		/* read nv from PARTITION_FIX_NV1 */
		memset(g_fix_nv_buf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
		if (0 == ((FIXNV_SIZE + 4) % EMMC_SECTOR_SIZE))
		 	nSectorCount = (FIXNV_SIZE + 4) / EMMC_SECTOR_SIZE;
		else
		 	nSectorCount = (FIXNV_SIZE + 4) / EMMC_SECTOR_SIZE + 1;

		base_sector = Boot_GetPartBaseSec(p_block_dev, PARTITION_FIX_NV1);
		if (!Emmc_Read(PARTITION_USER, base_sector, nSectorCount, (unsigned char *)g_fix_nv_buf))
			memset(g_fix_nv_buf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);

		read_nv_check = 0;
		if (eMMC_nv_is_correct(g_fix_nv_buf, FIXNV_SIZE)) {
			/* check nv in PARTITION_FIX_NV1 */
			if (!XCheckNVStructEx(g_fix_nv_buf, FIXNV_SIZE, 0, 1))
				printf("NV data in PARTITION_FIX_NV1 is crashed.\n");
			else
				read_nv_check += FIX_NV_IS_OK;
		}

		/* read nv from PARTITION_FIX_NV2 */
		memset(g_fixbucknv_buf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
		if (0 == ((FIXNV_SIZE + 4) % EMMC_SECTOR_SIZE))
		 	nSectorCount = (FIXNV_SIZE + 4) / EMMC_SECTOR_SIZE;
		else
		 	nSectorCount = (FIXNV_SIZE + 4) / EMMC_SECTOR_SIZE + 1;

		base_sector = Boot_GetPartBaseSec(p_block_dev, PARTITION_FIX_NV2);
		if (!Emmc_Read(PARTITION_USER, base_sector, nSectorCount, (unsigned char *)g_fixbucknv_buf))
			memset(g_fixbucknv_buf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);

		if (eMMC_nv_is_correct(g_fixbucknv_buf, FIXNV_SIZE)) {
			/* check nv in PARTITION_FIX_NV2 */
			if (!XCheckNVStructEx(g_fixbucknv_buf, FIXNV_SIZE, 0, 1))
				printf("NV data in PARTITION_FIX_NV2 is crashed.\n");
			else
				read_nv_check += FIX_BACKUP_NV_IS_OK;
		}

		if (read_nv_check > 0) {
			switch (read_nv_check) {
			case (FIX_NV_IS_OK):
				printf("nv is right!\n");
				lpReadBuffer = g_fix_nv_buf;
				dwReadSize = FIXNV_SIZE;
			break;
			case (FIX_BACKUP_NV_IS_OK):
				printf("backupnv is right!\n");
				lpReadBuffer = g_fixbucknv_buf;
				dwReadSize = FIXNV_SIZE;
			break;
			case (FIX_NV_IS_OK + FIX_BACKUP_NV_IS_OK):
				printf("nv and backupnv are all right!\n");
				lpReadBuffer = g_fix_nv_buf;
				dwReadSize = FIXNV_SIZE;
			break;
			}

			/* dump_all_buffer(lpReadBuffer, 1024); */
			if (!XCheckNVStructEx(lpReadBuffer, dwReadSize, 0, 1))
				printf("NV data in phone is crashed.\n");
			else
				printf("NV data in phone is right.\n");

			/* check calibration reserved 7 and struct itself only for GSM */
			/* if (!XCheckCalibration(lpReadBuffer, dwReadSize, 1)) {
				printf("the phone is not to be GSM calibrated.\n");
			} */

			/* calibrate every item in backupnvitem array */
			for (cnt = 0; cnt < backupnvitem_count; cnt ++) {
				pNvBkpItem = (backupnvitem + cnt);
				dump_nvitem(pNvBkpItem);

				unsigned long bReplace = 0;
				unsigned long bContinue = 0;
				unsigned long dwErrorRCID;
				int mm, nNvBkpFlagCount = pNvBkpItem->dwFlagCount;

				if (nNvBkpFlagCount > MAX_NV_BACKUP_FALG_NUM)
					nNvBkpFlagCount = MAX_NV_BACKUP_FALG_NUM;

				for (mm = 0; mm < nNvBkpFlagCount; mm++) {
					if (strcmp(pNvBkpItem->nbftArray[mm].szFlagName, "Replace") == 0)
						bReplace = pNvBkpItem->nbftArray[mm].dwCheck;
					else if (strcmp(pNvBkpItem->nbftArray[mm].szFlagName, "Continue") == 0)
						bContinue = pNvBkpItem->nbftArray[mm].dwCheck;
				}

				printf("bReplace = %d  bContinue = %d\n", bReplace, bContinue);
				/* ------------------------------ */
				if ((strcmp(pNvBkpItem->szItemName, "Calibration")) == 0) {
					if (pNvBkpItem->wIsBackup == 1) {
						dwErrorRCID = GSMCaliPreserve((unsigned char *)pDestCode, 									dwCodeSize, lpReadBuffer, dwReadSize,
									  bReplace, bContinue, GSMCaliVaPolicy);
						if (dwErrorRCID != 0) {
							printf("Preserve calibration Failed : not Verify.\n");
							return;
						}
					}
				} else if ((strcmp(pNvBkpItem->szItemName, "TD_Calibration")) == 0) {
					if (pNvBkpItem->wIsBackup == 1) {
						dwErrorRCID = XTDCaliPreserve((unsigned char *)pDestCode, 									dwCodeSize, lpReadBuffer, dwReadSize,	 										bReplace, bContinue);
						if (dwErrorRCID != 0) {
							printf("Preserve TD calibration Failed : not Verify.\n");
							return;
						}
					}
				} else if ((strcmp(pNvBkpItem->szItemName, "LTE_Calibration")) == 0) {
					if (pNvBkpItem->wIsBackup == 1) {
						dwErrorRCID = LTECaliPreserve((unsigned char *)pDestCode, 									dwCodeSize, lpReadBuffer, dwReadSize, 										bReplace, bContinue);
						if (dwErrorRCID != 0) {
							printf("Preserve LTE calibration Failed : not Verify.\n");
							return;
						}
					}
				} else if ((strcmp(pNvBkpItem->szItemName, "IMEI")) == 0) {
					if (pNvBkpItem->wIsBackup == 1) {
						dwErrorRCID = XPreserveIMEIs((unsigned short)pNvBkpItem->dwID,
								(unsigned char *)pDestCode, dwCodeSize, 								lpReadBuffer, dwReadSize, bReplace, bContinue);
						if (dwErrorRCID != 0) {
							printf("Preserve %s Failed : not Verify.\n",
								pNvBkpItem->szItemName);
							return;
						}	
					}
				} else {
					if ((strcmp(pNvBkpItem->szItemName, "MMITest") == 0) ||
						(strcmp(pNvBkpItem->szItemName, "MMITest Result") == 0))
							bContinue = 1;

					if (pNvBkpItem->wIsBackup == 1 && pNvBkpItem->dwID != 0xFFFFFFFF) {
						dwErrorRCID = XPreserveNVItem((unsigned short)(pNvBkpItem->dwID), 									(unsigned char *)pDestCode, dwCodeSize, 								lpReadBuffer, dwReadSize, bReplace, bContinue);
						if (dwErrorRCID != 0) {
							printf("Preserve %s Failed : not Verify.\n",
								pNvBkpItem->szItemName);
							return;
						}
					}
				}
				/* ------------------------------ */
			} /* for (cnt = 0; cnt < backupnvitem_count; cnt ++) */
		} else /* if (read_nv_check > 0) */
			printf("nv and backupnv are all empty or wrong, so download nv from DLoad Tools only.\n");
	} else /* if (backupnvitem_count) */
		printf("backupnvitem is empty, so don't backup nv and download nv from DLoad Tools only.\n");

	g_eMMCBuf[FIXNV_SIZE + 0] = g_eMMCBuf[FIXNV_SIZE + 1] = 0x5a;
	g_eMMCBuf[FIXNV_SIZE + 2] = g_eMMCBuf[FIXNV_SIZE + 3] = 0x5a;

	if (0 == ((FIXNV_SIZE + 4) % EMMC_SECTOR_SIZE))
		nSectorCount = (FIXNV_SIZE + 4) / EMMC_SECTOR_SIZE;
	else
		nSectorCount = (FIXNV_SIZE + 4) / EMMC_SECTOR_SIZE + 1;

	memset(g_fix_nv_buf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	memcpy(g_fix_nv_buf, g_eMMCBuf, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	nSectorBase = Boot_GetPartBaseSec(p_block_dev, PARTITION_FIX_NV1);
	nv_erase_partition(p_block_dev, PARTITION_FIX_NV1);
	if (!Emmc_Write(PARTITION_USER, nSectorBase, nSectorCount, (unsigned char *)g_fix_nv_buf))
		return;

	memset(g_fixbucknv_buf, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	memcpy(g_fixbucknv_buf, g_fix_nv_buf, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	nSectorBase = Boot_GetPartBaseSec(p_block_dev, PARTITION_FIX_NV2);
	nv_erase_partition(p_block_dev, PARTITION_FIX_NV2);
	if (!Emmc_Write(PARTITION_USER, nSectorBase, nSectorCount, (unsigned char *)g_fixbucknv_buf))
		return;

#endif /* BOOTING_BACKUP_NVCALIBRATION */

#if((!BOOT_NATIVE_LINUX)||(BOOT_NATIVE_LINUX_MODEM))

	/* recovery damaged fixnv or backupfixnv */
#ifdef CONFIG_SC8830
	orginal_right = 0;
	memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	if(0 == nv_read_partition(p_block_dev, PARTITION_TDFIX_NV1, (char *)TDFIXNV_ADR, FIXNV_SIZE + 4)){
		if (1 == fixnv_is_correct_endflag((unsigned char *)TDFIXNV_ADR, FIXNV_SIZE))
			orginal_right = 1;//right
	}

    backupfile_right = 0;
	memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	if(0 == nv_read_partition(p_block_dev, PARTITION_TDFIX_NV2, (char *)TDRUNTIMENV_ADR, FIXNV_SIZE + 4)){
		if (1 == fixnv_is_correct_endflag((unsigned char *)TDRUNTIMENV_ADR, FIXNV_SIZE))
			backupfile_right = 1;//right
	}

	if ((orginal_right == 1) && (backupfile_right == 1)) {
		/* check index */
		orginal_index = get_nv_index((unsigned char *)TDFIXNV_ADR, FIXNV_SIZE);
		backupfile_index = get_nv_index((unsigned char *)TDRUNTIMENV_ADR, FIXNV_SIZE);
		if (orginal_index != backupfile_index) {
			orginal_right = 1;
			backupfile_right = 0;
		}
	}

    if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("TDfixnv is right, but backupfixnv is wrong, so erase and recovery backupfixnv\n");
		nv_erase_partition(p_block_dev, PARTITION_TDFIX_NV2);
		nv_write_partition(p_block_dev, PARTITION_TDFIX_NV2, (char *)TDFIXNV_ADR, (FIXNV_SIZE + 4));
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("TDbackupfixnv is right, but fixnv is wrong, so erase and recovery fixnv\n");
		nv_erase_partition(p_block_dev, PARTITION_TDFIX_NV1);
		nv_write_partition(p_block_dev, PARTITION_TDFIX_NV1, (char *)TDRUNTIMENV_ADR, (FIXNV_SIZE + 4));
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nTDfixnv and backupfixnv are all wrong.\n\n");
	}

	orginal_right = 0;
	memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	if(0 == nv_read_partition(p_block_dev, PARTITION_WFIX_NV1, (char *)WFIXNV_ADR, FIXNV_SIZE + 4)){
		if (1 == fixnv_is_correct_endflag((unsigned char *)WFIXNV_ADR, FIXNV_SIZE))
			orginal_right = 1;//right
	}

    backupfile_right = 0;
	memset((unsigned char *)WRUNTIMENV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	if(0 == nv_read_partition(p_block_dev, PARTITION_WFIX_NV2, (char *)WRUNTIMENV_ADR, FIXNV_SIZE + 4)){
		if (1 == fixnv_is_correct_endflag((unsigned char *)WRUNTIMENV_ADR, FIXNV_SIZE))
			backupfile_right = 1;//right
	}

	if ((orginal_right == 1) && (backupfile_right == 1)) {
		/* check index */
		orginal_index = get_nv_index((unsigned char *)WFIXNV_ADR, FIXNV_SIZE);
		backupfile_index = get_nv_index((unsigned char *)WRUNTIMENV_ADR, FIXNV_SIZE);
		if (orginal_index != backupfile_index) {
			orginal_right = 1;
			backupfile_right = 0;
		}
	}

    if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("Wfixnv is right, but backupfixnv is wrong, so erase and recovery backupfixnv\n");
		nv_erase_partition(p_block_dev, PARTITION_WFIX_NV2);
		nv_write_partition(p_block_dev, PARTITION_WFIX_NV2, (char *)WFIXNV_ADR, (FIXNV_SIZE + 4));
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("Wbackupfixnv is right, but fixnv is wrong, so erase and recovery fixnv\n");
		nv_erase_partition(p_block_dev, PARTITION_WFIX_NV1);
		nv_write_partition(p_block_dev, PARTITION_WFIX_NV1, (char *)WRUNTIMENV_ADR, (FIXNV_SIZE + 4));
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nWfixnv and backupfixnv are all wrong.\n\n");
	}

#else
	orginal_right = 0;
	memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	if(0 == nv_read_partition(p_block_dev, PARTITION_FIX_NV1, (char *)FIXNV_ADR, FIXNV_SIZE + 4)){
		if (1 == fixnv_is_correct_endflag((unsigned char *)FIXNV_ADR, FIXNV_SIZE))
			orginal_right = 1;//right
	}

    backupfile_right = 0;
	memset((unsigned char *)RUNTIMENV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	if(0 == nv_read_partition(p_block_dev, PARTITION_FIX_NV2, (char *)RUNTIMENV_ADR, FIXNV_SIZE + 4)){
		if (1 == fixnv_is_correct_endflag((unsigned char *)RUNTIMENV_ADR, FIXNV_SIZE))
			backupfile_right = 1;//right
	}

	if ((orginal_right == 1) && (backupfile_right == 1)) {
		/* check index */
		orginal_index = get_nv_index((unsigned char *)FIXNV_ADR, FIXNV_SIZE);
		backupfile_index = get_nv_index((unsigned char *)RUNTIMENV_ADR, FIXNV_SIZE);
		if (orginal_index != backupfile_index) {
			orginal_right = 1;
			backupfile_right = 0;
		}
	}

    if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("fixnv is right, but backupfixnv is wrong, so erase and recovery backupfixnv\n");
		nv_erase_partition(p_block_dev, PARTITION_FIX_NV2);
		nv_write_partition(p_block_dev, PARTITION_FIX_NV2, (char *)FIXNV_ADR, (FIXNV_SIZE + 4));
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("backupfixnv is right, but fixnv is wrong, so erase and recovery fixnv\n");
		nv_erase_partition(p_block_dev, PARTITION_FIX_NV1);
		nv_write_partition(p_block_dev, PARTITION_FIX_NV1, (char *)RUNTIMENV_ADR, (FIXNV_SIZE + 4));
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nfixnv and backupfixnv are all wrong.\n\n");
	}
#endif
	///////////////////////////////////////////////////////////////////////
	/* FIXNV_PART */
#if defined(CONFIG_SC8830)
	printf("Reading TDfixnv to 0x%08x\n", TDFIXNV_ADR);
	memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	/* fixnv */
	if (nv_read_partition(p_block_dev, PARTITION_TDFIX_NV1, (char *)TDFIXNV_ADR, FIXNV_SIZE + 4) == 0) {
		if (-1 == fixnv_is_correct((unsigned char *)TDFIXNV_ADR, FIXNV_SIZE)) {
			printf("TD nv is wrong, read backup nv\n");
			memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
			if (nv_read_partition(p_block_dev, PARTITION_TDFIX_NV2, (char *)TDFIXNV_ADR, FIXNV_SIZE + 4) == 0) {
				if (-1 == fixnv_is_correct((unsigned char *)TDFIXNV_ADR, FIXNV_SIZE)) {
					memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
					printf("nv and backup nv are all wrong!\n");
				}
			} else {
				printf("TD read backup nv fail\n");
				memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
				printf("TD nv and backup nv are all wrong!\n");
			}
		}
	} else {
		printf("TD read nv fail, read backup nv\n");
		if (nv_read_partition(p_block_dev, PARTITION_TDFIX_NV2, (char *)TDFIXNV_ADR, FIXNV_SIZE + 4) == 0) {
			if (-1 == fixnv_is_correct((unsigned char *)TDFIXNV_ADR, FIXNV_SIZE)) {
				memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
				printf("TD nv and backup nv are all wrong!\n");
			}
		} else {
			printf("TD read backup nv fail\n");
			memset((unsigned char *)TDFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
			printf("nv and backup nv are all wrong!\n");
		}
	}
	printf("Reading Wfixnv to 0x%08x\n", WFIXNV_ADR);
	memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	/* fixnv */
	if (nv_read_partition(p_block_dev, PARTITION_WFIX_NV1, (char *)WFIXNV_ADR, FIXNV_SIZE + 4) == 0) {
		if (-1 == fixnv_is_correct((unsigned char *)WFIXNV_ADR, FIXNV_SIZE)) {
			printf("W nv is wrong, read backup nv\n");
			memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
			if (nv_read_partition(p_block_dev, PARTITION_WFIX_NV2, (char *)WFIXNV_ADR, FIXNV_SIZE + 4) == 0) {
				if (-1 == fixnv_is_correct((unsigned char *)WFIXNV_ADR, FIXNV_SIZE)) {
					memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
					printf("W nv and backup nv are all wrong!\n");
				}
			} else {
				printf("W read backup nv fail\n");
				memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
				printf("W nv and backup nv are all wrong!\n");
			}
		}
	} else {
		printf("W read nv fail, read backup nv\n");
		if (nv_read_partition(p_block_dev, PARTITION_WFIX_NV2, (char *)WFIXNV_ADR, FIXNV_SIZE + 4) == 0) {
			if (-1 == fixnv_is_correct((unsigned char *)WFIXNV_ADR, FIXNV_SIZE)) {
				memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
				printf("W nv and backup nv are all wrong!\n");
			}
		} else {
			printf("W read backup nv fail\n");
			memset((unsigned char *)WFIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
			printf("W nv and backup nv are all wrong!\n");
		}
	}
#else
	printf("Reading fixnv to 0x%08x\n", FIXNV_ADR);
	memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
	/* fixnv */
	if (nv_read_partition(p_block_dev, PARTITION_FIX_NV1, (char *)FIXNV_ADR, FIXNV_SIZE + 4) == 0) {
		if (-1 == fixnv_is_correct((unsigned char *)FIXNV_ADR, FIXNV_SIZE)) {
			printf("nv is wrong, read backup nv\n");
			memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
			if (nv_read_partition(p_block_dev, PARTITION_FIX_NV2, (char *)FIXNV_ADR, FIXNV_SIZE + 4) == 0) {
				if (-1 == fixnv_is_correct((unsigned char *)FIXNV_ADR, FIXNV_SIZE)) {
					memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
					printf("nv and backup nv are all wrong!\n");
				}
			} else {
				printf("read backup nv fail\n");
				memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
				printf("nv and backup nv are all wrong!\n");
			}
		}
	} else {
		printf("read nv fail, read backup nv\n");
		if (nv_read_partition(p_block_dev, PARTITION_FIX_NV2, (char *)FIXNV_ADR, FIXNV_SIZE + 4) == 0) {
			if (-1 == fixnv_is_correct((unsigned char *)FIXNV_ADR, FIXNV_SIZE)) {
				memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
				printf("nv and backup nv are all wrong!\n");
			}
		} else {
			printf("read backup nv fail\n");
			memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE + EMMC_SECTOR_SIZE);
			printf("nv and backup nv are all wrong!\n");
		}
	}
#endif
	//array_value((unsigned char *)FIXNV_ADR, FIXNV_SIZE);

#ifndef CONFIG_FS_EXT4
	///////////////////////////////////////////////////////////////////////
	/* PRODUCTINFO_PART */
	orginal_right = 0;
	memset((unsigned char *)PRODUCTINFO_ADR, 0xff, PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE);
	if(prodinfo_read_partition_flag(p_block_dev, PARTITION_PROD_INFO1, 0, (char *)PRODUCTINFO_ADR,
		PRODUCTINFO_SIZE + 4) == 0){
		orginal_right = 1;
	}
#ifdef CONFIG_SC8830
	backupfile_right = 0;
	memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE);
	if(prodinfo_read_partition_flag(p_block_dev, PARTITION_PROD_INFO2, 0, (char *)TDRUNTIMENV_ADR,
		PRODUCTINFO_SIZE + 4) == 0){
			backupfile_right = 1;
	}
#else
	backupfile_right = 0;
	memset((unsigned char *)RUNTIMENV_ADR, 0xff, PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE);
	if(prodinfo_read_partition_flag(p_block_dev, PARTITION_PROD_INFO2, 0, (char *)RUNTIMENV_ADR,
		PRODUCTINFO_SIZE + 4) == 0){
			backupfile_right = 1;
	}
#endif
	if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("productinfo is right, but productinfobkup is wrong, so recovery productinfobkup\n");
		nv_erase_partition(p_block_dev, PARTITION_PROD_INFO2);
		nv_write_partition(p_block_dev, PARTITION_PROD_INFO2, (char *)PRODUCTINFO_ADR, (PRODUCTINFO_SIZE + 8));
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("productinfobkup is right, but productinfo is wrong, so recovery productinfo\n");
		nv_erase_partition(p_block_dev, PARTITION_PROD_INFO1);
#ifdef CONFIG_SC8830
		nv_write_partition(p_block_dev, PARTITION_PROD_INFO1, (char *)TDRUNTIMENV_ADR, (PRODUCTINFO_SIZE + 8));
#else
		nv_write_partition(p_block_dev, PARTITION_PROD_INFO1, (char *)RUNTIMENV_ADR, (PRODUCTINFO_SIZE + 8));
#endif
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nproductinfo and productinfobkup are all wrong or no phasecheck.\n\n");
	}

	printf("Reading productinfo to 0x%08x\n", PRODUCTINFO_ADR);
	memset((unsigned char *)PRODUCTINFO_ADR, 0xff, PRODUCTINFO_SIZE +  EMMC_SECTOR_SIZE);
	if(prodinfo_read_partition(p_block_dev, PARTITION_PROD_INFO1, 0, (char *)PRODUCTINFO_ADR, 
		PRODUCTINFO_SIZE + 4) == 0){
		printf("productinfo is right\n");
	} else {
		memset((unsigned char *)PRODUCTINFO_ADR, 0xff, PRODUCTINFO_SIZE + EMMC_SECTOR_SIZE);
		if(prodinfo_read_partition(p_block_dev, PARTITION_PROD_INFO2, 0, (char *)PRODUCTINFO_ADR, 
			PRODUCTINFO_SIZE + 4) == 0) {
			printf("productbackinfo is right\n");
		} else
			memset((unsigned char *)PRODUCTINFO_ADR, 0xff, PRODUCTINFO_SIZE + EMMC_SECTOR_SIZE);
	}
	//array_value((unsigned char *)PRODUCTINFO_ADR, PRODUCTINFO_SIZE);
	eng_phasechecktest((unsigned char *)PRODUCTINFO_ADR, SP09_MAX_PHASE_BUFF_SIZE);
#endif
	/* RUNTIMEVN_PART */
#if defined(CONFIG_SC8830)
	orginal_right = 0;
	memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_TDRUNTIME_NV1, (char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
		if (1 == runtimenv_is_correct((unsigned char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
			orginal_right = 1;//right
		}
	}
	backupfile_right = 0;
	memset((unsigned char *)TDDSP_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_TDRUNTIME_NV2, (char *)TDDSP_ADR, RUNTIMENV_SIZE) == 0) {
		if (1 == runtimenv_is_correct((unsigned char *)TDDSP_ADR, RUNTIMENV_SIZE)) {
			backupfile_right = 1;//right
		}
	}
	if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("TDruntimenv is right, but runtimenvbkup is wrong, so recovery runtimenvbkup\n");
		nv_erase_partition(p_block_dev, PARTITION_TDRUNTIME_NV2);
		nv_write_partition(p_block_dev, PARTITION_TDRUNTIME_NV2, (char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE);
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("TDruntimenvbkup is right, but runtimenv is wrong, so recovery runtimenv\n");
		nv_erase_partition(p_block_dev, PARTITION_TDRUNTIME_NV1);
		nv_write_partition(p_block_dev, PARTITION_TDRUNTIME_NV1, (char *)TDDSP_ADR, RUNTIMENV_SIZE);
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nTDruntimenv and runtimenvbkup are all wrong or no runtimenv.\n\n");
	}
	printf("Reading TDruntimenv to 0x%08x\n", TDRUNTIMENV_ADR);
	/* runtimenv */
	memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_TDRUNTIME_NV1, (char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
		if (-1 == runtimenv_is_correct((unsigned char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
			/* file isn't right and read backup file */
			memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
			if(nv_read_partition(p_block_dev, PARTITION_TDRUNTIME_NV2, (char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
				if (-1 == runtimenv_is_correct((unsigned char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
					/* file isn't right */
					memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
				}
			}
		}
	} else {
		/* file don't exist and read backup file */
		memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
		if(nv_read_partition(p_block_dev, PARTITION_TDRUNTIME_NV2, (char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0){
			if (-1 == runtimenv_is_correct((unsigned char *)TDRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
				/* file isn't right */
				memset((unsigned char *)TDRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
			}
		}
	}

	//Wruntimenv
	orginal_right = 0;
	memset((unsigned char *)WRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_WRUNTIME_NV1, (char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
		if (1 == runtimenv_is_correct((unsigned char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
			orginal_right = 1;//right
		}
	}
	backupfile_right = 0;
	memset((unsigned char *)TDDSP_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_WRUNTIME_NV2, (char *)TDDSP_ADR, RUNTIMENV_SIZE) == 0) {
		if (1 == runtimenv_is_correct((unsigned char *)TDDSP_ADR, RUNTIMENV_SIZE)) {
			backupfile_right = 1;//right
		}
	}
	if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("Wruntimenv is right, but runtimenvbkup is wrong, so recovery runtimenvbkup\n");
		nv_erase_partition(p_block_dev, PARTITION_WRUNTIME_NV2);
		nv_write_partition(p_block_dev, PARTITION_WRUNTIME_NV2, (char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE);
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("Wruntimenvbkup is right, but runtimenv is wrong, so recovery runtimenv\n");
		nv_erase_partition(p_block_dev, PARTITION_WRUNTIME_NV1);
		nv_write_partition(p_block_dev, PARTITION_WRUNTIME_NV1, (char *)TDDSP_ADR, RUNTIMENV_SIZE);
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nWruntimenv and runtimenvbkup are all wrong or no runtimenv.\n\n");
	}

	printf("Reading Wruntimenv to 0x%08x\n", WRUNTIMENV_ADR);
	/* runtimenv */
	memset((unsigned char *)WRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_WRUNTIME_NV1, (char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
		if (-1 == runtimenv_is_correct((unsigned char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
			/* file isn't right and read backup file */
			memset((unsigned char *)WRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
			if(nv_read_partition(p_block_dev, PARTITION_WRUNTIME_NV2, (char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
				if (-1 == runtimenv_is_correct((unsigned char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
					/* file isn't right */
					memset((unsigned char *)WRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
				}
			}
		}
	} else {
		/* file don't exist and read backup file */
		memset((unsigned char *)WRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
		if(nv_read_partition(p_block_dev, PARTITION_WRUNTIME_NV2, (char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE) == 0){
			if (-1 == runtimenv_is_correct((unsigned char *)WRUNTIMENV_ADR, RUNTIMENV_SIZE)) {
				/* file isn't right */
				memset((unsigned char *)WRUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
			}
		}
	}
#else
	orginal_right = 0;
	memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV1, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
		if (1 == runtimenv_is_correct((unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE)) {
			orginal_right = 1;//right
		}
	}

	backupfile_right = 0;
	memset((unsigned char *)DSP_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV2, (char *)DSP_ADR, RUNTIMENV_SIZE) == 0) {
		if (1 == runtimenv_is_correct((unsigned char *)DSP_ADR, RUNTIMENV_SIZE)) {
			backupfile_right = 1;//right
		}
	}

	if ((orginal_right == 1) && (backupfile_right == 0)) {
		printf("runtimenv is right, but runtimenvbkup is wrong, so recovery runtimenvbkup\n");
		nv_erase_partition(p_block_dev, PARTITION_RUNTIME_NV2);
		nv_write_partition(p_block_dev, PARTITION_RUNTIME_NV2, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE);
	} else if ((orginal_right == 0) && (backupfile_right == 1)) {
		printf("productinfobkup is right, but productinfo is wrong, so recovery productinfo\n");
		nv_erase_partition(p_block_dev, PARTITION_RUNTIME_NV1);
		nv_write_partition(p_block_dev, PARTITION_RUNTIME_NV1, (char *)DSP_ADR, RUNTIMENV_SIZE);
	} else if ((orginal_right == 0) && (backupfile_right == 0)) {
		printf("\n\nruntimenv and runtimenvbkup are all wrong or no runtimenv.\n\n");
	}

	printf("Reading runtimenv to 0x%08x\n", RUNTIMENV_ADR);
	/* runtimenv */
	memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
	if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV1, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
		if (-1 == runtimenv_is_correct((unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE)) {
			/* file isn't right and read backup file */
			memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
			if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV2, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE) == 0) {
				if (-1 == runtimenv_is_correct((unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE)) {
					/* file isn't right */
					memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
				}
			}
		}
	} else {
		/* file don't exist and read backup file */
		memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE + EMMC_SECTOR_SIZE);
		if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV2, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE) == 0){
			if (-1 == runtimenv_is_correct((unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE)) {
				/* file isn't right */
				memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
			}
		}
	}
#endif
	//array_value((unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE);

	/* DSP_PART */
#ifdef CONFIG_SC8830
	if (!get_partition_info(p_block_dev, PARTITION_TDDSP, &info)) {
		printf("Reading TDdsp to 0x%08x, base_sector:%8x\r\n", TDDSP_ADR, info.start);
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, DSP_SIZE/512+1, (uint8*)TDDSP_ADR)){
			printf("TDdsp nand read error \n");
			return;
		}
	}
	else{
		printf("get TDDsp info failed!\r\n");
		return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_WDSP, &info)) {
		printf("Reading Wdsp to 0x%08x, base_sector:%8x\r\n", WDSP_ADR, info.start);
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, DSP_SIZE/512+1, (uint8*)WDSP_ADR)){
			printf("Wdsp nand read error \n");
			return;
		}
	}
	else{
		printf("get WDsp info failed!\r\n");
		return;
	}
#else
	printf("Reading dsp to 0x%08x\n", DSP_ADR);
	if (!get_partition_info(p_block_dev, PARTITION_DSP, &info)) {
		 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, DSP_SIZE/512+1, (uint8*)DSP_ADR)){
			printf("dsp nand read error \n");
			return;
		}
	}
	else{
		return;
	}
	secure_check(DSP_ADR, 0, DSP_ADR + DSP_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif
#elif defined(CONFIG_CALIBRATION_MODE_NEW)
#if defined(CONFIG_SP7702) || defined(CONFIG_SP8810W)
	/*
		force dsp sleep in native 8810 verson to reduce power consumption
	*/
	extern void DSP_ForceSleep(void);
	DSP_ForceSleep();
	printf("8810 dsp read ok1 \n");
#endif

	if(poweron_by_calibration())
	{
		// ---------------------fix nv--------------------------------
		// 1 read orighin fixNv
		memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE);
		do{
			orginal_right = 1;	// means pass
			memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE);
			if(nv_read_partition(p_block_dev, PARTITION_FIX_NV1, (char *)FIXNV_ADR, FIXNV_SIZE)){
				orginal_right = 0;
				printf("Read origin fixnv IO fail\n");
				break;
			}
			if(!fixnv_chkEcc(FIXNV_ADR, FIXNV_SIZE)){
				orginal_right = 0;
				printf("Read origin fixnv ECC fail\n");
				break;
			}
			printf("Read origin fixnv pass\n");
			break;
		}while(0)
		if(!orginal_right)
		{
			do{
				memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE);
				if(nv_read_partition(p_block_dev, PARTITION_FIX_NV2, (char *)FIXNV_ADR, FIXNV_SIZE)){
					printf("Read backup fixnv IO fail\n");
					break;
				}
				if(!fixnv_chkEcc(FIXNV_ADR, FIXNV_SIZE)){
					printf("Read backup fixnv ECC fail\n");
					break;
				}
				printf("Read backup fixnv pass\n");
				break;
			}while(0);
		}

		// ---------------------runtime nv----------------------------
		// 1 read orighin fixNv
		do{
			orginal_right = 1;	// means pass
			memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
			if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV1, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE)){
				orginal_right = 0;
				printf("Read origin runtimeNv  IO fail\n");
				break;
			}
			if(!fixnv_chkEcc(RUNTIMENV_ADR, RUNTIMENV_SIZE)){
				orginal_right = 0;
				printf("Read origin runtimeNv  ECC fail\n");
				break;
			}
			printf("Read origin runtimeNv pass\n");
			break;
		}while(0)
		if(!orginal_right)
		{
			do{
				memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
				if(nv_read_partition(p_block_dev, PARTITION_RUNTIME_NV2, (char *)RUNTIMENV_ADR, RUNTIMENV_SIZE)){
					printf("Read backup runtimeNv  IO fail\n");
					break;
				}
				if(!fixnv_chkEcc(RUNTIMENV_ADR, RUNTIMENV_SIZE)){
					printf("Read backup runtimeNv  ECC fail\n");
					break;
				}
				break;
				printf("Read backup runtimeNv pass\n");
			}while(0);
		}

		// ---------------------DSP ----------------------------
        printf("Reading dsp to 0x%08x\n", DSP_ADR);

		if (!get_partition_info(p_block_dev, PARTITION_DSP, &info)) {
        	if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, DSP_SIZE/EMMC_SECTOR_SIZE+1, (uint8*)DSP_ADR)){
				printf("dsp nand read error \n");
				return;
			}
		}
		else{
			return;
		}

		/* MODEM_IMG_PART */
		printf("Reading modem to 0x%08x\n", FIRMWARE_ADR);
		size = (FIRMWARE_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
		if(size <= 0) {
			printf("modem image should not be zero\n");
			return;
		}
		if (!get_partition_info(p_block_dev, PARTITION_FIRMWARE, &info)) {
			 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)FIRMWARE_ADR)){
				printf("modem emmc read error \n");
				return;
			}
		}
		else{
			return;
		}

		/*MODEM FDL PART*/   
		printf("Reading cp fdl to 0x%08x\n", VMJALUNA_ADR);
		size = (VMJALUNA_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
		if(size <= 0) {
			printf("vmjuluna image should not be zero\n");
			return;
		}
		if (!get_partition_info(p_block_dev, PARTITION_VM, &info)) {
			 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)VMJALUNA_ADR)){
				printf("vmjaluna nand read error \n");
				return;
			}
		}
		else{
			return;
		}
		
		printf("call bootup modem in vlx_nand_boot,0x%x 0x%x\n",FIXNV_ADR, FIXNV_SIZE);

		bootup_modem((char *)VMJALUNA_ADR,0x3000);
		calibration_mode(cmdline, 10);
	}
#endif

	////////////////////////////////////////////////////////////////
	/* KERNEL_PART */
	printf("Reading kernel to 0x%08x\n", KERNEL_ADR);
	if(memcmp(kernel_pname, RECOVERY_PART, strlen(RECOVERY_PART))){
		printf("Reading bootimg to 0x%08x\n", KERNEL_ADR);
		if(get_partition_info(p_block_dev, PARTITION_KERNEL, &info) != 0)
			return ;
	}else{
		printf("Reading recoverimg to 0x%08x\n", KERNEL_ADR);
		if(get_partition_info(p_block_dev, PARTITION_RECOVERY, &info) != 0)
			return ;
	}
	 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, 4, (uint8*)(uint8*)hdr)){
		printf("kernel nand read error \n");
		return;
	}

	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		printf("bad boot image header, give up read!!!!\n");
                                      return;
	}
	else
	{
	                  uint32 noffsetsector = 4;
		//read kernel image
		size = (hdr->kernel_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
		if(size <=0){
			printf("kernel image should not be zero\n");
			return;
		}
                                     if(TRUE !=  Emmc_Read(PARTITION_USER, info.start+noffsetsector, size/EMMC_SECTOR_SIZE, (uint8*)KERNEL_ADR)){
			printf("kernel nand read error\n");
			return;
		}
		noffsetsector += size/512;
		noffsetsector = (noffsetsector+3)/4;
		noffsetsector = noffsetsector*4;
		//read ramdisk image
		size = (hdr->ramdisk_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
		if(size<0){
			printf("ramdisk size error\n");
			return;
		}
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start+noffsetsector, size/EMMC_SECTOR_SIZE, (uint8*)RAMDISK_ADR)){
			printf("ramdisk nand read error \n");
			return;
		}
#ifdef CONFIG_SDRAMDISK
		{
		int sd_ramdisk_size = 0;
		size = TDDSP_ADR - RAMDISK_ADR;
		if (size>0)
			sd_ramdisk_size = load_sd_ramdisk((uint8*)RAMDISK_ADR,size);
		if (sd_ramdisk_size>0)
			hdr->ramdisk_size=sd_ramdisk_size;
		}
#endif
	}

#if((!BOOT_NATIVE_LINUX)||(BOOT_NATIVE_LINUX_MODEM))
	////////////////////////////////////////////////////////////////
#ifdef CONFIG_SC8830
	/* TDMODEM_PART */
	size = (MODEM_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	if(size <= 0) {
		printf("modem image should not be zero\n");
		return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_TDMODEM, &info)) {
		printf("Reading TDmodem to 0x%08x, base_sector:%08x\r\n", TDMODEM_ADR, info.start);
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)TDMODEM_ADR)){
			printf("modem nand read error \n");
			return;
		}
	}
	else{
		printf("get TDModem info failed!\r\n");
		return;
	}

	/* WMODEM_PART */
	size = (MODEM_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	if(size <= 0) {
		printf("modem image should not be zero\n");
		return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_WMODEM, &info)) {
		printf("Reading Wmodem to 0x%08x, base_sector:%08x\r\n", WMODEM_ADR, info.start);
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)WMODEM_ADR)){
			printf("Wmodem nand read error \n");
			return;
		}
	}
	else{
		printf("get WModem info failed!\r\n");
		return;
	}
#else
	/* MODEM_PART */
	printf("Reading modem to 0x%08x\n", MODEM_ADR);
	size = (MODEM_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	if(size <= 0) {
		printf("modem image should not be zero\n");
		return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_MODEM, &info)) {
		 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)MODEM_ADR)){
			printf("modem nand read error \n");
			return;
		}
	}
	else{
		return;
	}
	secure_check(MODEM_ADR, 0, MODEM_ADR + MODEM_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif
	//array_value((unsigned char *)MODEM_ADR, MODEM_SIZE);
#endif

#if !BOOT_NATIVE_LINUX
	////////////////////////////////////////////////////////////////
	/* VMJALUNA_PART */
	printf("Reading vmjaluna to 0x%08x\n", VMJALUNA_ADR);
	size = (VMJALUNA_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	if(size <= 0) {
		printf("vmjuluna image should not be zero\n");
		return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_VM, &info)) {
		 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)VMJALUNA_ADR)){
			printf("vmjaluna nand read error \n");
			return;
		}
	}
	else{
		return;
	}
	secure_check(VMJALUNA_ADR, 0, VMJALUNA_ADR + VMJALUNA_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif
#ifdef CONFIG_SIMLOCK
#if((!BOOT_NATIVE_LINUX)||(BOOT_NATIVE_LINUX_MODEM))
	////////////////////////////////////////////////////////////////
	/* SIMLOCK_PART */
	printf("Reading simlock to 0x%08x\n", SIMLOCK_ADR);
	size = (SIMLOCK_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	if(size <= 0) {
		printf("simlock should not be zero\n");
		return;
	}
	if (!get_partition_info(p_block_dev, PARTITION_SIMLOCK, &info)) {
		 if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, (uint8*)SIMLOCK_ADR)){
			printf("simlock read error \n");
			return;
		}
	}
	else{
		return;
	}
	secure_check(SIMLOCK_ADR, 0, SIMLOCK_ADR + SIMLOCK_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif
#endif
	creat_cmdline(cmdline,hdr);

#if BOOT_NATIVE_LINUX_MODEM
	//sipc addr clear
	sipc_addr_reset();
	// start modem CP
	modem_entry();
#endif
#ifdef CONFIG_SC8830
	Emmc_DisSdClk();
#endif
	vlx_entry();
}

