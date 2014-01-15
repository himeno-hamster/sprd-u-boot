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
#include "asm/arch/sprd_reg_base.h"


#if defined(CONFIG_SPX15)
#include "asm/arch/sprd_reg.h"
#if defined(CONFIG_CLK_PARA)
#include <asm/arch/clk_para_config.h>
#endif
#endif
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
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SHARK_CHIP_ID_ADDR 0x402E00FC
#define IS_SHARK_CS  ((REG32(SHARK_CHIP_ID_ADDR) == 0)?0:1)
#define SHARK_DDR_CTL_EB_ADDR 0x402B00C8
#define SHARK_DDR_CTL_CLK_DIV_ADDR 0x402E0018
#define SHARK_DDR_CTL_CLK_SEL_ADDR 0x402D0024

/**---------------------------------------------------------------------------*
 **                            Extern Declare
 **---------------------------------------------------------------------------*/
//extern MEM_IODS_E       MEM_IO_DS;
extern DRAM_BURSTTYPE_E MEM_BURST_TYPE;
extern DRAM_WC_E        MEM_WC_TYPE;


extern uint32 SDRAM_BASE;
extern uint32 DQS_PDU_RES;	//dqs pull up and pull down resist
extern uint32 DQS_GATE_EARLY_LATE; 
       
extern uint32 B0_SDLL_PHS_DLY;
extern uint32 B1_SDLL_PHS_DLY;
extern uint32 B2_SDLL_PHS_DLY;
extern uint32 B3_SDLL_PHS_DLY;
    
extern uint32 B0_DQS_STEP_DLY;
extern uint32 B1_DQS_STEP_DLY;
extern uint32 B2_DQS_STEP_DLY;
extern uint32 B3_DQS_STEP_DLY;
extern lpddr2_timing_t LPDDR2_ACTIMING_NATIVE;
extern ddr3_timing_t DDR3_ACTIMING_NATIVE;
static DRAM_TYPE_E DDR_TYPE_LOCAL;
/**---------------------------------------------------------------------------*
 **                            Local Variables
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Local Functions
 **---------------------------------------------------------------------------*/

uint32 reg_bits_set(uint32 addr, 
                           uint32 start_bitpos,
                           uint32 bit_num,
                           uint32 value)
{
    /*create bit mask according to input param*/
    uint32 bit_mask = (1<<bit_num)-1;
    uint32 reg_data = *((volatile uint32*)(addr));
    
    reg_data &= ~(bit_mask<<start_bitpos);
    reg_data |= ((value&bit_mask)<<start_bitpos);
    
    *((volatile uint32*)(addr)) = reg_data;
    return 0;
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

static void wait_40ns(uint32 ns_40)
{
	volatile uint32 i;
	volatile value;

	for(i = 0; i < ns_40; i++)
	{
		value = REG32(PUBL_PGSR);
	}
	value = value;
}
static void wait_us(uint32 us)
{
	uint32 i = 0;
	for(i = 0; i < us; i++)\
	{
		wait_40ns(25);
	}
}

static void __mem_init(MEM_CMD_E wr_rd, MEM_MR_E mr_addr, uint32 mr_data, MEM_CS_E cs)
{
	uint32 reg_val = 0;
	uint32 cs_id = 0;

	switch(cs)
	{
		case MEM_CS0 : cs_id = 1;break;
		case MEM_CS1 : cs_id = 2;break;
		default : cs_id = 1;break;
	}

	//wait if mode register busy
	while(REG32(UMCTL_MRSTAT));

	//send mode register command
	reg_val = (mr_addr<<8)|mr_data;
	REG32(UMCTL_MRCTRL1) = reg_val;

	reg_val = (1<<31) 		|
		      (cs_id<<4)	|
		      (wr_rd);
	REG32(UMCTL_MRCTRL0) = reg_val;

	//wait if mode register busy
	while(REG32(UMCTL_MRSTAT));

	//wait some time
	wait_us(100);
}


	
void umctl2_ctl_en(BOOLEAN is_en)
{
    uint32 reg_value=0,i;

    if(is_en) 
    {
        // Assert soft reset
        reg_value =   UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
        reg_value |=  0x07;
        UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
    else 
    {
        reg_value =   UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
        reg_value &=  ~0x07;
        UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
}
void umctl2_soft_reset(BOOLEAN is_en) 
{
	  uint32 reg_value=0, i=0;
#if defined(CONFIG_SPX15)
    if(is_en) 
    {
        reg_value =   UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
		reg_value |=   (0x1f00);
        UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_value);

        reg_value = UMCTL2_REG_GET(REG_PMU_APB_DDR_PHY_RET_CFG);
        reg_value |= (1<<16);
        UMCTL2_REG_SET(REG_PMU_APB_DDR_PHY_RET_CFG,reg_value);
        for(i = 0; i <= 1000; i++);
    } 
    else 
    {
        reg_value =   UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
        reg_value &=  ~(0x1f00);
        UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_value);
        reg_value = UMCTL2_REG_GET(REG_PMU_APB_DDR_PHY_RET_CFG);
        reg_value &= ~(1<<16);
        UMCTL2_REG_SET(REG_PMU_APB_DDR_PHY_RET_CFG,reg_value);
        for(i = 0; i <= 1000; i++);
    }
#else
    /*soft reset for uMCTL2 in user interface.*/
    if(is_en) 
    {
        // Assert soft reset
        reg_value =   UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
		reg_value |=   (0x1f00);
        UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
    else 
    {
        reg_value =   UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
        reg_value &=  ~(0x1f00);
        UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_value);
        for(i = 0; i <= 1000; i++);
    } 
#endif 
}

void umctl2_publ_reg_open()
{
	uint32 reg_val = 0;

	reg_val   = UMCTL2_REG_GET(SHARK_DDR_CTL_EB_ADDR);
	reg_val  &= ~(0x1d00);
	UMCTL2_REG_SET(SHARK_DDR_CTL_EB_ADDR, reg_val);
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
	return FALSE;
}

BOOLEAN umctl2_low_pd_set(UMCTL_LP_E auto_sf,
	                          UMCTL_LP_E auto_pd,
	                          UMCTL_LP_E auto_dpd,
	                          UMCTL_LP_E auto_ckp)
{
    //PWRTMG
    //REG32(UMCTL_PWRTMG) = 0x1f;
    //PWRCTL
    reg_bits_set(UMCTL_PWRCTL,0, 1, auto_sf);//auto self-refresh
    reg_bits_set(UMCTL_PWRCTL,1, 1, auto_pd);//auto power down
    reg_bits_set(UMCTL_PWRCTL,2, 1, auto_dpd);//auto deep power down
    reg_bits_set(UMCTL_PWRCTL,3, 1, auto_ckp); //en_dfi_dram_clk_disable
    return FALSE;
}

void umctl2_low_power_open()
{
	wait_pclk(50);
	/*
    umctl2_low_pd_set(UMCTL_AUTO_SF_DIS,
                      UMCTL_AUTO_PD_DIS,
                      UMCTL_AUTO_DPD_DIS,
                      UMCTL_AUTO_CKP_EN);
	*/
	
	UMCTL2_REG_SET(UMCTL_DFILPCFG0, 0x0700f000); /*DFI LP setting*/
	wait_pclk(50);
	
	//UMCTL2_REG_SET(PUBL_PIR, 0x40010);/*auto trigger ITM reset*/
	reg_bits_set(PUBL_PIR,4,1,1);/*auto trigger ITM reset*/
	wait_pclk(50);
	
	umctl2_port_auto_gate();
	umctl2_ctl_auto_gate();
	wait_pclk(50);

	#if 1
	//for kevin
	if(IS_SHARK_CS)
	{
		//reg_bits_set(UMCTL_HWLPCTL,16,12,0X40);//hardware idle period
		reg_bits_set(UMCTL_HWLPCTL,16,12,0);//hardware idle period
	}
	#endif
	
	#ifdef DDR_DDR3
        umctl2_low_pd_set(UMCTL_AUTO_SF_DIS,
                          UMCTL_AUTO_PD_EN,
                          UMCTL_AUTO_DPD_DIS,
                          UMCTL_AUTO_CKP_DIS);
	#else
        umctl2_low_pd_set(UMCTL_AUTO_SF_DIS,
                          UMCTL_AUTO_PD_EN,
                          UMCTL_AUTO_DPD_DIS,
                          UMCTL_AUTO_CKP_EN);
	#endif
	wait_pclk(50);
	

}

void umctl2_basic_mode_init(DRAM_INFO* dram) 
{
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint32 BL = dram->bl;
    uint32 CL = dram->rl;
    uint32 RL = dram->rl;
    uint32 WL = dram->wl;
    uint32 ranks=dram->cs_num;
    uint32 width=dram->io_width;

    /* master register config */
    //to set rank/cs number
    /*
    reg_bits_set(UMCTL_MSTR, 24, 4, ((ranks==1)?0x01:0)|
                                    ((ranks==2)?0x03:0)|
                                    ((ranks==3)?0x07:0)|
                                    ((ranks==4)?0x0F:0));
	*/
	reg_bits_set(UMCTL_MSTR, 24, 4, 3);//both  one cs and two cs lpddr2 should set this bit, the real reaseon should check by asic
	
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
    reg_bits_set(UMCTL_MSTR,  0, 4, (IS_DDR3(dram_type)?0x01:0x00) |
                                    (IS_LPDDR1(dram_type)?0x02:0x00) |
                                    (IS_LPDDR2(dram_type)?0x04:0x00));

    UMCTL2_REG_SET(UMCTL_SCHED,0x00071501);

    /*Only present for multi-rank configurations.
    *[11:8]:rank_wr_gap,clks of gap in data responses when performing consecutive writes to different ranks.
    *[07:4]:rank_rd_gap,clks of gap in data responses when performing consecutive reads to different ranks.
    *[03:0]:max_rank_rd,This param represents the max number of 64B reads(or 32B in some short read cases)
    *       that can bescheduled consecutively to the same rank.
    */
    
	// to be confirm by johnnywang
	reg_bits_set(UMCTL_DIMMCTL,  0, 1, 1); //stagger_cs_en

    // to be confirm by johnnywang
    reg_bits_set(UMCTL_RANKCTL,  0, 32,(0x06<<8)|(0x06<<4)|(0x03<<0));
//    reg_bits_set(UMCTL_RANKCTL,  0, 32,(0x0f<<8)|(0x0f<<4)|(0x03<<0));                                    
}

void umctl2_odt_init(DRAM_INFO* dram) 
{
    uint32 bl = dram->bl;
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint32 cl = dram->rl;
    uint32 cwl = dram->wl;
    
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
    uint32 rl = dram->rl;
    uint32 wl = dram->wl;
    uint32 cs_num = dram->cs_num;
    DRAM_TYPE_E dram_type = dram->dram_type;
    
    //to be confirm by johnnywang
    //DFITMG0
    reg_bits_set(UMCTL_DFITMG0, 24, 5, (cs_num==1)?2:3); //dfi_t_ctrl_delay, SDR and HDR mode =2,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 23, 1, 0); //dfi_rddata_use_sdr, 0:in terms of HDR cycles ??? I think should be 1!!!
                                           //                    1:in terms of SDR cycles
    #if (defined(DDR_LPDDR2)||defined(DDR_LPDDR1))
    reg_bits_set(UMCTL_DFITMG0, 16, 5, rl-1);//dfi_t_ctrl_delay, rl-1,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 8, 5, 1);  //dfi_tphy_wrdata, SDR and HDR mode =1,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 0, 5, wl); //dfi_tphy_wrlat, wl,refer PUBL spec p143
	#else
    reg_bits_set(UMCTL_DFITMG0, 16, 5, rl-2);//dfi_t_ctrl_delay, refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 8, 5, 1);  //dfi_tphy_wrdata, SDR and HDR mode =1,refer PUBL spec p143
    reg_bits_set(UMCTL_DFITMG0, 0, 5, wl-1); //dfi_tphy_wrlat, wl,refer PUBL spec p143
	#endif

    //DFITMG1
    reg_bits_set(UMCTL_DFITMG1, 16, 5, wl); //dfi_t_wrdata_delay, tphy_wrlat + (delay of DFI write data to the DRAM)???
    reg_bits_set(UMCTL_DFITMG1,  8, 4, 2);  //dfi_t_dram_clk_disable,number of DFI clock cycles from the assertion of the 
                                            //                      dfi_dram_clk_disable signal on the DFI until the clock 
                                            //                      to the DRAM memory devices
    reg_bits_set(UMCTL_DFITMG1,  0, 4, 2);  //dfi_t_dram_clk_enable,number of DFI clock cycles from the de-assertion of the
                                            //                      dfi_dram_clk_disable signal on the DFI until the first 
                                            //                      valid rising edge of the clock to the DRAM memory devices

    //DFILPCFG0
    #if 0
    reg_bits_set(UMCTL_DFILPCFG0,24,4,7);//dfi_tlp_resp
    reg_bits_set(UMCTL_DFILPCFG0,20,4,8);//dfi_lp_wakeup_dpd
    reg_bits_set(UMCTL_DFILPCFG0,16,1,IS_DDR3(dram_type)?0:1);//dfi_lp_en_dpd
    reg_bits_set(UMCTL_DFILPCFG0,12,4,3);//dfi_lp_wakeup_sr
    reg_bits_set(UMCTL_DFILPCFG0, 8,1,1);//dfi_lp_en_sr
    reg_bits_set(UMCTL_DFILPCFG0, 4,4,2);//dfi_lp_wakeup_pd
    reg_bits_set(UMCTL_DFILPCFG0, 0,1,0);//dfi_lp_en_pd
    #endif
    UMCTL2_REG_SET(UMCTL_DFILPCFG0,0X0700F000);

    //DFIUPD0
    UMCTL2_REG_SET(UMCTL_DFIUPD0, 0X80400003);
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
        case DRAM_LPDDR2_1CS_2G_X32:
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0X00001F1F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0X00060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0X00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0X0F0F0000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0X00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0X05050505);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0X0F0F0505);
            break;
            
        }
        case DRAM_LPDDR2_2CS_2G_X32 :
        {
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0X00001F13);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0X00060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0X00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0X0F0F0000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0X00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0X05050505);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0X0F0F0505);
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
	{
	    UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F1F);
	    UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
	    UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
	    UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
	    UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
	    UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
	    UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F0F0606);
	    break;
	}
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
	#if 0
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
	#endif
	case DRAM_DDR3_1CS_2G_X8_4P:
	{
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F1F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x06060606);
            break;
	}
	case DRAM_DDR3_1CS_4G_X16_2P:
	{
            UMCTL2_REG_SET(UMCTL_ADDRMAP0, 0x00001F1F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP1, 0x00070707);
            UMCTL2_REG_SET(UMCTL_ADDRMAP2, 0x00000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP3, 0x0F000000);
            UMCTL2_REG_SET(UMCTL_ADDRMAP4, 0x00000F0F);
            UMCTL2_REG_SET(UMCTL_ADDRMAP5, 0x06060606);
            UMCTL2_REG_SET(UMCTL_ADDRMAP6, 0x0F060606);
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
    UMCTL2_REG_SET(UMCTL_PERFHPR0, 0x00000001);
    UMCTL2_REG_SET(UMCTL_PERFHPR1, 0x10000001);
    UMCTL2_REG_SET(UMCTL_PERFLPR0, 0x00000100);
    UMCTL2_REG_SET(UMCTL_PERFLPR1, 0x10000100);
    UMCTL2_REG_SET(UMCTL_PERFWR0,  0x00000020);
    UMCTL2_REG_SET(UMCTL_PERFWR1,  0x10000020);
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
	
	reg_bits_set(UMCTL_RFSHTMG,16,12,(IS_LPDDR2(dram_type)?(lpddr2_timing->tREFI>>5):(ddr3_timing->tREFI>>5)));

	reg_bits_set(UMCTL_RFSHTMG,0,9,(IS_LPDDR2(dram_type)?lpddr2_timing->tRFCab:0) |
								  (IS_DDR3(dram_type)?ddr3_timing->tRFC:0) );
									
}
void umctl2_port_en(UMCTL2_PORT_ID_E port_id,BOOLEAN en)
{  
	#if defined(CONFIG_SPX15)
	UMCTL2_REG_SET(UMCTL_PORT_EN_0+port_id*0xb0,en);    //prot enable
	#else
	if(!IS_SHARK_CS)
	{
		return;
	}
	else
	{
    	UMCTL2_REG_SET(UMCTL_PORT_EN_0+port_id*0xb0,en);    //prot enable
	}	
	#endif
}

void umctl2_allport_en()
{
    uint32 i = 0;
	
    for(i = UMCTL2_PORT_MIN; i < UMCTL2_PORT_MAX; i++)
    {
        umctl2_port_en(i,TRUE);
    }
}

void umctl2_port_auto_gate()
{
	#if defined(CONFIG_SPX15)
        REG32(0x402B00F0) = 0X83ff03FF;
        #else
        //for kevin
	REG32(0x402B00F0) = 0X3ff03FF;
	//REG32(0x402B00F0) = 0X3FF;
        #endif
}

void umctl2_ctl_auto_gate()
{
	if(!IS_SHARK_CS)
	{
		return;
	}
	else
	{
		//enable ddr phy,ctl,publ clock auto gate, only shark cs chip have
    	reg_bits_set(SHARK_DDR_CTL_EB_ADDR,4,3,7);    
		//disable ddr phy,ctl,publ clock force eb,because auto gate had been enable
		reg_bits_set(SHARK_DDR_CTL_EB_ADDR,0,3,0);    
	}		
}

void umctl2_port_init(umctl2_port_info_t* port_info)
{
    uint32 i = 0;
    reg_bits_set(UMCTL_PCCFG,4,1,TRUE);//pagematch_limit
    reg_bits_set(UMCTL_PCCFG,0,1,FALSE);//go2critical_en


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
    return 0;
}

/*
 *Refer to uMCTL2 databook Chapter5.4.45~5.4.53.
 *Set sdram timing parameters,refer to SDRAM spec for details.
*/
void umctl2_dramtiming_init(DRAM_INFO* dram,CLK_TYPE_E umctl2_clk) {
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint32 BL = dram->bl;
    uint32 CL = dram->rl;
    uint32 RL = dram->rl;
    uint32 WL = dram->wl;
    uint32 AL = dram->al;

    /*Get the timing we used.*/
    lpddr1_timing_t*  lpddr1_timing =(lpddr1_timing_t*)(dram->ac_timing);
    lpddr2_timing_t* lpddr2_timing =(lpddr2_timing_t*)(dram->ac_timing);
    DDR2_ACTIMING*   ddr2_timing = (DDR2_ACTIMING*)(dram->ac_timing);
    ddr3_timing_t*   ddr3_timing = (ddr3_timing_t*)(dram->ac_timing);

	#if 0
    uint32 tWR=(IS_LPDDR1(dram_type)?(lpddr1_timing->tWR):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tWR):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tWR):0x00) ;
    uint32 tCKESR=(IS_LPDDR2(dram_type)?(lpddr2_timing->tCKESR):0x00);
    uint32 tCKSRX=(IS_DDR3(dram_type)?(ddr3_timing->tCKSRX):0x00) ;
    uint32 tMOD=IS_DDR3(dram_type)?(ddr3_timing->tMOD):0x00;/*DDR3 only*/
    uint32 tMRD=(IS_LPDDR2(dram_type)?4:0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tMRD):0x00)|/*DDR3/2 only*/
               (IS_LPDDR1(dram_type)?(lpddr1_timing->tMRD):0x00) ;
    uint32 tRTP=(IS_LPDDR2(dram_type)?(lpddr2_timing->tRTP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRTP):0x00) ;
    uint32 tWTR=(IS_LPDDR1(dram_type)?(lpddr1_timing->tWTR):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tWTR):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tWTR):0x00) ;
    uint32 tRP =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRP):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRP):0x00) ;
    uint32 tRCD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRCD):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRCD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRCD):0x00) ;
    uint32 tRAS=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRAS):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRAS):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRAS):0x00) ;
    uint32 tRRD=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRRD):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRRD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRRD):0x00) ;
    uint32 tRC =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRC):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRC):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRC):0x00) ;
    uint32 tCCD=(IS_LPDDR2(dram_type)?(lpddr2_timing->tCCD):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tCCD):0x00) ; 
    uint32 tFAW=(IS_LPDDR2(dram_type)?(lpddr2_timing->tFAW):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tFAW):0x00) ;
    uint32 tRFC=(IS_LPDDR1(dram_type)?(lpddr1_timing->tRFC):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tRFCab):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tRFC):0x00) ;  
    uint32 tDQSCK=IS_LPDDR2(dram_type)?(lpddr2_timing->tDQSCK):0x00;/*LPDDR2 only*/
    uint32 tXS =(IS_DDR2(dram_type)?(ddr2_timing->tXS):200)|/*for DDR2/DDR3,default200*/
               (IS_DDR3(dram_type)?(ddr3_timing->tXS):200) ;
    uint32 tXP =(IS_LPDDR1(dram_type)?(lpddr1_timing->tXP):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tXP):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tXP):0x00) ;
    uint32 tCKE=(IS_LPDDR1(dram_type)?(lpddr1_timing->tCKE):0x00)|
               (IS_LPDDR2(dram_type)?(lpddr2_timing->tCKE):0x00)|
               (IS_DDR3(dram_type)?(ddr3_timing->tCKE):0x00) ;
    /*tDLLK:Dll locking time.Valid value are 2 to 1023*/
    uint32 tDLLK=IS_DDR3(dram_type)?(ddr3_timing->tDLLK):0x00;
    uint32  tDQSCKmax=IS_LPDDR2(dram_type)?(lpddr2_timing->tDQSCKmax):0x00;
    /*tAOND:ODT turnon/turnoff delays,DDR2 only*/
    uint32 tAOND=(IS_DDR2(dram_type)?(ddr2_timing->tAOND):0x00);
    /*tRTODT:Read to ODT delay,DDR3 only
     *0--ODT maybe turned on immediately after read post-amble
     *1--ODT maybe not turned on until one clock after read post-amble
    */
    uint32 tRTODT= 0x00;
    /*Get the timing we used.*/
	#else
	
	#if defined(DDR_LPDDR2)
	uint32 tWR= lpddr2_timing->tWR;
	uint32 tCKESR=lpddr2_timing->tCKESR;
	uint32 tCKSRX=0x00;
	uint32 tMOD=0x00;/*DDR3 only*/
	uint32 tMRD=4;
	uint32 tRTP=lpddr2_timing->tRTP;
	uint32 tWTR=lpddr2_timing->tWTR;
	uint32 tRP =lpddr2_timing->tRP;
	uint32 tRCD=lpddr2_timing->tRCD;
	uint32 tRAS=lpddr2_timing->tRAS;
	uint32 tRRD=lpddr2_timing->tRRD;
	uint32 tRC =lpddr2_timing->tRC;
	uint32 tCCD=lpddr2_timing->tCCD;
	uint32 tFAW=lpddr2_timing->tFAW;
	uint32 tRFC=lpddr2_timing->tRFCab;
	uint32 tDQSCK=lpddr2_timing->tDQSCK;/*LPDDR2 only*/
	uint32 tXS =200;/*for DDR2/DDR3,default200*/
	uint32 tXP =lpddr2_timing->tXP;
	uint32 tCKE=lpddr2_timing->tCKE;
	/*tDLLK:Dll locking time.Valid value are 2 to 1023*/
	uint32 tDLLK=0x00;
	uint32 tDQSCKmax=lpddr2_timing->tDQSCKmax;		
	#endif
	
	#if defined(DDR_DDR3)
	uint32 tWR=ddr3_timing->tWR;
	uint32 tCKESR=0x00;
	uint32 tCKSRX=ddr3_timing->tCKSRX;
	uint32 tMOD=ddr3_timing->tMOD;/*DDR3 only*/
	uint32 tMRD=ddr3_timing->tMRD;/*DDR3/2 only*/
	uint32 tRTP=ddr3_timing->tRTP;
	uint32 tWTR=ddr3_timing->tWTR;
	uint32 tRP =ddr3_timing->tRP;
	uint32 tRCD=ddr3_timing->tRCD;
	uint32 tRAS=ddr3_timing->tRAS;
	uint32 tRRD=ddr3_timing->tRRD;
	uint32 tRC =ddr3_timing->tRC;
	uint32 tCCD=ddr3_timing->tCCD; 
	uint32 tFAW=ddr3_timing->tFAW;
	uint32 tRFC=ddr3_timing->tRFC;
	uint32 tDQSCK=0x00;/*LPDDR2 only*/
	uint32 tXS =ddr3_timing->tXS;
	uint32 tXP =ddr3_timing->tXP;
	uint32 tCKE=ddr3_timing->tCKE;
	uint32 tDQSCKmax=ddr3_timing->tDQSCK;
	#endif
	
	#endif
#if 1
    reg_bits_set(UMCTL_DRAMTMG0, 24, 6, ((WL+(BL>>1)+tWR)+1));/*wr2pre*/
    reg_bits_set(UMCTL_DRAMTMG0, 16, 6, tFAW);    /*t_FAW*/
    reg_bits_set(UMCTL_DRAMTMG0,  8, 6, ns_to_x1024(70000,umctl2_clk));/*t_ras_max,Maxinum time of tRAS,clocks*/
    reg_bits_set(UMCTL_DRAMTMG0,  0, 6, tRAS);/*t_ras_min,Mininum time of tRAS,clocks*/

    reg_bits_set(UMCTL_DRAMTMG1, 16, 6, tXP);
    /*Minimun from read to precharge of same bank*/
    reg_bits_set(UMCTL_DRAMTMG1,  8,5,(IS_LPDDR2(dram_type)?((BL>>1)+MAX(tRTP,2)-2):(AL+MAX(tRTP,4))));

    reg_bits_set(UMCTL_DRAMTMG1,  0, 6, tRC); /*Active-to-Active command period*/

    reg_bits_set(UMCTL_DRAMTMG2, 24, 6, WL);
    reg_bits_set(UMCTL_DRAMTMG2, 16, 5, RL);
    /*Minimam time from read command to write command*/
    reg_bits_set(UMCTL_DRAMTMG2,  8, 5, IS_LPDDR2(dram_type)?(RL+(BL>>1)+tDQSCKmax+1-WL):(RL+(BL>>1)+4-WL));
    reg_bits_set(UMCTL_DRAMTMG2,  0, 6, (WL+(BL>>1)+tWTR));

    /*tMRW, time to wait during load mode register writes.*/
    reg_bits_set(UMCTL_DRAMTMG3, 20,10,0x05);
    reg_bits_set(UMCTL_DRAMTMG3, 12, 3, tMRD);
    reg_bits_set(UMCTL_DRAMTMG3,  0,10, tMOD);

    reg_bits_set(UMCTL_DRAMTMG4, 24, 5, tRCD);
    reg_bits_set(UMCTL_DRAMTMG4, 16, 3, tCCD);
    reg_bits_set(UMCTL_DRAMTMG4,  8, 4, tRRD);
    reg_bits_set(UMCTL_DRAMTMG4,  0, 5, tRP);

    /*tCKSRX,the time before SelfRefreshExit that CK is maintained as a valid clock.*/
    /*Specifies the clock stable time before SRX.*/
    reg_bits_set(UMCTL_DRAMTMG5, 24, 4,(IS_LPDDR2(dram_type)?0x02:tCKSRX));
    /*tCKSRE,the time after SelfRefreshDownEntry that CK is maintained as a valid clock.*/
    /*Specifies the clock disable delay after SRE.*/
    reg_bits_set(UMCTL_DRAMTMG5, 16, 4,(IS_LPDDR2(dram_type)?0x02:0x06));
    /*tCKESR,Minimum CKE low width for selfrefresh entry to exit timing in clock cycles.*/
    reg_bits_set(UMCTL_DRAMTMG5,  8, 6,(IS_LPDDR2(dram_type)?tCKESR:(tCKE+1)));
    /*tCKE,Minimum number of cycles of CKE HIGH/LOW during power-down and selfRefresh.*/
    reg_bits_set(UMCTL_DRAMTMG5,  0, 4, tCKE);
    
    /*tCKDPDE,time after DeepPowerDownEntry that CK is maintained as a valid clock.*/
    reg_bits_set(UMCTL_DRAMTMG6, 24, 4,0x02);
    /*tCKDPDX,time before DeepPowerDownExit that CK is maintained as a valid clock before issuing DPDX.*/
    reg_bits_set(UMCTL_DRAMTMG6, 16, 4,0x02);
    /*tCKCSX,time before ClockStopExit that CK is maintained as a valid clock before issuing DPDX.*/
    reg_bits_set(UMCTL_DRAMTMG6,  0, 4,(tXP+0x02));

    /*tCKPDE,time after PowerDownEntry that CK is maintained as a valid clock before issuing PDE.*/
    reg_bits_set(UMCTL_DRAMTMG7,  8, 4,0x02);
    /*tCKPDX,time before PowerDownExit that CK is maintained as a valid clock before issuing PDX.*/
    reg_bits_set(UMCTL_DRAMTMG7,  0, 4,0x02);

    /*post_selfref_gap_x32,time after coming out of selfref before doing anything.Default:0x44
    //reg_bits_set(MCTL_DRAMTMG8,  0, 4, max(tXSNR,max(tXSRD,tXSDLL)));*/
    reg_bits_set(UMCTL_DRAMTMG8,  0,7,IS_LPDDR2(dram_type)?0x2:0x10);
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

/*
 *Power-up timing initialization and Mode Register Setting for sdram.
*/
void umctl2_poweron_init(DRAM_INFO* dram,CLK_TYPE_E umctl2_clk) {
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint32 BL = dram->bl;
    uint32 CL = dram->rl;
    uint32 RL = dram->rl;
    uint32 WL = dram->wl;
    uint32 AL = dram->al;
    uint32 mr=0,emr=0,mr2=0,mr3=0;

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
    reg_bits_set(UMCTL_INIT0, 16,10,(IS_LPDDR2(dram_type)?us_to_x1024(200,umctl2_clk):ns_to_x1024(360,umctl2_clk)));
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
    reg_bits_set(UMCTL_INIT2,  0, 4,(IS_LPDDR2(dram_type)?tINIT2:0x05));

    /*Note:
     *    Set Mode register 0~3 for SDRAM memory.
     *    Refer to JESD spec for detail.
    */

    #if defined(DDR_LPDDR1)
    {
        /*mr store the value to write to MR register*/
        mr= (BL<<0)|/*burst_length:BL2/4/8/16*/
            (0x00<<3)|/*0:Sequentia;1:interleavel*/
            (CL<<4);/*cas_latency:2/3/4 optional*/
        /*emr store the value to write to EMR register*/
        emr=(0x00<<0)|/*PASR:0:AllBanks;1:HalfArray;2:QuarterArray,etc*/
            (0x00<<3)|/*TCSR:0:70'C;1:45'C;2:15'C;3:85'C;*/
            ((
             ((MEM_IO_DS==LPDDR1_DS_FULL)?0x00:0x00)|
             ((MEM_IO_DS==LPDDR1_DS_HALF)?0x01:0x00)|
             ((MEM_IO_DS==LPDDR1_DS_QUARTER)?0x02:0x00)|
             ((MEM_IO_DS==LPDDR1_DS_OCTANT)?0x03:0x00)|
             ((MEM_IO_DS==LPDDR1_DS_THREE_QUATERS)?0x04:0x00)
             )<<5);/*DS:0:FullDriverStrength;1:HalfDS;2:QuarterDS;3:OctantDS;4:Three-Quater*/
        
        /*mr2 unused for MDDR*/
        /*mr3 unused for MDDR/LPDDR2/LPDDR3*/
    }
	#endif
    #if defined(DDR_LPDDR2)
    {
		uint32 tWR  =lpddr2_timing->tWR;
		//tZQINIT = lpddr2_timing->tZQINIT;
		
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
            (MEM_WC_TYPE<<4)|/*0:Wrap(default);1:No wrap(allow for SDRAM BL4 only)*/
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
        mr2 = ((MEM_IO_DS==LPDDR2_DS_34R3)?0x01:0x00) |
              ((MEM_IO_DS==LPDDR2_DS_40R)?0x02:0x00) |
              ((MEM_IO_DS==LPDDR2_DS_48R)?0x03:0x00) |
              ((MEM_IO_DS==LPDDR2_DS_60R)?0x04:0x00) |
              ((MEM_IO_DS==LPDDR2_DS_68R6)?0x05:0x00) |
              ((MEM_IO_DS==LPDDR2_DS_80R)?0x06:0x00) |
              ((MEM_IO_DS==LPDDR2_DS_120R)?0x07:0x00) ;
        /*mr3 unused for MDDR/LPDDR2/LPDDR3*/
    }
	#endif
	#if defined(DDR_DDR3)
    {
        uint32 tWR  = (uint32)(ddr3_timing->tWR);
        uint32 CWL = (uint32)((WL-AL)?(WL-AL):0x00);
		//tZQINIT = lpddr2_timing->tZQINIT;
		
        /*mr store the value to write to MR1 register*/
        /*Set sequential burst-type with wrap*/
        mr= ((BL==8)?0x00:0x02)| /*Fixed on 8*/
            (MEM_BURST_TYPE<<3)|
            ((((CL==5)?0x01:0x00)| /*Bit[6:4],CAS Latency*/
              ((CL==6)?0x02:0x00)|
              ((CL==7)?0x03:0x00)|
              ((CL==8)?0x04:0x00)|
              ((CL==9)?0x05:0x00)|
              ((CL==10)?0x06:0x00)|
              ((CL==11)?0x07:0x00))<<4)|           
            ((((tWR<=5)?0x01:0x00)|/*Bit[11:9],Write recovery for autoprecharge*/
              ((tWR==6)?0x02:0x00)|
              ((tWR==7)?0x03:0x00)|
              ((tWR==8)?0x04:0x00)|
              ((tWR==10)?0x05:0x00)|
              ((tWR==12)?0x06:0x00)|
              ((tWR==14)?0x07:0x00)|
              ((tWR==16)?0x00:0x00))<<9)|
              (((DDR3_DLL_ON==FALSE)?0:1)<<12);
                             
        /*emr store the value to write to MR1 register*/
        /*A0:0-DLL Enable;1-DLL Disable*/
        reg_bits_set((uint32)&emr,  0, 1, ((DDR3_DLL_ON==TRUE)?0:1));
        /*Output Driver Impedance Control
         [A5,A1]:00-RZQ/6;01-RZQ/7;10/11-RZQ/TBD;Note: RZQ=240ohm*/
        reg_bits_set((uint32)&emr,  1, 1, (MEM_IO_DS==DDR3_DS_40R)?0:1);
        reg_bits_set((uint32)&emr,  5, 1, 0);
        /*[A4:A3]:Additive Latency.*/
        reg_bits_set((uint32)&emr,  3, 2, ((AL==0)?0x00:0x00)|
	                                      ((AL==CL-1)?0x01:0x00)|
	                                      ((AL==CL-2)?0x02:0x00)
	                                      );            
        /*[A7]:1-Write leveling enable;0-Disabled*/
        reg_bits_set((uint32)&emr,  7, 1, (DDR3_DLL_ON==TRUE)?0:1);

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
    #endif
    reg_bits_set(UMCTL_INIT3, 16,16, mr);
    reg_bits_set(UMCTL_INIT3,  0,16, emr);
    reg_bits_set(UMCTL_INIT4, 16,16, mr2);
    reg_bits_set(UMCTL_INIT4,  0,16, mr3);


    /*dev_zqinit_x32,ZQ initial calibration,tZQINIT.*/ //to be confirm by johnnywang
    reg_bits_set(UMCTL_INIT5, 16, 8,us_to_x32(tZQINIT,umctl2_clk));
    /*max_auto_init_x1024,max duration of the auto initialization,tINIT5.*/ // to be confirm by johnnywang
    reg_bits_set(UMCTL_INIT5,  0,10,(IS_LPDDR2(dram_type)?us_to_x1024(10,umctl2_clk):0x00));

}




void sdram_clk_set(CLK_TYPE_E clock) 
{

    uint32 reg_val = 0;
	uint32 volatile i = 0;

	//disable ddr_phy_eb
	reg_bits_set(SHARK_DDR_CTL_EB_ADDR,2,1,0);

	//disable address/command/data dll
	reg_bits_set(PUBL_ACDLLCR,30,1,0);
	reg_bits_set(PUBL_DX0DLLCR,30,1,0);
	reg_bits_set(PUBL_DX1DLLCR,30,1,0);
	reg_bits_set(PUBL_DX2DLLCR,30,1,0);
	reg_bits_set(PUBL_DX3DLLCR,30,1,0);	
#if defined(CONFIG_SPX15)
#if defined(CONFIG_CLK_PARA)
	extern void set_ddr_clk(uint32 ddr_clk);
	set_ddr_clk(clock);
#else
    //set divide
    reg_val = UMCTL2_REG_GET(SHARK_DDR_CTL_CLK_DIV_ADDR);
    reg_val &=~0x7ff;
	reg_val |= (clock>>2);
	UMCTL2_REG_SET(SHARK_DDR_CTL_CLK_DIV_ADDR,reg_val);
	reg_val = UMCTL2_REG_GET(SHARK_DDR_CTL_CLK_SEL_ADDR);
	reg_val &= ~0x3;
	reg_val |= 3; //DPLL_533M
	UMCTL2_REG_SET(SHARK_DDR_CTL_CLK_SEL_ADDR,reg_val);
#endif
#else
	reg_val = UMCTL2_REG_GET(SHARK_DDR_CTL_CLK_DIV_ADDR);
	reg_val &=~0x7ff;
	reg_val |= (clock>>2);
    UMCTL2_REG_SET(SHARK_DDR_CTL_CLK_DIV_ADDR,reg_val);

    //select DPLL 533MHZ source clock
    reg_val = UMCTL2_REG_GET(SHARK_DDR_CTL_CLK_SEL_ADDR);
    reg_val &= ~0x3;
//    reg_val |= 0; //default:XTL_26M
//    reg_val |= 1; //TDPLL_256M
//    reg_val |= 2; //TDPLL_384M
    reg_val |= 3; //DPLL_533M
    UMCTL2_REG_SET(SHARK_DDR_CTL_CLK_SEL_ADDR,reg_val);
#endif

	wait_pclk(5263);//publ apb clk=26m-38ns, 5263*38ns = 200us

	//enable ddr_phy_eb
	reg_bits_set(SHARK_DDR_CTL_EB_ADDR,2,1,1);
	
	//enable address/command/data dll
	reg_bits_set(PUBL_ACDLLCR,30,1,1);
	reg_bits_set(PUBL_DX0DLLCR,30,1,1);
	reg_bits_set(PUBL_DX1DLLCR,30,1,1);
	reg_bits_set(PUBL_DX2DLLCR,30,1,1);
	reg_bits_set(PUBL_DX3DLLCR,30,1,1);	
	wait_pclk(263);//publ apb clk=26m-38ns, 263*38ns = 10us

	
}

#if 0
void sdram_vddmem_set(VDDMEM_TYPE_E vddmem)
{
	UMCTL2_REG_SET(0x40038968,vddmem);
}
#endif

#define UART1_TX_BUF_ADDR 0X70100000
#define UART1_TX_BUF_CNT ((REG32(0x70100000 + 0xc)>>8)&0xff)
void ddr_print(const unsigned char *string)
{
    unsigned char *s1 = NULL;

    s1 = string;

    while (*s1 != NULL)
    {
		//wait until uart1 tx fifo empty
		while(UART1_TX_BUF_CNT != 0);

		//put out char by uart1 tx fifo
		REG32(UART1_TX_BUF_ADDR) = *s1;
		s1++;
	}
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

void publ_basic_mode_init(CLK_TYPE_E clk,DRAM_INFO* dram)
{
    
    volatile uint32 temp = 0;
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint32 bl = dram->bl;
    uint32 cs_num = dram->cs_num;

    //ZQ0CR0
    if(IS_LPDDR1(dram_type))
    {
        //when lpddr1,zq power down, override,0xc: 48ohm typical,refer to P155 of multiPHY databook 
        UMCTL2_REG_SET(PUBL_ZQ0CR0, (1<<31)|(1<<28)|(PUBL_LPDDR1_DS<<5)|(PUBL_LPDDR1_DS)); 
		UMCTL2_REG_SET(PUBL_ZQ1CR0, (1<<31)|(1<<28)|(PUBL_LPDDR1_DS<<5)|(PUBL_LPDDR1_DS)); 
    }

    //ZQ0CR1
    #if defined(DDR_LPDDR2)
    reg_bits_set(PUBL_ZQ0CR1,0,4,PUBL_LPDDR2_DS); //lpddr2 driver strength
    reg_bits_set(PUBL_ZQ1CR1,0,4,PUBL_LPDDR2_DS); //lpddr2 driver strength
    #endif

	#if defined(DDR_DDR3)
    reg_bits_set(PUBL_ZQ0CR1,0,4,PUBL_DDR3_DS); //ddr3 driver strength
    reg_bits_set(PUBL_ZQ1CR1,0,4,PUBL_DDR3_DS); //ddr3 driver strength	
	#endif
    
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
    if(clk >= CLK_200MHZ)
    {
    	reg_bits_set(PUBL_DLLGCR,23,1,1) ; //this bit can't find in publ spec,if 200MHz dllbypass, this bit23 must be set
    }
	else
	{
    	reg_bits_set(PUBL_DLLGCR,23,1,0) ; //this bit can't find in publ spec,if 100MHZ dllbypass, this bit23 must be clear
	}
 
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
    //reg_bits_set(PUBL_DSGCR,4,1,1);//Low Power DLL Power Down
    reg_bits_set(PUBL_DSGCR,4,1,0);//Low Power DLL Power Down
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

#ifdef DLL_BYPASS
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
	if(clk > CLK_200MHZ)
	{
	    UMCTL2_REG_SET(PUBL_PIR,0x40001);     
	}
	else
	{
		UMCTL2_REG_SET(PUBL_PIR,0x60001); 
	}
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
    reg_bits_set(PUBL_PTR0,0,6, ns_to_xclock(50,   DDR_APB_CLK));//tDLLSRST,reg apb clk,will change
    reg_bits_set(PUBL_PTR0,6,12,ns_to_xclock(5120, DDR_APB_CLK));//tDLLLOCK, reg apb clk, will change

    //PTR1
    reg_bits_set(PUBL_PTR1,0,19,IS_DDR3(dram_type)?us_to_xclock(500, umctl2_clk):us_to_xclock(200, umctl2_clk));//tDINIT0
    reg_bits_set(PUBL_PTR1,19,8,IS_DDR3(dram_type)?ns_to_xclock(360, umctl2_clk):ns_to_xclock(100, umctl2_clk));//tDINIT1
    
    //PTR2
    reg_bits_set(PUBL_PTR2,0,17,IS_DDR3(dram_type)?us_to_xclock(200, umctl2_clk):us_to_xclock(11, umctl2_clk));//tDINIT2
    reg_bits_set(PUBL_PTR2,17,10,us_to_xclock(1, umctl2_clk));//tDINIT3

    //DTPR0, to be confirm by johnnywang    
    UMCTL2_REG_SET(PUBL_DTPR0,IS_LPDDR1(dram_type)?0x3088444a:0x36916a6d);

	#if 0
    {
		uint32 tMRD =(IS_LPDDR1(dram_type)?(lpddr1_timing->tMRD):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tMRW):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tMRD):0x00);

		uint32 tRTP =(IS_LPDDR1(dram_type)?(dram->bl>>1):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tRTP):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tRTP):0x00);

		uint32 tWTR =(IS_LPDDR1(dram_type)?(lpddr1_timing->tWTR):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tWTR):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tWTR):0x00);

		uint32 tRP  =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRP):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tRP):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tRP):0x00);

		uint32 tRCD =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRCD):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tRCD):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tRCD):0x00);
		
		uint32 tRAS =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRAS):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tRAS):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tRAS):0x00);

		uint32 tRRD =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRRD):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tRRD):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tRRD):0x00);
		
		uint32 tRC  =(IS_LPDDR1(dram_type)?(lpddr1_timing->tRC):0x00)|
					(IS_LPDDR2(dram_type)?(lpddr2_timing->tRC):0x00)|
					(IS_DDR3(dram_type)?  (ddr3_timing->tRC):0x00);

		UMCTL2_REG_SET(PUBL_DTPR0,tMRD|(tRTP<<2)|(tWTR<<5)|(tRP<<8)|(tRCD<<12)|
			                      (tRAS<<16)|(tRRD<<21)|(tRC<<25)|(1<<31));
		
	}
	#endif


    //DTPR1	
	/*
    reg_bits_set(PUBL_DTPR1,0,2,0);//ODT turn-on/turn-off delays (DDR2 only)
    reg_bits_set(PUBL_DTPR1,2,1,1);//Read to Write command delay
    reg_bits_set(PUBL_DTPR1,3,6,IS_LPDDR2(dram_type)?lpddr2_timing->tFAW:0 |
                                IS_DDR3(dram_type)  ?  ddr3_timing->tFAW:0);//tFAW
    reg_bits_set(PUBL_DTPR1,9,2,IS_DDR3(dram_type)?(ddr3_timing->tMOD-12):0);    //tMOD
    reg_bits_set(PUBL_DTPR1,11,1,0);//Read to ODT delay (DDR3 only) 
    reg_bits_set(PUBL_DTPR1,16,8,IS_LPDDR2(dram_type)?lpddr2_timing->tRFCab:0 |
                                 IS_LPDDR1(dram_type)?lpddr1_timing->tRFC:0 |
                                 IS_DDR3(dram_type)  ?ddr3_timing->tRFC:0);//
    reg_bits_set(PUBL_DTPR1,24,3,IS_LPDDR2(dram_type)?lpddr2_timing->tDQSCK:0);//tDQSCK
    reg_bits_set(PUBL_DTPR1,27,3,IS_LPDDR2(dram_type)?lpddr2_timing->tDQSCKmax:0);//tDQSCKmax	
	*/
	UMCTL2_REG_SET(PUBL_DTPR1,0x193400A0);
	
    //DTPR2, to set tXS,tXP,tCKE,tDLLK //to be confirm by johnnywang
    //reg_bits_set(PUBL_DTPR2, 0,10,IS_LPDDR2(dram_type)?lpddr2_timing->tXSR: ddr3_timing->tXS);
    //reg_bits_set(PUBL_DTPR2,10, 5,IS_LPDDR2(dram_type)?lpddr2_timing->tXP: ddr3_timing->tXPDLL);
    //reg_bits_set(PUBL_DTPR2,15, 4,IS_LPDDR2(dram_type)?lpddr2_timing->tCKESR: (ddr3_timing->tCKE+1));	
    //only used in PUBL DCU unite,I suppose
    //don't need to set,use the default value    
}

void publ_mdr_init(CLK_TYPE_E umctl2_clk,DRAM_INFO* dram)
{
    DRAM_TYPE_E dram_type = dram->dram_type;
    uint32 bl = dram->bl;
    uint32 cl = dram->rl;
    uint32 rl = dram->rl;
    uint32 wl = dram->wl;
	uint32 tWR = 0;
	uint32 al = 0;

    lpddr2_timing_t* lpddr2_timing = dram->ac_timing;
	ddr3_timing_t* ddr3_timing = dram->ac_timing;
    
    #if defined(DDR_LPDDR1)
    {
        //MR
        reg_bits_set(PUBL_MR0,0,3,(bl==4)?2:0);//bl
        reg_bits_set(PUBL_MR0,4,3,cl);//cl
        reg_bits_set(PUBL_MR0,7,1,0); //operating mode (0) or test mode (1)

        //EMR
        UMCTL2_REG_SET(PUBL_MR2,0);
    }
	#endif
	
    #if defined(DDR_LPDDR2)
    {		
		tWR = lpddr2_timing->tWR;
        //MR1
        reg_bits_set(PUBL_MR1,0,3,(bl==4)?2:3);//bl
        reg_bits_set(PUBL_MR1,3,1,MEM_BURST_TYPE);//Burst Type 0:sequential 1:interleaved
		#if 0
        reg_bits_set(PUBL_MR1,4,1,1);//0:wrap 1:no wrap
		#else
		reg_bits_set(PUBL_MR1,4,1,MEM_WC_TYPE);//0:wrap 1:no wrap		
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
        UMCTL2_REG_SET(PUBL_MR3,(MEM_IO_DS==LPDDR2_DS_34R3)?1:0 |
                                (MEM_IO_DS==LPDDR2_DS_40R)?2:0 |
                                (MEM_IO_DS==LPDDR2_DS_48R)?3:0 |
                                (MEM_IO_DS==LPDDR2_DS_60R)?4:0 |
                                (MEM_IO_DS==LPDDR2_DS_80R)?6:0);
    }
	#endif
    #if defined(DDR_DDR3)
    {
        tWR = ddr3_timing->tWR;
		al = dram->al;

		//MR0
		reg_bits_set(PUBL_MR0,0,2,(bl==4)?2:0); //bl
		reg_bits_set(PUBL_MR0,3,1,MEM_BURST_TYPE); //burst type: 0:sequential 1:interleaved
		reg_bits_set(PUBL_MR0,4,3,(cl==5)?1:0 |
			                      (cl==6)?2:0 |
			                      (cl==7)?3:0 |
			                      (cl==8)?4:0 |
			                      (cl==9)?5:0 |
			                      (cl==10)?6:0 |
			                      (cl==11)?7:0);
		reg_bits_set(PUBL_MR0,7,1,0); //0:operationg mode, 1:test mode
		reg_bits_set(PUBL_MR0,9,3,(tWR==5)? 1:0 |
			                      (tWR==6)? 2:0 |
			                      (tWR==7)? 3:0 |
			                      (tWR==8)? 4:0 |
			                      (tWR==10)? 5:0 |
			                      (tWR==12)? 6:0);
		reg_bits_set(PUBL_MR0,12,1,((DDR3_DLL_ON==FALSE)?0:1));

		//MR1
		//DLL enable/disable 0:enable 1:disable
		reg_bits_set(PUBL_MR1,0,1,((DDR3_DLL_ON==TRUE)?0:1));
		//output driver impedance, 00=RZQ/6, 01=RZQ/7
		reg_bits_set(PUBL_MR1,1,1,(MEM_IO_DS == DDR3_DS_40R)?0:1);
		reg_bits_set(PUBL_MR1,5,1,0);
		//on die termiantion
		reg_bits_set(PUBL_MR1,2,1,0);
		reg_bits_set(PUBL_MR1,6,1,0);
		reg_bits_set(PUBL_MR1,9,1,0);
		//cas additive latency
		reg_bits_set(PUBL_MR1,3,2,(al==0)?0:0    |
		                          (al==cl-1)?1:0 |
		                          (al==cl-2)?2:0 );
		//write leveling, 0:disable 1:enable
		reg_bits_set(PUBL_MR1,7,1,0);

		//MR2
		//Partial Array Self RefResh
		reg_bits_set(PUBL_MR2,0,3,0);
		//CAS Write Latency
		reg_bits_set(PUBL_MR2,3,3,(wl==5)? 0:0 |
		                          (wl==6)? 1:0 |
		                          (wl==7)? 2:0 |
		                          (wl==8)? 3:0);
		//Auto Self-Refresh
		reg_bits_set(PUBL_MR2,6,1,0);
		//Self-Refresh Temperature Range, 0:normal  1:extended
		reg_bits_set(PUBL_MR2,7,1,0);

		//MR3
		//Multi-Purpose Register(MPR) Location
		reg_bits_set(PUBL_MR3,0,2,0);
		//Multi-purpose register Enable, 0:disable 1:enable
		reg_bits_set(PUBL_MR3,2,1,0);		
    }	
	#endif
}

void publ_poll_dllock()
{
	uint32 volatile temp = 0;
	
    do temp = UMCTL2_REG_GET(PUBL_PGSR);
    while((temp&0x2) == 0);

}
void publ_reg_init(CLK_TYPE_E clk,DRAM_INFO* dram)
{
	publ_poll_dllock();
    publ_basic_mode_init(clk,dram);
    publ_timing_init(clk,dram);
    publ_mdr_init(clk, dram);
}

BOOLEAN publ_do_training()
{
	volatile uint32 reg_val = 0;
	
	//do dqs training
	wait_pclk(50);	
	REG32(PUBL_PIR) |= 0x101; //do dqs gate training
	wait_pclk(50);	

    //wait training done
    do{reg_val = UMCTL2_REG_GET(PUBL_PGSR);}
    while( (!(reg_val&BIT_0)) || (!(reg_val&BIT_4)));
    wait_pclk(50);

    reg_val = UMCTL2_REG_GET(PUBL_PGSR);
    //check if training success or fail
	if(reg_val&0x3E0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}		
}

void ddr_external_qos_set()
{   //*** the ddr controller channel external priority setting***//
    //priority value: 0x0~0xf
    //description: 0xf is highest priority, 0x0 is lowest priority
    //priority setting: port4 wr&rd (CPx DSP)      =
    //                  port5 wr&rd (CP0W)         =
    //                  port6 wr&rd (CP0 ARM)      =
    //                  port8 wr&rd (CP1 ARM)      = 
    //                  port9 wr&rd (CP2)          >
    //                  port2 rd    (display/gsp)  =
    //                  port0 wr    (mm/dcam/vsp)  >
    //                  port0 rd    (mm/dcam/vsp)  >
    //                  port3 wr&rd (CA7)          >
    //                  port2 wr    (display/gsp)  =
    //                  port7 wr&rd (AP matrix)    >
    //                  port1 wr&rd (GPU)
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C,  0,4,0xc); //chanel 0 wr qos, mm/dcam/vsp
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C,  4,4,0x8); //chanel 0 rd qos, mm/dcam/vsp
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C,  8,4,0x0); //chanel 1 wr qos, GPU
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C, 12,4,0x0); //chanel 1 rd qos, GPU
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C, 16,4,0x2); //chanel 2 wr qos, display/gsp
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C, 20,4,0xc); //chanel 2 rd qos, display/gsp
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C, 24,4,0x4); //chanel 3 wr qos	, CA7
	reg_bits_set(CTL_BASE_PUB_APB + 0x9C, 28,4,0x4); //chanel 3 rd qos, CA7		

	reg_bits_set(CTL_BASE_PUB_APB + 0xA0,  0,4,0xf); //chanel 4 wr qos, CPx DSP
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0,  4,4,0xf); //chanel 4 rd qos, CPx DSP  HPR no use
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0,  8,4,0xe); //chanel 5 wr qos, CP0W
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0, 12,4,0xe); //chanel 5 rd qos, CP0W
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0, 16,4,0xe); //chanel 6 wr qos, CP0 ARM
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0, 20,4,0xe); //chanel 6 rd qos, CP0 ARM
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0, 24,4,0x6); //chanel 7 wr qos, AP matrix 
	reg_bits_set(CTL_BASE_PUB_APB + 0xA0, 28,4,0x6); //chanel 7 rd qos, AP matrix 	

	reg_bits_set(CTL_BASE_PUB_APB + 0xA4,  0,4,0xe); //chanel 8 wr qos, CP1 ARM
	reg_bits_set(CTL_BASE_PUB_APB + 0xA4,  4,4,0xe); //chanel 8 rd qos, CP1 ARM
	reg_bits_set(CTL_BASE_PUB_APB + 0xA4,  8,4,0xe); //chanel 9 wr qos, CP2
	reg_bits_set(CTL_BASE_PUB_APB + 0xA4, 12,4,0xe); //chanel 9 rd qos, CP2
	
}

#ifdef DDR_DFS_SUPPORT
void record_dfs_val(CLK_TYPE_E ddr_clk,uint32 record_base)
{
	ddr_dfs_val_t dfs_val = {NULL};
	uint32 i = 0;
	
	dfs_val.ddr_clk  = ddr_clk;
	dfs_val.umctl2_rfshtmg    = REG32(UMCTL_RFSHTMG);
	dfs_val.umctl2_init0      = REG32(UMCTL_INIT0);
	dfs_val.umctl2_init1      = REG32(UMCTL_INIT1);
	dfs_val.umctl2_init2      = REG32(UMCTL_INIT2);
	dfs_val.umctl2_init3      = REG32(UMCTL_INIT3);
	dfs_val.umctl2_init4      = REG32(UMCTL_INIT4);
	dfs_val.umctl2_init5      = REG32(UMCTL_INIT5);
	dfs_val.umctl2_dramtmg0   = REG32(UMCTL_DRAMTMG0);
	dfs_val.umctl2_dramtmg1   = REG32(UMCTL_DRAMTMG1);
	dfs_val.umctl2_dramtmg2   = REG32(UMCTL_DRAMTMG2);
	dfs_val.umctl2_dramtmg3   = REG32(UMCTL_DRAMTMG3);
	dfs_val.umctl2_dramtmg4   = REG32(UMCTL_DRAMTMG4);
	dfs_val.umctl2_dramtmg5   = REG32(UMCTL_DRAMTMG5);
	dfs_val.umctl2_dramtmg6   = REG32(UMCTL_DRAMTMG6);
	dfs_val.umctl2_dramtmg7   = REG32(UMCTL_DRAMTMG7);
	dfs_val.umctl2_dramtmg8   = REG32(UMCTL_DRAMTMG8);
	dfs_val.umctl2_dfitmg0    = REG32(UMCTL_DFITMG0);
	dfs_val.umctl2_dfitmg1    = REG32(UMCTL_DFITMG1);
	
	dfs_val.publ_ptr0         = REG32(PUBL_PTR0);
	dfs_val.publ_ptr1         = REG32(PUBL_PTR1);
	dfs_val.publ_dtpr0        = REG32(PUBL_DTPR0);
	dfs_val.publ_dtpr1        = REG32(PUBL_DTPR1);
	dfs_val.publ_dtpr2        = REG32(PUBL_DTPR2);
	dfs_val.publ_mr0          = REG32(PUBL_MR0);
	dfs_val.publ_mr1          = REG32(PUBL_MR1);
	dfs_val.publ_mr2          = REG32(PUBL_MR2);
	dfs_val.publ_mr3          = REG32(PUBL_MR3);	
	dfs_val.publ_dx0gcr       = REG32(PUBL_DX0GCR);
	dfs_val.publ_dx1gcr       = REG32(PUBL_DX1GCR);
	dfs_val.publ_dx2gcr       = REG32(PUBL_DX2GCR);
	dfs_val.publ_dx3gcr       = REG32(PUBL_DX3GCR);
	dfs_val.publ_dx0dqstr     = REG32(PUBL_DX0DQSTR);
	dfs_val.publ_dx1dqstr     = REG32(PUBL_DX1DQSTR);
	dfs_val.publ_dx2dqstr     = REG32(PUBL_DX2DQSTR);
	dfs_val.publ_dx3dqstr     = REG32(PUBL_DX3DQSTR);

	for(i = 0; i<(sizeof(dfs_val)/4);i++)
	{
		REG32(record_base + i*4) = REG32((uint32)(&dfs_val)+i*4);
	}
	
}
#endif
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
#if 0
	sdram_vddmem_set(VDDMEM_1V20);
#endif
    
    sdram_clk_set(dmc_clk);

    #if defined(CONFIG_SPX15)
    REG32(0x402b00f0) |= (1<<31);
    #endif

    //enable umctl,publ,phy
    umctl2_ctl_en(TRUE);
    
    //to assert umctl reset,in order to prevent umctl issue mode register cmd to SDRAM automatically
    umctl2_soft_reset(TRUE);

    //open umctl and publ reg
    umctl2_publ_reg_open();

    // EVB FIX for dolphin----------------------------------------------
    #if 0
    #if defined(CONFIG_SPX15)
    reg_bits_set(PUBL_PGCR,12,2,1);  //dp dm 
    reg_bits_set(PUBL_PGCR,14,1,1);  //dp dm 
    #endif
    #endif

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

    wait_pclk(1500); //to wait dfi_init fsm finished, 1500/153.6mhz=10us,this delay is necessary
    //do training
    if(!publ_do_training())
    {
	ddr_print("ERROR!!! DDR TRAINING ERROR");
        return FALSE;
    }    
    
    //enable all port
    umctl2_allport_en();

#if 0
	umctl2_low_power_open();
#endif

#if 1
	ddr_external_qos_set();
#endif

    return TRUE;
}


#if defined(CONFIG_SPX15)

#ifdef DDR_LPDDR2
CLK_TYPE_E DDR_DFS_POINT[] = 
{
        CLK_192MHZ,
        CLK_332MHZ,
        CLK_400MHZ
};
#else
CLK_TYPE_E DDR_DFS_POINT[] = 
{
	CLK_200MHZ,
	CLK_333MHZ,
	CLK_400MHZ, 
	CLK_533MHZ,
};
#endif
#define NS2CLK_T(x_ns,clk) ((clk*x_ns)/1000 + 1)

void  __cal_actiming(lpddr2_timing_t *cal_timing,void *native_timing,CLK_TYPE_E clk)
{
    lpddr2_timing_t* lpddr2_timing = (lpddr2_timing_t*)(native_timing);
    ddr3_timing_t*   ddr3_timing   = (ddr3_timing_t*)(native_timing);
	#ifdef DDR_LPDDR2	
	cal_timing->tREFI 	= NS2CLK_T(lpddr2_timing->tREFI,clk);
	cal_timing->tRAS  	= NS2CLK_T(lpddr2_timing->tRAS,clk);
	cal_timing->tRC   	= NS2CLK_T(lpddr2_timing->tRC,clk);
	cal_timing->tRFCab	= NS2CLK_T(lpddr2_timing->tRFCab,clk);
	cal_timing->tRFCpb	= NS2CLK_T(lpddr2_timing->tRFCpb,clk);	
	cal_timing->tRCD  	= NS2CLK_T(lpddr2_timing->tRCD,clk);
	cal_timing->tRP   	= NS2CLK_T(lpddr2_timing->tRP,clk);
	cal_timing->tRRD  	= NS2CLK_T(lpddr2_timing->tRRD,clk);	
	cal_timing->tXP   	= NS2CLK_T(lpddr2_timing->tXP,clk);
	cal_timing->tXSR  	= NS2CLK_T(lpddr2_timing->tXSR,clk);
	cal_timing->tCKESR	= NS2CLK_T(lpddr2_timing->tCKESR,clk);	
	cal_timing->tRTP  	= NS2CLK_T(lpddr2_timing->tRTP,clk);
	cal_timing->tFAW  	= NS2CLK_T(lpddr2_timing->tFAW,clk);
	cal_timing->tDPD  	= NS2CLK_T(lpddr2_timing->tDPD,clk);	
	cal_timing->tZQINIT	= NS2CLK_T(lpddr2_timing->tZQINIT,clk);
	cal_timing->tZQCL   = NS2CLK_T(lpddr2_timing->tZQCL,clk);
	cal_timing->tZQCS   = NS2CLK_T(lpddr2_timing->tZQCS,clk);
	cal_timing->tZQreset= NS2CLK_T(lpddr2_timing->tZQreset,clk);
	cal_timing->tDQSCK  = NS2CLK_T(lpddr2_timing->tDQSCK,clk);
	cal_timing->tDQSCKmax= NS2CLK_T(lpddr2_timing->tDQSCKmax,clk);
	#else
	cal_timing->tREFI 	= NS2CLK_T(ddr3_timing->tREFI,clk);
	#endif
	
}
#else

#ifdef DDR_DFS_SUPPORT

#if defined(DDR_LPDDR2)
CLK_TYPE_E DDR_DFS_POINT[] = 
{
	CLK_200MHZ,
	CLK_384MHZ,
	CLK_533MHZ,
};
#else
CLK_TYPE_E DDR_DFS_POINT[] = 
{
	CLK_200MHZ,
	CLK_384MHZ,
	CLK_464MHZ,
};
#endif
#define NS2CLK_T(x_ns,clk) ((clk*x_ns)/1000 + 1)

void  __cal_actiming(lpddr2_timing_t *cal_timing,void *native_timing,CLK_TYPE_E clk)
{
    lpddr2_timing_t* lpddr2_timing = (lpddr2_timing_t*)(native_timing);
    ddr3_timing_t*   ddr3_timing   = (ddr3_timing_t*)(native_timing);
	#ifdef DDR_LPDDR2	
	cal_timing->tREFI 	= NS2CLK_T(lpddr2_timing->tREFI,clk);
	cal_timing->tRAS  	= NS2CLK_T(lpddr2_timing->tRAS,clk);
	cal_timing->tRC   	= NS2CLK_T(lpddr2_timing->tRC,clk);
	cal_timing->tRFCab	= NS2CLK_T(lpddr2_timing->tRFCab,clk);
	cal_timing->tRFCpb	= NS2CLK_T(lpddr2_timing->tRFCpb,clk);	
	cal_timing->tRCD  	= NS2CLK_T(lpddr2_timing->tRCD,clk);
	cal_timing->tRP   	= NS2CLK_T(lpddr2_timing->tRP,clk);
	cal_timing->tRRD  	= NS2CLK_T(lpddr2_timing->tRRD,clk);	
	cal_timing->tXP   	= NS2CLK_T(lpddr2_timing->tXP,clk);
	cal_timing->tXSR  	= NS2CLK_T(lpddr2_timing->tXSR,clk);
	cal_timing->tCKESR	= NS2CLK_T(lpddr2_timing->tCKESR,clk);	
	cal_timing->tRTP  	= NS2CLK_T(lpddr2_timing->tRTP,clk);
	cal_timing->tFAW  	= NS2CLK_T(lpddr2_timing->tFAW,clk);
	cal_timing->tDPD  	= NS2CLK_T(lpddr2_timing->tDPD,clk);	
	cal_timing->tZQINIT	= NS2CLK_T(lpddr2_timing->tZQINIT,clk);
	cal_timing->tZQCL   = NS2CLK_T(lpddr2_timing->tZQCL,clk);
	cal_timing->tZQCS   = NS2CLK_T(lpddr2_timing->tZQCS,clk);
	cal_timing->tZQreset= NS2CLK_T(lpddr2_timing->tZQreset,clk);
	cal_timing->tDQSCK  = NS2CLK_T(lpddr2_timing->tDQSCK,clk);
	cal_timing->tDQSCKmax= NS2CLK_T(lpddr2_timing->tDQSCKmax,clk);
	#else
	cal_timing->tREFI 	= NS2CLK_T(ddr3_timing->tREFI,clk);
	#endif
	
}
#endif
#endif

#ifdef DDR_AUTO_DETECT
volatile uint32 MRR_VLD = 0;

uint32 __reorder_mrr_data(uint32 mrr_data)
{
	uint32 mrr_data_new = 0;

	mrr_data_new = ((mrr_data&BIT_7)>>3) | // bit7->data4
	               ((mrr_data&BIT_6)>>3) | // bit6->data3
	               ((mrr_data&BIT_5)>>3) | // bit5->data2
	               ((mrr_data&BIT_4)>>4) | // bit4->data0
	               ((mrr_data&BIT_3)<<4) | // bit3->data7
	               ((mrr_data&BIT_2)<<4) | // bit2->data6
	               ((mrr_data&BIT_1)   ) | // bit1->data1
	               ((mrr_data&BIT_0)<<5);  // bit0->data5
	               
	return mrr_data_new;
}

uint32 __get_mr8(MEM_CS_E cs)
{
	volatile uint32 mr8 = 0;
	volatile uint32 cnt = 0;
	volatile uint32 mrr_vld_old = 0;
	volatile uint32 temp = 0;

	wait_pclk(50);
	REG32(PUBL_PIR) |= 0X11; //trigger itm reset
	wait_pclk(50);
	do temp = UMCTL2_REG_GET(PUBL_PGSR);
	while((temp&0x1) == 0);
	wait_pclk(5263);//publ apb clk=26m-38ns, 5263*38ns = 200us

	__mem_init(MEM_MRD, MEM_MR8, 0, cs); //read MR8

	wait_pclk(50);
	REG32(PUBL_PIR) |= 0X11; //trigger itm reset
	wait_pclk(50);
	do temp = UMCTL2_REG_GET(PUBL_PGSR);
	while((temp&0x1) == 0);
	wait_pclk(5263);//publ apb clk=26m-38ns, 5263*38ns = 200us

	mrr_vld_old = MRR_VLD;

	while(1)
	{
		mr8 = REG32(0x300200a8);
		MRR_VLD = mr8&0x100;

		if(mrr_vld_old == MRR_VLD)
		{
			cnt++;
			if(cnt >= 0x100000)
			{
				return 0xffffffff;
			}
		}
		else
		{
			break;
		}
	}
	return(__reorder_mrr_data(mr8&0xff));
}

BOOLEAN __update_dram_info(DRAM_TYPE_E* ddr_type)
{
	volatile uint32 mr8_cs0[5] = {0};
	volatile uint32 mr8_cs1[5] = {0};
	uint32 i = 0;
	uint32 cs0_cap = 0;
	uint32 cs1_cap = 1;

	//get cs1 mr8
	for(i = 0; i < 5; i++)
	{
		mr8_cs1[i] = __get_mr8(MEM_CS1);

		//if mr8 time out or mr8 is not equal, read mr8 fail, return fail
		if(mr8_cs1[i] == 0xffffffff ||
		   mr8_cs1[i] != mr8_cs1[0])
		{
        	return FALSE;
		}
	}
	cs1_cap = (mr8_cs1[0]&0x3c)>>2;

	//get cs0 mr8
	for(i = 0; i < 5; i++)
	{
		mr8_cs0[i] = __get_mr8(MEM_CS0);

		//if mr8 time out or mr8 is not equal, read mr8 fail, return fail
		if(mr8_cs0[i] == 0xffffffff ||
		   mr8_cs0[i] != mr8_cs0[0])
		{
			return FALSE;
		}
	}
	cs0_cap = (mr8_cs0[0]&0x3c)>>2;

	if(mr8_cs1[0] == 0) // only cs0
	{
		if(cs0_cap ==4)
		{
			*ddr_type = DRAM_LPDDR2_1CS_1G_X32;
		}
		else if(cs0_cap ==5)
		{
			*ddr_type = DRAM_LPDDR2_1CS_2G_X32;
		}
		else if(cs0_cap ==6)
		{
			*ddr_type = DRAM_LPDDR2_1CS_4G_X32;
		}
		else if(cs0_cap ==7)
		{
			*ddr_type = DRAM_LPDDR2_1CS_8G_X32;
		}
		else if(cs0_cap ==8)
		{
			*ddr_type = DRAM_LPDDR2_1CS_16G_X32;
		}
		else
		{
			*ddr_type = DRAM_LPDDR2_1CS_4G_X32;
		}
		#if 0
		switch(cs0_cap)
		{
			case 4: *ddr_type = DRAM_LPDDR2_1CS_1G_X32;break;
			case 5: *ddr_type = DRAM_LPDDR2_1CS_2G_X32;break;
			case 6: *ddr_type = DRAM_LPDDR2_1CS_4G_X32;break;
			case 7: *ddr_type = DRAM_LPDDR2_1CS_8G_X32;break;
			case 8: *ddr_type = DRAM_LPDDR2_1CS_16G_X32;break;
			default : *ddr_type = DRAM_LPDDR2_1CS_4G_X32;break;
		}
		#endif
	}
	else // both cs0 and cs1 alive
	{
		if(cs0_cap ==4) //cs0 1g, cs1 1g
		{
			*ddr_type = DRAM_LPDDR2_2CS_2G_X32;
		}
		else if((cs0_cap ==5)&&(cs1_cap==4)) //cs0 2g, cs1 1g
		{
			*ddr_type = DRAM_LPDDR2_2CS_3G_X32;
		}
		else if((cs0_cap ==5)&&(cs1_cap==5)) //cs0 2g, cs1 2g
		{
			*ddr_type = DRAM_LPDDR2_2CS_4G_X32;
		}
		else if((cs0_cap ==6)&&(cs1_cap==5)) //cs0 4g, cs1 2g
		{
			*ddr_type = DRAM_LPDDR2_2CS_6G_X32;
		}
		else if((cs0_cap ==6)&&(cs1_cap==6)) //cs0 4g, cs1 4g
		{
			*ddr_type = DRAM_LPDDR2_2CS_8G_X32;
		}
		else
		{
			*ddr_type = DRAM_LPDDR2_2CS_8G_X32;
		}

		
		#if 0
		switch(cs0_cap)
		{
			case 4: *ddr_type = DRAM_LPDDR2_2CS_2G_X32;break; //cs0 1g, cs1 1g
			case 5: //cs0 2g
			{
				if(cs1_cap ==4) //cs1 1g
				{
					*ddr_type == DRAM_LPDDR2_2CS_3G_X32;
				}
				else if(cs1_cap ==5) //cs1 2g
				{
					*ddr_type == DRAM_LPDDR2_2CS_4G_X32;
				}
				else
				{
					*ddr_type == DRAM_LPDDR2_2CS_4G_X32;
				}
				break;
			}
			case 6: //cs0 4g
			{
				if(cs1_cap ==5) //cs1 2g
				{
					*ddr_type = DRAM_LPDDR2_2CS_6G_X32;
				}
				else //cs1 4g
				{
					*ddr_type == DRAM_LPDDR2_2CS_8G_X32;
				}
				break;
			}
			case 7: //cs0 8g
			{
				if(cs1_cap == 6) //cs1 4g
				{
					*ddr_type == DRAM_LPDDR2_2CS_12G_X32;
				}
				else //cs1 8g
				{
					*ddr_type == DRAM_LPDDR2_2CS_16G_X32;
				}
				break;
			}
			default: *ddr_type == DRAM_LPDDR2_2CS_16G_X32;break;
		}
		#endif
	}
	return TRUE;	
}

BOOLEAN __ddr_info_detect(DRAM_TYPE_E* ddr_type)
{
	uint32 mr8 = 0;

	if(!__update_dram_info(ddr_type))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}
#endif

#ifdef DDR_SCAN_SUPPORT
void ddr_scan_data_update(uint32 ddr_clk)
{
	uint32 arry_size = sizeof(ddr_dfs_val_t);
	uint32 i = 0;
	ddr_dfs_val_t * dfs_val_ptr = NULL;
	
	for(i = 0; i < ARRAY_SIZE(DDR_DFS_POINT);i++)
	{

		dfs_val_ptr = (ddr_dfs_val_t *)(DDR_DFS_VAL_BASE+sizeof(ddr_dfs_val_t)*i);
		
		if(dfs_val_ptr->ddr_clk == DDR_DFS_POINT[i])
		{
			dfs_val_ptr->publ_dx0dqstr = REG32(PUBL_DX0DQSTR);
			dfs_val_ptr->publ_dx1dqstr = REG32(PUBL_DX1DQSTR);
			dfs_val_ptr->publ_dx2dqstr = REG32(PUBL_DX2DQSTR);
			dfs_val_ptr->publ_dx3dqstr = REG32(PUBL_DX3DQSTR);
		}

		if(REG32(DDR_DFS_VAL_BASE+sizeof(ddr_dfs_val_t)*(i+1)) == 0x12345678)
		{
			return;
		}
	}
}
#endif


void sdram_init()
{
#if defined(CONFIG_SPX15)
	#if defined(CONFIG_CLK_PARA)
	uint32 ddr_clk = mcu_clk_para.ddr_freq/1000000;
	#else
	uint32 ddr_clk = DDR_CLK ;
	#endif
#endif

	#ifdef DDR_DFS_SUPPORT
	uint32 i = 0;
	DRAM_INFO * dram_info = NULL;
	void *ddr_timing_native = NULL;




	#ifdef DDR_LPDDR2
		ddr_timing_native = &LPDDR2_ACTIMING_NATIVE;
	#else
		ddr_timing_native = &DDR3_ACTIMING_NATIVE;
	#endif

	

	#ifdef DDR_AUTO_DETECT	
		if(IS_SHARK_CS)
		{
			DDR_TYPE_LOCAL = DRAM_LPDDR2_1CS_4G_X32;
		}
		else
		{		
			DDR_TYPE_LOCAL = DDR_TYPE;		
		}
	#endif
	
	//clear dfs value space
	for(i = 0; i < 256; i++)
	{
		REG32(DDR_DFS_VAL_BASE+i*4) = 0;
	}
	
	for(i = 0; i <ARRAY_SIZE(DDR_DFS_POINT); i++)
	{
		#ifdef DDR_AUTO_DETECT
			dram_info = get_dram_cfg(DDR_TYPE_LOCAL);
		#else
			dram_info = get_dram_cfg(DDR_TYPE);
		#endif

		#if defined(CONFIG_SPX15)
		if(DDR_DFS_POINT[i] >= ddr_clk)
		{
			DDR_DFS_POINT[i] = ddr_clk;
		}
		#else
			if(DDR_DFS_POINT[i] >= DDR_CLK)		
			{
				DDR_DFS_POINT[i] = DDR_CLK;
			}
		#endif		
		
		__cal_actiming(dram_info->ac_timing,ddr_timing_native,DDR_DFS_POINT[i]);

		#ifdef DDR_AUTO_DETECT		
		if(IS_SHARK_CS)
		{
			if(DDR_DFS_POINT[i] == CLK_100MHZ)
			{
				__sdram_init(DDR_DFS_POINT[i], UMCTL2_PORT_CONFIG, dram_info);
				
				if(__ddr_info_detect(&DDR_TYPE_LOCAL))
				{
					dram_info = get_dram_cfg(DDR_TYPE_LOCAL);
				}
				else
				{
					//ddr auto detect fail
				}
			}
			else
			{
				__sdram_init(DDR_DFS_POINT[i], UMCTL2_PORT_CONFIG, dram_info);
			}
		}
		else
		{
			__sdram_init(DDR_DFS_POINT[i], UMCTL2_PORT_CONFIG, dram_info);
		}
		#else
		__sdram_init(DDR_DFS_POINT[i], UMCTL2_PORT_CONFIG, dram_info);
		#endif
		
		record_dfs_val(DDR_DFS_POINT[i], DDR_DFS_VAL_BASE+sizeof(ddr_dfs_val_t)*i);
		#if defined(CONFIG_SPX15)
		if(DDR_DFS_POINT[i] >= ddr_clk)
		{
			REG32(DDR_DFS_VAL_BASE+sizeof(ddr_dfs_val_t)*(i+1)) = 0x12345678;
			break;
		}
		#else
		if(DDR_DFS_POINT[i] >= DDR_CLK)		
		{
			REG32(DDR_DFS_VAL_BASE+sizeof(ddr_dfs_val_t)*(i+1)) = 0x12345678;
			break;
		}
		#endif		
	}
	
	REG32(DDR_DFS_VAL_BASE+sizeof(ddr_dfs_val_t)*ARRAY_SIZE(DDR_DFS_POINT)) = 0x12345678;
	

	#else
		#if defined(CONFIG_SPX15)
			#if defined(CONFIG_CLK_PARA)
			DRAM_INFO * dram_info = NULL;
			dram_info = get_dram_cfg(DDR_TYPE);
			__cal_actiming(dram_info->ac_timing,&LPDDR2_ACTIMING_NATIVE,ddr_clk);
			__sdram_init(ddr_clk, UMCTL2_PORT_CONFIG, dram_info);
			#else
	        __sdram_init(ddr_clk, UMCTL2_PORT_CONFIG, get_dram_cfg(DDR_TYPE));
			#endif
		#else
        __sdram_init(DDR_CLK, UMCTL2_PORT_CONFIG, get_dram_cfg(DDR_TYPE));
		#endif		
	#endif

	#ifdef DDR_SCAN_SUPPORT
		ddr_scan(dram_info);
		ddr_scan_data_update(DDR_CLK);
	#endif
	
	umctl2_low_power_open();
	
}

