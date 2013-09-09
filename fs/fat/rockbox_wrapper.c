/*------------------------------------------------------------------------------
 * (C) Copyright 2006
 * Keith Outwater, (outwater4@comcast.net)
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

#include <common.h>
#include <config.h>

/*
 * FIXME: The pwd, cd and file path builder code needs to be fixed/implemented.
 */

#include <asm/errno.h>
#include <asm/string.h>
#include <fat.h>
#include <linux/string.h>
#include "rockbox_debug.h"
#include <rockbox_mv.h>

/* Routines, exported or not... */
long file_fat_size(const char *filename);

extern int errno;	/* see board/<board>/<board>.c */

const char *month[] = {
	"(null)",
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/*
 * The current working directory.
 * Should we keep track of this on a per-partiton basis?
 */
char file_cwd[ROCKBOX_CWD_LEN + 1] = "/";

#ifndef __HAVE_ARCH_STRNICMP
/*
 * Convert a character to lower case
 */
__inline__ static char
_tolower(char c)
{
	if ((c >= 'A') && (c <= 'Z')) {
		c = (c - 'A') + 'a';
	}
	return c;
}

/**
 * strnicmp - Case insensitive, length-limited string comparison
 * @s1: One string
 * @s2: The other string
 * @len: the maximum number of characters to compare
 *
 * FIXME: replaces strnicmp() in rockbox-3.1. check is keep best one
 */
int
strnicmp(const char *s1, const char *s2, size_t len)
{
	/*
	 * Yes, Virginia, it had better be unsigned 
	 */
	unsigned char c1, c2;

	c1 = 0;
	c2 = 0;
	if (len) {
		do {
			c1 = *s1;
			c2 = *s2;
			s1++;
			s2++;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = _tolower(c1);
			c2 = _tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int) c1 - (int) c2;
}
#endif

/**
 * strcasecmp - Case insensitive string comparison
 * @s1: One string
 * @s2: The other string
 */
int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && _tolower(*s1) == _tolower(*s2)) {
        s1++;
        s2++;
    }

    return _tolower(*(unsigned char *) s1) - _tolower(*(unsigned char *) s2);
}


/**
 * strncasecmp - Case insensitive, length-limited string comparison
 * @s1: One string
 * @s2: The other string
 * @len: the maximum number of characters to compare
 *
 * FIXME: replaces strnicmp() in rockbox-3.1. check is keep best one
 */
int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if(!n)
      return 0;

    while (n-- != 0 && _tolower(*s1) == _tolower(*s2)) {
        if(n == 0 || *s1 == '\0')
          break;
        s1++;
        s2++;
    }

    return _tolower(*(unsigned char *) s1) - _tolower(*(unsigned char *) s2);
}


/*
 * U-Boot 'fat ls' wrapper. Code lifted from rockbox "filetree.c"
 * Return number of bytes read on success, -1 on error.
 */
int
file_fat_ls(const char *dirname)
{
	uint i, max_entries;
	int name_buffer_used = 0;
	DIR *dir;

	struct tree_context {
		int filesindir;
		int dirsindir;
		int dirlength;
		void *dircache;   /* unused, see below */
		char name_buffer[2048];
		int name_buffer_size;
		bool dirfull;
	} c;

	struct entry {
		short attr;	/* FAT attributes + file type flags */
		unsigned long time_write;	/* Last write time */
		char *name;
	};

	if (*dirname == '/') {
		while(*(dirname+1) == '/') dirname++;
	}
	if (strlen(dirname) > ROCKBOX_CWD_LEN) {
		errno = ENAMETOOLONG;
		return -1;
	}

	dir = opendir(dirname);
	if (!dir) {
		/* maybe it's a regular file */
		i = file_fat_size(dirname);
		if (i != -1) {
			printf("%9lu  %s\n", (ulong) i, dirname);
			return 0;
		}
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Error: ls() of \"%s\" failed: %s\n",
			dirname, strerror(errno));
#endif
		return -1;
	}

	c.dirsindir = 0;
	c.dirfull = false;
	c.name_buffer_size = sizeof (c.name_buffer) - 1;

	/*
	 * FIXME: figure out max i 
	 */
	max_entries = (strncmp("/", dirname, 2)==0) ? 512 : 0xFFFFFFFF;
	for (i = 0; i < max_entries; i++) {
		int len;
		struct dirent *entry = readdir(dir);
#if 0	/* dircache unused and not initialized ! */
		struct entry *dptr =
		    (struct entry *) (c.dircache + i * sizeof (struct entry));
#endif
		if (!entry)
			break;

		len = strlen((char*)entry->d_name);
		printf("%9lu %s %02d %4d %02d:%02d:%02d [%s] %s\n",
		       entry->size,
		       month[(entry->wrtdate >> 5) & 0xf],
		       entry->wrtdate & 0xf,
		       ((entry->wrtdate >> 9) & 0x7f) + 1980,
		       (entry->wrttime >> 11) & 0x1f,
		       (entry->wrttime >> 5) & 0x3f, entry->wrttime & 0x1f,
			   (entry->attribute & FAT_ATTR_DIRECTORY) ? "dir " : "file",
		       entry->d_name);
#if 0
		/*
		 * skip directories . and ..
		 */
		if ((entry->attribute & FAT_ATTR_DIRECTORY) &&
		    (((len == 1) &&
		      (!strncmp(entry->d_name, ".", 1))) ||
		     ((len == 2) && (!strncmp(entry->d_name, "..", 2))))) {
			i--;
			continue;
		}

		/*
		 * Skip FAT volume ID
		 */
		if (entry->attribute & FAT_ATTR_VOLUME_ID) {
			i--;
			continue;
		}
#endif

#if 0	/* dircache unused and not initialized ! */
		dptr->attr = entry->attribute;
#endif
		if (len > c.name_buffer_size - name_buffer_used - 1) {
			/*
			 * Tell the world that we ran out of buffer space 
			 */
			c.dirfull = true;
			fat_dprintf("Dir buffer is full\n");
			break;
		}

#if 0	/* dircache unused and not initialized ! */
		dptr->name = &c.name_buffer[name_buffer_used];
		dptr->time_write =
		    (long) entry->wrtdate << 16 | (long) entry->wrttime;
		strcpy(dptr->name, (char*)entry->d_name);
#endif
		name_buffer_used += len + 1;

		/*
		 * count the remaining dirs 
		 */
#if 0	/* dircache unused and not initialized ! */
		if (dptr->attr & FAT_ATTR_DIRECTORY)
#else
		if (entry->attribute & FAT_ATTR_DIRECTORY)
#endif
			c.dirsindir++;
	}

	c.filesindir = i;
	c.dirlength = i;
	closedir(dir);
	printf("\n%d file%s %d dir%s",
	       c.filesindir, c.filesindir == 1 ? "," : "s,",
	       c.dirsindir, c.dirsindir == 1 ? "\n" : "s\n");
	return 0;
}

/*
 * return target file size (negative if failure)
 * Return number of bytes of the target file, -1 on error.
 */
long
file_fat_size(const char *filename)
{
	int fd;
	long file_size;

	printf("Reading %s\n", filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Error: open() of \"%s\" failed: %s\n",
			filename, strerror(errno));
#endif
		return -1;
	}

	file_size = filesize(fd);
	if (file_size < 0) {
		fat_eprintf("Call to filesize() failed\n");
		close(fd);
		return -1;
	}

	close(fd);
	return file_size;
}


/*
 * U-Boot 'file read' wrapper.
 * Return number of bytes read on success, -1 on error.
 */
long
file_fat_read(const char *filename, void *buffer, unsigned long maxsize)
{
	int fd;
	long bytes;
	long file_size;

	printf("Reading %s\n", filename);
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Error: open() of \"%s\" failed: %s\n",
			filename, strerror(errno));
#endif
		return -1;
	}

	file_size = filesize(fd);
	if (file_size < 0) {
		fat_eprintf("Call to filesize() failed\n");
		return -1;
	}

	/*
	 * If the number of bytes to read is zero, read the entire file.
	 */
	if ((maxsize != 0) && (maxsize < file_size))
		file_size = maxsize;

	bytes = (long) read(fd, buffer, (size_t) file_size);
	if ((unsigned long) bytes != file_size) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Read error: %s\n", strerror(errno));
#endif
		close(fd);
		return -1;
	}

	close(fd);
	return bytes;
}

/*
 * U-Boot 'file write' wrapper.
 * Return number of bytes written on success, -1 on error.
 */
long
file_fat_write(const char *filename, void *buffer, unsigned long maxsize)
{
	int fd;
	long bytes;

	printf("Writing %s\n", filename);
	fd = open(filename, O_WRONLY | O_CREAT);
	if (fd < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Open of \"%s\" failed: %s\n",
			filename, strerror(errno));
#endif
		return -1;
	}

	bytes = (long) write(fd, buffer, (size_t) maxsize);
	if ((unsigned long) bytes != maxsize) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Write failed: %s\n", strerror(errno));
#endif
		close(fd);
		return -1;
	}

	close(fd);
	return bytes;
}

int
file_fat_pwd(void)
{
	printf("%s\n", file_cwd);
	return 1;
}

/*
 * U-Boot 'file rm' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_rm(const char *filename)
{
	int rc;

	fat_dprintf("Remove \"%s\"\n", filename);
	rc = remove(filename);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Removal of \"%s\" failed: %s\n",
			filename, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

/*
 * U-Boot 'file mkdir' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_mkdir(const char *dirname)
{
	int rc;

	fat_dprintf("Make dir \"%s\"\n", dirname);
	rc = mkdir(dirname);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Creation of \"%s\" failed: %s\n",
			dirname, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

/*
 * U-Boot 'file rmdir' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_rmdir(const char *dirname)
{
	int rc;

	fat_dprintf("Remove dir \"%s\"\n", dirname);
	rc = rmdir(dirname);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Removal of \"%s\" failed: %s\n",
			dirname, strerror(errno));
#endif
		return -1;
	}

	return 1;
}

static void
pathcpy(char *dest, const char *src)
{
	char *origdest = dest;

	do {
		if (dest - file_cwd >= CWD_LEN) {
			*dest = '\0';
			return;
		}

		*(dest) = *(src);
		if (*src == '\0') {
			if (dest-- != origdest && ISDIRDELIM(*dest)) {
				*dest = '\0';
			}
			return;
		}

		++dest;
		if (ISDIRDELIM(*src)) {
			while (ISDIRDELIM(*src))
				src++;
		} else {
			src++;
		}
	} while (1);
}

/*
 * U-Boot 'file cd' wrapper.
 */
int
file_fat_cd(const char *path)
{
	if (ISDIRDELIM(*path)) {
		while (ISDIRDELIM(*path))
			path++;
		strncpy(file_cwd + 1, path, CWD_LEN - 1);
	} else {
		const char *origpath = path;
		char *tmpstr = file_cwd;
		int back = 0;

		while (*tmpstr != '\0')
			tmpstr++;

		do {
			tmpstr--;
		} while (ISDIRDELIM(*tmpstr));

		while (*path == '.') {
			path++;
			while (*path == '.') {
				path++;
				back++;
			}

			if (*path != '\0' && !ISDIRDELIM(*path)) {
				path = origpath;
				back = 0;
				break;
			}

			while (ISDIRDELIM(*path))
				path++;

			origpath = path;
		}

		while (back--) {
			/*
			 * Strip off path component 
			 */
			while (!ISDIRDELIM(*tmpstr)) {
				tmpstr--;
			}

			if (tmpstr == file_cwd) {
				/*
				 * Incremented again right after the loop. 
				 */
				tmpstr--;
				break;
			}

			/*
			 * Skip delimiters 
			 */
			while (ISDIRDELIM(*tmpstr))
				tmpstr--;
		}

		tmpstr++;
		if (*path == '\0') {
			if (tmpstr == file_cwd) {
				*tmpstr = '/';
				tmpstr++;
			}

			*tmpstr = '\0';
			return 0;
		}

		*tmpstr = '/';
		pathcpy(tmpstr + 1, path);
	}

	printf("New CWD is '%s'\n", file_cwd);
	return 0;
}

/*
 * U-Boot 'file mv' wrapper.
 * Return 1 on success, -1 on error and set 'errno'.
 */
int
file_fat_mv(const char *oldname, const char *newpath)
{
	int rc;

	fat_dprintf("Move \'%s\" to \"%s\"\n", oldname, newpath);
	rc = rename(oldname, newpath);
	if (rc < 0) {
#ifdef CONFIG_STRERROR
		fprintf(stderr, "Failed to move \"%s\" to \"%s\" : %s\n",
			oldname, newpath, strerror(errno));
#endif
		return -1;
	}

	return 1;
}


extern int fat_mount(IF_MV2(int volume,) IF_MV2(int drive,) long startsector);
int rockbox_fat_mount(long startsector);
/* @brief Map rockbox_fat_mount() (fs/fat/fat.c) on fat_mount() (fs/fat/rockbox_fat.c)
 */
int rockbox_fat_mount(long startsector)
{
#ifdef HAVE_MULTIVOLUME
#error "MULTIVOLUME from rockbox package not ported to U-boot"
#else
    return fat_mount(IF_MV2(volume,) IF_MV2(drive,) startsector);
#endif
}

/* @brief Get filesystem size and free left space
 * @return partition size in kByte 
 */
unsigned int rockbox_fat_size(void)
{
#ifdef HAVE_MULTIVOLUME
#error "MULTIVOLUME from rockbox package not ported to U-boot"
#else
	unsigned long tsize;
    fat_size(IF_MV2(int volume,) &tsize, NULL);
	return (unsigned int) tsize;
#endif
}

/* @brief Return available free space.
 * @param[in]  if size==0, compute free space, else check if enougth free space
 * @return     if size==0, ret free space, else return true if enougth free space
 * @internals
 *     Input and returned size values are formated in kByte.
 */
unsigned int rockbox_fat_free(unsigned long size)
{
#ifdef HAVE_MULTIVOLUME
#error "MULTIVOLUME from rockbox package not ported to U-boot"
#else
	if (size) { 
		return fat_check_free(IF_MV2(int volume,) size);
	} else {
		unsigned long tfree;
		fat_recalc_free(IF_MV2(int volume,));
	    fat_size(IF_MV2(int volume,) NULL, &tfree);
		return (unsigned int) tfree;
	}
#endif
}


