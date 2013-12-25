#include "sci_types.h"

#define NV_HEAD_MAGIC	(0x00004e56)
#define NV_VERSION		(101)
#define NV_HEAD_LEN	(512)
typedef struct  _NV_HEADER {
     uint32 magic;
     uint32 len;
     uint32 checksum;
     uint32 version;
}nv_header_t;

unsigned short fdl_calc_checksum(unsigned char *data, unsigned long len);
unsigned char fdl_check_crc(uint8* buf, uint32 size,uint32 checksum);
