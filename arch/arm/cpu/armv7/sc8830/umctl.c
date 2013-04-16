/******************************************************************************
 ** File Name:      umctl2.c                                                  *
 ** Author:         changde                                                   *
 ** DATE:           01/06/2013                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    Refer to uMCTL2 databook for detail                       *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 01/06/2013     changde.li       Create.                                   *
 ******************************************************************************/
#include "asm/arch/dram_phy.h"
#include "asm/arch/umctl2_reg.h"


/**---------------------------------------------------------------------------*
 **                            Macro Define
 **---------------------------------------------------------------------------*/
#define REG32(x)                           (*((volatile uint32 *)(x)))
#define UMCTL2_REG_GET(reg_addr)             (*((volatile uint32 *)(reg_addr)))
#define UMCTL2_REG_SET( reg_addr, value )    *(volatile uint32 *)(reg_addr) = value

#define DELAY_CLK Delay

#define OPERATION_MODE_INIT 0x01
#define OPERATION_MODE_NORMAL 0x02
#define OPERATION_MODE_PD 0x03
#define OPERATION_MODE_SR 0x04
#define OPERATION_MODE_DPD 0x05


/**---------------------------------------------------------------------------*
 **                            Extern Declare
 **---------------------------------------------------------------------------*/
extern MEM_IODS_E       MEM_IO_DS;
extern DRAM_BURSTTYPE_E MEM_BURST_TYPE;
extern uint32 SDRAM_BASE;
extern uint32 DQS_PDU_RES;	//dqs pull up and pull down resist
extern uint32 DQS_GATE_EARLY_LATE; 
    
extern uint32 UMCTL2_LPDDR1_MEM_DS;
extern uint32 UMCTL2_LPDDR2_MEM_DS;
    
extern uint32 B0_SDLL_PHS_DLY;
extern uint32 B1_SDLL_PHS_DLY;
extern uint32 B2_SDLL_PHS_DLY;
extern uint32 B3_SDLL_PHS_DLY;
    
extern uint32 B0_DQS_STEP_DLY;
extern uint32 B1_DQS_STEP_DLY;
extern uint32 B2_DQS_STEP_DLY;
extern uint32 B3_DQS_STEP_DLY;


/**---------------------------------------------------------------------------*
 **                            Local Variables
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Local Functions
 **---------------------------------------------------------------------------*/

static uint32 reg_bits_set(uint32 addr, 
                           uint8 start_bitpos,
                           uint8 bit_num,
                           uint32 value)
{
    /*create bit mask according to input param*/
    uint32 bit_mask = (1<<bit_num)-1;
    uint32 reg_data = *((volatile uint32*)(addr));
    
    reg_data &= ~(bit_mask<<start_bitpos);
    reg_data |= ((value&bit_mask)<<start_bitpos);
    
    *((volatile uint32*)(addr)) = reg_data;
}

static uint32 ns_to_xclock(uint32 time_ns,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/10000000;
    uint32 clk_num = (clk*time_ns)/1000 + 1;
    return (clk_num);
}

static uint32 us_to_xclock(uint32 time_us,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/10000000;
    uint32 clk_num = (clk*time_us);
    return (clk_num+1);
}

static uint32 ns_to_x1024(uint32 time_ns,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/10000000;
    uint32 clk_num = (clk*time_ns)/1000;
    return ((clk_num>>10) + 1);
}

static uint32 us_to_x1024(uint32 time_us,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/10000000;
    uint32 clk_num = (clk*time_us);
    return ((clk_num>>10) + 1);
}

static uint32 ms_to_x1024(uint32 time_ms,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/1000000;
    uint32 clk_num = (clk*time_ms)*1000;
    return ((clk_num>>10) + 1);
}

static uint32 ns_to_x32(uint32 time_ns,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/1000000;
    uint32 clk_num = (clk*time_ns)/1000;
    return ((clk_num>>5) + 1);
}
static uint32 us_to_x32(uint32 time_us,CLK_TYPE_E clk) 
{
    //uint32 clk_mhz = clk/10000000;
    uint32 clk_num = (clk*time_us);
    return ((clk_num>>5) + 1);
}


/*
 *Read/write mode register in ddr memory.
 *Refer to uMCTL2 databook Chapter2.18 "Mode Register Reads and Writes"
*/
static uint32 mr_rw(uint32 dram_type, uint32 mr_ranks, uint32 rw_flag,
                                     uint32 mr_addr, uint32 mr_data) {
#if 0
//changde
    uint32 mrctrl0=0, mrctrl1=0;

    /* checking that there is no outstanding MR tansacton.MCTL_MRSTAT.[mr_wr_busy] */    
    while(UMCTL2_REG_GET(UMCTL_MRSTAT)&BIT_0);

    mrctrl0 = (mr_addr << 12) |
              ((((mr_ranks==MR_RANKS_0ONLY)?0x01:0x00) | 
                ((mr_ranks==MR_RANKS_1ONLY)?0x02:0x00) |
                ((mr_ranks==MR_RANKS_0AND2)?0x05:0x00) | 
                ((mr_ranks==MR_RANKS_1AND3)?0x0A:0x00) |
                ((mr_ranks==MR_RANKS_ALL)?0x0F:0x00)) << 4) |
              (((rw_flag==MR_READ)?1:0)<<0); /* Only used for LPDDR2/LPDDR3 */
    /* mr_addr:Don't care for LPDDR2/LPDDR3 */
    mrctrl0 &= ~(((IS_LPDDR2(dram_type)||IS_LPDDR3(dram_type))?0x07:0x00)<<12);
    UMCTL2_REG_SET(UMCTL_MRCTRL0, mrctrl0);

    mrctrl1 = ((IS_LPDDR2(dram_type)||IS_LPDDR3(dram_type))?mr_addr:0x00) << 8) |
              (mr_data << 0);
    UMCTL2_REG_SET(UMCTL_MRCTRL1, mrctrl1);

    /* tirger the MR transaction in BIT_31 MRCTRL0.[mr_wr] */
    reg_bits_set(UMCTL_MRCTRL0, 31, 1, 0x01);
    /* checking that there is no outstanding MR tansacton.MCTL_MRSTAT.[mr_wr_busy] */    
    while(UMCTL2_REG_GET(UMCTL_MRSTAT)&BIT_0);
#endif
}
void umctl2_ctl_en(BOOLEAN is_en)
{
    uint32 reg_value=0,i;

    if(is_en) 
    {
        // Assert soft reset
        reg_value =   UMCTL2_REG_GET(0x402b00c8);
        reg_value |=  0x07;
        UMCTL2_REG_SET(0x402b00c8, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
    else 
    {
        reg_value =   UMCTL2_REG_GET(0x402b00c8);
        reg_value &=  ~0x07;
        UMCTL2_REG_SET(0x402b00c8, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
}
void umctl2_soft_reset(BOOLEAN is_en) {
#if 1
	  uint32 reg_value=0, i=0;
    /*soft reset for uMCTL2 in user interface.*/
    if(is_en) 
    {
        // Assert soft reset
        reg_value =   UMCTL2_REG_GET(0x402b00c8);
        //reg_value |=  (0x700 << 8);
		reg_value &=  ~(0x1800);
        UMCTL2_REG_SET(0x402b00c8, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
    else 
    {
        reg_value =   UMCTL2_REG_GET(0x402b00c8);
        reg_value &=  ~(0x700);
        UMCTL2_REG_SET(0x402b00c8, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
#else
	  uint32 reg_value=0, i=0;
    /*soft reset for uMCTL2 in user interface.*/
    if(is_en) {
    // Assert soft reset
    reg_value =   UMCTL2_REG_GET(0x022b00c8);
    reg_value &=  ~(0x07 << 8);
    reg_value |=  0x07 << 8;
    UMCTL2_REG_SET(0x022b00c8, reg_value);
    for(i = 0; i <= 1000; i++);
    } else {
    reg_value =   UMCTL2_REG_GET(0x022b00c8);
    reg_value &=  ~(0x07 << 8);
    UMCTL2_REG_SET(0x022b00c8, reg_value);
    for(i = 0; i <= 1000; i++);
    } 
#endif  
}

//tempurature derate configuration
BOOLEAN umctl2_tderate_init(DRAM_INFO* dram,CLK_TYPE_E clk) 
{
	/*
    if(!IS_LPDDR2(dram->dram_type))
    {
        return FALSE;
    }
    else
    {   
        if(clk > CLK_533MHZ)
        {
            UMCTL2_REG_SET(UMCTL_DERATEEN, 3);
        }
        else
        {
            UMCTL2_REG_SET(UMCTL_DERATEEN, 1);            
        }

        //perform MRR from MR4 each 1 second ??? to be confirm
        UMCTL2_REG_SET(UMCTL_DERATEINT, clk);
    }
	*/
	return;
}

BOOLEAN umctl2_low_power_init()
{
    //PWRTMG
    //???
    //PWRCTL
    reg_bits_set(UMCTL_PWRCTL,0, 1, FALSE);//auto self-refresh
    reg_bits_set(UMCTL_PWRCTL,1, 1, FALSE);//auto power down
    reg_bits_set(UMCTL_PWRCTL,2, 1, FALSE);//auto deep power down
    reg_bits_set(UMCTL_PWRCTL,3, 1, FALSE); //en_dfi_dram_clk_disable
}

void umctl2_basic_mode_init(DRAM_INFO* dram) 
{
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 BL = dram->bl;
    uint8 CL = dram->rl;
    uint8 RL = dram->rl;
    uint8 WL = dram->wl;
    uint8 ranks=dram->cs_num;
    uint8 width=dram->io_width;

    /* master register config */
    //to set rank/cs number
    reg_bits_set(UMCTL_MSTR, 24, 4, ((ranks==1)?0x01:0)|
                                    ((ranks==2)?0x03:0)|
                                    ((ranks==3)?0x07:0)|
                                    ((ranks==4)?0x0F:0));
    //to set burst length
    reg_bits_set(UMCTL_MSTR, 16, 4, ((BL==2)?0x01:0)|
                                    ((BL==4)?0x02:0)|
                                    ((BL==8)?0x04:0)|
                                    ((BL==16)?0x08:0));
                                    
    /*data_bus_width:2'b00--Full DQ buswidth.2'b01--Half DQ buswidth.
     *               2'b10--Quater DQ buswidth.2'b11--Reserved
     */
    reg_bits_set(UMCTL_MSTR, 12, 2, ((width==32)?0x00:0x00)|
                                    ((width==16)?0x01:0x00)|
                                    ((width== 8)?0x02:0x00));
                                    
    /*disable en_2t_timing_mode,use 1T timing as default.*/
    reg_bits_set(UMCTL_MSTR, 10, 1, 0x00);
    
    /*burst_mode, 0--Sequential burst mode;1--Interleaved burst mode.*/
    reg_bits_set(UMCTL_MSTR,  8, 1, ((MEM_BURST_TYPE==DRAM_BT_SEQ)?0x00:0x00)|
                                    ((MEM_BURST_TYPE==DRAM_BT_INTER)?0x01:0x00));
                                    
    /*SDRAM selection for ddr2/ddr3/lpddr/lpddr2/lpddr3*/
    reg_bits_set(UMCTL_MSTR,  0, 4, (IS_DDR2(dram_type)?0x00:0x00) |
                                    (IS_DDR3(dram_type)?0x01:0x00) |
                                    (IS_LPDDR1(dram_type)?0x02:0x00) |
                                    (IS_LPDDR2(dram_type)?0x04:0x00)|
                                    (IS_LPDDR3(dram_type)?0x08:0x00));

    UMCTL2_REG_SET(UMCTL_SCHED,0x00070C01);

    /*Only present for multi-rank configurations.
    *[11:8]:rank_wr_gap,clks of gap in data responses when performing consecutive writes to different ranks.
    *[07:4]:rank_rd_gap,clks of gap in data responses when performing consecutive reads to different ranks.
    *[03:0]:max_rank_rd,This param represents the max number of 64B reads(or 32B in some short read cases)
    *       that can bescheduled consecutively to the same rank.
    */

    // to be confirm by johnnywang
    reg_bits_set(UMCTL_RANKCTL,  0, 32,(0x06<<8)|
                                   (0x06<<4)|
                                   (0x03<<0));
                                    
}

void umctl2_odt_init(DRAM_INFO* dram) 
{
    uint8 bl = dram->bl;
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 cl = dram->rl;
    uint8 cwl = dram->wl;
    
    //to be confirm by johnnywang
    reg_bits_set(UMCTL_ODTCFG,24,4,(bl==8)?4:2);    //wr_odt_hold
    reg_bits_set(UMCTL_ODTCFG,16,4,0);              //wr_odt_hold
    reg_bits_set(UMCTL_ODTCFG, 8,4,(bl==8)?4:2);    //rd_odt_hold
    reg_bits_set(UMCTL_ODTCFG, 2,4,IS_DDR3(dram_type)?(cl-cwl):0); //rd_odt_delay    
    reg_bits_set(UMCTL_ODTCFG, 0,2,1);              //wr_odt_block
    UMCTL2_REG_SET(UMCTL_ODTMAP,IS_DDR3(dram_type)?0x312:0);
}

void umctl2_zqctl_init(DRAM_INFO* dram)
{

    lpddr1_timing_t*  lpddr1_timing = dram->ac_timing;
    lpddr2_timing_t* lpddr2_timing =dram->ac_timing;
    DDR2_ACTIMING*   ddr2_timing = dram->ac_timing;
    ddr3_timing_t*   ddr3_timing = dram->ac_timing;
    DRAM_TYPE_E dram_type = dram->dram_type;
    
    //to be confirm by johnnywang
    //zqctl0
    reg_bits_set(UMCTL_ZQCTL0, 31, 1, 1); //disable auto zq 
    reg_bits_set(UMCTL_ZQCTL0, 30, 1, 1); //disable srx zqcl
    //reg_bits_set(UMCTL_ZQCTL0, 16, 10,IS_DDR3(dram_type)? ddr3_timing->tZQoper:lpddr2_timing->tZQCL; //t_zq_long_nop
    //reg_bits_set(UMCTL_ZQCTL0,  0, 10,IS_DDR3(dram_type)? ddr3_timing->tZQCS  :lpddr2_timing->tZQCS; //t_zq_long_nop    

    //zqctl1
    //reg_bits_set(UMCTL_ZQCTL1, 20, 10,IS_LPDDR2(dram_type)? lpddr2_timing->tZQreset:0); //t_zq_reset_nop    
    //reg_bits_set(UMCTL_ZQCTL1,  0, 20,IS_LPDDR2(dram_type)? lpddr2_timing->tZQCS:0);   
}

void umctl2_dfi_init(DRAM_INFO* dram)
{
    uint8 rl = dram->rl;
    uint8 wl = dram->wl;
    uint8 cs_num = dram->cs_num;
    DRAM_TYPE_E dram_type = dram->dram_type;
    
    //to be confirm by johnnywang
    //DFITMG0
    reg_bits_set(UMCTL_DFITMG0, 24, 5, (cs_num==1)?2:3); //dfi_t_ctrl_delay, SDR and HDR mode =2,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 23, 1, 0); //dfi_rddata_use_sdr, 0:in terms of HDR cycles ??? I think should be 1!!!
                                           //                    1:in terms of SDR cycles
    reg_bits_set(UMCTL_DFITMG0, 16, 5, rl-1);//dfi_t_ctrl_delay, rl-1,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 8, 5, 1);  //dfi_tphy_wrdata, SDR and HDR mode =1,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 0, 5, wl); //dfi_tphy_wrlat, wl,refer PUBL spec p143

    //DFITMG1
    reg_bits_set(UMCTL_DFITMG1, 16, 5, wl); //dfi_t_wrdata_delay, tphy_wrlat + (delay of DFI write data to the DRAM)???
    reg_bits_set(UMCTL_DFITMG1,  8, 4, 2);  //dfi_t_dram_clk_disable,number of DFI clock cycles from the assertion of the 
                                            //                      dfi_dram_clk_disable signal on the DFI until the clock 
                                            //                      to the DRAM memory devices
    reg_bits_set(UMCTL_DFITMG1,  0, 4, 2);  //dfi_t_dram_clk_enable,number of DFI clock cycles from the de-assertion of the
                                            //                      dfi_dram_clk_disable signal on the DFI until the first 
                                            //                      valid rising edge of the clock to the DRAM memory devices

    //DFITMG1
    reg_bits_set(UMCTL_DFILPCFG0,24,4,7);//dfi_tlp_resp
    reg_bits_set(UMCTL_DFILPCFG0,20,4,8);//dfi_lp_wakeup_dpd
    reg_bits_set(UMCTL_DFILPCFG0,16,1,IS_DDR3(dram_type)?0:1);//dfi_lp_en_dpd
    reg_bits_set(UMCTL_DFILPCFG0,12,4,3);//dfi_lp_wakeup_sr
    reg_bits_set(UMCTL_DFILPCFG0, 8,1,1);//dfi_lp_en_sr
    reg_bits_set(UMCTL_DFILPCFG0, 4,4,2);//dfi_lp_wakeup_pd
    reg_bits_set(UMCTL_DFILPCFG0, 0,1,0);//dfi_lp_en_pd

    //DFIUPD0
    UMCTL2_REG_SET(UMCTL_DFIUPD0, 0X00400003);
    UMCTL2_REG_SET(UMCTL_DFIUPD1, 0X001a0021);
    UMCTL2_REG_SET(UMCTL_DFIUPD2, 0X026904c9);
    UMCTL2_REG_SET(UMCTL_DFIUPD3, 0X030e051f);    
    //UMCTL2_REG_SET(UMCTL_DFIMISC, 0x00000001); //set dfi_init_complete_en=1,the the umctl state will change to NORMAL from INIT
}

void umctl2_trainctl_init()
{
    return;
    //to be confirm by johnnywang???
    //TRAINCTL0
    //TRAINCTL1
    //TRAINCTL2
}

void umctl2_addrmap_init(DRAM_INFO* dram)
{
    //to be confirm by johnnywang,refer to dolphin xiaohui doc,by I have some thing don't understand
    switch(dram->dram_type)
    {
        case DRAM_LPDDR2_2CS_2G_X32 :
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0X00001F12);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0X00060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0X00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0X0F0F0000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0X00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0X05050505);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0X0F0F0F05);
            break;
        }
        case DRAM_LPDDR2_2CS_4G_X32:
        case DRAM_LPDDR2_2CS_3G_X32:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0X00001F13);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0X00060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0X00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0X0F0F0000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0X00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0X05050505);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0X0F0F0F05);
            break;
        }
        case DRAM_LPDDR2_1CS_4G_X32:
        case DRAM_LPDDR2_2CS_8G_X32:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F14);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0606);
            break;
        }
        case DRAM_LPDDR2_2CS_6G_X32:
        case DRAM_LPDDR2_2CS_5G_X32:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F14);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F110000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x05050505);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0505);
            break;
        }
        case DRAM_LPDDR2_2CS_12G_X32:
        case DRAM_LPDDR2_2CS_16G_X32:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F15);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x05050505);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0505);
            break;
        }
        case DRAM_DDR3_1CS_1G_X16:
        case DRAM_DDR3_2CS_2G_X16:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F13);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0F06);
            break;
        }
        case DRAM_DDR3_1CS_2G_X16:
        case DRAM_DDR3_2CS_4G_X16:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F14);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0F06);
            break;
        }
        case DRAM_DDR3_1CS_4G_X16:
        case DRAM_DDR3_2CS_8G_X16:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F15);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0F06);
            break;
        }
        case DRAM_DDR3_1CS_4G_X8_4P:
        case DRAM_DDR3_1CS_8G_X16_2P:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F15);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0F06);
            break;
        }
        defalut:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F14);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0606);
        }        
        
    }
}

void umctl2_performance_init(DRAM_INFO* dram)
{
    //to be confirm by johnnywang, is these the best configuration???
    UMCTL2_REG_SET(UMCTL_PERFHPR0, 0x00000000);
    UMCTL2_REG_SET(UMCTL_PERFHPR1, 0x10000000);
    UMCTL2_REG_SET(UMCTL_PERFLPR0, 0x00000100);
    UMCTL2_REG_SET(UMCTL_PERFLPR1, 0x40000100);
    UMCTL2_REG_SET(UMCTL_PERFWR0,  0x00000000);
    UMCTL2_REG_SET(UMCTL_PERFWR1,  0x10000000);
}
void umctl2_refresh_init(DRAM_INFO* dram)
{
    lpddr1_timing_t*  lpddr1_timing =(lpddr1_timing_t*)(dram->ac_timing);
    lpddr2_timing_t* lpddr2_timing =(lpddr2_timing_t*)(dram->ac_timing);
    DDR2_ACTIMING*   ddr2_timing = (DDR2_ACTIMING*)(dram->ac_timing);
    ddr3_timing_t*   ddr3_timing = (ddr3_timing_t*)(dram->ac_timing);
	DRAM_TYPE_E dram_type = dram->dram_type;

    //to be confirm by johnnywang
    //UMCTL_RFSHCTL0
    //UMCTL_RFSHCTL1
    //UMCTL_RFSHCTL2
    //UMCTL_RFSHCTL3
    //UMCTL_RFSHTMG
	
	reg_bits_set(UMCTL_RFSHTMG,16,12,(IS_LPDDR2(dram_type)?(lpddr2_timing->tREFI>>5):0) |
									(IS_DDR3(dram_type)?(ddr3_timing->tREFI>>5):0) );

	reg_bits_set(UMCTL_RFSHTMG,0,9,(IS_LPDDR2(dram_type)?lpddr2_timing->tRFCpb:0) |
								  (IS_DDR3(dram_type)?ddr3_timing->tRFC:0) );
									
}
void umctl2_port_en(UMCTL2_PORT_ID_E port_id,BOOLEAN en)
{  
	return; 
    UMCTL2_REG_SET(UMCTL_PORT_EN_0+port_id*0xb0,en);    //prot enable
}

void umctl2_allport_en()
{
    uint8 i = 0;
	
	return;
    for(i = UMCTL2_PORT_MIN; i < UMCTL2_PORT_MAX; i++)
    {
        umctl2_port_en(i,TRUE);
    }
}

void umctl2_port_init(umctl2_port_info_t* port_info)
{
    uint32 i = 0;
    reg_bits_set(UMCTL_PCCFG,4,1,TRUE);//pagematch_limit
    reg_bits_set(UMCTL_PCCFG,0,1,TRUE);//go2critical_en


    for(i = 0; ;i++)
    {
        if(port_info[i].rdwr_order == 0xff)
        {
            return;
        }
        //read priority
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,16,1,port_info[i].rdwr_order);   //rdwr_ordered_en                
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,15,1,port_info[i].rd_hpr);       //rd_port_hpr_en            
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,14,1,port_info[i].rd_pagematch); //rd_port_pagematch_en        
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,13,1,port_info[i].rd_urgent);    //rd_port_urgent_en        
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,12,1,port_info[i].rd_aging);     //rd_port_aging_en        
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,11,1,port_info[i].rd_reorder_bypass);//read_reorder_bypass_en
        reg_bits_set(UMCTL_PCFGR_0+i*0xb0,0,10,port_info[i].rd_priority);  //rd_port_priority

        //write priority
        reg_bits_set(UMCTL_PCFGW_0+i*0xb0,14,1,port_info[i].wr_pagematch); //wr_port_pagematch_en
        reg_bits_set(UMCTL_PCFGW_0+i*0xb0,13,1,port_info[i].wr_urgent);    //wr_port_urgent_en        
        reg_bits_set(UMCTL_PCFGW_0+i*0xb0,12,1,port_info[i].wr_aging);     //wr_port_aging_en
        reg_bits_set(UMCTL_PCFGW_0+i*0xb0,0,10,port_info[i].wr_priority);  //wr_port_priority        
    }
#if 0    
    //port0,AP
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port1,Multi Media
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port2,GPU
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port3,CP2-WIFI port0
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port4,CP1-TD
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port5,AP-Matrix,DMA,SDIO,EMMC,NFC,USB
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port6,CP0-WCDMA
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port7,CP2-WIFI port1
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port8,WCDMA CP Matrix
    //read
    reg_bits_set(UMCTL_PCFGR_0, 0,10,0);   //rd_port_priority
    reg_bits_set(UMCTL_PCFGR_0,11,1,FALSE);//read_reorder_bypass_en
    reg_bits_set(UMCTL_PCFGR_0,12,1,TRUE); //rd_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,TRUE); //rd_port_urgent_en
    reg_bits_set(UMCTL_PCFGR_0,14,1,TRUE); //rd_port_pagematch_en
    reg_bits_set(UMCTL_PCFGR_0,15,1,TRUE); //rd_port_hpr_en    
    reg_bits_set(UMCTL_PCFGR_0,16,1,TRUE); //rdwr_ordered_en        
    //write
    reg_bits_set(UMCTL_PCFGR_0, 0,10,5);   //wr_port_priority
    reg_bits_set(UMCTL_PCFGR_0,12,1,FALSE);//wr_port_aging_en
    reg_bits_set(UMCTL_PCFGR_0,13,1,FALSE);//wr_port_urgent_en
    
    //port9,TD DSP&WCDMA DSP
#endif    
}
/*
 *[2:0]operation mode status
 *0x00-init
 *0x01-normal
 *0x02-power-down
 *0x03-SelfRefresh
 *0x04-DeepPowerdown,only support for mDDDR/LPDDR2/LPDDR3
*/
uint32 umctl2_wait_status(uint32 state) {
    uint32 poll_data = ((state==OPERATION_MODE_INIT)?0x00:0x00)|
                       ((state==OPERATION_MODE_NORMAL)?0x01:0x00)|
                       ((state==OPERATION_MODE_PD)?0x02:0x00)|
                       ((state==OPERATION_MODE_SR)?0x03:0x00)|
                       ((state==OPERATION_MODE_DPD)?0x04:0x00);

    while((UMCTL2_REG_GET(UMCTL_STAT)&0x07) != poll_data);
}

/*
 *Refer to uMCTL2 databook Chapter5.4.45~5.4.53.
 *Set sdram timing parameters,refer to SDRAM spec for details.
*/
void umctl2_dramtiming_init(DRAM_INFO* dram,CLK_TYPE_E umctl2_clk) {
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 BL = dram->bl;
    uint8 CL = dram->rl;
    uint8 RL = dram->rl;
    uint8 WL = dram->wl;
    uint8 AL = dram->al;

    /*Get the timing we used.*/
    lpddr1_timing_t*  lpddr1_timing =(lpddr1_timing_t*)(dram->ac_timing);
    lpddr2_timing_t* lpddr2_timing =(lpddr2_timing_t*)(dram->ac_timing);
    DDR2_ACTIMING*   ddr2_timing = (DDR2_ACTIMING*)(dram->ac_timing);
    ddr3_timing_t*   ddr3_timing = (ddr3_timing_t*)(dram->ac_timing);
    
    uint8 tWR=(IS_LPDDR1(dram_type)?(lpddr1_timing->tWR):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tWR):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tWR):0x00) ;
    uint8 tCKESR=(IS_LPDDR2(dram_type)?(lpddr2_timing->tCKESR):0x00);
    uint8 tCKSRX=(IS_DDR3(dram_type)?(ddr3_timing->tCKSRX):0x00) ;
    uint8 tMOD=IS_DDR3(dram_type)?(ddr3_timing->tMOD):0x00;/*DDR3 only*/
    uint8 tMRD=(IS_LPDDR2(dram_type)?4:0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tMRD):0x00)|/*DDR3/2 only*/
               (IS_LPDDR1(dram_type)?(lpddr1_timing->tMRD):0x00) ;
    uint8 tRTP=(IS_LPDDR2(dram_type)?(lpddr2_timing->tRTP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRTP):0x00) ;
    uint8 tWTR=(IS_LPDDR1(dram_type)?(lpddr1_timing->tWTR):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tWTR):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tWTR):0x00) ;
    uint8 tRP =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRP):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRP):0x00) ;
    uint8 tRCD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRCD):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRCD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRCD):0x00) ;
    uint8 tRAS=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRAS):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRAS):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRAS):0x00) ;
    uint8 tRRD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRRD):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRRD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRRD):0x00) ;
    uint8 tRC =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRC):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRC):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRC):0x00) ;
    uint8 tCCD=(IS_LPDDR2(dram_type)?(lpddr2_timing->tCCD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tCCD):0x00) ; 
    uint8 tFAW=(IS_LPDDR2(dram_type)?(lpddr2_timing->tFAW):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tFAW):0x00) ;
    uint8 tRFC=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRFC):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRFCab):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRFC):0x00) ;  
    uint8 tDQSCK=IS_LPDDR2(dram_type)?(lpddr2_timing->tDQSCK):0x00;/*LPDDR2 only*/
    uint8 tXS =(IS_DDR2(dram_type)?(ddr2_timing->tXS):200)|/*for DDR2/DDR3,default200*/
               (IS_DDR3(dram_type)?(ddr3_timing->tXS):200) ;
    uint8 tXP =(IS_LPDDR1(dram_type)?(lpddr1_timing->tXP):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tXP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tXP):0x00) ;
    uint8 tCKE=(IS_LPDDR1(dram_type)?(lpddr1_timing->tCKE):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tCKE):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tCKE):0x00) ;
    /*tDLLK:Dll locking time.Valid value are 2 to 1023*/
    uint8 tDLLK=IS_DDR3(dram_type)?(ddr3_timing->tDLLK):0x00;
    uint8  tDQSCKmax=IS_LPDDR2(dram_type)?(lpddr2_timing->tDQSCKmax):0x00;
    /*tAOND:ODT turnon/turnoff delays,DDR2 only*/
    uint8 tAOND=(IS_DDR2(dram_type)?(ddr2_timing->tAOND):0x00);
    /*tRTODT:Read to ODT delay,DDR3 only
     *0--ODT maybe turned on immediately after read post-amble
     *1--ODT maybe not turned on until one clock after read post-amble
    */
    uint8 tRTODT= 0x00;
    /*Get the timing we used.*/
#if 1
    reg_bits_set(UMCTL_DRAMTMG0, 24, 6, ((WL+(BL>>1)+tWR)+1));/*wr2pre*/
    reg_bits_set(UMCTL_DRAMTMG0, 16, 6, tFAW);    /*t_FAW*/
    reg_bits_set(UMCTL_DRAMTMG0,  8, 6, tRAS+ns_to_xclock(70000,umctl2_clk));/*t_ras_max,Maxinum time of tRAS,clocks*/
    reg_bits_set(UMCTL_DRAMTMG0,  0, 6, tRAS);/*t_ras_min,Mininum time of tRAS,clocks*/

    reg_bits_set(UMCTL_DRAMTMG1, 16, 6, tXP);
    /*Minimun from read to precharge of same bank*/
    reg_bits_set(UMCTL_DRAMTMG1,  8,5,(IS_DDR2(dram_type)?(AL+(BL>>1)+MAX(tRTP,2)-2):0x00) |
                                      (IS_DDR3(dram_type)?(AL+MAX(tRTP,4)):0x00) |
                                      (IS_LPDDR1(dram_type)?(BL>>1):0x00) |
                                      (IS_LPDDR2(dram_type)?((BL>>1)+MAX(tRTP,2)-2):0x00) |
                                    /*((IS_LPDDR2(dram_type)_S2)?((BL>>1)+tRTP-1):0x00) | changde
                                      ((IS_LPDDR2(dram_type)_S4)?((BL>>1)+MAX(tRTP,2)-2):0x00) |
                                      */
                                      (IS_LPDDR3(dram_type)?((BL>>1)+MAX(tRTP,4)-4):0x00) );
    reg_bits_set(UMCTL_DRAMTMG1,  0, 6, tRC); /*Active-to-Active command period*/

    reg_bits_set(UMCTL_DRAMTMG2, 24, 6, WL);
    reg_bits_set(UMCTL_DRAMTMG2, 16, 5, RL);
    /*Minimam time from read command to write command*/
    reg_bits_set(UMCTL_DRAMTMG2,  8, 5, IS_LPDDR2(dram_type)?(RL+(BL>>1)+tDQSCKmax+1-WL):(RL+(BL>>1)+2-WL));
    reg_bits_set(UMCTL_DRAMTMG2,  0, 6, (WL+(BL>>1)+tWTR));

    /*tMRW, time to wait during load mode register writes.*/
    reg_bits_set(UMCTL_DRAMTMG3, 16,10,(IS_LPDDR2(dram_type)?0x05:0x00) |
                                       (IS_LPDDR3(dram_type)?0x0A:0x00) );
    reg_bits_set(UMCTL_DRAMTMG3, 12, 3, tMRD);
    reg_bits_set(UMCTL_DRAMTMG3,  0,10, tMOD);

    reg_bits_set(UMCTL_DRAMTMG4, 24, 5, tRCD);
    reg_bits_set(UMCTL_DRAMTMG4, 16, 3, tCCD);
    reg_bits_set(UMCTL_DRAMTMG4,  8, 4, tRRD);
    reg_bits_set(UMCTL_DRAMTMG4,  0, 5, tRP);

    /*tCKSRX,the time before SelfRefreshExit that CK is maintained as a valid clock.*/
    /*Specifies the clock stable time before SRX.*/
    reg_bits_set(UMCTL_DRAMTMG5, 24, 4,(IS_LPDDR1(dram_type)?0x01:0x00) |
                                       (IS_LPDDR2(dram_type)?0x02:0x00) |
                                       (IS_LPDDR3(dram_type)?0x02:0x00) |
                                       (IS_DDR2(dram_type)?0x01:0x00) |
                                       (IS_DDR3(dram_type)?tCKSRX:0x00) );
    /*tCKSRE,the time after SelfRefreshDownEntry that CK is maintained as a valid clock.*/
    /*Specifies the clock disable delay after SRE.*/
    reg_bits_set(UMCTL_DRAMTMG5, 16, 4,(IS_LPDDR1(dram_type)?0x00:0x00) |
                                       (IS_LPDDR2(dram_type)?0x02:0x00) |
                                       (IS_LPDDR3(dram_type)?0x02:0x00) |
                                       (IS_DDR2(dram_type)?0x01:0x00) |
                                       (IS_DDR3(dram_type)?0x05:0x00) );
    /*tCKESR,Minimum CKE low width for selfrefresh entry to exit timing in clock cycles.*/
    reg_bits_set(UMCTL_DRAMTMG5,  8, 6,(IS_LPDDR1(dram_type)?tRFC:0x00) |
                                       (IS_LPDDR2(dram_type)?tCKESR:0x00) |
                                       (IS_LPDDR3(dram_type)?tCKESR:0x00) |
                                       (IS_DDR2(dram_type)?tCKE:0x00) |
                                       (IS_DDR3(dram_type)?(tCKE+1):0x00) );
    /*tCKE,Minimum number of cycles of CKE HIGH/LOW during power-down and selfRefresh.*/
    reg_bits_set(UMCTL_DRAMTMG5,  0, 4, tCKE);
    
    /*tCKDPDE,time after DeepPowerDownEntry that CK is maintained as a valid clock.*/
    reg_bits_set(UMCTL_DRAMTMG6, 24, 4,(IS_LPDDR1(dram_type)?0x00:0x00) |
                                       (IS_LPDDR2(dram_type)?0x02:0x00) |
                                       (IS_LPDDR3(dram_type)?0x02:0x00) );
    /*tCKDPDX,time before DeepPowerDownExit that CK is maintained as a valid clock before issuing DPDX.*/
    reg_bits_set(UMCTL_DRAMTMG6, 16, 4,(IS_LPDDR1(dram_type)?0x01:0x00) |
                                       (IS_LPDDR2(dram_type)?0x02:0x00) |
                                       (IS_LPDDR3(dram_type)?0x02:0x00) );
    /*tCKCSX,time before ClockStopExit that CK is maintained as a valid clock before issuing DPDX.*/
    reg_bits_set(UMCTL_DRAMTMG6,  0, 4,(IS_LPDDR1(dram_type)?0x01:0x00) |
                                       (IS_LPDDR2(dram_type)?(tXP+0x02):0x00) |
                                       (IS_LPDDR3(dram_type)?(tXP+0x02):0x00) );

    /*tCKPDE,time after PowerDownEntry that CK is maintained as a valid clock before issuing PDE.*/
    reg_bits_set(UMCTL_DRAMTMG7,  8, 4,(IS_LPDDR1(dram_type)?0x00:0x00) |
                                       (IS_LPDDR2(dram_type)?0x02:0x00) |
                                       (IS_LPDDR3(dram_type)?0x02:0x00) );
    /*tCKPDX,time before PowerDownExit that CK is maintained as a valid clock before issuing PDX.*/
    reg_bits_set(UMCTL_DRAMTMG7,  0, 4,(IS_LPDDR1(dram_type)?0x00:0x00) |
                                       (IS_LPDDR2(dram_type)?0x02:0x00) |
                                       (IS_LPDDR3(dram_type)?0x02:0x00) );

    /*post_selfref_gap_x32,time after coming out of selfref before doing anything.Default:0x44
    //reg_bits_set(MCTL_DRAMTMG8,  0, 4, max(tXSNR,max(tXSRD,tXSDLL)));*/
    reg_bits_set(UMCTL_DRAMTMG8,  0,32,(IS_LPDDR2(dram_type)?0x03:0x10));
#else
	reg_bits_set(UMCTL_DRAMTMG0,  0,32,0x0c147f11);
	reg_bits_set(UMCTL_DRAMTMG1,  0,32,0x00030313);
	reg_bits_set(UMCTL_DRAMTMG2,  0,32,0x03060909);
	reg_bits_set(UMCTL_DRAMTMG3,  0,32,0x00502000);
	reg_bits_set(UMCTL_DRAMTMG4,  0,32,0x0a03040a);
	reg_bits_set(UMCTL_DRAMTMG5,  0,32,0x02020606);
	reg_bits_set(UMCTL_DRAMTMG6,  0,32,0x02020005);
	reg_bits_set(UMCTL_DRAMTMG7,  0,32,0x00000202);
	reg_bits_set(UMCTL_DRAMTMG8,  0,32,0x00000003);	
#endif	
}
#if 0
/*
 *Refer to uMCTL2 databook for detail.
 *Set sdram timing parameters according to EDA simulation.
 *including DFI/ZQ/ADDR_MAPPING/PCFGR_x
*/
void umctl2_other_init(DRAM_INFO* dram) {
    DRAM_TYPE_E dram_type = dram->dram_type;

    reg_bits_set(UMCTL_RFSHTMG, 0,32, 0x620038);//yanbin
    
/*FPGA_TEST xiaohui in Beijing*/
    reg_bits_set(UMCTL_ZQCTL0,  0,32, 0x00900024);
    reg_bits_set(UMCTL_ZQCTL1,  0,32, 0x01400070);
    reg_bits_set(UMCTL_ZQCTL2,  0,32, 0x00000000);

    reg_bits_set(UMCTL_DFITMG0,  0,32,(IS_LPDDR1(dram_type)?0x02010100:0x00)|
                                      (IS_LPDDR2(dram_type)?0x02050103:0x00));
    reg_bits_set(UMCTL_DFITMG1,  0,32, 0x00300202);
    reg_bits_set(UMCTL_DFILPCFG0,  0,32, 0x07812120);//yanbin   0x07813120

    reg_bits_set(UMCTL_DFIUPD0,  0,32, 0x80400003);//yanbin   0x00400003
    reg_bits_set(UMCTL_DFIUPD1,  0,32, 0x001A0021);
    reg_bits_set(UMCTL_DFIUPD2,  0,32, 0x026904C9);
    reg_bits_set(UMCTL_DFIUPD3,  0,32, 0x030E051F);

    /*Phy initialization complete enable 
     *We asserted the MCTL_DFIMISC.[dfi_init_complete] to trigger SDRAM initialization.
    */
    reg_bits_set(UMCTL_DFIMISC,  0,32, 0x00000001);


    /*address mapping*/
    if(DRAM_LPDDR2 == dram_type)
    {
    reg_bits_set(UMCTL_ADDRMAP0,  0,32, 0x0000001f);//cs_bit_0 = 0
    reg_bits_set(UMCTL_ADDRMAP1,  0,32, 0x00070707);//bank_bit_2 = 11, bank_bit_1 = 10, bank_bit_0 = 9
    reg_bits_set(UMCTL_ADDRMAP2,  0,32, 0x00000000);//col_bit_5 = 5, col_bit_4 = 4, col_bit_3 = 3, col_bit_2 = 2
    reg_bits_set(UMCTL_ADDRMAP3,  0,32, 0x0f000000);//col_bit_9 = 0, col_bit_8 = 8, col_bit_7 = 7, col_bit_6 = 6
    reg_bits_set(UMCTL_ADDRMAP4,  0,32, 0x00000f0f);//col_bit_11 = 0, col_bit_10 = 0
    reg_bits_set(UMCTL_ADDRMAP5,  0,32, 0x06060606);//row_bit_11 = 23, row_bit_10:2 = 22:14, row_bit_1 = 13, row_bit_0 = 12  
    reg_bits_set(UMCTL_ADDRMAP6,  0,32, 0x0f0f0f06);//row_bit_15 = 0, row_bit_14 = 0, row_bit_13 = 0, row_bit_12 = 24
    } else {/*DRAM_LPDDR1*/
    reg_bits_set(UMCTL_ADDRMAP0,  0,32, 0x00000013);
    reg_bits_set(UMCTL_ADDRMAP1,  0,32, 0x000f0707);
    reg_bits_set(UMCTL_ADDRMAP2,  0,32, 0x00000000);
    reg_bits_set(UMCTL_ADDRMAP3,  0,32, 0x0f000000);
    reg_bits_set(UMCTL_ADDRMAP4,  0,32, 0x00000F0F);
    reg_bits_set(UMCTL_ADDRMAP5,  0,32, 0x05050505);
    reg_bits_set(UMCTL_ADDRMAP6,  0,32, 0x0F0F0505);
    }

    reg_bits_set(UMCTL_ODTCFG,  0,32, 0x02010205);
    reg_bits_set(UMCTL_ODTMAP,  0,32, 0x00000312);

    reg_bits_set(UMCTL_SCHED,  0,32, 0x00070C01);
    reg_bits_set(UMCTL_PERFHPR0,  0,32, 0x00000000);
    reg_bits_set(UMCTL_PERFHPR1,  0,32, 0x10000000);
    reg_bits_set(UMCTL_PERFLPR0,  0,32, 0x00000100);
    reg_bits_set(UMCTL_PERFLPR1,  0,32, 0x40000100);
    reg_bits_set(UMCTL_PERFWR0,  0,32, 0x00000000);
    reg_bits_set(UMCTL_PERFWR1,  0,32, 0x10000000);

    /*port common configuration register
     *[4]:pagematch_limit
     *[0]:go2critical_en
    */
    reg_bits_set(UMCTL_PCCFG,  0,32, 0x00000011);
    /*Port n configuration Read/Write register setting*/
    reg_bits_set(UMCTL_PCFGR_0, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_0, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_0,0,32,0x00000001);    //prot enable
    reg_bits_set(UMCTL_PCFGR_1, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_1, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_1,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_2, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_2, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_2,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_3, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_3, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_3,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_4, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_4, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_4,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_5, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_5, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_5,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_6, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_6, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_6,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_7, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_7, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_7,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_8, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_8, 0,32, 0x00004005);
    reg_bits_set(UMCTL_PORT_EN_8,0,32,0x00000001);
    reg_bits_set(UMCTL_PCFGR_9, 0,32, 0x0001F000);
    reg_bits_set(UMCTL_PCFGW_9, 0,32, 0x00004005);  
    reg_bits_set(UMCTL_PORT_EN_9,0,32,0x00000001);


}
#endif
/*
 *Power-up timing initialization and Mode Register Setting for sdram.
*/
void umctl2_poweron_init(DRAM_INFO* dram,CLK_TYPE_E umctl2_clk) {
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 BL = dram->bl;
    uint8 CL = dram->rl;
    uint8 RL = dram->rl;
    uint8 WL = dram->wl;
    uint8 AL = dram->al;
    uint8 mr=0,emr=0,mr2=0,mr3=0;

    /*Get the timing we used.*/
    lpddr1_timing_t*  lpddr1_timing =(lpddr1_timing_t*)(dram->ac_timing);
    lpddr2_timing_t* lpddr2_timing =(lpddr2_timing_t*)(dram->ac_timing);
    DDR2_ACTIMING*   ddr2_timing = (DDR2_ACTIMING*)(dram->ac_timing);
    ddr3_timing_t*   ddr3_timing = (ddr3_timing_t*)(dram->ac_timing);

    /* tINIT0:(-~20)ms  Maximum voltage ramp time */
    uint32 tINIT0 = 20;
    /* tINIT1:(100~-)ns Minimum CKE LOW time after completion of voltage ramp*/
    uint32 tINIT1 = 100;
    /* tINIT2:(5~-)tCK  Minimum stable clock before first CKE HIGH*/
    uint32 tINIT2 = 10;
    /* tINIT3:(200~-)us Minimum idle time after first CKE assertion*/
    uint32 tINIT3 = 300;
    /* tINIT4:(001~-)us Minimum idle time after RESET command*/
    uint32 tINIT4 = 10;
    /* tINIT5:(-~10)us  Maximum duration of device auto initialization*/
    uint32 tINIT5 = 8;
    /* tZQINIT:(1~-)us  ZQ initial calibration (S4 devices only)*/
    uint32 tZQINIT = 5;
    /* tCKb:(18~100)ns  Clock cycle time during boot*/
    uint32 tCKb = 50;

    /*post_cle_x1024,cycles to wait after driving CKE high to start the SDRAM init sequence.*/
    reg_bits_set(UMCTL_INIT0, 16,10,(IS_LPDDR2(dram_type)?us_to_x1024(200,umctl2_clk):0x00) |
                                    (IS_DDR3(dram_type)?  ns_to_x1024(360,umctl2_clk):0x00) 
//                                    (IS_LPDDR3(dram_type)?us_to_x1024(200,umctl2_clk):0x00) |
//                                    (IS_DDR2(dram_type)?  ns_to_x1024(400,umctl2_clk):0x00) |                                     
                                     );
    /*pre_cle_x1024,cycles to wait after reset before driving CKE high to start the SDRAM init sequence.*/
    reg_bits_set(UMCTL_INIT0,  0,10,
                                    /*tINIT0 of 20ms(max) + tINIT1 of 100ns(min)*/
                                    (IS_LPDDR2(dram_type)?(ms_to_x1024(tINIT0,umctl2_clk)+ns_to_x1024(tINIT1,umctl2_clk)):0x00)|
//                                    (IS_LPDDR3(dram_type)?(ms_to_x1024(tINIT0,umctl2_clk)+ns_to_x1024(tINIT1,umctl2_clk)):0x00)|
                                    (IS_DDR3(dram_type)  ?us_to_x1024(500,umctl2_clk):0x00)
                                    //(IS_DDR2(dram_type)?us_to_x1024(200,umctl2_clk):0x00)  /*>=200us*/ 
                                    );
                       

    /*dram_rstn_x1024,cycles to assert SDRAM reset signal during init sequence.*/
    reg_bits_set(UMCTL_INIT1, 16, 8,(IS_DDR3(dram_type)?0x02:0x00)); /*>=1tCK*/ 
    /*final_wait_x32,cycles to wait after completing the SDRAM init sequence.*/
    reg_bits_set(UMCTL_INIT1,  8, 7, 0x01); /*>=0tCK*/
    /*pre_ocd_x32,cycles to wait before driving the OCD complete command to SDRAM.*/
    reg_bits_set(UMCTL_INIT1,  0, 5, 0x01); /*>=0tCK*/

    /*idle_after_reset_x32,idle time after the reset command,tINT4.*/
    reg_bits_set(UMCTL_INIT2,  8, 8,(IS_LPDDR2(dram_type)?us_to_x32(tINIT4,umctl2_clk):0x00));
    /*min_stable_clock_x1,time to wait after the first CKE high,tINT2.*/
    reg_bits_set(UMCTL_INIT2,  0, 4,(IS_LPDDR2(dram_type)?tINIT2:0x00)|
                                    (IS_LPDDR3(dram_type)?tINIT2:0x00)|
                                    (IS_DDR3(dram_type)  ?0x05:0x00));

    /*Note:
     *    Set Mode register 0~3 for SDRAM memory.
     *    Refer to JESD spec for detail.
    */

    if(IS_LPDDR1(dram_type))
    {
        /*mr store the value to write to MR register*/
        mr= (BL<<0)|/*burst_length:BL2/4/8/16*/
            (0x00<<3)|/*0:Sequentia;1:interleavel*/
            (CL<<4);/*cas_latency:2/3/4 optional*/
        /*emr store the value to write to EMR register*/
        emr=(0x00<<0)|/*PASR:0:AllBanks;1:HalfArray;2:QuarterArray,etc*/
            (0x00<<3)|/*TCSR:0:70'C;1:45'C;2:15'C;3:85'C;*/
            ((
             ((MEM_IO_DS==DS_FULL)?0x00:0x00)|
             ((MEM_IO_DS==DS_HALF)?0x01:0x00)|
             ((MEM_IO_DS==DS_QUARTER)?0x02:0x00)|
             ((MEM_IO_DS==DS_OCTANT)?0x03:0x00)|
             ((MEM_IO_DS==DS_THREE_QUATERS)?0x04:0x00)
             )<<5);/*DS:0:FullDriverStrength;1:HalfDS;2:QuarterDS;3:OctantDS;4:Three-Quater*/
        
        /*mr2 unused for MDDR*/
        /*mr3 unused for MDDR/LPDDR2/LPDDR3*/
    }
    else if(IS_LPDDR2(dram_type))
    {
		uint8 tWR  =lpddr2_timing->tWR;
        /*mr store the value to write to MR1 register*/
        /*Set sequential burst-type with wrap*/
        mr= ((BL==4)?0x02:0x00)|/*burst_length:2:BL4(default;3:BL8;4:BL16)*/
            ((BL==8)?0x03:0x00)|
            ((BL==16)?0x04:0x00)|
            ((/*0:Sequential(default);1:interleavel(allow for SDRAM only)*/
             ((MEM_BURST_TYPE==DRAM_BT_SEQ)?0x00:0x00)|
             ((MEM_BURST_TYPE==DRAM_BT_INTER)?0x01:0x00)
             )<<3
            )|
            (0x00<<4)|/*0:Wrap(default);1:No wrap(allow for SDRAM BL4 only)*/
            ((/*WriteRecovery,Default(0x01)*/
              ((tWR==3)?0x01:0x00)|
              ((tWR==4)?0x02:0x00)|
              ((tWR==5)?0x03:0x00)|
              ((tWR==6)?0x04:0x00)|
              ((tWR==7)?0x05:0x00)|
              ((tWR==8)?0x06:0x00)
             )<<5
            );
        /*emr store the value to write to MR2 register,Default:0x01*/
        emr = (((RL==3)&&(WL==1))?0x01:0x00) |
              (((RL==4)&&(WL==2))?0x02:0x00) |
              (((RL==5)&&(WL==2))?0x03:0x00) |
              (((RL==6)&&(WL==3))?0x04:0x00) |
              (((RL==7)&&(WL==4))?0x05:0x00) |
              (((RL==8)&&(WL==4))?0x06:0x00) ;
        /*mr2 store the value to write to MR3 register*/
        /*DS,driver strength configuration.Default:40ohm*/
        mr2 = ((MEM_IO_DS==DS_34R3)?0x01:0x00) |
              ((MEM_IO_DS==DS_40R)?0x02:0x00) |
              ((MEM_IO_DS==DS_48R)?0x03:0x00) |
              ((MEM_IO_DS==DS_60R)?0x04:0x00) |
              ((MEM_IO_DS==DS_68R6)?0x05:0x00) |
              ((MEM_IO_DS==DS_80R)?0x06:0x00) |
              ((MEM_IO_DS==DS_120R)?0x07:0x00) ;
        /*mr3 unused for MDDR/LPDDR2/LPDDR3*/
    }
    else if(IS_DDR3(dram_type))
    {
        uint8 tWR  = ddr3_timing->tWR;
        uint8 CWL = (WL-AL)?(WL-AL):0x00;
        /*mr store the value to write to MR1 register*/
        /*Set sequential burst-type with wrap*/
        mr= ((BL==8)?0x00:0x00)| /*Fixed on 8*/
            ((BL==4)?0x01:0x00)| /*4or8 on the fly*/
            ((BL==4)?0x02:0x00)| /*Fixed on 4*/
            ((/*0:Sequential(default);1:interleavel(allow for SDRAM only)*/
             ((MEM_BURST_TYPE==DRAM_BT_SEQ)?0x00:0x00)|
             ((MEM_BURST_TYPE==DRAM_BT_INTER)?0x01:0x00)
             )<<3
            )|
            ((((CL==5)?0x01:0x00)| /*Bit[6:4],CAS Latency*/
              ((CL==6)?0x02:0x00)|
              ((CL==7)?0x03:0x00)|
              ((CL==8)?0x04:0x00)|
              ((CL==9)?0x05:0x00)|
              ((CL==10)?0x06:0x00)|
              ((CL==11)?0x07:0x00))<<4
            )|
            ((((tWR<=5)?0x01:0x00)|/*Bit[11:9],Write recovery for autoprecharge*/
              ((tWR==6)?0x02:0x00)|
              ((tWR==7)?0x03:0x00)|
              ((tWR==8)?0x04:0x00)|
              ((tWR==10)?0x05:0x00)|
              ((tWR==12)?0x06:0x00)|
              ((tWR==14)?0x07:0x00)|
              ((tWR==16)?0x00:0x00))<<9
            );
                             
        /*emr store the value to write to MR1 register*/
        /*A0:0-DLL Enable;1-DLL Disable*/
        reg_bits_set((uint32)&emr,  0, 1, 0);
        /*Output Driver Impedance Control
         [A5,A1]:00-RZQ/6;01-RZQ/7;10/11-RZQ/TBD;Note: RZQ=240ohm*/
        reg_bits_set((uint32)&emr,  1, 1, 0);
        reg_bits_set((uint32)&emr,  5, 1, 0);
        /*[A4:A3]:Additive Latency.*/
        reg_bits_set((uint32)&emr,  3, 2, ((AL==0)?0x00:0x00)|
	                                      ((AL==CL-1)?0x01:0x00)|
	                                      ((AL==CL-2)?0x02:0x00)
	                                      );            
        /*[A7]:1-Write leveling enable;0-Disabled*/
        reg_bits_set((uint32)&emr,  7, 1, 1);

        /*mr2 store the value to write to MR2 register*/
        /*Partial Array Self-Refresh (Optional),[A2:A0]
        */
        /*CAS write Latency (CWL),WL=CWL+AL
         [A5:A3]:5~12 tCK
        */
        reg_bits_set((uint32)&mr2,  3, 3, ((CWL==5)?0x00:0x00)|
	                                      ((CWL==6)?0x01:0x00)|
	                                      ((CWL==7)?0x02:0x00)|
	                                      ((CWL==8)?0x03:0x00)|
	                                      ((CWL==9)?0x04:0x00)|
	                                      ((CWL==10)?0x05:0x00)|
	                                      ((CWL==11)?0x06:0x00)|
	                                      ((CWL==12)?0x07:0x00)
	                                      );

        /*mr3 store the value to write to MR3 register*/
        /*[A1:A0],MPR location*/
        /*[A2],MPR*/
    }
    
    reg_bits_set(UMCTL_INIT3, 16,16, mr);
    reg_bits_set(UMCTL_INIT3,  0,16, emr);
    reg_bits_set(UMCTL_INIT4, 16,16, mr2);
    reg_bits_set(UMCTL_INIT4,  0,16, mr3);


    /*dev_zqinit_x32,ZQ initial calibration,tZQINIT.*/ //to be confirm by johnnywang
    reg_bits_set(UMCTL_INIT5, 16, 8,(IS_DDR3(dram_type)  ?us_to_x32(tZQINIT,umctl2_clk):0x00)| 
                                    (IS_LPDDR2(dram_type)?us_to_x32(tZQINIT,umctl2_clk):0x00)|
                                    (IS_LPDDR3(dram_type)?us_to_x32(tZQINIT,umctl2_clk):0x00));
    /*max_auto_init_x1024,max duration of the auto initialization,tINIT5.*/ // to be confirm by johnnywang
    reg_bits_set(UMCTL_INIT5,  0,10,(IS_LPDDR2(dram_type)?us_to_x1024(10,umctl2_clk):0x00)|
                                    (IS_LPDDR3(dram_type)?us_to_x1024(10,umctl2_clk):0x00));

}

/*
 *Refer to uMCTL2 databook Chapter2.15.1 "DDR2 initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/
void umctl_ddr2_init(DRAM_INFO* dram,   void* init_timing) {

}

/*
 *Refer to uMCTL2 databook Chapter2.15.2 "DDR3 initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/
void umctl_ddr3_init(DRAM_INFO* dram,   void* init_timing) {

}

/*
 *Refer to uMCTL2 databook Chapter2.15.3 "Mobile ddr initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/
void umctl_mddr_init(DRAM_INFO* dram,   void* init_timing) {
#if 0
//changde
    uint8 i=0;
    uint8 dram_type = dram->dram_type;
    uint8 mr_ranks =dram->cs_num;
    /*T=200us,Power-up: VDD and CK stable*/
    uint32 T=200;

    uint32 INIT0=0;
    INIT0 = (IS_LPDDR1(dram_type)?us_to_x1024(T):0x00);
    UMCTL2_REG_SET(UMCTL_INIT0, INIT0);
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);

    /*Issue precharege all command.*/
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);

    /*Issue refresh command 8 times.*/
    for(i=0; i<8; i++) {
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_REFRESH, 0x00);
    }

    /*Load mode register(MR).*/
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);

    /*Load extEnded mode register(EMR).*/
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);

    /*Issue active command.*/
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);

    /*Now,begin normal operation*/
#endif
}

/*
 *Refer to uMCTL2 databook Chapter2.15.4 "LPDDR2/LPDDR3 initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/
void umctl_ldppr2or3_init(DRAM_INFO* dram, void* init_timing) {
#if 0
//changde
    /*A reset command is issued to MRW63 register.*/
    mr_rw(dram_type, mr_ranks, MR_WRITE, 0x3F, 0x00);
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);
    /*A ZQ initialization calibration command is issued to MRW10 register.*/
    mr_rw(dram_type, mr_ranks, MR_WRITE, 0x0A, 0xFF);
    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);
    /*Setting MR2 register to INIT3.emr followed by a NOP*/

    /*Setting MR1 register to INIT3.emr followed by a NOP*/

    /*Setting MR3 register to INIT3.emr followed by a NOP*/

    /*Schedule multiple all bank refresh*/

    /*Now,begin normal operation*/
#endif
}
#if 0
/*
 *SDram power-on timing and common timing using PHY publ.
 *Configure PUBL_DCR,PUBL_PTR0/1/2 and PUBL_CFG_DTPR0/1/2
 *Attention:timing parameters in clock cycles.
*/
void publ_sdram_timing(DRAM_INFO* dram,CLK_TYPE_E umctl2_clk)
{
    uint32 dcr=0;
    uint32 dtpr0=0,dtpr1=0,dtpr2=0;
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 banks = dram->bank_num;
    uint8 BL = dram->bl;
    uint8 CL = dram->rl;
    uint8 RL = dram->rl;
    uint8 WL = dram->wl;

    /*Get the timing we used.*/
    lpddr1_timing_t*  lpddr1_timing =(lpddr1_timing_t*)(dram->ac_timing);
    lpddr2_timing_t* lpddr2_timing =(lpddr2_timing_t*)(dram->ac_timing);
    DDR2_ACTIMING*   ddr2_timing = (DDR2_ACTIMING*)(dram->ac_timing);
    ddr3_timing_t*   ddr3_timing = (ddr3_timing_t*)(dram->ac_timing);

    uint8 tWR=(IS_LPDDR1(dram_type)?(lpddr1_timing->tWR):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tWR):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tWR):0x00) ;
    uint8 tCKESR=(IS_LPDDR2(dram_type)?(lpddr2_timing->tCKESR):0x00);
    uint8 tCKSRX=(IS_DDR3(dram_type)?(ddr3_timing->tCKSRX):0x00) ;
    uint8 tMOD=IS_DDR3(dram_type)?(ddr3_timing->tMOD):0x00;/*DDR3 only*/
    uint8 tMRD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tMRD):0x00)|
               (IS_DDR2(dram_type)?(ddr2_timing->tMRD):0x00)|/*DDR3/2 only*/
               (IS_DDR3(dram_type)?(ddr3_timing->tMRD):0x00) ;
    uint8 tRTP=(IS_LPDDR2(dram_type)?(lpddr2_timing->tRTP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRTP):0x00) ;
    uint8 tWTR=(IS_LPDDR1(dram_type)?(lpddr1_timing->tWTR):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tWTR):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tWTR):0x00) ;
    uint8 tRP =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRP):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRP):0x00) ;
    uint8 tRCD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRCD):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRCD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRCD):0x00) ;
    uint8 tRAS=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRAS):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRAS):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRAS):0x00) ;
    uint8 tRRD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRRD):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRRD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRRD):0x00) ;
    uint8 tRC =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRC):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRC):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRC):0x00) ;
    uint8 tCCD=(IS_LPDDR2(dram_type)?(lpddr2_timing->tCCD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tCCD):0x00) ; 
    uint8 tFAW=(IS_LPDDR2(dram_type)?(lpddr2_timing->tFAW):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tFAW):0x00) ;
    uint8 tRFC=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRFC):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRFCab):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRFC):0x00) ;  
    uint8 tDQSCK=IS_LPDDR2(dram_type)?(lpddr2_timing->tDQSCK):0x00;/*LPDDR2 only*/
    uint8 tXS =(IS_DDR2(dram_type)?(ddr2_timing->tXS):200)|/*for DDR2/DDR3,default200*/
               (IS_DDR3(dram_type)?(ddr3_timing->tXS):200) ;
    uint8 tXP =(IS_LPDDR1(dram_type)?(lpddr1_timing->tXP):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tXP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tXP):0x00) ;
    uint8 tCKE=(IS_LPDDR1(dram_type)?(lpddr1_timing->tCKE):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tCKE):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tCKE):0x00) ;
    /*tDLLK:Dll locking time.Valid value are 2 to 1023*/
    uint8 tDLLK=IS_DDR3(dram_type)?(ddr3_timing->tDLLK):0x00;
    uint8 tDQSCKmax=IS_LPDDR2(dram_type)?(lpddr2_timing->tDQSCKmax):0x00;
    /*tAOND:ODT turnon/turnoff delays,DDR2 only*/
    uint8 tAOND=(IS_DDR2(dram_type)?(ddr2_timing->tAOND):0x00);
    /*tRTODT:Read to ODT delay,DDR3 only
     *0--ODT maybe turned on immediately after read post-amble
     *1--ODT maybe not turned on until one clock after read post-amble
    */
    uint8 tRTODT= 0x00;
    /*Get the timing we used.*/

    {/*dram configuration register (DCR)*/
        /*[2:0]:SDram ddr mode.*/
        reg_bits_set(PUBL_DCR, 0, 3, (IS_LPDDR1(dram_type)?0x00:0x00)|
                                     (IS_DDR2(dram_type)?0x02:0x00)|
                                     (IS_DDR3(dram_type)?0x03:0x00)|
                                     (IS_LPDDR2(dram_type)?0x04:0x00));
        /*DDR 8-bank,indicates if set that sdram used has 8 banks.
         *tRPA=tRP+1 and tFAW are used for 8banks DRAMs,other tRPA=tRP and no tFAW
         *is used.Note that a setting of 1for DRAMs that have fewer than 8banks still
         *results in correct functionality but less tigther daram parameters.
        */
        reg_bits_set(PUBL_DCR, 3, 1, (banks==8)?0x01:0x00);
        /*Select the ddr type for the specified lpddr mode
        reg_bits_set(PUBL_CFG_DCR, 8, 2, ((IS_LPDDR2(dram_type)_S4)?0x00:0x00)|
                                         ((IS_LPDDR2(dram_type)_S2)?0x01:0x00));*/
    }

    //PTR0, to set tDLLSRST, tDLLLOCK, tITMSRST
    {
        //DLL Soft Reset Time: Number of controller clock cycles that the DLL soft reset pin
        //must remain asserted when the soft reset is triggered through the PHY Initialization
        //Register (PIR). This must correspond to a value that is equal to or more than 50ns
        //or 8 controller clock cycles, whichever is bigger.
        //Default value is corresponds to 50ns at 533Mhz
        uint32 tDLLSRST = ns_to_xclock(50,umctl2_clk); //ns-->clocks       
        //DLL Lock Time: Number of clock cycles for the DLL to stabilize and lock, i.e. number
        //of clock cycles from when the DLL reset pin is de-asserted to when the DLL has
        //locked and is ready for use. Refer to the PHY databook for the DLL lock time.
        //Default value corresponds to 5.12us at 533MHz.        
        uint32 tDLLLOCK = ns_to_xclock(5120,umctl2_clk);//ns-->clocks
        //ITM Soft Reset Time: Number of controller clock cycles that the ITM soft reset pin
        //must remain asserted when the soft reset is applied to the ITMs. This must
        //correspond to a value that is equal to or more than 8 controller clock cycles. 
        //Default value corresponds to 8 controller clock cycles
        uint32 tITMSRST = 8;//clocks

        REG32(PUBL_PTR0) = (tITMSRST<<18)|(tDLLLOCK<<6)|(tDLLSRST);
    }
    
    //PTR1, to set tINT0,tINT1
    {
        uint32 tINT0 = (IS_DDR3(dram_type)?us_to_xclock(500,umctl2_clk):0x00) |/*Unit:us->clk*/
                       (IS_DDR2(dram_type)?us_to_xclock(200,umctl2_clk):0x00) |
                       (IS_LPDDR1(dram_type)?us_to_xclock(200,umctl2_clk):0x00) |
                       (IS_LPDDR2(dram_type)?us_to_xclock(200,umctl2_clk):0x00);
        uint32 tINT1 = (IS_DDR3(dram_type)?0x05:0x00) |/*Unit:clk*/
                       (IS_DDR2(dram_type)?ns_to_xclock(400,umctl2_clk):0x00) |
                       (IS_LPDDR2(dram_type)?ns_to_xclock(100,umctl2_clk):0x00);

        REG32(PUBL_PTR1) =  (tINT1<<19)|tINT0;
    }
    
    //PTR2, to set tINT2,tINT3
    {
        /*tINT2,default 200us, time for reset command to end of auto initialization */
        uint32 tINT2 = (IS_DDR3(dram_type)?ns_to_xclock(200000,umctl2_clk):0x00) |/*Unit:ns->clk*/
                       (IS_LPDDR2(dram_type)?ns_to_xclock(11000,umctl2_clk):0x00);
        //tINT3,default 1us, time for ZQ initialization command to first command */
        uint32 tINT3 = IS_LPDDR2(dram_type)?ns_to_xclock(1000,umctl2_clk):0x00;

        REG32(PUBL_PTR2 ) = (tINT3<<17)|tINT2;
    }

    /*Dram timing parameters in clock cycles can be set in register DTPR0-2.
     *Refer to the SDRAM datasheet for detailed description about these timing in more frequence.
    */
    dtpr0 = ((tMRD&0x03)<<0) |
            ((tRTP&0x07)<<2) |
            ((tWTR&0x07)<<5) |
            ((tRP&0x0F)<<8) |
            ((tRCD&0x0F)<<12) |
            ((tRAS&0x1F)<<16) |
            ((tRRD&0x0F)<<21) |
            ((tRC&0x3F)<<25) |
            ((tCCD&0x01)<<31) ;

    dtpr1 = ((tAOND&0x03)<<0) |
            /*((tRTW&0x01)<<2) | use default*/
            ((tFAW&0x3F)<<3) |
            ((tMOD&0x03)<<9) |
            ((tRTODT&0x0F)<<11)|
            ((tRFC&0xFF)<<16) |
            ((tDQSCK&0x07)<<24)
            /*((tDQSCKmax&0x07)<<27)use default*/ ;

    dtpr2 = ((tXS&0x3FF)<<0) |
            ((tXP&0x1F)<<10) |
            ((tCKE&0x0F)<<15) |
            ((tDLLK&0x3FF)<<19) ;
            
    REG32(PUBL_DTPR0) = dtpr0;
    REG32(PUBL_DTPR1) = dtpr1;
    REG32(PUBL_DTPR2) = dtpr2;
}

    uint32 ddr_rw_chk_single(uint32 offset, uint32 data)
    {
        uint32 rd;
        *(volatile uint32 *)(SDRAM_BASE + offset) = data;
        rd = *(volatile uint32 *)(SDRAM_BASE + offset);
        if(rd == data)
            return 1;
        else
            return 0;
    }
    uint32 ddr_rw_chk(uint32 offset)
    {
        uint32 i, data;
        for(i = 0; i < 6; i++)
        {
            if(i == 0)
            {
                data = 0x00000000;
            }
            else if(i == 1)
            {
                data = 0xffffffff;
            }
            else if(i == 2)
            {
                data = 0x12345678;
            }
            else if(i == 3)
            {
                data = 0x87654321;
            }
            else if(i == 4)
            {
                data = 0x5a5a5a5a;
            }
            else if(i == 5)
            {
                data = 0xa5a5a5a5;
            }
            if(ddr_rw_chk_single(offset, data) == 0)
                return 0;
            if(ddr_rw_chk_single(offset + 0x4, data) == 0)
                return 0;
            if(ddr_rw_chk_single(offset + 0x8, data) == 0)
                return 0;
            if(ddr_rw_chk_single(offset + 0xc, data) == 0)
                return 0;
        }
        return 1;
    }

uint32 dqs_manual_training(uint32 offset)
{
    uint32 i = 0;
    uint32 B0, B1, B2, B3;
    for(B0 = 0; B0 < 16; B0++)
    {
        UMCTL2_REG_SET(PUBL_DX0DQSTR, B0);

        for(B1 = 0; B1 < 16; B1++)
        {
            UMCTL2_REG_SET(PUBL_DX1DQSTR, B1);
            
            for(B2 = 0; B2 < 16; B2++)
            {
                UMCTL2_REG_SET(PUBL_DX2DQSTR, B2);
                for(B3 = 0; B3 < 16; B3++)
                {
                    UMCTL2_REG_SET(PUBL_DX3DQSTR, B3);

                    if(ddr_rw_chk(offset))
                    {
                        UMCTL2_REG_SET((SDRAM_BASE+0x1000+i), (0xBA55<<16)|(B3<<12)|(B2<<8)|(B1<<4)|B0);

        UMCTL2_REG_SET(PUBL_DX0DQSTR, (B0<<4)|B0);
        UMCTL2_REG_SET(PUBL_DX1DQSTR, (B1<<4)|B1);
        UMCTL2_REG_SET(PUBL_DX2DQSTR, (B2<<4)|B2);
        UMCTL2_REG_SET(PUBL_DX3DQSTR, (B3<<4)|B3);
                        return 1;
                    }
                    else
                    {
                        UMCTL2_REG_SET((SDRAM_BASE+0x1000+i), (0xFA11<<16)|(B3<<12)|(B2<<8)|(B1<<4)|B0);
                    }
                    i = i + 4;
                }
            }
        }
    }
    //if not found, set as default value
    UMCTL2_REG_SET(PUBL_DX0DQSTR, 0xAA);
    UMCTL2_REG_SET(PUBL_DX1DQSTR, 0xAA);
    UMCTL2_REG_SET(PUBL_DX2DQSTR, 0xAA);
    UMCTL2_REG_SET(PUBL_DX3DQSTR, 0xAA);
    return 0;
}

/*
 *Init sdram with publ build-in test.
 *Do training routine defined in PUBL_CFG_PIR.
*/
void publ_start_init(DRAM_INFO* dram) {
    uint8 dram_type = dram->dram_type;
    /*PGCR:PHY general configuration register.
    *[:0]-ITMDMD;0=ITMS uses DQS and DQS#,1=ITMS uses DQS only.
    *[:1]-DQSCFG;0=DQS gating is shut off using the rising edge of DQS_b(active windowing mode)
    *           1=DQS gating blankets the whole burst(passive windowing mode).
    *           Note:passive windowing must be used for LPDDR2
    *[:2]-DFTCMP;0=disable data strobe drift compensation,1=Enable
    *[4:3]-DFTLMT;DQS drift limit:0=NoLimit,1=90'C drift,2=180'C drift,3=270'C or more drift
    *[8:5]-DTOSEL;DigitalTestOutputSelect:
    *[11:9]-CKEN;Control whether the CK going to the SDRAM is enable(toggling) or disable.
    *[13:12]-CKDV;CK disable value.
    *[14]-CKINV
    *[15]-IOLB
    *[17:16]-IODDRM
    *[21:18]-RANKEN;Enable the ranks that are enabled for data-training. bit_n control trank_n.
    *[23:22]-ZCKSEL
    *[24]-PDDISDX
    *[28:25]-RFSHDT;Refresh during training.
    *[29]-LBDQSS
    *[30]-LBGQSS
    *[31]-LBMODE
    */
    reg_bits_set(PUBL_PGCR,  0, 1, IS_LPDDR1(dram_type)?0x01:0x00);
    reg_bits_set(PUBL_PGCR,  1, 2, 0x01); //attention lpddr1 set 0x01
    reg_bits_set(PUBL_PGCR, 18, 4, 0x03);

    /*DSGCR:ddr system general configuration register.
    *[0]-PUREN;phy update request enable.
    *[1]-BDISEN;Byte disable enable.If set PHY should respond to DFI byte disable request.
    *[2]-ZUEN;Impedance update enable.If set PHY should perform impedance calibraion 
    *         whenever there is a controller initiated DFI update request.
    *[3]-LPIOPD;Low power I/O power down.
    *[4]-LPDLLPD;
    *[07:5]-DQSGX,dqs gate extension.
    *[10:8]-DQSGE,dqs gate early.
    *[19:16]-CKEPDD,cke power down driver.
    *[23:20]-ODTPDD,ODT power down driver.
    */
    //xiaohui   yanbin_debug changde default:0xFA00001F
    /*if(IS_LPDDR2(dram_type) || IS_DDR3(dram_type))
    reg_bits_set(PUBL_DSGCR,  0,12, 0x25F);    
    */
    /*polling whether PHY initialization done*/
    /*PGSR:phy general status register.
    *[0]-IDONE,initialization done.This bit is set after all the selected initializatin 
    *    routines in PIR register has completed.
    *[1]-DLDONE,dll lock done.
    *[2]-ZCDONE,impedance calibrarion done.
    *[3]-DIDONE,DRAM initialization done
    *[4]-DTDONE,data training done.
    *[5]-DTERR,data training error.
    *[6]-DTIERR,data training intermittent error.
    *[7]-DFTERR,DQS drift Error.
    *[30:8]-reserved
    *[31]-TQ,temperature output(LPDDR only)
    */
    while(UMCTL2_REG_GET(PUBL_PGSR)&0x03 != 0x03);


//kevin.yanbin
{
	uint32 value_temp=0,i=0;
    //triggering publ PIR initialization 
    UMCTL2_REG_SET(PUBL_PIR, 0x00040001);
    for(i = 0; i <= 100; i++);

    //waite for initialize done
    do
    {
        value_temp = UMCTL2_REG_GET(PUBL_PGSR);
    }
    while( (value_temp & 0x1)  != 0x1);
}

    /*PIR:phy initialization register.
     *[0]-trigger init routine
     *[1]-Dll soft reset
     *[2]-Dll lock
     *[3]-Impendence calibration
     *[4]-Interface timing module soft reset
     *[5]*DRAM reset (DDR3 only)
     *[6]-DRAM initialization
     *[7]-QSTRN,read DQS training
     *[8]-EYETRN,Read data eye training,(Not supported in PUBL)
    */
    reg_bits_set(PUBL_PIR,  0,32, 0x41);/*trigger and do dram init*/
    DELAY_CLK(100);
    while(!(UMCTL2_REG_GET(PUBL_PGSR)&BIT_0));/*wait dram init done*/

#if 1
    dqs_manual_training(0x00);
#else
    /*use build-in DQS training,FPGA not support*/
    reg_bits_set(PUBL_PIR,  0,32, 0x81);/*trigger and do read DQS training*/
    DELAY_CLK(100);
    while(!(UMCTL2_REG_GET(PUBL_PGSR)&BIT_4));/*wait data training done*/    
#endif
    /*!!PHY initialization complete!!*/
}

/*
 *configure DATx8 register and its timing.
 *xiaohui.beijing
*/
void publ_datx8_config(DRAM_INFO* dram) {
    uint8 dram_type = dram->dram_type;
    /*datx8 common configuration register (DXCCR)*/
    /*[1]:*/
    reg_bits_set(PUBL_DXCCR,  1, 1, (IS_LPDDR2(dram_type)?0x00:0x01) );
    /*[7:4]:*/
    reg_bits_set(PUBL_DXCCR,  4, 4, (IS_LPDDR2(dram_type)?0x04:0x00)|
                                    (IS_LPDDR1(dram_type)?0x04:0x00));
    /*[11:8]:*/
    reg_bits_set(PUBL_DXCCR,  8, 4, (IS_LPDDR2(dram_type)?0x0C:0x00)|
                                    (IS_LPDDR1(dram_type)?0x0C:0x00));
    /*[14]:*/
    reg_bits_set(PUBL_DXCCR, 14, 1, (IS_LPDDR2(dram_type)?0x00:0x01) );


    /*DXxGCR[10:9]:disable DQ/DQS dynamic RTT controll*/
    reg_bits_set(PUBL_DX0GCR,  9, 2, 0x00);
    reg_bits_set(PUBL_DX1GCR,  9, 2, 0x00);
    reg_bits_set(PUBL_DX2GCR,  9, 2, 0x00);
    reg_bits_set(PUBL_DX3GCR,  9, 2, 0x00);

    reg_bits_set(PUBL_DX0DQTR,  0, 5, 0xf);
    reg_bits_set(PUBL_DX1DQTR,  0, 5, 0xf);
    reg_bits_set(PUBL_DX2DQTR,  0, 5, 0xf);
    reg_bits_set(PUBL_DX3DQTR,  0, 5, 0xf);
}
#endif
/*
 *Use umctl controller to initialize Mode register in sdram.
 *Refer to PUBL databook Chapter 3.3.13(MR0)~3.3.16(MR3) for detail.
*/



void sdram_clk_set(CLK_TYPE_E clock) 
{

    uint32 reg_val = 0;

    //set divide
    reg_val = UMCTL2_REG_GET(0x402E0018);
    reg_val &=~0x7ff;
    reg_val |= (clock>>2);
    UMCTL2_REG_SET(0x402E0018,reg_val);
    
    //select DPLL 533MHZ source clock
    reg_val = UMCTL2_REG_GET(0x402D0024);
    reg_val &= ~0x3;
//    reg_val |= 0; //default:XTL_26M
//    reg_val |= 1; //TDPLL_256M
//    reg_val |= 2; //TDPLL_384M
    reg_val |= 3; //DPLL_533M
    UMCTL2_REG_SET(0x402D0024,reg_val);
}

void umctl2_reg_init(CLK_TYPE_E umctl2_clk,umctl2_port_info_t* port_info,DRAM_INFO* dram)
{
    umctl2_basic_mode_init(dram);
    umctl2_poweron_init(dram,umctl2_clk);
    umctl2_dramtiming_init(dram,umctl2_clk);    
    umctl2_addrmap_init(dram);

    umctl2_refresh_init(dram);
    umctl2_zqctl_init(dram);
    umctl2_dfi_init(dram);
    umctl2_odt_init(dram);    
    umctl2_performance_init(dram);    
    umctl2_low_power_init();
    umctl2_trainctl_init();
    umctl2_tderate_init(dram,umctl2_clk);
    
    umctl2_port_init(port_info);
}
void wait_pclk(uint32 n_pclk)
{
    volatile uint32 i;
    volatile value;

    for(i = 0; i < n_pclk; i++)
    {
        value = REG32(PUBL_PGSR);
    }
    value = value;
}

void publ_basic_mode_init(DRAM_INFO* dram)
{
    
    volatile uint32 temp = 0;
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 bl = dram->bl;
    uint32 cs_num = dram->cs_num;

    //ZQ0CR0
    if(IS_LPDDR1(dram_type))
    {
        //when lpddr1,zq power down, override,0xc: 48ohm typical,refer to P155 of multiPHY databook 
        UMCTL2_REG_SET(PUBL_ZQ0CR0, (1<<31)|(1<<28)|(UMCTL2_LPDDR1_MEM_DS<<5)|(UMCTL2_LPDDR1_MEM_DS)); 
		UMCTL2_REG_SET(PUBL_ZQ1CR0, (1<<31)|(1<<28)|(UMCTL2_LPDDR1_MEM_DS<<5)|(UMCTL2_LPDDR1_MEM_DS)); 
    }

    //ZQ0CR1
    //reg_bits_set(PUBL_ZQ0CR1,4,4,0); //on-die termination driver strength, only support in DDR3, to be confirm
    reg_bits_set(PUBL_ZQ0CR1,0,4,UMCTL2_LPDDR2_MEM_DS); //lpddr2 driver strength
    //reg_bits_set(PUBL_ZQ1CR1,4,4,0); //on-die termination driver strength, only support in DDR3, to be confirm
    reg_bits_set(PUBL_ZQ1CR1,0,4,UMCTL2_LPDDR2_MEM_DS); //lpddr2 driver strength
    
    
    //PGCR
    reg_bits_set(PUBL_PGCR,0,1,IS_LPDDR1(dram_type)?1:0);//0:dqs and dqs# 1:dqs only
    reg_bits_set(PUBL_PGCR,1,1,1); //0:active windows 1:passive windows
    reg_bits_set(PUBL_PGCR,2,1,0); //data strobe drift compensation 0:disable 1:enable
    reg_bits_set(PUBL_PGCR,18,4,(cs_num==1)?1:3); 
    

    //DXnDLLCR
    reg_bits_set(PUBL_DX0DLLCR,14,4,B0_SDLL_PHS_DLY);
    reg_bits_set(PUBL_DX1DLLCR,14,4,B1_SDLL_PHS_DLY);
    reg_bits_set(PUBL_DX2DLLCR,14,4,B2_SDLL_PHS_DLY);
    reg_bits_set(PUBL_DX3DLLCR,14,4,B3_SDLL_PHS_DLY);
    
    //DXnDQSTR
    reg_bits_set(PUBL_DX0DQSTR,20,3,B0_DQS_STEP_DLY);
    reg_bits_set(PUBL_DX0DQSTR,23,3,B0_DQS_STEP_DLY);
    reg_bits_set(PUBL_DX1DQSTR,20,3,B1_DQS_STEP_DLY);
    reg_bits_set(PUBL_DX1DQSTR,23,3,B1_DQS_STEP_DLY);
    reg_bits_set(PUBL_DX2DQSTR,20,3,B2_DQS_STEP_DLY);
    reg_bits_set(PUBL_DX2DQSTR,23,3,B2_DQS_STEP_DLY);
    reg_bits_set(PUBL_DX3DQSTR,20,3,B3_DQS_STEP_DLY);    
    reg_bits_set(PUBL_DX3DQSTR,23,3,B3_DQS_STEP_DLY);        
    
    //DLLGCR
    //???don't need to set

    //ACDLLCR
    
    //ACIOCR
    reg_bits_set(PUBL_ACIOCR,0,1,IS_LPDDR1(dram_type)?1:0);//ACIOM
        
    //DXCCR, DATX8 common configuration register,to set data io,qds pin mode and pullup/pulldown resister
    reg_bits_set(PUBL_DXCCR,14,1,IS_LPDDR1(dram_type)?1:0);//dqs# reset,see PUBL page61 for detials
    reg_bits_set(PUBL_DXCCR, 4,4,  DQS_PDU_RES); //dqs resistor, 0:open 1:688ohm 2:611ohm 3:550ohm 4:500ohm 5:458ohm 6:393ohm 7:344ohm
    reg_bits_set(PUBL_DXCCR, 8,4,8|DQS_PDU_RES); //dqs# resistor,0:open 1:688ohm 2:611ohm 3:550ohm 4:500ohm 5:458ohm 6:393ohm 7:344ohm    
    reg_bits_set(PUBL_DXCCR, 1,1,IS_LPDDR1(dram_type)?1:0);//iom,0:SSTL mode 1:CMOS mode

    //DSGCR
    reg_bits_set(PUBL_DSGCR,0,1,1);//PHY Update Request Enable
    reg_bits_set(PUBL_DSGCR,1,1,1);//Byte Disable Enable
    reg_bits_set(PUBL_DSGCR,2,1,0);//Impedance Update Enable
    reg_bits_set(PUBL_DSGCR,3,1,1);//Low Power I/O Power Down
    reg_bits_set(PUBL_DSGCR,4,1,1);//Low Power DLL Power Down
    reg_bits_set(PUBL_DSGCR,5,3,DQS_GATE_EARLY_LATE);//DQS Gate Extension
    reg_bits_set(PUBL_DSGCR,8,3,DQS_GATE_EARLY_LATE);//DQS Gate Early
       
    //DCR
    reg_bits_set(PUBL_DCR,0,3,IS_LPDDR1(dram_type)?0:0 | //ddr mode
                              IS_LPDDR2(dram_type)?4:0 |
                              IS_DDR3(dram_type)?3:0);
    reg_bits_set(PUBL_DCR,3,1,IS_LPDDR1(dram_type)?0:1); //ddr 8-bank
    reg_bits_set(PUBL_DCR,8,2,0); //DDR Type  0:lpddr2-s4 1:lpddr2-s2


    //DXnGCR,DATX8 General Configuration Register
    REG32(PUBL_DX0GCR) &= ~(0x3<<9);//disable DQ/DQS Dynamic RTT Control
    REG32(PUBL_DX1GCR) &= ~(0x3<<9);//disable DQ/DQS Dynamic RTT Control    
    REG32(PUBL_DX2GCR) &= ~(0x3<<9);//disable DQ/DQS Dynamic RTT Control    
    REG32(PUBL_DX3GCR) &= ~(0x3<<9);//disable DQ/DQS Dynamic RTT Control        

    REG32(PUBL_ODTCR) &= ~0xff00ff;//disable ODT both for read and write

    //if not lpddr1, trigger zqcl
    if(!IS_LPDDR1(dram_type))
    {
        //trigger zqcl
        wait_pclk(50);
        UMCTL2_REG_SET(PUBL_PIR,0X9); 
        wait_pclk(50);
        //wait trigger zqcl done
        do temp = UMCTL2_REG_GET(PUBL_PGSR);
        while((temp&0x1) == 0);
    }   

//#ifdef DLL_BYPASS
#if 0
	//dll bypass mode
	wait_pclk(50); 
    UMCTL2_REG_SET(PUBL_PIR,0x60001); 
    wait_pclk(50);    
    //wait done
    do temp = UMCTL2_REG_GET(PUBL_PGSR);
    while((temp&0x1) == 0);
#else	
    //select use umctl2 to issue mode register, not publ
    wait_pclk(50);
    UMCTL2_REG_SET(PUBL_PIR,0x40001); 
    wait_pclk(50);    
    //wait done
    do temp = UMCTL2_REG_GET(PUBL_PGSR);
    while((temp&0x1) == 0);
#endif	
}

void publ_timing_init(CLK_TYPE_E umctl2_clk,DRAM_INFO* dram)
{
    ddr3_timing_t*   ddr3_timing   = dram->ac_timing;
    lpddr2_timing_t* lpddr2_timing = dram->ac_timing;
    lpddr1_timing_t* lpddr1_timing = dram->ac_timing;    
    
    DRAM_TYPE_E dram_type = dram->dram_type;
    //PTR0
    reg_bits_set(PUBL_PTR0,0,6, ns_to_xclock(50, umctl2_clk));//tDLLSRST
    reg_bits_set(PUBL_PTR0,6,12,ns_to_xclock(5120, umctl2_clk));//tDLLLOCK

    //PTR1
    reg_bits_set(PUBL_PTR1,0,19,IS_DDR3(dram_type)?us_to_xclock(500, umctl2_clk):us_to_xclock(200, umctl2_clk));//tDINIT0
    reg_bits_set(PUBL_PTR1,19,8,IS_DDR3(dram_type)?ns_to_xclock(360, umctl2_clk):ns_to_xclock(100, umctl2_clk));//tDINIT1
    
    //PTR2
    reg_bits_set(PUBL_PTR2,0,17,IS_DDR3(dram_type)?us_to_xclock(200, umctl2_clk):us_to_xclock(11, umctl2_clk));//tDINIT2
    reg_bits_set(PUBL_PTR2,17,10,us_to_xclock(1, umctl2_clk));//tDINIT3

    //DTPR0, to be confirm by johnnywang    
    UMCTL2_REG_SET(PUBL_DTPR0,IS_LPDDR1(dram_type)?0x3088444a:0x36916a6d);


    //DTPR1
	/*
    reg_bits_set(PUBL_DTPR1,0,2,0);//ODT turn-on/turn-off delays (DDR2 only)
    reg_bits_set(PUBL_DTPR1,2,1,1);//Read to Write command delay
    reg_bits_set(PUBL_DTPR1,3,6,IS_LPDDR2(dram_type)?lpddr2_timing->tFAW:0 |
                                IS_DDR3(dram_type)  ?  ddr3_timing->tFAW:0);//tFAW
    reg_bits_set(PUBL_DTPR1,9,2,IS_DDR3(dram_type)?ddr3_timing->tMOD:0);    //tMOD
    reg_bits_set(PUBL_DTPR1,11,1,0);//Read to ODT delay (DDR3 only) 
    reg_bits_set(PUBL_DTPR1,16,8,IS_LPDDR2(dram_type)?lpddr2_timing->tRFCab:0 |
                                 IS_LPDDR1(dram_type)?lpddr1_timing->tRFC:0 |
                                 IS_DDR3(dram_type)  ?ddr3_timing->tRFC:0);//
    reg_bits_set(PUBL_DTPR1,24,3,IS_LPDDR2(dram_type)?lpddr2_timing->tDQSCK:0);//tDQSCK
    reg_bits_set(PUBL_DTPR1,27,3,IS_LPDDR2(dram_type)?lpddr2_timing->tDQSCKmax:0);//tDQSCKmax
	*/
	UMCTL2_REG_SET(PUBL_DTPR1,0x193400A0);
    //DTPR2, to set tXS,tXP,tCKE,tDLLK //to be confirm by johnnywang
    //only used in PUBL DCU unite,I suppose
    //don't need to set,use the default value    
}

void publ_mdr_init(CLK_TYPE_E umctl2_clk,DRAM_INFO* dram)
{
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint8 bl = dram->bl;
    uint8 cl = dram->rl;
    uint8 rl = dram->rl;
    uint8 wl = dram->wl;
	uint8 tWR = 0;

    lpddr2_timing_t* lpddr2_timing = dram->ac_timing;
	tWR = lpddr2_timing->tWR;
    
    if(IS_LPDDR1(dram_type))
    {
        //MR
        reg_bits_set(PUBL_MR0,0,3,(bl==4)?2:3);//bl
        reg_bits_set(PUBL_MR0,4,3,cl);//cl
        reg_bits_set(PUBL_MR0,7,1,0); //operating mode (0) or test mode (1)

        //EMR
        UMCTL2_REG_SET(PUBL_MR2,0);
    }
    else if(IS_LPDDR2(dram_type))
    {
        //MR1
        reg_bits_set(PUBL_MR1,0,3,(bl==4)?2:3);//bl
        reg_bits_set(PUBL_MR1,3,1,MEM_BURST_TYPE);//Burst Type 0:sequential 1:interleaved
		#if 0
        reg_bits_set(PUBL_MR1,4,1,1);//0:wrap 1:no wrap
		#else
		reg_bits_set(PUBL_MR1,4,1,0);//0:wrap 1:no wrap		
		#endif
        reg_bits_set(PUBL_MR1,5,3,((tWR==3)?1:0) |
                                  ((tWR==4)?2:0) |
                                  ((tWR==5)?3:0) |
                                  ((tWR==6)?4:0) |
                                  ((tWR==7)?5:0) |
                                  ((tWR==8)?6:0) );//tWR

        //MR2
        UMCTL2_REG_SET(PUBL_MR2,(rl==3)&&(wl==1)?1:0 |
                                (rl==4)&&(wl==2)?2:0 |
                                (rl==5)&&(wl==2)?3:0 |
                                (rl==6)&&(wl==3)?4:0 |
                                (rl==7)&&(wl==4)?5:0 |
                                (rl==8)&&(wl==4)?6:0);
        UMCTL2_REG_SET(PUBL_MR3,(MEM_IO_DS==DS_34R3)?1:0 |
                                (MEM_IO_DS==DS_40R)?2:0 |
                                (MEM_IO_DS==DS_48R)?3:0 |
                                (MEM_IO_DS==DS_60R)?4:0 |
                                (MEM_IO_DS==DS_80R)?6:0);
    }
    else //DDR3
    {
        return;//to be confirm
    }	
}

void publ_reg_init(CLK_TYPE_E umctl2_clk,DRAM_INFO* dram)
{
    publ_basic_mode_init(dram);
    publ_timing_init(umctl2_clk,dram);
    publ_mdr_init(umctl2_clk, dram);
}

BOOLEAN publ_do_training()
{
	uint32 reg_val = 0;
	
	//do dqs training
	wait_pclk(50);	
	REG32(PUBL_PIR) |= 0x101; //do dqs gate training
	wait_pclk(50);	

    //wait training done
    do reg_val = UMCTL2_REG_GET(PUBL_PGSR);
    while( (!reg_val&BIT_0) || (!reg_val&BIT_5));

    //check if training success or fail
	if(reg_val&BIT_8||reg_val&BIT_9)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}		
}

/**---------------------------------------------------------------------------*
 **                            PUBLIC Functions
 **---------------------------------------------------------------------------*/
/*
 *Init the sdram using uMCTL and PUBL.
 *Refer to uMCTL2 user guide Chapter3.1.5 Table3-2
 *"DWC_ddr_umctl2 and memory initialization with PUBL"
*/
static BOOLEAN __sdram_init(CLK_TYPE_E dmc_clk,umctl2_port_info_t* port_info,DRAM_INFO* dram) 
{
/*NOTE:
 *Ensure that initializing all APB registers in reset mode,except for Dynamic Registers.
 */
    sdram_clk_set(dmc_clk);

    //enable umctl,publ,phy
    umctl2_ctl_en(TRUE);
    
    //to assert umctl reset,in order to prevent umctl issue mode register cmd to SDRAM automatically
    umctl2_soft_reset(TRUE);

    //to dis-assert to not prevent sdram init
    UMCTL2_REG_SET(UMCTL_DFIMISC, 0x0);

    //to wait umctl2 in init status before we excute sdram init
    umctl2_wait_status(OPERATION_MODE_INIT);

    //do umctl2 init
    umctl2_reg_init(dmc_clk, port_info, dram);

    //release umctl2 reset
    umctl2_soft_reset(FALSE);

    //do publ init
    publ_reg_init(dmc_clk,dram);

    //set dfi_init_complete_en, to trigger umctl2 issue DRAM power on and mode register set sequence
    UMCTL2_REG_SET(UMCTL_DFIMISC, 0x1);

    //wait umctl2 until NORMAL state
    umctl2_wait_status(OPERATION_MODE_NORMAL); 

    //enable AP port
    umctl2_port_en(UMCTL2_PORT_AP,TRUE);

    //do training
    if(!publ_do_training())
    {
        return FALSE;
    }    

    //enable all port
    umctl2_allport_en();

    return TRUE;
}
extern umctl2_port_info_t UMCTL2_PORT_CONFIG[];

void sdram_init()
{
    *(volatile unsigned int *)0x40038968 = VOLTAGE;//modify dcdc mem voltage.

    __sdram_init(DMC_CLK, UMCTL2_PORT_CONFIG, SDRAM_GetCfg());
}
