/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>

#include <asm/arch-sc8825/ldo.h>

#include <asm/arch/sc8810_reg_global.h>

#include <asm/arch/regs_global.h>
#include <asm/arch/regs_cpc.h>
#include <linux/gpio.h>
#include <asm/arch/pinmap.h>
#include <asm/arch/gpio.h>
#include <ubi_uboot.h>

/* 16 GPIO share a group of registers */
#define	GPIO_GROUP_NR		(16)
#define GPIO_GROUP_MASK		(0xFFFF)

#define	GPIO_GROUP_OFFSET	(0x80)
#define	ANA_GPIO_GROUP_OFFSET	(0x40)

/* registers definitions for GPIO controller */
#define REG_GPIO_DATA		(0x0000)
#define REG_GPIO_DMSK		(0x0004)
#define REG_GPIO_DIR		(0x0008)	/* only for gpio */
#define REG_GPIO_IS		(0x000c)	/* only for gpio */
#define REG_GPIO_IBE		(0x0010)	/* only for gpio */
#define REG_GPIO_IEV		(0x0014)
#define REG_GPIO_IE		(0x0018)
#define REG_GPIO_RIS		(0x001c)
#define REG_GPIO_MIS		(0x0020)
#define REG_GPIO_IC		(0x0024)
#define REG_GPIO_INEN		(0x0028)	/* only for gpio */

/* 8 EIC share a group of registers */
#define	EIC_GROUP_NR		(8)
#define EIC_GROUP_MASK		(0xFF)

/* registers definitions for EIC controller */
#define REG_EIC_DATA		REG_GPIO_DATA
#define REG_EIC_DMSK		REG_GPIO_DMSK
#define REG_EIC_IEV		REG_GPIO_IEV
#define REG_EIC_IE		REG_GPIO_IE
#define REG_EIC_RIS		REG_GPIO_RIS
#define REG_EIC_MIS		REG_GPIO_MIS
#define REG_EIC_IC		REG_GPIO_IC
#define REG_EIC_TRIG		(0x0028)	/* only for eic */
#define REG_EIC_0CTRL		(0x0040)
#define REG_EIC_1CTRL		(0x0044)
#define REG_EIC_2CTRL		(0x0048)
#define REG_EIC_3CTRL		(0x004c)
#define REG_EIC_4CTRL		(0x0050)
#define REG_EIC_5CTRL		(0x0054)
#define REG_EIC_6CTRL		(0x0058)
#define REG_EIC_7CTRL		(0x005c)
#define REG_EIC_DUMMYCTRL	(0x0000)

/* bits definitions for register REG_EIC_DUMMYCTRL */
#define BIT_FORCE_CLK_DBNC	BIT(15)
#define BIT_EIC_DBNC_EN		BIT(14)
#define SHIFT_EIC_DBNC_CNT	(0)
#define MASK_EIC_DBNC_CNT	(0xFFF)
#define BITS_EIC_DBNC_CNT(_x_)	((_x) & 0xFFF)

struct sci_gpio_chip {
	struct gpio_chip chip;

	uint32_t base_addr;
	uint32_t group_offset;

	 uint32_t(*read_reg) (uint32_t addr);
	void (*write_reg) (uint32_t value, uint32_t addr);
	void (*set_bits) (uint32_t bits, uint32_t addr);
	void (*clr_bits) (uint32_t bits, uint32_t addr);
};

#define	to_sci_gpio(c)		container_of(c, struct sci_gpio_chip, chip)

/* D-Die regs ops */
static uint32_t d_read_reg(uint32_t addr)
{
	return __raw_readl(addr);
}

static void d_write_reg(uint32_t value, uint32_t addr)
{
	__raw_writel(value, addr);
}

static void d_set_bits(uint32_t bits, uint32_t addr)
{
	__raw_writel(__raw_readl(addr) | bits, addr);
}

static void d_clr_bits(uint32_t bits, uint32_t addr)
{
	__raw_writel(__raw_readl(addr) & ~bits, addr);
}

/* A-Die regs ops */
static uint32_t a_read_reg(uint32_t addr)
{
	return sci_adi_read(addr);
}

static void a_write_reg(uint32_t value, uint32_t addr)
{
	sci_adi_raw_write(addr, value);
}

static void a_set_bits(uint32_t bits, uint32_t addr)
{
	sci_adi_set(addr, bits);
}

static void a_clr_bits(uint32_t bits, uint32_t addr)
{
	sci_adi_clr(addr, bits);
}

static int sci_gpio_read(struct gpio_chip *chip, uint32_t offset, uint32_t reg)
{
	struct sci_gpio_chip *sci_gpio = to_sci_gpio(chip);
	int group = offset / GPIO_GROUP_NR;
	int bitof = offset & (GPIO_GROUP_NR - 1);
	int addr = sci_gpio->base_addr + sci_gpio->group_offset * group + reg;
	int value = sci_gpio->read_reg(addr) & GPIO_GROUP_MASK;

	return (value >> bitof) & 0x1;
}

static void sci_gpio_write(struct gpio_chip *chip, uint32_t offset,
			   uint32_t reg, int value)
{
	struct sci_gpio_chip *sci_gpio = to_sci_gpio(chip);
	int group = offset / GPIO_GROUP_NR;
	int bitof = offset & (GPIO_GROUP_NR - 1);
	int addr = sci_gpio->base_addr + sci_gpio->group_offset * group + reg;

	if (value) {
		sci_gpio->set_bits(1 << bitof, addr);
	} else {
		sci_gpio->clr_bits(1 << bitof, addr);
	}
}

/* GPIO/EIC libgpio interfaces */
static int sci_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	pr_debug("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	sci_gpio_write(chip, offset, REG_GPIO_DMSK, 1);
	return 0;
}

static void sci_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	pr_debug("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	sci_gpio_write(chip, offset, REG_GPIO_DMSK, 0);
}

static int sci_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	pr_debug("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	sci_gpio_write(chip, offset, REG_GPIO_DIR, 0);
	sci_gpio_write(chip, offset, REG_GPIO_INEN, 1);
	return 0;
}

static int sci_eic_direction_input(struct gpio_chip *chip, unsigned offset)
{
	/* do nothing */
	pr_debug("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	return 0;
}

static int sci_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
				     int value)
{
	pr_debug("%s %d+%d %d\n", __FUNCTION__, chip->base, offset, value);
	sci_gpio_write(chip, offset, REG_GPIO_DIR, 1);
	sci_gpio_write(chip, offset, REG_GPIO_INEN, 0);
	sci_gpio_write(chip, offset, REG_GPIO_DATA, value);
	return 0;
}

static int sci_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	pr_debug("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	return sci_gpio_read(chip, offset, REG_GPIO_DATA);
}

static void sci_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	pr_debug("%s %d+%d %d\n", __FUNCTION__, chip->base, offset, value);
	sci_gpio_write(chip, offset, REG_GPIO_DATA, value);
}

static int sci_gpio_set_debounce(struct gpio_chip *chip, unsigned offset,
				 unsigned debounce)
{
	/* not supported */
	pr_err("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	return -EINVAL;
}

static int sci_eic_set_debounce(struct gpio_chip *chip, unsigned offset,
				unsigned debounce)
{
	/* TODO */
	pr_info("%s %d+%d\n", __FUNCTION__, chip->base, offset);
	return 0;
}

static int sci_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static int sci_irq_to_gpio(struct gpio_chip *chip, unsigned irq)
{
	return 0;
}

static struct sci_gpio_chip d_sci_gpio = {
	.chip.label = "sprd-d-gpio",
	.chip.request = sci_gpio_request,
	.chip.free = sci_gpio_free,
	.chip.direction_input = sci_gpio_direction_input,
	.chip.get = sci_gpio_get,
	.chip.direction_output = sci_gpio_direction_output,
	.chip.set = sci_gpio_set,
	.chip.set_debounce = sci_gpio_set_debounce,
	.chip.to_irq = sci_gpio_to_irq,

	.group_offset = GPIO_GROUP_OFFSET,
	.read_reg = d_read_reg,
	.write_reg = d_write_reg,
	.set_bits = d_set_bits,
	.clr_bits = d_clr_bits,
};

static struct sci_gpio_chip a_sci_gpio = {
	.chip.label = "sprd-a-gpio",
	.chip.request = sci_gpio_request,
	.chip.free = sci_gpio_free,
	.chip.direction_input = sci_gpio_direction_input,
	.chip.get = sci_gpio_get,
	.chip.direction_output = sci_gpio_direction_output,
	.chip.set = sci_gpio_set,
	.chip.set_debounce = sci_gpio_set_debounce,
	.chip.to_irq = sci_gpio_to_irq,

	.group_offset = ANA_GPIO_GROUP_OFFSET,
	.read_reg = a_read_reg,
	.write_reg = a_write_reg,
	.set_bits = a_set_bits,
	.clr_bits = a_clr_bits,
};

/*
 * EIC has the same register layout with GPIO when it's used as GPI.
 * So most implementation of GPIO can be shared by EIC.
 */
static struct sci_gpio_chip d_sci_eic = {
	.chip.label = "sprd-d-eic",
	.chip.request = sci_gpio_request,
	.chip.free = sci_gpio_free,
	.chip.direction_input = sci_eic_direction_input,
	.chip.get = sci_gpio_get,
	.chip.direction_output = NULL,
	.chip.set = NULL,
	.chip.set_debounce = sci_eic_set_debounce,
	.chip.to_irq = sci_gpio_to_irq,

	.group_offset = GPIO_GROUP_OFFSET,
	.read_reg = d_read_reg,
	.write_reg = d_write_reg,
	.set_bits = d_set_bits,
	.clr_bits = d_clr_bits,
};

static struct sci_gpio_chip a_sci_eic = {
	.chip.label = "sprd-a-eic",
	.chip.request = sci_gpio_request,
	.chip.free = sci_gpio_free,
	.chip.direction_input = sci_eic_direction_input,
	.chip.get = sci_gpio_get,
	.chip.direction_output = NULL,
	.chip.set = NULL,
	.chip.set_debounce = sci_eic_set_debounce,
	.chip.to_irq = sci_gpio_to_irq,

	.group_offset = ANA_GPIO_GROUP_OFFSET,
	.read_reg = a_read_reg,
	.write_reg = a_write_reg,
	.set_bits = a_set_bits,
	.clr_bits = a_clr_bits,
};


void sprd_gpio_init(struct eic_gpio_resource *r)
{
	if (!r)
		BUG();


	/* enable EIC */
	sci_glb_set(GR_GEN0, GEN0_EIC_EN);
	sci_glb_set(GR_GEN0, GEN0_GPIO_EN);
	sci_glb_set(GR_GEN0, GEN0_GPIO_RTC_EN);
	sci_adi_set(ANA_APB_CLK_EN,
		    EIC_EB | GPIO_EB | RTC_EIC_EB);


	d_sci_gpio.base_addr  = r[ENUM_ID_D_GPIO].base_addr;
	d_sci_gpio.chip.base  = r[ENUM_ID_D_GPIO].chip_base;
	d_sci_gpio.chip.ngpio = r[ENUM_ID_D_GPIO].chip_ngpio;

	d_sci_eic.base_addr  = r[ENUM_ID_D_EIC].base_addr;
	d_sci_eic.chip.base  = r[ENUM_ID_D_EIC].chip_base;
	d_sci_eic.chip.ngpio = r[ENUM_ID_D_EIC].chip_ngpio;

	a_sci_gpio.base_addr  = r[ENUM_ID_A_GPIO].base_addr;
	a_sci_gpio.chip.base  = r[ENUM_ID_A_GPIO].chip_base;
	a_sci_gpio.chip.ngpio = r[ENUM_ID_A_GPIO].chip_ngpio;

	a_sci_eic.base_addr  = r[ENUM_ID_A_EIC].base_addr;
	a_sci_eic.chip.base  = r[ENUM_ID_A_EIC].chip_base;
	a_sci_eic.chip.ngpio = r[ENUM_ID_A_EIC].chip_ngpio;

	gpiochip_add(&d_sci_eic.chip);
	gpiochip_add(&d_sci_gpio.chip);
	gpiochip_add(&a_sci_eic.chip);
	gpiochip_add(&a_sci_gpio.chip);


	return 0;
}

static int eic_gpio_remove(void)
{
	int ret = 0;

	ret += gpiochip_remove(&d_sci_eic.chip);
	ret += gpiochip_remove(&d_sci_gpio.chip);
	ret += gpiochip_remove(&a_sci_eic.chip);
	ret += gpiochip_remove(&a_sci_gpio.chip);

	return ret;
}

