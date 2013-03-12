#
# Makefile for DWC_otg Highspeed USB controller driver
#

#PWD		:= $(shell pwd)
#PORTLIB		:= $(PWD)/../dwc_common_port

include $(TOPDIR)/config.mk

LIB	:= $(obj)nvitemd.o

COBJS-y += nvitem_sync.o
COBJS-y += nvitem_server.o
COBJS-y += nvitem_packet.o
COBJS-y += nvitem_os.o
COBJS-y += nvitem_fs.o
COBJS-y += nvitem_channel.o
COBJS-y += nvitem_buf.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))


all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

$(obj)%.o:	$(obj)%.c
	$(CC) $(CFLAGS) -c -o $@ $<

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
