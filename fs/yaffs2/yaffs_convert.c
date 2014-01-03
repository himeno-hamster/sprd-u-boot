#include <asm/arch/sci_types.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include "yaffs_format_data_translate.h"
#define TRANS_CODE_SIZE         (12 * 1024)
#define	DATA_BUFFER_SIZE	(TRANS_CODE_SIZE * 2)

#define NAND_SUCCESS                0
#define NAND_SYSTEM_ERROR           1
#define NAND_UNKNOWN_DEVICE         2
#define NAND_INVALID_DEVICE_SIZE    3
#define NAND_INCOMPATIBLE_PART      4
#define NAND_INVALID_ADDR           5
#define NAND_INVALID_SIZE           6
extern int nand_curr_device;
unsigned long  yaffs_buffer_size = 0;
unsigned long g_BigSize = 0;
unsigned long g_ReadBufLen = 0;
unsigned long code_yaffs_buflen	= 0;
unsigned long code_yaffs_onewrite = 0;
unsigned char *g_BigBUF = NULL;

int nand_write_to_device(int dev_id,unsigned int size, unsigned char *buf)
{
	struct mtd_info *nand;
	struct nand_chip *chip;
	int ret=0,pos;
	int cur_offset = get_end_write_pos();

	if ((dev_id < 0) || (dev_id >= CONFIG_SYS_MAX_NAND_DEVICE))
		return NAND_SYSTEM_ERROR;
	nand = &nand_info[dev_id];
        chip = nand->priv;
	//find a good block to write 
	while(!(cur_offset & (nand->erasesize-1))) {
//		printf("function: %s to check bad block, check address 0x%x\n", __FUNCTION__,cur_offset&(~(nand->erasesize - 1)));
		if (nand_block_isbad(nand, cur_offset&(~(nand->erasesize - 1)))) {
			printf("%s skip bad block 0x%x\n", __FUNCTION__, cur_offset&(~(nand->erasesize - 1)));
			cur_offset = (cur_offset + nand->erasesize)&(~(nand->erasesize - 1));
		} else {
			break;
		}
	}

	if(size != (nand->writesize + nand->oobsize)){
	  	return NAND_INVALID_SIZE;
	}

	chip->ops.mode = MTD_OOB_AUTO;
	chip->ops.len = nand->writesize;
	chip->ops.datbuf = (uint8_t *)buf;
	chip->ops.oobbuf = (uint8_t *)buf + nand->writesize;
	chip->ops.ooblen = sizeof(yaffs_PackedTags2);
	chip->ops.ooboffs = 0;

//	printf("call nand_do_write_ops cur_offset = 0x%x\n",cur_offset );
	ret = nand_do_write_ops(nand, (unsigned long long)cur_offset, &(chip->ops));
	if (0 == ret) {
		cur_offset += nand->writesize;
	} else {
		printf("\nwrite error, mark bad block : 0x%08x\n", cur_offset);
		nand->block_markbad(nand, cur_offset&~(nand->erasesize - 1));
		printf("find new good partition to move and write data again\n");
		move2goodblk(nand, 1);
		printf("move and write end. new write pos : 0x%08x\n", cur_offset);
		ret = 0;
	}
	set_current_write_pos(cur_offset);
	return NAND_SUCCESS;
}

int init_yaffs_convert_variables(int writesize,int oobsize,int is_yaffs)
{
	int ret;
	yaffs_buffer_size = (DATA_BUFFER_SIZE + (DATA_BUFFER_SIZE / writesize) * oobsize);

	if(g_BigBUF != NULL)
		return 1;
	if (is_yaffs == 0) {
		code_yaffs_buflen = DATA_BUFFER_SIZE;
		code_yaffs_onewrite = writesize;
	} else if (is_yaffs == 1) {
		code_yaffs_buflen = yaffs_buffer_size;
		code_yaffs_onewrite = writesize + oobsize;
	}
	
	g_BigSize = 0;
	if (g_BigBUF == NULL)
		g_BigBUF = (unsigned char *)malloc(yaffs_buffer_size);

	if (g_BigBUF == NULL) {
		printf("malloc is wrong : %d\n", yaffs_buffer_size);
		ret = NAND_SYSTEM_ERROR;		
		return 0;
	}
	memset(g_BigBUF, 0xff, yaffs_buffer_size);
	g_ReadBufLen = 0;
	printf("code_yaffs_onewrite = %d  code_yaffs_buflen = %d  yaffs_buffer_size = %d\n", code_yaffs_onewrite, code_yaffs_buflen, yaffs_buffer_size);
	return 1;

}
int yaffs2_convertAndWrite(int last_flag,int writesize,int oobsize,char *buffer)
{
	yaffs_page src = {0};
	yaffs_page dst = {0};
	int count = 0;
	unsigned int size_threshold = 0;
	int ret = NAND_SUCCESS;
	unsigned int remaindata = 0;

	src.oobsize = 64;
	src.pagesize = 2048;
	src.p_pagedata = g_BigBUF;
	dst.oobsize = oobsize;
	dst.pagesize = writesize;
	dst.p_pagedata = buffer;
	count = dst.pagesize/src.pagesize;
	size_threshold = (src.oobsize + src.pagesize)*count;
	if(last_flag){
		size_threshold = src.oobsize + src.pagesize;
	}
	while ((g_BigSize - g_ReadBufLen) >= size_threshold) {
		//we have received more than one page data.
		count = yaffs_page_translate(&src, &dst);
	//	printf("trans_count= 0x%x ,Big=0x%x ,read=0x%x, threshod=0x%x\n",count,g_BigSize,g_ReadBufLen,size_threshold);
		if(count > 0){
			//update global buffer
			g_ReadBufLen += count*(src.oobsize + src.pagesize);
			//update src data stream pointer
			src.p_pagedata = (unsigned char*)&src.p_pagedata[count*(src.oobsize + src.pagesize)];
			//restore one page data
	         	ret = nand_write_to_device(nand_curr_device,dst.pagesize+ dst.oobsize, dst.p_pagedata);
			if(ret != NAND_SUCCESS){
				return NAND_SYSTEM_ERROR;
			}
		}
	}

	remaindata = g_BigSize - g_ReadBufLen;
	memcpy(g_BigBUF, g_BigBUF + g_ReadBufLen, remaindata);
	memset(g_BigBUF + remaindata, 0xff, yaffs_buffer_size - remaindata);
	g_BigSize = remaindata;
	g_ReadBufLen = 0;

	return ret;
}

void save_to_convert_buffer(char *buffer, int size)
{
	memcpy((g_BigBUF + g_BigSize), buffer, size);
	g_BigSize += size;
	//printf("g_BigSize = %d code_yaffs_buflen=%d\n",g_BigSize,code_yaffs_buflen);
}
int convert_buffer_is_full(void)
{
	if (g_BigSize < (code_yaffs_buflen / 2))
		return 0;
	return 1;
}
void set_convert_buffer(char *buffer,int size)
{
	g_BigBUF = buffer;
	g_BigSize = size;
}
