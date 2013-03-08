#include <common.h>
#include <asm/io.h>
#include <asm/arch/ldo.h>
#include <asm/arch/sc8810_reg_ahb.h>
#include <asm/arch/regs_ahb.h>
#include <asm/arch/common.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/u-boot.h>
#include <part.h>
#include <sdhci.h>
#include <asm/arch/mfp.h>
DECLARE_GLOBAL_DATA_PTR;

extern void sprd_gpio_init(void);
extern void ADI_init (void);
extern int LDO_Init(void);

#define  GB_SOFT_RST    (0x8B00004c)
#define  BIT_APCP_RST       (0x00000040)
#define mdelay(n)	udelay((n) * 1000)

void modem_poweron(void)
{
	/*Modem Power On*/
	*(volatile unsigned long *)GB_SOFT_RST  &= ~BIT_APCP_RST;
        
}
void modem_poweroff(void)
{
	/*Modem Power Off*/
	*(volatile unsigned long *)GB_SOFT_RST  |= BIT_APCP_RST;

	mdelay(100);
	/*Modem download pin,set to high,Modem can not enter downnload mode*/
	//dumy now        
}

void Init_7702_modem(void)
{
	/*Modem Power Off*/
	*(volatile unsigned long *)GB_SOFT_RST  |= BIT_APCP_RST;

	/*Modem AP_CP Rts pin set to low (default)*/
	gpio_direction_output(AP_CP_RTS, 1);
	gpio_set_value(AP_CP_RTS, 0);

	mdelay(100);
	/*Modem download pin,set to high,Modem can not enter downnload mode*/
	//dummy now

	/*Modem CP_Life status*/
	gpio_direction_output(CP_AP_LIV, 0);

}

void init_calibration_gpio(void)
{
	gpio_direction_output(AP_CP_RTS, 1);
	gpio_set_value(AP_CP_RTS, 0);

	gpio_direction_output(CP_AP_RDY, 0);
	//gpio_set_value(CP_AP_RDY, 0);

	gpio_direction_output(CP_AP_RTS, 0);
	//gpio_set_value(CP_AP_RTS, 0);

	gpio_direction_output(AP_CP_RDY, 1);
	gpio_set_value(AP_CP_RDY, 1);

	gpio_direction_output(CP_AP_LIV, 0);
	//gpio_set_value(CP_AP_LIV, 0);
}

void init_calibration_mode(void)
{
    //uart pin select

    //ap-cp gpio config
	init_calibration_gpio();
}

#define PIN_CTL_REG 0x8C000000
static void chip_init(void)
{
	//ANA_REG_SET(ANA_ADIE_CHIP_ID,0);
	/* setup pins configration when LDO shutdown*/
	//__raw_writel(0x1fff00, PIN_CTL_REG);
	*(volatile unsigned int *)PIN_CTL_REG = 0x1fff00;

	/*sim ldo constrol swith config*/
	/*should be changed with different project defination*/
	ANA_REG_OR(ANA_LDO_SWITCH,0x0f);/*switch sim0,1,2,wpadcdc to cp side*/

	/*adie headset detection config*/

	//ANA_REG_OR(0x82000830,0x02);//7710 usb ldo on
	/*cp jtag func and pin config*/
	//CHIP_REG_AND(0x8B0000B0, ~0x780000);//pin eb
	//CHIP_REG_SET(0x8C00043c, 0x00158);
	//CHIP_REG_SET(0x8C000440, 0x00198);
	//CHIP_REG_SET(0x8C000444, 0x00118);
	//CHIP_REG_SET(0x8C000448, 0x00198);
	//CHIP_REG_SET(0x8C00044c, 0x00198);

}

int board_init()
{
	gd->bd->bi_arch_number = MACH_TYPE_OPENPHONE;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	ADI_init();
	chip_init();
	LDO_Init();
	sprd_gpio_init();
	Init_7702_modem();
	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((volatile void *)PHYS_SDRAM_1,
			PHYS_SDRAM_1_SIZE);
	return 0;
}

