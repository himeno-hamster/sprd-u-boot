#include <common.h>
#include <asm/io.h>

#include <asm/arch/regs_adi.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/analog_reg_v3.h>

#ifdef DEBUG
#define debugf(fmt, args...) do { printf("%s(): ", __func__); printf(fmt, ##args); } while (0)
#else
#define debugf(fmt, args...)
#endif
#define ADC_CAL_TYPE_NO			0
#define ADC_CAL_TYPE_NV			1
#define ADC_CAL_TYPE_EFUSE		2

extern int read_adc_calibration_data(char *buffer,int size);
extern int sci_efuse_calibration_get(unsigned int * p_cal_data);
uint16_t adc_voltage_table[2][2] = {
	{3310, 4200},
	{2832, 3600},
};

uint32_t adc_cal_flag = 0;

uint16_t CHGMNG_AdcvalueToVoltage(uint16_t adcvalue)
{
	int32_t temp;
	temp = adc_voltage_table[0][1] - adc_voltage_table[1][1];
	temp = temp * (adcvalue - adc_voltage_table[0][0]);
	temp = temp / (adc_voltage_table[0][0] - adc_voltage_table[1][0]);

	debugf("uboot battery voltage:%d,adc4200:%d,adc3600:%d\n",
	       temp + adc_voltage_table[0][1], adc_voltage_table[0][0],
	       adc_voltage_table[1][0]);

	return temp + adc_voltage_table[0][1];
}

void CHG_TurnOn(void)
{
	ANA_REG_MSK_OR(ANA_APB_CHGR_CTL0, CHGR_PD_CLR_BIT, (CHGR_PD_SET_BIT | CHGR_PD_CLR_BIT));
}

void CHG_ShutDown(void)
{
	ANA_REG_MSK_OR(ANA_APB_CHGR_CTL0, CHGR_PD_SET_BIT, (CHGR_PD_SET_BIT | CHGR_PD_CLR_BIT));
}

void CHG_SetRecharge(void)
{
	ANA_REG_OR(ANA_APB_CHGR_CTL2, CHGR_RECHG_BIT);
}

uint32_t CHG_GetAdcCalType(void)
{
	return adc_cal_flag;
}

#ifndef CONFIG_FDL1
/* used to get adc calibration data from nv or efuse */
void get_adc_cali_data(void)
{
	unsigned int adc_data[64];
	int ret=0;

	adc_cal_flag = ADC_CAL_TYPE_NO;

#ifndef FDL_CHG_SP8830
	/* get voltage values from nv */
	ret = read_adc_calibration_data(adc_data,48);
	if((ret > 0) &&
			((adc_data[2] & 0xFFFF) < 4500 ) && ((adc_data[2] & 0xFFFF) > 3000) &&
			((adc_data[3] & 0xFFFF) < 4500 ) && ((adc_data[3] & 0xFFFF) > 3000)){
		debugf("adc_para from nv is 0x%x 0x%x \n",adc_data[2],adc_data[3]);
		adc_voltage_table[0][1]=adc_data[2] & 0xFFFF;
		adc_voltage_table[0][0]=(adc_data[2] >> 16) & 0xFFFF;
		adc_voltage_table[1][1]=adc_data[3] & 0xFFFF;
		adc_voltage_table[1][0]=(adc_data[3] >> 16) & 0xFFFF;
		adc_cal_flag = ADC_CAL_TYPE_NV;
	}
#endif
	/* get voltage values from efuse */
	if (adc_cal_flag == ADC_CAL_TYPE_NO){
		ret = sci_efuse_calibration_get(adc_data);
		if (ret > 0) {
			debugf("adc_para from efuse is 0x%x 0x%x \n",adc_data[0],adc_data[1]);
			adc_voltage_table[0][1]=adc_data[0] & 0xFFFF;
			adc_voltage_table[0][0]=(adc_data[0]>>16) & 0xFFFF;
			adc_voltage_table[1][1]=adc_data[1] & 0xFFFF;
			adc_voltage_table[1][0]=(adc_data[1] >> 16) & 0xFFFF;
			adc_cal_flag = ADC_CAL_TYPE_EFUSE;
		}
	}
}
#endif

#ifndef FDL_CHG_SP8830
enum sprd_adapter_type {
	ADP_TYPE_UNKNOW = 0,	//unknow adapter type
	ADP_TYPE_CDP = 1,	//Charging Downstream Port,USB&standard charger
	ADP_TYPE_DCP = 2,	//Dedicated Charging Port, standard charger
	ADP_TYPE_SDP = 4,	//Standard Downstream Port,USB and nonstandard charge
};
int sprd_charger_is_adapter(void)
{
	int ret = ADP_TYPE_UNKNOW;
	int charger_status;

	charger_status = sci_adi_read(ANA_REG_GLB_CHGR_STATUS)
	    & (BIT_CDP_INT | BIT_DCP_INT | BIT_SDP_INT);

	switch (charger_status) {
	case BIT_CDP_INT:
		ret = ADP_TYPE_CDP;
		break;
	case BIT_DCP_INT:
		ret = ADP_TYPE_DCP;
		break;
	case BIT_SDP_INT:
		ret = ADP_TYPE_SDP;
		break;
	default:
		break;
	}

	return ret;
}

#define REGS_FGU_BASE SPRD_ANA_FPU_PHYS
#define REG_FGU_START                   SCI_ADDR(REGS_FGU_BASE, 0x0000)
#define REG_FGU_CONFIG                  SCI_ADDR(REGS_FGU_BASE, 0x0004)
#define REG_FGU_VOLT_VAL                SCI_ADDR(REGS_FGU_BASE, 0x0020)
#define REG_FGU_OCV_VAL                 SCI_ADDR(REGS_FGU_BASE, 0x0024)
#define REG_FGU_POCV_VAL                SCI_ADDR(REGS_FGU_BASE, 0x0028)
#define REG_FGU_CURT_VAL                SCI_ADDR(REGS_FGU_BASE, 0x002c)
#define BIT_VOLT_H_VALID                ( BIT(12) )
#define BITS_VOLT_DUTY(_x_)             ( (_x_) << 5 & (BIT(5)|BIT(6)) )

#define mdelay(_ms) udelay(_ms*1000)
unsigned int fgu_vol, fgu_cur;
void fgu_init(void)
{
	sci_adi_set(ANA_REG_GLB_MP_MISC_CTRL, (BIT(1)));
	sci_adi_write(ANA_REG_GLB_DCDC_CTRL2, (4 << 8), (7 << 8));

	sci_adi_set(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_FGU_EN);
	sci_adi_set(ANA_REG_GLB_RTC_CLK_EN, BIT_RTC_FGU_EN | BIT_RTC_FGUA_EN);
	//sci_adi_clr(REG_FGU_CONFIG, BIT_VOLT_H_VALID);
	//sci_adi_clr(REG_FGU_CONFIG, BIT_AD1_ENABLE);
	sci_adi_write(REG_FGU_CONFIG, BITS_VOLT_DUTY(3), BITS_VOLT_DUTY(3)|BIT_VOLT_H_VALID);
	//mdelay(1000);

	fgu_vol = 0; //sci_adi_read(REG_FGU_VOLT_VAL);

	fgu_cur = 0; //sci_adi_read(REG_FGU_CURT_VAL);
	debugf("fgu_init fgu_vol 0x%x fgu_cur 0x%x \n", fgu_vol, fgu_cur);
}

void CHG_Init(void)
{
	//set charge current 500mA(USB) or 600mA(AC)
	if (charger_connected()) {
		enum sprd_adapter_type adp_type = sprd_charger_is_adapter();
		if (adp_type == ADP_TYPE_CDP || adp_type == ADP_TYPE_DCP) {
			debugf("uboot adp AC\n");
			ANA_REG_MSK_OR(ANA_APB_CHGR_CTL1,
				       (6 << CHGR_CHG_CUR_SHIFT) &
				       CHGR_CHG_CUR_MSK, CHGR_CHG_CUR_MSK);
		} else {
			debugf("uboot adp USB\n");
			ANA_REG_MSK_OR(ANA_APB_CHGR_CTL1,
				       (4 << CHGR_CHG_CUR_SHIFT) &
				       CHGR_CHG_CUR_MSK, CHGR_CHG_CUR_MSK);
		}
	}

	CHG_SetRecharge();
	ANA_REG_OR(ANA_APB_CHGR_CTL2, CHGR_CC_EN_BIT);

	get_adc_cali_data();
	fgu_init();
}
#endif
