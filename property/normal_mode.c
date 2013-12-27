#include "normal_mode.h"
#include <mmc.h>
#include <fat.h>
#if defined(CONFIG_OF_LIBFDT)
#include <libfdt.h>
#include <fdt_support.h>
#endif

#define FACTORY_PART "prodnv"
#define PRODUCTINFO_FILE_PATITION  "miscdata"

unsigned spl_data_buf[0x2000] __attribute__((align(4)))={0};
unsigned harsh_data_buf[8]__attribute__((align(4)))={0};
void *spl_data = spl_data_buf;
void *harsh_data = harsh_data_buf;
unsigned char raw_header[8192];
const int SP09_MAX_PHASE_BUFF_SIZE = sizeof(SP09_PHASE_CHECK_T);

extern void* lcd_base;

int eng_getphasecheck(SP09_PHASE_CHECK_T* phase_check)
{
	int aaa;
	unsigned long tested;

	if (phase_check->Magic == SP09_SPPH_MAGIC_NUMBER) {
		//printf("Magic = 0x%08x\n",phase_check->Magic);
		debugf("SN1 = %s   SN2 = %s\n",phase_check->SN1, phase_check->SN2);
		/*printf("StationNum = %d\n",phase_check->StationNum);
		printf("Reserved = %s\n",phase_check->Reserved);
		printf("SignFlag = 0x%02x\n",phase_check->SignFlag);
		printf("iTestSign = 0x%04x\n",phase_check->iTestSign);
		printf("iItem = 0x%04x\n",phase_check->iItem);*/
		if (phase_check->SignFlag == 1) {
			for (aaa = 0; aaa < phase_check->StationNum/*SP09_MAX_STATION_NUM*/; aaa ++) {
				debugf("%s : ", phase_check->StationName[aaa]);
				tested = 1 << aaa;
				if ((tested & phase_check->iTestSign) == 0) {
					if ((tested & phase_check->iItem) == 0)
						debugf("Pass; ");
					else
						debugf("Fail; ");
				} else
					debugf("UnTested; ");
			}
		} else {
			debugf("station status are all invalid!\n");
			for (aaa = 0; aaa < phase_check->StationNum/*SP09_MAX_STATION_NUM*/; aaa ++)
				debugf("%s  ", phase_check->StationName[aaa]);
		}
		debugf("\nLast error: %s\n",phase_check->szLastFailDescription);
	} else
		debugf("no production information / phase check!\n");

	return 0;
}

int eng_phasechecktest(unsigned char *array, int len)
{
	SP09_PHASE_CHECK_T phase;

	memset(&phase, 0, sizeof(SP09_PHASE_CHECK_T));
	memcpy(&phase, array, len);

	return eng_getphasecheck(&phase);
}

unsigned short calc_checksum(unsigned char *dat, unsigned long len)
{
        unsigned short num = 0;
        unsigned long chkSum = 0;
        while(len>1){
                num = (unsigned short)(*dat);
                dat++;
                num |= (((unsigned short)(*dat))<<8);
                dat++;
                chkSum += (unsigned long)num;
                len -= 2;
        }
        if(len){
                chkSum += *dat;
        }
        chkSum = (chkSum >> 16) + (chkSum & 0xffff);
        chkSum += (chkSum >> 16);
        return (~chkSum);
}

unsigned char _chkNVEcc(uint8_t* buf, uint32_t size,uint32_t checksum)
{
	uint16_t crc;

	crc = calc_checksum(buf,size);
	debugf("_chkNVEcc calcout 0x%lx, org 0x%llx\n",crc,checksum);
	return (crc == (uint16_t)checksum);
}

int chkEcc(unsigned char* buf, int size)
{
	unsigned short crc,crcOri;
//	crc = __crc_16_l_calc(buf, size-2);
//	crcOri = (uint16)((((uint16)buf[size-2])<<8) | ((uint16)buf[size-1]) );

	crc = calc_checksum(buf,size-4);
	crcOri = (unsigned short)((((unsigned short)buf[size-3])<<8) | ((unsigned short)buf[size-4]) );

	return (crc == crcOri);
}

#define NV_MULTI_LANG_ID   (405)
#define GSM_CALI_ITEM_ID   (0x2)
#define GSM_IMEI_ITEM_ID   (0x5)
#define XTD_CALI_ITEM_ID   (0x516)
#define LTE_CALI_ITEM_ID   (0x9C4)
#define BT_ITEM_ID         (0x191)

#define BT_ADDR_LEN  6

#define IMEI_LEN			(8)
#define GSM_CALI_VER_A      0xFF0A
#define GSM_CALI_VER_MIN    GSM_CALI_VER_A
#define GSM_CALI_VER_MAX    GSM_CALI_VER_A

#define NUM_TEMP_BANDS		(5)
#define NUM_RAMP_RANGES		(16)		/* constant parameter numbers, 16 level */
#define NUM_TX_LEVEL		(16)		/* 2db per step */
#define NUM_RAMP_POINTS		(20)
#define NUM_GSM_ARFCN_BANDS	(6)
#define NUM_DCS_ARFCN_BANDS	(8)
#define NUM_PCS_ARFCN_BANDS	(7)
#define NUM_GSM850_ARFCN_BANDS	(6)
#define MAX_COMPENSATE_POINT	(75)

static unsigned long XCheckNVStruct(unsigned char *lpPhoBuf, unsigned long dwPhoSize)
{
	unsigned long dwOffset = 0, dwLength = 0, bRet;
	unsigned char *lpCode = lpPhoBuf;
	unsigned long dwCodeSize = dwPhoSize;
	unsigned short wCurID;

	dwOffset = 4;     /* Skip first four bytes,that is time stamp */
    dwLength = 0;
    unsigned char *pTemp = lpCode + dwOffset;

	unsigned long bIMEI = 0;
	unsigned long bGSMCali = 0;
	unsigned short wGSMCaliVer = 0;
    while (dwOffset < dwCodeSize) {
	    wCurID = *(unsigned short *)pTemp;
        pTemp += 2;

        dwLength = *(unsigned short *)pTemp;
		/* printf("wCurID = 0x%08x  dwLength = 0x%08x\n", wCurID, dwLength); */
		if (wCurID == GSM_IMEI_ITEM_ID) {
			if (dwLength != IMEI_LEN) {
				return 0;
			} else {
				bIMEI = 1;
			}
		} else if (wCurID == GSM_CALI_ITEM_ID) {
			wGSMCaliVer =  *(unsigned short *)(pTemp + 2); /* pTemp + 2: skip length */
            /* printf("wGSMCaliVer = 0x%08x\n", wGSMCaliVer); */
			if ((wGSMCaliVer > GSM_CALI_VER_MAX) || (wGSMCaliVer < GSM_CALI_VER_MIN)) {
				return 0;
			} else {
				bGSMCali = 1;
			}
		}

		/* 0xFFFF is end-flag in module (NV in phone device) */
		if (wCurID == 0xFFFF) {
			if (!bIMEI || !bGSMCali) {
				return 0;
			}
			return 1;
		}

		if (wCurID == 0 || dwLength == 0) {
			break;
		}

        pTemp += 2;
        dwOffset += 4;
        /* Must be four byte aligned */
        bRet = dwLength % 4;
        if (bRet != 0)
                dwLength += 4 - bRet;
        dwOffset += dwLength;
        pTemp += dwLength;
        /* (dwOffset == dwCodeSize) is end condition in File */
		if (dwOffset == dwCodeSize) {
			if(!bIMEI || !bGSMCali) {
				return 0;
			}
			return 1;
		}
	}

	return 0;
}

unsigned long LogSwitch_Function(unsigned char *lpPhoBuf, unsigned long dwPhoSize)
{
	unsigned long dwOffset = 0, dwLength = 0, bRet;
	unsigned char *lpCode = lpPhoBuf;
	unsigned long dwCodeSize = dwPhoSize;
	unsigned short wCurID;
	unsigned long *timestamp;
	struct GSM_Download_Param_Tag *GSM_Download_Param;
	timestamp = lpPhoBuf;
	dwOffset = 4;     /* Skip first four bytes,that is time stamp */
	dwLength = 0;
	unsigned char *pTemp = lpCode + dwOffset;

	unsigned long bIMEI = 0;
	unsigned long bGSMCali = 0;
	unsigned short wGSMCaliVer = 0;
	while (dwOffset < dwCodeSize) {
		wCurID = *(unsigned short *)pTemp;
		pTemp += 2;

		dwLength = *(unsigned short *)pTemp;
		//printf("wCurID = %d  dwLength = 0x%08x\n", wCurID, dwLength);

		/* 0xFFFF is end-flag in module (NV in phone device) */
		if (wCurID == 0xFFFF) {
			if (!bIMEI || !bGSMCali) {
				return 0;
			}
			return 1;
		}

		if (wCurID == 0 || dwLength == 0) {
			break;
		}
		pTemp += 2;

		if (wCurID == 1) {
			debugf("pTemp = 0x%08x  dwLength = %d\n", pTemp, dwLength);
			GSM_Download_Param = (struct GSM_Download_Param_Tag *)pTemp;
			debugf("flag = %d sizeof = 0x%08x\n", GSM_Download_Param->log_switch_struct.DSP_log_switch.DSP_log_switch_value, sizeof(struct GSM_Download_Param_Tag));

			GSM_Download_Param->log_switch_struct.DSP_log_switch.DSP_log_switch_value = 0;
			debugf("flag = %d\n", GSM_Download_Param->log_switch_struct.DSP_log_switch);
			break;
		}
		dwOffset += 4;
		/* Must be four byte aligned */
		bRet = dwLength % 4;
		if (bRet != 0)
			dwLength += 4 - bRet;
		dwOffset += dwLength;
		pTemp += dwLength;
		/* (dwOffset == dwCodeSize) is end condition in File */
		if (dwOffset == dwCodeSize) {
			if(!bIMEI || !bGSMCali) {
				return 0;
			}
			return 1;
		}
	}

	return 0;
}

/*
* retval : -1 is wrong  ;  1 is correct
*/
int fixnv_is_correct(unsigned char *array, unsigned long size)
{
	unsigned short sum = 0, *dataaddr;

	if ((array[size - 4] == 0xff) && (array[size - 3] == 0xff) && (array[size - 2] == 0xff) \
		&& (array[size - 1] == 0xff)) {
		/* old version */
		if ((array[size] == 0x5a) && (array[size + 1] == 0x5a) && (array[size + 2] == 0x5a) \
			&& (array[size + 3] == 0x5a)) {
			/* check nv right or wrong */
			if (XCheckNVStruct(array, size) == 0) {
				debugf("NV data is crashed!!!.\n");
				return -1;
			} else {
				debugf("NV data is right!!!.\n");
				array[size] = 0xff; array[size + 1] = 0xff;
				array[size + 2] = 0xff; array[size + 3] = 0xff;
				return 1;
			}
		} else
			return -1;
	} else {
		/* new version */
		sum = calc_checksum(array, size - 4);
		dataaddr = (unsigned short *)(array + size - 4);

		if (*dataaddr == sum) {
			/* check nv right or wrong */
			if (XCheckNVStruct(array, size) == 0) {
				debugf("NV data is crashed!!!.\n");
				return -1;
			} else {
				debugf("NV data is right!!!.\n");
				array[size] = 0xff; array[size + 1] = 0xff;
				array[size + 2] = 0xff; array[size + 3] = 0xff;
				array[size - 4] = 0xff; array[size - 3] = 0xff;
				array[size - 2] = 0xff; array[size - 1] = 0xff;
				return 1;
			}
		} else {
			debugf("NV data crc error\n");
			return -1;
		}
	}
}

int fixnv_is_correct_endflag(unsigned char *array, unsigned long size)
{
	unsigned short sum = 0, *dataaddr;

	if ((array[size - 4] == 0xff) && (array[size - 3] == 0xff) && (array[size - 2] == 0xff) \
		&& (array[size - 1] == 0xff)) {
		/* old version */
		if ((array[size] == 0x5a) && (array[size + 1] == 0x5a) && (array[size + 2] == 0x5a) \
			&& (array[size + 3] == 0x5a)) {
			/* check nv right or wrong */
			if (XCheckNVStruct(array, size) == 0) {
				debugf("NV data is crashed!!!.\n");
				return -1;
			} else {
				debugf("NV data is right!!!.\n");
				return 1;
			}
		} else
			return -1;
	} else {
		/* new version */
		sum = calc_checksum(array, size - 4);
		dataaddr = (unsigned short *)(array + size - 4);

		if (*dataaddr == sum) {
			/* check nv right or wrong */
			if (XCheckNVStruct(array, size) == 0) {
				debugf("NV data is crashed!!!.\n");
				return -1;
			} else {
				debugf("NV data is right!!!.\n");
				return 1;
			}
		} else {
			debugf("NV data crc error\n");
			return -1;
		}
	}
}

unsigned long get_nv_index(unsigned char *array, unsigned long size)
{
	unsigned long index = 0;
	unsigned short sum = 0, *dataaddr;

	if ((array[FIXNV_SIZE - 4] == 0xff) && (array[FIXNV_SIZE - 3] == 0xff) && (array[FIXNV_SIZE - 2] == 0xff) \
		&& (array[FIXNV_SIZE - 1] == 0xff)) {
		/* old version */
		index = 1;
	} else {
		/* new version */
		dataaddr = (unsigned short *)(array + FIXNV_SIZE - 2);
		index = (unsigned long)(*dataaddr);
	}
	return index;
}

/* check runtimenv */
unsigned long check_npb(struct nv_dev dev, unsigned long size)
{
	struct npb_tag *backup_npb;
	unsigned long ret;

	if ((dev.npb->magic == NV_MAGIC) && (dev.npb->backup_npb)) {
		backup_npb = (struct npb_tag *)(dev.runtime + 1 * 512);
		ret = memcmp((unsigned char *)(dev.npb), (unsigned char *)backup_npb, size);
		if (ret == 0)
			return 1;
	}

	return 0;
}

unsigned long setup_devparam(struct nv_dev *dev)
{
	unsigned long dir_sects;

	dev->tot_size = dev->npb->tot_scts * dev->npb->sct_size;
	if ((dev->npb->max_id <= dev->npb->min_id) || (dev->npb->dir_entry_size != sizeof(struct direntry_tag)) \
		|| (dev->npb->dir_entry_count < (dev->npb->max_id - dev->npb->min_id + 1)))
		return 0;

	dev->first_dir_sct = dev->npb->backup_npb ? 2 : 1;
	dev->first_backup_dir_sct = dev->first_dir_sct;
	dir_sects = (dev->npb->dir_entry_count * dev->npb->dir_entry_size + dev->npb->sct_size - 1) / dev->npb->sct_size;

	if (dev->npb->backup_dir) {
		dev->first_backup_dir_sct += dir_sects;
		dir_sects *= 2;
	}

	dev->data_offset = (dev->first_dir_sct + dir_sects) * dev->npb->sct_size;

	if ((dev->npb->next_offset > dev->tot_size) || (dev->npb->next_offset < dev->data_offset) \
		|| (dev->data_offset >= dev->tot_size))
		return 0;

	return 1;
}

unsigned long check_dir_table(struct nv_dev *dev)
{
	unsigned long dir_sects, i, sct, backup_sct, ret;
	unsigned char *dir, *backup_dir;

	if (dev->npb->backup_npb == 0)
		return 1;
	dir_sects = (dev->npb->dir_entry_count * dev->npb->dir_entry_size + dev->npb->sct_size - 1) / dev->npb->sct_size;
	for (i = 0; i < dir_sects; i ++) {
		sct = dev->first_dir_sct + i;
		backup_sct = dev->first_backup_dir_sct + i;
		dir = dev->runtime + sct * dev->npb->sct_size;
		backup_dir = dev->runtime + backup_sct * dev->npb->sct_size;
		ret = memcmp(dir, backup_dir, dev->npb->sct_size);
		if (ret != 0) {
			debugf("sct = %d  backupsct = %d is diffrent\n", sct, backup_sct);
			return 0;
		}
	}

	return 1;
}

unsigned long check_dir_entry(struct nv_dev *dev, unsigned short id, struct direntry_tag *dir)
{
	if (0 == dir->status & STATUS_MASK) {
		if ((dir->offset == 0) && (dir->size == 0))
			return 1;
	} else if ((dir->offset >= dev->data_offset) && (dir->offset < dev->tot_size) && (dir->size <= dev->tot_size) \
			&& ((dir->offset + dir->size) <= dev->tot_size))
		return 1;

	return 0;
}

unsigned long read_dir(struct nv_dev *dev, unsigned short id, struct direntry_tag *dir)
{
	unsigned long addr, main_sct, backup_sct, off, ret;
	unsigned char *direntry;

	addr = (id - dev->npb->min_id) * sizeof(struct direntry_tag);
	main_sct = dev->first_dir_sct + addr / dev->npb->sct_size;
	backup_sct = dev->first_backup_dir_sct + addr / dev->npb->sct_size;
	off = addr % dev->npb->sct_size;
	if ((off + sizeof(struct direntry_tag)) > dev->npb->sct_size)
		return 0;

	direntry = dev->runtime + main_sct * dev->npb->sct_size + off;
	memcpy(dir, direntry, sizeof(struct direntry_tag));
	ret = check_dir_entry(dev, id, dir);
	/*printf("1id=%d offset=0x%08x ", id, main_sct * dev->npb->sct_size + off);
	printf("size=%d checksum=0x%04x offset=0x%08x status=0x%08x ret = %d\n", dir->size, dir->checksum, dir->offset, dir->status, ret);*/
	if (ret)
		return 1;

	direntry = dev->runtime + backup_sct * dev->npb->sct_size + off;
	memcpy(dir, direntry, sizeof(struct direntry_tag));
	ret = check_dir_entry(dev, id, dir);
	/*printf("2id=%d offset=0x%08x ", id, backup_sct * dev->npb->sct_size + off);
	printf("size = %d checksum=0x%04x offset=0x%08x status=0x%08x ret = %d\n", dir->size, dir->checksum, dir->offset, dir->status, ret);*/

	return ret;
}

void read_itemhdr(struct nv_dev *dev, unsigned long offset, struct item_tag *header)
{
	unsigned char *addr;

	addr = (unsigned char *)(dev->runtime + offset);
	memcpy(header, addr, sizeof(struct item_tag));
}

void read_itemdata(struct nv_dev *dev, unsigned long offset, unsigned long size, unsigned char *buffer)
{
	unsigned char *addr;

	addr = (unsigned char *)(dev->runtime + offset + sizeof(struct item_tag));
	memcpy(buffer, addr, size);
}


unsigned long check_items(struct nv_dev *dev)
{
	unsigned short id, checksum;
	struct direntry_tag dir;
	struct item_tag header;
	unsigned long bufsize = 64 * 1024;
	unsigned char buf[bufsize];

	for (id = dev->npb->min_id; id < dev->npb->max_id; id ++) {
		if (read_dir(dev, id, &dir) == 0)
			continue;
		if ((dir.status & STATUS_MASK) == 0)
			continue;

		read_itemhdr(dev, dir.offset, &header);
		if ((dir.size != header.size) || (header.id != id)) {
			debugf("item header is corrupted id = %d headerid = %d direntry.size = %d header.size = %d\n", id, header.id, dir.size, header.size);
		}

		if (dir.size > bufsize) {
			debugf("item size is too large : %d\n", dir.size);
			continue;
		}

		read_itemdata(dev, dir.offset, dir.size, buf);
		checksum = calc_checksum(buf, dir.size);
		if (checksum != dir.checksum) {
			debugf("item data is corrupted id = %d orgsum = 0x%04x checksum = 0x%04x\n", id, dir.checksum, checksum);
			return 0;
		}
	}

	return 1;
}

unsigned long XCheckRunningNVStruct(unsigned char *lpPhoBuf, unsigned long dwPhoSize)
{
	struct npb_tag *npb;
	struct nv_dev dev;
	unsigned long ret;

	memset(&dev, 0, sizeof(struct nv_dev));
	npb = (struct npb_tag *)(lpPhoBuf + 0 * 512);
	dev.runtime = lpPhoBuf;
	dev.npb = npb;
	ret = check_npb(dev, 512);
	if (ret == 0) {
		debugf("runtimenv is wrong\n");
		return 0;
	}

	/*printf("magic = 0x%08x timestamp = 0x%08x min_id = %d max_id = %d tot_scts = %d sct_size = %d dir_entry_count = %d dir_entry_size = %d next_offset = 0x%08x backup_npb = %d backup_dir = %d\n", npb->magic, npb->timestamp, npb->min_id, npb->max_id, npb->tot_scts, npb->sct_size, npb->dir_entry_count, npb->dir_entry_size, npb->next_offset, npb->backup_npb, npb->backup_dir);*/

	ret = setup_devparam(&dev);
	ret = check_dir_table(&dev);
	ret = check_items(&dev);

	return ret;
}

void dump_all_buffer(unsigned char *buf, unsigned long len)
{
	unsigned long row, col;
	unsigned long offset;
	unsigned long total_row, remain_col;
	unsigned long flag = 1;

	total_row = len / 16;
	remain_col = len - total_row * 16;
    offset = 0;
	for (row = 0; row < total_row; row ++) {
		debugf("%08xh: ", offset );
		for (col = 0; col < 16; col ++)
			debugf("%02x ", buf[offset + col]);
		debugf("\n");
        offset += 16;
	}

	if (remain_col > 0) {
		debugf("%08xh: ", offset);
		for (col = 0; col < remain_col; col ++)
			debugf("%02x ", buf[offset + col]);
		debugf("\n");
	}

	debugf("\n");
}


/*
* retval : -1 is wrong  ;  1 is correct
*/
int runtimenv_is_correct(unsigned char *array, unsigned long size)
{
	unsigned long ret;

	ret = XCheckRunningNVStruct(array, size);
	if (ret == 1) {
		debugf("runtimenv is right\n");
		return 1;
	} else
		return -1;
}

/* /* phasecheck : 0 --- 3071; crc : 3072 3073; index : 3074 3075 */
/*
* retval : -1 is wrong  ;  1 is correct
*/
int sn_is_correct(unsigned char *array, unsigned long size)
{
	unsigned long crc;
	unsigned short sum = 0, *dataaddr;

	if (size == PRODUCTINFO_SIZE) {
		sum = calc_checksum(array, size);
		dataaddr = (unsigned short *)(array + size);
		if (*dataaddr == sum) {
			array[size] = 0xff; array[size + 1] = 0xff;
			array[size + 2] = 0xff; array[size + 3] = 0xff;
			return 1;
		}
	}

	debugf("phasecheck crc error\n");
	return -1;
}

unsigned long get_productinfo_index(unsigned char *array)
{
	unsigned long index = 0;
	unsigned short sum = 0, *dataaddr;

	dataaddr = (unsigned short *)(array + PRODUCTINFO_SIZE + 2);
	index = (unsigned long)(*dataaddr);
	return index;
}

int sn_is_correct_endflag(unsigned char *array, unsigned long size)
{
	unsigned long crc;
	unsigned short sum = 0, *dataaddr;

	if (size == PRODUCTINFO_SIZE) {
		sum = calc_checksum(array, size);
		dataaddr = (unsigned short *)(array + size);
		if (*dataaddr == sum)
			return 1;
	}

	debugf("phasecheck crc error\n");
	return -1;
}

/*FDT_ADD_SIZE used to describe the size of the new bootargs items*/
/*include lcd id, lcd base, etc*/
#define FDT_ADD_SIZE (500)

static int start_linux()
{
	void (*theKernel)(int zero, int arch, u32 params);
	u32 exec_at = (u32)-1;
	u32 parm_at = (u32)-1;
	u32 machine_type;
	u8* fdt_blob;
	u32 fdt_size;
	boot_img_hdr *hdr=raw_header;
	int err;

	machine_type = machine_arch_type;         /* get machine type */
	theKernel = (void (*)(int, int, u32))KERNEL_ADR; /* set the kernel address */
#ifndef CONFIG_SC8830
	*(volatile u32*)0x84001000 = 'j';
	*(volatile u32*)0x84001000 = 'm';
	*(volatile u32*)0x84001000 = 'p';
#endif

#ifdef CONFIG_OF_LIBFDT
	fdt_blob = (u8*)DT_ADR;
	if (fdt_check_header(fdt_blob) != 0) {
		printk("image is not a fdt\n");
	}
	fdt_size=fdt_totalsize(fdt_blob);

	err = fdt_open_into(fdt_blob, fdt_blob, fdt_size + FDT_ADD_SIZE);
	if (err != 0) {
		printf ("libfdt fdt_open_into(): %s\n",
				fdt_strerror(err));
	}

	fdt_initrd_norsvmem(fdt_blob,RAMDISK_ADR,RAMDISK_ADR+hdr->ramdisk_size, 1);

	fdt_fixup_lcdid(fdt_blob);
	fdt_fixup_lcdbase(fdt_blob);

	theKernel(0, machine_type, (unsigned long)fdt_blob);
#else
	theKernel(0, machine_type, VLX_TAG_ADDR);    /* jump to kernel with register set */
#endif
	while(1);
	return 0;
}
void lcd_display_logo(int backlight_set,ulong bmp_img,size_t size)
{
#define mdelay(t)     ({unsigned long msec=(t); while (msec--) { udelay(1000);}})//LiWei add
#ifdef CONFIG_SPLASH_SCREEN
	extern int lcd_display_bitmap(ulong bmp_image, int x, int y);
	extern void lcd_display(void);
	extern void *lcd_base;
	extern void Dcache_CleanRegion(unsigned int addr, unsigned int length);
	extern void set_backlight(uint32_t value);
	if(backlight_set == BACKLIGHT_ON){
		lcd_display_bitmap((ulong)bmp_img, 0, 0);
#if defined(CONFIG_SC8810) || defined(CONFIG_SC8825) || defined(CONFIG_SC8830)
		Dcache_CleanRegion((unsigned int)(lcd_base), size<<1);//Size is to large.
#endif
		lcd_display();
#ifdef CONFIG_SC8830_LVDS
	    mdelay(100);//LiWei add
#endif
		set_backlight(255);
	}else{
		memset((unsigned int)lcd_base, 0, size);
#if defined(CONFIG_SC8810) || defined(CONFIG_SC8825) || defined(CONFIG_SC8830)
		Dcache_CleanRegion((unsigned int)(lcd_base), size<<1);//Size is to large.
#endif
		lcd_display();
	}
#endif
}

int is_factorymode()
{
  char factorymode_falg[8]={0};
  int ret = 0;

	if ( do_fs_file_read(FACTORY_PART,"/factorymode.file",factorymode_falg,8))
		return 0;
	debugf("Checking factorymode :  factorymode_falg = %s \n", factorymode_falg);
	if(!strcmp(factorymode_falg, "1"))
		ret = 1;
	else
		ret = 0;
	debugf("Checking factorymode :  ret = %d \n", ret);
	return ret;
}

static char* get_product_sn(void)
{
	SP09_PHASE_CHECK_T phase_check;
	static char sn[SP09_MAX_SN_LEN];

	memset(sn, 0x0, SP09_MAX_SN_LEN);

	if(do_raw_data_read(PRODUCTINFO_FILE_PATITION, 
					sizeof(phase_check),
					0,
					(char *)&phase_check)){
		debugf("%s: read miscdata error.\n", __func__);
		return NULL;
	}

	if(phase_check.Magic == SP09_SPPH_MAGIC_NUMBER){
		memcpy(sn, phase_check.SN1, SP09_MAX_SN_LEN);
		return sn;
	}

	return NULL;
}

void addcmdline(char *buf)
{
#if (!BOOT_NATIVE_LINUX) || BOOT_NATIVE_LINUX_MODEM
#if defined (CONFIG_SC8830)
	/* tdfixnv=0x????????,0x????????*/
	int str_len = strlen(buf);
#ifndef CONFIG_SPX15_WCDMA
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
#endif
#ifndef CONFIG_SPX15_TD
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
#endif

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
{
	char *sn = get_product_sn();
	if(NULL != sn)
	{
		str_len = strlen(buf);
		sprintf(&buf[str_len], " androidboot.serialno=%s", sn);
	}
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

char * creat_cmdline(char * cmdline,boot_img_hdr *hdr)
{
	int str_len;
	char * buf;
	unsigned int *adc_data;
	buf = malloc(1024);
	memset(buf, 0, 1024);

	if(hdr){
		sprintf(buf, "initrd=0x%x,0x%x", RAMDISK_ADR, hdr->ramdisk_size);
	}
	/* preset loop_per_jiffy */
#ifdef CONFIG_LOOP_PER_JIFFY
	str_len = strlen(buf);
	sprintf(&buf[str_len], " lpj=%d", CONFIG_LOOP_PER_JIFFY);
#endif

#ifdef CONFIG_AP_VERSION
	str_len = strlen(buf);
	sprintf(&buf[str_len], " apv=\"%s\"", CONFIG_AP_VERSION);
#endif

	if(cmdline && cmdline[0]){
		str_len = strlen(buf);
		sprintf(&buf[str_len], " %s", cmdline);
	}
	{
#ifndef CONFIG_FPGA
		extern uint32_t load_lcd_id_to_kernel();
		uint32_t lcd_id;

		lcd_id = load_lcd_id_to_kernel();
		//add lcd id
		if(lcd_id) {
			str_len = strlen(buf);
			sprintf(&buf[str_len], " lcd_id=ID");
			str_len = strlen(buf);
			sprintf(&buf[str_len], "%x",lcd_id);
			str_len = strlen(buf);
			buf[str_len] = '\0';
		}
#endif
	}
	if(lcd_base != NULL){
		//add lcd frame buffer base, length should be lcd w*h*2(RGB565)
		str_len = strlen(buf);
		sprintf(&buf[str_len], " lcd_base=");
		str_len = strlen(buf);
		sprintf(&buf[str_len], "%x",lcd_base);
		str_len = strlen(buf);
		buf[str_len] = '\0';
	}

	int ret=is_factorymode();

	if (ret == 1) {
		debugf("1\n");
		str_len = strlen(buf);
		sprintf(&buf[str_len], " factory=1");
	}

	str_len = strlen(buf);
#ifdef CONFIG_RAM512M
    sprintf(&buf[str_len], " ram=512M");
#elif defined (CONFIG_RAM768M)
    sprintf(&buf[str_len], " ram=768M");
#else
    sprintf(&buf[str_len], " ram=256M");
#endif

	str_len = strlen(buf);
	sprintf(&buf[str_len], " no_console_suspend");

#ifdef CONFIG_RAM_CONSOLE
	/* Fill ram log base address and size to cmdline.
	It will be used when assigning reserve memory in kernel and dump ram log
	*/
	str_len = strlen(buf);
	sprintf(&buf[str_len], " boot_ram_log=%#010x,%#x",
		CONFIG_RAM_CONSOLE_START, CONFIG_RAM_CONSOLE_SIZE);
#endif

	addcmdline(buf);
	ret =read_spldata();
	if(ret != 0){
		free(buf);
		return NULL;
	}
	if(harsh_data == NULL){
		debugf("harsh_data malloc failed\n");
		free(buf);
		return NULL;
	}
	debugf("spl_data adr 0x%x harsh_data adr 0x%x\n", spl_data, harsh_data);
	ret = cal_md5(spl_data, CONFIG_SPL_LOAD_LEN, harsh_data);
	if(ret){
		str_len = strlen(buf);
		sprintf(&buf[str_len], " securemd5=%08x%08x%08x%08x", *(uint32_t*)harsh_data, *(uint32_t*)(harsh_data+4),\
			*(uint32_t*)(harsh_data+8), *(uint32_t*)(harsh_data+12));
	}

#if defined( CONFIG_AP_ADC_CALIBRATION)||defined(CONFIG_SC8830)|| (defined(CONFIG_SC8825) && (!(BOOT_NATIVE_LINUX)))
{
	extern int read_adc_calibration_data(char *buffer,int size);
	extern void CHG_SetADCCalTbl (unsigned int *adc_data);
	ret=0;
	adc_data = malloc(64);
	if(adc_data){
		memset(adc_data,0,64);
		ret = read_adc_calibration_data(adc_data,48);
		if(ret > 0){
			if(((adc_data[2]&0xffff) < 4500 )&&((adc_data[2]&0xffff) > 3000)&&
			((adc_data[3]&0xffff) < 4500 )&&((adc_data[3]&0xffff) > 3000)){
				str_len = strlen(buf);
				sprintf(&buf[str_len], " adc_cal=%d,%d",adc_data[2],adc_data[3]);
			}
			if((0x00000002 == adc_data[10])&&(0x00000003 == adc_data[11])){
				str_len = strlen(buf);
				sprintf(&buf[str_len], " fgu_cal=%d,%d,%d",adc_data[4],adc_data[5],adc_data[6]);
			}

		#if (defined(CONFIG_SC8825) && (!(BOOT_NATIVE_LINUX)))
			CHG_SetADCCalTbl(adc_data);
			DCDC_Cal_ArmCore();
		#endif
		}

		free(adc_data);
	}

#if defined(CONFIG_SC8830)
	{
		extern unsigned int fgu_cur;
		extern unsigned int fgu_vol;
		str_len = strlen(buf);
		sprintf(&buf[str_len], " fgu_init=%d,%d",fgu_vol,fgu_cur);
	}
#endif
   {
       extern long long lcd_init_time;
       str_len = strlen(buf);
       sprintf(&buf[str_len], " lcd_init=%lld",lcd_init_time);
   }
   {
       extern long long load_image_time;
       str_len = strlen(buf);
       sprintf(&buf[str_len], " load_image=%lld",load_image_time);
   }
   str_len = strlen(buf);
   sprintf(&buf[str_len], " pl_t=%lld",get_ticks());
}
#endif
    debugf("cmdline_len = %d \n pass cmdline: %s \n",strlen(buf), buf);
    //lcd_printf(" pass cmdline : %s\n",buf);
    //lcd_display();
    creat_atags(VLX_TAG_ADDR, buf, NULL, 0);
    return buf;
}
void vlx_entry()
{
#if !(defined CONFIG_SC8810 || defined CONFIG_TIGER || defined CONFIG_SC8830)
	MMU_InvalideICACHEALL();
#endif

#if (defined CONFIG_SC8810) || (defined CONFIG_SC8825) || (defined CONFIG_SC8830)
	MMU_DisableIDCM();
#endif

#ifdef REBOOT_FUNCTION_INUBOOT
	reboot_func();
#endif

#if BOOT_NATIVE_LINUX
	start_linux();
#else
	void (*entry)(void) = (void*) VMJALUNA_ADR;
	entry();
#endif
}
void normal_mode(void)
{
#if defined (CONFIG_SC8810) || defined (CONFIG_SC8825) || defined (CONFIG_SC8830)
    //MMU_Init(CONFIG_MMU_TABLE_ADDR);
	vibrator_hw_init();
#endif
    set_vibrator(1);

#ifndef UART_CONSOLE_SUPPORT
#ifdef CONFIG_SC7710G2
	extern int  serial1_SwitchToModem(void);
	serial1_SwitchToModem();
#endif
#endif

#if BOOT_NATIVE_LINUX
    vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS, BACKLIGHT_ON);
#else
    vlx_nand_boot(BOOT_PART, NULL, BACKLIGHT_ON);
#endif

}
void special_mode(void)
{
    debugf("special_mode\n");
#if BOOT_NATIVE_LINUX
    vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS " androidboot.mode=special", BACKLIGHT_OFF);
#else
    vlx_nand_boot(BOOT_PART, "androidboot.mode=special", BACKLIGHT_OFF);
#endif

}
#ifdef CONFIG_GENERIC_MMC
#define MODEM_MEMORY_NAME "modem_memory.log"
#define MODEM_MEMORY_SIZE  (22 * 1024 * 1024)
#ifdef CONFIG_SC8810
#define MODEM_MEMORY_ADDR 0
#elif defined (CONFIG_SC8825) || defined (CONFIG_TIGER) || defined(CONFIG_SC8830)
#define MODEM_MEMORY_ADDR 0x80000000
#endif
void write_modem_memory()
{
	struct mmc *mmc;
	block_dev_desc_t *dev_desc=NULL;
	int ret;
	char bufread[50];
	debugf("go to dump memory\n");
	mmc = find_mmc_device(0);
	if(mmc){
		ret = mmc_init(mmc);
		if(ret < 0){
			debugf("mmc init failed %d\n", ret);
			return;
		}
	}else{
		debugf("no mmc card found\n");
		return;
	}

	dev_desc = &mmc->block_dev;
	if(dev_desc==NULL){
		debugf("no mmc block device found\n");
		return;
	}
	ret = fat_register_device(dev_desc, 1);
	if(ret < 0){
		debugf("fat regist fail %d\n", ret);
		return;
	}
	ret = file_fat_detectfs();
	if(ret){
		debugf("detect fs failed\n");
		return;
	}

	debugf("writing %s\n",  MODEM_MEMORY_NAME);
	ret = file_fat_write(MODEM_MEMORY_NAME, MODEM_MEMORY_ADDR, MODEM_MEMORY_SIZE);
	if(ret <= 0){
		debugf("sd file write error %d\n", ret);
	}
}
#endif
extern int fatal_dump_enabled(void);
void watchdog_mode(void)
{
	debugf("watchdog_mode\n");
#ifdef CONFIG_GENERIC_MMC
#ifndef CONFIG_SC8830
	if(fatal_dump_enabled())
		write_modem_memory();
#endif
#endif
#if BOOT_NATIVE_LINUX
	vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS " androidboot.mode=wdgreboot", BACKLIGHT_OFF);
#else
	vlx_nand_boot(BOOT_PART, "androidboot.mode=wdgreboot", BACKLIGHT_OFF);
#endif
}

void unknow_reboot_mode(void)
{
	debugf("unknow_reboot_mode\n");
#ifdef CONFIG_GENERIC_MMC
#ifndef CONFIG_SC8830
	if(fatal_dump_enabled())
		write_modem_memory();
#endif
#endif
#if BOOT_NATIVE_LINUX
	vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS " androidboot.mode=unknowreboot", BACKLIGHT_OFF);
#else
	vlx_nand_boot(BOOT_PART, "androidboot.mode=unknowreboot", BACKLIGHT_OFF);
#endif
}
void panic_reboot_mode(void)
{
	debugf("%s\n", __func__);
#ifdef CONFIG_GENERIC_MMC
#ifndef CONFIG_SC8830
	if(fatal_dump_enabled())
		write_modem_memory();
#endif
#endif
#if BOOT_NATIVE_LINUX
	vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS " androidboot.mode=panic", BACKLIGHT_OFF);
#else
	vlx_nand_boot(BOOT_PART, "androidboot.mode=panic", BACKLIGHT_OFF);
#endif
}

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

#if defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB) || defined(CONFIG_SC9620OPENPHONE) || defined(CONFIG_SC9620FPGA) || defined(CONFIG_SPX15_TD)
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

#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP5735) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SPX15_WCDMA)

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
#if defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB) || defined(CONFIG_SC9620OPENPHONE) || defined(CONFIG_SC9620FPGA) || defined(CONFIG_SPX15_TD)
	memset((void *)SIPC_TD_APCP_START_ADDR, 0x0, SIPC_APCP_RESET_ADDR_SIZE);

#elif defined(CONFIG_SP7735EC) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP5735) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SPX15_WCDMA)

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

