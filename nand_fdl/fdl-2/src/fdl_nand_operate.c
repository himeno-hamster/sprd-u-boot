#include <asm/arch/packet.h>
#include "fdl_conf.h"
#include "parsemtdparts.h"
#include "asm/arch/nand_controller.h"
#include <linux/mtd/mtd.h>
#include <linux/mtd/ubi.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <malloc.h>
#include <ubi_uboot.h>
#include "fdl_ubi.h"
#include "fdl_common.h"
#include "fdl_nand_operate.h"

typedef struct {
	char *vol;
	char *bakvol;
}dl_nv_info_t;

typedef struct {
	char *name;  //mtd partition name
	unsigned long long size;  //mtd part size
	unsigned long long rw_point;  //nand flash read/write offset point
}dl_mtd_info_t;

typedef struct {
	struct ubi_device *dev;
	int dev_num;
	char *cur_volnm;
	struct ubi_volume_desc *cur_voldesc;
}dl_ubi_info_t;

typedef enum{
	PART_TYPE_MIN,
	PART_TYPE_MTD,
	PART_TYPE_UBI,
	PART_TYPE_MAX
}dl_part_type;

typedef struct fdl_download_status
{
	nand_info_t *nand;
	dl_part_type part_type;
	dl_mtd_info_t mtd;
	dl_ubi_info_t ubi;
	unsigned long total_dl_size;  //size to be recvd
	unsigned long recv_size;  //recv size from tool
	unsigned long unsv_size; //data unsaved in buf
	char *buf;  //buf for data received from tool
	unsigned int bufsize;  //max buf size
	unsigned int rp; //read point of buf,no use now
	unsigned int wp;  //write point of buf
}dl_status_t;

typedef struct{
	char name[UBI_VOL_NAME_MAX+1];
	long long size;//size in byte
	int autoresize;
}fdl_ubi_vtbl_t;

#define UBIFS_NODE_MAGIC  0x06101831
#define AUTO_RESIZE_FLAG  0xFFFFFFFF
#define FDL_BUF_LEN  (1*1024*1024)
static dl_status_t dl_stat={0};
static char fdl_buf[FDL_BUF_LEN];
extern struct ubi_selected_dev cur_ubi;
static dl_nv_info_t s_nv_backup_cfg[]={
	{"fixnv1",			"fixnv2"},
	{"runtimenv1",	"runtimenv2"},
	{"tdfixnv1",		"tdfixnv2"},
	{"tdruntimenv1",	"tdruntimenv2"},
	{"wfixnv1",		"wfixnv2"},
	{"wruntimenv1",	"wruntimenv2"},
	{"wcnfixnv1",		"wcnfixnv2"},
	{"wcnruntimenv1",	"wcnruntimenv2"},
	{NULL,NULL}
};

static __inline void _send_rsp(unsigned long err)
{
    FDL_SendAckPacket(convert_err(err));
}

static struct mtd_info* _get_cur_nand(void)
{
	if ((nand_curr_device < 0) || (nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE))
	{
		printf("--->get current nand failed<---\n");
		_send_rsp(NAND_UNKNOWN_DEVICE);
		return NULL;
	}
	return &nand_info[nand_curr_device];
}

/**
 * check whether is a nv volume.
 * return backup nv volume in case of true, otherwise null.
 */
static char* _is_nv_volume(char *volume)
{
	int i;

	for(i=0; s_nv_backup_cfg[i].vol !=NULL; i++) {
		if(0 == strcmp(volume, s_nv_backup_cfg[i].vol)) {
			return s_nv_backup_cfg[i].bakvol;
		}
	}
	return NULL;
}

/**
 * parse mtd partitions from a string.
 */
static int _parse_mtd_partitions(void)
{
	struct mtd_info *nand = NULL;

	nand = _get_cur_nand();
	if(!nand)
		return 0;
	parse_cmdline_partitions(nand->size);
	return 1;
}

/**
 * get mtd partition info from partition table parsed.
 */
static int _get_mtd_partition_info(char *name, struct mtd_partition *part)
{
	int ret;

	if(!_parse_mtd_partitions())
		return 0;

	ret = parse_mtd_part_info(name, part);
	if(ret != 1)
		return 0;

	return 1;	
}

/**
 * parse volume table config, just compatible with dl tool.
 */
static void _fdl2_parse_volume_cfg(unsigned short* vol_cfg, unsigned short total_num, fdl_ubi_vtbl_t *vtbl)
{
	int i, j;
	long long size;

	/*Decode String: Partition Name(72Byte)+SIZE(4Byte)+...*/
	for(i=0;i<total_num;i++)
	{
		size = *(unsigned long *)(vol_cfg+38*(i+1)-2);
		//the partition size received from tool is MByte.
		if(AUTO_RESIZE_FLAG == size){
			vtbl[i].size = 1024*1024;//just set size as 1M when autoresize flag enable
			vtbl[i].autoresize = 1;
		}
		else
			vtbl[i].size = 1024*1024*size;

		for(j=0;j<36;j++)
		{
			//convert name wchar to char with violent
			vtbl[i].name[j] = *(vol_cfg+38*i+j) & 0xFF;
		}

		printf("volume name:%s,size:0x%llx,autoresize flag:%d\n",vtbl[i].name,vtbl[i].size,vtbl[i].autoresize);
	}
	return;
}

/**
 * update the nv volume with nv header.
 */
static int _fdl2_update_nv(char *vol, char *bakvol, int size, char *data)
{
	int ret,time=1;
	char *buf;
	char *nvbuf=NULL;
	char *curvol;
	char tmp[NV_HEAD_LEN];
	nv_header_t *header;

	if(size>dl_stat.bufsize){
		nvbuf = malloc(size+NV_HEAD_LEN);
		if(!nvbuf){
			printf("%s buf malloc failed.\n",__func__);
			return -1;
		}
		buf = nvbuf;
		fdl_ubi_volume_read(dl_stat.ubi.dev, dl_stat.ubi.cur_volnm, buf, size, 0);
		if(size != ret) {
			printf("%s read volume %s failed.\n",__func__,dl_stat.ubi.cur_volnm);
			goto err;
		}
	}else {
		buf = data;
	}

	memset(tmp,0x0,NV_HEAD_LEN);
	header = (nv_header_t *)tmp;
	header->magic = NV_HEAD_MAGIC;
	header->version = NV_VERSION;
	header->len = size;
	header->checksum = fdl_calc_checksum(buf,size);
	curvol = vol;
	do{
		ret = fdl_ubi_volume_start_update(dl_stat.ubi.dev, curvol, size+NV_HEAD_LEN);
		if(ret) {
			printf("%s: vol %s start update failed!\n",__func__,curvol);
			goto err;
		}
		ret = fdl_ubi_volume_write(dl_stat.ubi.dev, curvol, tmp, NV_HEAD_LEN);
		if(ret) {
			printf("%s volume write error %d!\n",curvol,ret);
			goto err;
		}
		ret = fdl_ubi_volume_write(dl_stat.ubi.dev, curvol, buf, size);
		if(ret) {
			printf("%s volume write error %d!\n",curvol,ret);
			goto err;
		}
		curvol = bakvol;
	}while(time--);

	printf("update nv success!\n");
	return 0;
err:
	if(nvbuf)
		free(nvbuf);
	return -1;
}

static int _fdl2_check_nv(char *vol)
{
	char *buf,*bakbuf,*bakvol;
	nv_header_t *header;
	int ret = NAND_SYSTEM_ERROR;
	int size = FIXNV_SIZE+NV_HEAD_LEN;
	//read org image
	buf = malloc(size);
	if(!buf){
		printf("%s buf malloc failed.\n",__func__);
		goto err;
	}

	ret = fdl_ubi_volume_read(dl_stat.ubi.dev, 
							vol,
							buf,
							size,
							0);
	if(size != ret){
		printf("%s can read 0x%x data from vol %s!!\n",__func__,size,vol);
	}
	header = (nv_header_t *)buf;
	ret = fdl_check_crc(buf+NV_HEAD_LEN, header->len, header->checksum);
	free(buf);
	if(ret)
		return 0;
	//read backup image needed
	bakbuf = malloc(size);
	if(!bakbuf){
		printf("%s buf malloc failed.\n",__func__);
		goto err;
	}

	ret = fdl_ubi_volume_read(dl_stat.ubi.dev, 
							_is_nv_volume(vol),
							bakbuf,
							size,
							0);
	if(size != ret){
		printf("%s can read 0x%x data from bakup of %s!!!\n",__func__,size,vol);
	}
	header = (nv_header_t *)bakbuf;
	ret = fdl_check_crc(bakbuf+NV_HEAD_LEN, header->len, header->checksum);
	if(!ret){
		printf("%s both org and bak nv is crash.\n",__func__);
		free(bakbuf);
		goto err;
	}
	//restore org image
	fdl_ubi_volume_start_update(dl_stat.ubi.dev, vol, size);
	ret = fdl_ubi_volume_write(dl_stat.ubi.dev, vol, bakbuf, size);
	if(ret){
		printf("%s ubi volume write error %d!\n",__func__,ret);
		free(bakbuf);
		goto err;
	}
	free(bakbuf);
	return 0;
err:
	//_send_rsp(NAND_SYSTEM_ERROR);
	return -1;
}

/**
 * check whether the new volumes table same with original.
 * return 0 in case of diff and 1 in case of same.
 */
static int _fdl2_vtbl_check(fdl_ubi_vtbl_t *vtbl, int total_vols)
{
	int i,j;
	int same=0;
	struct ubi_volume *vol;
	struct ubi_device *ubi = cur_ubi.dev;

	if(!cur_ubi.ubi_initialized){
		printf("%s:ubi init failed.\n",__FUNCTION__);
		return 0;
	}

	if(total_vols != (ubi->vol_count-UBI_INT_VOL_COUNT)){
		printf("%s:new vol count is %d,old vol count is %d.\n",__FUNCTION__,total_vols,ubi->vol_count-UBI_INT_VOL_COUNT);
		return 0;
	}

	for(i=0;i<total_vols;i++){
		same = 0;
		for (j = 0; j < ubi->vtbl_slots; j++) {
			vol = ubi->volumes[j];
			if (vol && !strcmp(vol->name, vtbl[i].name)) {
				printf("Volume \"%s\" found at volume id %d.\n", vtbl[i].name, j);
				if(vtbl[i].autoresize){
					//autoresize_vol_id flag will set -1 after ubi attach, so we can't get it.
					same = 1;
					printf("-->debug: old auto resize id %d.\n",ubi->autoresize_vol_id);
				}else{
					int new_rsvd_pebs=0;
					uint64_t bytes = vtbl[i].size;
					if (do_div(bytes, vol->usable_leb_size))
						new_rsvd_pebs = 1;
					new_rsvd_pebs += bytes;
					if(new_rsvd_pebs == vol->reserved_pebs)
						same = 1;
					else
						printf("reserved pebs not same,new %d,old %d.\n",new_rsvd_pebs,vol->reserved_pebs);
				}
				break;
			}
		}
		if (!same) {
			printf("%s volume not eq.\n", vtbl[i].name);
			return 0;
		}
	}

	return 1;
}
/**
 * nand write and dl_stat update.
 */
static void _fdl2_nand_write(nand_info_t *nand, unsigned long long offset, unsigned long length,
			char *buffer)
{
	int ret;

	//printf("fdl_nand_write:offset:0x%llx,len:0x%x\n",offset,length);

	//TODO:temp here for step 1 debug
	if(strcmp(dl_stat.mtd.name, "spl")==0){
		sprd_nand_write_spl(buffer, dl_stat.nand);
		printf("write spl\n");
		dl_stat.wp =0;
		dl_stat.unsv_size =0;
		return;
	}

	switch(dl_stat.part_type){
		case PART_TYPE_MTD:
			ret = nand_write_skip_bad(nand, offset, &length, buffer);
			dl_stat.unsv_size -= length;
			memmove(dl_stat.buf, dl_stat.buf+length, dl_stat.unsv_size);
			dl_stat.wp -= length;
			dl_stat.mtd.rw_point += length;
			if(ret){
				//mark a block as badblock
				printf("nand write error %d, mark bad block 0x%llx\n",ret,dl_stat.mtd.rw_point&~(nand->erasesize-1));
				nand->block_markbad(nand,dl_stat.mtd.rw_point&~(nand->erasesize-1));
			}
			break;
		case PART_TYPE_UBI:
			ret = fdl_ubi_volume_write(dl_stat.ubi.dev, dl_stat.ubi.cur_volnm, buffer, length);
			if(ret){
				printf("ubi volume write error %d!\n",ret);
				_send_rsp(NAND_SYSTEM_ERROR);
			}
			dl_stat.unsv_size -= length;
			memmove(dl_stat.buf, dl_stat.buf+length, dl_stat.unsv_size);
			dl_stat.wp -= length;
			break;
		default:
			printf("%s: part type error!\n",__FUNCTION__);
			return;
	}

	return;
}

/**
 * erase the given mtd part.
 * return 0 in case of success.
 */
static int _fdl2_mtd_part_erase(char *name)
{
	int ret;
	struct mtd_partition mtd_part;
	nand_erase_options_t opts;

	ret = _get_mtd_partition_info(name, &mtd_part);
	if(ret){
		memset(&opts, 0, sizeof(opts));
		opts.offset = mtd_part.offset;
		opts.length = mtd_part.size;
		opts.quiet  = 1;
		ret = nand_erase_opts(_get_cur_nand(), &opts);
		if(!ret)
			return 0;
		printf("%s:nand erase %s failure %d\n",__FUNCTION__, name, ret);
	}else
		printf("%s:Can't find part %s",__FUNCTION__,name);
	return 1;
}

/**
 * parse the given part is mtd partition or ubi volume.
 */
static void _fdl2_part_info_parse(char *part)
{
	int ret;
	struct mtd_partition mtd_part;
	struct ubi_volume_desc *vol;

	ret = _get_mtd_partition_info(part, &mtd_part);
	if(ret){
		dl_stat.mtd.name = part;
		dl_stat.mtd.size = mtd_part.size;
		dl_stat.mtd.rw_point = mtd_part.offset;
		dl_stat.part_type = PART_TYPE_MTD;
		return;
	}

	vol = ubi_open_volume_nm(cur_ubi.dev_num, part, UBI_READWRITE);
	if (IS_ERR(vol)){
		printf("cannot open volume \"%s\", error %d\n",part, (int)PTR_ERR(vol));
	}else{
		dl_stat.ubi.dev = cur_ubi.dev;
		dl_stat.ubi.dev_num = cur_ubi.dev_num;
		dl_stat.ubi.cur_volnm = part;
		dl_stat.ubi.cur_voldesc = vol;
		dl_stat.part_type = PART_TYPE_UBI;
		return;
	}
	dl_stat.part_type = PART_TYPE_MAX;
	printf("Can't find part %s.\n",part);
	return;
}

/**
 * fdl2_download_start
 *
 * Get download info from download start command which  
 * will used in next step
 *
 * @param part partition/volume name
 * @param size total download size
 * @param nv_checksum NA
 * @return 0 failed
 *             1 success
 */
int fdl2_download_start(char* name, unsigned long size, unsigned long nv_checksum)
{
	int ret;

	memset(&dl_stat, 0x0, sizeof(dl_status_t));

	_fdl2_part_info_parse(name);

	switch(dl_stat.part_type){
		case PART_TYPE_MTD:
			if(size > dl_stat.mtd.size){
				printf("%s:dl size 0x%x > partition size 0x%llx\n",__FUNCTION__,size,dl_stat.mtd.size);
				ret = NAND_INVALID_SIZE;
				goto err;
			}
			ret = _fdl2_mtd_part_erase(name);
			if(ret){
				printf("%s:mtd %d erase failed!\n",__FUNCTION__,name);
				ret = NAND_SYSTEM_ERROR;
				goto err;
			}
			break;
		case PART_TYPE_UBI:
			if(size > dl_stat.ubi.cur_voldesc->vol->used_bytes){
				printf("dl size > partition size!\n");
				ret = NAND_INVALID_SIZE;
				goto err;
			}
			ret = fdl_ubi_volume_start_update(dl_stat.ubi.dev, name, size);
			if(ret){
				printf("%s: vol %s start update failed!\n",__FUNCTION__,name);
				ret = NAND_SYSTEM_ERROR;
				goto err;
			}
			break;
		default:
			ret = NAND_INCOMPATIBLE_PART;
			goto err;
	}

	dl_stat.nand = _get_cur_nand();
	dl_stat.total_dl_size = size;
	dl_stat.recv_size = 0;
	dl_stat.unsv_size = 0;
	dl_stat.buf = fdl_buf;
	dl_stat.bufsize = FDL_BUF_LEN;
	dl_stat.rp = 0;
	dl_stat.wp = 0;

	printf("fdl2_download_start:part:%s,size:0x%x\n",name,size);

	_send_rsp(NAND_SUCCESS);
	return 1;
err:
	_send_rsp(ret);
	return 0;
}

/**
 * fdl2_download_midst
 *
 * Save data to fdl buf and finally write it to nand flash
 *
 * @param size total download size
 * @param buf data recvd
 * @return 0 failed
 *             1 success
 */
int fdl2_download_midst(unsigned short size, char *buf)
{
	int ret =0;
	unsigned int cpy_len=0,unsv_len=0;
	unsigned long len;
	nand_info_t *nand;

	nand = dl_stat.nand;
	dl_stat.recv_size += size;
	unsv_len = size;
	while((dl_stat.unsv_size+unsv_len)> dl_stat.bufsize)
	{
		len = dl_stat.unsv_size;
		len = len & ~(nand->erasesize - 1);

		_fdl2_nand_write(nand, dl_stat.mtd.rw_point, len, dl_stat.buf);

		cpy_len = dl_stat.bufsize - dl_stat.unsv_size;
		cpy_len = (unsv_len>cpy_len)?cpy_len:unsv_len;
		memcpy(dl_stat.buf+dl_stat.wp, &buf[size-unsv_len], cpy_len);
		unsv_len -= cpy_len;
		dl_stat.wp += cpy_len;
		dl_stat.unsv_size += cpy_len;
	}

	//copy data to dl buf
	memcpy(dl_stat.buf+dl_stat.wp, &buf[size-unsv_len], unsv_len);
	dl_stat.wp += unsv_len;
	dl_stat.unsv_size += unsv_len;

	if(dl_stat.recv_size == dl_stat.total_dl_size){
		len = dl_stat.unsv_size;
		_fdl2_nand_write(nand, dl_stat.mtd.rw_point, len, dl_stat.buf);
	}

	_send_rsp(NAND_SUCCESS);
	return 1;
}

/**
 * fdl2_download_end
 *
 * Set download end
 *
 * @param void
 * @return 0 failed
 *             1 success
 */
int fdl2_download_end(void)
{
	int i=0;
	int ret = NAND_SUCCESS;

	while(0 != dl_stat.unsv_size){
		_fdl2_nand_write(dl_stat.nand, dl_stat.mtd.rw_point, dl_stat.unsv_size, dl_stat.buf);
		if(i++>100){
			printf("download end write error, try 100 times.\n");
			_send_rsp(NAND_SYSTEM_ERROR);
			return 0;
		}
	}
	//close opened ubi volume
	if(PART_TYPE_UBI == dl_stat.part_type){
		int err;
		char *bakvol;
		bakvol = _is_nv_volume(dl_stat.ubi.cur_volnm);
		if(bakvol) {
			err = _fdl2_update_nv(dl_stat.ubi.cur_volnm, 
							bakvol,
							dl_stat.total_dl_size,
							dl_stat.buf);
			if(err)
				ret = NAND_SYSTEM_ERROR;
		}
		ubi_close_volume(dl_stat.ubi.cur_voldesc);
	}

	_send_rsp(ret);
	return 1;
}

/**
 * fdl2_read_start
 *
 * Get partition/volume info from read start command which  
 * will used in next step
 *
 * @param part partition/volume name
 * @param size total size
 * @return 0 failed
 *             1 success
 */
int fdl2_read_start(char* part, unsigned long size)
{
	int ret;

	memset(&dl_stat, 0x0, sizeof(dl_status_t));

	_fdl2_part_info_parse(part);

	switch(dl_stat.part_type){
		case PART_TYPE_MTD:
			if(size > dl_stat.mtd.size){
				printf("%s:read size 0x%x > partition size 0x%llx\n",__FUNCTION__,size,dl_stat.mtd.size);
				ret = NAND_INVALID_SIZE;
				goto err;
			}
			break;
		case PART_TYPE_UBI:
			if(size > dl_stat.ubi.cur_voldesc->vol->used_bytes){
				printf("%s:read size 0x%x > partition size 0x%llx\n",__FUNCTION__,size,dl_stat.ubi.cur_voldesc->vol->used_bytes);
				ret = NAND_INVALID_SIZE;
				goto err;
			}
			if(_is_nv_volume(dl_stat.ubi.cur_volnm)) {
				_fdl2_check_nv(dl_stat.ubi.cur_volnm);
			}
			if(!strcmp(dl_stat.ubi.cur_volnm, "prodnv")) {
				u32 magic;
				fdl_ubi_volume_read(dl_stat.ubi.dev,
								dl_stat.ubi.cur_volnm,
								&magic,
								sizeof(u32),
								0);
				if(magic != UBIFS_NODE_MAGIC) {
					printf("bad ubifs node magic %#08x, expected %#08x\n",
						magic, UBIFS_NODE_MAGIC);
					ret = NAND_SYSTEM_ERROR;
					goto err;
				}
			}
			break;
		default:
			printf("%s:Incompatible part %s!\n",__FUNCTION__,part);
			ret = NAND_INCOMPATIBLE_PART;
			goto err;
	}

	dl_stat.nand = _get_cur_nand();
	printf("fdl2_read_start:%s,size:0x%x\n",part,size);

	_send_rsp(NAND_SUCCESS);
	return 1;
err:
	_send_rsp(ret);
	return 0;
}

/**
 * fdl2_read_midst
 *
 * Read partition/volume data
 *
 * @param size size to be read
 * @param off offset of begin of part/vol
 * @param buf data saved
 * @return 0 failed
 *             1 success
 */
int fdl2_read_midst(unsigned long size, unsigned long off, unsigned char *buf)
{
	int ret;

	switch(dl_stat.part_type){
		case PART_TYPE_MTD:
			ret = nand_read_skip_bad(dl_stat.nand, dl_stat.mtd.rw_point+off, &size, buf);
			if(ret)
				goto err;
			break;
		case PART_TYPE_UBI:
			if(_is_nv_volume(dl_stat.ubi.cur_volnm)) {
				off += NV_HEAD_LEN;
			}
			ret = fdl_ubi_volume_read(dl_stat.ubi.dev, 
								dl_stat.ubi.cur_volnm,
								buf,
								size,
								off);
			if(size != ret)
				goto err;
			break;
		default:
			printf("%s:part type err!\n",__FUNCTION__);
			goto err;
	}

	return 1;
err:
	printf("%s:read error %d!\n",__FUNCTION__, ret);
	_send_rsp(NAND_SYSTEM_ERROR);
	return 0;
}

/**
 * fdl2_read_end
 *
 * Set read flash end
 *
 * @param void
 * @return 0 failed
 *             1 success
 */
int fdl2_read_end(void)
{
	//close opened ubi volume
	if(PART_TYPE_UBI == dl_stat.part_type){
		ubi_close_volume(dl_stat.ubi.cur_voldesc);
	}

	_send_rsp(NAND_SUCCESS);
	return 1;
}

/**
 * fdl2_erase
 *
 * Erase partition/volume
 *
 * @param part partition/volume name
 * @param size size to be erased(no use now)
 * @return 0 failed
 *             1 success
 */
int fdl2_erase(char* part, unsigned long size)
{
	int ret;

	if(!strcmp(part, "erase_all")){
		struct mtd_info *nand = NULL;
		nand_erase_options_t opts;

		memset(&opts, 0, sizeof(opts));
		nand = _get_cur_nand();
		opts.offset = 0;
		opts.length = nand->size;
		opts.quiet  = 1;
		ret = nand_erase_opts(nand, &opts);
		if(ret){
			ret =NAND_SYSTEM_ERROR;
			goto err;
		}
		goto end;
	}

	_fdl2_part_info_parse(part);

	switch(dl_stat.part_type){
		case PART_TYPE_MTD:
			ret = _fdl2_mtd_part_erase(part);
			if(ret){
				printf("%s:mtd %d erase failed!\n",__FUNCTION__,part);
				ret = NAND_SYSTEM_ERROR;
				goto err;
			}
			break;
		case PART_TYPE_UBI:
			ret = fdl_ubi_volume_start_update(dl_stat.ubi.dev, part, 0);
			if(ret){
				printf("%s: vol %s erase failed!\n",__FUNCTION__,part);
				ret = NAND_SYSTEM_ERROR;
				goto err;
			}
			ubi_close_volume(dl_stat.ubi.cur_voldesc);
			break;
		default:
			printf("%s:Incompatible part %s!\n",__FUNCTION__,part);
			ret = NAND_INCOMPATIBLE_PART;
			goto err;
	}
end:
	_send_rsp(NAND_SUCCESS);
	return 1;
err:
	_send_rsp(ret);
	return 0;
}

/**
 * fdl2_repartition
 *
 * Resize/Add/Delete volumes
 *
 * @param vol_cfg volume cfg
 * @param total_vol_num
 * @return 0 failed
 *             1 success
 */
int fdl2_repartition(void* vol_cfg, unsigned short total_vol_num)
{
	int ret,i,vol_id,auto_resize_id=-1;
	fdl_ubi_vtbl_t *vtbl=NULL;

	vtbl = malloc(total_vol_num * sizeof(fdl_ubi_vtbl_t));
	if(!vtbl){
		printf("%s:malloc vtbl failed!\n",__FUNCTION__);
		goto err;
	}
	memset(vtbl,0x0,total_vol_num*sizeof(fdl_ubi_vtbl_t));
	_fdl2_parse_volume_cfg(vol_cfg, total_vol_num, vtbl);

	ret = _fdl2_vtbl_check(vtbl, total_vol_num);
	if(ret){
		printf("ubi volumes are same.\n");
		goto end;
	}
	else{
		printf("ubi volumes not same.\n");
		goto repartition;
	}

repartition:
	ret = _fdl2_mtd_part_erase(UBIPAC_PART);
	if(ret)
		goto err;
	ret = fdl_ubi_dev_init();
	if(!ret){
		printf("attach ubi failed after erase!\n");
		goto err;
	}
	//create volumes
	for(i=0;i<total_vol_num;i++){
		ret = fdl_ubi_create_vol(cur_ubi.dev, vtbl[i].name, &vol_id, vtbl[i].size, 1);
		if(ret){
			printf("ubi vol create %s err %d.\n",vtbl[i].name,ret);
			goto err;
		}
		if(vtbl[i].autoresize){
			auto_resize_id = vol_id;
		}
	}
	//resize the autoresize volume
	if(-1 != auto_resize_id){
		ret = fdl_ubi_volume_autoresize(cur_ubi.dev, auto_resize_id);
		if(ret){
			printf("volume auto resize failed %d.\n",ret);
			goto err;
		}
	}

end:
	kfree(vtbl);
	_send_rsp(NAND_SUCCESS);
	return 1;
err:
	kfree(vtbl);
	_send_rsp(NAND_SYSTEM_ERROR);
	return 0;
}

