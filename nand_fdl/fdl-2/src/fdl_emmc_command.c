#include "sci_types.h"
#include "fdl_conf.h"
#ifdef CONFIG_EMMC_BOOT
#include "fdl_emmc.h"
#include "packet.h"
#include "fdl_stdio.h"
#include "asm/arch/sci_types.h"
#include "fdl_emmc_operate.h"

#ifdef FPGA_TRACE_DOWNLOAD
typedef struct
{
        unsigned short  type;
        unsigned short  size;
        char*           content;
}trace_packet_body;

typedef struct
{
    struct PACKET_tag *next;
    int     pkt_state;
    int     data_size;
    int     ack_flag;
    trace_packet_body packet_body;
}TRACE_PACKET_T;
#endif

#define PARTITION_SIZE_LENGTH  (4)

#ifdef FPGA_TRACE_DOWNLOAD
LOCAL void _decode_packet_data(TRACE_PACKET_T *packet, wchar_t* partition_name, unsigned long* size, unsigned long* checksum)
#else
LOCAL void _decode_packet_data(PACKET_T *packet, wchar_t* partition_name, unsigned long* size, unsigned long* checksum)
#endif
{
	int i;
	unsigned short *data = (unsigned short *) (packet->packet_body.content);
	*size = *(unsigned long *) ((unsigned char *)data + MAX_PARTITION_NAME_SIZE);
	if(NULL != checksum)
		*checksum = *(unsigned long *) ((unsigned char *)data + MAX_PARTITION_NAME_SIZE+4);

	for(i=0;i<MAX_UTF_PARTITION_NAME_LEN;i++)
	{
		partition_name[i] = *(data+i);
	}

	return;
}

#ifdef FPGA_TRACE_DOWNLOAD
int FDL2_eMMC_DataStart (TRACE_PACKET_T *packet, void *arg)
#else
int FDL2_eMMC_DataStart (PACKET_T *packet, void *arg)
#endif
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN]={0};
	unsigned long size,nv_checksum;

	_decode_packet_data(packet, partition_name, &size, &nv_checksum);

	printf("FDL2_eMMC_DataStart: Partition_Name:%S,Size:%d\n",partition_name,size);
	return fdl2_emmc_download_start(partition_name,size,nv_checksum);
}


#ifdef FPGA_TRACE_DOWNLOAD
int FDL2_eMMC_DataMidst(TRACE_PACKET_T *packet, void *arg)
#else
int FDL2_eMMC_DataMidst(PACKET_T *packet, void *arg)
#endif
{
	return fdl2_emmc_download(packet->packet_body.size, (char *)(packet->packet_body.content));
}

#ifdef FPGA_TRACE_DOWNLOAD
int FDL2_eMMC_DataEnd (TRACE_PACKET_T *packet, void *arg)
#else
int FDL2_eMMC_DataEnd (PACKET_T *packet, void *arg)
#endif
{
	return fdl2_emmc_download_end();
}

int FDL2_eMMC_ReadStart(PACKET_T *packet, void *arg)
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN]={0};
	unsigned long size;

	_decode_packet_data(packet, partition_name, &size, NULL);

	return fdl2_emmc_read_start(partition_name, size);
}

int FDL2_eMMC_ReadMidst(PACKET_T *packet, void *arg)
{
	unsigned long *data = (unsigned long *) (packet->packet_body.content);
	unsigned long size = *data;
	unsigned long off = *(data + 1);
	int           ret;

	if (size > MAX_PKT_SIZE) {
		FDL_SendAckPacket (BSL_REP_DOWN_SIZE_ERROR);
		return FALSE;
	}

	ret = fdl2_emmc_read_midst(size, off, (unsigned char *)(packet->packet_body.content));
	if(ret)
	{
		packet->packet_body.type = BSL_REP_READ_FLASH;
	        packet->packet_body.size = size;
	        FDL_SendPacket (packet);
		return TRUE;
	}
	else
	{
		//The Error Cause has been send to Tool
		return FALSE;
	}
}

int FDL2_eMMC_ReadEnd(PACKET_T *packet, void *arg)
{
	return fdl2_emmc_read_end();
}

int FDL2_eMMC_Erase(PACKET_T *packet, void *arg)
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN]={0};
	unsigned long size;

	_decode_packet_data(packet, partition_name, &size, NULL);

	printf("FDL2_eMMC_Erase: Partition_Name:%S,Size:%d\n",partition_name,size);

	return fdl2_emmc_erase(partition_name, size);
}

/**
	Packet body content:
		Partition Name(72Byte)+SIZE(4Byte)+...
*/
int FDL2_eMMC_Repartition (PACKET_T *pakcet, void *arg)
{
	unsigned short total_partition_num = 0;
	unsigned short size = pakcet->packet_body.size;
	unsigned short *data = (unsigned short *) (pakcet->packet_body.content);

	if(0 != (size%(MAX_PARTITION_NAME_SIZE + PARTITION_SIZE_LENGTH)))
	{
		FDL_SendAckPacket (BSL_INCOMPATIBLE_PARTITION);
		return 0;
	}
	total_partition_num = size/(MAX_PARTITION_NAME_SIZE + PARTITION_SIZE_LENGTH);
	printf("FDL2_eMMC_Repartition: Partition total num:%d\n",total_partition_num);
	return fdl2_emmc_repartition(data, total_partition_num);
}


#ifdef FPGA_TRACE_DOWNLOAD
uint32 FDL2_eMMC_DRAM_Download(uint32 start_addr, char* mem_addr, uint32 size)
{
	uint32 wr_size=0;
	char   tmp_buf[8];
	TRACE_PACKET_T packet;
	packet.next=NULL;
	memset(&packet, 0, sizeof(packet));
	printf("start_addr:%08x, mem_addr:%08x, size:%08x\r\n", start_addr, mem_addr, size);
	packet.ack_flag=0;
	packet.pkt_state=3;
	packet.data_size=0xe;
	packet.packet_body.type=0x100;
	packet.packet_body.size=8;
	packet.packet_body.content = tmp_buf;
	(packet.packet_body.content)[0]=(start_addr>>24) & 0xff;
	(packet.packet_body.content)[1]=(start_addr>>16) & 0xff;
	(packet.packet_body.content)[2]=(start_addr>>8) & 0xff;
	(packet.packet_body.content)[3]=(start_addr>>0) & 0xff;
	(packet.packet_body.content)[4]=(size>>24) & 0xff;
	(packet.packet_body.content)[5]=(size>>16) & 0xff;
	(packet.packet_body.content)[6]=(size>>8) & 0xff;
	(packet.packet_body.content)[7]=(size>>0) & 0xff;
	printf("packet addr :%08x\r\n", &packet);
	FDL2_eMMC_DataStart(&packet, NULL);
	packet.packet_body.type    = 0x200;
	for (wr_size=0; wr_size<size; )
	{
		if (size-wr_size>0x800)
			packet.packet_body.size = 0x800;
		else
			packet.packet_body.size = size-wr_size;
		packet.packet_body.content = mem_addr+wr_size;
		packet.data_size = packet.packet_body.size+6;
		wr_size += packet.packet_body.size;
		FDL2_eMMC_DataMidst(&packet, NULL);
	}
	packet.data_size=6;
	packet.packet_body.type   =0x300;
	packet.packet_body.size   =0;
	packet.packet_body.content= tmp_buf;
	(packet.packet_body.content)[0]=0xff;
	(packet.packet_body.content)[1]=0xfc;
	FDL2_eMMC_DataEnd(&packet, NULL);
}
#endif


// why ???? delete later
int get_end_write_pos(void)
{
	return 0;
}
void set_current_write_pos(int pos)
{
}
void move2goodblk(void)
{
}

#endif

