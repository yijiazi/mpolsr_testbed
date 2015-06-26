TARGET = ser_iptables
OBJS = sereadmo-module.o skb_tools.o hook_local_out.o hook_local_in.o hook_pre_routing.o \
       ser_path.o ser_device.o ser_path_djk.o ser_path_tc.o ser_path_test.o common.o


EXTRA_CFLAGS = -DEXPORT_SYMTAB
CURRENT_KERNEL = $(shell uname -r)
KERNEL_DIR = /lib/modules/$(CURRENT_KERNEL)/build
PWD = $(shell pwd)


obj-m      := $(TARGET).o

$(TARGET)-objs := $(OBJS)

default:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules


clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c *.c~ *.h~

-include $(KERNEL_DIR)/Rules.make






