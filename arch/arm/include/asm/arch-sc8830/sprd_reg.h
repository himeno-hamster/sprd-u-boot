
#ifndef _SC8830_REG_DEF_H_
#define _SC8830_REG_DEF_H_

#ifndef BIT
#define BIT(x) (1<<x)
#endif



#include "hardware.h"
#if defined CONFIG_SPX15
#include "chip_x15/__regs_ap_apb.h"
#include "chip_x15/__regs_ap_ahb.h"
//#include "chip_x15/sprd_reg_ap_clk.h"
#include "chip_x15/__regs_mm_ahb_rf.h"
//#include "chip_x15/sprd_reg_mm_clk.h"
//#include "chip_x15/sprd_reg_gpu_apb.h"
//#include "chip_x15/sprd_reg_gpu_clk.h"
#include "chip_x15/__regs_aon_apb.h"
//#include "chip_x15/sprd_reg_aon_ckg.h"
//#include "chip_x15/sprd_reg_aon_clk.h"
#include "chip_x15/__regs_ana_apb_if.h"
#include "chip_x15/__regs_pmu_apb.h"
#include "chip_x15/__regs_pub_apb.h"

#else 
#include "chip_x35/sprd_reg_ap_apb.h"
#include "chip_x35/sprd_reg_ap_ahb.h"
#include "chip_x35/sprd_reg_ap_clk.h"
#include "chip_x35/sprd_reg_mm_ahb.h"
#include "chip_x35/sprd_reg_mm_clk.h"
#include "chip_x35/sprd_reg_gpu_apb.h"
#include "chip_x35/sprd_reg_gpu_clk.h"
#include "chip_x35/sprd_reg_aon_apb.h"
#include "chip_x35/sprd_reg_aon_ckg.h"
#include "chip_x35/sprd_reg_aon_clk.h"
#include "chip_x35/sprd_reg_ana_glb.h"
#include "chip_x35/sprd_reg_pmu_apb.h"
#include "chip_x35/sprd_reg_pub_apb.h"
#endif
#endif

