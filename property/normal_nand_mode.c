#include "normal_mode.h"

extern void cmd_yaffs_mount(char *mp);
extern void cmd_yaffs_umount(char *mp);
extern int cmd_yaffs_ls_chk(const char *dirfilename);
extern void cmd_yaffs_mread_file(char *fn, unsigned char *addr);
extern void cmd_yaffs_mread_fileex(char *fn, unsigned char *addr, int size);
static int flash_page_size = 0;
int is_factorymode()
{
	int ret = 0;
	char *factorymodepoint = "/productinfo";
	char *factorymodefilename = "/productinfo/factorymode.file";
	cmd_yaffs_mount(factorymodepoint);
	ret = cmd_yaffs_ls_chk(factorymodefilename );
	cmd_yaffs_umount(factorymodepoint);
	return ret;
}
int read_logoimg(char *bmp_img,size_t size)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;
	loff_t off = 0;

	ret = mtdparts_init();
	if (ret != 0){
		printf("mtdparts init error %d\n", ret);
		return -1;
	}
#define SPLASH_PART "fastboot_logo"

	ret = find_dev_and_part(SPLASH_PART, &dev, &pnum, &part);
	if(ret){
		printf("No partition named %s\n", SPLASH_PART);
		return -1;
	}else if(dev->id->type != MTD_DEV_TYPE_NAND){
		printf("Partition %s not a NAND device\n", SPLASH_PART);
		return -1;
	}

	off=part->offset;
	nand = &nand_info[dev->id->num];

	ret = nand_read_offset_ret(nand, off, &size, (void *)bmp_img, &off);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
		return -1;
	}
	return 0;
}
void addbuf(char *buf)
{
#if !(BOOT_NATIVE_LINUX)
	int str_len = strlen(buf);
	char * mtdpart_def = NULL;
	mtdpart_def = get_mtdparts();
	sprintf(&buf[str_len], " %s", mtdpart_def);
#endif
}
void addcmdline(char *buf)
{

}
int read_spldata()
{
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	loff_t off = 0;
	struct mtd_info *nand;
	int size = CONFIG_SPL_LOAD_LEN;

	int ret = find_dev_and_part(SPL_PART, &dev, &pnum, &part);
	if (ret) {
		printf("No partition named %s\n", SPL_PART);
		return -1;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		printf("Partition %s not a NAND device\n", SPL_PART);
		return -1;
	}
	off = part->offset;
	nand = &nand_info[dev->id->num];
	flash_page_size = nand->writesize;

	ret = nand_read_offset_ret(nand, off, &size, (void*)spl_data, &off);
	if(ret != 0) {
		printf("spl nand read error %d\n", ret);
		return -1;
	}
	return 0;
}
#define VLX_RAND_TO_U32( _addr ) \
	if( (_addr) & 0x3 ){_addr += 0x4 -((_addr) & 0x3); }
static unsigned int Vlx_CalcFixnvLen(unsigned int search_start, unsigned int search_end) {
	unsigned int start_addr, end_addr;
	unsigned short id, len;
	volatile unsigned short *flash_ptr;

	start_addr = search_start;
	end_addr   = search_end;
	start_addr += sizeof(unsigned int); //skip update flag

	while(start_addr < end_addr)
	{
		flash_ptr = (volatile unsigned short *)(start_addr);
		id  = *flash_ptr++;
		len = *flash_ptr;
		if(0xFFFF == id)
		{
			return (start_addr-search_start);
		}
		else
		{
			start_addr += 4 + len +(len & 0x1);
			VLX_RAND_TO_U32( start_addr );
		}
	}
	return 0xffffffff;
}

static int check_fixnv_struct(unsigned int addr,unsigned int size) {
	unsigned int length = 0,keep_length=0;
	volatile unsigned int *flash_ptr;

	flash_ptr = (volatile unsigned int *)(addr+size-8);
	keep_length = *flash_ptr;
	printf("keep_length=%d  line=%d\r\n",keep_length ,__LINE__);
	if(keep_length != 0xffffffff){
		length = Vlx_CalcFixnvLen(addr, addr+size);
		if(keep_length != length){
			printf("keep_length=%d  length=%d line=%d\r\n",keep_length ,length,__LINE__);
			return -1;
		}
	}
	return 1;
}
static void recovery_sector(const char* dst_sector_path,
			const char* dst_sector_name,
			const char* backup_dst_sector_name,
			unsigned char * mem_addr,
			unsigned int size) {
	cmd_yaffs_mount(dst_sector_path);
	cmd_yaffs_mwrite_file(dst_sector_name, mem_addr, size);
	if(backup_dst_sector_name)
		cmd_yaffs_mwrite_file(backup_dst_sector_name, mem_addr, size);
	cmd_yaffs_umount(dst_sector_path);
}

static int load_sector_to_memory(const char* sector_path,
				const char* sector_name,
				const char* backup_sector_name,
				unsigned char * mem_addr,
				unsigned char * bck_addr,
				unsigned int size) {
	int ret = -1;
	int ptr = -1;
	int try_backup_file = 0;
	const char * curr_file = sector_name;
	unsigned short *master , *slave;
	//clear memory
	memset(mem_addr, 0xff,size);
	//mount yaffs
	cmd_yaffs_mount(sector_path);

TRY_BACKUP_FILE:
	//is file exist and has a right size?
	ret = cmd_yaffs_ls_chk(curr_file);
	if (ret == size) {
		//read file to mem
		cmd_yaffs_mread_file(curr_file, mem_addr);
		ret = fixnv_is_correct_endflag(mem_addr, size-4);
	}
	else{
		ret = -1;
	}

	//try backup files.
	if(backup_sector_name&&ret == -1&&!try_backup_file){
		curr_file = backup_sector_name;
		try_backup_file = 1;
		goto TRY_BACKUP_FILE;
	}else if(backup_sector_name&&ret == 1&&!try_backup_file){
		
		ptr = cmd_yaffs_ls_chk(backup_sector_name);
		if (ptr == size) {
			//read file to mem
			cmd_yaffs_mread_file(backup_sector_name, bck_addr);
			ptr = fixnv_is_correct_endflag(bck_addr, size-4);
		}
		else{
			ptr = -1;
		}

		if(ptr == -1) {
			printf("[load_sector_to_memory]backup_sector is error need recovery it......\n");
			cmd_yaffs_mwrite_file(backup_sector_name, mem_addr, size);
		} else {
			master = (unsigned short *)(bck_addr + size - 8);
			slave  = (unsigned short *)(mem_addr + size - 8);
			if(*master != *slave){
				printf("[load_sector_to_memory]slave file is error recovery it......\n");
				cmd_yaffs_mwrite_file(sector_name, bck_addr, size);
				cmd_yaffs_mread_file(backup_sector_name, mem_addr);
			} else {
				printf("[load_sector_to_memory]both master and slave  is ok don't sysc......\n");
			}
		}
	}

	if(try_backup_file&&ret==1){
		//recovery_sector(sector_path,sector_name,mem_addr,size);
		printf("[load_sector_to_memory]recovery the latest file......\n");
		cmd_yaffs_mwrite_file(sector_name, mem_addr, size);
	}else if(try_backup_file&&ret==-1){
		printf("[load_sector_to_memory] don't care this log ........\n");
	}

	//unmout yaffs
	cmd_yaffs_umount(sector_path);

	return ret;
}
static int load_kernel_and_layout(struct mtd_info *nand,
						unsigned int phystart,
						char *header,
						char *kernel,
						char *ramdisk,
						unsigned int virtual_page_size,
						unsigned int real_page_size
						) {
	int ret = -1;
	boot_img_hdr *hdr = (boot_img_hdr*)header;
	unsigned int off = phystart;
	int size = real_page_size;

	printf("virtual_page_size : %x\n",virtual_page_size);
	printf("real_page_size : %x\n",real_page_size);
	//read boot image header
	ret = nand_read_offset_ret(nand, off, &size, (void *)hdr, &off);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
        return -1;
	}
	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		printf("bad boot image header, give up read!!!!\n");
        return -1;
	}
	else
	{

		char* prev_page_addr = header;
		//we asume that the header takes only one page.
		//read kernel image prepare
		unsigned int used_size = 1*virtual_page_size;
		unsigned int spare_size = 0;
		unsigned int next_file_size = hdr->kernel_size;

		if(used_size > 0){
			spare_size = real_page_size - used_size;
		}else{
			spare_size = 0;
		}
		//read kernel image
		printf("file size: %x\n",hdr->kernel_size);
		printf("use size: %x\n",used_size);
		printf("spare size: %x\n",spare_size);

		if(spare_size) {
			memcpy(kernel,&prev_page_addr[used_size],spare_size);
			next_file_size -= spare_size;
		}
		size = (next_file_size+(real_page_size - 1)) & (~(real_page_size - 1));
		ret = nand_read_offset_ret(nand, off, &size, (void *)(kernel+spare_size), &off);
		if(ret != 0){
			printf("reading kernel error!\n");
			printf("try reading to %x\n",kernel+spare_size);
		}
		//read ramdisk image prepare
		prev_page_addr =  (char*)(kernel+spare_size+size-real_page_size);
		used_size = (next_file_size%real_page_size+virtual_page_size-1)&(~(virtual_page_size-1));
		if(used_size > 0){
			spare_size = real_page_size - used_size;
		}else{
			spare_size = 0;
		}
		next_file_size = hdr->ramdisk_size;
		printf("file size: %x\n",hdr->ramdisk_size);
		printf("use size: %x\n",used_size);
		printf("spare size: %x\n",spare_size);
		//read ramdisk image
		if(spare_size){
			memcpy(ramdisk,&prev_page_addr[used_size],spare_size);
			next_file_size -= spare_size;
		}
		size = (next_file_size+(real_page_size - 1)) & (~(real_page_size - 1));
		ret = nand_read_offset_ret(nand, off, &size, (void *)(ramdisk+spare_size), &off);
		if(ret != 0){
			printf("reading ramdisk error!\n");
			printf("try reading to %x\n",ramdisk+spare_size);
		}
	}

	return ret;
}

void vlx_nand_boot(char * kernel_pname, char * cmdline, int backlight_set)
{
    boot_img_hdr *hdr = (void *)raw_header;
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	int ret;
	size_t size;
	loff_t off = 0;

	char *fixnvpoint = "/fixnv";
	char *fixnvfilename = "/fixnv/fixnv.bin";
	char *fixnvfilename2 = "/fixnv/fixnvchange.bin";
	char *backupfixnvpoint = "/backupfixnv";
	char *backupfixnvfilename = "/backupfixnv/fixnv.bin";

	char *runtimenvpoint = "/runtimenv";
	char *runtimenvpoint2 = "/runtimenv";
	char *runtimenvfilename = "/runtimenv/runtimenv.bin";
	char *runtimenvfilename2 = "/runtimenv/runtimenvbkup.bin";

	char *productinfopoint = "/productinfo";
	char *productinfofilename = "/productinfo/productinfo.bin";
	char *productinfofilename2 = "/productinfo/productinfobkup.bin";

	int orginal_right, backupfile_right;
	unsigned long orginal_index, backupfile_index;
	nand_erase_options_t opts;
	char * mtdpart_def = NULL;
	#if (defined CONFIG_SC8810) || (defined CONFIG_SC8825)
	MMU_Init(CONFIG_MMU_TABLE_ADDR);
	#endif
	ret = mtdparts_init();
	if (ret != 0){
		printf("mtdparts init error %d\n", ret);
		return;
	}

#ifdef CONFIG_SPLASH_SCREEN
#define SPLASH_PART "boot_logo"
	ret = find_dev_and_part(SPLASH_PART, &dev, &pnum, &part);
	if(ret){
		printf("No partition named %s\n", SPLASH_PART);
		return;
	}else if(dev->id->type != MTD_DEV_TYPE_NAND){
		printf("Partition %s not a NAND device\n", SPLASH_PART);
		return;
	}

	off=part->offset;
	nand = &nand_info[dev->id->num];
	//read boot image header
	size = 1<<19;//where the size come from????
	char * bmp_img = malloc(size);
	if(!bmp_img){
	    printf("not enough memory for splash image\n");
	    return;
	}
	ret = nand_read_offset_ret(nand, off, &size, (void *)bmp_img, &off);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
		return;
	}
   lcd_display_logo(backlight_set,(ulong)bmp_img,size);
#endif
    set_vibrator(0);

#if !(BOOT_NATIVE_LINUX)
	/*int good_blknum, bad_blknum;
	nand_block_info(nand, &good_blknum, &bad_blknum);
	printf("good is %d  bad is %d\n", good_blknum, bad_blknum);*/
	///////////////////////////////////////////////////////////////////////
	/* recovery damaged fixnv or backupfixnv */
	/* FIXNV_PART */
	printf("Reading fixnv to 0x%08x\n", FIXNV_ADR);
	//try "/fixnv/fixnvchange.bin" first,if fail,
	//try /fixnv/fixnv.bin instead
	ret = load_sector_to_memory(fixnvpoint,
							fixnvfilename2,
							fixnvfilename,
							(unsigned char *)FIXNV_ADR,
							(unsigned char *)MODEM_ADR,
							FIXNV_SIZE + 4);
	if(ret == -1){
		//fixnvpoint's files are not correct
		//the "/backupfixnv/fixnv.bin" must be correct!
		ret = load_sector_to_memory(backupfixnvpoint,
						backupfixnvfilename,
						0,
						(unsigned char *)FIXNV_ADR,
						(unsigned char *)MODEM_ADR,//we just test if it's correct.
						FIXNV_SIZE + 4);
		if(ret ==1){
			//we got a right file in backupfixnvpoint,
			//use it to recovery fixnvpoint's files.
			recovery_sector(fixnvpoint,
				fixnvfilename,
				fixnvfilename2,
				(unsigned char *)FIXNV_ADR,
				FIXNV_SIZE + 4);
		}else{
			//backupfixnvpoint's files are still uncorrect.
			//then we can do nothing to get it right!!!!
			//there is an fatal error has occured.
			printf("\n\nfixnv and backupfixnv are all wrong!\n\n");
			return -1;
			//clear memory
			//memset(FIXNV_ADR, 0xff,FIXNV_SIZE + 4);
		}
	}else{
		//everything is right!!
		//we can chose to do it or not.
		ret = load_sector_to_memory(backupfixnvpoint,
						backupfixnvfilename,
						0,
						(unsigned char *)RUNTIMENV_ADR,//we just test if it's correct.
						(unsigned char *)MODEM_ADR,//we just test if it's correct.
						FIXNV_SIZE + 4);
		if(ret == -1){
			recovery_sector(backupfixnvpoint,
				backupfixnvfilename,
				0,
				(unsigned char *)FIXNV_ADR,
				FIXNV_SIZE + 4);
		}
	}

	//finally we check the fixnv structure,if fail,then u-boot will hung up!!!
	if(check_fixnv_struct(FIXNV_ADR,FIXNV_SIZE) == -1){
		printf("check fixnv structer error ............\r\n");
		return -1;
	}

	///////////////////////////////////////////////////////////////////////
	/* PRODUCTINFO_PART */
	ret = load_sector_to_memory(productinfopoint,
							productinfofilename2,
							productinfofilename,
							(unsigned char *)PRODUCTINFO_ADR,
							(unsigned char *)MODEM_ADR,//we just test if it's correct.
							PRODUCTINFO_SIZE + 4);
	if(ret == -1){
		printf("don't need read productinfo  to 0x%08x!\n", PRODUCTINFO_ADR);
	}
	eng_phasechecktest((unsigned char *)PRODUCTINFO_ADR, SP09_MAX_PHASE_BUFF_SIZE);
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	/* RUNTIMEVN_PART */
	ret = load_sector_to_memory(runtimenvpoint,
							runtimenvfilename2,
							runtimenvfilename,
							(unsigned char *)RUNTIMENV_ADR,
							(unsigned char *)MODEM_ADR,//we just test if it's correct.
							RUNTIMENV_SIZE + 4);
	if(ret == -1){
		//clear memory
		memset(RUNTIMENV_ADR, 0xff,RUNTIMENV_SIZE + 4);
	}
	//array_value((unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE);

	////////////////////////////////////////////////////////////////
	/* DSP_PART */
	printf("Reading dsp to 0x%08x\n", DSP_ADR);
	ret = find_dev_and_part(DSP_PART, &dev, &pnum, &part);
	if (ret) {
		printf("No partition named %s\n", DSP_PART);
		return;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		printf("Partition %s not a NAND device\n", DSP_PART);
		return;
	}

	off = part->offset;
	nand = &nand_info[dev->id->num];
	flash_page_size = nand->writesize;
	size = (DSP_SIZE + (flash_page_size - 1)) & (~(flash_page_size - 1));
	if(size <= 0) {
		printf("dsp image should not be zero\n");
		return;
	}
	ret = nand_read_offset_ret(nand, off, &size, (void*)DSP_ADR, &off);
	if(ret != 0) {
		printf("dsp nand read error %d\n", ret);
		return;
	}
	secure_check(DSP_ADR, 0, DSP_ADR + DSP_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#elif defined(CONFIG_CALIBRATION_MODE_NEW)
#if defined(CONFIG_SP7702) || defined(CONFIG_SP8810W)
		/*
			force dsp sleep in native 8810 verson to reduce power consumption
		*/ 
		extern void DSP_ForceSleep(void);
		DSP_ForceSleep();
		printf("dsp nand read ok1 %d\n", ret);
#endif	

	if(poweron_by_calibration())
	{
		// ---------------------fix nv--------------------------------
		// 1 read orighin fixNv
		memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE);
		cmd_yaffs_mount(fixnvpoint);
		cmd_yaffs_ls_chk(fixnvfilename);
		cmd_yaffs_mread_fileex(fixnvfilename, (unsigned char *)FIXNV_ADR, FIXNV_SIZE);
		cmd_yaffs_umount(fixnvpoint);
		if(!fixnv_chkEcc(FIXNV_ADR, FIXNV_SIZE)){
			// 2 read backup fixNv
			printf("Read origin fixnv fail\n");
			memset((unsigned char *)FIXNV_ADR, 0xff, FIXNV_SIZE);
			cmd_yaffs_mount(backupfixnvpoint);
			cmd_yaffs_ls_chk(backupfixnvfilename);
			cmd_yaffs_mread_fileex(backupfixnvfilename, (unsigned char *)FIXNV_ADR, FIXNV_SIZE);
			cmd_yaffs_umount(backupfixnvpoint);
			if(!fixnv_chkEcc(FIXNV_ADR, FIXNV_SIZE)){
				printf("Read backup fixnv fail\n");
			}
			else{
				printf("Read backup fixnv pass\n");
			}
		}
		else{
			printf("Read origin fixnv pass\n");
		}
		// ---------------------runtime nv----------------------------
		// 2 read orighin runtime nv
		memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
		cmd_yaffs_mount(runtimenvpoint);
		cmd_yaffs_ls_chk(runtimenvfilename);
		cmd_yaffs_mread_fileex(runtimenvfilename, (unsigned char *)RUNTIMENV_ADR,RUNTIMENV_SIZE);
		cmd_yaffs_umount(runtimenvpoint);
		if(!fixnv_chkEcc(RUNTIMENV_ADR, RUNTIMENV_SIZE)){
			// 2 read backup runtime nv
			printf("Read origin  runtime nv fail\n");
			memset((unsigned char *)RUNTIMENV_ADR, 0xff, RUNTIMENV_SIZE);
			cmd_yaffs_mount(runtimenvpoint2);
			cmd_yaffs_ls_chk(runtimenvfilename2);
			cmd_yaffs_mread_fileex(runtimenvfilename2, (unsigned char *)RUNTIMENV_ADR, RUNTIMENV_SIZE);
			cmd_yaffs_umount(runtimenvpoint2);
			if(!fixnv_chkEcc(RUNTIMENV_ADR, RUNTIMENV_SIZE)){
				printf("Read backup  runtime nv fail\n");
			}
			else{
				printf("Read backup  runtime nv pass\n");
			}
		}
		else{
			printf("Read origin  runtime nv pass\n");
		}

		// 3 read orighin product information
		memset((unsigned char *)(PRODUCTINFO_ADR), 0xff, PRODUCTINFO_SIZE);
		cmd_yaffs_mount(productinfopoint);
		cmd_yaffs_ls_chk(productinfofilename);
		cmd_yaffs_mread_file(productinfofilename, (unsigned char *)(PRODUCTINFO_ADR));
		cmd_yaffs_umount(productinfopoint);
		if(!fixnv_chkEcc((PRODUCTINFO_ADR), PRODUCTINFO_SIZE)){
			// 3 read backup productinfo
			printf("Read origin productinfo fail\n");
			memset((unsigned char *)(PRODUCTINFO_ADR), 0xff, PRODUCTINFO_SIZE);
			cmd_yaffs_mount(productinfopoint);
			cmd_yaffs_ls_chk(productinfofilename2);
			cmd_yaffs_mread_file(productinfofilename2, (unsigned char *)(PRODUCTINFO_ADR));
			cmd_yaffs_umount(productinfopoint);
			if(!fixnv_chkEcc((PRODUCTINFO_ADR), PRODUCTINFO_SIZE)){
				printf("Read backup productinfo fail\n");
			}
			else{
				printf("Read backup productinfo pass\n");
			}
		}
		else{
			char *product_data = (char *)PRODUCTINFO_ADR;
			printf("productinfo: %c %c %c %c\n",product_data[0],product_data[1],product_data[2],product_data[3]);
			printf("Read origin productinfo pass\n");
		}

		// ---------------------DSP ----------------------------
		printf("Reading dsp to 0x%08x\n", DSP_ADR);
		ret = find_dev_and_part(DSP_PART, &dev, &pnum, &part);
		if (ret) {
		        printf("No partition named %s\n", DSP_PART);
		        return;
		} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		        printf("Partition %s not a NAND device\n", DSP_PART);
		        return;
		}
		off = part->offset;
		nand = &nand_info[dev->id->num];
		flash_page_size = nand->writesize;
		size = (DSP_SIZE + (flash_page_size - 1)) & (~(flash_page_size - 1));
		if(size <= 0) {
		        printf("dsp image should not be zero\n");
		        return;
		}
		ret = nand_read_offset_ret(nand, off, &size, (void*)DSP_ADR, &off);
		if(ret != 0) {
		        printf("dsp nand read error %d\n", ret);
		        return;
		}

		printf("Reading firmware to 0x%08x\n", FIRMWARE_ADR);
		ret = find_dev_and_part(FIRMWARE_PART, &dev, &pnum, &part);
		if (ret) {
		        printf("No partition named %s\n", FIRMWARE_PART);
		        return;
		} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		        printf("Partition %s not a NAND device\n", FIRMWARE_PART);
		        return;
		}
		off = part->offset;
		nand = &nand_info[dev->id->num];
		size = (FIRMWARE_SIZE +(flash_page_size - 1)) & (~(flash_page_size - 1));
		if(size <= 0) {
		        printf("firmware image should not be zero\n");
		        return;
		}
		ret = nand_read_offset_ret(nand, off, &size, (void*)FIRMWARE_ADR, &off);
		if(ret != 0) {
		        printf("firmware nand read error %d\n", ret);
		        return;
		}

		printf("Reading vmjaluna to 0x%08x\n", VMJALUNA_ADR);
		ret = find_dev_and_part(VMJALUNA_PART, &dev, &pnum, &part);
		if (ret) {
		        printf("No partition named %s\n", VMJALUNA_PART);
		        return;
		} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		        printf("Partition %s not a NAND device\n", VMJALUNA_PART);
		        return;
		}
		off = part->offset;
		nand = &nand_info[dev->id->num];
		size = (VMJALUNA_SIZE +(flash_page_size - 1)) & (~(flash_page_size - 1));
		if(size <= 0) {
		        printf("modem image should not be zero\n");
		        return;
		}
		ret = nand_read_offset_ret(nand, off, &size, (void*)VMJALUNA_ADR, &off);
		if(ret != 0) {
		        printf("modem nand read error %d\n", ret);
		        return;
		}
		printf("call bootup modem in vlx_nand_boot,0x%x 0x%x\n",FIXNV_ADR, FIXNV_SIZE);
		{
			char * cmd_line_ptr;
			cmd_line_ptr = creat_cmdline(NULL,NULL);
			memset((PRODUCTINFO_ADR+16*1024),0,1024);
			strcpy((PRODUCTINFO_ADR+16*1024),cmd_line_ptr);
		}
		bootup_modem((char *)VMJALUNA_ADR,0x3000);
		calibration_mode(cmdline, 10);	
		memset(VMJALUNA_ADR,0,VMJALUNA_SIZE);
		memset(FIXNV_ADR,0,FIXNV_SIZE+4);
		memset(MODEM_ADR,0,MODEM_SIZE);
		memset(DSP_ADR,0,DSP_SIZE);
		memset(RUNTIMENV_ADR,0,RUNTIMENV_SIZE+4);
	}

#endif
	////////////////////////////////////////////////////////////////
	/* KERNEL_PART */
	printf("Reading kernel to 0x%08x\n", KERNEL_ADR);

	ret = find_dev_and_part(kernel_pname, &dev, &pnum, &part);
	if(ret){
		printf("No partition named %s\n", kernel_pname);
        return;
	}else if(dev->id->type != MTD_DEV_TYPE_NAND){
		printf("Partition %s not a NAND device\n", kernel_pname);
        return;
	}

	off=part->offset;
	nand = &nand_info[dev->id->num];
	//read boot image header
#if 0
	size = nand->writesize;
	flash_page_size = nand->writesize;
	ret = nand_read_offset_ret(nand, off, &size, (void *)hdr, &off);
	if(ret != 0){
		printf("function: %s nand read error %d\n", __FUNCTION__, ret);
        return;
	}
	if(memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)){
		printf("bad boot image header, give up read!!!!\n");
        return;
	}
	else
	{
		//read kernel image
		size = (hdr->kernel_size+(flash_page_size - 1)) & (~(flash_page_size - 1));
		if(size <=0){
			printf("kernel image should not be zero\n");
			return;
		}
		ret = nand_read_offset_ret(nand, off, &size, (void *)KERNEL_ADR, &off);
		if(ret != 0){
			printf("kernel nand read error %d\n", ret);
			return;
		}
		//read ramdisk image
		size = (hdr->ramdisk_size+(flash_page_size - 1)) & (~(flash_page_size - 1));
		if(size<0){
			printf("ramdisk size error\n");
			return;
		}
		ret = nand_read_offset_ret(nand, off, &size, (void *)RAMDISK_ADR, &off);
		if(ret != 0){
			printf("ramdisk nand read error %d\n", ret);
			return;
		}
	}
#else

	ret = load_kernel_and_layout(nand,
							(unsigned int)off,
							(char *)raw_header,
							(char *) KERNEL_ADR,
							(char *) RAMDISK_ADR,
							2048,
							nand->writesize);

	if (ret != 0) {
		printf("ramdisk nand read error %d\n", ret);
		return;
	}

#endif

#if !(BOOT_NATIVE_LINUX)
	////////////////////////////////////////////////////////////////
	/* MODEM_PART */
	printf("Reading modem to 0x%08x\n", MODEM_ADR);
	ret = find_dev_and_part(MODEM_PART, &dev, &pnum, &part);
	if (ret) {
		printf("No partition named %s\n", MODEM_PART);
		return;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		printf("Partition %s not a NAND device\n", MODEM_PART);
		return;
	}

	off = part->offset;
	nand = &nand_info[dev->id->num];
	flash_page_size = nand->writesize;
	size = (MODEM_SIZE +(flash_page_size - 1)) & (~(flash_page_size - 1));
	if(size <= 0) {
		printf("modem image should not be zero\n");
		return;
	}
	ret = nand_read_offset_ret(nand, off, &size, (void*)MODEM_ADR, &off);
	if(ret != 0) {
		printf("modem nand read error %d\n", ret);
		return;
	}

	secure_check(MODEM_ADR, 0, MODEM_ADR + MODEM_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
	//array_value((unsigned char *)MODEM_ADR, MODEM_SIZE);

	////////////////////////////////////////////////////////////////
	/* VMJALUNA_PART */
	printf("Reading vmjaluna to 0x%08x\n", VMJALUNA_ADR);
	ret = find_dev_and_part(VMJALUNA_PART, &dev, &pnum, &part);
	if (ret) {
		printf("No partition named %s\n", VMJALUNA_PART);
		return;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		printf("Partition %s not a NAND device\n", VMJALUNA_PART);
		return;
	}

	off = part->offset;
	nand = &nand_info[dev->id->num];
	size = (VMJALUNA_SIZE +(flash_page_size - 1)) & (~(flash_page_size - 1));
	if(size <= 0) {
		printf("VMJALUNA image should not be zero\n");
		return;
	}
	ret = nand_read_offset_ret(nand, off, &size, (void*)VMJALUNA_ADR, &off);
	if(ret != 0) {
		printf("VMJALUNA nand read error %d\n", ret);
		return;
	}
	secure_check(VMJALUNA_ADR, 0, VMJALUNA_ADR + VMJALUNA_SIZE - VLR_INFO_OFF, CONFIG_SYS_NAND_U_BOOT_DST + CONFIG_SYS_NAND_U_BOOT_SIZE - KEY_INFO_SIZ - VLR_INFO_OFF);
#endif
	creat_cmdline(cmdline,hdr);
	vlx_entry();
}

