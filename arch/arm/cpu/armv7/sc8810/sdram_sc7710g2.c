/******************************************************************************
 ** File Name:        sdram_sc7710g2.c
 ** Author:           henry.he
 ** DATE:             11/03/2013
 ** Copyright:        2013 Spreatrum, Incoporated. All Rights Reserved.
 ** Description:
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History
 **-------------------------------------------------------------------------
 ** DATE          NAME            DESCRIPTION
 ** 11/03/2013                    Create.
 ******************************************************************************/
#include <common.h>
#include <asm/arch/sci_types.h>
#include <asm/arch/arm_reg.h>
#include <asm/arch/sc_reg.h>
#include <asm/arch/sdram_sc7710g2.h>
#include <asm/arch/emc_config.h>
#include "asm/arch/chip_plf_export.h"
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/mfp.h>


#ifdef   __cplusplus
extern   "C"
{
#endif

/*----------------------------------------------------------------------------*
**                          Sub Function                                      *
**----------------------------------------------------------------------------*/



uint32 DRAM_CAP;
//uint32  SDRAM_BASE    =    	0x00000000;//(128*1024*1024/2)//0x30000000

#define MEM_REF_DATA0       0x12345678
#define SDRAM_BASE_ADDR     0x00000000UL

#define DMC_SOFT_RST BIT_11

LOCAL BOOLEAN s_emc_dll_open = FALSE;
LOCAL EMC_PARAM_T s_emc_config = {0};

LOCAL void DMC_Init(void);

/**---------------------------------------------------------------------------*
 **                     Static Function Prototypes                            *
 **---------------------------------------------------------------------------*/

LOCAL void delay(uint32 k)
{
    volatile uint32 i, j;

    for (i=0; i<k; i++)
    {
//        for (j=0; j<1000; j++);
    }

//    return k;
}

LOCAL void EMC_AddrMode_Set(SDRAM_CFG_INFO_T_PTR mem_info)
{
    uint32 reg_val = 0;

    reg_val = REG32(EXT_MEM_DCFG0);
    reg_val &= ~((DATA_WIDTH_MASK<<8) | (COL_MODE_MASK<<4) | ROW_MODE_MASK);
    reg_val |= (((mem_info->data_width & DATA_WIDTH_MASK)<< 8)
                | ((mem_info->col_mode & COL_MODE_MASK) << 4)
                | (mem_info->row_mode & ROW_MODE_MASK));

    REG32(EXT_MEM_DCFG0) = reg_val;
}


LOCAL void EMC_SoftReset(void)
{
    REG32(AHB_SOFT_RST) |= DMC_SOFT_RST;
    delay(10);
    REG32(AHB_SOFT_RST) &= (~DMC_SOFT_RST);
    delay(10);
}

#ifndef SDRAM_AUTODETECT_SUPPORT

PUBLIC uint32 SDRAM_GetCap(void)  // capability in bytes
{
    uint32 SDRAM_Cap;

    SDRAM_CHIP_FEATURE_T_PTR mem_feature = SDRAM_GetFeature();

    SDRAM_Cap = mem_feature->cap;

    return SDRAM_Cap;
}
#endif

PUBLIC EMC_PHY_L1_TIMING_T_PTR EMC_GetPHYL1_Timing(DMEM_TYPE_E mem_type, uint32 cas_latency)
{
#ifdef SDR_SDRAM_SUPPORT
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
#endif
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
    if (s_emc_dll_open)
    {
        return (EMC_PHY_L2_TIMING_T_PTR)(&(EMC_PHY_TIMING_L2_INFO[EMC_PHYL2_TIMING_DLL_ON]));
    }
    else
    {
        return (EMC_PHY_L2_TIMING_T_PTR)(&(EMC_PHY_TIMING_L2_INFO[EMC_PHYL2_TIMING_DLL_OFF]));
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
#ifdef SDR_SDRAM_SUPPORT
    else if (SDR_SDRAM == mem_info ->sdram_type)
    {
        burst_len = 0;
    }
#endif

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
LOCAL void EMC_AXI_CHL_Set(EMC_CHL_NUM_E emc_axi_num,
                                    EMC_CHL_PRI_E chl_wr_pri,
                                    EMC_CHL_PRI_E chl_rd_pri
                                    )
{
    uint32 i = 0;
    uint32 emc_axi_cfg0 = EXT_MEM_CFG0_CH0_BASE+ emc_axi_num*8;
    uint32 emc_axi_cfg1 = EXT_MEM_CFG1_CH0_BASE+ emc_axi_num*8;

    i = REG32(emc_axi_cfg0);
    i &= (~(ACH_RF_ENDIAN_SWT_CHX | ACH_CHL_PRI_WR_MASK));

//    i |= ACH_RF_AUTO_SLEEP_EN_CHX | ACH_RF_CH_EN_CHX | (EMC_ENDIAN_SWITCH_NONE<<4);
    i |= ACH_RF_CH_EN_CHX | (EMC_ENDIAN_SWITCH_NONE<<4);

    i |= (ACH_CHL_PRI_WR(chl_wr_pri));

    REG32(emc_axi_cfg0) = i;

    i = REG32(emc_axi_cfg1);
    i &= (~(ACH_RF_SYNC_SEL_CHX | ACH_CHL_PRI_RD_MASK)); // async
    i |= (EMC_CLK_ASYNC<<4); //emc clk async with axi clk
    i |= (ACH_RF_BRESP_MODE_CH); //axi channel response mode  0:at once  1:delay several clk
    i |= ACH_CHL_PRI_RD(chl_rd_pri);
    
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
LOCAL void EMC_AHB_CHL_Set(EMC_CHL_NUM_E emc_ahb_num,uint32 addr_offset, EMC_CHL_PRI_E chl_pri)
{
    uint32 emc_ahb_cfg0 = EXT_MEM_CFG0_CH0_BASE + emc_ahb_num*8;
    uint32 emc_ahb_cfg1 = EXT_MEM_CFG1_CH0_BASE + emc_ahb_num*8;

    REG32(emc_ahb_cfg1) &= (~HCH_CHL_PRI_MASK);

//    REG32(emc_ahb_cfg0) |= (HCH_RF_AUTO_SLEEP_EN_CHX | HCH_CHL_PRI(chl_pri));
    REG32(emc_ahb_cfg0) |= HCH_CHL_PRI(chl_pri);

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
#if 1 // low code size
    switch (emc_freq)
    {
        case EMC_CLK_26MHZ:
            REG32(EXT_MEM_DCFG1) = 0x13311211;
            REG32(EXT_MEM_DCFG2) = 0x02020322;
            break;
        case EMC_CLK_67MHZ:
            REG32(EXT_MEM_DCFG1) = 0x13321211;
            REG32(EXT_MEM_DCFG2) = 0x04050322;
            break;
        case EMC_CLK_133MHZ:
            REG32(EXT_MEM_DCFG1) = 0x13341322;
            REG32(EXT_MEM_DCFG2) = 0x080A0322;
            break;
        case EMC_CLK_200MHZ:
            REG32(EXT_MEM_DCFG1) = 0x13352333;
            REG32(EXT_MEM_DCFG2) = 0x0B0E0322;
            break;
        case EMC_CLK_266MHZ:
            REG32(EXT_MEM_DCFG1) = 0x13383455;
            REG32(EXT_MEM_DCFG2) = 0x10140322;
            break;
        case EMC_CLK_333MHZ:
            REG32(EXT_MEM_DCFG1) = 0x13393455;
            REG32(EXT_MEM_DCFG2) = 0x13180322;
            break;
        case EMC_CLK_370MHZ:
        case EMC_CLK_400MHZ:
        default:
            REG32(EXT_MEM_DCFG1) = 0x133A3566;
            REG32(EXT_MEM_DCFG2) = 0x161C0322;
            break;
    }
#else
    uint32 cycle_ns = (uint32)(2000000000/emc_freq);//2000/(clk); //device clock is half of emc clock.
    uint32 cycle_t_ref = 1000000/EMC_T_REF_CLK;

//    uint32 row_mode    = mem_info->row_mode + ROW_LINE_MIN;
    uint32 t_rtr  = 1;
    uint32 t_wtr    = mem_timing->wtr_min+1;
    uint32 t_rtw    = mem_info->cas_latency;
    uint32 t_ras;
    uint32 t_rrd;

    uint32 t_wr;
    uint32 t_rcd;
    uint32 t_rp;
    uint32 t_rfc;
    uint32 t_xsr;
    uint32 tREFI = mem_timing->trefi_max;
    uint32 t_ref;
    uint32 t_mrd   = mem_timing->mrd_min;

    //round all timing parameter
    t_ras = ((mem_timing->ras_min % cycle_ns) == 0) ? (mem_timing->ras_min/cycle_ns) : (mem_timing->ras_min/cycle_ns + 1);
    t_rrd = ((mem_timing->rrd_min % cycle_ns) == 0) ? (mem_timing->rrd_min/cycle_ns) : (mem_timing->rrd_min/cycle_ns + 1);
    t_rcd = ((mem_timing->rcd_min % cycle_ns) == 0) ? (mem_timing->rcd_min/cycle_ns) : (mem_timing->rcd_min/cycle_ns + 1);
    t_rp = ((mem_timing->row_pre_min % cycle_ns) == 0) ? (mem_timing->row_pre_min/cycle_ns) : (mem_timing->row_pre_min/cycle_ns + 1);
    t_rfc = ((mem_timing->rfc_min % cycle_ns) == 0) ? (mem_timing->rfc_min/cycle_ns) : (mem_timing->rfc_min/cycle_ns + 1);
    t_xsr = ((mem_timing->xsr_min % cycle_ns) == 0) ? (mem_timing->xsr_min/cycle_ns) : (mem_timing->xsr_min/cycle_ns + 1);

    
#ifdef SDR_SDRAM_SUPPORT
    if (SDR_SDRAM == mem_info->sdram_type)
    {
        t_wr = mem_timing->wr_min/cycle_ns; 
    }
    else
#endif
    {
        t_wr = mem_timing->wr_min/cycle_ns+2;//note: twr should add 2 for ddr
    }

    t_ref 	= tREFI / cycle_t_ref;

    //prevent the maximun overflow of all timing parameter
    t_ras	= (t_ras >= 0xf) ? 0xa  : t_ras;
    t_xsr 	= (t_xsr >= 0xff) ? 0x26 : t_xsr;
    t_rfc 	= (t_rfc >= 0x3f) ? 0x1a : t_rfc;
    t_wr  	= (t_wr  >= 0xf) ? 0x4  : t_wr;
    t_rcd 	= (t_rcd >= 0xf) ? 0x7  : t_rcd;
    t_rp  	= (t_rp  >= 0xf) ? 0x7  : t_rp;
    t_rrd 	= (t_rrd >= 0xf) ? 0x4  : t_rrd;
    t_mrd	= (t_mrd >= 0xf) ? 0x3  : t_mrd;
    t_wtr 	= (t_wtr >= 0xf) ? 0x3  : t_wtr;
    t_ref 	= (t_ref >=0xfff) ? 0x32: t_ref ;

    //prevent the minmun value of all timing
    t_ras	= (t_ras == 0) ? 0xa  : t_ras;
    t_xsr 	= (t_xsr == 0) ? 0x26 : t_xsr;
    t_rfc 	= (t_rfc == 0) ? 0x1a : t_rfc;
    t_wr  	= (t_wr  == 0) ? 0x4  : t_wr;
    t_rcd 	= (t_rcd == 0) ? 0x7  : t_rcd;
    t_rp  	= (t_rp  == 0) ? 0x7  : t_rp;
    t_rrd 	= (t_rrd == 0) ? 0x4  : t_rrd;
    t_mrd	= (t_mrd == 0) ? 0x3  : t_mrd;
    t_wtr 	= (t_wtr == 0) ? 0x3  : t_wtr;
    t_ref 	= (t_ref == 0) ? 0x32: t_ref ;

    REG32(EXT_MEM_DCFG1) =
        ((t_rtr << 28)	  |//read to read turn around time between different cs,default:0     2 cs or above:1
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
#endif
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
LOCAL void EMC_SCMD_Issue(SDRAM_CFG_INFO_T_PTR mem_info)
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
        mem_info->ext_mode_val &= (~(0x7 << 5));
        mem_info->ext_mode_val |= (s_emc_config.ddr_drv << 5);
        
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
#ifdef SDR_SDRAM_SUPPORT
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
#endif
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

//    SCI_ASSERT(mem_info != NULL);

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

    uint8 emc_dll_rd_val;
//    uint32 clkwr_dll = (64*s_emc_config.clk_wr)/(s_emc_config.read_value/2);
    uint8 clkwr_dll;

//    SCI_ASSERT((mem_info != NULL) && (emc_phy_l1_timing != NULL) && (emc_phy_l2_timing != NULL));

    REG32(EXT_MEM_DCFG8) = ((emc_phy_l1_timing->data_pad_ie_delay & 0xffff) <<16) |
                           (emc_phy_l1_timing->data_pad_oe_delay & 0xff);


    if (DDR_SDRAM == mem_info->sdram_type)
    {
        REG32(EXT_MEM_DCFG6) = ((emc_phy_l1_timing->dqs_gate_pst_delay& 0xffff) <<16) |
                               (emc_phy_l1_timing->dqs_gate_pre_delay& 0xffff);

        REG32(EXT_MEM_DCFG7) = ((emc_phy_l1_timing->dqs_ie_delay& 0xffff) <<16) |
                               (emc_phy_l1_timing->dqs_oe_delay& 0xff);

        if (s_emc_dll_open)
        {
            REG32(EXT_MEM_CFG0_DLL) = 0x11080; //DLL and compensation en

            WAIT_EMC_DLL_LOCK;
        }
        else
        {
            REG32(EXT_MEM_CFG0_DLL) &= ~BIT_10; //DLL disable
			for(i=0;i<20;i++);
			REG32(EXT_MEM_CFG0_DLL) &= ~BIT_7; //DLL disable
			for(i=0;i<20;i++);
			REG32(EXT_MEM_CFG0_DLL) = 0; //DLL disable
        }

        for (i = 0; i < 20; i++)
        {
            REG32(EXT_MEM_DL0 + i*4) = REG32((unsigned int)emc_phy_l2_timing + i * 4);
        }

        if (s_emc_dll_open)
        {
            emc_dll_rd_val = (REG32(EXT_MEM_CFG0_DLL_STS) & 0xFF);
            
            clkwr_dll = (s_emc_config.clk_wr << 6) / ((emc_dll_rd_val & 0xff) >> 1);
            
            for (i = 0; i < 4; i++)
            {
                REG32(EXT_MEM_DL0 + i*4) = (0x8000 | (clkwr_dll & 0x7F));
            }

            REG32(EXT_MEM_CFG0_DLL) |= DCFG0_DLL_COMPENSATION_EN;
        }        

    }
    else
    {
        REG32(EXT_MEM_DCFG6) = 0X400020;
        REG32(EXT_MEM_DCFG7) = 0XF0000E;
        REG32(EXT_MEM_DCFG8) = 0X400001;
    }
}


LOCAL void EMC_CHL_Init(EMC_CHL_INFO_PTR emc_chl_info)
{
    int i = 0;
    EMC_CHL_NUM_E emc_channel_num;

    while (emc_chl_info[i].emc_chl_num != EMC_CHL_MAX)
    {
        emc_channel_num = emc_chl_info[i].emc_chl_num;

        if ((emc_channel_num >= EMC_AXI_MIN) && (emc_channel_num <= EMC_AXI_MAX))
        {
            EMC_AXI_CHL_Set(emc_channel_num,
                        emc_chl_info[i].axi_chl_wr_pri,
                        emc_chl_info[i].axi_chl_rd_pri
                        );
        }
        else if ((emc_channel_num >= EMC_AHB_MIN) && (emc_channel_num <= EMC_AHB_MAX))
        {
            EMC_AHB_CHL_Set(emc_channel_num, 0, emc_chl_info[i].ahb_chl_pri);
        }
        else
        {
            SCI_ASSERT(0);
        }

        i++;
    }
}


LOCAL void set_emc_pad(uint32 dqs_drv,uint32 data_drv,uint32 ctl_drv, uint32 clk_drv)
{
    unsigned int i = 0;

//    SCI_ASSERT(dqs_drv < 4);
//    SCI_ASSERT(data_drv < 4);
//    SCI_ASSERT(ctl_drv < 4);
//    SCI_ASSERT(clk_drv < 4);

    dqs_drv &= 0x3;
    data_drv &= 0x3;
    ctl_drv &= 0x3;
    clk_drv &= 0x3;

    for(i = 0; i < 2; i++)
    {// ckdp ckdm
        REG32((CHIPPIN_CTL_BEGIN + PIN_CLKDMMEM_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_CLKDMMEM_REG_OFFS) + i*4) |= ((clk_drv<<8) | 0x4);
    }

    //addr
    for(i = 0; i < 14; i++)
    {
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMA0_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMA0_REG_OFFS) + i*4) |= ((ctl_drv<<8) | 0x4);
    }

    for(i = 0; i < 7; i++)
    {//bank0 bank1 casn cke0 cke1 csn0 csn1
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMBA0_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMBA0_REG_OFFS) + i*4) |= ((ctl_drv<<8) | 0x4);
    }

    for(i = 0; i < 2; i++)
    {//cke0 cke1
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMCKE0_REG_OFFS) + i*4) |= 0x5;
    }

    for(i = 0; i < 4; i++)
    {//dqm
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMDQM0_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMDQM0_REG_OFFS) + i*4) |= ((data_drv<<8) | 0x4);
    }

    for(i = 0; i < 4; i++)
    {//dqs
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMDQS0_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMDQS0_REG_OFFS) + i*4) |= ((dqs_drv<<8) | 0x4);
    }

    //data
    for(i = 0; i < 32; i++)
    {
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMD0_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMD0_REG_OFFS) + i*4) |= ((data_drv<<8) | 0x4);
    }

    for(i = 0; i < 2; i++)
    {//gpre_loop gpst_loop
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMGPRE_LOOP_REG_OFFS) + i*4) &= (~0x300F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMGPRE_LOOP_REG_OFFS) + i*4) |= ((ctl_drv<<8) | 0x4);
    }

    for(i = 0; i < 2; i++)
    {//rasn wen
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMRAS_N_REG_OFFS) + i*4) &= (~0x30F);
        REG32((CHIPPIN_CTL_BEGIN + PIN_EMRAS_N_REG_OFFS) + i*4) |= ((ctl_drv<<8) | 0x4);
    }
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
    {
        ret = SCI_TRUE;
    }
    else
    {
        ret = SCI_FALSE;
    }

    return ret;
}


LOCAL void dram_detect_write_addr(uint32 start_addr, uint32 detect_size)
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

LOCAL uint32 dram_detect_check_addr(uint32 start_addr, uint32 detect_size)
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


LOCAL BOOLEAN dram_mode_check(uint32 dram_cap)
{
    BOOLEAN ret = TRUE;
    uint32 i = 0;
    uint32 detect_block_size = CAP_2G_BIT;
    uint32 start_detect_addr_len = 0;
    uint32 start_detect_addr[] = {
        SDRAM_BASE_ADDR,
        0x10000000,
        0x30000000,
        INVALIDE_VAL
    };

//    SCI_ASSERT(dram_cap >= CAP_1G_BIT);
    
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

LOCAL BOOLEAN DRAM_Para_SelfAdapt(void)
{
    int i;
//    BOOLEAN ret = FALSE;
    SDRAM_CFG_INFO_T_PTR pCfg = SDRAM_GetCfg();
    SDRAM_MODE_PTR modetable_ptr = SDRAM_GetModeTable();

    for (;;)
    {
        DMC_Init();
        
        if (__is_rw_ok(SDRAM_BASE_ADDR, MEM_REF_DATA0))
        {
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

    for(i=0; modetable_ptr[i].capacity != CAP_ZERO; i++)
    {
        if(modetable_ptr[i].data_width != pCfg->data_width)
        {
            continue;
        }

        pCfg->cs_position = modetable_ptr[i].cs_position;
        pCfg->col_mode = modetable_ptr[i].col_mode;
        pCfg->row_mode = modetable_ptr[i].row_mode;

        DMC_Init();

        if(dram_mode_check(modetable_ptr[i].capacity))
        {
            DRAM_CAP = modetable_ptr[i].capacity;
            break;
        }
    }

    if (modetable_ptr[i].capacity == CAP_ZERO)
    {
        return FALSE;
        //SCI_ASSERT(0);
    }

    return TRUE;
}

#endif


LOCAL void DMC_Init(void)
{
    uint32 emc_freq = s_emc_config.emc_clk;
    SDRAM_CFG_INFO_T_PTR    mem_info = SDRAM_GetCfg();
    SDRAM_TIMING_PARA_T_PTR mem_timing = SDRAM_GetTimingPara();
    EMC_PHY_L1_TIMING_T_PTR emc_phy_l1_timing = NULL;
    EMC_PHY_L2_TIMING_T_PTR emc_phy_l2_timing = EMC_GetPHYL2_Timing();
    EMC_CHL_INFO_PTR emc_chl_info = EMC_GetChlInfo();

    EMC_SoftReset();
    EMC_CHL_Init(emc_chl_info);
        
    EMC_Base_Mode_Set(mem_info);
    EMC_AddrMode_Set(mem_info);
    EMC_CSx_Burst_Set(EMC_CS0, mem_info);
    EMC_CSx_Burst_Set(EMC_CS1, mem_info);
    EMC_PHY_Mode_Set(mem_info);
    EMC_PHY_Latency_Set(mem_info);

    emc_phy_l1_timing = EMC_GetPHYL1_Timing(mem_info->sdram_type, mem_info->cas_latency);
    EMC_PHY_Timing_Set(mem_info, emc_phy_l1_timing, emc_phy_l2_timing);
    EMC_MEM_Timing_Set(emc_freq, mem_info, mem_timing);
    EMC_SCMD_Issue(mem_info);

    return;
}

LOCAL void emc_clock_select(EMC_CLK_SOURCE_E emc_clk_sel)
{
    uint32 i;
    
//	SCI_ASSERT(emc_clk_sel < EMC_CLK_NONE);
	
    i = REG32(AHB_ARM_CLK);
    i &= ~(1 << 3); // clk_emc_async
    i &= ~(3 << 12);
    i |= ((emc_clk_sel & 0x3) << 12);
	
	REG32(AHB_ARM_CLK) = i;
}

LOCAL void set_emc_clock_div(uint32 emc_div)
{
    uint32 i;

    i = REG32(AHB_ARM_CLK);
    i &= ~(0xF << 8);
    i |= ((emc_div & 0xF) << 8); // emc div = n + 1

    REG32(AHB_ARM_CLK) = i;
}

LOCAL void set_dpll_clock_freq(uint32 clk_freq_hz)
{
    uint32 i;

    uint32 dpll_clk;

    dpll_clk = (clk_freq_hz / 1000000 / 4);

    //APB_GEN1_PCLK M_PLL_CTRL_WE
    REG32(GR_GEN1) |= (1 << 9);

    i = REG32(GR_DPLL_CTL); //0x8b000040
    i &= ~ 0x7FF;
    i |= (dpll_clk & 0x7FF);

    REG32(GR_DPLL_CTL) = i;
    REG32(GR_GEN1) &= ~(1 << 9);
}


LOCAL void set_emc_clock_freq(void)
{
    uint32 emc_div = 0;
    uint32 emc_clk_freq = s_emc_config.emc_clk;
    EMC_CLK_SOURCE_E emc_clk_sel;
    
    switch (emc_clk_freq)
    {
        case EMC_CLK_26MHZ:
            emc_clk_sel = EMC_CLK_XTL_SOURCE;
            break;
        case EMC_CLK_67MHZ:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            break;
        case EMC_CLK_133MHZ:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            break;
        case EMC_CLK_266MHZ:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            break;
        case EMC_CLK_333MHZ:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            break;
        case EMC_CLK_370MHZ:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            break;
        case EMC_CLK_200MHZ:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            break;
        case EMC_CLK_400MHZ:
        default:
            emc_clk_sel = EMC_CLK_DPLL_SOURCE;
            emc_clk_freq = EMC_CLK_400MHZ;
            break;
    }

    if (emc_clk_sel == EMC_CLK_DPLL_SOURCE)
    {
        set_dpll_clock_freq(emc_clk_freq);
        set_emc_clock_div(emc_div);
    }

	emc_clock_select(emc_clk_sel);
	
    if (emc_clk_freq <= EMC_CLK_67MHZ)
    {
        s_emc_dll_open = FALSE;
    }
    else
    {
        s_emc_dll_open = TRUE;
    }

}

void sdram_init(void)
{
    uint32 i;

    EMC_PARAM_PTR emc_ptr = EMC_GetPara();

//    s_emc_config.arm_clk = emc_ptr->arm_clk;
    s_emc_config.emc_clk = emc_ptr->emc_clk;
    s_emc_config.dqs_drv = emc_ptr->dqs_drv;
    s_emc_config.dat_drv = emc_ptr->dat_drv;
    s_emc_config.ctl_drv = emc_ptr->ctl_drv;
    s_emc_config.clk_drv = emc_ptr->clk_drv;
    s_emc_config.clk_wr  = emc_ptr->clk_wr;
    s_emc_config.ddr_drv = emc_ptr->ddr_drv;

    set_emc_pad(s_emc_config.dqs_drv, s_emc_config.dat_drv, s_emc_config.ctl_drv, s_emc_config.clk_drv);

    set_emc_clock_freq();

    delay(200000);

#ifdef SDRAM_AUTODETECT_SUPPORT
    for (i=0; i<3; i++)
    {
        if (DRAM_Para_SelfAdapt())
        {
            break;
        }
    }

    SCI_ASSERT(i < 3);
#else
    DMC_Init();
    DRAM_CAP =  SDRAM_GetCap(); // get size
#endif

}



#ifdef   __cplusplus
}
#endif




