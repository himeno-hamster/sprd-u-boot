#include <common.h>
#include <asm/io.h>

#include <asm/arch/regs_adi.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/analog_reg_v3.h>

uint16_t adc_voltage_table[2][2] =
{
#ifdef CONFIG_SC7710G2
    {3300, 4200},
    {2800, 3600},
#else
	{928,4200},
	{796,3600},
#endif
};
uint16_t CHGMNG_AdcvalueToVoltage (uint16_t adcvalue)
{
	int32_t temp;
	temp = adc_voltage_table[0][1] - adc_voltage_table[1][1];
	temp = temp * (adcvalue - adc_voltage_table[0][0]);
	temp = temp / (adc_voltage_table[0][0] - adc_voltage_table[1][0]);

	printf("mingwei uboot adcvalue:%u,vol:%d,adc4200:%d,adc3600:%d\n", (unsigned int)adcvalue, temp + adc_voltage_table[0][1],
		adc_voltage_table[0][0], adc_voltage_table[1][0]);

	return temp + adc_voltage_table[0][1];
}
void CHG_TurnOn (void)
{
	ANA_REG_AND (ANA_CHGR_CTL0,~CHGR_PD_BIT);
}

void CHG_ShutDown (void)
{
	ANA_REG_OR (ANA_CHGR_CTL0,CHGR_PD_BIT);
}

void CHG_SetRecharge (void)
{
	ANA_REG_OR (ANA_CHGR_CTL0,CHGR_RECHG_BIT);
}

void CHG_Init (void)
{
	unsigned int efuse_cal_data[2] = {0};
	if(sci_efuse_calibration_get(efuse_cal_data))
	{
		printf("sci_efuse_calibration_get OK");
		adc_voltage_table[0][1]=efuse_cal_data[0]&0xffff;
		adc_voltage_table[0][0]=(efuse_cal_data[0]>>16)&0xffff;
		adc_voltage_table[1][1]=efuse_cal_data[1]&0xffff;
		adc_voltage_table[1][0]=(efuse_cal_data[1]>>16)&0xffff;
	}
#ifdef CONFIG_AP_ADC_CALIBRATION
        {
                extern int read_adc_calibration_data(char *buffer,int size);
                unsigned int *adc_data;
                int ret=0;
                adc_data = malloc(64);
                if(adc_data){
                        ret = read_adc_calibration_data(adc_data,48);
                        if((ret > 0) &&
                           ((adc_data[2]&0xffff) < 4500 )&&((adc_data[2]&0xffff) > 3000)&&
                           ((adc_data[3]&0xffff) < 4500 )&&((adc_data[3]&0xffff) > 3000)){
                                printf("adc_para = 0x%x 0x%x \n",adc_data[2],adc_data[3]);
                                adc_voltage_table[0][1]=adc_data[2]&0xffff;
                                adc_voltage_table[0][0]=(adc_data[2]>>16)&0xffff;
                                adc_voltage_table[1][1]=adc_data[3]&0xffff;
                                adc_voltage_table[1][0]=(adc_data[3]>>16)&0xffff;
                        }
                        free(adc_data);  
                }
          }
#endif

	CHG_SetRecharge();
}

