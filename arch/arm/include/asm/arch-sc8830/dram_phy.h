/******************************************************************************
 ** File Name:      dram_phy.h                                               *
 ** Author:         changde                                                   *
 ** DATE:           01/11/2013                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    Refer to JEDEC databook for detail                       *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 01/11/2013     changde.li       Create.                                   *
 ******************************************************************************/
#ifndef _DRAM_PHY_H_
#define _DRAM_PHY_H_
/*----------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **------------------------------------------------------------------------- */
#include "umctl2_reg.h"
/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **--------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/******************************************************************************
                          Macro define
******************************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define IS_LPDDR1(x) (((x&0xF00) == 0)? TRUE:FALSE)
#define IS_LPDDR2(x) (((x&0xF00) == 0X100)? TRUE:FALSE)
#define IS_LPDDR3(x) (((x&0xF00) == 0X200)? TRUE:FALSE)
#define IS_DDR2(x)   (((x&0xF00) == 0X300)? TRUE:FALSE)
#define IS_DDR3(x)   (((x&0xF00) == 0X400)? TRUE:FALSE)
/******************************************************************************
                            Enum define
******************************************************************************/
typedef enum
{
    //lpddr1
    DRAM_LPDDR1             = 0x000, /*ALso called mobileDDR*/
    DRAM_LPDDR1_1CS_2G_X32  = 0x001,
    DRAM_LPDDR1_2CS_4G_X32  = 0x002,
    //lpddr2
    DRAM_LPDDR2             = 0x100,    
    DRAM_LPDDR2_S2          = 0x101,
    DRAM_LPDDR2_S4          = 0x102,
    DRAM_LPDDR2_1CS_1G_X32  = 0x103,
    DRAM_LPDDR2_1CS_2G_X32  = 0x104,
    DRAM_LPDDR2_1CS_4G_X32  = 0x105,
    DRAM_LPDDR2_1CS_8G_X32  = 0x106,
    DRAM_LPDDR2_1CS_16G_X32 = 0x107,
    DRAM_LPDDR2_2CS_2G_X32  = 0x108,
    DRAM_LPDDR2_2CS_3G_X32  = 0x109,
    DRAM_LPDDR2_2CS_4G_X32  = 0x10A,
    DRAM_LPDDR2_2CS_5G_X32  = 0x10B,
    DRAM_LPDDR2_2CS_6G_X32  = 0x10C,
    DRAM_LPDDR2_2CS_8G_X32  = 0x10D,
    DRAM_LPDDR2_2CS_12G_X32 = 0x10E,
    DRAM_LPDDR2_2CS_16G_X32 = 0x10F,
    //lpddr3
    DRAM_LPDDR3             = 0x200,
    //ddr2
    DRAM_DDR2               = 0x300,
    //ddr3
    DRAM_DDR3               = 0x400,
    DRAM_DDR3_1CS_2G_X8     = 0x401,
    DRAM_DDR3_1CS_4G_X8     = 0x402,
    DRAM_DDR3_1CS_8G_X8     = 0x403,
    DRAM_DDR3_1CS_2G_X8_4P  = 0x404, //4-piece 2g bit ddr3 chips, 4 cs bonded into 1 cs, 8g bit together
    DRAM_DDR3_1CS_4G_X8_4P  = 0x405, //4-piece 4g bit ddr3 chips, 4 cs bonded into 1 cs, 16g bit together
    DRAM_DDR3_1CS_8G_X8_4P  = 0x406, //4-piece 8g bit ddr3 chips, 4 cs bonded into 1 cs, 32g bit together
    DRAM_DDR3_1CS_1G_X16    = 0x407,    
    DRAM_DDR3_1CS_2G_X16    = 0x408,
    DRAM_DDR3_1CS_4G_X16    = 0x409,
    DRAM_DDR3_1CS_8G_X16    = 0x40A,
    DRAM_DDR3_2CS_2G_X16    = 0x40B,    
    DRAM_DDR3_2CS_4G_X16    = 0x40C,
    DRAM_DDR3_2CS_8G_X16    = 0x40D,
    DRAM_DDR3_2CS_16G_X16   = 0x40E,
    
    DRAM_DDR3_1CS_1G_X16_2P = 0x40F, 
    DRAM_DDR3_1CS_2G_X16_2P = 0x410, //2-piece 2g bit ddr3 chips, 2 cs bonded into 1 cs, 4g bit together
    DRAM_DDR3_1CS_4G_X16_2P = 0x411, //2-piece 4g bit ddr3 chips, 2 cs bonded into 1 cs, 8g bit together
    DRAM_DDR3_1CS_8G_X16_2P = 0x412, //2-piece 8g bit ddr3 chips, 2 cs bonded into 1 cs, 16g bit together
}DRAM_TYPE_E;

typedef enum
{
    DRAM_BT_SEQ = 0, //burst type = sequential(default)
    DRAM_BT_INTER= 1 //burst type = interleaved
}DRAM_BURSTTYPE_E;

typedef enum
{
    DRAM_WRAP   = 0, //warp mode
    DRAM_NOWRAP = 1  //no warp mode
}DRAM_WC_E;

typedef enum
{
/*Driver strength for lpddr1*/
    LPDDR1_DS_FULL,
    LPDDR1_DS_HALF,
    LPDDR1_DS_QUARTER,
    LPDDR1_DS_OCTANT,
    LPDDR1_DS_THREE_QUATERS,
/*Driver strength for lpddr2*/
    LPDDR2_DS_34R3 = 0x01,
    LPDDR2_DS_40R  = 0x02,
    LPDDR2_DS_48R  = 0x03,
    LPDDR2_DS_60R  = 0x04,
    LPDDR2_DS_68R6 = 0x05,
    LPDDR2_DS_80R  = 0x06,
    LPDDR2_DS_120R = 0x07,
/*Driver strength for ddr3*/
	DDR3_DS_40R    = 0x00,
	DDR3_DS_34R3   = 0x01
}MEM_IODS_E;

/*Truth Table-Commands*/
typedef enum
{
    CMD_NOP     = 1,
    CMD_DESLECT,
    CMD_ACTIVE,
    CMD_READ,
    CMD_READ_AP,
    CMD_WRITE,
    CMD_WRITE_AP,
    CMD_ENTER_DP,
    CMD_PRECHARGE,
    CMD_PRECHARGE_ALL,
    CMD_AUTO_REFRESH,
    CMD_MRS,
}MEM_CMD_FUNCTION_E;

typedef enum
{
    MR_WRITE = 1,
    MR_READ,
}MR_WR_E;

typedef enum{
    CLK_24MHZ    = 24,
    CLK_26MHZ    = 26,
    CLK_38_4MHZ  = 38,
    CLK_48MHZ    = 48,
    CLK_64MHZ    = 64,
    CLK_76_8MHZ  = 77,
    CLK_96MHZ    = 96,
    CLK_100MHZ   = 100,    
    CLK_153_6MHZ = 154,
    CLK_150MHZ   = 150,    
    CLK_166MHZ   = 166,
    CLK_192MHZ   = 192,
	CLK_200MHZ   = 200,  
	CLK_250MHZ   = 250,
    CLK_332MHZ   = 332,  
    CLK_333MHZ   = 333,
    CLK_384MHZ	 = 384,
    CLK_400MHZ   = 400,
    CLK_427MHZ   = 427,
    CLK_450MHZ   = 450,
    CLK_464MHZ	 = 464,
    CLK_500MHZ   = 500,
    CLK_525MHZ   = 525,
    CLK_533MHZ   = 533,    
    CLK_537MHZ   = 537,
    CLK_540MHZ   = 540,
    CLK_550MHZ   = 550,
    CLK_800MHZ	 = 800,
    CLK_1000MHZ	 = 1000,
    EMC_CLK_400MHZ = 400,
    EMC_CLK_450MHZ = 450,
    EMC_CLK_500MHZ = 500
}CLK_TYPE_E;

typedef enum
{
	VDDMEM_1V15 = 0X00,
	VDDMEM_1V20 = 0X20,
	VDDMEM_1V21 = 0X22,
	VDDMEM_1V22 = 0X24,
	VDDMEM_1V25 = 0X28,
	VDDMEM_1V30 = 0X30,
	VDDMEM_1V35 = 0X38,
	VDDMEM_1V40 = 0X3f,
	VDDMEM_1V45 = 0X48,
	VDDMEM_1V50 = 0X50,
	VDDMEM_1V55 = 0X58,
	VDDMEM_1V60 = 0X60,			
}VDDMEM_TYPE_E;

typedef enum
{
	PUBL_DQS_STEP_MIN  = 0,
	PUBL_DQS_STEP_SUB3 = 0,
	PUBL_DQS_STEP_SUB2 = 1,
	PUBL_DQS_STEP_SUB1 = 2,
	PUBL_DQS_STEP_NOM  = 3,
	PUBL_DQS_STEP_DEF  = 3,
	PUBL_DQS_STEP_ADD1 = 4,
	PUBL_DQS_STEP_ADD2 = 5,
	PUBL_DQS_STEP_ADD3 = 6,
	PUBL_DQS_STEP_ADD4 = 7,
	PUBL_DQS_STEP_MAX  = 7
}PUBL_DQS_STEP_E;

//DQS gating phase select
typedef enum
{
	PUBL_DQS_PHS_MIN = 0,
	PUBL_DQS_PHS_90  = 0,
	PUBL_DQS_PHS_180 = 1,
	PUBL_DQS_PHS_DEF = 1,
	PUBL_DQS_PHS_270 = 2,
	PUBL_DQS_PHS_360 = 3,
	PUBL_DQS_PHS_MAX = 3
}PUBL_DQS_PHS_E;

//DQS gating system latency
typedef enum
{
	PUBL_DQS_CLK_MIN  = 0,
	PUBL_DQS_CLK_DEF  = 0,
	PUBL_DQS_CLK_1CLK = 1,
	PUBL_DQS_CLK_2CLK = 2,
	PUBL_DQS_CLK_3CLK = 3,
	PUBL_DQS_CLK_4CLK = 4,
	PUBL_DQS_CLK_5CLK = 5,
	PUBL_DQS_CLK_MAX  = 5	
}PUBL_DQS_CLK_E;

//slave dll phase trim
typedef enum
{
	PUBL_SDLL_PHS_MIN  = 0x0,
	PUBL_SDLL_PHS_DEF  = 0x0,
	PUBL_SDLL_PHS_36   = 0x3,
	PUBL_SDLL_PHS_54   = 0x2,
	PUBL_SDLL_PHS_72   = 0x1,
	PUBL_SDLL_PHS_90   = 0x0,
	PUBL_SDLL_PHS_108  = 0x4,
	PUBL_SDLL_PHS_126  = 0x8,
	PUBL_SDLL_PHS_144  = 0x12,
	PUBL_SDLL_PHS_MAX  = 0x12,	
}PUBL_SDLL_PHS_E;

typedef enum
{
	PUBL_LPDDR1_DS_33OHM = 0xa,
	PUBL_LPDDR1_DS_31OHM = 0xb,
	PUBL_LPDDR1_DS_48OHM = 0xc,
	PUBL_LPDDR1_DS_43OHM = 0xd,
	PUBL_LPDDR1_DS_39OHM = 0xe,
	PUBL_LPDDR1_DS_55OHM = 0x5,
	PUBL_LPDDR1_DS_64OHM = 0x4,

	PUBL_LPDDR2_DS_MIN   = 0xd,
	PUBL_LPDDR2_DS_34OHM = 0xd,
	PUBL_LPDDR2_DS_40OHM = 0xb,
	PUBL_LPDDR2_DS_48OHM = 0x9,
	PUBL_LPDDR2_DS_60OHM = 0x7,
	PUBL_LPDDR2_DS_80OHM = 0x5,
	PUBL_LPDDR2_DS_MAX   = 0x5,

	PUBL_DDR3_DS_34OHM   = 0xd,
	PUBL_DDR3_DS_40OHM   = 0xb

}PUBL_DS_E;

typedef enum
{
	DQS_PDU_MIN    = 1,
	DQS_PDU_688OHM = 1,
	DQS_PDU_611OHM = 2,
	DQS_PDU_550OHM = 3,
	DQS_PDU_500OHM = 4,
	DQS_PDU_DEF    = 4,
	DQS_PDU_458OHM = 5,
	DQS_PDU_393OHM = 6,
	DQS_PDU_344OHM = 7,	
	DQS_PDU_MAX    = 7
}DQS_PDU_E;

typedef enum
{
	UMCTL2_PORT_MIN     = 0,
	UMCTL2_PORT_AP      = 0,
	UMCTL2_PORT_MM      = 1,
	UMCTL2_PORT_GPU     = 2,
	UMCTL2_PORT_CP2_P0  = 3,
	UMCTL2_PORT_CP1     = 4,
	UMCTL2_PORT_AP_MTX  = 5,
	UMCTL2_PORT_CP0     = 6,
	UMCTL2_PORT_CP2_P1  = 7,
	UMCTL2_PORT_CP0_MTX = 8,	
	UMCTL2_PORT_DSP     = 9,
	UMCTL2_PORT_MAX     = 10
}UMCTL2_PORT_ID_E;

typedef enum
{
	MEM_CS_MIN = 0,
	MEM_CS0 = 0,
	MEM_CS1 = 1,
	MEM_CS_MAX = 1,
}MEM_CS_E;

typedef enum
{
	MEM_MR0 = 0,
	MEM_MR1 = 1,
	MEM_MR2 = 2,
	MEM_MR3 = 3,
	MEM_MR8 = 8,
	MEM_MR10 = 10,
}MEM_MR_E;

typedef enum
{
	MEM_MRW = 0,
	MEM_MRD = 1,
}MEM_CMD_E;

typedef enum
{
	PUBL_BYTE_MIN = 0,
	PUBL_BYTE0 = 0,
	PUBL_BYTE1 = 1,
	PUBL_BYTE2 = 2,
	PUBL_BYTE3 = 3,
	PUBL_BYTE_MAX = 3
}PUBL_BYTE_E;

typedef enum
{
	UMCTL_AUTO_PD_EN   = 1,
	UMCTL_AUTO_PD_DIS  = 0,
	UMCTL_AUTO_DPD_EN  = 1,
	UMCTL_AUTO_DPD_DIS = 0,
	UMCTL_AUTO_SF_EN   = 1,
	UMCTL_AUTO_SF_DIS  = 0,
	UMCTL_AUTO_CKP_EN  = 1,
	UMCTL_AUTO_CKP_DIS = 0,	
}UMCTL_LP_E;
/******************************************************************************
                            Structure define
******************************************************************************/
/*
 *Refer to JEDEC STANDARD JESD209B for LPDDR
 *Take LPDDR200 as a example. 
*/
typedef struct 
{
	uint32 tREFI;	 //average Refresh interval time between each row,normall = 7800 ns	
    uint32 tRAS;    //ACTIVE to PRECHARGE command period,(50~70000 ns)	
    uint32   tRC;     //ACTIVE to ACTIVE command period,(>=tRAS+tRP ns)    
    uint32   tRFC;    //AUTO REPRESH to ACTIVE/AUTO REPRESH command period,128M/256Mb(>=80 ns)
                                              //512Mb(>=110 ns)
                                              // 1Gb/2Gb(>=140 ns)
    uint32   tRCD;    //ACTIVE to READ or WRITE delay,(>=30 ns)
    uint32   tRP;     //PRECHARGE command period,(>=30 ns)
    uint32   tRRD;    //ACTIVE bank A to ACTIVE bank B delay,(>=15 ns)
    uint32   tWR;     //WRITE recovery time,(>=15 ns)
    uint32   tWTR;    //internal write to Read command delay,(>=1 tck)
    uint32   tXP;     //Exit power down to next valid command delay,(>=25 ns)

    uint32   tXSR;    //self refresh exit to next valid command delay,(>=200 ns)    
    uint32   tMRD;    //MODE REGISTER SET command period,(>=2 tck)
    uint32   tCKE;    //CKE min pulse width,(>=2 tck)
} lpddr1_timing_t;

/*
 *Refer to JEDEC STANDARD JESD209-2E for LPDDR2
 *Take LPDDR2-800 as a example. 
*/
typedef struct 
{
    /*LPDDR2 SDRAM Core Parameters*/
    /*uint32 RL;   Read Latency,>=6@800Mhz*/
    /*uint32 WL;   Write Latency,>=3@800Mhz*/
	uint32 tREFI;	// average Refresh interval time between each row,normall = 7800 ns	    
    uint32 tRAS; /*Row Active Time,>=3tCK*/	
    uint32 tRC;  /*ACTIVE to ACTIVE command period,>=(tRAS + tRPab)ns*/
    uint32 tRFCab; /*Refresh Cycle time tRFCab,64M~256Mb(>=90 ns)
                                          / 1Gb/2Gb/4G(>=110 ns)
                                          / 8Gb(>=210 ns) */
    uint32 tRFCpb;
    uint32 tRCD; /*RAS to CAS Delayy,>=3tCK*/
    uint32 tRP;  /*Preactive to Activate command period,>=3tCK*/
    uint32 tRRD; /*Active bank A to Active bank B,>=2tCK*/
    uint32 tWR;  /*Write Recovery Time,>=3tCK*/
    uint32 tWTR; /*Internal Write to Read Command Delay,>=2tCK*/
    uint32 tXP;  /*Exit power down to next valid command delay,>=2tCK*/
    
    uint32 tXSR; /*Self refresh exit to next valid command delay,>=2tCK*/    
    uint32 tCKESR;/*low pulse width during Self-Refresh,>=3tCK*/
    uint32 tCCD; /*CAS to CAS delay,LPDDR2-S4>=2tCK,LPDDR2-S2>=1tCK*/
    uint32 tRTP; /*Internal Read to Precharge command delay,>=2tCK*/
    uint32 tFAW; /*Four Bank Activate Window,>=8tCK*/
    uint32 tDPD; /*Minimum Deep Power Down Time,==500us*/

    /*ZQ Calibration Parameters*/
    uint32 tZQINIT; /*Initialization Calibration Time,>=1us*/
    uint32 tZQCL;
    uint32 tZQCS;
    uint32 tZQreset;
    /*Read Parameters*/
    uint32 tDQSCK; /*DQS output access time from CK_t/CK_c,(2500~5500)ps*/
    /*CKE Input Parameters*/
    uint32 tCKE; /*CKE min. pulse width (high and low pulse width),>=3tCK*/

    /*Command Address Input Parameters*/
    /*Boot Parameters (10 MHz - 55 MHz)*/
    uint32 tDQSCKmax;/*DQS Output Data Access Time from CK_t/CK_c,(2~10)ns*/
    
    /*Mode Register Parameters*/
    uint32 tMRW; /*MODE REGISTER Write command period,>=5tCK*/
    uint32 tMRR; /*Mode Register Read command period,>=2tCK*/
} lpddr2_timing_t;


/*
 *Refer to JEDEC STANDARD JESD79-3D 2008 for DDR3
 *
*/
typedef struct 
{
	uint32 tREFI;	// average Refresh interval time between each row,normall = 7800 ns	
    uint32 tRAS; /*ACTIVE to PRECHARGE command period,(35~9tREFI)*/
    uint32 tRC; /*ACT to ACT or REF command period,>=(45~49)ns*/
    uint32 tRFC;/*AUTO REPRESH to ACTIVE/AUTO REPRESH command period,
                                  /512Mb(>=90 ns) /1Gb(>=110 ns)
                                  /2Gb(>=160 ns)  /4Gb(>=300 ns)  
                                  / 8Gb(>=350 ns) */                                             
    uint32 tRCD; /*ACT to internal read or write delay time,,>=(11~15)ns*/                                  
    uint32 tRP; /*PRE command period,>=(11~14)ns*/
    uint32 tRRD; /*ACTIVE to ACTIVE command period for1/2KB page size,>=4tCK*/
    uint32 tWR;  /*WRITE recovery time,>=15ns*/
    uint32 tWTR; /*Delay from start of internal write transaction to internal read command,>=4tCK*/

    /*ZQ Calibration Parameters*/
    uint32 tZQINIT; /*Power-up and RESET calibration time,>=512tCK*/
    uint32 tZQoper;
    uint32 tZQCS;
    /*Data Strobe Timing*/
    uint32 tDQSCK; /*DQS,DQS# rising edge output access time from rising CK*/

    /*Command and Address Timing*/
    uint32 tDLLK; /*DLL locking time,>=512tCK*/
    uint32 tRTP; /*Internal READ Command to PRECHARGE Command delay,>=4tCK*/
    uint32 tMRD; /*Mode Register Set command cycle time,>=4tCK*/
    uint32 tMOD; /*Mode Register Set command update delay,>=12tCK*/
    uint32 tCCD; /*CAS# to CAS# command delay,>=4tCK*/
    uint32 tFAW; /*Four activate window for 1/2KB page size,>=40ns*/

    /*Self Refresh Timings*/
    uint32 tXS;/*Exit Self Refresh to commands not requiring a locked DLL,>=max(5nCK,tRFC(min)+10ns)*/
    uint32 tXP; /*max(3nCK,7.5ns)*/
    uint32 tXPDLL; /*max(10nCK,24ns)*/
    uint32 tCKSRE;/*Valid Clock Requirement after Self Refresh Entry (SRE) or Power-Down Entry (PDE),>=max(5nCK,10ns)*/
    uint32 tCKSRX;/*Valid Clock Requirement before Self Refresh Exit (SRX) or Power-Down Exit(PDX) or Reset Exit,>=max(5nCK,10ns)*/

    /*Power Down Timings*/
    /*tXP:Exit Power Down with DLL on to any valid command; 
      Exit Precharge Power Down with DLL frozen to commands notrequiring a locked DLL*/

    uint32 tCKE; /*CKE minimum pulse width,max(3nCK,7.5ns)*/
} ddr3_timing_t;

/*
 *Refer to JEDEC STANDARD JESDxx-xx 2008 for DDR2
 *
*/
typedef struct 
{
    /*Need to be done for details*/
    /* avoid compiling error.*/
    uint32 to_be_done;
    uint32 tMRD;
    uint32 tCCD;
    uint32 tXS;
    uint32 tAOND;
} DDR2_ACTIMING;

typedef struct 
{
    DRAM_TYPE_E  dram_type;/*dram type: lpddr1,lpddr2,ddr3*/
    uint32     cs_num;     //cs/ranks number.
    uint32      bank_num;   //bank number,lpddr1 and lpddr2 usually 4,ddr3 usually 8

    uint32      io_width;   /*data io width, x8/x16/x32*/
    uint32      bl;         /*burst lenght,usually=2,4,8,16*/
    uint32      rl;         /*read cas latency, usually=1,2,3,4,5,6,7,8*/
    uint32      wl;         /*write cas latency, usually=1,2,3,4,5,6,7,8 */
    uint32      al;         /*DDR2/DDR3 only,Posted CAS additive latency(AL)
                                  //For DDR3,two constrains below must be satisfied.
                                    1.CL=AL+1/2/0 for DDR3
                                    2.WL=AL+CWL,and CWL is 5~12tCK for DDR3
                           */ 
    void       *ac_timing; //AC character referring to SDRAM spec.
} DRAM_INFO;

typedef struct
{
    uint32 rdwr_order;
    uint32 rd_hpr;
    uint32 rd_pagematch;
    uint32 rd_urgent;
    uint32 rd_aging;
    uint32 rd_reorder_bypass;
    uint32 rd_priority;

    uint32 wr_pagematch;
    uint32 wr_urgent;
    uint32 wr_aging;
    uint32 wr_priority;    
}umctl2_port_info_t;

#ifdef DDR_DFS_SUPPORT
typedef struct
{
	uint32 ddr_clk;
	//umctl reg
	uint32 umctl2_rfshtmg;
	uint32 umctl2_init0;
	uint32 umctl2_init1;
	uint32 umctl2_init2;
	uint32 umctl2_init3;
	uint32 umctl2_init4;
	uint32 umctl2_init5;
	uint32 umctl2_dramtmg0;
	uint32 umctl2_dramtmg1;
	uint32 umctl2_dramtmg2;
	uint32 umctl2_dramtmg3;
	uint32 umctl2_dramtmg4;
	uint32 umctl2_dramtmg5;
	uint32 umctl2_dramtmg6;
	uint32 umctl2_dramtmg7;
	uint32 umctl2_dramtmg8;
	uint32 umctl2_dfitmg0;
	uint32 umctl2_dfitmg1;
	
	//publ reg
	uint32 publ_ptr0;
	uint32 publ_ptr1;
	uint32 publ_dtpr0;
	uint32 publ_dtpr1;
	uint32 publ_dtpr2;
	uint32 publ_mr0;
	uint32 publ_mr1;
	uint32 publ_mr2;
	uint32 publ_mr3;
	uint32 publ_dx0gcr;
	uint32 publ_dx1gcr;
	uint32 publ_dx2gcr;
	uint32 publ_dx3gcr;
	uint32 publ_dx0dqstr;
	uint32 publ_dx1dqstr;
	uint32 publ_dx2dqstr;
	uint32 publ_dx3dqstr;
}ddr_dfs_val_t;
#endif


extern umctl2_port_info_t UMCTL2_PORT_CONFIG[];


/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

