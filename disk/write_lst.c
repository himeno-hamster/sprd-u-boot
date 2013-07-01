/* libs/diskconfig/write_lst.c
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

 #include "diskconfig.h"
 #include "part_cfg.h"
extern PART_DEVICE emmc_part_device;
#define DOS_PART_MAGIC_OFFSET 1fe
struct write_list *
alloc_wl(uint32_t data_len)
{
    struct write_list *item;

    if (!(item = malloc(sizeof(struct write_list) + data_len))) {
        printf("Unable to allocate memory.\n");
        return NULL;
    }

    item->len = data_len;
    return item;
}

void
free_wl(struct write_list *item)
{
    if (item)
        free(item);
}

struct write_list *
wlist_add(struct write_list **lst, struct write_list *item)
{
    item->next = (*lst);
    *lst = item;
    return item;
}

void
wlist_free(struct write_list *lst)
{
    struct write_list *temp_wr;
    while (lst) {
        temp_wr = lst->next;
        free_wl(lst);
        lst = temp_wr;
    }
}

int
wlist_commit( struct write_list *lst)
{
/*
    for(; lst; lst = lst->next) {
        if (lseek64(fd, lst->offset, SEEK_SET) != (loff_t)lst->offset) {
            printf("Cannot seek to the specified position (%lld).", lst->offset);
            goto fail;
        }

        if (!test) {
            if (write(fd, lst->data, lst->len) != (int)lst->len) {
                printf("Failed writing %u bytes at position %lld.", lst->len,
                     lst->offset);
                goto fail;
            }
        } else
            printf("Would write %d bytes @ offset %lld.", lst->len, lst->offset);
    }

    return 0;

fail:
    return -1;
*/

	for(; lst; lst = lst->next) {
		char buf[512];
		int lba = lst->offset / 512;
		emmc_part_device._device_io->_read(lba,1,buf);
		int i;
		static int dc=1;
		printf("write %d partition\n", dc);
		printf("off %lli len %u\n", lst->offset, lst->len);
		buf[DOS_PART_MAGIC_OFFSET] = 0x55;
		buf[DOS_PART_MAGIC_OFFSET + 1] = 0xaa;
		for(i=0;i<lst->len;i++) {
			if((i+lst->offset %512)%16 == 0)
				printf("0x%x   ", i+lst->offset %512);
			printf("0x%x, ", lst->data[i]);
			if((i+lst->offset %512)%16 == 15)
				printf("\n");
			buf[i+lst->offset %512] = lst->data[i];			
		}
		dc++;
		
		emmc_part_device._device_io->_write(lba,1,buf);
	}
	
	return 1;
	
	
}
