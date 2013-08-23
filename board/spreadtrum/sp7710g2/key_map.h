#ifndef _KEY_MAP_H_
#define _KEY_MAP_H_

#include <linux/key_code.h>

static unsigned char board_key_map[]={
#if 0
    0x27, KEY_HOME,
    0x42, KEY_BACK,
    0x41, KEY_VOLUMEUP,
#else
    0x00, KEY_BACK,//DOWN
    0x01, KEY_HOME,//CAM
    0x10, KEY_MENU,//UP
#endif
};

#define CONFIG_KEYPAD_ROW_CNT 5
#define CONFIG_KEYPAD_COL_CNT 5
#ifdef CONFIG_SC7710G2

//chip define begin
#define SCI_COL7	(0x01 << 15)
#define SCI_COL6	(0x01 << 14)
#define SCI_COL5	(0x01 << 13)
#define SCI_COL4	(0x01 << 12)
#define SCI_COL3	(0x01 << 11)
#define SCI_COL2	(0x01 << 10)

#define SCI_ROW7	(0x01 << 23)
#define SCI_ROW6	(0x01 << 22)
#define SCI_ROW5	(0x01 << 21)
#define SCI_ROW4	(0x01 << 20)
#define SCI_ROW3	(0x01 << 19)
#define SCI_ROW2	(0x01 << 18)
//chip define end

#define KPDCTL_ROW_MSK                  (0x3f << 18)	/* enable rows 2 - 7 */
#define KPDCTL_COL_MSK                  (0x3f << 10)	/* enable cols 2 - 7 */

#define CONFIG_KEYPAD_ROW_CHOOSE_HW	(0)	/*example SCI_ROW2 |SCI_ROW3 |SCI_ROW4*/
#define CONFIG_KEYPAD_COL_CHOOSE_HW	(0)    /*example SCI_COL2 |SCI_COL3 |SCI_COL4*/
#endif
#define CONFIG_KEYPAD_LONG_CNT 0xc
#define CONFIG_KEYPAD_DEBOUNCE_CNT 0x5
#endif //_KEY_MAP_H_
