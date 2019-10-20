#
# Copyright (C) 2010-2012, 2014, 2016-2017 ARM Limited. All rights reserved.
# 
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
# 
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

# For each arch check: CROSS_COMPILE , KDIR , CFLAGS += -DARCH

export ARCH ?= arm
BUILD ?= debug

check_cc2 = \
	$(shell if $(1) -S -o /dev/null -xc /dev/null > /dev/null 2>&1; \
	then \
		echo "$(2)"; \
	else \
		echo "$(3)"; \
	fi ;)

# Check that required parameters are supplied.
ifeq ($(CONFIG),)
CONFIG := default
endif
ifeq ($(CPU)$(KDIR),)
$(error "KDIR or CPU must be specified.")
endif

# Get any user defined KDIR-<names> or maybe even a hardcoded KDIR
-include KDIR_CONFIGURATION

# Define host system directory
KDIR-$(shell uname -m):=/lib/modules/$(shell uname -r)/build

ifeq ($(ARCH), arm)
# when compiling for ARM we're cross compiling
#export CROSS_COMPILE ?= $(call check_cc2, arm-linux-gnueabi-gcc, arm-linux-gnueabi-, arm-none-linux-gnueabi-)
#export CROSS_COMPILE ?= $(call check_cc2, /opt/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc, /opt/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-)
#export CROSS_COMPILE ?= $(call check_cc2, ~/buildroot_S3922C_20170710A/output/host/opt/ext-toolchain/bin/arm-linux-gnueabihf-gcc, ~/buildroot_S3922C_20170710A/output/host/opt/ext-toolchain/bin/arm-linux-gnueabihf-)
endif

# look up KDIR based om CPU selection
KDIR ?= $(KDIR-$(CPU))

export CONFIG

export CONFIG_UMP := m
ifeq ($(BUILD),debug)
export CONFIG_UMP_DEBUG := y
else
export CONFIG_UMP_DEBUG := n
endif

ifeq ($(KDIR),)
$(error No KDIR found for platform $(CPU))
endif

##Added by yashi,20141112, for build with DDK
export ALI_KERNEL_MODULE_BUILD=1

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

kernelrelease:
	$(MAKE) -C $(KDIR) kernelrelease

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	$(MAKE) -C $(KDIR) M=$(CURDIR)/../mali clean
