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


#define SYSDUMP_CORE_NAME_FMT 	"sysdump_%s_.core.%02d" /* time, number */
#define NR_KCORE_MEM		80
#define SYSDUMP_MAGIC		"SPRD_SYSDUMP_119"


struct sysdump_mem {
	unsigned long paddr;
	unsigned long vaddr;
	unsigned long soff;
	size_t size;
	int type;
};

struct sysdump_info {
	char magic[16];
	char time[32];
	char dump_path[128];
	int elfhdr_size;
	int mem_num;
	unsigned long dump_mem_paddr;
};

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
	if(ret == -1)
		return;
	extern int lcd_display_bitmap(ulong bmp_image, int x, int y);
	extern lcd_display(void);
	extern void set_backlight(uint32_t value);
	lcd_display_bitmap((ulong)bmp_img, 0, 0);
	lcd_printf("   -------------------------------  \n"
		   "   Sysdumpping now, keep power on.  \n"
		   "   -------------------------------  ");
	lcd_display();
	set_backlight(255);
	set_vibrator(0);

	free(bmp_img);
#endif
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
		return;
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

	printf("writing 0x%lx bytes sysdump info from 0x%p to sd file %s\n",
		memsize, memaddr, filename);
	ret = file_fat_write(filename, memaddr, memsize);
	if (ret <= 0) {
		printf("sd file write error %d\n", ret);
	}

	return;
}

void write_sysdump_before_boot(void)
{
	int i, j, len;
	char fnbuf[72], *path;
	struct rtc_time rtc;
	char *waddr;
	struct sysdump_mem *mem;

	struct sysdump_info *infop = (struct sysdump_info *)SYSDUMP_CORE_HDR;

	printf("check if need to write sysdump info of 0x%08lx to file...\t",
		SYSDUMP_CORE_HDR);

	if(!memcmp(infop->magic, SYSDUMP_MAGIC, sizeof(infop->magic))) {
		printf("\n");

		/* display on screen */
		display_writing_sysdump();

		memset(infop->magic, 0, sizeof(infop->magic));

		if (init_mmc_fat())
			goto FINISH;

		if (strlen(infop->dump_path))
			path = infop->dump_path;
		else
			path = NULL;

		sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, infop->time, 0);
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
				sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_0x%8.8x-0x%8.8x_dump.lst", infop->time, i + 1, mem[i].paddr, mem[i].paddr + mem[i].size -1);
				write_mem_to_mmc(path, fnbuf, waddr, mem[i].size);
		#else
			if (mem[i].size <= SZ_8M) {
				sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, infop->time, i + 1);
				write_mem_to_mmc(path, fnbuf, waddr, mem[i].size);
			} else {
				for (j = 0; j < mem[i].size / SZ_8M; j++) {
					sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_%03d",
						infop->time, i + 1, j);
					write_mem_to_mmc(path, fnbuf, waddr + j * SZ_8M, SZ_8M);
				}

				if (mem[i].size % SZ_8M) {
					sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_%03d",
						infop->time, i + 1, j);
					write_mem_to_mmc(path, fnbuf, waddr + j * SZ_8M,
									(mem[i].size % SZ_8M));
				}
			}
		#endif
		}
#else
		for (i = 0; i < infop->mem_num; i++) {
			sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, infop->time, i + 1);
			write_mem_to_mmc(path, fnbuf, mem[i].paddr, mem[i].size);
		}
#endif

		printf("\nwriting done.\n");
	} else
		printf("no need.\n");

FINISH:
	return;
}
