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

#ifndef __ASM_ARM_ARCH_GPIO_H
#define __ASM_ARM_ARCH_GPIO_H

/*
 * SC8825 GPIO&EIC bank and number summary:
 *
 * Bank	  From	  To	NR	Type
 * 1	  0   ~	  15		16	EIC
 * 2	  16  ~	  271	256	GPIO
 * 3	  272 ~	  287	16	ANA EIC
 * 4	  288 ~	  319	32	ANA GPIO
 */

#define	D_EIC_START		0
#define	D_EIC_NR		16

#define	D_GPIO_START	( D_EIC_START + D_EIC_NR )
#define	D_GPIO_NR		256

#define	A_EIC_START		( D_GPIO_START + D_GPIO_NR )
#define	A_EIC_NR		16

#define	A_GPIO_START	( A_EIC_START + A_EIC_NR )
#define	A_GPIO_NR		32

#define ARCH_NR_GPIOS	( D_EIC_NR + D_GPIO_NR + A_EIC_NR + A_GPIO_NR )

#define EIC_CHARGER_DETECT		(A_EIC_START + 2)
#define EIC_KEY_POWER		(A_EIC_START + 3)

#include <asm/arch/sc8810_reg_base.h>
#include <asm-generic/gpio.h>

#define gpio_get_value  __gpio_get_value
#define gpio_set_value  __gpio_set_value
#define gpio_cansleep   __gpio_cansleep
#define gpio_to_irq     __gpio_to_irq


/* Digital GPIO/EIC base address */
#define CTL_GPIO_BASE          (GPIO_BASE)
#define CTL_EIC_BASE           (EIC_BASE)

/* Analog GPIO/EIC base address */
#define ANA_CTL_GPIO_BASE      (ADI_BASE + 0x0480)
#define ANA_CTL_EIC_BASE       (ADI_BASE + 0x0100)


enum {
       ENUM_ID_D_GPIO = 0,
       ENUM_ID_D_EIC,
       ENUM_ID_A_GPIO,
       ENUM_ID_A_EIC,

       ENUM_ID_END_NR,
};

struct eic_gpio_resource {
       int base_addr;
       int chip_base;
       int chip_ngpio;
};

static inline int irq_to_gpio(int irq)
{
	return 0;
}

#endif
