#include <config.h>
#include "normal_mode.h"

#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>

#define KERNL_PAGE_SIZE 2048

#ifdef CONFIG_FS_EXT4
int normal_emc_partition = PARTITION_PROD_INFO3;
static char     	product_SN1[20+1];
static char     	product_SN2[20+1];
static int		product_SN_flag = 0;
#endif

static boot_image_required_t const s_boot_image_table[]={
#ifdef CONFIG_SC8830

#if defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB)
	{L"tdfixnv1",L"tdfixnv2",FIXNV_SIZE,TDFIXNV_ADR},
	{L"tdruntimenv1",L"tdruntimenv2",RUNTIMENV_SIZE,TDRUNTIMENV_ADR},
	{L"prodinfo1",L"prodinfo2",PRODUCTINFO_SIZE,TDPRODINFO_ADR},
	{L"tdmodem",NULL,MODEM_SIZE,TDMODEM_ADR},
	{L"tddsp",NULL,DSP_SIZE,TDDSP_ADR}, 
#ifdef CONFIG_SP8830WCN
	{L"wcnfixnv1",L"wcnfixnv2",FIXNV_SIZE,WCNFIXNV_ADR},
	{L"wcnruntimenv1",L"wcnruntimenv2",RUNTIMENV_SIZE,WCNRUNTIMENV_ADR},
	{L"wcnmodem",NULL,MODEM_SIZE,WCNMODEM_ADR},
#endif		
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC)
	{L"wfixnv1",L"wfixnv2",FIXNV_SIZE,WFIXNV_ADR},
	{L"wruntimenv1",L"wruntimenv2",RUNTIMENV_SIZE,WRUNTIMENV_ADR},
	{L"prodinfo1",L"prodinfo2",PRODUCTINFO_SIZE,WPRODINFO_ADR},
	{L"wmodem",NULL,MODEM_SIZE,WMODEM_ADR},
	{L"wdsp",NULL,DSP_SIZE,WDSP_ADR},
#ifdef CONFIG_SP8830WCN
	{L"wcnfixnv1",L"wcnfixnv2",FIXNV_SIZE,WCNFIXNV_ADR},
	{L"wcnruntimenv1",L"wcnruntimenv2",RUNTIMENV_SIZE,WCNRUNTIMENV_ADR},
	{L"wcnmodem",NULL,MODEM_SIZE,WCNMODEM_ADR},
#endif	
#else
	{L"tdfixnv1",L"tdfixnv2",FIXNV_SIZE,TDFIXNV_ADR},
	{L"tdruntimenv1",L"tdruntimenv2",RUNTIMENV_SIZE,TDRUNTIMENV_ADR},
	{L"wfixnv1",L"wfixnv2",FIXNV_SIZE,WFIXNV_ADR},
	{L"wruntimenv1",L"wruntimenv2",RUNTIMENV_SIZE,WRUNTIMENV_ADR},
	{L"prodinfo1",L"prodinfo2",PRODUCTINFO_SIZE,TDPRODINFO_ADR},
	{L"prodinfo1",L"prodinfo2",PRODUCTINFO_SIZE,WPRODINFO_ADR},
	{L"tdmodem",NULL,MODEM_SIZE,TDMODEM_ADR},
	{L"tddsp",NULL,DSP_SIZE,TDDSP_ADR},		
	{L"wmodem",NULL,MODEM_SIZE,WMODEM_ADR},
	{L"wdsp",NULL,DSP_SIZE,WDSP_ADR},
#endif	

#else
	{L"fixnv1",L"fixnv2",FIXNV_SIZE,FIXNV_ADR},
	{L"runtimenv1",L"runtimenv2",RUNTIMENV_SIZE,RUNTIMENV_ADR},
	{L"prodinfo1",L"prodinfo2",PRODUCTINFO_SIZE,PRODUCTINFO_ADR},
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

#ifdef CONFIG_FS_EXT4
static void product_SN_get(void);
#endif

int get_partition_info (block_dev_desc_t *dev_desc, int part, disk_partition_t *info);


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
	if(NULL == p_block_dev)
	{
		printf("\nCan't get mmc device,get_dev return error!\n");
		return ret;
	}

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

#ifdef CONFIG_FS_EXT4
	if(product_SN_flag ==1)
	{
		str_len = strlen(buf);
		sprintf(&buf[str_len], " SN1=%s", product_SN1);
		str_len = strlen(buf);
		sprintf(&buf[str_len], " SN2=%s", product_SN2);
	}
#endif

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
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC)
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
	u32 cp1data[3] = {0xe59f0000, 0xe12fff10, TDMODEM_ADR};
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
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC)
	memset((void *)SIPC_WCDMA_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
#else
	memset((void *)SIPC_TD_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
	memset((void *)SIPC_WCDMA_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);
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
LOCAL __inline int _boot_partition_read(block_dev_desc_t *dev, wchar_t* partition_name, u32 offsetsector, u32 size, u8* buf)
{
	disk_partition_t info;

	if(NULL == buf){
		printf("%s:buf is NULL!\n", __FUNCTION__);
		return 0;
	}
	size = (size +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	size = size/EMMC_SECTOR_SIZE;
	if(0 == get_partition_info_by_name(dev, partition_name, &info))
	{
		if(TRUE != Emmc_Read(PARTITION_USER, info.start+offsetsector, size, buf))
		{
			printf("%s: partition:%s read error!\n", __FUNCTION__,w2c(partition_name));
			return 0;
		}
	}
	else
	{
		printf("%s: partition:%s >>>get partition info failed!\n", __FUNCTION__,w2c(partition_name));
		return 0;
	}
	printf("%s: partition:%s read success!\n", __FUNCTION__,w2c(partition_name));
	return 1;
}

/**
	Function for reading user partition.
*/
LOCAL __inline int _boot_partition_write(block_dev_desc_t *dev, wchar_t* partition_name, u32 size, u8* buf)
{
	disk_partition_t info;

	if(NULL == buf){
		printf("%s:buf is NULL!\n", __FUNCTION__);
		return 0;
	}
	size = (size +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1));
	size = size/EMMC_SECTOR_SIZE;
	if(0 == get_partition_info_by_name(dev, partition_name, &info))
	{
		if(TRUE != Emmc_Write(PARTITION_USER, info.start, size, buf))
		{
			printf("%s: partition:%s read error!\n", __FUNCTION__,w2c(partition_name));
			return 0;
		}
	}
	else
	{
		printf("%s: partition:%s >>>get partition info failed!\n", __FUNCTION__,w2c(partition_name));
		return 0;
	}
	printf("%s: partition:%s write success!\n", __FUNCTION__,w2c(partition_name));
	return 1;
}

/**
	Function for displaying logo.
*/
LOCAL __inline void _boot_display_logo(block_dev_desc_t *dev, int backlight_set)
{
	size_t size;

#ifdef CONFIG_LCD_720P
	size = 1<<20;
#else
	size = 1<<19;
#endif
	uint8 * bmp_img = malloc(size);
	if(!bmp_img){
	    printf("%s: malloc for splash image failed!\n",__FUNCTION__);
	    return;
	}
	if(!_boot_partition_read(dev, L"logo", 0, size, bmp_img)) 
	{
		printf("%s: read logo partition failed!\n",__FUNCTION__);
		goto end;
	}
	lcd_display_logo(backlight_set,(ulong)bmp_img,size);
end:
	free(bmp_img);
	return;
}

/**
	we assume partition with backup must check ecc.
*/
LOCAL __inline int _boot_read_partition_with_backup(block_dev_desc_t *dev, boot_image_required_t info)
{
	uint8 *bakbuf = NULL;
	u8 status=0;

	if(_boot_partition_read(dev, info.partition, 0, info.size, (u8*)info.mem_addr))
		if(chkEcc((u8*)info.mem_addr, info.size))
			status += 1;

	bakbuf = malloc(info.size+EMMC_SECTOR_SIZE);
	if(NULL != bakbuf)
		memset(bakbuf, 0xff, info.size+EMMC_SECTOR_SIZE);
	if(_boot_partition_read(dev, info.bak_partition, 0, info.size, bakbuf))
		if(chkEcc(bakbuf, info.size))
			status += 1<<1;

	switch(status){
		case 0:
			printf("%s:(%s)both org and bak partition are damaged!\n",__FUNCTION__,w2c(info.partition));
			return 0;
		case 1:
			printf("%s:(%s)bak partition is damaged!\n",__FUNCTION__,w2c(info.bak_partition));
			_boot_partition_write(dev, info.bak_partition, info.size, (u8*)info.mem_addr);
			break;
		case 2:
			printf("%s:(%s)org partition is damaged!\n!",__FUNCTION__,w2c(info.partition));
			_boot_partition_write(dev, info.partition, info.size, bakbuf);
			break;
		case 3:
			printf("%s:(%s)both org and bak partition are ok!\n",__FUNCTION__,w2c(info.partition));
			break;
		default:
			printf("%s: status error!\n",__FUNCTION__);
			return 0;
	}

	return 1;
}

/**
	Function for reading image which is needed when power on.
*/
LOCAL __inline int _boot_load_required_image(block_dev_desc_t *dev, boot_image_required_t img_info)
{
	printf("%s: load %s to addr 0x%08x\n",__FUNCTION__,w2c(img_info.partition),img_info.mem_addr);

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
		printf("enter recovery mode!\n");
	}else{
		partition = L"kernel";
		printf("enter boot mode!\n");
	}

	if(!_boot_partition_read(dev, partition, 0, 4*EMMC_SECTOR_SIZE, (u8*) hdr)){
		printf("%s:%s read error!\n",__FUNCTION__,w2c(partition));
		return 0;
	}
	//image header check
	if(0 != memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		printf("bad boot image header, give up boot!!!!\n");
		return 0;
	}

	//read kernel image
	offset = 4;
	size = (hdr->kernel_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
	if(size <=0){
		printf("kernel image should not be zero!\n");
		return 0;
	}
	if(!_boot_partition_read(dev, partition, offset, size, (u8*) KERNEL_ADR)){
		printf("%s:%s kernel read error!\n",__FUNCTION__,w2c(partition));
		return 0;
	}

	//read ramdisk image
	offset += size/512;
	offset = ((offset+3)/4)*4;
	size = (hdr->ramdisk_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
	if(size<0){
		printf("ramdisk size error\n");
		return 0;
	}
	if(!_boot_partition_read(dev, partition, offset, size, (u8*) RAMDISK_ADR)){
		printf("%s:ramdisk read error!\n",__FUNCTION__);
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
	int i;

#if (defined CONFIG_SC8810) || (defined CONFIG_SC8825) || (defined CONFIG_SC8830)
	MMU_Init(CONFIG_MMU_TABLE_ADDR);
#endif

	dev = get_dev("mmc", 1);
	if(NULL == dev){
		printf("Fatal Error,get_dev mmc failed!\n");
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

#ifdef CONFIG_FS_EXT4
	product_SN_get();
#endif

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

#ifdef CONFIG_FS_EXT4
static void product_SN_get(void)
{
	SP09_PHASE_CHECK_T phase_check;

		if(!ext4_read_content(1,normal_emc_partition,"/productinfo.bin", &phase_check, 0, sizeof(phase_check)))
		{
			product_SN_flag =1;
			memcpy(product_SN1, phase_check.SN1, 21);
			memcpy(product_SN2, phase_check.SN2, 21);
		}
		else{
			product_SN_flag =0;
			printf("%s open fail  /productinfo/productinfo.bin ",__FUNCTION__);
		}
}
#endif