/*
 * sound/soc/sprd/codec/sprd/sprd-codec.h
 *
 * SPRD-CODEC -- SpreadTrum Tiger intergrated codec.
 *
 * Copyright (C) 2012 SpreadTrum Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY ork FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __SPRD_CODEC_H
#define __SPRD_CODEC_H

#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_global.h>
#include <asm/arch/sprd-audio.h>
#include <asm/arch/adi.h>
#include <asm/io.h>

#ifndef CONFIG_SPRD_CODEC_USE_INT
/* #define CONFIG_SPRD_CODEC_USE_INT */
#endif
#ifndef CONFIG_CODEC_DAC_MUTE_WAIT
/* #define CONFIG_CODEC_DAC_MUTE_WAIT */
#endif

/* unit: ms */
#define SPRD_CODEC_LDO_WAIT_TIME	(5)
#define SPRD_CODEC_LDO_VCM_TIME		(2)
#ifdef CONFIG_SPRD_CODEC_USE_INT
#define SPRD_CODEC_DAC_MUTE_TIMEOUT	(600)
#else
#define SPRD_CODEC_DAC_MUTE_WAIT_TIME	(40)
#endif

#ifdef CONFIG_SPRD_CODEC_USE_INT
#define SPRD_CODEC_HP_POP_TIMEOUT	(1000)
#else
#define SPRD_CODEC_HP_POP_TIME_STEP	(10)
#define SPRD_CODEC_HP_POP_TIME_COUNT	(80)	/* max 800ms will timeout */
#endif

#define SPRD_CODEC_RATE_8000   (10)
#define SPRD_CODEC_RATE_9600   ( 9)
#define SPRD_CODEC_RATE_11025  ( 8)
#define SPRD_CODEC_RATE_12000  ( 7)
#define SPRD_CODEC_RATE_16000  ( 6)
#define SPRD_CODEC_RATE_22050  ( 5)
#define SPRD_CODEC_RATE_24000  ( 4)
#define SPRD_CODEC_RATE_32000  ( 3)
#define SPRD_CODEC_RATE_44100  ( 2)
#define SPRD_CODEC_RATE_48000  ( 1)
#define SPRD_CODEC_RATE_96000  ( 0)

/* AUD_TOP_CTL */
#define DAC_EN_L		(0)
#define ADC_EN_L		(1)
#define DAC_EN_R		(2)
#define ADC_EN_R		(3)

/* AUD_DAC_CTL */
#define DAC_MUTE_START		(14)
#define DAC_MUTE_EN		(15)

/* AUD_AUD_STS0 */
#define DAC_MUTE_U_MASK		(5)
#define DAC_MUTE_D_MASK		(4)
#define DAC_MUTE_U_RAW		(3)
#define DAC_MUTE_D_RAW		(2)
#define DAC_MUTE_ST		(0)
#define DAC_MUTE_ST_MASK	(0x3)

/* AUD_INT_CLR */
/* AUD_INT_EN */
#define DAC_MUTE_U		(1)
#define DAC_MUTE_D		(0)

/* AUDIF_ENB */
#define AUDIFA_DAC_EN		(0)
#define AUDIFA_ADC_EN		(1)
#define AUDIFA_ADIE_LOOP_EN	(3)

/* AUDIF_INT_EN */
/* AUDIF_INT_CLR */
/* AUDIF_INT_RAW */
/* AUDIF_INT_MASK */
#define AUDIO_POP_IRQ		(7)
#define OVP_IRQ			(6)
#define OTP_IRQ			(5)
#define PA_OCP_IRQ		(4)
#define LOR_OCP_IRQ		(3)
#define LOL_OCP_IRQ		(2)
#define EAR_OCP_IRQ		(1)
#define HP_OCP_IRQ		(0)

/* PMUR1 */
#define VB_EN			(7)
#define BG_EN			(6)
#define BG_IBIAS_EN		(5)
#define VCM_EN			(4)
#define VCM_BUF_EN		(3)
#define VBO_EN			(2)
#define MICBIAS_EN		(1)
#define AUXMICBIAS_EN		(0)

/* PMUR2 */
#define PA_SW_EN		(7)
#define PA_LDO_EN		(6)
#define PA_EN			(5)
#define PAR_SW_EN		(4)
#define PAR_LDO_EN		(3)
#define PAR_EN			(2)
#define OVP_PD			(1)
#define OVP_LDO_EN		(0)

#define LDO_V_29		(0)
#define LDO_V_31		(1)
#define LDO_V_32		(2)
#define LDO_V_33		(3)
#define LDO_V_34		(4)
#define LDO_V_35		(5)
#define LDO_V_36		(6)
#define LDO_V_38		(7)

#define MIC_LDO_V_21		(0)
#define MIC_LDO_V_19		(1)
#define MIC_LDO_V_23		(2)
#define MIC_LDO_V_25		(3)

/* PMUR3 */
#define VCM_V			(5)
#define VCM_V_MASK		(0x7)
#define MICBIAS_V		(3)
#define MICBIAS_V_MASK		(0x3)
#define AUXMICBIAS_V		(1)
#define AUXMICBIAS_V_MASK	(0x3)

/* PMUR4 */
#define PA_SWOCP_PD		(7)
#define PA_LDOOCP_PD		(6)
#define PA_LDO_V		(3)
#define PA_LDO_V_MASK		(0x7)
#define BG_RST			(2)
#define BG_I			(0)
#define BG_I_MASK		(0x3)

/* AACR1 */
#define ADC_IBUF_PD		(7)
#define ADC_VREF1P5		(6)
#define ADCL_PD			(5)
#define ADCL_RST		(4)
#define ADCR_PD			(3)
#define ADCR_RST		(2)
#define FM_REC_EN		(1)

/* AACR2 */
#define ADCPGAL_PD		(7)
#define ADCPGAL_BYP		(6)
#define ADCPGAR_PD		(5)
#define ADCPGAR_BYP		(4)
#define ADCPGAL_P_EN		(3)
#define ADCPGAL_N_EN		(2)
#define ADCPGAR_P_EN		(1)
#define ADCPGAR_N_EN		(0)

/* AAICR1 */
#define MICP_ADCL		(7)
#define MICN_ADCL		(6)
#define MICP_ADCR		(5)
#define MICN_ADCR		(4)
#define HPMICP_ADCL		(3)
#define HPMICN_ADCL		(2)
#define HPMICP_ADCR		(1)
#define HPMICN_ADCR		(0)

/* AAICR2 */
#define AUXMICP_ADCL		(7)
#define AUXMICN_ADCL		(6)
#define AUXMICP_ADCR		(5)
#define AUXMICN_ADCR		(4)

/* AAICR3 */
#define AIL_ADCL		(7)
#define AIR_ADCL		(6)
#define VCM_ADCL		(5)
#define AIL_ADCR		(4)
#define AIR_ADCR		(3)
#define VCM_ADCR		(2)

/* DACR */
#define DACL_EN			(7)
#define DACR_EN			(6)
#define DACBUF_I		(5)

/* DAOCR1 */
#define ADCL_P_HPL		(7)
#define ADCL_N_HPR		(6)
#define ADCR_P_HPL		(5)
#define ADCR_P_HPR		(4)
#define DACL_P_HPL		(3)
#define DACL_N_HPR		(2)
#define DACR_P_HPL		(1)
#define DACR_P_HPR		(0)

/* DAOCR2 */
#define ADCL_P_AOLP		(7)
#define ADCL_N_AOLN		(6)
#define ADCR_P_AOLP		(5)
#define ADCR_N_AOLN		(4)
#define DACL_P_AOLP		(3)
#define DACL_N_AOLN		(2)
#define DACR_P_AOLP		(1)
#define DACR_N_AOLN		(0)

/* DAOCR3 */
#define ADCL_P_AORP		(7)
#define ADCL_N_AORN		(6)
#define ADCR_P_AORP		(5)
#define ADCR_N_AORN		(4)
#define DACL_P_AORP		(3)
#define DACL_N_AORN		(2)
#define DACR_P_AORP		(1)
#define DACR_N_AORN		(0)

/* DAOCR4 */
#define DACL_P_EARP		(7)
#define DACL_N_EARN		(6)

/* DCR1 */
#define HPL_EN			(7)
#define HPR_EN			(6)
#define EAR_EN			(5)
#define AOL_EN			(4)
#define AOR_EN			(3)
#define DIFF_EN			(2)

/* DCR2 */
#define PA_D_EN			(7)
#define PA_DTRI_F		(5)
#define PA_DTRI_F_MASK		(0x03)
#define PA_DEMI_EN		(4)
#define PA_SS_EN		(3)
#define PA_SS_RST		(2)
#define DRV_STOP_EN		(1)

/* DCR3 */
#define DRV_OCP_AOL_PD		(7)
#define DRV_OCP_AOR_PD		(6)
#define DRV_OCP_EAR_PD		(5)
#define DRV_OCP_HP_PD		(4)

/* PNRCR1 */
#define HP_POP_CTL		(6)
#define HP_POP_CTL_MASK		(0x03)
#define HP_POP_CTL_DIS		(0)
#define HP_POP_CTL_UP		(1)
#define HP_POP_CTL_DOWN		(2)
#define HP_POP_CTL_HOLD		(3)

#define HP_POP_STEP		(3)
#define HP_POP_STEP_MASK	(0x07)
#define HP_POP_STEP_012		(0)
#define HP_POP_STEP_025		(1)
#define HP_POP_STEP_05		(2)
#define HP_POP_STEP_1		(3)
#define HP_POP_STEP_2		(4)
#define HP_POP_STEP_4		(5)
#define HP_POP_STEP_8		(6)
#define HP_POP_STEP_16		(7)

/* CCR */
#define CLK_REVERSE		(7)
#define ADC_CLK_PD		(6)
#define ADC_CLK_RST		(5)
#define DAC_CLK_EN		(4)
#define DRV_CLK_EN		(3)

/* IFR1 */
#define HP_POP_FLG		(4)
#define HP_POP_FLG_MASK		(0x03)
#define HP_POP_FLG_NEAR_CMP	(3)

#define SPRD_CODEC_DP_BASE (CODEC_DP_BASE)

#define AUD_TOP_CTL		(SPRD_CODEC_DP_BASE + 0x0000)
#define AUD_AUD_CTL		(SPRD_CODEC_DP_BASE + 0x0004)
#define AUD_I2S_CTL		(SPRD_CODEC_DP_BASE + 0x0008)
#define AUD_DAC_CTL		(SPRD_CODEC_DP_BASE + 0x000C)
#define AUD_SDM_CTL0		(SPRD_CODEC_DP_BASE + 0x0010)
#define AUD_SDM_CTL1		(SPRD_CODEC_DP_BASE + 0x0014)
#define AUD_ADC_CTL		(SPRD_CODEC_DP_BASE + 0x0018)
#define AUD_LOOP_CTL		(SPRD_CODEC_DP_BASE + 0x001C)
#define AUD_AUD_STS0		(SPRD_CODEC_DP_BASE + 0x0020)
#define AUD_INT_CLR		(SPRD_CODEC_DP_BASE + 0x0024)
#define AUD_INT_EN		(SPRD_CODEC_DP_BASE + 0x0028)

#define SPRD_CODEC_DP_END	(SPRD_CODEC_DP_BASE + 0x002C)
#define IS_SPRD_CODEC_DP_RANG(reg) (((reg) >= SPRD_CODEC_DP_BASE) && ((reg) < SPRD_CODEC_DP_END))

#define SPRD_CODEC_AP_BASE (CODEC_AP_BASE)

#define AUDIF_ENB		(SPRD_CODEC_AP_BASE + 0x0000)
#define AUDIF_CLR		(SPRD_CODEC_AP_BASE + 0x0004)
#define AUDIF_SYNC_CTL		(SPRD_CODEC_AP_BASE + 0x0008)
#define AUDIF_OCPTMR_CTL	(SPRD_CODEC_AP_BASE + 0x000C)
#define AUDIF_OTPTMR_CTL	(SPRD_CODEC_AP_BASE + 0x0010)
#define AUDIF_SHUTDOWN_CTL	(SPRD_CODEC_AP_BASE + 0x0014)
#define AUDIF_INT_EN		(SPRD_CODEC_AP_BASE + 0x0018)
#define AUDIF_INT_CLR		(SPRD_CODEC_AP_BASE + 0x001C)
#define AUDIF_INT_RAW		(SPRD_CODEC_AP_BASE + 0x0020)
#define AUDIF_INT_MASK		(SPRD_CODEC_AP_BASE + 0x0024)
#define AUDIF_OVPTMR_CTL	(SPRD_CODEC_AP_BASE + 0x0028)
/* 0x002C ~ 0x003C is reserved for ADIE digital part */

#define PMUR1			(SPRD_CODEC_AP_BASE + 0x0040)
#define PMUR2			(SPRD_CODEC_AP_BASE + 0x0044)
#define PMUR3			(SPRD_CODEC_AP_BASE + 0x0048)
#define PMUR4			(SPRD_CODEC_AP_BASE + 0x004C)
#define PMUR5			(SPRD_CODEC_AP_BASE + 0x0050)
#define PMUR6			(SPRD_CODEC_AP_BASE + 0x0054)

#define HIBDR			(SPRD_CODEC_AP_BASE + 0x0058)

#define AACR1			(SPRD_CODEC_AP_BASE + 0x005C)
#define AACR2			(SPRD_CODEC_AP_BASE + 0x0060)
#define AAICR1			(SPRD_CODEC_AP_BASE + 0x0064)
#define AAICR2			(SPRD_CODEC_AP_BASE + 0x0068)
#define AAICR3			(SPRD_CODEC_AP_BASE + 0x006C)
#define ACGR			(SPRD_CODEC_AP_BASE + 0x0070)

#define DACR			(SPRD_CODEC_AP_BASE + 0x0074)
#define DAOCR1			(SPRD_CODEC_AP_BASE + 0x0078)
#define DAOCR2			(SPRD_CODEC_AP_BASE + 0x007C)
#define DAOCR3			(SPRD_CODEC_AP_BASE + 0x0080)
#define DAOCR4			(SPRD_CODEC_AP_BASE + 0x00BC)
#define DCR1			(SPRD_CODEC_AP_BASE + 0x0084)
#define DCR2			(SPRD_CODEC_AP_BASE + 0x0088)
#define DCR3			(SPRD_CODEC_AP_BASE + 0x008C)
#define DCR4			(SPRD_CODEC_AP_BASE + 0x0090)
#define DCGR1			(SPRD_CODEC_AP_BASE + 0x0094)
#define DCGR2			(SPRD_CODEC_AP_BASE + 0x0098)
#define DCGR3			(SPRD_CODEC_AP_BASE + 0x009C)

#define PNRCR1			(SPRD_CODEC_AP_BASE + 0x00A0)
#define PNRCR2			(SPRD_CODEC_AP_BASE + 0x00A4)
#define PNRCR3			(SPRD_CODEC_AP_BASE + 0x00A8)

#define CCR			(SPRD_CODEC_AP_BASE + 0x00AC)

#define IFR1			(SPRD_CODEC_AP_BASE + 0x00B0)
#define IFR2			(SPRD_CODEC_AP_BASE + 0x00B4)
#define IFR3			(SPRD_CODEC_AP_BASE + 0x00B8)

#define SPRD_CODEC_AP_END	(SPRD_CODEC_AP_BASE + 0x00C0)
#define IS_SPRD_CODEC_AP_RANG(reg) (((reg) >= SPRD_CODEC_AP_BASE) && ((reg) < SPRD_CODEC_AP_END))

#define ID_FUN(id, lr) ((int)(((id) << 1) | (lr)))

enum {
	SPRD_CODEC_PGA_SPKL = 0,
	SPRD_CODEC_PGA_SPKR,
	SPRD_CODEC_PGA_HPL,
	SPRD_CODEC_PGA_HPR,
	SPRD_CODEC_PGA_EAR,
	SPRD_CODEC_PGA_ADCL,
	SPRD_CODEC_PGA_ADCR,

	SPRD_CODEC_PGA_MAX
};

enum {
	SPRD_CODEC_LEFT = 0,
	SPRD_CODEC_RIGHT = 1,
};

enum {
	SPRD_CODEC_MIXER_START = 0,
	SPRD_CODEC_AIL = SPRD_CODEC_MIXER_START,
	SPRD_CODEC_AIR,
	SPRD_CODEC_MAIN_MIC,
	SPRD_CODEC_AUX_MIC,
	SPRD_CODEC_HP_MIC,
	SPRD_CODEC_ADC_MIXER_MAX,
	SPRD_CODEC_HP_DACL = SPRD_CODEC_ADC_MIXER_MAX,
	SPRD_CODEC_HP_DACR,
	SPRD_CODEC_HP_ADCL,
	SPRD_CODEC_HP_ADCR,
	SPRD_CODEC_HP_MIXER_MAX,
	SPRD_CODEC_SPK_DACL = SPRD_CODEC_HP_MIXER_MAX,
	SPRD_CODEC_SPK_DACR,
	SPRD_CODEC_SPK_ADCL,
	SPRD_CODEC_SPK_ADCR,
	SPRD_CODEC_SPK_MIXER_MAX,
	SPRD_CODEC_EAR_DACL = SPRD_CODEC_SPK_MIXER_MAX,
	SPRD_CODEC_EAR_MIXER_MAX,

	SPRD_CODEC_MIXER_MAX = SPRD_CODEC_EAR_MIXER_MAX << SPRD_CODEC_RIGHT
};

enum {
	SPRD_CODEC_MIC_BIAS,
	SPRD_CODEC_AUXMIC_BIAS,
	SPRD_CODEC_MIC_BIAS_MAX
};

int sprd_codec_init(void);
void sprd_codec_exit(void);
int sprd_codec_pcm_set_sample_rate(int playback, int rate);
int sprd_codec_digital_loop(int enable);

int mixer_get(int id);
int mixer_set(int id, int on);
int mixer_enable(int id, int enable);
int pga_enable(int id, int pgaval, int enable);
int hp_switch(int enable);
int ear_switch(int enable);
int spkl_switch(int enable);
int spkr_switch(int enable);
int dacl_digital_switch(int enable);
int dacr_digital_switch(int enable);

int adcl_digital_switch(int enable);
int adcr_digital_switch(int enable);
int adcl_switch(int enable);
int adcr_switch(int enable);
int mic_bias_enable(int id, int enable);

int sprd_inter_speaker_pa(int on);

#endif /* __SPRD_CODEC_H */
