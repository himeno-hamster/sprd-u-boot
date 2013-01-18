
#include <asm/arch/sc8810_reg_ahb.h>
#include <asm/arch/gpio.h>
struct eic_gpio_resource sprd_gpio_resource[] = {
       [ENUM_ID_D_GPIO] = {
               .base_addr      = CTL_GPIO_BASE,
               .chip_base      = D_GPIO_START,
               .chip_ngpio     = D_GPIO_NR,
       },
       [ENUM_ID_D_EIC] = {
               .base_addr      = CTL_EIC_BASE,
               .chip_base      = D_EIC_START,
               .chip_ngpio     = D_EIC_NR,
       },
       [ENUM_ID_A_GPIO] = {
               .base_addr      = ANA_CTL_GPIO_BASE,
               .chip_base      = A_GPIO_START,
               .chip_ngpio     = A_GPIO_NR,
       },
       [ENUM_ID_A_EIC] = {
               .base_addr      = ANA_CTL_EIC_BASE,
               .chip_base      = A_EIC_START,
               .chip_ngpio     = A_EIC_NR,
       },
};
