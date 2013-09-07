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


extern int cmd_yaffs_ls_chk(const char *dirfilename);
extern int ensure_path_mounted(const char * mountpoint);
extern int ensure_path_umounted(const char * mountpoint);
int alarm_file_check(char *time_buf)
{
    char *file_partition = "/productinfo";
    char *file_name = "/productinfo/alarm_flag";
    int ret = 0;
    ensure_path_mounted(file_partition);
    ret = cmd_yaffs_ls_chk(file_name);
    if(ret >=0){
        printf("file: %s exist\n", file_name);
        cmd_yaffs_mread_file(file_name, (unsigned char *)time_buf);

    }
    // ensure_path_umounted(file_partition);
    return ret;

}
int poweron_file_check(char *time_buf)
{
    char *file_partition = "/productinfo";
    char *file_name = "/productinfo/poweron_timeinmillis";
    int ret = 0;
    ensure_path_mounted(file_partition);
    ret = cmd_yaffs_ls_chk(file_name);
    if(ret >=0){
        printf("file: %s exist\n", file_name);
        cmd_yaffs_mread_file(file_name, (unsigned char *)time_buf);
    }
    // ensure_path_umounted(file_partition);
    return ret;

}
