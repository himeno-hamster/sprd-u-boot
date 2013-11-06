#include <config.h>
#include <common.h>
#include <command.h>
#include <linux/types.h>
#include <linux/keypad.h>
#include <linux/key_code.h>
#include <boot_mode.h>
#include <android_bootimg.h>
#include <asm/arch/gpio.h>

#define COMMAND_MAX 128

#define DEBUG
#ifdef DEBUG
#define DBG(fmt...) printf(fmt)
#else
#define DBG(fmt...) 
#endif

extern int power_button_pressed(void);
extern int charger_connected(void);
extern int alarm_triggered(void);
extern int cali_file_check(void);
unsigned check_reboot_mode(void);

int boot_pwr_check(void)
{
    static int total_cnt = 0;
    if(!power_button_pressed())
      total_cnt ++;
    return total_cnt;
}
#define mdelay(_ms) udelay(_ms*1000)


int do_cboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    uint32_t key_mode = 0;
    uint32_t key_code = 0;
    volatile int i;
    unsigned rst_mode = check_reboot_mode();

    if(argc > 2)
        goto usage;
    
#ifdef CONFIG_AUTOBOOT
        normal_mode();
#endif

// 0 get mode from calibration detect
    if(cali_file_check() && !boot_pwr_check()&&
    (!rst_mode || (rst_mode == CALIBRATION_MODE))&&
    (!alarm_triggered())){
        calibration_detect(2);
    }
#ifndef CONFIG_MACH_CORI
    if(is_bat_low()){
        DBG("cboot:low battery and shutdown\n");
        mdelay(10000);
        power_down_devices();
        while(1);
    }
#endif    
#ifdef CONFIG_SPRD_SYSDUMP
    write_sysdump_before_boot(rst_mode);
#endif

// 1 get mode from file
    boot_pwr_check();
    switch(get_mode_from_file()){
        case RECOVERY_MODE:
            DBG("cboot:get mode from file:recovery\n");
            recovery_mode();
        break;
        default: // unknown mode
        break;
    }

// 2 get mode from watch dog
    boot_pwr_check();
    DBG("cboot:get mode from watchdog 0x%x\n",rst_mode);
    switch(rst_mode){
        case RECOVERY_MODE:
            recovery_mode();
        break;
        case FASTBOOT_MODE:
            fastboot_mode();
        break;
        case NORMAL_MODE:
            normal_mode();
        break;
        case WATCHDOG_REBOOT:
            watchdog_mode();
        break;
        case UNKNOW_REBOOT_MODE:
            unknow_reboot_mode();
        break;
        case PANIC_REBOOT:
            panic_reboot_mode();
        break;
        case AUTODLOADER_REBOOT:
            autodloader_mode();
        break;
        case SPECIAL_MODE:
            special_mode();
        break;
        case ALARM_MODE:
        default:
        break;
    }
    // 3 get mode from alarm register
    boot_pwr_check();
    if(alarm_triggered() && alarm_flag_check()){
        int flag =alarm_flag_check();
        if(flag == 1){
            DBG("cboot:get mode from alarm:alarm_mode\n");
            alarm_mode();
        }
        else if(flag == 2){
            DBG("cboot:get mode from alarm:normal_mode\n");
            normal_mode();
        }
    }
#ifdef CONFIG_SP8830SSW
   else if (1) {
		;
   }
#endif
    // 4 get mode from charger
    else if(charger_connected()){
        DBG("cboot:get mode from charger\n");
        charge_mode();
    }
    // 5 get mode from keypad
    else if(boot_pwr_check() >= get_pwr_key_cnt()){
        mdelay(50);
        for(i=0; i<10;i++){
            key_code = board_key_scan();
            if(key_code != KEY_RESERVED)
                break;
        }
        key_mode = check_key_boot(key_code);
        DBG("cboot:get mode from keypad:0x%x\n",key_code);
        switch(key_mode){
            case BOOT_FASTBOOT:
                fastboot_mode();
            break;
            case BOOT_RECOVERY:
                recovery_mode();
            break;
            case BOOT_CALIBRATE:
                engtest_mode();
                return 0;
            break;
            default:
            break;
        }
    }
// 6 get mode from argument
    else if(2 == argc){
        DBG("cboot:get mode from argument:%s\n",argv[1]);
        if(!strcmp(argv[1],"normal")){
            normal_mode();
        }
        if(!strcmp(argv[1],"recovery")){
            recovery_mode();
        }
        if(!strcmp(argv[1],"fastboot")){
            fastboot_mode();
        }
        if(!strcmp(argv[1],"charge")){
            charge_mode();
        }
    }
    else{
        DBG("cboot:get mode fail , and shutdown device\n");
        power_down_devices();
        while(1);
    }
    // 7 unrecognize mode , system enter normal start
    DBG("cboot:get mode fail, and start with normal mode\n");
    normal_mode();
usage:
    cmd_usage(cmdtp);
    return 1;
}

U_BOOT_CMD(
            cboot, CONFIG_SYS_MAXARGS, 1, do_cboot,
            "choose boot mode",
            "mode: \nrecovery, fastboot, dloader, charge, normal, vlx, caliberation.\n"
            "cboot could enter a mode specified by the mode descriptor.\n"
            "it also could enter a proper mode automatically depending on "
            "the environment\n"
          );
