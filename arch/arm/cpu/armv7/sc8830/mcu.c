/******************************************************************************
    David.Jia   2007.10.29      share_version_union

******************************************************************************/

#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/chip_drv_common_io.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sprd_reg.h>
#include <asm/arch/chip_drvapi.h>

#if defined(CONFIG_CLK_PARA)
#include <asm/arch/clk_para_config.h>
const MCU_CLK_PARA_T mcu_clk_para=
{
    MAGIC_CLK,
    CLK_CA7_CORE,
    DDR_FREQ,
    CLK_CA7_AXI,
    CLK_CA7_DGB,
    CLK_CA7_AHB,
    CLK_CA7_APB,
    CLK_PUB_AHB,
    CLK_AON_APB,
    MAGIC_VOLTAGE,
    DCDC_ARM,
    DCDC_CORE
};
#endif

static void delay()
{
    volatile uint32 i;
    for (i=0; i<0x2000; i++);
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
#if defined(CONFIG_CLK_PARA)
    uint32 ahb_cfg;
    ahb_cfg  = REG32(REG_AP_CLK_AP_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= mcu_clk_para.clk_ca7_ahb;  //ahb select 192M           0:26M 1:76M 2:128M 3:192M
    REG32(REG_AP_CLK_AP_AHB_CFG) = ahb_cfg;

    ahb_cfg  = REG32(REG_AON_CLK_PUB_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= mcu_clk_para.clk_pub_ahb;  //pub ahb select 153M      0:26M 1:76M 2:128M 3:153M
    REG32(REG_AON_CLK_PUB_AHB_CFG) = ahb_cfg;
#else
    uint32 ahb_cfg;
    ahb_cfg  = REG32(REG_AP_CLK_AP_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= 3;  //ahb select 192M           0:26M 1:76M 2:128M 3:192M
    REG32(REG_AP_CLK_AP_AHB_CFG) = ahb_cfg;

    ahb_cfg  = REG32(REG_AON_CLK_PUB_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= 3;  //pub ahb select 153M      0:26M 1:76M 2:128M 3:153M
    REG32(REG_AON_CLK_PUB_AHB_CFG) = ahb_cfg;
#endif
    delay();
    return 0;
}

static uint32 ApbClkConfig()
{
#if defined(CONFIG_CLK_PARA)
    uint32 apb_cfg;
    apb_cfg  = REG32(REG_AP_CLK_AP_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= mcu_clk_para.clk_ca7_apb;  //apb select 64M            0:26M 1:64M 2:96M 3:128M
    REG32(REG_AP_CLK_AP_APB_CFG) = apb_cfg;

    apb_cfg = REG32(REG_AON_CLK_AON_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= mcu_clk_para.clk_aon_apb;  //aon apb select 128M        0:26M 1:76M 2:96M 3:128M
    REG32(REG_AON_CLK_AON_APB_CFG) = apb_cfg;
#else
    uint32 apb_cfg;
    apb_cfg  = REG32(REG_AP_CLK_AP_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= 1;  //apb select 64M            0:26M 1:64M 2:96M 3:128M
    REG32(REG_AP_CLK_AP_APB_CFG) = apb_cfg;

    apb_cfg = REG32(REG_AON_CLK_AON_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= 3;  //aon apb select 128M        0:26M 1:76M 2:96M 3:128M
    REG32(REG_AON_CLK_AON_APB_CFG) = apb_cfg;
#endif
    delay();
    return 0;
}

static uint32 AxiClkConfig(uint32 arm_clk)
{
#if defined(CONFIG_CLK_PARA)
    uint32 ca7_ckg_cfg;
    ca7_ckg_cfg  = REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &= ~(7<<8);
    ca7_ckg_cfg |= ((arm_clk/(mcu_clk_para.clk_ca7_axi+1))&0x7)<<8;
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;
#else
    uint32 ca7_ckg_cfg;
    ca7_ckg_cfg  = REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &= ~(7<<8);
    ca7_ckg_cfg |= ((arm_clk/(ARM_CLK_500M+1))&0x7)<<8;
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;
#endif
    delay();
    return 0;
}

static uint32 DbgClkConfig(uint32 arm_clk)
{
#if defined(CONFIG_CLK_PARA)
    uint32 ca7_ckg_cfg;
    ca7_ckg_cfg  =  REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &= ~(7<<16);
    ca7_ckg_cfg |=  ((arm_clk/(mcu_clk_para.clk_ca7_dgb+1))&0x7)<<16;
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;
#else
    uint32 ca7_ckg_cfg;
    ca7_ckg_cfg  =  REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &= ~(7<<16);
    ca7_ckg_cfg |=  ((arm_clk/(ARM_CLK_200M+1))&0x7)<<16;
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;
#endif
    delay();
    return 0;
}

static uint32 McuClkConfig(uint32 arm_clk)
{
    uint32 ca7_ckg_cfg;

    ca7_ckg_cfg  =  REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &= ~7; //a7 core select 26M
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;
    delay();

    SetMPllClk(arm_clk);

    ca7_ckg_cfg  =  REG32(REG_AP_AHB_CA7_CKG_CFG);
    ca7_ckg_cfg &= ~(7<<4);  //ap clk div = 0;
    ca7_ckg_cfg &= ~7;
    ca7_ckg_cfg |=  6; //a7 core select mcu MPLL       0:26M 1:(DPLL)533M 2:(CPLL)624M 3:(TDPLL)768M 4:(WIFIPLL)880M 5:(WPLL)921M 6:(MPLL)1200M
    REG32(REG_AP_AHB_CA7_CKG_CFG) = ca7_ckg_cfg;

    delay();
    return 0;
}

static uint32 ArmCoreConfig(uint32 arm_clk)
{
    uint32 dcdc_arm;

#if  defined(CONFIG_VOL_PARA)
    dcdc_arm  = ANA_REG_GET(ANA_REG_GLB_DCDC_ARM_ADI);
    dcdc_arm &= ~0xFF;

    if(mcu_clk_para.dcdc_arm < 700)
    {
	dcdc_arm |= (5<<5);
    }
    else if(mcu_clk_para.dcdc_arm < 800)
    {
	dcdc_arm |= (1<<5);
    }
    else if(mcu_clk_para.dcdc_arm < 900)
    {
	dcdc_arm |= (2<<5);
    }
    else if(mcu_clk_para.dcdc_arm < 1000)
    {
	dcdc_arm |= (3<<5);
    }
    else if(mcu_clk_para.dcdc_arm < 1100)
    {
	dcdc_arm |= (4<<5);
    }
    else if(mcu_clk_para.dcdc_arm < 1200)
    {
	dcdc_arm |= (0<<5);
    }
    else if(mcu_clk_para.dcdc_arm < 1300)
    {
	dcdc_arm |= (6<<5);
    }
    else
    {
	dcdc_arm |= (7<<5);
    }
    ANA_REG_SET(ANA_REG_GLB_DCDC_ARM_ADI, dcdc_arm);

    dcdc_arm  = ANA_REG_GET(ANA_REG_GLB_DCDC_CORE_ADI);
    dcdc_arm &= ~0xFF;

    if(mcu_clk_para.dcdc_core < 700)
    {
	dcdc_arm |= (5<<5);
    }
    else if(mcu_clk_para.dcdc_core < 800)
    {
	dcdc_arm |= (1<<5);
    }
    else if(mcu_clk_para.dcdc_core < 900)
    {
	dcdc_arm |= (2<<5);
    }
    else if(mcu_clk_para.dcdc_core < 1000)
    {
	dcdc_arm |= (3<<5);
    }
    else if(mcu_clk_para.dcdc_core < 1100)
    {
	dcdc_arm |= (4<<5);
    }
    else if(mcu_clk_para.dcdc_core < 1200)
    {
	dcdc_arm |= (0<<5);
    }
    else if(mcu_clk_para.dcdc_core < 1300)
    {
	dcdc_arm |= (6<<5);
    }
    else
    {
	dcdc_arm |= (7<<5);
    }
    ANA_REG_SET(ANA_REG_GLB_DCDC_CORE_ADI, dcdc_arm);
    REG32(REG_AP_APB_APB_EB) |= BIT_AP_CKG_EB;
#else

    dcdc_arm  = ANA_REG_GET(ANA_REG_GLB_DCDC_ARM_ADI);
    dcdc_arm &= ~0xFF;
    //1.0V  800M
    //1.1V  1100M
    //1.2V  1200M


#if (defined(CONFIG_SP8830EB)||defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP5735) || defined(CONFIG_SPX15) || defined(CONFIG_SP7730ECTRISIM))

    if (arm_clk <= ARM_CLK_900M)
    {
        dcdc_arm |= (6<<5); //set dcdcarmcore voltage 1.1V->1.2V
    }
    else
    {
        //dcdc_arm |= (6<<5); //set dcdcarmcore voltage 1.1V->1.2V
        //dcdc_arm = 1.1V + 0.5V
        dcdc_arm |= (16); //1000/32 mv=3.125mv, 16*3.125mv = 50mv
    }
#else
    if (arm_clk < ARM_CLK_1000M)
    {
        dcdc_arm |= (4<<5); //set dcdcarmcore voltage 1.1V->1.0V
    }
    else if (arm_clk >= ARM_CLK_1200M)
    {
        dcdc_arm |= (6<<5); //set dcdcarmcore voltage 1.1V->1.2V
    }
#endif

    ANA_REG_SET(ANA_REG_GLB_DCDC_ARM_ADI, dcdc_arm);
    REG32(REG_AP_APB_APB_EB) |= BIT_AP_CKG_EB;        // CKG enable
#endif

    delay();
    return 0;
}

static void AvsEb()
{
    REG32(REG_AON_APB_APB_EB1) |= BIT_AVS1_EB | BIT_AVS0_EB;
    REG32(0x4003003C) |= 0xF<<5; //enable channel5-8
    REG32(0x40300020)  = 2;
    REG32(0x4030001C)  = 1;
}

static uint32 ClkConfig(uint32 arm_clk)
{
    ArmCoreConfig(arm_clk);
    //AvsEb();
    AxiClkConfig(arm_clk);
    DbgClkConfig(arm_clk);
    McuClkConfig(arm_clk);
    AhbClkConfig();
    ApbClkConfig();
    return 0;
}

uint32 MCU_Init()
{

#if defined(CONFIG_CLK_PARA)
    if (ClkConfig(mcu_clk_para.core_freq))
#else
#if (defined(CONFIG_SP8830EB)||defined(CONFIG_SP8830EC)||defined(CONFIG_SP8835EB)||defined(CONFIG_SP7730EC)||defined(CONFIG_SP5735)||defined(CONFIG_SPX15) || defined(CONFIG_SP7730ECTRISIM))
    if (ClkConfig(ARM_CLK_1000M))
#else
    if (ClkConfig(ARM_CLK_800M))
#endif
#endif
        while(1);
    return 0;
}

#if defined(CONFIG_CLK_PARA)
void set_ddr_clk(uint32 ddr_clk)
{
    volatile uint32 reg_val;
    reg_val = REG32(REG_AON_APB_DPLL_CFG);
    reg_val &=~0x7ff;
    reg_val |= (ddr_clk/4);
    REG32(REG_AON_APB_DPLL_CFG)= reg_val;

    //select DPLL 533MHZ source clock
    reg_val = REG32(REG_AON_CLK_EMC_CFG);
    reg_val &= ~0x3;
    //    reg_val |= 0; //default:XTL_26M
    //    reg_val |= 1; //TDPLL_256M
    //    reg_val |= 2; //TDPLL_384M
    reg_val |= 3; //DPLL_533M
    REG32(REG_AON_CLK_EMC_CFG)= reg_val;

    delay();
}
#endif

void Chip_Init (void) /*lint !e765 "Chip_Init" is used by init.s entry.s*/
{
    uint32 value;
    
    #if defined(CONFIG_SPX15)
    value = ANA_REG_GET(0x4003883c);
    value &= ~0x7f00;
    value |= 0x38 << 8;
    ANA_REG_SET(0x4003883c,value);

    value = ANA_REG_GET(0x40038820);
    value &= ~0xff;
    value |= 0x38 << 0;
    ANA_REG_SET(0x40038820,value);
    #endif
    
    MCU_Init();
    sdram_init();
}

