/******************************************************************************
 ** File Name:        adc_reg_v3.h                                            *
 ** Author:           Ryan.Liao                                               *
 ** DATE:             09/16/2009                                              *
 ** Copyright:        2009 Spreatrum, Incoporated. All Rights Reserved.       *
 ** Description:                                                              *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE            NAME            DESCRIPTION                               *
 ** 09/16/2009    Ryan.Liao           Created for SC7710G2                    *
 ******************************************************************************/

#ifndef _ADC_REG_V5_H_
#define _ADC_REG_V5_H_
/*----------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **------------------------------------------------------------------------- */

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include <asm/arch/sc8810_reg_base.h>

#define ADC_REG_BASE    (ADC_BASE)

#define adc_write(val,reg) \
	do { \
		ANA_REG_SET((u32)reg,val); \
} while (0)
static unsigned adc_read(unsigned addr)
{
	return ANA_REG_GET(addr);
}

#define ADC_CTL		(0x00)
#define ADC_SW_CH_CFG		(0x04)
#define ADC_FAST_HW_CHX_CFG(_X_)		((_X_) * 0x4 + 0x8)
#define ADC_SLOW_HW_CHX_CFG(_X_)		((_X_) * 0x4 + 0x28)
#define ADC_HW_CH_DELAY		(0x48)

#define ADC_DAT		(0x4c)
#define adc_get_data(_SAMPLE_BITS_)		(adc_read(ADC_REG_BASE + ADC_DAT) & (_SAMPLE_BITS_))

#define ADC_IRQ_EN		(0x50)
#define adc_enable_irq(_X_)	do {adc_write(((_X_) & 0x1),ADC_REG_BASE + ADC_IRQ_EN);} while(0)

#define ADC_IRQ_CLR		(0x54)
#define adc_clear_irq()		do {adc_write(0x1, ADC_REG_BASE + ADC_IRQ_CLR);} while (0)

#define ADC_IRQ_STS		(0x58)
#define adc_mask_irqstatus()     adc_read(ADC_REG_BASE + ADC_IRQ_STS)

#define ADC_IRQ_RAW		(0x5c)
#define adc_raw_irqstatus()     adc_read(ADC_REG_BASE + ADC_IRQ_RAW)

#define ADC_DEBUG		(0x60)

/* adc global regs */
#define ANA_REG_GLB_ANA_APB_CLK_EN		    (ANA_REG_BASE)
#define BIT_ANA_CLK_AUXAD_EN                ( BIT_14 )
#define BIT_ANA_CLK_AUXADC_EN               ( BIT_13 )
#define BIT_ANA_ADC_EB                      ( BIT_5 )


/*ADC_CTL */
#define ADC_MAX_SAMPLE_NUM			(0x10)
#define BIT_SW_CH_RUN_NUM(_X_)		((((_X_) - 1) & 0xf ) << 4)
#define BIT_ADC_BIT_MODE(_X_)		(((_X_) & 0x1) << 2)	/*0: adc in 10bits mode, 1: adc in 12bits mode */
#define BIT_ADC_BIT_MODE_MASK		BIT_ADC_BIT_MODE(1)
#define BIT_SW_CH_ON                    ( BIT_1 ) /*WO*/
#define BIT_ADC_EN                      ( BIT_0 )

/*ADC_SW_CH_CFG && ADC_FAST(SLOW)_HW_CHX_CFG*/
#define BIT_CH_IN_MODE(_X_)		(((_X_) & 0x1) << 8)	/*0: resistance path, 1: capacitance path */
#define BIT_CH_SLOW(_X_)		(((_X_) & 0x1) << 6)	/*0: quick mode, 1: slow mode */
#define BIT_CH_SCALE(_X_)		(((_X_) & 0x1) << 5)	/*0: little scale, 1: big scale */
#define BIT_CH_ID(_X_)			((_X_) & 0x1f)

/*ADC_FAST(SLOW)_HW_CHX_CFG*/
#define BIT_CH_DLY_EN(_X_)		(((_X_) & 0x1) << 7)	/*0:disable, 1:enable */

/*ADC_HW_CH_DELAY*/
#define BIT_HW_CH_DELAY(_X_)		((_X_) & 0xff)	/*its unit is ADC clock */
#define BIT_ADC_EB                  ( BIT_5 )
#define BIT_CLK_AUXADC_EN           ( BIT_13 )
#define BIT_CLK_AUXAD_EN			( BIT_14 )

#ifdef __cplusplus
}
#endif
#endif
