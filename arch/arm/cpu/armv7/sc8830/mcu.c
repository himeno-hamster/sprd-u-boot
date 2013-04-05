/******************************************************************************
    David.Jia   2007.10.29      share_version_union

******************************************************************************/

#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/chip_drv_common_io.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sprd_reg.h>
#include <asm/arch/chip_drvapi.h>

static void delay()
{
    uint32 i;
    for (i=0; i<0x20; i++);
}

static uint32 SetMPllClk (uint32 clk)
{
    uint32 mpll_cfg, pll_sft_cnt;

    REG32(REG_AON_APB_PLL_SOFT_CNT_DONE) &= ~1;

    mpll_cfg  = REG32(REG_AON_APB_MPLL_CFG);
    mpll_cfg &=~(3<<24);
    mpll_cfg |= (1<<24);
    clk /= 4000000;
    mpll_cfg &=~(0x7ff);
    mpll_cfg |= clk&0x7ff;
    REG32(REG_AON_APB_MPLL_CFG) = mpll_cfg;

    delay();
    delay();
    delay();
    delay();
    delay();

    REG32(REG_AON_APB_PLL_SOFT_CNT_DONE) |=  1;
    return 0;
}

static uint32 AhbClkConfig()
{
    uint32 ahb_cfg;
    ahb_cfg  = REG32(REG_AP_CLK_AP_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= 2;  //ahb select 128M           0:26M 1:76M 2:128M 3:192M
    REG32(REG_AP_CLK_AP_AHB_CFG) = ahb_cfg;

    ahb_cfg  = REG32(REG_AON_CLK_PUB_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= 2;  //pub ahb select 128M      0:26M 1:76M 2:128M 3:153M
    REG32(REG_AON_CLK_PUB_AHB_CFG) = ahb_cfg;

    delay();
    return 0;
}

static uint32 ApbClkConfig()
{
    uint32 apb_cfg;
    apb_cfg  = REG32(REG_AP_CLK_AP_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= 1;  //apb select 64M            0:26M 1:64M 2:96M 3:128M
    REG32(REG_AP_CLK_AP_APB_CFG) = apb_cfg;

    apb_cfg = REG32(REG_AON_CLK_AON_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= 1;  //aon apb select 76M        0:26M 1:76M 2:96M 3:128M
    REG32(REG_AON_CLK_AON_APB_CFG) = apb_cfg;

    delay();
    return 0;
}

static uint32 AxiClkConfig()
{
    uint32 ca7_ckg_cfg;

    ca7_ckg_cfg  = REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &=~(7<<8);
    ca7_ckg_cfg |= (2<<8); //axi clk = a7 core / 3
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;

    delay();
    return 0;
}

static uint32 ArmClkPeriSet()
{
    return 0;
}

static uint32 DbgClkConfig()
{
    uint32 ca7_ckg_cfg;

    ca7_ckg_cfg  = REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &=~(7<<12);
    ca7_ckg_cfg |= (2<<12); //dbg clk = a7 core / 3
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;

    delay();
    return 0;
}

static uint32 McuClkConfig()
{
    uint32 ca7_ckg_cfg;

    //SetMPllClk(ARM_CLK_1500M);

    ca7_ckg_cfg  = REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &=~(7<<4);  //ap clk div = 0;
    ca7_ckg_cfg &=~7;
    ca7_ckg_cfg |= 6; //a7 core select mcu 900M        0:26M 1:(DPLL)533M 2:(CPLL)624M 3:(TDPLL)768M 4:(WIFIPLL)880M 5:(WPLL)921M 6:(MPLL)900M
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;

    delay();
    return 0;
}

static uint32 EmcClkConfig()
{
    uint32 emc_cfg;
    emc_cfg  = REG32(REG_AON_CLK_EMC_CFG);
    emc_cfg &= ~3; 
    emc_cfg |=  1; //emc select 256M                   0:26M 1:256M 2:384M 3:533M 
    REG32(REG_AON_CLK_EMC_CFG) = emc_cfg;

    delay();
    return 0;
}

static uint32 ArmCoreConfig()
{
    uint32 dcdc_arm;
    dcdc_arm  = ANA_REG_GET(ANA_REG_GLB_DCDC_ARM_ADI);
    dcdc_arm &=~(7<<5);
    dcdc_arm |= (6<<5); //set dcdcarmcore voltage 1.1V->1.2V
    ANA_REG_SET(ANA_REG_GLB_DCDC_ARM_ADI, dcdc_arm);
    REG32(REG_AP_APB_APB_EB) |= BIT_AP_CKG_EB;        // CKG enable
    delay();
    return 0;
}

static uint32 ClkConfig()
{
    ArmCoreConfig();
    AxiClkConfig();
    DbgClkConfig();
    McuClkConfig();
    AhbClkConfig();
    ApbClkConfig();
    EmcClkConfig();
    return 0;
}

uint32 MCU_Init()
{
    if (ClkConfig())
        while(1);
    return 0;
}

void Chip_Init (void) /*lint !e765 "Chip_Init" is used by init.s entry.s*/
{
    MCU_Init();
    sdram_init();
}

