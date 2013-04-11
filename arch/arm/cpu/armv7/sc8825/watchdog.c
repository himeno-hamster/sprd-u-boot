#include <config.h>
#include <asm/io.h>
#include <asm/arch/chip_drv_config_extern.h>
#include <asm/arch/bits.h>
#include <linux/types.h>

void start_watchdog(uint32_t init_time_ms)
{
    //WDG_TimerStart(init_time_ms);
    WDG_ClockOn();
    WDG_ResetMCU();
}

void stop_watchdog(void)
{
    WDG_TimerStop();
	
}

void init_watchdog(void)
{
    WDG_TimerInit();
}

void feed_watchdog(void)
{
}

void load_watchdog(uint32_t time_ms)
{
    WDG_TimerLoad(time_ms);
}
void hw_watchdog_reset(void)
{
   // WDG_ResetMCU();
   //load_watchdog(CONFIG_WDG_INIT_VALUE);
}

int hw_watchdog_rst_pending(void)
{
    WDG_ClockOn();
	return WDG_PHY_RST_INT_ON();
}

/* the last word is used as a flag of weather dump memory after fatal error 
 * such as kernel panic, hw watchdog fire
 * */
int fatal_dump_enabled(void)
{
	if(REG32(0x20900218) & 0x1) /* get remap status */
		return REG32(0x7ffc);
	else
		return REG32(0x27ffc);
}
