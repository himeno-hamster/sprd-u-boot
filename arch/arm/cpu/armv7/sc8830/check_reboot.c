#include <asm/arch/sci_types.h>
#include <asm/arch/sc_reg.h>
#include <boot_mode.h>
#include <asm/arch/sprd_reg.h>
#include <asm/arch/sprd_eic.h>
#include <asm/arch/rtc_reg_v3.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/chip_x35/sprd_reg_aon_apb.h>
#include <asm/arch/chip_drv_config.h>


#define   HWRST_STATUS_POWERON_MASK 		(0xf0)
#define   HWRST_STATUS_RECOVERY 		(0x20)
#define   HWRST_STATUS_FASTBOOT 		(0X30)
#define   HWRST_STATUS_NORMAL 			(0X40)
#define   HWRST_STATUS_ALARM 			(0X50)
#define   HWRST_STATUS_SLEEP 			(0X60)
#define   HWRST_STATUS_SPECIAL			(0x70)
#define   HWRST_STATUS_CALIBRATION			(0x90)
#define   HWRST_STATUS_PANIC			(0x80)
#define   HWRST_STATUS_AUTODLOADER (0Xa0)
#define   HWRST_STATUS_NORMAL2			(0Xf0)

#define   HW_7SRST_STATUS			(0x80)
#define   SW_7SRST_STATUS			(0x1000)

#ifdef DEBUG
#define debugf(fmt, args...) do { printf("%s(): ", __func__); printf(fmt, ##args); } while (0)
#else
#define debugf(fmt, args...)
#endif


extern int hw_watchdog_rst_pending(void);
unsigned check_reboot_mode(void)
{
	unsigned val, rst_mode= 0;
	unsigned hw_rst_mode = ANA_REG_GET(ANA_REG_GLB_POR_SRC_FLAG);
	debugf("hw_rst_mode==%x\n", hw_rst_mode);

	rst_mode = ANA_REG_GET(ANA_REG_GLB_POR_RST_MONITOR);
	rst_mode &= 0x7FFF;
	ANA_REG_SET(ANA_REG_GLB_POR_RST_MONITOR, 0); //clear flag

	debugf("rst_mode==%x\n",rst_mode);
	if(hw_watchdog_rst_pending()){
		debugf("hw watchdog rst int pending\n");
		if(rst_mode == HWRST_STATUS_RECOVERY)
			return RECOVERY_MODE;
		else if(rst_mode == HWRST_STATUS_FASTBOOT)
			return FASTBOOT_MODE;
		else if(rst_mode == HWRST_STATUS_NORMAL)
			return NORMAL_MODE;
		else if(rst_mode == HWRST_STATUS_NORMAL2)
			return WATCHDOG_REBOOT;
		else if(rst_mode == HWRST_STATUS_ALARM)
			return ALARM_MODE;
		else if(rst_mode == HWRST_STATUS_SLEEP)
			return SLEEP_MODE;
		else if(rst_mode == HWRST_STATUS_CALIBRATION)
			return CALIBRATION_MODE;
		else if(rst_mode == HWRST_STATUS_PANIC)
			return PANIC_REBOOT;
		else if(rst_mode == HWRST_STATUS_SPECIAL)
			return SPECIAL_MODE;
		else if(rst_mode == HWRST_STATUS_AUTODLOADER)
			return AUTODLOADER_REBOOT;
		else{
			debugf(" a boot mode not supported\n");
			return 0;
		}
	}else{
		debugf("no hw watchdog rst int pending\n");
		if(rst_mode == HWRST_STATUS_NORMAL2)
			return UNKNOW_REBOOT_MODE;
		else if(hw_rst_mode & HW_7SRST_STATUS)
		{
			return UNKNOW_REBOOT_MODE;
		}
		else
			return 0;
	}

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
	#if defined (CONFIG_SPX15)
	sci_glb_set(REG_AON_APB_APB_EB0,BIT_AON_GPIO_EB | BIT_EIC_EB);
	sci_glb_set(REG_AON_APB_APB_RTC_EB,BIT_EIC_RTC_EB);
	sci_adi_set(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_EIC_EN);
	#else
	sci_glb_set(REG_AON_APB_APB_EB0,BIT_GPIO_EB | BIT_EIC_EB);
	sci_glb_set(REG_AON_APB_APB_RTC_EB,BIT_EIC_RTC_EB);
	sci_adi_set(ANA_REG_GLB_ARM_MODULE_EN, BIT_ANA_EIC_EN | BIT_ANA_GPIO_EN);
	#endif
	sci_adi_set(ANA_REG_GLB_RTC_CLK_EN,BIT_RTC_EIC_EN);

	ANA_REG_SET(ADI_EIC_MASK, 0xff);

	udelay(3000);

	int status = ANA_REG_GET(ADI_EIC_DATA);
	status = status & (1 << 2);

	debugf("power_button_pressed eica status 0x%x\n", status );
	
	return !status;//low level if pb hold

}

int charger_connected(void)
{
	sprd_eic_request(EIC_CHG_INT);
#ifndef CONFIG_SPX15
	if((ANA_REG_GET(ANA_APB_CHGR_CTL2) & (1 << 8)) == 0){
		debugf("---err---ANA_APB_CHGR_CTL2 0x%x-------\n", ANA_REG_GET(ANA_APB_CHGR_CTL2));
		while(1);
		ANA_REG_OR(ANA_APB_CHGR_CTL2, 1<<8);
		debugf("---set bit8---ANA_APB_CHGR_CTL2 0x%x-------\n", ANA_REG_GET(ANA_APB_CHGR_CTL2));
	}
#endif
	udelay(3000);
	debugf("eica status %x\n", sprd_eic_get(EIC_CHG_INT));
	return !!sprd_eic_get(EIC_CHG_INT);
}

int alarm_triggered(void)
{
	//printf("ANA_RTC_INT_RSTS is 0x%x\n", ANA_RTC_INT_RSTS);
	debugf("value of it 0x%x\n", ANA_REG_GET(ANA_RTC_INT_RSTS));
	return ANA_REG_GET(ANA_RTC_INT_RSTS) & BIT_4;
}

