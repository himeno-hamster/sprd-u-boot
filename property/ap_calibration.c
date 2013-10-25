#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include "asm/arch/cmddef.h"
#include "asm/arch/sci_types.h"

#ifdef CONFIG_EMMC_BOOT
#include <part.h>
#include "../disk/part_uefi.h"
extern int Calibration_read_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, char *buf, int len);
extern int Calibration_write_partition(block_dev_desc_t *p_block_dev, EFI_PARTITION_INDEX part, char *buf, int len);
static unsigned int nv_buffer[256]={0};
static nv_read_flag = 0;
#endif

extern int ensure_path_mounted(const char * mountpoint);
extern int ensure_path_umounted(const char * mountpoint);
extern int nand_part_write(char *partname,char *buffer, int size);
extern int nand_part_read(char *partname,char *buffer, int size);
extern int nv_access_write(unsigned short sub_cmd,unsigned char * buf);
#ifdef CONFIG_AP_ADC_CALIBRATION

#define AP_ADC_CALIB    1
#define AP_ADC_LOAD     2
#define AP_ADC_SAVE     3
#define AP_GET_VOLT 4
#define AP_NV_ACCESS_IGNOR  0xA0
#define AP_NV_ACCESS_FAIL   0xA1
#define AP_NV_ACCESS_WRITE_SUCESS 0xA2
#define AP_NV_ACCESS_READ_SUCESS 0xA3
typedef struct {
    MSG_HEAD_T msg_head;
    unsigned short resp;
}nv_access_resp_t;

typedef struct
{
    uint32    adc[2];           // calibration of ADC, two test point
    uint32    battery[2];       // calibraton of battery(include resistance), two test point
    uint32    reserved[8];      // reserved for feature use.
} AP_ADC_T;

typedef struct
{
    MSG_HEAD_T  msg_head;
    TOOLS_DIAG_AP_CNF_T diag_ap_cnf;
    TOOLS_AP_ADC_REQ_T ap_adc_req;
}MSG_AP_ADC_CNF;
static unsigned char g_usb_buf_dest[8*1024];
static int power_off_Flag = 0;  //add by kenyliu in 2013 06 20 for bug 146310
static int force_nv_sync = 0;

static int AccessADCDataFile(uint8 flag, char *lpBuff, int size)
{
#ifdef CONFIG_EMMC_BOOT
    block_dev_desc_t *p_block_dev = NULL;
    p_block_dev = get_dev("mmc", 1);
    if(NULL == p_block_dev){
        return 0;
    }
    if(nv_read_flag == 0){
        if(-1 == Calibration_read_partition(p_block_dev, PARTITION_PROD_INFO4, (char *)nv_buffer,sizeof(nv_buffer)))
            return 0;
        nv_read_flag = 1;
    }
    printf("EMC_Read:nv_buffer[255]=0x%x \n",nv_buffer[255]);
    if(nv_read_flag == 1){
        if (flag == 0){
            if(nv_buffer[255] != 0x5a5a5a5a)
                return 0;
            memcpy(lpBuff,&nv_buffer[0],size);
        } else {
            nv_buffer[255] = 0x5a5a5a5a;
            memcpy(&nv_buffer[0],lpBuff,size);
            if(-1 == Calibration_write_partition(p_block_dev, PARTITION_PROD_INFO4, (char *)nv_buffer,sizeof(nv_buffer)))
                return 0;
        }
    }
    return size;
#else
#ifdef CONFIG_SC7710G2
    char *file_partition = "/productinfo";
    char *file_name = "/productinfo/adc_data";
    int ret = 0;

    ensure_path_mounted(file_partition);
    if (flag == 0) // read ADC data
    {
        ret = cmd_yaffs_ls_chk(file_name);
        if(ret > 0 && ret <= size)
        {
            printf("\n file: %s exist", file_name);
            cmd_yaffs_mread_file(file_name, (unsigned char *)lpBuff);
        }
        else
        {
            printf("\n file size error: ret = %d, size = %d", ret, size);
        }
    }
    else if (flag == 1)  //write ADC data
    {
        cmd_yaffs_mwrite_file(file_name, lpBuff, size);
        ret = size;
    }
    else
    {
        ret = 0;
    }
    //    ensure_path_umounted(file_partition);
    return ret;
#else
    char *file_partition = "modem";
    int ret = 0;

    if (flag == 0) // read ADC data
    {
        nand_part_read(file_partition,lpBuff,size);
        return size;
    }
    else if (flag == 1)  //write ADC data
    {

        nand_part_write(file_partition,lpBuff,size);
        ret = size;
    }
    else
    {
        ret = 0;
    }
    return ret;
#endif
#endif
}

/*copy from packet.c and modify*/
static int untranslate_packet_header(char *dest,char *src,int size, int unpackSize)
{
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
                if(src[i] != 0x7e)
                {
                    status = 2;
                    dest[translated_size++] = src[i];
                }
                break;
            case 2:
                if(src[i] == 0x7E)
                {
                    unsigned short crc;
                    crc = crc_16_l_calc((char const *)dest,translated_size-2);
                    return translated_size;
                }else
                {
                    if((dest[translated_size-1] == 0x7D)&&(!flag))
                    {
                        flag = 1;
                        if(src[i] == 0x5E)
                        {
                            dest[translated_size-1] = 0x7E;
                        }
                        else if(src[i] == 0x5D)
                        {
                            dest[translated_size-1] = 0x7D;
                        }
                    }
                    else
                    {
                        flag = 0;
                        dest[translated_size++] = src[i];
                    }

                    if (translated_size >= unpackSize+1 && unpackSize != -1)
                    {
                        return translated_size;
                    }
                }
                break;
        }
    }

    return translated_size;
}

static int translate_packet(char *dest,char *src,int size)
{
    int i;
    int translated_size = 0;

    dest[translated_size++] = 0x7E;

    for(i=0;i<size;i++){
        if(src[i] == 0x7E){
            dest[translated_size++] = 0x7D;
            dest[translated_size++] = 0x5E;
        } else if(src[i] == 0x7D) {
            dest[translated_size++] = 0x7D;
            dest[translated_size++] = 0x5D;
        } else
            dest[translated_size++] = src[i];
    }
    dest[translated_size++] = 0x7E;
    return translated_size;
}

static uint8 is_adc_calibration(char *dest, int destSize, char *src,int srcSize)
{
    int translated_size = 0;
    int msghead_size = sizeof(MSG_HEAD_T);

    memset(dest, 0, destSize);
    translated_size = untranslate_packet_header(dest, src, srcSize, msghead_size);
    if (translated_size >= msghead_size )
    {
        MSG_HEAD_T* lpHeader = (MSG_HEAD_T *)dest;
        if (DIAG_AP_F  == lpHeader->type)
        {
            TOOLS_DIAG_AP_CMD_T *lpAPCmd =(TOOLS_DIAG_AP_CMD_T *)(lpHeader+1);
            memset(dest, 0, destSize);
            translated_size = untranslate_packet_header(dest, src, srcSize, -1);

            switch (lpAPCmd->cmd)
            {
                case DIAG_AP_CMD_ADC:
                    {
                        TOOLS_AP_ADC_REQ_T *lpAPADCReq =(TOOLS_AP_ADC_REQ_T *)(lpAPCmd+1);
                        if (lpAPADCReq->operate == 0)
                        {
                            return AP_ADC_CALIB;
                        }
                        else if (lpAPADCReq->operate == 1)
                        {
                            return AP_ADC_LOAD;
                        }
                        else if (lpAPADCReq->operate == 2)
                        {
                            return AP_ADC_SAVE;
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    break;

                default:
                    break;
            }
        } else if(DIAG_POWER_SUPPLY_F  == lpHeader->type){
            return AP_GET_VOLT;
        } else if(DIAG_CURRENT_TEST_F == lpHeader->type){
            printf("DIAG_CURRENT_TEST_F command is received! sub_command is: 0x%02x\n\n",lpHeader->subtype);
            switch(lpHeader->subtype){
                case 0://CURRENT_TEST_STOP
                    break;
                case 1://CURRENT_TEST_RF_CLOSED
                    break;
                case 2://CURRENT_TEST_DEEP_SLEEP
                    break;
                case 3://CURRENT_TEST_LED_ON
                    break;
                case 4://CURRENT_TEST_VIBRATOR_ON
                    break;
                case 5://CURRENT_TEST_RX_MIN_PEAK_VALUE
                    break;
                case 6://CURRENT_TEST_TX_MAX_PEAK_VALUE
                    break;
                case 7://CURRENT_TEST_CHARGING
                    break;
                case 8://CURRENT_TEST_LED_OFF
                    break;
                case 9://CURRENT_TEST_VIBRATOR_OFF
                    break;
                case 10://CURRENT_TEST_DEEP_GET_SLEEP_FLAG
                    break;
                case 11://CURRENT_TEST_DEEP_SLEEP_FLAG_ENABLE
                    break;
                case 12://CURRENT_TEST_DEEP_SLEEP_FLAG_DISABLE
                    break;
                case 13://CURRENT_TEST_UART_ENABLESLEEP
                    break;
                case 14://CURRENT_TEST_POWER_OFF
                    //add by kenyliu in 2013 06 20 for bug 146310
                    power_off_Flag =0xE;
                    //end kenyliu
                    break;
                case 15://CURRENT_TEST_DEEP_SLEEP_WAKEUP
                    break;
                default:
                    break;
            }
        }else if(0x2B == lpHeader->type){
            //main command: 0x2B,  WCDMA NV access
            //sub command : 0x2,   save to flash
            switch(lpHeader->subtype){
                case 0x2:
                    force_nv_sync = 1;
                    break;
                default:
                    break;
            }
        } else if(DIAG_DIRECT_NV == lpHeader->type){
            //imei+bt+wifi access move from cp to ap
            unsigned short mode = lpHeader->subtype&0x80;
            printf("subcmd: 0x%02x,mode: 0x%02x\n",lpHeader->subtype,mode);
            printf("expected size = %d,real size = %d\n",lpHeader->len+2,srcSize);
            if(!mode){
                //write mode
                printf("Entry write mode ...\n");
                return (uint8) nv_access_write(lpHeader->subtype&0x7f,(unsigned char *)src+sizeof(MSG_HEAD_T)+1);
            }else{
                //read mode
                printf("Entry read mode ...\n");
            }
        }else if( 0x68 == lpHeader->type){
            //now only deal with AT+SPDIAG="AT+ETSRESET"
            unsigned char *at_cmd[36];
            printf("got a at cmd:\n");
            memcpy(at_cmd, (unsigned char *)src+sizeof(MSG_HEAD_T)+1, lpHeader->len-sizeof(MSG_HEAD_T));

            if ( 0==strncmp(at_cmd,"AT+SPDIAG=\"AT+ETSRESET\"\r\n", 24) ) {
                printf("factory default!\n");
                #define AP_AT_OK 0xD0
                #define AP_AT_ERROR 0xD1
                if ( -1 == erase_mtd_partition("userdata") )
                    return AP_AT_ERROR;
                if ( -1 == erase_mtd_partition("cache") )
                    return AP_AT_ERROR;
                return AP_AT_OK;
            }
       }
    }

    return 0;
}

static uint32 ap_adc_calibration(uint32 channel, MSG_AP_ADC_CNF *pMsgADC)
{
    volatile uint32 adc_channel = 0, adc_result = 0;
    int i = 0;
    adc_channel = channel;

    if (adc_channel <= 8)
    {
        if (adc_channel == 0)
        {
            adc_channel = 1;
        }

        adc_result = 0;
        for(; i < 16; i++)
        {
            adc_result += ADC_GetValue(adc_channel-1, 0);
        }
        adc_result >>= 4;
        pMsgADC->diag_ap_cnf.status  = 0;
        pMsgADC->ap_adc_req. parameters[0]= (uint16)(adc_result&0xFFFF);
    }
    else
    {
        pMsgADC->diag_ap_cnf.status  = 1;
        pMsgADC->ap_adc_req. parameters[0] = 0xFFFF;
    }

    printf("\n ReadADC: ( channel = %x)", adc_channel);

    return adc_result;
}

static int ap_adc_save(TOOLS_AP_ADC_REQ_T *pADCReq, MSG_AP_ADC_CNF *pMsgADC)
{
    AP_ADC_T adcValue = {0};
    int ret = 0;
    ret = AccessADCDataFile(1, pADCReq->parameters, sizeof(pADCReq->parameters));
    if (ret > 0)
    {
        pMsgADC->diag_ap_cnf.status = 0;
    }
    else
    {
        pMsgADC->diag_ap_cnf.status = 1;
    }

    return ret;
}
static int ap_adc_load(MSG_AP_ADC_CNF *pMsgADC)
{
    int ret = AccessADCDataFile(0, pMsgADC->ap_adc_req.parameters, sizeof(pMsgADC->ap_adc_req.parameters));
    if (ret > 0)
    {
        pMsgADC->diag_ap_cnf.status = 0;
    }
    else
    {
        pMsgADC->diag_ap_cnf.status = 1;
    }

    return ret;
}
static uint32 ap_get_voltage(uint32 channel, MSG_AP_ADC_CNF *pMsgADC)
{
    volatile uint32 adc_channel = 0, adc_result = 0;
    uint32 voltage = 0;
    uint32  *para=NULL;
    int i = 0;
    MSG_HEAD_T *msg;
    adc_channel = channel;

    if (adc_channel <= 8)
    {
        if (adc_channel == 0)
        {
            adc_channel = 1;
        }

        adc_result = 0;
        for(; i < 16; i++)
        {
            adc_result += ADC_GetValue(adc_channel-1, 0);
        }
        adc_result >>= 4;
        voltage = CHGMNG_AdcvalueToVoltage(adc_result);
        msg = (MSG_HEAD_T *)pMsgADC;
        para = (msg+1);
        *para = (voltage/10);
    }
    else
    {
        pMsgADC->diag_ap_cnf.status  = 1;
        pMsgADC->ap_adc_req. parameters[0] = 0xFFFF;
    }
    pMsgADC->msg_head.len = 12;
    printf("\n Read voltage : %d\n", voltage);

    return voltage;
}
uint8 ap_adc_process(int flag, char * src, int size, MSG_AP_ADC_CNF * pMsgADC)
{
    MSG_HEAD_T *lpHeader = (MSG_HEAD_T *)src;
    TOOLS_DIAG_AP_CMD_T *lpAPCmd =(TOOLS_DIAG_AP_CMD_T *)(lpHeader+1);
    TOOLS_AP_ADC_REQ_T *lpApADCReq = (TOOLS_AP_ADC_REQ_T *)(lpAPCmd+1);
    memcpy(&(pMsgADC->msg_head), lpHeader, sizeof(MSG_HEAD_T));
    pMsgADC->msg_head.len = sizeof(TOOLS_DIAG_AP_CNF_T)+sizeof(TOOLS_AP_ADC_REQ_T)+sizeof(MSG_HEAD_T);
    pMsgADC->diag_ap_cnf.length = sizeof(TOOLS_AP_ADC_REQ_T);
    memcpy(&(pMsgADC->ap_adc_req), lpApADCReq, sizeof(TOOLS_AP_ADC_REQ_T));

    switch (flag)
    {
        case AP_ADC_CALIB:
            {
                uint32 channel = lpApADCReq->parameters[0];
                ap_adc_calibration(channel, pMsgADC);
            }
            break;
        case AP_ADC_LOAD:
            ap_adc_load(pMsgADC);
            break;
        case AP_ADC_SAVE:
            ap_adc_save(lpApADCReq, pMsgADC);
            break;
        case AP_GET_VOLT:
             {
                 extern void CHG_Init (void);
                 CHG_Init();//re-load adc value
             }
            ap_get_voltage(6,pMsgADC);
            break;
        default:
            return 0;
    }
    return 1;
}
#endif
int  write_adc_calibration_data(char *data, int size)
{
    int ret = 0;
#ifdef CONFIG_AP_ADC_CALIBRATION
    ret = AccessADCDataFile(1, data, size);
#endif
    return ret;
}
int  read_adc_calibration_data(char *buffer,int size)
{
#ifdef CONFIG_AP_ADC_CALIBRATION
    int ret;
    if(size > 48)
        size = 48;
    ret = AccessADCDataFile(0, buffer, size);
    if(ret > 0)
        return size;
#endif
    return 0;
}
uint32 ap_calibration_proc(uint8 *data,uint32 count,uint8 *out_msg)
{
#ifdef CONFIG_AP_ADC_CALIBRATION
    int adcFlag = 0;
    int index = 0;

    if(count > 0){
        adcFlag = is_adc_calibration(g_usb_buf_dest, sizeof(g_usb_buf_dest), data, count );

        if(adcFlag == AP_NV_ACCESS_IGNOR||
                adcFlag == AP_NV_ACCESS_FAIL||
                adcFlag == AP_NV_ACCESS_READ_SUCESS||
                adcFlag == AP_NV_ACCESS_WRITE_SUCESS){
            nv_access_resp_t nv_access_resp ={0};
            MSG_HEAD_T* lpHeader = (MSG_HEAD_T *)g_usb_buf_dest;

            nv_access_resp.msg_head.seq_num = lpHeader->seq_num;
            nv_access_resp.msg_head.len = sizeof(nv_access_resp_t);
            nv_access_resp.msg_head.type = lpHeader->type;
            if(adcFlag == AP_NV_ACCESS_WRITE_SUCESS){
                nv_access_resp.msg_head.subtype = 1;//MSG_ACK
                nv_access_resp.resp = 0;//ERR_NONE:0
            }else if(adcFlag == AP_NV_ACCESS_FAIL){
                nv_access_resp.msg_head.subtype = 0;//MSG_NACK
                nv_access_resp.resp = 3;//CRC_ERR:1 CMD_ERR:2 SAVE_ERR:3 READ_ERR:4
            }else{
                nv_access_resp.msg_head.subtype = 0;//MSG_NACK
                nv_access_resp.resp = 2;//CRC_ERR:1 CMD_ERR:2 SAVE_ERR:3 READ_ERR:4
            }
            index = translate_packet(out_msg, &nv_access_resp, nv_access_resp.msg_head.len);
            count = 0;
            return index;
        }else if ( adcFlag == AP_AT_OK ) {
            MSG_HEAD_T* lpHeader = (MSG_HEAD_T *)g_usb_buf_dest;
            MSG_HEAD_T msg1 = {0};
            struct {
                MSG_HEAD_T msg_head;
                unsigned char at_resp[6];
            }__attribute__((packed)) msg_resp = {{0},{0}};
            printf("at cmd return adcFlag = %d\n", adcFlag);

            msg1.seq_num = lpHeader->seq_num;
            msg1.len = sizeof(MSG_HEAD_T);
            msg1.type = 0xD5;
            msg1.subtype = 0x00;
            index = translate_packet(out_msg, &msg1, msg1.len);

            msg_resp.msg_head.seq_num = lpHeader->seq_num;
            msg_resp.msg_head.len = sizeof(msg_resp);
            msg_resp.msg_head.type = 0x9C;
            msg_resp.msg_head.subtype = 0x00;
            memcpy(msg_resp.at_resp, "\x0D\x0A\x4F\x4B\x0D\x0A", 6);
            index += translate_packet(out_msg+index, &msg_resp, msg_resp.msg_head.len);

            count = 0;
            return index;
        }else if (adcFlag != 0){
            MSG_AP_ADC_CNF adcMsg = {0};
            printf("\n adcFlag = %d", adcFlag);

            ap_adc_process(adcFlag, g_usb_buf_dest, count, &adcMsg);
            index = translate_packet(out_msg, &adcMsg, adcMsg.msg_head.len);
            count = 0;

            return index;
        }
    }
#endif
    return 0;
}
//add by kenyliu in 2013 06 20 for bug 146310
int get_adc_flag(void)
{
#ifdef CONFIG_AP_ADC_CALIBRATION
    return power_off_Flag;
#endif
}
//end kenyliu
int get_nv_sync_flag(void){
    return force_nv_sync;
}
void set_nv_sync_flag(int flag){
    force_nv_sync = flag;
}
