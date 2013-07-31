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
#include "fdl_emmc_operate.h"

#define EFI_SECTOR_SIZE 		(512)
#define ERASE_SECTOR_SIZE		((64 * 1024) / EFI_SECTOR_SIZE)
#define EMMC_BUF_SIZE			(((216 * 1024 * 1024) / EFI_SECTOR_SIZE) * EFI_SECTOR_SIZE)

#define SPL_CHECKSUM_LEN	0x6000

#define PARTNER_PARTITION_NUM  (7)
#define MAX_GPT_PARTITION_SUPPORT  (50)
#define EMMC_ERASE_ALIGN_LENGTH  (0x800)

static DL_EMMC_FILE_STATUS g_status;
static DL_EMMC_STATUS g_dl_eMMCStatus = {0, 0, 0xffffffff,0, 0, 0};

static unsigned long g_checksum;
static unsigned long g_sram_addr;

#if defined (CONFIG_SC8825) || defined (CONFIG_TIGER) || defined (CONFIG_SC8830)
unsigned char *g_eMMCBuf = (unsigned char*)0x82000000;
#else
unsigned char *g_eMMCBuf = (unsigned char*)0x2000000;
#endif

unsigned int g_total_partition_number = 0;

static wchar_t s_uboot_partition[MAX_UTF_PARTITION_NAME_LEN] = L"uboot";
static wchar_t s_spl_loader_partition[MAX_UTF_PARTITION_NAME_LEN] = L"splloader";

static int uefi_part_info_ok_flag = 0;
static PARTITION_CFG uefi_part_info[MAX_GPT_PARTITION_SUPPORT] = {0};
static PARTITION_CFG s_sprd_emmc_partition_cfg[MAX_GPT_PARTITION_SUPPORT] = {0};

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
	//return gpio_get_value(EMMC_SELECT_GPIO);
	return 1;
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
LOCAL unsigned int get_pad_data(const unsigned int *src, int len, int offset, unsigned short sum)
{
	unsigned int sum_tmp;
	unsigned int sum1 = 0;
	unsigned int pad_data;
	unsigned int i;
	sum = ~sum;
	sum_tmp = sum & 0xffff;
	sum1 = 0;
	for(i = 0; i < offset; i++) {
		sum1 += src[i];
	}
	for(i = (offset + 1); i < len; i++) {
		sum1 += src[i];
	}
	pad_data = sum_tmp - sum1;
	return pad_data;
}

LOCAL void splFillCheckData(unsigned int * splBuf,  int len)
{
	#define MAGIC_DATA	0xAA55A5A5
	#define CHECKSUM_START_OFFSET	0x28
	#define MAGIC_DATA_SAVE_OFFSET	(0x20/4)
	#define CHECKSUM_SAVE_OFFSET	(0x24/4)

#if defined(CONFIG_TIGER) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
	EMMC_BootHeader *header;
#if defined(CONFIG_SC8830)
	unsigned int pad_data;
	unsigned int w_len;
	unsigned int w_offset;
	w_len = (SPL_CHECKSUM_LEN-(BOOTLOADER_HEADER_OFFSET+sizeof(*header))) / 4;
	w_offset = w_len - 1;
	//pad the data inorder to make check sum to 0
	pad_data = (unsigned int)get_pad_data((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET+sizeof(*header), w_len, w_offset, 0);
	*(volatile unsigned int *)((unsigned char*)splBuf+SPL_CHECKSUM_LEN - 4) = pad_data;
#endif
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

	for(i=0;i<g_total_partition_number;i++)
	{
		if(wcsncmp(partition_name, uefi_part_info[i].partition_name, wcslen(partition_name)) == 0)
		{
			return uefi_part_info[i].partition_offset;
		}
	}

	//Can't find the specified partition
	FDL_SendAckPacket (BSL_INCOMPATIBLE_PARTITION);
	return 0;
}

LOCAL unsigned long efi_GetPartSize(wchar_t *partition_name)
{
	int i;

	_uefi_get_part_info();

	for(i=0;i<g_total_partition_number;i++)
	{
		if(wcsncmp(partition_name, uefi_part_info[i].partition_name, wcslen(partition_name)) == 0)
			return (EFI_SECTOR_SIZE * uefi_part_info[i].partition_size);
	}

	//Can't find the specified partition
	FDL_SendAckPacket (BSL_INCOMPATIBLE_PARTITION);
	return 0;
}

LOCAL void _parser_repartition_cfg(unsigned short* partition_cfg, unsigned short total_partition_num)
{
	int i, j;
	unsigned long partition_size;

	/*Decode String: Partition Name(72Byte)+SIZE(4Byte)+...*/
	for(i=0;i<total_partition_num;i++)
	{
		partition_size = *(unsigned long *)(partition_cfg+38*(i+1)-2);
		s_sprd_emmc_partition_cfg[i].partition_index = i+1;
		//the partition size received from tool is MByte.
		if(MAX_SIZE_FLAG == partition_size)
			s_sprd_emmc_partition_cfg[i].partition_size = partition_size;
		else
			s_sprd_emmc_partition_cfg[i].partition_size = 1024*partition_size;
		s_sprd_emmc_partition_cfg[i].partition_attr = PARTITION_RAW;//TODO
		s_sprd_emmc_partition_cfg[i].partition_offset = 0;
		for(j=0;j<MAX_UTF_PARTITION_NAME_LEN;j++)
		{
			s_sprd_emmc_partition_cfg[i].partition_name[j] = *(partition_cfg+38*i+j);
		}

		printf("_decode_repartition_cfg:partition name:%S,partition_size:%d\n",s_sprd_emmc_partition_cfg[i].partition_name,s_sprd_emmc_partition_cfg[i].partition_size);
	}
	return;
}

/**
	Check whether the partition to be operated is compatible.
*/
LOCAL BOOLEAN _get_compatible_partition(wchar_t* partition_name)
{
	int i;

	//Get user partition info from eMMC
	_uefi_get_part_info();

	//Try to find the partition specified
	if(wcsncmp(partition_name, L"uboot", wcslen(L"uboot")) == 0)
	{
		g_dl_eMMCStatus.curUserPartitionName = s_uboot_partition;
		return TRUE;
	}
	if(wcsncmp(partition_name, L"splloader", wcslen(L"splloader")) == 0)
	{
		g_dl_eMMCStatus.curUserPartitionName = s_spl_loader_partition;
		return TRUE;
	}

	for(i=0;i<g_total_partition_number;i++)
	{
		if(wcsncmp(partition_name, uefi_part_info[i].partition_name, wcslen(partition_name)) == 0)
		{
			g_dl_eMMCStatus.curUserPartition = uefi_part_info[i].partition_index;
			g_dl_eMMCStatus.curUserPartitionName = uefi_part_info[i].partition_name;
			return TRUE;
		}
	}
	//Can't find the specified partition
	g_dl_eMMCStatus.curUserPartitionName = NULL;
	return FALSE;
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

/**
	Partition Check Function
	Return:
		0:No partition table or not same with New
		1:Same with new partition
*/
LOCAL int _fdl2_check_partition_table(PARTITION_CFG* new_cfg, PARTITION_CFG* old_cfg, unsigned short total_partition_num)
{
	int i;
	printf("_fdl2_check_partition_table -----\n");

	uefi_part_info_ok_flag = 0;

	if (!_uefi_get_part_info())
	{
		return 0;
	}

	//we may modify starting LBA of first partition,so we should check it.
	if(STARTING_LBA_OF_FIRST_PARTITION != old_cfg[0].partition_offset)
		return 0;

	if(total_partition_num == g_total_partition_number)
	{
		for(i=0;i<total_partition_num;i++)
		{
			if(0 !=wcsncmp(new_cfg[i].partition_name,old_cfg[i].partition_name,wcslen(old_cfg[i].partition_name)))
			{
				return 0;
			}
			else
			{
				/*
					the new_cfg partition size get from tool is xx kbyte, 
					the old_cfg partition size read from disk is yy*sector_size byte.
				*/
				if(2*new_cfg[i].partition_size != old_cfg[i].partition_size && new_cfg[i].partition_size !=0xffffffff)
					return 0;
			}
		}
	}
	return 1;
}

LOCAL int _format_sd_partition(void)
{
	unsigned long part_size = 0;
	unsigned long sd_data_size = 0;
	unsigned long base_sector = 0;

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
	Erase the whole partition.
*/
LOCAL int _emmc_real_erase_partition(wchar_t *partition_name)
{
	unsigned long count=0, base_sector=0;
	uint8 curArea = 0;

	if(NULL == partition_name)
		return 0;

	if (wcsncmp(L"splloader", partition_name, wcslen(L"splloader")) == 0){
		if(secureboot_enabled()){
			return 1;
		}
		count = Emmc_GetCapacity(PARTITION_BOOT1);
		curArea = PARTITION_BOOT1;
		base_sector = 0;
	}else if (wcsncmp(L"uboot", partition_name, wcslen(L"uboot")) == 0){
		count = Emmc_GetCapacity(PARTITION_BOOT2);
		curArea = PARTITION_BOOT2;
		base_sector = 0;
	}
	else{
		curArea = PARTITION_USER;
		count = efi_GetPartSize(partition_name); /* partition size : in bytes */
		count = count / EFI_SECTOR_SIZE; /* partition size : in blocks */
		base_sector = efi_GetPartBaseSec(partition_name);
	}

	if(count < EMMC_ERASE_ALIGN_LENGTH)
	{
		unsigned char buf[EMMC_ERASE_ALIGN_LENGTH*EFI_SECTOR_SIZE] = {0xFF};
		if (!Emmc_Write(curArea, base_sector,count,buf))
			return 0;
	}
	else
	{
		if(base_sector%EMMC_ERASE_ALIGN_LENGTH)
		{
			unsigned char buf[EMMC_ERASE_ALIGN_LENGTH*EFI_SECTOR_SIZE] = {0xFF};
			unsigned long base_sector_offset = 0;

			base_sector_offset = EMMC_ERASE_ALIGN_LENGTH - base_sector%EMMC_ERASE_ALIGN_LENGTH;
			if (!Emmc_Write(curArea, base_sector,base_sector_offset,buf))
				return 0;
			count = ((count-base_sector_offset)/EMMC_ERASE_ALIGN_LENGTH)*EMMC_ERASE_ALIGN_LENGTH;
			base_sector = base_sector + base_sector_offset;
		}
		else
			count = (count/EMMC_ERASE_ALIGN_LENGTH)*EMMC_ERASE_ALIGN_LENGTH;

		if(count == 0)
			return 1;

		if (!Emmc_Erase(curArea, base_sector,count))
			return 0;	
	}

	return 1;
}

/**
	Fast erase userpartition use emmc_write .
*/
LOCAL int _emmc_fast_erase_userpartition(wchar_t *partition_name)
{
	unsigned long i, count, len,  base_sector;
	uint8 curArea;

	curArea = PARTITION_USER;
	len = efi_GetPartSize(partition_name);
	count = len / EFI_SECTOR_SIZE;
	base_sector = efi_GetPartBaseSec(partition_name);
	count = (count > ERASE_SECTOR_SIZE)?ERASE_SECTOR_SIZE:count;
	memset(g_eMMCBuf, 0xff, count * EFI_SECTOR_SIZE);		
	if (!Emmc_Write(curArea, base_sector, count, (unsigned char *)g_eMMCBuf))
		 return 0;

	return 1;
}

LOCAL int _emmc_erase_allflash(void)
{
	int i;
	uint32 count;

	_uefi_get_part_info();
	memset(g_eMMCBuf, 0xff, ERASE_SECTOR_SIZE*EFI_SECTOR_SIZE);
	//erase user partitions
	for (i = 0; i < g_total_partition_number; i++) {
		if (!_emmc_fast_erase_userpartition(uefi_part_info[i].partition_name))
			return 0;
	}
	//erase gpt header and partition entry table
	if (!Emmc_Write(PARTITION_USER, 0, ERASE_SECTOR_SIZE, (unsigned char *)g_eMMCBuf))
		 return 0;
	//erase backup gpt header
	count = Emmc_GetCapacity(PARTITION_USER);
	if (!Emmc_Write(PARTITION_USER, count - 1, 1, (unsigned char *)g_eMMCBuf))
		 return 0;
	//erase boot1 partition
	if(!_emmc_real_erase_partition(L"splloader"))
		return 0;
	//erase boot2 partition
	if(!_emmc_real_erase_partition(L"uboot"))
		return 0;

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
			g_sram_addr = (unsigned long)(g_eMMCBuf+g_status.unsave_recv_size);
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

LOCAL int _emmc_nv_check_and_write(void)
{
	unsigned long  fix_nv_checksum, nSectorCount, nSectorBase;
	unsigned short sum = 0, *dataaddr;
	wchar_t *backup_partition_name = NULL;
	
	fix_nv_checksum = Get_CheckSum((unsigned char *) g_eMMCBuf, g_status.total_recv_size);
	if (fix_nv_checksum != g_checksum) {
		//may data transfer error
        	SEND_ERROR_RSP(BSL_CHECKSUM_DIFF);
		return 0;
        }

	if (0 == ((FIXNV_SIZE + 4) % EFI_SECTOR_SIZE))
		nSectorCount = (FIXNV_SIZE + 4) / EFI_SECTOR_SIZE;
	else
		nSectorCount = (FIXNV_SIZE + 4) / EFI_SECTOR_SIZE + 1;

	sum = calc_checksum(g_eMMCBuf, FIXNV_SIZE - 4);
	dataaddr = (unsigned short *)(g_eMMCBuf + FIXNV_SIZE - 4);
	*dataaddr = sum;
	dataaddr = (unsigned short *)(g_eMMCBuf + FIXNV_SIZE - 2);
	*dataaddr = 0;

	//write the original partition
	_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName);
	if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector,
		nSectorCount, (unsigned char *)g_eMMCBuf)) {
		//The fixnv checksum is error.
		SEND_ERROR_RSP (BSL_WRITE_ERROR);
		return 0;
	}

	//write the backup partition
	backup_partition_name = _get_backup_partition_name(g_dl_eMMCStatus.curUserPartitionName);
	nSectorBase = efi_GetPartBaseSec(backup_partition_name);
	_emmc_real_erase_partition(backup_partition_name);
	if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, nSectorBase, nSectorCount,
		(unsigned char *)g_eMMCBuf)) {
		//Write error
		SEND_ERROR_RSP (BSL_WRITE_ERROR);
		return 0;
	}

	g_status.unsave_recv_size = 0;
	return 1;
}

LOCAL int _emmc_prodinfo_check_and_write(void)
{
	unsigned long  nSectorCount, nSectorBase;
	unsigned short sum = 0, *dataaddr;
	wchar_t *backup_partition_name = NULL;
	
	if (0 == ((PRODUCTINFO_SIZE + 8) % EFI_SECTOR_SIZE))
		nSectorCount = (PRODUCTINFO_SIZE + 8) / EFI_SECTOR_SIZE;
	else
		nSectorCount = (PRODUCTINFO_SIZE + 8) / EFI_SECTOR_SIZE + 1;

	sum = calc_checksum(g_eMMCBuf, PRODUCTINFO_SIZE - 4);
	dataaddr = (unsigned short *)(g_eMMCBuf + PRODUCTINFO_SIZE - 4);
	*dataaddr = sum;
	dataaddr = (unsigned short *)(g_eMMCBuf + PRODUCTINFO_SIZE - 2);
	*dataaddr = 0;
	//write the original partition
	_emmc_real_erase_partition(g_dl_eMMCStatus.curUserPartitionName);
	if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector,
		nSectorCount, (unsigned char *)g_eMMCBuf)) {
		//write error
		SEND_ERROR_RSP (BSL_WRITE_ERROR);
		return 0;
	}
	//write the backup partition
	backup_partition_name = _get_backup_partition_name(g_dl_eMMCStatus.curUserPartitionName);
	nSectorBase = efi_GetPartBaseSec(backup_partition_name);
	_emmc_real_erase_partition(backup_partition_name);
	if (!Emmc_Write(g_dl_eMMCStatus.curEMMCArea, nSectorBase, nSectorCount, 
		(unsigned char *)g_eMMCBuf)) {
		SEND_ERROR_RSP (BSL_WRITE_ERROR);
		return 0;
	}

	g_status.unsave_recv_size = 0;
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

PUBLIC int fdl2_emmc_download_start(wchar_t* partition_name, unsigned long size, unsigned long nv_checksum)
{
	int i = 0;

	g_status.total_size  = size;

	if(!_get_compatible_partition(partition_name))
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
	} 
	else if (wcsncmp(L"splloader", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"splloader")) == 0)
	{
		if(secureboot_enabled()){
			return 1;
		}
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT1;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT1);
		g_dl_eMMCStatus.base_sector =  0;
		memset(g_eMMCBuf, 0xff, g_dl_eMMCStatus.part_total_size);
		if (size > g_dl_eMMCStatus.part_total_size) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return 0;
		}
	}
	else if (wcsncmp(L"uboot", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"uboot")) == 0)
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT2;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT2);
		g_dl_eMMCStatus.base_sector =  0;
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
	}

	g_status.total_recv_size   = 0;
	g_status.unsave_recv_size   = 0;
	g_sram_addr = (unsigned long)g_eMMCBuf;
	FDL_SendAckPacket (BSL_REP_ACK);
	return 1;
}

PUBLIC int fdl2_emmc_download(unsigned short size, char *buf)
{
	unsigned long lastSize, nSectorCount;
	unsigned long each_write_block = 10000;

	if ((g_status.total_recv_size + size) > g_status.total_size) {
		FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
		return 0;
	}

	g_status.total_recv_size += size;

	if (EMMC_BUF_SIZE >= (g_status.unsave_recv_size + size)) 
	{
		memcpy((unsigned char *)g_sram_addr, buf, size);
		g_sram_addr += size;
		g_status.unsave_recv_size += size;
		
		if (EMMC_BUF_SIZE == g_status.unsave_recv_size)
		{
			if (0 == (EMMC_BUF_SIZE % EFI_SECTOR_SIZE))
				nSectorCount = EMMC_BUF_SIZE / EFI_SECTOR_SIZE;
			else
				nSectorCount = EMMC_BUF_SIZE / EFI_SECTOR_SIZE + 1;

			if(!_emmc_download_image(nSectorCount, each_write_block, FALSE))
				return 0;
		}
		else if (g_status.total_recv_size == g_status.total_size) 
		{
			if ((wcsncmp(L"tdfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"tdfixnv1")) == 0)||
				(wcsncmp(L"wfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"wfixnv1")) == 0)||
				(wcsncmp(L"fixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"fixnv1")) == 0))
			{
				if(!_emmc_nv_check_and_write())
					return 0;
			}
			else if(wcsncmp(L"prodinfo1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"prodinfo1")) == 0)
			{
				if(!_emmc_prodinfo_check_and_write())
					return 0;
			}
			else
			{
				if (g_status.unsave_recv_size != 0) {
					if (0 == (g_status.unsave_recv_size % EFI_SECTOR_SIZE))
						nSectorCount = g_status.unsave_recv_size / EFI_SECTOR_SIZE;
					else
						nSectorCount = g_status.unsave_recv_size / EFI_SECTOR_SIZE + 1;
					
					if (wcsncmp(L"splloader", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"splloader")) == 0){
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
		} 
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

	FDL2_eMMC_SendRep (EMMC_SUCCESS);
	return  1; 
}

PUBLIC int fdl2_emmc_download_end(void)
{
	if(g_status.unsave_recv_size != 0)
	{
		FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
		return 0;
	}

    	FDL2_eMMC_SendRep (EMMC_SUCCESS);
	g_status.total_size  = 0;	
    	return 1;
}

PUBLIC int fdl2_emmc_read_start(wchar_t* partition_name, unsigned long size)
{
	if(!_get_compatible_partition(partition_name))
	{
		FDL_SendAckPacket (convert_err (EMMC_INCOMPATIBLE_PART));
		return FALSE;
	}

	g_dl_eMMCStatus.curEMMCArea = PARTITION_USER;

	if(wcsncmp(L"tdfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"tdfixnv1")) == 0 ||
		wcsncmp(L"wfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"wfixnv1")) == 0 ||
		wcsncmp(L"fixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"fixnv1")) == 0)
	{
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		if ((size > g_dl_eMMCStatus.part_total_size) /*|| (size > FIXNV_SIZE)*/){
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return FALSE;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
	}
	else if (wcsncmp(L"prodinfo1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"prodinfo1")) == 0)
	{
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		if ((size > g_dl_eMMCStatus.part_total_size)/* || (size > PRODUCTINFO_SIZE)*/) {
			FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
			return FALSE;
		}
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
	}
	else  if (wcsncmp(L"splloader", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"splloader")) == 0)
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT1;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT1);
		g_dl_eMMCStatus.base_sector =  0;
	}
	else if (wcsncmp(L"uboot", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"uboot")) == 0)
	{
		g_dl_eMMCStatus.curEMMCArea = PARTITION_BOOT2;
		g_dl_eMMCStatus.part_total_size = EFI_SECTOR_SIZE * Emmc_GetCapacity(PARTITION_BOOT2);
		g_dl_eMMCStatus.base_sector =  0;
	}
	else
	{
		g_dl_eMMCStatus.part_total_size = efi_GetPartSize(g_dl_eMMCStatus.curUserPartitionName);
		g_dl_eMMCStatus.base_sector = efi_GetPartBaseSec(g_dl_eMMCStatus.curUserPartitionName);
	}

	if (size > g_dl_eMMCStatus.part_total_size)
	{
		FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
		return FALSE;
	}

	FDL_SendAckPacket (BSL_REP_ACK);

	return TRUE;
}

PUBLIC int fdl2_emmc_read_midst(unsigned long size, unsigned long off, unsigned char *buf)
{
	unsigned long nSectorCount, nSectorOffset;

	if ((size +off) > g_dl_eMMCStatus.part_total_size)
	{
		FDL2_eMMC_SendRep (EMMC_INVALID_SIZE);
		return FALSE;
	}

	if(wcsncmp(L"tdfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"tdfixnv1")) == 0 ||
		wcsncmp(L"wfixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"wfixnv1")) == 0 ||
		wcsncmp(L"fixnv1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"fixnv1")) == 0)
	{
		if(_read_partition_with_backup(g_dl_eMMCStatus.curUserPartitionName, g_eMMCBuf, FIXNV_SIZE))
		{
			memcpy(buf, (unsigned char *)(g_eMMCBuf + off), size);
			return TRUE;
		}
		else
		{
			FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
			return FALSE;
		}
	}
	else if (wcsncmp(L"prodinfo1", g_dl_eMMCStatus.curUserPartitionName, wcslen(L"prodinfo1")) == 0)
	{
		if(_read_partition_with_backup(g_dl_eMMCStatus.curUserPartitionName, g_eMMCBuf, PRODUCTINFO_SIZE))
		{
			memcpy(buf, (unsigned char *)(g_eMMCBuf + off), size);
			return TRUE;
		}
		else
		{
			FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
			return FALSE;
		}
	}
	else
	{
		if (0 == (size % EFI_SECTOR_SIZE))
			 nSectorCount = size / EFI_SECTOR_SIZE;
		else
			 nSectorCount = size / EFI_SECTOR_SIZE + 1;
		nSectorOffset = off / EFI_SECTOR_SIZE;
		if (!Emmc_Read(g_dl_eMMCStatus.curEMMCArea, g_dl_eMMCStatus.base_sector + nSectorOffset,  nSectorCount, buf ))
		{
			FDL2_eMMC_SendRep (EMMC_SYSTEM_ERROR);
			return FALSE;
		}
	}

	return TRUE;
}

PUBLIC int fdl2_emmc_read_end(void)
{
	//Just send ack to tool in emmc
	FDL_SendAckPacket (BSL_REP_ACK);
	return TRUE;
}

PUBLIC int fdl2_emmc_erase(wchar_t* partition_name, unsigned long size)
{
	int retval;
	unsigned long part_size;
	wchar_t *backup_partition_name = NULL;

	if ((wcsncmp(partition_name, L"erase_all", wcslen(L"erase_all")) == 0) && (size = 0xffffffff)) {
		printf("Scrub to erase all of flash\n");
		if (!_emmc_erase_allflash()) {
			SEND_ERROR_RSP (BSL_WRITE_ERROR);			
			return 0;
		}
	} else {
		if(!_get_compatible_partition(partition_name))
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
	}

	FDL2_eMMC_SendRep (EMMC_SUCCESS);
	return 1;
}

PUBLIC int fdl2_emmc_repartition(unsigned short* partition_cfg, unsigned short total_partition_num)
{
	int i;

	_parser_repartition_cfg(partition_cfg, total_partition_num);

	if(_fdl2_check_partition_table(s_sprd_emmc_partition_cfg, uefi_part_info, total_partition_num))
	{
		printf("fdl2_emmc_repartition:Partition Config same with before!");
		FDL2_eMMC_SendRep (EMMC_SUCCESS);
		return 1;
	}

	for (i = 0; i < 3; i++) {
		#ifdef CONFIG_EBR_PARTITION
			if(write_mbr_partition_table()) {
				FDL2_eMMC_SendRep (EMMC_SUCCESS);
				return 1;
				}
		#else
			write_uefi_partition_table(s_sprd_emmc_partition_cfg);
		#endif
			if (_fdl2_check_partition_table(s_sprd_emmc_partition_cfg, uefi_part_info, total_partition_num))
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

