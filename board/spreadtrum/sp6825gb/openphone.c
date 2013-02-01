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
#include <linux/gpio.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmap.h>
DECLARE_GLOBAL_DATA_PTR;

extern void sprd_gpio_init(struct eic_gpio_resource *r);
extern void ADI_init (void);
extern int LDO_Init(void);

#ifdef CONFIG_GENERIC_MMC
int mv_sdh_init(u32 regbase, u32 max_clk, u32 min_clk, u32 quirks);
int board_mmc_init(bd_t *bd)
{
	ulong mmc_base_address[CONFIG_SYS_MMC_NUM] = CONFIG_SYS_MMC_BASE;
	u8 i, data;

	REG32(AHB_CTL0)     |= BIT_4;
	REG32(AHB_SOFT_RST) |= BIT_12;
	REG32(AHB_SOFT_RST) &= ~BIT_12;
	LDO_SetVoltLevel(LDO_LDO_SDIO1, LDO_VOLT_LEVEL1);
	LDO_TurnOnLDO(LDO_LDO_SDIO1);

	for (i = 0; i < CONFIG_SYS_MMC_NUM; i++) {
		if (mv_sdh_init(mmc_base_address[i], SDIO_BASE_CLK_96M, 
			SDIO_CLK_250K, 0))
			return 1;
	}

	return 0;
}
#endif

extern struct eic_gpio_resource sprd_gpio_resource[];

int board_init()
{
	gd->bd->bi_arch_number = MACH_TYPE_OPENPHONE;
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;
#if 0
    ADI_init();
    chip_init();
    LDO_Init();
    sprd_gpio_init();
    //board_gpio_init();
#endif	
	pin_init();
	sprd_gpio_init(sprd_gpio_resource);
	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((volatile void *)PHYS_SDRAM_1,
			PHYS_SDRAM_1_SIZE);
	return 0;
}
