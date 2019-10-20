
ifdef CONFIG_KFT
EXTRA_CFLAGS	+= -finstrument-functions
KBUILD_CFLAGS	+= -finstrument-functions
endif

obj-y := init.o console.o time.o cmdline.o memory.o m36_platform.o m36_irq.o m36_setup.o\
		board_config.o m36_setup_board.o 

obj-$(CONFIG_ALI_STANDBY_TO_RAM) += sleepdep.o sleep.o ali_suspend.o

ifdef CONFIG_OF
obj-y += of.o
endif

EXTRA_CFLAGS += -Werror
