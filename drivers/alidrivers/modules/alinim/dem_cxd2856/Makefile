#
# linux/drivers/media/dvb/frontends/ali_nim/Makefile
#
#common
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/ 
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/lib/
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/dvb_terr/
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/i2c/
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/tuner/
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/ali_cxd2856_t_t2_isdbt/
LINUXINCLUDE += -Idrivers/alidrivers/modules/alinim/dem_cxd2856/isdb_terr/
#LINUXINCLUDE += -Iinclude/linux/


obj-y += i2c/sony_i2c.o
obj-y += lib/sony_math.o
obj-y += sony_common.o
obj-y += sony_demod.o
obj-y += sony_integ.o
obj-y += sony_stopwatch_port.o
#dvbt/t2/isdbt
obj-$(CONFIG_CXD2856) += ali_cxd2856_t_t2_isdbt/nim_cxd2856_linux.o
obj-$(CONFIG_CXD2856) += ali_cxd2856_t_t2_isdbt/nim_cxd2856_common.o
obj-$(CONFIG_CXD2856) += ali_cxd2856_t_t2_isdbt/nim_cxd2856_monitor.o
obj-$(CONFIG_CXD2856) += ali_cxd2856_t_t2_isdbt/nim_cxd2856_hw_cfg.o
obj-$(CONFIG_CXD2856) += ali_cxd2856_t_t2_isdbt/nim_cxd2856_channel_change.o
obj-$(CONFIG_CXD2856) += ali_cxd2856_t_t2_isdbt/nim_cxd2856_proc.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_demod_dvbt.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_demod_dvbt_monitor.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_demod_dvbt2.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_demod_dvbt2_monitor.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_integ_dvbt.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_integ_dvbt_t2.o
obj-$(CONFIG_CXD2856) += dvb_terr/sony_integ_dvbt2.o
obj-$(CONFIG_CXD2856) += isdb_terr/sony_demod_isdbt.o
obj-$(CONFIG_CXD2856) += isdb_terr/sony_demod_isdbt_monitor.o
obj-$(CONFIG_CXD2856) += isdb_terr/sony_integ_isdbt.o
#dvbc:not supported at present 
#obj-$(CONFIG_CXD2856) += dvb_cable/sony_demod_dvbc.o
#obj-$(CONFIG_CXD2856) += dvb_cable/sony_demod_dvbc_monitor.o
#obj-$(CONFIG_CXD2856) += dvb_cable/sony_demod_dvbc2.o
#obj-$(CONFIG_CXD2856) += dvb_cable/sony_demod_dvbc2_monitor.o
#obj-$(CONFIG_CXD2856) += dvb_cable/sony_integ_dvbc.o
obj-$(CONFIG_CXD2838_DVBC) += dvb_cable/sony_integ_dvbc2.o

