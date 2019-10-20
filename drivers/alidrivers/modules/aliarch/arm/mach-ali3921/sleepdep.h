/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
/*
 * sleep dependnecy.h
 * This file contains all the sleep hardware dependencies.
 *
 */
#ifndef __ALI_M36_SLEEP_DEPENDENCY_INCLUDE____H__
#define __ALI_M36_SLEEP_DEPENDENCY_INCLUDE____H__

#include <linux/suspend.h>
#include <linux/delay.h>
#include <asm/io.h>
//=================================================================================//

#define VALUE_TOUT			                            24000/*Timeout threshold, in uS.*/
#define VALUE_CLK_CYC		                            8/*Work clock cycle, in uS.*/
#define VALUE_NOISETHR		                            80/* Noise threshold, in uS.*/
#define IR_RLC_SIZE			                            256
#define IO_BASE_ADDR                                       0x18000000
#define AUDIO_IO_BASE_ADDR                            0x18002000
#define USB_IO_BASE_ADDR                               0x1803d800
#define HDMI_PHY 	                                        0x1800006c
#define SYS_IC_NB_BASE_H		                     0x1800
#define SYS_IC_NB_CFG_SEQ0                            0x74
#define SYS_IC_NB_BIT1033                               0x1033
#define SYS_IC_NB_BIT1031                               0x1031
#define PM_ENABLE_DEVICE                               1
#define PM_DISABLE_DEVICE                              0
#define PM_ENTER_STANDBY                               1
#define PM_EXIT_STANDBY                                 0
//#define ALI_STR_PRINT_ENABEL
#define DDR_PHY1_BACKUP_ADDR                       0x18054150
#define DDR_PHY1_REG_ADDR                             0x1803e000
#define DDR_PHY2_BACKUP_ADDR                       0x180541D0
#define DDR_PHY2_REG_ADDR                             0x1803f000
#define DDR_CTRL_BACKUP_ADDR                        0x18054030
#define DDR_CTRL_REG_ADDR                              0x18001000
#define ALI_STR_RESUME_ADDR                           0x1805400C

#define PMU_RAM_ADDR                                      0x18050000
#define PMU_STANDBY_SAVE_OFFSET                  0x4000
#define DDR_PHY1_BASE_ADDR                           0x1803e000
#define DDR_PHY2_BASE_ADDR                           0x1803f000
#define DDR_DM_CTRL_BASE_ADDR                      0x18001000
#define MAILBOX_SET_PANEL_KEY_EN                 0x1805d209
#define STR_OTP_BASE_ADDR                              0x18042000
#define STR_OTP_ADDR_REG                                0x4
#define STR_OTP_WRITE_VALUE_REG                    0x8
#define STR_OTP_OP_TRIG_REG                            0xc
#define STR_READ_BIT_OFFSET                            8
#define STR_WRITE_BIT_OFFSET                          0
#define STR_OTP_READ_STATUS_REG                    0x10
#define STR_OTP_VALUE_REG                               0x18
#define STR_GET_OTP_READ_STATUS                    __REG32ALI(SF_OTP_BASE_35+SF_OTP_READ_STATUS_REG)
#define STR_OTP_READ_BUSY                              0x100
#define STR_RETENTION_BIT_OFFSET                   31

#define SB_TIMER4_CNT_REG                              0x18018A40
#define SB_TIMER4_CMP_REG                             0x18018A44
#define SB_TIMER4_CTRL_REG                             0x18018A48
#define SEE_ENTER_STANDBY_CMD                      0x10
#define SEE_EXIT_STANDBY_CMD                        0x0
#define ALI_SBTIMER_BASE                                 0x18018A00

#define ALI_OTP_BASE                                        0x18042000
#define OTP_ADDR_REG                                       0x4
#define OTP_READ_TRIG_REG                               0xc
#define OTP_READ_STATUS_REG                           0x10
#define OTP_VALUE_REG                                     0x18
#define GET_OTP_READ_STATUS                           __REG32ALI(ALI_OTP_BASE+OTP_READ_STATUS_REG)
#define OTP_READ_BUSY                                     0x100
#define SEE_TEXT_LEN_REG                                 0x18055FF4
#define DDR_PARA_BACKUP_ADDR                       0x18058000
#define SEE_CMAC_BACKUP_ADDR                      0x18059000
#define MAIN_CMAC_BACKUP_ADDR                    DDR_PARA_BACKUP_ADDR
#define MAIN_CMAC_SAVE_ADDR                        0x18059400
#define ALI_STR_PRINT_ENABLE
#define ALI_NEC_IR_POWER_KEY                        0x60DF708F
#define EXIT_DDR_SELF_REFRESH_FLAG              0xdeaddead
//=================================================================================//

extern int ali_3921_enter_lowpower(unsigned int cpu, unsigned int power_state);
extern int ali_3921_finish_suspend(unsigned long cpu_state);
extern void ali_3921_cpu_resume(void);

void operate_device(int enable);
void pm_standby_prepare(int enter);
//=================================================================================//

#ifdef ALI_STR_PRINT_ENABEL
unsigned int ali_str_print_enable = 0xFFFFFFFF;
#else
unsigned int ali_str_print_enable = 0x0;
#endif
EXPORT_SYMBOL(ali_str_print_enable);
#endif
