#include <config.h>
#include "normal_mode.h"

#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>

#define KERNL_PAGE_SIZE 2048
#define PRODUCTINFO_FILE_PATITION  L"miscdata"
#define SP09_SPPH_MAGIC             (0X53503039)    // "SP09"
static char     	product_SN[20+1];
static int		product_SN_flag = 0;
static wchar_t *factory_partition = L"prodnv";

typedef struct  _NV_HEADER {
     uint32 magic;
     uint32 len;
     uint32 checksum;
     uint32   version;
}nv_header_t;
#define NV_HEAD_MAGIC   0x00004e56
#define NV_VERSION      101

static boot_image_required_t const s_boot_image_table[]={
#ifdef CONFIG_SC8830

#if defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB)
	{L"tdfixnv1",L"tdfixnv2",FIXNV_SIZE,TDFIXNV_ADR},
	{L"tdruntimenv1",L"tdruntimenv2",RUNTIMENV_SIZE,TDRUNTIMENV_ADR},
	{L"tdmodem",NULL,MODEM_SIZE,TDMODEM_ADR},
	{L"tddsp",NULL,DSP_SIZE,TDDSP_ADR}, 
#ifdef CONFIG_SP8830WCN
	{L"wcnfixnv1",L"wcnfixnv2",FIXNV_SIZE,WCNFIXNV_ADR},
	{L"wcnruntimenv1",L"wcnruntimenv2",RUNTIMENV_SIZE,WCNRUNTIMENV_ADR},
	{L"wcnmodem",NULL,MODEM_SIZE,WCNMODEM_ADR},
#endif		
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SP5735)
	{L"wfixnv1",L"wfixnv2",FIXNV_SIZE,WFIXNV_ADR},
	{L"wruntimenv1",L"wruntimenv2",RUNTIMENV_SIZE,WRUNTIMENV_ADR},
	{L"wmodem",NULL,MODEM_SIZE,WMODEM_ADR},
	{L"wdsp",NULL,DSP_SIZE,WDSP_ADR},
#ifdef CONFIG_SP8830WCN
	{L"wcnfixnv1",L"wcnfixnv2",FIXNV_SIZE,WCNFIXNV_ADR},
	{L"wcnruntimenv1",L"wcnruntimenv2",RUNTIMENV_SIZE,WCNRUNTIMENV_ADR},
	{L"wcnmodem",NULL,MODEM_SIZE,WCNMODEM_ADR},
#endif	
#else
#ifndef CONFIG_NOT_BOOT_TD_MODEM
	{L"tdfixnv1",L"tdfixnv2",FIXNV_SIZE,TDFIXNV_ADR},
	{L"tdruntimenv1",L"tdruntimenv2",RUNTIMENV_SIZE,TDRUNTIMENV_ADR},
	{L"tdmodem",NULL,MODEM_SIZE,TDMODEM_ADR},
	{L"tddsp",NULL,DSP_SIZE,TDDSP_ADR},		
#endif
#ifndef CONFIG_NOT_BOOT_W_MODEM
	{L"wfixnv1",L"wfixnv2",FIXNV_SIZE,WFIXNV_ADR},
	{L"wruntimenv1",L"wruntimenv2",RUNTIMENV_SIZE,WRUNTIMENV_ADR},
	{L"wmodem",NULL,MODEM_SIZE,WMODEM_ADR},
#ifdef CONFIG_SP8830EB
	{L"wdsp",NULL,WDSP_SIZE,WDSP_ADR},
#else
	{L"wdsp",NULL,DSP_SIZE,WDSP_ADR},
#endif/*CONFIG_SP8830EB*/
#endif
#endif	

#else
	{L"fixnv1",L"fixnv2",FIXNV_SIZE,FIXNV_ADR},
	{L"runtimenv1",L"runtimenv2",RUNTIMENV_SIZE,RUNTIMENV_ADR},
	{L"modem",NULL,MODEM_SIZE,MODEM_ADR},
	{L"dsp",NULL,DSP_SIZE,DSP_ADR},
#endif
#if !BOOT_NATIVE_LINUX
	{L"vm",NULL,VMJALUNA_SIZE,VMJALUNA_ADR},
#endif
#ifdef CONFIG_SIMLOCK
	{L"simlock",NULL,SIMLOCK_SIZE,SIMLOCK_ADR},
#endif
	{NULL,NULL,0,0}
};
static void product_SN_get(void);
int Calibration_read_partition(block_dev_desc_t *p_block_dev, wchar_t* partition_name, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = len % EMMC_SECTOR_SIZE;
	uint32 numb = len / EMMC_SECTOR_SIZE;
	int ret = 0; /* success */
	char * buffer =NULL;

	if ( 0 != get_partition_info_by_name(p_block_dev, partition_name, &info)) {
		debugf("## %s partition not found ##\n", partition_name);
		return -1;
	}
	debugf("%s: numb = %d  size= %d\n", __FUNCTION__, numb, size);

	if(len == 0){
		debugf("The size for reading error \n");
		return -1;
	}

	if(size == 0){
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, numb, (uint8*)buf)) {
			debugf("emmc image0 read error \n");
			return -1;
		}
		return ret;
	}

	if(numb >0){
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, numb, (uint8*)buf)) {
			debugf("emmc image1 read error \n");
			return -1;
		}
		info.start = info.start + numb * EMMC_SECTOR_SIZE;
	}

	buffer = malloc(EMMC_SECTOR_SIZE);
	if(buffer == NULL){
		debugf("malloc memory  error \n");
		return -1;
	}
	memset(buffer, 0xff, EMMC_SECTOR_SIZE);
	if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, 1, (uint8*)buffer)) {
		debugf("emmc image2 read error \n");
		free(buffer);
		return -1;
	}
	memcpy((buf+numb*EMMC_SECTOR_SIZE),buffer,size);
	free(buffer);
	return ret;
}

int Calibration_write_partition(block_dev_desc_t *p_block_dev, wchar_t* partition_name, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = len % EMMC_SECTOR_SIZE;
	uint32 numb = len / EMMC_SECTOR_SIZE;
	int ret = 0; /* success */
	char * buffer =NULL;

	if ( 0 != get_partition_info_by_name(p_block_dev, partition_name, &info)) {
		debugf("## %s partition not found ##\n", partition_name);
		return -1;
	}
	debugf("%s: numb = %d  size= %d\n", __FUNCTION__, numb, size);

	if(len == 0){
		debugf("The size for writing error \n");
		return -1;
	}

	if(size == 0){
		if (TRUE !=  Emmc_Write(PARTITION_USER, info.start, numb, (uint8*)buf)) {
			debugf("emmc image0 write error \n");
			return -1;
		}
		return ret;
	}

	if(numb >0){
		if (TRUE !=  Emmc_Write(PARTITION_USER, info.start, numb, (uint8*)buf)) {
			debugf("emmc image1 write error \n");
			return -1;
		}
		info.start = info.start + numb * EMMC_SECTOR_SIZE;
	}

	buffer = malloc(EMMC_SECTOR_SIZE);
	if(buffer == NULL){
		debugf("malloc memory  error \n");
		return -1;
	}
	memset(buffer, 0xff, EMMC_SECTOR_SIZE);
	memcpy(buffer,(buf+numb*EMMC_SECTOR_SIZE),size);
	if (TRUE !=  Emmc_Write(PARTITION_USER, info.start, 1, (uint8*)buffer)) {
		debugf("emmc image2 write error \n");
		free(buffer);
		return -1;
	}
	free(buffer);
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

int prodinfo_read_partition(block_dev_desc_t *p_block_dev, wchar_t *partition, int offset, char *buf, int len)
{
	disk_partition_t info;
	unsigned long size = len % EMMC_SECTOR_SIZE;
	uint32 numb = len / EMMC_SECTOR_SIZE;
	int ret = 0; /* success */
	unsigned long crc;
	unsigned long offset_block = offset / EMMC_SECTOR_SIZE;
	char * buffer =NULL;

	if ( 0 != get_partition_info_by_name(p_block_dev, partition, &info)) {
		debugf("## %s partition not found ##\n", partition);
		return 1;
	}
	debugf("%s: numb = %d  size= %d\n", __FUNCTION__, numb, size);
	if(len == 0){
		debugf("The size for reading error \n");
		return 1;
	}
	info.start = info.start + offset_block;
	if(size != 0){
		if(numb >0){
			if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, numb, (uint8*)buf)) {
				debugf("emmc image1 read error \n");
				return 1;
			}
			info.start = info.start + numb * EMMC_SECTOR_SIZE;
		}

		buffer = malloc(EMMC_SECTOR_SIZE);
		if(buffer == NULL){
			debugf("malloc memory  error \n");
			return 1;
		}
		memset(buffer, 0xff, EMMC_SECTOR_SIZE);
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, 1, (uint8*)buffer)) {
			debugf("emmc image2 read error \n");
			free(buffer);
			return 1;
		}
		memcpy((buf+numb*EMMC_SECTOR_SIZE),buffer,size);
		free(buffer);
	}
	else{
		if (TRUE !=  Emmc_Read(PARTITION_USER, info.start, numb, (uint8*)buf)) {
			debugf("emmc image3 read error \n");
			return 1;
		}
	}

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



int read_logoimg(char *bmp_img,size_t size)
{
	block_dev_desc_t *p_block_dev = NULL;
	disk_partition_t info;

	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		return -1;
	}
	if (!get_partition_info_by_name(p_block_dev, L"logo", &info)) {
		if(TRUE !=  Emmc_Read(PARTITION_USER, info.start, size/EMMC_SECTOR_SIZE, bmp_img)){
			debugf("function: %s nand read error\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}

int is_factorymode()
{
  char factorymode_falg[8]={0};
  int ret = 0;

	if ( ext4_read_content(1,factory_partition,"/factorymode.file",factorymode_falg,0,8))
		return 0;
	debugf("Checking factorymode :  factorymode_falg = %s \n", factorymode_falg);
	if(!strcmp(factorymode_falg, "1"))
		ret = 1;
	else
		ret = 0;
	debugf("Checking factorymode :  ret = %d \n", ret);
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

#ifdef CONFIG_SP8830WCN
	/* wcnfixnv=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " wcnfixnv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", WCNFIXNV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", FIXNV_SIZE);

	/* wcnruntimenv=0x????????,0x????????*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " wcnruntimenv=0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%08x", WCNRUNTIMENV_ADR);
	str_len = strlen(buf);
	sprintf(&buf[str_len], ",0x");
	str_len = strlen(buf);
	sprintf(&buf[str_len], "%x", RUNTIMENV_SIZE);
#endif

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

	if(product_SN_flag ==1)
	{
		str_len = strlen(buf);
		sprintf(&buf[str_len], " androidboot.serialno=%s", product_SN);
	}

#if BOOT_NATIVE_LINUX_MODEM
	str_len = strlen(buf);
	buf[str_len] = '\0';
	char* nv_infor = (char*)(((volatile u32*)CALIBRATION_FLAG));
	sprintf(nv_infor, buf);
	nv_infor[str_len] = '\0';
	debugf("nv_infor:[%08x]%s \n", nv_infor, nv_infor);
#if defined (CONFIG_SC8830)
	nv_infor = (char*)(((volatile u32*)CALIBRATION_FLAG_WCDMA));
	sprintf(nv_infor, buf);
	nv_infor[str_len] = '\0';
	debugf("nv_infor:[%08x]%s \n", nv_infor, nv_infor);
#endif
#endif
}

int read_spldata()
{
	int size = CONFIG_SPL_LOAD_LEN;
	if(TRUE !=  Emmc_Read(PARTITION_BOOT1, 0, size/EMMC_SECTOR_SIZE, (uint8*)spl_data)){
		debugf("vmjaluna nand read error \n");
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

#if defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB)
	u32 cp1data[3] = {0xe59f0000, 0xe12fff10, TDMODEM_ADR};

	memcpy(0x50001800, cp1data, sizeof(cp1data));	   /* copy cp1 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000002;	   /* reset cp1 */
	*((volatile u32*)0x402B0050) &= ~0x02000000;	   /* clear cp1 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00BC);
		if (!(state & (0xf<<16)))
			break;
	}
	*((volatile u32*)0x402B0050) &= ~0x10000000;	   /* clear cp1 force deep sleep */
#ifdef CONFIG_SP8830WCN
	u32 cp2data[3] = {0xe59f0000, 0xe12fff10, WCNMODEM_ADR};
	memcpy(0x50003000, cp2data, sizeof(cp2data));	   /* copy cp2 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000004;	   /* reset cp2 */
	*((volatile u32*)0x402B0054) &= ~0x02000000;	   /* clear cp2 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00C0);
		if (!(state & (0xf<<16)))
		break;
	}
	*((volatile u32*)0x402B0060) &= ~0x12000000;	   /*system force shutdown deep_sleep*/
	*((volatile u32*)0x402B00A8) &= ~0x00000006;       /* clear reset cp0 cp1 cp2 */
#else
	*((volatile u32*)0x402B00A8) &= ~0x00000002;	   /* clear reset cp0 cp1 */
#endif
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SP5735)
	u32 cp0data[3] = {0xe59f0000, 0xe12fff10, WMODEM_ADR};

	memcpy(0x50000000, cp0data, sizeof(cp0data));	   /* copy cp0 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000001;	   /* reset cp0 */
	*((volatile u32*)0x402B003C) &= ~0x02000000;	   /* clear cp0 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00B8);
		if (!(state & (0xf<<28)))
			break;
	}
	*((volatile u32*)0x402B003C) &= ~0x10000000;	   /* clear cp0 force deep sleep */
#ifdef CONFIG_SP8830WCN
	u32 cp2data[3] = {0xe59f0000, 0xe12fff10, WCNMODEM_ADR};
	memcpy(0x50003000, cp2data, sizeof(cp2data));	   /* copy cp2 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000004;	   /* reset cp2 */
	*((volatile u32*)0x402B0054) &= ~0x02000000;	   /* clear cp2 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00C0);
		if (!(state & (0xf<<16)))
		break;
	}
	*((volatile u32*)0x402B0060) &= ~0x12000000;	   /*system force shutdown deep_sleep*/
	*((volatile u32*)0x402B00A8) &= ~0x00000005;       /* clear reset cp0 cp1 cp2 */
#else	
	*((volatile u32*)0x402B00A8) &= ~0x00000001;	   /* clear reset cp0 cp1 */
#endif

#else
#ifndef CONFIG_NOT_BOOT_W_MODEM
{
	u32 cp0data[3] = {0xe59f0000, 0xe12fff10, WMODEM_ADR};
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
}
#endif

#ifndef CONFIG_NOT_BOOT_TD_MODEM
{
	u32 cp1data[3] = {0xe59f0000, 0xe12fff10, TDMODEM_ADR};
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
}
#endif

#ifdef CONFIG_SP8830WCN
	u32 cp2data[3] = {0xe59f0000, 0xe12fff10, WCNMODEM_ADR};
	memcpy(0x50003000, cp2data, sizeof(cp2data));	   /* copy cp2 source code */
	*((volatile u32*)0x402B00A8) |=  0x00000004;	   /* reset cp2 */
	*((volatile u32*)0x402B0054) &= ~0x02000000;	   /* clear cp2 force shutdown */
	while(1)
	{
		state = *((volatile u32*)0x402B00C0);
		if (!(state & (0xf<<16)))
		break;
	}
	*((volatile u32*)0x402B0060) &= ~0x12000000;	   /*system force shutdown deep_sleep*/
	*((volatile u32*)0x402B00A8) &= ~0x00000007;       /* clear reset cp0 cp1 cp2 */
#else
	*((volatile u32*)0x402B00A8) &= ~0x00000003;       /* clear reset cp0 cp1 */
#endif	
#endif	
#endif
}

void sipc_addr_reset()
{
#ifdef CONFIG_SC8825
	memset((void *)SIPC_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#elif defined (CONFIG_SC8830)
#if defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB)
	memset((void *)SIPC_TD_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SP5735)
	memset((void *)SIPC_WCDMA_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#else
#ifndef CONFIG_NOT_BOOT_TD_MODEM
	memset((void *)SIPC_TD_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#endif
#ifndef CONFIG_NOT_BOOT_W_MODEM
	memset((void *)SIPC_WCDMA_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#endif
#endif
#ifdef CONFIG_SP8830WCN
	memset((void *)SIPC_WCN_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#endif
#endif
}

#endif

/**
	just convert partition name wchar to char with violent.
*/
LOCAL __inline char* w2c(wchar_t *wchar)
{
	static char buf[72]={0};
	unsigned int i=0;
	while((NULL != wchar[i]) && (i<72))
	{
		buf[i] = wchar[i]&0xFF;
		i++;
	}
	buf[i]=0;

	return buf;
}

LOCAL void _boot_secure_check(void)
{
#ifdef SECURE_BOOT_ENABLE
	secure_check(DSP_ADR, 0, DSP_ADR + DSP_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
	secure_check(MODEM_ADR, 0, MODEM_ADR + MODEM_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);

#if !BOOT_NATIVE_LINUX
	secure_check(VMJALUNA_ADR, 0, VMJALUNA_ADR + VMJALUNA_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif

#ifdef CONFIG_SIMLOCK
	secure_check(SIMLOCK_ADR, 0, SIMLOCK_ADR + SIMLOCK_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif
#endif
	return;
}

/**
	Function for reading user partition.
*/
PUBLIC int _boot_partition_read(block_dev_desc_t *dev, wchar_t* partition_name, u32 offsetsector, u32 size, u8* buf)
{
	disk_partition_t info;

	if(NULL == buf){
		debugf("%s:buf is NULL!\n", __FUNCTION__);
		return 0;
	}
	size = (size +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	size = size/EMMC_SECTOR_SIZE;
	if(0 == get_partition_info_by_name(dev, partition_name, &info))
	{
		if(TRUE != Emmc_Read(PARTITION_USER, info.start+offsetsector, size, buf))
		{
			debugf("%s: partition:%s read error!\n", __FUNCTION__,w2c(partition_name));
			return 0;
		}
	}
	else
	{
		debugf("%s: partition:%s >>>get partition info failed!\n", __FUNCTION__,w2c(partition_name));
		return 0;
	}
	debugf("%s: partition:%s read success!\n", __FUNCTION__,w2c(partition_name));
	return 1;
}

/**
	Function for writing user partition.
*/
PUBLIC int _boot_partition_write(block_dev_desc_t *dev, wchar_t* partition_name, u32 size, u8* buf)
{
	disk_partition_t info;

	if(NULL == buf){
		debugf("%s:buf is NULL!\n", __FUNCTION__);
		return 0;
	}
	size = (size +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	size = size/EMMC_SECTOR_SIZE;
	if(0 == get_partition_info_by_name(dev, partition_name, &info))
	{
		if(TRUE != Emmc_Write(PARTITION_USER, info.start, size, buf))
		{
			debugf("%s: partition:%s read error!\n", __FUNCTION__,w2c(partition_name));
			return 0;
		}
	}
	else
	{
		debugf("%s: partition:%s >>>get partition info failed!\n", __FUNCTION__,w2c(partition_name));
		return 0;
	}
	debugf("%s: partition:%s write success!\n", __FUNCTION__,w2c(partition_name));
	return 1;
}

/**
	Function for displaying logo.
*/
LOCAL __inline void _boot_display_logo(block_dev_desc_t *dev, int backlight_set)
{
	size_t size;

#if defined(CONFIG_LCD_720P) || defined(CONFIG_LCD_HD) //LiWei add CONFIG_LCD_HD
	size = 1<<20;
#else
	size = 1<<19;
#endif
	uint8 * bmp_img = malloc(size);
	if(!bmp_img){
	    debugf("%s: malloc for splash image failed!\n",__FUNCTION__);
	    return;
	}
	if(!_boot_partition_read(dev, L"logo", 0, size, bmp_img)) 
	{
		debugf("%s: read logo partition failed!\n",__FUNCTION__);
		goto end;
	}
	lcd_display_logo(backlight_set,(ulong)bmp_img,size);
end:
	free(bmp_img);
	return;
}

LOCAL BOOLEAN _chkNVEcc(uint8* buf, uint32 size,uint32 checksum)
{
	uint16 crc;

	crc = calc_checksum(buf,size);
	debugf("_chkNVEcc crc 0x%x\n",crc);
	return (crc == (uint16)checksum);
}

/**
	we assume partition with backup must check ecc.
*/
LOCAL __inline int _boot_read_partition_with_backup(block_dev_desc_t *dev, boot_image_required_t info)
{
	uint8 *bakbuf = NULL;
	uint8 *oribuf = NULL;
	u8 status=0;
	uint8 header[EMMC_SECTOR_SIZE];
	uint32 checksum = 0;//,magic = 0;
	nv_header_t * header_p = NULL;

	header_p = header;
	bakbuf = malloc(info.size+EMMC_SECTOR_SIZE);
	if(NULL != bakbuf)
		memset(bakbuf, 0xff, info.size+EMMC_SECTOR_SIZE);
	oribuf = malloc(info.size+EMMC_SECTOR_SIZE);
	if(NULL != oribuf)
		memset(oribuf,0xff,info.size+EMMC_SECTOR_SIZE);

	if(_boot_partition_read(dev, info.partition, 0, info.size+EMMC_SECTOR_SIZE, oribuf)){
		memset(header,0,EMMC_SECTOR_SIZE);
		memcpy(header,oribuf,EMMC_SECTOR_SIZE);
		checksum = header_p->checksum;
		debugf("_boot_read_partition_with_backup origin checksum 0x%x\n",checksum);
		if(_chkNVEcc(oribuf+EMMC_SECTOR_SIZE,info.size,checksum)){
			memcpy(info.mem_addr,oribuf+EMMC_SECTOR_SIZE,info.size);
			status += 1;
		}
	}
	if(_boot_partition_read(dev, info.bak_partition, 0, info.size+EMMC_SECTOR_SIZE, bakbuf)){
		memset(header,0,EMMC_SECTOR_SIZE);
		memcpy(header,bakbuf,EMMC_SECTOR_SIZE);
		checksum = header_p->checksum;
		debugf("_boot_read_partition_with_backup backup checksum 0x%x\n",checksum);
		if(_chkNVEcc(bakbuf+EMMC_SECTOR_SIZE, info.size,checksum))
			status += 1<<1;
	}

	switch(status){
		case 0:
			debugf("%s:(%s)both org and bak partition are damaged!\n",__FUNCTION__,w2c(info.partition));
			free(bakbuf);
			free(oribuf);
			return 0;
		case 1:
			debugf("%s:(%s)bak partition is damaged!\n",__FUNCTION__,w2c(info.bak_partition));
			_boot_partition_write(dev, info.bak_partition, info.size+EMMC_SECTOR_SIZE,oribuf);
			break;
		case 2:
			debugf("%s:(%s)org partition is damaged!\n!",__FUNCTION__,w2c(info.partition));
			memcpy(info.mem_addr,bakbuf+EMMC_SECTOR_SIZE,info.size);
			_boot_partition_write(dev, info.partition, info.size+EMMC_SECTOR_SIZE, bakbuf);
			break;
		case 3:
			debugf("%s:(%s)both org and bak partition are ok!\n",__FUNCTION__,w2c(info.partition));
			break;
		default:
			debugf("%s: status error!\n",__FUNCTION__);
			free(bakbuf);
			free(oribuf);
			return 0;
	}
	free(bakbuf);
	free(oribuf);
	return 1;
}

/**
	Function for reading image which is needed when power on.
*/
LOCAL __inline int _boot_load_required_image(block_dev_desc_t *dev, boot_image_required_t img_info)
{
	debugf("%s: load %s to addr 0x%08x\n",__FUNCTION__,w2c(img_info.partition),img_info.mem_addr);

	if(NULL != img_info.bak_partition)
	{
		_boot_read_partition_with_backup(dev, img_info);
	}
	else
	{
		_boot_partition_read(dev, img_info.partition, 0, img_info.size, (u8*)img_info.mem_addr);
	}

	return 1;
}

/**
	Function for checking and loading kernel/ramdisk image.
*/
LOCAL int _boot_load_kernel_ramdisk_image(block_dev_desc_t *dev, char* bootmode,boot_img_hdr* hdr)
{
	wchar_t* partition = NULL;
	uint32 size,offset;
	
	if(0 == memcmp(bootmode, RECOVERY_PART, strlen(RECOVERY_PART))){
		partition = L"recovery";
		debugf("enter recovery mode!\n");
	}else{
		partition = L"boot";
		debugf("enter boot mode!\n");
	}

	if(!_boot_partition_read(dev, partition, 0, 4*EMMC_SECTOR_SIZE, (u8*) hdr)){
		debugf("%s:%s read error!\n",__FUNCTION__,w2c(partition));
		return 0;
	}
	//image header check
	if(0 != memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		debugf("bad boot image header, give up boot!!!!\n");
		return 0;
	}

	//read kernel image
	offset = 4;
	size = (hdr->kernel_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
	if(size <=0){
		debugf("kernel image should not be zero!\n");
		return 0;
	}
	if(!_boot_partition_read(dev, partition, offset, size, (u8*) KERNEL_ADR)){
		debugf("%s:%s kernel read error!\n",__FUNCTION__,w2c(partition));
		return 0;
	}

	//read ramdisk image
	offset += size/512;
	offset = ((offset+3)/4)*4;
	size = (hdr->ramdisk_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
	if(size<0){
		debugf("ramdisk size error\n");
		return 0;
	}
	if(!_boot_partition_read(dev, partition, offset, size, (u8*) RAMDISK_ADR)){
		debugf("%s:ramdisk read error!\n",__FUNCTION__);
		return 0;
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
	return 1;
}

void vlx_nand_boot(char * kernel_pname, char * cmdline, int backlight_set)
{
	boot_img_hdr *hdr = (void *)raw_header;
	block_dev_desc_t *dev = NULL;
	char * buf = NULL;
	char * mode_ptr = NULL;
	int i;

#if (defined CONFIG_SC8810) || (defined CONFIG_SC8825) || (defined CONFIG_SC8830)
	MMU_Init(CONFIG_MMU_TABLE_ADDR);
#endif

	dev = get_dev("mmc", 1);
	if(NULL == dev){
		debugf("Fatal Error,get_dev mmc failed!\n");
		return;
	}

#ifdef CONFIG_SPLASH_SCREEN
	_boot_display_logo(dev, backlight_set);
#endif
	set_vibrator(FALSE);

#if((!BOOT_NATIVE_LINUX)||(BOOT_NATIVE_LINUX_MODEM))
	//load required image which config in table
	for(i=0;s_boot_image_table[i].partition != NULL;i++)
	{
		_boot_load_required_image(dev,s_boot_image_table[i]);
	}
#endif
	//loader kernel and ramdisk
	if(!_boot_load_kernel_ramdisk_image(dev, kernel_pname, hdr))
		return;
	//secure check for secure boot
	_boot_secure_check();
	product_SN_get();
	buf = creat_cmdline(cmdline,hdr);
	if (buf != NULL) {
		free(buf);
	}

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

static void product_SN_get(void)
{
   SP09_PHASE_CHECK_T phase_check;
   block_dev_desc_t *p_block_dev = NULL;

	p_block_dev = get_dev("mmc", 1);
	if(NULL == p_block_dev){
		debugf("%s:  get_dev() error\n", __FUNCTION__);
		product_SN_flag =0;
		return;
	}

	if(-1 == Calibration_read_partition(p_block_dev, PRODUCTINFO_FILE_PATITION, (char *)&phase_check,sizeof(phase_check))){
		debugf("%s:  read miscdata error\n", __FUNCTION__);
		product_SN_flag =0;
		return ;
	}
	debugf("%s: phase_check.Magic = %d \n", __FUNCTION__, phase_check.Magic);
	if(phase_check.Magic == SP09_SPPH_MAGIC){
		product_SN_flag =1;
		memcpy(product_SN, phase_check.SN1, 21);
	}else{
		product_SN_flag =0;
	}
}
