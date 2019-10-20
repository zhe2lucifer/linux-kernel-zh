-include $(LINUX_DIR)/.config

ifdef CONFIG_KFT
EXTRA_CFLAGS	+= -finstrument-functions
KBUILD_CFLAGS	+= -finstrument-functions
endif

ifdef CONFIG_ENABLE_RPC
LINUXINCLUDE += \
		-Idrivers/alidrivers/include 
KBUILD_CFLAGS += -DALI_CHIPPF_ -DALI_CHIPID_ -DALI_IC_ -DALI_BOARDTYPE_ \
	-DALI_BOARDID_ -DDEV_FLASHBOOT_ -D__ALI_LINUX_KERNEL__ \
	-D__NIM_LINUX_PLATFORM__
endif

#Warning treated as error
# KBUILD_CFLAGS += -Werror

## ethernet driver ##
obj-$(CONFIG_ALI_TOE2)			+= alienet/ali_toe2/
obj-$(CONFIG_ALI_SWITCH)        += alienet/ali_switch/
obj-$(CONFIG_ALI_GMAC)        += alienet/ali_gmac/

## rpc driver ##
obj-$(CONFIG_RPC_ALI)		+= alirpc/

## C3505 pinctrl driver ##
obj-$(CONFIG_PINCTRL_ALI)    +=alipinctrl/

## sbm driver ##
obj-$(CONFIG_ALI_SBM)           += alisbm/

## pok driver ##
obj-$(CONFIG_ALI_POK)		+=alipok/

obj-$(CONFIG_ALI_TRANSPORT) += alitransport/
## hdmi driver ##
obj-$(CONFIG_HDMI_ALI)			+= alihdmi/

## hec driver ##
obj-$(CONFIG_ALI_HEC)		+=alihec/

## mmc driver ##
obj-$(CONFIG_ALI_MMC)			+= alimmc/

## video driver ##
obj-$(CONFIG_VIDEO_ALI)			+= alivideo/

## fram buffer driver ##
obj-$(CONFIG_FB_ALI)			+= alifb/

## TTX VBI driver ##
obj-$(CONFIG_ALI_VBI)			+= alivbi/
## avsync driver ##
obj-$(CONFIG_ALI_AVSYNC_RPC)			+= aliavsync/

## DVB driver ##
obj-$(CONFIG_ALI_M36_AUDIO_RPC)		+= aliaudio/
obj-$(CONFIG_ALI_M36_CIC)		+= alicic/
obj-$(CONFIG_ALI_M36_SMARTCARD)		+= alismc/smartcard/
obj-$(CONFIG_DVB_ALI_M36_DMX)		+= alidmx/
obj-$(CONFIG_DVB_ALI_M36_TSG)		+= alitsg/
obj-$(CONFIG_DVB_ALI_M36_TSI)		+= alitsi/

obj-$(CONFIG_ALI_IP_BW_TEST)      +=alitest/


## gpio i2c ##
obj-$(CONFIG_ALI_GPIO_I2C)  += alii2c/

obj-$(CONFIG_ALI_GPIO)	+= aligpio/

## DVFS driver ##
obj-$(CONFIG_ALI_DVFS)  += alidvfs/

## tuner driver ##
obj-y		+= alinim/

## scart driver ##
obj-$(CONFIG_ALI_SCART)			+= aliscart/

## soc driver ##
obj-$(CONFIG_ALI_SOC)           += alisoc/

obj-$(CONFIG_ALI_KL_FD_FRAMEWORK) += ali_kl_fd_framework/
##Descrambler driver ##
obj-$(CONFIG_ALI_DSC)			+= alidsc/

## crypto engine driver ##                  
obj-$(CONFIG_ALI_KL)			+= alikl/

## trng new driver ##
obj-$(CONFIG_ALI_TRNG)			+= alitrng/
## CERT driver ##
obj-$(CONFIG_ALI_CERT)			+= alicert/
## CF driver ##
obj-$(CONFIG_ALI_CF)			+= alicf/

obj-$(CONFIG_ALI_SHA)			+= alisha/
obj-$(CONFIG_ALI_CRC32)			+= alicrc/

## Smart Card driver ##                  
obj-$(CONFIG_ALI_SMARTCARD)		+= alismc/smartcard/

## Crypto engine driver
obj-$(CONFIG_ALI_CRYPTO)		+= alicrypto/

## ALi power management implementation ##                  
obj-$(CONFIG_ALI_PM)		+= alipm/

## ALi UART READ WRITE driver ##
obj-$(CONFIG_ALI_UART_IO)       += aliuartio/

## ir driver ##
obj-$(CONFIG_ALI_IR)           += aliir/

## panel driver ##
obj-$(CONFIG_ALI_PAN_CH455)           += alipan/
obj-$(CONFIG_ALI_PAN_CH454)           += alipan/

##ALi spi driver ##
obj-$(CONFIG_ALI_SPI)           += alispi/

##ALi spi nor driver ##
obj-$(CONFIG_ALI_SPI_NOR)           += alispinor/

## ALi PMU driver ##
obj-$(CONFIG_ALI_PMU)						+= alipmu/

## ALi OTP driver ##
obj-$(CONFIG_ALI_OTP)			+= aliotp/

## ALi TRIM driver ##
obj-$(CONFIG_ALI_TRIM)			+= alitrim/

## ALi Video Encoder driver ##
obj-$(CONFIG_ALI_ENCODER)       += aliencoder/

## ALi PVR driver ##
obj-$(CONFIG_ALI_PVR_RPC)       += alipvr/

## ALi Video Encoder driver ##
obj-$(CONFIG_ALI_WATCHDOG)       += aliwatchdog/

obj-$(CONFIG_ALI_IMAGE)       += aliimage/

## Mali graphic driver ##
obj-$(CONFIG_MALI400)	+=mali/

## UMP module for Mali graphic driver ##
obj-$(CONFIG_UMP)	+=ump/

## DirectFB and FusionSound IPC API driver ##
obj-$(CONFIG_FUSION_DEVICE) += opensrc/fusion/

## Ali VSC driver ##
obj-$(CONFIG_ALI_VSC) += alivsc/

## Ali CASI driver ##
obj-$(CONFIG_ALI_CASI) += alicasi/

## ALi SBM client driver ##
obj-$(CONFIG_ALI_SBM_CLIENT) += alisbmclient/

## ALi see bus driver ##
obj-$(CONFIG_ALI_SEE_BUS_ENABLE) += aliseebus/

##ali scr###
obj-$(CONFIG_ALI_SCR) += aliscr/

##logsm see to main##
obj-$(CONFIG_LOGSM_SEE2MAIN) += alilogsm/

## tac driver ##
obj-$(CONFIG_ALI_TAC)           += alitac/

## sec driver ##
obj-$(CONFIG_ALI_SEC)           += alisec/

obj-$(CONFIG_MULTI_RAMDISK)	+= multi_ramdisk/

obj-$(CONFIG_ALI_RT7601_WIFI)	+= alirt7601/
obj-$(CONFIG_ALI_RT5370_WIFI)	+= alirt5370/

obj-y           += alie2driver/
.PHONY : symlinks

symlinks:
	echo "make some link or other action"
