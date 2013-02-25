/******************************************************************************
    David.Jia   2007.10.29      share_version_union

    TCC -fpu None -O2 -bi -g+ -apcs /interwork -D__RUN_IN_SDRAM sdram_init.c
SC6600R    -D_BL_NF_SC6600R_
SC6600H    -D_BL_NF_SC6600H_
SC6600I    -D_BL_NF_SC6600I_
SC6800     -gtp -cpu ARM926EJ-S -D_REF_SC6800_ -D_BL_NF_SC6800_
******************************************************************************/

#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/arm_reg.h>
#include <asm/arch/sdram_cfg.h>
#include <asm/arch/chng_freq.h>
#include <asm/arch/sc_reg.h>
#include <asm/arch/sdram.h>
#include <asm/arch/chip.h>

#define REG32(x)   (*((volatile uint32 *)(x)))


typedef enum
{
	LPDDR2_DS_34_OHM = 0xd,
	LPDDR2_DS_40_OHM = 0xb,
	LPDDR2_DS_48_OHM = 0x9,
	LPDDR2_DS_60_OHM = 0x7,
	LPDDR2_DS_80_OHM = 0x5
}LPDDR2_MEM_DS_T_E;

typedef enum
{
	LPDDR1_DS_33_OHM = 0xa,
	LPDDR1_DS_31_OHM = 0xb,
	LPDDR1_DS_48_OHM = 0xc,
	LPDDR1_DS_43_OHM = 0xd,
	LPDDR1_DS_39_OHM = 0xe,
	LPDDR1_DS_55_OHM = 0x5,
	LPDDR1_DS_64_OHM = 0x4
}LPDDR1_MEM_DS_T_E;

typedef enum
{
	SDLL_PHS_DLY_DEF  = 0x0,
	SDLL_PHS_DLY_36   = 0x3,
	SDLL_PHS_DLY_54   = 0x2,
	SDLL_PHS_DLY_72   = 0x1,
	SDLL_PHS_DLY_90   = 0x0,
	SDLL_PHS_DLY_108  = 0x4,
	SDLL_PHS_DLY_126  = 0x8,
	SDLL_PHS_DLY_144  = 0x12
}SDLL_PHS_DLY_E;

typedef enum
{
	DQS_STEP_DLY_MIN  = 0,
	DQS_STEP_DLY_SUB3 = 0,
	DQS_STEP_DLY_SUB2 = 1,
	DQS_STEP_DLY_SUB1 = 2,
	DQS_STEP_DLY_NOM  = 3,
	DQS_STEP_DLY_DEF  = 3,
	DQS_STEP_DLY_ADD1 = 4,
	DQS_STEP_DLY_ADD2 = 5,
	DQS_STEP_DLY_ADD3 = 6,
	DQS_STEP_DLY_ADD4 = 7,
	DQS_STEP_DLY_MAX  = 7
}DQS_STEP_DLY_E;


#if 1
extern uint32 LPDDR1_MEM_DS; //lpddr1 driver strength,refer to multiPHY p155
extern uint32 LPDDR2_MEM_DS; //lpddr1 driver strength,

extern uint32 B0_SDLL_PHS_DLY; //byte0 sll dll phase delay 
extern uint32 B1_SDLL_PHS_DLY; //byte1 sll dll phase delay 
extern uint32 B2_SDLL_PHS_DLY; //byte2 sll dll phase delay 
extern uint32 B3_SDLL_PHS_DLY; //byte3 sll dll phase delay 

extern uint32 B0_DQS_STEP_DLY; //byte0 dqs step delay
extern uint32 B1_DQS_STEP_DLY; //byte1 dqs step delay
extern uint32 B2_DQS_STEP_DLY; //byte2 dqs step delay
extern uint32 B3_DQS_STEP_DLY; //byte3 dqs step delay
#endif

typedef enum MCU_CLK_TYPE_TAG
{
    ARM100_EMC50_AHB100,
    ARM200_EMC100_AHB100,
    ARM400_EMC200_AHB100,
    ARM400_EMC200_AHB200,
    ARM800_EMC100_AHB200,
    ARM870_EMC100_AHB200,
    ARM800_EMC200_AHB200,
    ARM800_EMC400_AHB200,
    ARM1000_EMC50_AHB200,
    ARM1000_EMC100_AHB200,
    ARM1000_EMC200_AHB200,
    ARM1000_EMC400_AHB200,
    ARM1200_EMC100_AHB200,
    ARM1200_EMC200_AHB200,
    ARM1200_EMC400_AHB200,
    MCU_CLK_TYPE_MAX
} MCU_CLK_TYPE_E;

typedef struct ARM_EMC_AHB_CLK_TAG
{
    uint32         mcu_clk;
    uint32         arm_clk;
    uint32         emc_clk;
    uint32         ahb_clk;
    MCU_CLK_TYPE_E clk_type;
} ARM_EMC_AHB_CLK_T;
#if 0
static const ARM_EMC_AHB_CLK_T s_arm_emc_ahb_clk[] =
{
//   mcu_clk       arm_clk       emc_clk       ahb_clk
    {ARM_CLK_800M, ARM_CLK_800M, ARM_CLK_100M, ARM_CLK_200M, ARM800_EMC100_AHB200},
    {ARM_CLK_800M, ARM_CLK_800M, ARM_CLK_200M, ARM_CLK_200M, ARM800_EMC200_AHB200},
    {ARM_CLK_800M, ARM_CLK_800M, ARM_CLK_400M, ARM_CLK_200M, ARM800_EMC400_AHB200},
    {ARM_CLK_1000M,ARM_CLK_1000M,ARM_CLK_100M, ARM_CLK_200M, ARM1000_EMC100_AHB200},
    {ARM_CLK_1000M,ARM_CLK_1000M,ARM_CLK_200M, ARM_CLK_200M, ARM1000_EMC200_AHB200},
    {ARM_CLK_1000M,ARM_CLK_1000M,ARM_CLK_400M, ARM_CLK_200M, ARM1000_EMC400_AHB200},
    {ARM_CLK_1200M,ARM_CLK_1200M,ARM_CLK_200M, ARM_CLK_200M, ARM1200_EMC200_AHB200},
    {ARM_CLK_1200M,ARM_CLK_1200M,ARM_CLK_400M, ARM_CLK_200M, ARM1200_EMC400_AHB200},
};

static uint32 GetClockCfg(MCU_CLK_TYPE_E clk_type, uint32 *mcu_clk, uint32 *arm_clk, uint32 *emc_clk, uint32 *ahb_clk)
{
    uint32 i;
    for (i=0; i<(sizeof(s_arm_emc_ahb_clk)/sizeof(s_arm_emc_ahb_clk[0])); i++)
    {
        if (s_arm_emc_ahb_clk[i].clk_type == clk_type)
        {
            *mcu_clk = s_arm_emc_ahb_clk[i].mcu_clk;
            *arm_clk = s_arm_emc_ahb_clk[i].arm_clk;
            *emc_clk = s_arm_emc_ahb_clk[i].emc_clk;
            *ahb_clk = s_arm_emc_ahb_clk[i].ahb_clk;
            return 0;
        }
    }
    return -1;
}
#endif
static void delay()
{
    uint32 i;
    for (i=0; i<0x100; i++);
}

static uint32 GET_MPLL_N()
{	
    return REG32(GR_MPLL_MN)&0x07FF;
}

static uint32 GET_MPLL_M()
{
    uint32 M;
    switch ((REG32(GR_MPLL_MN)>>16)&0x3)
    {
        case 0x0: M=2; break;
        case 0x3: M=13;break;
        default:  M=4; break;	
    }
    return M;
}

static void SET_MPLL_N(uint32 N)
{
    uint32 mpll;
    mpll =REG32(GR_MPLL_MN);
    mpll &= ~0x07FF;
    mpll |= N & 0x07FF;
    REG32(GR_MPLL_MN) = mpll;
        delay();
}
static int SET_MPLL_M(uint32 M)
{
    uint32 mpll;
    switch(M)
    {
        case 2: M=0; break;
        case 4: M=1; break;
        case 13:M=3; break;
        default:     return -1;
    }

    mpll = REG32(GR_MPLL_MN);
    mpll &= ~(0x3<<16);
    mpll |= (M<<16);
    REG32(GR_MPLL_MN) = mpll;
    delay();
    return 0;
}

static uint32 GetMPllClk (void)
{
    return GET_MPLL_M()*GET_MPLL_N()*1000000;
}

static uint32 SetMPllClk (uint32 clk)
{
    uint32 M, N, ret=1;
    clk /= 1000000;
    M = 4;
    N = clk/M;
    
    REG32(GR_GEN1) |= BIT_9;        // MPLL Write En
    
    if (!SET_MPLL_M(M))
    {       
        SET_MPLL_N(N);
        ret = 0;
    }
    REG32(GR_GEN1) &= ~BIT_9;       // MPLL Write Dis
    return ret;
}

static uint32 GET_DPLL_N()
{	
    return REG32(GR_DPLL_MN)&0x07FF;
}

static uint32 GET_DPLL_M()
{
    uint32 M;
    switch ((REG32(GR_DPLL_MN)>>16)&0x3)
    {
        case 0x0: M=2; break;
        case 0x3: M=13;break;
        default:  M=4; break;	
    }
    return M;
}

static uint32 GetDPllClk(void)
{
    return GET_DPLL_M()*GET_DPLL_N()*1000000;
}
#if 0
static uint32 EmcClkConfig(uint32 emc_clk)
{
    uint32 src_clk, div, ahb_arm_clk;

    ahb_arm_clk = REG32(AHB_ARM_CLK);
    
    ahb_arm_clk &= ~(0x3f<<14);
    ahb_arm_clk |= 0xf<<14;
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    delay();

    ahb_arm_clk &= ~(0x0f<<8);       //emc clock div = 0
    src_clk = GetDPllClk();
    div = src_clk / emc_clk;
    ahb_arm_clk &= ~(0x0f<<8);
    ahb_arm_clk |= ((div-1)&0x0f)<<8;
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    delay();

    ahb_arm_clk &= ~BIT_3;           //emc lcokc async
    ahb_arm_clk &= ~(0x03<<12);      //emc clock src= MPLL/2
    ahb_arm_clk |= (1<<12);          //emc clock src=DPLL
    
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    delay();
    return 0;
}
#endif
#define CLK_APB_SHIFT	(14)
#define CLK_APB_MASK	(0x3<<CLK_APB_SHIFT)
#define CLK_APB_26M	(26000000)
#define CLK_APB_51M2	(51200000)
#define CLK_APB_76M8	(76800000)
static void ApbClkConfig(uint32 apb_clk)
{
	uint32 clkdly_cfg;
	uint32 clk_apb_sel;
	switch(apb_clk){
	case CLK_APB_26M:
		clk_apb_sel = 0;
		break;
	case CLK_APB_51M2:
		clk_apb_sel = 1;
		break;
	case CLK_APB_76M8:
		clk_apb_sel = 3;
		break;
	}

	clkdly_cfg = REG32(GR_CLK_DLY);
	clkdly_cfg &= ~(CLK_APB_MASK);
	clkdly_cfg |= (clk_apb_sel<<CLK_APB_SHIFT);
	REG32(GR_CLK_DLY) = clkdly_cfg;
}

static uint32 AhbClkConfig(uint32 ahb_clk)
{
    uint32 ahb_arm_clk, div, mcu_clk;
    
    mcu_clk = GetMPllClk();

    ahb_arm_clk = REG32(AHB_ARM_CLK);
    div = mcu_clk/ahb_clk;
    if (div*ahb_clk != mcu_clk)
        div++;
    ahb_arm_clk &= ~(0x07<<4);
    ahb_arm_clk |= ((div-1)&0x7)<<4; //ahb clock
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    delay();
    
    ahb_arm_clk &= ~(0x03<<23);      //MCU clock = MPLL
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    delay();
    return 0;
}

static uint32 AxiClkConfig()
{
    uint32 ca5_cfg;
    ca5_cfg = REG32(AHB_CA5_CFG);
    ca5_cfg &= ~(3<<11);
    ca5_cfg |= (1<<11);
    REG32(AHB_CA5_CFG) = ca5_cfg;
    delay();
    return 0;
}

static uint32 ArmClkPeriSet()
{
    uint32 ahb_arm_clk;
    ahb_arm_clk  = REG32(AHB_ARM_CLK);
    ahb_arm_clk &= ~(7<<20);
    ahb_arm_clk |= 1<<20;
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    return 0;
}

static uint32 DbgClkConfig()
{
    uint32 ahb_arm_clk, dbg_div;
    ahb_arm_clk  = REG32(AHB_ARM_CLK);
    dbg_div = (ahb_arm_clk>>14)&0x3f;
    dbg_div++;
    ahb_arm_clk |= dbg_div<<14;
    REG32(AHB_ARM_CLK) = ahb_arm_clk;
    return 0;
}

static uint32 McuClkConfig(uint32 mcu_clk)
{
    if (SetMPllClk(mcu_clk))
        return -1;
    if (mcu_clk > ARM_CLK_800M)
    {
        ArmClkPeriSet();
        DbgClkConfig();
    }
    return 0;
}

static uint32 ClkConfig()
{
    //uint32 mcu_clk, arm_clk, emc_clk, ahb_clk, ahb_arm_clk, div;
    //if (GetClockCfg(clk_type, &mcu_clk, &arm_clk, &emc_clk, &ahb_clk))
    //    return -1;

    AxiClkConfig();
    McuClkConfig(1000000000);
    AhbClkConfig(200000000);
    ApbClkConfig(CLK_APB_76M8);
    //EmcClkConfig(emc_clk);

    return 0;
}

uint32 MCU_Init()
{
    MCU_CLK_TYPE_E clk_type;

    //clk_type = ARM1000_EMC50_AHB200;
    //clk_type = ARM1000_EMC100_AHB200;
    clk_type = ARM1000_EMC100_AHB200;
    //clk_type = ARM1000_EMC400_AHB200;
    //clk_type = ARM800_EMC100_AHB200;
    //clk_type = ARM800_EMC200_AHB200;
    //clk_type = ARM800_EMC400_AHB200;

    if (ClkConfig())
        while(1);
    return 0;
}

#define USE_SPL_DATA
#if defined USE_SPL_DATA

#define SPL_DATA_ADR (CONFIG_SYS_TEXT_BASE + (23*1024))

#define SPL_DATA_DBG

typedef struct
{
    uint32  tag;
    uint32  len;
    uint32  data[];
}spl_priv_data;

typedef struct
{
    uint32  flag;
    uint32  mem_drv;
    uint32  sdll_phase;
    uint32  dqs_step;
    uint32  check_sum; 
}emc_priv_data;

#define EMC_PRIV_DATA  0x10
#define EMC_MAGIC_DATA 0xabcd1234

#if defined SPL_DATA_DBG
#define SPL_DATA_DBG_ADR (SPL_DATA_ADR + 0x100)
#endif

static spl_priv_data* spl_data = NULL;
static emc_priv_data* emc_data = NULL; 

static uint32 GET_SPL_Data()
{
    uint32 ret = 1;

    spl_data = (spl_priv_data *)SPL_DATA_ADR;
    if (spl_data->tag == EMC_PRIV_DATA)
    {
        emc_data = (emc_priv_data *)(spl_data->data);
        if (emc_data->flag == EMC_MAGIC_DATA)
        {
            uint32 check_sum = 0;
            check_sum ^= emc_data->flag;
            check_sum ^= emc_data->mem_drv;
            check_sum ^= emc_data->sdll_phase;
            check_sum ^= emc_data->dqs_step;
            if (check_sum == emc_data->check_sum)
                ret = 0;
        }
    }
    return ret;
}
#endif

void Chip_Init (void) /*lint !e765 "Chip_Init" is used by init.s entry.s*/
{
    uint32 ret;
#if defined CONFIG_GARDA
    /*disable 1s debounce at system boot.*/
    REG32(ANA_POR_SRC_STATUS) = POR_PBCHGR_MASK_SET;
#endif
    MCU_Init();
    
    ret = GET_SPL_Data();

#if 0    
#if defined USE_SPL_DATA

    if (ret != 0)
    {
        DMC_Init(0, 0, 0, 0);
    }
    else
    {
#if defined SPL_DATA_DBG
        *((volatile unsigned int *)(SPL_DATA_DBG_ADR + 0x00)) = emc_data->mem_drv;
        *((volatile unsigned int *)(SPL_DATA_DBG_ADR + 0x04)) = emc_data->sdll_phase;
        *((volatile unsigned int *)(SPL_DATA_DBG_ADR + 0x08)) = emc_data->dqs_step;
        *((volatile unsigned int *)(SPL_DATA_DBG_ADR + 0x0C)) = emc_data->check_sum;
#endif
        DMC_Init(0, emc_data->mem_drv, emc_data->sdll_phase, emc_data->dqs_step);
    }
#else
    DMC_Init(0, 0, 0, 0);
#endif
#else

    if(CONFIG_DDR_TIMING_CUSTOM == TRUE)
    {
        LPDDR1_MEM_DS = CONFIG_LPDDR1_DS;
        LPDDR2_MEM_DS = CONFIG_LPDDR2_DS;

        B0_SDLL_PHS_DLY = CONFIG_BYTE0_PHS_DLY;
        B1_SDLL_PHS_DLY = CONFIG_BYTE1_PHS_DLY;
        B2_SDLL_PHS_DLY = CONFIG_BYTE2_PHS_DLY;
        B3_SDLL_PHS_DLY = CONFIG_BYTE3_PHS_DLY;

        B0_DQS_STEP_DLY = CONFIG_BYTE0_STEP_DLY;
        B1_DQS_STEP_DLY = CONFIG_BYTE1_STEP_DLY;
        B2_DQS_STEP_DLY = CONFIG_BYTE2_STEP_DLY;
        B3_DQS_STEP_DLY = CONFIG_BYTE3_STEP_DLY;
    }


    if (ret == 0) //chesum is pass       
    {
        LPDDR1_MEM_DS = emc_data->mem_drv;
        LPDDR2_MEM_DS = emc_data->mem_drv;
        
        B0_SDLL_PHS_DLY = (emc_data->sdll_phase&0xff000000)>>24;
        B1_SDLL_PHS_DLY = (emc_data->sdll_phase&0xff0000)>>16;
        B2_SDLL_PHS_DLY = (emc_data->sdll_phase&0xff00)>>8;
        B3_SDLL_PHS_DLY = emc_data->sdll_phase&0xff;
        
        B0_DQS_STEP_DLY = (emc_data->dqs_step&0xff000000)>>24;        
        B1_DQS_STEP_DLY = (emc_data->dqs_step&0xff0000)>>16;
        B2_DQS_STEP_DLY = (emc_data->dqs_step&0xff00)>>8;
        B3_DQS_STEP_DLY = emc_data->dqs_step&0xff;
    }

    DMC_Dev_Init(400000000);

#endif
}

