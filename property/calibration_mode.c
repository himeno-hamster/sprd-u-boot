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


#define TOOL_CHANNEL (1)
#define mdelay(n) udelay((n) * 1000)


typedef struct _Mode_Data
{
    uint32 sn;
    uint16 length;
    uint8 type;
    uint8 subtype;
}Mode_Data_Type;



struct FDL_ChannelHandler *gUsedChannel;
static unsigned char g_usb_buf[MAX_USB_BUF_LEN];
static unsigned char g_usb_buf_ex[MAX_USB_BUF_LEN];
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



static int calibration_device=0;
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
    int ch;

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
extern int nvitem_sync_enable(void);
extern int nvitem_sync_disable(void);
extern int nvitem_is_sync_done(void);
extern void nvitem_sync_reset(void);
extern int get_nv_sync_flag(void);
extern void set_nv_sync_flag(int flag);
typedef struct{
    unsigned int seq;
    unsigned short len;
    unsigned char main_cmd;
    unsigned char sub_cmd;
}packet_head_t;
static int untranslate_packet_header(char *dest,char *src,int size, int unpackSize){
    int i;
    int translated_size = 0;
    int status = 0;
    int flag = 0;
    for(i=0;i<size;i++){
        switch(status){
            case 0:
                if(src[i] == 0x7e)
                    status = 1;
                break;
            case 1:
                if(src[i] != 0x7e){
                    status = 2;
                    dest[translated_size++] = src[i];
                }
                break;
            case 2:
                if(src[i] == 0x7E){
                    unsigned short crc;
                    crc = crc_16_l_calc((char const *)dest,translated_size-2);
                    return translated_size;
                }else{
                    if((dest[translated_size-1] == 0x7D)&&(!flag)){
                        flag = 1;
                        if(src[i] == 0x5E){
                            dest[translated_size-1] = 0x7E;
                        }else if(src[i] == 0x5D){
                            dest[translated_size-1] = 0x7D;
                        }
                    }else{
                        flag = 0;
                        dest[translated_size++] = src[i];
                    }

                    if (translated_size >= unpackSize+1 && unpackSize != -1){
                        return translated_size;
                    }
                }
                break;
        }
    }

    return translated_size;
}
static int is_get_whole_cmd(unsigned char* cmd_buf,unsigned short* count){
    static int is_head = 1;
    static unsigned short cmd_len = 0,got_len = 0;
    unsigned short len = *count;
    unsigned char* tail = NULL;

    if(is_head&&len < 64){
        //in this condition we do not check!
        return 1;
    }

    tail = cmd_buf + len -1;
    if(is_head){
        packet_head_t Head;
        packet_head_t* pHead = &Head;

        printf("PARSE A NEW COMMEND ...\n");
        untranslate_packet_header(pHead, cmd_buf, *count, sizeof(Head));
        printf("seq:0x%x,len:0x%x,main_cmd:0x%x,sub_cmd:0x%x\n",
                pHead->seq,
                pHead->len,
                pHead->main_cmd,
                pHead->sub_cmd);
        cmd_len = pHead->len+2;
    }
    got_len += len;
    printf("cmd_len = %d,got_len = %d\n",cmd_len,got_len);
    if(got_len < cmd_len||*tail != 0x7e){
        printf("tmp store the in-completed cmd ...tail character is 0x%x\n",*tail);
        memcpy(g_usb_buf_ex+got_len-len,g_usb_buf,len);
        is_head = 0;
        *count = 0;
        return 0;
    }else{
        if(got_len > len){
            printf("sync cmd ...\n");
            memcpy(g_usb_buf_ex+got_len-len,g_usb_buf,len);
            memcpy(g_usb_buf,g_usb_buf_ex,got_len);
        }
        printf("got an completed cmd!!!\n");
        *count = got_len;
        is_head = 1;
        cmd_len = 0;
        got_len = 0;
        return 1;
    }
}
void calibration_mode(const uint8_t *pcmd, int length)
{
    int ret;
    int count = 0;
    unsigned char ch;
    int index = 0;
    int i;
    unsigned char buf[MODE_REQUEST_LENGTH] = {0};
    int sync_index = 0;

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

        if(count > 0 && 0 == is_get_whole_cmd(g_usb_buf,&count)){
            goto SKIP_CMD_PROCESS;
        }


        if((index = ap_calibration_proc( g_usb_buf, count,g_uart_buf)) == 0){
            if(get_nv_sync_flag()){
                //printf("NV_SYNC,get_nv_sync_flag so enable nvitem sync ...\n");
                //tools request to save wcdma calibration params to flash
                //so we must make sure to write it to flash before power off
                nvitem_sync_enable();
            }
            if(count > 0){
                if(count != gUsedChannel->Write(gUsedChannel, g_usb_buf, count)) {
                    return ;
                }
                mdelay(3);
            }
            count = 0;
#if 1
            while(-1 != (ret = gUsedChannel->GetSingleChar(gUsedChannel))){
                if(get_nv_sync_flag()){
                    g_uart_buf[sync_index++] = (ret & 0xff);
                }else{
                    g_uart_buf[index++] = (ret & 0xff);
                }
            }
#else
            if(get_nv_sync_flag()){
                sync_index += gUsedChannel->Read(gUsedChannel, g_uart_buf, MAX_UART_BUF_LEN);
            }else{
                index += gUsedChannel->Read(gUsedChannel, g_uart_buf, MAX_UART_BUF_LEN);
            }
#endif
        }else{
            count = 0;
        }

SKIP_CMD_PROCESS:

        if(gpio_get_value(CP_AP_LIV) == 1) {
            nvitem_sync();
        }
        if(get_nv_sync_flag()){
            if(sync_index > 0 && nvitem_is_sync_done()){
                printf("NV_SYNC,nvitem_is_sync_done=true,so we reset all the gloables ...\n");
                nvitem_sync_reset();
                nvitem_sync_disable();
                set_nv_sync_flag(0);
                printf("NV_SYNC,we can notify the tool now!!!\n");
                {
                    int i = 0;
                    printf("\n--------------------------------------------\n");
                    for(i = 0;i < sync_index;i++){
                        printf("%02x ",g_uart_buf[i]);
                    }
                    printf("\n--------------------------------------------\n");
                }
                tool_channel_write(g_uart_buf, sync_index);
                sync_index = 0;
            }else if (sync_index > 0){
                //printf("NV_SYNC,nvitem_is_sync_done=flase,sync_index=%d ...\n",sync_index);
            }else{
                //printf("NV_SYNC,we haven't got response from cp ...\n");
            }
        }else{
            if(index > 0){
                if(1 == index && 'K' == g_uart_buf[0] && 0 == gpio_get_value(CP_AP_LIV)) {
                    printf("change bandrate ...");
                    gUsedChannel->SetBaudrate(gUsedChannel, 1843200/*921600*/);
                }else {
                    tool_channel_write(g_uart_buf, index);
                }
            }
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
