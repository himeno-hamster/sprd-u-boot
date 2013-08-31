#include "normal_mode.h"
#include <asm/arch/wdt_monitor.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/arch/sci_types.h>
#include <command.h>
#include <common.h>


#define BUFLEN  (128)
#define MAXLEN  (2048)
#define REG32(x)     (*((volatile uint32 *)(x)))

static struct wdt_monitor_struct wdt_monitor_regs[] = {
	WDT_MONITOR_SET(WDT_MON_R13),
	WDT_MONITOR_SET(WDT_MON_R14),
	WDT_MONITOR_SET(WDT_MON_R13_FIQ),
	WDT_MONITOR_SET(WDT_MON_R14_FIQ),
	WDT_MONITOR_SET(WDT_MON_R13_IRQ),
	WDT_MONITOR_SET(WDT_MON_R14_IRQ),
	WDT_MONITOR_SET(WDT_MON_R13_SVC),
	WDT_MONITOR_SET(WDT_MON_R14_SVC),
	WDT_MONITOR_SET(WDT_MON_R13_UND),
	WDT_MONITOR_SET(WDT_MON_R14_UND),
	WDT_MONITOR_SET(WDT_MON_R13_ABT),
	WDT_MONITOR_SET(WDT_MON_R14_ABT),
	WDT_MONITOR_SET(WDT_MON_R13_MON),
	WDT_MONITOR_SET(WDT_MON_R14_MON),
	WDT_MONITOR_SET(WDT_MON_PC),
	WDT_MONITOR_SET(WDT_MON_CPSR),
	WDT_MONITOR_SET(WDT_MON_SPSR_FIQ),
	WDT_MONITOR_SET(WDT_MON_SPSR_IRQ),
	WDT_MONITOR_SET(WDT_MON_SPSR_SVC),
	WDT_MONITOR_SET(WDT_MON_SPSR_UND),
	WDT_MONITOR_SET(WDT_MON_SPSR_ABT),
	WDT_MONITOR_SET(WDT_MON_SPSR_MON),
	WDT_MONITOR_SET(WDT_MON_R0),
	WDT_MONITOR_SET(WDT_MON_R1),
	WDT_MONITOR_SET(WDT_MON_R2),
	WDT_MONITOR_SET(WDT_MON_R3),
	WDT_MONITOR_SET(WDT_MON_R4),
	WDT_MONITOR_SET(WDT_MON_R5),
	WDT_MONITOR_SET(WDT_MON_R6),
	WDT_MONITOR_SET(WDT_MON_R7),
	WDT_MONITOR_SET(WDT_MON_R8),
	WDT_MONITOR_SET(WDT_MON_R9),
	WDT_MONITOR_SET(WDT_MON_R10),
	WDT_MONITOR_SET(WDT_MON_R11),
	WDT_MONITOR_SET(WDT_MON_R12),
	WDT_MONITOR_SET(WDT_MON_R8_FIQ),
	WDT_MONITOR_SET(WDT_MON_R9_FIQ),
	WDT_MONITOR_SET(WDT_MON_R10_FIQ),
	WDT_MONITOR_SET(WDT_MON_R11_FIQ),
	WDT_MONITOR_SET(WDT_MON_R12_FIQ)
};

extern int lcd_display_bitmap(ulong bmp_image, int x, int y);
extern void lcd_display(void);
extern void set_backlight(uint32_t value);

int  dump_wdtmonitor_reg(void)
{
	char *productinfopoint = "/cache";
	char *wdtmonitor_file = "/cache/wdt_monitor.txt";
	char *wdtmonitor_bk = "/cache/wdt_monitor_bk.txt";
	struct wdt_monitor_struct *ptr = wdt_monitor_regs;
	char buf[BUFLEN]="";
	char buffer[MAXLEN] = "";
	int it;
	int ret = -1;
	size_t size = 1<<19;
	char * bmp_img;

	cmd_yaffs_mount(productinfopoint);

	/**
	* check file exits, if so , backup up
	*/
	ret = cmd_yaffs_ls_chk(wdtmonitor_file);
	if( ret > 0)
		cmd_yaffs_mv(wdtmonitor_file, wdtmonitor_bk);

	/**
	* Dump all the monitor registers
	*/
	for(it=0; it < ARR_SIZE(wdt_monitor_regs); it++) {
		memset(buf, 0, BUFLEN);
		sprintf(buf, "%s = 0x%08x\n", ptr->name, REG32(ptr->addr));
                strcat(buffer, buf);
                ptr++;
	}

        cmd_yaffs_mwrite_file(wdtmonitor_file, buffer, strlen(buffer));
        cmd_yaffs_umount(productinfopoint);

        /*display watchdog rest on screen*/
	bmp_img = malloc(size);
	if(!bmp_img){
		printf("not enough memory for splash image\n");
		return -1;
	}

	ret = read_logoimg(bmp_img,size);
	if(ret == -1)
		return -1;

	lcd_display_bitmap((ulong)bmp_img, 0, 0);
        memset(bmp_img, 0, size);
        sprintf(bmp_img, "%s", "   watchdog mode\n\n");
        strcat(bmp_img, buffer);

        /*display*/
        lcd_printf(bmp_img);
	lcd_display();
	set_backlight(255);

        /**/
	printf("wdt reboot dump registers ok in %s, go to die\n", wdtmonitor_file);
	hang();

        /*never reach here*/
	return 0;
}

