/*
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

#ifndef __ASM_ARCH_SPRD_IRQS_SC8830_H
#define __ASM_ARCH_SPRD_IRQS_SC8830_H

#define IRQ_SPECIAL_LATCH		0
#define IRQ_SOFT_TRIGGED0_INT		1
#define IRQ_SER0_INT			2
#define IRQ_SER1_INT			3
#define IRQ_SER2_INT			4
#define IRQ_SER3_INT			5
#define IRQ_SER4_INT			6
#define IRQ_SPI0_INT			7
#define IRQ_SPI1_INT			8
#define IRQ_SPI2_INT			9
#define IRQ_SIM0_INT 			10
#define IRQ_I2C0_INT 			11
#define IRQ_I2C1_INT 			12
#define IRQ_I2C2_INT 			13
#define IRQ_I2C3_INT 			14
#define IRQ_I2C4_INT 			15
#define IRQ_IIS0_INT			16
#define IRQ_IIS1_INT			17
#define IRQ_IIS2_INT			18
#define IRQ_IIS3_INT			19
#define IRQ_REQ_AUD_INT			20
#define IRQ_REQ_AUD_VBC_AFIFO_INT	21
#define IRQ_REQ_AUD_VBC_DA_INT		22
#define IRQ_REQ_AUD_VBC_AD01_INT	23
#define IRQ_REQ_AUD_VBC_AD23_INT	24
#define IRQ_ADI_INT			25
#define IRQ_THM_INT			26
#define IRQ_FM_INT			27
#define IRQ_AONTMR0_INT			28
#define IRQ_APTMR0_INT			29
#define IRQ_AONSYST_INT			30
#define IRQ_APSYST_INT			31

#define IRQ_AONI2C_INT			34
#define IRQ_GPIO_INT			35
#define IRQ_KPD_INT			36
#define IRQ_EIC_INT			37
#define IRQ_ANA_INT			38
#define IRQ_GPU_INT			39
#define IRQ_CSI_INT0			40
#define IRQ_CSI_INT1			41
#define IRQ_JGP_INT			42
#define IRQ_VSP_INT			43
#define IRQ_ISP_INT			44
#define IRQ_DCAM_INT			45
#define IRQ_DISPC0_INT			46
#define IRQ_DISPC1_INT			47
#define IRQ_DSI0_INT			48
#define IRQ_DSI1_INT			49
#define IRQ_DMA_INT			50
#define IRQ_GSP_INT			51
#define IRQ_GPS_INT			52
#define IRQ_GPS_RTCEXP_INT		53
#define IRQ_GPS_WAKEUP_INT		54
#define IRQ_USBD_INT			55
#define IRQ_NFC_INT			56
#define IRQ_SDIO0_INT			57
#define IRQ_SDIO1_INT			58
#define IRQ_SDIO2_INT			59
#define IRQ_EMMC_INT			60
#define IRQ_BM0_INT			61
#define IRQ_BM1_INT			62
#define IRQ_BM2_INT			63
#define IRQ_DRM_INT			66
#define IRQ_CP0_DSP_INT			67
#define IRQ_CP0_MCU0_INT		68
#define IRQ_CP0_MCU1_INT		69
#define IRQ_CP1_DSP_INT			70
#define IRQ_CP1_MCU0_INT		71
#define IRQ_CP1_MCU1_INT		72
#define IRQ_CP2_INT0_INT		73
#define IRQ_CP2_INT1_INT		74
#define IRQ_CP0_DSP_FIQ_INT		75
#define IRQ_CP0_MCU_FIQ0_INT		76
#define IRQ_CP0_MCU_FIQ1_INT		77
#define IRQ_CP1_MCU_FIQ_INT		78
#define IRQ_NFC_INT			79
#define IRQ_NFC_INT			80
#define IRQ_NFC_INT			81
#define IRQ_NFC_INT			82
#define IRQ_NFC_INT			83
#define IRQ_NFC_INT			84
#define IRQ_NFC_INT			85
#define IRQ_NFC_INT			86
#define IRQ_NFC_INT			87
#define IRQ_NFC_INT			88
#define IRQ_NFC_INT			89
#define IRQ_NFC_INT			90
#define IRQ_NFC_INT			91
#define IRQ_NPMUIRQ0_INT		92
#define IRQ_NPMUIRQ1_INT		93
#define IRQ_NPMUIRQ2_INT		94
#define IRQ_NPMUIRQ3_INT		95
#define IRQ_CA7COM0_INT			98
#define IRQ_CA7COM1_INT			99
#define IRQ_CA7COM2_INT			100
#define IRQ_CA7COM3_INT			101
#define IRQ_NCNTV0_INT			102
#define IRQ_NCNTV1_INT			103
#define IRQ_NCNTV2_INT			104
#define IRQ_NCNTV3_INT			105
#define IRQ_NCNTHP0_INT			106
#define IRQ_NCNTHP1_INT			107
#define IRQ_NCNTHP2_INT			108
#define IRQ_NCNTHP3_INT			109
#define IRQ_NCNTPN0_INT			110
#define IRQ_NCNTPN1_INT			111
#define IRQ_NCNTPN2_INT			112
#define IRQ_NCNTPN3_INT			113
#define IRQ_NCNTPS0_INT			114
#define IRQ_NCNTPS1_INT			115
#define IRQ_NCNTPS2_INT			116
#define IRQ_NCNTPS3_INT			117
#define IRQ_APTMR1_INT			118
#define IRQ_APTMR2_INT			119
#define IRQ_APTMR3_INT			120
#define IRQ_APTMR4_INT			121
#define IRQ_AVS_INT			122
#define IRQ_APWDG_INT			123
#define IRQ_CA7WDG_INT			124

/* analog die interrupt number */
#define IRQ_ANA_ADC_INT			0
#define IRQ_ANA_GPIO_INT		1
#define IRQ_ANA_RTC_INT			2
#define IRQ_ANA_WDG_INT			3
#define IRQ_ANA_TPC_INT			4
#define IRQ_ANA_EIC_INT			5
#define IRQ_ANA_CHGRWDG_INT		6
#define IRQ_ANA_AUD_INT			7
#define IRQ_ANA_DCDC_OTP_INT		8
#define IRQ_ANA_INT_START		IRQ_ANA_ADC_INT
#define NR_ANA_IRQS			(9)
#define GPIO_IRQ_START			9

#endif
