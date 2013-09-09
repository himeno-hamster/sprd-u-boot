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
 * $Id: dir.c 13741 2007-06-30 02:08:27Z jethead71 $
 * Copyright (C) 2002 by Björn Stenberg
 ****************************************************************************
 * See file CREDITS for list of people who contributed to the U-boot 
 * project.
 *
 * 01/17/2006 Keith Outwater (outwater4@comcast.net) - port to U-Boot using
 *     CVS version 1.29 of 'firmware/common/dir.c' from rockbox CVS server.
 * 01/07/2009 etienne.carriere@stnwireless.com - update u-boot port from rockbox-3.1
 ****************************************************************************
 */
#include <common.h>
#include <config.h>

#include <asm/errno.h>
#include <linux/string.h>
#include <fat.h>
#include "rockbox_debug.h"

extern int errno;	/* see board/<board>/<board>.c */
extern int strcasecmp(const char *s1, const char *s2); /* from rockbox_wrapper.c */

#ifndef __HAVE_ARCH_STRNICMP
extern int strnicmp(const char *s1, const char *s2, size_t len);
#endif

#if !defined(CONFIG_ROCKBOX_FAT_MAX_OPEN_DIRS)
#define CONFIG_ROCKBOX_FAT_MAX_OPEN_DIRS 8
#endif

static DIR_UNCACHED opendirs[CONFIG_ROCKBOX_FAT_MAX_OPEN_DIRS];

#define DEBUGF fat_dprintf		/* map debug on old fat_dprintf from rockbox_debug.h */ 

#ifdef HAVE_MULTIVOLUME
/* @brief returns on which volume this is, and copies the reduced name
   (sortof a preprocessor for volume-decorated pathnames) 
*/
int strip_volume(const char* name, char* namecopy)
{
    int volume = 0;
    const char *temp = name;
    
    while (*temp == '/')          /* skip all leading slashes */
        ++temp;
        
    if (*temp && !strncmp(temp, VOL_NAMES, VOL_ENUM_POS))
    {
        temp += VOL_ENUM_POS;     /* behind special name */
        volume = atoi(temp);      /* number is following */
        temp = strchr(temp, '/'); /* search for slash behind */
        if (temp != NULL)
            name = temp;          /* use the part behind the volume */
        else
            name = "/";           /* else this must be the root dir */
    }

    strncpy(namecopy, name, MAX_PATH);
    namecopy[MAX_PATH-1] = '\0';

    return volume;
}
#endif /* #ifdef HAVE_MULTIVOLUME */


#ifdef HAVE_HOTSWAP
// release all dir handles on a given volume "by force", to avoid leaks
int release_dirs(int volume)
{
    DIR_UNCACHED* pdir = opendirs;
    int dd;
    int closed = 0;
    for ( dd=0; dd<CONFIG_ROCKBOX_FAT_MAX_OPEN_DIRS; dd++, pdir++)
    {
        if (pdir->fatdir.file.volume == volume)
        {
            pdir->busy = false; /* mark as available, no further action */
            closed++;
        }
    }
    return closed; /* return how many we did */
}
#endif /* #ifdef HAVE_HOTSWAP */

DIR_UNCACHED* opendir_uncached(const char* name)
{
	char namecopy[MAX_PATH];
	char *part;
    char* end;
	struct fat_direntry entry;
	int dd;
    DIR_UNCACHED* pdir = opendirs;
#ifdef HAVE_MULTIVOLUME
    int volume;
#endif

	if (name[0] != '/') {
        DEBUGF("Only absolute paths supported right now\n");
		return NULL;
	}

    /* find a free dir descriptor */
    for ( dd=0; dd<CONFIG_ROCKBOX_FAT_MAX_OPEN_DIRS; dd++, pdir++)
		if (!pdir->busy)
			break;

    if (dd == CONFIG_ROCKBOX_FAT_MAX_OPEN_DIRS) {
        DEBUGF("Too many dirs open\n");
		errno = EMFILE;
		return NULL;
	}

	pdir->busy = true;

#ifdef HAVE_MULTIVOLUME
    /* try to extract a heading volume name, if present */
    volume = strip_volume(name, namecopy);
    pdir->volumecounter = 0;
#else
	strncpy(namecopy, name, sizeof (namecopy));	/* just copy */
	namecopy[sizeof (namecopy) - 1] = '\0';
#endif

    if ( fat_opendir(IF_MV2(volume,) &pdir->fatdir, 0, NULL) < 0 ) {
        DEBUGF("Failed opening root dir\n");
		pdir->busy = false;
		return NULL;
	}

    for ( part = strtok_r(namecopy, "/", &end); part;
          part = strtok_r(NULL, "/", &end)) {
        /* scan dir for name */
		while (1) {
			if ((fat_getnext(&pdir->fatdir, &entry) < 0) ||
			    (!entry.name[0])) {
				pdir->busy = false;
				return NULL;
			}
			if ((entry.attr & FAT_ATTR_DIRECTORY) &&
                 (!strcasecmp(part, (char*)entry.name)) ) {
				pdir->parent_dir = pdir->fatdir;
                if ( fat_opendir(IF_MV2(volume,)
                                 &pdir->fatdir,
						entry.firstcluster,
						&pdir->parent_dir) < 0) {
                    DEBUGF("Failed opening dir '%s' (%ld)\n",
                           part, entry.firstcluster);
					pdir->busy = false;
					return NULL;
				}
#ifdef HAVE_MULTIVOLUME
                pdir->volumecounter = -1; /* n.a. to subdirs */
#endif
				break;
			}
		}
	}

	return pdir;
}

int closedir_uncached(DIR_UNCACHED* dir)
{
	dir->busy = false;
	return 0;
}

struct dirent_uncached* readdir_uncached(DIR_UNCACHED* dir)
{
	struct fat_direntry entry;
    struct dirent_uncached* theent = &(dir->theent);

	if (!dir->busy)
		return NULL;

#ifdef HAVE_MULTIVOLUME
    /* Volumes (secondary file systems) get inserted into the root directory
        of the first volume, since we have no separate top level. */
    if (dir->volumecounter >= 0 /* on a root dir */
     && dir->volumecounter < NUM_VOLUMES /* in range */
     && dir->fatdir.file.volume == 0) /* at volume 0 */
    {   /* fake special directories, which don't really exist, but
           will get redirected upon opendir_uncached() */
        while (++dir->volumecounter < NUM_VOLUMES)
        {
            if (fat_ismounted(dir->volumecounter))
            {
                memset(theent, 0, sizeof(*theent));
                theent->attribute = FAT_ATTR_DIRECTORY | FAT_ATTR_VOLUME;
                snprintf(theent->d_name, sizeof(theent->d_name), 
                         VOL_NAMES, dir->volumecounter);
                return theent;
            }
        }
    }
#endif
    /* normal directory entry fetching follows here */
	if (fat_getnext(&(dir->fatdir), &entry) < 0)
		return NULL;

	if (!entry.name[0])
		return NULL;

	strncpy((char*)theent->d_name, (char*)entry.name, sizeof(theent->d_name));
	theent->attribute = entry.attr;
	theent->size = entry.filesize;
	theent->startcluster = entry.firstcluster;
	theent->wrtdate = entry.wrtdate;
	theent->wrttime = entry.wrttime;

	return theent;
}

int mkdir_uncached(const char *name)
{
    DIR_UNCACHED *dir;
	char namecopy[MAX_PATH];
	char *end;
	char *basename;
	char *parent;
    struct dirent_uncached *entry;
	struct fat_dir newdir;
	int rc;

	if (name[0] != '/') {
        DEBUGF("mkdir: Only absolute paths supported right now\n");
		return -1;
	}

	strncpy(namecopy, name, sizeof (namecopy));
	namecopy[sizeof (namecopy) - 1] = 0;

    /* Split the base name and the path */
	end = strrchr(namecopy, '/');
	*end = 0;
	basename = end + 1;

	if (namecopy == end)	/* Root dir? */
		parent = "/";
	else
		parent = namecopy;

    DEBUGF("mkdir: parent: %s, name: %s\n", parent, basename);

    dir = opendir_uncached(parent);

	if (!dir) {
        DEBUGF("mkdir: can't open parent dir\n");
		return -2;
	}

	if (basename[0] == 0) {
        DEBUGF("mkdir: Empty dir name\n");
		errno = EINVAL;
		return -3;
	}

    /* Now check if the name already exists */
    while ((entry = readdir_uncached(dir))) {
        if ( !strcasecmp(basename, (char*)entry->d_name) ) {
            DEBUGF("mkdir error: file exists\n");
			errno = EEXIST;
            closedir_uncached(dir);
			return -4;
		}
	}

	memset(&newdir, 0, sizeof (struct fat_dir));

	rc = fat_create_dir(basename, &newdir, &(dir->fatdir));
    closedir_uncached(dir);
    
	return rc;
}

int rmdir_uncached(const char* name)
{
	int rc;
    DIR_UNCACHED* dir;
    struct dirent_uncached* entry;

    dir = opendir_uncached(name);
    if (!dir)
    {
		errno = ENOENT;	/* open error */
		return -1;
	}

    /* check if the directory is empty */
    while ((entry = readdir_uncached(dir)))
    {
        if (strcmp((char*)entry->d_name, ".") &&
            strcmp((char*)entry->d_name, ".."))
        {
            DEBUGF("rmdir error: not empty\n");
			errno = ENOTEMPTY;
            closedir_uncached(dir);
			return -2;
		}
	}

	rc = fat_remove(&(dir->fatdir.file));
	if (rc < 0) {
        DEBUGF("Failed removing dir: %d\n", rc);
		errno = EIO;
		rc = rc * 10 - 3;
	}

    closedir_uncached(dir);
	return rc;
}
