
#include "nvitem_common.h"
#include "nvitem_fs.h"
#include "nvitem_config.h"

#define CRC_16_L_SEED			0x80
#define CRC_16_L_POLYNOMIAL	0x8000
#define CRC_16_POLYNOMIAL		0x1021
unsigned short __crc_16_l_calc (uint8 *buf_ptr,uint32 len)
{
	unsigned int i;
	unsigned short crc = 0;

	while (len--!=0)	{
		for (i = CRC_16_L_SEED; i !=0 ; i = i>>1){
			if ( (crc & CRC_16_L_POLYNOMIAL) !=0){
				crc = crc << 1 ;
				crc = crc ^ CRC_16_POLYNOMIAL;
			}
			else	{
				crc = crc << 1 ;
			}
			if ( (*buf_ptr & i) != 0){
				crc = crc ^ CRC_16_POLYNOMIAL;
			}
		}
		buf_ptr++;
	}
	return (crc);
}

static unsigned short calc_checksum(unsigned char *dat, unsigned long len)
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



/*
	TRUE(1): pass
	FALSE(0): fail
*/
BOOLEAN _chkEcc(uint8* buf, uint32 size)
{
	uint16 crc,crcOri;
//	crc = __crc_16_l_calc(buf, size-2);
//	crcOri = (uint16)((((uint16)buf[size-2])<<8) | ((uint16)buf[size-1]) );

	crc = calc_checksum(buf,size-4);
	crcOri = (uint16)((((uint16)buf[size-3])<<8) | ((uint16)buf[size-4]) );

	return (crc == crcOri);
}


void _makEcc(uint8* buf, uint32 size)
{
	uint16 crc;
	//crc = __crc_16_l_calc(buf, size-2);
	crc = calc_checksum(buf,size-4);
	buf[size-4] = (uint8)(0xFF&crc);
	buf[size-3] = (uint8)(0xFF&(crc>>8));
	buf[size-2] = 0;
	buf[size-1] = 0;

	return;
}


#ifdef CONFIG_EMMC_BOOT
static block_dev_desc_t *s_block_dev = 0;

static RAM_NV_CONFIG _ramdiskCfg[RAMNV_NUM+1] = 
{
	{1,	L"fixnv1",		L"fixnv2",		0x20000	},
	{2,	L"runtimenv1",L"runtimenv2",0x40000	},
	{0,	L"",L"",0}
};

const RAM_NV_CONFIG*	ramDisk_Init(void)
{
	s_block_dev = get_dev("mmc", 1);
	return _ramdiskCfg;
}

RAMDISK_HANDLE	ramDisk_Open(uint32 partId)
{
	return (RAMDISK_HANDLE)partId;
}

int _getIdx(RAMDISK_HANDLE handle)
{
	uint32 i;
	uint32 partId = (uint32)handle;

	for(i = 0; i < sizeof(_ramdiskCfg)/sizeof(RAM_NV_CONFIG); i++){
		if(0 == _ramdiskCfg[i].partId){
			return -1;
		}
		else if(partId == _ramdiskCfg[i].partId){
			return i;
		}
	}
	return -1;
}

/*
	1 read imagPath first, if succes return , then
	2 read imageBakPath, if fail , return, then
	3 fix imagePath

	note:
		first imagePath then imageBakPath
*/
BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	int idx;
	disk_partition_t info;
	wchar_t *firstName, *secondName;

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}

	if(!s_block_dev){
		printf("NVITEM partId%x:image read error!\n",_ramdiskCfg[idx].partId);
		return 1;
	}
// 0 get read order
	if(1){
		firstName = _ramdiskCfg[idx].image_path;
		secondName = _ramdiskCfg[idx].imageBak_path;
	}
	else{
		secondName = _ramdiskCfg[idx].image_path;
		firstName = _ramdiskCfg[idx].imageBak_path;
	}
// 1 read origin image
	memset(buf, 0xFF, size);
	if(!get_partition_info_by_name(s_block_dev, firstName, &info)){
		if(Emmc_Read(PARTITION_USER, info.start, (size>>9)+1, (uint8*)buf)){
			//check crc
			if(_chkEcc(buf, size)){
				printf("NVITEM partId%x:%s read success!\n",_ramdiskCfg[idx].partId,firstName);
				return 1;
			}
			printf("NVITEM partId%x:%s ECC error!\n",_ramdiskCfg[idx].partId,firstName);
		}
	}
	printf("NVITEM partId%x:%s read error!\n",_ramdiskCfg[idx].partId,firstName);
// 2 read bakup image
	memset(buf, 0xFF, size);
	if(get_partition_info_by_name(s_block_dev, secondName, &info)){
		printf("NVITEM partId%x:%s read error!\n",_ramdiskCfg[idx].partId,secondName);
		return 1;
	}
	if(!Emmc_Read(PARTITION_USER, info.start, (size>>9), (uint8*)buf))
	{
		printf("NVITEM partId%x:%s read error!\n",_ramdiskCfg[idx].partId,secondName);
	}
	if(!_chkEcc(buf, size)){
		printf("NVITEM partId%x:%s ECC error!\n",_ramdiskCfg[idx].partId,secondName);
		return 1;
	}

	if(!get_partition_info_by_name(s_block_dev, firstName, &info)){
		Emmc_Write(PARTITION_USER, info.start, (size>>9), (uint8*)buf);
	}

	printf("NVITEM  partId%x:%s read success!\n",_ramdiskCfg[idx].partId,secondName);
	return 1;

}


/*
	1 get Ecc first
	2 write  imageBakPath
	3 write imagePath

	note:
		first imageBakPath then imagePath
*/
BOOLEAN		ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	int idx;
	BOOLEAN oriRet,bakRet;
	disk_partition_t info;

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
	if(!s_block_dev){
		printf("NVITEM partId%x:image write fail!\n",_ramdiskCfg[idx].partId);
		return 0;
	}
// 1 get Ecc
	_makEcc( buf, size);
// 2 write bakup image
	bakRet = 0;
	if(!get_partition_info_by_name(s_block_dev, _ramdiskCfg[idx].imageBak_path, &info)){
		if(Emmc_Write(PARTITION_USER, info.start, (size>>9), (uint8*)buf)){
			bakRet = 1;
		}
	}
	if(!bakRet){
		printf("NVITEM partId%x:bakup image write fail!\n",_ramdiskCfg[idx].partId);
	}
// 3 write origin image
	oriRet = 0;
	if(!get_partition_info_by_name(s_block_dev, _ramdiskCfg[idx].image_path, &info)){
		if(Emmc_Write(PARTITION_USER, info.start, (size>>9), (uint8*)buf)){
			oriRet = 1;
		}
	}
	if(!oriRet){
		printf("NVITEM partId%x:origin image write fail!\n",_ramdiskCfg[idx].partId);
	}
	printf("NVITEM partId%x:image write finished %d!\n",_ramdiskCfg[idx].partId,(bakRet && oriRet));
	return (bakRet && oriRet);

}

void		ramDisk_Close(RAMDISK_HANDLE handle)
{
	return;
}

#else

static RAM_NV_CONFIG _ramdiskCfg[RAMNV_NUM+1] = 
{
	{3,     "/productinfo/productinfo.bin", "/productinfo/productinfobkup.bin",     0x4000  },
	{1,	 "/fixnv/fixnv.bin",			 "/backupfixnv/fixnv.bin",		0x20000	},
	{2,	"/runtimenv/runtimenv.bin",	"/runtimenv/runtimenvbkup.bin",	0x40000	},
        {0,	"",	"",						0		},
};

static void _getPath(/*IN*/char *totalPath, /*OUT*/char *path)
{
	char ch;
	strcpy(path,totalPath);

	ch = path[strlen(path)-1];
	while(ch&&('/' != ch)&&('\\' != ch))
	{
		path[strlen(path)-1] = '\0';
		ch = path[strlen(path)-1];
	}
}

const RAM_NV_CONFIG*	ramDisk_Init(void)
{
	return _ramdiskCfg;
}

RAMDISK_HANDLE	ramDisk_Open(uint32 partId)
{
	return (RAMDISK_HANDLE)partId;
}

int _getIdx(RAMDISK_HANDLE handle)
{
	uint32 i;
	uint32 partId = (uint32)handle;

	for(i = 0; i < sizeof(_ramdiskCfg)/sizeof(RAM_NV_CONFIG); i++){
		if(0 == _ramdiskCfg[i].partId){
			return -1;
		}
		else if(partId == _ramdiskCfg[i].partId){
			return i;
		}
	}
	return -1;
}

/*
	1 read imagPath first, if succes return , then
	2 read imageBakPath, if fail , return, then
	3 fix imagePath

	note:
		first imagePath then imageBakPath
*/
BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	int idx;
	char path[100];
	char *firstName, *secondName;

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
// 0 get read order
	if(1){
		firstName = _ramdiskCfg[idx].image_path;
		secondName = _ramdiskCfg[idx].imageBak_path;
	}
	else{
		secondName = _ramdiskCfg[idx].image_path;
		firstName = _ramdiskCfg[idx].imageBak_path;
	}
// 1 read origin image
	memset(buf, 0xFF, size);
	_getPath(firstName,path);
	cmd_yaffs_mount(path);
	cmd_yaffs_mread_file(firstName, buf);
//	cmd_yaffs_umount(path);

	//check crc
	if(_chkEcc(buf, size)){
		printf("NVITEM partId%x:%s read success!\n",_ramdiskCfg[idx].partId,firstName);
		return 1;
	}
	printf("NVITEM partId%x:%s ECC error......!\n",_ramdiskCfg[idx].partId,firstName);
// 2 read bakup image
	memset(buf, 0xFF, size);
	_getPath(secondName,path);
	printf("secondName : %s path: %s\n",secondName,path);
//	cmd_yaffs_mount(path);
	cmd_yaffs_mread_file(secondName, buf);
//	cmd_yaffs_umount(path);

	if(!_chkEcc(buf, size)){
		printf("NVITEM partId%x:%s ECC error!\n",_ramdiskCfg[idx].partId,secondName);
		return 1;
	}

	_getPath(firstName,path);
	cmd_yaffs_mount(path);
	cmd_yaffs_mwrite_file(firstName, (char*)buf, _ramdiskCfg[idx].image_size);
	cmd_yaffs_umount(path);

	printf("NVITEM  partId%x:%s read success!\n",_ramdiskCfg[idx].partId,secondName);
	return 1;

}


/*
	1 get Ecc first
	2 write  imageBakPath
	3 write imagePath

	note:
		first imageBakPath then imagePath
*/
BOOLEAN		ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	int idx;
	char path[100];

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
// 1 get Ecc
	_makEcc( buf, size);
// 2 write bakup image
	_getPath(_ramdiskCfg[idx].imageBak_path,path);
	cmd_yaffs_mount(path);
	cmd_yaffs_mwrite_file(_ramdiskCfg[idx].imageBak_path, (char*)buf, _ramdiskCfg[idx].image_size);
	cmd_yaffs_umount(path);
// 3 write origin image
	_getPath(_ramdiskCfg[idx].image_path,path);
	cmd_yaffs_mount(path);
	cmd_yaffs_mwrite_file(_ramdiskCfg[idx].image_path, (char*)buf, _ramdiskCfg[idx].image_size);
	cmd_yaffs_umount(path);
	return 1;

}

void		ramDisk_Close(RAMDISK_HANDLE handle)
{
	return;
}

#endif

