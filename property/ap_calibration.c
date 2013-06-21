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

extern int nand_part_write(char *partname,char *buffer, int size);
extern int nand_part_read(char *partname,char *buffer, int size);
#ifdef CONFIG_AP_ADC_CALIBRATION

#define AP_ADC_CALIB    1
#define AP_ADC_LOAD     2
#define AP_ADC_SAVE     3
#define	AP_GET_VOLT	4

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
		}
		//add by kenyliu in 2013 06 20 for bug 146310
		else if((DIAG_CURRENT_TEST_F == lpHeader->type) && (0xE == lpHeader->subtype)){
				power_off_Flag =0xE;
			}
		//end kenyliu
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
	uint32	voltage = 0;
	uint32  *para=NULL;
        int i = 0;
	MSG_HEAD_T	*msg;
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
			ap_get_voltage(6,pMsgADC);
		break;
		default:
		return 0;                     
	}
	return 1;
}
#endif
int 	write_adc_calibration_data(char *data, int size)
{
	int ret = 0;
#ifdef CONFIG_AP_ADC_CALIBRATION
	ret = AccessADCDataFile(1, data, size);
#endif
	return ret;
}
int 	read_adc_calibration_data(char *buffer,int size)
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

                if (adcFlag != 0)
                {
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
int get_adc_flag()
{
  #ifdef CONFIG_AP_ADC_CALIBRATION
	return power_off_Flag;
  #endif
}
//end kenyliu
