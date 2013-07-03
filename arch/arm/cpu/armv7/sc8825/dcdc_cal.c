#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/adc_drvapi.h>
#include <asm/arch/regs_glb.h>
#include <asm/arch/regs_ana_glb.h>
#include <asm/arch/adi.h>

#define abs(x) ({									\
				long ret;							\
				if (sizeof(x) == sizeof(long)) {		\
					long __x = (x);				\
					ret = (__x < 0) ? -__x : __x;		\
				} else {							\
					int __x = (x);					\
					ret = (__x < 0) ? -__x : __x;		\
				}								\
				ret;								\
			})
#undef ffs
#undef fls

static inline int fls(int x)
{
	int ret;

	asm("clz\t%0, %1": "=r"(ret):"r"(x));
	ret = 32 - ret;
	return ret;
}
#define __fls(x)							(fls(x) - 1)
#define ffs(x)								({ unsigned long __t = (x); fls(__t & -__t); })
#define __ffs(x)							(ffs(x) - 1)
#define ffz(x)								__ffs( ~(x) )

#define MEASURE_TIMES					(15)

static unsigned int  bat_numerators, bat_denominators = 0;
extern uint16_t CHGMNG_AdcvalueToVoltage (uint16_t adcvalue);
extern void udelay(unsigned long usec);

/*********************     Get vlaue from adc     *****************************/
#define MEASURE_TIMES					(15)
#define RATIO(_n_, _d_)					(_n_ << 16 | _d_)

static void bubble_sort(int a[], int N)
{
	int i, j, t;
	for (i = 0; i < N - 1; i++) {
		for (j = i + 1; j < N; j++) {
			if (a[i] > a[j]) {
				t = a[i];
				a[i] = a[j];
				a[j] = t;
			}
		}
	}
}

int sci_adc_request(int channel)
{
	int i;
	static int results[MEASURE_TIMES];

	if (-1 == ADC_GetValues(channel, false, MEASURE_TIMES, results)) {
		return 0;
	}

	bubble_sort(results, MEASURE_TIMES);

	/* dump results */
	//printf("channel is %d:\r\n",channel);
	for (i = 0; i < MEASURE_TIMES; i++) {
		printf("%d ", results[i]);
	}
	printf("\r\n");

	return results[MEASURE_TIMES / 2];
}

int sci_is_match_chip_id(u32 id)
{
	static u32 metalfix = 0;
	if (!metalfix)
		metalfix = sci_adi_read(ANA_REG_GLB_CHIP_ID_LOW) |
		    (sci_adi_read(ANA_REG_GLB_CHIP_ID_HIGH) << 16);
	return id == metalfix;
}

int sci_adc_ratio(int channel)
{
	switch (channel) {
	case ADC_CHANNEL_VBAT:
		if (sci_is_match_chip_id(0x8820A001 /*2712AD */ ))
			return RATIO(7, 29);
		else
			return RATIO(8, 30);
	case ADC_CHANNEL_DCDCCORE:
	case ADC_CHANNEL_DCDCARM:
		return RATIO(4, 5);
	case ADC_CHANNEL_DCDCMEM:
		return RATIO(3, 5);
	case ADC_CHANNEL_DCDCLDO:
		return RATIO(4, 9);
	case ADC_CHANNEL_LDO0:
	case ADC_CHANNEL_LDO1:
		return RATIO(1, 3);
	case ADC_CHANNEL_LDO2:
		return RATIO(1, 2);
	default:
		return RATIO(1, 1);
	}
	return RATIO(1, 1);
}

int sci_adc_vol_request(int channel)
{
	int adc_res;
	adc_res = sci_adc_request(channel);
	if (adc_res) {
		u32 res, chan_numerators, chan_denominators;
		res = (u32) sci_adc_ratio(channel);
		chan_numerators = res >> 16;
		chan_denominators = res & 0xffff;
		return CHGMNG_AdcvalueToVoltage(adc_res)
		    * (bat_numerators * chan_denominators)
		    / (bat_denominators * chan_numerators);
	}
	return 0;
}
/*********************      Ldo      ***************************************/
#ifndef __section
#define __section(S) __attribute__ ((__section__(#S)))
#endif

#define __init0	__section(.data.regu.init0)
#define __init1	__section(.data.regu.init1)
#define __init2	__section(.data.regu.init2)

const u32 __init0 __init_begin = 0xeeeebbbb;
const u32 __init2 __init_end = 0xddddeeee;

struct regulator_regs {
	int typ;
	u32 pd_set, pd_set_bit;
	u32 pd_rst, pd_rst_bit;
	u32 slp_ctl, slp_ctl_bit;
	u32 vol_trm, vol_trm_bits;
	u32 vol_ctl, vol_ctl_bits;
	u32 vol_sel_cnt, vol_sel[];
};

struct regulator_desc {
	int id;
	const char *name;
	const struct regulator_regs *regs;
};

#define SCI_REGU_REG(VDD, TYP, PD_SET, SET_BIT, PD_RST, RST_BIT, SLP_CTL, SLP_CTL_BIT, \
                     VOL_TRM, VOL_TRM_BITS, VOL_CTL, VOL_CTL_BITS, VOL_SEL_CNT, ...)	\
	static const struct regulator_regs REGS_##VDD = {								\
		.typ = TYP,							\
		.pd_set = PD_SET,						\
		.pd_set_bit = SET_BIT,					\
		.pd_rst = PD_RST,						\
		.pd_rst_bit = RST_BIT,					\
		.slp_ctl = SLP_CTL,						\
		.slp_ctl_bit = SLP_CTL_BIT,				\
		.vol_trm = VOL_TRM,					\
		.vol_trm_bits = VOL_TRM_BITS,			\
		.vol_ctl = VOL_CTL,					\
		.vol_ctl_bits = VOL_CTL_BITS,			\
		.vol_sel_cnt = VOL_SEL_CNT,			\
		.vol_sel = {__VA_ARGS__},				\
	};										\
	struct regulator_desc __init1 DESC_##VDD = {	\
		.id = -1,								\
		.name = #VDD,						\
		.regs = &REGS_##VDD,					\
	};										\

#include "__sc8825_regulator_map.h"

int sci_ldo_turn_on(struct regulator_desc *desc)
{
	const struct regulator_regs *regs = desc->regs;
	if (!regs->pd_rst || !regs->pd_set)
		return -1;
	sci_adi_set(regs->pd_rst, regs->pd_rst_bit);
	sci_adi_clr(regs->pd_set, regs->pd_set_bit);
	return 0;
}

void *sci_ldo_request(const char *name)
{
	struct regulator_desc *desc = (struct regulator_desc *)(&__init_begin + 1);
	while (desc < (struct regulator_desc *)&__init_end) {
		if (0 == strcmp(name, desc->name)) {
			return desc;
		}
		desc++;
	}
	return (void *)-1;
}

static int __match_dcdc_vol(const struct regulator_regs *regs, u32 vol)
{
	int i, j = -1;
	int ds, min_ds = 100;	/* mV, the max range of small voltage */
	for (i = 0; i < regs->vol_sel_cnt; i++) {
		ds = vol - regs->vol_sel[i];
		if (ds >= 0 && ds < min_ds) {
			min_ds = ds;
			j = i;
		}
	}
	return j;
}

int dcdc_set_voltage(struct regulator_desc *desc, int ctl_vol, int to_vol)
{
	const struct regulator_regs *regs = desc->regs;
	int mv = ctl_vol;
	int i, shft = __ffs(regs->vol_ctl_bits);
	int max = regs->vol_ctl_bits >> shft;
	u32 trim;

	if (!regs->vol_ctl || !regs->vol_trm)
		return -1;

	trim = (sci_adi_read(regs->vol_trm) & regs->vol_trm_bits) >> 0;
	if (trim != 0) {
		printf("already set trimming\n");
		return 0;
	}

	/* found the closely vol ctrl bits */
	i = __match_dcdc_vol(regs, mv);
	if (i < 0)
		return -1;

	printf("dcdc (%s) %d = %d %+dmv\n", desc->name, 
		mv, regs->vol_sel[i], mv - regs->vol_sel[i]);
	{
		/* dcdc calibration control bits (default 00000),
		* small adjust voltage: 100/32mv ~= 3.125mv */
		int j = DIV_ROUND((mv - regs->vol_sel[i]) * 32, 100) % 32;
		sci_adi_write(regs->vol_trm,
			(BITS_DCDC_CAL(j) | (BITS_DCDC_CAL_RST(BITS_DCDC_CAL(-1) - j))), -1);
	}

	sci_adi_write(regs->vol_ctl, i | (max - i) << 4, -1);
	return 0;
}

int sci_ldo_trimming(struct regulator_desc *desc, int ctl_vol, int to_vol)
{
	int cal_vol = ctl_vol - to_vol * 90 / 100;	/* cal range 90% ~ 110% */
	const struct regulator_regs *regs = desc->regs;
	int shft = __ffs(regs->vol_trm_bits);
	u32 trim;

	if (2 /*DCDC*/ == desc->regs->typ)
		return dcdc_set_voltage(desc, ctl_vol, to_vol);

	trim = (sci_adi_read(regs->vol_trm) &  regs->vol_trm_bits) >> shft;
	if (trim != 0x10 /* 100 % */ ) {
		printf("already set trimming\n");
		goto exit;
	}

	/* assert 5 valid trim bits */
	trim = DIV_ROUND(cal_vol * 100 * 32, to_vol * 20) & 0x1f;
	printf("ldo (%s) trimming %u = %u %+dmv, got [%02X] %u.%03u%%\n",
		desc->name, ctl_vol, to_vol, (cal_vol - to_vol / 10), trim, ctl_vol * 100 / to_vol,
		(ctl_vol * 100 * 1000 / to_vol) % 1000);

	if (trim != 0x10 /* 100 % */ ) {
		sci_adi_write(regs->vol_trm, trim << __ffs(regs->vol_trm_bits), regs->vol_trm_bits);
	}
exit:
	return 0;
}

int sci_ldo_calibrate(struct regulator_desc *desc, int adc_chan, int def_vol, int to_vol, int is_cal)
{
	int ret, cal_vol, ctl_vol, adc_vol;
	adc_vol = sci_adc_vol_request(adc_chan);
	cal_vol = abs(adc_vol - to_vol);

	printf("%s default %dmv, from %dmv to %dmv, %c%d.%02d%%, vbat %d\n",
	       __FUNCTION__, def_vol, adc_vol, to_vol,
	       (adc_vol > to_vol) ? '+' : '-', cal_vol * 100 / to_vol,
	       cal_vol * 100 * 100 / to_vol % 100, sci_adc_vol_request(ADC_CHANNEL_VBAT));

	if (adc_vol && def_vol && to_vol) {
		cal_vol = abs(adc_vol - to_vol);
		if (cal_vol > to_vol / 10)	/* adjust limit 10% */
			goto exit;
		else if (!is_cal && (cal_vol < to_vol / 100 || 
							(adc_vol > to_vol && cal_vol < to_vol * 15 / 1000))) {
			/* margin 1% ~ 1.5% */
			printf("%s %s is ok\n\n", __FUNCTION__, desc->name);
			return 0;
		} else if (!is_cal)
			goto exit;

		/* always set valid vol ctrl and trim bits */
		ctl_vol = DIV_ROUND(def_vol * to_vol, adc_vol);

		ret = sci_ldo_trimming(desc, ctl_vol, to_vol);
		if (ret == 0) {
			udelay(10 * 1000);	/* wait a moment before cal verify */
			sci_ldo_calibrate(desc, adc_chan, ctl_vol, to_vol, 0);
		}

		return ctl_vol;
	}

exit:
	printf("%s %s failure\n", __FUNCTION__, desc->name);
	return -1;
}
/*******************      Calibration      ************************************/
struct ldo_map {
	const char *name;	/* a-die DCDC or LDO name */
	int def_on;		/* 1: default ON, 0: default OFF */
	u32 def_vol;		/* default voltage (mV), could not read from a-die */
	u32 cal_sel;		/* only one ldo cal can be enable at the same time */
	int adc_chan;		/* multiplexed adc-channel id for LDOs */
};
#define BITS_LDO_VSIM_CAL_EN(_x_)				( (_x_) & (BIT(3)|BIT(5)) )
struct ldo_map ldo_map[] = {
	{"vddcore", 1, 1100, 0, ADC_CHANNEL_DCDCCORE},
	{"vddarm", 1, 1200, 0, ADC_CHANNEL_DCDCARM},
	{"vddmem", 1, 1200, 0, ADC_CHANNEL_DCDCMEM},	/*DDR2 */
//      {"vddmem1", 0, 1800, 0, ADC_CHANNEL_DCDCMEM},       /*DDR1 */
	{"vddsim0", 0, 1800, BITS_LDO_VSIM_CAL_EN(-1), ADC_CHANNEL_LDO1},
};

int DCDC_Cal_ArmCore(void)
{
	int i, ret;

	ADC_Init();

	ret = (u32) sci_adc_ratio(ADC_CHANNEL_VBAT);
	bat_numerators = ret >> 16;
	bat_denominators = ret & 0xffff;
	
	/* four DCDCs and all LDOs Trimming if default ON or not */
	for (i = 0; i < (sizeof(ldo_map) / sizeof(ldo_map[0])); i++) {
		int adc_chan = ldo_map[i].adc_chan;
		int def_vol = ldo_map[i].def_vol;
		const char *name = ldo_map[i].name;
		struct regulator_desc *desc = sci_ldo_request(name);

		/* turn on ldo at first */
		if (ldo_map[i].def_on) {
			printf("regu (%s) default on\n", name);
		} else {
			ret = sci_ldo_turn_on(desc);
			printf("regu (%s) turn on\n", name);
		}

		/* enable ldo cal before adc sampling and ldo calibration */
		if (0 != ldo_map[i].cal_sel) {
			sci_adi_write((ANA_REGS_GLB2_BASE + 0x24),
				      ldo_map[i].cal_sel, -1);
		}

		/* calibrate ldo throught adc channel feedback */
		ret = sci_ldo_calibrate(desc, adc_chan, def_vol, def_vol, 1);

		/* close ldo cal */
		if (0 != ldo_map[i].cal_sel) {
			sci_adi_write((ANA_REGS_GLB2_BASE + 0x24), 0, -1);
		}
	}
	return 0;
}

