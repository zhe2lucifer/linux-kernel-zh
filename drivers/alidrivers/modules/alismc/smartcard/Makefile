#
# Makefile for the ali smart card driver
#

KBUILD_CFLAGS += -Werror

obj-$(CONFIG_ALI_M36_SMARTCARD) += smartcard.o
smartcard-y := ali_smartcard_main.o ali_smartcard_dev.o ali_smartcard_gpio.o \
			   ali_smartcard_t1.o ali_smartcard_txrx.o ali_smartcard_misc.o \
			   ali_smartcard_irq.o ali_smartcard_atr.o
smartcard-$(CONFIG_PM)          += ali_smartcard_pm.o
