
include  $(TOPDIR)/config.mk
include  $(TOPDIR)/nand_fdl/board/$(VENDOR)/$(SOC)/config.mk
include  $(TOPDIR)/board/$(BOARDDIR)/config.mk
sinclude $(TOPDIR)/include/idh_config.mk

gcclibdir    := $(shell dirname `$(CC) -print-libgcc-file-name`)

LDSCRIPT      = $(TOPDIR)/nand_fdl/board/$(VENDOR)/$(SOC)/fdl1-2nd.lds
FDL1_LDFLAGS  = -Bstatic -T $(LDSCRIPT) $(PLATFORM_LDFLAGS)

AFLAGS       += -I $(OBJTREE)/include/asm/arch
CFLAGS       += -D CONFIG_FRMCHECK -I $(OBJTREE)/include/asm/arch

FDL1_PATH     = $(TOPDIR)/nand_fdl/fdl-1
COM_PATH      = $(TOPDIR)/nand_fdl/common

FDL1_COBJ_FILE= migrate.o fdl_main.o fdl_command.o
FDL1_SOBJ_FILE= init.o
FDL1_COBJS    = $(addprefix $(obj),$(FDL1_COBJ_FILE))
FDL1_SOBJS    = $(addprefix $(obj),$(FDL1_SOBJ_FILE))
FDL1_OBJS     = $(FDL1_COBJS) $(FDL1_SOBJS)

ARCH_LIB      =
DRIVERS_LIB   = $(OBJTREE)/drivers/serial/serial_sc8800x.o
GENERIC_LIB   = 

COM_OBJ_FILE  = dl_engine.o fdl_crc.o packet.o fdl_channel.o fdl_stdio.o usb_boot.o drv_usb.o virtual_com.o
COM_OBJS      = $(addprefix $(obj),$(COM_OBJ_FILE))

CPU_OBJ_FILE  = chip_cfg.o
CPU_OBJ_FILE += adi.o dram_cfg.o umctl.o mcu.o

CPU_LIB       = $(addprefix $(obj),$(CPU_OBJ_FILE))

UBOOT_LIBS    = $(ARM_LIB) $(COMMON_LIB) $(GENERIC_LIB) $(DRIVERS_LIB)

FDL1_LNDIR   := $(OBJTREE)/nand_fdl/board/$(VENDOR)/$(SOC)
fdlobj       := $(OBJTREE)/nand_fdl/
nandobj	     := $(OBJTREE)/nand_spl/
fdl1_obj     := $(fdlobj)fdl-1/src/

ALL = $(nandobj)u-boot-sec.bin $(fdlobj)fdl1-2nd.bin $(fdlobj)fdl1.bin file_size_check

all:$(ALL)

file_size_check: $(fdlobj)fdl1.bin
	./file_size_check.sh 23552 $< $(fdl1obj)fdl1-2nd.bin  $(fdlobj)fdl1-2nd

$(fdlobj)fdl1.bin : $(nandobj)u-boot-sec.bin $(fdlobj)fdl1-2nd.bin
	cat $(nandobj)u-boot-sec.bin $(fdlobj)fdl1-2nd.bin > $@

$(fdlobj)fdl1-2nd.bin : $(fdlobj)fdl1-2nd
	$(OBJCOPY) ${OBJCFLAGS} --pad-to "0x5000AC00" -O binary $< $@

$(fdlobj)fdl1-2nd: $(FDL1_OBJS) $(COM_OBJS)  $(CPU_LIB) $(UBOOT_LIBS)
	@mkdir -p $(FDL1_LNDIR)
	cd $(FDL1_LNDIR) && $(LD) $(FDL1_LDFLAGS) \
		$(FDL1_COBJ_FILE) $(FDL1_SOBJ_FILE) $(COM_OBJ_FILE) $(CPU_OBJ_FILE) $(UBOOT_LIBS) \
		-Map $(fdlobj)fdl1-2nd.map \
		-o $@ $(PLATFORM_LIBS)

#########################################################################

ifndef CONFIG_IDH_BUILD
$(obj)%.o:$(SRCTREE)/nand_fdl/common/src/%.c
	$(CC) $(CFLAGS) -I $(FDL1_PATH)/inc -I $(COM_PATH)/inc -c -o $@ $<
$(obj)%.o:$(SRCTREE)/nand_fdl/fdl-1/src/%.c
	$(CC) $(CFLAGS) -I $(FDL1_PATH)/inc -I $(COM_PATH)/inc -c -o $@ $<
$(obj)%.o:$(SRCTREE)/nand_fdl/fdl-1/src/%.S
	$(CC) $(AFLAGS) -I $(FDL1_PATH)/inc -I $(COM_PATH)/inc -c -o $@ $<
$(obj)%.o:$(SRCTREE)/arch/$(ARCH)/cpu/$(CPU)/$(SOC)/%.S
	$(CC) $(AFLAGS) -I $(FDL1_PATH)/inc -I $(COM_PATH)/inc -c -o $@ $<
$(obj)%.o:$(SRCTREE)/arch/$(ARCH)/cpu/$(CPU)/$(SOC)/%.c
	$(CC) $(AFLAGS) -I $(FDL1_PATH)/inc -I $(COM_PATH)/inc -c -o $@ $<
endif

include $(SRCTREE)/rules.mk

sinclude $(obj).depend

.PHONY:clean
clean:
	rm -f *.bin *.map fdl1.axf

#########################################################################
