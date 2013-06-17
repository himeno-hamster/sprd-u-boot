/****************************************************
*	Author: dayong.yang
*	Last modified: 2013-04-03 13:30
*	Filename: special_downloading.c
*	Description: 
****************************************************/
#include <config.h>
#include "special_downloading.h"





//external functions declaration
extern void cmd_yaffs_mount(char *mp);
extern void cmd_yaffs_umount(char *mp);
extern void cmd_yaffs_mwrite_file(char *fn, char *addr, int size);
extern void cmd_yaffs_mread_file(char *fn, unsigned char *addr);
extern unsigned short calc_checksum(unsigned char *dat, unsigned long len);

//macro definition
#define VLX_RAND_TO_UNSIGNED_INT( _addr ) \
		if( (_addr) & 0x3 ){_addr += 0x4 -((_addr) & 0x3); }

static int check_compatible = 0;
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
*	description:check the file structure to identify 
*				whether it has been damaged or not.
****************************************************/
static int file_check(unsigned char *array, unsigned long size)
{
	//check both crc16 , fixed flag	and make sure the crc16 value is not zero!
	if((unsigned short)crc16(0,array, size-2)== *((unsigned short*)&array[size-2])&&
		//0x5a5a5a5a == *((unsigned int*)&array[size])&&
		0 != *((unsigned short*)&array[size-2])&&
		(unsigned short)calc_checksum(array,size-4) == *((unsigned short*)&array[size-4]))
	{
		return 1; 
	}else{
        if(check_compatible){
            if((unsigned short)calc_checksum(array,size-4) == *((unsigned short*)&array[size-4])){
                return 1; 
            }
        }
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
	const char * curr_file = sector_name;

	//clear memory
	memset(mem_addr, 0xff,size);
	//mount yaffs
	cmd_yaffs_mount(sector_path);

TRY_BACKUP_FILE:
	//is file exist and has a right size?
	ret = cmd_yaffs_ls_chk(curr_file);
	if (ret == size||ret == size - 4) {             //0x5a5a5a5a end flags maybe cutted by NVITEMD!
		//read file to mem
		cmd_yaffs_mread_file(curr_file, mem_addr);
		ret = file_check(mem_addr, size-4);
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
		printf("[load_sector_to_memory]sync the latest file......\n");
		cmd_yaffs_mwrite_file((char*)backup_sector_name, (char*)mem_addr,(int)size);
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
*	description: util function used to read fixnv
*			gBuff -- a global addr used to temp store the whole file
*			offset -- from where to read
*			size -- how long to read
*			buf -- where to put the readed data
****************************************************/
int fdl_read_fixnv(unsigned char * gBuf,unsigned int offset, unsigned int size, unsigned char *buf){

	char *fixnvpoint = "/fixnv";
	char *fixnvfilename = "/fixnv/fixnv.bin";
	char *fixnvfilename2 = "/fixnv/fixnvbkup.bin";
	char *backupfixnvpoint = "/backupfixnv";
	char *backupfixnvfilename = "/backupfixnv/fixnv.bin";
	int ret = 0;

    check_compatible = 1;//In order to support the old file end flags.

	/* FIXNV_PART */
	printf("Reading fixnv to 0x%08x\n", gBuf);
	//try "/fixnv/fixnvchange.bin" first,if fail,
	//try /fixnv/fixnv.bin instead
	ret = load_sector_to_memory(fixnvpoint,
			fixnvfilename,//fixnvfilename,
			fixnvfilename2,//fixnvfilename2,
			(unsigned char *)gBuf,
			FIXNV_SIZE + 4);
	if(ret == -1){
		//fixnvpoint's files are not correct
		//the "/backupfixnv/fixnv.bin" must be correct!
		ret = load_sector_to_memory(backupfixnvpoint,
				backupfixnvfilename,
				0,//backupfixnvfilename2,
				(unsigned char *)gBuf,
				FIXNV_SIZE + 4);
		if(ret ==1){
			//we got a right file in backupfixnvpoint,
			//use it to recovery fixnvpoint's files.
			recovery_sector(fixnvpoint, 
					fixnvfilename, 
					fixnvfilename2, 
					(unsigned char *)gBuf,
					FIXNV_SIZE + 4);
		}else{
			//backupfixnvpoint's files are still uncorrect.
			//then we can do nothing to get it right!!!!
			//there is an fatal error has occured.
			printf("\n\nfixnv and backupfixnv are all wrong!\n\n");
			//clear memory
			//memset(gBuf, 0xff,FIXNV_SIZE + 4);
			ret = -1;
			goto EXIT;
		}
	}else{
		//everything is right!!
		//there's nothing to do here ......
	}	
	printf("Reading fixnv success!\n");
    
    if(!check_compatible){
        //finally we check the fixnv structure,if fail,then u-boot will hung up!!!		
        if(check_fixnv_struct(gBuf,FIXNV_SIZE) == -1){
            ret = -1;
            goto EXIT;
        }
    }

	memcpy(buf, (unsigned char *)(gBuf + offset), size);
	ret = size;

EXIT:
	return ret;
}

/****************************************************
*	description: util function used to read productinfo
*			gBuff -- a global addr used to temp store the whole file
*			offset -- from where to read
*			size -- how long to read
*			buf -- where to put the readed data
****************************************************/
int fdl_read_productinfo(unsigned char * gBuf,unsigned int offset, unsigned int size, unsigned char *buf){
	int ret = 0;
	char *productinfopoint = "/productinfo";
	char *productinfofilename = "/productinfo/productinfo.bin";
	char *productinfofilename2 = "/productinfo/productinfobkup.bin";

	printf("Reading productinfo to 0x%08x\n", gBuf);
	ret = load_sector_to_memory(productinfopoint,
			productinfofilename,
			productinfofilename2,
			(unsigned char *)gBuf,
			PRODUCTINFO_SIZE + 4);
	if(ret == -1){
		printf("Reading productinfo to 0x%08x error!\n", gBuf);
		goto EXIT;
	}

	memcpy(buf, (unsigned char *)(gBuf + offset), size);
	ret = size;


EXIT:
	return ret;
}

/****************************************************
*	description: This function used to download fixnv
*				file in research download process.
****************************************************/
void fdl_download_fixnv(unsigned char *gBuf,int is_fixnv){
	char *fixnvpoint = "/fixnv";
	char *fixnvfilename = "/fixnv/fixnv.bin";
	char *fixnvfilename2 = "/fixnv/fixnvbkup.bin";
	char *backupfixnvpoint = "/backupfixnv";
	char *backupfixnvfilename = "/backupfixnv/fixnv.bin";
	unsigned int length = 0;

	length = Vlx_CalcFixnvLen(gBuf, gBuf+FIXNV_SIZE);	
	*((unsigned int*)&gBuf[FIXNV_SIZE-8])= length;//keep the real  fixnv file size.	
	*((unsigned short*)&gBuf[FIXNV_SIZE-4])= calc_checksum(gBuf,FIXNV_SIZE-4); 
	*(unsigned short*)&gBuf[FIXNV_SIZE-2] = crc16(0,(unsigned const char*)gBuf,FIXNV_SIZE-2);
	*(unsigned int*)&gBuf[FIXNV_SIZE] = 0x5a5a5a5a;

	if(is_fixnv){
		recovery_sector(fixnvpoint, 
				fixnvfilename, 
				fixnvfilename2, 
				(unsigned char *)gBuf,
				FIXNV_SIZE + 4);
	}else{
		recovery_sector(backupfixnvpoint, 
				backupfixnvfilename , 
				0, 
				(unsigned char *)gBuf,
				FIXNV_SIZE + 4);
	}
}

/****************************************************
*	description: This function used to download productinfo
*			file in research download process.
****************************************************/
void fdl_download_productinfo(unsigned char *gBuf){
	int ret = 0;
	char *productinfopoint = "/productinfo";
	char *productinfofilename = "/productinfo/productinfo.bin";
	char *productinfofilename2 = "/productinfo/productinfobkup.bin";

	*((unsigned short*)&gBuf[PRODUCTINFO_SIZE-4])= calc_checksum(gBuf,PRODUCTINFO_SIZE-4); 
	*(unsigned short*)&gBuf[PRODUCTINFO_SIZE-2] = crc16(0,(unsigned const char*)gBuf,PRODUCTINFO_SIZE-2);
	*(unsigned int*)&gBuf[PRODUCTINFO_SIZE] = 0x5a5a5a5a;

	recovery_sector(productinfopoint, 
			productinfofilename, 
			productinfofilename2, 
			(unsigned char *)gBuf,
			PRODUCTINFO_SIZE + 4);

}
