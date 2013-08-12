/****************************************************
*	Author: dayong.yang
*	Last modified: 2013-04-02 23:09
*	Filename: special_loading.c
*	Description:
****************************************************/
#include "normal_mode.h"
#include "special_loading.h"

//external functions declaration
extern void cmd_yaffs_mount(char *mp);
extern void cmd_yaffs_umount(char *mp);
extern void cmd_yaffs_mwrite_file(char *fn, char *addr, int size);
extern void cmd_yaffs_mread_file(char *fn, unsigned char *addr);
extern int find_dev_and_part(const char *id, struct mtd_device **dev,unsigned char *part_num, struct part_info **part);
extern int nand_write_spl(unsigned char *buf, struct mtd_info *mtd);
extern unsigned short calc_checksum(unsigned char *dat, unsigned long len);
extern struct mtd_info nand_info[];
//macro definition
#define VLX_RAND_TO_UNSIGNED_INT( _addr ) \
		if( (_addr) & 0x3 ){_addr += 0x4 -((_addr) & 0x3); }

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

///////////////////////////////////////////////////////////////////
//CRC Table
///////////////////////////////////////////////////////////////////
/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
static const unsigned short  crc16_table[256] =
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static int uart_log_off = 1;
static void setup_uart_log_flag(unsigned char *buf);

/****************************************************
*	description: util function used to calc CRC16 value
****************************************************/
static unsigned short crc16 (unsigned short crc, unsigned char const *buffer, unsigned int len)
{
	while (len--)
	{
		crc = (crc >> 8) ^ (crc16_table[ (crc^ (*buffer++)) &0xff]);
	}

	return crc;
}

/****************************************************
*	description:caculate the valid length of NV file
****************************************************/
static unsigned int Vlx_CalcFixnvLen(unsigned int search_start, unsigned int search_end)
{
	unsigned int start_addr, end_addr;
	unsigned short id, len;
	volatile unsigned short *flash_ptr;

	start_addr = search_start;
	end_addr   = search_end;
	start_addr += sizeof(unsigned int); //skip update flag

	while(start_addr < end_addr)
	{
		flash_ptr = (volatile unsigned short *)(start_addr);
		id  = *flash_ptr++;
		len = *flash_ptr;
		if(0xFFFF == id)
		{
			return (start_addr-search_start);
		}
		else
		{
			start_addr += 4 + len +(len & 0x1);
			VLX_RAND_TO_UNSIGNED_INT( start_addr );
		}
	}
	return 0xffffffff;
}

/****************************************************
*	description: to caculate a specified NV item's
*				offset addr in given NV file.
****************************************************/
static unsigned int Vlx_GetFixedBvitemAddr(unsigned short identifier, unsigned int search_start, unsigned int search_end)
{
	unsigned int start_addr, end_addr;
	unsigned short id, len;
	volatile unsigned short *flash_ptr;

	start_addr = search_start;
	end_addr   = search_end;
	start_addr += sizeof(unsigned int); //skip update flag

	while(start_addr < end_addr)
	{
		flash_ptr = (volatile unsigned short *)(start_addr);
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
			VLX_RAND_TO_UNSIGNED_INT( start_addr )
		}
	}
	return 0xffffffff;
}

/****************************************************
*	description:check the file structure to identify
*				whether it has been damaged or not.
****************************************************/
static int file_check(unsigned char *array, unsigned long size, unsigned short *crc)
{
	//check both crc16 , fixed flag	and make sure the crc16 value is not zero!
	if((unsigned short)crc16(0,array, size-2)== *((unsigned short*)&array[size-2])&&
		//0x5a5a5a5a == *((unsigned int*)&array[size])&&
		0 != *((unsigned short*)&array[size-2])&&
		(unsigned short)calc_checksum(array,size-4) == *((unsigned short*)&array[size-4]))
	{
	        if(crc){
                    *crc = *((unsigned short*)&array[size-2]);
                }
                return 1;
	}else{
        printf("file_check error!\n");
        printf("*((unsigned short*)&array[size-2]) = 0x%x\n",*((unsigned short*)&array[size-2]));
        printf("(unsigned short)crc16(0,array, size-2) = 0x%x\n",(unsigned short)crc16(0,array, size-2));
        printf("(unsigned short)calc_checksum(array,size-4) = 0x%x\n",(unsigned short)calc_checksum(array,size-4));
        printf("*((unsigned short*)&array[size-4]) = 0x%x\n",*((unsigned short*)&array[size-4]));
		return -1;
	}
}

/****************************************************
*	description: util function used to check the NV
*			length keept in file is wheather correct
*			or not.
****************************************************/
static int check_fixnv_struct(unsigned int addr,unsigned int size){
	unsigned int length = 0,keep_length=0;
	volatile unsigned int *flash_ptr;

	flash_ptr = (volatile unsigned int *)(addr+size-8);
	keep_length = *flash_ptr;
	//printf("keep_length=%d  line=%d\r\n",keep_length ,__LINE__);
	if(keep_length != 0xffffffff){
		length = Vlx_CalcFixnvLen(addr, addr+size);
		if(keep_length != length){
			printf("keep_length=%d  length=%d line=%d\r\n",keep_length ,length,__LINE__);
			return -1;
		}
	}
	return 1;
}

/****************************************************
*	description:util function used to recovery file
****************************************************/
static void recovery_sector(const char* dst_sector_path,
		const char* dst_sector_name,
		const char* backup_dst_sector_name,
		unsigned char * mem_addr,
		unsigned int size)
{
	cmd_yaffs_mount(dst_sector_path);
	cmd_yaffs_mwrite_file((char*)dst_sector_name,(char*)mem_addr, (int)size);
	if(backup_dst_sector_name)
		cmd_yaffs_mwrite_file((char*)backup_dst_sector_name,(char*)mem_addr,(int)size);
	cmd_yaffs_umount(dst_sector_path);
}

/****************************************************
*	description:load file to memory and do some sync
*				options,if nessary.
****************************************************/
static int load_sector_to_memory(const char* sector_path,
		const char* sector_name,
		const char* backup_sector_name,
		unsigned char * mem_addr,
		unsigned int size)
{
    int ret = -1;
    int try_backup_file = 0;
    int is_back_file_ok= 0;
    const char * curr_file = sector_name;
    const char * back_file = backup_sector_name;
    unsigned short back_file_crc=0;
    unsigned short master_file_crc=1;
    //clear memory
    memset(mem_addr, 0xff,size);
    //mount yaffs
    cmd_yaffs_mount(sector_path);

    if(backup_sector_name){
        //is file exist and has a right size?
        ret = cmd_yaffs_ls_chk(back_file);
        if (ret == size||ret == size - 4) {
            //read file to mem
            cmd_yaffs_mread_file(back_file, mem_addr);
            ret = file_check(mem_addr, size-4, (unsigned short*)&back_file_crc);
        }
        else{
            ret = -1;
        }

        if(ret == 1){
            is_back_file_ok=1;
        }
    }
TRY_BACKUP_FILE:
    ret = cmd_yaffs_ls_chk(curr_file);
    if (ret == size||ret == size - 4) {             //0x5a5a5a5a end flags maybe cutted by NVITEMD!
        //read file to mem
        cmd_yaffs_mread_file(curr_file, mem_addr);
        ret = file_check(mem_addr, size-4, (unsigned short*)&master_file_crc);
    }
    else{
        ret = -1;
    }

    //try backup files.
    if(backup_sector_name&&ret == -1&&!try_backup_file){
        curr_file = backup_sector_name;
        try_backup_file = 1;
        goto TRY_BACKUP_FILE;
    }else if(backup_sector_name&&ret == 1&&!try_backup_file){
        if(is_back_file_ok && (back_file_crc == master_file_crc)){
            printf("[load_sector_to_memory]both of files are correct, no need sync......\n");
        }
        else{
            printf("[load_sector_to_memory]sync the latest file......\n");
            cmd_yaffs_mwrite_file((char*)backup_sector_name, (char*)mem_addr,(int)size);
        }
    }

    if(try_backup_file&&ret==1){
        //recovery_sector(sector_path,sector_name,mem_addr,size);
        printf("[load_sector_to_memory]recovery the latest file......\n");
        cmd_yaffs_mwrite_file((char*)sector_name, (char*)mem_addr,(int)size);
    }else if(try_backup_file&&ret==-1){
        printf("[load_sector_to_memory]both of the files are not correct......\n");
    }

    //unmout yaffs
    cmd_yaffs_umount(sector_path);

    return ret;
}

/****************************************************
*	description: util function used to load boot.img
*			file to memory.
****************************************************/
static int load_kernel_and_layout(struct mtd_info *nand,
		unsigned int phystart,
		char *header,
		char *kernel,
		char *ramdisk,
		unsigned int virtual_page_size,
		unsigned int real_page_size
		){
	int ret = -1;
	boot_img_hdr *hdr = (boot_img_hdr*)header;
	unsigned int off = phystart;
	int size = real_page_size;

	printf("virtual_page_size : %x\n",virtual_page_size);
	printf("real_page_size : %x\n",real_page_size);
	//read boot image header
	ret = nand_read_offset_ret(nand, off, &size, (void *)hdr, &off);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
		return -1;
	}
	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		printf("bad boot image header, give up read!!!!\n");
		return -1;
	}
	else
	{

		char* prev_page_addr = header;
		//we asume that the header takes only one page.
		//read kernel image prepare
		unsigned int used_size = 1*virtual_page_size;
		unsigned int spare_size = 0;
		unsigned int next_file_size = hdr->kernel_size;

		if(used_size > 0){
			spare_size = real_page_size - used_size;
		}else{
			spare_size = 0;
		}
		//read kernel image
		printf("file size: %x\n",hdr->kernel_size);
		printf("use size: %x\n",used_size);
		printf("spare size: %x\n",spare_size);

		if(spare_size){
			memcpy(kernel,&prev_page_addr[used_size],spare_size);
			next_file_size -= spare_size;
		}
		size = (next_file_size+(real_page_size - 1)) & (~(real_page_size - 1));
		ret = nand_read_offset_ret(nand, off, &size, (void *)(kernel+spare_size), &off);
		if(ret != 0){
			printf("reading kernel error!\n");
			printf("try reading to %x\n",kernel+spare_size);
		}
		//read ramdisk image prepare
		prev_page_addr =  (char*)(kernel+spare_size+size-real_page_size);
		used_size = (next_file_size%real_page_size+virtual_page_size-1)&(~(virtual_page_size-1));
		if(used_size > 0){
			spare_size = real_page_size - used_size;
		}else{
			spare_size = 0;
		}
		next_file_size = hdr->ramdisk_size;
		printf("file size: %x\n",hdr->ramdisk_size);
		printf("use size: %x\n",used_size);
		printf("spare size: %x\n",spare_size);
		//read ramdisk image
		if(spare_size){
			memcpy(ramdisk,&prev_page_addr[used_size],spare_size);
			next_file_size -= spare_size;
		}
		size = (next_file_size+(real_page_size - 1)) & (~(real_page_size - 1));
		ret = nand_read_offset_ret(nand, off, &size, (void *)(ramdisk+spare_size), &off);
		if(ret != 0){
			printf("reading ramdisk error!\n");
			printf("try reading to %x\n",ramdisk+spare_size);
		}
	}

	return ret;
}

/****************************************************
*	description: util function used to backup NV
*			parameters.
****************************************************/
static void update_fixnv(unsigned char *old_addr,unsigned char*new_addr){
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

	length = Vlx_CalcFixnvLen(new_addr, new_addr+FIXNV_SIZE);
	*((unsigned int*)&new_addr[FIXNV_SIZE-8]) = length;//keep the real  fixnv file size.
	*((unsigned short*)&new_addr[FIXNV_SIZE-4]) = calc_checksum(new_addr,FIXNV_SIZE-4);
	*((unsigned short*)&new_addr[FIXNV_SIZE-2]) = crc16(0,(unsigned const char*)new_addr,FIXNV_SIZE-2);
	*(unsigned int*)&new_addr[FIXNV_SIZE] = 0x5a5a5a5a;
	memcpy(old_addr,new_addr,FIXNV_SIZE+4);
}

/****************************************************
*	description: In the second part of OTA process,
*				we need to try updating 'spl.bin' in
*				case there is a need to update it.
****************************************************/
int try_update_spl(void){
	char *spl_mount_point = "/spl";
	char *tmp_mount_point = "/productinfo";
	char *tmp_file_name = "/productinfo/spl.bin";
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	struct nand_chip *chip;
	unsigned char pnum;
	int length = 0;
	int ret = 0;

	printf("test if there is a need to update spl......\n");
	ret = find_dev_and_part("spl", &dev, &pnum, &part);
	if(!ret)
		nand = (struct mtd_info *)&nand_info[dev->id->num];
	cmd_yaffs_mount(tmp_mount_point);
	length = cmd_yaffs_ls_chk(tmp_file_name);
	if(length > 0){
		//try to update spl.
		printf("trying to update spl ......\n");
		memset((unsigned char*)RUNTIMENV_ADR, 0xff,16*1024);//spl.bin's valid size is 16K
		cmd_yaffs_mread_file(tmp_file_name, RUNTIMENV_ADR);
		ret = nand_write_spl((unsigned char*)RUNTIMENV_ADR, nand);
		if(ret != 0){
			goto EXIT;
		}
		printf("spl is updated remove the tmp bin file in backupfixnv partition!\n");
		cmd_yaffs_rm(tmp_file_name);
	}
	cmd_yaffs_umount(tmp_mount_point);

EXIT:
	return ret;
}

/****************************************************
*	description: get crc of a given file
****************************************************/
static unsigned short get_crc_status(const char * mount_point,const char *file,
        unsigned int size,unsigned char * addr){
    int ret = -1;

    ret = load_sector_to_memory(mount_point,
            file,
            0,//backupfixnvfilename2,
            addr,
            size);
    if(ret == 1){
        return  *((unsigned short*)&addr[size-6]);
    }else{
        return 0;
    }
}

/****************************************************
*	description: This function designed to deal with
*				NV partion's update and loading logic.
****************************************************/
int try_load_fixnv(void){
	int length = 0,need_update = 0;
	char *nvitem_point = "/productinfo";
	char *nvitem_name = "/productinfo/nvitem.bin";
	//default names,if already has been changed,please update it.
	char *fixnvpoint = "/fixnv";
	char *fixnvfilename = "/fixnv/fixnv.bin";
	char *fixnvfilename2 = "/fixnv/fixnvbkup.bin";
	char *backupfixnvpoint = "/backupfixnv";
	char *backupfixnvfilename = "/backupfixnv/fixnv.bin";
	int ret = 0;
    unsigned short crc = 0;

	printf("test if there is a need to update fixnv......\n");
	cmd_yaffs_mount(nvitem_point);
	length = cmd_yaffs_ls_chk(nvitem_name);
	if(length > 0&&length < FIXNV_SIZE){
		memset((unsigned char*)RUNTIMENV_ADR, 0xff,FIXNV_SIZE + 4);
		cmd_yaffs_mread_file(nvitem_name, RUNTIMENV_ADR);
		need_update = 1;
		printf("fixnv file needs to be updated......\n");
	}

	/* FIXNV_PART */
    crc = get_crc_status(backupfixnvpoint,
                backupfixnvfilename,
                FIXNV_SIZE + 4,
                (unsigned char *)FIXNV_ADR);

	printf("Reading fixnv to 0x%08x\n", FIXNV_ADR);
	//try "/fixnv/fixnvchange.bin" first,if fail,
	//try /fixnv/fixnv.bin instead
	ret = load_sector_to_memory(fixnvpoint,
			fixnvfilename,//fixnvfilename,
			fixnvfilename2,//fixnvfilename2,
			(unsigned char *)FIXNV_ADR,
			FIXNV_SIZE + 4);
	if(ret == -1){
		//fixnvpoint's files are not correct
		//the "/backupfixnv/fixnv.bin" must be correct!
		ret = load_sector_to_memory(backupfixnvpoint,
				backupfixnvfilename,
				0,//backupfixnvfilename2,
				(unsigned char *)FIXNV_ADR,
				FIXNV_SIZE + 4);
		if(ret ==1){
			//we got a right file in backupfixnvpoint,
			//use it to recovery fixnvpoint's files.
			recovery_sector(fixnvpoint,
					fixnvfilename,
					fixnvfilename2,
					(unsigned char *)FIXNV_ADR,
					FIXNV_SIZE + 4);
		}else{
			//backupfixnvpoint's files are still uncorrect.
			//then we can do nothing to get it right!!!!
			//there is an fatal error has occured.
			printf("\n\nfixnv and backupfixnv are all wrong!\n\n");
			//clear memory
			//memset(FIXNV_ADR, 0xff,FIXNV_SIZE + 4);
			ret = -1;
			goto EXIT;
		}
	}else{
		//everything is right!!
		//there's nothing to do here ......
        if(crc == *((unsigned short *)(FIXNV_ADR+FIXNV_SIZE-2))){
            printf("backupfixnv/fixnv.bin is lastest,no need syncing ...\n");
        }else{
            printf("sync fixnv partition to backupfixnv partition ...\n");
            recovery_sector(backupfixnvpoint,
                    backupfixnvfilename,
                    0,
                    (unsigned char *)FIXNV_ADR,
                    FIXNV_SIZE + 4);
        }
	}
	printf("Reading fixnv success!\n");

	if(need_update){
		printf("update fixnv's items\n");
		update_fixnv(FIXNV_ADR,RUNTIMENV_ADR);
		printf("update fixnv partition");
		recovery_sector(fixnvpoint,
				fixnvfilename,
				fixnvfilename2,
				(unsigned char *)FIXNV_ADR,
				FIXNV_SIZE + 4);
		printf("remove /fixnv/nvitem.bin");
		cmd_yaffs_rm(nvitem_name);
	}

	//finally we check the fixnv structure,if fail,then u-boot will hung up!!!
	if(check_fixnv_struct(FIXNV_ADR,FIXNV_SIZE) == -1){
        printf("check_fixnv_struct error !!\n");
		ret = -1;
		goto EXIT;
	}

    setup_uart_log_flag((unsigned char*)FIXNV_ADR);
EXIT:
	cmd_yaffs_umount(nvitem_point);
	return ret;
}


/****************************************************
*	description: loading runtime nv partition
****************************************************/
int try_load_runtimenv(void){
	int ret = 0;
	char *runtimenvpoint = "/runtimenv";
	char *runtimenvfilename = "/runtimenv/runtimenv.bin";
	char *runtimenvfilename2 = "/runtimenv/runtimenvbkup.bin";

	printf("Reading runtimenv to 0x%08x\n",RUNTIMENV_ADR);
	ret = load_sector_to_memory(runtimenvpoint,
			runtimenvfilename,
			runtimenvfilename2,
			(unsigned char *)RUNTIMENV_ADR,
			RUNTIMENV_SIZE + 4);
	if(ret == -1){
		//clear memory
		memset(RUNTIMENV_ADR, 0xff,RUNTIMENV_SIZE + 4);
		ret = 1;
	}

	return ret;
}

/****************************************************
*	description: loading productinfo partition
****************************************************/
int try_load_productinfo(void){
	int ret = 0;
	char *productinfopoint = "/productinfo";
	char *productinfofilename = "/productinfo/productinfo.bin";
	char *productinfofilename2 = "/productinfo/productinfobkup.bin";

	printf("Reading productinfo to 0x%08x\n", PRODUCTINFO_ADR);
	ret = load_sector_to_memory(productinfopoint,
			productinfofilename,
			productinfofilename2,
			(unsigned char *)PRODUCTINFO_ADR,
			PRODUCTINFO_SIZE + 4);
	if(ret == -1){
		printf("Reading productinfo to 0x%08x error!\n", PRODUCTINFO_ADR);
	}

	return ret;
}

/****************************************************
*	description: set up uart log flag
****************************************************/
static void setup_uart_log_flag(unsigned char *buf){

    if(buf[0xcd58] == 0x0c&&
       buf[0xcd59] == 0x00&&
       buf[0xcd5a] == 0x32&&
       buf[0xcd5b] == 0x00){
        printf("find valid magic number!\n");
        if(buf[0xcd5c]==0x00){
            printf("open uart log!\n");
            uart_log_off = 0;
        }else if(buf[0xcd5c]==0x01){
            printf("close uart log!\n");
            uart_log_off = 1;
        }
    }else{
        printf("can't find valid magic number!\n");
        printf("buf[0xcd58] = %d\nbuf[0xcd59] = %d\nbuf[0xcd5a] = %d\nbuf[0xcd5b] = %d\n",
                buf[0xcd58],      buf[0xcd59],      buf[0xcd5a],      buf[0xcd5b]);
    }
}

/****************************************************
*	description: get uart log flag
****************************************************/
int is_uart_log_off(void){
    return uart_log_off;
}

