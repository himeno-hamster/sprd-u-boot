#include <common.h>
#include <malloc.h>
#include "key_map.h"
#include <boot_mode.h>
#include <asm/arch/mfp.h>
#include <asm/arch/sprd_keypad.h>
#include <asm/arch/chip_drv_common_io.h>

struct key_map_info * sprd_key_map = 0;

void board_keypad_init(void)
{
    unsigned int key_type;

    sprd_key_map = malloc(sizeof(struct key_map_info));

    if(NULL == sprd_key_map){
      printf("%s malloc faild\n", __FUNCTION__);
      return;
    }

    sprd_key_map->total_size = ARRAY_SIZE(board_key_map);
    sprd_key_map->keycode_size = sizeof(board_key_map[0])*2;
    sprd_key_map->key_map = board_key_map;
    sprd_key_map->total_row = CONFIG_KEYPAD_ROW_CNT;
    sprd_key_map->total_col = CONFIG_KEYPAD_COL_CNT;

    if(sprd_key_map->total_size % sprd_key_map->keycode_size){
        printf("%s: board_key_map config error, it should be %d aligned\n", __FUNCTION__, sprd_key_map->keycode_size);
        return;
    }

    /* init sprd keypad controller */
    REG32(REG_AON_APB_APB_EB0) |= BIT_8;
    REG32(REG_AON_APB_APB_RTC_EB) |= BIT_1;

    REG_KPD_INT_CLR = KPD_INT_ALL;
    REG_KPD_POLARITY = CFG_ROW_POLARITY | CFG_COL_POLARITY;
    REG_KPD_CLK_DIV_CNT = CFG_CLK_DIV & KPDCLK0_CLK_DIV0;
    REG_KPD_LONG_KEY_CNT = CONFIG_KEYPAD_LONG_CNT;
    REG_KPD_DEBOUNCE_CNT = CONFIG_KEYPAD_DEBOUNCE_CNT;//0x8;0x13
    REG_KPD_CTRL  = (7<<8)/*Col0-Col2 Enable*/|(7<<16)/*Row0-Row2 Enable*/;
    REG_KPD_CTRL |= 1; /*Keypad Enable*/;
}

static char handle_scan_code(unsigned char scan_code)
{
    int cnt;
    int key_map_cnt;
    unsigned char * key_map;
    int pos = 0;

    if(NULL == sprd_key_map){
        printf("plase call board_keypad_init first\n");
        return 0;
    }

    key_map_cnt = sprd_key_map->total_size / sprd_key_map->keycode_size;
    key_map = sprd_key_map->key_map;
#ifdef KEYPAD_DEBUG
    printf("scan code %d\n", scan_code);
#endif
    for(cnt = 0; cnt<key_map_cnt; cnt++){
        pos = cnt * 2;
        if(key_map[pos] == scan_code)
          return key_map[pos + 1];
    }
    return 0;
}

//it can only handle one key now
unsigned char board_key_scan(void)
{
    uint32_t s_int_status = REG_KPD_INT_RAW_STATUS;
    uint32_t s_key_status = REG_KPD_KEY_STATUS;
    uint32_t scan_code = 0;
    uint32_t key_code =0;
#ifdef KEYPAD_DEBUG
	printf("key operation flags is %08x, key %08x\n", REG_KPD_INT_RAW_STATUS, REG_KPD_KEY_STATUS);
#endif
    if((s_int_status & KPD_PRESS_INT0) || (s_int_status & KPD_LONG_KEY_INT0)){
        scan_code = s_key_status & (KPD1_ROW_CNT | KPD1_COL_CNT);
        key_code = handle_scan_code(scan_code);
    }else if((s_int_status & KPD_PRESS_INT1) || (s_int_status & KPD_LONG_KEY_INT1)){
        scan_code = (s_key_status & (KPD2_ROW_CNT | KPD2_COL_CNT)>>8);
        key_code = handle_scan_code(scan_code);
    }else if((s_int_status & KPD_PRESS_INT2) || (s_int_status & KPD_LONG_KEY_INT2)){
        scan_code = (s_key_status & (KPD3_ROW_CNT | KPD3_COL_CNT))>>16;
        key_code = handle_scan_code(scan_code);
    }else if((s_int_status & KPD_PRESS_INT3) || (s_int_status & KPD_LONG_KEY_INT3)){
        scan_code = (s_key_status & (KPD4_ROW_CNT | KPD4_COL_CNT))>>24;
        key_code = handle_scan_code(scan_code);
    }

    if(s_int_status)
        REG_KPD_INT_CLR = KPD_INT_ALL;
    return key_code;
}

unsigned int check_key_boot(unsigned char key)
{
    if(KEY_MENU == key)
      return BOOT_CALIBRATE;
    else if(KEY_HOME == key)
      return BOOT_FASTBOOT;
    else if(KEY_BACK == key)
      return BOOT_RECOVERY;
    else if(KEY_VOLUMEUP== key)
      return BOOT_DLOADER;
    else 
      return 0;
}
