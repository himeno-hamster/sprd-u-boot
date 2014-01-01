/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define DOLPHIN_UBOOT	 

#ifdef DOLPHIN_UBOOT
#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <config.h>

#include <asm/arch/sci_types.h>
#include <asm/arch/pinmap.h>
#include <asm/arch/bits.h>

#include <nand.h>
#include <linux/mtd/nand.h>

#include <asm/arch-sc8830/sprd_nfc_reg_v2.h>
//#include <asm/arch/sc8810_reg_base.h>
#include <asm/arch/regs_ana.h>
#include <asm/arch/analog_reg_v3.h>
//#include <asm/arch/sc8810_reg_ahb.h>
//#include <asm/arch/sc8810_reg_global.h>
#include <asm/arch/gpio_drvapi.h>
#include <asm/arch/regs_global.h>
//#include <asm/arch/regs_cpc.h>
//#include <asm/arch/pin_reg_v3.h>

#define mdelay(ms) do{ int volatile i = 0; for(i; i < 0xFFFF; i++); } while(0)

//#define NAND_DBG
#ifdef CONFIG_NAND_SPL
#define printf(arg...) do{}while(0)
#endif
#ifndef NAND_DBG
#define printf(arg...) do{}while(0)
#endif

#define DPRINT(arg...) do{}while(0)

/* 2 bit correct, sc8810 support 1, 2, 4, 8, 12,14, 24 */
#define CONFIG_SYS_NAND_ECC_MODE	2
//#define CONFIG_SYS_NAND_ECC_MODE	4
/* Number of ECC bytes per OOB - S3C6400 calculates 4 bytes ECC in 1-bit mode */
#define CONFIG_SYS_NAND_ECCBYTES	4
//#define CONFIG_SYS_NAND_ECCBYTES	7
/* Size of the block protected by one OOB (Spare Area in Samsung terminology) */
#define CONFIG_SYS_NAND_ECCSIZE	512
#define CONFIG_SYS_NAND_5_ADDR_CYCLE	5

#define NFC_MC_ICMD_ID	(0xCD)
#define NFC_MC_ADDR_ID	(0x0A)
#define NFC_MC_WRB0_ID	(0xB0)
#define NFC_MC_WRB1_ID	(0xB1)
#define NFC_MC_MRDT_ID	(0xD0)
#define NFC_MC_MWDT_ID	(0xD1)
#define NFC_MC_SRDT_ID	(0xD2)
#define NFC_MC_SWDT_ID	(0xD3)
#define NFC_MC_IDST_ID	(0xDD)
#define NFC_MC_CSEN_ID	(0xCE)
#define NFC_MC_NOP_ID	(0xF0)
#define NFC_MC_DONE_ID	(0xFF)
#define NFC_MAX_CHIP	1
#define NFC_TIMEOUT_VAL		0x1000000

#define NAND_MC_CMD(x)  (uint16_t)(((x & 0xff) << 8) | NFC_MC_ICMD_ID)
#define NAND_MC_ADDR(x) (uint16_t)(((x & 0xff) << 8) | (NFC_MC_ADDR_ID << 4))
#define NAND_MC_WRB0(x) (uint16_t)(NFC_MC_WRB0_ID)
#define NAND_MC_MRDT	(uint16_t)(NFC_MC_MRDT_ID)
#define NAND_MC_MWDT	(uint16_t)(NFC_MC_MWDT_ID)
#define NAND_MC_SRDT	(uint16_t)(NFC_MC_SRDT_ID)
#define NAND_MC_SWDT	(uint16_t)(NFC_MC_SWDT_ID)
#define NAND_MC_IDST(x)	(uint16_t)((NFC_MC_IDST_ID) | ((x -1) << 8))
#define NAND_MC_NOP(x)	(uint16_t)(((x & 0xff) << 8) | NFC_MC_NOP_ID)


#define NAND_MC_BUFFER_SIZE (24)

static int mtderasesize = 0;
static int mtdwritesize = 0;
static int mtdoobsize = 0;

#endif  //DOLPHIN_UBOOT end


#ifdef DOLPHIN_KERNEL
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <mach/globalregs.h>
#include "sc8830_nand.h"
#include <mach/pinmap.h>

#define DPRINT(arg...) do{}while(0)
#endif  //DOLPHIN_KERNEL


struct sprd_dolphin_nand_param {
	uint8_t id[5];
	uint8_t bus_width;
	uint8_t a_cycles;
	uint8_t sct_pg; //sector per page
	uint8_t oob_size; /* oob size per sector*/
	uint8_t ecc_pos; /* oob size per sector*/
	uint8_t info_pos; /* oob size per sector*/
	uint8_t info_size; /* oob size per sector*/
	uint8_t eccbit; /* ecc level per eccsize */
	uint16_t eccsize; /*bytes per sector for ecc calcuate once time */
	uint8_t	ace_ns;	/* ALE, CLE end of delay timing, unit: ns */
	uint8_t	rwl_ns;	/* WE, RE, IO, pulse  timing, unit: ns */
	uint8_t	rwh_ns;	/* WE, RE, IO, high hold  timing, unit: ns */
};

struct sprd_dolphin_nand_info {
	struct mtd_info *mtd;
	struct nand_chip *nand;

	#ifdef DOLPHIN_UBOOT
	struct device *pdev;
	#endif

	#ifdef DOLPHIN_KERNEL
	struct platform_device *pdev;
	#endif

	struct sprd_dolphin_nand_param *param;
	uint32_t chip; //chip index
	uint32_t v_mbuf; //virtual main buffer address
	uint32_t p_mbuf; //phy main buffer address
	uint32_t v_oob; // virtual oob buffer address
	uint32_t p_oob; //phy oob buffer address
	uint32_t page; //page address
	uint16_t column; //column address
	uint16_t oob_size;
	uint16_t m_size; //main part size per sector
	uint16_t s_size; //oob size per sector
	uint8_t a_cycles;//address cycles, 3, 4, 5
	uint8_t sct_pg; //sector per page
	uint8_t info_pos;
	uint8_t info_size;
	uint8_t ecc_mode;//0-1bit, 1-2bit, 2-4bit, 3-8bit,4-12bit,5-16bit,6-24bit
	uint8_t ecc_pos; // ecc postion
	uint8_t wp_en; //write protect enable
	uint16_t write_size;
	uint16_t page_per_bl;//page per block
	uint16_t  buf_head;
	uint16_t _buf_tail;
	uint8_t ins_num;//instruction number
	uint32_t ins[NAND_MC_BUFFER_SIZE >> 1];	
};


#define mtd_to_dolphin(m) (&g_dolphin)

//gloable variable 
static struct nand_ecclayout sprd_dolphin_nand_oob_default = {
	.eccbytes = 0,
	.eccpos = {0},
	.oobfree = {
		{.offset = 2,
		 .length = 46}}
};
struct sprd_dolphin_nand_info g_dolphin = {0};
//save the data read by read_byte and read_word api interface functon
static __attribute__((aligned(4))) uint8_t s_oob_data[NAND_MAX_OOBSIZE];
//static __attribute__((aligned(4))) uint8_t s_oob_data[8];
//static __attribute__((aligned(4))) uint8_t s_id_status[8];

static void sprd_dolphin_nand_read_id(struct sprd_dolphin_nand_info *dolphin, uint32_t *buf);
static void sprd_dolphin_nand_reset(struct sprd_dolphin_nand_info *dolphin);
static int sprd_dolphin_nand_wait_finish(struct sprd_dolphin_nand_info *dolphin); 

static uint32_t sprd_dolphin_reg_read(uint32_t addr)
{
	return readl(addr);
}
static void sprd_dolphin_reg_write(uint32_t addr, uint32_t val)
{
	writel(val, addr);
}
static void sprd_dolphin_reg_or(uint32_t addr, uint32_t val)
{
	sprd_dolphin_reg_write(addr, sprd_dolphin_reg_read(addr) | val);
}
static void sprd_dolphin_reg_and(uint32_t addr, uint32_t mask)
{
	sprd_dolphin_reg_write(addr, sprd_dolphin_reg_read(addr) & mask);
}
static void sprd_dolphin_nand_int_clr(uint32_t bit_clear)
{
	sprd_dolphin_reg_write(NFC_INT_REG, bit_clear);
}
unsigned int ecc_mode_convert(uint32_t mode)
{
	uint32_t mode_m;
	switch(mode)
	{
	case 1:
		mode_m = 0;
		break;
	case 2:
		mode_m = 1;
		break;
	case 4:
		mode_m = 2;
		break;
	case 8:
		mode_m = 3;
		break;
	case 12:
		mode_m = 4;
		break;
	case 16:
		mode_m = 5;
		break;
	case 24:
		mode_m = 6;
		break;
	case 40:
		mode_m = 7;
		break;
	case 60:
		mode_m = 8;
		break;
	default:
		mode_m = 0;
		break;
	}
	return mode_m;
}

/*spare info must be align to ecc pos, info_pos + info_size = ecc_pos, 
 *the hardware must be config info_size and info_pos when ecc enable,and the ecc_info size can't be zero,
 *to simplify the nand_param_tb, the info is align with ecc and ecc at the last postion in one sector
*/
static struct sprd_dolphin_nand_param sprd_dolphin_nand_param_tb[] = {
	{{0xec, 0xbc, 0x00,0x55, 0x54}, 	1, 	5, 	4, 	16, 	12, 	11, 	1, 	2, 	512, 5, 21, 10},
	{{0xec, 0xbc, 0x00,0x6A, 0x56}, 	1, 	5, 	8, 	16, 	12, 	11, 	1, 	2, 	512, 10, 25, 15},
	//{{0xec, 0xbc, 0x00,0x6A, 0x56}, 	1, 	5, 	8, 	16, 	12, 	11, 	1, 	2, 	512, 5, 21, 10},
	//{{0xec, 0xbc, 0x00,0x6A, 0x56}, 	1,	5,	8,	8, 		4,		3,		1,	2,	512, 5, 21, 10},
	{{0xad, 0xbc, 0x90,0x55, 0x54}, 	1, 	5, 	4, 	16, 	12, 	11, 	1, 	2, 	512, 10, 25, 15},
	{{0x2c, 0xbc, 0x90,0x66, 0x54}, 	1,	5,	8,	16, 	12, 	11, 	1,	2,	512, 10, 25, 15},
};



#ifdef CONFIG_NAND_SPL
struct sprd_dolphin_boot_header_info {
	uint32_t check_sum;
	uint32_t sct_size; //
	uint32_t acycle; // 3, 4, 5
	uint32_t bus_width; //0 ,1
	uint32_t spare_size; //spare part sise for one sector
	uint32_t ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	uint32_t ecc_pos; // ecc postion at spare part
	uint32_t sct_per_page; //sector per page
	uint32_t info_pos;
	uint32_t info_size;
	uint32_t magic_num; //0xaa55a5a5	
	uint32_t ecc_value[27];
};
void boad_nand_param_init(struct sprd_dolphin_nand_info *dolphin, struct nand_chip *chip, uint8 *id)
{
	int extid;
	uint32_t writesize;
	uint32_t oobsize;
	uint32_t erasesize;
	uint32_t busw;
	
	/* The 4th id byte is the important one */
	extid = id[3];
	writesize = 1024 << (extid & 0x3);
	extid >>= 2;
	/* Calc oobsize */
	oobsize = (8 << (extid & 0x01)) * (writesize >> 9);
	extid >>= 2;
	/* Calc blocksize. Blocksize is multiples of 64KiB */
	erasesize = (64 * 1024) << (extid & 0x03);
	extid >>= 2;
	/* Get buswidth information */
	busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
	dolphin->write_size = writesize;
	dolphin->m_size =CONFIG_SYS_NAND_ECCSIZE;
	dolphin->sct_pg = writesize / CONFIG_SYS_NAND_ECCSIZE;
	dolphin->s_size = oobsize / dolphin->sct_pg;
	dolphin->ecc_mode = ecc_mode_convert(CONFIG_SYS_NAND_ECC_MODE);
	dolphin->ecc_pos = dolphin->s_size - ((14 * CONFIG_SYS_NAND_ECC_MODE + 7) / 8);
	dolphin->info_pos = dolphin->ecc_pos - 1;
	dolphin->info_size = 1;
	dolphin->page_per_bl = erasesize / dolphin->write_size;
	dolphin->a_cycles = CONFIG_SYS_NAND_5_ADDR_CYCLE;
	if(NAND_BUSWIDTH_16 == busw)
	{
		chip->options |= NAND_BUSWIDTH_16;
	}
	else
	{
		chip->options &= ~NAND_BUSWIDTH_16;
	}
}

/*
 * because the dolphin firmware use the nand identify process
 * and the data at the header of nand_spl is the nand param used at nand read and write,
 * so in nand_spl, don't need read the id or use the onfi spec to calculate the nand param,
 * just use the param at the nand_spl header instead of
 */
void nand_hardware_config(struct mtd_info *mtd,struct nand_chip *chip)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	struct sprd_dolphin_boot_header_info *header;
	int index;
	int array;
	struct sprd_dolphin_nand_param * param;
	uint8 *id;
	sprd_dolphin_nand_reset(dolphin);
	sprd_dolphin_nand_read_id(dolphin, (uint32_t *)s_oob_data);
	boad_nand_param_init(dolphin, dolphin->nand, s_oob_data);
	id = s_oob_data;
	array = ARRAY_SIZE(sprd_dolphin_nand_param_tb);

	for (index = 0; index < array; index ++) {
		param = sprd_dolphin_nand_param_tb + index;
		if ((param->id[0] == id[0]) 
			&& (param->id[1] == id[1]) 
			&& (param->id[2] == id[2]) 
			&& (param->id[3] == id[3]) 
			&& (param->id[4] == id[4]))
			break;
	}
	if (index < array) {
		dolphin->ecc_mode = ecc_mode_convert(param->eccbit);
		dolphin->m_size = param->eccsize;
		dolphin->s_size = param->oob_size;
		dolphin->a_cycles = param->a_cycles;
		dolphin->sct_pg = param->sct_pg;
		dolphin->info_pos = param->info_pos;
		dolphin->info_size = param->info_size;
		dolphin->write_size = dolphin->m_size * dolphin->sct_pg;
		dolphin->ecc_pos = param->ecc_pos;
		if(param->bus_width)
		{
			chip->options |= NAND_BUSWIDTH_16;
		}
		else
		{
			chip->options &= ~NAND_BUSWIDTH_16;
		}
		mtd->writesize = dolphin->write_size;
		mtd->oobsize = dolphin->s_size * dolphin->sct_pg;
		dolphin->nand=chip;
	}
	mtd->writesize = dolphin->write_size;
	mtd->oobsize = dolphin->s_size * dolphin->sct_pg;
	mtd->erasesize = dolphin->page_per_bl * dolphin->write_size;
}
#else 

static void sprd_dolphin_nand_ecc_layout_gen(struct sprd_dolphin_nand_info *dolphin, struct sprd_dolphin_nand_param *param, struct nand_ecclayout *layout)
{
	uint32_t sct = 0;
	uint32_t i = 0;
	uint32_t offset;
	uint32_t used_len ; //one sector ecc data size(byte)
	uint32_t eccbytes = 0; //one page ecc data size(byte)
	uint32_t oobfree_len = 0;
	used_len = (14 * param->eccbit + 7) / 8 + param->info_size;
	if(param->sct_pg > ARRAY_SIZE(layout->oobfree))
	{
		while(1);
	}
	for(sct = 0; sct < param->sct_pg; sct++)
	{
		//offset = (oob_size * sct) + ecc_pos;
		//for(i = 0; i < ecc_len; i++)
		offset = (param->oob_size * sct) + param->info_pos;
		for(i = 0; i < used_len; i++)
		{
			layout->eccpos[eccbytes++] = offset + i;
		}
		layout->oobfree[sct].offset = param->oob_size * sct;
		layout->oobfree[sct].length = param->oob_size - used_len ;
		oobfree_len += param->oob_size - used_len;
	}
	//for bad mark postion
	layout->oobfree[0].offset = 2;
	layout->oobfree[0].length = param->oob_size - used_len - 2;
	oobfree_len -= 2;
	layout->eccbytes = used_len * param->sct_pg;
}

void nand_hardware_config(struct mtd_info *mtd, struct nand_chip *chip, u8 id[5])
{
	int index;
	int array;
	struct sprd_dolphin_nand_param * param;
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	
	array = ARRAY_SIZE(sprd_dolphin_nand_param_tb);
	for (index = 0; index < array; index ++) {
		param = sprd_dolphin_nand_param_tb + index;
		if ((param->id[0] == id[0]) 
			&& (param->id[1] == id[1]) 
			&& (param->id[2] == id[2]) 
			&& (param->id[3] == id[3]) 
			&& (param->id[4] == id[4]))
			break;
	}
	if (index < array) {
		//save the param config
		dolphin->ecc_mode = ecc_mode_convert(param->eccbit);
		dolphin->m_size = param->eccsize;
		dolphin->s_size = param->oob_size;
		dolphin->a_cycles = param->a_cycles;
		dolphin->sct_pg = param->sct_pg;
		dolphin->info_pos = param->info_pos;
		dolphin->info_size = param->info_size;
		dolphin->write_size = dolphin->m_size * dolphin->sct_pg;
		dolphin->ecc_pos = param->ecc_pos;
		chip->eccbitmode = param->eccbit;
		chip->ecc.bytes  = (param->eccbit*14+7)/8;
		//dolphin->bus_width = param->bus_width;
		if(param->bus_width)
		{
			chip->options |= NAND_BUSWIDTH_16;
		}
		else
		{
			chip->options &= ~NAND_BUSWIDTH_16;
		}
		dolphin->param = param;
		//update the mtd and nand default param after nand scan
		mtd->writesize = dolphin->write_size;
		mtd->oobsize = dolphin->s_size * dolphin->sct_pg;
		dolphin->oob_size = mtd->oobsize;
		/* Calculate the address shift from the page size */
		chip->page_shift = ffs(mtd->writesize) - 1;
		/* Convert chipsize to number of pages per chip -1. */
		chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;
		sprd_dolphin_nand_ecc_layout_gen(dolphin, param, &sprd_dolphin_nand_oob_default);
		chip->ecc.layout = &sprd_dolphin_nand_oob_default;
		dolphin->mtd = mtd;
	}
	else {
		int steps;
		struct sprd_dolphin_nand_param  param1;
		
		//save the param config
		steps = mtd->writesize / CONFIG_SYS_NAND_ECCSIZE;
		dolphin->ecc_mode = ecc_mode_convert(CONFIG_SYS_NAND_ECC_MODE);
		dolphin->m_size = CONFIG_SYS_NAND_ECCSIZE;
		dolphin->s_size = mtd->oobsize / steps;
		dolphin->a_cycles = mtd->writesize / CONFIG_SYS_NAND_ECCSIZE;
		dolphin->sct_pg = steps;
		dolphin->info_pos = dolphin->s_size - CONFIG_SYS_NAND_ECCBYTES - 1;
		dolphin->info_size = 1;
		dolphin->write_size = mtd->writesize;
		dolphin->oob_size = mtd->oobsize;
		dolphin->ecc_pos = dolphin->s_size - CONFIG_SYS_NAND_ECCBYTES;
		param1.sct_pg = dolphin->sct_pg;
		param1.info_pos = dolphin->info_pos;
		param1.info_size = dolphin->info_size;
		param1.oob_size = dolphin->s_size;
		param1.eccbit = CONFIG_SYS_NAND_ECC_MODE;
		param1.eccsize = dolphin->s_size;

		chip->eccbitmode = CONFIG_SYS_NAND_ECC_MODE;
		chip->ecc.bytes  = (CONFIG_SYS_NAND_ECC_MODE * 14 + 7) / 8;
		//printf("hardware default eccbitmode %d\n", chip->eccbitmode);
		
		if(chip->chipsize > (128 << 20)) {
			dolphin->a_cycles = 5;
		}
		else {
			dolphin->a_cycles = 4;
		}

		sprd_dolphin_nand_ecc_layout_gen(dolphin, &param1, &sprd_dolphin_nand_oob_default);
		chip->ecc.layout = &sprd_dolphin_nand_oob_default;
		dolphin->mtd = mtd;
	}
	dolphin->mtd = mtd;
}
#endif  //end CONFIG_NAND_SPL



#ifdef DOLPHIN_UBOOT
#ifndef CONFIG_NAND_SPL
typedef struct {
	uint8_t *m_buf;
	uint8_t *s_buf;
	uint8_t m_sct;
	uint8_t s_sct;
	uint8_t dir; //if dir is 0, read dadta from NFC buffer, if 1, write data to NFC buffer
	uint16_t m_size;
	uint16_t s_size;
} sprd_dolphin_nand_data_param;

static unsigned int sprd_dolphin_data_trans(sprd_dolphin_nand_data_param *param)
{
	uint32_t cfg0 = 0;
	uint32_t cfg1 = 0;
	uint32_t cfg2 = 0;
	cfg0 = NFC_ONLY_MST_MODE | MAIN_SPAR_APT | NFC_WPN;
	if(param->dir)
	{
		cfg0 |= NFC_RW;
	}
	if(param->m_sct != 0)
	{
		cfg0 |= (param->m_sct - 1) << SECTOR_NUM_OFFSET;
		cfg0 |= MAIN_USE;
		cfg1 |= (param->m_size - 1);
		sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)param->m_buf);
	}
	if(param->s_sct != 0)
	{
		cfg0 |= SPAR_USE;
		cfg1 |= (param->s_size - 1) << SPAR_SIZE_OFFSET;
		cfg2 |= (param->s_sct - 1) << SPAR_SECTOR_NUM_OFFSET;
		sprd_dolphin_reg_write(NFC_SPAR_ADDR_REG, (uint32_t)param->s_buf);
	}
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_CFG1_REG, cfg1);
	sprd_dolphin_reg_write(NFC_CFG2_REG, cfg2);
	sprd_dolphin_nand_int_clr(INT_STSMCH_CLR | INT_WP_CLR | INT_TO_CLR | INT_DONE_CLR);//clear all interrupt
	sprd_dolphin_reg_write(NFC_START_REG, NFC_START);
	sprd_dolphin_nand_wait_finish(&g_dolphin);
	return 0;
}
void sprd_ecc_ctrl(struct sprd_ecc_param *param, uint32_t dir)
{
	uint32_t cfg0 = 0;
	uint32_t cfg1 = 0;
	uint32_t cfg2 = 0;
	cfg0 = NFC_ONLY_ECC_MODE | MAIN_SPAR_APT;
	if(dir)
	{
		cfg0 |= NFC_RW;
	}
	cfg1 |=(param->sinfo_size - 1) << SPAR_INFO_SIZE_OFFSET;
	cfg1 |=(param->sp_size - 1) << SPAR_SIZE_OFFSET;
	cfg1 |= (param->m_size - 1);
	
	cfg2 |= (param->sinfo_pos)<< SPAR_INFO_POS_OFFSET;
	cfg2 |= ecc_mode_convert(param->mode) << ECC_MODE_OFFSET;
	cfg2 |= param->ecc_pos;
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_CFG1_REG, cfg1);
	sprd_dolphin_reg_write(NFC_CFG2_REG, cfg2);
	sprd_dolphin_nand_int_clr(INT_STSMCH_CLR | INT_WP_CLR | INT_TO_CLR | INT_DONE_CLR);//clear all interrupt
	sprd_dolphin_reg_write(NFC_START_REG, NFC_START);
	sprd_dolphin_nand_wait_finish(&g_dolphin);
}

unsigned int sprd_ecc_encode(struct sprd_ecc_param *param)
{
	struct sprd_dolphin_nand_info *dolphin;
	sprd_dolphin_nand_data_param d_param;

	dolphin = &g_dolphin;
	memset(&d_param, 0, sizeof(d_param));

	d_param.m_buf = param->p_mbuf;
	d_param.s_buf = param->p_sbuf;
	d_param.m_sct = param->ecc_num;
	d_param.s_sct = param->ecc_num;
	d_param.dir = 1;
	d_param.m_size = param->m_size;
	d_param.s_size = param->sp_size;

	Dcache_CleanRegion((unsigned int)d_param.m_buf, d_param.m_sct*d_param.m_size);
	Dcache_CleanRegion((unsigned int)d_param.s_buf, d_param.s_sct*d_param.s_size);

	sprd_dolphin_data_trans(&d_param);
	sprd_ecc_ctrl(param, 1);
	d_param.dir = 0;
	d_param.m_sct = 0;

	Dcache_InvalRegion((unsigned int)d_param.m_buf , d_param.m_sct*d_param.m_size);
	Dcache_InvalRegion((unsigned int)d_param.s_buf , d_param.s_sct*d_param.s_size);

	sprd_dolphin_data_trans(&d_param); //read the ecc value from nfc buffer
	return 0;
}
#endif  //CONFIG_NAND_SPL end
#endif  //DOLPHIN_UBOOT END



//add one macro instruction to nand controller
/*static */void sprd_dolphin_nand_ins_init(struct sprd_dolphin_nand_info *dolphin)
{
	dolphin->ins_num = 0;
}
/*static */void sprd_dolphin_nand_ins_add(uint16_t ins, struct sprd_dolphin_nand_info *dolphin)
{
	uint16_t *buf = (uint16_t *)dolphin->ins;
	if(dolphin->ins_num >= NAND_MC_BUFFER_SIZE)
	{
		while(1);
	}
	*(buf + dolphin->ins_num) = ins;
	dolphin->ins_num++;
}

static void sprd_dolphin_nand_ins_exec(struct sprd_dolphin_nand_info *dolphin)
{
	uint32_t i;
	uint32_t cfg0;
	
	for(i = 0; i < ((dolphin->ins_num + 1) >> 1); i++)
	{
		sprd_dolphin_reg_write(NFC_INST0_REG + (i << 2), dolphin->ins[i]);
	}
	cfg0 = sprd_dolphin_reg_read(NFC_CFG0_REG);
	if(dolphin->wp_en)
	{
		cfg0 &= ~NFC_WPN;
	}
	else
	{
		cfg0 |= NFC_WPN;
	}
	if(dolphin->chip)
	{
		cfg0 |= CS_SEL;
	}
	else
	{
		cfg0 &= ~CS_SEL;
	}
	sprd_dolphin_nand_int_clr(INT_STSMCH_CLR | INT_WP_CLR | INT_TO_CLR | INT_DONE_CLR);//clear all interrupt
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_START_REG, NFC_START);
}
static int sprd_dolphin_nand_wait_finish(struct sprd_dolphin_nand_info *dolphin)
{
	unsigned int value;
	unsigned int counter = 0;
	while((counter < NFC_TIMEOUT_VAL/*time out*/))
	{
		value = sprd_dolphin_reg_read(NFC_INT_REG);
		if(value & INT_DONE_RAW)
		{
			break;
		}
		counter ++;
	}
	sprd_dolphin_reg_write(NFC_INT_REG, 0xf00); //clear all interrupt status
	if(counter > NFC_TIMEOUT_VAL)
	{
		while (1);
		return -1;
	}
	return 0;
}
static void sprd_dolphin_nand_wp_en(struct sprd_dolphin_nand_info *dolphin, int en)
{
	if(en)
	{
		dolphin->wp_en = 1;
	}
	else
	{
		dolphin->wp_en = 0;
	}
}
static void sprd_dolphin_select_chip(struct mtd_info *mtd, int chip)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	if(chip < 0) { //for release caller
		return;
	}
	//DPRINT("sprd_dolphin_select_chip, %x\r\n", chip);
	dolphin->chip = chip;
#ifdef CONFIG_NAND_SPL
	nand_hardware_config(mtd,dolphin->nand);
#endif
}
static void sprd_dolphin_nand_read_status(struct sprd_dolphin_nand_info *dolphin)
{
	uint32_t *buf;
	//DPRINT("sprd_dolphin_nand_read_status\r\n");
	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_STATUS), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_NOP(10), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_IDST(1), dolphin);
	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	sprd_dolphin_reg_write(NFC_CFG0_REG, NFC_ONLY_NAND_MODE);
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	buf = (uint32_t *)s_oob_data;
	*buf = sprd_dolphin_reg_read(NFC_STATUS0_REG);
	dolphin->buf_head = 0;
	dolphin->_buf_tail = 1;
	//DPRINT("--sprd_dolphin_nand_read_status--\r\n");
}
static void sprd_dolphin_nand_read_id(struct sprd_dolphin_nand_info *dolphin, uint32_t *buf)
{
	//DPRINT("sprd_dolphin_nand_read_id\r\n");
	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READID), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(0), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_NOP(10), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_IDST(8), dolphin);
	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	
	sprd_dolphin_reg_write(NFC_CFG0_REG, NFC_ONLY_NAND_MODE);
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	*buf = sprd_dolphin_reg_read(NFC_STATUS0_REG);
	*(buf + 1) = sprd_dolphin_reg_read(NFC_STATUS1_REG);
	dolphin->buf_head = 0;
	dolphin->_buf_tail = 8;
	//DPRINT("--sprd_dolphin_nand_read_id--\r\n");
}
static void sprd_dolphin_nand_reset(struct sprd_dolphin_nand_info *dolphin)
{
	//DPRINT("sprd_dolphin_nand_reset\r\n");
	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_RESET), dolphin);
	sprd_dolphin_nand_ins_add(NFC_MC_WRB0_ID, dolphin); //wait rb
	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	//config register
	sprd_dolphin_reg_write(NFC_CFG0_REG, NFC_ONLY_NAND_MODE);
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	//DPRINT("--sprd_dolphin_nand_reset--\r\n");
}
static u32 sprd_dolphin_get_decode_sts(u32 index)
{
	uint32_t err;
	uint32_t shift;
	uint32_t reg_addr;
	reg_addr = NFC_STATUS0_REG + (index & 0xfffffffc);
	shift = (index & 0x3) << 3;
	err = sprd_dolphin_reg_read(reg_addr);
	err >>= shift;
	if((err & ECC_ALL_FF))
	{
		err &= ERR_ERR_NUM0_MASK;
	}
	else
	{
		err = 0;
	}
	return err;
}


#ifdef DOLPHIN_UBOOT
//read large page
static int sprd_dolphin_nand_read_lp(struct mtd_info *mtd,uint8_t *mbuf, uint8_t *sbuf,uint32_t raw)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	struct nand_chip *chip = dolphin->nand;
	uint32_t column;
	uint32_t page_addr;
	uint32_t cfg0;
	uint32_t cfg1;
	uint32_t cfg2;
	uint32_t i;
	uint32_t err;
	page_addr = dolphin->page;
	
	if(sbuf) {
		column = mtd->writesize;
	}
	else
	{
		column = 0;
	}
	if(chip->options & NAND_BUSWIDTH_16)
	{
		column >>= 1;
	}
	//DPRINT("sprd_dolphin_nand_read_lp,page_addr = %x,column = %x\r\n",page_addr, column);
	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READ0), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);
	column >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);

	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	page_addr >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	
	if (5 == dolphin->a_cycles)// five address cycles
	{
		page_addr >>= 8;
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	}
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READSTART), dolphin);
	
	sprd_dolphin_nand_ins_add(NFC_MC_WRB0_ID, dolphin); //wait rb
	if(mbuf && sbuf)
	{
		sprd_dolphin_nand_ins_add(NAND_MC_SRDT, dolphin);
		//switch to main part
		sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_RNDOUT), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(0), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(0), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_RNDOUTSTART), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_MRDT, dolphin);
	}
	else
	{
		sprd_dolphin_nand_ins_add(NAND_MC_MRDT, dolphin);
	}
	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	//config registers 
	cfg0 = NFC_AUTO_MODE | MAIN_SPAR_APT | ((dolphin->sct_pg - 1)<< SECTOR_NUM_OFFSET);
	if((!raw) && mbuf && sbuf)
	{
		cfg0 |= ECC_EN | DETECT_ALL_FF;
	}
	if(chip->options & NAND_BUSWIDTH_16)
	{
		cfg0 |= BUS_WIDTH;
	}
	cfg1 = (dolphin->info_size) << SPAR_INFO_SIZE_OFFSET;
	cfg2 = (dolphin->ecc_mode << 12) | (dolphin->info_pos << SPAR_INFO_POS_OFFSET) | ((dolphin->sct_pg - 1) << SPAR_SECTOR_NUM_OFFSET) | dolphin->ecc_pos;

#ifndef CONFIG_NAND_SPL
	if (mbuf)
	{
		Dcache_CleanRegion((unsigned int)mbuf, dolphin->m_size*dolphin->sct_pg);
		Dcache_InvalRegion((unsigned int)mbuf, dolphin->m_size*dolphin->sct_pg);
	}

	if (sbuf)
	{
		Dcache_CleanRegion((unsigned int)sbuf, dolphin->s_size*dolphin->sct_pg);
		Dcache_InvalRegion((unsigned int)sbuf, dolphin->s_size*dolphin->sct_pg);
	}
#endif

	if(mbuf && sbuf)
	{
		cfg1 |= (dolphin->m_size - 1) | ((dolphin->s_size  - 1)<< SPAR_SIZE_OFFSET);
		sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)mbuf);
		sprd_dolphin_reg_write(NFC_SPAR_ADDR_REG, (uint32_t)sbuf);
		cfg0 |= MAIN_USE | SPAR_USE;
	}
	else
	{
		if(mbuf)
		{
			cfg1 |= (dolphin->m_size - 1);
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)mbuf);
		}
		if(sbuf)
		{
			cfg1 |= (dolphin->s_size - 1);
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)sbuf);
		}
		cfg0 |= MAIN_USE;
	}	
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_CFG1_REG, cfg1);
	sprd_dolphin_reg_write(NFC_CFG2_REG, cfg2);
	
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	if(!raw) {
		for(i = 0; i < dolphin->sct_pg; i++) {
			err = sprd_dolphin_get_decode_sts(i);
			if(err == ERR_ERR_NUM0_MASK) {
				mtd->ecc_stats.failed++;
			}
			else {
				mtd->ecc_stats.corrected += err;
			}
		}
	}

	return 0;
}


static int sprd_dolphin_nand_write_lp(struct mtd_info *mtd,const uint8_t *mbuf, uint8_t *sbuf,uint32_t raw)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	struct nand_chip *chip = dolphin->nand;
	uint32_t column;
	uint32_t page_addr;
	uint32_t cfg0;
	uint32_t cfg1;
	uint32_t cfg2;
	page_addr = dolphin->page;
	if(mbuf) {
		column = 0;
	}
	else {
		column = mtd->writesize;
	}	
	if(chip->options & NAND_BUSWIDTH_16)
	{
		column >>= 1;
	}

	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_SEQIN), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);
	column >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	page_addr >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	
	if (5 == dolphin->a_cycles)// five address cycles
	{
		page_addr >>= 8;
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	}
	
	sprd_dolphin_nand_ins_add(NAND_MC_MWDT, dolphin);
	if(mbuf && sbuf)
	{
		sprd_dolphin_nand_ins_add(NAND_MC_SWDT, dolphin);
	}
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_PAGEPROG), dolphin);
	sprd_dolphin_nand_ins_add(NFC_MC_WRB0_ID, dolphin); //wait rb

	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	//config registers 
	cfg0 = NFC_AUTO_MODE | NFC_RW |  NFC_WPN | MAIN_SPAR_APT | ((dolphin->sct_pg - 1)<< SECTOR_NUM_OFFSET);
	if((!raw) && mbuf && sbuf)
	{
		cfg0 |= ECC_EN;
	}
	if(chip->options & NAND_BUSWIDTH_16)
	{
		cfg0 |= BUS_WIDTH;
	}
	cfg1 = ((dolphin->info_size) << SPAR_INFO_SIZE_OFFSET);
	cfg2 = (dolphin->ecc_mode << 12) | (dolphin->info_pos << SPAR_INFO_POS_OFFSET) | ((dolphin->sct_pg - 1) << SPAR_SECTOR_NUM_OFFSET) | dolphin->ecc_pos;

#ifndef CONFIG_NAND_SPL
	if (mbuf)
	{
		Dcache_CleanRegion((unsigned int)mbuf, dolphin->m_size*dolphin->sct_pg);
		Dcache_InvalRegion((unsigned int)mbuf, dolphin->m_size*dolphin->sct_pg);
	}

	if (sbuf)
	{
		Dcache_CleanRegion((unsigned int)sbuf, dolphin->s_size*dolphin->sct_pg);
		Dcache_InvalRegion((unsigned int)sbuf, dolphin->s_size*dolphin->sct_pg);
	}
#endif

	if(mbuf && sbuf)
	{
		cfg0 |= MAIN_USE | SPAR_USE;
		cfg1 |= (dolphin->m_size - 1) | ((dolphin->s_size - 1) << SPAR_SIZE_OFFSET);
		sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)mbuf);
		sprd_dolphin_reg_write(NFC_SPAR_ADDR_REG, (uint32_t)sbuf);
	}
	else
	{
		cfg0 |= MAIN_USE;
		if(mbuf)
		{
			cfg1 |= dolphin->m_size - 1;
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)mbuf);
		}
		else
		{
			cfg1 |= dolphin->s_size - 1;
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, (uint32_t)sbuf);
		}
	}
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_CFG1_REG, cfg1);
	sprd_dolphin_reg_write(NFC_CFG2_REG, cfg2);
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	return 0;
}
#endif



#ifdef DOLPHIN_KERNEL
//read large page
static int sprd_dolphin_nand_read_lp(struct mtd_info *mtd,uint8_t *mbuf, uint8_t *sbuf,uint32_t raw)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	struct nand_chip *chip = dolphin->nand;
	uint32_t column;
	uint32_t page_addr;
	uint32_t cfg0;
	uint32_t cfg1;
	uint32_t cfg2;
	uint32_t i;
	uint32_t err;
	page_addr = dolphin->page;
	
	if(sbuf) {
		column = mtd->writesize;
	}
	else
	{
		column = 0;
	}
	if(chip->options & NAND_BUSWIDTH_16)
	{
		column >>= 1;
	}
	//DPRINT("sprd_dolphin_nand_read_lp,page_addr = %x,column = %x\r\n",page_addr, column);
	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READ0), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);
	column >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);

	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	page_addr >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	
	if (5 == dolphin->a_cycles)// five address cycles
	{
		page_addr >>= 8;
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	}
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READSTART), dolphin);
	
	sprd_dolphin_nand_ins_add(NFC_MC_WRB0_ID, dolphin); //wait rb
	if(mbuf && sbuf)
	{
		sprd_dolphin_nand_ins_add(NAND_MC_SRDT, dolphin);
		//switch to main part
		sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_RNDOUT), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(0), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(0), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_RNDOUTSTART), dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_MRDT, dolphin);
	}
	else
	{
		sprd_dolphin_nand_ins_add(NAND_MC_MRDT, dolphin);
	}
	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	//config registers 
	cfg0 = NFC_AUTO_MODE | MAIN_SPAR_APT | ((dolphin->sct_pg - 1)<< SECTOR_NUM_OFFSET);
	if((!raw) && mbuf && sbuf)
	{
		cfg0 |= ECC_EN | DETECT_ALL_FF;
	}
	if(chip->options & NAND_BUSWIDTH_16)
	{
		cfg0 |= BUS_WIDTH;
	}
	cfg1 = (dolphin->info_size) << SPAR_INFO_SIZE_OFFSET;
	cfg2 = (dolphin->ecc_mode << 12) | (dolphin->info_pos << SPAR_INFO_POS_OFFSET) | ((dolphin->sct_pg - 1) << SPAR_SECTOR_NUM_OFFSET) | dolphin->ecc_pos;

	if(mbuf && sbuf)
	{
		cfg1 |= (dolphin->m_size - 1) | ((dolphin->s_size  - 1)<< SPAR_SIZE_OFFSET);
		sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, dolphin->p_mbuf);
		sprd_dolphin_reg_write(NFC_SPAR_ADDR_REG, dolphin->p_oob);
		cfg0 |= MAIN_USE | SPAR_USE;
	}
	else
	{
		if(mbuf)
		{
			cfg1 |= (dolphin->m_size - 1);
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, dolphin->p_mbuf);
		}
		if(sbuf)
		{
			cfg1 |= (dolphin->s_size - 1);
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, dolphin->p_oob);
		}
		cfg0 |= MAIN_USE;
	}	
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_CFG1_REG, cfg1);
	sprd_dolphin_reg_write(NFC_CFG2_REG, cfg2);
	
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	if(!raw) {
		for(i = 0; i < dolphin->sct_pg; i++) {
			err = sprd_dolphin_get_decode_sts(i);
			if(err == ERR_ERR_NUM0_MASK) {
				mtd->ecc_stats.failed++;
			}
			else {
				mtd->ecc_stats.corrected += err;
			}
		}
	}

	if(mbuf) {
		memcpy(mbuf, (const void *)dolphin->v_mbuf, dolphin->write_size);
	}
	if(sbuf) {
		memcpy(sbuf, (const void *)dolphin->v_oob, dolphin->oob_size);
	}

	return 0;
}



static int sprd_dolphin_nand_write_lp(struct mtd_info *mtd,const uint8_t *mbuf, uint8_t *sbuf,uint32_t raw)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	struct nand_chip *chip = dolphin->nand;
	uint32_t column;
	uint32_t page_addr;
	uint32_t cfg0;
	uint32_t cfg1;
	uint32_t cfg2;
	page_addr = dolphin->page;
	if(mbuf) {
		column = 0;
	}
	else {
		column = mtd->writesize;
	}	
	if(chip->options & NAND_BUSWIDTH_16)
	{
		column >>= 1;
	}
	
	if(mbuf) {
		memcpy((void *)dolphin->v_mbuf, (const void *)mbuf, dolphin->write_size);
	}
	if(sbuf) {
		memcpy((void *)dolphin->v_oob, (const void *)sbuf, dolphin->oob_size);
	}

	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_SEQIN), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);
	column >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(column & 0xff), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	page_addr >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	
	if (5 == dolphin->a_cycles)// five address cycles
	{
		page_addr >>= 8;
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	}
	
	sprd_dolphin_nand_ins_add(NAND_MC_MWDT, dolphin);
	if(mbuf && sbuf)
	{
		sprd_dolphin_nand_ins_add(NAND_MC_SWDT, dolphin);
	}
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_PAGEPROG), dolphin);
	sprd_dolphin_nand_ins_add(NFC_MC_WRB0_ID, dolphin); //wait rb

	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	//config registers 
	cfg0 = NFC_AUTO_MODE | NFC_RW |  NFC_WPN | MAIN_SPAR_APT | ((dolphin->sct_pg - 1)<< SECTOR_NUM_OFFSET);
	if((!raw) && mbuf && sbuf)
	{
		cfg0 |= ECC_EN;
	}
	if(chip->options & NAND_BUSWIDTH_16)
	{
		cfg0 |= BUS_WIDTH;
	}
	cfg1 = ((dolphin->info_size) << SPAR_INFO_SIZE_OFFSET);
	cfg2 = (dolphin->ecc_mode << 12) | (dolphin->info_pos << SPAR_INFO_POS_OFFSET) | ((dolphin->sct_pg - 1) << SPAR_SECTOR_NUM_OFFSET) | dolphin->ecc_pos;

	if(mbuf && sbuf)
	{
		cfg0 |= MAIN_USE | SPAR_USE;
		cfg1 |= (dolphin->m_size - 1) | ((dolphin->s_size - 1) << SPAR_SIZE_OFFSET);
		sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, dolphin->p_mbuf);
		sprd_dolphin_reg_write(NFC_SPAR_ADDR_REG, dolphin->p_oob);
	}
	else
	{
		cfg0 |= MAIN_USE;
		if(mbuf)
		{
			cfg1 |= dolphin->m_size - 1;
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, dolphin->p_mbuf);
		}
		else
		{
			cfg1 |= dolphin->s_size - 1;
			sprd_dolphin_reg_write(NFC_MAIN_ADDR_REG, dolphin->p_oob);
		}
	}
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_reg_write(NFC_CFG1_REG, cfg1);
	sprd_dolphin_reg_write(NFC_CFG2_REG, cfg2);
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	return 0;
}
#endif



static int sprd_dolphin_nand_read_sp(struct mtd_info *mtd,uint8_t *mbuf, uint8_t *sbuf,uint32_t raw)
{
	return 0;
}
static int sprd_dolphin_nand_write_sp(struct mtd_info *mtd,const uint8_t *mbuf, uint8_t *sbuf,uint32_t raw)
{
	return 0;
}
static void sprd_dolphin_erase(struct mtd_info *mtd, int page_addr)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	uint32_t cfg0 = 0;
	//DPRINT("sprd_dolphin_erase, %x\r\n", page_addr);
	sprd_dolphin_nand_ins_init(dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_ERASE1), dolphin);
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	page_addr >>= 8;
	sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	if((5 == dolphin->a_cycles) || ((4 == dolphin->a_cycles) && (512 == dolphin->write_size)))
	{
		page_addr >>= 8;
		sprd_dolphin_nand_ins_add(NAND_MC_ADDR(page_addr & 0xff), dolphin);
	}
	sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_ERASE2), dolphin);
	sprd_dolphin_nand_ins_add(NFC_MC_WRB0_ID, dolphin); //wait rb

	sprd_dolphin_nand_ins_add(NFC_MC_DONE_ID, dolphin);
	cfg0 = NFC_WPN | NFC_ONLY_NAND_MODE;
	sprd_dolphin_reg_write(NFC_CFG0_REG, cfg0);
	sprd_dolphin_nand_ins_exec(dolphin);
	sprd_dolphin_nand_wait_finish(dolphin);
	//DPRINT("--sprd_dolphin_erase--\r\n");
}
static uint8_t sprd_dolphin_read_byte(struct mtd_info *mtd)
{
	uint8_t ch = 0xff;
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	if(dolphin->buf_head < dolphin->_buf_tail)
	{
		ch = s_oob_data[dolphin->buf_head ++];
	}
	return ch;
}
static uint16_t sprd_dolphin_read_word(struct mtd_info *mtd)
{
	uint16_t data = 0xffff;
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	if(dolphin->buf_head < (dolphin->_buf_tail - 1))
	{
		data = s_oob_data[dolphin->buf_head ++];
		data |= ((uint16_t)s_oob_data[dolphin->buf_head ++]) << 8;
	}
	return data;
}

static int sprd_dolphin_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
	return 0;
}

static int sprd_dolphin_ecc_calculate(struct mtd_info *mtd, const uint8_t *data,
				uint8_t *ecc_code)
{
	return 0;
}

static int sprd_dolphin_ecc_correct(struct mtd_info *mtd, uint8_t *data,
				uint8_t *read_ecc, uint8_t *calc_ecc)
{
	return 0;
}

static int sprd_dolphin_read_page(struct mtd_info *mtd, struct nand_chip *chip,
			uint8_t *buf, int page)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	dolphin->page = page;
	if(512 == mtd->writesize)
	{
		sprd_dolphin_nand_read_sp(mtd, buf, chip->oob_poi, 0);
	}
	else
	{
		sprd_dolphin_nand_read_lp(mtd, buf, chip->oob_poi, 0);
	}
	return 0;
}
static int sprd_dolphin_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int page)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	dolphin->page = page;
	if(512 == mtd->writesize)
	{
		sprd_dolphin_nand_read_sp(mtd, buf, chip->oob_poi, 1);
	}
	else
	{
		sprd_dolphin_nand_read_lp(mtd, buf, chip->oob_poi, 1);
	}
	return 0;
}
static int sprd_dolphin_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			   int page, int sndcmd)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	dolphin->page = page;
	if(512 == mtd->writesize)
	{
		sprd_dolphin_nand_read_sp(mtd, 0, chip->oob_poi, 1);
	}
	else
	{
		sprd_dolphin_nand_read_lp(mtd, 0, chip->oob_poi, 1);
	}
	return 0;
}
static void sprd_dolphin_write_page(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf)
{
	if(512 == mtd->writesize)
	{
		sprd_dolphin_nand_write_sp(mtd, buf, chip->oob_poi, 0);	
	}
	else
	{
		sprd_dolphin_nand_write_lp(mtd, buf, chip->oob_poi, 0);	
	}

}
static void sprd_dolphin_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
					const uint8_t *buf)
{
	if(512 == mtd->writesize)
	{
		sprd_dolphin_nand_write_sp(mtd, buf, chip->oob_poi, 1);	
	}
	else
	{
		sprd_dolphin_nand_write_lp(mtd, buf, chip->oob_poi, 1);	
	}
}
static int sprd_dolphin_write_oob(struct mtd_info *mtd, struct nand_chip *chip,
				int page)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);	
	dolphin->page = page;
	if(512 == mtd->writesize)
	{
		sprd_dolphin_nand_write_sp(mtd, 0, chip->oob_poi, 1);
	}
	else
	{
		sprd_dolphin_nand_write_lp(mtd, 0, chip->oob_poi, 1);
	}
	return 0;
}


/**
 * nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 *
 * Check, if the block is bad.
 */
static int sprd_dolphin_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	int page, chipnr, res = 0;
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	struct nand_chip *chip = mtd->priv;
	uint16_t bad;
	uint16_t *buf;

	page = (int)((long)ofs >> chip->page_shift) & chip->pagemask;

	if (getchip) {
		chipnr = (int)((long)ofs >> chip->chip_shift);
		/* Select the NAND device */
		chip->select_chip(mtd, chipnr);
	}

	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	if(512 == dolphin->write_size) {
		sprd_dolphin_nand_read_sp(mtd, 0, s_oob_data, 1);
	}
	else  {
		sprd_dolphin_nand_read_lp(mtd, 0, s_oob_data, 1);
	}
	dolphin->buf_head = 0;
	dolphin->_buf_tail = mtd->oobsize;
	buf = (uint16_t *)(s_oob_data + chip->badblockpos);

	if (chip->options & NAND_BUSWIDTH_16) {
		bad = *(buf);
		if ((bad & 0xFF) != 0xff) {
			res = 1;
		}
	} else {
		bad = *(buf) & 0xff;
		if (bad != 0xff){
			res = 1;
		}
	}
	return res;
}

static void sprd_dolphin_nand_cmdfunc(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	struct sprd_dolphin_nand_info *dolphin = mtd_to_dolphin(mtd);
	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}
	/*
	 * program and erase have their own busy handlers
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {
	case NAND_CMD_STATUS:
		sprd_dolphin_nand_read_status(dolphin);
		break;
	case NAND_CMD_READID:
		sprd_dolphin_nand_read_id(dolphin, (uint32_t *)s_oob_data);
		break;
	case NAND_CMD_RESET:
		sprd_dolphin_nand_reset(dolphin);
		break;
	case NAND_CMD_ERASE1:
		sprd_dolphin_erase(mtd, page_addr);
		break;
	case NAND_CMD_READ0:
	case NAND_CMD_SEQIN:
		dolphin->column = column;
		dolphin->page = page_addr;
	default:
		break;
	}
}
static void sprd_dolphin_nand_hwecc_ctl(struct mtd_info *mtd, int mode)
{
	return; //do nothing
}



static void dolphin_set_timing_config(struct sprd_dolphin_nand_info *dolphin , uint32_t nfc_clk_MHz) {
	int index, array;
	u8 id_buf[8];
	u32 reg_val, temp_val;
	struct sprd_dolphin_nand_param * param;

	/* read id */
	sprd_dolphin_nand_read_id(dolphin, (uint32_t *)id_buf);

	/* get timing para */
	array = ARRAY_SIZE(sprd_dolphin_nand_param_tb);
	for (index = 0; index < array; index ++) {
		param = sprd_dolphin_nand_param_tb + index;
		if ((param->id[0] == id_buf[0])
			&& (param->id[1] == id_buf[1])
			&& (param->id[2] == id_buf[2])
			&& (param->id[3] == id_buf[3])
			&& (param->id[4] == id_buf[4]))
			break;
	}

	if (index < array) {
		reg_val = 0;

		/* get acs value : 0ns */
		reg_val |= ((2 & 0x1F) << NFC_ACS_OFFSET);

		/* get ace value + 6ns read delay time, and rwl added */
		temp_val = (param->ace_ns + 6) * nfc_clk_MHz / 1000;
		if (((param->ace_ns * nfc_clk_MHz) % 1000)  != 0) {
			temp_val++;
		}
		reg_val |= ((temp_val & 0x1F) << NFC_ACE_OFFSET);

		/* get rws value : 20 ns */
		temp_val = 20 * nfc_clk_MHz / 1000;
		if (((param->ace_ns * nfc_clk_MHz) % 1000)  != 0) {
			temp_val++;
		}
		reg_val |= ((temp_val & 0x3F) << NFC_RWS_OFFSET);

		/* get rws value : 0 ns */
		reg_val |= ((2 & 0x1F) << NFC_RWE_OFFSET);

		/* get rwh value */
		temp_val = (param->rwh_ns + 6) * nfc_clk_MHz / 1000;
		if (((param->ace_ns * nfc_clk_MHz) % 1000)  != 0) {
			temp_val++;
		}
		reg_val |= ((temp_val & 0x1F) << NFC_RWH_OFFSET);

		/* get rwl value, 6 is read delay time*/
		temp_val = (param->rwl_ns + 6) * nfc_clk_MHz / 1000;
		if (((param->ace_ns * nfc_clk_MHz) % 1000)  != 0) {
			temp_val++;
		}
		reg_val |= (temp_val & 0x3F);

		DPRINT("nand timing val: 0x%x\n\r", reg_val);

		sprd_dolphin_reg_write(NFC_TIMING_REG, reg_val);
	}
}


#ifdef DOLPHIN_UBOOT
#define DOLPHIN_AHB_BASE SPRD_AHB_PHYS
#define DOLPHIN_PIN_BASE SPRD_PIN_PHYS
#define DOLPHIN_AHB_RST  (DOLPHIN_AHB_BASE + 0x0004)
#define DOLPHIN_NANC_CLK_CFG  (DOLPHIN_AHB_BASE + 0x0060)

#define DOLPHIN_ADISLAVE_BASE	 	SPRD_ADISLAVE_PHYS
#define DOLPHIN_ANA_CTL_GLB_BASE		(DOLPHIN_ADISLAVE_BASE + 0x8800)

#define DOLPHIN_NFC_REG_BASE  SPRD_NFC_PHYS
#define DOLPHIN_NFC_TIMING_REG  (DOLPHIN_NFC_REG_BASE + 0x14)
#define DOLPHIN_NFC_TIMEOUT_REG  (DOLPHIN_NFC_REG_BASE + 0x34)
#endif

#ifdef DOLPHIN_KERNEL
#define DOLPHIN_AHB_BASE SPRD_AHB_BASE
#define DOLPHIN_PIN_BASE SPRD_PIN_BASE
#define DOLPHIN_AHB_RST  (DOLPHIN_AHB_BASE + 0x0004)
#define DOLPHIN_NANC_CLK_CFG  (DOLPHIN_AHB_BASE + 0x0060)

#define DOLPHIN_ADISLAVE_BASE	 	SPRD_ADISLAVE_BASE
#define DOLPHIN_ANA_CTL_GLB_BASE		(DOLPHIN_ADISLAVE_BASE + 0x8800)

#define DOLPHIN_NFC_REG_BASE  SPRD_NFC_BASE
#define DOLPHIN_NFC_TIMING_REG  (DOLPHIN_NFC_REG_BASE + 0x14)
#define DOLPHIN_NFC_TIMEOUT_REG  (DOLPHIN_NFC_REG_BASE + 0x34)
#endif


static void sprd_dolphin_nand_hw_init(struct sprd_dolphin_nand_info *dolphin)
{
	int i = 0;
	uint32_t val;

	//sprd_dolphin_reg_and(DOLPHIN_NANC_CLK_CFG, ~(BIT(1) | BIT(0)));
	//sprd_dolphin_reg_or(DOLPHIN_NANC_CLK_CFG, BIT(0));

	sprd_dolphin_reg_or(DOLPHIN_AHB_BASE,BIT(19) | BIT(18) | BIT(17) | BIT(6));

	sprd_dolphin_reg_or(DOLPHIN_AHB_RST,BIT(9));
	mdelay(1);
	sprd_dolphin_reg_and(DOLPHIN_AHB_RST, ~(BIT(9)));

	val = (3)  | (4 << NFC_RWH_OFFSET) | (3 << NFC_RWE_OFFSET) | (3 << NFC_RWS_OFFSET) | (3 << NFC_ACE_OFFSET) | (3 << NFC_ACS_OFFSET);
	sprd_dolphin_reg_write(DOLPHIN_NFC_TIMING_REG, val);
	sprd_dolphin_reg_write(DOLPHIN_NFC_TIMEOUT_REG, 0xffffffff);
	//sprd_dolphin_reg_write(DOLPHIN_NFC_REG_BASE + 0x118, 3);

#if 0
	sprd_tiger_reg_or(REG_PIN_NFRB, BIT_7);   //set PIN_NFRB pull up
	for (i=REG_PIN_NFWPN; i<=REG_PIN_NFD15; i+=4)
	{
		sprd_tiger_reg_or( i, BIT_9);
		sprd_tiger_reg_and(i, ~(BIT_8|BIT_10|BIT_6));
	}
#endif

#if 0
	sprd_dolphin_reg_or(DOLPHIN_PIN_BASE + REG_PIN_NFRB, BIT(7));
	for (i = REG_PIN_NFWPN; i <= REG_PIN_NFD15; i += 4)
	{
		sprd_dolphin_reg_or(DOLPHIN_PIN_BASE + i, BIT(9));
		sprd_dolphin_reg_and(DOLPHIN_PIN_BASE + i, ~(BIT(10) | BIT(8) | BIT(6)));
		sprd_dolphin_reg_and(DOLPHIN_PIN_BASE + i, ~(BIT(4) | BIT(5)));
	}

#endif

#if 0
	sprd_dolphin_reg_or(DOLPHIN_PIN_BASE + REG_PIN_NFWPN, BIT(7) | BIT(8) | BIT(9));
	sprd_dolphin_reg_and(DOLPHIN_PIN_BASE + REG_PIN_NFWPN, ~(BIT(4) | BIT(5)));
	sprd_dolphin_reg_or(DOLPHIN_PIN_BASE + REG_PIN_NFRB, BIT(7) | BIT(8) | BIT(9));
	sprd_dolphin_reg_and(DOLPHIN_PIN_BASE + REG_PIN_NFRB, ~(BIT(4) | BIT(5)));
	for(i = 0; i < 22; ++i)
	{
		sprd_dolphin_reg_or(DOLPHIN_PIN_BASE + REG_PIN_NFCLE + (i << 2), BIT(7) | BIT(8) | BIT(9));
		sprd_dolphin_reg_and(DOLPHIN_PIN_BASE + REG_PIN_NFCLE + (i << 2), ~(BIT(4) | BIT(5)));
	}
#endif

#if 0
	i = sprd_dolphin_reg_read(DOLPHIN_ANA_CTL_GLB_BASE+0x3c);
	i &= ~0x7F00;
	i |= 0x38 << 8;
	sprd_dolphin_reg_write(DOLPHIN_ANA_CTL_GLB_BASE + 0x3c, i);

	i = sprd_dolphin_reg_read(DOLPHIN_ANA_CTL_GLB_BASE+0x20);
	i &= ~0xFF;
	i |= 0x38 << 0;
	sprd_dolphin_reg_write(DOLPHIN_ANA_CTL_GLB_BASE + 0x20, i);
#endif
	
	//close write protect
	sprd_dolphin_nand_wp_en(dolphin, 0);
}


int board_nand_init(struct nand_chip *chip)
{
	//DPRINT("board_nand_init\r\n");

	sprd_dolphin_nand_hw_init(&g_dolphin);

	chip->select_chip = sprd_dolphin_select_chip;
	chip->cmdfunc = sprd_dolphin_nand_cmdfunc;
	chip->read_byte = sprd_dolphin_read_byte;
	chip->read_word	= sprd_dolphin_read_word;
	chip->waitfunc = sprd_dolphin_waitfunc;
	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.calculate = sprd_dolphin_ecc_calculate;
	chip->ecc.hwctl = sprd_dolphin_nand_hwecc_ctl;
	
	chip->ecc.correct = sprd_dolphin_ecc_correct;
	chip->ecc.read_page = sprd_dolphin_read_page;
	chip->ecc.read_page_raw = sprd_dolphin_read_page_raw;
	chip->ecc.write_page = sprd_dolphin_write_page;
	chip->ecc.write_page_raw = sprd_dolphin_write_page_raw;
	chip->ecc.read_oob = sprd_dolphin_read_oob;
	chip->ecc.write_oob = sprd_dolphin_write_oob;
	chip->erase_cmd = sprd_dolphin_erase;
	
	chip->ecc.bytes = CONFIG_SYS_NAND_ECCBYTES;
	g_dolphin.ecc_mode = ecc_mode_convert(CONFIG_SYS_NAND_ECC_MODE);
	g_dolphin.nand = chip;

	dolphin_set_timing_config(&g_dolphin, 64);	/* 153 is current clock 153MHz */

	chip->eccbitmode = CONFIG_SYS_NAND_ECC_MODE;
	chip->ecc.size = CONFIG_SYS_NAND_ECCSIZE;

	chip->chip_delay = 20;
	chip->priv = &g_dolphin;

	//printf("v2   board eccbitmode %d\n", chip->eccbitmode);

	chip->options = NAND_BUSWIDTH_16;

	return 0;
}

#ifdef DOLPHIN_UBOOT
#ifndef CONFIG_NAND_SPL
void McuReadNandType(unsigned char *array)
{

}
#endif

static unsigned long nfc_read_status(void)
{
	unsigned long status = 0;

	sprd_dolphin_nand_read_status(&g_dolphin);	
	status = s_oob_data[0];

	return status;
}

#ifndef CONFIG_NAND_SPL
static int sprd_scan_one_block(int blk, int erasesize, int writesize)
{
	int i, cmd;
	int status = 1, ii;
	u32 size = 0;
	int oobsize = mtdoobsize;
	int column, page_addr;

	page_addr = blk * (erasesize / writesize);
	for (ii = 0; ii < 2; ii ++) {
		DPRINT("please debug here : %s %d\n", __FUNCTION__, __LINE__);
		sprd_dolphin_nand_ins_init(&g_dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READ0), &g_dolphin);
		sprd_dolphin_nand_ins_add(NAND_MC_CMD(NAND_CMD_READSTART), &g_dolphin);
		if ((s_oob_data[0] != 0xff) || (s_oob_data[1] != 0xff))
			break;
	} //for (ii = 0; ii < 2; ii ++)

	if ((s_oob_data[0] == 0xff) && (s_oob_data[1] == 0xff))
		status = 0; //good block
	else
		status = 1; //bad block

	return status;
}

static unsigned long nand_ctl_erase_block(int blk, int erasesize, int writesize)
{
	int cmd, status;
	int page_addr;

	page_addr = blk * (erasesize / writesize);
	sprd_dolphin_erase(&g_dolphin, page_addr);
	status = nfc_read_status();

	return status;
}
#endif


#ifndef CONFIG_NAND_SPL
void nand_scan_patition(int blocks, int erasesize, int writesize)
{
	int blk;
	int ret;
	int status;

	//read_chip_id();
	for (blk = 0; blk < blocks; blk ++) {
		ret = sprd_scan_one_block(blk, erasesize, writesize);
		if (ret != 0) {
			DPRINT("\n%d is bad, scrub to erase it, ", blk);
			ret = nand_ctl_erase_block(blk, erasesize, writesize);
			DPRINT("0x%02x\n", ret);
		} else {
			ret = nand_ctl_erase_block(blk, erasesize, writesize);
			DPRINT("erasing block : %d    %d % \r", blk, (blk * 100 ) / blocks);
		}
	}
}
int nand_scan_block(int block, int erasesize, int writesize){
	int ret = 0;
	ret = nand_ctl_erase_block(block, erasesize, writesize);
	ret = ret&1;

	return ret;
}
#endif
#endif  //DOLPHIN_UBOOT end



#ifdef DOLPHIN_KERNEL
extern int parse_mtd_partitions(struct mtd_info *master, const char **types,
				struct mtd_partition **pparts,
				struct mtd_part_parser_data *data);

static struct mtd_info *sprd_mtd = NULL;
#ifdef CONFIG_MTD_CMDLINE_PARTS
const char *part_probes[] = { "cmdlinepart", NULL };
#endif

static int sprd_nand_dma_init(struct sprd_dolphin_nand_info *dolphin)
{
	dma_addr_t phys_addr = 0;
	void *virt_ptr = 0;
	virt_ptr = dma_alloc_coherent(NULL, dolphin->write_size, &phys_addr, GFP_KERNEL);
	if (virt_ptr == NULL) {
		DPRINT(KERN_ERR "NAND - Failed to allocate memory for DMA main buffer\n");
		return -ENOMEM;
	}
	dolphin->v_mbuf = (u32)virt_ptr;
	dolphin->p_mbuf = (u32)phys_addr;

	virt_ptr = dma_alloc_coherent(NULL, dolphin->oob_size, &phys_addr, GFP_KERNEL);
	if (virt_ptr == NULL) {
		DPRINT(KERN_ERR "NAND - Failed to allocate memory for DMA oob buffer\n");
		dma_free_coherent(NULL, dolphin->write_size, (void *)dolphin->v_mbuf, (dma_addr_t)dolphin->p_mbuf);
		return -ENOMEM;
	}
	dolphin->v_oob = (u32)virt_ptr;
	dolphin->p_oob = (u32)phys_addr;
	return 0;
}
static void sprd_nand_dma_deinit(struct sprd_dolphin_nand_info *dolphin)
{
	dma_free_coherent(NULL, dolphin->write_size, (void *)dolphin->v_mbuf, (dma_addr_t)dolphin->p_mbuf);
	dma_free_coherent(NULL, dolphin->write_size, (void *)dolphin->v_oob, (dma_addr_t)dolphin->p_oob);
}
static int sprd_nand_probe(struct platform_device *pdev)
{
	struct nand_chip *this;
	struct resource *regs = NULL;
	struct mtd_partition *partitions = NULL;
	int num_partitions = 0;
	int ret = 0;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev,"resources unusable\n");
		goto prob_err;
	}

	memset(&g_dolphin, 0 , sizeof(g_dolphin));

	platform_set_drvdata(pdev, &g_dolphin);
	g_dolphin.pdev = pdev;

	sprd_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	this = (struct nand_chip *)(&sprd_mtd[1]);
	memset((char *)sprd_mtd, 0, sizeof(struct mtd_info));
	memset((char *)this, 0, sizeof(struct nand_chip));

	sprd_mtd->priv = this;

	this->options |= NAND_BUSWIDTH_16;
	this->options |= NAND_NO_READRDY;

	board_nand_init(this);


    if (sprd_dolphin_nand_reset(&g_dolphin) != 0)
    {
        ret = -ENXIO;
        printk("nand reset failed!!!!!!!!!!!!!\n");
        goto prob_err;
    }
    msleep(1);

    if (sprd_dolphin_nand_read_id(&g_dolphin, (uint32_t *)s_oob_data) != 0)
    {
        ret = -ENXIO;
        printk("nand read id failed, no nand device!!!!!!!!!!!!!\n");
        goto prob_err;
    }
    printk("nand read id ok, nand exists!!!!!!!!!!!!!\n");

	//nand_scan(sprd_mtd, 1);
	/* first scan to find the device and get the page size */
	if (nand_scan_ident(sprd_mtd, 1, NULL)) {
		ret = -ENXIO;
		goto prob_err;
	}
	sprd_dolphin_nand_read_id(&g_dolphin, (uint32_t *)s_oob_data);
	nand_hardware_config(sprd_mtd, this, s_oob_data);
	if(sprd_nand_dma_init(&g_dolphin) != 0) {
		return -ENOMEM;
	}
	
	//this->IO_ADDR_R = g_dolphin.v_mbuf;
	//this->IO_ADDR_W = g_dolphin.v_mbuf;

	/* second phase scan */
	if (nand_scan_tail(sprd_mtd)) {
		ret = -ENXIO;
		goto prob_err;
	}

	sprd_mtd->name = "sprd-nand";
	num_partitions = parse_mtd_partitions(sprd_mtd, part_probes, &partitions, 0);

	if ((!partitions) || (num_partitions == 0)) {
		DPRINT("No parititions defined, or unsupported device.\n");
		goto release;
	}

#ifdef CONFIG_MTD_CMDLINE_PARTS
	mtd_device_register(sprd_mtd, partitions, num_partitions);
#endif

	return 0;
release:
	nand_release(sprd_mtd);
	sprd_nand_dma_deinit(&g_dolphin);
prob_err:
	sprd_dolphin_reg_and(DOLPHIN_AHB_BASE,~(BIT(17) | BIT(18) | BIT(19) | BIT(6)));
	kfree(sprd_mtd);
	return ret;
}

static int sprd_nand_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	nand_release(sprd_mtd);
	sprd_nand_dma_deinit(&g_dolphin);
	kfree(sprd_mtd);
	return 0;
}

#ifdef CONFIG_PM
static int sprd_nand_suspend(struct platform_device *dev, pm_message_t pm)
{
	//nothing to do
	return 0;
}

static int sprd_nand_resume(struct platform_device *dev)
{
	sprd_dolphin_reg_write(NFC_TIMING_REG, NFC_DEFAULT_TIMING);
	sprd_dolphin_reg_write(NFC_TIMEOUT_REG, 0x80400000);
	//close write protect
	sprd_dolphin_nand_wp_en(&g_dolphin, 0);
	return 0;
}
#else
#define sprd_nand_suspend NULL
#define sprd_nand_resume NULL
#endif

static struct platform_driver sprd_nand_driver = {
	.probe		= sprd_nand_probe,
	.remove		= sprd_nand_remove,
	.suspend	= sprd_nand_suspend,
	.resume		= sprd_nand_resume,
	.driver		= {
		.name	= "sprd-nand",
		.owner	= THIS_MODULE,
	},
};

static int __init sprd_nand_init(void)
{
	return platform_driver_register(&sprd_nand_driver);
}

static void __exit sprd_nand_exit(void)
{
	platform_driver_unregister(&sprd_nand_driver);
}

module_init(sprd_nand_init);
module_exit(sprd_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("giya.li@spreadtrum.com");
MODULE_DESCRIPTION("SPRD dolphin MTD NAND driver");
MODULE_ALIAS("platform:sprd-nand");

#endif







