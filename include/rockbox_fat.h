/***************************************************************************
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************
 * Source file dumped from rockbox-3.1 distribution.
 * $Id: fat.h 18960 2008-11-01 16:14:28Z gevaerts $
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 ****************************************************************************
 * See file CREDITS for list of people who contributed to the U-boot 
 * project.
 *
 * 01/17/2006 Keith Outwater (outwater4@comcast.net) - port to U-Boot using
 *     CVS version 1.13 of 'firmware/export/fat.h' from rockbox CVS server.
 * 09-jan-2008 etienne.carriere@stnwireless.com - update from rockbox-3.1
 ****************************************************************************
 */
#ifndef ROCKBOX_FAT_H
#define ROCKBOX_FAT_H

#include <stdbool.h>
#include "rockbox_mv.h" /* for volume definitions */

/* Check that sector size is 512 bytes */
#ifndef SECTOR_SIZE 
#define SECTOR_SIZE 512
#elif  (SECTOR_SIZE != 512)
#error "SECTOR_SIZE expected to be 512"
#endif  
/*
 * The max length of the current working dorectory.
 */
#define	ROCKBOX_CWD_LEN	512

/* Number of bytes reserved for a file name (including the trailing \0).
   Since names are stored in the entry as UTF-8, we won't be able to
   store all names allowed by FAT. In FAT, a name can have max 255
   characters (not bytes!). Since the UTF-8 encoding of a char may take
   up to 4 bytes, there will be names that we won't be able to store
   completely. For such names, the short DOS name is used. */
#define FAT_FILENAME_BYTES 256

struct fat_direntry
{
    unsigned char name[FAT_FILENAME_BYTES]; /* UTF-8 encoded name plus \0 */
	unsigned short attr;		/* Attributes */
    unsigned char crttimetenth;     /* Millisecond creation
                                       time stamp (0-199) */
	unsigned short crttime;		/* Creation time */
	unsigned short crtdate;		/* Creation date */
	unsigned short lstaccdate;	/* Last access date */
	unsigned short wrttime;		/* Last write time */
	unsigned short wrtdate;		/* Last write date */
	unsigned long filesize;		/* File size in bytes */
	long firstcluster;		/* fstclusterhi<<16 + fstcluslo */
};

#define FAT_ATTR_READ_ONLY   0x01
#define FAT_ATTR_HIDDEN      0x02
#define FAT_ATTR_SYSTEM      0x04
#define FAT_ATTR_VOLUME_ID   0x08
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_ARCHIVE     0x20
#define FAT_ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

struct fat_file
{
	long firstcluster;	/* first cluster in file */
	long lastcluster;	/* cluster of last access */
	long lastsector;	/* sector of last access */
	long clusternum;	/* current clusternum */
	long sectornum;		/* sector number in this cluster */
	unsigned int direntry;	/* short dir entry index from start of dir */
    unsigned int direntries; /* number of dir entries used by this file */
	long dircluster;	/* first cluster of dir */
	bool eof;
#ifdef HAVE_MULTIVOLUME
    int volume;          /* file resides on which volume */
#endif
};

struct fat_dir
{
	unsigned int entry;
	unsigned int entrycount;
	long sector;
	struct fat_file file;
	unsigned char sectorcache[3][SECTOR_SIZE];
};

#ifdef HAVE_HOTSWAP
extern void fat_lock(void);
extern void fat_unlock(void);
#endif

extern void fat_init(void);
extern int fat_mount(IF_MV2(int volume,) IF_MV2(int drive,) long startsector);
extern int fat_unmount(int volume, bool flush);
extern void fat_size(IF_MV2(int volume,) /* public for info */
                     unsigned long* size,
                     unsigned long* free);
extern int fat_check_free(IF_MV2(int volume,) unsigned long size);
extern void fat_recalc_free(IF_MV_NONVOID(int volume)); /* public for debug info screen */
extern int fat_create_dir(const char *name,
                          struct fat_dir* newdir,
                          struct fat_dir* dir);
extern int fat_open(IF_MV2(int volume,)
                    long cluster,
                    struct fat_file* ent,
                    const struct fat_dir* dir);
extern int fat_create_file(const char *name,
                           struct fat_file* ent,
                           struct fat_dir* dir);
extern long fat_readwrite(struct fat_file *ent, long sectorcount,
			  void *buf, bool write);
extern int fat_closewrite(struct fat_file *ent, long size, int attr);
extern int fat_seek(struct fat_file *ent, unsigned long sector);
extern int fat_remove(struct fat_file *ent);
extern int fat_truncate(const struct fat_file *ent);
extern int fat_rename(struct fat_file *file,
		      struct fat_dir *dir,
                      const unsigned char* newname,
                      long size, int attr);

extern int fat_opendir(IF_MV2(int volume,)
                       struct fat_dir *ent, unsigned long currdir,
		       const struct fat_dir *parent_dir);
extern int fat_getnext(struct fat_dir *ent, struct fat_direntry *entry);
extern unsigned int fat_get_cluster_size(IF_MV_NONVOID(int volume)); /* public for debug info screen */
extern bool fat_ismounted(int volume);

#ifndef _ROCKBOX_FILE_H_
#include <rockbox_file.h>
#endif

#ifndef _ROCKBOX_DIR_H
#include <rockbox_dir.h>
#endif

/* Added to rockbox_wrapper.c */
unsigned int rockbox_fat_size(void);
unsigned int rockbox_fat_free(unsigned long size);


#endif				/* #ifndef ROCKBOX_FAT_H */
