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

#ifdef   __cplusplus
extern   "C"
{
#endif


/*******************************************************************************
                            ddr parameter config custom
*******************************************************************************/

LOCAL CONST EMC_PARAM_T s_emc_parm[] =
{
	// arm_clk          emc_clk        ddr driver strength   dqs_drv / dat_drv / ctl_drv / clk_drv / clk_wr			nand_id
	//{CHIP_CLK_1000MHZ, EMC_CLK_133MHZ, DDR_DRV_STR_TR_Q,        2,		1,          0,      3,          15,		{0, 0, 0, 0, 0}},    //EVB
	//{CHIP_CLK_1000MHZ, EMC_CLK_400MHZ, DDR_DRV_STR_TR_Q,        2,		2,          0,      2,          15, 	{0, 0, 0, 0, 0}},  // PCB_V1.0.0
	//{CHIP_CLK_1000MHZ, EMC_CLK_400MHZ, DDR_DRV_STR_TR_Q,        2,		3,          1,      2,          12, 	{0, 0, 0, 0, 0}},  // 4+2 nandmcp
	//{CHIP_CLK_1000MHZ, EMC_CLK_333MHZ, DDR_DRV_STR_TR_Q,        1,		1,          1,      2,          19, 	{0, 0, 0, 0, 0}},  // openphone
//	{CHIP_CLK_1000MHZ, EMC_CLK_133MHZ, DDR_DRV_STR_TR_Q,        2,		1,          2,      1,          15,			{0, 0, 0, 0, 0}},  // auto slt soket
	{CHIP_CLK_1000MHZ, EMC_CLK_400MHZ, DDR_DRV_STR_TR_Q,        1,		2,          1,      2,          12,			{0, 0, 0, 0, 0}},  // PCB_V1.1.0

	//{CHIP_CLK_1000MHZ, EMC_CLK_400MHZ, DDR_DRV_STR_TR_Q,        1,		2,          1,      2,          12,			{0x2c, 0xbc, 0x90, 0x55, 0x56}},  // MT29C4G96MAZBACKD-5 WT, just example, scan your board then modify para
	//{CHIP_CLK_1000MHZ, EMC_CLK_400MHZ, DDR_DRV_STR_TR_Q,        2,		2,          1,      2,          10,			{0xad, 0xbc, 0x90, 0x55, 0x56}},  // H9DA4GH2GJBMCR, just example, scan your board then modify para

  //{CHIP_CLK_1000MHZ, EMC_CLK_400MHZ, DDR_DRV_STR_TR_Q,		1,		2,			1,		2,			12, 		{0x2c, 0xb3, 0x90, 0x66, 0x64}},  // custom para, just example

};


/*******************************************************************************
                           Variable and Array definiation
*******************************************************************************/

//#define SDRAM_CLK   (EMC_CLK/2)              // 96MHz
//#define SDRAM_T     (1000000000/SDRAM_CLK)   // ns
#define SDRAM_T (1000000000/EMC_CLK_400MHZ)  // ns

LOCAL CONST SDRAM_TIMING_PARA_T s_sdram_timing_param =
//  ms    ns   ns    		ns      ns    	ns     	  ns   	ns  	clk   clk
// tREF,tRP,tRCD, tWR/tRDL/tDPL,tRFC,	tXSR,     tRAS,	tRRD,	tMRD, tWTR(wtr is only for ddr)
#if defined(CHIP0_HYNIX_DDR_H8BCS0RJ0MCP)
{7800,   30,  30,   	15,         110, 	140,      50,  	15,  	2,    1   };
#elif defined(CHIP1_TOSHIBA_SDR_TY9000A800JFGP40)
{7800,   23,  23,  2*SDRAM_T,   	80,  	120,      50,  	15,  	2,    0   };
#elif defined(CHIP2_ST_SDR_M65K)
{7800,   24,  18,   	15,         80,  	18,       60,  	18,  	2,    0   };
#elif defined(CHIP3_SAMSUNG_SDR_K5D1G13DCA)
{7800,   24,  18,   	15,         80,  	18,       60,  	18,  	2,    0   };
#elif defined(CHIP4_SAMSUNG_SDR_K5D5657DCBD090)
{7800,   24,  18,   	15,         80,  	18,       60,  	18,  	2,    0   };
#elif defined(CHIP5_HYNIX_SDR_HYC0SEH0AF3P)
{7800,   29,  29,  2*SDRAM_T,   	80,  2*SDRAM_T,	  60,  	19,  	2,    0   };
#elif defined(CHIP6_SAMSUNG_SDR_K5D1257ACFD090)
{7800,   27,  27,   	15,         80,  	120,      50,  	18,  	2,    0   };
#elif defined(CHIP7_HYNIX_SDR_H8ACUOCEOBBR)
{7800,   27,  27,   	15,         80,  	120,      60,  	19,  	2,    0   };
#elif defined(CHIP8_HYNIX_SDR_H8ACS0EJ0MCP)
{7800,   27,  27,   	15,         80,  	120,      60,  	19,  	2,    0   };
#elif defined(CHIP9_HYNIX_SDR_H8AES0SQ0MCP)
{7800,   27,  27,   	15,         80,  	120,      60,  	19,  	2,    0   };
#elif defined(CHIP10_HYNIX_SDR_HYC0SEH0AF3P)
{7800,   27,  27,   	15,         80,  	120,      60,  	19,  	2,    0   };
#elif defined(CHIP11_MICRON_SDR_MT48H)
{7800,   20,  20,   	15,         100, 	120,      60, 2*SDRAM_T,2,    0   };
#elif defined(CHIP12_HYNIX_DDR_H9DA4GH4JJAMCR4EM)
{7800,   30,  30,   	15,         90, 	140,      50,  	15,  	2,    2   };
#elif defined(CHIP13_HYNIX_SDR_H8ACS0PH0MCP)
{7800,   27,  27,   	15,         80,  	120,      60,  	19,  	2,    0   };
#elif defined(CHIP14_HYNIX_DDR_H9DA4GH2GJAMCR)
{7800,   30,  30,   	15,         90, 	140,      50,  	15,  	2,    2   };
#elif defined(CHIP15_SAMSUNG_DDR_K522H1HACF)
{7800,   21,  15,   	12,         80, 	120,      40,  	10,  	2,    2   };
#else
{7800,   30,  30,   	15,         110, 	140,      50,  	15,  	2,    2   };
#endif

#ifndef SDRAM_AUTODETECT_SUPPORT

LOCAL CONST SDRAM_CFG_INFO_T s_sdram_config_info =
#if defined(CHIP0_HYNIX_DDR_H8BCS0RJ0MCP)
{ROW_MODE_13, COL_MODE_10, DATA_WIDTH_16, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     DDR_SDRAM,   EMC_ONE_CS_MAP_1GBIT};//for sc7702 emc 16bit, actually this ddr is 32bit
#elif defined(CHIP1_TOSHIBA_SDR_TY9000A800JFGP40)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_32, BURST_LEN_1_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     SDR_SDRAM,   EMC_ONE_CS_MAP_512MBIT};
#elif defined(CHIP2_ST_SDR_M65K)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP3_SAMSUNG_SDR_K5D1G13DCA)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP4_SAMSUNG_SDR_K5D5657DCBD090)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_16, BURST_LEN_8_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP5_HYNIX_SDR_HYC0SEH0AF3P)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_32, BURST_LEN_8_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP6_SAMSUNG_SDR_K5D1257ACFD090)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_16, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP7_HYNIX_SDR_H8ACUOCEOBBR)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_16, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP8_HYNIX_SDR_H8ACS0EJ0MCP)
{ROW_MODE_13, COL_MODE_10, DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_1GBIT};
#elif defined(CHIP9_HYNIX_SDR_H8AES0SQ0MCP)
{ROW_MODE_13, COL_MODE_10, DATA_WIDTH_16, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     SDR_SDRAM,   EMC_ONE_CS_MAP_1GBIT};
#elif defined(CHIP10_HYNIX_SDR_HYC0SEH0AF3P)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_16, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP11_MICRON_SDR_MT48H)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     SDR_SDRAM,   EMC_ONE_CS_MAP_256MBIT};
#elif defined(CHIP12_HYNIX_DDR_H9DA4GH4JJAMCR4EM)
{ROW_MODE_14, COL_MODE_10, DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     DDR_SDRAM,   EMC_ONE_CS_MAP_2GBIT};
#elif defined(CHIP13_HYNIX_SDR_H8ACS0PH0MCP)
{ROW_MODE_13, COL_MODE_9,  DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_INVALID, SDR_SDRAM,   EMC_ONE_CS_MAP_512MBIT};
#elif defined(CHIP14_HYNIX_DDR_H9DA4GH2GJAMCR)
{ROW_MODE_14, COL_MODE_10, DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     DDR_SDRAM,   EMC_ONE_CS_MAP_2GBIT};
#elif defined(CHIP15_SAMSUNG_DDR_K522H1HACF)
{ROW_MODE_14, COL_MODE_10, DATA_WIDTH_16, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     DDR_SDRAM,   EMC_ONE_CS_MAP_1GBIT};
#else
{ROW_MODE_14, COL_MODE_10, DATA_WIDTH_32, BURST_LEN_2_WORD, CAS_LATENCY_3, SDRAM_EXT_MODE_REG,     DDR_SDRAM,   EMC_ONE_CS_MAP_1GBIT};
#endif

#endif

#ifndef SDRAM_AUTODETECT_SUPPORT

LOCAL CONST SDRAM_CHIP_FEATURE_T s_sdram_feature =
#if defined(CHIP0_HYNIX_DDR_H8BCS0RJ0MCP)
{                   SDRAM_FEATURE_CL_3,                    SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_1G_BIT  };//for sc7702 emc 512m, actually this ddr is 1g
#elif defined(CHIP1_TOSHIBA_SDR_TY9000A800JFGP40)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_512M_BIT};
#elif defined(CHIP2_ST_SDR_M65K)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP3_SAMSUNG_SDR_K5D1G13DCA)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP4_SAMSUNG_SDR_K5D5657DCBD090)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP5_HYNIX_SDR_HYC0SEH0AF3P)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP6_SAMSUNG_SDR_K5D1257ACFD090)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP7_HYNIX_SDR_H8ACUOCEOBBR)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP8_HYNIX_SDR_H8ACS0EJ0MCP)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_1G_BIT  };
#elif defined(CHIP9_HYNIX_SDR_H8AES0SQ0MCP)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_1G_BIT  };
#elif defined(CHIP10_HYNIX_SDR_HYC0SEH0AF3P)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP11_MICRON_SDR_MT48H)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3,                    SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_256M_BIT};
#elif defined(CHIP12_HYNIX_DDR_H9DA4GH4JJAMCR4EM)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3,                    SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_4G_BIT  };
#elif defined(CHIP13_HYNIX_SDR_H8ACS0PH0MCP)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3, SDRAM_FEATURE_BL_1|SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_512M_BIT};
#elif defined(CHIP14_HYNIX_DDR_H9DA4GH2GJAMCR)
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3,                    SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_2G_BIT  };
#elif defined(CHIP15_SAMSUNG_DDR_K522H1HACF)
{                   SDRAM_FEATURE_CL_3,                    SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_1G_BIT  };
#else
{SDRAM_FEATURE_CL_2|SDRAM_FEATURE_CL_3,                    SDRAM_FEATURE_BL_2|SDRAM_FEATURE_BL_4|SDRAM_FEATURE_BL_8, CAP_1G_BIT  };
#endif

#endif

CONST EMC_PHY_L1_TIMING_T EMC_PHY_TIMING_L1_INFO[EMC_PHYL1_TIMING_MATRIX_MAX] =
{
//data_ie, data_oe, dqs_pst_gate, dqs_pre_gate, dqs_ie, dqs_oe
#ifdef SDR_SDRAM_SUPPORT
	{0x20,		1,		0,				0,			0,		0},		//sdram cas_latency = 2
	{0x40,		1,		0,				0,			0,		0},  	//sdram cas_latency = 3
#endif
	{0xf0,		0xe,	0x10,			0x8,		0xf0,	0xe}, 	//ddram cas_latency = 2
	{0xf0,		0xe,	0x20,			0x10,		0xf0,	0xe},  	//ddram cas_latency = 3
};

CONST EMC_PHY_L2_TIMING_T EMC_PHY_TIMING_L2_INFO[EMC_PHYL2_TIMING_MATRIX_MAX] =
{
//emc_dl3,   4,     5,     6,     7,     8,     9,     10,    11,    12,    13,    14,    15,    16,    17,    18,    19
	//{L2_PAR, L2_PAR/2,L2_PAR/2,L2_PAR/2,L2_PAR/2,L2_PAR/2,L2_PAR/2,L2_PAR/2,L2_PAR/2,L2_PAR,L2_PAR,L2_PAR,L2_PAR,L2_PAR,L2_PAR,L2_PAR,L2_PAR},	//DLL_OFF
	{12,12,12,12,	6,	6,	6,	6,	6,	6,	6,	6,	12,	12,	12,	12,	12,	12,	12,	12},	//DLL_OFF emc=400mhz
	{0x8040,0x8040,0x8040,0x8040,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8020,0x8040,0x8040,0x8040,0x8040,0x8040,0x8040,0x8040,0x8040}	//DLL_ON
};

#ifdef SDRAM_AUTODETECT_SUPPORT

CONST SDRAM_MODE_T sdram_mode_table[] =
{
    {CAP_6G_BIT, EMC_ONE_CS_MAP_4GBIT, ROW_MODE_14, ROW_MODE_15_6G,   DATA_WIDTH_32},
    {CAP_6G_BIT, EMC_ONE_CS_MAP_4GBIT, ROW_MODE_14, COL_MODE_11_6G,   DATA_WIDTH_32},
    {CAP_4G_BIT, EMC_ONE_CS_MAP_2GBIT, ROW_MODE_14, COL_MODE_10,      DATA_WIDTH_32},
    {CAP_2G_BIT, EMC_ONE_CS_MAP_2GBIT, ROW_MODE_14, COL_MODE_10,      DATA_WIDTH_32},
    {CAP_1G_BIT, EMC_ONE_CS_MAP_1GBIT, ROW_MODE_14, COL_MODE_9,       DATA_WIDTH_32},
    {CAP_1G_BIT, EMC_ONE_CS_MAP_1GBIT, ROW_MODE_13, COL_MODE_10,      DATA_WIDTH_32},

    {CAP_2G_BIT, EMC_ONE_CS_MAP_2GBIT, ROW_MODE_14, COL_MODE_11,      DATA_WIDTH_16},    
    {CAP_1G_BIT, EMC_ONE_CS_MAP_1GBIT, ROW_MODE_14, COL_MODE_10,      DATA_WIDTH_16},
    {CAP_ZERO, EMC_ONE_CS_MAP_DEFAULT, SDRAM_MIN_ROW, SDRAM_MIN_COLUMN, DATA_WIDTH_16}
};

PUBLIC SDRAM_MODE_PTR SDRAM_GetModeTable(void)
{
    return (SDRAM_MODE_PTR)sdram_mode_table;
}

SDRAM_CFG_INFO_T s_sdram_config_info = {
    SDRAM_MAX_ROW, 
    SDRAM_MAX_COLUMN, 
    DATA_WIDTH_32, 
    BURST_LEN_2_WORD, 
    CAS_LATENCY_3, 
    SDRAM_EXT_MODE_REG,     
    DDR_SDRAM,   
    EMC_ONE_CS_MAP_4GBIT
};

#endif


LOCAL EMC_CHL_INFO_T s_emc_chl_info[] =
{// emc_chl_num       axi_chl_wr_pri  axi_chl_rd_pri      ahb_chl_pri
    {EMC_AXI_ARM,       EMC_CHL_PRI_2,  EMC_CHL_PRI_2,  EMC_CHL_NONE},
    {EMC_AXI_GPU,       EMC_CHL_PRI_0,  EMC_CHL_PRI_0,  EMC_CHL_NONE},
    {EMC_AXI_DISPC,     EMC_CHL_PRI_0,  EMC_CHL_PRI_3,  EMC_CHL_NONE},
    {EMC_AHB_CP_MTX,    EMC_CHL_NONE,   EMC_CHL_NONE,   EMC_CHL_PRI_1},
    {EMC_AHB_MST_MTX,   EMC_CHL_NONE,   EMC_CHL_NONE,   EMC_CHL_PRI_3},
    {EMC_AHB_LCDC,      EMC_CHL_NONE,   EMC_CHL_NONE,   EMC_CHL_PRI_0},
    {EMC_AHB_DCAM,      EMC_CHL_NONE,   EMC_CHL_NONE,   EMC_CHL_PRI_2},
    {EMC_AHB_VSP,       EMC_CHL_NONE,   EMC_CHL_NONE,   EMC_CHL_PRI_1},
    {EMC_CHL_MAX,       EMC_CHL_NONE,   EMC_CHL_NONE,   EMC_CHL_NONE}
};

#include <linux/mtd/nand.h>
#include <asm/arch/regs_nfc.h>
#define NAND_CMD_RESET  (0xFF)
#define NF_MC_CMD_ID	(0xFD)
#define NF_MC_ADDR_ID	(0xF1)
#define NF_MC_WAIT_ID	(0xF2)
#define NF_MC_RWORD_ID	(0xF3)
#define NF_MC_RBLK_ID	(0xF4)
#define NF_MC_WWORD_ID	(0xF6)
#define NF_MC_WBLK_ID	(0xF7)
#define NF_MC_DEACTV_ID	(0xF9)
#define NF_MC_NOP_ID	(0xFA)
void send_reset_command(void)
{
	u8 mc_ins_num = 0;
	u32 ins, mode;
	unsigned int offset, high_flag, value;

	/* nfc_mcr_inst_add(cmd, NF_MC_CMD_ID); */
	ins       = NAND_CMD_RESET;
	mode      = NF_MC_CMD_ID;
	offset	  = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
	}
	REG32(NFC_START_ADDR0 + (offset << 2)) = value;
	mc_ins_num ++;

	/* nfc_mcr_inst_exc(); */
    value = REG32(NFC_CFG0);
    value |= NFC_BUS_WIDTH_16;
    value |= (1 << NFC_CMD_SET_OFFSET);

	REG32(NFC_CFG0) = value ;
    value = NFC_CMD_VALID | ((unsigned int)NF_MC_NOP_ID) |  ((mc_ins_num - 1) << 16);
    REG32(NFC_CMD) = value;
}
void send_readid_command(void)
{
	u8 mc_ins_num = 0;
	u32 ins, mode;
	unsigned int offset, high_flag, value;

	/* nfc_mcr_inst_add(cmd, NF_MC_CMD_ID); */
	ins = NAND_CMD_READID;
	mode = NF_MC_CMD_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
	}
	REG32(NFC_START_ADDR0 + (offset << 2)) = value;
	mc_ins_num ++;

	/* nfc_mcr_inst_add(0x00, NF_MC_ADDR_ID); */
	ins = 0x00;
	mode = NF_MC_ADDR_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
	}
	REG32(NFC_START_ADDR0 + (offset << 2)) = value;
	mc_ins_num ++;

	/* nfc_mcr_inst_add(7, NF_MC_RWORD_ID); */
	ins = 7;
	mode = NF_MC_RWORD_ID;
	offset = mc_ins_num >> 1;
	high_flag = mc_ins_num & 0x1;
	if (high_flag) {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0x0000ffff;
		value |= ins << 24;
		value |= mode << 16;
	} else {
		value = REG32(NFC_START_ADDR0 + (offset << 2));
		value &= 0xffff0000;
		value |= ins << 8;
		value |= mode;
	}
	REG32(NFC_START_ADDR0 + (offset << 2)) = value;
	mc_ins_num ++;
	/* nfc_mcr_inst_exc_for_id(); */
	value = REG32(NFC_CFG0);
	value &= ~NFC_BUS_WIDTH_16;
	value |= (1 << NFC_CMD_SET_OFFSET);

	REG32(NFC_CFG0) = value;
	value = NFC_CMD_VALID | ((unsigned int)NF_MC_NOP_ID) |((mc_ins_num - 1) << 16);
	REG32(NFC_CMD) = value;
}

void read_nand_id(uint8 *nand_id, uint8 id_len)
{
	int ik_cnt = 0;
	uint32 value;

	REG32(AHB_CTL0) |= BIT_8;//no BIT_9
	REG32(AHB_SOFT_RST) |= BIT_5;
	for(ik_cnt = 0; ik_cnt < 0x1000; ik_cnt++);
	REG32(AHB_SOFT_RST) &= ~BIT_5;
	value = REG32(NFC_CFG0);
	value |= NFC_WPN;
	REG32(NFC_CFG0) = value;
	REG32(NFC_TIMING) = ((12 << 0) | (7 << 5) | (10 << 10) | (6 << 16) | (5 << 21) | (7 << 26));
	REG32(NFC_TIMING+0X4) = 0xffffffff;//TIMEOUT
	send_reset_command();
	for(ik_cnt = 0; ik_cnt < 0x1000; ik_cnt++);//wait cmd finish
	send_readid_command();
	for(ik_cnt = 0; ik_cnt < 0x1000; ik_cnt++);//wait cmd finish
	value = REG32(NFC_CLR_RAW);
	REG32(NFC_CLR_RAW) = 0xFFFF0000;

	if (value & NFC_DONE_RAW)
	{
		memcpy(nand_id, (void *)NFC_MBUF_ADDR, id_len);
	}
}


PUBLIC EMC_PARAM_PTR EMC_GetPara(void)
{
	uint32 emc_index = 0;
	uint32 emc_index_max = 0;
	uint8 nand_id[5] = {0};
	static uint32 index = 0xFFFFFFFF;

	emc_index_max = sizeof(s_emc_parm) / sizeof(s_emc_parm[0]);

	if (index < emc_index_max)
	{
		return (EMC_PARAM_PTR)&s_emc_parm[index];
	}

	read_nand_id(nand_id, sizeof(nand_id));

	for (emc_index = 0; emc_index < emc_index_max; emc_index ++) {
		if ((s_emc_parm[emc_index].nand_id[0] == nand_id[0])
		        && (s_emc_parm[emc_index].nand_id[1] == nand_id[1])
		        && (s_emc_parm[emc_index].nand_id[2] == nand_id[2])
		        && (s_emc_parm[emc_index].nand_id[3] == nand_id[3])
		        && (s_emc_parm[emc_index].nand_id[4] == nand_id[4])
		    )
		{
		    break;
		}
	}

	if (emc_index == emc_index_max)
	{
		emc_index = 0;
	}

	index = emc_index;

    return (EMC_PARAM_PTR)&s_emc_parm[emc_index];
}

PUBLIC SDRAM_CFG_INFO_T_PTR SDRAM_GetCfg(void)
{
    return (SDRAM_CFG_INFO_T_PTR)&s_sdram_config_info;
}


PUBLIC SDRAM_TIMING_PARA_T_PTR SDRAM_GetTimingPara(void)
{
    return (SDRAM_TIMING_PARA_T_PTR)&s_sdram_timing_param;
}

#ifndef SDRAM_AUTODETECT_SUPPORT

PUBLIC SDRAM_CHIP_FEATURE_T_PTR SDRAM_GetFeature(void)
{
    return (SDRAM_CHIP_FEATURE_T_PTR)&s_sdram_feature;
}
#endif

PUBLIC EMC_CHL_INFO_PTR EMC_GetChlInfo(void)
{
    return (EMC_CHL_INFO_PTR)&s_emc_chl_info;
}

#ifdef   __cplusplus
}
#endif

