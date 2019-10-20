#
# linux/drivers/media/dvb/frontends/ali_nim/Makefile
#

EXTRA_CFLAGS	+= -D__NIM_LINUX_PLATFORM__

obj-$(CONFIG_DVB_SUPPORT) += porting_linux_common.o
obj-$(CONFIG_DVB_SUPPORT) += tun_common.o
obj-$(CONFIG_DVB_SUPPORT) += lnb_common.o


#DVBS/DVBS2 CONFIG_ALI_DVBS_AUTOSCAN
obj-$(CONFIG_ALI_DVBS_COMMON) += ali_dvbs_common/nim_s3501_autosearch.o
obj-$(CONFIG_ALI_DVBS_COMMON) += ali_dvbs_common/nim_s3501_autosearch_fft.o
obj-$(CONFIG_ALI_DVBS_COMMON) += ali_dvbs_common/R2FFT.o
obj-$(CONFIG_ALI_DVBS_COMMON) += ali_dvbs_common/nim_m3501_common_linux.o
obj-$(CONFIG_ALI_DVBS_COMMON) += ali_dvbs_common/porting_m3501_linux.o

#DVBS2 PLSN_SUPPORT
obj-$(CONFIG_ALI_PLSN) += ali_dvbs_common/nim_dvbs_c3505_plsn_plsync.o
obj-$(CONFIG_ALI_PLSN) += ali_dvbs_common/nim_dvbs_c3505_plsn_search.o

#CONFIG_M3501
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_linux.o
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_im.o
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_im_tr.o
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_im_cr.o
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_ic.o
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_ic_tr.o
obj-$(CONFIG_M3501) += dem_m3501/nim_m3501_ic_cr.o
#CONFIG_S3503
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_im.o
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_im_tr.o
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_im_cr.o
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_ic.o
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_ic_tr.o
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_ic_cr.o
obj-$(CONFIG_S3503) += dem_s3503/nim_s3503_linux.o
#CONFIG_C3505
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_data_cap.o
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_debug.o
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_hw_cfg.o
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_monitor.o
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_private.o
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_tp_scan.o
obj-$(CONFIG_C3505) += dem_c3505/nim_dvbs_c3505_linux.o

#CONFIG_C3501H
obj-$(CONFIG_C3501H) += dem_c3501h/nim_dvbs_c3501h.o
obj-$(CONFIG_C3501H) += dem_c3501h/nim_dvbs_c3501h_linux.o

#DVBC
#CONFIG_S3281
obj-$(CONFIG_S3281) += dem_s3281/nim_s3281_linux.o
obj-$(CONFIG_S3281) += dem_s3281/nim_s3281_ic.o 
obj-$(CONFIG_S3281) += dem_s3281/nim_s3281_im.o
#CONFIG_MXL214C
obj-$(CONFIG_MXL214C) += dem_mxl214/nim_mxl214c_linux.o
obj-$(CONFIG_MXL214C) += dem_mxl214/nim_mxl214c.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_XptApi.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_SerdesApi.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_PhyCtrl.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_OobApi.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_DemodApi.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_DebugApi.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_CommonApi.o
obj-$(CONFIG_MXL214C) += dem_mxl214/MxL_HRCLS_CalCtrl.o
#CONFIG_TDA10025
obj-$(CONFIG_TDA10025) += dem_tda10025/nim_tda10025_linux.o
obj-$(CONFIG_TDA10025) += dem_tda10025/nim_tda10025_im.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslHCDP.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslHCDP_Advanced.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslHCDP_Instance.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslHCDP_Reg.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslTDA10025.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslTDA10025_Cfg.o
obj-$(CONFIG_TDA10025) += dem_tda10025/tmbslTDA10025Instance.o
#CONFIG_S3202
#obj-$(CONFIG_S3202) += dem_s3202/atbm886x.o


#DVBT/DVBT2/ISDBT
obj-$(CONFIG_S3821) += dem_s3821/nim_s3821_im.o
obj-$(CONFIG_S3821) += dem_s3821/nim_s3821_ic_cr.o
obj-$(CONFIG_S3821) += dem_s3821/nim_s3821_ic_tr.o
obj-$(CONFIG_S3821) += dem_s3821/nim_s3821_linux.o
#CONFIG_CXD2837
obj-$(CONFIG_CXD2837) += dem_cxd2837/nim_cxd2837_linux.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/nim_cxd2837_im.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_common.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_dvbc.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_dvbc_monitor.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_dvbt.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_dvbt_monitor.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_dvbt2.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_dvbt2_monitor.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_i2c.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_demod_integ.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_math.o
obj-$(CONFIG_CXD2837) += dem_cxd2837/sony_stopwatch_port.o
#CONFIG_CXD2838
obj-$(CONFIG_CXD2838) += dem_cxd2838/nim_cxd2838_linux.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/nim_cxd2838_im.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_common.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_demod.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_demod_i2c.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_demod_isdbt.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_integ_isdbt.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_demod_isdbt_monitor.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_math.o
obj-$(CONFIG_CXD2838) += dem_cxd2838/sony_stopwatch_port.o

#CONFIG_CXD2856
obj-$(CONFIG_CXD2856) += dem_cxd2856/

#TUNER DRIVER

obj-$(CONFIG_MXL603) += tun_mxl603/MxL603_TunerApi.o
obj-$(CONFIG_MXL603) += tun_mxl603/MxL603_TunerCfg.o
obj-$(CONFIG_MXL603) += tun_mxl603/tun_mxl603.o
#CONFIG_SHARP_VZ7306
obj-$(CONFIG_SHARP_VZ7306) += tun_sharp/sharp_vz7306.o
#CONFIG_AV2012
obj-$(CONFIG_AV2012) += tun_av2012/tun_av2011.o
#CONFIG_M3031
obj-$(CONFIG_M3031) += tun_m3031/tun_m3031.o
#CONFIG_RDA5815M
obj-$(CONFIG_RDA5815M) += tun_rda5815m/nim_rda5815m.o
#CONFIG_TDA18250
obj-$(CONFIG_TDA18250) += tun_tda18250/System_Lo_Detuning.o
obj-$(CONFIG_TDA18250) += tun_tda18250/tmbslTDA18250.o
obj-$(CONFIG_TDA18250) += tun_tda18250/tmbslTDA18250AdvanceCustom.o
obj-$(CONFIG_TDA18250) += tun_tda18250/tmbslTDA18250Instance.o
obj-$(CONFIG_TDA18250) += tun_tda18250/tmddTDA18250.o
obj-$(CONFIG_TDA18250) += tun_tda18250/tmddTDA18250Instance.o
obj-$(CONFIG_TDA18250) += tun_tda18250/tun_tda18250.o
#CONFIG_TDA18250_AB
obj-$(CONFIG_TDA18250_AB) += tun_tda18250ab/tmbslTDA18250A.o
obj-$(CONFIG_TDA18250_AB) += tun_tda18250ab/tmbslTDA18250A_Advanced.o
obj-$(CONFIG_TDA18250_AB) += tun_tda18250ab/tun_tda18250ab.o
#CONFIG_MXL203
#obj-$(CONFIG_MXL203) += tun_mxl203/MxL_User_Define.o
#obj-$(CONFIG_MXL203) += tun_mxl203/MxL203RF.o
#obj-$(CONFIG_MXL203) += tun_mxl203/MxL203RF_API.o
#obj-$(CONFIG_MXL203) += tun_mxl203/tun_mxl203rf.o
#CONFIG_RT810
obj-$(CONFIG_RT810) += tun_rt810/RT810.o
obj-$(CONFIG_RT810) += tun_rt810/tun_rt810.o
#CONFIG_RT820C
obj-$(CONFIG_RT820C) += tun_rt820c/R820C.o
obj-$(CONFIG_RT820C) += tun_rt820c/tun_rt820.o
#CONFIG_CXD2872
obj-$(CONFIG_CXD2872) += tun_cxd2872/sony_tuner_common.o
obj-$(CONFIG_CXD2872) += tun_cxd2872/sony_tuner_ascot3.o
obj-$(CONFIG_CXD2872) += tun_cxd2872/sony_tuner_i2c.o
obj-$(CONFIG_CXD2872) += tun_cxd2872/sony_ascot3.o
#CONFIG_SI2141_DVBT
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/tun_si2141.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Si2141_44_24_L1_API.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Si2141_44_24_L1_Commands.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Si2141_44_24_L1_Properties.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Si2141_44_24_L2_API.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Si2141_44_24_Properties_Strings.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Si2141_44_24_User_Properties.o
obj-$(CONFIG_SI2141_DVBT) += tun_si2141/Silabs_L0_Connection.o
#CONFIG_SI2141_ISDBT
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/tun_si2141.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Si2141_44_24_L1_API.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Si2141_44_24_L1_Commands.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Si2141_44_24_L1_Properties.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Si2141_44_24_L2_API.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Si2141_44_24_Properties_Strings.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Si2141_44_24_User_Properties.o
obj-$(CONFIG_SI2141_ISDBT) += tun_si2141/Silabs_L0_Connection.o
#CONFIG_DCT70701
obj-$(CONFIG_DCT70701) += tun_dct70701/tun_dct70701.o
#CONFIG_R858
obj-$(CONFIG_R858) += tun_r858/R858.o
obj-$(CONFIG_R858) += tun_r858/tun_r858.o
#CONFIG_RT7X0
obj-$(CONFIG_RT7X0) += tun_rt7x0/tun_rt7x0.o
obj-$(CONFIG_RT7X0) += tun_rt7x0/RT710.o

#CONFIG_TUN_R836
obj-$(CONFIG_TUN_R836) += tun_r836/R836.o
obj-$(CONFIG_TUN_R836) += tun_r836/tun_r836.o


#lnb driver 
#CONFIG_LNB_TPS65233
obj-$(CONFIG_LNB_TPS65233) += lnb_tps65233/lnb_tps65233.o

#CONFIG_LNB_A8304
obj-$(CONFIG_LNB_A8304) += lnb_a8304/lnb_a8304.o


