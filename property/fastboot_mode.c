#include <config.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <boot_mode.h>
#include <common.h>
#include <linux/string.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>

extern int dwc_otg_driver_init(void);
extern int usb_fastboot_initialize(void);

int read_logoimg(char *bmp_img,size_t size)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;
	loff_t off = 0;

	ret = mtdparts_init();
	if (ret != 0){
		printf("mtdparts init error %d\n", ret);
		return -1;
	}
#define SPLASH_PART "fastboot_logo"

	ret = find_dev_and_part(SPLASH_PART, &dev, &pnum, &part);
	if(ret){
		printf("No partition named %s\n", SPLASH_PART);
		return -1;
	}else if(dev->id->type != MTD_DEV_TYPE_NAND){
		printf("Partition %s not a NAND device\n", SPLASH_PART);
		return -1;
	}

	off=part->offset;
	nand = &nand_info[dev->id->num];

	ret = nand_read_offset_ret(nand, off, &size, (void *)bmp_img, &off);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
		return -1;
	}
	return 0;
}

extern int lcd_display_bitmap(ulong bmp_image, int x, int y);
extern lcd_display(void);
extern void set_backlight(uint32_t value);

void fastboot_mode(void)
{
    printf("%s\n", __FUNCTION__);
#ifdef CONFIG_SPLASH_SCREEN
    const char *cmdline;
    size_t size;

    //read boot image header
    size = 1<<20;
    char * bmp_img = malloc(size);
    if(!bmp_img){
        printf("not enough memory for splash image\n");
        return;
    }

    read_logoimg(bmp_img, size);

    lcd_display_bitmap((ulong)bmp_img, 0, 0);
    lcd_printf("   fastboot mode");
    lcd_display();
    set_backlight(255);
    //char img_addr[9];
    //sprintf(img_addr, "%x\n", bmp_img);
    //setenv("splashimage", img_addr);

#endif

#ifdef CONFIG_CMD_FASTBOOT
	dwc_otg_driver_init();
	usb_fastboot_initialize();
#endif

}
