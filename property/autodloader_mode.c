#include <config.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
//#include <boot_mode.h>
#include <common.h>
#include <asm/io.h>

#include <asm/arch/dl_engine.h>
#include <asm/arch/cmd_def.h>
#include <asm/arch/fdl_channel.h>
#include <asm/arch/packet.h>
#include <asm/arch/regs_ahb.h>
//#include "../drivers/mmc/card_sdio.h"
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sprd_reg.h>


#define USB_ENUM_MS 15000
#define USB_IO_MS 100000
#ifdef HWRST_STATUS_AUTODLOADER
#undef HWRST_STATUS_AUTODLOADER
#endif
#define HWRST_STATUS_AUTODLOADER (0xa0)


typedef struct _DL_FILE_STATUS
{
    unsigned long start_address;
    unsigned long total_size;
    unsigned long recv_size;
    unsigned long next_address;
} DL_FILE_STATUS, *PDL_FILE_STATUS;

static DL_FILE_STATUS g_file;
static const char VERSION_STR[] = {"SPRD3"};


extern int dwc_otg_driver_init(void);
extern int usb_serial_init(void);
extern int usb_is_port_open(void);
extern int usb_is_configured(void);
extern void usb_boot (uint32 ext_clk26M);
extern void FDL_PacketInit (void);
extern unsigned char FDL_DlReg (CMD_TYPE cmd, CMDPROC proc, void *arg);

int autodloader_initusb(void)
{
	ulong start_time, now ;
	dwc_otg_driver_init();
	usb_serial_init();
	start_time = get_timer_masked();
	while(!usb_is_configured()){
		now = get_timer_masked();
		if(now - start_time > USB_ENUM_MS) {
			printf("USB SERIAL CONFIGED failed \n");
			return -1;
		}
	}
	printf("USB SERIAL CONFIGED\n");

	 start_time = get_timer_masked();
	while(!usb_is_port_open()) {
		now = get_timer_masked();
		if(now - start_time > USB_IO_MS) {
			printf("USB SERIAL PORT OPEN failed \n");
			return -1;
		}
	}

	printf("USB SERIAL PORT OPENED\n");

	//gs_open();

	return 0;
}

static unsigned int autodloader_EndianConv_32 (unsigned int value)
{
#ifdef _LITTLE_ENDIAN
    unsigned int nTmp = 0;
	nTmp = (value >> 24 | value << 24);

    nTmp |= ( (value >> 8) & 0x0000FF00);
    nTmp |= ( (value << 8) & 0x00FF0000);
    return nTmp;
#else
    return value;
#endif
}
int autodloader_connect(PACKET_T *packet, void *arg)
{
    FDL_SendAckPacket(BSL_REP_ACK);
    return 1;
}

int autodloader_start(PACKET_T *packet, void *arg)
{
    unsigned long *data = (unsigned long*)(packet->packet_body.content);
    unsigned long start_addr = *data;
    unsigned long file_size  = *(data + 1);

#if defined (CHIP_ENDIAN_LITTLE)
    start_addr = autodloader_EndianConv_32(start_addr);
    file_size  = autodloader_EndianConv_32(file_size);
#endif

    g_file.start_address = start_addr;
    g_file.total_size = file_size;
    g_file.recv_size = 0;
    g_file.next_address = start_addr;

    memset((void*)start_addr, 0, file_size);
    if (!packet->ack_flag)
    {
        packet->ack_flag = 1;
        FDL_SendAckPacket(BSL_REP_ACK);
    }
    return 1;
}

int autodloader_midst(PACKET_T *packet, void *arg)
{
    unsigned short data_len = packet->packet_body.size;

    if ((g_file.recv_size + data_len) > g_file.total_size) {
        FDL_SendAckPacket(BSL_REP_DOWN_SIZE_ERROR);
        return 0;
    }

    memcpy((void *)g_file.next_address, (const void*)(packet->packet_body.content), data_len);
    g_file.next_address += data_len;
    g_file.recv_size += data_len;
    if (!packet->ack_flag)
    {
        packet->ack_flag = 1;
        FDL_SendAckPacket(BSL_REP_ACK);
    }
    return 1;
}

int autodloader_end(PACKET_T *packet, void *arg)
{
    if (!packet->ack_flag)
    {
        packet->ack_flag = 1;
        FDL_SendAckPacket(BSL_REP_ACK);
    }
    return 1;
}

int autodloader_exec(PACKET_T *packet, void *arg)
{
    FDL_SendAckPacket(BSL_REP_ACK);
    ANA_REG_SET(ANA_REG_GLB_POR_RST_MONITOR, HWRST_STATUS_AUTODLOADER);
//    JumpToTarget(g_file.start_address);
    typedef void(*entry)(void);
    entry entry_func = (entry)((void*)g_file.start_address);
    entry_func();
    return 0;
}


int autodloader_mainhandler(void)
{
	PACKET_T *packet;
	usb_boot(1);
	FDL_PacketInit();

	FDL_DlInit();
	FDL_DlReg(BSL_CMD_CONNECT,	  autodloader_connect,	0);
	FDL_DlReg(BSL_CMD_START_DATA, autodloader_start,	0);
	FDL_DlReg(BSL_CMD_MIDST_DATA, autodloader_midst,	0);
	FDL_DlReg(BSL_CMD_EXEC_DATA,  autodloader_exec,	0);
	FDL_DlReg(BSL_CMD_END_DATA,   autodloader_end, 	0);

	for (;;)
	{
		char ch = gFdlUsedChannel->GetChar(gFdlUsedChannel);
		if (0x7e == ch)
			break;
	}

	packet = FDL_MallocPacket();
	packet->packet_body.type = BSL_REP_VER;
	packet->packet_body.size = sizeof(VERSION_STR);
	memcpy(packet->packet_body.content, VERSION_STR, sizeof(VERSION_STR));
	FDL_SendPacket(packet);
	FDL_FreePacket(packet);

	FDL_DlEntry(DL_STAGE_NONE);

	return 0;
}

static void __raw_bits_and(unsigned int v, unsigned int a)
{
	__raw_writel((__raw_readl(a) & v), a);
}

void autodlader_remap(void)
{
	__raw_bits_and(~BIT_0, AHB_REMAP);
}

void autodloader_mode(void)
{
	int i = 0;
	printf("%s\n", __FUNCTION__);
	//for(i = 0; i < 0xfffffff; i++)
		//printf("hello world! \n");

	//CARD_SDIO_PwrCtl(emmc_handle, FALSE);

	/* initialize usb module*/
	if(autodloader_initusb())
		return ;
	/* remap iram */
	autodlader_remap();
	/* main handler receive and jump */
	autodloader_mainhandler();

	/* cannot reach here */
	for(;;);
}
