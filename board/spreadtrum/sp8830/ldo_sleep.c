#include <asm/arch/sci_types.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/chip_drv_common_io.h>
#include <asm/arch/sprd_reg.h>

/***************************************************************************************************************************/
/*     VDD18 VDD28 VDD25 RF0 RF1 RF2 EMMCIO EMMCCORE DCDCARM DCDCWRF DCDCWPA DCDCGEN DCDCOTP AVDD18 SD SIM0 SIM1 SIM2 CAMA */
/* AP    x     x    v     v   v   v     v      v        v       v       v       x       v      v    v    v   v     v    v  */
/* CP0   x     x    v     v   v   x     x      x        x       v       x       x       x      x    x    x   x     x    x  */
/* CP1   x     x    v     x   x   x     x      x        x       x       x       x       x      x    x    x   x     x    x  */
/* CP2   x     x    v     v   x   v     x      x        x       v       x       x       x      x    x    x   x     x    x  */
/* EX0   x     x    x     v   x   x     x      x        x       x       x       x       x      x    x    x   x     x    x  */
/* EX1   x     x    x     x   v   x     x      x        x       x       x       x       x      x    x    x   x     x    x  */
/* EX2   x     x    x     v   x   x     x      x        x       x       x       x       x      x    x    x   x     x    x  */
/***************************************************************************************************************************/

/***************************************************************************************************************************/
/*     CAMD CMAIO CAMMOT USB CLSG LPREF LPRF0 LPRF1 LPRF2 LPEMMCIO LPEMMCCORE LPWPA  LPGEN   LPARM LPMEM LPCORE LPBG  BG   */
/* AP    v     v    v     v   v   v     v      v     v       v       v          x       v      v     v     v     v     v   */
/* CP0   x     x    x     x   x   x     x      x     x       x       x          x       x      x     x     x     x     x   */
/* CP1   x     x    x     x   x   x     x      x     x       x       x          x       x      x     x     x     x     x   */
/* CP2   x     x    x     x   x   x     v      v     x       x       x          x       x      x     x     x     x     x   */
/* EX0   x     x    x     x   x   x     x      x     v       x       x          x       x      x     x     x     x     x   */
/* EX1   x     x    x     x   x   x     x      x     x       x       x          x       x      x     x     x     x     x   */
/* EX2   x     x    x     x   x   x     x      x     x       x       x          x       x      x     x     x     x     x   */
/***************************************************************************************************************************/

void init_ldo_sleep_gr(void)
{
	/**********************************************
	 *   Following is AP LDO A DIE Sleep Control  *
	 *********************************************/
	ANA_REG_SET(ANA_REG_GLB_LDO_SLP_CTRL0,
		BIT_SLP_IO_EN |
		BIT_SLP_DCDC_OTP_PD_EN |
		//BIT_SLP_DCDCGEN_PD_EN |
		BIT_SLP_DCDCWPA_PD_EN |
		BIT_SLP_DCDCWRF_PD_EN |
		BIT_SLP_DCDCARM_PD_EN |
		BIT_SLP_LDOEMMCCORE_PD_EN |
		BIT_SLP_LDOEMMCIO_PD_EN |
		BIT_SLP_LDORF2_PD_EN |
		BIT_SLP_LDORF1_PD_EN |
		BIT_SLP_LDORF0_PD_EN |
		BIT_SLP_LDOVDD25_PD_EN |
		//BIT_SLP_LDOVDD28_PD_EN |
		//BIT_SLP_LDOVDD18_PD_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_LDO_SLP_CTRL1,
		BIT_SLP_LDO_PD_EN |
		BIT_SLP_LDOLPREF_PD_EN |
		BIT_SLP_LDOCLSG_PD_EN |
		BIT_SLP_LDOUSB_PD_EN |
		BIT_SLP_LDOCAMMOT_PD_EN |
		BIT_SLP_LDOCAMIO_PD_EN |
		BIT_SLP_LDOCAMD_PD_EN |
		BIT_SLP_LDOCAMA_PD_EN |
		BIT_SLP_LDOSIM2_PD_EN |
		//BIT_SLP_LDOSIM1_PD_EN |
		//BIT_SLP_LDOSIM0_PD_EN |
		BIT_SLP_LDOSD_PD_EN |
		BIT_SLP_LDOAVDD18_PD_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_LDO_SLP_CTRL2,
		//BIT_SLP_DCDC_BG_LP_EN |
		//BIT_SLP_DCDCCORE_LP_EN |
		//BIT_SLP_DCDCMEM_LP_EN |
		//BIT_SLP_DCDCARM_LP_EN |
		//BIT_SLP_DCDCGEN_LP_EN |
		//BIT_SLP_DCDCWPA_LP_EN |
		//BIT_SLP_DCDCWRF_LP_EN |
		//BIT_SLP_LDOEMMCCORE_LP_EN |
		//BIT_SLP_LDOEMMCIO_LP_EN |
		//BIT_SLP_LDORF2_LP_EN |
		//BIT_SLP_LDORF1_LP_EN |
		//BIT_SLP_LDORF0_LP_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_LDO_SLP_CTRL3,
		//BIT_SLP_BG_LP_EN |
		//BIT_SLP_LDOVDD25_LP_EN |
		//BIT_SLP_LDOVDD28_LP_EN |
		//BIT_SLP_LDOVDD18_LP_EN |
		//BIT_SLP_LDOCLSG_LP_EN |
		//BIT_SLP_LDOUSB_LP_EN |
		//BIT_SLP_LDOCAMMOT_LP_EN |
		//BIT_SLP_LDOCAMIO_LP_EN |
		//BIT_SLP_LDOCAMD_LP_EN |
		//BIT_SLP_LDOCAMA_LP_EN |
		//BIT_SLP_LDOSIM2_LP_EN |
		//BIT_SLP_LDOSIM1_LP_EN |
		//BIT_SLP_LDOSIM0_LP_EN |
		//BIT_SLP_LDOSD_LP_EN |
		//BIT_SLP_LDOAVDD18_LP_EN |
		0
	);

	/****************************************
	*   Following is CP LDO Sleep Control  *
	****************************************/

	ANA_REG_SET(ANA_REG_GLB_LDO1828_XTL_CTL,
		//BIT_LDO_VDD18_EXT_XTL2_EN |
		//BIT_LDO_VDD18_EXT_XTL1_EN |
		//BIT_LDO_VDD18_EXT_XTL0_EN |  
		//BIT_LDO_VDD18_XTL2_EN     |
		//BIT_LDO_VDD18_XTL1_EN     |
		//BIT_LDO_VDD18_XTL0_EN     |
		//BIT_LDO_VDD28_EXT_XTL2_EN |
		//BIT_LDO_VDD28_EXT_XTL1_EN |
		//BIT_LDO_VDD28_EXT_XTL0_EN |
		//BIT_LDO_VDD28_XTL2_EN     |
		//BIT_LDO_VDD28_XTL1_EN     |
		//BIT_LDO_VDD28_XTL0_EN     |
		0
	); 

	ANA_REG_SET(ANA_REG_GLB_PWR_XTL_EN0,
		BIT_LDO_XTL_EN |
		//BIT_LDO_RF1_EXT_XTL2_EN |
		//BIT_LDO_RF1_EXT_XTL1_EN |
		//BIT_LDO_RF1_EXT_XTL0_EN |
		//BIT_LDO_RF1_XTL2_EN |
		BIT_LDO_RF1_XTL1_EN |
		BIT_LDO_RF1_XTL0_EN |
		//BIT_LDO_RF0_EXT_XTL2_EN |
		//BIT_LDO_RF0_EXT_XTL1_EN |
		//BIT_LDO_RF0_EXT_XTL0_EN |
		BIT_LDO_RF0_XTL2_EN |
		BIT_LDO_RF0_XTL1_EN |
		BIT_LDO_RF0_XTL0_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_PWR_XTL_EN1,
		//BIT_LDO_VDD25_EXT_XTL2_EN |
		//BIT_LDO_VDD25_EXT_XTL1_EN |
		//BIT_LDO_VDD25_EXT_XTL0_EN |
		BIT_LDO_VDD25_XTL2_EN |
		BIT_LDO_VDD25_XTL1_EN |
		BIT_LDO_VDD25_XTL0_EN |
		//BIT_LDO_RF2_EXT_XTL2_EN |
		//BIT_LDO_RF2_EXT_XTL1_EN |
		//BIT_LDO_RF2_EXT_XTL0_EN |
		//BIT_LDO_RF2_XTL2_EN |
		//BIT_LDO_RF2_XTL1_EN |
		//BIT_LDO_RF2_XTL0_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_PWR_XTL_EN2,
		//BIT_LDO_AVDD18_EXT_XTL2_EN |
		//BIT_LDO_AVDD18_EXT_XTL1_EN |
		//BIT_LDO_AVDD18_EXT_XTL0_EN |
		//BIT_LDO_AVDD18_XTL2_EN |
		//BIT_LDO_AVDD18_XTL1_EN |
		//BIT_LDO_AVDD18_XTL0_EN |
		//BIT_LDO_SIM2_EXT_XTL2_EN |
		//BIT_LDO_SIM2_EXT_XTL1_EN |
		//BIT_LDO_SIM2_EXT_XTL0_EN |
		//BIT_LDO_SIM2_XTL2_EN |
		//BIT_LDO_SIM2_XTL1_EN |
		//BIT_LDO_SIM2_XTL0_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_PWR_XTL_EN3,
		//BIT_DCDC_BG_EXT_XTL2_EN |
		//BIT_DCDC_BG_EXT_XTL1_EN |
		//BIT_DCDC_BG_EXT_XTL0_EN |
		BIT_DCDC_BG_XTL2_EN |
		BIT_DCDC_BG_XTL1_EN |
		BIT_DCDC_BG_XTL0_EN |
		//BIT_BG_EXT_XTL2_EN |
		//BIT_BG_EXT_XTL1_EN |
		//BIT_BG_EXT_XTL0_EN |
		//BIT_BG_XTL2_EN |
		//BIT_BG_XTL1_EN |
		//BIT_BG_XTL0_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_PWR_XTL_EN4,
		BIT_DCDC_WRF_XTL2_EN |
		//BIT_DCDC_WRF_XTL1_EN |
		BIT_DCDC_WRF_XTL0_EN |
		//BIT_DCDC_WPA_XTL2_EN |
		//BIT_DCDC_WPA_XTL1_EN |
		BIT_DCDC_WPA_XTL0_EN |
		BIT_DCDC_MEM_XTL2_EN |
		BIT_DCDC_MEM_XTL1_EN |
		BIT_DCDC_MEM_XTL0_EN |
		BIT_DCDC_GEN_XTL2_EN |
		BIT_DCDC_GEN_XTL1_EN |
		BIT_DCDC_GEN_XTL0_EN |
		BIT_DCDC_CORE_XTL2_EN |
		BIT_DCDC_CORE_XTL1_EN |
		BIT_DCDC_CORE_XTL0_EN |
		0
	);

	ANA_REG_SET(ANA_REG_GLB_PWR_XTL_EN5,
		//BIT_DCDC_WRF_EXT_XTL2_EN |
		//BIT_DCDC_WRF_EXT_XTL1_EN |
		//BIT_DCDC_WRF_EXT_XTL0_EN |
		//BIT_DCDC_WPA_EXT_XTL2_EN |
		//BIT_DCDC_WPA_EXT_XTL1_EN |
		//BIT_DCDC_WPA_EXT_XTL0_EN |
		//BIT_DCDC_MEM_EXT_XTL2_EN |
		//BIT_DCDC_MEM_EXT_XTL1_EN |
		//BIT_DCDC_MEM_EXT_XTL0_EN |
		//BIT_DCDC_GEN_EXT_XTL2_EN |
		//BIT_DCDC_GEN_EXT_XTL1_EN |
		//BIT_DCDC_GEN_EXT_XTL0_EN |
		//BIT_DCDC_CORE_EXT_XTL2_EN |
		//BIT_DCDC_CORE_EXT_XTL1_EN |
		//BIT_DCDC_CORE_EXT_XTL0_EN |
		0
	);

	/************************************************
	*   Following is AP/CP LDO D DIE Sleep Control   *
	*************************************************/

	CHIP_REG_SET(REG_PMU_APB_XTL0_REL_CFG,
		BIT_XTL0_AP_SEL |
		BIT_XTL0_CP0_SEL |
		BIT_XTL0_CP1_SEL |
		BIT_XTL0_CP2_SEL |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_XTL1_REL_CFG,
		BIT_XTL1_AP_SEL |
		BIT_XTL1_CP0_SEL |
		BIT_XTL1_CP1_SEL |
		//BIT_XTL1_CP2_SEL |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_XTL2_REL_CFG,
		//BIT_XTL2_AP_SEL |
		//BIT_XTL2_CP0_SEL |
		//BIT_XTL2_CP1_SEL |
		//BIT_XTL2_CP2_SEL |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_XTLBUF0_REL_CFG,
		BIT_XTLBUF0_CP2_SEL |
		BIT_XTLBUF0_CP1_SEL |
		BIT_XTLBUF0_CP0_SEL |
		BIT_XTLBUF0_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_XTLBUF1_REL_CFG,
		//BIT_XTLBUF1_CP2_SEL |
		BIT_XTLBUF1_CP1_SEL |
		BIT_XTLBUF1_CP0_SEL |
		BIT_XTLBUF1_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_MPLL_REL_CFG,
		//BIT_MPLL_REF_SEL |
		//BIT_MPLL_CP2_SEL |
		//BIT_MPLL_CP1_SEL |
		//BIT_MPLL_CP0_SEL |
		BIT_MPLL_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_DPLL_REL_CFG,
		//BIT_DPLL_REF_SEL |
		BIT_DPLL_CP2_SEL |
		BIT_DPLL_CP1_SEL |
		BIT_DPLL_CP0_SEL |
		BIT_DPLL_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_TDPLL_REL_CFG,
		//BIT_TDPLL_REF_SEL |
		BIT_TDPLL_CP2_SEL |
		BIT_TDPLL_CP1_SEL |
		BIT_TDPLL_CP0_SEL |
		BIT_TDPLL_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_WPLL_REL_CFG,
		//BIT_WPLL_REF_SEL |
		//BIT_WPLL_CP2_SEL |
		//BIT_WPLL_CP1_SEL |
		BIT_WPLL_CP0_SEL |
		//BIT_WPLL_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_CPLL_REL_CFG,
		//BIT_CPLL_REF_SEL |
		BIT_CPLL_CP2_SEL |
		BIT_CPLL_CP1_SEL |
		//BIT_CPLL_CP0_SEL |
		BIT_CPLL_AP_SEL  |
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_CA7_TOP_CFG,
		BIT_PD_CA7_TOP_AUTO_SHUTDOWN_EN		|
		BITS_PD_CA7_TOP_PWR_ON_DLY(8)     	|
		BITS_PD_CA7_TOP_PWR_ON_SEQ_DLY(2)	|
		BITS_PD_CA7_TOP_ISO_ON_DLY(4)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_CA7_C0_CFG,
		BIT_PD_CA7_C0_AUTO_SHUTDOWN_EN		|
		BITS_PD_CA7_C0_PWR_ON_DLY(8)		|
		BITS_PD_CA7_C0_PWR_ON_SEQ_DLY(6)	|
		BITS_PD_CA7_C0_ISO_ON_DLY(2)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_CA7_C1_CFG,
		BIT_PD_CA7_C1_FORCE_SHUTDOWN		|
		BITS_PD_CA7_C1_PWR_ON_DLY(8)		|
		BITS_PD_CA7_C1_PWR_ON_SEQ_DLY(4)	|
		BITS_PD_CA7_C1_ISO_ON_DLY(2)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_CA7_C2_CFG,
		BIT_PD_CA7_C2_FORCE_SHUTDOWN		|
		BITS_PD_CA7_C2_PWR_ON_DLY(8)		|
		BITS_PD_CA7_C2_PWR_ON_SEQ_DLY(4)	|
		BITS_PD_CA7_C2_ISO_ON_DLY(2)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_CA7_C3_CFG,
		BIT_PD_CA7_C3_FORCE_SHUTDOWN		|
		BITS_PD_CA7_C3_PWR_ON_DLY(8)		|
		BITS_PD_CA7_C3_PWR_ON_SEQ_DLY(4)	|
		BITS_PD_CA7_C3_ISO_ON_DLY(2)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_AP_SYS_CFG,
		BIT_PD_AP_SYS_AUTO_SHUTDOWN_EN		|
		BITS_PD_AP_SYS_PWR_ON_DLY(8)		|
		BITS_PD_AP_SYS_PWR_ON_SEQ_DLY(0)	|
		BITS_PD_AP_SYS_ISO_ON_DLY(6)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_MM_TOP_CFG,
		BIT_PD_MM_TOP_FORCE_SHUTDOWN		|
		BITS_PD_MM_TOP_PWR_ON_DLY(8)		|
		BITS_PD_MM_TOP_PWR_ON_SEQ_DLY(0)	|
		BITS_PD_MM_TOP_ISO_ON_DLY(4)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_GPU_TOP_CFG,
		BIT_PD_GPU_TOP_FORCE_SHUTDOWN		|
		BITS_PD_GPU_TOP_PWR_ON_DLY(8)	|
		BITS_PD_GPU_TOP_PWR_ON_SEQ_DLY(0)	|
		BITS_PD_GPU_TOP_ISO_ON_DLY(4)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PD_PUB_SYS_CFG,
		BIT_PD_PUB_SYS_AUTO_SHUTDOWN_EN		|
		BITS_PD_PUB_SYS_PWR_ON_DLY(8)		|
		BITS_PD_PUB_SYS_PWR_ON_SEQ_DLY(0)	|
		BITS_PD_PUB_SYS_ISO_ON_DLY(6)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_XTL_WAIT_CNT,
		BITS_XTL1_WAIT_CNT(0x39)		|
		BITS_XTL0_WAIT_CNT(0x39)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_XTLBUF_WAIT_CNT,
		BITS_XTLBUF1_WAIT_CNT(7)		|
		BITS_XTLBUF0_WAIT_CNT(7)		|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PLL_WAIT_CNT1,
		BITS_WPLL_WAIT_CNT(7)			|
		BITS_TDPLL_WAIT_CNT(7)			|
		BITS_DPLL_WAIT_CNT(7)			|
		BITS_MPLL_WAIT_CNT(7)			|
		0
	);

	CHIP_REG_SET(REG_PMU_APB_PLL_WAIT_CNT2,
		BITS_WIFIPLL2_WAIT_CNT(7)		|
		BITS_WIFIPLL1_WAIT_CNT(7)		|
		BITS_CPLL_WAIT_CNT(7)			|
		0
	);

	ANA_REG_SET(ANA_REG_GLB_SLP_WAIT_DCDCARM,
		BITS_SLP_IN_WAIT_DCDCARM(9)		|
		BITS_SLP_OUT_WAIT_DCDCARM(8)		|
		0
	);
}
