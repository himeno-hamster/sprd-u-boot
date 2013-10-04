#include <config.h>
#include <asm/io.h>
#include <asm/arch/chip_drv_config_extern.h>
#include <asm/arch/bits.h>
#include <linux/types.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sprd_reg.h>
//#include <asm/arch/analog_reg_v3.h>

#define VIBRATOR_REG_UNLOCK (0x1A2B)
#define VIBRATOR_REG_LOCK   (~VIBRATOR_REG_UNLOCK)
#define VIBRATOR_STABLE_LEVEL   (4)
#define VIBRATOR_INIT_LEVEL (11)    //init level must larger than stable level
#define VIBRATOR_INIT_STATE_CNT (2)

#define mdelay(_ms) udelay(_ms*1000)

void set_vibrator(int on)
{
	int i = 0;
	ANA_REG_SET(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_UNLOCK); //unlock vibrator registor
	if(on == 0)
	{
		mdelay(150);
		ANA_REG_AND (ANA_REG_GLB_VIBR_CTRL0, ~(BIT_VIBR_PON));
	}
	else
	{
		ANA_REG_OR (ANA_REG_GLB_VIBR_CTRL0, BIT_VIBR_PON);
	}
	ANA_REG_SET(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_LOCK);   //lock vibrator registor
}

#define VIBR_STABLE_V_SHIFT 12
#define VIBR_STABLE_V_MSK   (0x0F << VIBR_STABLE_V_SHIFT)
#define VIBR_INIT_V_SHIFT   8
#define VIBR_INIT_V_MSK     (0x0F << VIBR_INIT_V_SHIFT)
#define VIBR_V_BP_SHIFT     4
#define VIBR_V_BP_MSK       (0x0F << VIBR_V_BP_SHIFT)

void vibrator_hw_init(void)
{
	ANA_REG_SET(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_UNLOCK); //unlock vibrator registor
	ANA_REG_OR(ANA_REG_GLB_RTC_CLK_EN, BIT_RTC_VIBR_EN);
	ANA_REG_MSK_OR(ANA_REG_GLB_VIBR_CTRL0, (VIBRATOR_INIT_LEVEL << VIBR_INIT_V_SHIFT), VIBR_INIT_V_MSK); //set init current level
	ANA_REG_MSK_OR(ANA_REG_GLB_VIBR_CTRL0, (VIBRATOR_STABLE_LEVEL << VIBR_STABLE_V_SHIFT), VIBR_STABLE_V_MSK); //set stable current level
	ANA_REG_SET(ANA_REG_GLB_VIBR_CTRL1, VIBRATOR_INIT_STATE_CNT);   //set convert count

	ANA_REG_SET(ANA_REG_GLB_VIBR_WR_PROT_VALUE, VIBRATOR_REG_LOCK);   //lock vibrator registor
}



