#!/bin/bash

TOOLCHAIN_DIR=/opt/arm-linux-gnueabi/bin
export ALI_KERNEL_CFLAGS="-DALI_CHIPPF_ -DALI_CHIPID_ -DALI_IC_ -DALI_BOARDTYPE_ -DALI_BOARDID_ -DDEV_FLASHBOOT_ -D__ALI_LINUX_KERNEL__  -D__NIM_LINUX_PLATFORM__"

if [ -d ../alicommon/alirpcng ]; then \
echo -e "\e[42;37m Copy the alicommon files to drivers/alidrivers\e[0m" ;\
cp -rfp ../alicommon/alirpcng/inc/* drivers/alidrivers/include/alirpcng/ ;\
cp -rfp ../alicommon/alirpcng/libmcapi/* drivers/alidrivers/modules/alirpcng/libmcapi/ ;\
cp -rfp ../alicommon/alirpcng/rpc/* drivers/alidrivers/modules/alirpcng/rpc/ ;\
cp -rfp ../alicommon/alirpcng/mbx/* drivers/alidrivers/modules/alirpcng/mbx/ ; \
cp -rfp ../alicommon/alidefinition drivers/alidrivers/include ; \
fi

make -j16 ARCH=arm CROSS_COMPILE=${TOOLCHAIN_DIR}/arm-linux-gnueabi-

EXCODE=$?
if [ "$EXCODE" == "0" ]; then
    # get commit log
    if [ -f "$v_commit_log" ] ; then
        # CommitLog=`git log --since="1 day" .`
        echo "$CommitLog" >> $v_commit_log
    fi
	${TOOLCHAIN_DIR}/arm-linux-gnueabi-objcopy -O binary vmlinux main_bin.abs
    echo -e "\e[42;37m make Successfully ! \e[0m"
else
	echo -e "\e[41;33m make Failed ! \e[0m"
fi

