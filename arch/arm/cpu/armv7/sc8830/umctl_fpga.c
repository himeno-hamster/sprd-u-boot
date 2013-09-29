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

#include <asm/arch-sc8830/umctl2_reg_fpga.h>
#include <asm/arch-sc8830/dram_phy_fpga.h>

/*With unit of Mhz*/
#define          SDRAM_CORE_CLK 200
/**---------------------------------------------------------------------------*
 **                            Macro Define
 **---------------------------------------------------------------------------*/
#define REG32(x)                           (*((volatile uint32 *)(x)))
#define UMCTL_REG_GET(reg_addr)             (*((volatile uint32 *)(reg_addr)))
#define UMCTL_REG_SET( reg_addr, value )    *(volatile uint32 *)(reg_addr) = value

#define OPERATION_MODE_INIT 0x01
#define OPERATION_MODE_NORMAL 0x02
#define OPERATION_MODE_PD 0x03
#define OPERATION_MODE_SR 0x04
#define OPERATION_MODE_DPD 0x05

#define NS2CLK(x_ns) ((SDRAM_CORE_CLK*x_ns)/1000 + 1)

/**---------------------------------------------------------------------------*
 **                            Extern Declare
 **---------------------------------------------------------------------------*/
extern MEM_IODS_E IO_DS;
extern DRAM_BURSTTYPE_E BURST_TYPE;
const uint32 SDRAM_BASE = 0x80000000;
/**---------------------------------------------------------------------------*
 **                            Local Variables
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Local Functions
 **---------------------------------------------------------------------------*/

static uint32 reg_bits_set(uint32 addr,
			   uint8 start_bitpos, uint8 bit_num, uint32 value)
{
	/*create bit mask according to input param */
	uint32 bit_mask = (1 << bit_num) - 1;
	uint32 reg_data = *((volatile uint32 *)(addr));

	reg_data &= ~(bit_mask << start_bitpos);
	reg_data |= ((value & bit_mask) << start_bitpos);

	*((volatile uint32 *)(addr)) = reg_data;
}

static uint32 ns_to_xclock(uint32 time_ns)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_ns) / 1000 + 1;
	return (clk_num);
}

static uint32 us_to_xclock(uint32 time_us)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_us);
	return (clk_num);
}

static uint32 ns_to_x1024(uint32 time_ns)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_ns) / 1000 + 1;
	return (clk_num / 1024 + 1);
}

static uint32 us_to_x1024(uint32 time_us)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_us);
	return (clk_num / 1024 + 1);
}

static uint32 ms_to_x1024(uint32 time_ms)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_ms) * 1000;
	return (clk_num / 1024 + 1);
}

static uint32 ns_to_x32(uint32 time_ns)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_ns) / 1000 + 1;
	return (clk_num / 32 + 1);
}

static uint32 us_to_x32(uint32 time_us)
{
	uint32 clk_num = (SDRAM_CORE_CLK * time_us);
	return (clk_num / 32 + 1);
}

/*
 *Read/write mode register in ddr memory.
 *Refer to uMCTL2 databook Chapter2.18 "Mode Register Reads and Writes"
*/
static uint32 mr_rw(uint32 dram_type, uint32 mr_ranks, uint32 rw_flag,
		    uint32 mr_addr, uint32 mr_data)
{
#if 0
//changde
	uint32 mrctrl0 = 0, mrctrl1 = 0;

	/* checking that there is no outstanding MR tansacton.MCTL_MRSTAT.[mr_wr_busy] */
	while (UMCTL_REG_GET(UMCTL_MRSTAT) & BIT_0) ;

	mrctrl0 = (mr_addr << 12) | ((((mr_ranks == MR_RANKS_0ONLY) ? 0x01 : 0x00) | ((mr_ranks == MR_RANKS_1ONLY) ? 0x02 : 0x00) | ((mr_ranks == MR_RANKS_0AND2) ? 0x05 : 0x00) | ((mr_ranks == MR_RANKS_1AND3) ? 0x0A : 0x00) | ((mr_ranks == MR_RANKS_ALL) ? 0x0F : 0x00)) << 4) | (((rw_flag == MR_READ) ? 1 : 0) << 0);	/* Only used for LPDDR2/LPDDR3 */
	/* mr_addr:Don't care for LPDDR2/LPDDR3 */
	mrctrl0 &= ~((((dram_type == DRAM_LPDDR2)
		       || (dram_type == DRAM_LPDDR3)) ? 0x07 : 0x00) << 12);
	UMCTL_REG_SET(UMCTL_MRCTRL0, mrctrl0);

	mrctrl1 = (((dram_type == DRAM_LPDDR2)
		    || (dram_type ==
			DRAM_LPDDR3)) ? mr_addr : 0x00) << 8) | (mr_data << 0);
	UMCTL_REG_SET(UMCTL_MRCTRL1, mrctrl1);

	/* tirger the MR transaction in BIT_31 MRCTRL0.[mr_wr] */
	reg_bits_set(UMCTL_MRCTRL0, 31, 1, 0x01);
	/* checking that there is no outstanding MR tansacton.MCTL_MRSTAT.[mr_wr_busy] */
	while (UMCTL_REG_GET(UMCTL_MRSTAT) & BIT_0) ;
#endif
}

void umctl_soft_reset(BOOLEAN is_en) {
	uint32 reg_value = 0, i = 0;
	/*soft reset for uMCTL2 in user interface. */
	if (is_en) {
		// Assert soft reset
		reg_value = UMCTL_REG_GET(0x402b00c8);
		reg_value &= ~(0x03 << 11);
		reg_value |= ((0x1 << 8) | (0x1 << 10));
		UMCTL_REG_SET(0x402b00c8, reg_value);
		reg_value = UMCTL_REG_GET(0x402b0128);
		reg_value |= (0x1 << 16);
		UMCTL_REG_SET(0x402b0128, reg_value);
		for (i = 0; i <= 1000; i++) ;
	} else {
		// dessert soft reset
		reg_value = UMCTL_REG_GET(0x402b00c8);
		reg_value &= ~((0x1 << 8) | (0x1 << 10));
		UMCTL_REG_SET(0x402b00c8, reg_value);
		reg_value = UMCTL_REG_GET(0x402b0128);
		reg_value &= ~(0x1 << 16);
		UMCTL_REG_SET(0x402b0128, reg_value);

		for (i = 0; i <= 1000; i++) ;
	}
}

void umctl_core_init(DRAM_DESC * dram) {
	uint8 dram_type = dram->dram_type;
	uint8 BL = dram->bl;
	uint8 CL = dram->rl;
	uint8 RL = dram->rl;
	uint8 WL = dram->wl;
	uint8 ranks = dram->cs_num;
	uint8 width = dram->io_width;

	/* master register config */
	reg_bits_set(UMCTL_MSTR, 24, 4, ((ranks == 1) ? 0x01 : 0) |
		     ((ranks == 2) ? 0x03 : 0) |
		     ((ranks == 3) ? 0x07 : 0) | ((ranks == 4) ? 0x0F : 0));
	reg_bits_set(UMCTL_MSTR, 16, 4, ((BL == 2) ? 0x01 : 0) |
		     ((BL == 4) ? 0x02 : 0) |
		     ((BL == 8) ? 0x04 : 0) | ((BL == 16) ? 0x08 : 0));
	/*data_bus_width:2'b00--Full DQ buswidth.2'b01--Half DQ buswidth.
	 *               2'b10--Quater DQ buswidth.2'b11--Reserved
	 */
	reg_bits_set(UMCTL_MSTR, 12, 2, ((width == 32) ? 0x00 : 0x00) |
		     ((width == 16) ? 0x01 : 0x00) |
		     ((width == 8) ? 0x02 : 0x00));
	/*disable en_2t_timing_mode,use 1T timing as default. */
	reg_bits_set(UMCTL_MSTR, 10, 1, 0x00);
	/*burst_mode, 0--Sequential burst mode;1--Interleaved burst mode. */
	reg_bits_set(UMCTL_MSTR, 8, 1,
		     ((BURST_TYPE ==
		       DRAM_BT_SEQ) ? 0x00 : 0x00) | ((BURST_TYPE ==
						       DRAM_BT_INTER) ? 0x01 :
						      0x00));
	/*SDRAM selection for ddr2/ddr3/lpddr/lpddr2/lpddr3 */
	reg_bits_set(UMCTL_MSTR, 0, 4,
		     ((dram_type == DRAM_DDR2) ? 0x00 : 0x00) | ((dram_type ==
								  DRAM_DDR3) ?
								 0x01 : 0x00) |
		     ((dram_type == DRAM_LPDDR1) ? 0x02 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x04 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x08 : 0x00));
}

/*
 *[2:0]operation mode status
 *0x00-init
 *0x01-normal
 *0x02-power-down
 *0x03-SelfRefresh
 *0x04-DeepPowerdown,only support for mDDDR/LPDDR2/LPDDR3
*/
uint32 umctl_wait_status(uint32 state) {
	uint32 poll_data = ((state == OPERATION_MODE_INIT) ? 0x00 : 0x00) |
	    ((state == OPERATION_MODE_NORMAL) ? 0x01 : 0x00) |
	    ((state == OPERATION_MODE_PD) ? 0x02 : 0x00) |
	    ((state == OPERATION_MODE_SR) ? 0x03 : 0x00) |
	    ((state == OPERATION_MODE_DPD) ? 0x04 : 0x00);

	while ((UMCTL_REG_GET(UMCTL_STAT) & 0x07) != poll_data) ;
}

/*
 *Refer to uMCTL2 databook Chapter5.4.45~5.4.53.
 *Set sdram timing parameters,refer to SDRAM spec for details.
*/
void umctl_sdram_timing(DRAM_DESC * dram) {
	uint8 dram_type = dram->dram_type;
	uint8 BL = dram->bl;
	uint8 CL = dram->rl;
	uint8 RL = dram->rl;
	uint8 WL = dram->wl;
	uint8 AL = dram->al;

	/*Get the timing we used. */
	LPDDR_ACTIMING *lpddr1_timing = (LPDDR_ACTIMING *) (dram->ac_timing);
	LPDDR2_ACTIMING *lpddr2_timing = (LPDDR2_ACTIMING *) (dram->ac_timing);
	DDR2_ACTIMING *ddr2_timing = (DDR2_ACTIMING *) (dram->ac_timing);
	DDR3_ACTIMING *ddr3_timing = (DDR3_ACTIMING *) (dram->ac_timing);

	uint8 tWR = ((dram_type == DRAM_LPDDR1) ? (lpddr1_timing->tWR) : 0x00) |
	    ((dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tWR) : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tWR) : 0x00);
	uint8 tCKESR =
	    ((dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tCKESR) : 0x00);
	uint8 tCKSRX =
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tCKSRX) : 0x00);
	uint8 tMOD = (dram_type == DRAM_DDR3) ? (ddr3_timing->tMOD) : 0x00;	/*DDR3 only */
	uint8 tMRD = ((dram_type == DRAM_LPDDR1) ? (lpddr1_timing->tMRD) : 0x00) | ((dram_type == DRAM_DDR2) ? (ddr2_timing->tMRD) : 0x00) |	/*DDR3/2 only */
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tMRD) : 0x00);
	uint8 tRTP =
	    ((dram_type ==
	      DRAM_LPDDR2) ? (lpddr2_timing->tRTP) : 0x00) | ((dram_type ==
							       DRAM_DDR3)
							      ?
							      (ddr3_timing->tRTP)
							      : 0x00);
	uint8 tWTR =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tWTR) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tWTR)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tWTR) : 0x00);
	uint8 tRP =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRP) : 0x00) | ((dram_type ==
							      DRAM_LPDDR2)
							     ?
							     (lpddr2_timing->tRP)
							     : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRP) : 0x00);
	uint8 tRCD =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRCD) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRCD)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRCD) : 0x00);
	uint8 tRAS =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRAS) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRAS)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRAS) : 0x00);
	uint8 tRRD =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRRD) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRRD)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRRD) : 0x00);
	uint8 tRC =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRC) : 0x00) | ((dram_type ==
							      DRAM_LPDDR2)
							     ?
							     (lpddr2_timing->tRC)
							     : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRC) : 0x00);
	uint8 tCCD =
	    ((dram_type ==
	      DRAM_LPDDR2) ? (lpddr2_timing->tCCD) : 0x00) | ((dram_type ==
							       DRAM_DDR3)
							      ?
							      (ddr3_timing->tCCD)
							      : 0x00);
	uint8 tFAW =
	    ((dram_type ==
	      DRAM_LPDDR2) ? (lpddr2_timing->tFAW) : 0x00) | ((dram_type ==
							       DRAM_DDR3)
							      ?
							      (ddr3_timing->tFAW)
							      : 0x00);
	uint8 tRFC =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRFC) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRFC)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRFC) : 0x00);
	uint8 tDQSCK = (dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tDQSCK) : 0x00;	/*LPDDR2 only */
	uint8 tXS = ((dram_type == DRAM_DDR2) ? (ddr2_timing->tXS) : 200) |	/*for DDR2/DDR3,default200 */
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tXS) : 200);
	uint8 tXP = ((dram_type == DRAM_LPDDR1) ? (lpddr1_timing->tXP) : 0x00) |
	    ((dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tXP) : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tXP) : 0x00);
	uint8 tCKE =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tCKE) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tCKE)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tCKE) : 0x00);
	/*tDLLK:Dll locking time.Valid value are 2 to 1023 */
	uint8 tDLLK = (dram_type == DRAM_DDR3) ? (ddr3_timing->tDLLK) : 0x00;
	uint8 tDQSCKmax =
	    (dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tDQSCKmax) : 0x00;
	/*tAOND:ODT turnon/turnoff delays,DDR2 only */
	uint8 tAOND = ((dram_type == DRAM_DDR2) ? (ddr2_timing->tAOND) : 0x00);
	/*tRTODT:Read to ODT delay,DDR3 only
	 *0--ODT maybe turned on immediately after read post-amble
	 *1--ODT maybe not turned on until one clock after read post-amble
	 */
	uint8 tRTODT = 0x00;
	/*Get the timing we used. */

	reg_bits_set(UMCTL_DRAMTMG0, 24, 6, (WL + (BL >> 1) + tWR));	/*wr2pre */
	reg_bits_set(UMCTL_DRAMTMG0, 16, 6, tFAW);	/*t_FAW */
	reg_bits_set(UMCTL_DRAMTMG0, 13, 6, tRAS + ns_to_xclock(70000));	/*t_ras_max,Maxinum time of tRAS,clocks */
	reg_bits_set(UMCTL_DRAMTMG0, 0, 6, tRAS);	/*t_ras_min,Mininum time of tRAS,clocks */

	reg_bits_set(UMCTL_DRAMTMG1, 16, 6, tXP);
	/*Minimun from read to precharge of same bank */
	reg_bits_set(UMCTL_DRAMTMG1, 8, 5,
		     ((dram_type ==
		       DRAM_DDR2) ? (AL + (BL >> 1) + MAX(tRTP,
							  2) -
				     2) : 0x00) | ((dram_type ==
						    DRAM_DDR3) ? (AL + MAX(tRTP,
									   4)) :
						   0x00) | ((dram_type ==
							     DRAM_LPDDR1) ? (BL
									     >>
									     1)
							    : 0x00) |
		     ((dram_type ==
		       DRAM_LPDDR2) ? ((BL >> 1) + MAX(tRTP, 2) - 2) : 0x00) |
		     /*((dram_type==DRAM_LPDDR2_S2)?((BL>>1)+tRTP-1):0x00) | changde
		        ((dram_type==DRAM_LPDDR2_S4)?((BL>>1)+MAX(tRTP,2)-2):0x00) |
		      */
		     ((dram_type ==
		       DRAM_LPDDR3) ? ((BL >> 1) + MAX(tRTP, 4) - 4) : 0x00));
	reg_bits_set(UMCTL_DRAMTMG1, 0, 6, tRC);	/*Active-to-Active command period */

	reg_bits_set(UMCTL_DRAMTMG2, 24, 6, WL);
	reg_bits_set(UMCTL_DRAMTMG2, 16, 5, RL);
	/*Minimam time from read command to write command */
	reg_bits_set(UMCTL_DRAMTMG2, 8, 5,
		     (((dram_type == DRAM_DDR2) || (dram_type == DRAM_DDR3)
		       || (dram_type ==
			   DRAM_LPDDR1)) ? (RL + (BL >> 1) + 2 -
					    WL) : 0x00) | (((dram_type ==
							     DRAM_LPDDR2)
							    || (dram_type ==
								DRAM_LPDDR3))
							   ? (RL + (BL >> 2) +
							      tDQSCKmax + 1 -
							      WL) : 0x00));
	reg_bits_set(UMCTL_DRAMTMG2, 0, 6, (WL + (BL >> 1) + tWTR));

	/*tMRW, time to wait during load mode register writes. */
	reg_bits_set(UMCTL_DRAMTMG3, 26, 6,
		     ((dram_type == DRAM_LPDDR2) ? 0x05 : 0x00) | ((dram_type ==
								    DRAM_LPDDR3)
								   ? 0x0A :
								   0x00));
	reg_bits_set(UMCTL_DRAMTMG3, 12, 3, tMRD);
	reg_bits_set(UMCTL_DRAMTMG3, 0, 10, tMOD);

	reg_bits_set(UMCTL_DRAMTMG4, 24, 5, tRCD);
	reg_bits_set(UMCTL_DRAMTMG4, 16, 3, tCCD);
	reg_bits_set(UMCTL_DRAMTMG4, 8, 4, tRRD);
	reg_bits_set(UMCTL_DRAMTMG4, 0, 5, tRP);

	/*tCKSRX,the time before SelfRefreshExit that CK is maintained as a valid clock. */
	/*Specifies the clock stable time before SRX. */
	reg_bits_set(UMCTL_DRAMTMG5, 24, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x01 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x02 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x02 : 0x00) | ((dram_type ==
								    DRAM_DDR2) ?
								   0x01 : 0x00)
		     | ((dram_type == DRAM_DDR3) ? tCKSRX : 0x00));
	/*tCKSRE,the time after SelfRefreshDownEntry that CK is maintained as a valid clock. */
	/*Specifies the clock disable delay after SRE. */
	reg_bits_set(UMCTL_DRAMTMG5, 16, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x00 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x02 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x02 : 0x00) | ((dram_type ==
								    DRAM_DDR2) ?
								   0x01 : 0x00)
		     | ((dram_type == DRAM_DDR3) ? 0x05 : 0x00));
	/*tCKESR,Minimum CKE low width for selfrefresh entry to exit timing in clock cycles. */
	reg_bits_set(UMCTL_DRAMTMG5, 8, 6,
		     ((dram_type == DRAM_LPDDR1) ? tRFC : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? tCKESR :
								   0x00) |
		     ((dram_type ==
		       DRAM_LPDDR3) ? tCKESR : 0x00) | ((dram_type ==
							 DRAM_DDR2) ? tCKE :
							0x00) | ((dram_type ==
								  DRAM_DDR3)
								 ? (tCKE +
								    1) : 0x00));
	/*tCKE,Minimum number of cycles of CKE HIGH/LOW during power-down and selfRefresh. */
	reg_bits_set(UMCTL_DRAMTMG5, 8, 6, tCKE);

	/*tCKDPDE,time after DeepPowerDownEntry that CK is maintained as a valid clock. */
	reg_bits_set(UMCTL_DRAMTMG6, 24, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x00 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x02 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x02 : 0x00));
	/*tCKDPDX,time before DeepPowerDownExit that CK is maintained as a valid clock before issuing DPDX. */
	reg_bits_set(UMCTL_DRAMTMG6, 16, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x01 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x02 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x02 : 0x00));
	/*tCKCSX,time before ClockStopExit that CK is maintained as a valid clock before issuing DPDX. */
	reg_bits_set(UMCTL_DRAMTMG6, 0, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x01 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? (tXP +
								      0x02) :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? (tXP + 0x02) : 0x00));

	/*tCKPDE,time after PowerDownEntry that CK is maintained as a valid clock before issuing PDE. */
	reg_bits_set(UMCTL_DRAMTMG7, 8, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x00 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x02 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x02 : 0x00));
	/*tCKPDX,time before PowerDownExit that CK is maintained as a valid clock before issuing PDX. */
	reg_bits_set(UMCTL_DRAMTMG7, 0, 4,
		     ((dram_type == DRAM_LPDDR1) ? 0x00 : 0x00) | ((dram_type ==
								    DRAM_LPDDR2)
								   ? 0x02 :
								   0x00) |
		     ((dram_type == DRAM_LPDDR3) ? 0x02 : 0x00));

	/*post_selfref_gap_x32,time after coming out of selfref before doing anything.Default:0x44
	   //reg_bits_set(MCTL_DRAMTMG8,  0, 4, max(tXSNR,max(tXSRD,tXSDLL))); */
	reg_bits_set(UMCTL_DRAMTMG8, 0, 32, 0x00000014);
}

/*
 *Refer to uMCTL2 databook for detail.
 *Set sdram timing parameters according to EDA simulation.
 *including DFI/ZQ/ADDR_MAPPING/PCFGR_x
*/
void umctl_other_init(DRAM_DESC * dram) {
	uint8 dram_type = dram->dram_type;

	reg_bits_set(UMCTL_RFSHTMG, 0, 32, 0x620038);	//yanbin

/*FPGA_TEST xiaohui in Beijing*/
	reg_bits_set(UMCTL_ZQCTL0, 0, 32, 0x00900024);
	reg_bits_set(UMCTL_ZQCTL1, 0, 32, 0x01400070);
	reg_bits_set(UMCTL_ZQCTL2, 0, 32, 0x00000000);

	reg_bits_set(UMCTL_DFITMG0, 0, 32,
		     ((dram_type ==
		       DRAM_LPDDR1) ? 0x02010100 : 0x00) | ((dram_type ==
							     DRAM_LPDDR2) ?
							    0x02050103 : 0x00));
	reg_bits_set(UMCTL_DFITMG1, 0, 32, 0x00300202);
	reg_bits_set(UMCTL_DFILPCFG0, 0, 32, 0x07812120);	//yanbin   0x07813120

	reg_bits_set(UMCTL_DFIUPD0, 0, 32, 0x80400003);	//yanbin   0x00400003
	reg_bits_set(UMCTL_DFIUPD1, 0, 32, 0x001A0021);
	reg_bits_set(UMCTL_DFIUPD2, 0, 32, 0x026904C9);
	reg_bits_set(UMCTL_DFIUPD3, 0, 32, 0x030E051F);

	/*Phy initialization complete enable 
	 *We asserted the MCTL_DFIMISC.[dfi_init_complete] to trigger SDRAM initialization.
	 */
	reg_bits_set(UMCTL_DFIMISC, 0, 32, 0x00000001);

	/*address mapping */
	if (DRAM_LPDDR2 == dram_type) {
		reg_bits_set(UMCTL_ADDRMAP0, 0, 32, 0x0000001f);	//cs_bit_0 = 0
		reg_bits_set(UMCTL_ADDRMAP1, 0, 32, 0x00070707);	//bank_bit_2 = 11, bank_bit_1 = 10, bank_bit_0 = 9
		reg_bits_set(UMCTL_ADDRMAP2, 0, 32, 0x00000000);	//col_bit_5 = 5, col_bit_4 = 4, col_bit_3 = 3, col_bit_2 = 2
		reg_bits_set(UMCTL_ADDRMAP3, 0, 32, 0x0f000000);	//col_bit_9 = 0, col_bit_8 = 8, col_bit_7 = 7, col_bit_6 = 6
		reg_bits_set(UMCTL_ADDRMAP4, 0, 32, 0x00000f0f);	//col_bit_11 = 0, col_bit_10 = 0
		reg_bits_set(UMCTL_ADDRMAP5, 0, 32, 0x06060606);	//row_bit_11 = 23, row_bit_10:2 = 22:14, row_bit_1 = 13, row_bit_0 = 12  
		reg_bits_set(UMCTL_ADDRMAP6, 0, 32, 0x0f0f0f06);	//row_bit_15 = 0, row_bit_14 = 0, row_bit_13 = 0, row_bit_12 = 24
	} else {		/*DRAM_LPDDR1 */
		reg_bits_set(UMCTL_ADDRMAP0, 0, 32, 0x00000013);
		reg_bits_set(UMCTL_ADDRMAP1, 0, 32, 0x000f0707);
		reg_bits_set(UMCTL_ADDRMAP2, 0, 32, 0x00000000);
		reg_bits_set(UMCTL_ADDRMAP3, 0, 32, 0x0f000000);
		reg_bits_set(UMCTL_ADDRMAP4, 0, 32, 0x00000F0F);
		reg_bits_set(UMCTL_ADDRMAP5, 0, 32, 0x05050505);
		reg_bits_set(UMCTL_ADDRMAP6, 0, 32, 0x0F0F0505);
	}

	reg_bits_set(UMCTL_ODTCFG, 0, 32, 0x02010205);
	reg_bits_set(UMCTL_ODTMAP, 0, 32, 0x00000312);

	reg_bits_set(UMCTL_SCHED, 0, 32, 0x00070C01);
	reg_bits_set(UMCTL_PERFHPR0, 0, 32, 0x00000000);
	reg_bits_set(UMCTL_PERFHPR1, 0, 32, 0x10000000);
	reg_bits_set(UMCTL_PERFLPR0, 0, 32, 0x00000100);
	reg_bits_set(UMCTL_PERFLPR1, 0, 32, 0x40000100);
	reg_bits_set(UMCTL_PERFWR0, 0, 32, 0x00000000);
	reg_bits_set(UMCTL_PERFWR1, 0, 32, 0x10000000);

	/*port common configuration register
	 *[4]:pagematch_limit
	 *[0]:go2critical_en
	 */
	reg_bits_set(UMCTL_PCCFG, 0, 32, 0x00000011);
	/*Port n configuration Read/Write register setting */
	reg_bits_set(UMCTL_PCFGR_0, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_0, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_0, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_1, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_1, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_1, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_2, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_2, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_2, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_3, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_3, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_3, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_4, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_4, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_4, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_5, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_5, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_5, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_6, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_6, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_6, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_7, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_7, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_7, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_8, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_8, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_8, 0, 32, 0x00000001);
	reg_bits_set(UMCTL_PCFGR_9, 0, 32, 0x0001F000);
	reg_bits_set(UMCTL_PCFGW_9, 0, 32, 0x00004005);
	reg_bits_set(UMCTL_PORT_EN_9, 0, 32, 0x00000001);
}

/*
 *Power-up timing initialization and Mode Register Setting for sdram.
*/
void umctl_poweron_init(DRAM_DESC * dram) {
	uint8 dram_type = dram->dram_type;
	uint8 BL = dram->bl;
	uint8 CL = dram->rl;
	uint8 RL = dram->rl;
	uint8 WL = dram->wl;
	uint8 AL = dram->al;
	uint8 mr = 0, emr = 0, mr2 = 0, mr3 = 0;

	/*Get the timing we used. */
	LPDDR_ACTIMING *lpddr1_timing = (LPDDR_ACTIMING *) (dram->ac_timing);
	LPDDR2_ACTIMING *lpddr2_timing = (LPDDR2_ACTIMING *) (dram->ac_timing);
	DDR2_ACTIMING *ddr2_timing = (DDR2_ACTIMING *) (dram->ac_timing);
	DDR3_ACTIMING *ddr3_timing = (DDR3_ACTIMING *) (dram->ac_timing);

	/* tINIT0:(-~20)ms  Maximum voltage ramp time */
	uint32 tINIT0 = 15;
	/* tINIT1:(100~-)ns Minimum CKE LOW time after completion of voltage ramp */
	uint32 tINIT1 = 200;
	/* tINIT2:(5~-)tCK  Minimum stable clock before first CKE HIGH */
	uint32 tINIT2 = 10;
	/* tINIT3:(200~-)us Minimum idle time after first CKE assertion */
	uint32 tINIT3 = 300;
	/* tINIT4:(001~-)us Minimum idle time after RESET command */
	uint32 tINIT4 = 10;
	/* tINIT5:(-~10)us  Maximum duration of device auto initialization */
	uint32 tINIT5 = 8;
	/* tZQINIT:(1~-)us  ZQ initial calibration (S4 devices only) */
	uint32 tZQINIT = 5;
	/* tCKb:(18~100)ns  Clock cycle time during boot */
	uint32 tCKb = 50;

	/*post_cle_x1024,cycles to wait after driving CKE high to start the SDRAM init sequence. */
	reg_bits_set(UMCTL_INIT0, 16, 10,
		     ((dram_type ==
		       DRAM_DDR2) ? ns_to_x1024(400) : 0x00) | ((dram_type ==
								 DRAM_LPDDR2) ?
								us_to_x1024(200)
								: 0x00) |
		     ((dram_type == DRAM_LPDDR3) ? us_to_x1024(200) : 0x00));
	/*pre_cle_x1024,cycles to wait after reset before driving CKE high to start the SDRAM init sequence. */
	reg_bits_set(UMCTL_INIT0, 0, 10, ((dram_type == DRAM_DDR2) ? us_to_x1024(200) : 0x00) |	/*>=200us */
		     /*tINIT0 of 20ms(max) + tINIT1 of 100ns(min) */
		     ((dram_type ==
		       DRAM_LPDDR2) ? (ms_to_x1024(tINIT0) +
				       ns_to_x1024(tINIT1)) : 0x00) |
		     ((dram_type ==
		       DRAM_LPDDR3) ? (ms_to_x1024(tINIT0) +
				       ns_to_x1024(tINIT1)) : 0x00));

	/*dram_rstn_x1024,cycles to assert SDRAM reset signal during init sequence. */
	reg_bits_set(UMCTL_INIT1, 16, 8, ((dram_type == DRAM_DDR3) ? 0x02 : 0x00));	/*>=1tCK */
	/*final_wait_x32,cycles to wait after completing the SDRAM init sequence. */
	reg_bits_set(UMCTL_INIT1, 8, 7, 0x01);	/*>=0tCK */
	/*pre_ocd_x32,cycles to wait before driving the OCD complete command to SDRAM. */
	reg_bits_set(UMCTL_INIT1, 0, 5, 0x01);	/*>=0tCK */

	/*idle_after_reset_x32,idle time after the reset command,tINT4. */
	reg_bits_set(UMCTL_INIT2, 8, 8,
		     ((dram_type == DRAM_LPDDR2) ? us_to_x32(tINIT4) : 0x00));
	/*min_stable_clock_x1,time to wait after the first CKE high,tINT2. */
	reg_bits_set(UMCTL_INIT2, 0, 4,
		     ((dram_type ==
		       DRAM_LPDDR2) ? tINIT2 : 0x00) | ((dram_type ==
							 DRAM_LPDDR3) ? tINIT2 :
							0x00));

	/*Note:
	 *    Set Mode register 0~3 for SDRAM memory.
	 *    Refer to JESD spec for detail.
	 */
	switch (dram_type) {
	case DRAM_LPDDR1:
		{
			/*mr store the value to write to MR register */
			mr = (BL << 0) |	/*burst_length:BL2/4/8/16 */
			    (0x00 << 3) |	/*0:Sequentia;1:interleavel */
			    (CL << 4);	/*cas_latency:2/3/4 optional */
			/*emr store the value to write to EMR register */
			emr = (0x00 << 0) |	/*PASR:0:AllBanks;1:HalfArray;2:QuarterArray,etc */
			    (0x00 << 3) |	/*TCSR:0:70'C;1:45'C;2:15'C;3:85'C; */
			    ((((IO_DS == DS_FULL) ? 0x00 : 0x00) |
			      ((IO_DS == DS_HALF) ? 0x01 : 0x00) |
			      ((IO_DS == DS_QUARTER) ? 0x02 : 0x00) |
			      ((IO_DS == DS_OCTANT) ? 0x03 : 0x00) |
			      ((IO_DS == DS_THREE_QUATERS) ? 0x04 : 0x00)
			     ) << 5);	/*DS:0:FullDriverStrength;1:HalfDS;2:QuarterDS;3:OctantDS;4:Three-Quater */

			/*mr2 unused for MDDR */
			/*mr3 unused for MDDR/LPDDR2/LPDDR3 */
			break;
		}
	case DRAM_LPDDR2:
		{
			uint8 tWR = lpddr2_timing->tWR;
			/*mr store the value to write to MR1 register */
			/*Set sequential burst-type with wrap */
			mr = ((BL == 4) ? 0x02 : 0x00) |	/*burst_length:2:BL4(default;3:BL8;4:BL16) */
			    ((BL == 8) ? 0x03 : 0x00) | ((BL == 16) ? 0x04 : 0x00) | ((	/*0:Sequential(default);1:interleavel(allow for SDRAM only) */
											      ((BURST_TYPE == DRAM_BT_SEQ) ? 0x00 : 0x00) | ((BURST_TYPE == DRAM_BT_INTER) ? 0x01 : 0x00)
										      ) << 3) | (0x00 << 4) |	/*0:Wrap(default);1:No wrap(allow for SDRAM BL4 only) */
			    ((	/*WriteRecovery,Default(0x01) */
				     ((tWR == 3) ? 0x01 : 0x00) |
				     ((tWR == 4) ? 0x02 : 0x00) |
				     ((tWR == 5) ? 0x03 : 0x00) |
				     ((tWR == 6) ? 0x04 : 0x00) |
				     ((tWR == 7) ? 0x05 : 0x00) |
				     ((tWR == 8) ? 0x06 : 0x00)
			     ) << 5);
			/*emr store the value to write to MR2 register,Default:0x01 */
			emr = (((RL == 3) && (WL == 1)) ? 0x01 : 0x00) |
			    (((RL == 4) && (WL == 2)) ? 0x02 : 0x00) |
			    (((RL == 5) && (WL == 2)) ? 0x03 : 0x00) |
			    (((RL == 6) && (WL == 3)) ? 0x04 : 0x00) |
			    (((RL == 7) && (WL == 4)) ? 0x05 : 0x00) |
			    (((RL == 8) && (WL == 4)) ? 0x06 : 0x00);
			/*mr2 store the value to write to MR3 register */
			/*DS,driver strength configuration.Default:40ohm */
			mr2 = ((IO_DS == DS_34R3) ? 0x01 : 0x00) |
			    ((IO_DS == DS_40R) ? 0x02 : 0x00) |
			    ((IO_DS == DS_48R) ? 0x03 : 0x00) |
			    ((IO_DS == DS_60R) ? 0x04 : 0x00) |
			    ((IO_DS == DS_68R6) ? 0x05 : 0x00) |
			    ((IO_DS == DS_80R) ? 0x06 : 0x00) |
			    ((IO_DS == DS_120R) ? 0x07 : 0x00);
			/*mr3 unused for MDDR/LPDDR2/LPDDR3 */
			break;
		}
	case DRAM_DDR2:
		{
			/*mr2 store the value to write to EMR2 register */
			/*mr3 store the value to write to EMR3 register */
			break;
		}
	case DRAM_DDR3:
		{
			DDR3_ACTIMING *ddr3_timing =
			    (DDR3_ACTIMING *) (dram->ac_timing);
			uint8 tWR = ddr3_timing->tWR;
			uint8 CWL = (WL - AL) ? (WL - AL) : 0x00;
			/*mr store the value to write to MR1 register */
			/*Set sequential burst-type with wrap */
			mr = ((BL == 8) ? 0x00 : 0x00) |	/*Fixed on 8 */
			    ((BL == 4) ? 0x01 : 0x00) |	/*4or8 on the fly */
			    ((BL == 4) ? 0x02 : 0x00) |	/*Fixed on 4 */
			    ((	/*0:Sequential(default);1:interleavel(allow for SDRAM only) */
				     ((BURST_TYPE ==
				       DRAM_BT_SEQ) ? 0x00 : 0x00) |
				     ((BURST_TYPE ==
				       DRAM_BT_INTER) ? 0x01 : 0x00)
			     ) << 3) | ((((CL == 5) ? 0x01 : 0x00) |	/*Bit[6:4],CAS Latency */
					 ((CL == 6) ? 0x02 : 0x00) | ((CL == 7) ? 0x03 : 0x00) | ((CL == 8) ? 0x04 : 0x00) | ((CL == 9) ? 0x05 : 0x00) | ((CL == 10) ? 0x06 : 0x00) | ((CL == 11) ? 0x07 : 0x00)) << 4) | ((((tWR <= 5) ? 0x01 : 0x00) |	/*Bit[11:9],Write recovery for autoprecharge */
																											    ((tWR == 6) ? 0x02 : 0x00) | ((tWR == 7) ? 0x03 : 0x00) | ((tWR == 8) ? 0x04 : 0x00) | ((tWR == 10) ? 0x05 : 0x00) | ((tWR == 12) ? 0x06 : 0x00) | ((tWR == 14) ? 0x07 : 0x00) | ((tWR == 16) ? 0x00 : 0x00)) << 9);

			/*emr store the value to write to MR1 register */
			/*A0:0-DLL Enable;1-DLL Disable */
			reg_bits_set((uint32) & emr, 0, 1, 0);
			/*Output Driver Impedance Control
			   [A5,A1]:00-RZQ/6;01-RZQ/7;10/11-RZQ/TBD;Note: RZQ=240ohm */
			reg_bits_set((uint32) & emr, 1, 1, 0);
			reg_bits_set((uint32) & emr, 5, 1, 0);
			/*[A4:A3]:Additive Latency. */
			reg_bits_set((uint32) & emr, 3, 2,
				     ((AL == 0) ? 0x00 : 0x00) | ((AL ==
								   CL -
								   1) ? 0x01 :
								  0x00) | ((AL
									    ==
									    CL -
									    2) ?
									   0x02
									   :
									   0x00)
			    );
			/*[A7]:1-Write leveling enable;0-Disabled */
			reg_bits_set((uint32) & emr, 7, 1, 1);

			/*mr2 store the value to write to MR2 register */
			/*Partial Array Self-Refresh (Optional),[A2:A0]
			 */
			/*CAS write Latency (CWL),WL=CWL+AL
			   [A5:A3]:5~12 tCK
			 */
			reg_bits_set((uint32) & mr2, 3, 3,
				     ((CWL == 5) ? 0x00 : 0x00) | ((CWL ==
								    6) ? 0x01 :
								   0x00) | ((CWL
									     ==
									     7)
									    ?
									    0x02
									    :
									    0x00)
				     | ((CWL == 8) ? 0x03 : 0x00) | ((CWL == 9)
								     ? 0x04 :
								     0x00) |
				     ((CWL == 10) ? 0x05 : 0x00) | ((CWL ==
								     11) ? 0x06
								    : 0x00) |
				     ((CWL == 12) ? 0x07 : 0x00)
			    );

			/*mr3 store the value to write to MR3 register */
			/*[A1:A0],MPR location */
			/*[A2],MPR */
			break;
		}
	case DRAM_LPDDR3:
		{
			/*mr store the value to write to MR1 register */
			/*mr2 store the value to write to MR3 register */
			/*mr3 unused for MDDR/LPDDR2/LPDDR3 */
			break;
		}
	}
	reg_bits_set(UMCTL_INIT3, 16, 16, mr);
	reg_bits_set(UMCTL_INIT3, 0, 16, emr);
	reg_bits_set(UMCTL_INIT4, 16, 16, mr2);
	reg_bits_set(UMCTL_INIT4, 0, 16, mr3);

	/*dev_zqinit_x32,ZQ initial calibration,tZQINIT. */
	reg_bits_set(UMCTL_INIT5, 16, 8,
		     ((dram_type ==
		       DRAM_DDR3) ? us_to_x32(tZQINIT) : 0x00) | ((dram_type ==
								   DRAM_LPDDR2)
								  ?
								  us_to_x32
								  (tZQINIT) :
								  0x00) |
		     ((dram_type == DRAM_LPDDR3) ? us_to_x32(tZQINIT) : 0x00));
	/*max_auto_init_x1024,max duration of the auto initialization,tINIT5. */
	reg_bits_set(UMCTL_INIT5, 0, 10,
		     ((dram_type ==
		       DRAM_LPDDR2) ? us_to_x1024(tINIT5) : 0x00) | ((dram_type
								      ==
								      DRAM_LPDDR3)
								     ?
								     us_to_x1024
								     (tINIT5) :
								     0x00));

	/*Only present for multi-rank configurations.
	 *[11:8]:rank_wr_gap,clks of gap in data responses when performing consecutive writes to different ranks.
	 *[07:4]:rank_rd_gap,clks of gap in data responses when performing consecutive reads to different ranks.
	 *[03:0]:max_rank_rd,This param represents the max number of 64B reads(or 32B in some short read cases)
	 *       that can bescheduled consecutively to the same rank.
	 */
	reg_bits_set(UMCTL_RANKCTL, 0, 32, (0x06 << 8) |
		     (0x06 << 4) | (0x03 << 0));
}

/*
 *Refer to uMCTL2 databook Chapter2.15.1 "DDR2 initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/
void umctl_ddr2_init(DRAM_DESC * dram, void *init_timing) {

}
/*
 *Refer to uMCTL2 databook Chapter2.15.2 "DDR3 initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/ void umctl_ddr3_init(DRAM_DESC * dram, void *init_timing) {

}
/*
 *Refer to uMCTL2 databook Chapter2.15.3 "Mobile ddr initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/ void umctl_mddr_init(DRAM_DESC * dram, void *init_timing) {
#if 0
//changde
	uint8 i = 0;
	uint8 dram_type = dram->dram_type;
	uint8 mr_ranks = dram->cs_num;
	/*T=200us,Power-up: VDD and CK stable */
	uint32 T = 200;

	uint32 INIT0 = 0;
	 INIT0 = ((dram_type == DRAM_LPDDR1) ? us_to_x1024(T) : 0x00);
	 UMCTL_REG_SET(UMCTL_INIT0, INIT0);
	 mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);

	/*Issue precharege all command. */
	 mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);

	/*Issue refresh command 8 times. */
	for (i = 0; i < 8; i++) {
		mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_REFRESH, 0x00);
	}
	/*Load mode register(MR). */
	    mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);

	/*Load extEnded mode register(EMR). */
	mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);

	/*Issue active command. */
	mr_rw(dram_type, mr_ranks, MR_WRITE, MR_CMD_PRECHARGE, 0x00);
	mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);

	/*Now,begin normal operation */
#endif
}

/*
 *Refer to uMCTL2 databook Chapter2.15.4 "LPDDR2/LPDDR3 initialization sequence" 
 *Also you can use PUBL build-in-test init sequence.
*/
void umctl_ldppr2or3_init(DRAM_DESC * dram, void *init_timing) {
#if 0
//changde
	/*A reset command is issued to MRW63 register. */
	mr_rw(dram_type, mr_ranks, MR_WRITE, 0x3F, 0x00);
	mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);
	/*A ZQ initialization calibration command is issued to MRW10 register. */
	mr_rw(dram_type, mr_ranks, MR_WRITE, 0x0A, 0xFF);
	mr_rw(dram_type, mr_ranks, MR_WRITE, MR_ADDR_NOP, 0x00);
	/*Setting MR2 register to INIT3.emr followed by a NOP */

	/*Setting MR1 register to INIT3.emr followed by a NOP */

	/*Setting MR3 register to INIT3.emr followed by a NOP */

	/*Schedule multiple all bank refresh */

	/*Now,begin normal operation */
#endif
}
/*
 *SDram power-on timing and common timing using PHY publ.
 *Configure PUBL_DCR,PUBL_PTR0/1/2 and PUBL_CFG_DTPR0/1/2
 *Attention:timing parameters in clock cycles.
*/ void phy_sdram_timing(DRAM_DESC * dram) {
	uint32 dcr = 0;
	uint32 dtpr0 = 0, dtpr1 = 0, dtpr2 = 0;
	uint8 dram_type = dram->dram_type;
	uint8 banks = dram->bank_num;
	uint8 BL = dram->bl;
	uint8 CL = dram->rl;
	uint8 RL = dram->rl;
	uint8 WL = dram->wl;

	/*Get the timing we used. */
	LPDDR_ACTIMING *lpddr1_timing = (LPDDR_ACTIMING *) (dram->ac_timing);
	LPDDR2_ACTIMING *lpddr2_timing = (LPDDR2_ACTIMING *) (dram->ac_timing);
	DDR2_ACTIMING *ddr2_timing = (DDR2_ACTIMING *) (dram->ac_timing);
	DDR3_ACTIMING *ddr3_timing = (DDR3_ACTIMING *) (dram->ac_timing);

	uint8 tWR = ((dram_type == DRAM_LPDDR1) ? (lpddr1_timing->tWR) : 0x00) |
	    ((dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tWR) : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tWR) : 0x00);
	uint8 tCKESR =
	    ((dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tCKESR) : 0x00);
	uint8 tCKSRX =
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tCKSRX) : 0x00);
	uint8 tMOD = (dram_type == DRAM_DDR3) ? (ddr3_timing->tMOD) : 0x00;	/*DDR3 only */
	uint8 tMRD = ((dram_type == DRAM_LPDDR1) ? (lpddr1_timing->tMRD) : 0x00) | ((dram_type == DRAM_DDR2) ? (ddr2_timing->tMRD) : 0x00) |	/*DDR3/2 only */
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tMRD) : 0x00);
	uint8 tRTP =
	    ((dram_type ==
	      DRAM_LPDDR2) ? (lpddr2_timing->tRTP) : 0x00) | ((dram_type ==
							       DRAM_DDR3)
							      ?
							      (ddr3_timing->tRTP)
							      : 0x00);
	uint8 tWTR =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tWTR) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tWTR)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tWTR) : 0x00);
	uint8 tRP =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRP) : 0x00) | ((dram_type ==
							      DRAM_LPDDR2)
							     ?
							     (lpddr2_timing->tRP)
							     : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRP) : 0x00);
	uint8 tRCD =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRCD) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRCD)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRCD) : 0x00);
	uint8 tRAS =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRAS) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRAS)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRAS) : 0x00);
	uint8 tRRD =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRRD) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRRD)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRRD) : 0x00);
	uint8 tRC =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRC) : 0x00) | ((dram_type ==
							      DRAM_LPDDR2)
							     ?
							     (lpddr2_timing->tRC)
							     : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRC) : 0x00);
	uint8 tCCD =
	    ((dram_type ==
	      DRAM_LPDDR2) ? (lpddr2_timing->tCCD) : 0x00) | ((dram_type ==
							       DRAM_DDR3)
							      ?
							      (ddr3_timing->tCCD)
							      : 0x00);
	uint8 tFAW =
	    ((dram_type ==
	      DRAM_LPDDR2) ? (lpddr2_timing->tFAW) : 0x00) | ((dram_type ==
							       DRAM_DDR3)
							      ?
							      (ddr3_timing->tFAW)
							      : 0x00);
	uint8 tRFC =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tRFC) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tRFC)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tRFC) : 0x00);
	uint8 tDQSCK = (dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tDQSCK) : 0x00;	/*LPDDR2 only */
	uint8 tXS = ((dram_type == DRAM_DDR2) ? (ddr2_timing->tXS) : 200) |	/*for DDR2/DDR3,default200 */
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tXS) : 200);
	uint8 tXP = ((dram_type == DRAM_LPDDR1) ? (lpddr1_timing->tXP) : 0x00) |
	    ((dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tXP) : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tXP) : 0x00);
	uint8 tCKE =
	    ((dram_type ==
	      DRAM_LPDDR1) ? (lpddr1_timing->tCKE) : 0x00) | ((dram_type ==
							       DRAM_LPDDR2)
							      ?
							      (lpddr2_timing->tCKE)
							      : 0x00) |
	    ((dram_type == DRAM_DDR3) ? (ddr3_timing->tCKE) : 0x00);
	/*tDLLK:Dll locking time.Valid value are 2 to 1023 */
	uint8 tDLLK = (dram_type == DRAM_DDR3) ? (ddr3_timing->tDLLK) : 0x00;
	uint8 tDQSCKmax =
	    (dram_type == DRAM_LPDDR2) ? (lpddr2_timing->tDQSCKmax) : 0x00;
	/*tAOND:ODT turnon/turnoff delays,DDR2 only */
	uint8 tAOND = ((dram_type == DRAM_DDR2) ? (ddr2_timing->tAOND) : 0x00);
	/*tRTODT:Read to ODT delay,DDR3 only
	 *0--ODT maybe turned on immediately after read post-amble
	 *1--ODT maybe not turned on until one clock after read post-amble
	 */
	uint8 tRTODT = 0x00;
	/*Get the timing we used. */

	{			/*dram configuration register (DCR) */
		/*[2:0]:SDram ddr mode. */
		reg_bits_set(PUBL_DCR, 0, 3,
			     ((dram_type ==
			       DRAM_LPDDR1) ? 0x00 : 0x00) | ((dram_type ==
							       DRAM_DDR) ? 0x01
							      : 0x00) |
			     ((dram_type ==
			       DRAM_DDR2) ? 0x02 : 0x00) | ((dram_type ==
							     DRAM_DDR3) ? 0x03 :
							    0x00) | ((dram_type
								      ==
								      DRAM_LPDDR2)
								     ? 0x04 :
								     0x00));
		/*DDR 8-bank,indicates if set that sdram used has 8 banks.
		 *tRPA=tRP+1 and tFAW are used for 8banks DRAMs,other tRPA=tRP and no tFAW
		 *is used.Note that a setting of 1for DRAMs that have fewer than 8banks still
		 *results in correct functionality but less tigther daram parameters.
		 */
		reg_bits_set(PUBL_DCR, 3, 1, (banks == 8) ? 0x01 : 0x00);
		/*Select the ddr type for the specified lpddr mode
		   reg_bits_set(PUBL_CFG_DCR, 8, 2, ((dram_type==DRAM_LPDDR2_S4)?0x00:0x00)|
		   ((dram_type==DRAM_LPDDR2_S2)?0x01:0x00)); */
	}

	//PTR0, to set tDLLSRST, tDLLLOCK, tITMSRST
	{
		//DLL Soft Reset Time: Number of controller clock cycles that the DLL soft reset pin
		//must remain asserted when the soft reset is triggered through the PHY Initialization
		//Register (PIR). This must correspond to a value that is equal to or more than 50ns
		//or 8 controller clock cycles, whichever is bigger.
		//Default value is corresponds to 50ns at 533Mhz
		uint32 tDLLSRST = ns_to_xclock(50);	//ns-->clocks       
		//DLL Lock Time: Number of clock cycles for the DLL to stabilize and lock, i.e. number
		//of clock cycles from when the DLL reset pin is de-asserted to when the DLL has
		//locked and is ready for use. Refer to the PHY databook for the DLL lock time.
		//Default value corresponds to 5.12us at 533MHz.        
		uint32 tDLLLOCK = ns_to_xclock(5120);	//ns-->clocks
		//ITM Soft Reset Time: Number of controller clock cycles that the ITM soft reset pin
		//must remain asserted when the soft reset is applied to the ITMs. This must
		//correspond to a value that is equal to or more than 8 controller clock cycles. 
		//Default value corresponds to 8 controller clock cycles
		uint32 tITMSRST = 8;	//clocks

		REG32(PUBL_PTR0) =
		    (tITMSRST << 18) | (tDLLLOCK << 6) | (tDLLSRST);
	}

	//PTR1, to set tINT0,tINT1
	{
		uint32 tINT0 = ((dram_type == DRAM_DDR3) ? us_to_xclock(500) : 0x00) |	/*Unit:us->clk */
		    ((dram_type == DRAM_DDR2) ? us_to_xclock(200) : 0x00) |
		    ((dram_type == DRAM_LPDDR1) ? us_to_xclock(200) : 0x00) |
		    ((dram_type == DRAM_LPDDR2) ? us_to_xclock(200) : 0x00);
		uint32 tINT1 = ((dram_type == DRAM_DDR3) ? 0x05 : 0x00) |	/*Unit:clk */
		    ((dram_type == DRAM_DDR2) ? ns_to_xclock(400) : 0x00) | ((dram_type == DRAM_DDR) ? 0x01 : 0x00) |	/*400ns or 1tCK */
		    ((dram_type == DRAM_LPDDR2) ? ns_to_xclock(100) : 0x00);

		REG32(PUBL_PTR1) = (tINT1 << 19) | tINT0;
	}

	//PTR2, to set tINT2,tINT3
	{
		/*tINT2,default 200us, time for reset command to end of auto initialization */
		uint32 tINT2 = ((dram_type == DRAM_DDR3) ? ns_to_xclock(200000) : 0x00) |	/*Unit:ns->clk */
		    ((dram_type == DRAM_LPDDR2) ? ns_to_xclock(11000) : 0x00);
		//tINT3,default 1us, time for ZQ initialization command to first command */
		uint32 tINT3 =
		    (dram_type == DRAM_LPDDR2) ? ns_to_xclock(1000) : 0x00;

		REG32(PUBL_PTR2) = (tINT3 << 17) | tINT2;
	}

	/*Dram timing parameters in clock cycles can be set in register DTPR0-2.
	 *Refer to the SDRAM datasheet for detailed description about these timing in more frequence.
	 */
	dtpr0 = ((tMRD & 0x03) << 0) |
	    ((tRTP & 0x07) << 2) |
	    ((tWTR & 0x07) << 5) |
	    ((tRP & 0x0F) << 8) |
	    ((tRCD & 0x0F) << 12) |
	    ((tRAS & 0x1F) << 16) |
	    ((tRRD & 0x0F) << 21) |
	    ((tRC & 0x3F) << 25) | ((tCCD & 0x01) << 31);

	dtpr1 = ((tAOND & 0x03) << 0) |
	    /*((tRTW&0x01)<<2) | use default */
	    ((tFAW & 0x3F) << 3) |
	    ((tMOD & 0x03) << 9) |
	    ((tRTODT & 0x0F) << 11) |
	    ((tRFC & 0xFF) << 16) | ((tDQSCK & 0x07) << 24)
	    /*((tDQSCKmax&0x07)<<27)use default */ ;

	dtpr2 = ((tXS & 0x3FF) << 0) |
	    ((tXP & 0x1F) << 10) |
	    ((tCKE & 0x0F) << 15) | ((tDLLK & 0x3FF) << 19);

	REG32(PUBL_DTPR0) = dtpr0;
	REG32(PUBL_DTPR1) = dtpr1;
	REG32(PUBL_DTPR2) = dtpr2;
}

uint32 ddr_rw_chk_single(uint32 offset, uint32 data) {
	uint32 rd;
	*(volatile uint32 *)(SDRAM_BASE + offset) = data;
	rd = *(volatile uint32 *)(SDRAM_BASE + offset);
	if (rd == data)
		return 1;
	else
		return 0;
}

uint32 ddr_rw_chk(uint32 offset) {
	uint32 i, data;
	for (i = 0; i < 6; i++) {
		if (i == 0) {
			data = 0x00000000;
		} else if (i == 1) {
			data = 0xffffffff;
		} else if (i == 2) {
			data = 0x12345678;
		} else if (i == 3) {
			data = 0x87654321;
		} else if (i == 4) {
			data = 0x5a5a5a5a;
		} else if (i == 5) {
			data = 0xa5a5a5a5;
		}
		if (ddr_rw_chk_single(offset, data) == 0)
			return 0;
		if (ddr_rw_chk_single(offset + 0x4, data) == 0)
			return 0;
		if (ddr_rw_chk_single(offset + 0x8, data) == 0)
			return 0;
		if (ddr_rw_chk_single(offset + 0xc, data) == 0)
			return 0;
	}
	return 1;
}

uint32 dqs_manual_training(uint32 offset) {
	uint32 i = 0;
	uint32 B0, B1, B2, B3;
	for (B0 = 0; B0 < 16; B0++) {
		UMCTL_REG_SET(PUBL_DX0DQSTR, B0);

		for (B1 = 0; B1 < 16; B1++) {
			UMCTL_REG_SET(PUBL_DX1DQSTR, B1);

			for (B2 = 0; B2 < 16; B2++) {
				UMCTL_REG_SET(PUBL_DX2DQSTR, B2);
				for (B3 = 0; B3 < 16; B3++) {
					UMCTL_REG_SET(PUBL_DX3DQSTR, B3);

					if (ddr_rw_chk(offset)) {
						UMCTL_REG_SET((SDRAM_BASE +
							       0x1000 + i),
							      (0xBA55 << 16) |
							      (B3 << 12) | (B2
									    <<
									    8) |
							      (B1 << 4) | B0);

						UMCTL_REG_SET(PUBL_DX0DQSTR,
							      (B0 << 4) | B0);
						UMCTL_REG_SET(PUBL_DX1DQSTR,
							      (B1 << 4) | B1);
						UMCTL_REG_SET(PUBL_DX2DQSTR,
							      (B2 << 4) | B2);
						UMCTL_REG_SET(PUBL_DX3DQSTR,
							      (B3 << 4) | B3);
						return 1;
					} else {
						UMCTL_REG_SET((SDRAM_BASE +
							       0x1000 + i),
							      (0xFA11 << 16) |
							      (B3 << 12) | (B2
									    <<
									    8) |
							      (B1 << 4) | B0);
					}
					i = i + 4;
				}
			}
		}
	}
	//if not found, set as default value
	UMCTL_REG_SET(PUBL_DX0DQSTR, 0xAA);
	UMCTL_REG_SET(PUBL_DX1DQSTR, 0xAA);
	UMCTL_REG_SET(PUBL_DX2DQSTR, 0xAA);
	UMCTL_REG_SET(PUBL_DX3DQSTR, 0xAA);
	return 0;
}

/*
 *Init sdram with publ build-in test.
 *Do training routine defined in PUBL_CFG_PIR.
*/
void phy_start_init(DRAM_DESC * dram) {
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
	reg_bits_set(PUBL_PGCR, 0, 1, (dram_type == DRAM_LPDDR1) ? 0x01 : 0x00);
	reg_bits_set(PUBL_PGCR, 1, 2, 0x01);	//attention lpddr1 set 0x01
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
	/*if((dram_type==DRAM_LPDDR2) || (dram_type==DRAM_DDR3))
	   reg_bits_set(PUBL_DSGCR,  0,12, 0x25F);    
	 */
	/*polling whether PHY initialization done */
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
	while (UMCTL_REG_GET(PUBL_PGSR) & 0x03 != 0x03) ;

//kevin.yanbin
	{
		uint32 value_temp = 0, i = 0;
		//triggering publ PIR initialization 
		UMCTL_REG_SET(PUBL_PIR, 0x00040001);
		for (i = 0; i <= 100; i++) ;

		//waite for initialize done
		do {
			value_temp = UMCTL_REG_GET(PUBL_PGSR);
		}
		while ((value_temp & 0x1) != 0x1);
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
	reg_bits_set(PUBL_PIR, 0, 32, 0x41);	/*trigger and do dram init */

	while (!(UMCTL_REG_GET(PUBL_PGSR) & BIT_0)) ;	/*wait dram init done */

#if 1
	dqs_manual_training(0x00);
#else
	/*use build-in DQS training,FPGA not support */
	reg_bits_set(PUBL_PIR, 0, 32, 0x81);	/*trigger and do read DQS training */
	DELAY_CLK(100);
	while (!(UMCTL_REG_GET(PUBL_PGSR) & BIT_4)) ;	/*wait data training done */
#endif
	/*!!PHY initialization complete!! */
}

/*
 *configure DATx8 register and its timing.
 *xiaohui.beijing
*/
void phy_datx8_config(DRAM_DESC * dram) {
	uint8 dram_type = dram->dram_type;
	/*datx8 common configuration register (DXCCR) */
	/*[1]: */
	reg_bits_set(PUBL_DXCCR, 1, 1,
		     ((dram_type == DRAM_LPDDR2) ? 0x00 : 0x01));
	/*[7:4]: */
	reg_bits_set(PUBL_DXCCR, 4, 4,
		     ((dram_type == DRAM_LPDDR2) ? 0x04 : 0x00) | ((dram_type ==
								    DRAM_LPDDR1)
								   ? 0x04 :
								   0x00));
	/*[11:8]: */
	reg_bits_set(PUBL_DXCCR, 8, 4,
		     ((dram_type == DRAM_LPDDR2) ? 0x0C : 0x00) | ((dram_type ==
								    DRAM_LPDDR1)
								   ? 0x0C :
								   0x00));
	/*[14]: */
	reg_bits_set(PUBL_DXCCR, 14, 1,
		     ((dram_type == DRAM_LPDDR2) ? 0x00 : 0x01));

	/*DXxGCR[10:9]:disable DQ/DQS dynamic RTT controll */
	reg_bits_set(PUBL_DX0GCR, 9, 2, 0x00);
	reg_bits_set(PUBL_DX1GCR, 9, 2, 0x00);
	reg_bits_set(PUBL_DX2GCR, 9, 2, 0x00);
	reg_bits_set(PUBL_DX3GCR, 9, 2, 0x00);

	reg_bits_set(PUBL_DX0DQTR, 0, 5, 0xf);
	reg_bits_set(PUBL_DX1DQTR, 0, 5, 0xf);
	reg_bits_set(PUBL_DX2DQTR, 0, 5, 0xf);
	reg_bits_set(PUBL_DX3DQTR, 0, 5, 0xf);
}

/*
 *Use umctl controller to initialize Mode register in sdram.
 *Refer to PUBL databook Chapter 3.3.13(MR0)~3.3.16(MR3) for detail.
*/
void publ_mdr_init(DRAM_DESC * dram) {
	uint8 dram_type = dram->dram_type;
	uint8 BL = dram->bl;
	uint8 CL = dram->rl;
	uint8 RL = dram->rl;
	uint8 WL = dram->wl;
	uint8 AL = dram->al;

	/*Get the timing we used. */
	LPDDR_ACTIMING *lpddr1_timing = (LPDDR_ACTIMING *) (dram->ac_timing);
	LPDDR2_ACTIMING *lpddr2_timing = (LPDDR2_ACTIMING *) (dram->ac_timing);
	DDR2_ACTIMING *ddr2_timing = (DDR2_ACTIMING *) (dram->ac_timing);
	DDR3_ACTIMING *ddr3_timing = (DDR3_ACTIMING *) (dram->ac_timing);

	if (dram_type == DRAM_DDR3) {
		uint8 tWR = ddr3_timing->tWR;
		uint8 CWL = (WL - AL) ? (WL - AL) : 0x00;
		/*Mode register 0(MR0) */
		reg_bits_set(PUBL_MR0, 0, 2, ((BL == 8) ? 0x00 : 0x00) |	/*Fixed on 8 */
			     ((BL == 4) ? 0x01 : 0x00) |	/*4or8 on the fly */
			     ((BL == 4) ? 0x02 : 0x00));	/*Fixed on 4 */
		/*BurstType:0-sequential;1-interleave */
		reg_bits_set(PUBL_MR0, 3, 1,
			     ((BURST_TYPE ==
			       DRAM_BT_SEQ) ? 0x00 : 0x00) | ((BURST_TYPE ==
							       DRAM_BT_INTER) ?
							      0x01 : 0x00));
		reg_bits_set(PUBL_MR0, 4, 3, ((CL == 5) ? 0x01 : 0x00) |	/*CAS Latency */
			     ((CL == 6) ? 0x02 : 0x00) |
			     ((CL == 7) ? 0x03 : 0x00) |
			     ((CL == 8) ? 0x04 : 0x00) |
			     ((CL == 9) ? 0x05 : 0x00) |
			     ((CL == 10) ? 0x06 : 0x00) |
			     ((CL == 11) ? 0x07 : 0x00));
		reg_bits_set(PUBL_MR0, 7, 1, 0x00);	/*OperationMode:0-opMode;1-TestMode */
		reg_bits_set(PUBL_MR0, 8, 1, 0x00);	/*DLLReset:1-ResetDll,this bit is self-clearing */
		reg_bits_set(PUBL_MR0, 9, 3, ((tWR <= 5) ? 0x01 : 0x00) |	/*WriteRecovery */
			     ((tWR == 6) ? 0x02 : 0x00) |
			     ((tWR == 7) ? 0x03 : 0x00) |
			     ((tWR == 8) ? 0x04 : 0x00) |
			     ((tWR == 10) ? 0x05 : 0x00) |
			     ((tWR == 12) ? 0x06 : 0x00));
		/*PowerdownControl:0-SlowExit(DllOff);1-FastExit(DllOn) */
		reg_bits_set(PUBL_MR0, 12, 1, 0x00);

		/*Mode register 1(MR1) */
		reg_bits_set(PUBL_MR1, 0, 1, 0x00);	/*DllEnable(0),DllDis(1) */
		/*Output Driver Impedance Control
		   [A5,A1]:00-RZQ/6;01-RZQ/7;10/11-RZQ/TBD;Note: RZQ=240ohm */
		reg_bits_set(PUBL_MR1, 1, 1, 0);
		reg_bits_set(PUBL_MR1, 5, 1, 0);
		/*[A4:A3]:Additive Latency. */
		reg_bits_set(PUBL_MR1, 3, 2, ((AL == 0) ? 0x00 : 0x00) |
			     ((AL == CL - 1) ? 0x01 : 0x00) |
			     ((AL == CL - 2) ? 0x02 : 0x00)
		    );
		/*[A7]:1-Write leveling enable;0-Disabled */
		reg_bits_set(PUBL_MR1, 7, 1, 1);

		/*mr2 store the value to write to MR2 register */
		/*Partial Array Self-Refresh (Optional),[A2:A0]
		 */
		/*CAS write Latency (CWL),WL=CWL+AL
		   [A5:A3]:5~12 tCK
		 */
		reg_bits_set(PUBL_MR1, 3, 3, ((CWL == 5) ? 0x00 : 0x00) |
			     ((CWL == 6) ? 0x01 : 0x00) |
			     ((CWL == 7) ? 0x02 : 0x00) |
			     ((CWL == 8) ? 0x03 : 0x00) |
			     ((CWL == 9) ? 0x04 : 0x00) |
			     ((CWL == 10) ? 0x05 : 0x00) |
			     ((CWL == 11) ? 0x06 : 0x00) |
			     ((CWL == 12) ? 0x07 : 0x00)
		    );
		/*Rtt_WR
		   00-Dynamic ODT off; 01-RZQ/4;
		   10-RZQ/2;           11-Reserved
		 */
		reg_bits_set(PUBL_MR1, 9, 2, 0x02);
	} else if (dram_type == DRAM_DDR) {
		/*to be done */
	} else if (dram_type == DRAM_LPDDR1) {
		/*Mode register 0(MR0) */
		reg_bits_set(PUBL_MR0, 0, 3, ((BL == 2) ? 0x01 : 0x00) |
			     ((BL == 4) ? 0x02 : 0x00) |
			     ((BL == 8) ? 0x03 : 0x00) |
			     ((BL == 16) ? 0x04 : 0x00));
		/*BurstType:0-sequential;1-interleave */
		reg_bits_set(PUBL_MR0, 3, 1,
			     ((BURST_TYPE ==
			       DRAM_BT_SEQ) ? 0x00 : 0x00) | ((BURST_TYPE ==
							       DRAM_BT_INTER) ?
							      0x01 : 0x00));
		reg_bits_set(PUBL_MR0, 4, 3, ((CL == 2) ? 0x02 : 0x00) |	/*CAS Latency */
			     ((CL == 3) ? 0x03 : 0x00) |
			     ((CL == 4) ? 0x04 : 0x00));
		reg_bits_set(PUBL_MR0, 7, 1, 0x00);	/*OperationMode:0-opMode;1-TestMode */
		reg_bits_set(PUBL_MR0, 8, 8, 0x00);	/*[15:8] Reserved. */

		/*Mode register 2(map to LPDDR EMR) */
		reg_bits_set(PUBL_MR2, 0, 3, 0x00);	/*PASR:0:AllBanks;1:HalfArray;2:QuarterArray,etc */
		reg_bits_set(PUBL_MR2, 3, 2, 0x00);	/*TCSR:0:70'C;1:45'C;2:15'C;3:85'C; */
		reg_bits_set(PUBL_MR2, 5, 3,
			     ((IO_DS == DS_FULL) ? 0x00 : 0x00) | ((IO_DS ==
								    DS_HALF) ?
								   0x01 : 0x00)
			     | ((IO_DS == DS_QUARTER) ? 0x02 : 0x00) |
			     ((IO_DS == DS_OCTANT) ? 0x03 : 0x00) | ((IO_DS ==
								      DS_THREE_QUATERS)
								     ? 0x04 :
								     0x00)
		    );		/*DS:0:FullDriverStrength;1:HalfDS;2:QuaterDS;3:OctantDS;4:Three-Quater */
		reg_bits_set(PUBL_MR2, 8, 8, 0x00);	/*[15:8] Reserved. */
	} else if (dram_type == DRAM_DDR2) {
		/*to be done */
	} else if (dram_type == DRAM_LPDDR2) {
		uint8 tWR = lpddr2_timing->tWR;
		/*Mode register 1(MR1) */
		reg_bits_set(PUBL_MR0, 0, 32, 0x00);
		/*Mode register 1(MR1) */
		reg_bits_set(PUBL_MR1, 0, 3, ((BL == 4) ? 0x02 : 0x00) |
			     ((BL == 8) ? 0x03 : 0x00) |
			     ((BL == 16) ? 0x04 : 0x00));
		/*BurstType:0-sequential;1-interleave */
		reg_bits_set(PUBL_MR1, 3, 1,
			     ((BURST_TYPE ==
			       DRAM_BT_SEQ) ? 0x00 : 0x00) | ((BURST_TYPE ==
							       DRAM_BT_INTER) ?
							      0x01 : 0x00));
		reg_bits_set(PUBL_MR1, 4, 1, 0x00);	/*WrapControl:0-wrap;1-Nowrap */
		reg_bits_set(PUBL_MR1, 5, 3, ((tWR == 3) ? 0x01 : 0x00) |	/*WriteRecovery,Default(0x01) */
			     ((tWR == 4) ? 0x02 : 0x00) |
			     ((tWR == 5) ? 0x03 : 0x00) |
			     ((tWR == 6) ? 0x04 : 0x00) |
			     ((tWR == 7) ? 0x05 : 0x00) |
			     ((tWR == 8) ? 0x06 : 0x00));

		/*Mode register 2(MR2) */
		reg_bits_set(PUBL_MR2, 0, 4, (((RL == 3)
					       && (WL ==
						   1)) ? 0x01 : 0x00) | (((RL ==
									   4)
									  && (WL
									      ==
									      2))
									 ? 0x02
									 : 0x00)
			     | (((RL == 5)
				 && (WL == 2)) ? 0x03 : 0x00) | (((RL == 6)
								  && (WL ==
								      3)) ? 0x04
								 : 0x00) |
			     (((RL == 7)
			       && (WL == 4)) ? 0x05 : 0x00) | (((RL == 8)
								&& (WL ==
								    4)) ? 0x06 :
							       0x00));
		reg_bits_set(PUBL_MR2, 4, 4, 0x00);	/*Reserved. */

		/*Mode register 3(MR3) */
		/*DS,driver strength configuration.Default,40ohm */
		reg_bits_set(PUBL_MR3, 0, 4,
			     ((IO_DS == DS_34R3) ? 0x01 : 0x00) | ((IO_DS ==
								    DS_40R) ?
								   0x02 : 0x00)
			     | ((IO_DS == DS_48R) ? 0x03 : 0x00) |
			     ((IO_DS == DS_60R) ? 0x04 : 0x00) | ((IO_DS ==
								   DS_68R6) ?
								  0x05 : 0x00) |
			     ((IO_DS == DS_80R) ? 0x06 : 0x00) | ((IO_DS ==
								   DS_120R) ?
								  0x07 : 0x00));
		reg_bits_set(PUBL_MR2, 4, 4, 0x00);	/*[7:4],Reserved. */
	} else if (dram_type == DRAM_LPDDR3) {
		/*to be done */
	}
}

void sdram_clk_sel(DRAM_DESC * dram, uint32 clock) {
	/*changde to be done */
	//SDRAM_CORE_CLK = clock;
}

/**---------------------------------------------------------------------------*
 **                            PUBLIC Functions
 **---------------------------------------------------------------------------*/
/*
 *Init the sdram using uMCTL and PUBL.
 *Refer to uMCTL2 user guide Chapter3.1.5 Table3-2
 *"DWC_ddr_umctl2 and memory initialization with PUBL"
*/

void sdram_init(void) {
	DRAM_DESC *dram = SDRAM_GetCfg();
	 DDR_init(dram);
} void DDR_init(DRAM_DESC * dram) {
/*NOTE:
 *Ensure that initializing all APB registers in reset mode,except for Dynamic Registers.
 */
	sdram_clk_sel(dram, SDRAM_CORE_CLK);
	umctl_soft_reset(TRUE);
	UMCTL_REG_SET(UMCTL_DFIMISC, 0x0);
	//umctl_wait_status(OPERATION_MODE_INIT);
	umctl_core_init(dram);
	/*Power-up timing initialization and ModeRegister. */
	umctl_poweron_init(dram);
    /**Set sdram timing parameters,MCTL_DRAMTMG0~MCTL_DRAMTMG8.*/
	umctl_sdram_timing(dram);
	umctl_soft_reset(FALSE);

	//DFI initialization
	//Address mapping
	//PERFxx setting
	//Each port configuration
	umctl_other_init(dram);
	umctl_wait_status(OPERATION_MODE_NORMAL);

	/*configure PUBL_CFG_PTR0/1/2 and PUBL_CFG_DTPR0/1/2 */
	phy_sdram_timing(dram);
	/*configure mdr in phy */
	publ_mdr_init(dram);
	phy_datx8_config(dram);
	phy_start_init(dram);

	/*configure power control */
	UMCTL_REG_SET(UMCTL_PWRCTL, 0x08);
}

