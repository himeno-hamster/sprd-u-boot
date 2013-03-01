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
#include <asm/arch/sci_types.h>
#include <asm/arch/umctl2_reg.h>
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

/******************************************************************************
                            Enum define
******************************************************************************/
typedef enum
{
    DRAM_LPDDR1     = 1, /*ALso called mobileDDR*/
    DRAM_LPDDR2_S2,
    DRAM_LPDDR2_S4,
    DRAM_LPDDR2,
    DRAM_DDR,
    DRAM_DDR2,
    DRAM_DDR3,
    DRAM_LPDDR3,
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
    DS_FULL,
    DS_HALF,
    DS_QUARTER,
    DS_OCTANT,
    DS_THREE_QUATERS,
/*Driver strength for lpddr2 or ddr3*/
    DS_34R3 = 0x01,
    DS_40R,
    DS_48R,
    DS_60R,
    DS_68R6,
    DS_80R,
    DS_120R,
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

/******************************************************************************
                            Structure define
******************************************************************************/
/*
 *Refer to JEDEC STANDARD JESD209B for LPDDR
 *Take LPDDR200 as a example. 
*/
typedef struct 
{
    uint32   tMRD;    //MODE REGISTER SET command period,(>=2 tck)
    uint32  tRAS;    //ACTIVE to PRECHARGE command period,(50~70000 ns)
    uint32   tRC;     //ACTIVE to ACTIVE command period,(>=tRAS+tRP ns)
    uint32   tRFC;    //AUTO REPRESH to ACTIVE/AUTO REPRESH command period,128M/256Mb(>=80 ns)
                                              //512Mb(>=110 ns)
                                              // 1Gb/2Gb(>=140 ns)
    uint32   tRCD;    //ACTIVE to READ or WRITE delay,(>=30 ns)
    uint32   tRP;     //PRECHARGE command period,(>=30 ns)
    uint32   tRRD;    //ACTIVE bank A to ACTIVE bank B delay,(>=15 ns)
    uint32   tWR;     //WRITE recovery time,(>=15 ns)
    uint32   tWTR;    //internal write to Read command delay,(>=1 tck)
    uint32   tXSR;    //self refresh exit to next valid command delay,(>=200 ns)
    uint32   tXP;     //Exit power down to next valid command delay,(>=25 ns)
    uint32   tCKE;    //CKE min pulse width,(>=2 tck)
} LPDDR_ACTIMING;

/*
 *Refer to JEDEC STANDARD JESD209-2E for LPDDR2
 *Take LPDDR2-800 as a example. 
*/
typedef struct 
{
    /*LPDDR2 SDRAM Core Parameters*/
    /*uint8 RL;   Read Latency,>=6@800Mhz*/
    /*uint8 WL;   Write Latency,>=3@800Mhz*/
    uint8 tRC;  /*ACTIVE to ACTIVE command period,>=(tRAS + tRPab)ns*/
    uint8 tCKESR;/*low pulse width during Self-Refresh,>=3tCK*/
    uint8 tXSR; /*Self refresh exit to next valid command delay,>=2tCK*/
    uint8 tXP;  /*Exit power down to next valid command delay,>=2tCK*/
    uint8 tCCD; /*CAS to CAS delay,LPDDR2-S4>=2tCK,LPDDR2-S2>=1tCK*/
    uint8 tRTP; /*Internal Read to Precharge command delay,>=2tCK*/
    uint8 tRCD; /*RAS to CAS Delayy,>=3tCK*/
    uint8 tRP;  /*Preactive to Activate command period,>=3tCK*/
    uint8 tRAS; /*Row Active Time,>=3tCK*/
    uint8 tWR;  /*Write Recovery Time,>=3tCK*/
    uint8 tWTR; /*Internal Write to Read Command Delay,>=2tCK*/
    uint8 tRRD; /*Active bank A to Active bank B,>=2tCK*/
    uint8 tFAW; /*Four Bank Activate Window,>=8tCK*/
    uint8 tDPD; /*Minimum Deep Power Down Time,==500us*/

    /*ZQ Calibration Parameters*/
    /*uint8 tZQINIT; Initialization Calibration Time,>=1us*/
    /*Read Parameters*/
    uint8 tDQSCK; /*DQS output access time from CK_t/CK_c,(2500~5500)ps*/
    /*CKE Input Parameters*/
    uint8 tCKE; /*CKE min. pulse width (high and low pulse width),>=3tCK*/

    /*Command Address Input Parameters*/
    /*Boot Parameters (10 MHz - 55 MHz)*/
    uint8 tDQSCKmax;/*DQS Output Data Access Time from CK_t/CK_c,(2~10)ns*/
    
    /*Mode Register Parameters*/
    uint8 tMRW; /*MODE REGISTER Write command period,>=5tCK*/
    uint8 tMRR; /*Mode Register Read command period,>=2tCK*/
    uint8 tRFC; /*Refresh Cycle time tRFCab,64M~256Mb(>=90 ns)
                                          / 1Gb/2Gb/4G(>=110 ns)
                                          / 8Gb(>=210 ns) */
} LPDDR2_ACTIMING;


/*
 *Refer to JEDEC STANDARD JESD79-3D 2008 for DDR3
 *
*/
typedef struct 
{
    /*ZQ Calibration Parameters*/
    /*uint8 tZQINIT; Power-up and RESET calibration time,>=512tCK*/
    /*Data Strobe Timing*/
    uint8 tDQSCK; /*DQS,DQS# rising edge output access time from rising CK*/

    /*Command and Address Timing*/
    uint8 tDLLK; /*DLL locking time,>=512tCK*/
    uint8 tRTP; /*Internal READ Command to PRECHARGE Command delay,>=4tCK*/
    uint8 tWTR; /*Delay from start of internal write transaction to internal read command,>=4tCK*/
    uint8 tWR;  /*WRITE recovery time,>=15ns*/
    uint8 tMRD; /*Mode Register Set command cycle time,>=4tCK*/
    uint8 tMOD; /*Mode Register Set command update delay,>=12tCK*/
    uint8   tRFC;/*AUTO REPRESH to ACTIVE/AUTO REPRESH command period,
                                  /512Mb(>=90 ns) /1Gb(>=110 ns)
                                  /2Gb(>=160 ns)  /4Gb(>=300 ns)  
                                  / 8Gb(>=350 ns) */                                             
    uint8 tRCD; /*ACT to internal read or write delay time,,>=(11~15)ns*/
    uint8 tRP; /*PRE command period,>=(11~14)ns*/
    uint8 tRC; /*ACT to ACT or REF command period,>=(45~49)ns*/
    uint8 tCCD; /*CAS# to CAS# command delay,>=4tCK*/
    uint8 tRAS; /*ACTIVE to PRECHARGE command period,(35~9tREFI)*/
    uint8 tRRD; /*ACTIVE to ACTIVE command period for1/2KB page size,>=4tCK*/
    uint8 tFAW; /*Four activate window for 1/2KB page size,>=40ns*/

    /*Self Refresh Timings*/
    uint8 tXS;/*Exit Self Refresh to commands not requiring a locked DLL,>=max(5nCK,tRFC(min)+10ns)*/
    uint8 tCKSRE;/*Valid Clock Requirement after Self Refresh Entry (SRE) or Power-Down Entry (PDE),>=max(5nCK,10ns)*/
    uint8 tCKSRX;/*Valid Clock Requirement before Self Refresh Exit (SRX) or Power-Down Exit(PDX) or Reset Exit,>=max(5nCK,10ns)*/

    /*Power Down Timings*/
    /*tXP:Exit Power Down with DLL on to any valid command; 
      Exit Precharge Power Down with DLL frozen to commands notrequiring a locked DLL*/
    uint8 tXP; /*max(3nCK,7.5ns)*/
    uint8 tCKE; /*CKE minimum pulse width,max(3nCK,7.5ns)*/
} DDR3_ACTIMING;

/*
 *Refer to JEDEC STANDARD JESDxx-xx 2008 for DDR2
 *
*/
typedef struct 
{
    /*Need to be done for details*/
    /* avoid compiling error.*/
    uint8 to_be_done;
    uint8 tMRD;
    uint8 tCCD;
    uint8 tXS;
    uint8 tAOND;
} DDR2_ACTIMING;

typedef struct 
{
    DRAM_TYPE_E  dram_type;/*dram type: lpddr1,lpddr2,ddr3*/
    uint32     cs_num;     //cs/ranks number.
    uint8      bank_num;   //bank number,lpddr1 and lpddr2 usually 4,ddr3 usually 8

    uint8      io_width;   /*data io width, x8/x16/x32*/
    uint8      bl;         /*burst lenght,usually=2,4,8,16*/
    uint8      rl;         /*read cas latency, usually=1,2,3,4,5,6,7,8*/
    uint8      wl;         /*write cas latency, usually=1,2,3,4,5,6,7,8 */
    uint8      al;         /*DDR2/DDR3 only,Posted CAS additive latency(AL)
                                  //For DDR3,two constrains below must be satisfied.
                                    1.CL=AL+1/2/0 for DDR3
                                    2.WL=AL+CWL,and CWL is 5~12tCK for DDR3
                           */ 
    void       *ac_timing; //AC character referring to SDRAM spec.
} DRAM_DESC;


/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

