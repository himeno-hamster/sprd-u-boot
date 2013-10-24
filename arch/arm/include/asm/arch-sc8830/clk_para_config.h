/******************************************************************************
 ** File Name:                                                                *
 ** Author:                                                                   *
 ** DATE:                                                                     *
 ** Copyright:      2002 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic input and output operations   *
 **                 on hardware, it can be treated as a hardware abstract     *
 **                 layer interface.                                          *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 06/12/2010                                                                *
 ******************************************************************************/

#ifndef _CLK_PARA_CONFIG_H_
#define _CLK_PARA_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

enum clk_ap_ahb_sel
{
    AHB_CLK_26M,
    AHB_CLK_76_8M,
    AHB_CLK_128M,
    AHB_CLK_192M
};
enum clk_ap_apb_sel
{
    APB_CLK_26M,
    APB_CLK_64M,
    APB_CLK_96M,
    APB_CLK_128M
};
enum clk_pub_ahb_sel
{
    PUB_AHB_CLK_26M,
    PUB_AHB_CLK_96M,
    PUB_AHB_CLK_128M,
    PUB_AHB_CLK_153_6M
};
enum clk_aon_apb_sel
{
    AON_APB_CLK_26M,
    AON_APB_CLK_76_8M,
    AON_APB_CLK_96M,
    AON_APB_CLK_128M
};
typedef struct{
    uint32 magic_header;
    uint8  version;
    uint8  reserved0;
    uint8  reserved1;
    uint8  reserved2;
    uint32 core_freq;
    uint32 ddr_freq;
    uint32 axi_freq;
    uint32 dgb_freq;
    uint32 ahb_freq;
    uint32 apb_freq;
    uint32 pub_ahb_freq;
    uint32 aon_apb_freq;
    uint32 dcdc_arm;
    uint32 dcdc_core;
    uint32 magic_end;
}MCU_CLK_PARA_T;

extern const MCU_CLK_PARA_T mcu_clk_para;

#ifdef __cplusplus
}
#endif

#endif
