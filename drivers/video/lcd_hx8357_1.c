/* drivers/video/sc8800g/sc8800g_lcd_hx8357.c
 *
 * Support for HX8357 LCD device
 *
 * Copyright (C) 2010 Spreadtrum
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

#include <asm/arch/sc8810_lcd.h>
#define printk printf

/* #define  LCD_DEBUG */
#ifdef LCD_DEBUG
#define LCD_PRINT printk
#else
#define LCD_PRINT(...)
#endif

static int32_t hx8357_init(struct lcd_spec *self)
{
	Send_data send_cmd = self->info.mcu->ops->send_cmd;
	Send_cmd_data send_cmd_data = self->info.mcu->ops->send_cmd_data;
	Send_data send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("hx8357_init\n");
	self->ops->lcd_reset(self);

	send_cmd(0x11);				// SLPOUT 
	LCD_DelayMS(120);
	send_cmd(0xB9);
	send_data(0xFF);
	send_data(0x83);
	send_data(0x57);
	LCD_DelayMS(1);

	send_cmd(0xB1);	  //SETPower 
	send_data(0x00);	 //STB 
	send_data(0x14);	 //VGH = 13V, VGL = -10V 
	send_data(0x1C);	 //VSPR = 4.41V 
	send_data(0x1C);	 //VSNR = -4.41V 
	send_data(0xC3);	 //AP 
	send_data(0x6C); //38		 //FS 
	LCD_DelayMS(1); 


	send_cmd(0xB4);   //SETCYC 
	send_data(0x22);		 //2-dot 
	send_data(0x40);	 //RTN 
	send_data(0x00);	 //DIV 
	send_data(0x2A);	 //N_DUM 
	send_data(0x2A);	 //I_DUM 
	send_data(0x20);	 //GDON 
	send_data(0x78);	 //GDOFF  ==4E
	LCD_DelayMS(1); 

	send_cmd(0xB6);	//VCOMDC 
	send_data(0x22);
	LCD_DelayMS(1);

	send_cmd(0xC0);   //SETSTBA 
	send_data(0x34);	 //N_OPON 
	send_data(0x34);	 //I_OPON 
	send_data(0x02);	 //STBA 
	send_data(0x3C);	 //STBA 
	send_data(0xC8);	 //STBA 
	send_data(0x08);	 //GENON 
	LCD_DelayMS(1); 

	send_cmd(0xC2);   // Set Gate EQ 
	send_data(0x00); 
	send_data(0x08); 
	send_data(0x04); 
	LCD_DelayMS(1); 

	send_cmd(0xCC);   //Set Panel 
	send_data(0x01);  


	send_cmd(0xE0);	//Set Gamma 
	send_data(0x03);		  //VRP0[6:0]
	send_data(0x07);		  //VRP1[6:0]
	send_data(0x13);		  //VRP2[6:0]
	send_data(0x20);		  //VRP3[6:0]
	send_data(0x29);		  //VRP4[6:0]
	send_data(0x3C);		  //VRP5[6:0]
	send_data(0x49);		  //VRP6[6:0]
	send_data(0x52);		  //VRP7[6:0]
	send_data(0x47);		  //VRP8[6:0]
	send_data(0x40);		  //VRP9[6:0]
	send_data(0x3A);		  //VRP10[6:0]
	send_data(0x32);		  //VRP11[6:0]
	send_data(0x30);		  //VRP12[6:0]
	send_data(0x2B);		  //VRP13[6:0]
	send_data(0x27);		  //VRP14[6:0]
	send_data(0x1C);		  //VRP15[6:0]

	send_data(0x03);		  //VRP0[6:0]
	send_data(0x07);		  //VRP1[6:0]
	send_data(0x13);		  //VRP2[6:0]
	send_data(0x20);		  //VRP3[6:0]
	send_data(0x29);		  //VRP4[6:0]
	send_data(0x3C);		  //VRP5[6:0]
	send_data(0x49);		  //VRP6[6:0]
	send_data(0x52);		  //VRP7[6:0]
	send_data(0x47);		  //VRP8[6:0]
	send_data(0x40);		  //VRP9[6:0]
	send_data(0x3A);		  //VRP10[6:0]
	send_data(0x32);		  //VRP11[6:0]
	send_data(0x30);		  //VRP12[6:0]
	send_data(0x2B);		  //VRP13[6:0]
	send_data(0x27);		  //VRP14[6:0]
	send_data(0x1C);		  //VRP15[6:0]

	send_data(0x00);
	send_data(0x01);		
	LCD_DelayMS(1); 


	send_cmd(0x3A);			   
	send_data(0x55);	   


	send_cmd(0x36);
	//send_data(0x48);
	send_data(0x40);
	//send_data(0x08);  

	send_cmd(0x35);	// TE on
	send_data(0x00);

	send_cmd(0x29); // display on
	LCD_DelayMS(5);
	send_cmd(0x2C);

	return 0;
}

static int32_t hx8357_set_window(struct lcd_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	Send_data send_cmd = self->info.mcu->ops->send_cmd;
	Send_cmd_data send_cmd_data = self->info.mcu->ops->send_cmd_data;
	Send_data send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("hx8357_set_window: %d, %d, %d, %d\n",left, top, right, bottom);
    
	send_cmd(0x2A); // col
	send_data((left >> 8));
	send_data((left & 0xFF));
	send_data((right >> 8));
	send_data((right & 0xFF));

	send_cmd(0x2B); // row
	send_data((top >> 8));
	send_data((top & 0xFF));
	send_data((bottom >> 8));
	send_data((bottom & 0xFF));
	
	send_cmd(0x2C); //Write data	
    #if 0
	/* set window size  */
	send_cmd_data(0x0002, (left  >> 8));
	send_cmd_data(0x0003, (left  & 0xff));
	send_cmd_data(0x0004, (right >> 8));
	send_cmd_data(0x0005, (right & 0xff));

	send_cmd_data(0x0006, (top    >> 8));
	send_cmd_data(0x0007, (top    &  0xff));
	send_cmd_data(0x0008, (bottom >> 8));
	send_cmd_data(0x0009, (bottom &  0xff));

	switch (self->direction) {
	case LCD_DIRECT_NORMAL:
	default:
		send_cmd_data(0x0080, (left >> 8));
		send_cmd_data(0x0081, (left &  0xff));
		send_cmd_data(0x0082, (top  >> 8));
		send_cmd_data(0x0083, (top  &  0xff));
		break;

	case LCD_DIRECT_ROT_90:
		send_cmd_data(0x0080, (top >> 8));
		send_cmd_data(0x0081, (top & 0xff));
		send_cmd_data(0x0082, (left >> 8));
		send_cmd_data(0x0083, (left & 0xff));
	    break;

	case LCD_DIRECT_ROT_180:
	case LCD_DIRECT_MIR_HV:
		send_cmd_data(0x0080, (right >> 8));
		send_cmd_data(0x0081, (right & 0xff));
		send_cmd_data(0x0082, (bottom >> 8));
		send_cmd_data(0x0083, (bottom & 0xff));
		break;

	case LCD_DIRECT_ROT_270:
		send_cmd_data(0x0080, (bottom >> 8));
		send_cmd_data(0x0081, (bottom & 0xff));
		send_cmd_data(0x0082, (left >> 8));
		send_cmd_data(0x0083, (left & 0xff));
	    break;

	case LCD_DIRECT_MIR_H:
		send_cmd_data(0x0080, (left >> 8));
		send_cmd_data(0x0081, (left & 0xff));
		send_cmd_data(0x0082, (bottom >> 8));
		send_cmd_data(0x0083, (bottom & 0xff));
		break;

	case LCD_DIRECT_MIR_V:
		send_cmd_data(0x0080, (right >> 8));
		send_cmd_data(0x0081, (right & 0xff));
		send_cmd_data(0x0082, (top >> 8));
		send_cmd_data(0x0083, (top & 0xff));
	    break;
	}
	
	send_cmd(0x0022);
#endif
	return 0;
}

static int32_t hx8357_invalidate(struct lcd_spec *self)
{
	LCD_PRINT("hx8357_invalidate\n");

	return self->ops->lcd_set_window(self, 0, 0, 
			self->width - 1, self->height - 1);
	
}

static int32_t hx8357_invalidate_rect(struct lcd_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	Send_cmd_data send_cmd_data = self->info.mcu->ops->send_cmd_data;
	Send_data send_cmd = self->info.mcu->ops->send_cmd;
	Send_data send_data = self->info.mcu->ops->send_data;
	LCD_PRINT("hx8357_invalidate_rect \n");
	// TE scaneline
	//send_cmd_data(0x000b, (top >> 8));
	//send_cmd_data(0x000c, (top & 0xff));
	send_cmd(0x44); // TE scanline
	send_data((top >> 8));
	send_data((top & 0xFF));	
	return self->ops->lcd_set_window(self, left, top, 
			right, bottom);
}

static int32_t hx8357_set_direction(struct lcd_spec *self, uint16_t direction)
{
	Send_data send_cmd = self->info.mcu->ops->send_cmd;
	Send_data send_data = self->info.mcu->ops->send_data;

	LCD_PRINT("hx8369_set_direction\n");
	send_cmd(0x36);

	switch (direction) {
	case LCD_DIRECT_NORMAL:
		send_data(0);
		break;
	case LCD_DIRECT_ROT_90:
		send_data(0xA0);
		break;
	case LCD_DIRECT_ROT_180:
		send_data(0x60);
		break;
	case LCD_DIRECT_ROT_270:
		send_data(0xB0);
		break;
	case LCD_DIRECT_MIR_H:
		send_data(0x40);
		break;
	case LCD_DIRECT_MIR_V:
		send_data(0x10);
		break;
	case LCD_DIRECT_MIR_HV:
		send_data(0xE0);
		break;
	default:
		LCD_PRINT("unknown lcd direction!\n");
		send_data(0x0);
		direction = LCD_DIRECT_NORMAL;
		break;
	}

	self->direction = direction;
	
	return 0;
}
#if 0
static int32_t hx8357_set_direction(struct lcd_spec *self, uint16_t direction)
{
	Send_cmd_data send_cmd_data = self->info.mcu->ops->send_cmd_data;

	LCD_PRINT("hx8357_set_direction\n");

	switch (direction) {
	case LCD_DIRECT_NORMAL:
		send_cmd_data(0x0016, 0x0009);
		break;
	case LCD_DIRECT_ROT_90:
		send_cmd_data(0x0016, 0x0069);
		break;
	case LCD_DIRECT_ROT_180:
		send_cmd_data(0x0016, 0x00c9);
		break;
	case LCD_DIRECT_ROT_270:
		send_cmd_data(0x0016, 0x00a9);
		break;
	case LCD_DIRECT_MIR_H:
		send_cmd_data(0x0016, 0x00c8);
		break;
	case LCD_DIRECT_MIR_V:
		send_cmd_data(0x0016, 0x0008);
		break;
	case LCD_DIRECT_MIR_HV:
		send_cmd_data(0x0016, 0x0048);
		break;
	default:
		LCD_PRINT("unknown lcd direction!\n");
		send_cmd_data(0x0016, 0x0009);
		direction = LCD_DIRECT_NORMAL;
		break;
	}

	self->direction = direction;
	
	return 0;
}
#endif
static int32_t hx8357_enter_sleep(struct lcd_spec *self, uint8_t is_sleep)
{
	Send_data send_cmd = self->info.mcu->ops->send_cmd;

	if(is_sleep) {
		//Sleep In
		send_cmd(0x28);
		LCD_DelayMS(120); 
		send_cmd(0x10);
		LCD_DelayMS(120); 
	}
	else {
		//Sleep Out
		send_cmd(0x11);
		LCD_DelayMS(120); 
		send_cmd(0x29);
		LCD_DelayMS(120); 
	}
	return 0;
}
#if 0
static int32_t hx8357_enter_sleep(struct lcd_spec *self, uint8_t is_sleep)
{
	Send_cmd_data send_cmd_data = self->info.mcu->ops->send_cmd_data;

	if(is_sleep) {
		send_cmd_data(0x00FF, 0x00);//Select Command Page 0
		// Display off Setting
		send_cmd_data(0x0028, 0x38); // GON=1, DTE=1, D[1:0]=10
		LCD_DelayMS(40);  //delay	
		send_cmd_data(0x0028, 0x04); // GON=0, DTE=0, D[1:0]=01
		// Power off Setting
		send_cmd_data(0x001F, 0x90); // Stop VCOMG
		// GAS_EN=1, VCOMG=0, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
		LCD_DelayMS(5);  //delay	
		send_cmd_data(0x001F, 0x88);// Stop step-up circuit
		// GAS_EN=1, VCOMG=1, PON=0, DK=1, XDK=1, DDVDH_TRI=0, STB=0
		send_cmd_data(0x001C, 0x00);// AP=000
		send_cmd_data(0x001F, 0x89);// Enter Standby mode
		//GAS_EN=1, VCOMG=0, PON=0, DK=1, XDK=0, DDVDH_TRI=0, STB=1
		send_cmd_data(0x0019, 0x00);//OSC_EN=0, Stop to Oscillate
	}
	else {
		send_cmd_data(0x00FF, 0x00);//Select Command Page 0
		send_cmd_data(0x0019, 0x01);//OSC_EN=1, Start to Oscillate
		LCD_DelayMS(5);  //delay
		send_cmd_data(0x001F, 0x88);
		//GAS_EN=1, VCOMG=0, PON=0, DK=1, XDK=0, DDVDH_TRI=0, STB=0
		// Power on Setting
		send_cmd_data(0x001C, 0x03);// AP=011
		send_cmd_data(0x001F, 0x80);// Exit standby mode and Step-up circuit 1 enable
		// GAS_EN=1, VCOMG=0, PON=0, DK=0, XDK=0, DDVDH_TRI=0, STB=0
		LCD_DelayMS(5);  //delay
		send_cmd_data(0x001F, 0x90);// Step-up circuit 2 enable
		// GAS_EN=1, VCOMG=0, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
		LCD_DelayMS(5);  //delay
		send_cmd_data(0x001F, 0xD4);
		// GAS_EN=1, VCOMG=1, PON=1, DK=0, XDK=1, DDVDH_TRI=0, STB=0
		LCD_DelayMS(5);  //delay
		// Display on Setting
		send_cmd_data(0x0028, 0x08);// GON=0, DTE=0, D[1:0]=01
		LCD_DelayMS(40);  //delay
		send_cmd_data(0x0028, 0x38);// GON=1, DTE=1, D[1:0]=10
		LCD_DelayMS(40);  //delay
		send_cmd_data(0x0028, 0x3C);// GON=1, DTE=1, D[1:0]=11
	}
	return 0;
}
#endif	
static int32_t hx8357_read_id(struct lcd_spec *self)
{
	Send_data send_cmd = self->info.mcu->ops->send_cmd;
	Send_data send_data = self->info.mcu->ops->send_data;
	Read_data read_data = self->info.mcu->ops->read_data;
	int32_t   chip_id = 0;

	send_cmd(0xB9);
	send_data(0xFF);
	send_data(0x83);
	send_data(0x57);
	send_cmd(0xC2);
	send_cmd(0x30);
	
	LCD_DelayMS(10);
	send_cmd(0xD0);
	chip_id = read_data();
	LCD_PRINT("hx8357_read_id1  u-boot chip_id = 0x%x!\n", chip_id);
	chip_id = read_data();
	LCD_PRINT("hx8357_read_id2  u-boot chip_id = 0x%x!\n", chip_id);
	if(0x99 == chip_id){
		chip_id=0x57;
	}
	return chip_id;
}

static struct lcd_operations lcd_hx8357_operations = {
	.lcd_init = hx8357_init,
	.lcd_set_window = hx8357_set_window,
	.lcd_invalidate_rect= hx8357_invalidate_rect,
	.lcd_invalidate = hx8357_invalidate,
	.lcd_set_direction = hx8357_set_direction,
	.lcd_enter_sleep = hx8357_enter_sleep,
	.lcd_readid = hx8357_read_id,
};

static struct timing_mcu lcd_hx8357_timing[] = {
[LCD_REGISTER_TIMING] = {                    // read/write register timing
		.rcss = 15,  // 15ns
		.rlpw = 60,
		.rhpw = 60,
		.wcss = 10,
		.wlpw = 35,
		.whpw = 35,
	},
[LCD_GRAM_TIMING] = {                    // read/write gram timing
		.rcss = 15,  // 15ns
		.rlpw = 60,
		.rhpw = 60,
		.wcss = 10,
		.wlpw = 35,
		.whpw = 35,
	},
};

static struct info_mcu lcd_hx8357_info = {
	.bus_mode = LCD_BUS_8080,
	.bus_width = 16,
	.timing = lcd_hx8357_timing,
	.ops = NULL,
};

struct lcd_spec lcd_panel_hx8357 = {
	.width = 320,
	.height = 480,
	.mode = LCD_MODE_MCU,
	.direction = LCD_DIRECT_NORMAL,
	.info = {.mcu = &lcd_hx8357_info},
	.ops = &lcd_hx8357_operations,
};
