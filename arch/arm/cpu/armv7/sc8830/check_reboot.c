#include <asm/arch/sci_types.h>
#include <asm/arch/sc_reg.h>
#include <asm/arch/adi_hal_internal.h>
#include <boot_mode.h>
#include <asm/arch/gpio.h>
#include <asm/arch/asm_generic_gpio.h>
#include <asm/arch/gpio_phy.h>
#include <asm/arch/rtc_reg_v3.h>
#include <asm/arch/mfp.h>
#include <asm/arch/gpio.h>

unsigned check_reboot_mode(void)
{
	unsigned rst_mode= 0;

	rst_mode = ANA_REG_GET(ANA_APB_POR_RST_MONTR);
	rst_mode &= 0x7FFF;
	ANA_REG_SET(ANA_APB_POR_RST_MONTR, 0); //clear flag

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

    ANA_REG_SET(ANA_APB_POR_RST_MONTR, rst_mode);
    reset_cpu(0);
}
void power_down_devices(unsigned pd_cmd)
{
    power_down_cpu(0);
}

int power_button_pressed(void)
{
	ANA_REG_OR(ANA_APB_MOD_EN, BIT_3); //EIC enable
	ANA_REG_OR(ANA_APB_CLK_EN, BIT_3); //EIC RTC enable
	ANA_REG_SET(ADI_EIC_MASK, 0xff);
	udelay(3000);
	int status = ANA_REG_GET(ADI_EIC_DATA);
	printf("eica status %x\n", status);
#ifdef CONFIG_MACH_SP8830FPGA
	return 0;
#endif
	return !!(status & (1 << 3)/*PBINT*/);//low level if pb hold
}

int charger_connected(void)
{
	ANA_REG_OR(ANA_APB_MOD_EN, BIT_3); //EIC enable
	ANA_REG_OR(ANA_APB_CLK_EN, BIT_3); //EIC RTC enable
	ANA_REG_SET(ADI_EIC_MASK, 0xff);
	udelay(3000);
	int status = ANA_REG_GET(ADI_EIC_DATA);
	//printf("charger_connected eica status %x\n", status);
	return !!(status & (1 << 2));
}

int alarm_triggered(void)
{
    printf("ANA_RTC_INT_RSTS is 0x%x\n", ANA_RTC_INT_RSTS);
    printf("value of it 0x%x\n", ANA_REG_GET(ANA_RTC_INT_RSTS));
    return ANA_REG_GET(ANA_RTC_INT_RSTS) & BIT_4;
}

