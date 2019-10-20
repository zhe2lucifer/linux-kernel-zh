#obj-m		:= see_bus.o see_bus_dsc.o see_bus_scr.o see_bus_rng.o
#see_bus-y	:= see_bus_core.o see_bus_rpc.o see_bus_sbm.o

#modules modules_install clean:
#	$(MAKE) -C $(KERNELDIR) M=$(abspath .) $@

#tags:
#	ctags *.[ch]

obj-y += aliseebus.o
aliseebus-y := see_bus_core.o see_bus_rpc.o

clean:
	rm -f *.mod.c
	rm -f *.ko .*.ko.cmd
	rm -f *.o .*.o.cmd
	rm -f modules.*
	rm -fr .tmp_versions
	