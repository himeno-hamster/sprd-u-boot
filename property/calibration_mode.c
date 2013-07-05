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


#define MODE_REQUEST_LENGTH 10
#define RUNMODE_REQUESET_CMD 0xFE
#define CALIBRATION_REQUEST_SUBCMD 0x1
#define MAX_USB_BUF_LEN 10*1024
#define MAX_UART_BUF_LEN 1024


#define TOOL_CHANNEL	(1)
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
extern uint32 ap_calibration_proc(uint8 *data,uint32 count,uint8 *out_msg);
extern uint32 get_adc_flag(); //add by kenyliu in 2013 06 20 for bug 146310
extern int poweron_by_calibration(void);
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
        case 3:
            channel = &gUart3Channel;
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



static int	calibration_device=0;
int  tool_channel_open(void)
{
    calibration_device = poweron_by_calibration();
    switch(calibration_device)
    {
        case 1: //USB calibration ;
            printf("USB calibration\n");
            if(-1 == Calibration_ReinitUsb())
                return -1;
            gs_open();
            break;
        case 2: //UART calibration;
            printf("UART calibration:...\n");
            break;
        default:
            break;
    }
    return 0;
}
int  tool_channel_write(char *buffer,int count)
{
    int ret;
    switch(calibration_device)
    {
        case 1: //USB calibration ;
            {
                while(count > 0){
                    ret = gs_write(g_uart_buf, count);
#if 0
                    printf("func: %s waitting %d write done\n", __func__, count);
                    if(usb_trans_status)
                        printf("func: %s line %d usb trans with error %d\n", __func__, __LINE__, usb_trans_status);
#endif
                    usb_wait_trans_done(1);
                    if(ret > 0)
                        count -= ret;
                }
            }
            break;
        case 2: //UART calibration;
            {
                struct FDL_ChannelHandler *UartChannel;
                UartChannel = Calibration_ChannelGet(TOOL_CHANNEL);
                UartChannel->Write(UartChannel,buffer,count);
            }
            break;
        default:
            break;
    }
}

int  tool_channel_read_status = 0;
int  tool_channel_read(char *buffer,int count)
{
    int index;
    int size = count;
    int	ch;

    index = 0;
    switch(calibration_device)
    {
        case 1: //USB calibration ;
            {
                if(usb_is_trans_done(0)){
                    int ret;
#if 0
                    if(usb_trans_status)
                        printf("func: %s line %d read error %d\n", __func__, __LINE__, usb_trans_status);
#endif
                    ret = gs_read(buffer, &size);
                    if(ret)
                        index = size;
#if 0
                    if(usb_trans_status)
                        printf("func: %s line %d read error %d\n", __func__, __LINE__, usb_trans_status);
#endif
                }
            }
            break;
        case 2:
            {
                struct FDL_ChannelHandler *UartChannel;

                UartChannel = Calibration_ChannelGet(TOOL_CHANNEL);

                ch = UartChannel->GetSingleChar(UartChannel);
                if(0x7E == ch )
                {
                    do
                    {
                        buffer[index++] = ch;
                    }while(0x7E != (ch = UartChannel->GetChar(UartChannel)));

                    buffer[index++] = ch;
                }
            }
        default:
            break;
    }
    return index;
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
    if(CALIBRATION_CHANNEL==1)
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

    printf("calibration_mode step2\n");

    // wait for cp ready
    i=0;
    while(gpio_get_value(CP_AP_LIV) == 0);
    printf("calibration_mode step3\n");

#if defined(CONFIG_SC7710G2)
    serial3_flowctl_enable();
#endif

    int nvitem_sync(void);
    nvitem_sync();

    printf("calibration_mode step4\n");

#endif
    if(tool_channel_open() == -1)
        return;

    printf("calibration_mode step5\n");

    while(TRUE){
        count = tool_channel_read(g_usb_buf, MAX_USB_BUF_LEN);
        if((index = ap_calibration_proc( g_usb_buf, count,g_uart_buf)) == 0){
            if(count > 0){
                if(count != gUsedChannel->Write(gUsedChannel, g_usb_buf, count)) {
                    return ;
                }
                mdelay(3);
            }
            count = 0;
            index += gUsedChannel->Read(gUsedChannel, g_uart_buf, MAX_UART_BUF_LEN);
        }
        if(index > 0){
            if(1 == index && 'K' == g_uart_buf[0] && 0 == gpio_get_value(CP_AP_LIV)) {
                printf("change bandrate ...");
                gUsedChannel->SetBaudrate(gUsedChannel, 1843200/*921600*/);
            }else {
                tool_channel_write(g_uart_buf, index);
            }
        }
        if(gpio_get_value(CP_AP_LIV) == 1) {
            nvitem_sync();
        }
        //add by kenyliu in 2013 06 20 for bug 146310
        if(0xE == get_adc_flag())
        {
            power_down_devices();
        }
        //end kenyliu
    }
#ifndef __DL_UART0__
    __dl_log_share__ = 0;
#endif

}

#endif /* CONFIG_CALIBRATION_MODE_NEW */
