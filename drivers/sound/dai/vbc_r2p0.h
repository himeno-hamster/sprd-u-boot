/*
 * sound/soc/sprd/dai/vbc/vbc_r2p0.h
 *
 * SPRD SoC CPU-DAI -- SpreadTrum SOC DAI with EQ&ALC and some loop.
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
#ifndef __VBC_R2P0H
#define __VBC_R2P0H

#include <asm/arch/sprd-audio.h>

#define VBC_FIFO_FRAME_NUM (160)

#define VBC_VERSION	"vbc.r2p0"

#define VBC_EQ_FIRMWARE_MAGIC_LEN	(4)
#define VBC_EQ_FIRMWARE_MAGIC_ID	("VBEQ")
#define VBC_EQ_PROFILE_VERSION		(0x00000002)	/*todo:  set number */
#define VBC_EQ_PROFILE_CNT_MAX		(50)
#define VBC_EQ_PROFILE_NAME_MAX		(32)
#define VBC_EFFECT_PARAS_LEN            (23+3+72*2)	/*todo:  set len */

/* VBADBUFFDTA */
enum {
	VBISADCK_INV = 9,
	VBISDACK_INV,
	VBLSB_EB,
	VBIIS_DLOOP = 13,
	VBPCM_MODE,
	VBIIS_LRCK,
};
/* VBDABUFFDTA */
enum {
	RAMSW_NUMB = 9,
	RAMSW_EN,
	VBAD0DMA_EN,
	VBAD1DMA_EN,
	VBDA0DMA_EN,
	VBDA1DMA_EN,
	VBENABLE,
};
/* VBDAPATH FM MIXER*/
enum {
	DAPATH_NO_MIX = 0,
	DAPATH_ADD_FM,
	DAPATH_SUB_FM,
};

/* VBADPATH ST INMUX*/
enum {
	ST_INPUT_NORMAL = 0,
	ST_INPUT_INVERSE,
	ST_INPUT_ZERO,
};

/* -------------------------- */

#define PHYS_VBDA0		(VBC_PHY_BASE + 0x0000)	/* 0x0000  Voice band DAC0 data buffer */
#define PHYS_VBDA1		(VBC_PHY_BASE + 0x0004)	/* 0x0004  Voice band DAC1 data buffer */
#define PHYS_VBAD0		(VBC_PHY_BASE + 0x0008)	/* 0x0008  Voice band ADC0 data buffer */
#define PHYS_VBAD1		(VBC_PHY_BASE + 0x000C)	/* 0x000C  Voice band ADC1 data buffer */
#define PHYS_VBAD2		(VBC_PHY_BASE + 0x00C0)	/* 0x00C0  Voice band ADC0 data buffer */
#define PHYS_VBAD3		(VBC_PHY_BASE + 0x00C4)	/* 0x00C4  Voice band ADC1 data buffer */

#define ARM_VB_BASE		VBC_BASE
#define VBDA0			(ARM_VB_BASE + 0x0000)	/*  Voice band DAC0 data buffer */
#define VBDA1			(ARM_VB_BASE + 0x0004)	/*  Voice band DAC1 data buffer */
#define VBAD0			(ARM_VB_BASE + 0x0008)	/*  Voice band ADC0 data buffer */
#define VBAD1			(ARM_VB_BASE + 0x000C)	/*  Voice band ADC1 data buffer */
#define VBBUFFSIZE		(ARM_VB_BASE + 0x0010)	/*  Voice band buffer size */
#define VBADBUFFDTA		(ARM_VB_BASE + 0x0014)	/*  Voice band AD buffer control */
#define VBDABUFFDTA		(ARM_VB_BASE + 0x0018)	/*  Voice band DA buffer control */
#define VBADCNT			(ARM_VB_BASE + 0x001C)	/*  Voice band AD buffer counter */
#define VBDACNT			(ARM_VB_BASE + 0x0020)	/*  Voice band DA buffer counter */
#define VBAD23CNT		(ARM_VB_BASE + 0x0024)	/*  Voice band AD23 buffer counter */
#define VBADDMA			(ARM_VB_BASE + 0x0028)	/*  Voice band AD23 DMA control */
#define VBBUFFAD23		(ARM_VB_BASE + 0x002C)	/*  Voice band AD23 buffer size */
#define VBINTTYPE		(ARM_VB_BASE + 0x0034)
#define VBDATASWT		(ARM_VB_BASE + 0x0038)
#define VBIISSEL			(ARM_VB_BASE + 0x003C)

#define DAPATCHCTL		(ARM_VB_BASE + 0x0040)
#define DADGCTL			(ARM_VB_BASE + 0x0044)
#define DAHPCTL			(ARM_VB_BASE + 0x0048)
#define DAALCCTL0		(ARM_VB_BASE + 0x004C)
#define DAALCCTL1		(ARM_VB_BASE + 0x0050)
#define DAALCCTL2		(ARM_VB_BASE + 0x0054)
#define DAALCCTL3		(ARM_VB_BASE + 0x0058)
#define DAALCCTL4		(ARM_VB_BASE + 0x005C)
#define DAALCCTL5		(ARM_VB_BASE + 0x0060)
#define DAALCCTL6		(ARM_VB_BASE + 0x0064)
#define DAALCCTL7		(ARM_VB_BASE + 0x0068)
#define DAALCCTL8		(ARM_VB_BASE + 0x006C)
#define DAALCCTL9		(ARM_VB_BASE + 0x0070)
#define DAALCCTL10		(ARM_VB_BASE + 0x0074)
#define STCTL0			(ARM_VB_BASE + 0x0078)
#define STCTL1			(ARM_VB_BASE + 0x007C)
#define ADPATCHCTL		(ARM_VB_BASE + 0x0080)
#define ADDG01CTL		(ARM_VB_BASE + 0x0084)
#define ADDG23CTL		(ARM_VB_BASE + 0x0088)
#define ADHPCTL			(ARM_VB_BASE + 0x008C)
#define ADCSRCCTL		(ARM_VB_BASE + 0x0090)
#define DACSRCCTL		(ARM_VB_BASE + 0x0094)
#define MIXERCTL			(ARM_VB_BASE + 0x0098)
#if 0
#define STFIFOLVL		(ARM_VB_BASE + 0x009C)
#define VBIRQEN			(ARM_VB_BASE + 0x00A0)
#define VBIRQCLR		(ARM_VB_BASE + 0x00A4)
#define VBIRQRAW		(ARM_VB_BASE + 0x00A8)
#define VBIRQSTS			(ARM_VB_BASE + 0x00AC)
#endif
#define VBNGCVTHD		(ARM_VB_BASE + 0x00B0)
#define VBNGCTTHD		(ARM_VB_BASE + 0x00B4)
#define VBNGCTL			(ARM_VB_BASE + 0x00B8)

/* DA HPF EQ6 start,  end */
#define HPCOEF0_H			(ARM_VB_BASE + 0x0100)
#define HPCOEF0_L			(ARM_VB_BASE + 0x0104)
#define HPCOEF42_H			(ARM_VB_BASE + 0x0250)
#define HPCOEF42_L			(ARM_VB_BASE + 0x0254)
/*DA HPF EQ4  start,  end*/
#define HPCOEF43_H			(ARM_VB_BASE + 0x0258)
#define HPCOEF43_L			(ARM_VB_BASE + 0x025C)
#define HPCOEF71_H			(ARM_VB_BASE + 0x0338)
#define HPCOEF71_L			(ARM_VB_BASE + 0x033C)

/* AD01 HPF EQ6 start,  end */
#define AD01_HPCOEF0_H			(ARM_VB_BASE + 0x0400)
#define AD01_HPCOEF0_L			(ARM_VB_BASE + 0x0404)
#define AD01_HPCOEF42_H			(ARM_VB_BASE + 0x0550)
#define AD01_HPCOEF42_L			(ARM_VB_BASE + 0x0554)
/* AD23 HPF EQ6 start,  end */
#define AD23_HPCOEF0_H			(ARM_VB_BASE + 0x0600)
#define AD23_HPCOEF0_L			(ARM_VB_BASE + 0x0604)
#define AD23_HPCOEF42_H			(ARM_VB_BASE + 0x0750)
#define AD23_HPCOEF42_L			(ARM_VB_BASE + 0x0754)

#define ARM_VB_END		(ARM_VB_BASE + 0x0754)

#define VBADBUFFERSIZE_SHIFT	(0)
#define VBADBUFFERSIZE_MASK	(0xFF<<VBADBUFFERSIZE_SHIFT)
#define VBDABUFFERSIZE_SHIFT	(8)
#define VBDABUFFERSIZE_MASK	(0xFF<<VBDABUFFERSIZE_SHIFT)
#define VBAD23BUFFERSIZE_SHIFT	(0)
#define VBAD23BUFFERSIZE_MASK	(0xFF<<VBAD23BUFFERSIZE_SHIFT)

#define VBCHNEN			(ARM_VB_BASE + 0x00C8)
#define VBDACHEN_SHIFT  	(0)
#define VBADCHEN_SHIFT  	(2)
#define VBAD23CHEN_SHIFT  	(4)

#define VBAD2DMA_EN 	(0)
#define VBAD3DMA_EN	(1)
 /*VBIISSEL*/
#define VBIISSEL_AD01_PORT_SHIFT	(0)
#define VBIISSEL_AD01_PORT_MASK	(0x7)
#define VBIISSEL_AD23_PORT_SHIFT	(3)
#define VBIISSEL_AD23_PORT_MASK	(0x7<<VBIISSEL_AD23_PORT_SHIFT)
     /*DAPATHCTL*/
#define VBDAPATH_DA0_ADDFM_SHIFT	(0)
#define VBDAPATH_DA0_ADDFM_MASK	(0x3<<VBDAPATH_DA0_ADDFM_SHIFT)
#define VBDAPATH_DA1_ADDFM_SHIFT	(2)
#define VBDAPATH_DA1_ADDFM_MASK	(0x3<<VBDAPATH_DA1_ADDFM_SHIFT)
     /*ADPATHCTL*/
#define VBADPATH_ST0_INMUX_SHIFT	(12)
#define VBADPATH_ST0_INMUX_MASK	(0x3<<VBADPATH_ST0_INMUX_SHIFT)
#define VBADPATH_ST1_INMUX_SHIFT	(14)
#define VBADPATH_ST1_INMUX_MASK	(0x3<<VBADPATH_ST1_INMUX_SHIFT)
     /*MIXERCTL*/
#define VBMIXER_DAC0_MUXIN_SEL_SHIFT	(0)
#define VBMIXER_DAC0_MUXIN_SEL_MASK	(0x7<<VBMIXER_DAC0_MUXIN_SEL_SHIFT)
#define VBMIXER_DAC1_MUXIN_SEL_SHIFT	(3)
#define VBMIXER_DAC1_MUXIN_SEL_MASK	(0x7<<VBMIXER_DAC1_MUXIN_SEL_SHIFT)
#define VBMIXER_DAC0_OUT_SEL		(6)
#define VBMIXER_DAC1_OUT_SEL		(7)
     /*DACSRCCTL*/
#define 	VBDACSRC_EN		(0)
#define 	VBDACSRC_CLR		(1)
#define 	VBDACSRC_F1F2F3_BP	(3)
#define 	VBDACSRC_F1_SEL		(4)
#define	VBDACSRC_F0_BP		(5)
#define 	VBDACSRC_F0_SEL		(6)





enum {
	VBC_LEFT = 0,
	VBC_RIGHT = 1,
};

int vbc_init(void);
void vbc_exit(void);
int vbc_startup(int stream);
void vbc_shutdown(int stream);
int vbc_trigger(int stream, int enable);
int vbc_adc_sel_iis(int port);
int vbc_adc23_sel_iis(int port);
int vbc_dac0_fm_mixer(int mode);
int vbc_dac1_fm_mixer(int mode);

#endif /* __VBC_H */
