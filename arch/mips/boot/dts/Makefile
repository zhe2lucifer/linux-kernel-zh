# Built-in dtb

#default platform
builtindtb-y		:= ali_3515a

ifneq ($(CONFIG_ALI_BUILTIN_DTB_NAME),"")
	builtindtb-y	:= $(patsubst "%",%,$(CONFIG_ALI_BUILTIN_DTB_NAME))
endif

obj-y   += $(builtindtb-y).dtb.o
targets += $(builtindtb-y).dtb

.SECONDARY: $(obj)/$(builtindtb-y).dtb.S

dtbs:  $(addprefix  $(obj)/, $(builtindtb-y).dtb)

clean-files := *.dtb  *.dtb.S
