#
# Makefile for the kernel DVB device drivers.
#

obj-$(CONFIG_ALI_M36_AUDIO_RPC) += aliaudio.o
aliaudio-y := ali_m36_audio_rpc.o ali_audio_procfs.o

ifeq ($(shell echo "$$(((($(VERSION)<<8)|$(PATCHLEVEL))>((3<<8)|6)))"), 1)
EXTRA_CFLAGS += -I$(LINUX_DIR)/drivers/media/dvb-core/
else
EXTRA_CFLAGS += -I$(LINUX_DIR)/drivers/media/dvb/dvb-core/
endif

