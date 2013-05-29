#include <common.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_ana.h>
#include <asm/arch/adc_drvapi.h>
#include <asm/arch/adi_hal_internal.h>
#include <asm/arch/adc_reg_v5.h>

#define pr_err(fmt...) printf(fmt)
#define pr_warning(fmt...) printf(fmt)

#define ADC_CHANNEL_INVALID	-1

struct adc_sample_data {
	int sample_num;		/* from 1 to 15 */
	int sample_bits;	/*0: 10bits mode, 1:12 bits mode */
	int signal_mode;	/*0:resistance,1:capacitance */
	int sample_speed;	/*0:quick mode, 1: slow mode */
	int scale;		/*0:little scale, 1:big scale */
	int hw_channel_delay;	/*0:disable, 1:enable */
	int channel_id;		/*channel id of software, Or dedicatid hw channel number */
	int channel_type;	/*0: software, 1: slow hardware , 2: fast hardware */
	int *pbuf;
};

void ADC_Init(void)
{
    ANA_REG_OR(ANA_REG_GLB_ANA_APB_CLK_EN,
		    BIT_ANA_ADC_EB | BIT_ANA_CLK_AUXADC_EN | BIT_ANA_CLK_AUXAD_EN);

#ifdef CONFIG_SC7710G2
	ADI_ClkAlwaysOn(1);

	ANA_REG_OR((ADC_REG_BASE + ADC_CTL), BIT_ADC_EN);
#endif
}

static int sci_adc_config(struct adc_sample_data *adc)
{
	unsigned addr = 0;
	unsigned val = 0;
	int ret = 0;

	BUG_ON(!adc);
	BUG_ON(adc->channel_id > ADC_MAX);

	val = BIT_CH_IN_MODE(adc->signal_mode);
	val |= BIT_CH_SLOW(adc->sample_speed);
	val |= BIT_CH_SCALE(adc->scale);
	val |= BIT_CH_ID(adc->channel_id);
	val |= BIT_CH_DLY_EN(adc->hw_channel_delay ? 1 : 0);

	adc_write(val, ADC_REG_BASE + ADC_SW_CH_CFG);

	if (adc->channel_type > 0) {	/*hardware */
		adc_write(BIT_HW_CH_DELAY(adc->hw_channel_delay),
			  ADC_REG_BASE + ADC_HW_CH_DELAY);

		if (adc->channel_type == 1) {	/*slow */
			addr = ADC_REG_BASE + ADC_SLOW_HW_CHX_CFG(adc->channel_id);
		} else {
			addr = ADC_REG_BASE + ADC_FAST_HW_CHX_CFG(adc->channel_id);
		}
		adc_write(val, addr);
	}

	return ret;
}

static int sci_adc_get_values(struct adc_sample_data *adc)
{
	unsigned long flags, hw_flags;
	int cnt = 12;
	unsigned addr = 0;
	unsigned val = 0;
	int ret = 0;
	int num = 0;
	int sample_bits_msk = 0;
	int *pbuf = 0;

	if (!adc || adc->channel_id > ADC_MAX)
		return -1;

	pbuf = adc->pbuf;
	if (!pbuf)
		return -1;

	num = adc->sample_num;
	if (num > ADC_MAX_SAMPLE_NUM)
		return -1;

	sci_adc_config(adc);	//configs adc sample.

	addr = ADC_REG_BASE + ADC_CTL;
	val = adc_read(addr);
	val &= ~(BIT_SW_CH_ON | BIT_ADC_BIT_MODE_MASK);
	adc_write(val, addr);

	adc_clear_irq();

	val = BIT_SW_CH_RUN_NUM(num);
	val |= BIT_ADC_BIT_MODE(adc->sample_bits);
	val |= BIT_SW_CH_ON;

	adc_write((adc_read(addr)|val), addr);

	while ((!adc_raw_irqstatus()) && cnt--) {
		udelay(50);
	}

	if (!cnt) {
		ret = -1;
		goto Exit;
	}

	adc_clear_irq();

	if (adc->sample_bits)
		sample_bits_msk = ((1 << 12) - 1);	//12
	else
		sample_bits_msk = ((1 << 10) - 1);	//10

	while (num--)
		*pbuf++ = adc_get_data(sample_bits_msk);

	return ret;

Exit:
	val = adc_read(addr);
	val &= ~BIT_SW_CH_ON;
	adc_write(val, addr);

	return ret;
}


int32_t ADC_GetValue(adc_channel id, bool scale)
{
	struct adc_sample_data adc;
	int32_t result[1];

	adc.channel_id = id;
	adc.channel_type = 0;
	adc.hw_channel_delay = 0;
	adc.pbuf = &result[0];
	adc.sample_bits = 1;    /*use 12 bits mode*/
	adc.sample_num = 1;
	adc.sample_speed = 0;
	adc.scale = scale;
	adc.signal_mode = 0;

	if (0 != sci_adc_get_values(&adc)) {
		pr_err("sci_adc_get_value, return error\n");
		BUG();
	}

	return result[0];

}
