/*
 * This file is produced by tools!!
 *
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/io.h>
#include <asm/arch/pinmap.h>

#define PIN_NULL    0


struct other_pin_ctr_reg {
    uint32_t reg;               /*pin register offset*/


    uint32_t wpus:1;            /*[12] pull up resistor select*/
                                #define PIN_WPUS    1

    uint32_t func_sel:2;        /*[5:4]function slect.*/
                                /*value of .func_sel*/
                                #define FUNC0       0   /*function0*/
                                #define FUNC1       1   /*function1*/
                                #define FUNC2       2   /*function2*/
                                #define FUNC3       3   /*function3*/

    uint32_t func_wpu_wpd:2;    /*[7:6] weakly pull up/down for function mode*/
                                /*value of .func_wpu_wpd*/
                                #define FUNC_WPU        (1<<1)       /*weakly pull up for function mode*/
                                #define FUNC_WPD        (1<<0)       /*weakly pull down for function mode*/

    uint32_t slp_wpu_wpd:2;     /*[3:2]weak pull up/down for chip deep sleep mode*/
                                /*value of .slp_wpu_wpd*/
                                #define SLP_WPU     (1<<1) /*weakly pull up for chip deep sleep mode*/
                                #define SLP_WPD     (1<<0) /*weakly pull down for chip deep sleep mode*/

    uint32_t drv:3;             /*[10:8] driver strength select.*/
                                /*value of .drv*/
                                #define DS_L0       0
                                #define DS_L1       1
                                #define DS_L2       2
                                #define DS_L3       3
                                #define DS_L4       4
                                #define DS_L5       5
                                #define DS_L6       6
                                #define DS_L7       7

    uint32_t ie_oe:2;           /*[1:0]input/output enable for chip deep sleep mode*/
                                /*value of .ie_oe*/
                                #define SLP_IE      (1<<1) /* input enable for chip deep sleep mode*/
                                #define SLP_OE      (1<<0) /*output enable for chip deep sleep mode*/

    uint32_t se:1;              /*[11] schmitt trigger input enalbe*/
                                #define PIN_SCHMITT 1

    uint32_t slp_en:4;          /*[16:13] sleep mode bit map: SLP_AP|SLP_CP0|SLP_CP1|SLP_CP2 */
                                #define SLP_AP      BIT_0   /* sleep with AP*/
                                #define SLP_CP0     BIT_1   /* sleep with CP0*/
                                #define SLP_CP1     BIT_2   /* sleep with CP1*/
                                #define SLP_CP2     BIT_3   /* sleep with CP2*/
};


struct other_pin_ctr_reg other[] = {

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_TRACECLK,          PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_TRACECTRL,         PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT0,         PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT1,         PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT2,         PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT3,         PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT4,         PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT5,         PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_TRACEDAT6,         PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_TRACEDAT7,         PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U0TXD,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_U0RXD,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U0CTS,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U0RTS,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U1TXD,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_U1RXD,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U2TXD,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_U2RXD,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U3TXD,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_U3RXD,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U3CTS,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U3RTS,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_EXTINT2,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EXTINT3,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSDA2,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSCK2,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSEN2,            PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP2_RFCTL0,        PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP2_RFCTL1,        PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_CP2_RFCTL2,        PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_FM_RXIQD0,         PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_FM_RXIQD1,         PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN0,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN1,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN2,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN3,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN4,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN5,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WIFI_AGCGAIN6,     PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_WBENA,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_WBENB,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPSREAL,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPSIMAG,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPSCLK,            PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSDA0,            PIN_WPUS,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSCK0,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSEN0,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSDA1,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_RFSCK1,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_RFSEN1,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL0,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL1,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL2,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL3,        PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL4,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL5,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL6,        PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL7,        PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL8,        PIN_NULL,   FUNC1,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_CP1_RFCTL9,        PIN_NULL,   FUNC1,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL10,       PIN_NULL,   FUNC1,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL11,       PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL12,       PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL13,       PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL14,       PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP1_RFCTL15,       PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL0,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL1,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL2,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_CP0_RFCTL3,        PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL4,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL5,        PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL6,        PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CP0_RFCTL7,        PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_XTLEN,             PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO6,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO7,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO8,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO9,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_U4TXD,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_U4RXD,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_U4CTS,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_U4RTS,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SCL3,              PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SDA3,              PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SPI0_CSN,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SPI0_DO,           PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SPI0_DI,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SPI0_CLK,          PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_EXTINT0,           PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_EXTINT1,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SCL1,              PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SDA1,              PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO0,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO1,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO2,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO3,             PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SIMCLK0,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMDA0,            PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_SIMRST0,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMCLK1,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMDA1,            PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMRST1,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMCLK2,           PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMDA2,            PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SIMRST2,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_MEMS_MIC_CLK0,     PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_MEMS_MIC_DATA0,    PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_MEMS_MIC_CLK1,     PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_MEMS_MIC_DATA1,    PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD1_CLK,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD1_CMD,           PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD1_D0,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD1_D1,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD1_D2,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD1_D3,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD0_D3,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD0_D2,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD0_CMD,           PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_SD0_D0,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD0_D1,            PIN_NULL,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD0_CLK1,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SD0_CLK0,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_PTEST,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_ANA_INT,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EXT_RST_B,         PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CHIP_SLEEP,        PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_XTL_BUF_EN0,       PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_XTL_BUF_EN1,       PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_XTL_BUF_EN2,       PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CLK_32K,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_SCLK,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_DANGL,         PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_DANGR,         PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_ADD0,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_ADSYNC,        PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_DAD1,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_DAD0,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_AUD_DASYNC,        PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_ADI_D,             PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_ADI_SYNC,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_ADI_SCLK,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_CSN1,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_CSN0,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_RSTN,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_CD,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_FMARK,         PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_WRN,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_RDN,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_LCD_D0,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D1,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D2,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D3,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D4,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D5,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D6,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D7,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D8,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D9,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_LCD_D10,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D11,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D12,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D13,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D14,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D15,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D16,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D17,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D18,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D19,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_LCD_D20,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D21,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D22,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_LCD_D23,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SPI2_CSN,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SPI2_DO,           PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SPI2_DI,           PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SPI2_CLK,          PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_CLK,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L2,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_CMD,          PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_EMMC_D0,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D1,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D2,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D3,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D4,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D5,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D6,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_D7,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_EMMC_RST,          PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFWPN,             PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_NFRB,              PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFCLE,             PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFALE,             PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFCEN0,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFCEN1,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFREN,             PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFWEN,             PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFD0,              PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFD1,              PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFD2,              PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_NFD3,              PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L2,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFD4,              PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFD5,              PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_NFD6,              PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD7,              PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD8,              PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD9,              PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD10,             PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD11,             PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD12,             PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_NFD13,             PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD14,             PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_NFD15,             PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRCK0,           PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRCK1,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRMCLK,          PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRHS,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRVS,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD0,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD1,            PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_CCIRD2,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD3,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD4,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD5,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD6,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD7,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD8,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRD9,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRRST,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_CCIRPD1,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_CCIRPD0,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SCL0,              PIN_WPUS,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_SDA0,              PIN_WPUS,   FUNC0,   FUNC_WPU,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT0,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT1,           PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT2,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT3,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT4,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT5,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYOUT6,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_KEYOUT7,           PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN0,            PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN1,            PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN2,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN3,            PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN4,            PIN_NULL,   FUNC3,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN5,            PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN6,            PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_KEYIN7,            PIN_NULL,   FUNC3,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_GPIO4,             PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_GPIO5,             PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SCL2,              PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_SDA2,              PIN_WPUS,   FUNC0,   FUNC_WPU,    SLP_WPU,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_CLK_AUX0,          PIN_NULL,   FUNC0,   PIN_NULL,    PIN_NULL,     DS_L1,    SLP_OE,          PIN_NULL,      PIN_NULL},
{REG_PIN_IIS0DI,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS0DO,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS0CLK,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS0LRCK,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS0MCK,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS1DI,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_IIS1DO,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS1CLK,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS1LRCK,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS1MCK,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS2DI,            PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS2DO,            PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS2CLK,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS2LRCK,          PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_IIS2MCK,           PIN_NULL,   FUNC0,   FUNC_WPD,    SLP_WPD,      DS_L1,    SLP_IE,          PIN_NULL,      PIN_NULL},
{REG_PIN_MTDO,              PIN_NULL,   FUNC0,   PIN_NULL,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},

/*    reg                  |pin pull up|function|func pull up|sleep pull up|drv level|sleep i/o enable|schmitt enable|sleep enable */
{REG_PIN_MTDI,              PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_MTCK,              PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_MTMS,              PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
{REG_PIN_MTRST_N,           PIN_NULL,   FUNC0,   FUNC_WPU,    SLP_WPD,      DS_L1,    PIN_NULL,        PIN_NULL,      PIN_NULL},
};




static pinmap_t  ctrl[] = {
{REG_PIN_CTRL0,			0},
{REG_PIN_CTRL1,			0},
{REG_PIN_CTRL2,			(1<<22)|(0<<20)|(1<<16)|(2<<10)},
{REG_PIN_CTRL3,			0},
};



int  pin_init(void)
{
    int i;
    uint32_t v;

    /*ctrl pin init*/
    for (i = 0; i < sizeof(ctrl)/sizeof(ctrl[0]); i++) {
        __raw_writel(ctrl[i].val, CTL_PIN_BASE + ctrl[i].reg);
    }

    /*other pin init*/
    for (i = 0; i < sizeof(other)/sizeof(other[0]); i++) {
        v  = ((other[i].slp_en & 0xF)    << 13); /*[16:13]*/
        v |= ((other[i].wpus & 1)        << 12); /*[12]*/
        v |= ((other[i].se & 1)          << 11); /*[11]e*/
        v |= ((other[i].drv & 7)         << 8);  /*[10:8]*/
        v |= ((other[i].func_wpu_wpd & 3)<< 6);  /*[7:6]*/
        v |= ((other[i].func_sel & 3)    << 4);  /*[5:4]*/
        v |= ((other[i].slp_wpu_wpd & 3) << 2);  /*[3:2]*/
        v |= ((other[i].ie_oe & 3)       << 0);  /*[1:0]*/

        __raw_writel(v, CTL_PIN_BASE + other[i].reg);
    }

    return 0;
}

