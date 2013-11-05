
#include <linux/mtd/mtd.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <common.h>
#if defined CONFIG_NAND_DOLPHIN 
#include <asm/arch/sprd_nfc_reg_v2.h>
#elif defined CONFIG_NAND_TIGER
#include <asm/arch/sprd_nfc_reg_v2.h>
#elif defined CONFIG_NAND_SC8810
#include <asm/arch/regs_nfc.h>
#else
#include "asm/arch/nand_controller.h"
#endif

#define BOOTLOADER_HEADER_OFFSET    32
#define CONFIG_SYS_SPL_ECC_POS    8
#define MAX_SPL_SIZE    0x6000


#if defined CONFIG_NAND_SC7710G2
struct bootloader_header
{
	uint32_t version; //version, fot tiger this member must be 0
	uint32_t magic_num; //0xaa55a5a5
	uint32_t sct_size; //
	uint32_t hash_len;//word length, only used when secure boot enable
	uint32_t acycle; // 3, 4, 5
	uint32_t bus_width; //0 ,1
	uint32_t spare_size; //spare part sise for one sector
	uint32_t ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	uint32_t ecc_pos; // ecc postion at spare part
	uint32_t sct_per_page; //sector per page
	uint32_t info_pos;
	uint32_t info_size;
	uint32_t ecc_value[27];
};

/*
 * spare info data is don't used at the romcode, so the fdl only set the s_info size to 1, and the data value 0xff
 */
void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
	struct bootloader_header *header;
	struct nand_chip *chip = nand->priv;
	struct sc8810_ecc_param param;
	u8 ecc[44];
	header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
	memset(header, 0, sizeof(struct bootloader_header));
	memset(ecc, 0xff, sizeof(ecc));

	header->version = 0x1;
	header->sct_size = chip->ecc.size;
	header->hash_len = 0x400;
	if (chip->options & NAND_BUSWIDTH_16){
		header->bus_width = 1;
	}
	if(nand->writesize > 512) {
		if (chip->chipsize > (128 << 20)){
			header->acycle = 5;
		}
		else{
			header->acycle = 4;
		}
	}
	else{
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20)){
			header->acycle = 3;
		}
		else{
			header->acycle = 3;
		}
	}

	header->acycle -= 3;

	header->magic_num = 0xaa55a5a5;
	header->spare_size = (nand->oobsize/chip->ecc.steps);
	header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
	header->ecc_pos = ecc_pos;
	header->sct_size = (nand->writesize/chip->ecc.steps);

	header->sct_per_page = chip->ecc.steps;

	param.mode = 24;
	param.ecc_num = 1;
	param.sp_size = sizeof(ecc);
	param.ecc_pos = 0;
	param.m_size = chip->ecc.size;
	param.p_mbuf = (u8 *)bl_data;
	param.p_sbuf = ecc;

	sc8810_ecc_encode(&param);
	memcpy(header->ecc_value, ecc, sizeof(ecc));

}

static int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
{
	int eccsteps;
	u32 eccsize;
	struct nand_chip *chip = mtd->priv;
	int eccbytes = chip->ecc.bytes;
	u32 i;
	u32 page;
	u32 spare_per_sct;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	eccsteps = chip->ecc.steps;
	eccsize = chip->ecc.size;
	spare_per_sct = mtd->oobsize / eccsteps;
	memset(chip->buffers->ecccode, 0xff, mtd->oobsize);
	
	page = (int)(pg >> chip->page_shift);
	
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);
	
	for (i = 0; i < eccsteps; i ++, buf += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, buf, eccsize);
		chip->ecc.calculate(mtd, buf, &ecc_calc[0]);
		memcpy(chip->buffers->ecccode + i * spare_per_sct + ecc_pos, &ecc_calc[0], eccbytes);
	}
	chip->write_buf(mtd, chip->buffers->ecccode, mtd->oobsize);
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	chip->waitfunc(mtd, chip);
	
	return 0;
}

#elif defined CONFIG_NAND_DOLPHIN

struct bootloader_header
{
	uint32_t version; //version
	uint32_t magic_num; //0xaa55a5a5	
	uint32_t check_sum;
	uint32_t hash_len; //word length, only used when secure boot enable
	uint32_t sct_size; //
	uint32_t acycle; // 3, 4, 5
	uint32_t bus_width; //0 ,1
	uint32_t spare_size; //spare part sise for one sector
	uint32_t ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
	uint32_t ecc_pos; // ecc postion at spare part
	uint32_t sct_per_page; //sector per page
	uint32_t info_pos;
	uint32_t info_size;
	uint32_t ecc_value[27];
};

/*
 * spare info data is don't used at the romcode, so the fdl only set the s_info size to 1, and the data value 0xff
 */
static void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
	struct bootloader_header *header;
	struct nand_chip *chip = nand->priv;
	struct sprd_ecc_param param;
	u8 ecc[108];

	header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
	memset(header, 0, sizeof(struct bootloader_header));
	memset(ecc, 0xff, sizeof(ecc));
	header->version = 1;
	header->sct_size = chip->ecc.size;
	if (chip->options & NAND_BUSWIDTH_16)	{
		header->bus_width = 1;
	}
	if(nand->writesize > 512) {
		if (chip->chipsize > (128 << 20))		{
			header->acycle = 5;
		}
		else	 {
			header->acycle = 4;
		}
	}
	else{
		/* One more address cycle for devices > 32MiB */

		if (chip->chipsize > (32 << 20)) {
			header->acycle = 4;
		}
		else	{
			header->acycle = 3;
		}
	}

	header->magic_num = 0xaa55a5a5;
	header->spare_size = (nand->oobsize/chip->ecc.steps);
	header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
	/*ecc is at the last postion at spare part*/
	header->ecc_pos = header->spare_size - (chip->ecc.bytes);
	header->sct_per_page = chip->ecc.steps;

	header->info_pos = header->ecc_pos - 1;
	header->info_size = 1;

	param.mode = 60;
	param.ecc_pos = 0;
	param.sinfo_size = 1;
	param.ecc_num = 1;
	param.sp_size = sizeof(ecc);
	param.m_size = chip->ecc.size;
	param.p_mbuf = (u8 *)bl_data;
	param.p_sbuf = ecc;
	param.sinfo_pos = 0;
	sprd_ecc_encode(&param);
	memcpy(header->ecc_value, ecc, sizeof(ecc));
}

static int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
{
	struct nand_chip *chip = mtd->priv;
	int status;
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, pg);
	chip->ecc.write_page(mtd, chip, buf);
	chip->ecc.write_page(mtd, chip, buf);
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	chip->waitfunc(mtd, chip);
	if (status & NAND_STATUS_FAIL) {
		return -1;
	}
	if (status & NAND_STATUS_FAIL) {
		return -1;
	}
	return 0;
}

#else
//dummy
static void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
	return;
}

static int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
{
	return -1;
}
#endif

int fdl_nand_write_spl(u8 *buf, struct mtd_info *mtd)
{
	u32 i = 0;
	u32 pg_start;
	u32 pg_end;
	u32 pg;
	u8 * data;
	int ret = 0;

	set_header_info(buf, mtd, CONFIG_SYS_SPL_ECC_POS);

	/* write spl to flash*/
	for (i = 0; i < 2; i++)
	{
		pg_start = i * MAX_SPL_SIZE / mtd->writesize;
		pg_end = (i + 1) * MAX_SPL_SIZE / mtd->writesize;
		data = buf;
		for(pg  = pg_start; pg < pg_end; pg += 1) {
			ret = nand_write_spl_page(data, mtd, pg, CONFIG_SYS_SPL_ECC_POS);
			data += mtd->writesize;
		}
	}
	return ret;
}
