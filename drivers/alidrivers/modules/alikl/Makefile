# Key Ladder Core driver
# Copyright(C) 2014 ALi Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

KERNELDIR := ../../../..

obj-$(CONFIG_ALI_KL) += alice.o
alice-y := ca_kl_core.o ca_kl_ioctl.o ca_kl_rpc.o

alice-$(CONFIG_KL_LEGACY_IOCTL) += ca_kl_ioctl_legacy.o
alice-$(CONFIG_DEBUG_FS) += ca_kl_dbgfs.o
alice-$(CONFIG_SYSFS) += ca_kl_sysfs.o
alice-$(CONFIG_ALI_VSC) += ca_kl_vsc_ioctl.o

CHECKPATCH=$(KERNELDIR)/scripts/checkpatch.pl
sources = $(wildcard *.c *.h Makefile)
checkpatch:
	$(foreach var, $(sources), $(CHECKPATCH) -f $(var);)

clean:
	rm -f *.mod.c
	rm -f *.ko .*.ko.cmd
	rm -f *.o .*.o.cmd
	rm -f modules.*
	rm -fr .tmp_versions

