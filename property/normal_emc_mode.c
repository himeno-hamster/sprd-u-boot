#include <config.h>
#include "normal_mode.h"

#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>

#ifdef CONFIG_OF_LIBFDT
#include "dev_tree.h"
#endif

#define KERNL_PAGE_SIZE 2048

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
#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP5735) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SPX15)

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

int read_spldata()
{
	int size = CONFIG_SPL_LOAD_LEN;
	if(TRUE !=  Emmc_Read(PARTITION_BOOT1, 0, size/EMMC_SECTOR_SIZE, (uint8*)spl_data)){
		debugf("vmjaluna nand read error \n");
		return -1;
	}
	return 0;
}


LOCAL BOOLEAN _chkNVEcc(uint8* buf, uint32 size,uint32 checksum)
{
	uint16 crc;

	crc = calc_checksum(buf,size);
	debugf("_chkNVEcc crc 0x%x\n",crc);
	return (crc == (uint16)checksum);
}

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
	int ret =0;
	u32 left;
	u32 nsct;
	char *sctbuf=NULL;
	disk_partition_t info;

	if(NULL == buf){
		debugf("%s:buf is NULL!\n", __func__);
		goto end;
	}
	nsct = size/EMMC_SECTOR_SIZE;
	left = size%EMMC_SECTOR_SIZE;

	if(get_partition_info_by_name(dev, partition_name, &info)){
		debugf("get partition %s info failed!\n", w2c(partition_name));
		goto end;
	}

	if(TRUE != Emmc_Read(PARTITION_USER, info.start+offsetsector, nsct, buf))
		goto end;

	if(left){
		sctbuf = malloc(EMMC_SECTOR_SIZE);
		if(NULL != sctbuf){
			if(TRUE == Emmc_Read(PARTITION_USER, info.start+offsetsector+nsct, 1, sctbuf)){
				memcpy(buf+(nsct*EMMC_SECTOR_SIZE), sctbuf, left);
				ret = 1;
			}
			free(sctbuf);
		}
	}else{
		ret = 1;
	}

end:
	debugf("%s: partition %s read %s!\n", __func__,w2c(partition_name),ret?"success":"failed");
	return ret;
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

/**
	we assume partition with backup must check ecc.
*/
LOCAL __inline int _boot_read_partition_with_backup(block_dev_desc_t *dev, boot_image_required_t info)
{
	uint8 *bakbuf = NULL;
	uint8 *oribuf = NULL;
	u8 status=0;
	uint8 header[EMMC_SECTOR_SIZE];
	uint32 checksum = 0;
	nv_header_t * header_p = NULL;
	uint32 bufsize = info.size+EMMC_SECTOR_SIZE;

	header_p = header;
	bakbuf = malloc(bufsize);
	if(NULL == bakbuf)
		return 0;
	memset(bakbuf, 0xff, bufsize);
	oribuf = malloc(bufsize);
	if(NULL == oribuf){
		free(bakbuf);
		return 0;
	}
	memset(oribuf, 0xff, bufsize);

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
	uint32 dt_img_adr;
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
#ifdef CONFIG_OF_LIBFDT
	//read dt image
	offset += size/512;
	offset = ((offset+3)/4)*4;
	size = (hdr->dt_size+(KERNL_PAGE_SIZE - 1)) & (~(KERNL_PAGE_SIZE - 1));
	dt_img_adr = RAMDISK_ADR - size - KERNL_PAGE_SIZE;
	if(size<0){
		debugf("dt size error\n");
		return 0;
	}
	if(!_boot_partition_read(dev, partition, offset, size, (u8*)dt_img_adr)){
		debugf("%s:dt read error!\n",__FUNCTION__);
		return 0;
	}
	if (load_dtb((int)DT_ADR,(void*)dt_img_adr)){
		debugf("%s:dt load error!\n",__FUNCTION__);
		return 0;
	}
#endif
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

#if ((!BOOT_NATIVE_LINUX)||(BOOT_NATIVE_LINUX_MODEM))
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

