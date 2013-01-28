#include <common.h>
#include <asm/io.h>
#include <asm/arch/sc8810_reg_global.h>
#include <ubi_uboot.h>
#include <asm/arch/pinmap.h>

pinmap_t pinmap[] = {
#ifdef CONFIG_GENERIC_MMC
	{REG_PIN_SD0_CLK,             BITS_PIN_DS(3)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD0_CMD,             BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD0_D0,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD0_D1,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD0_D2,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD0_D3,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD1_CLK,             BITS_PIN_DS(2)|BITS_PIN_AF(0)|BIT_PIN_NUL|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD1_CMD,             BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD1_D0,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD1_D1,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD1_D2,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
	{REG_PIN_SD1_D3,              BITS_PIN_DS(1)|BITS_PIN_AF(0)|BIT_PIN_WPU|BIT_PIN_SLP_NUL|BIT_PIN_SLP_Z},
#endif
};
int pin_init(void)
{
	int i;
	printf("pinmap size %d\n", ARRAY_SIZE(pinmap));
	for (i = 0; i < ARRAY_SIZE(pinmap); i++) {
		__raw_writel(pinmap[i].val, CTL_PIN_BASE + pinmap[i].reg);
	}
	return 0;
}
