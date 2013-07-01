#ifndef _FDL_EMMC_OPERATE_H
#define _FDL_EMMC_OPERATE_H

#include <asm/arch/fdl_stdio.h>
#include <asm/arch/cmd_def.h>
#include <asm/arch/packet.h>
#ifdef CONFIG_EMMC_BOOT

PUBLIC int fdl2_emmc_download_start(unsigned long start_addr, unsigned long size, unsigned long nv_checksum);
PUBLIC int fdl2_emmc_download(unsigned short size, char *buf);
PUBLIC int fdl2_emmc_download_end(void);
PUBLIC int fdl2_emmc_read(unsigned long addr, unsigned long size, unsigned long off, unsigned char *buf);
PUBLIC int fdl2_emmc_erase(unsigned long addr, unsigned long size);
PUBLIC int fdl2_emmc_repartition(void);

#endif  //CONFIG_EMMC_BOOT
#endif  //_FDL_EMMC_OPERATE_H
