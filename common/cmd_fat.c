/*
 * (C) Copyright 2002
 * Richard Jones, rjones@nexus-tech.net
 *
 * Keith Outwater (outwater4@comcast.net) - add additional
 *   FAT commands for use with rockbox (www.rockbox.org) FAT
 *   filesystem driver.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <s_record.h>
#include <net.h>
#include <ata.h>
#include <part.h>
#include <fat.h>

#ifdef CONFIG_ROCKBOX_FAT
int do_fat_fsload (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
#else
int do_fat_fsload (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
#endif
{
	long size;
	unsigned long offset;
	unsigned long count;
	char buf [12];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 5) {
		printf( "usage: fatload <interface> <dev[:part]> "
			"<addr> <filename> [bytes]\n");
		return 1;
	}

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fatload **\n",
			argv[1], dev, part);
		return 1;
	}
	offset = simple_strtoul(argv[3], NULL, 16);
	if (argc == 6)
		count = simple_strtoul(argv[5], NULL, 16);
	else
		count = 0;
	size = file_fat_read(argv[4], (unsigned char *)offset, count);

	if(size==-1) {
		printf("\n** Unable to read \"%s\" from %s %d:%d **\n",
			argv[4], argv[1], dev, part);
		return 1;
	}

	printf("\n%ld bytes read\n", size);

	sprintf(buf, "%lX", size);
	setenv("filesize", buf);

	return 0;
}


U_BOOT_CMD(
	fatload,	6,	0,	do_fat_fsload,
	"load binary file from a dos filesystem",
	"<interface> <dev[:part]>  <addr> <filename> [bytes]\n"
	"    - load binary file 'filename' from 'dev' on 'interface'\n"
	"      to address 'addr' from dos filesystem"
);


#ifdef CONFIG_ROCKBOX_FAT
int
do_fat_fswrite (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	long size;
	unsigned long offset;
	unsigned long count;
	char buf[12];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 6) {
		printf ("usage: fatwrite <interface> <dev[:part]> "
				"<addr> <filename> <bytes>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatwrite **\n",
				argv[1],dev,part);
		return 1;
	}

	offset = simple_strtoul (argv[3], NULL, 16);
	count = simple_strtoul (argv[5], NULL, 16);
	size = file_fat_write(argv[4], (unsigned char *)offset, count);

	if (size <= 0) {
		printf("\n** Unable to write\"%s\" to %s %d:%d **\n",
			   argv[4],argv[1],dev,part);
		return 1;
	}

	printf ("\n%ld bytes written\n", size);
	sprintf(buf, "0x%lX", size);
	setenv("filesize", buf);
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatwrite,	6,	0,	do_fat_fswrite,
	"store a binary file to a dos filesystem\n",
	"<interface> <dev[:part]> <addr> <filename> <bytes>\n"
	"    - store binary file 'filename' to 'dev' on 'interface'\n"
	"      containing a dos filesystem from address 'addr'\n"
	"      with length 'bytes'\n"
);
#endif

int
do_fat_fsrm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char buf[256];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 4) {
		printf ("usage: fatrm <interface> <dev[:part]> <filename>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatrm **\n",
				argv[1],dev,part);
		return 1;
	}

	if (file_fat_rm(argv[3]) < 0) {
		printf("\n** Unable to remove \"%s\" **\n", argv[3]);
		return 1;
	}

	strncpy(buf, argv[3], sizeof(buf));
	setenv("rm_filename", buf);
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatrm,	4,	0,	do_fat_fsrm,
	"remove a file in a FAT16/32 filesystem\n",
	"<interface> <dev[:part]> <filename>\n"
	"    - remove 'filename' in the FAT16/32 filesytem\n"
	"      on 'dev' on 'interface'\n"
);
#endif

int
do_fat_fsmkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char buf[256];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 4) {
		printf ("usage: fatmkdir <interface> <dev[:part]> <dirname>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatmkdir **\n",
				argv[1],dev,part);
		return 1;
	}

	if (file_fat_mkdir(argv[3]) < 0) {
		printf("\n** Unable to make dir \"%s\" **\n", argv[3]);
		return 1;
	}

	strncpy(buf, argv[3], sizeof(buf));
	setenv("mkdir_dirname", buf);
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatmkdir,	4,	0,	do_fat_fsmkdir,
	"make a directory in a FAT16/32 filesystem\n",
	"<interface> <dev[:part]> <dirname>\n"
	"    - make 'dirname' in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
);
#endif

int
do_fat_fsrmdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char buf[256];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 4) {
		printf ("usage: fatrmdir <interface> <dev[:part]> <dirname>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatrmdir **\n",
				argv[1],dev,part);
		return 1;
	}

	if (file_fat_rmdir(argv[3]) < 0) {
		printf("\n** Unable to remove dir \"%s\" **\n", argv[3]);
		return 1;
	}

	strncpy(buf, argv[3], sizeof(buf));
	setenv("rm_dirname", buf);
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatrmdir,	4,	0,	do_fat_fsrmdir,
	"remove an empty directory in a FAT16/32 filesystem\n",
	"<interface> <dev[:part]> <dirname>\n"
	"    - remove 'dirname' in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
);
#endif

int
do_fat_fscd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char buf[256];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 4) {
		printf ("usage: fatcd <interface> <dev[:part]> <dirname>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatcd **\n",
				argv[1],dev,part);
		return 1;
	}

	if (file_fat_cd(argv[3]) < 0) {
		printf("\n** Unable to change to dir \"%s\" **\n", argv[3]);
		return 1;
	}

	strncpy(buf, argv[3], sizeof(buf));
	setenv("cwd", buf);
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatcd,	4,	0,	do_fat_fscd,
	"change to the specified dir in a FAT16/32 filesystem\n",
	"<interface> <dev[:part]> <dirname>\n"
	"    - change to 'dirname' in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
);
#endif

int
do_fat_fspwd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 3) {
		printf ("usage: fatpwd <interface> <dev[:part]>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatpwd **\n",
				argv[1],dev,part);
		return 1;
	}

	file_fat_pwd();
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatpwd,	3,	0,	do_fat_fspwd,
	"print the current working dir in a FAT16/32 filesystem\n",
	"<interface> <dev[:part]>\n"
	"    - print the CWD in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
);
#endif

int
do_fat_fsmv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char buf[256];
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;

	if (argc < 5) {
		printf ("usage: fatmv <interface> <dev[:part]> <oldname> <newname>\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatmv **\n",
				argv[1],dev,part);
		return 1;
	}

	if (file_fat_mv(argv[3], argv[4]) < 0) {
		printf("\n** Unable to change \"%s\" to \"%s\" **\n",
			   argv[3], argv[4]);
		return 1;
	}

	strncpy(buf, argv[3], sizeof(buf));
	setenv("filename", buf);
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatmv,	5,	0,	do_fat_fsmv,
	"move a file in a FAT16/32 filesystem\n",
	"<interface> <dev[:part]> <oldname> <newname>\n"
	"    - change <oldname> to <newname> in the FAT16/32 filesytem\n"
	"      on 'dev' on 'interface'\n"
);
#endif

int
do_fat_free (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	block_dev_desc_t *dev_desc=NULL;
	int dev=0;
	int part=1;
	char *ep;
	unsigned int size;

	if (argc < 3) {
		printf ("usage: fatfree <interface> <dev[:part]> [<size>]\n");
		return 1;
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid device **\n");
		return 1;
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (fat_register_device(dev_desc,part)!=0) {
		printf ("\n** Unable to use %s %d:%d for fatmv **\n",
				argv[1],dev,part);
		return 1;
	}

	if(argc>=4) {
		size = simple_strtoul(argv[3], NULL, 10);
		printf("Scanning FAT partition for ");
		if      (size>0x00100000) printf("%d.%02dMB", (int)(size>>20), (int)(size&0x000FFFFF)>>10);
		else if (size>0x00000400) printf("%d.%02dkB", (int)(size>>10), (int)(size&0x000003FF));
		else                      printf("%d Byte", (int)size);
		printf(" of free space.\n");
		if (rockbox_fat_free(size>>10)) {
			printf("Enougth free space found.\n");
			return 0;
		} else {
			printf("Not enougth free space found in partition.\n");
			return 1;
		}
	}
	{	unsigned long size;
		printf("Scanning full FAT partition for free clusters...\n");
		size = rockbox_fat_size();
		printf("FAT filesystem:  total size = ");
		if      (size>=0x00100000) printf("%d.%02dGB", (int)(size>>20), (int)(size&0x000FFFFF)>>10);
		else if (size>=0x00000400) printf("%d.%02dMB", (int)(size>>10), (int)(size&0x000003FF));
		else                       printf("%dkB", (int)size);
		printf(" /  free size = ");
		size = rockbox_fat_free(0);
		if      (size>=0x00100000) printf("%d.%02dGB", (int)(size>>20), (int)(size&0x000FFFFF)>>10);
		else if (size>=0x00000400) printf("%d.%02dMB", (int)(size>>10), (int)(size&0x000003FF));
		else                       printf("%dkB", (int)size);
		putc('\n');
	}
	return 0;
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatfree,	4,	0,	do_fat_free,
	"print free space\n",
	"<interface> <dev[:part]> [size]\n"
	"    - print free sapce available in filesystem.\n"
	"      If <size> not specified, full partition is scanned.\n"
	"      If <size> is specified, return true if enougth free space found.\n"
);
#endif


#endif /* #ifdef CONFIG_ROCKBOX_FAT */
#ifdef CONFIG_ROCKBOX_FAT
int do_fat_ls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
#else
int do_fat_ls (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
#endif 
{
	char *filename = "/";
	int ret;
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;

	if (argc < 3) {
		printf("usage: fatls <interface> <dev[:part]> [directory]\n");
		return 0;
	}
	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fatls **\n",
			argv[1], dev, part);
		return 1;
	}
	if (argc == 4)
		ret = file_fat_ls(argv[3]);
	else
		ret = file_fat_ls(filename);

	if(ret!=0)
		printf("No Fat FS detected\n");
	return ret;
}

U_BOOT_CMD(
	fatls,	4,	1,	do_fat_ls,
	"list files in a directory (default /)",
	"<interface> <dev[:part]> [directory]\n"
	"    - list files from 'dev' on 'interface' in a 'directory'"
);
#ifdef CONFIG_ROCKBOX_FAT
int do_fat_fsinfo (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
#else
int do_fat_fsinfo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
#endif
{
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;

	if (argc < 2) {
		printf("usage: fatinfo <interface> <dev[:part]>\n");
		return 0;
	}
	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}
	if (fat_register_device(dev_desc,part)!=0) {
		printf("\n** Unable to use %s %d:%d for fatinfo **\n",
			argv[1], dev, part);
		return 1;
	}
	return file_fat_detectfs();
}

#ifndef CONFIG_CMD_TREE_FAT
U_BOOT_CMD(
	fatinfo,	3,	1,	do_fat_fsinfo,
	"print information about filesystem",
	"<interface> <dev[:part]>\n"
	"    - print information about filesystem from 'dev' on 'interface'"
);
#endif

#ifdef NOT_IMPLEMENTED_YET
/* find first device whose first partition is a DOS filesystem */
int find_fat_partition (void)
{
	int i, j;
	block_dev_desc_t *dev_desc;
	unsigned char *part_table;
	unsigned char buffer[ATA_BLOCKSIZE];

	for (i = 0; i < CONFIG_SYS_IDE_MAXDEVICE; i++) {
		dev_desc = ide_get_dev (i);
		if (!dev_desc) {
			debug ("couldn't get ide device!\n");
			return (-1);
		}
		if (dev_desc->part_type == PART_TYPE_DOS) {
			if (dev_desc->
				block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
				debug ("can't perform block_read!\n");
				return (-1);
			}
			part_table = &buffer[0x1be];	/* start with partition #4 */
			for (j = 0; j < 4; j++) {
				if ((part_table[4] == 1 ||	/* 12-bit FAT */
				     part_table[4] == 4 ||	/* 16-bit FAT */
				     part_table[4] == 6) &&	/* > 32Meg part */
				    part_table[0] == 0x80) {	/* bootable? */
					curr_dev = i;
					part_offset = part_table[11];
					part_offset <<= 8;
					part_offset |= part_table[10];
					part_offset <<= 8;
					part_offset |= part_table[9];
					part_offset <<= 8;
					part_offset |= part_table[8];
					debug ("found partition start at %ld\n", part_offset);
					return (0);
				}
				part_table += 16;
			}
		}
	}

	debug ("no valid devices found!\n");
	return (-1);
}

int
do_fat_dump (cmd_tbl_t *cmdtp, bd_t *bd, int flag, int argc, char *argv[])
{
	__u8 block[1024];
	int ret;
	int bknum;

	ret = 0;

	if (argc != 2) {
		printf ("needs an argument!\n");
		return (0);
	}

	bknum = simple_strtoul (argv[1], NULL, 10);

	if (disk_read (0, bknum, block) != 0) {
		printf ("Error: reading block\n");
		return -1;
	}
	printf ("FAT dump: %d\n", bknum);
	hexdump (512, block);

	return (ret);
}

int disk_read (__u32 startblock, __u32 getsize, __u8 *bufptr)
{
	ulong tot;
	block_dev_desc_t *dev_desc;

	if (curr_dev < 0) {
		if (find_fat_partition () != 0)
			return (-1);
	}

	dev_desc = ide_get_dev (curr_dev);
	if (!dev_desc) {
		debug ("couldn't get ide device\n");
		return (-1);
	}

	tot = dev_desc->block_read (0, startblock + part_offset,
				    getsize, (ulong *) bufptr);

	/* should we do this here?
	   flush_cache ((ulong)buf, cnt*ide_dev_desc[device].blksz);
	 */

	if (tot == getsize)
		return (0);

	debug ("unable to read from device!\n");

	return (-1);
}


static int isprint (unsigned char ch)
{
	if (ch >= 32 && ch < 127)
		return (1);

	return (0);
}


void hexdump (int cnt, unsigned char *data)
{
	int i;
	int run;
	int offset;

	offset = 0;
	while (cnt) {
		printf ("%04X : ", offset);
		if (cnt >= 16)
			run = 16;
		else
			run = cnt;
		cnt -= run;
		for (i = 0; i < run; i++)
			printf ("%02X ", (unsigned int) data[i]);
		printf (": ");
		for (i = 0; i < run; i++)
			printf ("%c", isprint (data[i]) ? data[i] : '.');
		printf ("\n");
		data = &data[16];
		offset += run;
	}
}
#endif	/* NOT_IMPLEMENTED_YET */


#ifdef CONFIG_CMD_TREE_FAT
/* replace all fatXXX commands with 'fat xxx' command.
 */
int do_fat(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int (*fn)(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
	fn = NULL;
	argc--;
	argv++;

	if      (!strncmp(argv[0], "load", 5)) fn = do_fat_fsload;
	else if (!strncmp(argv[0], "ls",   2)) fn = do_fat_ls;
	else if (!strncmp(argv[0], "info", 5)) fn = do_fat_fsinfo;
#ifdef CONFIG_ROCKBOX_FAT
	else if (!strncmp(argv[0], "write", 6)) fn = do_fat_fswrite;
	else if (!strncmp(argv[0], "rm",    3)) fn = do_fat_fsrm;
	else if (!strncmp(argv[0], "mkdir", 6)) fn = do_fat_fsmkdir;
	else if (!strncmp(argv[0], "rmdir", 6)) fn = do_fat_fsrmdir;
	else if (!strncmp(argv[0], "cd",    3)) fn = do_fat_fscd;
	else if (!strncmp(argv[0], "pwd",   4)) fn = do_fat_fspwd;
	else if (!strncmp(argv[0], "mv",    3)) fn = do_fat_fsmv;
	else if (!strncmp(argv[0], "free",  5)) fn = do_fat_free;
#endif
	if (fn) {
		if(fn(cmdtp, flag, argc, argv) == 0) return 0;
		return 1;
	}
	printf("unknown 'fat' command, check 'help fat'\n");
	return 1;
}

U_BOOT_CMD(
	fat, 7, 0, do_fat,
	"some fat support commands\n",
	"\n"
	"fat load <interface> <dev[:part]> <addr> <filename> [bytes]\n"
	"    - load binary file 'filename' from 'dev' on 'interface'\n"
	"      to address 'addr' from dos filesystem\n"
	"\n"
	"fat ls   <interface> <dev[:part]> [directory]\n"
	"    - list files from 'dev' on 'interface' in a 'directory'\n"
	"\n"
	"fat info <interface> <dev[:part]>\n"
	"    - print information about filesystem from 'dev' on 'interface'\n"
	"\n"
#ifdef CONFIG_ROCKBOX_FAT
	"fat write <interface> <dev[:part]> <addr> <filename> <bytes>\n"
	"    - store binary file 'filename' to 'dev' on 'interface'\n"
	"      containing a dos filesystem from address 'addr'\n"
	"      with length 'bytes'\n"
	"\n"
	"fat rm    <interface> <dev[:part]> <filename>\n"
	"    - remove 'filename' in the FAT16/32 filesytem\n"
	"      on 'dev' on 'interface'\n"
	"\n"
	"fat mkdir <interface> <dev[:part]> <dirname>\n"
	"    - make 'dirname' in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
	"\n"
	"fat rmdir <interface> <dev[:part]> <dirname>\n"
	"    - remove 'dirname' in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
	"\n"
	"fat cd    <interface> <dev[:part]> <dirname>\n"
	"    - change to 'dirname' in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
	"\n"
	"fat pwd   <interface> <dev[:part]>\n"
	"    - print the CWD in the FAT16/32 filesytem on 'dev'\n"
	"      on 'interface'\n"
	"\n"
	"fat mv    <interface> <dev[:part]> <oldname> <newname>\n"
	"    - change <oldname> to <newname> in the FAT16/32 filesytem\n"
	"      on 'dev' on 'interface'\n"
	"\n"
	"fat free  <interface> <dev[:part]> \n"
	"    - print free sapce available in filesystem (full partition scan)\n"
#endif
);
#endif
