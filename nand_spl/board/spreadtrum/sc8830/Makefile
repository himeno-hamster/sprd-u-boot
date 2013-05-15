#
# (C) Copyright 2009 DENX Software Engineering
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundatio; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#
CONFIG_NAND_SPL	= y

include $(TOPDIR)/config.mk
include $(TOPDIR)/nand_spl/board/$(VENDOR)/$(SOC)/config.mk

nandobj	:= $(OBJTREE)/nand_spl/
SEC_LDSCRIPT	= $(TOPDIR)/nand_spl/board/$(VENDOR)/$(SOC)/u-boot-sec.lds
SPL_LDSCRIPT	= $(TOPDIR)/nand_spl/board/$(VENDOR)/$(SOC)/u-boot-spl.lds
SEC_LDFLAGS	= -Bstatic -T $(SEC_LDSCRIPT) $(PLATFORM_LDFLAGS)
SPL_LDFLAGS	= -Bstatic -T $(SPL_LDSCRIPT) $(PLATFORM_LDFLAGS)
AFLAGS		+= -DCONFIG_PRELOADER -DCONFIG_NAND_SPL
CFLAGS		+= -DCONFIG_PRELOADER -DCONFIG_NAND_SPL -g

SPL_SOBJS	= spl_start.o _udivsi3.o _divsi3.o _umodsi3.o
SEC_SOBJS	= sec_start.o
ifdef CONFIG_EMMC_BOOT
SPL_COBJS	= emmc_boot.o mmc.o
else
SPL_COBJS	= sprd_tiger_nand_boot.o nand_ecc.o sc8830_nand.o
endif

SPL_COBJS	+= mcu.o dram_cfg.o umctl.o adi_drv.o chip_cfg.o div0.o string.o lowlevel_init.o
SEC_COBJS	+= sec_mcu.o sec_boot.o

SEC_OBJS	:= $(addprefix $(obj),$(SEC_SOBJS) $(SEC_COBJS))
SPL_OBJS	:= $(addprefix $(obj),$(SPL_SOBJS) $(SPL_COBJS))

__SEC_OBJS	:= $(SEC_COBJS) $(SEC_SOBJS)
__SPL_OBJS	:= $(SPL_COBJS) $(SPL_SOBJS)

LNDIR		:= $(OBJTREE)/nand_spl/board/$(VENDOR)/$(SOC)

ALL		= $(nandobj)u-boot-spl $(nandobj)u-boot-sec.bin $(nandobj)u-boot-spl.bin $(nandobj)u-boot-spl-16k.bin

all:$(ALL)

$(nandobj)u-boot-spl-16k.bin: $(nandobj)u-boot-spl.bin $(nandobj)u-boot-sec.bin
	cat $(nandobj)u-boot-sec.bin $(nandobj)u-boot-spl.bin >& $@ 2>&1

$(nandobj)u-boot-sec.bin:	$(nandobj)u-boot-sec
	$(OBJCOPY) ${OBJCFLAGS} --pad-to "0x50005C00" -O binary $< $@

$(nandobj)u-boot-spl.bin:	$(nandobj)u-boot-spl
	$(OBJCOPY) ${OBJCFLAGS} --pad-to "0x5000B000" -O binary $< $@

$(nandobj)u-boot-sec:	$(SEC_OBJS) $(SEC_LDSCRIPT)
	cd $(LNDIR) && $(LD) $(SEC_LDFLAGS) $(__SEC_OBJS) \
		-Map $(nandobj)u-boot-sec.map \
		-o $@

$(nandobj)u-boot-spl:	$(SPL_OBJS) $(SPL_LDSCRIPT)
	cd $(LNDIR) && $(LD) $(SPL_LDFLAGS) $(__SPL_OBJS) \
		-Map $(nandobj)u-boot-spl.map \
		-o $@

#########################################################################

$(obj)%.o:	$(SRCTREE)/$(CPUDIR)/%.S
	$(CC) $(AFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:	$(SRCTREE)/$(CPUDIR)/$(SOC)/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:	$(SRCTREE)/nand_spl/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:	$(SRCTREE)/nand_spl/board/$(VENDOR)/$(SOC)%.S
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:	$(SRCTREE)/arch/$(ARCH)/lib/%.S
	$(CC) $(AFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:      $(SRCTREE)/arch/$(ARCH)/lib/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:      $(SRCTREE)/drivers/mtd/nand/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:	$(SRCTREE)/$(CPUDIR)/$(SOC)/%.S
	$(CC) $(AFLAGS) -mlittle-endian -c -o $@ $<

$(obj)%.o:	$(SRCTREE)/drivers/serial/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<
$(obj)%.o:	$(SRCTREE)/lib/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<
$(nandobj)u-boot.lds: $(LDSCRIPT)
	$(CPP) $(CPPFLAGS) $(LDPPFLAGS) -ansi -D__ASSEMBLY__ -P - <$^ >$@
ifdef CONFIG_EMMC_BOOT
$(obj)%.o:	$(SRCTREE)/drivers/mmc/%.c
	$(CC) $(CFLAGS) -mlittle-endian -c -o $@ $<
endif
# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
