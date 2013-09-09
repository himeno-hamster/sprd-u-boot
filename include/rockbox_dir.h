/***************************************************************************
 * Copyright (C) 2002 by Björn Stenberg
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
 * $Id: dir.h 13741 2007-06-30 02:08:27Z jethead71 $
 * Copyright (C) 2002 by Björn Stenberg
 ****************************************************************************
 * See file CREDITS for list of people who contributed to the U-boot 
 * project.
 *
 * 01/17/2006 Keith Outwater (outwater4@comcast.net) - port to U-Boot using
 *     CVS version 1.12 of 'firmware/common/dir.h' from rockbox CVS server.
 * 01/07/2008 etienne.carriere@stnwireless.com - update u-boot port from rockbox-3.1
 ***************************************************************************
 */
#ifndef _ROCKBOX_DIR_H_
#define _ROCKBOX_DIR_H_

#ifdef HAVE_DIRCACHE
#error ""
# include "dircache.h"
# define DIR DIR_CACHED
# define dirent dircache_entry
# define opendir opendir_cached
# define closedir closedir_cached
# define readdir readdir_cached
# define closedir closedir_cached
# define mkdir mkdir_cached
# define rmdir rmdir_cached
#else
/*#include "dir_uncached.h"  U-boot port: dir-uncached.h and dir.h current melded */
# define DIR DIR_UNCACHED
# define dirent dirent_uncached
# define opendir opendir_uncached
# define closedir closedir_uncached
# define readdir readdir_uncached
# define closedir closedir_uncached
# define mkdir mkdir_uncached
# define rmdir rmdir_uncached
#endif

#include <stdbool.h>
#include <rockbox_file.h>

#define ATTR_READ_ONLY   0x01
#define ATTR_HIDDEN      0x02
#define ATTR_SYSTEM      0x04
#define ATTR_VOLUME_ID   0x08
#define ATTR_DIRECTORY   0x10
#define ATTR_ARCHIVE     0x20
#define ATTR_VOLUME      0x40 /* this is a volume, not a real directory */

#ifdef SIMULATOR
#define dirent_uncached sim_dirent
#define DIR_UNCACHED SIM_DIR
#define opendir_uncached sim_opendir
#define readdir_uncached sim_readdir
#define closedir_uncached sim_closedir
#define mkdir_uncached sim_mkdir
#define rmdir_uncached sim_rmdir
#endif
#ifndef DIRENT_DEFINED

struct dirent {
	unsigned char d_name[MAX_PATH];
	int attribute;
	long size;
	long startcluster;
	unsigned short wrtdate;	/* Last write date */
	unsigned short wrttime;	/* Last write time */
};
#endif

#include <rockbox_fat.h>

typedef struct {
#ifndef SIMULATOR
	bool busy;
	long startcluster;
	struct fat_dir fatdir;
	struct fat_dir parent_dir;
    struct dirent_uncached theent;
#ifdef HAVE_MULTIVOLUME
    int volumecounter; /* running counter for faked volume entries */
#endif
#else
    /* simulator: */
    void *dir; /* actually a DIR* dir */
    char *name;
#endif
} DIR_UNCACHED;

#ifdef HAVE_HOTSWAP
char *get_volume_name(int volume);
#endif

#ifdef HAVE_MULTIVOLUME
    int strip_volume(const char*, char*);
#endif

#ifndef DIRFUNCTIONS_DEFINED

extern DIR_UNCACHED* opendir_uncached(const char* name);
extern int closedir_uncached(DIR_UNCACHED* dir);
extern int mkdir_uncached(const char* name);
extern int rmdir_uncached(const char* name);

extern struct dirent_uncached* readdir_uncached(DIR_UNCACHED* dir);

extern int release_dirs(int volume);

#endif				/* DIRFUNCTIONS_DEFINED */

#endif				/* #ifndef _ROCKBOX_DIR_H_ */
