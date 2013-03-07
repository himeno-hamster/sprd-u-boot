#include <common.h>
#include <asm/io.h>
DECLARE_GLOBAL_DATA_PTR;

#define		PIN_REG_SIMDA3		(0x8C000470)
#define		PIN_REG_SIMRST3		(0x8C000478)
#define		PIN_CTRL_REG     	(0x8b000028)

extern void sprd_gpio_init(void);
extern void ADI_init (void);
extern int LDO_Init(void);

#include <asm/arch/regs_ana.h>
#include <asm/arch/adi_hal_internal.h>

#define mdelay(n)	udelay((n) * 1000)

void modem_poweron(void)
{
        /*Modem Power On*/

        __raw_writel(0x31,0x8C0003b8);
        gpio_direction_output(106,1);
        gpio_set_value(106,1);
}
void modem_poweroff(void)
{
        /*Modem Power Off*/
        __raw_writel(0x31,0x8C0003b8);
        gpio_direction_output(106,1);
        gpio_set_value(106,0);
        mdelay(100);
        /*Modem download pin,set to high,Modem can not enter downnload mode*/
        __raw_writel(0x31,0x8C000114);
        gpio_direction_output(34,1);
        gpio_set_value(34,1);
}

void Init_7702_modem(void)
{
	/*Modem Power Off*/
	__raw_writel(0x31,0x8C0003b8);
	gpio_direction_output(106,1);
	gpio_set_value(106,0);
	/*Modem AP_CP Rst pin set to low (default)*/
	__raw_writel(0x31,0x8C000124);
	gpio_direction_output(38,1);
	gpio_set_value(38,0);

	mdelay(100);
	/*Modem download pin,set to high,Modem can not enter downnload mode*/
	__raw_writel(0x31,0x8C000114);
	gpio_direction_output(34,1);
	gpio_set_value(34,1);
	/*Modem Power On*/
	__raw_writel(0x31,0x8C0003b8);
	gpio_direction_output(106,1);
	gpio_set_value(106,1);
	/*Wait Modem Power On 5s*/
	mdelay(5000);
}

void init_calibration_gpio(void)
{
	__raw_writel(0x134, 0x8C000430);
	gpio_direction_output(AP_CP_RTS, 1);
	gpio_set_value(AP_CP_RTS, 0);

        __raw_writel(0x1B4, 0x8C000424);
        gpio_direction_output(CP_AP_RDY, 0);
        //gpio_set_value(CP_AP_RDY, 0);

        __raw_writel(0x1B0, 0x8C00039c);
        gpio_direction_output(CP_AP_RTS, 0);
        //gpio_set_value(CP_AP_RTS, 0);

        __raw_writel(0x134, 0x8C000304);
        gpio_direction_output(AP_CP_RDY, 1);
        gpio_set_value(AP_CP_RDY, 1);

        __raw_writel(0x174, 0x8C000398);
        gpio_direction_output(CP_AP_LIV, 0);
        //gpio_set_value(CP_AP_LIV, 0);
}

void init_calibration_mode(void)
{
        *(volatile unsigned long *)PIN_REG_SIMDA3  |= 0x000003a0;
	*(volatile unsigned long *)PIN_REG_SIMRST3 |= 0x000003a0;
	*(volatile unsigned long *)PIN_CTRL_REG |= 0x00000040;
	
	init_calibration_gpio();
}

#define PIN_CTL_REG 0x8C000000
static void chip_init(void)
{
    //ANA_REG_SET(ANA_ADIE_CHIP_ID,0);
    /* setup pins configration when LDO shutdown*/
    //__raw_writel(0x1fff00, PIN_CTL_REG);
     *(volatile unsigned int *)PIN_CTL_REG = 0x1fff00;
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

    //board_gpio_init();
	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((volatile void *)PHYS_SDRAM_1,
			PHYS_SDRAM_1_SIZE);
	return 0;
}
