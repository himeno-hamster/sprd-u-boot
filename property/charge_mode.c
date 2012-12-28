#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>
#include <boot_mode.h>

#if defined(CONFIG_SP7702) || defined(CONFIG_SP8810W)
extern 	void modem_poweroff(void);
#endif

void charge_mode(void)
{
    printf("%s\n", __func__);

#if defined(CONFIG_SP7702) || defined(CONFIG_SP8810W)
	modem_poweroff();
#endif


#if BOOT_NATIVE_LINUX
    vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS " androidboot.mode=charger", BACKLIGHT_ON);
#else
    vlx_nand_boot(BOOT_PART, "androidboot.mode=charger", BACKLIGHT_ON);
#endif
}

