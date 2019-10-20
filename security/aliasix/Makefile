#
# Makefile for the SMACK LSM
#

ifdef CONFIG_KFT
EXTRA_CFLAGS	+= -finstrument-functions
KBUILD_CFLAGS	+= -finstrument-functions
endif

obj-$(CONFIG_SECURITY_ALIASIX) := aliasix.o

aliasix-y := aliasix_lsm.o aliasix_sha.o aliasix_rsa.o aliasix_misc.o \
						 aliasix_perm.o aliasix_daemon.o aliasix_memchk.o
