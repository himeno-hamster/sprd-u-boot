/******************************************************************************
 ** File Name:        EMC_test.c
 ** Author:           Johnny Wang
 ** DATE:             27/07/2010
 ** Copyright:        2007 Spreatrum, Incoporated. All Rights Reserved.
 ** Description:
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History
 **-------------------------------------------------------------------------
 ** DATE          NAME            DESCRIPTION
 ** 23/04/2009                    Create.
 ******************************************************************************/
#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/arm_reg.h>
#include <asm/arch/sc_reg.h>

//#include "mem_bist.h"

//#include "dma.h"
//#include "dma_drv.h"

#include <asm/arch/sdram_sc7710g2.h>
#include <asm/arch/emc_config.h>
#include "asm/arch/chip_plf_export.h"

#if defined(EMC_TEST)
//#include "EMC_test.h"
#endif

#ifdef   __cplusplus
extern   "C"
{
#endif

/*----------------------------------------------------------------------------*
**                          Sub Function                                      *
**----------------------------------------------------------------------------*/



uint32 DRAM_CAP;
uint32  SDRAM_BASE    =    	0x00000000;//(128*1024*1024/2)//0x30000000


#define ROW_MODE_MASK           0x3
#define COL_MODE_MASK           0x7
#define DATA_WIDTH_MASK         0x1
#define AUTO_PRECHARGE_MASK     0x3
#define CS_POSITION_MASK        0x3

#define MEM_REF_DATA0       0x12345678
#define MEM_REF_DATA1       0x55AA9889
#define ZERO_ADDR           0x00000000UL

#define BYTE_OFFSET         3UL   // 1BYTE = 8BIT = 2^3
#define WIDTH16_OFFSET      4UL   // 16BIT = 2^4
#define WIDTH32_OFFSET      5UL   // 32BIT = 2^5
#define BANK_OFFSET         2UL   // 4BANK = 2^2

LOCAL void set_cp_emc_pad(void);
LOCAL void set_cp_jtag_pad(void);

/**---------------------------------------------------------------------------*
 **                     Static Function Prototypes                            *
 **---------------------------------------------------------------------------*/

__inline uint32 delay(uint32 k)
{
    uint32 i, j;

    for (i=0; i<k; i++)
    {
        for (j=0; j<1000; j++);
    }

    return k;
}

void EMC_AddrMode_Set(SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 reg_val = 0;

    reg_val = REG32(EXT_MEM_DCFG0);
    reg_val &= ~((DATA_WIDTH_MASK<<8) | (COL_MODE_MASK<<4) | ROW_MODE_MASK);
    reg_val |= (((mem_info->data_width & DATA_WIDTH_MASK)<< 8)
                | ((mem_info->col_mode & COL_MODE_MASK) << 4)
                | (mem_info->row_mode & ROW_MODE_MASK));

    REG32(EXT_MEM_DCFG0) = reg_val;
}

LOCAL BOOLEAN SDRAM_Type_Set(SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 err_line = 0;
    DMEM_TYPE_E sdram_mode = mem_info->sdram_type;

    REG32(EXT_MEM_DCFG0) &= ~ BIT_14;

    if (SDR_SDRAM == sdram_mode)
    {
                
        REG32(EXT_MEM_DCFG6) = 0X400020;
        REG32(EXT_MEM_DCFG7) = 0XF0000E;
        REG32(EXT_MEM_DCFG8) = 0X400001;

        delay(100);
        REG32(EXT_MEM_DCFG4) = 0X40010000;

        delay(100);
        REG32(EXT_MEM_DCFG4) = 0X40020000;

        delay(100);
        REG32(EXT_MEM_DCFG4) = 0X40020000;

        delay(100);
        REG32(EXT_MEM_DCFG4) = 0X40040031;

        delay(100);

        REG32(EXT_MEM_DCFG0) &= ~(AUTO_PRECHARGE_MASK<<2);
    }
    else if (DDR_SDRAM == sdram_mode)
    {

        REG32(EXT_MEM_DCFG0) &= ~(AUTO_PRECHARGE_MASK<<2);

        if (DATA_WIDTH_16 == mem_info->data_width)
        {
            if (CAS_LATENCY_2 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG6) = 0x00080004;
            }
            else if (CAS_LATENCY_3 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG6) = 0x00200010;
            }
        }
        else
        {
            if (CAS_LATENCY_2 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG6) = 0x00200010;
            }
            else if (CAS_LATENCY_3 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG6) = 0x00400020;
            }
        }
        
        REG32(EXT_MEM_DCFG7) = 0x00F0000E;
        REG32(EXT_MEM_DCFG8) = 0x00F0000E; 

#ifdef DLL_OPEN
        REG32(EXT_MEM_CFG0_DLL) = 0x21080; // open dll
        while (0==(REG32(0xA0000170))&BIT_14); //wait dll locked
        REG32(EXT_MEM_DL0) = 0x8020; //0x8040;
        REG32(EXT_MEM_DL1) = 0x8020; //0x8040;
        REG32(EXT_MEM_DL2) = 0x8020; //0x8040;
        REG32(EXT_MEM_DL3) = 0x8020; //0x8040;

        REG32(EXT_MEM_DL4) = 0x8020;
        REG32(EXT_MEM_DL5) = 0x8020;
        REG32(EXT_MEM_DL6) = 0x8020;
        REG32(EXT_MEM_DL7) = 0x8020;
        REG32(EXT_MEM_DL8) = 0x8020;
        REG32(EXT_MEM_DL9) = 0x8020;
        REG32(EXT_MEM_DL10) = 0x8020;
        REG32(EXT_MEM_DL11) = 0x8020;

        REG32(EXT_MEM_DL12) = 0x8040;
        REG32(EXT_MEM_DL13) = 0x8040;
        REG32(EXT_MEM_DL14) = 0x8040;
        REG32(EXT_MEM_DL15) = 0x8040;
        REG32(EXT_MEM_DL16) = 0x8040;
        REG32(EXT_MEM_DL17) = 0x8040;
        REG32(EXT_MEM_DL18) = 0x8040;
        REG32(EXT_MEM_DL19) = 0x8040;
        REG32(EXT_MEM_CFG0_DLL) |= BIT_10; //open dll cmp

#else
        REG32(EXT_MEM_DL0) = 0x0008;
        REG32(EXT_MEM_DL1) = 0x0008;
        REG32(EXT_MEM_DL2) = 0x0008;
        REG32(EXT_MEM_DL3) = 0x0008;

        REG32(EXT_MEM_DL4) = 0x0004;
        REG32(EXT_MEM_DL5) = 0x0004;
        REG32(EXT_MEM_DL6) = 0x0004;
        REG32(EXT_MEM_DL7) = 0x0004;
        REG32(EXT_MEM_DL8) = 0x0004;
        REG32(EXT_MEM_DL9) = 0x0004;
        REG32(EXT_MEM_DL10) = 0x0004;
        REG32(EXT_MEM_DL11) = 0x0004;

        REG32(EXT_MEM_DL12) = 0x0008;
        REG32(EXT_MEM_DL13) = 0x0008;
        REG32(EXT_MEM_DL14) = 0x0008;
        REG32(EXT_MEM_DL15) = 0x0008;
        REG32(EXT_MEM_DL16) = 0x0008;
        REG32(EXT_MEM_DL17) = 0x0008;
        REG32(EXT_MEM_DL18) = 0x0008;
        REG32(EXT_MEM_DL19) = 0x0008;

#endif

        REG32(EXT_MEM_DCFG4) = 0x40010000;
        delay(100);
        REG32(EXT_MEM_DCFG4) = 0x40020000;
        delay(100);

        REG32(EXT_MEM_DCFG4) = 0x40020000;
        delay(100);

        REG32(EXT_MEM_DCFG4) = 0x40040031;
        delay(100);

        REG32(EXT_MEM_DCFG4) = 0x40048000;
        delay(100);

    }
    else
    {
        SCI_ASSERT(0);
    }

    //enable auto refresh.
    REG32(EXT_MEM_DCFG3) |= BIT_15;  //clear refresh count.
    REG32(EXT_MEM_DCFG0) |= DCFG0_AUTOREF_EN;  //Enable auto refresh.
    
    return TRUE;
}


LOCAL void EMC_SoftReset(void)
{
    REG32(AHB_SOFT_RST) |= BIT_11;
    delay(10);
    REG32(AHB_SOFT_RST) &= (~BIT_11);
    delay(10);
}

PUBLIC uint32 SDRAM_GetCap(SDRAM_CFG_INFO_T_PTR mem_info)  // capability in bytes
{
    uint32 SDRAM_Cap;

    SDRAM_CHIP_FEATURE_T_PTR mem_feature = SDRAM_GetFeature();

    SDRAM_Cap = mem_feature->cap;

    return SDRAM_Cap;
}

PUBLIC EMC_PHY_L1_TIMING_T_PTR EMC_GetPHYL1_Timing(DMEM_TYPE_E mem_type, uint32 cas_latency)
{
    if (SDR_SDRAM == mem_type)
    {
        if (CAS_LATENCY_2 == cas_latency)
        {
            return (EMC_PHY_L1_TIMING_T_PTR)(&(EMC_PHY_TIMING_L1_INFO[EMC_PHYL1_TIMING_SDRAM_LATENCY2]));
        }
        else
        {
            return (EMC_PHY_L1_TIMING_T_PTR)(&(EMC_PHY_TIMING_L1_INFO[EMC_PHYL1_TIMING_SDRAM_LATENCY3]));
        }
    }
    else
    {
        if (CAS_LATENCY_2 == cas_latency)
        {
            return (EMC_PHY_L1_TIMING_T_PTR)(&(EMC_PHY_TIMING_L1_INFO[EMC_PHYL1_TIMING_DDRAM_LATENCY2]));
        }
        else
        {
            return (EMC_PHY_L1_TIMING_T_PTR)(&(EMC_PHY_TIMING_L1_INFO[EMC_PHYL1_TIMING_DDRAM_LATENCY3]));
        }
    }
}

PUBLIC EMC_PHY_L2_TIMING_T_PTR EMC_GetPHYL2_Timing(void)
{
    if (EMC_DLL_ON_OFF == DLL_OFF)
    {
        return (EMC_PHY_L2_TIMING_T_PTR)(&(EMC_PHY_TIMING_L2_INFO[EMC_PHYL2_TIMING_DLL_OFF]));
    }
    else
    {
        return (EMC_PHY_L2_TIMING_T_PTR)(&(EMC_PHY_TIMING_L2_INFO[EMC_PHYL2_TIMING_DLL_ON]));
    }
}


/*****************************************************************************/
//  Description:	EMC basic mode set function
//				set the base mode like:
//				EMC device endian
//				EMC auto gate en for power saving
//				EMC auto sleep en
//				EMC cmd queue mode
//  Global resource dependence:  NONE
//  Related register: EMC_CFG0
//  Author:		Johnny.Wang
//  Note:			The default cs map space is 4g, if dram capability is larger than 4g,
//				emc_cs_map parameter must adjust.
/*****************************************************************************/
LOCAL void EMC_Base_Mode_Set(SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 i = 0;
    EMC_CS_MAP_E cs_position;

    cs_position = mem_info->cs_position;

    i = REG32(EXT_MEM_CFG0);
    i &= ~0x1fff;
    i |=(EMC_DVC_ENDIAN_DEFAULT <<12) 	|
        (EMC_AUTO_GATE_EN		<<11)	|
        (EMC_AUTO_SLEEP_EN		<<10)	|
        (EMC_2DB_1CB			<<6)	|
        (EMC_CS_MODE_DEFAULT	<<3)	|
        (cs_position   <<0)	;

    REG32(EXT_MEM_CFG0) = i;
}

/*****************************************************************************/
//  Description:	EMC each cs work mode set function
//				set each cs work mode parameter like:
//				memory write burst length
//				memory read burst length
//				memory write burst mode:wrap/increase
//				memory read burst mode:wrap/increase
//				AHB write busrt divided to single/busrt access
//				AHB read busrt divided to single/busrt access
//  Global resource dependence:  memory burst length type
//  Related register: EMC_CFG0_CSx
//  Author:		Johnny.Wang
//  Note:			There are two cs pin in sc8810 emc, we usuall use cs0 to control external memory
//
/*****************************************************************************/
PUBLIC void EMC_CSx_Burst_Set(EMC_CS_NUM_E emc_cs_num, SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 i = 0;
    uint32 emc_cs_cfg = EXT_MEM_CFG0_CS0 + emc_cs_num*4;
    uint32 burst_len = 0;

    if (DDR_SDRAM == mem_info ->sdram_type)
    {
        if (DATA_WIDTH_16 == mem_info ->data_width)
        {
            burst_len = mem_info->burst_length -1;
        }
        else
        {
            burst_len = mem_info->burst_length;
        }
    }
    else if (SDR_SDRAM == mem_info ->sdram_type)
    {
        burst_len = 0;
    }
    
    i = REG32(emc_cs_cfg);
    i &= ~((0x7<<8)|(0x7<<4)|(1<<1)|1);
    i |=((burst_len			<<8) | //write burst length
         (burst_len			<<4) | //read burst length
         (HBURST_TO_BURST	<<1) | //write hburst invert to mem burst
         (HBURST_TO_BURST	<<0));  //rrite hburst invert to mem burst

    REG32(emc_cs_cfg) = i;
}

/*****************************************************************************/
//  Description:	EMC AXI channel set function
//				set each axi channel parameter like:
//				axi channel en
//				axi channel auto sleep en
//				channel endian switch
//				channel priority
//  Global resource dependence:  NONE
//  Related register: EMC_CFG0_ACHx
//				  EMC_CFG1_ACHx
//  Author:		Johnny.Wang
//  Note:			There are two axi channel in sc8810 emc, one for A5,the other for GPU
//
/*****************************************************************************/
LOCAL void EMC_AXI_CHL_Set(EMC_CHL_NUM_E emc_axi_num)
{
    uint32 i = 0;
    uint32 emc_axi_cfg0 = EXT_MEM_CFG0_CH0_BASE+ emc_axi_num*8;
    uint32 emc_axi_cfg1 = EXT_MEM_CFG1_CH0_BASE+ emc_axi_num*8;
    
    i = REG32(emc_axi_cfg0);
    i &= ~ACH_RF_ENDIAN_SWT_CHX;
    i |= ACH_RF_AUTO_SLEEP_EN_CHX | ACH_RF_CH_EN_CHX | (EMC_ENDIAN_SWITCH_NONE<<4);
    REG32(emc_axi_cfg0) = i;

    i = REG32(emc_axi_cfg1);
    i &= ~ACH_RF_SYNC_SEL_CHX; //clear bit4
    i |= (EMC_CLK_ASYNC<<4); //emc clk async with axi clk
    i |= (ACH_RF_BRESP_MODE_CH); //axi channel response mode  0:at once  1:delay several clk
    REG32(emc_axi_cfg1) = i;
}
/*****************************************************************************/
//  Description:	EMC AHB channel set function
//  Global resource dependence:  NONE
//  Author:		Johnny.Wang
//  Note:			There are 7 ahb channel in sc8810 emc, but no one is relate with ARM,
//				so don't used temporarily
//
/*****************************************************************************/
LOCAL void EMC_AHB_CHL_Set(EMC_CHL_NUM_E emc_ahb_num,uint32 addr_offset)
{
    uint32 emc_ahb_cfg0 = EXT_MEM_CFG0_CH0_BASE + emc_ahb_num*8;
    uint32 emc_ahb_cfg1 = EXT_MEM_CFG1_CH0_BASE + emc_ahb_num*8;

    REG32(emc_ahb_cfg0) |= HCH_RF_AUTO_SLEEP_EN_CHX;
    
    REG32(emc_ahb_cfg1) &= ~0x03ff0000;	//clear bit16~25
    REG32(emc_ahb_cfg1) |= (addr_offset & 0x03ff) << 16;
}

/*****************************************************************************/
//  Description:	EMC Memroy all timing parameter set function
//				set all timing parameter of EMC when operate external memory like:
//				t_rtw,
//				t_ras,
//				t_xsr,
//				t_rfc,
//				t_wr,
//				t_rcd,
//				t_rp,
//				t_rrd,
//				t_mrd,
//				t_wtr,
//				t_ref,
//  Global resource dependence:  NONE
//  Author:		Johnny.Wang
//  Related register: EMC_DCFG1
//				  EMC_DCFG2
//  Note:			None
//
/*****************************************************************************/
LOCAL void EMC_MEM_Timing_Set(uint32 emc_freq,
                        SDRAM_CFG_INFO_T_PTR     mem_info,
                        SDRAM_TIMING_PARA_T_PTR  mem_timing)
{
    uint32 cycle_ns = (uint32)(2000000000/emc_freq);//2000/(clk); //device clock is half of emc clock.
    uint32 cycle_t_ref = 1000000000/EMC_T_REF_CLK;

    uint32 row_mode    = mem_info->row_mode + ROW_LINE_MIN;
    uint32 t_rtw       = mem_info->cas_latency;

    //round all timing parameter
    uint32  t_ras	= mem_timing->ras_min/cycle_ns;
    uint32  t_xsr 	= mem_timing->xsr_min/cycle_ns;
    uint32  t_rfc 	= mem_timing->rfc_min/cycle_ns;
    uint32  t_wr  	= mem_timing->wr_min/cycle_ns+2; //note: twr should add 2 for ddr
    uint32  t_rcd 	= mem_timing->rcd_min/cycle_ns;
    uint32  t_rp  	= mem_timing->row_pre_min/cycle_ns;
    uint32  t_rrd 	= mem_timing->rrd_min/cycle_ns;
    uint32  t_mrd	= mem_timing->mrd_min;
    uint32  t_wtr 	= mem_timing->wtr_min+1;
    uint32  t_ref 	= mem_timing->row_ref_max*1000000/cycle_t_ref/(1<<row_mode) -2;

    //prevent the maximun overflow of all timing parameter
    t_ras	= (t_ras >= 0xf) ? 0x7  : t_ras;
    t_xsr 	= (t_xsr >= 0xff) ? 0x26 : t_xsr;
    t_rfc 	= (t_rfc >= 0x3f) ? 0x1a : t_rfc;
    t_wr  	= (t_wr  >= 0xf) ? 0x4  : t_wr;
    t_rcd 	= (t_rcd >= 0xf) ? 0x3  : t_rcd;
    t_rp  	= (t_rp  >= 0xf) ? 0x3  : t_rp;
    t_rrd 	= (t_rrd >= 0xf) ? 0x2  : t_rrd;
    t_mrd	= (t_mrd >= 0xf) ? 0x0  : t_mrd;
    t_wtr 	= (t_wtr >= 0xf) ? 0x3  : t_wtr;
    t_ref 	= (t_ref >=0xfff) ? 0x100: t_ref ;

    //prevent the minmun value of all timing
    t_ras	= (t_ras <= 0) ? 0x7  : t_ras;
    t_xsr 	= (t_xsr <= 0) ? 0x26 : t_xsr;
    t_rfc 	= (t_rfc <= 0) ? 0x1a : t_rfc;
    t_wr  	= (t_wr  <= 0) ? 0x4  : t_wr;
    t_rcd 	= (t_rcd <= 0) ? 0x3  : t_rcd;
    t_rp  	= (t_rp  <= 0) ? 0x3  : t_rp;
    t_rrd 	= (t_rrd <= 0) ? 0x2  : t_rrd;
    t_mrd	= (t_mrd <= 0) ? 0x2  : t_mrd;
    t_wtr 	= (t_wtr <= 0) ? 0x3  : t_wtr;
    t_ref 	= (t_ref <=00) ? 0x100: t_ref ;



    REG32(EXT_MEM_DCFG1) =
        ((1<<28)	  |//read to read turn around time between different cs,default:0     2 cs or above:1
         (t_wtr << 24) |
         (t_rtw << 20) |
         (t_ras << 16) |
         (t_rrd << 12) |
         (t_wr  << 8)  |
         (t_rcd << 4)  |
         (t_rp  << 0));

    REG32(EXT_MEM_DCFG2) =
        ((t_rfc << 24) |
         (t_xsr << 16) |
         (t_ref << 4)  |
         (t_mrd << 0));
}


/*****************************************************************************/
//  Description:	EMC software command send function
//				this function will send software initialization command to external memory
//  Global resource dependence:  memory type
//  Author:		Johnny.Wang
//  Related register:
//  Note:			None
//
/*****************************************************************************/
void EMC_SCMD_Issue(SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 i = 0;

    //shut down auto-refresh
    REG32(EXT_MEM_DCFG0) &= ~(DCFG0_AUTOREF_EN);

    //precharge all bank
    REG32(EXT_MEM_DCFG4) = 0x40010000;
    while (REG32(EXT_MEM_DCFG4) & BIT_16);
    for (i=0; i<=50; i++);

    //software auto refresh
    REG32(EXT_MEM_DCFG4) = 0x40020000;
    while (REG32(EXT_MEM_DCFG4) & BIT_17);
    for (i=0; i<=50; i++);

    //software auto refresh
    REG32(EXT_MEM_DCFG4) = 0x40020000;
    while (REG32(EXT_MEM_DCFG4) & BIT_17);
    for (i=0; i<=50; i++);

    //load nornal mode register
    REG32(EXT_MEM_DCFG4) = 0x40040000 | (mem_info->cas_latency<<4) | (mem_info->burst_length);
    while (REG32(EXT_MEM_DCFG4) & BIT_18);
    for (i=0; i<=50; i++);

    if (SDRAM_EXT_MODE_INVALID != mem_info->ext_mode_val)
    {
        //load external mode register
        REG32(EXT_MEM_DCFG4) = 0x40040000 | mem_info->ext_mode_val;
        while (REG32(EXT_MEM_DCFG4) & BIT_18);
        for (i=0; i<=50; i++);
    }

    //open auto-refresh
    REG32(EXT_MEM_DCFG0) |= (DCFG0_AUTOREF_EN);

}


/*****************************************************************************/
//  Description:	EMC phy latency set function
//				set parameter relate with cas latency like:
//				timing adjustment sample clock latency
//				DQS output latency
//				write dm latency
//				read dm latency
//				write data latency
//				read data latency
//  Global resource dependence:  dram type , cas_latency
//  Author:		Johnny.Wang
//  Related register: EMC_DCFG5
//
//  Note:			None
//
/*****************************************************************************/
PUBLIC void EMC_PHY_Latency_Set(SDRAM_CFG_INFO_T_PTR mem_info)
{
    if (SDR_SDRAM == mem_info->sdram_type)
    {

            if (CAS_LATENCY_2 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG5) = 0x00400007;
            }
            else if (CAS_LATENCY_3 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG5) = 0x00600209;
            }
        
    }
    else
    {

            if (CAS_LATENCY_2 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG5) = 0x00622728;
            }
            else if (CAS_LATENCY_3 == mem_info->cas_latency)
            {
                REG32(EXT_MEM_DCFG5) = 0x0062272A;
            }
        
    }
}
/*****************************************************************************/
//  Description:	EMC phy mode set function
//				set parameter relate with emc phy work mode like:
//				cke map to cs0 or cs1
//				dqs gate delay,delay line or loopback
//				dqs gate mode,mode0 or mode1
//				DMEM data output mode,dff or dl
//				DMEM DDR DQS[3:0] output mode,dff or dl
//				DMEM DDR DQS PAD IE mode,dff or dl
//				DMEM sample clock mode,internal or from dqs
//				DMEM CK/CK# output mode,dff or dl
//				DMEM READ strobe clock loopback dis/en
//  Global resource dependence:  dll on or off, external memory type
//  Author:		Johnny.Wang
//  Related register: EMC_CFG1
//
//  Note:			None
//
/*****************************************************************************/
LOCAL void EMC_PHY_Mode_Set(SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 i = 0;

    SCI_ASSERT(mem_info != NULL);

    i = REG32(EXT_MEM_CFG1);
    i &= ~((3<<14)|0x3ff);
    i |=(EMC_CKE_SEL_DEFAULT << 14) |
        (EMC_DQS_GATE_DEFAULT<< 8)	|
        (EMC_DQS_GATE_MODE_DEFAULT<< 7) |
        (mem_info->sdram_type<< 6) |//DMEM data output mode,0:dff 1:dl
        (0<<5) |//DMEM DDR DQS[3:0] output mode,0:dff 1:dl
        (0<<4) |//DMEM DDR DQS PAD IE mode,0:dff 1:dl
        (mem_info->sdram_type<<3) |//DMEM sample clock mode,0:internal 1:out-of-chip
        (0<<2) |//DMEM CK/CK# output mode,0:dff 1:dl
        ((~mem_info->sdram_type & 0x1) <<1) |//DMEM READ strobe clock loopback 0:dis 1:en
        mem_info->sdram_type;

    REG32(EXT_MEM_CFG1) = i;

#ifdef FPGA_TEST
    //REG32(EXT_MEM_CFG1) |= BIT_6;
#endif
}



/*****************************************************************************/
//  Description:	EMC phy timing set function
//				set parameter relate with emc phy work mode like:
//				data pad ie delay
//				data pad oe delay
//				dqs pad ie delay
//				dqs pad oe delay
//				all delay line timing parameter
//  Global resource dependence:  dll on or off, external memory type
//  Author:		Johnny.Wang
//  Related register: EMC_DCFG6,7,8 and EMC_DMEM_DL0~DL19
//  Note:			None
//
/*****************************************************************************/
PUBLIC void EMC_PHY_Timing_Set(SDRAM_CFG_INFO_T_PTR mem_info,
                        EMC_PHY_L1_TIMING_T_PTR emc_phy_l1_timing,
                        EMC_PHY_L2_TIMING_T_PTR emc_phy_l2_timing)
{
    uint32 i = 0;

    SCI_ASSERT((mem_info != NULL) && (emc_phy_l1_timing != NULL) && (emc_phy_l2_timing != NULL));

    REG32(EXT_MEM_DCFG8) = ((emc_phy_l1_timing->data_pad_ie_delay & 0xffff) <<16) |
                           (emc_phy_l1_timing->data_pad_oe_delay & 0xff);


    if (DDR_SDRAM == mem_info->sdram_type)
    {
        REG32(EXT_MEM_DCFG6) = ((emc_phy_l1_timing->dqs_gate_pst_delay& 0xffff) <<16) |
                               (emc_phy_l1_timing->dqs_gate_pre_delay& 0xffff);

        REG32(EXT_MEM_DCFG7) = ((emc_phy_l1_timing->dqs_ie_delay& 0xffff) <<16) |
                               (emc_phy_l1_timing->dqs_oe_delay& 0xff);

#if EMC_DLL_ON_OFF
        {
            REG32(EXT_MEM_CFG0_DLL) = 0x11080; //DLL and compensation en

            WAIT_EMC_DLL_LOCK;
        }
#else
        {
            REG32(EXT_MEM_CFG0_DLL) = 0x0; //DLL disable
        }
#endif

        for (i = 0; i < 20; i++)
        {
            REG32(EXT_MEM_DL0 + i*4) = REG32((unsigned int)emc_phy_l2_timing + i * 4);
        }

#if (EMC_DLL_ON_OFF == DLL_ON)
        REG32(EXT_MEM_CFG0_DLL) |= DCFG0_DLL_COMPENSATION_EN;
#endif

    }
    else
    {
        REG32(EXT_MEM_DCFG6) = 0X400020;
        REG32(EXT_MEM_DCFG7) = 0XF0000E;
        REG32(EXT_MEM_DCFG8) = 0X400001;
    }
}

void EMC_CHL_Init(EMC_CHL_NUM_E emc_axi_num)
{
    int i;

    if ((emc_axi_num >= EMC_AHB_MIN) && (emc_axi_num <= EMC_AHB_MAX))
    {
        for (i = EMC_AHB_MIN; i <= EMC_AHB_MAX; i++)
        {
            EMC_AHB_CHL_Set(i, 0);
        }
    }
    else
    {
        EMC_AXI_CHL_Set(emc_axi_num);
    }
}


#if 1
void set_emc_pad(uint32 dqs_drv,uint32 data_drv,uint32 ctl_drv, uint32 clk_drv)
{
    unsigned int i = 0;

    //ckdm
    REG32(PINMAP_REG_BASE + 0x204) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x204) |= clk_drv<<8;

    //ckdp
    REG32(PINMAP_REG_BASE + 0x208) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x208) |= clk_drv<<8;

    //addr
    for(i = 0; i < 14; i++)
    {
        REG32(PINMAP_REG_BASE + 0x20c + i*4) &= ~0x300;
        REG32(PINMAP_REG_BASE + 0x20c + i*4) |= ctl_drv<<8;
    }

    //bank0
    REG32(PINMAP_REG_BASE + 0x248) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x248) |= ctl_drv<<8;

    //bank1
    REG32(PINMAP_REG_BASE + 0x24c) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x24c) |= ctl_drv<<8;

    //casn
    REG32(PINMAP_REG_BASE + 0x250) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x250) |= ctl_drv<<8;
    //cke0
    REG32(PINMAP_REG_BASE + 0x254) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x254) |= ctl_drv<<8;

    //cke1
    REG32(PINMAP_REG_BASE + 0x258) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x258) |= ctl_drv<<8;

    //csn0
    REG32(PINMAP_REG_BASE + 0x25c) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x25c) |= ctl_drv<<8;
    //csn1
    REG32(PINMAP_REG_BASE + 0x260) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x260) |= ctl_drv<<8;

    //dqm
    REG32(PINMAP_REG_BASE + 0x264) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x264) |= data_drv<<8;
    REG32(PINMAP_REG_BASE + 0x268) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x268) |= data_drv<<8;
    REG32(PINMAP_REG_BASE + 0x26c) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x26c) |= data_drv<<8;
    REG32(PINMAP_REG_BASE + 0x270) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x270) |= data_drv<<8;

    //dqs
    REG32(PINMAP_REG_BASE + 0x274) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x274) |= dqs_drv<<8;
    REG32(PINMAP_REG_BASE + 0x278) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x278) |= dqs_drv<<8;
    REG32(PINMAP_REG_BASE + 0x27c) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x27c) |= dqs_drv<<8;
    REG32(PINMAP_REG_BASE + 0x280) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x280) |= dqs_drv<<8;

    //data
    for(i = 0; i < 32; i++)
    {
        REG32(PINMAP_REG_BASE + 0x284 + i*4) &= ~0x300;
        REG32(PINMAP_REG_BASE + 0x284 + i*4) |= data_drv<<8;
    }

    //gpre_loop
    REG32(PINMAP_REG_BASE + 0x304) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x304) |= ctl_drv<<8;

    //gpst_loop
    REG32(PINMAP_REG_BASE + 0x308) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x308) |= ctl_drv<<8;

    //rasn
    REG32(PINMAP_REG_BASE + 0x338) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x338) |= ctl_drv<<8;

    //wen
    REG32(PINMAP_REG_BASE + 0x33c) &= ~0x300;
    REG32(PINMAP_REG_BASE + 0x33c) |= ctl_drv<<8;

}



void set_sc7702_clk(void)
{

    //CLK_ARM = 230.4MHZ  CLK_AHB=57.5MHZ CLK_EMC=96MHZ
    {
        uint32 tmp_clk = 0;

        //disable DSP affect clk_emc
        REG32(DSP_BOOT_EN)  |=  BIT_2;  // ARM access DSP ASHB bridge enable
        REG32(AHB_CTL1)     &= ~BIT_16; // ARM disable matrix to sleep
        REG32(0x10130010)   |=  BIT_28; // DSP Zbus 32bit access enable
        REG32(0x1013000C)   |= (0xf<<12);// bit[15:12], DSP SIDE clk_emc_drv
        REG32(0x1013000C)   |= (3<<10); //bit[11:10], DSP SIDE clk_emc_sel: 0:384M, 1:256M, 2:230M 3:26M

        REG32(0x10130010)   &= ~BIT_28; // DSP Zbus 32bit access disable
        REG32(AHB_CTL1)     |=  BIT_16; // ARM enable matrix to sleep
        REG32(DSP_BOOT_EN)  &= ~BIT_2;  // ARM access DSP ASHB bridge enable


        REG32(AHB_ARM_CLK) |= (BIT_23|BIT_24); //set clk_mcu =26MHz

        tmp_clk |=                      // bit[31],    reserved
            (0 << 30)        |   // bit[30],    clk_mcu div2 en
            // bit[29:25], read only
            (0 << 23)        |   // bit[24:23], clk_mcu select, 0:460.8M(MPLL),1:153.6M,2:64MHZ,3:26MHZ
            // bit[22:19], reserved
            (1 << 17)        |   // bit[18:17], clk_arm_if div, clk_mcu/(n+1)
            // bit[16:14], reserved
            (0 << 12)        |   // bit[13:12], clk_emc async sel: 0:384M, 1:256M, 2:230M 3:26M
            (1 << 8)         |   // bit[11:8],  clk_emc async div: (n+1)
            (0 << 0)         |   // bit[7],     clk_mcu highest freq set 0:460.8M 1:384M
            (1 << 4)         |   // bit[6:4],   clk_ahb div clk_arm_if/(n+1)
            (0 << 3)  ;          // bit[3],     emc sync:1, async:0
        // bit[2:0],   reserved

        REG32(AHB_ARM_CLK) = tmp_clk;

        REG32(AHB_ARM_CLK) &= ~(BIT_23|BIT_24);
    }
    return;

}
#endif



LOCAL void set_cp_emc_pad(void) {
    u32 dqs_drv = 0;
    u32 data_drv = 0;
    u32 ctl_drv = 1;
    u32 clk_drv = 0;

    u32 i = 0;
    //ckdp
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0xE0), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0xE0), clk_drv<<8);

    //ckdm
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0xDC), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0xDC), clk_drv<<8);

    //addr
    for (i = 0; i<14; i++) {
        CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0xE4 + i*4), ~0x300);
        CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0xE4 + i*4), ctl_drv<<8);
    }

    //bank0
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x11c), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x11c), ctl_drv<<8);
    //bank1
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x120), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x120), ctl_drv<<8);
    //casn
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x124), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x124), ctl_drv<<8);

    //cke0
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x128), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x128), ctl_drv<<8);

    //csn0
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x12c), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x12c), ctl_drv<<8);

    //dqm
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x130), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x130), data_drv<<8);

    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x134), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x134), data_drv<<8);

    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x138), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x138), data_drv<<8);

    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x13C), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x13C), data_drv<<8);

    //dqs
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x140), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x140), dqs_drv<<8);

    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x144), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x144), dqs_drv<<8);

    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x148), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x148), dqs_drv<<8);

    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x14C), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x14C), dqs_drv<<8);

    //data
    for (i = 0; i<32; i++) {
        CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x150 + i*4), ~0x300);
        CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x150 + i*4), data_drv<<8);
    }

    //gpre_loop
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x1D0), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x1D0), ctl_drv<<8);
    //gpst_loop
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x1D4), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x1D4), ctl_drv<<8);

    //rasn
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x1D8), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x1D8), ctl_drv<<8);

    //wen
    CHIP_REG_AND((CHIPPIN_CTL_BEGIN + 0x1DC), ~0x300);
    CHIP_REG_OR((CHIPPIN_CTL_BEGIN + 0x1DC), ctl_drv<<8);

}

LOCAL void set_cp_jtag_pad(void) {
    /*CP Jtag pin config*/
    CHIP_REG_OR(0x8B000008, BIT_13);//pin eb

    CHIP_REG_SET(0x8C000588, 0x10108);
    CHIP_REG_SET(0x8C00058C, 0x10188);
    CHIP_REG_SET(0x8C000590, 0x10188);
    CHIP_REG_SET(0x8C000594, 0x10188);
    CHIP_REG_SET(0x8C000598, 0x10188);
}

#ifdef SDRAM_AUTODETECT_SUPPORT
LOCAL BOOLEAN __is_rw_ok(uint32 addr,uint32 val)
{
    volatile uint32 i;
    BOOLEAN ret = SCI_TRUE;

    REG32(addr) = 0;
    REG32(addr) 	= val;

    REG32(addr + 4) = 0;
    REG32(addr + 4) = (~val);

    delay(100);

    if ((REG32(addr) == val) && (REG32(addr + 4) == (~val)))
//    if (REG32(addr) == val)
    {
        ret = SCI_TRUE;
    }
    else
    {
        ret = SCI_FALSE;
    }

    return ret;
}


void dram_detect_write_addr(uint32 start_addr, uint32 detect_size)
{
    uint32 addr;
    uint32 detect_unit = 0x200;
    uint32 detect_region = (start_addr + detect_size);

    for(addr = start_addr; addr < detect_region; addr = start_addr + detect_unit)
    {
        *(volatile uint32 *)addr = addr;
        detect_unit <<= 1;
    }
}

uint32 dram_detect_check_addr(uint32 start_addr, uint32 detect_size)
{
    uint32 addr;
    uint32 detect_unit = 0x200;
    uint32 detect_region = (start_addr + detect_size);

    for(addr = start_addr; addr < detect_region; addr = start_addr + detect_unit)
    {
        if(*(volatile uint32 *)addr != addr)
        {
            break;
        }
        
        detect_unit <<= 1;
    }
    
    if (addr < detect_region)
    {
        return addr;
    } 

    return detect_region;
}


BOOLEAN dram_mode_check(uint32 dram_cap)
{
    BOOLEAN ret = TRUE;
    uint32 i = 0;
    uint32 detect_block_size = CAP_2G_BIT;
    uint32 start_detect_addr_len = 0;
    uint32 start_detect_addr[] = {
        0x00000000,
        0x10000000,
        0x30000000,
        INVALIDE_VAL
    };

    SCI_ASSERT(dram_cap >= CAP_1G_BIT);
    
    switch (dram_cap)
    {
        case CAP_6G_BIT:
            break;
        case CAP_4G_BIT:
            start_detect_addr[2] = INVALIDE_VAL;
            break;
        case CAP_2G_BIT:
        default:
            start_detect_addr[1] = INVALIDE_VAL;
            start_detect_addr[2] = INVALIDE_VAL;
            break;
    }

    start_detect_addr_len = sizeof(start_detect_addr) / sizeof(start_detect_addr[0]);

    for(i = 0; i < start_detect_addr_len; i++)
    {
        if (start_detect_addr[i] != INVALIDE_VAL)
        {
            dram_detect_write_addr(start_detect_addr[i], detect_block_size);
        }
    }

    for(i = 0; i < start_detect_addr_len; i++)
    {
        if (start_detect_addr[i] != INVALIDE_VAL)
        {
            if (dram_detect_check_addr(start_detect_addr[i], detect_block_size) != (start_detect_addr[i] + detect_block_size))
            {
                ret = FALSE;
                break;
            }
        }  
    }
    
    return ret;
}


BOOLEAN dram_mode_set(SDRAM_CFG_INFO_T_PTR pCfg)
{
    EMC_Base_Mode_Set(pCfg);
    EMC_AddrMode_Set(pCfg);

    return SCI_TRUE;
}

BOOLEAN dram_mode_detect(SDRAM_CFG_INFO_T_PTR pCfg)
{
    uint32 i;
    BOOLEAN ret = TRUE;
    SDRAM_MODE_PTR modetable_ptr = SDRAM_GetModeTable();

    for(i=0; modetable_ptr[i].capacity != CAP_ZERO; i++)
    {
        if(modetable_ptr[i].data_width != pCfg->data_width)
        {
            continue;
        }

        pCfg->cs_position = modetable_ptr[i].cs_position;
        pCfg->col_mode = modetable_ptr[i].col_mode;
        pCfg->row_mode = modetable_ptr[i].row_mode;
        
        dram_mode_set(pCfg);

        if(dram_mode_check(modetable_ptr[i].capacity))
        {
            DRAM_CAP = modetable_ptr[i].capacity;
            break;
        }    
    }

    if (modetable_ptr[i].capacity == CAP_ZERO)
    {
        ret = FALSE;
        //SCI_ASSERT(0);
    }

    return ret;
}

LOCAL BOOLEAN DRAM_Para_SelfAdapt(SDRAM_CFG_INFO_T_PTR pCfg)
{
    BOOLEAN ret = FALSE;

    for (;;)
    {
        EMC_SoftReset();
        EMC_Base_Mode_Set(pCfg);
        SDRAM_Type_Set(pCfg);
        EMC_PHY_Latency_Set(pCfg);
        EMC_AddrMode_Set(pCfg);
        EMC_CSx_Burst_Set(EMC_CS0, pCfg);
        EMC_CSx_Burst_Set(EMC_CS1, pCfg);
        EMC_PHY_Mode_Set(pCfg);
        EMC_SCMD_Issue(pCfg);

        if (__is_rw_ok(ZERO_ADDR, MEM_REF_DATA0))
        {
            ret = TRUE;
            break;
        }
        
        if((pCfg->data_width == DATA_WIDTH_32)
                && (pCfg->sdram_type == DDR_SDRAM)
            )
        {
            pCfg->data_width = DATA_WIDTH_16;
            
        }
        else if((pCfg->data_width == DATA_WIDTH_16)
                && (pCfg->sdram_type == DDR_SDRAM)
            )
        {
            pCfg->data_width = DATA_WIDTH_32;
            pCfg->sdram_type = SDR_SDRAM;
        }
        else if((pCfg->data_width == DATA_WIDTH_32)
                && (pCfg->sdram_type == SDR_SDRAM)
            )
        {
            pCfg->data_width = DATA_WIDTH_16;
        }
        else
        {
            return FALSE;
        }
    }

    ret = dram_mode_detect(pCfg);

    return ret;
}

#endif


void DMC_Init(uint32 clk)
{
    int i;
    uint32 emc_freq = clk;
    SDRAM_CFG_INFO_T_PTR    mem_info = SDRAM_GetCfg();
    SDRAM_TIMING_PARA_T_PTR mem_timing = SDRAM_GetTimingPara();
    EMC_PHY_L1_TIMING_T_PTR emc_phy_l1_timing = NULL;
    EMC_PHY_L2_TIMING_T_PTR emc_phy_l2_timing = EMC_GetPHYL2_Timing();

#ifdef CONFIG_SC7710G2
    set_emc_pad(2,0,2,3);
#endif
//    set_sc7702_clk();	//open only when chip test
//    emc_phy_l1_timing = EMC_GetPHYL1_Timing(mem_info->sdram_type, mem_info->cas_latency);
//    EMC_Init(EMC_CLK, EMC_AHB_ARM0, mem_info, mem_timing, emc_phy_l1_timing, emc_phy_l2_timing);
//    DRAM_CAP =  SDRAM_GetCap(mem_info); // get size
    EMC_SoftReset();

    EMC_CHL_Init(EMC_AXI_ARM);

#if 0 //def SDRAM_AUTODETECT_SUPPORT
    for (i=0; i<3; i++)
    {
        if (DRAM_Para_SelfAdapt(mem_info))
        {
            break;
        }
    }

    SCI_ASSERT(i < 3);
#else
    DRAM_CAP =  SDRAM_GetCap(mem_info); // get size

    EMC_Base_Mode_Set(mem_info);
    EMC_AddrMode_Set(mem_info);
    EMC_CSx_Burst_Set(EMC_CS0, mem_info);
    EMC_CSx_Burst_Set(EMC_CS1, mem_info);
//    SDRAM_Type_Set(mem_info);
    
    EMC_PHY_Mode_Set(mem_info);
    EMC_PHY_Latency_Set(mem_info);
    emc_phy_l1_timing = EMC_GetPHYL1_Timing(mem_info->sdram_type, mem_info->cas_latency);
    EMC_PHY_Timing_Set(mem_info, emc_phy_l1_timing, emc_phy_l2_timing);

    EMC_MEM_Timing_Set(emc_freq, mem_info, mem_timing);

    EMC_SCMD_Issue(mem_info);
    delay(100);
#endif

    

    return;
}

void set_dll_clock(void) {
    uint32 i;

    // GPU AXI 256M
    REG32(0x8b00002c) &= ~(0x3);

    // A5 AXI DIV
    REG32(0x20900238) |= (1 << 11);
    REG32(0x20900238) &= ~(1 <<12);

    //APB_GEN1_PCLK M_PLL_CTRL_WE
    REG32(0x8b000018) |= (1 << 9);

    //set MPLL to 900MHz
    i = REG32(0x8b000024);
    i &= ~ 0x7ff;
#if ARMCLK_CONFIG_EN
    i |= s_emc_config.arm_clk;
#else
    //i |= 0xFA;     //1000M
    //i |= 0xe1;     //900M
    i |= 0xC8;   //800M
#endif
    REG32(0x8b000024) = i;

    //set DPLL of EMC to 400MHz
    i = REG32(0x8b000040);
    i &= ~ 0x7ff;

#if ARMCLK_CONFIG_EN
    i |= s_emc_config.emc_clk;
#else
    //i |= 0x80;     //512M
    //i |= 0x69;   //420M
    i |= 0x64;   //400M
#endif

    REG32(0x8b000040) = i;
    REG32(0x8b000018) &= ~(1 << 9);

    // AHB_ARM_CLK SET
#if 1	// emc from DPLL
    //                    CLK_MCU_SEL | CLK_EMC_SEL
    REG32(0x20900224) = (3 << 23) | (3 << 12);  // 26MHz
    //                   CLK_AHB_DIV | CLK_EMC_DIV | CLK_DBG_DIV
    REG32(0x20900224) |= (3 << 4) | (1 << 8) | (4 << 14);

    REG32(0x20900224) = (3 << 4) | (1 << 8) | (4 << 14) | (1 << 12/*select dpll*/);

    for(i = 0; i < 50; i++);
#else   // EMC from MPLL
    REG32(0x20900224) = (3 << 23) | (3 << 12);
    REG32(0x20900224) |= (3 << 4) | (1 << 8) | (7 << 14);
    REG32(0x20900224) |= (1 << 23) | (3 << 4) | (1 << 8) | (7 << 14);
    for (i = 0; i < 1000; i++);

    REG32(0x20900224) = (3 << 4) | (1 << 8) | (7 << 14) | (0 << 12/*select mpll*/);
#endif


}


PUBLIC void Chip_Init(void) { /*lint !e765 "Chip_Init" is used by init.s entry.s*/
#ifdef CONFIG_SC7710G2
    set_dll_clock();

//    *(volatile uint32 *)0x20900224 = 0x19001130;
//    *(volatile uint32 *)0x20900224 = 0x19803000;  // clk

    *(volatile uint32 *)0x8200092C |= 0x6; // dcdc mem 1.8V
    delay(200);
    *(volatile uint32 *)0x8200092C &= (~0x1F);

    set_cp_emc_pad();
    set_cp_jtag_pad();
#endif

    DMC_Init(EMC_CLK);
}

#ifdef   __cplusplus
}
#endif




