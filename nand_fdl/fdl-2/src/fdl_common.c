#include "fdl_common.h"

unsigned short fdl_calc_checksum(unsigned char *data, unsigned long len)
{
	unsigned short num = 0;
	unsigned long chkSum = 0;
	while(len>1) {
		num = (unsigned short)(*data);
		data++;
		num |= (((unsigned short)(*data))<<8);
		data++;
		chkSum += (unsigned long)num;
		len -= 2;
	}
	if(len) {
		chkSum += *data;
	}
	chkSum = (chkSum >> 16) + (chkSum & 0xffff);
	chkSum += (chkSum >> 16);
	return (~chkSum);
}

unsigned char fdl_check_crc(uint8* buf, uint32 size,uint32 checksum)
{
	uint16 crc;

	crc = fdl_calc_checksum(buf,size);
	printf("fdl_check_crc  calcout = 0x%x,org = 0x%x\n",crc,checksum);
	return (crc == (uint16)checksum);
}