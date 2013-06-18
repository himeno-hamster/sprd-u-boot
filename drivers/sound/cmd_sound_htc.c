/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <ubi_uboot.h>
#include <asm/io.h>

#include "sound_common.h"

#define pr_fmt(fmt) "[sound: htc ]" fmt
#ifdef	CMD_SND_DEBUG
#define	snd_dbg(fmt,args...)	pr_info(fmt ,##args)
#else
#define snd_dbg(fmt,args...)
#endif

enum {
	SOUND_DIGITAL_LOOP = 0x01,
	SOUND_ANALOG_LOOP = 0x02,
};

enum {
	SOUND_MAIN_MIC = 0x01,
	SOUND_AUX_MIC = 0x02,
	SOUND_HEADSET_MIC = 0x04,
	SOUND_AIL = 0x08,
	SOUND_AIR = 0x10,
};

enum {
	SOUND_SPEAKER = 0x01,
	SOUND_EARPIECE = 0x02,
	SOUND_HEADPHONE = 0x04,
	SOUND_SPEAKER2 = 0x08,
};

static inline char *_2str(int enable)
{
	return enable ? "enable" : "disable";
}

static inline int is_analog_loop(int loop)
{
	return loop & SOUND_ANALOG_LOOP;
}

static int speaker_pa_enable(int on)
{
	sprd_inter_speaker_pa(on);
	return 0;
}

static int hp_pa_enable(int on)
{
#ifdef CONFIG_SOUND_CODEC_SPRD_V3
	sprd_inter_headphone_pa(on);
#endif
	return 0;
}

static int audio_codec(int enable, int loop, int drv_in, int drv_out)
{
	static int s_loop = 0;
	static int s_drv_in = 0;
	static int s_drv_out = 0;
	static int s_enable = 0;
	static int s_adc_power = 0;
	static int s_dac_power = 0;
	int adc_need_power_on = 0;
	int dac_need_power_on = 0;

	if (enable == 0xA) {
		drv_in = s_drv_in;
		drv_out = s_drv_out;
		loop = s_loop;
		s_loop = 0;
		s_drv_in = 0;
		s_drv_out = 0;
		enable = 0;
		printf("force disable all\n");
	} else {
		enable -= 0xD;
		if (enable) {
			s_loop |= loop;
			s_drv_in |= drv_in;
			s_drv_out |= drv_out;
			if (loop != s_loop) {
				printf
				    ("warnning: had some other loop path enable\n");
			}
			if (drv_in != s_drv_in) {
				printf
				    ("warnning: had some other driver in opened\n");
			}
			if (drv_out != s_drv_out) {
				printf
				    ("warnning: had some other driver out opened\n");
			}
		} else {
			s_loop &= ~loop;
			s_drv_in &= ~drv_in;
			s_drv_out &= ~drv_out;
			if ((s_drv_in == 0) && (s_drv_out != 0)) {
				printf
				    ("warnning: can't loop by driver in all disable\n");
				drv_out |= s_drv_out;
				s_drv_out = 0;
				loop |= s_loop;
				s_loop = 0;
			}
			if ((s_drv_out == 0) && (s_drv_in != 0)) {
				printf
				    ("warnning: can't loop by driver out all disable\n");
				drv_in |= s_drv_in;
				s_drv_in = 0;
				loop |= s_loop;
				s_loop = 0;
			}
		}
	}
	if (s_drv_in) {
		adc_need_power_on = 1;
	}
	if (s_drv_out) {
		dac_need_power_on = 1;
	}

	if (enable) {
		if (adc_need_power_on != s_adc_power) {
			adcl_switch(enable);
			adcr_switch(enable);
			if (!is_analog_loop(loop)) {
				adcl_digital_switch(enable);
				adcr_digital_switch(enable);
			}
			s_adc_power = adc_need_power_on;
		}

		if (dac_need_power_on != s_dac_power) {
			if (!is_analog_loop(loop)) {
				dacl_digital_switch(enable);
				dacr_digital_switch(enable);
			}
			s_dac_power = dac_need_power_on;
		}
	}

	if (drv_in & SOUND_MAIN_MIC) {
		printf("main mic %s\n", _2str(enable));
		mic_bias_enable(SPRD_CODEC_MIC_BIAS, enable);
		mixer_set(ID_FUN(SPRD_CODEC_MAIN_MIC, SPRD_CODEC_LEFT), enable);
		mixer_set(ID_FUN(SPRD_CODEC_MAIN_MIC, SPRD_CODEC_RIGHT),
			  enable);
	}
	if (drv_in & SOUND_AUX_MIC) {
		printf("aux mic %s\n", _2str(enable));
		mic_bias_enable(SPRD_CODEC_AUXMIC_BIAS, enable);
		mixer_set(ID_FUN(SPRD_CODEC_AUX_MIC, SPRD_CODEC_LEFT), enable);
		mixer_set(ID_FUN(SPRD_CODEC_AUX_MIC, SPRD_CODEC_RIGHT), enable);
	}
	if (drv_in & SOUND_HEADSET_MIC) {
		printf("headset mic %s\n", _2str(enable));
		mixer_set(ID_FUN(SPRD_CODEC_HP_MIC, SPRD_CODEC_LEFT), enable);
		mixer_set(ID_FUN(SPRD_CODEC_HP_MIC, SPRD_CODEC_RIGHT), enable);
	}
	if (drv_in & SOUND_AIL) {
		printf("analog left signal %s\n", _2str(enable));
		mixer_set(ID_FUN(SPRD_CODEC_AIL, SPRD_CODEC_LEFT), enable);
		mixer_set(ID_FUN(SPRD_CODEC_AIL, SPRD_CODEC_RIGHT), enable);
	}
	if (drv_in & SOUND_AIR) {
		printf("analog right signal %s\n", _2str(enable));
		mixer_set(ID_FUN(SPRD_CODEC_AIR, SPRD_CODEC_LEFT), enable);
		mixer_set(ID_FUN(SPRD_CODEC_AIR, SPRD_CODEC_RIGHT), enable);
	}

	if (s_drv_in) {
		pga_enable(SPRD_CODEC_PGA_ADCL, 0x0A, enable);
		pga_enable(SPRD_CODEC_PGA_ADCR, 0x0A, enable);
	}

	if (drv_out & SOUND_SPEAKER) {
		printf("speaker %s\n", _2str(enable));
		spkl_switch(enable);
		if (is_analog_loop(loop)) {
			mixer_set(ID_FUN(SPRD_CODEC_SPK_ADCL, SPRD_CODEC_LEFT),
				  enable);
			mixer_set(ID_FUN(SPRD_CODEC_SPK_ADCR, SPRD_CODEC_LEFT),
				  enable);
		} else {
			mixer_set(ID_FUN(SPRD_CODEC_SPK_DACL, SPRD_CODEC_LEFT),
				  enable);
			mixer_set(ID_FUN(SPRD_CODEC_SPK_DACR, SPRD_CODEC_LEFT),
				  enable);
		}
		pga_enable(SPRD_CODEC_PGA_SPKL, 0x03, enable);
		speaker_pa_enable(enable);
	}
	if (drv_out & SOUND_EARPIECE) {
		printf("earpiece %s\n", _2str(enable));
		ear_switch(enable);
		if (is_analog_loop(loop)) {
			printf("[err]earpiece can't support analog loop\n");
		} else {
			mixer_enable(ID_FUN
				     (SPRD_CODEC_EAR_DACL, SPRD_CODEC_LEFT),
				     enable);
		}
		pga_enable(SPRD_CODEC_PGA_EAR, 0x0F, enable);
	}
	if (drv_out & SOUND_HEADPHONE) {
		printf("headphone %s\n", _2str(enable));
		hp_switch(enable);
		if (is_analog_loop(loop)) {
			mixer_set(ID_FUN(SPRD_CODEC_HP_ADCL, SPRD_CODEC_LEFT),
				  enable);
			mixer_set(ID_FUN(SPRD_CODEC_HP_ADCR, SPRD_CODEC_RIGHT),
				  enable);
		} else {
			mixer_set(ID_FUN(SPRD_CODEC_HP_DACL, SPRD_CODEC_LEFT),
				  enable);
			mixer_set(ID_FUN(SPRD_CODEC_HP_DACR, SPRD_CODEC_RIGHT),
				  enable);
		}
		pga_enable(SPRD_CODEC_PGA_HPL, 0x0C, enable);
		pga_enable(SPRD_CODEC_PGA_HPR, 0x0C, enable);
		hp_pa_enable(enable);
	}
	if (drv_out & SOUND_SPEAKER2) {
		printf("speaker2 %s\n", _2str(enable));
		spkr_switch(enable);
		if (is_analog_loop(loop)) {
			mixer_set(ID_FUN(SPRD_CODEC_SPK_ADCL, SPRD_CODEC_RIGHT),
				  enable);
			mixer_set(ID_FUN(SPRD_CODEC_SPK_ADCR, SPRD_CODEC_RIGHT),
				  enable);
		} else {
			mixer_set(ID_FUN(SPRD_CODEC_SPK_DACL, SPRD_CODEC_RIGHT),
				  enable);
			mixer_set(ID_FUN(SPRD_CODEC_SPK_DACR, SPRD_CODEC_RIGHT),
				  enable);
		}
		pga_enable(SPRD_CODEC_PGA_SPKR, 0x03, enable);
	}

	if (s_drv_out) {
	}

	if (!enable) {
		if (adc_need_power_on != s_adc_power) {
			if (!is_analog_loop(loop)) {
				adcl_digital_switch(enable);
				adcr_digital_switch(enable);
			}
			adcl_switch(enable);
			adcr_switch(enable);
			s_adc_power = adc_need_power_on;
		}

		if (dac_need_power_on != s_dac_power) {
			if (!is_analog_loop(loop)) {
				dacl_digital_switch(enable);
				dacr_digital_switch(enable);
			}
			s_dac_power = dac_need_power_on;
		}
	}

	if (enable == s_enable) {
		return 0;
	} else {
		if (enable && loop) {
			printf("%s audio loop\n", _2str(enable));
			if (!is_analog_loop(loop)) {
				sprd_codec_digital_loop(enable);
			}
			s_enable = enable;
		} else {
			if ((s_drv_in == 0) && (s_drv_out == 0)) {
				printf("%s audio loop\n", _2str(enable));
				if (!is_analog_loop(loop)) {
					sprd_codec_digital_loop(enable);
				}
				s_enable = enable;
			}
		}
	}
	return 0;
}

int do_audio_codec(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int enable = 0;
	int loop = SOUND_DIGITAL_LOOP;
	int drv_in = SOUND_MAIN_MIC;
	int drv_out = SOUND_SPEAKER;

	if (argc < 2)
		return cmd_usage(cmdtp);

	enable = simple_strtoul(argv[1], NULL, 16);

	if (!((enable == 0xA) || (enable == 0xD) || (enable == 0xE))) {
		printf("enable or disable? please refer:\n");
		return cmd_usage(cmdtp);
	}

	if (argc > 2)
		drv_in = simple_strtoul(argv[2], NULL, 16);
	if (argc > 3)
		drv_out = simple_strtoul(argv[3], NULL, 16);
	if (argc > 4)
		loop = simple_strtoul(argv[4], NULL, 16);

	printf("\n");
	return audio_codec(enable, loop, drv_in, drv_out);
}

#ifdef CONFIG_SOUND_DAI_VBC_R2P0
/* audio pin mux function */

static int audio_iis_fm(int enable, int port, int drv_out)
{
	int ret = 0;
	if (ret = arch_audio_pin_func_i2s_port(port, 0)) {
		printf("error iis port %d\n", port);
		return ret;
	}
	printf("iis port %d\n", port);
	arch_audio_i2s_port_sys_sel(port, I2S_PORT_SYS_SEL_VBC);
	/* codec setting */
	ret = audio_codec(enable, 0, 0, drv_out);
	sprd_codec_pcm_set_sample_rate(1, 32000);
	sprd_codec_pcm_set_sample_rate(0, 32000);

	if (enable == 0xA) {
		enable = 0;
	} else {
		enable -= 0xD;
	}
	/* vbc setting */
	if (enable) {
		vbc_startup(SNDRV_PCM_STREAM_PLAYBACK);
		vbc_startup(SNDRV_PCM_STREAM_CAPTURE);
		vbc_adc_sel_iis(2);
		vbc_dac0_fm_mixer(1);
		vbc_dac1_fm_mixer(1);
		vbc_trigger(SNDRV_PCM_STREAM_PLAYBACK, enable);
		vbc_trigger(SNDRV_PCM_STREAM_CAPTURE, enable);
	} else {
		vbc_trigger(SNDRV_PCM_STREAM_PLAYBACK, enable);
		vbc_trigger(SNDRV_PCM_STREAM_CAPTURE, enable);
		vbc_dac0_fm_mixer(0);
		vbc_dac1_fm_mixer(0);
		vbc_adc_sel_iis(0);
		vbc_shutdown(SNDRV_PCM_STREAM_PLAYBACK);
		vbc_shutdown(SNDRV_PCM_STREAM_CAPTURE);
	}

	return ret;
}


int do_audio_iis_fm(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int enable = 0;
	int port = 3;
	int drv_out = SOUND_SPEAKER;

	if (argc < 2)
		return cmd_usage(cmdtp);

	enable = simple_strtoul(argv[1], NULL, 16);

	if (!((enable == 0xA) || (enable == 0xD) || (enable == 0xE))) {
		printf("enable or disable? please refer:\n");
		return cmd_usage(cmdtp);
	}

	if (argc > 2)
		port = simple_strtoul(argv[2], NULL, 16);
	if (argc > 3)
		drv_out = simple_strtoul(argv[3], NULL, 16);

	printf("\n");
	return audio_iis_fm(enable, port, drv_out);
}
#endif

int do_func(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	return 0;
}

/* command define */

U_BOOT_CMD(audio_codec, 5, 1, do_audio_codec,
	   "switch audio analog route",
	   "\n"
	   "	- switch audio analog route, switch many driver in/out\n"
	   "audio_codec enable [driver_in] [driver_out] [loop]\n"
	   "	enable    - enable or diable audio loop\n"
	   "	    d  	  - disable\n"
	   "	    e  	  - enable\n"
	   "	    a  	  - force all disable\n"
	   "	driver_in - select driver in\n"
	   "	    0x01  - main mic(d)\n"
	   "	    0x02  - aux mic\n"
	   "	    0x04  - headset mic\n"
	   "	    0x08  - ail\n"
	   "	    0x10  - air\n"
	   "	driver_out- select driver out\n"
	   "	    0x01  - speaker(d)\n"
	   "	    0x02  - earpiece\n"
	   "	    0x04  - headphone\n"
	   "	    0x08  - speaker2\n"
	   "	loop 	  - select loop\n"
	   "	    0x01  - digital loop(d)\n"
	   "	    0x02  - analog loop\n"
);

#ifdef CONFIG_SOUND_DAI_VBC_R2P0
U_BOOT_CMD(audio_iis_fm, 4, 1, do_audio_iis_fm,
	   "select iis port to play fm",
	   "\n"
	   "	- select iis port to play fm with vbc fm mixer\n"
	   "audio_iis_fm enable [port] [driver_out]\n"
	   "	enable    - enable or diable audio loop\n"
	   "	    d  	  - disable\n"
	   "	    e  	  - enable\n"
	   "	    a  	  - force all disable\n"
	   "	port 	  - select iis port\n"
	   "	    0  	  - iis port 0\n"
	   "	    1  	  - iis port 1\n"
	   "	    2  	  - iis port 2\n"
	   "	    3  	  - iis port 3(d)\n"
	   "	driver_out- select driver out\n"
	   "	    0x01  - speaker(d)\n"
	   "	    0x02  - earpiece\n"
	   "	    0x04  - headphone\n"
	   "	    0x08  - speaker2\n"
);
#endif
