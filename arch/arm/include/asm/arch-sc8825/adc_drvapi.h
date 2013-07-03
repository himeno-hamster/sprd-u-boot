/******************************************************************************
 ** File Name:      adc_drvapi.h                                                  *
 ** Author:         hao.liu                                             *
 ** DATE:           06/12/2010                                                *
 ** Copyright:      2002 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic input and output operations   *
 **                 on hardware, it can be treated as a hardware abstract     *
 **                 layer interface.                                          *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               * ** 06/12/2010     hao.liu    Create.                                   *
 ******************************************************************************/

#ifndef _ADC_DRVAPI_H_
#define _ADC_DRVAPI_H_

#define ADC_SCALE_3V       0 
#define ADC_SCALE_1V2   1

typedef enum
{
	ADC_CHANNEL_0 = 0,
	ADC_CHANNEL_1 = 1,
	ADC_CHANNEL_2 = 2,
	ADC_CHANNEL_3 = 3,
	ADC_CHANNEL_PROG = 4,
	ADC_CHANNEL_VBAT = 5,
	ADC_CHANNEL_VCHGSEN = 6,
	ADC_CHANNEL_VCHGBG = 7,
	ADC_CHANNEL_ISENSE = 8,
	ADC_CHANNEL_TPYD = 9,
	ADC_CHANNEL_TPYU = 10,
	ADC_CHANNEL_TPXR = 11,
	ADC_CHANNEL_TPXL = 12,
	ADC_CHANNEL_DCDCCORE = 13,
	ADC_CHANNEL_DCDCARM = 14,
	ADC_CHANNEL_DCDCMEM = 15,
	ADC_CHANNEL_DCDCLDO = 16,
	ADC_CHANNEL_VBATBK = 17,
	ADC_CHANNEL_HEADMIC = 18,
	ADC_CHANNEL_LDO0 = 19,	/* ldo rf/abb/cama */
	ADC_CHANNEL_LDO1 = 20,	/* ldo v3v/v28/vsim0/vsim1/cammot/sd0/usb/dvdd18/v25 */
	ADC_CHANNEL_LDO2 = 21,	/* ldo camio/camcore/cmmb1v2/cmmb1v8/v18/sd1/sd3/ */
	ADC_MAX = 22
} adc_channel;
typedef enum{false, true} bool;

#ifdef CONFIG_MACH_SP6810A
#define ADC_CHANNEL_TEMP 0
#else
#define ADC_CHANNEL_TEMP 1
#endif

#ifdef __cplusplus
extern   "C"
{
#endif

void ADC_Init (void);
int32_t ADC_GetValues(adc_channel id, bool scale, uint8_t num, int32_t * p_buf);
int32_t ADC_GetValue(adc_channel adcSource, bool scale);

#ifdef __cplusplus
}
#endif

#endif  // _ADC_DRVAPI_H_

