/******************************************************************************
 ** File Name:    sprdfb_dispc.c                                            *
 ** Author:                                                           *
 ** DATE:                                                           *
 ** Copyright:    2005 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                            *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 **
 ******************************************************************************/

#include <common.h>

#include "sprdfb.h"
#include "sprdfb_chip_common.h"

#ifdef CONFIG_SPX15
#define SPRDFB_DPI_CLOCK_SRC (192000000)
#else
#define SPRDFB_DPI_CLOCK_SRC (384000000)
#endif

static uint16_t		is_first_frame = 1;

extern void sprdfb_panel_before_refresh(struct sprdfb_device *dev);
extern void sprdfb_panel_after_refresh(struct sprdfb_device *dev);
extern void sprdfb_panel_invalidate(struct panel_spec *self);
extern void sprdfb_panel_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom);

static void __raw_bits_and(unsigned int v, unsigned int a)
{
	__raw_writel((__raw_readl(a) & v), a);
}

/* dispc soft reset */
static void dispc_reset(void)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);
	__raw_writel(__raw_readl(DISPC_AHB_SOFT_RST) | (BIT_DISPC_SOFT_RST), DISPC_AHB_SOFT_RST);
	udelay(10);
	__raw_writel(__raw_readl(DISPC_AHB_SOFT_RST) & (~(BIT_DISPC_SOFT_RST)), DISPC_AHB_SOFT_RST);
}

static inline void dispc_set_bg_color(uint32_t bg_color)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	dispc_write(bg_color, DISPC_BG_COLOR);
}

static inline void dispc_set_osd_ck(uint32_t ck_color)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	dispc_write(ck_color, DISPC_OSD_CK);
}

static void dispc_dithering_enable(uint16_t enable)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	if(enable){
		dispc_set_bits((1<<6), DISPC_CTRL);
	}else{
		dispc_clear_bits((1<<6), DISPC_CTRL);
	}
}

static void dispc_set_exp_mode(uint16_t exp_mode)
{
	uint32_t reg_val = dispc_read(DISPC_CTRL);

	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	reg_val &= ~(0x3 << 16);
	reg_val |= (exp_mode << 16);
	dispc_write(reg_val, DISPC_CTRL);
}

static void dispc_module_enable(void)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	/*dispc module enable */
	dispc_write((1<<0), DISPC_CTRL);

	/*disable dispc INT*/
	dispc_write(0x0, DISPC_INT_EN);

	/* clear dispc INT */
	dispc_write(0x1F, DISPC_INT_CLR);
}

static inline int32_t  dispc_set_disp_size(struct sprdfb_device *dev)
{
	uint32_t reg_val;

	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	reg_val = (dev->panel->width& 0xfff) | ((dev->panel->height & 0xfff ) << 16);
	dispc_write(reg_val, DISPC_SIZE_XY);

	return 0;
}

static void dispc_layer_init(struct sprdfb_device *dev)
{
	uint32_t reg_val = 0;

	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	dispc_clear_bits((1<<0),DISPC_IMG_CTRL);
	dispc_clear_bits((1<<0),DISPC_OSD_CTRL);

	/******************* OSD layer setting **********************/

	/*enable OSD layer*/
	reg_val |= (1 << 0);

	/*disable  color key */

	/* alpha mode select  - block alpha*/
	reg_val |= (1 << 2);

	/* data format */
	/* RGB565 */
	reg_val |= (5 << 4);
	/* B2B3B0B1 */
	reg_val |= (2 << 8);

	dispc_write(reg_val, DISPC_OSD_CTRL);

	/* OSD layer alpha value */
	dispc_write(0xff, DISPC_OSD_ALPHA);

	/* OSD layer size */
#ifdef CONFIG_FB_LOW_RES_SIMU_SUPPORT
	if((0 != dev->panel->display_width) && (0 != dev->panel->display_height)){
		reg_val = ( dev->panel->display_width& 0xfff) | ((dev->panel->display_height & 0xfff ) << 16);
	}else
#endif	
	reg_val = ( dev->panel->width& 0xfff) | ((dev->panel->height & 0xfff ) << 16);
	dispc_write(reg_val, DISPC_OSD_SIZE_XY);

	/* OSD layer start position */
	dispc_write(0, DISPC_OSD_DISP_XY);

	/* OSD layer pitch */
#ifdef CONFIG_FB_LOW_RES_SIMU_SUPPORT
	if((0 != dev->panel->display_width) && (0 != dev->panel->display_height)){
		reg_val = ( dev->panel->display_width & 0xfff) ;
	}else
#endif	
	reg_val = ( dev->panel->width & 0xfff) ;
	dispc_write(reg_val, DISPC_OSD_PITCH);

	/*OSD base address*/
	dispc_write(dev->smem_start, DISPC_OSD_BASE_ADDR);

	/* OSD color_key value */
	dispc_set_osd_ck(0x0);

	/* DISPC workplane size */
	dispc_set_disp_size(dev);

	FB_PRINT("DISPC_OSD_CTRL: 0x%x\n", dispc_read(DISPC_OSD_CTRL));
	FB_PRINT("DISPC_OSD_ALPHA: 0x%x\n", dispc_read(DISPC_OSD_ALPHA));
	FB_PRINT("DISPC_OSD_SIZE_XY: 0x%x\n", dispc_read(DISPC_OSD_SIZE_XY));
	FB_PRINT("DISPC_OSD_DISP_XY: 0x%x\n", dispc_read(DISPC_OSD_DISP_XY));
	FB_PRINT("DISPC_OSD_PITCH: 0x%x\n", dispc_read(DISPC_OSD_PITCH));
	FB_PRINT("DISPC_OSD_BASE_ADDR: 0x%x\n", dispc_read(DISPC_OSD_BASE_ADDR));
}

static void dispc_update_clock(struct sprdfb_device *dev)
{
	uint32_t hpixels, vlines, need_clock, dividor;

	struct panel_spec* panel = dev->panel;
	struct info_mipi * mipi = panel->info.mipi;
	struct info_rgb* rgb = panel->info.rgb;

	FB_PRINT("sprdfb:[%s]\n", __FUNCTION__);

	if(0 == panel->fps){
		printf("sprdfb: No panel->fps specified!\n");
		return;
	}

	if(SPRDFB_PANEL_IF_DPI == dev->panel_if_type){
		if(LCD_MODE_DSI == dev->panel->type ){
			hpixels = panel->width + mipi->timing->hsync + mipi->timing->hbp + mipi->timing->hfp;
			vlines = panel->height + mipi->timing->vsync + mipi->timing->vbp + mipi->timing->vfp;
		}else if(LCD_MODE_RGB == dev->panel->type ){
			hpixels = panel->width + rgb->timing->hsync + rgb->timing->hbp + rgb->timing->hfp;
			vlines = panel->height + rgb->timing->vsync + rgb->timing->vbp + rgb->timing->vfp;
		}else{
			printf("sprdfb:[%s] unexpected panel type!(%d)\n", __FUNCTION__, dev->panel->type);
			return;
		}

		need_clock = hpixels * vlines * panel->fps;
		dividor = SPRDFB_DPI_CLOCK_SRC/need_clock;
		if(SPRDFB_DPI_CLOCK_SRC - dividor*need_clock > (need_clock/2) ) {
			dividor += 1;
		}
		if((dividor<1) || (dividor > 0x100)){
			printf("sprdfb:[%s]: Invliad dividor(%d)!Not update dpi clock!\n", __FUNCTION__, dividor);
			return;
		}

		dev->dpi_clock = SPRDFB_DPI_CLOCK_SRC / dividor;
		dispc_print_clk();
#ifdef CONFIG_SPX15
		dispc_dpi_clk_set(DISPC_DPI_SEL_DEFAULT, (dividor-1));
#else
		dispc_dpi_clk_set(DISPC_DPI_SEL_384M, (dividor-1));
#endif
		dispc_print_clk();

		printf("sprdfb:[%s] need_clock = %d, dividor = %d, dpi_clock = %d\n", __FUNCTION__, need_clock, dividor, dev->dpi_clock);
	}

}

#ifdef CONFIG_SPX15
//#define GSP_IOMMU_WORKAROUND1
#endif

#ifdef GSP_IOMMU_WORKAROUND1

static void cycle_delay(uint32_t delay)
{
	while(delay--);
}

/*
func:gsp_iommu_workaround
desc:dolphin IOMMU workaround, configure GSP-IOMMU CTL REG before dispc_emc enable,
     including config IO base addr and enable gsp-iommu
warn:when dispc_emc disabled, reading ctl or entry register will hung up AHB bus .
     only 4-writting operations are allowed to exeute, over 4 ops will also hung uo AHB bus
     GSP module soft reset is not allowed , beacause it will clear gsp-iommu-ctrl register
*/
static void gsp_iommu_workaround(void)
{
	uint32_t emc_en_cfg = 0;
	uint32_t gsp_en_cfg = 0;
	#define REG_WRITE(reg,value)	(*(volatile uint32_t*)reg = value)
	#define GSP_MMU_CTRL_BASE		(0x21404000)

	emc_en_cfg = __raw_readl(DISPC_EMC_EN);
	gsp_en_cfg = __raw_readl(DISPC_AHB_EN);
	printf("ytc:gsp_iommu_workaround: emc_eb_cfg:%x; gsp_eb_cfg:%x;\n", emc_en_cfg, gsp_en_cfg);
	REG_WRITE(DISPC_EMC_EN,emc_en_cfg & (~0x800)); // disable emc clk
	REG_WRITE(DISPC_AHB_EN,gsp_en_cfg | 0x8); // enable gsp
	cycle_delay(5); // delay for a while
	REG_WRITE(GSP_MMU_CTRL_BASE,0x10000001);//set iova base as 0x10000000, and enable gsp_iommu
	cycle_delay(5); // delay for a while
	printf("ytc:gsp_iommu_workaround: %x, gsp_eb_cfg: %x\n", __raw_readl(DISPC_EMC_EN), __raw_readl(DISPC_AHB_EN));
	REG_WRITE(DISPC_AHB_EN,gsp_en_cfg); // restore gsp
	REG_WRITE(DISPC_EMC_EN,emc_en_cfg); // restore emc clk
}

#endif
static int32_t sprdfb_dispc_early_init(struct sprdfb_device *dev)
{
	FB_PRINT("sprdfb:[%s]\n", __FUNCTION__);

        dispc_print_clk();


#ifdef CONFIG_SPX15
	dispc_pll_clk_set(DISPC_PLL_SEL_DEFAULT, 0);
	dispc_dbi_clk_set(DISPC_DBI_SEL_DEFAULT, 0);
	dispc_dpi_clk_set(DISPC_DPI_SEL_DEFAULT, DISPC_DPI_DIV_DEFAULT);
#else
	dispc_pll_clk_set(DISPC_PLL_SEL_256M, 0);
	dispc_dbi_clk_set(DISPC_DBI_SEL_256M, 0);
	dispc_dpi_clk_set(DISPC_DPI_SEL_384M, DISPC_DPI_DIV_DEFAULT);
#endif
	
 	dev->dpi_clock = SPRDFB_DPI_CLOCK_SRC /(DISPC_DPI_DIV_DEFAULT + 1);


	__raw_bits_or(BIT_DISPC_CORE_EN, DISPC_CORE_EN);  //core_clock_en
#ifdef GSP_IOMMU_WORKAROUND1
	gsp_iommu_workaround();
#endif
	__raw_bits_or(BIT_DISPC_EMC_EN, DISPC_EMC_EN);  //matrix clock en

	__raw_bits_or(BIT_DISPC_AHB_EN, DISPC_AHB_EN);//enable DISPC clock

	FB_PRINT("sprdfb:DISPC_CORE_EN:%x \n",__raw_readl(DISPC_CORE_EN));
	FB_PRINT("sprdfb:DISPC_EMC_EN:%x \n",__raw_readl(DISPC_EMC_EN));
	FB_PRINT("sprdfb:DISPC_AHB_EN:%x \n",__raw_readl(DISPC_AHB_EN));


        dispc_print_clk();


	dispc_reset();
	dispc_module_enable();
	is_first_frame = 1;

	return 0;
}


static int32_t sprdfb_dispc_init(struct sprdfb_device *dev)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	/*set bg color*/
	dispc_set_bg_color(0xFFFFFFFF);
	/*enable dithering*/
	dispc_dithering_enable(1);
	/*use MSBs as img exp mode*/
	dispc_set_exp_mode(0x0);

	dispc_layer_init(dev);

//	dispc_update_clock(dev);

	if(SPRDFB_PANEL_IF_DPI == dev->panel_if_type){
		if(is_first_frame){
			/*set dpi register update only with SW*/
			dispc_set_bits((1<<4), DISPC_DPI_CTRL);
		}else{
			/*set dpi register update with SW & VSYNC*/
			dispc_clear_bits((1<<4), DISPC_DPI_CTRL);
		}
		/*enable dispc update done INT*/
//		dispc_write((1<<4), DISPC_INT_EN);
	}else{
		/* enable dispc DONE  INT*/
//		dispc_write((1<<0), DISPC_INT_EN);
	}
#ifdef CONFIG_SC8830
		{/*for debug*/
			int32_t i;
			for(i=0x20800000;i<0x208000c0;i+=16){
				FB_PRINT("sprdfb: %x: 0x%x, 0x%x, 0x%x, 0x%x\n", i, __raw_readl(i), __raw_readl(i+4), __raw_readl(i+8), __raw_readl(i+12));
			}
			FB_PRINT("**************************************\n");
		}
#endif
	return 0;
}

static int32_t sprdfb_dispc_uninit(struct sprdfb_device *dev)
{
	FB_PRINT("sprdfb:[%s]\n",__FUNCTION__);

	__raw_bits_and(~(BIT_DISPC_AHB_EN), DISPC_AHB_EN);

	return 0;
}

static int32_t sprdfb_dispc_refresh (struct sprdfb_device *dev)
{
	uint32_t i;

	printf("sprdfb:[%s]\n",__FUNCTION__);

	if(SPRDFB_PANEL_IF_DPI != dev->panel_if_type){
		sprdfb_panel_invalidate(dev->panel);
	}

	sprdfb_panel_before_refresh(dev);

	if(SPRDFB_PANEL_IF_DPI == dev->panel_if_type){
		/*dpi register update*/
		dispc_set_bits((1<<5), DISPC_DPI_CTRL);
		if(is_first_frame){
			udelay(30);
			/*dpi register update with SW and VSync*/
			dispc_clear_bits((1<<4), DISPC_DPI_CTRL);
			/* start refresh */
			dispc_set_bits((1 << 4), DISPC_CTRL);
			is_first_frame = 0;
		}else{
			for(i=0;i<1000;i++){
				if(!(dispc_read(DISPC_INT_RAW) & (0x10))){
					udelay(100);
				}else{
					break;
				}
			}
			if(i >= 1000){
				printf("sprdfb:[%s] wait dispc update  int time out!! (0x%x)\n", __FUNCTION__, dispc_read(DISPC_INT_RAW));
			}else{
				FB_PRINT("sprdfb:[%s] got dispc update int (0x%x)\n", __FUNCTION__, dispc_read(DISPC_INT_RAW));
			}
			dispc_set_bits((1<<5), DISPC_INT_CLR);
                   }
	}else{
		/* start refresh */
		dispc_set_bits((1 << 4), DISPC_CTRL);
		for(i=0;i<500;i++){
			if(0x1 != (dispc_read(DISPC_INT_RAW) & (1<<0))){
				udelay(1000);
			}else{
				break;
			}
		}
		if(i >= 1000){
			printf("sprdfb:[%s] wait dispc done int time out!! (0x%x)\n", __FUNCTION__, dispc_read(DISPC_INT_RAW));
		}else{
			FB_PRINT("sprdfb:[%s] got dispc done int (0x%x)\n", __FUNCTION__, dispc_read(DISPC_INT_RAW));
		}
		dispc_set_bits((1<<0), DISPC_INT_CLR);
	}

	sprdfb_panel_after_refresh(dev);

	return 0;
}

struct display_ctrl sprdfb_dispc_ctrl = {
	.name		= "dispc",
	.early_init		= sprdfb_dispc_early_init,
	.init		 	= sprdfb_dispc_init,
	.uninit		= sprdfb_dispc_uninit,
	.refresh		= sprdfb_dispc_refresh,
	.update_clk     = dispc_update_clock,
};

