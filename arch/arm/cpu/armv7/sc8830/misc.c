#include <asm/arch/sprd_reg.h>
#include <asm/arch/sci_types.h>

/*
	REG_AON_APB_BOND_OPT0  ==> romcode set
	REG_AON_APB_BOND_OPT1  ==> set it later

	!!! notice: these two registers can be set only one time!!!

	B1[0] : B0[0]
	0     : 0     Jtag enable
	0     : 1     Jtag disable
	1     : 0     Jtag enable
	1     : 1     Jtag enable
*/

/*************************************************
* 1 : enable jtag success                        *
* 0 : enable jtag fail                           *
*************************************************/
int sprd_jtag_enable()
{
	if (*((volatile unsigned int *)(REG_AON_APB_BOND_OPT0)) & 1)
	{
		*((volatile unsigned int *)(REG_AON_APB_BOND_OPT1)) = 1;
		if (!((*(volatile unsigned int *)(REG_AON_APB_BOND_OPT1)) & 1))
			return 0;
	}
	return 1;
}

/*************************************************
* 1 : disable jtag success                       *
* 0 : disable jtag fail                          *
*************************************************/
int sprd_jtag_disable()
{
	if (!(*((volatile unsigned int *)(REG_AON_APB_BOND_OPT0)) & 1))
	{
		return 0;
	}
	else
	{
		*((volatile unsigned int *)(REG_AON_APB_BOND_OPT1)) = 0;
		if (*((volatile unsigned int *)(REG_AON_APB_BOND_OPT1)) & 1)
			return 0;
		else
			return 1;
	}
}

static void ap_slp_cp_dbg_cfg()
{
	*((volatile unsigned int *)(REG_AP_AHB_MCU_PAUSE)) |= BIT_MCU_SLEEP_FOLLOW_CA7_EN; //when ap sleep, cp can continue debug
}

static void ap_cpll_rel_cfg()
{
	*((volatile unsigned int *)(REG_PMU_APB_CPLL_REL_CFG)) |= BIT_CPLL_AP_SEL;
}

static void bb_bg_auto_en()
{
	*((volatile unsigned int *)(REG_AON_APB_RES_REG0)) |= 1<<8;
}


static void ap_close_wpll_en()
{
       *((volatile unsigned int *)(REG_PMU_APB_CGM_AP_EN)) &= ~BIT_CGM_WPLL_AP_EN;
}

static void ap_close_cpll_en()
{
       *((volatile unsigned int *)(REG_PMU_APB_CGM_AP_EN)) &= ~BIT_CGM_CPLL_AP_EN;
}

static void ap_close_wifipll_en()
{
       *((volatile unsigned int *)(REG_PMU_APB_CGM_AP_EN)) &= ~BIT_CGM_WIFIPLL1_AP_EN;
}


static void bb_ldo_auto_en()
{
	*((volatile unsigned int *)(REG_AON_APB_RES_REG0)) |= 1<<9;
} 

void misc_init()
{
	ap_slp_cp_dbg_cfg();
	ap_cpll_rel_cfg();
	ap_close_wpll_en();
	ap_close_cpll_en();
	ap_close_wifipll_en();
	bb_bg_auto_en();
	bb_ldo_auto_en();
}

