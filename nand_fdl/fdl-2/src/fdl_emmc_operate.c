#include "sci_types.h"
#include "fdl_conf.h"
#ifdef CONFIG_EMMC_BOOT
#include "card_sdio.h"
#include "dload_op.h"
#include "fdl_emmc.h"
#include "packet.h"
#include "fdl_crc.h"
#include "fdl_stdio.h"
#include "asm/arch/sci_types.h"
#include <linux/crc32b.h>
#include <malloc.h>
#include <asm/arch/secure_boot.h>

#define EFI_SECTOR_SIZE 		(512)
#define ERASE_SECTOR_SIZE		((64 * 1024) / EFI_SECTOR_SIZE)
#define EMMC_BUF_SIZE			(((216 * 1024 * 1024) / EFI_SECTOR_SIZE) * EFI_SECTOR_SIZE)

#define MAGIC_DATA	0xAA55A5A5
#define SPL_CHECKSUM_LEN	0x6000
#define CHECKSUM_START_OFFSET	0x28

#define MAGIC_DATA_SAVE_OFFSET	(0x20/4)
#define CHECKSUM_SAVE_OFFSET	(0x24/4)

typedef struct DL_EMMC_STATUS_TAG
{
	uint32 part_total_size ;
	uint32 base_sector;
	uint32 curUserPartition;
	wchar_t *curUserPartitionName;
	uint8 isLastPakFlag ;
	uint8 curEMMCArea ;
} DL_EMMC_STATUS;

typedef struct DL_FILE_STATUS_TAG
{
	unsigned long   total_size;
	unsigned long   total_recv_size;
	unsigned long   unsave_recv_size;
} DL_EMMC_FILE_STATUS;

typedef struct ADDR_TO_PART_TAG
{
	unsigned long   custom;
	unsigned long   partition;
} ADDR_TO_PART;

typedef struct
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN];
	wchar_t backup_partition_name[MAX_UTF_PARTITION_NAME_LEN];
}PARTNER_PARTITION;

#if defined(CONFIG_TIGER) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
#define BOOTLOADER_HEADER_OFFSET 0x20
typedef struct{
	uint32 version;
	uint32 magicData;
	uint32 checkSum;
	uint32 hashLen;
}EMMC_BootHeader;
#endif


static DL_EMMC_STATUS g_dl_eMMCStatus = {0, 0, 0xffffffff,0, 0, 0};

static unsigned long g_checksum;
static unsigned long g_sram_addr;
static int is_nv_flag = 0;
static int is_ProdInfo_flag = 0;

#if defined (CONFIG_SC8825) || defined (CONFIG_TIGER) || defined (CONFIG_SC8830)
unsigned char *g_eMMCBuf = (unsigned char*)0x82000000;
#else
unsigned char *g_eMMCBuf = (unsigned char*)0x2000000;
#endif
unsigned char g_fix_nv_buf[FIXNV_SIZE + EFI_SECTOR_SIZE];
unsigned char g_fixbucknv_buf[FIXNV_SIZE + EFI_SECTOR_SIZE];
unsigned char g_prod_info_buf[PRODUCTINFO_SIZE + EFI_SECTOR_SIZE];

unsigned int g_total_partition_number = MAX_PARTITION_INFO;

//temp here before tools ready
static wchar_t s_uboot_partition[MAX_UTF_PARTITION_NAME_LEN] = L"UBOOT";
static wchar_t s_spl_loader_partition[MAX_UTF_PARTITION_NAME_LEN] = L"SPL_LOADER";

static DL_EMMC_FILE_STATUS g_status;
static int g_prevstatus;
static int read_nv_check = 0;

extern PARTITION_CFG g_sprd_emmc_partition_cfg[];
PARTITION_CFG uefi_part_info[MAX_PARTITION_INFO];
static int uefi_part_info_ok_flag = 0;

#define PARTITION_SPL_LOADER	(MAX_PARTITION_INFO + 0)
#define PARTITION_UBOOT		(MAX_PARTITION_INFO + 1)

#ifdef CONFIG_SC8830
static ADDR_TO_PART g_eMMC_Addr2Part_Table[] = {
	{0x80000000, PARTITION_SPL_LOADER},
	{0x80000002, PARTITION_UBOOT},
	{0x80000004, PARTITION_TDMODEM},
	{0x80000006, PARTITION_TDDSP},
	{0x80000008, PARTITION_TDFIX_NV1},
	{0x8000000a, PARTITION_TDRUNTIME_NV1},
	{0x8000000c, PARTITION_WMODEM},
	{0x8000000e, PARTITION_WDSP},
	{0x80000010, PARTITION_WFIX_NV1},
	{0x80000012, PARTITION_WRUNTIME_NV1},
	{0x80000014, PARTITION_PROD_INFO1},
	{0x80000016, PARTITION_PROD_INFO3},
	{0x80000018, PARTITION_LOGO},
	{0x8000001a, PARTITION_FASTBOOT_LOGO},
	{0x8000001c, PARTITION_KERNEL},
	{0x8000001e, PARTITION_SYSTEM},
	{0x80000020, PARTITION_CACHE},
	{0x80000022, PARTITION_RECOVERY},
	{0x80000024, PARTITION_MISC},
	{0x80000026, PARTITION_SD},
	{0x80000028, PARTITION_USER_DAT},
	{0xffffffff, 0xffffffff}
};
#else
static ADDR_TO_PART g_eMMC_Addr2Part_Table[] = {
	{0x80000000, PARTITION_SPL_LOADER}, 
	{0x80000001, PARTITION_UBOOT}, 
	{0x80000003, PARTITION_VM}, 
	{0x80000004, PARTITION_MODEM}, 
	{0x80000007, PARTITION_DSP}, 
	{0x80000009, PARTITION_KERNEL}, 
	{0x8000000a, PARTITION_RECOVERY}, 
	{0x8000000b, PARTITION_SYSTEM}, 
	{0x8000000c, PARTITION_USER_DAT}, 
	{0x8000000d, PARTITION_CACHE}, 
	{0x8000000e, PARTITION_MISC}, 
	{0x8000000f, PARTITION_LOGO},
	{0x80000010, PARTITION_FASTBOOT_LOGO},
#if defined(CONFIG_SP7702) || defined(CONFIG_SP8810W) || defined (CONFIG_SC7710G2)
	{0x80000013, PARTITION_FIRMWARE},
#endif
#ifdef CONFIG_SIMLOCK
	{0x80000015, PARTITION_SIMLOCK},
#endif
	{0x90000001, PARTITION_FIX_NV1}, 
	{0x90000002, PARTITION_PROD_INFO1},
	{0x9000000f, PARTITION_PROD_INFO3},
	{0x90000003, PARTITION_RUNTIME_NV1}, 
	{0xffffffff, 0xffffffff}
};
#endif

#define PARTNER_PARTITION_NUM  (7)

static PARTNER_PARTITION const s_partner_partition_cfg[PARTNER_PARTITION_NUM]={
	{{L"fixnv1"},             {L"fixnv2"}},
	{{L"runtimenv1"},      {L"runtimenv2"}},		
	{{L"tdfixnv1"},          {L"tdfixnv2"}},
	{{L"tdruntimenv1"},   {L"tdruntimenv2"}},
	{{L"wfixnv1"},           {L"wfixnv2"}},
	{{L"wruntimenv1"},    {L"wruntimenv2"}},
	{{L"prodinfo1"},         {L"prodinfo2"}}
};



static __inline void FDL2_eMMC_SendRep (unsigned long err)
{
	FDL_SendAckPacket (convert_err (err));
}

PUBLIC int FDL_BootIsEMMC(void)
{
//	return gpio_get_value(EMMC_SELECT_GPIO);
	return 1;
}

LOCAL unsigned long addr2part(unsigned long custom)
{
	unsigned long idx, log = 0xffffffff;

	for (idx = 0; g_eMMC_Addr2Part_Table[idx].custom != 0xffffffff; idx ++) {
		if (g_eMMC_Addr2Part_Table[idx].custom == custom) {
			log = g_eMMC_Addr2Part_Table[idx].partition;
			break;
		}
	}
	
	return log;
}

LOCAL unsigned short calc_checksum(unsigned char *dat, unsigned long len)
{
	unsigned long checksum = 0;
	unsigned short *pstart, *pend;
	if (0 == (unsigned long)dat % 2)  {
		pstart = (unsigned short *)dat;
		pend = pstart + len / 2;
		while (pstart < pend) {
			checksum += *pstart;
			pstart ++;
		}
		if (len % 2)
			checksum += *(unsigned char *)pstart;
		} else {
		pstart = (unsigned char *)dat;
		while (len > 1) {
			checksum += ((*pstart) | ((*(pstart + 1)) << 8));
			len -= 2;
			pstart += 2;
		}
		if (len)
			checksum += *pstart;
	}
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);
	return (~checksum);
}

/*
	TRUE(1): pass
	FALSE(0): fail
*/
LOCAL BOOLEAN _chkEcc(uint8* buf, uint32 size)
{
	uint16 crc,crcOri;

	crc = calc_checksum(buf,size-4);
	crcOri = (uint16)((((uint16)buf[size-3])<<8) | ((uint16)buf[size-4]) );

	return (crc == crcOri);
}


/**
	Get the backup partition name
*/
LOCAL wchar_t * _get_backup_partition_name(wchar_t *partition_name)
{
	int i = 0;

	for(i=0;i<PARTNER_PARTITION_NUM;i++)
	{
		if(wcsncmp(partition_name, s_partner_partition_cfg[i].partition_name, wcslen(partition_name)) == 0)
		{
			return s_partner_partition_cfg[i].backup_partition_name;
		}
	}

	return NULL;
}

LOCAL int _uefi_get_part_info(void)
{
	block_dev_desc_t *dev_desc = NULL;
	disk_partition_t info;
	int i;
	int part_num = 1;

	if(uefi_part_info_ok_flag)
		return 1;

	dev_desc = get_dev("mmc", 1);
	if (dev_desc==NULL) {
		return 0;
	}

	#ifdef CONFIG_EBR_PARTITION
		printf("EBR PARTITION \n");
		for(i=0; i < MAX_PARTITION_INFO; i++) {
			if(part_num ==PARTITION_EMPTY) {
				part_num++;
				continue;
				}
			if (get_partition_info(dev_desc, part_num, &info))
				return 0;

			if(info.size <= 0 )
				return 0;
			uefi_part_info[i].partition_index =part_num;
			uefi_part_info[i].partition_size = info.size;
			uefi_part_info[i].partition_offset = info.start;
			printf("partiton num =%d,size=%d,offset=%d\n",part_num,info.size,info.start);
			part_num++;
		}
	#else
	printf("GPT PARTITION \n");

	if (get_all_partition_info(dev_desc, uefi_part_info, &g_total_partition_number) != 0)
		return 0;

	#endif
	uefi_part_info_ok_flag = 1;

	return 1;
}

LOCAL unsigned long efi_GetPartBaseSec(wchar_t *partition_name)
{
	int i;
	_uefi_get_part_info();
	//TODO:not use MAX_PARTITION_INFO later
	for(i=0;i<MAX_PARTITION_INFO;i++)
	{
		if(wcsncmp(partition_name, uefi_part_info[i].partition_name, wcslen(partition_name)) == 0)
		{
			return uefi_part_info[i].partition_offset;
		}
	}
	//if return 0 ok???
	return 0;
}

LOCAL unsigned long efi_GetPartSize(wchar_t *partition_name)
{
	int i;
	_uefi_get_part_info();
	for(i=0;i<MAX_PARTITION_INFO;i++)
	{
		if(wcsncmp(partition_name, uefi_part_info[i].partition_name, wcslen(partition_name)) == 0)
			return (EFI_SECTOR_SIZE * uefi_part_info[i].partition_size);
	}

	return 0;
}


LOCAL int _format_sd_partition(void)
{
	unsigned long part_size = 0;
	unsigned long sd_data_size = 0;
	unsigned long base_sector = 0;
	g_dl_eMMCStatus.curUserPartition = PARTITION_SD;
	part_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
	if(0 == part_size){
		return -1;
	}
	sd_data_size = newfs_msdos_main(g_eMMCBuf,part_size);
	g_dl_eMMCStatus.curEMMCArea = PARTITION_USER;
	base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);

	if (!Emmc_Erase(g_dl_eMMCStatus.curEMMCArea,base_sector,part_size / EFI_SECTOR_SIZE))  {
		return -1;
	}

	if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, base_sector,sd_data_size / EFI_SECTOR_SIZE, g_eMMCBuf)) {
		return -1;
	}
	return 0;
}

/**
	Partition Size Check don't need anymore!
	Just get all partition info here.
*/
PUBLIC int FDL_Check_Partition_Table(void)
{
	printf("FDL Check Partition Table -----\n");
	int i = 0;

	uefi_part_info_ok_flag = 0;

	if (!_uefi_get_part_info())
	{
		return 0;
	}
	return 1;
}

LOCAL int _emmc_real_erase_partition(wchar_t *partition_name)
{
	unsigned long i, count, len=0,  base_sector;
	uint8 curArea;

	if(NULL == g_dl_eMMCStatus.curUserPartitionName)
	{
		return 0;
	}

	if (wcsncmp(L"SPL_LOADER", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"SPL_LOADER")) == 0){
		if(secureboot_enabled()){
			return 1;
		}
		count = Emmc_GetCapacity(PARTITION_BOOT1);
		count = count / ERASE_SECTOR_SIZE;
		curArea = PARTITION_BOOT1;
		base_sector = 0;
	}else if (wcsncmp(L"UBOOT", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"UBOOT")) == 0){
		count = Emmc_GetCapacity(PARTITION_BOOT2);
		count = count / ERASE_SECTOR_SIZE;
		curArea = PARTITION_BOOT2;
		base_sector = 0;		
	}
	else{
		curArea = PARTITION_USER;
		len = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName); /* partition size : in bytes */
		len = len / EFI_SECTOR_SIZE; /* partition size : in blocks */
		base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
		memset(g_eMMCBuf, 0xff, EMMC_BUF_SIZE);
		count = len / (EMMC_BUF_SIZE / EFI_SECTOR_SIZE);
	}
	
	for (i = 0; i < count; i++) {
		if (!Emmc_Write(curArea, base_sector + i * (EMMC_BUF_SIZE / EFI_SECTOR_SIZE), 
			EMMC_BUF_SIZE / EFI_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
			return 0;
	}
	
	count = len % (EMMC_BUF_SIZE / EFI_SECTOR_SIZE);	
	if (count) {
		if (!Emmc_Write(curArea, base_sector + i * (EMMC_BUF_SIZE / EFI_SECTOR_SIZE),
			count, (unsigned char *)g_eMMCBuf))
			return 0;	
	}

	return 1;
}

/**
	Only for PARTITION_USER now.
	
	TODO:
	The pre implementation seems have some problems. Modify Later...
*/
LOCAL int _emmc_erase_partition(wchar_t *partition_name, int fastEraseFlag)
{
	unsigned long i, count, len,  base_sector;
	uint8 curArea;

	curArea = PARTITION_USER;
	len = efi_GetPartSize(partition_name);
	len = len / EFI_SECTOR_SIZE;
	base_sector = efi_GetPartBaseSec(partition_name);

	if (fastEraseFlag) {
		memset(g_eMMCBuf, 0xff, ERASE_SECTOR_SIZE * EFI_SECTOR_SIZE);		
		if (!Emmc_Write(curArea, base_sector, ERASE_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
			 return 0;
	} else {
		count = len / (EMMC_BUF_SIZE / EFI_SECTOR_SIZE);
		memset(g_eMMCBuf, 0xff, EMMC_BUF_SIZE);
		for (i = 0; i < count; i++) {
			if (!Emmc_Write(curArea, base_sector + i * EMMC_BUF_SIZE / EFI_SECTOR_SIZE, 
				EMMC_BUF_SIZE / EFI_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
				 return 0;
		}
		count = len % (EMMC_BUF_SIZE / EFI_SECTOR_SIZE);		
		if (count) {
			if (!Emmc_Write(curArea, base_sector + i * EMMC_BUF_SIZE / EFI_SECTOR_SIZE,
				count, (unsigned char *)g_eMMCBuf))
				 return 0;	
		}
	}

	return 1;
}

LOCAL int _emmc_erase_allflash(void)
{
	int i;
	uint32 count;

	memset(g_eMMCBuf, 0xff, ERASE_SECTOR_SIZE*EFI_SECTOR_SIZE);
	for (i = 0; i < g_total_partition_number; i++) {
		if (!_emmc_erase_partition(uefi_part_info[i].partition_name, 1))
			return 0;
	}

	if (!Emmc_Write(PARTITION_USER, 0, ERASE_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
		 return 0;
	count = Emmc_GetCapacity(PARTITION_USER);
	if (!Emmc_Write(PARTITION_USER, count - 1, 1, (unsigned char *)g_eMMCBuf))
		 return 0;
	if(!secureboot_enabled()){ // PARTITION_BOOT1 is for spl, if secure boot enabled, skip it
		count = Emmc_GetCapacity(PARTITION_BOOT1);
		count = count / ERASE_SECTOR_SIZE;
		for (i = 0; i < count; i++) {	
			if (!Emmc_Write(PARTITION_BOOT1, i * ERASE_SECTOR_SIZE,
						ERASE_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
				return 0;
		}
	}
	
	count = Emmc_GetCapacity(PARTITION_BOOT2);
	count = count / ERASE_SECTOR_SIZE;
	for (i = 0; i < count; i++) {	
		if (!Emmc_Write(PARTITION_BOOT2, i * ERASE_SECTOR_SIZE,
			ERASE_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
			 return 0;
	}

	return 1;
}

LOCAL unsigned short eMMCCheckSum(const unsigned int *src, int len)
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

LOCAL void splFillCheckData(unsigned int * splBuf,  int len)
{
#if defined(CONFIG_TIGER) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
	EMMC_BootHeader *header;
	header = (EMMC_BootHeader *)((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET);
	header->version  = 0;
	header->magicData= MAGIC_DATA;
	header->checkSum = (unsigned int)eMMCCheckSum((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET+sizeof(*header), SPL_CHECKSUM_LEN-(BOOTLOADER_HEADER_OFFSET+sizeof(*header)));
	header->hashLen  = 0;
#else
	*(splBuf + MAGIC_DATA_SAVE_OFFSET) = MAGIC_DATA;
	*(splBuf + CHECKSUM_SAVE_OFFSET) = (unsigned int)eMMCCheckSum((unsigned int *)&splBuf[CHECKSUM_START_OFFSET/4], SPL_CHECKSUM_LEN - CHECKSUM_START_OFFSET);
//	*(splBuf + CHECKSUM_SAVE_OFFSET) = splCheckSum(splBuf);
#endif
}


/**
	Get the partition name before Tools ready
*/
LOCAL int _get_partition_name(void)
{
	int i=0;

	g_dl_eMMCStatus.curUserPartitionName = NULL;
	
	for(i=0;i<g_total_partition_number;i++)
	{
		if(g_dl_eMMCStatus.curUserPartition == g_sprd_emmc_partition_cfg[i].partition_index)
		{
			g_dl_eMMCStatus.curUserPartitionName = g_sprd_emmc_partition_cfg[i].partition_name;
			break;
		}
	}

	
	if(PARTITION_SPL_LOADER == g_dl_eMMCStatus.curUserPartition)
	{
		g_dl_eMMCStatus.curUserPartitionName = s_spl_loader_partition;
	}
	else if(PARTITION_UBOOT == g_dl_eMMCStatus.curUserPartition)
	{
		g_dl_eMMCStatus.curUserPartitionName = s_uboot_partition;
	}
	
	//Can not find the partition specified
	if(NULL == g_dl_eMMCStatus.curUserPartitionName)
	{
		//FDL_SendAckPacket (convert_err (EMMC_INCOMPATIBLE_PART));
		return 0;
	}
	
	return 1;
}

PUBLIC int fdl2_emmc_download_start(unsigned long start_addr, unsigned long size, unsigned long nv_checksum)
{
	int i = 0;

	g_status.total_size  = size;
	is_ProdInfo_flag = 0;
	is_nv_flag = 0;
	g_dl_eMMCStatus.curUserPartition = addr2part(start_addr);
	g_dl_eMMCStatus.curUserPartitionName = NULL;

	if ((g_dl_eMMCStatus.curUserPartition < 0) || ((g_dl_eMMCStatus.curUserPartition >= MAX_PARTITION_INFO) && 
		(g_dl_eMMCStatus.curUserPartition !=PARTITION_SPL_LOADER && g_dl_eMMCStatus.curUserPartition != PARTITION_UBOOT)))
	{
		FDL_SendAckPacket (convert_err (EMMC_INCOMPATIBLE_PART));
		return 0;
	}

	if(!_get_partition_name())
	{
		FDL_SendAckPacket (convert_err (EMMC_INCOMPATIBLE_PART));
		return 0;
	}
	
#ifndef DOWNLOAD_IMAGE_WITHOUT_SPARSE
	if (((wcsncmp(L"system", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"system")) != 0) && \
		(wcsncmp(L"userdata", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"userdata")) != 0) && \
		(wcsncmp(L"cache", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"cache")) != 0)) && (size > EMMC_BUF_SIZE)) {
		printf("image file size : 0x%08x is bigger than EMMC_BUF_SIZE : 0x%08x\n", size, EMMC_BUF_SIZE);
		FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
		return 0;
	}
#endif

	if ((wcsncmp(L"tdfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"tdfixnv1")) == 0)||
		(wcsncmp(L"wfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"wfixnv1")) == 0)||
		(wcsncmp(L"fixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"fixnv1")) == 0)) 
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_USER;
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);

		if ((size > g_dl_eMMCStatus.part_total_size) || (size > FIXNV_SIZE)) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return 0;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
		is_nv_flag = 1;
		memset(g_eMMCBuf, 0xff, FIXNV_SIZE + EFI_SECTOR_SIZE);
		g_checksum = nv_checksum;
	}
	else if (wcsncmp(L"prodinfo1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"prodinfo1")) == 0)
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_USER;

		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		if ((size > g_dl_eMMCStatus.part_total_size) || (size > FIXNV_SIZE)) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return 0;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
		_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName);
		memset(g_eMMCBuf, 0xff, PRODUCTINFO_SIZE + EFI_SECTOR_SIZE);
		is_ProdInfo_flag = 1;
	} 
	else if (wcsncmp(L"SPL_LOADER", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"SPL_LOADER")) == 0) 
	{
		if(secureboot_enabled()){
			return 1;
		}
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT1;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT1);
		g_dl_eMMCStatus.base_sector =  0;
		g_dl_eMMCStatus.isLastPakFlag = 0;
		memset(g_eMMCBuf, 0xff, g_dl_eMMCStatus.part_total_size);
		if (size > g_dl_eMMCStatus.part_total_size) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return 0;
		}

	}
	else if (wcsncmp(L"UBOOT", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"UBOOT")) == 0)
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT2;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT2);
		g_dl_eMMCStatus.base_sector =  0;
		g_dl_eMMCStatus.isLastPakFlag = 0;	
		memset(g_eMMCBuf, 0xff, g_dl_eMMCStatus.part_total_size);
		if (size > g_dl_eMMCStatus.part_total_size) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return 0;
		}
	} 
	else 
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_USER;

		if (!((wcsncmp(L"system", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"system")) == 0) \
			|| (wcsncmp(L"userdata", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"userdata")) == 0) \
			|| (wcsncmp(L"cache", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"cache")) == 0)))
			_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName);
		else
			memset(g_eMMCBuf, 0, EMMC_BUF_SIZE);

		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		if (size > g_dl_eMMCStatus.part_total_size) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return 0;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
		g_dl_eMMCStatus.isLastPakFlag = 0;
	}
	
	g_status.total_recv_size   = 0;
	g_status.unsave_recv_size   = 0;
	g_sram_addr = (unsigned long)g_eMMCBuf;
	g_prevstatus = EMMC_SUCCESS;
	//set_dl_op_val(start_addr, size, STARTDATA, SUCCESS, 1);
	FDL_SendAckPacket (BSL_REP_ACK);
	return 1;
}

LOCAL int _emmc_download_image(unsigned long nSectorCount, unsigned long each_write_block, int is_total_recv)
{
	unsigned long cnt, base_sector, trans_times, remain_block;
	unsigned char *point;
	int retval;

#ifdef DOWNLOAD_IMAGE_WITHOUT_SPARSE
	if ((wcsncmp(L"userdata", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"userdata")) == 0) \
		|| (wcsncmp(L"cache", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"cache")) == 0)) {
		retval = write_simg2emmc("mmc", 1, g_dl_eMMCStatus.curUserPartition, 
			g_eMMCBuf, g_status.unsave_recv_size);
		if (retval == -1) {
			g_status.unsave_recv_size = 0;
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		} else if (retval > 0) {
			memmove(g_eMMCBuf, g_eMMCBuf + retval, g_status.unsave_recv_size - retval);
			g_status.unsave_recv_size -= retval;
			g_sram_addr = (unsigned long)g_eMMCBuf;
		} else {
			g_status.unsave_recv_size = 0;
			g_sram_addr = (unsigned long)g_eMMCBuf;
		}
	} else if (wcsncmp(L"system", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"system")) == 0) {
		base_sector = g_dl_eMMCStatus.base_sector;
		point = g_eMMCBuf;
		trans_times = nSectorCount / each_write_block;
		remain_block = nSectorCount % each_write_block;

		for (cnt = 0; cnt < trans_times; cnt ++) {
			if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, base_sector,
				each_write_block, (unsigned char *) point)) {
 				g_status.unsave_recv_size = 0;
				SEND_ERROR_RSP (BSL_WRITE_ERROR);
				return 0;
			}
			base_sector += each_write_block;
			point += EFI_SECTOR_SIZE * each_write_block;
		}

		if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, base_sector,
			remain_block, (unsigned char *)point)) {
 			g_status.unsave_recv_size = 0;
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}
		base_sector += remain_block;
		point += EFI_SECTOR_SIZE * remain_block;
	} else if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector,
			nSectorCount, (unsigned char *) g_eMMCBuf)) {
			g_status.unsave_recv_size = 0;
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
	}

	g_status.unsave_recv_size = 0;
	if(!is_total_recv){
		g_dl_eMMCStatus.base_sector += nSectorCount;
		g_sram_addr = (unsigned long)g_eMMCBuf;
	}
#else
	if ((wcsncmp(L"system", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"system")) == 0) \
		|| (wcsncmp(L"userdata", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"userdata")) == 0) \
		|| (wcsncmp(L"cache", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"cache")) == 0)) {
		retval = write_simg2emmc("mmc", 1, g_dl_eMMCStatus.curUserPartition, 
			g_eMMCBuf, g_status.unsave_recv_size);
		if (retval == -1) {
			g_status.unsave_recv_size = 0;
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}
	} else if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector,
			nSectorCount, (unsigned char *) g_eMMCBuf)) {
			g_status.unsave_recv_size = 0;
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
	}

	if ((wcsncmp(L"system", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"system")) == 0) \
		|| (wcsncmp(L"userdata", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"userdata")) == 0) \
		|| (wcsncmp(L"cache", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"cache")) == 0)) {
		if (retval > 0) {
			memmove(g_eMMCBuf, g_eMMCBuf + retval, g_status.unsave_recv_size - retval);
			g_status.unsave_recv_size -= retval;
			g_sram_addr = (unsigned long)g_eMMCBuf;
		} else {
			g_status.unsave_recv_size = 0;
			g_sram_addr = (unsigned long)g_eMMCBuf;
		}
	} else {
		g_status.unsave_recv_size = 0;
		if(!is_total_recv){
			g_dl_eMMCStatus.base_sector += nSectorCount;
			g_sram_addr = (unsigned long)g_eMMCBuf;
		}
	}
#endif
	return 1;
}

PUBLIC int fdl2_emmc_download(unsigned short size, char *buf)
{
	unsigned long lastSize, nSectorCount;
	unsigned long each_write_block = 10000;

	/* The previous download step failed. */
	if ((EMMC_SUCCESS != g_prevstatus) || (g_dl_eMMCStatus.isLastPakFlag)) {
		//set_dl_op_val(0, 0, MIDSTDATA, FAIL, 1);
		FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
		return 0;
	}

	//size = packet->packet_body.size;
	if ((g_status.total_recv_size + size) > g_status.total_size) {
		g_prevstatus = EMMC_INVALID_SIZE;
		//set_dl_op_val(0, 0, MIDSTDATA, FAIL, 2);
		FDL2_eMMC_SendRep (g_prevstatus);
		return 0;
	}

	g_status.total_recv_size += size;
  	if (is_nv_flag || is_ProdInfo_flag) 
	{
		memcpy((unsigned char *)g_sram_addr, buf, size);
		g_sram_addr += size;
		g_status.unsave_recv_size += size;
		//set_dl_op_val(0, 0, MIDSTDATA, SUCCESS, 8);
		FDL_SendAckPacket (BSL_REP_ACK);
		return 1;
   	} 
	else if (EMMC_BUF_SIZE >= (g_status.unsave_recv_size + size)) 
	{
		memcpy((unsigned char *)g_sram_addr, buf, size);
		g_sram_addr += size;
		
		if (EMMC_BUF_SIZE == (g_status.unsave_recv_size + size))
		{
			g_status.unsave_recv_size = EMMC_BUF_SIZE;
			if (0 == (EMMC_BUF_SIZE % EFI_SECTOR_SIZE))
				nSectorCount = EMMC_BUF_SIZE / EFI_SECTOR_SIZE;
			else
				nSectorCount = EMMC_BUF_SIZE / EFI_SECTOR_SIZE + 1;

			if(!_emmc_download_image(nSectorCount, each_write_block, FALSE))
				return 0;

		} 
		else if (g_status.total_recv_size == g_status.total_size) 
		{
			g_status.unsave_recv_size += size;
			if (g_status.unsave_recv_size != 0) {
				if (0 == (g_status.unsave_recv_size % EFI_SECTOR_SIZE))
					nSectorCount = g_status.unsave_recv_size / EFI_SECTOR_SIZE;
				else
					nSectorCount = g_status.unsave_recv_size / EFI_SECTOR_SIZE + 1;
				
				if (PARTITION_SPL_LOADER == g_dl_eMMCStatus.curUserPartition) {
					if(secureboot_enabled()){
						return 1;
					}
					if (g_status.total_recv_size < SPL_CHECKSUM_LEN)
						nSectorCount = SPL_CHECKSUM_LEN / EFI_SECTOR_SIZE;
					splFillCheckData((unsigned int *) g_eMMCBuf, (int)g_status.total_recv_size);
				}
				
				if(!_emmc_download_image(nSectorCount, each_write_block, TRUE))
					return 0;
			} 
		} 
		else
			g_status.unsave_recv_size += size;
	} 
	else 
	{
		lastSize = EMMC_BUF_SIZE - g_status.unsave_recv_size;
		memcpy((unsigned char *)g_sram_addr, buf, lastSize);
		g_status.unsave_recv_size = EMMC_BUF_SIZE;
		if (0 == (EMMC_BUF_SIZE % EFI_SECTOR_SIZE))
			nSectorCount = EMMC_BUF_SIZE / EFI_SECTOR_SIZE;
		else
			nSectorCount = EMMC_BUF_SIZE / EFI_SECTOR_SIZE + 1;
		
		if(!_emmc_download_image(nSectorCount, each_write_block, FALSE))
			return 0;
		
		g_sram_addr = (unsigned long)(g_eMMCBuf + g_status.unsave_recv_size);
		memcpy((unsigned char *)g_sram_addr, (char *)(&buf[lastSize]), size - lastSize);
		g_status.unsave_recv_size += size - lastSize;
		g_sram_addr = (unsigned long)(g_eMMCBuf + g_status.unsave_recv_size);
	 }

	//set_dl_op_val(0, 0, MIDSTDATA, FAIL, 4);
	g_prevstatus = EMMC_SUCCESS;
	FDL2_eMMC_SendRep (g_prevstatus);
	return  1; 
}

PUBLIC int fdl2_emmc_download_end(void)
{
	unsigned long  fix_nv_checksum, nSectorCount, nSectorBase;
	unsigned short sum = 0, *dataaddr;
	wchar_t *backup_partition_name = NULL;

	if (is_nv_flag) 
	{
		fix_nv_checksum = Get_CheckSum((unsigned char *) g_eMMCBuf, g_status.total_recv_size);
		fix_nv_checksum = EndianConv_32 (fix_nv_checksum);

	        if (fix_nv_checksum != g_checksum) {
			//may data transfer error
	        	SEND_ERROR_RSP(BSL_CHECKSUM_DIFF);
			return 0;
	        }

		if (0 == ((FIXNV_SIZE + 4) % EFI_SECTOR_SIZE))
			nSectorCount = (FIXNV_SIZE + 4) / EFI_SECTOR_SIZE;
		else
			nSectorCount = (FIXNV_SIZE + 4) / EFI_SECTOR_SIZE + 1;

		memset(g_fix_nv_buf, 0xff, FIXNV_SIZE + EFI_SECTOR_SIZE);
		memcpy(g_fix_nv_buf, g_eMMCBuf, FIXNV_SIZE);

		sum = calc_checksum(g_fix_nv_buf, FIXNV_SIZE - 4);
		dataaddr = (unsigned short *)(g_fix_nv_buf + FIXNV_SIZE - 4);
		*dataaddr = sum;
		dataaddr = (unsigned short *)(g_fix_nv_buf + FIXNV_SIZE - 2);
		*dataaddr = 0;

		//write the original partition
		_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName);
		if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector,
			nSectorCount, (unsigned char *)g_fix_nv_buf)) {
			//The fixnv checksum is error.
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}

		memset(g_fixbucknv_buf, 0xff, FIXNV_SIZE + EFI_SECTOR_SIZE);
		memcpy(g_fixbucknv_buf, g_fix_nv_buf, FIXNV_SIZE + 4);

		//write the backup partition
		backup_partition_name = _get_backup_partition_name(g_dl_eMMCStatus.curUserPartitionName);
		nSectorBase = efi_GetPartBaseSec(backup_partition_name);
		_emmc_real_erase_partition(backup_partition_name);
		if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, nSectorBase, nSectorCount,
			(unsigned char *)g_fix_nv_buf)) {
			//Write error
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}
	} 
	else if (is_ProdInfo_flag) 
	{
		if (0 == ((PRODUCTINFO_SIZE + 8) % EFI_SECTOR_SIZE))
			nSectorCount = (PRODUCTINFO_SIZE + 8) / EFI_SECTOR_SIZE;
		else
			nSectorCount = (PRODUCTINFO_SIZE + 8) / EFI_SECTOR_SIZE + 1;

		memset(g_prod_info_buf, 0xff, PRODUCTINFO_SIZE + EFI_SECTOR_SIZE);
		memcpy(g_prod_info_buf, g_eMMCBuf, PRODUCTINFO_SIZE + EFI_SECTOR_SIZE);

		sum = calc_checksum(g_prod_info_buf, PRODUCTINFO_SIZE - 4);
		dataaddr = (unsigned short *)(g_prod_info_buf + PRODUCTINFO_SIZE - 4);
		*dataaddr = sum;
		dataaddr = (unsigned short *)(g_prod_info_buf + PRODUCTINFO_SIZE - 2);
		*dataaddr = 0;
		//write the original partition
		_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName);
		if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector,
			nSectorCount, (unsigned char *)g_prod_info_buf)) {
			//write error
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}
		//write the backup partition
		backup_partition_name = _get_backup_partition_name(g_dl_eMMCStatus.curUserPartitionName);
		nSectorBase = efi_GetPartBaseSec(backup_partition_name);
		_emmc_real_erase_partition(backup_partition_name);
		if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, nSectorBase, nSectorCount, 
			(unsigned char *)g_prod_info_buf)) {
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}
	}
	
	g_dl_eMMCStatus.isLastPakFlag = 0;
	g_prevstatus = EMMC_SUCCESS;
    	FDL2_eMMC_SendRep (g_prevstatus);
	g_status.total_size  = 0;	
    	return 1;
}

/**
	Function used for reading NV or ProdInfo partition which has backup partition.
*/
LOCAL BOOLEAN _read_partition_with_backup(wchar_t *partition_name, uint8* buf, uint32 size)
{
	wchar_t *backup_partition_name = NULL;
	u32 base_sector;

	//read origin image
	memset(buf, 0xFF, size);
	base_sector = efi_GetPartBaseSec(partition_name);
	if(Emmc_Read(PARTITION_USER, base_sector, (size>>9)+1, (uint8*)buf)){
		//check crc
		if(_chkEcc(buf, size)){
			return 1;
		}
	}
	
	//get the backup partition name
	backup_partition_name = _get_backup_partition_name(partition_name);

	//read bakup image
	if(NULL== backup_partition_name){
		return 0;
	}
	memset(buf, 0xFF, size);
	base_sector = efi_GetPartBaseSec(backup_partition_name);
	if(!Emmc_Read(PARTITION_USER, base_sector, (size>>9), (uint8*)buf))
	{
		//...
		return 0;
	}
	if(!_chkEcc(buf, size)){
		SEND_ERROR_RSP(BSL_EEROR_CHECKSUM);
		return 0;
	}
	
	//write the backup image to origin image
	base_sector = efi_GetPartBaseSec(partition_name);
	Emmc_Write(PARTITION_USER, base_sector, (size>>9), (uint8*)buf);

	return 1;
}

PUBLIC int fdl2_emmc_read(unsigned long addr, unsigned long size, unsigned long off, unsigned char *buf)
{
    	unsigned long nSectorCount, nSectorOffset;
	unsigned long base_sector;
    	int ret = EMMC_SYSTEM_ERROR;

	g_dl_eMMCStatus.curUserPartition = addr2part(addr);
	g_dl_eMMCStatus.curUserPartitionName = NULL;
	g_dl_eMMCStatus.curEMMCArea = PARTITION_USER;

	if ((g_dl_eMMCStatus.curUserPartition < 0) || ((g_dl_eMMCStatus.curUserPartition >= MAX_PARTITION_INFO) && 
		(g_dl_eMMCStatus.curUserPartition !=PARTITION_SPL_LOADER && g_dl_eMMCStatus.curUserPartition != PARTITION_UBOOT)))
	{
		FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
		return FALSE;
	}

	//get the partition name before Tools ready
	if(!_get_partition_name())
	{
		FDL_SendAckPacket (EMMC_SYSTEM_ERROR);
		return FALSE;
	}
	
	if(wcsncmp(L"tdfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"tdfixnv1")) == 0 ||
		wcsncmp(L"wfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"wfixnv1")) == 0 ||
		wcsncmp(L"fixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"fixnv1")) == 0)
	{
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		if ((size > g_dl_eMMCStatus.part_total_size) || (size > FIXNV_SIZE)){
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return FALSE;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);

		if(_read_partition_with_backup(g_dl_eMMCStatus.curUserPartitionName, g_fix_nv_buf, FIXNV_SIZE))
		{
			memcpy(buf, (unsigned char *)(g_fix_nv_buf + off), size);
			ret =EMMC_SUCCESS;
		}
		else
		{
			FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
			return FALSE;
		}
	}
	else if (wcsncmp(L"prodinfo1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"prodinfo1")) == 0) 
	{
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		if ((size > g_dl_eMMCStatus.part_total_size) || (size > PRODUCTINFO_SIZE)) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return FALSE;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);

		if(_read_partition_with_backup(g_dl_eMMCStatus.curUserPartitionName, g_prod_info_buf, PRODUCTINFO_SIZE))
		{
			memcpy(buf, (unsigned char *)(g_prod_info_buf + off), size);
			ret =EMMC_SUCCESS;
		}
		else
		{
			FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
			return FALSE;
		}
	}
	else  if (wcsncmp(L"SPL_LOADER", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"SPL_LOADER")) == 0) 
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT1;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT1);
		g_dl_eMMCStatus.base_sector =  0;
	}
	else if (wcsncmp(L"UBOOT", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"UBOOT")) == 0) 
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT2;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT2);
		g_dl_eMMCStatus.base_sector =  0;
	}
	else 
	{
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
		g_dl_eMMCStatus.isLastPakFlag = 0;
	}

	if (size > g_dl_eMMCStatus.part_total_size) 
	{
		FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
		return FALSE;
	}

	if (EMMC_SUCCESS != ret)
	{
		if (0 == (size % EFI_SECTOR_SIZE))
			 nSectorCount = size / EFI_SECTOR_SIZE;
		else
			 nSectorCount = size / EFI_SECTOR_SIZE + 1;
		nSectorOffset = off / EFI_SECTOR_SIZE;
		if (Emmc_Read(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector + nSectorOffset,  nSectorCount, buf ))
			ret = EMMC_SUCCESS;
		else
			ret = EMMC_SYSTEM_ERROR;
	}

    	if (EMMC_SUCCESS == ret) {
	        return TRUE;
    	} else {
        	FDL2_eMMC_SendRep (ret);
        	return FALSE;
    	}
}

PUBLIC int fdl2_emmc_erase(unsigned long addr, unsigned long size)
{
	int ret = EMMC_SUCCESS;
	int retval;
	unsigned long part_size;
	wchar_t *backup_partition_name = NULL;

	if ((addr == 0) && (size = 0xffffffff)) {
		printf("Scrub to erase all of flash\n");
		if (!_emmc_erase_allflash()) {
			SEND_ERROR_RSP (BSL_WRITE_ERROR);			
			return 0;
		}
		ret = EMMC_SUCCESS;
	} else {
		g_dl_eMMCStatus.curUserPartition = addr2part(addr);
		g_dl_eMMCStatus.curUserPartitionName = NULL;

		//get the partition name before Tools ready
		if(!_get_partition_name())
		{
			FDL_SendAckPacket (BSL_INCOMPATIBLE_PARTITION);
			return 0;
		}
		
		if (!_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName)) {
			SEND_ERROR_RSP (BSL_WRITE_ERROR);
			return 0;
		}

		backup_partition_name = _get_backup_partition_name(g_dl_eMMCStatus.curUserPartitionName);

		if(NULL != backup_partition_name)
		{
			if (!_emmc_real_erase_partition(backup_partition_name)) {
				SEND_ERROR_RSP (BSL_WRITE_ERROR);
				return 0;
			}
		}

		if (wcsncmp(L"prodinfo3", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"prodinfo3")) == 0) {
			part_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
			make_ext4fs_main(g_eMMCBuf, part_size);

			retval = write_simg2emmc("mmc", 1, PARTITION_PROD_INFO3, g_eMMCBuf, EMMC_BUF_SIZE);
			if (retval == -1) {
				SEND_ERROR_RSP (BSL_WRITE_ERROR);
				return 0;
			}
		}
		
		if (wcsncmp(L"sd", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"sd")) == 0) {
			printf("formating sd partition, waiting for a while!\n");
			if (_format_sd_partition() == -1){
				printf("format sd partition failed\n");
				SEND_ERROR_RSP (BSL_WRITE_ERROR);
				return 0;
			}
		}
		ret = EMMC_SUCCESS;
	}

	FDL2_eMMC_SendRep (EMMC_SUCCESS);

	return 1;
}

//will be modified later because of partition info should send by tool
PUBLIC int fdl2_emmc_repartition(void)
{
	int i, ret = 0;
	for (i = 0; i < 3; i++) {
		#ifdef CONFIG_EBR_PARTITION
			if(write_mbr_partition_table()) {
				FDL2_eMMC_SendRep (EMMC_SUCCESS);
				return 1;
				}
		#else
			write_uefi_partition_table(g_sprd_emmc_partition_cfg);
		#endif
			if (FDL_Check_Partition_Table())
				break;
	}

	if (i < 3) {
		FDL2_eMMC_SendRep (EMMC_SUCCESS);
		return 1;
	} else {
		FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
		return 0;
	}
}
#endif

