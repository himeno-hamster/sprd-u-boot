#include <asm/arch/sci_types.h>
#include <asm/arch/sc_reg.h>
#include <boot_mode.h>
#include <asm/arch/sprd_reg.h>
#include <asm/arch/rtc_reg_v3.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/adi_hal_internal.h>

#define   HWRST_STATUS_POWERON_MASK 		(0xf0)
#define   HWRST_STATUS_RECOVERY 		(0x20)
#define   HWRST_STATUS_FASTBOOT 		(0X30)
#define   HWRST_STATUS_NORMAL 			(0X40)
#define   HWRST_STATUS_ALARM 			(0X50)
#define   HWRST_STATUS_SLEEP 			(0X60)
#define   HWRST_STATUS_NORMAL2 			(0Xf0)

unsigned check_reboot_mode(void)
{
	unsigned rst_mode= 0;

	rst_mode = ANA_REG_GET(ANA_REG_GLB_POR_RST_MONITOR);
	rst_mode &= 0x7FFF;
	ANA_REG_SET(ANA_REG_GLB_POR_RST_MONITOR, 0); //clear flag

	return NORMAL_MODE;
}

void reboot_devices(unsigned reboot_mode)
{
	unsigned rst_mode = 0;
	if(reboot_mode == RECOVERY_MODE){
		rst_mode = HWRST_STATUS_RECOVERY;
	}
	else if(reboot_mode == FASTBOOT_MODE){
		rst_mode = HWRST_STATUS_FASTBOOT;
	}else if(reboot_mode == NORMAL_MODE){
		rst_mode = HWRST_STATUS_NORMAL;
	}else{
		rst_mode = 0;
	}

	ANA_REG_SET(ANA_REG_GLB_POR_RST_MONITOR, rst_mode);
	reset_cpu(0);
}
void power_down_devices(unsigned pd_cmd)
{
	power_down_cpu(0);
}

int power_button_pressed(void)
{
	//ANA_REG_OR(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_EIC_EN); //EIC enable
	//ANA_REG_OR(ANA_REG_GLB_RTC_CLK_EN,    BIT_RTC_EIC_EN); //EIC RTC enable
	//ANA_REG_SET(ADI_EIC_MASK, 0xff);
	//sprd_eic_init();
	sprd_eic_request(162);
	udelay(3000);
	//int status = ANA_REG_GET(ADI_EIC_DATA);
	//printf("eica status %x\n", status);
	//return !!(status & (1 << 3)/*PBINT*/);//low level if pb hold
	printf("eica status %x\n", sprd_eic_get(162));
	return !!sprd_eic_get(162);
}

int charger_connected(void)
{
	//ANA_REG_OR(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_EIC_EN); //EIC enable
	//ANA_REG_OR(ANA_REG_GLB_RTC_CLK_EN,    BIT_RTC_EIC_EN); //EIC RTC enable
	//ANA_REG_SET(ADI_EIC_MASK, 0xff);
	//sprd_eic_init();
	sprd_eic_request(160);
	udelay(3000);
	//int status = ANA_REG_GET(ADI_EIC_DATA);
	//printf("charger_connected eica status %x\n", status);
	//return !!(status & (1 << 2));
	printf("eica status %x\n", sprd_eic_get(160));
	return !!sprd_eic_get(160);
}

int alarm_triggered(void)
{
	//printf("ANA_RTC_INT_RSTS is 0x%x\n", ANA_RTC_INT_RSTS);
	printf("value of it 0x%x\n", ANA_REG_GET(ANA_RTC_INT_RSTS));
	return ANA_REG_GET(ANA_RTC_INT_RSTS) & BIT_4;
}

