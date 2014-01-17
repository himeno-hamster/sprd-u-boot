/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
//#define DEBUG
#include <config.h>
#include <common.h>
#include <asm/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "gadget_chips.h"
#include <linux/ctype.h>
#include <malloc.h>
#include <command.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <jffs2/jffs2.h>
#include <asm/types.h>
#include <android_boot.h>
#include <android_bootimg.h>
#include <boot_mode.h>
#include <asm/arch/secure_boot.h>

struct	dl_image_inf{
	uint32_t base_address;
	uint32_t max_size;
	uint32_t data_size;
}ImageInfo[2];

#define NV_HEAD_LEN (512)
#define NV_HEAD_MAGIC   (0x00004e56)
#define NV_VERSION      (101)

typedef struct  _NV_HEADER {
     uint32_t magic;
     uint32_t len;
     uint32_t checksum;
     uint32_t version;
}nv_header_t;

#ifdef CONFIG_EMMC_BOOT
#include "../disk/part_uefi.h"
#include "../drivers/mmc/card_sdio.h"
#include "asm/arch/sci_types.h"
#include <ext_common.h>
#include <ext4fs.h>

#define MAGIC_DATA	0xAA55A5A5
#define SPL_CHECKSUM_LEN	0x6000
#define CHECKSUM_START_OFFSET	0x28
#define MAGIC_DATA_SAVE_OFFSET	(0x20/4)
#define CHECKSUM_SAVE_OFFSET	(0x24/4)
PARTITION_CFG uefi_part_info[MAX_PARTITION_INFO];
#define EFI_SECTOR_SIZE 		(512)
#define ERASE_SECTOR_SIZE		((64 * 1024) / EFI_SECTOR_SIZE)
#define EMMC_BUF_SIZE			(((216 * 1024 * 1024) / EFI_SECTOR_SIZE) * EFI_SECTOR_SIZE)
#define FB_ERASE_ALIGN_LENGTH  (0x800)
#if defined (CONFIG_SC8825) || defined (CONFIG_TIGER)
unsigned char *g_eMMCBuf = (unsigned char*)0x82000000;
#else
unsigned char *g_eMMCBuf = (unsigned char*)0x2000000;
#endif

#if defined CONFIG_SC8825 || defined(CONFIG_SC8830)
#define BOOTLOADER_HEADER_OFFSET 0x20
typedef struct{
	uint32 version;
	uint32 magicData;
	uint32 checkSum;
	uint32 hashLen;
}EMMC_BootHeader;
#endif

typedef enum
{
	FB_IMG_RAW = 0,
	FB_IMG_WITH_SPARSE = 1,
	FB_IMG_TYPE_MAX
}FB_PARTITION_IMG_TYPE;

typedef enum
{
	FB_PARTITION_PURPOSE_NORMAL,
	FB_PARTITION_PURPOSE_NV,
	FB_PARTITION_PURPOSE_PROD,
	FB_PARTITION_PURPOSE_MAX
}FB_PARTITION_PURPOSE;

typedef struct
{
	CARD_EMMC_PARTITION_TPYE type;
	FB_PARTITION_PURPOSE purpose;
	FB_PARTITION_IMG_TYPE img_format;
	wchar_t* partition_name;
}FB_PARTITION_INFO;

static FB_PARTITION_INFO const s_fb_special_partition_cfg[]={
	{PARTITION_BOOT1,FB_PARTITION_PURPOSE_NORMAL,FB_IMG_RAW,L"params"},
	{PARTITION_BOOT2,FB_PARTITION_PURPOSE_NORMAL,FB_IMG_RAW,L"2ndbl"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NORMAL,FB_IMG_RAW,L"system"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NORMAL,FB_IMG_WITH_SPARSE,L"userdata"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NORMAL,FB_IMG_WITH_SPARSE,L"cache"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NORMAL,FB_IMG_RAW,L"prodnv"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NV,FB_IMG_RAW,L"fixnv1"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NV,FB_IMG_RAW,L"tdfixnv1"},
	{PARTITION_USER,FB_PARTITION_PURPOSE_NV,FB_IMG_RAW,L"wfixnv1"},
	{PARTITION_MAX,FB_PARTITION_PURPOSE_MAX,FB_IMG_TYPE_MAX,NULL}
};
#else

typedef struct {
	char *vol;
	char *bakvol;
}FB_NV_VOL_INFO;

static FB_NV_VOL_INFO s_nv_vol_info[]={
	{"fixnv1","fixnv2"},
	{"wfixnv1","wfixnv2"},
	{"tdfixnv1","tdfixnv2"},
	{NULL,NULL}
};

#endif

//#define FASTBOOT_DEBUG
#ifdef FASTBOOT_DEBUG
#define fb_printf(fmt, args...) printf(fmt, ##args)
#else
#define fb_printf(fmt, args...) do {} while(0)
#endif

#ifdef FLASH_PAGE_SIZE
#undef FLASH_PAGE_SIZE
#endif
#define FLASH_PAGE_SIZE 2048

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

#define GFP_ATOMIC ((gfp_t) 0)
static int current_write_position;

int get_end_write_pos(void)
{
	return current_write_position;
}
void set_current_write_pos(int pos)
{
	current_write_position = pos;
}
void move2goodblk(void)
{
	while(1) fb_printf("suspend in move2goodblk\n");
}
/* todo: give lk strtoul and nuke this */
static unsigned hex2unsigned(const char *x)
{
    unsigned n = 0;

    while(*x) {
        switch(*x) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            n = (n << 4) | (*x - '0');
            break;
        case 'a': case 'b': case 'c':
        case 'd': case 'e': case 'f':
            n = (n << 4) | (*x - 'a' + 10);
            break;
        case 'A': case 'B': case 'C':
        case 'D': case 'E': case 'F':
            n = (n << 4) | (*x - 'A' + 10);
            break;
        default:
            return n;
        }
        x++;
    }

    return n;
}

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const char *prefix;
	unsigned prefix_len;
	void (*handle)(const char *arg, void *data, unsigned sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	const char *name;
	const char *value;
};
	
static struct fastboot_cmd *cmdlist;

void fastboot_register(const char *prefix,
		       void (*handle)(const char *arg, void *data, unsigned sz))
{
	struct fastboot_cmd *cmd;
	cmd = malloc(sizeof(*cmd));
	if (cmd) {
		cmd->prefix = prefix;
		cmd->prefix_len = strlen(prefix);
		cmd->handle = handle;
		cmd->next = cmdlist;
		cmdlist = cmd;
	}
}

static struct fastboot_var *varlist;

void fastboot_publish(const char *name, const char *value)
{
	struct fastboot_var *var;
	var = malloc(sizeof(*var));
	if (var) {
		var->name = name;
		var->value = value;
		var->next = varlist;
		varlist = var;
	}
}


//static event_t usb_online;
//static event_t txn_done;
static volatile int txn_done;
static unsigned char buffer[4096];
static struct usb_ep *in, *out;
//static struct udc_request *req;
static struct usb_request *tx_req, *rx_req;
int txn_status;

#define STATE_OFFLINE	0
#define STATE_COMMAND	1
#define STATE_COMPLETE	2
#define STATE_ERROR	3

static unsigned fastboot_state = STATE_OFFLINE;

//static void req_complete(struct udc_request *req, unsigned actual, int status)
static void req_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		debug("req complete --> %d, %d/%d\n",
				req->status, req->actual, req->length);
	
	txn_status = req->status;
	txn_done = 1;
	/*
	req->length = actual;
	event_signal(&txn_done, 0);
	*/
}

static int usb_read(void *_buf, unsigned len)
{
	int r;
	unsigned xfer;
	unsigned char *buf = _buf;
	int count = 0;
	struct usb_request * req = rx_req;
	
	if (fastboot_state == STATE_ERROR)
		goto oops;

	fb_printf("usb_read(address = 0x%x,len=0x%x)\n",_buf,len);
	while (len > 0) {
		xfer = (len > 4096) ? 4096 : len;
		req->buf = buf;
		req->length = xfer;
		req->complete = req_complete;
		//r = udc_request_queue(out, req);
		r = usb_ep_queue(out, req, GFP_ATOMIC);
		if (r < 0) {
			fb_printf("usb_read() queue failed\n");
			goto oops;
		}
		//event_wait(&txn_done);
		txn_done = 0;
		while(!txn_done)
			usb_gadget_handle_interrupts();

		if (txn_status < 0) {
			fb_printf("usb_read() transaction failed\n");
			goto oops;
		}
		if((count % 0x100000) == 0)
		fb_printf("remained size = 0x%x\n",len);

		count += req->actual;
		buf += req->actual;
		len -= req->actual;

		/* short transfer? */
		if (req->actual != xfer) break;
	}

	return count;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

static int usb_write(void *buf, unsigned len)
{
	int r;
	struct usb_request * req = tx_req;
	
	if (fastboot_state == STATE_ERROR)
		goto oops;

	req->buf = buf;
	req->length = len;
	req->complete = req_complete;
	txn_done = 0;
	//r = udc_request_queue(in, req);
	r = usb_ep_queue(in, req, GFP_ATOMIC);
	if (r < 0) {
		fb_printf("usb_write() queue failed\n");
		goto oops;
	}
	//event_wait(&txn_done);
	while(!txn_done)
		usb_gadget_handle_interrupts();
	if (txn_status < 0) {
		fb_printf("usb_write() transaction failed\n");
		goto oops;
	}
	return req->actual;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

void fastboot_ack(const char *code, const char *reason)
{
	char response[64] = {0};

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		reason = "";

	//snprintf(response, 64, "%s%s", code, reason);
	if(strlen(code) + strlen(reason) >= 64) {
		fb_printf("%s too long string\r\n", __func__);
	}
	sprintf(response, "%s%s", code, reason);
	fastboot_state = STATE_COMPLETE;

	usb_write(response, strlen(response));

}

void fastboot_fail(const char *reason)
{
	fastboot_ack("FAIL", reason);
}

void fastboot_okay(const char *info)
{
	fastboot_ack("OKAY", info);
}

static void cmd_getvar(const char *arg, void *data, unsigned sz)
{
	struct fastboot_var *var;

	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			fastboot_okay(var->value);
			return;
		}
	}
	fastboot_okay("");
}

static void dump_log(char * buf, int len)
{
	int i = 0;

	fb_printf("**dump log_buf ...addr:0x%08x, len:%d\r\n", buf, len);

	for (i = 0; i < len; i++)	{
		fb_printf("%02x ", *((unsigned char*)buf+i) );
		if(i%0x20 == 0x1f)
			fb_printf("\n");
	}
}
static void cmd_download(const char *arg, void *data, unsigned sz)
{
	char response[64];
	unsigned len = hex2unsigned(arg);
	unsigned read_len,index,max_buffer_size;
	int r;

	fb_printf("%s\n", __func__);
	
	fb_printf("arg'%s' data %p, %d,len=0x%x\n",arg, data,sz,len);
	fb_printf("base0 0x%x size0 0x%x base1=0x%x size1=0x%x\n",\
		ImageInfo[0].base_address,ImageInfo[0].max_size,\
		ImageInfo[1].base_address,ImageInfo[1].max_size);
	max_buffer_size = ImageInfo[0].max_size + ImageInfo[1].max_size;
	if (len > max_buffer_size) {
		fastboot_fail("data too large");
		return;
	}

	sprintf(response,"DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0)
		return;

	ImageInfo[0].data_size = 0;
	ImageInfo[1].data_size = 0;
	index = 0;
	read_len = 0;
	fb_printf("%s-start\n", __func__);
	do{
		if(ImageInfo[index].max_size==0){
			fastboot_state = STATE_ERROR;
			fb_printf("%s- error1\n", __func__);
			return;
		}
		if(len > ImageInfo[index].max_size)
			read_len = ImageInfo[index].max_size;
		else
			read_len = len;
		fb_printf("save data to area[%d].address=0x%x read_len = 0x%x\n",index,ImageInfo[index].base_address,read_len);
		r = usb_read(ImageInfo[index].base_address, read_len);
		if ((r < 0) || (r != read_len)) {
			fastboot_state = STATE_ERROR;
			return;
		}
		len -= read_len;
		ImageInfo[index].data_size = read_len;
		index++;
	}while(len > 0);
	fastboot_okay("");
}

#ifdef CONFIG_EMMC_BOOT
unsigned short fastboot_eMMCCheckSum(const unsigned int *src, int len)
{
	unsigned int   sum = 0;
	unsigned short *src_short_ptr = PNULL;

	while (len > 3){
		sum += *src++;
		len -= 4;
	}
	src_short_ptr = (unsigned short *) src;
	if (0 != (len&0x2)){
		sum += * (src_short_ptr);
		src_short_ptr++;
	}
	if (0 != (len&0x1)){
		sum += * ( (unsigned char *) (src_short_ptr));
	}
	sum  = (sum >> 16) + (sum & 0x0FFFF);
	sum += (sum >> 16);

	return (unsigned short) (~sum);
}


void fastboot_splFillCheckData(unsigned int * splBuf,  int len)
{
#if   defined(CONFIG_SC8810)
	*(splBuf + MAGIC_DATA_SAVE_OFFSET) = MAGIC_DATA;
	*(splBuf + CHECKSUM_SAVE_OFFSET) = (unsigned int)fastboot_eMMCCheckSum((unsigned int *)&splBuf[CHECKSUM_START_OFFSET/4], SPL_CHECKSUM_LEN - CHECKSUM_START_OFFSET);

#elif defined(CONFIG_SC8825) || defined(CONFIG_SC7710G2) || defined(CONFIG_SC8830)
	EMMC_BootHeader *header;
	header = (EMMC_BootHeader *)((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET);
	header->version  = 0;
	header->magicData= MAGIC_DATA;
	header->checkSum = (unsigned int)fastboot_eMMCCheckSum((unsigned char*)splBuf+BOOTLOADER_HEADER_OFFSET+sizeof(*header), SPL_CHECKSUM_LEN-(BOOTLOADER_HEADER_OFFSET+sizeof(*header)));
	header->hashLen  = 0;
#endif
}

LOCAL void _makEcc(uint8* buf, uint32 size)
{
	uint16 crc;
	crc = calc_checksum(buf,size-4);
	buf[size-4] = (uint8)(0xFF&crc);
	buf[size-3] = (uint8)(0xFF&(crc>>8));
	buf[size-2] = 0;
	buf[size-1] = 0;

	return;
}

/**
	Erase the whole partition.
*/
LOCAL int _fb_erase_partition(wchar_t *partition_name,unsigned int curArea,unsigned long base,unsigned long count)
{
	if(NULL == partition_name)
		return 0;

	if(count < FB_ERASE_ALIGN_LENGTH)
	{
		unsigned char buf[FB_ERASE_ALIGN_LENGTH*EFI_SECTOR_SIZE] = {0xFF};
		if (!Emmc_Write(curArea, base,count,buf))
			return 0;
	}
	else
	{
		if(base%FB_ERASE_ALIGN_LENGTH)
		{
			unsigned char buf[FB_ERASE_ALIGN_LENGTH*EFI_SECTOR_SIZE] = {0xFF};
			unsigned long base_sector_offset = 0;

			base_sector_offset = FB_ERASE_ALIGN_LENGTH - base%FB_ERASE_ALIGN_LENGTH;
			if (!Emmc_Write(curArea, base,base_sector_offset,buf))
				return 0;
			count = ((count-base_sector_offset)/FB_ERASE_ALIGN_LENGTH)*FB_ERASE_ALIGN_LENGTH;
			base = base + base_sector_offset;
		}
		else
			count = (count/FB_ERASE_ALIGN_LENGTH)*FB_ERASE_ALIGN_LENGTH;

		if(count == 0)
			return 1;

		if (!Emmc_Erase(curArea, base,count))
			return 0;
	}

	return 1;
}

void cmd_flash(const char *arg, void *data, unsigned sz)
{
	int i;
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN];
	unsigned int partition_type = PARTITION_USER;
	unsigned int partition_purpose = FB_PARTITION_PURPOSE_NORMAL;
	unsigned int img_format = FB_IMG_RAW;
	uint32 startblock;
	uint32 count;
	disk_partition_t info;
	block_dev_desc_t *dev = NULL;
	uint32 total_sz = 0;
	int32 retval = 0;
	uint32 idx = 0;
	uint32 size_left = 0;

	fb_printf("%s\n", __func__);
	dev = get_dev("mmc", 1);
	fb_printf("cmd_flash:data = 0x%x,ImageInfo[0].base_address = 0x%x,ImageInfo[1].base_address = 0x%x\n",data,ImageInfo[0].base_address,ImageInfo[1].base_address);
	if(NULL == dev){
		fastboot_fail("Block device not supported!");
		return;
	}

	fb_printf("Cmd Flash partition:%s \n", arg);
	for(i=0;i<MAX_UTF_PARTITION_NAME_LEN;i++)
	{
		partition_name[i] = arg[i];
		if(0 == arg[i])
			break;
	}
	//get the special partition info
	for(i=0;NULL != s_fb_special_partition_cfg[i].partition_name;i++)
	{
		if(wcscmp(s_fb_special_partition_cfg[i].partition_name, partition_name) == 0)
		{
			partition_type = s_fb_special_partition_cfg[i].type;
			partition_purpose = s_fb_special_partition_cfg[i].purpose;
			img_format = s_fb_special_partition_cfg[i].img_format;
			break;
		}
	}

	count = ((sz +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1)))/EMMC_SECTOR_SIZE;
	switch(partition_type)
	{
		case PARTITION_BOOT1:
			fastboot_splFillCheckData(data, sz);
			count = ((SPL_CHECKSUM_LEN +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1)))/EMMC_SECTOR_SIZE;
			startblock = 0;
			goto emmc_write;
		case PARTITION_BOOT2:
			startblock = 0;
			goto emmc_write;
		case PARTITION_USER:
			{
				//Check boot&recovery img's magic
				if (!strcmp(arg, "boot") ||!strcmp(arg, "recovery")) {
					if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
						fastboot_fail("image is not a boot image");
						return;
					}
				}
				//get partition info from emmc
				if(0 != get_partition_info_by_name(dev, partition_name, &info))
				{
					fastboot_fail("eMMC get partition ERROR!");
					return;
				}

				startblock = info.start;

				if(FB_IMG_WITH_SPARSE == img_format)
				{
					int write_addr = ImageInfo[0].base_address;
					int write_size = ImageInfo[0].data_size;
					int move_size = 0;
					int write_in = 0;

					//1 move once
					//2 block[1] is big enough
					while (write_size > 0)
					{
						fb_printf("cmd_flash: write_size %d\n",write_size);
						retval = write_simg2emmc("mmc", 1, partition_name, write_addr, write_size);

						if(-1 == retval){
							fb_printf("cmd_flash : retval =%d,idx = %d\n",retval,idx);
							fastboot_fail("eMMC WRITE_ERROR!");
							return;
						}
						if(0 == retval){
							fb_printf("cmd_flash : write end!\n");
							goto end;
						}

						size_left = write_size - retval;

						fb_printf("cmd flash:size = %d, retval = %d,size left %d\n", write_size, retval,size_left);

						write_in += retval;

						if (write_addr == ImageInfo[0].base_address)	{
							//move once
							move_size = size_left;
							fb_printf("flash second times size_left = %d!\n",size_left);

							if(move_size + ImageInfo[1].data_size > ImageInfo[1].max_size){
								//error tips
								fastboot_fail("eMMC imageinfo2 is too small!");
								return;
							}

							if (move_size > 0){
								memmove(ImageInfo[1].base_address + move_size,
									ImageInfo[1].base_address,
									ImageInfo[1].data_size);
								memcpy(ImageInfo[1].base_address,
									ImageInfo[0].base_address + retval, move_size);
							}
							write_addr = ImageInfo[1].base_address;
							write_size = move_size + ImageInfo[1].data_size;
						}
						else{
							fb_printf("flash third times size_left = %d!\n",size_left);
							write_addr = ImageInfo[1].base_address + write_in - (ImageInfo[0].data_size - move_size);
							write_size = size_left;
						}
					}
					goto end;
				}
				else if(FB_IMG_RAW == img_format){
					uint32  data_left = 0;

					fb_printf("cmd_flash: raw data\n");

					if(FB_PARTITION_PURPOSE_NV == partition_purpose)
					{
						nv_header_t *header_p = NULL;
						uint8  header_buf[EMMC_SECTOR_SIZE];

						memset(header_buf,0x00,EMMC_SECTOR_SIZE);
						header_p = header_buf;
						header_p->magic = NV_HEAD_MAGIC;
						header_p->len = FIXNV_SIZE;
						header_p->checksum =(uint32)calc_checksum((unsigned char *) data,FIXNV_SIZE);
						header_p->version = NV_VERSION;
						if(!Emmc_Write(partition_type, startblock, 1,header_buf)){
							fastboot_fail("eMMC WRITE_NVHEADER_ERROR!");
							return;
						}
						startblock++;
						count = ((FIXNV_SIZE +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1)))/EMMC_SECTOR_SIZE;

						goto emmc_write;
					}

					//first part
					data_left = ImageInfo[0].data_size%EMMC_SECTOR_SIZE;
					count = ImageInfo[0].data_size/EMMC_SECTOR_SIZE;
					if(!Emmc_Write(partition_type, startblock,count,(uint8*)ImageInfo[0].base_address)){
						fb_printf("cmd_flash: raw data write frist part error!\n");
						fastboot_fail("write frist part error!");
						return;
					}
					fb_printf("cmd_flash:raw data write first part success\n");
					startblock += count;

					if(0 == ImageInfo[1].data_size){
						//no data in second block
						if(0 == data_left){
							//write finish
							goto end;
						}
						else{
							if(!Emmc_Write(partition_type, startblock,1,ImageInfo[0].base_address+count*EMMC_SECTOR_SIZE)){
								fb_printf("cmd_flash: raw data write tail part error!\n");
								fastboot_fail("write tail part error!");
								return;
							}
						}
					}
					else{
						if(data_left+ImageInfo[1].data_size > ImageInfo[1].max_size){
							//error tips
							fastboot_fail("eMMC raw data imageinfo2 is too small!\n");
							return;
						}
						memmove(ImageInfo[1].base_address+data_left,
							ImageInfo[1].base_address,
							ImageInfo[1].data_size);
						memcpy(ImageInfo[1].base_address,
							ImageInfo[0].base_address+ImageInfo[0].data_size-data_left,
							data_left);

						count = ((ImageInfo[1].data_size +(EMMC_SECTOR_SIZE - 1)) & (~(EMMC_SECTOR_SIZE - 1)))/EMMC_SECTOR_SIZE;
						if(!Emmc_Write(partition_type, startblock,count,(uint8*)ImageInfo[1].base_address)){
							fb_printf("cmd_flash: raw data write second part error!\n");
							fastboot_fail("write second part error!");
							return;
						}
						fb_printf("cmd_flash:raw data write second part success\n");
					}
					goto end;
				}
				else
				{
					fastboot_fail("Image Format Unkown!");
					return;
				}
			}
		default:
			fastboot_fail("Partition Type ERROR!");
			return;
	}

emmc_write:
	if(!Emmc_Write(partition_type, startblock, count,(uint8*)data)){
		fastboot_fail("eMMC WRITE_ERROR!");
		return;
	}
end:
	fastboot_okay("");
}

void cmd_erase(const char *arg, void *data, unsigned sz)
{
	int i;
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN];
	unsigned long count=0, base_sector=0;
	unsigned int curArea = 0;
	disk_partition_t info;
	block_dev_desc_t *dev = NULL;

	dev = get_dev("mmc", 1);
	if(NULL == dev){
		fastboot_fail("Block device not supported!");
		return;
	}

	fb_printf("Cmd Erase partition:%s \n", arg);

	if (strcmp("params", arg) == 0){
		if(secureboot_enabled()){
			fastboot_fail("secureboot enable!");
			return;
		}
		count = Emmc_GetCapacity(PARTITION_BOOT1);
		curArea = PARTITION_BOOT1;
		base_sector = 0;
	}else if (strcmp("2ndbl", arg) == 0){
		count = Emmc_GetCapacity(PARTITION_BOOT2);
		curArea = PARTITION_BOOT2;
		base_sector = 0;
	}
	else{
		for(i=0;i<MAX_UTF_PARTITION_NAME_LEN;i++)
		{
			partition_name[i] = arg[i];
			if(0 == arg[i])
				break;
		}
		//get partition info from emmc
		if(0 != get_partition_info_by_name(dev, partition_name, &info))
		{
			fastboot_fail("eMMC get partition ERROR!");
			return;
		}
		curArea = PARTITION_USER;
		count = info.size;
		base_sector = info.start;
	}

	if(!_fb_erase_partition(partition_name, curArea, base_sector, count))
	{
		fastboot_fail("eMMC Erase Partition ERROR!");
		return;
	}
	fb_printf("Cmd Erase OK\n");
	fastboot_okay("");
	return;
}

#else
void cmd_flash(const char *arg, void *data, unsigned sz)
{
	int ret =-1;
	int i;

	fb_printf("%s, arg:%x date: 0x%x, sz 0x%x\n", __func__, arg, data, sz);

	if (!strcmp(arg, "boot") || !strcmp(arg, "recovery")) {
		if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			fastboot_fail("image is not a boot image");
			return;
		}
	}

	/**
	 *	FIX ME!
	 *	assume first image buffer is big enough for nv
	 */
	for(i=0;s_nv_vol_info[i].vol != NULL;i++) {
		if(!strcmp(arg, s_nv_vol_info[i].vol)) {
			nv_header_t *header = NULL;
			uint8_t  tmp[NV_HEAD_LEN];

			memset(tmp,0x00,NV_HEAD_LEN);
			header = tmp;
			header->magic = NV_HEAD_MAGIC;
			header->len = FIXNV_SIZE;
			header->checksum =(uint32_t)calc_checksum((unsigned char *) data,FIXNV_SIZE);
			header->version = NV_VERSION;
			//write org nv
			ret = do_raw_data_write(arg, FIXNV_SIZE+NV_HEAD_LEN, NV_HEAD_LEN, 0, tmp);
			if(ret)
				goto end;
			ret = do_raw_data_write(arg, 0, FIXNV_SIZE, NV_HEAD_LEN, data);
			if(ret)
				goto end;
			//write bak nv
			ret = do_raw_data_write(s_nv_vol_info[i].bakvol, FIXNV_SIZE+NV_HEAD_LEN, NV_HEAD_LEN, 0, tmp);
			if(ret)
				goto end;
			ret = do_raw_data_write(s_nv_vol_info[i].bakvol, 0, FIXNV_SIZE, NV_HEAD_LEN, data);
			goto end;
		}
	}

	if(ImageInfo[1].data_size) {
		uint32_t total_sz  = ImageInfo[0].data_size + ImageInfo[1].data_size;
		ret = do_raw_data_write(arg, total_sz, ImageInfo[0].data_size, 0, ImageInfo[0].base_address);
		if(ret)
			goto end;
		ret = do_raw_data_write(arg, 0, ImageInfo[1].data_size, ImageInfo[0].data_size, ImageInfo[1].base_address);
	}
	else {
		ret = do_raw_data_write(arg, ImageInfo[0].data_size, ImageInfo[0].data_size, 0, ImageInfo[0].base_address);
	}

end:
	if(!ret)
		fastboot_okay("");
	else
		fastboot_fail("flash error");
	return;
}

void cmd_erase(const char *arg, void *data, unsigned sz)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	nand_erase_options_t opts;
	u8 pnum;
	int ret;
	char buf[1024];

	fb_printf("%s\n", __func__);

	ret = find_dev_and_part(arg, &dev, &pnum, &part);
	if(!ret){
		nand = &nand_info[dev->id->num];
		memset(&opts, 0, sizeof(opts));
		opts.offset = (loff_t)part->offset;
		opts.length = (loff_t)part->size;
		opts.jffs2 = 0;
		opts.quiet = 1;
		ret = nand_erase_opts(nand, &opts);
		if(ret)
			goto end;
	}

	//just erase 1k now
	memset(buf, 0x0, 1024);
	ret = do_raw_data_write(arg, 1024, 1024, 0, buf);

end:
	if(ret)
		fastboot_fail("nand erase error");
	else
		fastboot_okay("");
	return;
}
#endif

extern void udc_power_off(void);

extern unsigned char raw_header[2048];

void cmd_boot(const char *arg, void *data, unsigned sz)
{
	boot_img_hdr *hdr = raw_header;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned kernel_addr;
	unsigned ramdisk_addr;
	char * cmdline;

	fb_printf("%s, arg: %s, data: %p, sz: 0x%x\n", __func__, arg, data, sz);
	memcpy(raw_header, data, 2048);
	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		fb_printf("boot image headr: %s\n", hdr->magic);
		fastboot_fail("bad boot image header");
		return;
	}
	kernel_actual= ROUND_TO_PAGE(hdr->kernel_size,(FLASH_PAGE_SIZE - 1));
	if(kernel_actual<=0){
		fastboot_fail("kernel image should not be zero");
		return;
	}
	ramdisk_actual= ROUND_TO_PAGE(hdr->ramdisk_size,(FLASH_PAGE_SIZE - 1));
	if(ramdisk_actual==0){
		fastboot_fail("ramdisk size error");
		return;
	}
	
	memcpy((void*)hdr->kernel_addr, (void *)data + FLASH_PAGE_SIZE, kernel_actual);
	memcpy((void*)hdr->ramdisk_addr, (void *)data + FLASH_PAGE_SIZE + kernel_actual, ramdisk_actual);
	
	fb_printf("kernel @0x%08x (0x%08x bytes)\n", hdr->kernel_addr, kernel_actual);
	fb_printf("ramdisk @0x%08x (0x%08x bytes)\n", hdr->ramdisk_addr, ramdisk_actual);
	//set boot environment
	if(hdr->cmdline[0]){
		cmdline = (char *)hdr->cmdline;
	}else{
		cmdline = getenv("bootargs");
	}
	fb_printf("cmdline %s\n", cmdline);

	fastboot_okay("");
	udc_power_off();
	creat_atags(hdr->tags_addr, cmdline, hdr->ramdisk_addr, hdr->ramdisk_size);
	boot_linux(hdr->kernel_addr,hdr->tags_addr);
}

extern int do_cboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);

void cmd_continue(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	udc_power_off();
	//do_cboot(NULL, 0, 1, NULL);
    normal_mode();
}

void cmd_reboot(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	//udc_power_off();
    reboot_devices(NORMAL_MODE);
}

void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
	udc_power_off();
    reboot_devices(FASTBOOT_MODE);
}

void cmd_powerdown(const char *arg, void *data, unsigned sz)
{
	fastboot_okay("");
    power_down_devices(0);

}

static void fastboot_command_loop(void)
{
	struct fastboot_cmd *cmd;
	int r;
	fb_printf("fastboot: processing commands\n");

again:
	while (fastboot_state != STATE_ERROR) {
		memset(buffer, 0 , 64);
		r = usb_read(buffer, 64);
		if (r < 0) break;
		buffer[r] = 0;
		fb_printf("fastboot: %s, r:%d\n", buffer, r);

		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			fb_printf("cmd list :%s \n", cmd->prefix);
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
				continue;
			fastboot_state = STATE_COMMAND;
			cmd->handle((const char*) buffer + cmd->prefix_len,
				    (void*) ImageInfo[0].base_address, ImageInfo[0].data_size);
			if (fastboot_state == STATE_COMMAND)
				fastboot_fail("unknown reason");
			goto again;
		}

		fastboot_fail("unknown command");
			
	}
	fastboot_state = STATE_OFFLINE;
	fb_printf("fastboot: oops!\n");
}

static int fastboot_handler(void *arg)
{
	for (;;) {
		fastboot_command_loop();
	}
	return 0;
}

/*
static void fastboot_notify(struct udc_gadget *gadget, unsigned event)
{
	if (event == UDC_EVENT_ONLINE) {
		event_signal(&usb_online, 0);
	}
}

static struct udc_endpoint *fastboot_endpoints[2];

static struct udc_gadget fastboot_gadget = {
	.notify		= fastboot_notify,
	.ifc_class	= 0xff,
	.ifc_subclass	= 0x42,
	.ifc_protocol	= 0x03,
	.ifc_endpoints	= 2,
	.ifc_string	= "fastboot",
	.ept		= fastboot_endpoints,
};
*/
#if defined(CONFIG_CMD_FASTBOOT)
int do_fastboot (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	fb_printf("%s is alive\n", __func__);
	while(1){
		usb_gadget_handle_interrupts();
	}
	
	return 0;
}

U_BOOT_CMD(
	fastboot,	1,	1,	do_fastboot,
	"android fastboot protocol",
);
#endif

int fastboot_init(void *base, unsigned size, struct usb_ep * ep_in, struct usb_ep *ep_out)
{
	fb_printf("fastboot_init()\n");

	ImageInfo[0].base_address = base;
	ImageInfo[0].max_size = size;
	ImageInfo[0].data_size = 0;
	ImageInfo[1].max_size =  0;
	ImageInfo[1].data_size = 0;
#ifdef SCRATCH_ADDR_EXT1
	ImageInfo[1].base_address = SCRATCH_ADDR_EXT1;
	ImageInfo[1].max_size = FB_DOWNLOAD_BUF_EXT1_SIZE;
#endif
	if(!ep_in) {
		fb_printf("ep in is not alloc\r\n");
		return -1;
	}
	in = ep_in;
	
	if(!ep_out) {
		fb_printf("ep out is not alloc\r\n");
		return -1;
	}
	out = ep_out;

	tx_req = usb_ep_alloc_request(in, 0);
	if(!tx_req)
	{
	      fb_printf("ep tx request is not alloc\r\n");
	      return -1;
	}
	rx_req =  usb_ep_alloc_request(out, 0);
	if(!rx_req)
	{
	      fb_printf("ep rx request is not alloc\r\n");
	      usb_ep_free_request(in,tx_req);
	      return -1;
	}
/*
	in = udc_endpoint_alloc(UDC_TYPE_BULK_IN, 512);
	if (!in)
		goto fail_alloc_in;
	out = udc_endpoint_alloc(UDC_TYPE_BULK_OUT, 512);
	if (!out)
		goto fail_alloc_out;

	fastboot_endpoints[0] = in;
	fastboot_endpoints[1] = out;

	req = udc_request_alloc();
	if (!req)
		goto fail_alloc_req;

	if (udc_register_gadget(&fastboot_gadget))
		goto fail_udc_register;
*/
/*
	static char cmd1[] = "getvar:";
	fastboot_register(cmd1, cmd_getvar);
*/
	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	fastboot_publish("version", "1.0");

	fastboot_register("flash:", cmd_flash);
	fastboot_register("erase:", cmd_erase);
	fastboot_register("boot", cmd_boot);
	fastboot_register("reboot", cmd_reboot);
	fastboot_register("powerdown", cmd_powerdown);
	fastboot_register("continue", cmd_continue);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);

	fastboot_handler(0);

	return 0;
/*
fail_udc_register:
	udc_request_free(req);
fail_alloc_req:
	udc_endpoint_free(out);	
fail_alloc_out:
	udc_endpoint_free(in);
fail_alloc_in:
	return -1;
*/
}

