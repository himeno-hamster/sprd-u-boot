#include <config.h>
#include <common.h>
#ifdef CONFIG_CALIBRATION_MODE_NEW

#include <linux/types.h>
#include <asm/arch/bits.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>
#include <boot_mode.h>
#include <malloc.h>

#include <asm/io.h>
#include <asm/arch/sio_drv.h>
#include "asm/arch/sci_types.h"

#define CALIBRATION_CHANNEL 1 // 0 : UART0 1: UART1
#define MODE_REQUEST_LENGTH 10
#define RUNMODE_REQUESET_CMD 0xFE
#define CALIBRATION_REQUEST_SUBCMD 0x1
#define MAX_USB_BUF_LEN 10*1024
#define MAX_UART_BUF_LEN 1024


#define mdelay(n)	udelay((n) * 1000)


typedef struct _Mode_Data
{
	uint32 sn;
	uint16 length;
	uint8 type;
	uint8 subtype;
}Mode_Data_Type;



struct FDL_ChannelHandler *gUsedChannel;
static unsigned char g_usb_buf[MAX_USB_BUF_LEN];
static unsigned char g_uart_buf[MAX_UART_BUF_LEN];
extern int __dl_log_share__ ;
extern int usb_trans_status;

extern int dwc_otg_driver_init(void);
extern int usb_serial_init(void);
extern int usb_is_configured(void);
extern int get_cal_enum_ms(void);
extern int usb_is_port_open(void);
extern int get_cal_io_ms(void);
extern int gpio_get_value(int PinNo);
extern int gs_open(void);
extern int spi_channel_init(unsigned long phy_id);
extern int usb_is_trans_done(int direct);
extern int gs_read(const unsigned char *buf, int *count);
extern int gs_write(const unsigned char *buf, int count);
extern void usb_wait_trans_done(int direct);
extern void gs_reset_usb_param(void);
extern void calibration_reset_composite(void);
extern void init_calibration_mode(void);

unsigned short EndianConv_16 (unsigned short value)
{
#if 0 //def _LITTLE_ENDIAN
    return (value >> 8 | value << 8);
#else
    return value;
#endif
}

unsigned int EndianConv_32 (unsigned int value)
{
#if 0 //def _LITTLE_ENDIAN
    unsigned int nTmp = 0;
	nTmp = (value >> 24 | value << 24);

    nTmp |= ( (value >> 8) & 0x0000FF00);
    nTmp |= ( (value << 8) & 0x00FF0000);
    return nTmp;
#else
    return value;
#endif
}


static struct FDL_ChannelHandler *Calibration_ChannelGet(int nchannel)
{
    struct FDL_ChannelHandler *channel;

    switch (nchannel)
    {
        case 1:
            channel = &gUart1Channel;
            break;
        case 0:
            channel = &gUart0Channel;
            break;
        default:
            channel = &gUart0Channel;
            break;
    }

    return channel;
}
static int Calibration_ParseRequsetMode(const unsigned char *buf, int len)
{
	Mode_Data_Type *pReqType = NULL;
	char tempbuf[MODE_REQUEST_LENGTH - 2] = {0};
	if(buf[0] != 0x7e || buf[len - 1] != 0x7e)
		return -1;
	memcpy((void*)tempbuf, (void*)&buf[1], len-2);
	pReqType = (Mode_Data_Type*)tempbuf;
	if(pReqType->type != RUNMODE_REQUESET_CMD || pReqType->subtype != CALIBRATION_REQUEST_SUBCMD){
		return -1;
	}
	return 0;
}

static int Calibration_SetMode(const uint8_t *pcmd, int length)
{
	if(pcmd[0] != 0x7e || pcmd[length - 1] != 0x7e)
		return -1;
	if(gUsedChannel->Write(gUsedChannel, pcmd, length) != length)
		return -1;
	return 0;
}

static int Calibration_ReinitUsb(void)
{
	unsigned long long start_time, end_time;
	gs_reset_usb_param();
	calibration_reset_composite();
	dwc_otg_driver_init();
	usb_serial_init();
	start_time = get_timer_masked();
	while(!usb_is_configured()){
		end_time = get_timer_masked();
		if(end_time - start_time > get_cal_enum_ms())
			return -1;
	}	
	printf("USB SERIAL CONFIGED\n");

	start_time = get_timer_masked();
	while(!usb_is_port_open()){
		end_time = get_timer_masked();
		if(end_time - start_time > get_cal_io_ms())
			return -1;
	}	
	return 0;
}

void calibration_mode(const uint8_t *pcmd, int length)
{
	int ret;
	int count = 0;
	unsigned char ch;
	int index = 0;
	int i;
	unsigned char buf[MODE_REQUEST_LENGTH] = {0};

	init_calibration_mode();
#ifndef __DL_UART0__
	__dl_log_share__ = 1;
#endif	

	gUsedChannel = Calibration_ChannelGet(CALIBRATION_CHANNEL);
	gUsedChannel->Open(gUsedChannel, 115200);
#if 1    
        while(gUsedChannel->GetChar(gUsedChannel) != 0x7e); // purge fifo
        buf[0] = 0x7e;
	for(i = 1; i < MODE_REQUEST_LENGTH; i++){
		ch = gUsedChannel->GetChar(gUsedChannel);
		buf[i] = ch;
	}
    
	if(-1 == Calibration_ParseRequsetMode(buf, MODE_REQUEST_LENGTH))
		return ;
	if(-1 == Calibration_SetMode(pcmd, length))
		return ;

	// wait for cp ready
	while(gpio_get_value(CP_AP_LIV) == 0);
	int nvitem_sync(void);
	nvitem_sync();
#endif
	printf("Calibration_ReinitUsb......\n");
	if(-1 == Calibration_ReinitUsb())
		return ;
	gs_open();

	while(TRUE){
		if(usb_is_trans_done(0)){
			if(usb_trans_status)
				printf("func: %s line %d usb trans with error %d\n", __func__, __LINE__, usb_trans_status);
			count = MAX_USB_BUF_LEN;
			gs_read(g_usb_buf, &count);
			if(usb_trans_status)
				printf("func: %s line %d usb trans with error %d\n", __func__, __LINE__, usb_trans_status);
		}
		if(count > 0){
			if(count != gUsedChannel->Write(gUsedChannel, g_usb_buf, count)) {
				return ;
			}
                        mdelay(3);
		}
		count = 0;		
		while(-1 != (ret = gUsedChannel->GetSingleChar(gUsedChannel))){
			g_uart_buf[index++] = (ret & 0xff);
		}
		while(index > 0){
			ret = gs_write(g_uart_buf, index);
			printf("func: %s waitting %d write done\n", __func__, index);
			if(usb_trans_status)
				printf("func: %s line %d usb trans with error %d\n", __func__, __LINE__, usb_trans_status);
			usb_wait_trans_done(1);
			if(ret > 0)
				index -= ret;			
		}
		nvitem_sync();
		
	}

#ifndef __DL_UART0__
		__dl_log_share__ = 0;
#endif	

}

#endif /* CONFIG_CALIBRATION_MODE_NEW */
