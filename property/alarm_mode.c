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

extern void cmd_yaffs_mount(char *mp);
extern void cmd_yaffs_umount(char *mp);
extern int cmd_yaffs_ls_chk(const char *dirfilename);
extern unsigned int get_alarm_lead_set(void);
#define msleep(a) udelay(a * 1000)
void alarm_mode(void)
{
    printf("%s\n", __func__);

#if BOOT_NATIVE_LINUX
    vlx_nand_boot(BOOT_PART, CONFIG_BOOTARGS, BACKLIGHT_ON);
#else
    vlx_nand_boot(BOOT_PART, "androidboot.mode=alarm", BACKLIGHT_OFF);
#endif
}

unsigned long sprd_rtc_get_alarm_sec(void);
unsigned long sprd_rtc_get_sec(void);
void sprd_rtc_set_alarm_sec(unsigned long secs);
void sprd_rtc_init(void);
char time_buf[200]={0};
int alarm_flag_check(void)
{
    char *file_partition = "/productinfo";
    char *file_name = "/productinfo/alarm_flag";
    int ret = 0;
    long time = 0;
    unsigned long time_rtc= 0;
    unsigned long read_secs;
    unsigned long now_rtc = 0;
    int time_lead = 0;
    int i = 0;
    cmd_yaffs_mount(file_partition);
    ret = cmd_yaffs_ls_chk(file_name);
    if (ret == -1) {
        printf("file: %s not found\n", file_name);
        ret = 0;
    } else {
        printf("file: %s exist\n", file_name);
        cmd_yaffs_mread_file(file_name, (unsigned char *)time_buf);
        printf("time get %s", time_buf);
        time = simple_strtol(time_buf, NULL, 10);
        time_rtc = time;
        sprd_rtc_init();
        now_rtc = sprd_rtc_get_sec();
        printf("now rtc %lu\n", now_rtc);
        time = time - now_rtc;
        time_lead = get_alarm_lead_set();
        if((time < time_lead +180) && (time > time_lead -10))
          ret = 1;
        else{
          sprd_rtc_set_alarm_sec(time_rtc);
          msleep(150);
          do {
                     if(i!=0){
                       sprd_rtc_set_alarm_sec(time_rtc);
		         msleep(150);
                        }
			read_secs = sprd_rtc_get_alarm_sec();

			msleep(1);
			i++;
		}while(read_secs != time_rtc && i < 100);
          ret = 0;
         }
    }
    cmd_yaffs_umount(file_partition);
    return ret;
}
