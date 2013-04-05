#include <common.h>
#include <asm/io.h>
#include <asm/arch/sprd_reg.h>
#include <asm/arch/gpio_reg_v0.h>
#include <asm/arch/gpio_phy.h>

const static struct gpio_section  s_gpio_section_table[] = {
    {   (SPRD_GPIO_PHYS + 0*0x80),    0x8,     GPIO_SECTION_GPI   },
    {   (SPRD_GPIO_PHYS + 1*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 2*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 3*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 4*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 5*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 6*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 7*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 8*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_GPIO_PHYS + 9*0x80),    0x10,    GPIO_SECTION_GPIO  },
    {   (SPRD_ANA_GPIO_PHYS + 0*0x80),   0x8,  GPIO_SECTION_GPI   },
    {   (SPRD_ANA_GPIO_PHYS + 1*0x80),   0x10, GPIO_SECTION_GPIO  },
    {   (SPRD_ANA_GPIO_PHYS + 2*0x80),   0x10, GPIO_SECTION_GPIO  },
};

struct gpio_section * gpio_get_section_table (u32 *table_size)
{
    *table_size = ARRAY_SIZE(s_gpio_section_table);

    return (struct gpio_section *) s_gpio_section_table;
}

