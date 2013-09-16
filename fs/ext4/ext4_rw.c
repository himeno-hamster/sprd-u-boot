#include <linux/types.h>
#include <common.h>
#include <part.h>
#include <ext4fs.h>

#if 0
int ext4_read_content(int dev,int part,const char *filename, void *buf, int offset, int len)
{
	disk_partition_t info;
	block_dev_desc_t *dev_desc;

	dev_desc = get_dev("mmc", dev);
	if(NULL == dev_desc){
		printf("ext4_read_content get dev error\n");
		return 1;
	}

	get_partition_info(dev_desc,part,&info);
	/* set the device as block device */
	ext4fs_set_blk_dev(dev_desc, &info);

	/* mount the filesystem */
	if (!ext4fs_mount(info.size)) {
		printf("Bad ext4 partition part=%d\n",  part);
		goto fail;
	}

	/* start read */
	if (ext4_read_file(filename, (unsigned char *)buf, offset,len) == -1) {
		printf("** Error ext4_read_file() **\n");
		goto fail;
	}
	printf("ext4_read_content %s\n",buf);
	ext4fs_close();
	return 0;
fail:
	ext4fs_close();

	return 1;
}
#endif

int ext4_read_content(int dev, wchar_t* partition_name, const char *filename, void *buf, int offset, int len)
{
	disk_partition_t info;
	block_dev_desc_t *dev_desc;

	dev_desc = get_dev("mmc", dev);
	if(NULL == dev_desc){
		printf("ext4_read_content get dev error\n");
		return 1;
	}

	if(get_partition_info_by_name (dev_desc, partition_name, &info) == -1){
		printf("## Valid EFI partition not found ##\n");
		return 1;
	}

	/* set the device as block device */
	ext4fs_set_blk_dev(dev_desc, &info);

	/* mount the filesystem */
	if (!ext4fs_mount(info.size)) {
//		printf("Bad ext4 partition part=%d\n",  part);
		goto fail;
	}

	/* start read */
	if (ext4_read_file(filename, (unsigned char *)buf, offset,len) == -1) {
		printf("** Error ext4_read_file() **\n");
		goto fail;
	}

	ext4fs_close();
	return 0;
fail:
	ext4fs_close();

	return 1;
}


int ext4_write_content(int dev, wchar_t* partition_name,char* filename,unsigned char *ram_address,int file_size)
{
	disk_partition_t info;
	block_dev_desc_t *dev_desc;

	dev_desc = get_dev("mmc", dev);
	if(NULL == dev_desc){
		printf("ext4_write_data get dev error\n");
		return 1;
	}

	if(get_partition_info_by_name (dev_desc, partition_name, &info) == -1){
		printf("## Valid EFI partition not found ##\n");
		return 1;
	}

	/* set the device as block device */
	ext4fs_set_blk_dev(dev_desc, &info);

	/* mount the filesystem */
	if (!ext4fs_mount(info.size)) {
//		printf("Bad ext4 partition part=%d\n",  part);
		goto fail;
	}

	/* start write */
	if (ext4fs_write(filename, (unsigned char *)ram_address, file_size)) {
		printf("** Error ext4fs_write() **\n");
		goto fail;
	}
	ext4fs_close();
	return 0;
fail:
	ext4fs_close();

	return 1;
}


#if 0
int ext4_write_content(int dev,int part,char* filename,unsigned char *ram_address,int file_size)
{
	disk_partition_t info;
	block_dev_desc_t *dev_desc;

	dev_desc = get_dev("mmc", dev);
	if(NULL == dev_desc){
		printf("ext4_write_data get dev error\n");
		return 1;
	}

	get_partition_info(dev_desc,part,&info);
	/* set the device as block device */
	ext4fs_set_blk_dev(dev_desc, &info);

	/* mount the filesystem */
	if (!ext4fs_mount(info.size)) {
		printf("Bad ext4 partition part=%d\n",  part);
		goto fail;
	}

	/* start write */
	if (ext4fs_write(filename, (unsigned char *)ram_address, file_size)) {
		printf("** Error ext4fs_write() **\n");
		goto fail;
	}
	ext4fs_close();
	return 0;
fail:
	ext4fs_close();

	return 1;
}
#endif

