/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
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

#include "normal_mode.h"
#include <mmc.h>
#include <fat.h>
#include <rtc.h>
#include <asm/sizes.h>
#include <boot_mode.h>
#include <common.h>

#include <linux/keypad.h>
#include <linux/key_code.h>
#include "sysdump.h"



extern int lcd_display_bitmap(ulong bmp_image, int x, int y);
extern lcd_display(void);
extern void set_backlight(uint32_t value);

void display_writing_sysdump(void)
{
	printf("%s\n", __FUNCTION__);
#ifdef CONFIG_SPLASH_SCREEN

	vibrator_hw_init();
	set_vibrator(1);
	//read boot image header
	size_t size = 1<<19;
	char * bmp_img = malloc(size);
	if(!bmp_img){
		printf("not enough memory for splash image\n");
		return;
	}
	int ret = read_logoimg(bmp_img,size);
	if(ret == -1) {
		free(bmp_img);
		return;
	}
	lcd_display_bitmap((ulong)bmp_img, 0, 0);
	lcd_printf("   -------------------------------  \n"
		   "   Sysdumpping now, keep power on.  \n"
		   "   -------------------------------  \n");
	lcd_display();
	set_backlight(255);
	set_vibrator(0);

	free(bmp_img);
#endif
}

static void wait_for_keypress()
{
	int key_code;

	do {
		udelay(50 * 1000);
		key_code = board_key_scan();
		//printf("key_code: %d, (vd:%d,vu:%d,p:%d)\n", key_code, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_POWER);
		if (key_code == KEY_BACK || key_code == KEY_MENU || key_code == KEY_HOME)
			break;
	} while (1);
	printf("Pressed key: %d\n", key_code);
	lcd_printf("Pressed key: %d\n", key_code);
	lcd_display();
}

int init_mmc_fat(void)
{
	struct mmc *mmc;
	block_dev_desc_t *dev_desc = NULL;
	int ret;
	char bufread[50];

	mmc = find_mmc_device(0);
	if(mmc) {
		ret = mmc_init(mmc);
		if(ret < 0){
			printf("mmc init failed %d\n", ret);
			return -1;
		}
	} else {
		printf("no mmc card found\n");
		return -1;
	}

	dev_desc = &mmc->block_dev;
	if(dev_desc==NULL){
		printf("no mmc block device found\n");
		return -1;
	}

	ret = fat_register_device(dev_desc, 1);
	if(ret < 0) {
		printf("fat regist fail %d\n", ret);
		return -1;
	}

	ret = file_fat_detectfs();
	if(ret) {
		printf("detect fs failed\n");
		return -1;
	}

	return 0;
}

void write_mem_to_mmc(char *path, char *filename,
	void *memaddr, unsigned long memsize)
{
	int ret;

	if (path) {
		do {} while (0); /* TODO: jianjun.he */
	}

	printf("writing 0x%lx bytes to sd file %s\n",
		memsize, filename);
	lcd_printf("writing 0x%lx bytes to sd file %s\n", memsize, filename);
	lcd_display();
	ret = file_fat_write(filename, memaddr, memsize);
	if (ret <= 0) {
		printf("sd file write error %d\n", ret);
	}

	return;
}

extern unsigned check_reboot_mode(void);


static size_t get_elfhdr_size(int nphdr)
{
	size_t elfhdr_len;

	elfhdr_len = sizeof(struct elfhdr) +
		(nphdr + 1) * sizeof(struct elf_phdr);
#if SETUP_NOTE
	elfhdr_len += ((sizeof(struct elf_note)) +
		roundup(sizeof(CORE_STR), 4)) * 3 +
		roundup(sizeof(struct elf_prstatus), 4) +
		roundup(sizeof(struct elf_prpsinfo), 4) +
		roundup(sizeof(struct task_struct), 4);
#endif
	elfhdr_len = PAGE_ALIGN(elfhdr_len); //why?

	return elfhdr_len;
}

#if SETUP_NOTE
static int notesize(struct memelfnote *en)
{
	int sz;

	sz = sizeof(struct elf_note);
	sz += roundup((strlen(en->name) + 1), 4);
	sz += roundup(en->datasz, 4);

	return sz;
}

static char *storenote(struct memelfnote *men, char *bufp)
{
	struct elf_note en;

#define DUMP_WRITE(addr,nr) do { memcpy(bufp,addr,nr); bufp += nr; } while(0)

	en.n_namesz = strlen(men->name) + 1;
	en.n_descsz = men->datasz;
	en.n_type = men->type;

	DUMP_WRITE(&en, sizeof(en));
	DUMP_WRITE(men->name, en.n_namesz);

	/* XXX - cast from long long to long to avoid need for libgcc.a */
	bufp = (char*) roundup((unsigned long)bufp,4);
	DUMP_WRITE(men->data, men->datasz);
	bufp = (char*) roundup((unsigned long)bufp,4);

#undef DUMP_WRITE

	return bufp;
}

#endif

static void sysdump_fill_core_hdr(struct pt_regs *regs,
						struct sysdump_mem *sysmem, int mem_num,
						char *bufp, int nphdr, int dataoff)
{
#if 0
	struct elf_prstatus prstatus;	/* NT_PRSTATUS */
	struct elf_prpsinfo prpsinfo;	/* NT_PRPSINFO */
#endif
	struct elf_phdr *nhdr, *phdr;
	struct elfhdr *elf;
	struct memelfnote notes[3];
	off_t offset = 0;
	int i;

	/* setup ELF header */
	elf = (struct elfhdr *) bufp;
	bufp += sizeof(struct elfhdr); //printk("sizeof(struct elfhdr): %d\n");
	offset += sizeof(struct elfhdr); //printk("sizeof(struct elfhdr): %d\n");
	memcpy(elf->e_ident, ELFMAG, SELFMAG); //printk("ELFMAG: %s, SELFMAG:%d\n", ELFMAG, SELFMAG);
	elf->e_ident[EI_CLASS]	= ELF_CLASS;//printk("EI_CLASS:%d, ELF_CLASS: %d", EI_CLASS, ELF_CLASS);
	elf->e_ident[EI_DATA]	= ELF_DATA;//printk("EI_DATA:%");
	elf->e_ident[EI_VERSION]= EV_CURRENT;
	elf->e_ident[EI_OSABI] = ELF_OSABI;
	memset(elf->e_ident+EI_PAD, 0, EI_NIDENT-EI_PAD);
	elf->e_type	= ET_CORE;
	elf->e_machine	= ELF_ARCH;
	elf->e_version	= EV_CURRENT;
	elf->e_entry	= 0;
	elf->e_phoff	= sizeof(struct elfhdr);
	elf->e_shoff	= 0;
	elf->e_flags	= ELF_CORE_EFLAGS;
	elf->e_ehsize	= sizeof(struct elfhdr);
	elf->e_phentsize= sizeof(struct elf_phdr);
	elf->e_phnum	= nphdr;
	elf->e_shentsize= 0;
	elf->e_shnum	= 0;
	elf->e_shstrndx	= 0;

	/* setup ELF PT_NOTE program header */
	nhdr = (struct elf_phdr *) bufp;
	bufp += sizeof(struct elf_phdr);
	offset += sizeof(struct elf_phdr);
	nhdr->p_type	= PT_NOTE;
	nhdr->p_offset	= 0;
	nhdr->p_vaddr	= 0;
	nhdr->p_paddr	= 0;
	nhdr->p_filesz	= 0;
	nhdr->p_memsz	= 0;
	nhdr->p_flags	= 0;
	nhdr->p_align	= 0;

	/* setup ELF PT_LOAD program header for every area */
	for (i = 0; i < mem_num; i++) {
		phdr = (struct elf_phdr *) bufp;
		bufp += sizeof(struct elf_phdr);
		offset += sizeof(struct elf_phdr);

		phdr->p_type	= PT_LOAD;
		phdr->p_flags	= PF_R|PF_W|PF_X;
		phdr->p_offset	= dataoff;
		phdr->p_vaddr	= sysmem[i].vaddr;
		phdr->p_paddr	= sysmem[i].paddr;
		phdr->p_filesz	= phdr->p_memsz	= sysmem[i].size;
		phdr->p_align	= 0;//PAGE_SIZE;
		dataoff += sysmem[i].size;
	}
#if SETUP_NOTE
	/*
	 * Set up the notes in similar form to SVR4 core dumps made
	 * with info from their /proc.
	 */
	nhdr->p_offset	= offset;

	/* set up the process status */
	notes[0].name = CORE_STR;
	notes[0].type = NT_PRSTATUS;
	notes[0].datasz = sizeof(struct elf_prstatus);
	notes[0].data = &prstatus;

	memset(&prstatus, 0, sizeof(struct elf_prstatus));
	//fill_prstatus(&prstatus, current, 0);
	//if (regs)
	//	memcpy(&prstatus.pr_reg, regs, sizeof(*regs));
	//else
	//	crash_setup_regs((struct pt_regs *)&prstatus.pr_reg, NULL);

	nhdr->p_filesz	= notesize(&notes[0]);
	bufp = storenote(&notes[0], bufp);

	/* set up the process info */
	notes[1].name	= CORE_STR;
	notes[1].type	= NT_PRPSINFO;
	notes[1].datasz	= sizeof(struct elf_prpsinfo);
	notes[1].data	= &prpsinfo;

	memset(&prpsinfo, 0, sizeof(struct elf_prpsinfo));
	//fill_psinfo(&prpsinfo, current, current->mm);

	strcpy(prpsinfo.pr_fname, "vmlinux");
	//strncpy(prpsinfo.pr_psargs, saved_command_line, ELF_PRARGSZ);

	nhdr->p_filesz	+= notesize(&notes[1]);
	bufp = storenote(&notes[1], bufp);

	/* set up the task structure */
	notes[2].name	= CORE_STR;
	notes[2].type	= NT_TASKSTRUCT;
	notes[2].datasz	= sizeof(struct task_struct);
	notes[2].data	= current;

	printk("%s: data size is %d, data addr is %p",__func__,notes[2].datasz,notes[2].data);

	nhdr->p_filesz	+= notesize(&notes[2]);
	bufp = storenote(&notes[2], bufp);
#endif
	return;
} /* end elf_kcore_store_hdr() */


void write_sysdump_before_boot(int rst_mode)
{
	int i, j, len;
	char fnbuf[72], *path;
	struct rtc_time rtc;
	char *waddr;
	struct sysdump_mem *mem;
	struct sysdump_info *infop = (struct sysdump_info *)SPRD_SYSDUMP_MAGIC;

	struct sysdump_mem sprd_dump_mem[] = {
		{
			.paddr = CONFIG_PHYS_OFFSET,
			.vaddr = PAGE_OFFSET,
			.soff = 0xffffffff,
			.size = MEM_TOTAL_SIZE,
			.type = SYSDUMP_RAM,
		},
	};

	int sprd_dump_mem_num = 1;

	printf("rst_mode:0x%x, Check if need to write sysdump info of 0x%08lx to file...\t", rst_mode,
		SPRD_SYSDUMP_MAGIC);

	if ((rst_mode == WATCHDOG_REBOOT) || (rst_mode == UNKNOW_REBOOT_MODE) || \
		((rst_mode == PANIC_REBOOT)/* && !memcmp(infop->magic, SYSDUMP_MAGIC, sizeof(infop->magic))*/)) {
		printf("\n");

		memset(infop->magic, 0, sizeof(infop->magic));

#ifdef SYSDUMP_BYPASS
		//printf("infop->crash_key: %d\n", infop->crash_key);
		//if ((rst_mode != PANIC_REBOOT)) {
			printf("skip sysdump for user build\n");
			goto FINISH;
		//}
#endif

		if (init_mmc_fat())
			goto FINISH;

		/* display on screen */
		display_writing_sysdump();
		printf("prepare info for sysdump\n");
		//if ((rst_mode == WATCHDOG_REBOOT) || (rst_mode == UNKNOW_REBOOT_MODE))
		{
			infop->dump_path[0] = '\0';
			infop->mem_num = sprd_dump_mem_num;
			infop->dump_mem_paddr = (unsigned long)sprd_dump_mem;
			strcpy(infop->time, "hw_watchdog");
			infop->elfhdr_size = get_elfhdr_size(infop->mem_num);
			infop->crash_key = 0;

			sysdump_fill_core_hdr(NULL,
					sprd_dump_mem,
					sprd_dump_mem_num,
					(char *)infop + sizeof(*infop),
					infop->mem_num + 1,
					infop->elfhdr_size);
		}

		if (strlen(infop->dump_path))
			path = infop->dump_path;
		else
			path = NULL;

		sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, 0);
		write_mem_to_mmc(path, fnbuf,
			(char *)infop + sizeof(*infop), infop->elfhdr_size);

#if 1 /* TODO: jianjun.he */
		mem = (struct sysdump_mem *)infop->dump_mem_paddr;
		for (i = 0; i < infop->mem_num; i++) {
			if (0xffffffff != mem[i].soff)
				waddr = (char *)infop + sizeof(*infop) +
						infop->elfhdr_size + mem[i].soff;
			else
				waddr = mem[i].paddr;

		#ifdef CONFIG_RAMDUMP_NO_SPLIT
				sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_0x%8.8x-0x%8.8x_dump.lst", i + 1, mem[i].paddr, mem[i].paddr + mem[i].size -1);
				write_mem_to_mmc(path, fnbuf, waddr, mem[i].size);
		#else
			if (mem[i].size <= SZ_8M) {
				sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, /*infop->time,*/ i + 1);
				write_mem_to_mmc(path, fnbuf, waddr, mem[i].size);
			} else {
				for (j = 0; j < mem[i].size / SZ_8M; j++) {
					sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_%03d",
						/*infop->time,*/ i + 1, j);
					write_mem_to_mmc(path, fnbuf, waddr + j * SZ_8M, SZ_8M);
				}

				if (mem[i].size % SZ_8M) {
					sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_%03d",
						/*infop->time,*/ i + 1, j);
					write_mem_to_mmc(path, fnbuf, waddr + j * SZ_8M,
									(mem[i].size % SZ_8M));
				}
			}
		#endif
		}
#else
		for (i = 0; i < infop->mem_num; i++) {
			sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, /*infop->time,*/ i + 1);
			write_mem_to_mmc(path, fnbuf, mem[i].paddr, mem[i].size);
		}
#endif

		printf("\nwriting done.\nPress any key to continue...");
		lcd_printf("\nWriting done.\nPress any key (Exp power key) to continue...");
		lcd_display();
		wait_for_keypress();
	} else
		printf("no need.\n");

FINISH:
	return;
}
