# Scramber Core driver
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

obj-$(CONFIG_ALI_SCR) += aliscr.o
aliscr-y := ca_scr_core.o ca_scr_ioctl.o ca_scr_rpc.o \
	ca_scr_sbm.o ca_scr_session_engine.o

aliscr-$(CONFIG_SCR_LEGACY_IOCTL) += ca_scr_ioctl_legacy.o
aliscr-$(CONFIG_DEBUG_FS) += ca_scr_dbgfs.o
aliscr-$(CONFIG_SYSFS) += ca_scr_sysfs.o

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

