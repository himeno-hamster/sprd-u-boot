/* libs/diskconfig/diskconfig.c
 *
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config_utils.h"
#include <command.h>
#include "diskconfig.h"


static int
parse_len(const char *str, uint64_t *plen)
{
    char tmp[64];
    int len_str;
    uint32_t multiple = 1;

    strncpy(tmp, str, sizeof(tmp));
    tmp[sizeof(tmp)-1] = '\0';
    len_str = strlen(tmp);
    if (!len_str) {
        printf("Invalid disk length specified.");
        return 1;
    }

    switch(tmp[len_str - 1]) {
        case 'M': case 'm':
            /* megabyte */
            multiple <<= 10;
        case 'K': case 'k':
            /* kilobytes */
            multiple <<= 10;
            tmp[len_str - 1] = '\0';
            break;
        default:
            break;
    }

    *plen = strtoull(tmp, NULL, 0);
    if (!*plen) {
		
        printf("Invalid length specified: %s", str);
        return 1;
    }

    if (*plen == (uint64_t)-1) {
        if (multiple > 1) {
            printf("Size modifier illegal when len is -1");
            return 1;
        }
    } else {
        /* convert len to kilobytes */
        if (multiple > 1024)
            multiple >>= 10;
        *plen *= multiple;

        if (*plen > 0xffffffffULL) {
            printf("Length specified is too large!: %llu KB", *plen);
            return 1;
        }
    }

    return 0;
}


static int
load_partitions(cnode *root, struct disk_info *dinfo)
{
    cnode *partnode;
    int plba = 2048 /dinfo->sect_size;
    dinfo->num_parts = 0;
    for (partnode = root->first_child; partnode; partnode = partnode->next) {
        struct part_info *pinfo = &dinfo->part_lst[dinfo->num_parts];
        const char *tmp;

        /* bleh, i will leak memory here, but i DONT CARE since
         * the only right thing to do when this function fails
         * is to quit */
        pinfo->name = strdup(partnode->name);

        if(config_bool(partnode, "active", 0))
            pinfo->flags |= PART_ACTIVE_FLAG;

        if (!(tmp = config_str(partnode, "type", NULL))) {
            printf("Partition type required: %s", pinfo->name);
            return 1;
        }

        /* possible values are: linux, fat32 */
        if (!strcmp(tmp, "linux")) {
            pinfo->type = PC_PART_TYPE_LINUX;
        } else if (!strcmp(tmp, "fat32")) {
            pinfo->type = PC_PART_TYPE_FAT32;
        } else {
            printf("Unsupported partition type found: %s", tmp);
            return 1;
        }

        if ((tmp = config_str(partnode, "len", NULL)) != NULL) {
            uint64_t len;
            if (parse_len(tmp, &len))
                return 1;
            pinfo->len_kb = (uint32_t) len;
	    pinfo->start_lba = plba;
	    plba = pinfo->len_kb /dinfo->sect_size + plba;
        } else 
            pinfo->len_kb = 0;

        ++dinfo->num_parts;
    }

    return 0;
}

struct disk_info *
load_diskconfig (void)
{
    struct disk_info *dinfo;
    cnode *devroot;
    cnode *partnode;
    cnode *root = config_node("", "");
    const char *tmp;

    if (!(dinfo = malloc(sizeof(struct disk_info)))) {
        printf("Could not malloc disk_info");
        return NULL;
    }
    memset(dinfo, 0, sizeof(struct disk_info));

    if (!(dinfo->part_lst = malloc(MAX_NUM_PARTS * sizeof(struct part_info)))) {
        printf("Could not malloc part_lst\n");
        goto fail;
    }
    memset(dinfo->part_lst, 0,
           (MAX_NUM_PARTS * sizeof(struct part_info)));
 
    config_load_file(root);
 
    if (root->first_child == NULL) {
        printf("Could not read config file\n");
        goto fail;
    }

    if (!(devroot = config_find(root, "device"))) {
        printf("Could not find device section in config file \n'");
        goto fail;
    }

/*
    if (!(tmp = config_str(devroot, "path", path_override))) {
        printf("device path is requried\n");
        goto fail;
    }
    
    dinfo->device = strdup(tmp);
*/
    /* find the partition scheme */
    if (!(tmp = config_str(devroot, "scheme", NULL))) {
        printf("partition scheme is required\n");
        goto fail;
    } else if (!strcmp(tmp, "mbr")) {
        dinfo->scheme = PART_SCHEME_MBR;
    } else if (!strcmp(tmp, "gpt")) {
        printf("'gpt' partition scheme not supported yet.\n");
        goto fail;
    } else {
        printf("Unknown partition scheme specified: %s", tmp);
        goto fail;
    }

    /* grab the sector size (in bytes) */
    tmp = config_str(devroot, "sector_size", "512");
    dinfo->sect_size = strtol(tmp, NULL, 0);
    if (!dinfo->sect_size) {
        printf("Invalid sector size: %s", tmp);
        goto fail;
    }

    /* first lba where the partitions will start on disk */
    if (!(tmp = config_str(devroot, "start_lba", NULL))) {
        printf("start_lba must be provided\n");
        goto fail;
    }

    if (!(dinfo->skip_lba = strtol(tmp, NULL, 0))) {
        printf("Invalid starting LBA (or zero): %s", tmp);
        goto fail;
    }

    /* Number of LBAs on disk */
    if (!(tmp = config_str(devroot, "num_lba", NULL))) {
        printf("num_lba is required\n");
        goto fail;
    }
    dinfo->num_lba = strtoul(tmp, NULL, 0);

    if (!(partnode = config_find(devroot, "partitions"))) {
        printf("Device must specify partition list\n");
        goto fail;
    }

    if (load_partitions(partnode, dinfo))
        goto fail;

    return dinfo;

fail:
    if (dinfo->part_lst)
        free(dinfo->part_lst);
    if (dinfo->device)
        free(dinfo->device);
    free(dinfo);
    return NULL;
}

static int
sync_ptable(int fd)
{
printf("sync_ptable\n");
/*
    struct stat stat;
    int rv;

    sync();

    if (fstat(fd, &stat)) {
       printf("Cannot stat, errno=%d.", errno);
       return -1;
    }

    if (S_ISBLK(stat.st_mode) && ((rv = ioctl(fd, BLKRRPART, NULL)) < 0)) {
        printf("Could not re-read partition table. REBOOT!. (errno=%d)", errno);
        return -1;
    }
*/
    return 0;
}

/* This function verifies that the disk info provided is valid, and if so,
 * returns an open file descriptor.
 *
 * This does not necessarily mean that it will later be successfully written
 * though. If we use the pc-bios partitioning scheme, we must use extended
 * partitions, which eat up some hd space. If the user manually provisioned
 * every single partition, but did not account for the extra needed space,
 * then we will later fail.
 *
 * TODO: Make validation more complete.
 */
 #if 0
static int
validate(struct disk_info *dinfo)
{
    int fd;
    int sect_sz;
    uint64_t disk_size;
    uint64_t total_size;
    int cnt;
    struct stat stat;

    if (!dinfo)
        return -1;

    if ((fd = open(dinfo->device, O_RDWR)) < 0) {
        printf("Cannot open device '%s' (errno=%d)", dinfo->device, errno);
        return -1;
    }

    if (fstat(fd, &stat)) {
        printf("Cannot stat file '%s', errno=%d.", dinfo->device, errno);
        goto fail;
    }


    /* XXX: Some of the code below is kind of redundant and should probably
     * be refactored a little, but it will do for now. */

    /* Verify that we can operate on the device that was requested.
     * We presently only support block devices and regular file images. */
    if (S_ISBLK(stat.st_mode)) {
        /* get the sector size and make sure we agree */
        if (ioctl(fd, BLKSSZGET, &sect_sz) < 0) {
            printf("Cannot get sector size (errno=%d)", errno);
            goto fail;
        }

        if (!sect_sz || sect_sz != dinfo->sect_size) {
            printf("Device sector size is zero or sector sizes do not match!");
            goto fail;
        }

        /* allow the user override the "disk size" if they provided num_lba */
        if (!dinfo->num_lba) {
            if (ioctl(fd, BLKGETSIZE64, &disk_size) < 0) {
                printf("Could not get block device size (errno=%d)", errno);
                goto fail;
            }
            /* XXX: we assume that the disk has < 2^32 sectors :-) */
            dinfo->num_lba = (uint32_t)(disk_size / (uint64_t)dinfo->sect_size);
        } else
            disk_size = (uint64_t)dinfo->num_lba * (uint64_t)dinfo->sect_size;
    } else if (S_ISREG(stat.st_mode)) {
        printf("Requesting operation on a regular file, not block device.");
        if (!dinfo->sect_size) {
            printf("Sector size for regular file images cannot be zero");
            goto fail;
        }
        if (dinfo->num_lba)
            disk_size = (uint64_t)dinfo->num_lba * (uint64_t)dinfo->sect_size;
        else {
            dinfo->num_lba = (uint32_t)(stat.st_size / dinfo->sect_size);
            disk_size = (uint64_t)stat.st_size;
        }
    } else {
        printf("Device does not refer to a regular file or a block device!");
        goto fail;
    }

#if 1
    printf("Device/file %s: size=%llu bytes, num_lba=%u, sect_size=%d",
         dinfo->device, disk_size, dinfo->num_lba, dinfo->sect_size);
#endif

    /* since this is our offset into the disk, we start off with that as
     * our size of needed partitions */
    total_size = dinfo->skip_lba * dinfo->sect_size;

    /* add up all the partition sizes and make sure it fits */
    for (cnt = 0; cnt < dinfo->num_parts; ++cnt) {
        struct part_info *part = &dinfo->part_lst[cnt];
        if (part->len_kb != (uint32_t)-1) {
            total_size += part->len_kb * 1024;
        } else if (part->len_kb == 0) {
            printf("Zero-size partition '%s' is invalid.", part->name);
            goto fail;
        } else {
            /* the partition requests the rest of the disk. */
            if (cnt + 1 != dinfo->num_parts) {
                printf("Only the last partition in the list can request to fill "
                     "the rest of disk.");
                goto fail;
            }
        }

        if ((part->type != PC_PART_TYPE_LINUX) &&
            (part->type != PC_PART_TYPE_FAT32)) {
            printf("Unknown partition type (0x%x) encountered for partition "
                 "'%s'\n", part->type, part->name);
            goto fail;
        }
    }

    /* only matters for disks, not files */
    if (S_ISBLK(stat.st_mode) && total_size > disk_size) {
        printf("Total requested size of partitions (%llu) is greater than disk "
             "size (%llu).", total_size, disk_size);
        goto fail;
    }

    return fd;

fail:
    close(fd);
    return -1;
}
#endif

static int
validate_and_config(struct disk_info *dinfo,  struct write_list **lst)
{
    *lst = NULL;

    switch (dinfo->scheme) {
        case PART_SCHEME_MBR:
            *lst = config_mbr(dinfo);
            return *lst == NULL;
        default:
            printf("Uknown partition scheme.");
            break;
    }

    *lst = NULL;
    return 1;
}

/* validate and process the disk layout configuration.
 * This will cause an update to the partitions' start lba.
 *
 * Basically, this does the same thing as apply_disk_config in test mode,
 * except that wlist_commit is not called to print out the data to be
 * written.
 */
 #if 0
int
process_disk_config(struct disk_info *dinfo)
{
    struct write_list *lst;
    int fd;

    if (validate_and_config(dinfo, &lst) != 0)
        return 1;

    close(fd);
    wlist_free(lst);
    return 0;
}
#endif 

int
apply_disk_config(struct disk_info *dinfo)
{
    struct write_list *wr_lst = NULL;
    int rv;

    if (validate_and_config(dinfo, &wr_lst) != 0) {
        printf("Configuration is invalid.\n");
        goto fail;
    }

    if ((rv = wlist_commit(wr_lst)) >= 0)
       // rv = 1 ? 0 : sync_ptable(fd);
    	printf("partition list write success\n");
 
    wlist_free(wr_lst);
    return rv;

fail:
    if (wr_lst)
        wlist_free(wr_lst);
    return 0;
}

int
dump_disk_config(struct disk_info *dinfo)
{
    int cnt;
    struct part_info *part;

    printf("Device: %s\n", dinfo->device);
    printf("Scheme: ");
    switch (dinfo->scheme) {
        case PART_SCHEME_MBR:
            printf("MBR");
            break;
        case PART_SCHEME_GPT:
            printf("GPT (unsupported)");
            break;
        default:
            printf("Unknown");
            break;
    }
    printf ("\n");

    printf("Sector size: %d\n", dinfo->sect_size);
    printf("Skip leading LBAs: %u\n", dinfo->skip_lba);
    printf("Number of LBAs: %u\n", dinfo->num_lba);
    printf("Partitions:\n");

    for (cnt = 0; cnt < dinfo->num_parts; ++cnt) {
        part = &dinfo->part_lst[cnt];
        printf("\tname = %s\n", part->name);
        printf("\t\tflags = %s\n",
               part->flags & PART_ACTIVE_FLAG ? "Active" : "None");
        printf("\t\ttype = %s\n",
               part->type == PC_PART_TYPE_LINUX ? "Linux" : "Unknown");
        if (part->len_kb == (uint32_t)-1)
            printf("\t\tlen = rest of disk\n");
        else
            printf("\t\tlen = %uKB\n", part->len_kb);
    }
    printf("Total number of partitions: %d\n", cnt);
    printf("\n");

    return 0;
}

struct part_info *
find_part(struct disk_info *dinfo, const char *name)
{
    struct part_info *pinfo;
    int cnt;

    for (cnt = 0; cnt < dinfo->num_parts; ++cnt) {
        pinfo = &dinfo->part_lst[cnt];
        if (!strcmp(pinfo->name, name))
            return pinfo;
    }

    return NULL;
}

/* NOTE: If the returned ptr is non-NULL, it must be freed by the caller. */
#if 0
char *
find_part_device(struct disk_info *dinfo, const char *name)
{
    switch (dinfo->scheme) {
        case PART_SCHEME_MBR:
            return find_mbr_part(dinfo, name);
        case PART_SCHEME_GPT:
            printf("GPT is presently not supported");
            break;
        default:
            printf("Unknown partition table scheme");
            break;
    }

    return NULL;
}
#endif

