/******************************************************************************
    David.Jia   2007.10.29      share_version_union

******************************************************************************/

#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/chip_drv_common_io.h>
#include <asm/arch/arm_reg.h>
#include <asm/arch/dram_phy.h>
#include <asm/arch/chng_freq.h>
#include <asm/arch/sc_reg.h>
#include <asm/arch/chip.h>
#include <asm/arch/regs_ana.h>
#include <asm/arch/adi_hal_internal.h>

#define REG32(x)   (*((volatile uint32 *)(x)))

static void delay()
{
    uint32 i;
    for (i=0; i<0x100; i++);
}

static uint32 GET_MPLL_N()
{	
    return 0x4;
}

static uint32 GET_MPLL_M()
{
    return 0x12C;
}

static void SET_MPLL_N(uint32 N)
{
    delay();
}

static int SET_MPLL_M(uint32 M)
{
    delay();
    return 0;
}

static uint32 GetMPllClk (void)
{
    return 1000000;
}

static uint32 SetMPllClk (uint32 clk)
{
    uint32 M, N, ret=1;
    return ret;
}

static uint32 GET_DPLL_N()
{	
    return 0x04;
}

static uint32 GET_DPLL_M()
{
    uint32 M;
    return M;
}

static uint32 GetDPllClk(void)
{
    return GET_DPLL_M()*GET_DPLL_N()*1000000;
}

static uint32 AhbClkConfig(uint32 ahb_clk)
{
    uint32 ahb_arm_clk, div, mcu_clk;
    delay();
    return 0;
}

static uint32 ApbClkConfig(uint32 apb_clk)
{
	return 0;
}

static uint32 AxiClkConfig()
{
    delay();
    return 0;
}

static uint32 ArmClkPeriSet()
{
    return 0;
}

static uint32 DbgClkConfig()
{
    return 0;
}

static uint32 McuClkConfig(uint32 mcu_clk)
{
    return 0;
}

static uint32 ArmCoreConfig()
{
	uint32 dcdc_arm;
	//set dcdcarmcore voltage 1.1V->1.2V
	dcdc_arm  = ANA_REG_GET(ANA_APB_DCDC_ARM);
	dcdc_arm &= (7<<5);
	dcdc_arm |= (6<<5);
	ANA_REG_SET(ANA_APB_DCDC_ARM,dcdc_arm);
	return 0;
}

static uint32 ClkConfig()
{
	ArmCoreConfig();
    AxiClkConfig();
	DbgClkConfig();
    McuClkConfig(ARM_CLK_1400M);
    AhbClkConfig(ARM_CLK_200M);
	ApbClkConfig(ARM_CLK_50M);
    return 0;
}

uint32 MCU_Init()
{
    if (ClkConfig())
        while(1);
    return 0;
}

typedef struct
{
    uint32  flag;
    uint32  mem_drv;
    uint32  sdll_phase;
    uint32  dqs_step;
    uint32  check_sum;
}emc_priv_data;

void Chip_Init (void) /*lint !e765 "Chip_Init" is used by init.s entry.s*/
{
    //MCU_Init();
    //sdram_init(SDRAM_GetCfg());
}

