#include <config.h>
#include <common.h>
#include <linux/types.h>
#include <asm/arch/bits.h>
#include <image.h>
#include <linux/string.h>
#include <android_bootimg.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <nand.h>
#include <android_boot.h>
#include <environment.h>
#include <jffs2/jffs2.h>
#include <boot_mode.h>
#include <android_recovery.h>
#include <fat.h>
#include <asm/byteorder.h>
#include <part.h>
#include <mmc.h>
#if   defined CONFIG_NAND_TIGER
#include <asm/arch/sprd_nfc_reg_v2.h>
#elif defined CONFIG_NAND_SC8810
#include <asm/arch/regs_nfc.h>
#else
#include "asm/arch/nand_controller.h"
#endif
#include "asm/arch/sci_types.h"

#ifdef dprintf
#undef dprintf
#endif
#define dprintf(fmt, args...) printf(fmt, ##args)

#define CONFIG_SYS_SPL_ECC_POS  8
#define MAX_SPL_SIZE    0x6000
#define BOOTLOADER_HEADER_OFFSET   32
#define NAND_PAGE_LEN              512
static const int MISC_COMMAND_PAGE = 1;  // bootloader command is this page
static char buf[8192];


extern int ensure_path_mounted(const char * mountpoint);
extern int ensure_path_umounted(const char * mountpoint);

int get_recovery_message(struct recovery_message *out)
{
    loff_t offset = 0;
    unsigned pagesize;
    size_t size;

    struct mtd_info *nand;
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;
    int ret;

    ret = mtdparts_init();
    if(ret != 0){
        dprintf("mtdparts init error %d\n", ret);
        return -1;
    }

    ret = find_dev_and_part("misc", &dev, &pnum, &part);
    if(ret){
        dprintf("No partiton named %s found\n", "misc");
        return -1;
    }else if(dev->id->type != MTD_DEV_TYPE_NAND){
        printf("Partition %s not a NAND device\n", "misc");
        return -1;
    }

    nand = &nand_info[dev->id->num];
    pagesize = nand->writesize;

    offset = pagesize * MISC_COMMAND_PAGE + part->offset;
    size = pagesize;
    ret = nand_read_skip_bad(nand, offset, &size, (void *)buf);
    if(ret != 0){
        printf("function: %s nand read error %d\n", __FUNCTION__, ret);
        return -1;
    }

    memcpy(out, buf, sizeof(*out));
    return 0;
}

int set_recovery_message(const struct recovery_message *in)
{
    loff_t offset = 0;
    unsigned pagesize;
    size_t size;

    struct mtd_info *nand;
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;
    int ret;

    ret = mtdparts_init();
    if(ret != 0){
        dprintf("mtdparts init error %d\n", ret);
        return -1;
    }

    ret = find_dev_and_part("misc", &dev, &pnum, &part);
    if(ret){
        dprintf("No partiton named %s found\n", "misc");
        return -1;
    }else if(dev->id->type != MTD_DEV_TYPE_NAND){
        dprintf("Partition %s not a NAND device\n", "misc");
        return -1;
    }

    nand = &nand_info[dev->id->num];
    pagesize = nand->writesize;

    size = pagesize*(MISC_COMMAND_PAGE + 1);

    ret = nand_read_skip_bad(nand, part->offset, &size, (void *)SCRATCH_ADDR);
    if(ret != 0){
        dprintf("%s: nand read error %d\n", __FUNCTION__, ret);
        return -1;
    }


    offset = SCRATCH_ADDR;
    offset += (pagesize * MISC_COMMAND_PAGE);
    memcpy(offset, in, sizeof(*in));

    nand_erase_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.offset = part->offset;
    opts.length = pagesize *(MISC_COMMAND_PAGE + 1);
    opts.jffs2 = 0;
    opts.scrub = 0;
    opts.quiet = 1;
    ret = nand_erase_opts(nand, &opts);
    if(ret != 0){
        dprintf("%s, nand erase error %d\n", __FUNCTION__, ret);
        return -1;
    }
    ret = nand_write_skip_bad(nand, part->offset, &size, (void *)SCRATCH_ADDR);
    if(ret != 0){
        dprintf("%s, nand erase error %d\n", __FUNCTION__, ret);
        return -1;
    }

}
unsigned short CheckSum(const unsigned int *src, int len)
{
    unsigned int   sum = 0;
    unsigned short *src_short_ptr = PNULL;

    while (len > 3)
    {
        sum += *src++;
        len -= 4;
    }

    src_short_ptr = (unsigned short *) src;

    if (0 != (len&0x2))
    {
        sum += * (src_short_ptr);
        src_short_ptr++;
    }

    if (0 != (len&0x1))
    {
        sum += * ( (unsigned char *) (src_short_ptr));
    }

    sum  = (sum >> 16) + (sum & 0x0FFFF);
    sum += (sum >> 16);

    return (unsigned short) (~sum);
}
#if defined CONFIG_NAND_SC8810 && !defined CONFIG_NAND_SC7710G2
struct bootloader_header
{
    u32 check_sum;
    u32 page_type; //page type 0-512,1-1K,2-2k,3-4k,4-8K
    u32 acycle; // 3, 4, 5
    u32 bus_width; //0 ,1
    u32 advance; // 0, 1
    u32 magic_num; //0xaa55a5a5
    u32 spare_size; //spare part sise for one sector
    u32 ecc_mode; //0--1bit, 1--2bit,2--4bit,3--8bit,4--12bit, 5--16bit, 6--24bit
    u32 ecc_pos; // ecc postion at spare part
    u32 sct_size; //sector size;
    u32 sct_per_page; //sector per page
    u32 ecc_value[11];
};
u32 get_nand_page_type(u32 pg_sz)
{
    u32 pg_type = 0;
    switch(pg_sz)
    {
        case 512:
            pg_type = 0;
            break;
        case 1024:
            pg_type = 1;
            break;
        case 2048:
            pg_type = 2;
            break;
        case 4096:
            pg_type = 3;
            break;
        case 892:
            pg_type = 4;
            break;
        default:
            while(1);
            break;
    }
    return pg_type;
}
extern unsigned short CheckSum(const unsigned int *src, int len);
void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
    struct bootloader_header *header;
    struct nand_chip *chip = nand->priv;
    struct sc8810_ecc_param param;
    u8 ecc[44];
    header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
    memset(header, 0, sizeof(struct bootloader_header));
    memset(ecc, 0, sizeof(ecc));
#if 1
    header->page_type = get_nand_page_type(nand->writesize);
    if (chip->options & NAND_BUSWIDTH_16) {
        header->bus_width = 1;
    }
    if(nand->writesize > 512) {
        header->advance = 1;
        /* One more address cycle for devices > 128MiB */
        if (chip->chipsize > (128 << 20))  {
            header->acycle = 5;
        }
        else  {
            header->acycle = 4;
        }
    }
    else{
        header->advance = 0;
        /* One more address cycle for devices > 32MiB */
        if (chip->chipsize > (32 << 20)) {
            header->acycle = 3;
        }
        else {
            header->acycle = 3;
        }
    }
    header->magic_num = 0xaa55a5a5;
    header->spare_size = (nand->oobsize/chip->ecc.steps);
    header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
    header->ecc_pos = ecc_pos;
    header->sct_size = (nand->writesize/chip->ecc.steps);
    header->sct_per_page = chip->ecc.steps;
    header->check_sum = CheckSum((unsigned int *)(bl_data + BOOTLOADER_HEADER_OFFSET + 4), (NAND_PAGE_LEN - BOOTLOADER_HEADER_OFFSET - 4));

    param.mode = 24;
    param.ecc_num = 1;
    param.sp_size = sizeof(ecc);
    param.ecc_pos = 0;
    param.m_size = chip->ecc.size;
    param.p_mbuf = (u8 *)bl_data;
    param.p_sbuf = ecc;
    sc8810_ecc_encode(&param);
    memcpy(header->ecc_value, ecc, sizeof(ecc));
#endif
}
int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
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
int nand_write_spl(u8 *buf, struct mtd_info *mtd)
{
    u32 i;
    u32 pg_start;
    u32 pg_end;
    u32 pg;
    u8 * data;
    int ret = 0;

    //struct nand_chip *chip = mtd->priv;
    set_header_info(buf, mtd, CONFIG_SYS_SPL_ECC_POS);
    /* erase spl block in nand_start_write(), so erase spl block code is deleted */

    /* write spl to flash*/
    for (i = 0; i < 3; i++) {
        pg_start = i * MAX_SPL_SIZE;
        pg_end = (i + 1) * MAX_SPL_SIZE;
        data = buf;
        for(pg  = pg_start; pg < pg_end; pg += mtd->writesize) {
            ret = nand_write_spl_page(data, mtd, pg, CONFIG_SYS_SPL_ECC_POS);
            data += mtd->writesize;
        }
    }

    return ret;
}
#endif
#ifdef CONFIG_NAND_TIGER
struct bootloader_header
{
    uint32_t version; //version, fot tiger this member must be 0
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
    uint32_t hash_len; //word length, only used when secure boot enable
    uint32_t magic_num; //0xaa55a5a5
    uint32_t ecc_value[11];
};
extern unsigned short CheckSum(const unsigned int *src, int len);
/*
 * spare info data is don't used at the romcode, so the fdl only set the s_info size to 1, and the data value 0xff
 */
void set_header_info(u8 *bl_data, struct mtd_info *nand, int ecc_pos)
{
    struct bootloader_header *header;
    struct nand_chip *chip = nand->priv;
    struct sprd_ecc_param param;
    u8 ecc[44];
    header = (struct bootloader_header *)(bl_data + BOOTLOADER_HEADER_OFFSET);
    memset(header, 0, sizeof(struct bootloader_header));
    memset(ecc, 0xff, sizeof(ecc));
#if 1
    header->version = 0;
    header->sct_size = chip->ecc.size;
    if (chip->options & NAND_BUSWIDTH_16) {
        header->bus_width = 1;
    }
    if(nand->writesize > 512) {
        if (chip->chipsize > (128 << 20))  {
            header->acycle = 5;
        }
        else  {
            header->acycle = 4;
        }
    }
    else{
        /* One more address cycle for devices > 32MiB */
        if (chip->chipsize > (32 << 20)) {
            header->acycle = 4;
        }
        else {
            header->acycle = 3;
        }
    }
    header->magic_num = 0xaa55a5a5;
    header->spare_size = (nand->oobsize/chip->ecc.steps);
    header->ecc_mode = ecc_mode_convert(chip->eccbitmode);
    /*ecc is at the last postion at spare part*/
    header->ecc_pos = header->spare_size - (chip->ecc.bytes);
    header->sct_per_page = chip->ecc.steps;

    header->sct_per_page = chip->ecc.steps;
    header->info_pos = header->ecc_pos - 1;
    header->info_size = 1;

    header->check_sum = CheckSum((unsigned int *)(bl_data + BOOTLOADER_HEADER_OFFSET + 4), (NAND_PAGE_LEN - BOOTLOADER_HEADER_OFFSET - 4));

    param.mode = 24;
    param.ecc_num = 1;
    param.sp_size = sizeof(ecc);
    param.ecc_pos = 1;
    param.m_size = chip->ecc.size;
    param.p_mbuf = (u8 *)bl_data;
    param.p_sbuf = ecc;
    param.sinfo_pos = 0;
    param.sinfo_size = 1;
    sprd_ecc_encode(&param);
    memcpy(header->ecc_value, ecc, sizeof(ecc));
#endif
}
int nand_write_spl_page(u8 *buf, struct mtd_info *mtd, u32 pg, u32 ecc_pos)
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
int nand_write_spl(u8 *buf, struct mtd_info *mtd)
{
    u32 i;
    u32 pg_start;
    u32 pg_end;
    u32 pg;
    u8 * data;
    int ret = 0;

    //struct nand_chip *chip = mtd->priv;
    set_header_info(buf, mtd, CONFIG_SYS_SPL_ECC_POS);
    /* erase spl block in nand_start_write(), so erase spl block code is deleted */

    /* write spl to flash*/
    for (i = 0; i < 3; i++) {
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
#endif
#ifdef CONFIG_GENERIC_MMC
#define FIX_SIZE (64*1024)

#define BUF_ADDR CONFIG_SYS_SDRAM_BASE+0x1000000
#define SD_NV_NAME "nvitem.bin"
#define NAND_NV_NAME " "
#define MODEM_PART "modem"
#define SD_MODEM_NAME "modem.bin"
#define SD_SPL_NAME "u-boot-spl-16k.bin"
#define DSP_PART "dsp"
#define SD_DSP_NAME "dsp.bin"
#define VM_PART "vmjaluna"
#define SD_VM_NAME "vmjaluna.bin"

void try_update_modem(void)
{
    struct mmc *mmc;
    block_dev_desc_t *dev_desc=NULL;
    loff_t off, size;
    nand_info_t *nand;
    struct mtd_device *dev;
    u8 pnum;
    struct part_info *part;
    int ret;
    char *fixnvpoint = "/fixnv";
    //char *fixnvfilename = "/fixnv/fixnv.bin";
    char *fixnvfilename = "/fixnv/fixnvchange.bin";
    nand_erase_options_t opts;

    mmc = find_mmc_device(0);
    if(mmc){
        ret = mmc_init(mmc);
        if(ret < 0){
            printf("mmc init failed %d\n", ret);
            return;
        }
    }else{
        printf("no mmc card found\n");
        return;
    }

    dev_desc = &mmc->block_dev;
    if(dev_desc==NULL){
        printf("no mmc block device found\n");
        return;
    }
    ret = fat_register_device(dev_desc, 1);
    if(ret < 0){
        printf("fat regist fail %d\n", ret);
        return;
    }
    ret = file_fat_detectfs();
    if(ret){
        printf("detect fs failed\n");
        return;
    }
    do{
        printf("reading %s\n", SD_NV_NAME);
        ret = do_fat_read(SD_NV_NAME, BUF_ADDR, 0, LS_NO);
        if(ret <= 0){
            printf("sd file read error %d\n", ret);
            break;
        }
        size = FIX_SIZE + 4;
        nv_patch(BUF_ADDR, ret);
        ensure_path_mounted(fixnvpoint);
        cmd_yaffs_mwrite_file(fixnvfilename, BUF_ADDR, size);
        //  ensure_path_umounted(fixnvpoint);

        file_fat_rm(SD_NV_NAME);
    }while(0);
    do{
        printf("reading %s\n", SD_SPL_NAME);
        ret = do_fat_read(SD_SPL_NAME, BUF_ADDR, 0, LS_NO);
        if(ret <= 0){
            printf("sd file read error %d\n", ret);
            break;
        }
        size = ret;
        ret = find_dev_and_part("spl", &dev, &pnum, &part);
        if (ret) {
            printf("No partition named %s\n", MODEM_PART);
            break;
        } else if (dev->id->type != MTD_DEV_TYPE_NAND) {
            printf("Partition %s not a NAND device\n", MODEM_PART);
            break;
        }
        off = part->offset;
        nand = &nand_info[dev->id->num];
        memset(&opts, 0, sizeof(opts));
        opts.offset = off;
        opts.length = part->size;
        opts.quiet  = 1;
        ret = nand_erase_opts(nand, &opts);
        if(ret){
            printf("nand erase bad %d\n", ret);
            break;
        }
        ret = nand_write_spl((u8*)BUF_ADDR, nand);
        if(ret){
            printf("nand write bad %d\n", ret);
            break;
        }

        file_fat_rm(SD_SPL_NAME);
    }while(0);
    do{
        printf("reading %s\n", SD_MODEM_NAME);
        ret = do_fat_read(SD_MODEM_NAME, BUF_ADDR, 0, LS_NO);
        if(ret <= 0){
            printf("sd file read error %d\n", ret);
            break;
        }
        size = ret;
        ret = find_dev_and_part(MODEM_PART, &dev, &pnum, &part);
        if (ret) {
            printf("No partition named %s\n", MODEM_PART);
            break;
        } else if (dev->id->type != MTD_DEV_TYPE_NAND) {
            printf("Partition %s not a NAND device\n", MODEM_PART);
            break;
        }
        off = part->offset;
        nand = &nand_info[dev->id->num];
        memset(&opts, 0, sizeof(opts));
        opts.offset = off;
        opts.length = part->size;
        opts.quiet  = 1;
        ret = nand_erase_opts(nand, &opts);
        if(ret){
            printf("nand erase bad %d\n", ret);
            break;
        }
        ret = nand_write_skip_bad(nand, off, &size, BUF_ADDR);
        if(ret){
            printf("nand write bad %d\n", ret);
            break;
        }

        file_fat_rm(SD_MODEM_NAME);
    }while(0);

    do{
        printf("reading %s\n", SD_DSP_NAME);
        ret = do_fat_read(SD_DSP_NAME, BUF_ADDR, 0, LS_NO);
        if(ret <= 0){
            printf("sd file read error %d\n", ret);
            break;
        }
        size = ret;
        ret = find_dev_and_part(DSP_PART, &dev, &pnum, &part);
        if (ret) {
            printf("No partition named %s\n", DSP_PART);
            break;
        } else if (dev->id->type != MTD_DEV_TYPE_NAND) {
            printf("Partition %s not a NAND device\n", DSP_PART);
            break;
        }
        off = part->offset;
        nand = &nand_info[dev->id->num];
        memset(&opts, 0, sizeof(opts));
        opts.offset = off;
        opts.length = part->size;
        opts.quiet  = 1;
        ret = nand_erase_opts(nand, &opts);
        if(ret){
            printf("nand erase bad %d\n", ret);
            break;
        }
        ret = nand_write_skip_bad(nand, off, &size, BUF_ADDR);
        if(ret < 0){
            printf("nand write bad %d\n", ret);
            break;
        }

        file_fat_rm(SD_DSP_NAME);
    }while(0);

    do{
        printf("reading %s\n", SD_VM_NAME);
        ret = do_fat_read(SD_VM_NAME, BUF_ADDR, 0, LS_NO);
        if(ret <= 0){
            printf("sd file read error %d\n", ret);
            break;
        }
        size = ret;
        ret = find_dev_and_part(VM_PART, &dev, &pnum, &part);
        if (ret) {
            printf("No partition named %s\n", VM_PART);
            break;
        } else if (dev->id->type != MTD_DEV_TYPE_NAND) {
            printf("Partition %s not a NAND device\n", VM_PART);
            break;
        }
        off = part->offset;
        nand = &nand_info[dev->id->num];
        memset(&opts, 0, sizeof(opts));
        opts.offset = off;
        opts.length = part->size;
        opts.quiet  = 1;
        ret = nand_erase_opts(nand, &opts);
        if(ret){
            printf("nand erase bad %d\n", ret);
            break;
        }
        ret = nand_write_skip_bad(nand, off, &size, BUF_ADDR);
        if(ret < 0){
            printf("nand write bad %d\n", ret);
            break;
        }

        file_fat_rm(SD_VM_NAME);
    }while(0);

    printf("update done\n");
    return;
}
#else
void try_update_modem(void)
{
}
#endif
