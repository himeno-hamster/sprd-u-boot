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
#include <asm/arch/dram_phy.h>
#include <asm/arch/umctl2_reg.h>



/*----------------------------------------------------------------------------*
 *Configuration for SDRAM used --*--*--*--*--*--*--*--*--*--*--*--*--*--*--*--*
**----------------------------------------------------------------------------*/
#if 0
#ifdef  CONFIG_SP8830EC
/*--*/   MEM_IODS_E       MEM_IO_DS          =LPDDR2_DS_48R; //DS_34R3(lpddr2 used)                 /*--*--*--*/
#else
/*--*/   MEM_IODS_E       MEM_IO_DS          =LPDDR2_DS_40R; //DS_34R3(lpddr2 used)                 /*--*--*--*/
#endif
#endif
/*--*/   DRAM_BURSTTYPE_E MEM_BURST_TYPE     =DRAM_BT_SEQ;              /*--*--*--*/
/*--*/   DRAM_WC_E        MEM_WC_TYPE        =DRAM_WRAP;              /*--*--*--*/

/*--*/   uint32 DQS_PDU_RES = DQS_PDU_500OHM;	//dqs pull up and pull down resist
/*--*/   uint32 DQS_GATE_EARLY_LATE = 2;	                      

#if 0    
/*--*/   uint32 PUBL_LPDDR1_DS = PUBL_LPDDR1_DS_48OHM; //lpddr1 driver strength,refer to multiPHY p155
#ifdef  CONFIG_SP8830EC
/*--*/   uint32 PUBL_LPDDR2_DS = PUBL_LPDDR2_DS_48OHM; //lpddr2 driver strength,
#else
/*--*/   uint32 PUBL_LPDDR2_DS = PUBL_LPDDR2_DS_40OHM; //lpddr2 driver strength,
#endif
/*--*/   uint32 PUBL_DDR3_DS   = PUBL_DDR3_DS_34OHM;   //ddr3 driver strength,
#endif
    
/*--*/   uint32 B0_SDLL_PHS_DLY = PUBL_SDLL_PHS_DEF; //byte0 sll dll phase delay 
/*--*/   uint32 B1_SDLL_PHS_DLY = PUBL_SDLL_PHS_DEF; //byte1 sll dll phase delay 
/*--*/   uint32 B2_SDLL_PHS_DLY = PUBL_SDLL_PHS_DEF; //byte2 sll dll phase delay 
/*--*/   uint32 B3_SDLL_PHS_DLY = PUBL_SDLL_PHS_DEF; //byte3 sll dll phase delay 
    
/*--*/   uint32 B0_DQS_STEP_DLY = PUBL_DQS_STEP_DEF; //byte0 dqs step delay
/*--*/   uint32 B1_DQS_STEP_DLY = PUBL_DQS_STEP_DEF; //byte1 dqs step delay
/*--*/   uint32 B2_DQS_STEP_DLY = PUBL_DQS_STEP_DEF; //byte2 dqs step delay
/*--*/   uint32 B3_DQS_STEP_DLY = PUBL_DQS_STEP_DEF; //byte3 dqs step delay

/**---------------------------------------------------------------------------*
 **                            Macro Define
 **---------------------------------------------------------------------------*/
#define NS2CLK(x_ns) ((DDR_CLK*x_ns)/1000 + 1)


/**---------------------------------------------------------------------------*
 **                            Local Define
 **---------------------------------------------------------------------------*/
const lpddr1_timing_t LPDDR1_ACTIMING = 
{
    2,   		  //tMRD	unite : tCK
    NS2CLK(50),   //tRAS	unite : tCK
    NS2CLK(80),   //tRC		unite : tCK
    NS2CLK(140),  //tRFC	unite : tCK
    NS2CLK(30),   //tRCD	unite : tCK
    NS2CLK(30),   //tRP		unite : tCK
    NS2CLK(15),   //tRRD	unite : tCK
    NS2CLK(50),   //tWR		unite : tCK
    3,            //tWTR	unite : tCK
    NS2CLK(140),  //tXSR	unite : tCK
    NS2CLK(25),   //tXP		unite : tCK
    2             //tCKE	unite : tCK
};

const lpddr2_timing_t LPDDR2_ACTIMING = 
{
    NS2CLK(3900),   //tREFI     uinte : tCK
    NS2CLK(43),     //tRAS 		unite : tCK    
    NS2CLK(65),		//tRC 		unite : tCK
    NS2CLK(130),    //tRFCab    unite : tCK
    NS2CLK(60),     //tRFCpb    unite : tCK
    NS2CLK(20),     //tRCD      unite : tCK
    NS2CLK(20),     //tRP       unite : tCK
    NS2CLK(20),     //tRRD      unite : tCK
    //NS2CLK(15),     //tWR       unite : tCK
    //NS2CLK(8),     //tWTR      unite : tCK    
	8, 			    //tWR       unite : tCK
    5,              //tWTR      unite : tCK
    NS2CLK(8),     //tXP       unite : tCK
    NS2CLK(140),     //tXSR      unite : tCK  
    NS2CLK(15),      //tCKESR 	unite : tCK
    2,              //tCCD 		unite : tCK
    NS2CLK(8),      //tRTP 		unite : tCK
    NS2CLK(60),     //tFAW 		unite : tCK
    NS2CLK(500000), //tDPD 		unite : tCK
    NS2CLK(1000),   //tZQINIT	unite : tCK    
    NS2CLK(360),    //tZQCL		unite : tCK    
    NS2CLK(90),     //tZQCS		unite : tCK    
    NS2CLK(50),     //tZQreset  unite : tCK        
    NS2CLK(3),      //tDQSCK 	unite : tCK       
    3,              //tCKE 		unite : tCK     
    NS2CLK(6),      //tDQSCKmax unite : tCK
    5,              //tMRW 		unite : tCK
    2               //tMRR 		unite : tCK
};

#ifdef DDR_DFS_SUPPORT

#ifdef DDR_LPDDR2
const lpddr2_timing_t LPDDR2_ACTIMING_NATIVE = 
{
    (3900),   //tREFI     uinte : ns
    (43),     //tRAS 	unite : ns    
    (65),	  //tRC 		unite : ns
    (130),    //tRFCab    unite : ns
    (60),     //tRFCpb    unite : ns
    (20),     //tRCD      unite : ns
    (20),     //tRP       unite : ns
    (20),     //tRRD      unite : ns
	8, 		  //tWR       unite : tCK
    5,        //tWTR      unite : tCK
    (8),     //tXP       unite : ns
    (140),    //tXSR      unite : ns  
    (15),     //tCKESR 	unite : ns
    2,        //tCCD 	unite : tCK
    (8),      //tRTP 	unite : ns
    (60),     //tFAW 	unite : ns
    (500000), //tDPD 	unite : ns
    (1000),   //tZQINIT	unite : ns    
    (360),    //tZQCL	unite : ns    
    (90),     //tZQCS	unite : ns    
    (50),     //tZQreset  unite : ns        
    (3),      //tDQSCK 	unite : ns       
    3,        //tCKE 	unite : tCK     
    (6),      //tDQSCKmax unite : ns
    5,        //tMRW 	unite : tCK
    2         //tMRR 	unite : tCK
};
#endif

#ifdef DDR_DDR3
const ddr3_timing_t DDR3_ACTIMING_NATIVE = 
{
    (7800),//tREFI		unit : ns
    22,	 		//tRAS 		unit : tCK    
    30,	 		//tRC 		unit : tCK    
    190,	 	//tRFC 		unit : tCK    
    9, 	 		//tRCD 		unit : tCK
    9,    		//tRP 		unit : tCK
    5, 	 		//tRRD 		unit : tCK
    9,    		//tWR 		unit : tCK
    5,	 		//tWTR 		unit : tCK    
    512,  		//tZQINIT   	unit : tCK    
    256,  		//tZQoper   	unit : tCK        
    64,   		//tZQCS   	unit : tCK                
    5,  	 	//tDQSCK		unit : tCK
    512,  		//tDLLK		unit : tCK
    4,  	 	//tRTP  		unit : tCK
    4,    		//tMRD  		unit : tCK
    12,   		//tMOD  		unit : tCK
    4,    		//tCCD  		unit : tCK
    30, 	 	//tFAW  		unit : tCK    
    200,	 	//tXS   		unit : tCK    
    5,    		//tXP   		unit : tCK
    13,   		//tXPDLL		unit : tCK
    5,    		//tCKSRE  	unit : tCK
    5,    		//tCKSRX   	unit : tCK
    3     		//tCKE	   	unit : tCK
};
#endif

#endif
const ddr3_timing_t DDR3_ACTIMING = 
{
#if 1
	NS2CLK(7800),//tREFI		unit : ns
	NS2CLK(38), //tRAS		unit : ns
	NS2CLK(53), //tRC		unit : ns
	NS2CLK(350),//tRFC		unit : ns
	NS2CLK(15),	//tRCD		unit : ns
	NS2CLK(15),	//tRP		unit : ns
	5,			//tRRD		unit : tCK
	9,			//tWR		unit : tCK
	5,			//tWTR		unit : tCK	  
	512,		//tZQINIT	unit : tCK	  
	256,		//tZQoper	unit : tCK		  
	64,			//tZQCS 		unit : tCK				  
	0,			//tDQSCK		unit : tCK
	512,		//tDLLK 		unit : tCK
	5,			//tRTP		unit : tCK
	5,			//tMRD		unit : tCK
	13, 		//tMOD		unit : tCK
	5,			//tCCD		unit : tCK
	NS2CLK(50), //tFAW		unit : ns
	NS2CLK(360),//tXS		unit : ns
	5,			//tXP		unit : tCK
	13, 		//tXPDLL		unit : tCK
	6,			//tCKSRE		unit : tCK
	6,			//tCKSRX		unit : tCK
	4			//tCKE		unit : tCK
#else
    NS2CLK(7800),//tREFI		unit : ns
    22,	 		//tRAS 		unit : tCK    
    30,	 		//tRC 		unit : tCK    
    190,	 	//tRFC 		unit : tCK    
    9, 	 		//tRCD 		unit : tCK
    9,    		//tRP 		unit : tCK
    5, 	 		//tRRD 		unit : tCK
    9,    		//tWR 		unit : tCK
    5,	 		//tWTR 		unit : tCK    
    512,  		//tZQINIT   	unit : tCK    
    256,  		//tZQoper   	unit : tCK        
    64,   		//tZQCS   	unit : tCK                
    5,  	 	//tDQSCK		unit : tCK
    512,  		//tDLLK		unit : tCK
    4,  	 	//tRTP  		unit : tCK
    4,    		//tMRD  		unit : tCK
    12,   		//tMOD  		unit : tCK
    4,    		//tCCD  		unit : tCK
    30, 	 	//tFAW  		unit : tCK    
    200,	 	//tXS   		unit : tCK    
    5,    		//tXP   		unit : tCK
    13,   		//tXPDLL		unit : tCK
    5,    		//tCKSRE  	unit : tCK
    5,    		//tCKSRX   	unit : tCK
    3     		//tCKE	   	unit : tCK
#endif    
};

DRAM_INFO DRAM_INFO_ARRAY[] =
{ 
/*-dram_type-----------cs_num--bank_num--BUS_WIDTH--BL-CL/RL-WL-AL-actiming-*/
  //{DRAM_LPDDR2_1CS_2G_X32,   1,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},
  {DRAM_LPDDR2_1CS_4G_X32,   1,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},
  {DRAM_LPDDR2_1CS_8G_X32,   1,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},  	
  //{DRAM_LPDDR2_2CS_4G_X32,   2,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},  	  	
  {DRAM_LPDDR2_2CS_6G_X32,   2,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},  	
  {DRAM_LPDDR2_2CS_8G_X32,   2,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},
  //{DRAM_LPDDR2_2CS_12G_X32,  2,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},  	
  //{DRAM_LPDDR2_2CS_16G_X32,  2,   8,      32,        4,   8,  4, 0, (void *)(&LPDDR2_ACTIMING)},
  {DRAM_DDR3_1CS_2G_X8_4P,   1,   8,	  32,	     8,   6,  5, 0, (void *)(&DDR3_ACTIMING)},
  {DRAM_DDR3_1CS_4G_X16_2P,  1,   8,	  32,	     8,   6,  5, 0, (void *)(&DDR3_ACTIMING)}  
};

umctl2_port_info_t UMCTL2_PORT_CONFIG[] = 
{
//rw_order r_hpr r_pg  r_ugent r_age r_rord_bp r_age_cnt, w_pg  w_ugent w_age  w_age_cnt
    {TRUE, FALSE,TRUE, FALSE,  TRUE,  FALSE,     0x040,   TRUE, FALSE,  TRUE, 0x040},//port0,mm/dcam/vsp
    {TRUE, FALSE,TRUE, FALSE,  FALSE, FALSE,     0x100,   TRUE, FALSE,  FALSE,0x100},//port1,GPU
    {TRUE, TRUE, TRUE, FALSE,  TRUE,  FALSE,     0x020,   TRUE, FALSE,  TRUE, 0x020},//port2,display/gsp
    {TRUE, FALSE,TRUE, FALSE,  FALSE, FALSE,     0x100,   TRUE, FALSE,  FALSE,0x100},//port3,CA7
    {TRUE, TRUE, TRUE, FALSE,  TRUE,  FALSE,     0x002,   TRUE, FALSE,  TRUE, 0x002},//port4,CPx DSP
    {TRUE, TRUE, TRUE, FALSE,  TRUE,  FALSE,     0x008,   TRUE, FALSE,  TRUE, 0x008},//port5,CP0W
    {TRUE, TRUE, TRUE, FALSE,  TRUE,  FALSE,     0x010,   TRUE, FALSE,  TRUE, 0x010},//port6,CP0 ARM
    {TRUE, FALSE,TRUE, FALSE,  TRUE,  FALSE,     0x080,   TRUE, FALSE,  TRUE, 0x080},//port7,AP matrix    
    {TRUE, TRUE, TRUE, FALSE,  TRUE,  FALSE,     0x008,   TRUE, FALSE,  TRUE, 0x008},//port8,CP1 ARM
    {TRUE, TRUE, TRUE, FALSE,  TRUE,  FALSE,     0x010,   TRUE, FALSE,  TRUE, 0x010}, //port9,CP2
    {0xff},
};

/**---------------------------------------------------------------------------*
 **                            PUBLIC Functions
 **---------------------------------------------------------------------------*/
PUBLIC DRAM_INFO* get_dram_cfg(DRAM_TYPE_E dram_type)
{
    uint i=0;

#if 0
    return (DRAM_INFO*)(&DRAM_INFO_ARRAY[0]);
#else
    for(i=0; i<ARRAY_SIZE(DRAM_INFO_ARRAY); i++) 
	{
        if(DRAM_INFO_ARRAY[i].dram_type == dram_type)
        {
			return (DRAM_INFO*)(&DRAM_INFO_ARRAY[i]);
        }
    }
	
	return (DRAM_INFO*)(&DRAM_INFO_ARRAY[0]);	
    
#endif
}


