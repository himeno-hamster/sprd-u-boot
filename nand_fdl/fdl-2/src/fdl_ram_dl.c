#include "fdl_ram_dl.h"

//TODO:Function not ready.

typedef struct{
	char *part;
	unsigned int imgaddr;
	unsigned int imgsize;
	/*xxx*/
}fdl_ramimg_table;

#define IMG_TABLE_ADDR	0x80a00000

/**
 * fdl_ram2flash_dl
 *
 * For fpga trace download,   
 * write data from ram to nand/emmc flash
 *
 * @param void
 * @return void
 */
void fdl_ram2flash_dl(void)
{
	for(;;)
	{
		//step1 get ramimg table
		//step2 write img from ram to flash
			//step2.1 dl start
			//step2.2 dl midst
			for(;;){
				break;
			}
			//step2.3 dl end
		//return when dl all img in table
		return;
	}
}

