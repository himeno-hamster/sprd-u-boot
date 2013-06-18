/*
 * sound/soc/sprd/codec/sprd/sprd-codec-v3.c
 *
 * SPRD-CODEC -- SpreadTrum Tiger intergrated codec.
 *
 * Copyright (C) 2013 SpreadTrum Ltd.
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
#define pr_fmt(fmt) "[audio:codec] " fmt

#include <common.h>
#include <errno.h>
#include <asm/atomic.h>
#include <ubi_uboot.h>

#include <asm/arch/ldo.h>
#include <asm/arch/sprd-audio.h>
#include "sprd-codec-v3.h"

#ifdef CONFIG_SPRD_AUDIO_DEBUG
#define sprd_codec_dbg pr_info
#define sprd_bug_on BUG_ON
#else
#define sprd_codec_dbg(...)
#define sprd_bug_on(...)
#endif

#define SOC_REG(reg) reg

#define DEFINE_SPINLOCK(...)
#define DEFINE_MUTEX(...)

struct snd_soc_codec {
	/* codec IO */
	unsigned int (*read) (struct snd_soc_codec *, unsigned int);
	int (*write) (struct snd_soc_codec *, unsigned int, unsigned int);
	struct sprd_codec_priv *sprd_codec;
};

static inline void snd_soc_codec_set_drvdata(struct snd_soc_codec *codec,
					     void *data)
{
	codec->sprd_codec = data;
}

static inline void *snd_soc_codec_get_drvdata(struct snd_soc_codec *codec)
{
	return (void *)codec->sprd_codec;
}

const char *sprd_codec_pga_debug_str[SPRD_CODEC_PGA_MAX] = {
	"SPKL",
	"SPKR",
	"HPL",
	"HPR",
	"EAR",
	"ADCL",
	"ADCR",
	"DACL",
	"DACR",
	"MIC",
	"AUXMIC",
	"HEADMIC",
	"AIL",
	"AIR",
};

typedef int (*sprd_codec_pga_set) (struct snd_soc_codec * codec, int pgaval);

struct sprd_codec_pga {
	sprd_codec_pga_set set;
	int min;
};

struct sprd_codec_pga_op {
	int pgaval;
	sprd_codec_pga_set set;
};

const char *sprd_codec_mixer_debug_str[SPRD_CODEC_MIXER_MAX] = {
	"AIL->ADCL",
	"AIL->ADCR",
	"AIR->ADCL",
	"AIR->ADCR",
	"MAIN MIC->ADCL",
	"MAIN MIC->ADCR",
	"AUX MIC->ADCL",
	"AUX MIC->ADCR",
	"HP MIC->ADCL",
	"HP MIC->ADCR",
	"DACL->HPL",
	"DACL->HPR",
	"DACR->HPL",
	"DACR->HPR",
	"ADCL->HPL",
	"ADCL->HPR",
	"ADCR->HPL",
	"ADCR->HPR",
	"DACL->SPKL",
	"DACL->SPKR",
	"DACR->SPKL",
	"DACR->SPKR",
	"ADCL->SPKL",
	"ADCL->SPKR",
	"ADCR->SPKL",
	"ADCR->SPKR",
	"DACL->EAR",
	"DACR->EAR(bug)"
};

#define IS_SPRD_CODEC_MIXER_RANG(reg) (((reg) >= SPRD_CODEC_MIXER_START) && ((reg) <= SPRD_CODEC_MIXER_MAX))

typedef int (*sprd_codec_mixer_set) (struct snd_soc_codec * codec, int on);
struct sprd_codec_mixer {
	int on;
	sprd_codec_mixer_set set;
};

struct sprd_codec_inter_pa {
	/* FIXME little endian */
	int LDO_V_sel:4;
	int DTRI_F_sel:4;
	int is_DEMI_mode:1;
	int is_classD_mode:1;
	int is_LDO_mode:1;
	int is_auto_LDO_mode:1;
	int RESV:20;
};

struct sprd_codec_pa_setting {
	union {
		struct sprd_codec_inter_pa setting;
		u32 value;
	};
	int set;
};

DEFINE_MUTEX(inter_pa_mutex);
static struct sprd_codec_pa_setting inter_pa;

struct sprd_codec_inter_hp_pa {
	/* FIXME little endian */
	int class_g_osc:2;
	int class_g_mode:2;
	int class_g_low_power:2;
	int RESV:26;
};

struct sprd_codec_hp_pa_setting {
	union {
		struct sprd_codec_inter_hp_pa setting;
		u32 value;
	};
	int set;
};

DEFINE_MUTEX(inter_hp_pa_mutex);
static struct sprd_codec_hp_pa_setting inter_hp_pa;

static const char *mic_bias_name[SPRD_CODEC_MIC_BIAS_MAX] = {
	"Mic Bias",
	"AuxMic Bias",
	"HeadMic Bias",
};

/* codec private data */
struct sprd_codec_priv {
	struct snd_soc_codec *codec;
	atomic_t power_refcount;
	atomic_t adie_dac_refcount;
	atomic_t adie_adc_refcount;
	atomic_t adc_power_refcount;
	atomic_t adc_digital_power_refcount;
	atomic_t dac_power_refcount;
	atomic_t dac_digital_power_refcount;
	atomic_t digital_power_refcount;
	atomic_t analog_power_refcount;
	int da_sample_val;
	int ad_sample_val;
	struct sprd_codec_mixer mixer[SPRD_CODEC_MIXER_MAX];
	struct sprd_codec_pga_op pga[SPRD_CODEC_PGA_MAX];
	int mic_bias[SPRD_CODEC_MIC_BIAS_MAX];
#ifdef CONFIG_SPRD_CODEC_USE_INT
	int ap_irq;
	struct completion completion_hp_pop;

	int dp_irq;
	struct completion completion_dac_mute;
#endif
};

/* codec local power suppliy */
static struct sprd_codec_power_suppliy {
	atomic_t mic_on;
	atomic_t auxmic_on;
	atomic_t headmic_on;
	atomic_t ldo_refcount;
	int audio_ldo_open_ok;
} sprd_codec_power;

static inline char *_2str(int enable)
{
	return enable ? "enable" : "disable";
}

#define SPRD_CODEC_PA_SW_AOL (BIT(0))
#define SPRD_CODEC_PA_SW_EAR (BIT(1))
#define SPRD_CODEC_PA_SW_FUN (SPRD_CODEC_PA_SW_AOL | SPRD_CODEC_PA_SW_EAR)
static int sprd_codec_fun = 0;
DEFINE_SPINLOCK(sprd_codec_fun_lock);

static void sprd_codec_set_fun(int fun)
{
	spin_lock(&sprd_codec_fun_lock);
	sprd_codec_fun |= fun;
	spin_unlock(&sprd_codec_fun_lock);
}

static void sprd_codec_clr_fun(int fun)
{
	spin_lock(&sprd_codec_fun_lock);
	sprd_codec_fun &= ~fun;
	spin_unlock(&sprd_codec_fun_lock);
}

static int sprd_codec_test_fun(int fun)
{
	int ret;
	spin_lock(&sprd_codec_fun_lock);
	ret = sprd_codec_fun & fun;
	spin_unlock(&sprd_codec_fun_lock);
	return ret;
}

static unsigned int sprd_codec_read(struct snd_soc_codec *codec,
				    unsigned int reg)
{
	if (IS_SPRD_CODEC_AP_RANG(reg)) {
		return arch_audio_codec_read(reg);
	} else if (IS_SPRD_CODEC_DP_RANG(reg)) {
		return __raw_readl(reg);
	}
	sprd_codec_dbg("read the register is not codec's reg = 0x%x\n", reg);
	return 0;
}

static int sprd_codec_write(struct snd_soc_codec *codec, unsigned int reg,
			    unsigned int val)
{
	sprd_codec_dbg("reg = 0x%08x val = 0x%08x\n", reg, val);
	if (IS_SPRD_CODEC_AP_RANG(reg)) {
		return arch_audio_codec_write(reg, val);
	} else if (IS_SPRD_CODEC_DP_RANG(reg)) {
		return __raw_writel(val, reg);
	}
	sprd_codec_dbg("write the register is not codec's reg = 0x%x\n", reg);
	return 0;
}

static inline unsigned int snd_soc_read(struct snd_soc_codec *codec,
					unsigned int reg)
{
	return sprd_codec_read(codec, reg);
}

static inline unsigned int snd_soc_write(struct snd_soc_codec *codec,
					 unsigned int reg, unsigned int val)
{
	return sprd_codec_write(codec, reg, val);
}

static struct sprd_codec_priv s_sprd_codec_priv = { 0 };

static struct snd_soc_codec s_sprd_codec = {
	.read = sprd_codec_read,
	.write = sprd_codec_write,
	.sprd_codec = &s_sprd_codec_priv,
};

/**
 * snd_soc_update_bits - update codec register bits
 * @codec: audio codec
 * @reg: codec register
 * @mask: register mask
 * @value: new value
 *
 * Writes new register value.
 *
 * Returns 1 for change, 0 for no change, or negative error code.
 */
static int snd_soc_update_bits(struct snd_soc_codec *codec, unsigned int reg,
			       unsigned int mask, unsigned int value)
{
	int change;
	unsigned int old, new;
	int ret;

	ret = snd_soc_read(codec, reg);
	if (ret < 0)
		return ret;

	old = ret;
	new = (old & ~mask) | value;
	change = old != new;
	if (change) {
		ret = snd_soc_write(codec, reg, new);
		if (ret < 0)
			return ret;
	}

	return change;
}

static void sprd_codec_wait(u32 wait_time)
{
	udelay(wait_time * 1000);
}

static unsigned int sprd_codec_read(struct snd_soc_codec *codec,
				    unsigned int reg);
static void sprd_codec_print_regs(struct snd_soc_codec *codec)
{
	int reg;
	pr_warn("sprd_codec register digital part\n");
	for (reg = SPRD_CODEC_DP_BASE; reg < SPRD_CODEC_DP_END; reg += 0x10) {
		pr_warn("0x%04x | 0x%04x 0x%04x 0x%04x 0x%04x\n",
			(reg - SPRD_CODEC_DP_BASE)
			, sprd_codec_read(codec, reg + 0x00)
			, sprd_codec_read(codec, reg + 0x04)
			, sprd_codec_read(codec, reg + 0x08)
			, sprd_codec_read(codec, reg + 0x0C)
		    );
	}
	pr_warn("sprd_codec register analog part\n");
	for (reg = SPRD_CODEC_AP_BASE; reg < SPRD_CODEC_AP_END; reg += 0x10) {
		pr_warn("0x%04x | 0x%04x 0x%04x 0x%04x 0x%04x\n",
			(reg - SPRD_CODEC_AP_BASE)
			, sprd_codec_read(codec, reg + 0x00)
			, sprd_codec_read(codec, reg + 0x04)
			, sprd_codec_read(codec, reg + 0x08)
			, sprd_codec_read(codec, reg + 0x0C)
		    );
	}
}

static inline void sprd_codec_vcm_v_sel(int v_sel)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, v_sel);
	mask = VCM_V_MASK << VCM_V;
	val = (v_sel << VCM_V) & mask;
	arch_audio_codec_write_mask(PMUR4_PMUR3, val, mask);
}

static int sprd_codec_pga_spk_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DCGR3;
	val = (pgaval & 0xF) << 4;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0xF0, val);
}

static int sprd_codec_pga_spkr_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DCGR3;
	val = pgaval & 0xF;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x0F, val);
}

static int sprd_codec_pga_hpl_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DCGR2_DCGR1;
	val = (pgaval & 0xF) << 4;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0xF0, val);
}

static int sprd_codec_pga_hpr_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DCGR2_DCGR1;
	val = pgaval & 0xF;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x0F, val);
}

static int sprd_codec_pga_ear_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DCGR2_DCGR1;
	val = ((pgaval & 0xF) << 12);
	return snd_soc_update_bits(codec, SOC_REG(reg), 0xF000, val);
}

static int sprd_codec_pga_adcl_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = ACGR3_ACGR2;
	val = pgaval & 0x3F;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x3F, val);
}

static int sprd_codec_pga_adcr_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = ACGR3_ACGR2;
	val = (pgaval & 0x3F) << 8;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x3F00, val);
}

static int sprd_codec_pga_dacl_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DACGR_DACR;
	val = (pgaval & 0x07) << 12;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x7000, val);
}

static int sprd_codec_pga_dacr_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = DACGR_DACR;
	val = (pgaval & 0x07) << 8;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x0700, val);
}

static int sprd_codec_pga_mic_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = ACGR1;
	val = (pgaval & 0x03) << 6;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0xC0, val);
}

static int sprd_codec_pga_auxmic_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = ACGR1;
	val = (pgaval & 0x03) << 4;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x30, val);
}

static int sprd_codec_pga_headmic_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = ACGR1;
	val = (pgaval & 0x03) << 2;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x0C, val);
}

static int sprd_codec_pga_ailr_set(struct snd_soc_codec *codec, int pgaval)
{
	int reg, val;
	reg = ACGR1;
	val = pgaval & 0x03;
	return snd_soc_update_bits(codec, SOC_REG(reg), 0x03, val);
}

static struct sprd_codec_pga sprd_codec_pga_cfg[SPRD_CODEC_PGA_MAX] = {
	{sprd_codec_pga_spk_set, 0},
	{sprd_codec_pga_spkr_set, 0},
	{sprd_codec_pga_hpl_set, 0},
	{sprd_codec_pga_hpr_set, 0},
	{sprd_codec_pga_ear_set, 0},

	{sprd_codec_pga_adcl_set, 0},
	{sprd_codec_pga_adcr_set, 0},

	{sprd_codec_pga_dacl_set, 0},
	{sprd_codec_pga_dacr_set, 0},
	{sprd_codec_pga_mic_set, 0},
	{sprd_codec_pga_auxmic_set, 0},
	{sprd_codec_pga_headmic_set, 0},
	{sprd_codec_pga_ailr_set, 0},
	{sprd_codec_pga_ailr_set, 0},
};

/* adc mixer */

static int sprd_codec_is_ai_enable(struct snd_soc_codec *codec)
{
	return ! !(snd_soc_read(codec, AAICR2_AAICR1) &
		   (BIT(AIL_ADCR) | BIT(AIL_ADCL) | BIT(AIR_ADCR) |
		    BIT(AIR_ADCL)));
}

static int ailadcl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1), BIT(AIL_ADCL),
				   on << AIL_ADCL);
}

static int ailadcr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1), BIT(AIL_ADCR),
				   on << AIL_ADCR);
}

static int airadcl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1), BIT(AIR_ADCL),
				   on << AIR_ADCL);
}

static int airadcr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1), BIT(AIR_ADCR),
				   on << AIR_ADCR);
}

static int mainmicadcl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1), BIT(MIC_ADCL),
				   on << MIC_ADCL);
}

static int mainmicadcr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1), BIT(MIC_ADCR),
				   on << MIC_ADCR);
}

static int auxmicadcl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1),
				   BIT(AUXMIC_ADCL), on << AUXMIC_ADCL);
}

static int auxmicadcr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1),
				   BIT(AUXMIC_ADCR), on << AUXMIC_ADCR);
}

static int hpmicadcl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1),
				   BIT(HEADMIC_ADCL), on << HEADMIC_ADCL);
}

static int hpmicadcr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(AAICR2_AAICR1),
				   BIT(HEADMIC_ADCR), on << HEADMIC_ADCR);
}

/* hp mixer */

static int daclhpl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(DACL_P_HPL), on << DACL_P_HPL);
}

static int daclhpr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(DACL_N_HPR), on << DACL_N_HPR);
}

static int dacrhpl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(DACR_P_HPL), on << DACR_P_HPL);
}

static int dacrhpr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(DACR_P_HPR), on << DACR_P_HPR);
}

static int adclhpl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(ADCL_P_HPL), on << ADCL_P_HPL);
}

static int adclhpr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(ADCL_N_HPR), on << ADCL_N_HPR);
}

static int adcrhpl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(ADCR_P_HPL), on << ADCR_P_HPL);
}

static int adcrhpr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1),
				   BIT(ADCR_P_HPR), on << ADCR_P_HPR);
}

/* spkl mixer */

static int daclspkl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(DACL_AOL),
				   on << DACL_AOL);
}

static int dacrspkl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(DACR_AOL),
				   on << DACR_AOL);
}

static int adclspkl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(ADCL_AOL),
				   on << ADCL_AOL);
}

static int adcrspkl_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(ADCR_AOL),
				   on << ADCR_AOL);
}

/* spkr mixer */

static int daclspkr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(DACL_AOR),
				   on << DACL_AOR);
}

static int dacrspkr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(DACR_AOR),
				   on << DACR_AOR);
}

static int adclspkr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(ADCL_AOR),
				   on << ADCL_AOR);
}

static int adcrspkr_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR2), BIT(ADCR_AOR),
				   on << ADCR_AOR);
}

/* ear mixer */

static int daclear_set(struct snd_soc_codec *codec, int on)
{
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	return snd_soc_update_bits(codec, SOC_REG(DAOCR3_DAOCR1), BIT(DACL_EAR),
				   on << DACL_EAR);
}

static sprd_codec_mixer_set mixer_setting[SPRD_CODEC_MIXER_MAX] = {
	/* adc mixer */
	ailadcl_set, ailadcr_set,
	airadcl_set, airadcr_set,
	mainmicadcl_set, mainmicadcr_set,
	auxmicadcl_set, auxmicadcr_set,
	hpmicadcl_set, hpmicadcr_set,
	/* hp mixer */
	daclhpl_set, daclhpr_set,
	dacrhpl_set, dacrhpr_set,
	adclhpl_set, adclhpr_set,
	adcrhpl_set, adcrhpr_set,
	/* spk mixer */
	daclspkl_set, daclspkr_set,
	dacrspkl_set, dacrspkr_set,
	adclspkl_set, adclspkr_set,
	adcrspkl_set, adcrspkr_set,
	/* ear mixer */
	daclear_set, 0,
};

int sprd_codec_digital_loop(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	int mask = BIT(AUDIFA_ADIE_LOOP_EN);
	if (enable) {
		snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB), mask, mask);
		arch_audio_codec_loop_enable();
	} else {
		snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB), mask, 0);
		arch_audio_codec_loop_disable();
	}
	return 0;
}

/* DO NOT USE THIS FUNCTION */
static inline void __sprd_codec_pa_sw_en(int on)
{
	int mask;
	int val;
	mask = BIT(PA_SW_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(PMUR2_PMUR1, val, mask);
}

DEFINE_SPINLOCK(sprd_codec_pa_sw_lock);
static inline void sprd_codec_pa_sw_set(int fun)
{
	sprd_codec_dbg("Entering %s fun 0x%08x\n", __func__, fun);
	spin_lock(&sprd_codec_pa_sw_lock);
	sprd_codec_set_fun(fun);
	__sprd_codec_pa_sw_en(1);
	spin_unlock(&sprd_codec_pa_sw_lock);
}

static inline void sprd_codec_pa_sw_clr(int fun)
{
	sprd_codec_dbg("Entering %s fun 0x%08x\n", __func__, fun);
	spin_lock(&sprd_codec_pa_sw_lock);
	sprd_codec_clr_fun(fun);
	if (!sprd_codec_test_fun(SPRD_CODEC_PA_SW_FUN))
		__sprd_codec_pa_sw_en(0);
	spin_unlock(&sprd_codec_pa_sw_lock);
}

/* inter PA */

static inline void sprd_codec_pa_d_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(PA_D_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR2_DCR1, val, mask);
}

static inline void sprd_codec_pa_demi_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(PA_DEMI_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR2_DCR1, val, mask);

	mask = BIT(DRV_OCP_AOL_PD) | BIT(DRV_OCP_AOR_PD);
	val = mask;
	arch_audio_codec_write_mask(DCR4_DCR3, val, mask);
}

static inline void sprd_codec_pa_ldo_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(PA_LDO_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(PMUR2_PMUR1, val, mask);
	if (on) {
		sprd_codec_pa_sw_clr(SPRD_CODEC_PA_SW_AOL);
	}
}

static inline void sprd_codec_pa_ldo_v_sel(int v_sel)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, v_sel);
	mask = PA_LDO_V_MASK << PA_LDO_V;
	val = (v_sel << PA_LDO_V) & mask;
	arch_audio_codec_write_mask(PMUR6_PMUR5, val, mask);
}

static inline void sprd_codec_pa_dtri_f_sel(int f_sel)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, f_sel);
	mask = PA_DTRI_F_MASK << PA_DTRI_F;
	val = (f_sel << PA_DTRI_F) & mask;
	arch_audio_codec_write_mask(DCR2_DCR1, val, mask);
}

static inline void sprd_codec_pa_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	spin_lock(&sprd_codec_pa_sw_lock);
	if (on) {
		mask = BIT(PA_EN);
		val = mask;
	} else {
		if (!sprd_codec_test_fun(SPRD_CODEC_PA_SW_FUN))
			mask = BIT(PA_EN) | BIT(PA_SW_EN) | BIT(PA_LDO_EN);
		else
			mask = BIT(PA_EN) | BIT(PA_LDO_EN);
		val = 0;
	}
	arch_audio_codec_write_mask(PMUR2_PMUR1, val, mask);
	spin_unlock(&sprd_codec_pa_sw_lock);
}

static inline void sprd_codec_inter_pa_init(void)
{
	inter_pa.setting.LDO_V_sel = 0x03;
	inter_pa.setting.DTRI_F_sel = 0x01;
}

int sprd_inter_speaker_pa(int on)
{
	pr_info("inter PA switch %s\n", on ? "ON" : "OFF");
	mutex_lock(&inter_pa_mutex);
	if (on) {
		sprd_codec_pa_d_en(inter_pa.setting.is_classD_mode);
		sprd_codec_pa_demi_en(inter_pa.setting.is_DEMI_mode);
		sprd_codec_pa_ldo_en(inter_pa.setting.is_LDO_mode);
		if (inter_pa.setting.is_LDO_mode) {
			if (inter_pa.setting.is_auto_LDO_mode) {
				/* sprd_codec_pa_ldo_v_sel(1); */
			} else {
				sprd_codec_pa_ldo_v_sel(inter_pa.setting.
							LDO_V_sel);
			}
		}
		sprd_codec_pa_dtri_f_sel(inter_pa.setting.DTRI_F_sel);
		sprd_codec_pa_en(1);
		inter_pa.set = 1;
	} else {
		inter_pa.set = 0;
		sprd_codec_pa_en(0);
		sprd_codec_pa_ldo_en(0);
	}
	mutex_unlock(&inter_pa_mutex);
	return 0;
}

EXPORT_SYMBOL(sprd_inter_speaker_pa);

static inline void sprd_codec_hp_pa_lpw(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_LPW);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_mode(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_MODE);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_osc(int osc)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, osc);
	mask = AUDIO_CHP_OSC_MASK << AUDIO_CHP_OSC;
	val = (osc << AUDIO_CHP_OSC) & mask;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_ref_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_REF_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_hpl_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_HPL_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_hpr_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_HPR_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_hpl_mute(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_LMUTE);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_hp_pa_hpr_mute(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUDIO_CHP_RMUTE);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(DCR8_DCR7, val, mask);
}

static inline void sprd_codec_inter_hp_pa_init(void)
{
	inter_hp_pa.setting.class_g_osc = 0x01;
}

int sprd_inter_headphone_pa(int on)
{
	pr_info("inter HP PA switch %s\n", on ? "ON" : "OFF");
	mutex_lock(&inter_hp_pa_mutex);
	if (on) {
		LDO_TurnOnLDO(CLASS_G_LDO_ID);
		sprd_codec_hp_pa_lpw(inter_hp_pa.setting.class_g_low_power);
		sprd_codec_hp_pa_mode(inter_hp_pa.setting.class_g_mode);
		sprd_codec_hp_pa_osc(inter_hp_pa.setting.class_g_osc);
		sprd_codec_hp_pa_hpl_en(1);
		sprd_codec_hp_pa_hpr_en(1);
		sprd_codec_hp_pa_ref_en(1);
		sprd_codec_hp_pa_en(1);
		inter_hp_pa.set = 1;
	} else {
		inter_hp_pa.set = 0;
		sprd_codec_hp_pa_en(0);
		sprd_codec_hp_pa_ref_en(0);
		sprd_codec_hp_pa_hpl_en(0);
		sprd_codec_hp_pa_hpr_en(0);
		LDO_TurnOffLDO(CLASS_G_LDO_ID);
	}
	mutex_unlock(&inter_hp_pa_mutex);
	return 0;
}

EXPORT_SYMBOL(sprd_inter_headphone_pa);

/* mic bias external */

static inline void sprd_codec_mic_bias_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(MICBIAS_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(PMUR4_PMUR3, val, mask);
}

static inline void sprd_codec_auxmic_bias_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(AUXMICBIAS_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(PMUR4_PMUR3, val, mask);
}

static inline void sprd_codec_headmic_bias_en(int on)
{
	int mask;
	int val;
	sprd_codec_dbg("Entering %s set %d\n", __func__, on);
	mask = BIT(HEADMICBIAS_EN);
	val = on ? mask : 0;
	arch_audio_codec_write_mask(PMUR2_PMUR1, val, mask);
}

static int sprd_codec_set_sample_rate(struct snd_soc_codec *codec, int rate,
				      int mask, int shift)
{
	switch (rate) {
	case 8000:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_8000 << shift);
		break;
	case 11025:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_11025 << shift);
		break;
	case 16000:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_16000 << shift);
		break;
	case 22050:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_22050 << shift);
		break;
	case 32000:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_32000 << shift);
		break;
	case 44100:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_44100 << shift);
		break;
	case 48000:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_48000 << shift);
		break;
	case 96000:
		snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL), mask,
				    SPRD_CODEC_RATE_96000 << shift);
		break;
	default:
		pr_err("sprd_codec not supports rate %d\n", rate);
		break;
	}
	return 0;
}

static int sprd_codec_set_ad_sample_rate(struct snd_soc_codec *codec, int rate,
					 int mask, int shift)
{
	int set;
	set = rate / 4000;
	if (set > 13) {
		pr_err("sprd_codec not supports ad rate %d\n", rate);
	}
	snd_soc_update_bits(codec, SOC_REG(AUD_ADC_CTL), mask, set << shift);
	return 0;
}

static int sprd_codec_sample_rate_setting(struct sprd_codec_priv *sprd_codec)
{
	if (sprd_codec->ad_sample_val) {
		sprd_codec_set_ad_sample_rate(sprd_codec->codec,
					      sprd_codec->ad_sample_val, 0x0F,
					      0);
#ifdef CONFIG_SPRD_CODEC_DMIC
		/*set adc1(dmic) sample rate */
		sprd_codec_set_ad_sample_rate(sprd_codec->codec,
					      sprd_codec->ad_sample_val, 0xF0,
					      4);
#endif
	}
	if (sprd_codec->da_sample_val) {
		sprd_codec_set_sample_rate(sprd_codec->codec,
					   sprd_codec->da_sample_val, 0x0F, 0);
	}
	return 0;
}

static int sprd_codec_update_bits(struct snd_soc_codec *codec,
				  unsigned int reg, unsigned int mask,
				  unsigned int value)
{
	if (!codec) {
		return arch_audio_codec_write_mask(reg, value, mask);
	} else {
		return snd_soc_update_bits(codec, reg, mask, value);
	}
}

static int sprd_codec_ldo_on(struct sprd_codec_priv *sprd_codec)
{
	struct snd_soc_codec *codec = 0;
	sprd_codec_dbg("Entering %s\n", __func__);

	atomic_inc(&sprd_codec_power.ldo_refcount);
	if (atomic_read(&sprd_codec_power.ldo_refcount) == 1) {
		sprd_codec_dbg("ldo on!\n");
		if (sprd_codec) {
			codec = sprd_codec->codec;
		}
		arch_audio_codec_switch(AUDIO_TO_AP_ARM_CTRL);
		arch_audio_codec_analog_reg_enable();
		arch_audio_codec_enable();
		arch_audio_codec_reset();
		/* sprd_codec_vcm_v_sel(0); */

		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3),
				       BIT(BG_IBIAS_EN), BIT(BG_IBIAS_EN));
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3), BIT(BG_EN),
				       BIT(BG_EN));
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3), BIT(VCM_EN),
				       BIT(VCM_EN));
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3),
				       BIT(VCM_BUF_EN), BIT(VCM_BUF_EN));
		sprd_codec_update_bits(codec, SOC_REG(PMUR2_PMUR1), BIT(VB_EN),
				       BIT(VB_EN));
		sprd_codec_update_bits(codec, SOC_REG(PMUR2_PMUR1), BIT(VBO_EN),
				       BIT(VBO_EN));

		if (sprd_codec) {
			sprd_codec_wait(SPRD_CODEC_LDO_WAIT_TIME);
		}
	}

	sprd_codec_dbg("Leaving %s\n", __func__);
	return 0;
}

static int sprd_codec_ldo_off(struct sprd_codec_priv *sprd_codec)
{
	struct snd_soc_codec *codec = 0;
	sprd_codec_dbg("Entering %s\n", __func__);

	if (atomic_dec_and_test(&sprd_codec_power.ldo_refcount)) {
		if (sprd_codec) {
			codec = sprd_codec->codec;
		}
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3), BIT(VCM_EN),
				       0);
		sprd_codec_wait(SPRD_CODEC_LDO_VCM_TIME);
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3),
				       BIT(VCM_BUF_EN), 0);
		sprd_codec_update_bits(codec, SOC_REG(PMUR2_PMUR1), BIT(VB_EN),
				       0);
		sprd_codec_update_bits(codec, SOC_REG(PMUR2_PMUR1), BIT(VBO_EN),
				       0);
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3),
				       BIT(BG_IBIAS_EN), 0);
		sprd_codec_update_bits(codec, SOC_REG(PMUR4_PMUR3), BIT(BG_EN),
				       0);

		arch_audio_codec_reset();
		arch_audio_codec_disable();
		arch_audio_codec_analog_reg_disable();
		sprd_codec_dbg("ldo off!\n");
	}

	sprd_codec_dbg("Leaving %s\n", __func__);
	return 0;
}

static int sprd_codec_ldo_control(int on)
{
	if (on) {
		return sprd_codec_ldo_on(0);
	} else {
		return sprd_codec_ldo_off(0);
	}
}

static void sprd_codec_mic_delay_worker(void)
{
	int on = atomic_read(&sprd_codec_power.mic_on) > 0;
	sprd_codec_ldo_control(on);
	sprd_codec_mic_bias_en(on);
}

static void sprd_codec_auxmic_delay_worker(void)
{
	int on = atomic_read(&sprd_codec_power.auxmic_on) > 0;
	sprd_codec_ldo_control(on);
	sprd_codec_auxmic_bias_en(on);
}

static void sprd_codec_headmic_delay_worker(void)
{
	int on = atomic_read(&sprd_codec_power.headmic_on) > 0;
	sprd_codec_ldo_control(on);
	sprd_codec_headmic_bias_en(on);
}

static int sprd_codec_mic_bias_inter(int on, atomic_t * v)
{
	if (on) {
		atomic_inc(v);
		return 1;
	} else {
		if (atomic_read(v) > 0) {
			atomic_dec(v);
			return 1;
		}
	}
	return 0;
}

int sprd_codec_mic_bias_control(int on)
{
	if (sprd_codec_mic_bias_inter(on, &sprd_codec_power.mic_on)) {
		sprd_codec_mic_delay_worker();
	}
	return 0;
}

EXPORT_SYMBOL(sprd_codec_mic_bias_control);

int sprd_codec_auxmic_bias_control(int on)
{
	if (sprd_codec_mic_bias_inter(on, &sprd_codec_power.auxmic_on)) {
		sprd_codec_auxmic_delay_worker();
	}
	return 0;
}

EXPORT_SYMBOL(sprd_codec_auxmic_bias_control);

int sprd_codec_headmic_bias_control(int on)
{
	if (sprd_codec_mic_bias_inter(on, &sprd_codec_power.headmic_on)) {
		sprd_codec_headmic_delay_worker();
	}
	return 0;
}

EXPORT_SYMBOL(sprd_codec_headmic_bias_control);

static int sprd_codec_open(struct snd_soc_codec *codec)
{
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	sprd_codec_dbg("Entering %s\n", __func__);

	sprd_codec_sample_rate_setting(sprd_codec);

	sprd_codec_dbg("Leaving %s\n", __func__);
	return ret;
}

static void sprd_codec_power_enable(struct snd_soc_codec *codec)
{
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int ret;

	atomic_inc(&sprd_codec->power_refcount);
	if (atomic_read(&sprd_codec->power_refcount) == 1) {
		sprd_codec_dbg("Entering %s\n", __func__);
		ret = sprd_codec_ldo_on(sprd_codec);
		if (ret != 0)
			pr_err("sprd_codec open ldo error %d\n", ret);
		sprd_codec_open(codec);
		sprd_codec_dbg("Leaving %s\n", __func__);
	}
}

static void sprd_codec_power_disable(struct snd_soc_codec *codec)
{
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);

	if (atomic_dec_and_test(&sprd_codec->power_refcount)) {
		sprd_codec_dbg("Entering %s\n", __func__);
		sprd_codec_ldo_off(sprd_codec);
		sprd_codec_dbg("Leaving %s\n", __func__);
	}
}

static int analog_power_enable(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (enable) {
		atomic_inc(&sprd_codec->analog_power_refcount);
		if (atomic_read(&sprd_codec->analog_power_refcount) == 1) {
			sprd_codec_power_enable(codec);
		}
	} else {
		if (atomic_dec_and_test(&sprd_codec->analog_power_refcount)) {
			sprd_codec_power_disable(codec);
		}
	}

	return ret;
}

static int digital_power_enable(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);

	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (enable) {
		atomic_inc(&sprd_codec->digital_power_refcount);
		if (atomic_read(&sprd_codec->digital_power_refcount) == 1) {
			arch_audio_codec_digital_reg_enable();
		}
	} else {
		if (atomic_dec_and_test(&sprd_codec->digital_power_refcount)) {
			arch_audio_codec_digital_reg_disable();
		}
	}
	ret = analog_power_enable(enable);

	return ret;
}

static int adie_dac_enable(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (enable) {
		atomic_inc(&sprd_codec->adie_dac_refcount);
		if (atomic_read(&sprd_codec->adie_dac_refcount) == 1) {
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_DACL_EN),
					    BIT(AUDIFA_DACL_EN));
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_DACR_EN),
					    BIT(AUDIFA_DACR_EN));
			pr_info("DAC ON\n");
		}
	} else {
		if (atomic_dec_and_test(&sprd_codec->adie_dac_refcount)) {
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_DACL_EN), 0);
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_DACR_EN), 0);
			pr_info("DAC OFF\n");
		}
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

static int adie_adc_enable(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (enable) {
		atomic_inc(&sprd_codec->adie_adc_refcount);
		if (atomic_read(&sprd_codec->adie_adc_refcount) == 1) {
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_ADCL_EN),
					    BIT(AUDIFA_ADCL_EN));
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_ADCR_EN),
					    BIT(AUDIFA_ADCR_EN));
			pr_info("ADC ON\n");
		}
	} else {
		if (atomic_dec_and_test(&sprd_codec->adie_adc_refcount)) {
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_ADCL_EN), 0);
			snd_soc_update_bits(codec, SOC_REG(AUDIF_ENB),
					    BIT(AUDIFA_ADCR_EN), 0);
			pr_info("ADC OFF\n");
		}
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

static int _mixer_set_mixer(struct snd_soc_codec *codec, int id, int lr,
			    int try_on)
{
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int reg = ID_FUN(id, lr);
	struct sprd_codec_mixer *mixer = &(sprd_codec->mixer[reg]);
	if (try_on) {
		mixer->set = mixer_setting[reg];
		return mixer->set(codec, mixer->on);
	} else {
		mixer_setting[reg] (codec, 0);
		mixer->set = 0;
	}
	return 0;
}

static inline int _mixer_setting(struct snd_soc_codec *codec, int start,
				 int end, int lr, int try_on)
{
	int id;
	for (id = start; id < end; id++) {
		_mixer_set_mixer(codec, id, lr, try_on);
	}
	return 0;
}

static inline int _mixer_setting_one(struct snd_soc_codec *codec, int id,
				     int try_on)
{
	int lr = id & 0x1;
	id >>= 1;
	return _mixer_setting(codec, id, id + 1, lr, try_on);
}

static int dac_digital_power(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	sprd_codec_dbg("%s %s %d\n", __func__, _2str(enable),
		       atomic_read(&sprd_codec->dac_digital_power_refcount));
	if (enable) {
		atomic_inc(&sprd_codec->dac_digital_power_refcount);
		if (atomic_read(&sprd_codec->dac_digital_power_refcount) == 1) {
			digital_power_enable(enable);
			snd_soc_update_bits(codec, SOC_REG(DACGR_DACR),
					    BIT(DACL_EN), BIT(DACL_EN));
			snd_soc_update_bits(codec, SOC_REG(DACGR_DACR),
					    BIT(DACR_EN), BIT(DACR_EN));
			snd_soc_update_bits(codec, SOC_REG(CCR),
					    BIT(DAC_CLK_EN), BIT(DAC_CLK_EN));
			ret = adie_dac_enable(enable);
		}
	} else {
		if (atomic_dec_and_test
		    (&sprd_codec->dac_digital_power_refcount)) {
			snd_soc_update_bits(codec, SOC_REG(DACGR_DACR),
					    BIT(DACL_EN), 0);
			snd_soc_update_bits(codec, SOC_REG(DACGR_DACR),
					    BIT(DACR_EN), 0);
			snd_soc_update_bits(codec, SOC_REG(CCR),
					    BIT(DAC_CLK_EN), 0);
			ret = adie_dac_enable(enable);
			digital_power_enable(enable);
		}
	}
	return ret;
}

int dacl_digital_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		ret = dac_digital_power(enable);
		snd_soc_update_bits(codec, SOC_REG(DACGR_DACR), BIT(DACL_EN),
				    BIT(DACL_EN));
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(DAC_EN_L),
				    BIT(DAC_EN_L));
	} else {
		snd_soc_update_bits(codec, SOC_REG(DACGR_DACR), BIT(DACL_EN),
				    0);
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(DAC_EN_L),
				    0);
		ret = dac_digital_power(enable);
	}
	return ret;
}

int dacr_digital_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		ret = dac_digital_power(enable);
		snd_soc_update_bits(codec, SOC_REG(DACGR_DACR), BIT(DACR_EN),
				    BIT(DACR_EN));
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(DAC_EN_R),
				    BIT(DAC_EN_R));
	} else {
		snd_soc_update_bits(codec, SOC_REG(DACGR_DACR), BIT(DACR_EN),
				    0);
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(DAC_EN_R),
				    0);
		ret = dac_digital_power(enable);
	}
	return ret;
}

static int dac_power(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	sprd_codec_dbg("%s %s %d\n", __func__, _2str(enable),
		       atomic_read(&sprd_codec->dac_power_refcount));
	if (enable) {
		atomic_inc(&sprd_codec->dac_power_refcount);
		if (atomic_read(&sprd_codec->dac_power_refcount) == 1) {
			analog_power_enable(enable);
			snd_soc_update_bits(codec, SOC_REG(CCR),
					    BIT(DRV_CLK_EN), BIT(DRV_CLK_EN));
		}
	} else {
		if (atomic_dec_and_test(&sprd_codec->dac_power_refcount)) {
			snd_soc_update_bits(codec, SOC_REG(CCR),
					    BIT(DRV_CLK_EN), 0);
			analog_power_enable(enable);
		}
	}
	return ret;
}

#ifdef CONFIG_SPRD_CODEC_USE_INT
#ifndef CONFIG_CODEC_NO_HP_POP
static void sprd_codec_hp_pop_irq_enable(struct snd_soc_codec *codec)
{
	int mask = BIT(AUDIO_POP_IRQ);
	snd_soc_update_bits(codec, SOC_REG(AUDIF_INT_CLR), mask, mask);
	snd_soc_update_bits(codec, SOC_REG(AUDIF_INT_EN), mask, mask);
}
#endif

static irqreturn_t sprd_codec_ap_irq(int irq, void *dev_id)
{
	int mask;
	struct sprd_codec_priv *sprd_codec = dev_id;
	struct snd_soc_codec *codec = sprd_codec->codec;
	mask = snd_soc_read(codec, AUDIF_INT_MASK);
	sprd_codec_dbg("hp pop irq mask = 0x%x\n", mask);
	if (BIT(AUDIO_POP_IRQ) & mask) {
		mask = BIT(AUDIO_POP_IRQ);
		snd_soc_update_bits(codec, SOC_REG(AUDIF_INT_EN), mask, 0);
		complete(&sprd_codec->completion_hp_pop);
	}
	return IRQ_HANDLED;
}
#endif

#ifndef CONFIG_CODEC_NO_HP_POP
static inline int is_hp_pop_compelet(struct snd_soc_codec *codec)
{
	int val;
	val = snd_soc_read(codec, IFR2_IFR1);
	val = (val >> HP_POP_FLG) & HP_POP_FLG_MASK;
	sprd_codec_dbg("HP POP= 0x%x\n", val);
	return HP_POP_FLG_NEAR_CMP == val;
}

static inline int hp_pop_wait_for_compelet(struct snd_soc_codec *codec)
{
#ifdef CONFIG_SPRD_CODEC_USE_INT
	int i;
	int hp_pop_complete;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	hp_pop_complete = msecs_to_jiffies(SPRD_CODEC_HP_POP_TIMEOUT);
	for (i = 0; i < 2; i++) {
		sprd_codec_dbg("hp pop %d irq enable\n", i);
		sprd_codec_hp_pop_irq_enable(codec);
		init_completion(&sprd_codec->completion_hp_pop);
		hp_pop_complete =
		    wait_for_completion_timeout(&sprd_codec->completion_hp_pop,
						hp_pop_complete);
		sprd_codec_dbg("hp pop %d completion %d\n", i, hp_pop_complete);
		if (!hp_pop_complete) {
			if (!is_hp_pop_compelet(codec)) {
				pr_err("hp pop %d timeout not complete\n", i);
			} else {
				pr_err("hp pop %d timeout but complete\n", i);
			}
		} else {
			/* 01 change to 10 maybe walk to 11 shortly,
			   so, check it double. */
			sprd_codec_wait(2);
			if (is_hp_pop_compelet(codec)) {
				return 0;
			}
		}
	}
#else
	int times;
	for (times = 0; times < SPRD_CODEC_HP_POP_TIME_COUNT; times++) {
		if (is_hp_pop_compelet(codec)) {
			/* 01 change to 10 maybe walk to 11 shortly,
			   so, check it double. */
			sprd_codec_wait(2);
			if (is_hp_pop_compelet(codec)) {
				return 0;
			}
		}
		sprd_codec_wait(SPRD_CODEC_HP_POP_TIME_STEP);
	}
	pr_err("hp pop wait timeout: times = %d \n", times);
#endif
	return 0;
}

static int hp_pop_enable(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	int mask;
	int ret = 0;

	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (enable) {
		mask = HP_POP_STEP_MASK << HP_POP_STEP;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_STEP_2 << HP_POP_STEP);
		mask = HP_POP_CTL_MASK << HP_POP_CTL;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_CTL_UP << HP_POP_CTL);
		sprd_codec_dbg("U PNRCR1 = 0x%x\n",
			       snd_soc_read(codec, PNRCR2_PNRCR1));

		ret = hp_pop_wait_for_compelet(codec);
		mask = HP_POP_CTL_MASK << HP_POP_CTL;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_CTL_HOLD << HP_POP_CTL);
		sprd_codec_dbg("HOLD PNRCR1 = 0x%x\n",
			       snd_soc_read(codec, PNRCR2_PNRCR1));
	} else {
		mask = HP_POP_STEP_MASK << HP_POP_STEP;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_STEP_1 << HP_POP_STEP);
		mask = HP_POP_CTL_MASK << HP_POP_CTL;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_CTL_DOWN << HP_POP_CTL);
		sprd_codec_dbg("D PNRCR1 = 0x%x\n",
			       snd_soc_read(codec, PNRCR2_PNRCR1));

		ret = hp_pop_wait_for_compelet(codec);
		mask = HP_POP_CTL_MASK << HP_POP_CTL;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_CTL_DIS << HP_POP_CTL);
		sprd_codec_dbg("DIS PNRCR1 = 0x%x\n",
			       snd_soc_read(codec, PNRCR2_PNRCR1));
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}
#else
#ifndef CONFIG_HP_POP_DELAY_TIME
#define CONFIG_HP_POP_DELAY_TIME (0)
#endif
static int hp_pop_enable(int enable)
{
	int ret = 0;
	sprd_codec_dbg("Entering %s is %s wait %dms\n", __func__,
		       _2str(enable), CONFIG_HP_POP_DELAY_TIME);
	if (enable) {
		sprd_codec_wait(CONFIG_HP_POP_DELAY_TIME);
	}
	sprd_codec_dbg("Leaving %s\n", __func__);
	return ret;
}
#endif

static int hp_switch_enable(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
#ifndef CONFIG_CODEC_NO_HP_POP
	int mask;
#endif
	int ret = 0;

	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (enable) {
#if 0				/* do not enable the diff function from weifeng.ni */
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(DIFF_EN),
				    BIT(DIFF_EN));
#endif

#ifndef CONFIG_CODEC_NO_HP_POP
		mask = HP_POP_CTL_MASK << HP_POP_CTL;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_CTL_DIS << HP_POP_CTL);
		sprd_codec_dbg("DIS(en) PNRCR1 = 0x%x\n",
			       snd_soc_read(codec, PNRCR2_PNRCR1));
#endif
	} else {
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(DIFF_EN), 0);

#ifndef CONFIG_CODEC_NO_HP_POP
		mask = HP_POP_CTL_MASK << HP_POP_CTL;
		snd_soc_update_bits(codec, SOC_REG(PNRCR2_PNRCR1), mask,
				    HP_POP_CTL_HOLD << HP_POP_CTL);
		sprd_codec_dbg("HOLD(en) PNRCR1 = 0x%x\n",
			       snd_soc_read(codec, PNRCR2_PNRCR1));
#endif
	}

	_mixer_setting(codec, SPRD_CODEC_HP_DACL,
		       SPRD_CODEC_HP_MIXER_MAX, SPRD_CODEC_LEFT,
		       snd_soc_read(codec, DCR2_DCR1) & BIT(HPL_EN));

	_mixer_setting(codec, SPRD_CODEC_HP_DACL,
		       SPRD_CODEC_HP_MIXER_MAX, SPRD_CODEC_RIGHT,
		       snd_soc_read(codec, DCR2_DCR1) & BIT(HPR_EN));

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

int hp_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		dac_power(enable);
#ifndef CONFIG_CODEC_NO_HP_POP
		hp_pop_enable(enable);
#endif
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(HPL_EN),
				    BIT(HPL_EN));
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(HPR_EN),
				    BIT(HPR_EN));
		ret = hp_switch_enable(enable);
#ifdef CONFIG_CODEC_NO_HP_POP
		hp_pop_enable(enable);
#endif
	} else {
#ifdef CONFIG_CODEC_NO_HP_POP
		hp_pop_enable(enable);
#endif
		ret = hp_switch_enable(enable);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(HPL_EN), 0);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(HPR_EN), 0);
#ifndef CONFIG_CODEC_NO_HP_POP
		hp_pop_enable(enable);
#endif
		dac_power(enable);
	}
	return ret;
}

static int spk_switch_enable(int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (snd_soc_read(codec, DCR2_DCR1) & BIT(AOL_EN)) {
		if (enable) {
			sprd_codec_pa_sw_set(SPRD_CODEC_PA_SW_AOL);
		} else {
			sprd_codec_pa_sw_clr(SPRD_CODEC_PA_SW_AOL);
		}
	}

	_mixer_setting(codec, SPRD_CODEC_SPK_DACL,
		       SPRD_CODEC_SPK_MIXER_MAX, SPRD_CODEC_LEFT,
		       (snd_soc_read(codec, DCR2_DCR1) & BIT(AOL_EN)));

	_mixer_setting(codec, SPRD_CODEC_SPK_DACL,
		       SPRD_CODEC_SPK_MIXER_MAX, SPRD_CODEC_RIGHT,
		       (snd_soc_read(codec, DCR2_DCR1) & BIT(AOR_EN)));

	sprd_codec_dbg("Leaving %s\n", __func__);

	return 0;
}

int spkl_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		dac_power(enable);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(AOL_EN),
				    BIT(AOL_EN));
		ret = spk_switch_enable(enable);
	} else {
		ret = spk_switch_enable(enable);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(AOL_EN), 0);
		dac_power(enable);
	}
	return ret;
}

int spkr_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		dac_power(enable);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(AOR_EN),
				    BIT(AOR_EN));
		ret = spk_switch_enable(enable);
	} else {
		ret = spk_switch_enable(enable);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(AOR_EN), 0);
		dac_power(enable);
	}
	return ret;
}

static int ear_switch_enable(int enable)
{
	int ret = 0;
	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

#ifdef CONFIG_SPRD_CODEC_EAR_WITH_IN_SPK
	if (enable) {
		sprd_codec_pa_sw_set(SPRD_CODEC_PA_SW_EAR);
	} else {
		sprd_codec_pa_sw_clr(SPRD_CODEC_PA_SW_EAR);
	}
#endif

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

int ear_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		dac_power(enable);
		ret = ear_switch_enable(enable);
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(EAR_EN),
				    BIT(EAR_EN));
	} else {
		snd_soc_update_bits(codec, SOC_REG(DCR2_DCR1), BIT(EAR_EN), 0);
		ret = ear_switch_enable(enable);
		dac_power(enable);
	}
	return ret;
}

static int adcpgal_set(struct snd_soc_codec *codec, int on)
{
	int mask = ADCPGAL_EN_MASK << ADCPGAL_EN;
	return snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1), mask,
				   on ? mask : 0);
}

static int adcpgar_set(struct snd_soc_codec *codec, int on)
{
	int mask = ADCPGAR_EN_MASK << ADCPGAR_EN;
	return snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1), mask,
				   on ? mask : 0);
}

static int adc_switch_enable(int enable, int right)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	int is_right = (right == SPRD_CODEC_RIGHT);
	sprd_codec_dbg("Entering %s is %s\n", __func__, _2str(enable));

	if (is_right) {
		adcpgar_set(codec, enable);
		_mixer_setting(codec, SPRD_CODEC_AIL, SPRD_CODEC_ADC_MIXER_MAX,
			       SPRD_CODEC_RIGHT, enable);
	} else {
		adcpgal_set(codec, enable);
		_mixer_setting(codec, SPRD_CODEC_AIL, SPRD_CODEC_ADC_MIXER_MAX,
			       SPRD_CODEC_LEFT, enable);
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return 0;
}

static int adc_digital_power(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	sprd_codec_dbg("%s %s %d\n", __func__, _2str(enable),
		       atomic_read(&sprd_codec->adc_digital_power_refcount));
	if (enable) {
		atomic_inc(&sprd_codec->adc_digital_power_refcount);
		if (atomic_read(&sprd_codec->adc_digital_power_refcount) == 1) {
			digital_power_enable(enable);
			snd_soc_update_bits(codec, SOC_REG(CCR),
					    BIT(ADC_CLK_EN), BIT(ADC_CLK_EN));
			adie_adc_enable(enable);
		}
	} else {
		if (atomic_dec_and_test
		    (&sprd_codec->adc_digital_power_refcount)) {
			snd_soc_update_bits(codec, SOC_REG(CCR),
					    BIT(ADC_CLK_EN), 0);
			adie_adc_enable(enable);
			digital_power_enable(enable);
		}
	}
	return ret;
}

int adcl_digital_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		ret = adc_digital_power(enable);
		snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1), BIT(ADCL_PD),
				    0);
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(ADC_EN_L),
				    BIT(ADC_EN_L));
	} else {
		snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1), BIT(ADCL_PD),
				    BIT(ADCL_PD));
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(ADC_EN_L),
				    0);
		ret = adc_digital_power(enable);
	}
	return ret;
}

int adcr_digital_switch(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	if (enable) {
		ret = adc_digital_power(enable);
		snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1), BIT(ADCR_PD),
				    0);
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(ADC_EN_R),
				    BIT(ADC_EN_R));
	} else {
		snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1), BIT(ADCR_PD),
				    BIT(ADCR_PD));
		snd_soc_update_bits(codec, SOC_REG(AUD_TOP_CTL), BIT(ADC_EN_R),
				    0);
		ret = adc_digital_power(enable);
	}
	return ret;
}

static int adc_power(int enable)
{
	int ret = 0;
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	sprd_codec_dbg("%s %s %d\n", __func__, _2str(enable),
		       atomic_read(&sprd_codec->adc_power_refcount));
	if (enable) {
		atomic_inc(&sprd_codec->adc_power_refcount);
		if (atomic_read(&sprd_codec->adc_power_refcount) == 1) {
			analog_power_enable(enable);
			snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1),
					    BIT(ADC_IBUF_PD), 0);
		}
	} else {
		if (atomic_dec_and_test(&sprd_codec->adc_power_refcount)) {
			snd_soc_update_bits(codec, SOC_REG(AACR2_AACR1),
					    BIT(ADC_IBUF_PD), BIT(ADC_IBUF_PD));
			analog_power_enable(enable);
		}
	}
	return ret;
}

int adcl_switch(int enable)
{
	int ret = 0;
	if (enable) {
		adc_power(enable);
		ret = adc_switch_enable(enable, SPRD_CODEC_LEFT);
	} else {
		ret = adc_switch_enable(enable, SPRD_CODEC_LEFT);
		adc_power(enable);
	}
	return ret;
}

int adcr_switch(int enable)
{
	int ret = 0;
	if (enable) {
		adc_power(enable);
		ret = adc_switch_enable(enable, SPRD_CODEC_RIGHT);
	} else {
		ret = adc_switch_enable(enable, SPRD_CODEC_RIGHT);
		adc_power(enable);
	}
	return ret;
}

int pga_enable(int id, int pgaval, int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	struct sprd_codec_pga_op *pga = &(sprd_codec->pga[id]);
	int ret = 0;
	int min = sprd_codec_pga_cfg[id].min;
	static int s_need_wait = 1;

	pga->pgaval = pgaval;

	sprd_codec_dbg("Entering %s set %s(%d) is %s\n", __func__,
		       sprd_codec_pga_debug_str[id], pga->pgaval,
		       _2str(enable));

	if (enable) {
		if ((id == SPRD_CODEC_PGA_ADCL) || (id == SPRD_CODEC_PGA_ADCR)) {
			if (s_need_wait == 1) {
				/* NOTES: reduce linein pop noise must delay 250ms
				   after linein mixer switch on.
				   actually this function perform after
				   adc_switch_enable function for
				   both ADCL/ADCR switch complete.
				 */
				if (sprd_codec_is_ai_enable(codec)) {
					sprd_codec_wait(250);
					sprd_codec_dbg("ADC Switch ON delay\n");
				}
				s_need_wait++;
			}
		}
		pga->set = sprd_codec_pga_cfg[id].set;
		ret = pga->set(codec, pga->pgaval);
	} else {
		if ((id == SPRD_CODEC_PGA_ADCL) || (id == SPRD_CODEC_PGA_ADCR)) {
			s_need_wait = 1;
		}
		pga->set = 0;
		ret = sprd_codec_pga_cfg[id].set(codec, min);
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

int mic_bias_enable(int id, int enable)
{
	int ret = 0;

	sprd_codec_dbg("Entering %s %s is %s\n", __func__,
		       mic_bias_name[id], _2str(enable));

	switch (id) {
	case SPRD_CODEC_MIC_BIAS:
		if (!(atomic_read(&sprd_codec_power.mic_on) > 0)) {
			sprd_codec_mic_bias_en(enable);
		}
		break;
	case SPRD_CODEC_AUXMIC_BIAS:
		if (!(atomic_read(&sprd_codec_power.auxmic_on) > 0)) {
			sprd_codec_auxmic_bias_en(enable);
		}
		break;
	default:
		BUG();
		ret = -EINVAL;
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

int mixer_enable(int id, int enable)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	struct sprd_codec_mixer *mixer = &(sprd_codec->mixer[id]);
	int ret = 0;

	pr_info("%s is %s\n", sprd_codec_mixer_debug_str[id], _2str(enable));

	if (enable) {
		mixer->on = 1;
	} else {
		mixer->on = 0;
	}
	if (ret >= 0)
		_mixer_setting_one(codec, id, mixer->on);

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

int mixer_get(int id)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	return sprd_codec->mixer[id].on;
}

int mixer_set(int id, int on)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	struct sprd_codec_mixer *mixer = &(sprd_codec->mixer[id]);
	int ret = 0;

	pr_info("set %s switch %s\n", sprd_codec_mixer_debug_str[id],
		on ? "ON" : "OFF");

	if (mixer->on == on)
		return 0;

	mixer->on = on;

	if (mixer->set)
		ret = mixer->set(codec, mixer->on);

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

static int sprd_codec_vol_put(int id, int pgaval)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	struct sprd_codec_pga_op *pga = &(sprd_codec->pga[id]);
	int ret = 0;

	pr_info("set PGA[%s] to %d\n", sprd_codec_pga_debug_str[id], pgaval);

	pga->pgaval = pgaval;
	if (pga->set) {
		ret = pga->set(codec, pga->pgaval);
	}
	sprd_codec_dbg("Leaving %s\n", __func__);
	return ret;
}

static int sprd_codec_vol_get(int id)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	struct sprd_codec_pga_op *pga = &(sprd_codec->pga[id]);

	return pga->pgaval;
}

static int sprd_codec_inter_pa_put(int config)
{
	int ret = 0;

	pr_info("config inter PA 0x%08x\n", config);

	mutex_lock(&inter_pa_mutex);
	inter_pa.value = (u32) config;
	if (inter_pa.set) {
		mutex_unlock(&inter_pa_mutex);
		sprd_inter_speaker_pa(1);
	} else {
		mutex_unlock(&inter_pa_mutex);
	}
	sprd_codec_dbg("Leaving %s\n", __func__);
	return ret;
}

static int sprd_codec_inter_pa_get(void)
{
	int ret;
	mutex_lock(&inter_pa_mutex);
	ret = inter_pa.value;
	mutex_unlock(&inter_pa_mutex);

	return ret;
}

static int sprd_codec_inter_hp_pa_put(int config)
{
	int ret = 0;

	pr_info("config inter HP PA 0x%08x\n", config);

	mutex_lock(&inter_hp_pa_mutex);
	inter_hp_pa.value = (u32) config;
	if (inter_hp_pa.set) {
		mutex_unlock(&inter_hp_pa_mutex);
		sprd_inter_headphone_pa(1);
	} else {
		mutex_unlock(&inter_hp_pa_mutex);
	}
	sprd_codec_dbg("Leaving %s\n", __func__);
	return ret;
}

static int sprd_codec_inter_hp_pa_get(void)
{
	int ret;
	mutex_lock(&inter_hp_pa_mutex);
	ret = inter_hp_pa.value;
	mutex_unlock(&inter_hp_pa_mutex);

	return ret;
}

int sprd_codec_pcm_set_sample_rate(int playback, int rate)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int mask = 0x0F;
	int shift = 0;

	if (playback) {
		sprd_codec->da_sample_val = rate;
		pr_info("playback rate is [%d]\n", rate);
		sprd_codec_set_sample_rate(codec, rate, mask, shift);
	} else {
		sprd_codec->ad_sample_val = rate;
		pr_info("capture rate is [%d]\n", rate);
		sprd_codec_set_ad_sample_rate(codec, rate, mask, shift);
		sprd_codec_set_ad_sample_rate(codec, rate,
					      ADC1_SRC_N_MASK, ADC1_SRC_N);
	}

	return 0;
}

#ifdef CONFIG_SPRD_CODEC_USE_INT
#ifdef CONFIG_CODEC_DAC_MUTE_WAIT
static void sprd_codec_dac_mute_irq_enable(struct snd_soc_codec *codec)
{
	int mask = BIT(DAC_MUTE_D);
	snd_soc_update_bits(codec, SOC_REG(AUD_INT_CLR), mask, mask);
	snd_soc_update_bits(codec, SOC_REG(AUD_INT_EN), mask, mask);
}
#endif

static irqreturn_t sprd_codec_dp_irq(int irq, void *dev_id)
{
	int mask;
	struct sprd_codec_priv *sprd_codec = dev_id;
	struct snd_soc_codec *codec = sprd_codec->codec;
	mask = snd_soc_read(codec, AUD_AUD_STS0);
	sprd_codec_dbg("dac mute irq mask = 0x%x\n", mask);
	if (BIT(DAC_MUTE_D_MASK) & mask) {
		mask = BIT(DAC_MUTE_D);
		complete(&sprd_codec->completion_dac_mute);
	}
	if (BIT(DAC_MUTE_U_MASK) & mask) {
		mask = BIT(DAC_MUTE_U);
	}
	snd_soc_update_bits(codec, SOC_REG(AUD_INT_EN), mask, 0);
	return IRQ_HANDLED;
}
#endif

static int sprd_codec_digital_mute(int mute)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	sprd_codec_dbg("Entering %s\n", __func__);

	if (atomic_read(&sprd_codec->power_refcount) >= 1) {
		sprd_codec_dbg("mute %i\n", mute);

		ret =
		    snd_soc_update_bits(codec, SOC_REG(AUD_DAC_CTL),
					BIT(DAC_MUTE_START),
					mute ? BIT(DAC_MUTE_START) : 0);
#ifdef CONFIG_CODEC_DAC_MUTE_WAIT
#ifdef CONFIG_SPRD_CODEC_USE_INT
		if (mute && ret) {
			int dac_mute_complete;
			sprd_codec_dbg("dac mute irq enable\n");
			sprd_codec_dac_mute_irq_enable(codec);
			init_completion(&sprd_codec->completion_dac_mute);
			dac_mute_complete =
			    wait_for_completion_timeout
			    (&sprd_codec->completion_dac_mute,
			     msecs_to_jiffies(SPRD_CODEC_DAC_MUTE_TIMEOUT));
			sprd_codec_dbg("dac mute completion %d\n",
				       dac_mute_complete);
			if (!dac_mute_complete) {
				pr_err("dac mute timeout\n");
			}
		}
#else
		if (mute && ret) {
			sprd_codec_wait(SPRD_CODEC_DAC_MUTE_WAIT_TIME);
		}
#endif
#endif

		sprd_codec_dbg("return %i\n", ret);
	}

	sprd_codec_dbg("Leaving %s\n", __func__);

	return ret;
}

static int sprd_codec_probe(void)
{
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);

	sprd_codec_dbg("Entering %s\n", __func__);

	sprd_codec->codec = codec;

	atomic_set(&sprd_codec->power_refcount, 0);
	atomic_set(&sprd_codec->adie_adc_refcount, 0);
	atomic_set(&sprd_codec->adie_dac_refcount, 0);
	atomic_set(&sprd_codec->adc_power_refcount, 0);
	atomic_set(&sprd_codec->adc_digital_power_refcount, 0);
	atomic_set(&sprd_codec->dac_power_refcount, 0);
	atomic_set(&sprd_codec->dac_digital_power_refcount, 0);
	atomic_set(&sprd_codec->digital_power_refcount, 0);
	atomic_set(&sprd_codec->analog_power_refcount, 0);

#ifdef CONFIG_SPRD_CODEC_USE_INT
	sprd_codec->ap_irq = CODEC_AP_IRQ;

	ret =
	    request_irq(sprd_codec->ap_irq, sprd_codec_ap_irq, 0,
			"sprd_codec_ap", sprd_codec);
	if (ret) {
		pr_err("request_irq ap failed!\n");
		goto err_irq;
	}

	sprd_codec->dp_irq = CODEC_DP_IRQ;

	ret =
	    request_irq(sprd_codec->dp_irq, sprd_codec_dp_irq, 0,
			"sprd_codec_dp", sprd_codec);
	if (ret) {
		pr_err("request_irq dp failed!\n");
		goto dp_err_irq;
	}
#endif

	sprd_codec_dbg("Leaving %s\n", __func__);

	return 0;

#ifdef CONFIG_SPRD_CODEC_USE_INT
dp_err_irq:
	free_irq(sprd_codec->ap_irq, sprd_codec);
err_irq:
	return -EINVAL;
#endif
}

static int sprd_codec_deinit(void)
{
#ifdef CONFIG_SPRD_CODEC_USE_INT
	struct snd_soc_codec *codec = &s_sprd_codec;
	struct sprd_codec_priv *sprd_codec = snd_soc_codec_get_drvdata(codec);
	free_irq(sprd_codec->ap_irq, sprd_codec);
	free_irq(sprd_codec->dp_irq, sprd_codec);
#endif
	return 0;
}

int sprd_codec_init(void)
{
	sprd_codec_inter_pa_init();
	sprd_codec_inter_hp_pa_init();
	arch_audio_codec_switch(AUDIO_TO_AP_ARM_CTRL);
	sprd_codec_probe();
	return 0;
}

void sprd_codec_exit(void)
{
	sprd_codec_deinit();
}

module_init(sprd_codec_init);
module_exit(sprd_codec_exit);

MODULE_DESCRIPTION("SPRD-CODEC ALSA SoC codec driver");
MODULE_AUTHOR("Zhenfang Wang <zhenfang.wang@spreadtrum.com>");
MODULE_LICENSE("GPL");
