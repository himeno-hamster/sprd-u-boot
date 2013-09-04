/*****************************************************************************
 ** File Name:        emc.c
 ** Author:           Johnny Wang
 ** DATE:             2012/12/04
 ** Copyright:        2007 Spreatrum, Incoporated. All Rights Reserved.
 ** Description:
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History
 **------------------------------------------------------------------------
 ** DATE          NAME            DESCRIPTION
 ** 2012/12/04                    Create.
 ******************************************************************************/
#include "asm/arch/dram_phy.h"
#include "asm/arch/umctl2_reg.h"
#include "asm/arch/ddr_scan.h"


#ifdef   __cplusplus
extern   "C"
{
#endif


extern void ddr_print(const unsigned char *string);

#if 1
    static uint32 G_P1 = 0; //just for debug
    static uint32 G_P2 = 0;
    static uint32 G_PHS = 0;
    static uint32 G_STP = 0;
#endif
uint32 __phs2par(SCAN_PHS_E sdll_phs)
{
	switch(sdll_phs)
	{
	    case SCAN_PHS_36  : return 3;
	    case SCAN_PHS_54  : return 2;
	    case SCAN_PHS_72  : return 1;
	    case SCAN_PHS_90  : return 0;
	    case SCAN_PHS_108 : return 4;
	    case SCAN_PHS_126 : return 8;
	    case SCAN_PHS_144 : return 12;
	    default : return 0;
	}
}
extern  wait_pclk(uint32 n);
extern  reg_bits_set(uint32 addr, uint32 start_bit, uint32 bit_num, uint32 value);

void set_publ_timing(DXN_E bytex,SCAN_STEP_E publ_dqs_step,SCAN_PHS_E  publ_sdll_phs)
{
	uint32 dxn_dqstr = PUBL_DX0DQSTR+0X10*4*bytex;
	uint32 dxn_dllcr = PUBL_DX0DLLCR+0X10*4*bytex;

	reg_bits_set(dxn_dqstr,20,3,publ_dqs_step);
	reg_bits_set(dxn_dqstr,23,3,publ_dqs_step);

	reg_bits_set(dxn_dllcr,14,4,__phs2par(publ_sdll_phs));
	reg_bits_set(dxn_dllcr,30,1,0);

	wait_pclk(50);
	reg_bits_set(dxn_dllcr,30,1,1);
	wait_pclk(50);
    }
    BOOLEAN publ_get_scan_result()
    {
	volatile uint32 bist_result;

	do{bist_result = REG32(PUBL_BISTGSR);}
	while((bist_result&0x1) == 0);

	if(bist_result == 1)
	{
	    return TRUE;
	}
	else
	{
	    return FALSE;
	}
}


void publ_do_itmsrst()
{
	reg_bits_set(PUBL_PIR,4,1,1);
	reg_bits_set(PUBL_PIR,0,1,1);
	wait_pclk(10);
	while((REG32(PUBL_PIR)&BIT_1) == BIT_1);
	wait_pclk(1500);
}

void publ_do_bist_stop()
{
	reg_bits_set(PUBL_BISTRR, 0, 3, 2);
	wait_pclk(10);
}

void publ_do_bist_reset()
{
	reg_bits_set(PUBL_BISTRR, 0, 3, 3);
	wait_pclk(10);
}

void publ_do_dram_bist(
	uint32 start_col, uint32 start_row, uint32 start_bank, uint32 start_cs,
    uint32 end_col, uint32 end_row, uint32 end_bank, uint32 end_cs,
    uint32 byte_lane,
    uint32 infinite_mode)
{
	reg_bits_set(PUBL_BISTAR0, 0,  12, start_col);
	reg_bits_set(PUBL_BISTAR0, 12, 16, start_row);
	reg_bits_set(PUBL_BISTAR0, 28,  4, start_bank);

	reg_bits_set(PUBL_BISTAR1, 0,  2, start_cs);
	reg_bits_set(PUBL_BISTAR1, 2,  2, end_cs);
	reg_bits_set(PUBL_BISTAR1, 4, 12, 4);

	reg_bits_set(PUBL_BISTAR2, 0,  12, end_col);
	reg_bits_set(PUBL_BISTAR2, 12, 16, end_row);
	reg_bits_set(PUBL_BISTAR2, 28,  4, end_bank);

	reg_bits_set(PUBL_BISTRR,  3,  1, 1);
	reg_bits_set(PUBL_BISTRR,  4,  1, infinite_mode);
	reg_bits_set(PUBL_BISTRR,  5,  8, 1);
	reg_bits_set(PUBL_BISTRR, 13,  1, 1);
	reg_bits_set(PUBL_BISTRR, 14,  2, 1);
	reg_bits_set(PUBL_BISTRR, 17,  2, 2);
	reg_bits_set(PUBL_BISTRR, 19,  4, byte_lane);

	reg_bits_set(PUBL_BISTWCR, 0, 16, 32764);

	REG32(PUBL_BISTLSR) = 0x1234abcd;
	reg_bits_set(PUBL_BISTRR, 0, 3, 1);
}


void ddr_scan_one_byte(MEM_CS_E cs,DXN_E publ_byte,DRAM_INFO* dram_info)
{
	uint32 i = 0;

	uint8 sdll_phs     = SCAN_PHS_DEF;
	uint8 sdll_phs_str = SCAN_PHS_MIN;
	uint8 sdll_phs_end  = SCAN_PHS_MAX;

	uint8 dqs_step      = SCAN_STEP_DEF;
	uint8 dqs_step_str  = SCAN_STEP_MIN;
	uint8 dqs_step_end  = SCAN_STEP_MAX;

	uint8 scan_result[SCAN_LEN] = {SCAN_FAIL};
	uint8 scan_middle = SCAN_LEN>1;

	uint32 col_str = 0, row_str = 0,bank_str = 0;
	uint32 col_end = 0, row_end = 0,bank_end = 0;


	if(dram_info->dram_type == DRAM_LPDDR2_1CS_4G_X32 ||
	   dram_info->dram_type == DRAM_LPDDR2_1CS_8G_X32 ||
	   dram_info->dram_type == DRAM_LPDDR2_2CS_6G_X32 ||
	   dram_info->dram_type == DRAM_LPDDR2_2CS_8G_X32)
	{
	    col_end = 0x3fc;
	    row_end = 0x3ffc;
	    bank_end = dram_info->bank_num-1;
	}
	else
	{		
	    col_end = 0x1fc;
	    row_end = 0x3ffc;
	    bank_end = dram_info->bank_num-1;
	}

	for(sdll_phs = sdll_phs_str; sdll_phs <= sdll_phs_end; sdll_phs++ )
	{
	    for(dqs_step = dqs_step_str; dqs_step <= dqs_step_end; dqs_step++)
	    {
			set_publ_timing(publ_byte,dqs_step,sdll_phs);

			publ_do_itmsrst();

			publ_do_bist_stop();

			publ_do_bist_reset();

			publ_do_dram_bist(
				col_str,row_str,bank_str,cs, //strat address
				col_end,row_end,bank_end,cs, //end address
				publ_byte,                   //byte lane index
				0);                          //infinite mode
			wait_pclk(50);
			scan_result[sdll_phs*7+dqs_step] = publ_get_scan_result();
	    }
	}

	{

	    volatile uint8 p1=0,p2=0;
	    volatile uint8 p1_temp = 0,p2_temp = 0;

	    //normal conditionl:
	    //f f f f p p p f f f f p p p p p f f
	    //        p1  p2        p1      p2
	    for(i = 0; i <( SCAN_LEN-1 ); i++)
	    {
			if(scan_result[i] != scan_result[i+1])
			{
			    if(scan_result[i] == SCAN_FAIL)
			    {
					p1_temp = p1;
					p1 = i+1;
			    }
			    else
			    {
					p2_temp = p2;
					p2 = i;

					if( (p2-p1)<=(p2_temp-p1_temp))
					{
					    p1 = p1_temp;
					    p2 = p2_temp;
					}
			    }
			}
	    }

	    //abnormal condition 1 :
	    //p p p p p p p p p p f f p p p p
	    //                  p2    p1
	    //abnormal condition 2 :
	    //f f f f f f f f f f f f p p p p
	    //					      p1
	    if(p2 < p1)
	    {
			p2_temp = p2;
			p2 = SCAN_LEN -1;

			if( (p2-p1)<=(p2_temp<p1_temp))
			{
			    p1 = p1_temp;
			    p2 = p2_temp;
			}
	    }

	    if ((p2 - p1) < 28) 
		{
			if ((p2 - p1) > 20 ) 
			{
			    ddr_print("\r\nWARNING!!! DDR SCAN LEN LESS TAHN 28");
			} 
			else 
			{
			    ddr_print("\r\nERROR!!! DDR SCAN LEN LESS TAHN 20");
			}
	    }

	    scan_middle = p1+ ((p2-p1)>>1);

		G_P1 = p1;
		G_P2 = p2;
	}
	dqs_step = scan_middle%(SCAN_STEP_MAX+1);
	sdll_phs = scan_middle>>3;

	set_publ_timing(publ_byte,dqs_step,sdll_phs);

	publ_do_itmsrst();

	G_PHS = sdll_phs;
	G_STP = dqs_step;
}

#define IRAM_DDR_DATA_ADDR 0x1f00
void ddr_scan(DRAM_INFO* dram_info)
{
	DXN_E byte = 0;

	for(byte = DXN0; byte<DXN_MAX; byte++)
	{
	    ddr_scan_one_byte(MEM_CS0,byte,dram_info);
#if 1
	    REG32(IRAM_DDR_DATA_ADDR+byte*0x10+0x0) = G_P1;
	    REG32(IRAM_DDR_DATA_ADDR+byte*0x10+0x4) = G_P2;
	    REG32(IRAM_DDR_DATA_ADDR+byte*0x10+0x8) = G_PHS;
	    REG32(IRAM_DDR_DATA_ADDR+byte*0x10+0xc) = G_STP;
#endif
	}

}


#ifdef   __cplusplus
}
#endif
