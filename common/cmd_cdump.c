/*
 * Copyright (C)
 */

#include <common.h>
#include <command.h>
#include "malloc.h"
#include <mmc.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <elf.h>
#include <fat.h>
#include <exports.h>
#include <linux/ctype.h>
#include <linux/time.h>
#ifdef CONFIG_LCD
#include <lcd.h>
#endif

#ifdef CONFIG_LCD
extern int lcd_display_bitmap(ulong bmp_image, int x, int y);
extern lcd_display(void);
extern void set_backlight(uint32_t value);
#endif

#define MAX_PREFIX 1024
#define MAX_CRASH_FILE_NAME_LEN 100
#define debug(fmt, args...) printf(fmt, ##args)

typedef struct range {
	unsigned long vstart;
        unsigned long pstart;
	unsigned long size;
} range_t;

#ifdef CONFIG_RAM512M
range_t mem_range[]=
{
{0xc0000000,0x00000000,0x20000000}
//{0xF1290000,0x47000000,0x00001000}//IO RESOUCE
//{0x80473000,0x0000D000},
//{0x804e0000,0x0001f000},
//{0x82400000,0x1DC00000}
};
#else
range_t mem_range[]=
{
{0xc0000000,0x00000000,0x10000000}
//{0xF1290000,0x47000000,0x00001000}//IO RESOUCE
//{0x80303000,0x000fd000},
//{0x80470000,0x00010000},
//{0x804e0000,0x0001f000},
//{0x814cf000,0x0eb31000}
};
#endif


/*-----------------------------------------------------------------------
 * crash dump related configs
 */
#define CONFIG_SD_FAT_DEV_NUM	0	/* sdcard fat filesystem on mmc dev */
#define CONFIG_SD_FAT_PART_NUM	1	/* sdcard fat filesystem partition */

/*
 * Note: with the Rockbox FAT support, the file path must be an absolute path,
 * i.e. with leading /.
 */
static char crash_filename[MAX_CRASH_FILE_NAME_LEN];
static char crash_modem_filename[MAX_CRASH_FILE_NAME_LEN];

/*
 * Check ELF header
 */
static int check_elfhdr(Elf32_Ehdr *elfhdr_addr)
{
	unsigned char *elfhdr = (unsigned char *) elfhdr_addr;

	/* check ELF core image header MAGIC */
	if (memcmp(elfhdr, ELFMAG, SELFMAG) != 0)
		return 1;

	/* check that this is ELF32 */
	if (elfhdr[EI_CLASS] == ELFCLASS32)
		return 0;

	return 1;
}

/*
 * Write a chunk
 */
static int write_chunk(int fd, void *addr, size_t count)
{
	size_t bytes;

	bytes = write(fd, addr, count);
	if (bytes != count) {
		debug("write error\n");
		close(fd);
		return -1;
	}
	return 0;
}

/*
 * Write a big chunk with 'progress' indicator '.' for every MiB
 */
static int write_big_chunk(int fd, void *addr, size_t count)
{
	unsigned char *a = addr;
	size_t total = 0;

	debug("write_big_chunk: fd=%d, addr=%x, count=%x\n", fd, addr, count);
	if (count < 0x100000)
		return write_chunk(fd, addr, count);
	/* if large chunk then print dot to show progress */
	while (total < count) {
		size_t bytes = count - total;
		if (bytes > 0x100000)
			bytes = 0x100000;
		if (write_chunk(fd, a, bytes))
			return -1;
		total += bytes;
		a += bytes;
		/* Output one . each megabytes */
		debug(".");
#ifdef CONFIG_LCD
		lcd_printf(".");
#endif
	}
	debug("\n");
#ifdef CONFIG_LCD
	lcd_printf("\n");
#endif
	return 0;
}

/*
 * Open the dump file for writing. Create if it not exists.
 * Note that with the Rockbox FAT support, the file path must be an absolute
 * path, i.e. with leading /.
 */
static int open_create(const char *filename)
{
	int fd;

	fd = open(filename, O_WRONLY | O_CREAT);
	if (fd < 0)
		debug("%s open error\n", filename);
	return fd;
}

/*
 * Check program header and segment
 * Truncate note segments.
 * Return segment size.
 */
static u32 check_phdr(Elf32_Phdr *proghdr)
{
	u32 i;
	u32 *note;
	Elf32_Phdr *phdr = proghdr;

	if (phdr->p_type == PT_NOTE) {
		/* see Linux kernel/kexec.c:append_elf_note() */
		note = (u32 *)(phdr->p_paddr);
		for (i = 0; i < phdr->p_filesz/4;) {
			if (note[i] == 0 && note[i+1] == 0 && note[i+2] == 0)
				return i*4;
			i += 3 + (note[i] + 3) / 4 + (note[i+1] + 3) / 4;
		}
	}

	return phdr->p_filesz;
}

/*
 * Dump crash to file
 */
static int write_elf(Elf32_Ehdr *elfhdr_addr, int fd)
{
	Elf32_Ehdr *oldhdr = elfhdr_addr;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	u32 i;
	u32 offset;
	u32 tot;
	u32 phdr_cnt;
	u32 notes_cnt = 0;
	u32 save;
	u32 len;

	offset = oldhdr->e_ehsize + oldhdr->e_phentsize * oldhdr->e_phnum;
	ehdr = (Elf32_Ehdr *) malloc(offset);
	if (ehdr == NULL) {
		debug("elf header alloc error\n");
		return -1;
	}
	memcpy(ehdr, oldhdr, offset);

	/*
	 * check program header entries and update length
	 * for merged PT_NOTE segments
	 */
	tot = 0;
	phdr_cnt = ehdr->e_phnum;
	debug("phdr_cnt=%d, ehdr=0x%x\n", phdr_cnt, ehdr);
	for (i = 0; i < phdr_cnt; i++) {
		phdr = (Elf32_Phdr *) ((char *) ehdr + ehdr->e_ehsize +
				       i * ehdr->e_phentsize);
		len = check_phdr(phdr);
		debug("prog hdr %d: %x ad %x len %x adjusted to %x\n",
		      i, (u32) phdr, phdr->p_paddr, phdr->p_filesz, len);
		phdr->p_filesz = len;
		phdr->p_memsz = len;
		if (phdr->p_type == PT_NOTE) {	/* note segment */
			tot += len;
			notes_cnt++;
		}
	}
	debug("Length of %d note segments: %x\n", notes_cnt, tot);

	/* God will touch the kernel physical memory 0x8FF8 2000 - 0x8FFFF FFFF */
#if 0
	{
		int i;
		unsigned int *p = (unsigned int*) 0x8ff82000;
		
		debug("data in 0x8ff82000:\n");
		for (i = 0; i < 0x100; i = i+8)
			debug("%x %x %x %x %x %x %x %x\n", *(p+i), *(p+i+1), *(p+i+2), *(p+i+3),
				*(p+i+4), *(p+i+5), *(p+i+6), *(p+i+7));
	}
#endif

	/*
	 * all PT_NOTE segments have been merged into one.
	 * Update ELF Header accordingly
	 */
	ehdr->e_phnum = phdr_cnt - notes_cnt + 1;

	/* write elf header into file on sdcard */
	if (write_chunk(fd, ehdr, (size_t) ehdr->e_ehsize)) {
		free(ehdr);
		return -1;
	}

	/* write program headers into file on sdcard */
	offset = ehdr->e_ehsize + ehdr->e_phentsize * ehdr->e_phnum;
	debug("Write Phdr: proghdr_cnt=%d\n", phdr_cnt);
	for (i = 0; i < phdr_cnt; i++) {
		phdr = (Elf32_Phdr *) ((char *)ehdr + ehdr->e_ehsize +
				       i * ehdr->e_phentsize);
		save = phdr->p_filesz;
		if (i == 0) {
			phdr->p_filesz = tot;
			phdr->p_memsz = tot;
		} else if (phdr->p_type == PT_NOTE) /* note segment */
			continue;
		phdr->p_offset = offset;
		debug("prog hdr %d: %x ad %x len %x off %x\n",
		       i, (u32) phdr, phdr->p_paddr, phdr->p_filesz,
		       phdr->p_offset);
		offset += phdr->p_filesz;
		if (write_chunk(fd, (void *) phdr, (size_t)
				ehdr->e_phentsize)) {
			free(ehdr);
			return -1;
		}
		phdr->p_filesz = save;
		phdr->p_memsz = save;
	}

	/* write segments into file on sdcard */
	debug("write segments...\n");
	for (i = 0; i < phdr_cnt; i++) {
		phdr = (Elf32_Phdr *) ((char *) ehdr + ehdr->e_ehsize +
				       i * ehdr->e_phentsize);
		if (phdr->p_type > PT_NULL) {
			if (write_big_chunk(fd, (void *) phdr->p_paddr,
							 phdr->p_filesz)) {
				free(ehdr);
				return -1;
			}
		}
	}

	free(ehdr);
	return 0;
}
/*
 * Dump crash to file
 */
static int dump_elf(Elf32_Ehdr *elfhdr_addr, char *filename)
{
	int fd;
	int rc;

#ifdef CONFIG_LCD
	lcd_printf("Dump crash to file %s,please wait!!!\n", filename);
	lcd_display();
	set_backlight(255);
#endif
	debug("Crash dump to file %s\n", filename);

	fd = open_create(filename);
	if (fd < 0) {
#ifdef CONFIG_LCD
	        lcd_printf("Do not have slog in sdcard\n");
	        lcd_display();
	        set_backlight(255);
#endif
		return 1;
        }
	rc = write_elf(elfhdr_addr, fd);
	close(fd);

#ifdef CONFIG_LCD
	if (rc !=0) {
		lcd_printf("Dump crash to file failed.\n");
	}
	else {

		lcd_printf("Dump crash to file completed.\n");
	}
#endif
	/* wait for 5s to notify user for reboot*/
	udelay(5*1000*1000);
	return rc;
}
/*
 * Dump modem to file
 */
 /*
static int dump_modem_bin(char *filename)
{
    int fd;
    int rc;
	
#ifdef CONFIG_LCD 
    lcd_printf("Dump modem memory to file %s,please wait!!!\n", filename);
    lcd_display();
    set_backlight(255);
#endif
    
    debug("dump modem to file %s\n", filename);
    
    fd = open_create(filename);
    if (fd < 0)
        return 1;
    
    rc = write_big_chunk(fd, (void *)MODEM_MEMORY_ADDR, MODEM_MEMORY_SIZE);
    close(fd);
    
#ifdef CONFIG_LCD
    if (rc !=0) {
        lcd_printf("Dump modem to file failed.\n");
    } else {
        lcd_printf("Dump modem to file completed.\n");
    }
#endif
    
    udelay(5*1000*1000);
    return rc;
}
*/

/*
 * Wait for MMC/SD card to be inserted
 */
static int wait_for_mmc(void)
{
	struct mmc *mmc;

	mmc = find_mmc_device(CONFIG_SD_FAT_DEV_NUM);
	if (!mmc) {
		debug("MMC device %d not found\n", CONFIG_SD_FAT_DEV_NUM);
#ifdef CONFIG_LCD
		lcd_printf("MMC device %d not found\n", CONFIG_SD_FAT_DEV_NUM);
		lcd_display();
		set_backlight(255);
#endif
		return 1;
	}

	while (mmc_init(mmc) != 0) {
		debug("Insert MMC/SD card or press ctrl-c to abort\n");

#ifdef CONFIG_LCD
		lcd_printf("Insert MMC/SD card or remove battery to abort ...\n");
		lcd_display();
		set_backlight(255);
#endif
		putc('.');
		udelay(500000);
		/* check for ctrl-c to abort... */
		if (ctrlc()) {
			puts("Abort\n");
			return -1;
		}
	}
	return 0;
}
/*
 * Find then next dump file name to ensure we do not
 * overwrite an old core dump.
 */
static int find_dump_file_name(bool is_modem)
{
	int fd;
	int prefix = 0;
	int ret = 0;
	time_t t;
	struct tm tm;

	/* add timestamp */
	t = get_timer(0);
	localtime_r(&t, &tm);

        do {
		prefix++;
	    if (is_modem) {
                sprintf(crash_modem_filename,"/slog/cdump_modem_%d_%02d_%02d_%02d_%02d_%02d_%02d.mem",
                        prefix,
                        tm.tm_year % 100,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec
                );
			fd = open(crash_modem_filename, O_WRONLY);
	    } else {
                sprintf(crash_filename,"/slog/cdump_%d_%02d_%02d_%02d_%02d_%02d_%02d.elf",
                        prefix,
                        tm.tm_year % 100,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec
                );
			fd = open(crash_filename, O_WRONLY);
            }
                        if(fd >= 0)
			close(fd);
	} while (fd >= 0 && prefix < MAX_PREFIX);

	return prefix;
}

/*
 * Dump crash to file (typically FAT file on SD/MMC).
 */
static int crashdump(Elf32_Ehdr *elfhdr_addr)
{
	int rc;
	block_dev_desc_t *dev_desc=NULL;

	rc = wait_for_mmc();
	if (rc == 0) {
#if 0
		dev_desc = get_dev("mmc", CONFIG_SD_FAT_DEV_NUM);
#else
	struct mmc *mmc;

	mmc = find_mmc_device(CONFIG_SD_FAT_DEV_NUM);
	if (!mmc) {
		debug("MMC device %d not found\n", CONFIG_SD_FAT_DEV_NUM);
		return 1;
	}
	dev_desc = &mmc->block_dev;
#endif
		rc = fat_register_device(dev_desc, CONFIG_SD_FAT_PART_NUM);
		if (rc != 0) {
			debug("crashdump: fat_register_device failed %d\n",
					rc);
			return -1;
		}

		if(find_dump_file_name(false) < MAX_PREFIX)
			rc = dump_elf(elfhdr_addr, crash_filename);
		else
			debug("Number of dumps have reached maximum on SD card: %d\n", MAX_PREFIX);
	}

	if (rc != 0)
		debug("crashdump: error writing dump to %s\n", crash_filename);

	return rc;
}

/*
static int dumpModem(void)
{
	int rc;
	block_dev_desc_t *dev_desc=NULL;

	rc = wait_for_mmc();
	debug("wait_for_mmc %d\n", rc);
	if (rc == 0) {
#if 0
		dev_desc = get_dev("mmc", CONFIG_SD_FAT_DEV_NUM);
#else
	struct mmc *mmc;

	mmc = find_mmc_device(CONFIG_SD_FAT_DEV_NUM);
	if (!mmc) {
		debug("MMC device %d not found\n", CONFIG_SD_FAT_DEV_NUM);
		return 1;
	}
	dev_desc = &mmc->block_dev;
#endif
		rc = fat_register_device(dev_desc, CONFIG_SD_FAT_PART_NUM);
		if (rc != 0) {
			debug("dumpModem: fat_register_device failed %d\n",
					rc);
			return -1;
		}

		if(find_dump_file_name(true) < MAX_PREFIX)
			rc = dump_modem_bin(crash_modem_filename);
		else
			debug("Number of dumps have reached maximum on SD card: %d\n", MAX_PREFIX);
	}

	if (rc != 0)
		debug("dumpModem: error writing dump to %s\n", crash_filename);

	return rc;
}
*/

Elf32_Ehdr *create_elfhdr(range_t *regions, unsigned int region_no) {
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	unsigned int hdr_size;
	unsigned int i;

	hdr_size = sizeof(*ehdr)+(region_no+1)*sizeof(*phdr);
	ehdr = malloc(hdr_size);
	if(ehdr == NULL) {
		return NULL;
	}

	memset(ehdr, 0, hdr_size);

	ehdr->e_ident[EI_MAG0] = ELFMAG0;
	ehdr->e_ident[EI_MAG1] = ELFMAG1;
	ehdr->e_ident[EI_MAG2] = ELFMAG2;
	ehdr->e_ident[EI_MAG3] = ELFMAG3;
	ehdr->e_ident[EI_CLASS] = ELFCLASS32;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = 1;
	ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
	ehdr->e_ident[EI_ABIVERSION] = ELFABIVERSION;
	ehdr->e_type = ET_CORE;
	ehdr->e_machine = EM_ARM;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = 0x0;
	ehdr->e_phoff = sizeof(*ehdr);
	ehdr->e_shoff = 0x0;
	ehdr->e_flags = 0x0;
	ehdr->e_ehsize = sizeof(*ehdr);
	ehdr->e_phentsize = sizeof(*phdr);
	ehdr->e_phnum = region_no+1;
	ehdr->e_shentsize = 0;
	ehdr->e_shnum = 0;
	ehdr->e_shstrndx = 0;

	phdr = (Elf32_Phdr*)(ehdr + 1);
	phdr[0].p_type = PT_NOTE;

	for (i=0; i<region_no; i++) {
		phdr[i+1].p_type = PT_LOAD;
		phdr[i+1].p_vaddr = regions[i].vstart;
		phdr[i+1].p_paddr = regions[i].pstart;
		phdr[i+1].p_filesz = regions[i].size;
		phdr[i+1].p_memsz = regions[i].size;
		phdr[i+1].p_flags = PF_X | PF_W | PF_R;
	}
	return ehdr;
}

int dump_regions(void)
{
	Elf32_Ehdr *elfhdr_addr;
	
	elfhdr_addr = create_elfhdr(mem_range, (sizeof(mem_range)/sizeof(mem_range[0])));
	if (elfhdr_addr == NULL) {
		debug("Could not create elfhdr\n");
                return 1;
	}

	if (crashdump(elfhdr_addr) != 0) {
		return 1;
	}

	free(elfhdr_addr);
	return 0;
}

/*
 * Dump crash to file (typically FAT file on SD/MMC).
 */
int do_checkcrash(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    //#ifdef DUMP_ALL_MEMORY
    //return dumpModem();
    //#else
	int rc =-1;
	printf("do_checkcrash !!!\n");

	rc = dump_regions();
	printf("dump_regions_from_environment ret %d\n", rc);
	//dump modem memory to file
	//rc = dumpModem();
	//printf("dumpModem ret %d\n", rc);
    //    lcd_puts("system reboot now,please wait...\n");
	udelay(5*1000*1000);
	
	return rc;
    //#endif
}

U_BOOT_CMD(
	checkcrash,	1,	0,	do_checkcrash,
	"dump to file",
	"    - dump crash info to file and stop autoboot\n"
);
