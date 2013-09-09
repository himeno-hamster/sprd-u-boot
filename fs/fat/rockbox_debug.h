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
 * but WITHOUT ANY WARRANTY; without eve8 the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define	fat_eprintf(fmt,args...) fprintf(stderr, "Error: %s:%d:%s(): "fmt"", \
					__FILE__, __LINE__,__FUNCTION__,##args)
#define	fat_pprintf(fmt,args...) fprintf(stderr, "Fatal: %s:%d:%s(): "fmt"", \
					__FILE__, __LINE__,__FUNCTION__,##args)
#define	fat_wprintf(fmt,args...) fprintf(stderr, "Warning: %s:%d:%s(): "fmt"", \
					__FILE__, __LINE__,__FUNCTION__,##args)

#if (defined(DEBUG) || defined(FAT_DEBUG))
#define	fat_dprintf(fmt,args...) printf("Debug: %s:%d:%s(): "fmt"", \
					__FILE__, __LINE__,__FUNCTION__,##args)
#define	fat_deprintf(fmt,args...) fprintf(stderr, "Error: %s:%d:%s(): "fmt"",\
					__FILE__, __LINE__,__FUNCTION__,##args)
#else
#define	fat_dprintf(fmt,args...)
#define	fat_deprintf(fmt,args...)
#endif
