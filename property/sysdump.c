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

#define SYSDUMP_CORE_NAME_FMT 	"sysdump_%s_.core.%d" /* time, number */
#define SYSDUMP_CORE_HDR  		0x8e300000  /* SPRD_IO_MEM_BASE in kernel */
#define NR_KCORE_MEM			8
#define SYSDUMP_MAGIC			"SPRD_SYSDUMP_119"

struct sysdump_mem {
	unsigned long paddr;
	unsigned long vaddr;
	size_t size;
	int type;
};

struct sysdump_info {
	char magic[16];
	char time[32];
	char dump_path[128];
	int elfhdr_size;
	char *elfhdr;
	int mem_num;
	struct sysdump_mem mem[NR_KCORE_MEM];
};


void write_mem_to_mmc(char *path, char *filename, void *memaddr, unsigned long memsize)
{
	struct mmc *mmc;
	block_dev_desc_t *dev_desc = NULL;
	int ret;
	char bufread[50];
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

	if (path)
	{
		do {} while (0); /* TODO: jianjun.he */
	}

	printf("writing %d bytes sysdump info from 0x%p to sd file %s\n", memsize, memaddr, filename);
	ret = file_fat_write(filename, memaddr, memsize);
	if(ret <= 0){
		printf("sd file write error %d\n", ret);
	}

	return;
}

void write_sysdump_before_boot(void)
{
	int i, j, len;
	char fnbuf[72], *path;
	struct rtc_time rtc;

	struct sysdump_info *infop = (struct sysdump_info *)SYSDUMP_CORE_HDR;

	printf("check if need to write sysdump info of 0x%08lx to file...\t", SYSDUMP_CORE_HDR);

	if(!memcmp(infop->magic, SYSDUMP_MAGIC, sizeof(infop->magic)))
	{
		printf("\n");

		if (strlen(infop->dump_path))
			path = infop->dump_path;
		else
			path = NULL;

		sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, infop->time, 0);
		write_mem_to_mmc(path, fnbuf, (char *)infop + sizeof(*infop), infop->elfhdr_size);
#if 1 /* TODO: jianjun.he */
		for (i = 0; i < infop->mem_num; i++) {
			for (j = 0; j < infop->mem[i].size / 0x00400000; j++) {
				sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT"_%03d", infop->time, i + 1, j);
				write_mem_to_mmc(path, fnbuf, infop->mem[i].paddr + j * 0x00400000, 0x00400000);
			}
		}
#else
		for (i = 0; i < infop->mem_num; i++) {
			sprintf(fnbuf, SYSDUMP_CORE_NAME_FMT, timebuf, i + 1);
			write_mem_to_mmc(path, fnbuf, infop->mem[i].paddr, infop->mem[i].size);
		}
#endif
		memset(infop->magic, 0, sizeof(infop->magic));
	} else
		printf("no need.\n");

	return;
}
