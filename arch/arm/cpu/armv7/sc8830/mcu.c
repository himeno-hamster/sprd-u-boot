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
    .magic_header = MAGIC_HEADER,
    .version = CONFIG_PARA_VERSION,
    .core_freq = CLK_CA7_CORE,
    .ddr_freq = DDR_FREQ,
    .axi_freq = CLK_CA7_AXI,
    .dgb_freq = CLK_CA7_DGB,
    .ahb_freq = CLK_CA7_AHB,
    .apb_freq = CLK_CA7_APB,
    .pub_ahb_freq = CLK_PUB_AHB,
    .aon_apb_freq = CLK_AON_APB,
    .dcdc_arm = DCDC_ARM,
    .dcdc_core = DCDC_CORE,
#ifdef DCDC_MEM
	.dcdc_mem = DCDC_MEM,
#endif
#ifdef DCDC_GEN
	.dcdc_gen = DCDC_GEN,
#endif
    .magic_end = MAGIC_END
};
#endif

static void delay()
{
    volatile uint32 i;
    for (i=0; i<0x100; i++);
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
    ahb_cfg |= mcu_clk_para.ahb_freq;  //ahb select 192M           0:26M 1:76M 2:128M 3:192M
    REG32(REG_AP_CLK_AP_AHB_CFG) = ahb_cfg;

    ahb_cfg  = REG32(REG_AON_CLK_PUB_AHB_CFG);
    ahb_cfg &=~3;
    ahb_cfg |= mcu_clk_para.pub_ahb_freq;  //pub ahb select 153M      0:26M 1:76M 2:128M 3:153M
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
    apb_cfg |= mcu_clk_para.apb_freq;  //apb select 64M            0:26M 1:64M 2:96M 3:128M
    REG32(REG_AP_CLK_AP_APB_CFG) = apb_cfg;

    apb_cfg = REG32(REG_AON_CLK_AON_APB_CFG);
    apb_cfg &=~3;
    apb_cfg |= mcu_clk_para.aon_apb_freq;  //aon apb select 128M        0:26M 1:76M 2:96M 3:128M
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
    ca7_ckg_cfg |= ((arm_clk/(mcu_clk_para.axi_freq+1))&0x7)<<8;
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
    ca7_ckg_cfg |=  ((arm_clk/(mcu_clk_para.dgb_freq+1))&0x7)<<16;
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


#if defined(CONFIG_VOL_PARA)
static const int dcdc_ctl_vol[][2] = {
	{5,650},{1,700},{2,800},{3,900},{4,1000},{0,1100},{6,1200},{7,1300}
};
void dcdc_calibrate(int chan, int to_vol)
{
	int i;
	uint32 cal_vol, ctl_vol = to_vol;

	uint32 length = ARRAY_SIZE(dcdc_ctl_vol);
	for (i = 0; i < length; i++) {
		if (ctl_vol < dcdc_ctl_vol[i][1])
			break;
	}

	if(i==0)
	{
		if (chan == 10) { // dcdc arm
			ANA_REG_SET(ANA_REG_GLB_DCDC_ARM_ADI,((dcdc_ctl_vol[i][0]&0x7)<<5));
		}
		else if (chan == 11) {//dcdc core
			ANA_REG_SET(ANA_REG_GLB_DCDC_CORE_ADI,((dcdc_ctl_vol[i][0]&0x7)<<5));
		}
	}
	else
	{
		cal_vol = ((ctl_vol - dcdc_ctl_vol[i-1][1]) * 32 / 100) & 0x1f;
		if (chan == 10) { // dcdc arm
			ANA_REG_SET(ANA_REG_GLB_DCDC_ARM_ADI, cal_vol |((dcdc_ctl_vol[i-1][0]&0x7)<<5));
		}
		else if (chan == 11) {//dcdc core
			ANA_REG_SET(ANA_REG_GLB_DCDC_CORE_ADI, cal_vol |((dcdc_ctl_vol[i-1][0]&0x7)<<5));
		}
	}
	for(i = 0; i < 0x1000; ++i){};
	return ;
}

/* DCDC MEM output select:
 * [BONDOPT2 BONDOPT1]
 * 00: DDR2 application (1.2v)
 * 01: DDR3L application (1.35v)
 * 10: DDR3 application (1.5v)
 * 11: DDR1 application (1.8v)
 * DCDC MEM converter control bits with two bonding options as [bpt2 bpt1 xxx], list below:
 * 000: 1.2v
 * 001: 1.25v
 * 010: 1.35v
 * 011: 1.30v
 * 100: 1.50v
 * 101: 1.40v
 * 110: 1.80v
 * 111: 1.90v
 * DCDC MEM calibration control bits with small adjust step is 200/32mv.
 */

struct dcdc_ctl_t {
	int idx:4, vol:12;
};

/* FIXME: manual sort dcdc control voltage
 */
const struct dcdc_ctl_t dcdc_mem_vols[] = {
	{.idx = 0, .vol = 1200,},	//chip default
	{.idx = 1, .vol = 1250,},
	{.idx = 3, .vol = 1300,},
	{.idx = 2, .vol = 1350,},
	{.idx = 5, .vol = 1400,},
	{.idx = 4, .vol = 1500,},
	{.idx = 6, .vol = 1800,},
	{.idx = 7, .vol = 1900,},
};

const struct dcdc_ctl_t dcdc_gen_vols[] = {
	{.idx = 1, .vol = 1800,},
	{.idx = 3, .vol = 1900,},
	{.idx = 2, .vol = 2000,},
	{.idx = 5, .vol = 2100,},
	{.idx = 0, .vol = 2200,},
	{.idx = 4, .vol = 2300,},
	{.idx = 6, .vol = 2400,},	//chip default
	{.idx = 7, .vol = 2500,},
};

void dcdc_mem_calibrate(int to_vol)
{
	int i;
	for (i = ARRAY_SIZE(dcdc_mem_vols) - 1; i >= 0; i--) {
		if (to_vol >= dcdc_mem_vols[i].vol) break;
	}
	if (i >= 0) {
		int cal = (to_vol - dcdc_mem_vols[i].vol) * 32 / 200;	/* FIXME: non roundup*/
		cal += 0x10;
		if (cal <= BITS_DCDC_MEM_CAL_ADI(~0)) {
			ANA_REG_SET(ANA_REG_GLB_DCDC_MEM_ADI, (dcdc_mem_vols[i].idx << 5) | (cal << 0));
		}
	}
	else {
		//TODO: downward adjustment
	}
}

void dcdc_gen_calibrate(int to_vol)
{
	int i;
	for (i = ARRAY_SIZE(dcdc_gen_vols) - 1; i >= 0; i--) {
		if (to_vol >= dcdc_gen_vols[i].vol) break;
	}
	if (i >= 0) {
		int cal = (to_vol - dcdc_gen_vols[i].vol) * 32 / 100;	/* FIXME: non roundup*/
		if (cal <= BITS_DCDC_GEN_CAL_ADI(~0)) {
			ANA_REG_SET(ANA_REG_GLB_DCDC_MEM_ADI, (dcdc_gen_vols[i].idx << 5) | (cal << 0));
		}
	}
}

#endif

static uint32 ArmCoreConfig(uint32 arm_clk)
{
    uint32 dcdc_arm;

#if defined(CONFIG_VOL_PARA)
    dcdc_calibrate(10,mcu_clk_para.dcdc_arm);	//dcdc arm
    dcdc_calibrate(11,mcu_clk_para.dcdc_core);	//dcdc core
    dcdc_mem_calibrate(mcu_clk_para.dcdc_mem);	//dcdc mem
    //dcdc_gen_calibrate(mcu_clk_para.dcdc_gen);	//dcdc gen for LDOs

    REG32(REG_AP_APB_APB_EB) |= BIT_AP_CKG_EB;
#else
    dcdc_arm  = ANA_REG_GET(ANA_REG_GLB_DCDC_ARM_ADI);
    dcdc_arm &= ~0xFF;
    //1.0V  800M
    //1.1V  1100M
    //1.2V  1200M


#if (defined(CONFIG_SP8830EB)||defined(CONFIG_SP8830EC) || defined(CONFIG_SP8835EB) || defined(CONFIG_SP7730EC) || defined(CONFIG_SP5735) || defined(CONFIG_SPX15) || defined(CONFIG_SP7730ECTRISIM) || defined(CONFIG_SC9620OPENPHONE))

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
#if (defined(CONFIG_SP8830EB)||defined(CONFIG_SP8830EC)||defined(CONFIG_SP8835EB)||defined(CONFIG_SP7730EC)||defined(CONFIG_SP5735)||defined(CONFIG_SPX15) || defined(CONFIG_SP7730ECTRISIM)||defined(CONFIG_SC9620OPENPHONE))
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

#if defined(CONFIG_VOL_PARA)
typedef struct {
	uint16 ideal_vol;
	const char name[14];
}vol_para_t;

vol_para_t vol_para[] __align(16) = {
	[0] = { /* Begin Array, DO NOT remove it! */
		.ideal_vol = 0xfaed,	.name = "volpara_begin",
	},
	[1] = {
		.ideal_vol = 1200,	.name = "vddarm",
	},
	[2] = {
		.ideal_vol = 1100,	.name = "vddcore",
	},
	[3] = {
		.ideal_vol = 0,	.name = "dcdcmem",
	},

	[4] = {
		.ideal_vol = 0,	.name = "dcdcgen",
	},

	//TODO: add your ideal ldo here like the following example
	{
		.ideal_vol = 1800,	.name = "vddsim2",
	},

	{ /* End Array, DO NOT remove it! */
		.ideal_vol = 0xdeaf,	.name = "volpara_end",
	},

};

int Vol_Init()
{
	/*
	 * FIXME: Update LDOs voltage in u-boot
	 */
	BUG_ON(sizeof(vol_para_t) != 16);
	(mcu_clk_para.dcdc_arm)?vol_para[1].ideal_vol = mcu_clk_para.dcdc_arm:0;
	(mcu_clk_para.dcdc_core)?vol_para[2].ideal_vol = mcu_clk_para.dcdc_core:0;
	(mcu_clk_para.dcdc_mem)?vol_para[3].ideal_vol = mcu_clk_para.dcdc_mem:0;
	(mcu_clk_para.dcdc_gen)?vol_para[4].ideal_vol = mcu_clk_para.dcdc_gen:0;
	return (sizeof(vol_para) << 16) + sizeof(vol_para_t);
}
#endif

static uint32 get_adie_chipid(void)
{
	uint32 chip_id;

	chip_id = (ANA_REG_GET(ANA_REG_GLB_CHIP_ID_HIGH) & 0xffff) << 16;
	chip_id |= ANA_REG_GET(ANA_REG_GLB_CHIP_ID_LOW) & 0xffff;

	return chip_id;
}

void Chip_Init (void) /*lint !e765 "Chip_Init" is used by init.s entry.s*/
{
    uint32 value;
    
    #if defined(CONFIG_SPX15)
	if(0x2711A000 == get_adie_chipid()) {
	    value = ANA_REG_GET(0x4003883c);
	    value &= ~0x7f00;
	    value |= 0x38 << 8;
	    ANA_REG_SET(0x4003883c,value);

	    value = ANA_REG_GET(0x40038820);
	    value &= ~0xff;
	    value |= 0x38 << 0;
	    ANA_REG_SET(0x40038820,value);
	}
    #endif
    
    MCU_Init();
#if defined(CONFIG_VOL_PARA)
    Vol_Init();
#endif
    sdram_init();
}

