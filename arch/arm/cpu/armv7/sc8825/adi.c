/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 * Copyright (C) 2012 steve zhan
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

/**
*For arm read ,delay about 1us when clk_adi runs at 76.8M
*The interface timing is compatible with spi timing
*/

#include <common.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch-sc8825/ldo.h>
#include <asm/arch/sc8810_reg_global.h>
#include <asm/arch/regs_adi.h>
#include <asm/arch/regs_global.h>
#include <asm/arch/regs_cpc.h>
#include <asm/arch/analog_reg_v3.h>
#include <ubi_uboot.h>
#include <asm/sizes.h>


/* soc defined begin*/
#define CTL_ADI_BASE			(SPRD_ADI_BASE)
#define SPRD_MISC_BASE	(SPRD_ADI_BASE)
#define SPRD_MISC_PHYS  (SPRD_ADI_BASE)

/* registers definitions for controller CTL_ADI */
#define REG_ADI_CTRL0					(CTL_ADI_BASE + 0x04)
#define REG_ADI_CHNL_PRI				(CTL_ADI_BASE + 0x08)
#define REG_ADI_INT_RAW					(CTL_ADI_BASE + 0x10)
#define REG_ADI_RD_CMD					(CTL_ADI_BASE + 0x24)
#define REG_ADI_RD_DATA					(CTL_ADI_BASE + 0x28)
#define REG_ADI_FIFO_STS				(CTL_ADI_BASE + 0x2c)

/* bits definitions for register REG_ADI_CTRL0 */
#define BIT_ARM_SCLK_EN                 ( BIT_1 )
#define BITS_CMMB_WR_PRI			( (1) << 4 & (BIT_4|BIT_5) )

/* bits definitions for register REG_ADI_CHNL_PRI */
#define BITS_PD_WR_PRI             ( (1) << 14 & (BIT_14|BIT_15) )
#define BITS_RFT_WR_PRI       	   ( (1) << 12 & (BIT_12|BIT_13) )
#define BITS_DSP_RD_PRI            ( (2) << 10 & (BIT_10|BIT_11) )
#define BITS_DSP_WR_PRI            ( (2) << 8 & (BIT_8|BIT_9) )
#define BITS_ARM_RD_PRI            ( (3) << 6 & (BIT_6|BIT_7) )
#define BITS_ARM_WR_PRI            ( (3) << 4 & (BIT_4|BIT_5) )
#define BITS_STC_WR_PRI            ( (1) << 2 & (BIT_2|BIT_3) )
#define BITS_INT_STEAL_PRI         ( (3) << 0 & (BIT_0|BIT_1) )

/* bits definitions for register REG_ADI_RD_DATA */
#define BIT_RD_CMD_BUSY                 ( BIT_31 )
#define SHIFT_RD_ADDR                   ( 16 )

#define SHIFT_RD_VALU                   ( 0 )
#define MASK_RD_VALU                    ( 0xFFFF )

/* bits definitions for register REG_ADI_FIFO_STS */
#define BIT_FIFO_FULL                   ( BIT_11 )
#define FIFO_IS_FULL()	(__raw_readl(REG_ADI_FIFO_STS) & BIT_FIFO_FULL)

#define BIT_FIFO_EMPTY                  ( BIT_10 )

/* special V1 (sc8830 soc) defined */
/* bits definitions for register REG_ADI_CTRL0 */
#define BIT_ADI_WR(_X_)                 ( (_X_) << 2 )
#define BITS_ADDR_BYTE_SEL(_X_)			( (_X_) << 0 & (BIT_0|BIT_1) )

/* bits definitions for register REG_ADI_CHNL_PRI */
#define VALUE_CH_PRI	(0x0)

#define REG_ADI_GSSI_CFG0					(CTL_ADI_BASE + 0x1C)
#define REG_ADI_GSSI_CFG1					(CTL_ADI_BASE + 0x20)

/* soc defined end*/

static u32 readback_addr_mak = 0;

/*FIXME: Now define adi IP version, sc8825 is zero, sc8830 is one,
* Adi need init early that than read soc id, now using this ARCH dependency.
*/
static inline int __adi_ver(void)
{
#ifdef	CONFIG_SC8825
	return 0;
#else
	return 1;
#endif
}

#define	TO_ADDR(_x_)		( ((_x_) >> SHIFT_RD_ADDR) & readback_addr_mak )

/*FIXME:If we have not hwspinlock , we need use spinlock to do it*/
static inline void __adi_lock(unsigned long *flags, unsigned long *hw_flags)
{
	return;
}

static inline void __adi_unlock(unsigned long *flags, unsigned long *hw_flags)
{
	return;
}

static inline int __adi_fifo_drain(void)
{
	int cnt = 1000;
	while (!(__raw_readl(REG_ADI_FIFO_STS) & BIT_FIFO_EMPTY) && cnt--) {
		udelay(1);
	}
	WARN(cnt == 0, "ADI WAIT timeout!!!");
	return 0;
}

#define ANA_VIRT_BASE			( SPRD_MISC_BASE )
#define ANA_PHYS_BASE			( SPRD_MISC_PHYS )
#define ANA_ADDR_SIZE			(SZ_4K)
#define ADDR_VERIFY(_X_)	do { \
	BUG_ON((_X_) < ANA_VIRT_BASE || (_X_) > (ANA_VIRT_BASE + ANA_ADDR_SIZE));} while (0)

static inline u32 __adi_translate_addr(u32 regvddr)
{
	regvddr = regvddr - ANA_VIRT_BASE + ANA_PHYS_BASE;
	return regvddr;
}

static inline int __adi_read(u32 regPddr)
{
	unsigned long val;
	int cnt = 2000;

	/*
	 * We don't wait write fifo empty in here,
	 * Because if normal write is SYNC, that will
	 * wait fifo empty at the end of the write.
	 */
	__raw_writel(regPddr, REG_ADI_RD_CMD);

	/*
	 * wait read operation complete, RD_data[31]
	 * is set simulaneously when writing read command.
	 * RD_data[31] will be cleared after the read operation complete, 
	 */
	do {
		val = __raw_readl(REG_ADI_RD_DATA);
	} while ((val & BIT_RD_CMD_BUSY) && cnt--);

	WARN(cnt == 0, "ADI READ timeout!!!");
	/* val high part should be the address of the last read operation */
	BUG_ON(TO_ADDR(val) != (regPddr & readback_addr_mak));

	return (val & MASK_RD_VALU);
}

int sci_adi_read(u32 reg)
{
	unsigned long val;
	unsigned long flags;
	ADDR_VERIFY(reg);
	reg = __adi_translate_addr(reg);
	__adi_lock(&flags, NULL);
	val = __adi_read(reg);
	__adi_unlock(&flags, NULL);
	return val;
}

EXPORT_SYMBOL(sci_adi_read);

/*This value have a bit of depending on  real hardware fifo size*/
#define CACHE_SIZE	(16)
static struct __data {
	u32 reg;
	u16 val;
} __data_array[CACHE_SIZE];
struct __data *head_p = &__data_array[0];
struct __data *tail_p = &__data_array[0];
static u32 data_in_cache = 0;
#define HEAD_ADD	(1)
#define TAIL_ADD	(0)
static inline void __p_add(struct __data **p, u32 isHead)
{
	if (++(*p) > &__data_array[CACHE_SIZE - 1])
		(*p) = &__data_array[0];
	if (head_p == tail_p) {
		if (isHead == HEAD_ADD) {
			data_in_cache = 0;
		} else {
			data_in_cache = CACHE_SIZE;
		}
	} else {
		data_in_cache = 2;
	}
}

static inline int __adi_write(u32 reg, u16 val, u32 sync)
{
	tail_p->reg = reg;
	tail_p->val = val;
	__p_add(&tail_p, TAIL_ADD);
	while (!FIFO_IS_FULL() && (data_in_cache != 0)) {
		__raw_writel(head_p->val, head_p->reg);
		__p_add(&head_p, HEAD_ADD);
	}

	if (sync || data_in_cache == CACHE_SIZE) {
		__adi_fifo_drain();
		while (data_in_cache != 0) {
			while (FIFO_IS_FULL()) {
				//cpu_relax();
			}
			__raw_writel(head_p->val, head_p->reg);
			__p_add(&head_p, HEAD_ADD);
		}
		__adi_fifo_drain();
	}

	return 0;
}

int sci_adi_write_fast(u32 reg, u16 val, u32 sync)
{
	unsigned long flags;
	ADDR_VERIFY(reg);
	__adi_lock(&flags, NULL);
	__adi_write(reg, val, sync);
	__adi_unlock(&flags, NULL);
	return 0;
}

EXPORT_SYMBOL(sci_adi_write_fast);

int sci_adi_write(u32 reg, u16 or_val, u16 clear_msk)
{
	unsigned long flags;

	ADDR_VERIFY(reg);
	__adi_lock(&flags, NULL);
	__adi_write(reg,
		    (__adi_read(__adi_translate_addr(reg)) &
		     ~clear_msk) | or_val, 1);
	__adi_unlock(&flags, NULL);
	return 0;
}

EXPORT_SYMBOL(sci_adi_write);

inline int sci_adi_raw_write(u32 reg, u16 val)
{
		return sci_adi_write_fast(reg, val, 1);
}

inline int sci_adi_set(u32 reg, u16 bits)
{
		return sci_adi_write(reg, bits, 0);
}

inline int sci_adi_clr(u32 reg, u16 bits)
{
		return sci_adi_write(reg, 0, bits);
}

static void __adi_init(void)
{
	uint32_t value;
	value = __raw_readl(REG_ADI_CTRL0);

	if (__adi_ver() == 0) {
		value &= ~BIT_ARM_SCLK_EN;
		value |= BITS_CMMB_WR_PRI;
		__raw_writel(value, REG_ADI_CTRL0);

		value = __raw_readl(REG_ADI_CHNL_PRI);
		value |= BITS_PD_WR_PRI | BITS_RFT_WR_PRI |
		    BITS_DSP_RD_PRI | BITS_DSP_WR_PRI |
		    BITS_ARM_RD_PRI | BITS_ARM_WR_PRI |
		    BITS_STC_WR_PRI | BITS_INT_STEAL_PRI;
		__raw_writel(value, REG_ADI_CHNL_PRI);

		readback_addr_mak = 0x7ff;
	} else if (__adi_ver() == 1) {
		if (value) {
			WARN_ON(1);
		}
		value = VALUE_CH_PRI;
		__raw_writel(value, REG_ADI_CHNL_PRI);

		value = __raw_readl(REG_ADI_GSSI_CFG0);
		readback_addr_mak = (value & 0x3f) - ((value >> 11) & 0x1f) - 1;
	}
}

int sci_adi_init(void)
{
	/* enable adi in global regs */
	sci_glb_set(GR_GEN0, GEN0_ADI_EN);

	/* reset adi */
	sci_glb_set(GR_SOFT_RST, ADI_SOFT_RST);
	udelay(2);
	sci_glb_clr(GR_SOFT_RST, ADI_SOFT_RST);

	__adi_init();

	return 0;
}
