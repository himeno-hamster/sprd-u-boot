#include <common.h>
#include <linux/types.h>
#include <linux/string.h>

int do_fs_file_read(char *mpart, char *filenm, void *buf, int len)
{
	int ret=-1;
#ifdef CONFIG_FS_EXT4
	int mmc_dev=1;
	wchar_t *part=NULL;
	wchar_t *tmp;
	part = malloc(sizeof(wchar_t)*(strlen(mpart)+1));
	if(!part)
		return -1;
	tmp = part;
	while ((*tmp++ = *mpart++) != '\0')
		/* do nothing */;
	ret = ext4_read_content(mmc_dev,
			part,
			filenm,
			buf,
			0,/*not support non-zero offset*/
			len);
	free(part);
#elif defined(CONFIG_FS_UBIFS)
	static int is_ubifs_init = 0;
	if(is_ubifs_init == 0){
		ubifs_init();
		is_ubifs_init = 1;
	}

	ret = ubifs_mount(mpart);
	if(ret){
		printf("do_fs_file_read:mount %s failed!\n",mpart);
		return ret;
	}
	ret = ubifs_load(filenm, buf, len);
	if(ret)
		printf("do_fs_file_read:file %s not found!\n", filenm);
#endif
	return ret;
}

int do_fs_file_write(char *mpart, char *filenm, void *buf, int len)
{
	/*do not write in uboot now*/
	return -1;
}
