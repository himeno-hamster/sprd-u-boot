#include <common.h>
#include <asm/io.h>
#include <asm/arch/adc_drvapi.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/sprd_reg.h>

int regulator_default_get(const char con_id[]);
void regulator_default_set(const char con_id[], int vol);
int regulator_default_set_all(void);
u32 regulator_get_calibration_mask(void);


#undef debug
#define debug0(format, arg...)
#define debug(format, arg...) printf("\t" format, ## arg)
#define debug2(format, arg...) printf("\t\t" format, ## arg)

/* abs() handles unsigned and signed longs, ints, shorts and chars.  For all input types abs()
 * returns a signed long.
 * abs() should not be used for 64-bit types (s64, u64, long long) - use abs64() for those.*/
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

/* On ARMv5 and above those functions can be implemented around the clz instruction for
 * much better code efficiency.		*/

static inline int fls(int x)
{
	int ret;

	asm("clz\t%0, %1": "=r"(ret):"r"(x));
	ret = 32 - ret;
	return ret;
}

#define __fls(x) (fls(x) - 1)
#define ffs(x) ({ unsigned long __t = (x); fls(__t & -__t); })
#define __ffs(x) (ffs(x) - 1)
#define ffz(x) __ffs( ~(x) )

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define MEASURE_TIMES				(15)

#define ADC_DROP_CNT	( DIV_ROUND(MEASURE_TIMES, 5) )
static int __average(int a[], int N)
{
	int i, sum = 0;
	for (i = 0; i < N; i++)
		sum += a[i];
	return DIV_ROUND(sum, N);
}

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
	int results[MEASURE_TIMES];

	if (-1 == ADC_GetValues(channel, ADC_SCALE_3V, MEASURE_TIMES, results)) {
		return 0;
	}

	bubble_sort(results, MEASURE_TIMES);

	/* dump results */
	for (i = 0; i < MEASURE_TIMES; i++) {
		printf("%d ", results[i]);
	}
	printf("\n");

	return __average(&results[ADC_DROP_CNT], MEASURE_TIMES - ADC_DROP_CNT * 2);
}

#define RATIO(_n_, _d_) (_n_ << 16 | _d_)

static int sci_adc_ratio(int channel, int mux)
{
	switch (channel) {
	case 0x05:		//vbat
		return RATIO(7, 29);
	case 0x0D:		//dcdccore
	case 0x0E:		//dcdcarm
		return RATIO(4, 5);
	case 0x0F:		//dcdcmem
		return RATIO(3, 5);
	case 0x10:		//dcdcgen
		return RATIO(4, 9);
	case 0x15:		//DCDC Supply LDO
		return RATIO(1, 2);
	case 0x13:		//DCDCVBATBK
	case 0x14:		//DCDCHEADMIC
	case 0x16:		//VBATD Domain LDO
	case 0x17:		//VBATA Domain LDO
	case 0x1E:		//DP from terminal
	case 0x1F:		//DM from terminal
		return RATIO(1, 3);

	default:
		return RATIO(1, 1);
	}
	return RATIO(1, 1);
}

static u32 bat_numerators, bat_denominators = 0;
extern uint16_t CHGMNG_AdcvalueToVoltage (uint16_t adcvalue);

int sci_adc_vol_request(int channel, int mux)
{
	int adc_res;
	adc_res = sci_adc_request(channel);
	if (adc_res) {
		u32 res, chan_numerators, chan_denominators;
		res = (u32) sci_adc_ratio(channel, mux);
		chan_numerators = res >> 16;
		chan_denominators = res & 0xffff;
		return CHGMNG_AdcvalueToVoltage(adc_res)
		    * (bat_numerators * chan_denominators)
		    / (bat_denominators * chan_numerators);
	}
	return 0;
}

/* Simple shorthand for a section definition */
#ifndef __section
# define __section(S) __attribute__ ((__section__(#S)))
#endif

#define __init0	__section(.rodata.regu.init0)
#define __init1	__section(.rodata.regu.init1)
#define __init2	__section(.rodata.regu.init2)

const u32 __init0 __init_begin = 0xeeeebbbb;
const u32 __init2 __init_end = 0xddddeeee;

struct regulator_regs {
	int typ;
	u32 pd_set, pd_set_bit;
	/* at new feature, some LDOs had only set, no rst bits.
	 * and DCDCs voltage and trimming controller is the same register */
	u32 pd_rst, pd_rst_bit;
	u32 slp_ctl, slp_ctl_bit;
	u32 vol_trm, vol_trm_bits;
	u32 cal_ctl, cal_ctl_bits;
	u32 vol_def;
	u32 vol_ctl, vol_ctl_bits;
	u32 vol_sel_cnt, vol_sel[];
};

struct regulator_desc {
	int id;
	const char *name;
	struct regulator_regs *regs;
};

#define REGU_VERIFY_DLY	(1000)	/*ms */
#define SCI_REGU_REG(VDD, TYP, PD_SET, SET_BIT, PD_RST, RST_BIT, SLP_CTL, SLP_CTL_BIT, \
                     VOL_TRM, VOL_TRM_BITS, CAL_CTL, CAL_CTL_BITS, VOL_DEF,	\
                     VOL_CTL, VOL_CTL_BITS, VOL_SEL_CNT, ...)					\
	static struct regulator_regs REGS_##VDD = {						\
		.typ = TYP,							\
		.pd_set = PD_SET, 					\
		.pd_set_bit = SET_BIT,					\
		.pd_rst = PD_RST,						\
		.pd_rst_bit = RST_BIT,					\
		.slp_ctl = SLP_CTL,						\
		.slp_ctl_bit = SLP_CTL_BIT,				\
		.vol_trm = VOL_TRM,					\
		.vol_trm_bits = VOL_TRM_BITS,			\
		.cal_ctl = CAL_CTL,					\
		.cal_ctl_bits = CAL_CTL_BITS,			\
		.vol_def = VOL_DEF,					\
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

#include <asm/arch/chip_x15/__regs_regulator_map.h>


/* standard dcdc ops*/
static int adjust_ldo_vol_base(struct regulator_desc *desc)
{
	struct regulator_regs *regs = desc->regs;

	if (regs->vol_sel_cnt == 2) {
		if ((0 == strcmp(desc->name, "vdd18"))
			|| (0 == strcmp(desc->name, "vddemmcio"))
			|| (0 == strcmp(desc->name, "vddcamd"))
			|| (0 == strcmp(desc->name, "vddcamio"))) {

			regs->vol_sel[0] = 1400;
		}
	}
	debug0("%s vol base %dmv\n", desc->name, regs->vol_sel[0]);
	return regs->vol_sel[0];
}

static int __dcdc_is_up_down_adjust(struct regulator_desc *desc)
{
	return ((0 == strcmp(desc->name, "dcdcmem")) ? 1 : 0);
}

static int dcdc_get_trimming_step(struct regulator_desc *desc, int to_vol)
{
	/* FIXME: vddmem step 200/32mV */
	return (!strcmp(desc->name, "dcdcmem") ) ? (1000 * 200 / 32) : (1000 * 100 / 32) /*uV */;
}

static int __match_dcdc_vol(struct regulator_desc *desc, u32 vol)
{
	int i, j = -1;
	int ds, min_ds = 100;	/* mV, the max range of small voltage */
	const struct regulator_regs *regs = desc->regs;

	for (i = 0; i < regs->vol_sel_cnt; i++) {
		ds = vol - regs->vol_sel[i];
		if (ds >= 0 && ds < min_ds) {
			min_ds = ds;
			j = i;
		}
	}

	if (j < 0) {
		for (i = 0; i < regs->vol_sel_cnt; i++) {
			ds = abs(vol - regs->vol_sel[i]);
			if (ds < min_ds) {
				min_ds = ds;
				j = i;
			}
		}
	}

	return j;
}

static int dcdc_get_voltage(struct regulator_desc *desc)
{
	const struct regulator_regs *regs = desc->regs;
	u32 mv;
	int cal = 0;		/* uV */
	int i, shft = __ffs(regs->vol_ctl_bits);

	i = (ANA_REG_GET(regs->vol_ctl) & regs->vol_ctl_bits) >> shft;
	mv = regs->vol_sel[i];

	cal = (ANA_REG_GET(regs->vol_trm) & regs->vol_trm_bits) >> __ffs(regs->vol_trm_bits);
	if (__dcdc_is_up_down_adjust(desc)) {
		cal -= 0x10;
	}
	cal *= dcdc_get_trimming_step(desc, 0);	/*uV */

	debug0("%s %d +%dmv\n", desc->name, mv, cal / 1000);
	return mv + cal / 1000;
}

static int dcdc_set_voltage(struct regulator_desc *desc, int min_mV, int max_mV)
{
	const struct regulator_regs *regs = desc->regs;
	int i, mv = min_mV;

	/* found the closely vol ctrl bits */
	i = __match_dcdc_vol(desc, mv);
	if (i < 0) {
		debug2("%s: %s failed to match voltage: %d\n",__func__,desc->name,mv);
		return -1;
	}

	/* dcdc calibration control bits (default 0) small adjust voltage: 100/32mv ~= 3.125mv */
	{
		int shft_ctl = __ffs(regs->vol_ctl_bits);
		int shft_trm = __ffs(regs->vol_trm_bits);
		int step = dcdc_get_trimming_step(desc, mv);
		int j = (int)(mv - (int)regs->vol_sel[i]) * 1000 / step;

		if (__dcdc_is_up_down_adjust(desc))
			j += 0x10;

		debug2("regu_dcdc %p (%s) %d = %d %+dmv (trim=%d step=%duv);\n", regs, desc->name,
			   mv, regs->vol_sel[i], mv - regs->vol_sel[i], j, step);

		if (j >= 0 && j <= (regs->vol_trm_bits >> shft_trm))
			ANA_REG_MSK_OR(regs->vol_ctl, (j << shft_trm) | (i << shft_ctl),
				regs->vol_trm_bits | regs->vol_ctl_bits);
	}
	return 0;
}

static int dcdc_set_trimming(struct regulator_desc *desc, int def_vol, int to_vol, int adc_vol)
{
	//int acc_vol = dcdc_get_trimming_step(desc, to_vol) / 1000;
	int ctl_vol = (def_vol - (adc_vol - to_vol));

	return dcdc_set_voltage(desc, ctl_vol, ctl_vol);
}

static int ldo_get_voltage(struct regulator_desc *desc)
{
	const struct regulator_regs *regs = desc->regs;
	u32 vol;

	if (regs->vol_trm && regs->vol_sel_cnt == 2) {
		int shft = __ffs(regs->vol_trm_bits);
		u32 trim =
		    (ANA_REG_GET(regs->vol_trm) & regs->vol_trm_bits) >> shft;
		vol = regs->vol_sel[0] * 1000 + trim * regs->vol_sel[1];
		vol /= 1000;
		debug0("%s voltage %dmv\n", desc->name, vol);
		return vol;
	}

	return -1;
}

static int ldo_set_trimming(struct regulator_desc *desc, int def_vol, int to_vol, int adc_vol)
{
	const struct regulator_regs *regs = desc->regs;
	int ret = -1;

	if (!regs->vol_ctl && regs->vol_sel_cnt == 2) {
		/* ctl_vol = vol_base + reg[vol_trm] * vol_step  */
		int shft = __ffs(regs->vol_trm_bits);
		int ctl_vol = (def_vol - (adc_vol - to_vol));	//same as dcdc?
		u32 trim = 0;
		if(adc_vol > to_vol)
			trim = DIV_ROUND_UP((ctl_vol - regs->vol_sel[0]) * 1000, regs->vol_sel[1]);
		else
			trim = ((ctl_vol - regs->vol_sel[0]) * 1000 / regs->vol_sel[1]);

		debug2("regu_ldo %p (%s) %d = %d %+dmv (trim=%d step=%duv vol_base=%dmv)\n", regs, desc->name,
			ctl_vol, regs->vol_sel[0], ctl_vol - regs->vol_sel[0], trim, regs->vol_sel[1], regs->vol_sel[0]);

		if ((trim >= 0) && (trim <= (regs->vol_trm_bits >> shft))) {
			ANA_REG_MSK_OR(regs->vol_trm,
					trim << shft,
					regs->vol_trm_bits);
			ret = 0;
		}
	}

	return ret;
}

static int DCDC_Cal_One(struct regulator_desc *desc, int is_cal)
{
	struct regulator_regs *regs = desc->regs;
	int def_vol = 0, to_vol = 0;
	int adc_vol = 0, cal_vol = 0;
	int ret = -1, adc_chan = regs->cal_ctl_bits >> 16;
	u16 ldo_cal_sel = regs->cal_ctl_bits & 0xFFFF;

	if (!adc_chan || !regs->vol_def)
		return -1;

	if(0x2711A000 == ANA_GET_CHIP_ID()) {
		if (desc->regs->typ == 0) {
			adjust_ldo_vol_base(desc);
		}
	}

	if (ldo_cal_sel)
		ANA_REG_OR(regs->cal_ctl, ldo_cal_sel);

	/*
	 * FIXME: force get dcdc&ldo voltage from ana global regs
	 * and get ideal voltage from vol para.
	 */
	if (desc->regs->typ == 2 /*DCDC*/) {
		def_vol = dcdc_get_voltage(desc);
	}
	else if (desc->regs->typ == 0 /*LDO*/) {
		def_vol = ldo_get_voltage(desc);
	}

	to_vol = regulator_default_get(desc->name);
	if (!to_vol)
		to_vol = regs->vol_def;

	adc_vol = sci_adc_vol_request(adc_chan, ldo_cal_sel);
	if (adc_vol <= 0) {
		debug("%s default %dmv, adc channel %d, maybe not enable\n", desc->name, def_vol, adc_chan);
		goto exit;
	}

	cal_vol = abs(adc_vol - to_vol);
	debug("%s default %dmv, from %dmv to %dmv, bias %c%d.%03d%%\n",
	      desc->name, def_vol, adc_vol, to_vol,
	      (adc_vol > to_vol) ? '+' : '-',
	      cal_vol * 100 / to_vol, cal_vol * 100 * 1000 / to_vol % 1000);

	if (!def_vol || !to_vol || adc_vol <= 0)
		goto exit;
	if (abs(adc_vol - def_vol) >= def_vol / 9)	/* adjust limit 9% */
		goto exit;
	else if (cal_vol < to_vol / 100) {	/* bias 1% */
		goto exit;
	}

	if (is_cal) {
		if (regs->typ == 2/*VDD_TYP_DCDC*/)
			ret = dcdc_set_trimming(desc, def_vol, to_vol, adc_vol);
		else if (regs->typ == 0/*VDD_TYP_LDO*/)
			ret = ldo_set_trimming(desc, def_vol, to_vol, adc_vol);

		if(ret < 0)
			regulator_default_set(desc->name, 0);
	}

exit:
	if (ldo_cal_sel)
		ANA_REG_BIC(regs->cal_ctl, ldo_cal_sel);

	return ret;
}

int DCDC_Cal_ArmCore(void)
{
	u16 regval_dcdc_store, regval_ldo_store;
	u32 res;
	struct regulator_desc *desc = NULL;
	struct regulator_desc *desc_end = NULL;
	u32 cali_mask = regulator_get_calibration_mask();
	u32 chip_id = ANA_GET_CHIP_ID();

	printf("%s; adie chip id 0x%08x\n", __FUNCTION__, chip_id);

	regval_dcdc_store = ANA_REG_GET(ANA_REG_GLB_LDO_DCDC_PD) & 0xFFFF;
	ANA_REG_MSK_OR(ANA_REG_GLB_PWR_WR_PROT_VALUE, BITS_PWR_WR_PROT_VALUE(0x6e7f), 0x7FFF);
	ANA_REG_BIC(ANA_REG_GLB_LDO_DCDC_PD, (cali_mask >> 16));
	ANA_REG_MSK_OR(ANA_REG_GLB_PWR_WR_PROT_VALUE, 0, 0x7FFF);

	regval_ldo_store = ANA_REG_GET(ANA_REG_GLB_LDO_PD_CTRL) & 0xFFFF;
	ANA_REG_BIC(ANA_REG_GLB_LDO_PD_CTRL, cali_mask & 0xFFFF);

	if(0x2711A000 == chip_id) {
		//FIXME: vddcamio/vddcamd/vddemmcio/vdd18 real voltage value is greater than design value
		ANA_REG_MSK_OR(ANA_REG_GLB_LDO_V_CTRL9, BITS_LDO_VDD18_V(0x40),
				BITS_LDO_VDD18_V(-1)); //0x68D2 -->0x40D2
		ANA_REG_MSK_OR(ANA_REG_GLB_LDO_V_CTRL2, BITS_LDO_EMMCIO_V(0x40),
				BITS_LDO_EMMCIO_V(-1));  //0x3C68 -->0x3C40
		ANA_REG_MSK_OR(ANA_REG_GLB_LDO_V_CTRL1, BITS_LDO_CAMIO_V(0x40) | BITS_LDO_CAMD_V(0x10),
				BITS_LDO_CAMIO_V(-1) | BITS_LDO_CAMD_V(-1)); //0x6838 -->0x4010
		udelay(200 * 1000); //wait 200ms
	}

	/* FIXME: Update CHGMNG_AdcvalueToVoltage table before setup vbat ratio. */
	/*ADC_CHANNEL_VBAT is 5*/
	res = (u32) sci_adc_ratio(5, 0);
	bat_numerators = res >> 16;
	bat_denominators = res & 0xffff;


	/* TODO: calibrate all DCDCs */
	desc = (struct regulator_desc *)(&__init_begin + 1);

	printf("%p (%x) -- %p -- %p (%x)\n", &__init_begin, __init_begin,
		desc, &__init_end, __init_end);

	desc_end = (struct regulator_desc *)&__init_end;
	while (--desc_end >= desc) { /* reverse order */
		printf("\nCalibrate %s ...\n", desc_end->name);
		DCDC_Cal_One(desc_end, 1);
	}

	/* wait a moment for LDOs ready */
	udelay(200 * 1000);

	/* TODO: verify all DCDCs */
	desc = (struct regulator_desc *)(&__init_begin + 1);
	desc_end = (struct regulator_desc *)&__init_end;
	while (--desc_end >= desc) { /* reverse order */
		printf("\nVerify %s ...\n", desc_end->name);
		DCDC_Cal_One(desc_end, 0);
	}

	/* restore adie dcdc/ldo PD bits */
	ANA_REG_SET(ANA_REG_GLB_LDO_PD_CTRL, regval_ldo_store);
	ANA_REG_MSK_OR(ANA_REG_GLB_PWR_WR_PROT_VALUE, BITS_PWR_WR_PROT_VALUE(0x6e7f), 0x7FFF);
	ANA_REG_SET(ANA_REG_GLB_LDO_DCDC_PD, regval_dcdc_store);
	ANA_REG_MSK_OR(ANA_REG_GLB_PWR_WR_PROT_VALUE, 0, 0x7FFF);

	return 0;
}


int regulator_init(void)
{
	/*
	 * FIXME: turn on all DCDC/LDOs if need
	 */
	return 0;
}

struct regulator_desc *regulator_get(void/*struct device*/ *dev, const char *id)
{
	struct regulator_desc *desc =
		(struct regulator_desc *)(&__init_begin + 1);
	while (desc < (struct regulator_desc *)&__init_end) {
		if (0 == strcmp(desc->name, id))
			return desc;
		desc++;
	}
	return 0;
}

int regulator_disable_all(void)
{
	ANA_REG_OR(ANA_REG_GLB_LDO_PD_CTRL, 0x7ff);
	ANA_REG_OR(ANA_REG_GLB_LDO_DCDC_PD, 0x1fff);
}

int regulator_enable_all(void)
{
	ANA_REG_BIC(ANA_REG_GLB_LDO_DCDC_PD, 0x1fff);
	ANA_REG_BIC(ANA_REG_GLB_LDO_PD_CTRL, 0x7ff);
}

int regulator_disable(const char con_id[])
{
	struct regulator_desc *desc = regulator_get(0, con_id);
	if (desc) {
		struct regulator_regs *regs = desc->regs;
		ANA_REG_OR(regs->pd_set, regs->pd_set_bit);
	}
	return 0;
}

int regulator_enable(const char con_id[])
{
	struct regulator_desc *desc = regulator_get(0, con_id);
	if (desc) {
		struct regulator_regs *regs = desc->regs;
		ANA_REG_BIC(regs->pd_set, regs->pd_set_bit);
	}
	return 0;
}

int regulator_set_voltage(const char con_id[], int to_vol)
{
	int ret = 0;
	struct regulator_desc *desc = regulator_get(0, con_id);
	if (desc) {
		struct regulator_regs *regs = desc->regs;
		if (regs->typ == 2/*VDD_TYP_DCDC*/)
			ret = dcdc_set_voltage(desc, to_vol, 0);
		else if (regs->typ == 0/*VDD_TYP_LDO*/)
			ret = ldo_set_trimming(desc, 0, to_vol, 0);
	}
	return ret;
}

typedef struct {
	uint16 ideal_vol;
	const char name[14];
}vol_para_t;

vol_para_t **ppvol_para = 0x50005c20;

static int get_vol_para_num(void)
{
	int i = 0;

	if (!(ppvol_para && *ppvol_para))
		return 0;

	if(strcmp((*ppvol_para)[0].name, "volpara_begin") || (0xfaed != (*ppvol_para)[0].ideal_vol))
		return 0;

	while(0 != strcmp((*ppvol_para)[i++].name, "volpara_end"))
		;

	return (i+1);
}

static vol_para_t * match_vol_para(const char* vol_name)
{
	int i = 0;

	BUG_ON(NULL == vol_name);

	if (!(ppvol_para && *ppvol_para))
		return NULL;

	if(strcmp((*ppvol_para)[0].name, "volpara_begin") || (0xfaed != (*ppvol_para)[0].ideal_vol))
		return NULL;

	while(0 != strcmp((*ppvol_para)[i++].name, "volpara_end")) {
		if (0 == strcmp((*ppvol_para)[i].name, vol_name)) {
			debug("%s name %s, ideal_vol %d\n", __func__, (*ppvol_para)[i].name, (*ppvol_para)[i].ideal_vol);
			return (vol_para_t*)(&(*ppvol_para)[i]);
		}
	}

	return NULL;
}

int regulator_default_get(const char con_id[])
{
	vol_para_t * pvol_para = match_vol_para(con_id);

	return (int)(pvol_para ? pvol_para->ideal_vol : 0);
}

void regulator_default_set(const char con_id[], int vol)
{
	vol_para_t * pvol_para = match_vol_para(con_id);

	if(pvol_para) {
		pvol_para->ideal_vol = vol;
	}
}

int regulator_default_set_all(void)
{
	int i = 0, ret = 0;

	//dump & check all vol para
	if (!(ppvol_para && *ppvol_para))
		return -1;

	if(strcmp((*ppvol_para)[0].name, "volpara_begin") || (0xfaed != (*ppvol_para)[0].ideal_vol))
		return 0;

	while(0 != strcmp((*ppvol_para)[i++].name, "volpara_end")) {
		debug("regu: [%d] %s : %d\n", i, (*ppvol_para)[i].name, (*ppvol_para)[i].ideal_vol);

		ret |= regulator_set_voltage((*ppvol_para)[i].name, (*ppvol_para)[i].ideal_vol);
	}

	return ret;
}

/********************************************************************
*
* regulator_get_calibration_mask - get dcdc/ldo calibration flag
*
//
//High 16bit: dcdc ctrl calibration flag
//
* bit[13] ~ bit[15] : reserved
* bit[12] : dcdcgen
* bit[11] : dcdcmem
* bit[10] : dcdcarm
* bit[9]   : dcdccore
* bit[8]   : vddrf0
* bit[7]   : vddemmccore
* bit[6]   : vddemmcio
* bit[5]   : vdddcxo
* bit[4]   : vddcon
* bit[3]   : vdd25
* bit[2]   : vdd28
* bit[1]   : vdd18
* bit[0]   : vddbg

//
//Low 16bit: ldo ctrl calibration flag
//
* bit[12] ~ bit[15] : reserved
* bit[11] : vddlpref
* bit[10] : dcdcwpa
* bit[9]   : vddclsg
* bit[8]   : vddusb
* bit[7]   : vddcammot
* bit[6]   : vddcamio
* bit[5]   : vddcamd
* bit[4]   : vddcama
* bit[3]   : vddsim2
* bit[2]   : vddsim1
* bit[1]   : vddsim0
* bit[0]   : vddsd
********************************************************************/
u32 regulator_get_calibration_mask(void)
{
	int len = get_vol_para_num();
	volatile vol_para_t *pvol_para = (volatile vol_para_t *)(*ppvol_para);
	volatile u32* pdebug_flag = (u32*)(&pvol_para[len-1]);

	if(len > 2) {
		printf("%s, vol_para_tbl_len %d, ldo_pd_mask 0x%08x; \n", __func__, len, *pdebug_flag);
		return (*pdebug_flag);
	}
}

