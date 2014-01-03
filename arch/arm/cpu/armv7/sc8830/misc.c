#include <common.h>
#include <asm/io.h>
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

#ifdef CONFIG_PBINT_7S_RESET
static inline int pbint_7s_rst_disable(uint32 disable)
{
	if (disable) {
		sci_adi_set(ANA_REG_GLB_POR_7S_CTRL, BIT_PBINT_7S_RST_DISABLE);
	} else {
		sci_adi_clr(ANA_REG_GLB_POR_7S_CTRL, BIT_PBINT_7S_RST_DISABLE);
	}
	return 0;
}

#define PBINT_7S_RST_HW_MODE 1
#define PBINT_7S_RST_SW_MODE 0
static inline int pbint_7s_rst_set_sw(uint32 mode)
{
	if (mode) {
		sci_adi_set(ANA_REG_GLB_POR_7S_CTRL, BIT_PBINT_7S_RST_MODE);
	} else {
		sci_adi_clr(ANA_REG_GLB_POR_7S_CTRL, BIT_PBINT_7S_RST_MODE);
	}
	return 0;
}

#define PBINT_7S_RST_SW_LONG_MODE 0
#define PBINT_7S_RST_SW_SHORT_MODE 1
static inline int pbint_7s_rst_set_swmode(uint32 mode)
{
	if (mode) {
		sci_adi_set(ANA_REG_GLB_POR_7S_CTRL, BIT_PBINT_7S_RST_SWMODE);
	} else {
		sci_adi_clr(ANA_REG_GLB_POR_7S_CTRL, BIT_PBINT_7S_RST_SWMODE);
	}
	return 0;
}

static inline int pbint_7s_rst_set_threshold(uint32 th)
{
	int mask = BITS_PBINT_7S_RST_THRESHOLD(-1);
	int shift = ffs(mask) - 1;
	sci_adi_write(ANA_REG_GLB_POR_7S_CTRL, (th << shift) & mask, mask);
	return 0;
}

int pbint_7s_rst_cfg(uint32 en, uint32 sw_rst)
{
	/* ignore sw_rst, please refer to config.h */
	if (en) {
#if defined CONFIG_PBINT_7S_RST_THRESHOLD
		pbint_7s_rst_set_threshold(CONFIG_PBINT_7S_RST_THRESHOLD);
#endif
#if defined CONFIG_PBINT_7S_RST_SW_SHORT
		pbint_7s_rst_set_sw(PBINT_7S_RST_SW_MODE);
		pbint_7s_rst_set_swmode(PBINT_7S_RST_SW_SHORT_MODE);
#elif defined CONFIG_PBINT_7S_RST_SW_LONG
		pbint_7s_rst_set_sw(PBINT_7S_RST_SW_MODE);
		pbint_7s_rst_set_swmode(PBINT_7S_RST_SW_LONG_MODE);
#elif defined CONFIG_PBINT_7S_RST_HW_LONG
		pbint_7s_rst_set_sw(PBINT_7S_RST_HW_MODE);
		pbint_7s_rst_set_swmode(PBINT_7S_RST_SW_LONG_MODE);
#else
		pbint_7s_rst_set_sw(PBINT_7S_RST_HW_MODE);
		pbint_7s_rst_set_swmode(PBINT_7S_RST_SW_SHORT_MODE);
#endif
	}
	return pbint_7s_rst_disable(!en);
}
#else
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
#endif

#ifdef CONFIG_OF_LIBFDT
void scx35_pmu_reconfig(void)
{
	/* FIXME:
	 * turn on gpu/mm domain for clock device initcall, and then turn off asap.
	 */
	__raw_writel(__raw_readl(REG_PMU_APB_PD_MM_TOP_CFG)
		     & ~(BIT_PD_MM_TOP_FORCE_SHUTDOWN),
		     REG_PMU_APB_PD_MM_TOP_CFG);

	__raw_writel(__raw_readl(REG_PMU_APB_PD_GPU_TOP_CFG)
		     & ~(BIT_PD_GPU_TOP_FORCE_SHUTDOWN),
		     REG_PMU_APB_PD_GPU_TOP_CFG);

	__raw_writel(__raw_readl(REG_AON_APB_APB_EB0) | BIT_MM_EB |
		     BIT_GPU_EB, REG_AON_APB_APB_EB0);

	__raw_writel(__raw_readl(REG_MM_AHB_AHB_EB) | BIT_MM_CKG_EB,
		     REG_MM_AHB_AHB_EB);

	__raw_writel(__raw_readl(REG_MM_AHB_GEN_CKG_CFG)
		     | BIT_MM_MTX_AXI_CKG_EN | BIT_MM_AXI_CKG_EN,
		     REG_MM_AHB_GEN_CKG_CFG);

	__raw_writel(__raw_readl(REG_MM_CLK_MM_AHB_CFG) | 0x3,
		     REG_MM_CLK_MM_AHB_CFG);

}

#else
void scx35_pmu_reconfig(void) {}
#endif

#ifdef CONFIG_SMPL_MODE
int is_smpl_bootup(void)
{
	return sci_adi_read(ANA_REG_GLB_BA_CTRL1) & BIT_IS_SMPL_ON;
}

#define SMPL_MODE_ENABLE_SET	(0x1935)
static int smpl_config(void)
{
	u32 val = BITS_SMPL_ENABLE(SMPL_MODE_ENABLE_SET);
#ifdef CONFIG_SMPL_THRESHOLD
	val |= BITS_SMPL_THRESHOLD(CONFIG_SMPL_THRESHOLD);
#endif
	return sci_adi_write_fast(ANA_REG_GLB_BA_CTRL0, val, 1);
}
#else
inline int is_smpl_bootup(void)
{
	return 0;
}

inline static int smpl_config(void)
{
	return 0;
}
#endif

void misc_init()
{
	scx35_pmu_reconfig();
	ap_slp_cp_dbg_cfg();
	ap_cpll_rel_cfg();
#ifndef  CONFIG_SPX15
	ap_close_wpll_en();
	ap_close_cpll_en();
	ap_close_wifipll_en();
#endif
	bb_bg_auto_en();
	bb_ldo_auto_en();
	pbint_7s_rst_cfg(1, 0);
	smpl_config();
}

