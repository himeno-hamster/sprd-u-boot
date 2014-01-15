#include <asm/arch/sci_types.h>
#include "fdl_main.h"
#include <asm/arch/cmd_def.h>
#include <asm/arch/packet.h>
#include <asm/arch/dl_engine.h>
#include <asm/arch/sio_drv.h>
#include "mcu_command.h"
#include <asm/arch/usb_boot.h>
#include <asm/arch/dma_drv_fdl.h>
#include <asm/arch/sc_reg.h>

#include "fdl_cmd_proc.h"


extern  const unsigned char FDL2_signature[][24];
extern int sprd_clean_rtc(void);

static void fdl_error(void)
{
    //sio_putstr("The second FDL failed!\r\n");
    for (;;) /*Do nothing*/;
}
/*
 * Avoid const string comment out by -O2 opt level of compiler
*/
const unsigned char **FDL2_GetSig (void)
{
    return (const unsigned char **) FDL2_signature;
}
extern unsigned long _bss_end;
#ifdef CONFIG_SC8810
extern unsigned long _bss_start;

static int bss_end_end;
static int bss_start_start;
char mempool[1024*1024] = {0};

extern void get_adc_cali_data (void);
extern uint32_t CHG_GetAdcCalType(void);
extern int DCDC_Cal_ArmCore(void);
#endif
int main(void)
{
	/* All hardware initialization has been done in the 1st FDL,
	 * so we don't do initialization stuff here.
	 * The UART has also been opened by the 1st FDL and the baudrate
	 * has been setted correctly.
	 */  
	int err = 0;
	uint32 sigture_address;
	unsigned int i, j;

	printf("fdl main \n");
	//MMU_Init(0);

	sigture_address = (uint32)FDL2_signature;

#if defined(CHIP_ENDIAN_DEFAULT_LITTLE) && defined(CHIP_ENDIAN_BIG)    
	usb_boot(1);  
#endif

	FDL_PacketInit();

#ifdef CONFIG_SC8810	
	bss_start_start = _bss_start;
	bss_end_end = _bss_end;
	mem_malloc_init (&mempool[0], 1024*1024);
#else
	mem_malloc_init (_bss_end, CONFIG_SYS_MALLOC_LEN);	   
#endif	   
	timer_init();
#if defined (CONFIG_SC8830)
	/* add calibration in fdl-2 */
	get_adc_cali_data();
	if (CHG_GetAdcCalType() != 0)
	{
		DCDC_Cal_ArmCore();
	}
#else
	sprd_clean_rtc();
#endif

	do {
#ifdef CONFIG_EMMC_BOOT
		/* Initialize eMMC. */
		extern int mmc_legacy_init(int dev);
		mmc_legacy_init(1);
		err = EMMC_SUCCESS;
#else
		nand_init();
		fdl_ubi_dev_init();
#endif
		MMU_Init(0);

#ifdef FPGA_TRACE_DOWNLOAD
		if(!err)
		{
			fdl_ram2flash_dl();
		}
		while(1);
#else
		/* Register command handler */
		FDL_DlInit();
		FDL_DlReg(BSL_CMD_START_DATA,     FDL2_Download_Start,         0);
		FDL_DlReg(BSL_CMD_MIDST_DATA,     FDL2_Download_Midst,         0);
		FDL_DlReg(BSL_CMD_END_DATA,       FDL2_Download_End,           0);
		FDL_DlReg(BSL_CMD_READ_FLASH_START,     FDL2_Read_Start,         0);
		FDL_DlReg(BSL_CMD_READ_FLASH_MIDST,     FDL2_Read_Midst,         0);
		FDL_DlReg(BSL_CMD_READ_FLASH_END,     FDL2_Read_End,         0);
		FDL_DlReg(BSL_ERASE_FLASH,        FDL2_Erase,        0);
		FDL_DlReg(BSL_REPARTITION,    	   FDL2_Repartition,       0);
		FDL_DlReg(BSL_CMD_NORMAL_RESET,   FDL_McuResetNormal/*mcu_reset_boot*/,   0);
		FDL_DlReg(BSL_CMD_READ_CHIP_TYPE, FDL_McuReadChipType, 0);  
                FDL_DlReg(BSL_CMD_READ_MCP_TYPE, FDL_McuReadMcpType, 0);
		//Send BSL_INCOMPATIBLE_PARTITION because of FDL2 will know nothing about new partition
		FDL_SendAckPacket (BSL_INCOMPATIBLE_PARTITION);

		/* Start the download process. */
		FDL_DlEntry (DL_STAGE_CONNECTED);
#endif
	} while (0);

	/* If we get here, there must be something wrong. */
	fdl_error();
	return 0;
}

