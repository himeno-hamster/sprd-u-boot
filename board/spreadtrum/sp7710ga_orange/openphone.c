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
#ifdef CONFIG_GENERIC_MMC
static unsigned long sdio_func_cfg[] = {
	MFP_CFG_X(SD0_CLK0, AF0, DS3, F_PULL_NONE, S_PULL_NONE, IO_Z),
	MFP_CFG_X(SD0_CLK1, AF0, DS3, F_PULL_NONE, S_PULL_NONE, IO_Z),
	MFP_CFG_X(SD0_CMD, AF0, DS0, F_PULL_UP,  S_PULL_NONE, IO_Z),
	MFP_CFG_X(SD0_D0, AF0, DS0, F_PULL_UP, S_PULL_NONE, IO_Z),
	MFP_CFG_X(SD0_D1, AF0, DS0, F_PULL_DOWN, S_PULL_NONE, IO_Z),
	MFP_CFG_X(SD0_D2, AF0, DS0, F_PULL_DOWN, S_PULL_NONE, IO_Z),
	MFP_CFG_X(SD0_D3, AF0, DS0, F_PULL_DOWN, S_PULL_NONE, IO_Z),
};

static unsigned long sdcard_detect_gpio_cfg =
MFP_CFG_X(CP_RFCTL11, AF3, DS1, F_PULL_UP,S_PULL_NONE, IO_Z);

void sprd_config_sdio_pins(void)
{
	sprd_mfp_config(sdio_func_cfg, ARRAY_SIZE(sdio_func_cfg));
	sprd_mfp_config(&sdcard_detect_gpio_cfg, 1);
}
int mv_sdh_init(u32 regbase, u32 max_clk, u32 min_clk, u32 quirks);
int board_mmc_init(bd_t *bd)
{
	ulong mmc_base_address[CONFIG_SYS_MMC_NUM] = CONFIG_SYS_MMC_BASE;
	u8 i, data;

	REG32(AHB_CTL0)     |= BIT_4;
	REG32(AHB_SOFT_RST) |= BIT_12;
	REG32(AHB_SOFT_RST) &= ~BIT_12;
	LDO_SetVoltLevel(LDO_LDO_SDIO0, LDO_VOLT_LEVEL1);
	LDO_TurnOnLDO(LDO_LDO_SDIO0);
	sprd_config_sdio_pins();

	for (i = 0; i < CONFIG_SYS_MMC_NUM; i++) {
		if (mv_sdh_init(mmc_base_address[i], SDIO_BASE_CLK_96M, 
			SDIO_CLK_250K, 0))
			return 1;
	}

	return 0;
}
#endif

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

