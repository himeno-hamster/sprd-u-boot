#include <asm/arch/sprd_reg.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/adi_hal_internal.h>
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

void pbint_7s_rst_cfg(uint32 en_rst, uint32 sw_rst)
{
	uint16 reg_data = ANA_REG_GET(ANA_REG_GLB_POR_7S_CTRL);

	if (!en_rst)
	{
		reg_data |=  BIT_PBINT_7S_RST_DISABLE;
	}
	else
	{
		reg_data &= ~BIT_PBINT_7S_RST_DISABLE;
		if (sw_rst)
		{
			reg_data |=  BIT_PBINT_7S_RST_MODE_RTCSET;
			reg_data &= ~BIT_PBINT_7S_RST_MODE_RTCCLR;
			ANA_REG_SET(ANA_REG_GLB_POR_7S_CTRL, reg_data);

			do {
				reg_data = ANA_REG_GET(ANA_REG_GLB_POR_7S_CTRL);
			}while(!(reg_data&BIT_PBINT_7S_RST_MODE_RTCSTS));

			reg_data &= ~BIT_PBINT_7S_RST_MODE_RTCSET;
		}
		else
		{
			reg_data &= ~BIT_PBINT_7S_RST_MODE_RTCSET;
			reg_data |=  BIT_PBINT_7S_RST_MODE_RTCCLR;
			ANA_REG_SET(ANA_REG_GLB_POR_7S_CTRL, reg_data);

			do {
				reg_data = ANA_REG_GET(ANA_REG_GLB_POR_7S_CTRL);
			}while(reg_data&BIT_PBINT_7S_RST_MODE_RTCSTS);
			reg_data &= ~BIT_PBINT_7S_RST_MODE_RTCCLR;
		}
	}

	ANA_REG_SET(ANA_REG_GLB_POR_7S_CTRL, reg_data);
	//printf("ANA_REG_GLB_POR_7S_CTRL:%04X\r\n", ANA_REG_GET(ANA_REG_GLB_POR_7S_CTRL));
}

void misc_init()
{
	ap_slp_cp_dbg_cfg();
	ap_cpll_rel_cfg();
#ifndef  CONFIG_SPX15
	ap_close_wpll_en();
	ap_close_cpll_en();
	ap_close_wifipll_en();
#endif
	bb_bg_auto_en();
	bb_ldo_auto_en();
#ifndef CONFIG_SPX15
	pbint_7s_rst_cfg(1, 0);
#endif
}

