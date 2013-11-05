#include "fdl_cmd_proc.h"


#define PARTITION_SIZE_LENGTH  (4)
#ifndef MAX_PARTITION_NAME_SIZE
#define MAX_PARTITION_NAME_SIZE (72)
#endif
#ifndef MAX_UTF_PARTITION_NAME_LEN
#define MAX_UTF_PARTITION_NAME_LEN (36)
#endif
//TODO:
#ifdef CONFIG_EMMC_BOOT
#define PART_NAME_FORMAT_UTF
#endif

/**
	just convert partition name wchar to char with violent.
*/
static __inline char* _nformatconvert(wchar_t *wchar)
{
	static char buf[MAX_PARTITION_NAME_SIZE]={0};
	unsigned int i=0;
	while((NULL != wchar[i]) && (i<(MAX_PARTITION_NAME_SIZE-1)))
	{
		buf[i] = wchar[i]&0xFF;
		i++;
	}
	buf[i]=0;

	return buf;
}

static void _decode_packet_data(PACKET_T *packet, wchar_t* partition_name, unsigned long* size, unsigned long* checksum)
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

int FDL2_Download_Start (PACKET_T *packet, void *arg)
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN]={0};
	unsigned long size,nv_checksum;
	void *name;

	printf("Enter %s \n", __FUNCTION__);

	_decode_packet_data(packet, partition_name, &size, &nv_checksum);
#ifdef PART_NAME_FORMAT_UTF
	name = partition_name;
#else
	name = _nformatconvert(partition_name);
#endif

	return fdl2_download_start(name,size,nv_checksum);
}

int FDL2_Download_Midst(PACKET_T *packet, void *arg)
{
	return fdl2_download_midst(packet->packet_body.size, (char *)(packet->packet_body.content));
}

int FDL2_Download_End (PACKET_T *packet, void *arg)
{
	printf("Enter %s \n", __FUNCTION__);
	return fdl2_download_end();
}

int FDL2_Read_Start(PACKET_T *packet, void *arg)
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN]={0};
	unsigned long size;
	void *name;

	printf("Enter %s \n", __FUNCTION__);

	_decode_packet_data(packet, partition_name, &size, NULL);
#ifdef PART_NAME_FORMAT_UTF
	name = partition_name;
#else
	name = _nformatconvert(partition_name);
#endif
	return fdl2_read_start(name, size);
}

int FDL2_Read_Midst(PACKET_T *packet, void *arg)
{
	unsigned long *data = (unsigned long *) (packet->packet_body.content);
	unsigned long size = *data;
	unsigned long off = *(data + 1);
	int           ret;

	if (size > MAX_PKT_SIZE) {
		printf("%s:Size:0x%x beyond MAX_PKT_SIZE(0x%x)\n", __FUNCTION__,size,MAX_PKT_SIZE);
		FDL_SendAckPacket (BSL_REP_DOWN_SIZE_ERROR);
		return FALSE;
	}

	ret = fdl2_read_midst(size, off, (unsigned char *)(packet->packet_body.content));
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

int FDL2_Read_End(PACKET_T *packet, void *arg)
{
	printf("Enter %s \n", __FUNCTION__);
	return fdl2_read_end();
}

int FDL2_Erase(PACKET_T *packet, void *arg)
{
	wchar_t partition_name[MAX_UTF_PARTITION_NAME_LEN]={0};
	unsigned long size;
	void *name;

	printf("Enter %s \n", __FUNCTION__);

	_decode_packet_data(packet, partition_name, &size, NULL);
#ifdef PART_NAME_FORMAT_UTF
	name = partition_name;
#else
	name = _nformatconvert(partition_name);
#endif
	return fdl2_erase(name, size);
}

/**
	Packet body content:
		Partition Name(72Byte)+SIZE(4Byte)+...
*/
int FDL2_Repartition (PACKET_T *pakcet, void *arg)
{
	unsigned short total_partition_num = 0;
	unsigned short size = pakcet->packet_body.size;
	unsigned short *data = (unsigned short *) (pakcet->packet_body.content);

	if(0 != (size%(MAX_PARTITION_NAME_SIZE + PARTITION_SIZE_LENGTH)))
	{
		printf("%s:recvd packet size(%d) is error \n", __FUNCTION__,size);
		FDL_SendAckPacket (BSL_INCOMPATIBLE_PARTITION);
		return 0;
	}
	total_partition_num = size/(MAX_PARTITION_NAME_SIZE + PARTITION_SIZE_LENGTH);
	printf("Enter %s,Partition total num:%d \n", __FUNCTION__,total_partition_num);
	return fdl2_repartition(data, total_partition_num);
}


