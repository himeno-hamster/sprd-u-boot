#ifndef _BOOT_MODE_H_
#define _BOOT_MODE_H_

int get_mode_from_file(void);
void normal_mode(void);
void recovery_mode(void);
void charge_mode(void);
void fastboot_mode(void);
void alarm_mode(void);
void calibration_detect(int key);
void engtest_mode(void);
void watchdog_mode(void);
void unknow_reboot_mode(void);
void special_mode(void);
void panic_reboot_mode(void);
#ifdef CONFIG_AUTODLOADER
void autodloader_mode(void);
#else
#define autodloader_mode() do {} while (0)
#endif
int is_bat_low(void);
int alarm_flag_check(void);
int cali_file_check(void);
int read_adc_calibration_data(char *buffer,int size);
#define RECOVERY_MODE   0x77665502
#define FASTBOOT_MODE   0x77665500
#define NORMAL_MODE   0x77665503
#define ALARM_MODE   0x77665504
#define SLEEP_MODE   0x77665505
#define WATCHDOG_REBOOT 0x77665506
#define SPECIAL_MODE 0x77665507
#define UNKNOW_REBOOT_MODE 0x77665508
#define PANIC_REBOOT 0x77665509
#define CALIBRATION_MODE 0x7766550a
#define AUTODLOADER_REBOOT 0x77665510

#define BOOT_NORAML 0xf1
#define BOOT_FASTBOOT 0xf2
#define BOOT_RECOVERY 0xf3
#define BOOT_CALIBRATE 0xf4
#define BOOT_DLOADER 0xf5
#define BOOT_CHARGE 0xf6
#define BOOT_PANIC 0xf7

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 0

extern unsigned int check_key_boot(unsigned char key);
extern void vlx_nand_boot(char * kernel_pname, char * cmdline, int backlight_set);
#endif
