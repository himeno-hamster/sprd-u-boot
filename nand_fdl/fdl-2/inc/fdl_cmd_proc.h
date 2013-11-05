#ifndef FDL_CMD_PROC_H
#define FDL_CMD_PROC_H

#include "sci_types.h"
#include "fdl_conf.h"
#include "packet.h"
#include "fdl_stdio.h"
#include "asm/arch/sci_types.h"
#ifdef CONFIG_EMMC_BOOT
#include "fdl_emmc_operate.h"
#else
#include "fdl_nand_operate.h"
#endif
int FDL2_Download_Start (PACKET_T *packet, void *arg);
int FDL2_Download_Midst(PACKET_T *packet, void *arg);
int FDL2_Download_End (PACKET_T *packet, void *arg);
int FDL2_Read_Start(PACKET_T *packet, void *arg);
int FDL2_Read_Midst(PACKET_T *packet, void *arg);
int FDL2_Read_End(PACKET_T *packet, void *arg);
int FDL2_Erase(PACKET_T *packet, void *arg);
int FDL2_Repartition (PACKET_T *pakcet, void *arg);


#endif  /*FDL_CMD_PROC_H*/