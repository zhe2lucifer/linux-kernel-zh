#
# Makefile for the ali hdmi driver
#

KBUILD_CFLAGS += -Werror

obj-$(CONFIG_HDMI_ALI)          += alihdmi.o

alihdmi-y := ali_hdmi.o hdmi_proc.o hdmi_interrupt.o hdmi_edid.o hdmi_reg.o

alihdmi-$(CONFIG_HDCP_ENABLE_ALI) += hdmi_hdcp.o

alihdmi-$(CONFIG_CEC_ENABLE_ALI) += hdmi_cec.o

