#CFLAGS = -g -O1 -EL
CFLAGS = -g -O1

MAKETOOLS = ../../../../../../../host/opt/ext-toolchain/bin/mips-linux-gnu-

CC=$(MAKETOOLS)gcc

ip_bw_test: ip_bw_test.c
	$(CC) -I../../../include/linux -DCONFIG_ALI_CHIP_3922 -EL -o $@ $<


install:
	cp -f ip_bw_test ../../../../../../../target/usr/bin

clean:
	rm -f ip_bw_test
