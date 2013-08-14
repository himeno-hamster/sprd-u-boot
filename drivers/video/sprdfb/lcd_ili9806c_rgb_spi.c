/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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

#include <asm/arch/sprd_lcd.h>
#include <asm/arch/adc_drvapi.h>
#include <asm/io.h>

#define printk printf

#define  LCD_DEBUG
#ifdef LCD_DEBUG
#define LCD_PRINT printk
#else
#define LCD_PRINT(...)
#endif

#define ILI9806C_SpiWriteCmd(cmd) \ 
{ \
	spi_send_cmd((cmd& 0xFF));\
}

#define  ILI9806C_SpiWriteData(data)\
{ \
	spi_send_data((data& 0xFF));\
}

enum {
	LCD_YAXING=1,
	LCD_DEZHIXIN,
	LCD_YICAI,
};

static int32_t lcd_type=LCD_YAXING;
extern void ADC_Init (void);
extern int32_t ADC_GetValue(adc_channel adcSource, bool scale);

static int32_t ili9806c_init(struct panel_spec *self)
{
	uint32_t test_data[8] = {0};
	uint32_t left = 0;
	uint32_t top = 0;	
	uint32_t right = 480;
	uint32_t bottom = 800;		
	uint32_t data = 0;
	spi_send_cmd_t spi_send_cmd = self->info.rgb->bus_info.spi->ops->spi_send_cmd;
	spi_send_data_t spi_send_data = self->info.rgb->bus_info.spi->ops->spi_send_data;
	spi_read_t spi_read = self->info.rgb->bus_info.spi->ops->spi_read;

	LCD_PRINT("ili9806c_init\n");

	//************* Start Initial Sequence **********// 
	ILI9806C_SpiWriteCmd(0xFF); // EXTC Command Set enable register 
	ILI9806C_SpiWriteData(0xFF); 
	ILI9806C_SpiWriteData(0x98); 
	ILI9806C_SpiWriteData(0x16); 
	 
	ILI9806C_SpiWriteCmd(0xBA); // SPI Interface Setting 
	ILI9806C_SpiWriteData(0x60); 
	 
	ILI9806C_SpiWriteCmd(0XB0); // Interface Mode Control 
	ILI9806C_SpiWriteData(0x01); 
	 
	ILI9806C_SpiWriteCmd(0xBC); // GIP 1 
	ILI9806C_SpiWriteData(0x03); 
	ILI9806C_SpiWriteData(0x0D); 
	ILI9806C_SpiWriteData(0x61); 
	ILI9806C_SpiWriteData(0xFF); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x1B); 
	ILI9806C_SpiWriteData(0x11); 
	ILI9806C_SpiWriteData(0x38); 
	ILI9806C_SpiWriteData(0x63); 
	ILI9806C_SpiWriteData(0xFF); 
	ILI9806C_SpiWriteData(0xFF); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x10);
	ILI9806C_SpiWriteData(0x00); 
	ILI9806C_SpiWriteData(0xFF); 
	ILI9806C_SpiWriteData(0XF2); 
	 
	ILI9806C_SpiWriteCmd(0xBD); // GIP 2 
	ILI9806C_SpiWriteData(0x02); 
	ILI9806C_SpiWriteData(0x13); 
	ILI9806C_SpiWriteData(0x45); 
	ILI9806C_SpiWriteData(0x67); 
	ILI9806C_SpiWriteData(0x45); 
	ILI9806C_SpiWriteData(0x67); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x23); 
	 
	ILI9806C_SpiWriteCmd(0xBE); // GIP 3 
	ILI9806C_SpiWriteData(0x03); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0xDD); 
	ILI9806C_SpiWriteData(0xCC); 
	ILI9806C_SpiWriteData(0xBB); 
	ILI9806C_SpiWriteData(0xAA); 
	ILI9806C_SpiWriteData(0x66); 
	ILI9806C_SpiWriteData(0x77); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	ILI9806C_SpiWriteData(0x22); 
	 
	ILI9806C_SpiWriteCmd(0xED); // en_volt_reg measure VGMP 
	ILI9806C_SpiWriteData(0x7F); 
	ILI9806C_SpiWriteData(0x0F); 
	 
	ILI9806C_SpiWriteCmd(0xF3);   
	ILI9806C_SpiWriteData(0x70); 

	ILI9806C_SpiWriteCmd(0XB4); // Display Inversion Control 
	ILI9806C_SpiWriteData(0x02); 
	 
	ILI9806C_SpiWriteCmd(0XC0); // Power Control 1 
	ILI9806C_SpiWriteData(0x0F); 
	ILI9806C_SpiWriteData(0x0B); 
	ILI9806C_SpiWriteData(0x0A); 
	 
	ILI9806C_SpiWriteCmd(0XC1); // Power Control 2 
	ILI9806C_SpiWriteData(0x17); 
	ILI9806C_SpiWriteData(0x80); 
	ILI9806C_SpiWriteData(0x68); 
	ILI9806C_SpiWriteData(0x20); 
	 
	ILI9806C_SpiWriteCmd(0XD8); // VGLO Selection 
	ILI9806C_SpiWriteData(0x50); 
	 
	ILI9806C_SpiWriteCmd(0XFC); // VGLO Selection 
	ILI9806C_SpiWriteData(0x07); 
	 
	ILI9806C_SpiWriteCmd(0XE0); // Positive Gamma Control 
	ILI9806C_SpiWriteData(0x00); 
	ILI9806C_SpiWriteData(0x04); 
	ILI9806C_SpiWriteData(0x0C); 
	ILI9806C_SpiWriteData(0x12); 
	ILI9806C_SpiWriteData(0x13); 
	ILI9806C_SpiWriteData(0x1D); 
	ILI9806C_SpiWriteData(0XCA); 
	ILI9806C_SpiWriteData(0x09); 
	ILI9806C_SpiWriteData(0x04); 
	ILI9806C_SpiWriteData(0x0B); 
	ILI9806C_SpiWriteData(0x03); 
	ILI9806C_SpiWriteData(0x0B); 
	ILI9806C_SpiWriteData(0x0E); 
	ILI9806C_SpiWriteData(0x2D); 
	ILI9806C_SpiWriteData(0x2A); 
	ILI9806C_SpiWriteData(0x00);

	ILI9806C_SpiWriteCmd(0XE1); // Negative Gamma Control 
	ILI9806C_SpiWriteData(0x00); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x04); 
	ILI9806C_SpiWriteData(0x0A); 
	ILI9806C_SpiWriteData(0x0E); 
	ILI9806C_SpiWriteData(0x11); 
	ILI9806C_SpiWriteData(0X79); 
	ILI9806C_SpiWriteData(0x09); 
	ILI9806C_SpiWriteData(0x04); 
	ILI9806C_SpiWriteData(0x08); 
	ILI9806C_SpiWriteData(0x08); 
	ILI9806C_SpiWriteData(0x0B); 
	ILI9806C_SpiWriteData(0x09); 
	ILI9806C_SpiWriteData(0x34); 
	ILI9806C_SpiWriteData(0x2E); 
	ILI9806C_SpiWriteData(0x00); 
	 
	ILI9806C_SpiWriteCmd(0XD5); // Source Timing Adjust 
	ILI9806C_SpiWriteData(0x0D); 
	ILI9806C_SpiWriteData(0x0A); 
	ILI9806C_SpiWriteData(0x05); 
	ILI9806C_SpiWriteData(0x05); 
	ILI9806C_SpiWriteData(0xCB); 
	ILI9806C_SpiWriteData(0XA5); 
	ILI9806C_SpiWriteData(0x01); 
	ILI9806C_SpiWriteData(0x04); 
	 
	ILI9806C_SpiWriteCmd(0XF7); // Resolution 
	ILI9806C_SpiWriteData(0x8A); 
	 
	ILI9806C_SpiWriteCmd(0XC7); // Vcom 
	ILI9806C_SpiWriteData(0x6F); 

	 ILI9806C_SpiWriteCmd(0X3A); // Vcom 
	ILI9806C_SpiWriteData(0x77);
		
	ILI9806C_SpiWriteCmd(0X11);        // Exit Sleep 
	udelay(120000);                                 
	
	ILI9806C_SpiWriteCmd(0XEE);   
	ILI9806C_SpiWriteData(0x0A); 
	ILI9806C_SpiWriteData(0x1B); 
	ILI9806C_SpiWriteData(0x5F); 
	ILI9806C_SpiWriteData(0x40); 
	ILI9806C_SpiWriteData(0x00); 
	ILI9806C_SpiWriteData(0X00); 
	ILI9806C_SpiWriteData(0x10); 
	ILI9806C_SpiWriteData(0x00); 
	ILI9806C_SpiWriteData(0x58); 
	
	ILI9806C_SpiWriteCmd(0X29);        // Display On
}

static int32_t ili9806c_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	spi_send_cmd_t spi_send_cmd = self->info.rgb->bus_info.spi->ops->spi_send_cmd;
	spi_send_data_t spi_send_data = self->info.rgb->bus_info.spi->ops->spi_send_data;
	
	if(is_sleep==1){
		//Sleep In
		ILI9806C_SpiWriteCmd(0x0028);
		udelay(20000); 
		ILI9806C_SpiWriteCmd(0x0010);
		udelay(120000); 
	}else{
		//Sleep Out
		ILI9806C_SpiWriteCmd(0x11);
		udelay(120000);
		ILI9806C_SpiWriteCmd(0x29);
		udelay(20000); 
	}

	return 0;
}




static int32_t ili9806c_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	return 0;
}
static int32_t ili9806c_invalidate(struct panel_spec *self)
{
	LCD_PRINT("ili9806c_invalidate\n");

	return self->ops->panel_set_window(self, 0, 0,
		self->width - 1, self->height - 1);
}



static int32_t ili9806c_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	LCD_PRINT("ili9806c_invalidate_rect \n");

	return self->ops->panel_set_window(self, left, top,
			right, bottom);
}

static int32_t ili9806c_read_id(struct panel_spec *self)
{
	return 0x9816;
}

static int32_t ili9806c_read_md(struct panel_spec *self)
{
	return lcd_type; 
}

static struct panel_operations lcd_ili9806c_rgb_spi_operations = {
	.panel_init = ili9806c_init,
	.panel_set_window = ili9806c_set_window,
	.panel_invalidate_rect= ili9806c_invalidate_rect,
	.panel_invalidate = ili9806c_invalidate,
	.panel_enter_sleep = ili9806c_enter_sleep,
	.panel_readid          = ili9806c_read_id,
	.panel_readmd          = ili9806c_read_md
};

static struct timing_rgb lcd_ili9806c_rgb_spi_timing = {
	.hfp = 30,  /* unit: pixel */
	.hbp = 50,
	.hsync = 10,
	.vfp = 20, /*unit: line*/
	.vbp = 14,
	.vsync = 6,
};

static struct spi_info lcd_ili9806c_rgb_spi_info = {
	.ops = NULL,
};

static struct info_rgb lcd_ili9806c_rgb_info = {
	.cmd_bus_mode  = SPRDFB_RGB_BUS_TYPE_SPI,
	.video_bus_width = 24, /*18,16*/
	.h_sync_pol = SPRDFB_POLARITY_NEG,
	.v_sync_pol = SPRDFB_POLARITY_NEG,
	.de_pol = SPRDFB_POLARITY_POS,
	.timing = &lcd_ili9806c_rgb_spi_timing,
	.bus_info = {
		.spi = &lcd_ili9806c_rgb_spi_info,
	}
};

struct panel_spec lcd_ili9806c_rgb_spi_spec = {
	.width = 480,
	.height = 800,
	.fps = 60,
	.type = LCD_MODE_RGB,
	.direction = LCD_DIRECT_NORMAL,
	.info = {
		.rgb = &lcd_ili9806c_rgb_info
	},
	.ops = &lcd_ili9806c_rgb_spi_operations,
};
