#
# Copyright (C) ALi Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

KERNELDIR := ../../../kernel/build/linux-3.12.20

obj-$(CONFIG_ALI_KL_FD_FRAMEWORK) += alikl.o
alikl-y := ca_kl_fd_dispatch.o

modules modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(abspath .) $@

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
