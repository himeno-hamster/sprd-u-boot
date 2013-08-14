#include "nvitem_common.h"
#include "nvitem_fs.h"
#include "nvitem_config.h"
#ifdef CONFIG_SC7710G2
#include "special_nvitemd.h"
#endif

#define CRC_16_L_SEED   0x80
#define CRC_16_L_POLYNOMIAL 0x8000
#define CRC_16_POLYNOMIAL  0x1021
unsigned short __crc_16_l_calc (uint8 *buf_ptr,uint32 len)
{
    unsigned int i;
    unsigned short crc = 0;

    while (len--!=0) {
        for (i = CRC_16_L_SEED; i !=0 ; i = i>>1){
            if ( (crc & CRC_16_L_POLYNOMIAL) !=0){
                crc = crc << 1 ;
                crc = crc ^ CRC_16_POLYNOMIAL;
            }
            else {
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
BOOLEAN _chkEcc(uint8* buf, uint32 size)
{
#ifdef CONFIG_SC7710G2
    int ret = 0;
    ret = file_check(buf,size);
    if(ret == -1){
        return 0;
    }else{
        return 1;
    }
#else
    uint16 crc,crcOri;
    // crc = __crc_16_l_calc(buf, size-2);
    // crcOri = (uint16)((((uint16)buf[size-2])<<8) | ((uint16)buf[size-1]) );

    crc = calc_checksum(buf,size-4);
    crcOri = (uint16)((((uint16)buf[size-3])<<8) | ((uint16)buf[size-4]) );

    return (crc == crcOri);
#endif
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
    {1, PARTITION_FIX_NV1,  PARTITION_FIX_NV2,  0x20000 },
    {2, PARTITION_RUNTIME_NV1, PARTITION_RUNTIME_NV2, 0x40000 },
    {0, "",      "",      0  }
};

const RAM_NV_CONFIG* ramDisk_Init(void)
{
    s_block_dev = get_dev("mmc", 1);
    return _ramdiskCfg;
}

RAMDISK_HANDLE ramDisk_Open(uint32 partId)
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
BOOLEAN  ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
    int idx;
    disk_partition_t info;
    int firstName, secondName;

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
    if(!get_partition_info(s_block_dev, firstName, &info)){
        if(Emmc_Read(PARTITION_USER, info.start, (size>>9)+1, (uint8*)buf)){
            //check crc
            if(_chkEcc(buf, size)){
                printf("NVITEM partId%x:%x read success!\n",_ramdiskCfg[idx].partId,firstName);
                return 1;
            }
            printf("NVITEM partId%x:%x ECC error!\n",_ramdiskCfg[idx].partId,firstName);
        }
    }
    printf("NVITEM partId%x:%x read error!\n",_ramdiskCfg[idx].partId,firstName);
    // 2 read bakup image
    memset(buf, 0xFF, size);
    if(get_partition_info(s_block_dev, secondName, &info)){
        printf("NVITEM partId%x:%x read error!\n",_ramdiskCfg[idx].partId,secondName);
        return 1;
    }
    if(!Emmc_Read(PARTITION_USER, info.start, (size>>9), (uint8*)buf))
    {
        printf("NVITEM partId%x:%x read error!\n",_ramdiskCfg[idx].partId,secondName);
    }
    if(!_chkEcc(buf, size)){
        printf("NVITEM partId%x:%x ECC error!\n",_ramdiskCfg[idx].partId,secondName);
        return 1;
    }

    if(!get_partition_info(s_block_dev, firstName, &info)){
        Emmc_Write(PARTITION_USER, info.start, (size>>9), (uint8*)buf);
    }

    printf("NVITEM  partId%x:%x read success!\n",_ramdiskCfg[idx].partId,secondName);
    return 1;

}


/*
   1 get Ecc first
   2 write  imageBakPath
   3 write imagePath

note:
first imageBakPath then imagePath
 */
BOOLEAN  ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
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
    if(!get_partition_info(s_block_dev, _ramdiskCfg[idx].imageBak_path, &info)){
        if(Emmc_Write(PARTITION_USER, info.start, (size>>9), (uint8*)buf)){
            bakRet = 1;
        }
    }
    if(!bakRet){
        printf("NVITEM partId%x:bakup image write fail!\n",_ramdiskCfg[idx].partId);
    }
    // 3 write origin image
    oriRet = 0;
    if(!get_partition_info(s_block_dev, _ramdiskCfg[idx].image_path, &info)){
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

void  ramDisk_Close(RAMDISK_HANDLE handle)
{
    return;
}

#else

static RAM_NV_CONFIG _ramdiskCfg[RAMNV_NUM+1] =
{
#ifdef CONFIG_SC7710G2
    {1,  "/fixnv/fixnv.bin",    "/fixnv/fixnvbkup.bin",  0x20000 },
    {2, "/runtimenv/runtimenv.bin", "/runtimenv/runtimenvbkup.bin", 0x40000 },
    {3,     "/productinfo/productinfo.bin", "/productinfo/productinfobkup.bin",     0x4000  },
#else
    {3,     "/productinfo/productinfo.bin", "/productinfo/productinfobkup.bin",     0x4000  },
    {1,  "/fixnv/fixnv.bin",    "/backupfixnv/fixnv.bin",  0x20000 },
    {2, "/runtimenv/runtimenv.bin", "/runtimenv/runtimenvbkup.bin", 0x40000 },
#endif
    {0, "", "",      0  },
};
static int fixnv_mounted = 0;
static int backupfixnv_mounted = 0;
static int runtimenv_mounted = 0;
static int productinfo_mounted = 0;
static int ensure_path_mounted(const char * mountpoint){
    printf("%s::mountpoint == %s\n",__func__,mountpoint);
    if(!strcmp("/backupfixnv/",mountpoint)){
        if(backupfixnv_mounted == 1){
            goto MOUNT_OK;
        }else{
            backupfixnv_mounted = 1;
            goto TRY_MOUNT;
        }
    }else if(!strcmp("/fixnv/",mountpoint)){
        if(fixnv_mounted == 1){
            goto MOUNT_OK;
        }else{
            fixnv_mounted = 1;
            goto TRY_MOUNT;
        }
    }else if(!strcmp("/runtimenv/",mountpoint)){
        if(runtimenv_mounted == 1){
            goto MOUNT_OK;
        }else{
            runtimenv_mounted = 1;
            goto TRY_MOUNT;
        }
    }else if(!strcmp("/productinfo/",mountpoint)){
        if(productinfo_mounted == 1){
            goto MOUNT_OK;
        }else{
            productinfo_mounted = 1;
            goto TRY_MOUNT;
        }
    }else{
        goto TRY_MOUNT;
    }


TRY_MOUNT:
    printf("%s::trying mount ...\n",__func__);
    cmd_yaffs_mount(mountpoint);
MOUNT_OK:
    printf("%s::exit\n",__func__);
    return 0;
}
static int ensure_path_umounted(const char * mountpoint){
    printf("%s::mountpoint == %s\n",__func__,mountpoint);
    if(!strcmp("/backupfixnv/",mountpoint)){
        if(backupfixnv_mounted == 0){
            goto UMOUNT_OK;
        }else{
            backupfixnv_mounted = 0;
            goto TRY_UMOUNT;
        }
    }else if(!strcmp("/fixnv/",mountpoint)){
        if(fixnv_mounted == 0){
            goto UMOUNT_OK;
        }else{
            fixnv_mounted = 0;
            goto TRY_UMOUNT;
        }
    }else if(!strcmp("/runtimenv/",mountpoint)){
        if(runtimenv_mounted == 0){
            goto UMOUNT_OK;
        }else{
            runtimenv_mounted = 0;
            goto TRY_UMOUNT;
        }
    }else if(!strcmp("/productinfo/",mountpoint)){
        if(productinfo_mounted == 0){
            goto UMOUNT_OK;
        }else{
            productinfo_mounted = 0;
            goto TRY_UMOUNT;
        }
    }else{
        goto TRY_UMOUNT;
    }


TRY_UMOUNT:
    printf("%s::trying mount ...\n",__func__);
    cmd_yaffs_umount(mountpoint);
UMOUNT_OK:
    printf("%s::exit\n",__func__);
    return 0;
}

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

const RAM_NV_CONFIG* ramDisk_Init(void)
{
    return _ramdiskCfg;
}

RAMDISK_HANDLE ramDisk_Open(uint32 partId)
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
BOOLEAN  ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
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
    ensure_path_mounted(path);
    cmd_yaffs_mread_file(firstName, buf);

    //check crc
    if(_chkEcc(buf, size)){
        printf("NVITEM partId%x:%s read success!\n",_ramdiskCfg[idx].partId,firstName);
        ensure_path_umounted(path);
        return 1;
    }
    printf("NVITEM partId%x:%s ECC error......!\n",_ramdiskCfg[idx].partId,firstName);
    // 2 read bakup image
    memset(buf, 0xFF, size);
    _getPath(secondName,path);
    printf("secondName : %s path: %s\n",secondName,path);
    ensure_path_mounted(path);
    cmd_yaffs_mread_file(secondName, buf);

    if(!_chkEcc(buf, size)){
        printf("NVITEM partId%x:%s ECC error!\n",_ramdiskCfg[idx].partId,secondName);
        memset(buf, 0xFF, size);
        ensure_path_umounted(path);
        return 1;
    }

    _getPath(firstName,path);
    ensure_path_mounted(path);
    cmd_yaffs_mwrite_file(firstName, (char*)buf, _ramdiskCfg[idx].image_size);

    printf("NVITEM  partId%x:%s read success!\n",_ramdiskCfg[idx].partId,secondName);
    ensure_path_umounted(path);
    return 1;

}


/*
   1 get Ecc first
   2 write  imageBakPath
   3 write imagePath

note:
first imageBakPath then imagePath
 */
BOOLEAN     try_read_back(const char* path,const char * filename,uint8* buf,uint32 size){
    memset(buf, 0xFF, size);
    ensure_path_mounted(path);
    cmd_yaffs_mread_file(filename, buf);
    if(_chkEcc(buf, size)){
        printf("read back sucess!\n");
        return 1;
    }
    return 0;
}

BOOLEAN  ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
    int idx;
    char path[100];

    idx = _getIdx(handle);
    if(-1 == idx){
        return 0;
    }
    // 1 get Ecc
#ifdef CONFIG_SC7710G2
    if(handle == RAMBSD_FIXNV_ID){
        nvitemd_add_fixnv_len(buf,size);
    }
#endif
    _makEcc( buf, size);
#ifdef CONFIG_SC7710G2
    nvitemd_add_crc16(buf,size);
#endif
    // 2 write bakup image
    _getPath(_ramdiskCfg[idx].imageBak_path,path);
    ensure_path_mounted(path);
    cmd_yaffs_mwrite_file(_ramdiskCfg[idx].imageBak_path, (char*)buf, _ramdiskCfg[idx].image_size);
#ifdef CONFIG_SC7710G2
    while(0 == try_read_back(path,_ramdiskCfg[idx].imageBak_path,buf,size)){
        printf("Fatal error,readback failed!\n");
    }
#endif
    // 3 write origin image
    _getPath(_ramdiskCfg[idx].image_path,path);
    ensure_path_mounted(path);
    cmd_yaffs_mwrite_file(_ramdiskCfg[idx].image_path, (char*)buf, _ramdiskCfg[idx].image_size);
#ifdef CONFIG_SC7710G2
    while(0 == try_read_back(path,_ramdiskCfg[idx].image_path,buf,size)){
        printf("Fatal error,readback failed!\n");
    }
#endif


#ifdef CONFIG_SC7710G2
    if(idx == 0){
        printf("update /backupfixnv/fixnv.bin ...\n");
        ensure_path_mounted("/backupfixnv/");
        cmd_yaffs_mwrite_file("/backupfixnv/fixnv.bin", (char*)buf, _ramdiskCfg[idx].image_size);
        while(0 == try_read_back("/backupfixnv/","/backupfixnv/fixnv.bin",buf,size)){
            printf("Fatal error,readback failed!\n");
        }
        ensure_path_umounted("/backupfixnv/");
    }
#endif

    ensure_path_umounted(path);
    return 1;

}

void  ramDisk_Close(RAMDISK_HANDLE handle)
{
    return;
}

#endif

