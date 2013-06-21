/*
 * sound/soc/sprd/dai/vbc/vbc.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "[audio: vbc ] " fmt

#include <common.h>
#include <errno.h>
#include <asm/atomic.h>
#include <ubi_uboot.h>

#include "vbc_r2p0.h"
#include "../sound_common.h"

#ifdef CONFIG_SPRD_AUDIO_DEBUG
#define vbc_dbg pr_info
#else
#define vbc_dbg(...)
#endif

#define FUN_REG(f) ((unsigned short)(-((f) + 1)))

#define DEFINE_SPINLOCK(...)
#define DEFINE_MUTEX(...)

#define VBC_DG_VAL_MAX (0x7F)

struct vbc_fw_header {
	char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
	u32 profile_version;
	u32 num_profile;
};

struct vbc_eq_profile {
	char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
	char name[VBC_EQ_PROFILE_NAME_MAX];
	/* TODO */
	u32 effect_paras[VBC_EFFECT_PARAS_LEN];
};

static const u32 vbc_eq_profile_default[VBC_EFFECT_PARAS_LEN] = {
/* TODO the default register value */
	0x00000000,		/*  DAPATCHCTL      */
	0x00001818,		/*  DADGCTL         */
	0x0000007F,		/*  DAHPCTL         */
	0x00000000,		/*  DAALCCTL0       */
	0x00000000,		/*  DAALCCTL1       */
	0x00000000,		/*  DAALCCTL2       */
	0x00000000,		/*  DAALCCTL3       */
	0x00000000,		/*  DAALCCTL4       */
	0x00000000,		/*  DAALCCTL5       */
	0x00000000,		/*  DAALCCTL6       */
	0x00000000,		/*  DAALCCTL7       */
	0x00000000,		/*  DAALCCTL8       */
	0x00000000,		/*  DAALCCTL9       */
	0x00000000,		/*  DAALCCTL10      */
	0x00000183,		/*  STCTL0          */
	0x00000183,		/*  STCTL1          */
	0x00000000,		/*  ADPATCHCTL      */
	0x00001818,		/*  ADDG01CTL         */
	0x00001818,		/*  ADDG23CTL         */
	0x00000000,		/*  ADHPCTL         */
	0x00000000,		/*  ADCSRCCTL         */
	0x00000000,		/*  DACSRCCTL         */
	0x00000000,		/*  MIXERCTL         */
	0x00000000,		/*  VBNGCVTHD        */
	0x00000000,		/*  VBNGCTTHD          */
	0x00000000,		/*  VBNGCTL         */
	0x00000000,		/*  HPCOEF0_H         */
};

struct vbc_equ {
	struct device *dev;
	int is_active;
	int is_loaded;
	int is_loading;
	int now_profile;
	struct vbc_fw_header hdr;
	struct vbc_eq_profile *data;
	void (*vbc_eq_apply) (void *data);
};

typedef int (*vbc_dma_set) (int enable);
typedef int (*vbc_dg_set) (int enable, int dg);
DEFINE_MUTEX(vbc_mutex);
struct vbc_priv {
	vbc_dma_set dma_set[2];

	int (*arch_enable) (int chan);
	int (*arch_disable) (int chan);
	int is_open;
	int is_active;
	int used_chan_count;
	int dg_switch[2];
	int dg_val[2];
	vbc_dg_set dg_set[2];
};

DEFINE_MUTEX(load_mutex);
static struct vbc_equ vbc_eq_setting = { 0 };

static void vbc_eq_try_apply(void);
static struct vbc_priv vbc[3];

DEFINE_SPINLOCK(vbc_lock);
/* local register setting */
static int vbc_reg_write(int reg, int val, int mask)
{
	int tmp, ret;
	spin_lock(&vbc_lock);
	tmp = __raw_readl(reg);
	ret = tmp;
	tmp &= ~(mask);
	tmp |= val & mask;
	__raw_writel(tmp, reg);
	spin_unlock(&vbc_lock);
	return ret & (mask);
}

static inline int vbc_reg_read(int reg)
{
	int tmp;
	tmp = __raw_readl(reg);
	return tmp;
}

static int vbc_set_buffer_size(int ad_buffer_size, int da_buffer_size,
			       int ad23_buffer_size)
{
	int val = vbc_reg_read(VBBUFFSIZE);
	WARN_ON(ad_buffer_size > VBC_FIFO_FRAME_NUM);
	WARN_ON(da_buffer_size > VBC_FIFO_FRAME_NUM);
	WARN_ON(ad23_buffer_size > VBC_FIFO_FRAME_NUM);
	if ((ad_buffer_size > 0)
	    && (ad_buffer_size <= VBC_FIFO_FRAME_NUM)) {
		val &= ~(VBADBUFFERSIZE_MASK);
		val |= (((ad_buffer_size - 1) << VBADBUFFERSIZE_SHIFT)
			& VBADBUFFERSIZE_MASK);
	}
	if ((da_buffer_size > 0)
	    && (da_buffer_size <= VBC_FIFO_FRAME_NUM)) {
		val &= ~(VBDABUFFERSIZE_MASK);
		val |= (((da_buffer_size - 1) << VBDABUFFERSIZE_SHIFT)
			& VBDABUFFERSIZE_MASK);
	}
	vbc_reg_write(VBBUFFSIZE, val,
		      (VBDABUFFERSIZE_MASK | VBADBUFFERSIZE_MASK));
	if ((ad23_buffer_size > 0)
	    && (ad23_buffer_size <= VBC_FIFO_FRAME_NUM)) {
		val &= ~(VBAD23BUFFERSIZE_MASK);
		val |= (((ad23_buffer_size - 1) << VBAD23BUFFERSIZE_SHIFT)
			& VBAD23BUFFERSIZE_MASK);
	}
	vbc_reg_write(VBBUFFAD23, val, VBAD23BUFFERSIZE_MASK);
	return 0;
}

static inline int vbc_sw_write_buffer(int enable)
{
	/* Software access ping-pong buffer enable when VBENABE bit low */
	vbc_reg_write(VBDABUFFDTA, ((enable ? 1 : 0) << RAMSW_EN),
		      (1 << RAMSW_EN));
	return 0;
}

static inline int vbc_da_enable(int enable, int chan)
{
	vbc_reg_write(VBCHNEN, ((enable ? 1 : 0) << (VBDACHEN_SHIFT + chan)),
		      (1 << (VBDACHEN_SHIFT + chan)));
	return 0;
}

static inline int vbc_ad_enable(int enable, int chan)
{
	vbc_reg_write(VBCHNEN, ((enable ? 1 : 0) << (VBADCHEN_SHIFT + chan)),
		      (1 << (VBADCHEN_SHIFT + chan)));
	return 0;
}

static inline int vbc_ad23_enable(int enable, int chan)
{
	vbc_reg_write(VBCHNEN, ((enable ? 1 : 0) << (VBAD23CHEN_SHIFT + chan)),
		      (1 << (VBAD23CHEN_SHIFT + chan)));
	return 0;
}

static inline int vbc_enable_set(int enable)
{
	vbc_reg_write(VBADBUFFDTA, (0 << VBIIS_LRCK), (1 << VBIIS_LRCK));
	vbc_reg_write(VBDABUFFDTA, ((enable ? 1 : 0) << VBENABLE),
		      (1 << VBENABLE));
	return 0;
}

static inline int vbc_ad0_dma_set(int enable)
{
	vbc_reg_write(VBDABUFFDTA, ((enable ? 1 : 0) << VBAD0DMA_EN),
		      (1 << VBAD0DMA_EN));
	return 0;
}

static inline int vbc_ad1_dma_set(int enable)
{
	vbc_reg_write(VBDABUFFDTA, ((enable ? 1 : 0) << VBAD1DMA_EN),
		      (1 << VBAD1DMA_EN));
	return 0;
}

static inline int vbc_ad2_dma_set(int enable)
{
	vbc_reg_write(VBADDMA, ((enable ? 1 : 0) << VBAD2DMA_EN),
		      (1 << VBAD2DMA_EN));
	return 0;
}

static inline int vbc_ad3_dma_set(int enable)
{
	vbc_reg_write(VBADDMA, ((enable ? 1 : 0) << VBAD3DMA_EN),
		      (1 << VBAD3DMA_EN));
	return 0;
}

static inline int vbc_da0_dma_set(int enable)
{
	vbc_reg_write(VBDABUFFDTA, ((enable ? 1 : 0) << VBDA0DMA_EN),
		      (1 << VBDA0DMA_EN));
	return 0;
}

static inline int vbc_da1_dma_set(int enable)
{
	vbc_reg_write(VBDABUFFDTA, ((enable ? 1 : 0) << VBDA1DMA_EN),
		      (1 << VBDA1DMA_EN));
	return 0;
}

static void vbc_da_buffer_clear(int id)
{
	int i;
	vbc_reg_write(VBDABUFFDTA, ((id ? 1 : 0) << RAMSW_NUMB),
		      (1 << RAMSW_NUMB));
	for (i = 0; i < VBC_FIFO_FRAME_NUM; i++) {
		__raw_writel(0, VBDA0);
		__raw_writel(0, VBDA1);
	}
}

static void vbc_da_buffer_clear_all(void)
{
	int i;
	int ret;
	int save_enable = 0;
	ret = arch_audio_vbc_enable();
	if (ret < 0) {
		pr_err("Failed to enable VBC\n");
	}
	save_enable |= (ret << 2);
	for (i = 0; i < 2; i++) {
		ret = vbc_da_enable(1, i);
		if (ret < 0) {
			pr_err("Failed to enable VBC DA%d\n", i);
		}
		save_enable |= (ret << i);
	}
	vbc_sw_write_buffer(true);
	vbc_set_buffer_size(0, VBC_FIFO_FRAME_NUM, 0);
	vbc_da_buffer_clear(1);	/* clear data buffer 1 */
	vbc_da_buffer_clear(0);	/* clear data buffer 0 */
	vbc_sw_write_buffer(false);
	for (i = 0; i < 2; i++, save_enable >>= 1) {
		if (save_enable & 0x1) {
			vbc_da_enable(0, i);
		}
	}
	save_enable >>= 1;
	if (save_enable & 0x1) {
		arch_audio_vbc_disable();
	}
}

static inline int vbc_str_2_index(int stream);

static int vbc_da_arch_enable(int chan)
{
	int ret;
	ret = vbc_da_enable(1, chan);
	if (ret < 0) {
		pr_err("VBC da enable error:%i\n", ret);
		return ret;
	} else {
		arch_audio_vbc_enable();
		vbc[vbc_str_2_index(SNDRV_PCM_STREAM_PLAYBACK)].is_active = 1;
	}
	return ret;
}

static int vbc_da_arch_disable(int chan)
{
	int ret;
	ret = vbc_da_enable(0, chan);
	if (ret < 0) {
		pr_err("VBC da disable error:%i\n", ret);
		return ret;
	} else {
		vbc[vbc_str_2_index(SNDRV_PCM_STREAM_PLAYBACK)].is_active = 0;
	}
	return ret;
}

static int vbc_ad_arch_enable(int chan)
{
	int ret;
	ret = vbc_ad_enable(1, chan);
	if (ret < 0) {
		pr_err("VBC ad enable error:%i\n", ret);
		return ret;
	} else {
		arch_audio_vbc_enable();
		vbc[vbc_str_2_index(SNDRV_PCM_STREAM_CAPTURE)].is_active = 1;
	}
	return ret;
}

static int vbc_ad_arch_disable(int chan)
{
	int ret;
	ret = vbc_ad_enable(0, chan);
	if (ret < 0) {
		pr_err("VBC ad disable error:%i\n", ret);
		return ret;
	} else {
		vbc[vbc_str_2_index(SNDRV_PCM_STREAM_CAPTURE)].is_active = 0;
	}
	return ret;
}

static int vbc_ad23_arch_enable(int chan)
{
	int ret;
	ret = vbc_ad23_enable(1, chan);
	if (ret < 0) {
		pr_err("VBC ad enable error:%i\n", ret);
		return ret;
	} else {
		arch_audio_vbc_enable();
		vbc[vbc_str_2_index(SNDRV_PCM_STREAM_CAPTURE) + 1].is_active =
		    1;
	}
	return ret;
}

static int vbc_ad23_arch_disable(int chan)
{
	int ret;
	ret = vbc_ad23_enable(0, chan);
	if (ret < 0) {
		pr_err("VBC ad23 disable error:%i\n", ret);
		return ret;
	} else {
		vbc[vbc_str_2_index(SNDRV_PCM_STREAM_CAPTURE) + 1].is_active =
		    0;
	}
	return ret;
}

static inline int vbc_da0_dg_set(int enable, int dg)
{
	if (enable) {
		vbc_reg_write(DADGCTL, 0x80 | (0xFF & dg), 0xFF);
	} else {
		vbc_reg_write(DADGCTL, 0, 0x80);
	}
	return 0;
}

static inline int vbc_da1_dg_set(int enable, int dg)
{
	if (enable) {
		vbc_reg_write(DADGCTL, (0x80 | (0xFF & dg)) << 8, 0xFF00);
	} else {
		vbc_reg_write(DADGCTL, 0, 0x8000);
	}
	return 0;
}

static inline int vbc_ad0_dg_set(int enable, int dg)
{
	if (enable) {
		vbc_reg_write(ADDG01CTL, 0x80 | (0xFF & dg), 0xFF);
	} else {
		vbc_reg_write(ADDG01CTL, 0, 0x80);
	}
	return 0;
}

static inline int vbc_ad1_dg_set(int enable, int dg)
{
	if (enable) {
		vbc_reg_write(ADDG01CTL, (0x80 | (0xFF & dg)) << 8, 0xFF00);
	} else {
		vbc_reg_write(ADDG01CTL, 0, 0x8000);
	}
	return 0;
}

static inline int vbc_ad2_dg_set(int enable, int dg)
{
	if (enable) {
		vbc_reg_write(ADDG23CTL, 0x80 | (0xFF & dg), 0xFF);
	} else {
		vbc_reg_write(ADDG23CTL, 0, 0x80);
	}
	return 0;
}

static inline int vbc_ad3_dg_set(int enable, int dg)
{
	if (enable) {
		vbc_reg_write(ADDG23CTL, (0x80 | (0xFF & dg)) << 8, 0xFF00);
	} else {
		vbc_reg_write(ADDG23CTL, 0, 0x8000);
	}
	return 0;
}

static int vbc_try_dg_set(int vbc_idx, int id)
{
	int dg = vbc[vbc_idx].dg_val[id];
	if (vbc[vbc_idx].dg_switch[id]) {
		vbc[vbc_idx].dg_set[id] (1, dg);
	} else {
		vbc[vbc_idx].dg_set[id] (0, dg);
	}
	return 0;
}

int vbc_adc_sel_iis(int port)
{
	vbc_reg_write(VBIISSEL, port << VBIISSEL_AD01_PORT_SHIFT,
		      VBIISSEL_AD01_PORT_MASK);
	return 0;
}

int vbc_adc23_sel_iis(int port)
{
	vbc_reg_write(VBIISSEL, port << VBIISSEL_AD23_PORT_SHIFT,
		      VBIISSEL_AD23_PORT_MASK);
	return 0;
}

int vbc_dac0_fm_mixer(int mode)
{
	vbc_reg_write(DAPATCHCTL, mode << VBDAPATH_DA0_ADDFM_SHIFT,
		      VBDAPATH_DA0_ADDFM_MASK);
	return 0;
}

int vbc_dac1_fm_mixer(int mode)
{
	vbc_reg_write(DAPATCHCTL, mode << VBDAPATH_DA1_ADDFM_SHIFT,
		      VBDAPATH_DA1_ADDFM_MASK);
	return 0;
}

static int vbc_dac_src_enable(int enable)
{
	int i;
	int mask =
	    (1 << VBDACSRC_F1F2F3_BP) | (1 << VBDACSRC_F1_SEL) | (1 <<
								  VBDACSRC_F0_BP)
	    | (1 << VBDACSRC_F0_SEL) | (1 << VBDACSRC_EN);
	if (enable) {
		//src_clr
		vbc_reg_write(DACSRCCTL, (1 << VBDACSRC_CLR),
			      (1 << VBDACSRC_CLR));
		for (i = 0; i < 10; i++) ;
		vbc_reg_write(DACSRCCTL, 0, (1 << VBDACSRC_CLR));

		//src_set and enable
		vbc_reg_write(DACSRCCTL, 0x31, mask);
	} else {
		vbc_reg_write(DACSRCCTL, 0, 0x7F);
	}
	return 0;
}

static int vbc_st0_enable(int enable)
{
	vbc_reg_write(STCTL0, (enable ? (1 << 12) : 0), 1 << 12);
	return 0;
}

static int vbc_st1_enable(int enable)
{
	vbc_reg_write(STCTL0, (enable ? (1 << 12) : 0), 1 << 12);
	return 0;
}

static void digtal_fm_input_enable(int enable)
{
	/*we suppose that  digital fm input from vbc ad01 */
	if (enable) {
		vbc_adc_sel_iis(1);	/*digital fm ==> vbc */
	} else {
		vbc_adc_sel_iis(0);	/*codec ==> vbc */
	}

	/*ST enable */
	vbc_st0_enable(enable);
	vbc_st1_enable(enable);

	/*SRC set */
	vbc_dac_src_enable(enable);	/*todo:maybe we need to reserve SRC value */

	/*todo:  set fm input pin */
}

static struct vbc_priv vbc[3] = {
	{			/*PlayBack */
	 .dma_set = {vbc_da0_dma_set, vbc_da1_dma_set},
	 .arch_enable = vbc_da_arch_enable,
	 .arch_disable = vbc_da_arch_disable,
	 .is_active = 0,
	 .dg_switch = {0, 0},
	 .dg_val = {0x18, 0x18},
	 .dg_set = {vbc_da0_dg_set, vbc_da1_dg_set},
	 },
	{			/*Capture for ad01 */
	 .dma_set = {vbc_ad0_dma_set, vbc_ad1_dma_set},
	 .arch_enable = vbc_ad_arch_enable,
	 .arch_disable = vbc_ad_arch_disable,
	 .is_active = 0,
	 .dg_switch = {0, 0},
	 .dg_val = {0x18, 0x18},
	 .dg_set = {vbc_ad0_dg_set, vbc_ad1_dg_set},
	 },
	{			/*Capture for ad23 */
	 .dma_set = {vbc_ad2_dma_set, vbc_ad3_dma_set},
	 .arch_enable = vbc_ad23_arch_enable,
	 .arch_disable = vbc_ad23_arch_disable,
	 .is_active = 0,
	 .dg_switch = {0, 0},
	 .dg_val = {0x18, 0x18},
	 .dg_set = {vbc_ad2_dg_set, vbc_ad3_dg_set},
	 },
};

/* NOTE:
   this index need use for the [struct vbc_priv] vbc[2] index
   default MUST return 0.
 */
static inline int vbc_str_2_index(int stream)
{
	if (stream == SNDRV_PCM_STREAM_CAPTURE) {
		return 1;
	}

	return 0;
}

static inline void vbc_reg_enable(void)
{
	arch_audio_vbc_reg_enable();
}

int vbc_startup(int stream)
{
	int vbc_idx;

	vbc_dbg("Entering %s\n", __func__);
	vbc_idx = vbc_str_2_index(stream);

	if (vbc[vbc_idx].is_open || vbc[vbc_idx].is_active) {
		pr_err("vbc is actived:%d\n", stream);
	}

	mutex_lock(&vbc_mutex);
	vbc[vbc_idx].is_open = 1;
	mutex_unlock(&vbc_mutex);

	vbc_reg_enable();

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		vbc_da_buffer_clear_all();
		vbc_set_buffer_size(0, VBC_FIFO_FRAME_NUM, 0);
		vbc_eq_try_apply();
	} else if (vbc_idx == 1) {
		vbc_set_buffer_size(VBC_FIFO_FRAME_NUM, 0, 0);
	} else {
		vbc_set_buffer_size(0, 0, VBC_FIFO_FRAME_NUM);
	}

	vbc_try_dg_set(vbc_idx, VBC_LEFT);
	vbc_try_dg_set(vbc_idx, VBC_RIGHT);

	vbc[vbc_idx].used_chan_count = 2;

	WARN_ON(!vbc[vbc_idx].arch_enable);
	WARN_ON(!vbc[vbc_idx].arch_disable);
	WARN_ON(!vbc[vbc_idx].dma_set[0]);
	WARN_ON(!vbc[vbc_idx].dma_set[1]);

	vbc_dbg("Leaving %s\n", __func__);
	return 0;
}

static inline int vbc_can_close(void)
{
	return !(vbc[vbc_str_2_index(SNDRV_PCM_STREAM_PLAYBACK)].is_open
		 || vbc[vbc_str_2_index(SNDRV_PCM_STREAM_CAPTURE)].is_open
		 ||
		 vbc[(vbc_str_2_index(SNDRV_PCM_STREAM_CAPTURE) + 1)].is_open);
}

void vbc_shutdown(int stream)
{
	int vbc_idx;
	int i;

	vbc_dbg("Entering %s\n", __func__);

	/* vbc da close MUST clear da buffer */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		vbc_da_buffer_clear_all();
	}

	vbc_idx = vbc_str_2_index(stream);

	for (i = 0; i < 2; i++) {
		vbc[vbc_idx].arch_disable(i);
		vbc[vbc_idx].dma_set[i] (0);
	}

	mutex_lock(&vbc_mutex);
	vbc[vbc_idx].is_open = 0;
	if (vbc_can_close()) {
		vbc_enable_set(0);
		arch_audio_vbc_reset();
		arch_audio_vbc_disable();
		arch_audio_vbc_reg_disable();
		pr_info("close the VBC\n");
	}
	mutex_unlock(&vbc_mutex);

	vbc_dbg("Leaving %s\n", __func__);
}

int vbc_trigger(int stream, int enable)
{
	int vbc_idx;
	int ret = 0;
	int i;

#if 0
	vbc_dbg("Entering %s\n", __func__);
#endif

	vbc_idx = vbc_str_2_index(stream);

	if (enable) {
		for (i = 0; i < vbc[vbc_idx].used_chan_count; i++) {
			vbc[vbc_idx].arch_enable(i);
			vbc[vbc_idx].dma_set[i] (1);
		}
		vbc_enable_set(1);
	} else {
		for (i = 0; i < vbc[vbc_idx].used_chan_count; i++) {
			vbc[vbc_idx].arch_disable(i);
			vbc[vbc_idx].dma_set[i] (0);
		}

		if (vbc_can_close()) {
			vbc_enable_set(0);
			arch_audio_vbc_disable();
		}
	}

#if 0
	vbc_dbg("Leaving %s\n", __func__);
#endif
	return ret;
}

static int vbc_eq_reg_offset(u32 reg)
{
	int i = 0;
	if ((reg >= DAPATCHCTL) && (reg <= MIXERCTL)) {
		i = (reg - DAPATCHCTL) >> 2;
	} else if ((reg >= VBNGCVTHD) && (reg <= VBNGCTL)) {
		i = ((reg - VBNGCVTHD) >> 2) + ((MIXERCTL - DAPATCHCTL) >> 2) +
		    1;
	} else if ((reg >= HPCOEF0_H) && (reg <= HPCOEF71_L)) {
		i = ((reg - HPCOEF0_H) >> 2) + ((VBNGCTL - VBNGCVTHD) >> 2) +
		    ((MIXERCTL - DAPATCHCTL) >> 2) + 2;
	}
	BUG_ON(i >= VBC_EFFECT_PARAS_LEN);
	return i;
}

static inline void vbc_eq_reg_set(u32 reg, void *data)
{
	u32 *effect_paras = data;
	vbc_dbg("reg(0x%x) = (0x%x)\n", reg,
		effect_paras[vbc_eq_reg_offset(reg)]);
	__raw_writel(effect_paras[vbc_eq_reg_offset(reg)], reg);
}

static inline void vbc_eq_reg_set_range(u32 reg_start, u32 reg_end, void *data)
{
	u32 reg_addr;
	for (reg_addr = reg_start; reg_addr <= reg_end; reg_addr += 4) {
		vbc_eq_reg_set(reg_addr, data);
	}
}

static void vbc_eq_reg_apply(void *data)
{
	vbc_eq_reg_set_range(DAALCCTL0, DAALCCTL10, data);
	vbc_eq_reg_set_range(HPCOEF0_H, HPCOEF71_L, data);

	vbc_eq_reg_set(DAHPCTL, data);
	vbc_eq_reg_set(DAPATCHCTL, data);

	vbc_eq_reg_set(STCTL0, data);
	vbc_eq_reg_set(STCTL1, data);

	vbc_eq_reg_set(ADPATCHCTL, data);
	vbc_eq_reg_set_range(ADHPCTL, VBNGCTL, data);
}

static void vbc_eq_profile_apply(void *data)
{
	vbc_eq_reg_apply(data);
}

static void vbc_eq_profile_close(void)
{
	vbc_eq_profile_apply(&vbc_eq_profile_default);
}

static void vbc_eq_try_apply(void)
{
	u32 *data;
	vbc_dbg("Entering %s 0x%x\n", __func__,
		(int)vbc_eq_setting.vbc_eq_apply);
	if (vbc_eq_setting.vbc_eq_apply) {
		mutex_lock(&load_mutex);
		if (vbc_eq_setting.is_loaded) {
			struct vbc_eq_profile *now =
			    &vbc_eq_setting.data[vbc_eq_setting.now_profile];
			data = now->effect_paras;
			pr_info("vbc eq apply '%s'\n", now->name);
			vbc_eq_setting.vbc_eq_apply(data);
		}
		mutex_unlock(&load_mutex);
	}
	vbc_dbg("Leaving %s\n", __func__);
}

static int vbc_eq_profile_get(void)
{
	return vbc_eq_setting.now_profile;
}

static int vbc_eq_profile_put(int select)
{
	int ret = 0;

	pr_info("vbc eq select %d max %d\n", select,
		vbc_eq_setting.hdr.num_profile);

	ret = select;
	if (ret == vbc_eq_setting.now_profile) {
		return ret;
	}
	if (ret < vbc_eq_setting.hdr.num_profile) {
		vbc_eq_setting.now_profile = ret;
	}

	vbc_eq_try_apply();

	vbc_dbg("Leaving %s\n", __func__);
	return ret;
}

static int vbc_switch_get(void)
{
	int ret;
	ret = arch_audio_vbc_switch(AUDIO_NO_CHANGE);
	if (ret >= 0)
		return ((ret ==
		      AUDIO_TO_CP0_DSP_CTRL) ? 0 : ((ret ==
						     AUDIO_TO_CP1_DSP_CTRL) ? 1
						    : 2));

	return ret;
}

static int vbc_switch_put(int is_arm)
{
	int ret;
	pr_info("VBC switch to %s\n", is_arm ? "ARM" : "DSP");

	ret = is_arm;
	ret = arch_audio_vbc_switch(ret == 0 ?
				    AUDIO_TO_CP0_DSP_CTRL : ((ret == 1) ?
							     AUDIO_TO_CP1_DSP_CTRL
							     :
							     AUDIO_TO_AP_ARM_CTRL));

	vbc_dbg("Leaving %s\n", __func__);
	return ret;
}

static int vbc_eq_switch_get(void)
{
	return vbc_eq_setting.is_active;
}

static int vbc_eq_switch_put(int is_active)
{
	int ret;
	pr_info("VBC eq switch %s\n", is_active ? "ON" : "OFF");

	ret = is_active;
	if (ret == vbc_eq_setting.is_active) {
		return ret;
	}
	if ((ret == 0) || (ret == 1)) {
		vbc_eq_setting.is_active = ret;
		if (vbc_eq_setting.is_active) {
			vbc_eq_setting.vbc_eq_apply = vbc_eq_profile_apply;
			vbc_eq_try_apply();
		} else {
			vbc_eq_setting.vbc_eq_apply = 0;
			vbc_eq_profile_close();
		}
	}

	vbc_dbg("Leaving %s\n", __func__);
	return ret;
}

static int vbc_dg_get(int stream, int id)
{
	int vbc_idx = vbc_str_2_index(stream);
	return vbc[vbc_idx].dg_val[id];
}

static int vbc_dg_put(int stream, int id, int dg)
{
	int ret = 0;
	int vbc_idx = vbc_str_2_index(stream);

	pr_info("VBC %s%s DG set 0x%02x\n",
		(vbc_idx == 2) ? "ADC23" : (vbc_idx == 1 ? "ADC" : "DAC"),
		id == VBC_LEFT ? "L" : "R",	dg);

	ret = dg;
	if (ret == vbc[vbc_idx].dg_val[id]) {
		return ret;
	}
	if (ret <= VBC_DG_VAL_MAX) {
		vbc[vbc_idx].dg_val[id] = ret;
	}

	vbc_try_dg_set(vbc_idx, id);

	vbc_dbg("Leaving %s\n", __func__);
	return ret;
}

static int vbc_dg_switch_get(int stream, int id)
{
	int vbc_idx = vbc_str_2_index(stream);
	return vbc[vbc_idx].dg_switch[id];
}

static int vbc_dg_switch_put(int stream, int id, int enable)
{
	int ret = 0;
	int vbc_idx = vbc_str_2_index(stream);

	pr_info("VBC %s%s DG switch %s\n",
		(vbc_idx == 2) ? "ADC23" : (vbc_idx == 1 ? "ADC" : "DAC"),
		id == VBC_LEFT ? "L" : "R",
		enable ? "ON" : "OFF");

	ret = enable;
	if (ret == vbc[vbc_idx].dg_switch[id]) {
		return ret;
	}

	vbc[vbc_idx].dg_switch[id] = ret;

	vbc_try_dg_set(vbc_idx, id);

	vbc_dbg("Leaving %s\n", __func__);
	return ret;
}

int vbc_init(void)
{
	arch_audio_vbc_switch(AUDIO_TO_AP_ARM_CTRL);
	return 0;
}

void vbc_exit(void)
{
}

MODULE_DESCRIPTION("SPRD ASoC VBC CUP-DAI driver");
MODULE_AUTHOR("Zhenfang Wang <zhenfang.wang@spreadtrum.com>");
MODULE_LICENSE("GPL");
