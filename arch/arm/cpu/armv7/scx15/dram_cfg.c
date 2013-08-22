/******************************************************************************
 ** File Name:      dram_cfg.c                                               *
 ** Author:         changde                                                   *
 ** DATE:           01/11/2013                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    Refer to uMCTL2 databook for detail                       *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 01/11/2013     changde.li       Create.                                   *
 ******************************************************************************/
#include <asm/arch-scx15/umctl2_reg.h>
#include <asm/arch-scx15/dram_phy.h>

/**---------------------------------------------------------------------------*
 **                            Macro Define
 **---------------------------------------------------------------------------*/
#define NS2CLK(x_ns) ((SDRAM_CORE_CLK*x_ns)/1000 + 1)
/**---------------------------------------------------------------------------*
 **                            Local Protocol
 **---------------------------------------------------------------------------*/
typedef enum {
	NORMAL_LPDDR1_JEDEC = 1,
	NORMAL_LPDDR1_1CS_1G_32BIT,
	NORMAL_LPDDR1_1CS_2G_32BIT,
	NORMAL_LPDDR1_2CS_4G_32BIT,
	NORMAL_LPDDR2_1CS_4G_32BIT,
	NORMAL_LPDDR2_2CS_8G_32BIT,
	HYNIX_LPDDR1_H9DA4GH4JJAMCR4EM,
	SAMSUNG_LPDDR2_KMKJS000VM,
} SDRAM_USED_E;

/*----------------------------------------------------------------------------*
 *Configuration for SDRAM used --*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*
**----------------------------------------------------------------------------*/
/*--*/
#define          SDRAM_CORE_CLK 200			      /*--*--*--*/
/*--*/
#define          SDRAM_USING    NORMAL_LPDDR1_JEDEC		    /*--*--*--*/
					/*--*/ MEM_IODS_E IO_DS = DS_HALF;
					//DS_34R3(lpddr2 used)                 /*--*--*--*/
								    /*--*/ DRAM_BURSTTYPE_E BURST_TYPE = DRAM_BT_SEQ;
								    /*--*--*--*/
/*----------------------------------------------------------------------------*
 *Configuration for SDRAM used End--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Local Define
 **---------------------------------------------------------------------------*/
LPDDR_ACTIMING lpddr1_jedec_actiming = {
//  tCK  ns   ns  ns   ns   ns  ns   ns  tCK  ns   ns  tCK 
//  tMRD tRAS tRC tRFC tRCD tRP tRRD tWR tWTR tXSR tXP tCKE
	2,
	NS2CLK(50),
	NS2CLK(80),
	NS2CLK(140),
	NS2CLK(30),
	NS2CLK(30),
	NS2CLK(15),
	NS2CLK(50),
	3,
	NS2CLK(140),
	NS2CLK(25),
	2,
};

LPDDR2_ACTIMING lpddr2_jedec_actiming = {
//  ns  tCK    tCK  tCK tCK  tCK  tCK  tCK tCK  tCK tCK  tCK  tCK  tCK  ps     tCK  ns        tCK  tCK  ns
//  tRC tCKESR tXSR tXP tCCD tRTP tRCD tRP tRAS tWR tWTR tRRD tFAW tDPD tDQSCK tCKE tDQSCKmax tMRW tMRR tRFC
	NS2CLK(80),
	4,
	3,
	3,
	2,
	4,
	4,
	4,
	4,
	6,
	3,
	3,
	9,
	NS2CLK(600000),
	NS2CLK(5),
	4,
	NS2CLK(5),
	6,
	3,
	NS2CLK(220),
};

DDR3_ACTIMING ddr3_jedec_actiming = {
//  tCK    tCK    tCK  tCK  tCK tCK  tCK  tCK  tCK  tCK tCK tCK  tCK  tCK  tCK  ns  tCK    tCK    tCK tCK
//  tDQSCK tDLLK  tRTP tWTR tWR tMRD tMOD tRFC tRCD tRP tRC tCCD tRAS tRRD tFAW tXS tCKSRE tCKSRX tXP tCKE
	0, 529, 5, 5,
	NS2CLK(20),
	5, 12, NS2CLK(350),
	NS2CLK(12),
	NS2CLK(12),
	NS2CLK(46),
	4, 100,
	4, NS2CLK(50),
	10, 10, 10,
	5, 3
};

DRAM_DESC DRAM_INFO_ARRAY[] = {
/*-dram_type-----cs_num--bank_num--BUS_WIDTH--BL-CL/RL-WL-AL-actiming-*/
	{DRAM_LPDDR1, 2, 4, 32, 8, 3, 1, 0, &lpddr1_jedec_actiming},	//NORMAL_LPDDR1_1CS_1G_32BIT
//{DRAM_LPDDR1,    1,      4,         32,       2,   3,  2, 0, &lpddr1_jedec_actiming},//NORMAL_LPDDR1_1CS_2G_32BIT    
//{DRAM_LPDDR1,    2,      4,         32,       2,   3,  2, 0, &lpddr1_jedec_actiming},//NORMAL_LPDDR1_2CS_4G_32BIT
	{DRAM_LPDDR2, 2, 8, 32, 4, 6, 3, 0, &lpddr2_jedec_actiming},	//NORMAL_LPDDR2_1CS_4G_32BIT
	{DRAM_LPDDR2_S4, 2, 4, 32, 4, 6, 3, 0, &lpddr2_jedec_actiming},	//NORMAL_LPDDR2_2CS_8G_32BIT
	{DRAM_LPDDR1, 2, 4, 32, 4, 3, 0, 0, NULL},	//HYNIX_LPDDR1_H9DA4GH4JJAMCR4EM
	{DRAM_LPDDR2_S4, 2, 4, 32, 4, 6, 3, 0, NULL},	//SAMSUNG_LPDDR2_KMKJS000VM
	{DRAM_LPDDR3, 2, 4, 32, 8, 14, 22, 12, NULL},	//CL=14(DDR3-2133),CL=13(DDR3-1866),CL=11(DDR3-1600),,CL=9(DDR3-1600),CL=7(DDR3-1066)
	//CL=AL+1/2/0 for DDR3
	//WL=AL+CWL,and CWL is 5~12tCK for DDR3
};

/**---------------------------------------------------------------------------*
 **                            PUBLIC Functions
 **---------------------------------------------------------------------------*/
PUBLIC DRAM_DESC *SDRAM_GetCfg()
{
	uint i = 0;

#if 1
	return (DRAM_DESC *) (&DRAM_INFO_ARRAY[0]);
#else
	for (i = 0; i < ARRAY_SIZE(DRAM_INFO_ARRAY); i++) {
		if (DRAM_INFO_ARRAY[i].dram_index == SDRAM_USING)
			break;
	}
	if (i == ARRAY_SIZE(DRAM_INFO_ARRAY))
		i = 0;

	return (DRAM_DESC *) (&DRAM_INFO_ARRAY[i]);
#endif
}
