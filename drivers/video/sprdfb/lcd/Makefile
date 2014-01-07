#
#
#


include $(TOPDIR)/config.mk

LIB	:= $(obj)liblcd.o

#ifdef CONFIG_LCD
COBJS-$(CONFIG_FB_LCD_HX8363_RGB_SPI) += lcd_hx8363_rgb_spi.o
COBJS-$(CONFIG_FB_LCD_HX8363_MCU) += lcd_hx8363_mcu.o
COBJS-$(CONFIG_FB_LCD_S6D0139) += lcd_s6d0139.o
COBJS-$(CONFIG_FB_LCD_NT35516_MCU) += lcd_nt35516_mcu.o
COBJS-$(CONFIG_FB_LCD_NT35516_MIPI) += lcd_nt35516_mipi.o
COBJS-$(CONFIG_FB_LCD_NT35516_RGB_I2C) += lcd_nt35516_rgb_i2c.o
COBJS-$(CONFIG_FB_LCD_NT35516_RGB_SPI) += lcd_nt35516_rgb_spi.o
COBJS-$(CONFIG_FB_LCD_NT35510_MIPI) += lcd_nt35510_mipi.o
COBJS-$(CONFIG_FB_LCD_NT35512_MIPI) += lcd_nt35512_mipi.o
COBJS-$(CONFIG_FB_LCD_OTM8018B_MIPI) += lcd_otm_8018b_mipi.o
COBJS-$(CONFIG_FB_LCD_SSD2075_MIPI) += lcd_ssd2075_mipi.o
COBJS-$(CONFIG_FB_LCD_ILI9486)  += lcd_ili9486.o
COBJS-$(CONFIG_FB_LCD_ILI9486_RGB_SPI) += lcd_ili9486_rgb_spi.o
COBJS-$(CONFIG_FB_LCD_NT51017_LVDS) += lcd_nt51017_mipi_lvds.o
COBJS-$(CONFIG_FB_LCD_HX8369B_MIPI) += lcd_hx8369b_mipi.o
COBJS-$(CONFIG_FB_LCD_ILI9341)  += lcd_ili9341.o
COBJS-$(CONFIG_FB_LCD_ST7789V_MCU)  += lcd_st7789v_mcu.o
#endif

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
