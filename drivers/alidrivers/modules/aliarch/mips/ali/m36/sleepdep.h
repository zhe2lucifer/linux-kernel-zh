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
#include <ali_board_config.h>
//=======================================================================================//

#define ALI_IRC_BASE                                            (0x18018100)
#define INFRA_IRCCFG                                            (ALI_IRC_BASE + 0x00)
#define INFRA_FIFOCTRL                                         (ALI_IRC_BASE + 0x01)
#define INFRA_TIMETHR                                          (ALI_IRC_BASE + 0x02)
#define INFRA_NOISETHR                                        (ALI_IRC_BASE + 0x03)
#define INFRA_IER                                                  (ALI_IRC_BASE + 0x06)
#define INFRA_ISR                                                  (ALI_IRC_BASE + 0x07)
#define INFRA_RLCBYTE                                          (ALI_IRC_BASE + 0x08)
#define INTV_REPEAT                                              (250)/*in mini second.*/
#define INTV_REPEAT_FIRST                                    (300)/*in mini second.*/
#define PAN_KEY_INVALID	                                      (0xFFFFFFFF)
#define VALUE_TOUT                                               (24000)/* Timeout threshold, in uS.*/
#define VALUE_CLK_CYC                                         (8)/*Work clock cycle, in uS.*/
#define VALUE_NOISETHR                                        (80)/* Noise threshold, in uS.*/
#define IR_RLC_SIZE                                               (256)
#define IO_BASE_ADDR                                           (0x18000000)
#define AUDIO_IO_BASE_ADDR                                (0x18002000)
#define USB_IO_BASE_ADDR                                    (0x1803D800)
#define HDMI_PHY                                                  (0x1800006C)
#define SYS_IC_NB_BASE_H                                    (0x1800)
#define SYS_IC_NB_CFG_SEQ0                                (0x74)
#define SYS_IC_NB_BIT1033                                   (0x1033)
#define SYS_IC_NB_BIT1031                                   (0x1031)
#define ALI_STR_DEBUG_ENABLE
#define DDR_VERIFY_LEN                                        (0x4000)
#define PMU_STANDBY_SAVE_OFFSET                      (0x4000)
#define DDR_PHY1_BASE_ADDR                               (0x1803E000)
#define DDR_PHY2_BASE_ADDR                               (0x1803F000)
#define DDR_DM_CTRL_BASE_ADDR                          (0x18001000)
#define STR_WATCHDOG_REG_ADDR                         (0x18018504)
#define STR_INTERRUPT_ENABLE_REG1                     (0x18000038)
#define STR_INTERRUPT_ENABLE_REG2                     (0x1800003C)
#define STR_PMU_IP_RESET_REG                              (0x18000084)
#define STR_PMU_RAM_SWITCH_REG                        (0x1805D100)
#define STR_CPU_TO_MCU_IE_REG1                         (0x1805D211)
#define STR_CPU_TO_MCU_IE_REG2                         (0x1805D210)
//#define ALI_STR_PRINT_ENABLE
#define DISABLE_ALI_WATCHDOG                            do {__REG32ALI(0x18018504) = 0x0;} while(0)
#define ALI_STR_RESUME_ADDR                               (0x1805400C)
//#define ALI_STR_DDR_CHECK_ENABLE
#define PMU_SRAM_BASE_ADDR                               (0x18050000)
#define STR_STANDBYBIN_BASE_ADDR                     (0x18055000)
#define DDR_PHY1_REG_ADDR                                 (0x1803E000)
#define DDR_PHY1_BACKUP_ADDR                           (0x18054180)
#define DDR_CTRL_REG_ADDR                                  (0x18001000)
#define DDR_CTRL_BACKUP_ADDR                            (0xB8054030)

#define PWRDM_POWER_OFF                                   (0x0)
#define PWRDM_POWER_RET                                    (0x1)
#define PWRDM_POWER_INACTIVE                           (0x2)
#define PWRDM_POWER_ON                                     (0x3)
#define PM_ENABLE_DEVICE                                    (1)
#define PM_DISABLE_DEVICE                                   (0)
#define PM_ENTER_STANDBY                                    (1)
#define PM_EXIT_STANDBY                                      (0)
#define SEE_RUN_ADDR                                           (__G_ALI_MM_PRIVATE_AREA_START_ADDR + 0x200)
#define EXIT_DDR_SELF_REFRESH_FLAG                   (0xDEADDEAD)
#define SEEBL_LOCATION_ADDR_REG                       (0xB80402B4)
#define SEEBL_LEN_REG                                          (0xB80402B8)
#define SEEBL_RUNNING_ADDR_REG                         (0xB80402B0)
#define SEEBL_SIGN_ADDR_REG                               (0xB80402BC)
#define SEE_SW_ENETR_ADDR_REG                          (0xB80402C0)
#define SEE_ENTER_ADDR_REG                                (0xB8000200)

/*Useful MACROs.*/
#define SYNC()                                                       do {asm volatile ("sync; ehb");}while(0)
#define SDBBP()                                                     do {asm volatile (".word 0x7000003f; nop");}while(0)

#define ALI_OTP_BASE                                            (0x18042000)
#define OTP_ADDR_REG                                           (0x4)
#define OTP_READ_TRIG_REG                                   (0xC)
#define OTP_READ_STATUS_REG                               (0x10)
#define OTP_VALUE_REG                                          (0x18)
#define GET_OTP_READ_STATUS                               __REG32ALI(ALI_OTP_BASE+OTP_READ_STATUS_REG)
#define OTP_READ_BUSY                                         (0x100)
#define SEE_TEXT_LEN_REG                                     (0x18055FF4)
#define DDR_PARA_BACKUP_ADDR                           (0x18058000)
#define SEE_CMAC_BACKUP_ADDR                           (0x18059000)
#define MAIN_CMAC_BACKUP_ADDR                         DDR_PARA_BACKUP_ADDR
#define MAIN_CMAC_SAVE_ADDR                             (0xB8059600)
#define SEE_CMAC_SAVE_ADDR                               0xA0000120
#define WHOLE_AREA_SIGN_ADDR                           (0xC00F4200-0x20)
#define DDR_ODT_REG                                             (0x18001030)
#define DDR_ODT_1033                                           (0x18001033)
#define ALI_UART_BASE_ADDR                                 (0x18018300)
#define SCI_16550_URBR                                         (0)
#define SCI_16550_UTBR                                         (0)
#define SCI_16550_UIER                                          (1)
#define SCI_16550_UIIR                                          (2)
#define SCI_16550_UFCR                                         (2)
#define SCI_16550_UDLL                                         (0)
#define SCI_16550_UDLM                                        (1)
#define SCI_16550_ULCR                                         (3)
#define SCI_16550_UMCR                                        (4)
#define SCI_16550_ULSR                                         (5)
#define SCI_16550_UMSR                                        (6)
#define SCI_16550_USCR                                        (7)
#define SCI_16550_DEVC                                        (8)
#define SCI_16550_RCVP                                        (9)
#define PMU_MCU_RESET_ENABLE                            (1<<25)
#define PMU_SW_RESET_ENABLE                             (1<<24)
#define PMU_MCU_RESET_DISABLE                          (~(1<<25))
#define PMU_SW_RESET_DISABLE                            (~(1<<24))
#define ALI_SB_TIMER_BASE                                   (0x18018A00)
#define M3505_CHIP_ID                                          (0x35050000)
#define AUX_RETURN_BACK_ADDR                            (0xB8054004)
#define MEM_CLK_TRRIGER                                      (1<<21)
#define CPU_CLK_TRIGGER                                      (1<<22)
#define MEM_CLK_SETTING_BIT                               (0xF0)
#define STRAP_INFO_REG                                        (0x18000070)
#define LINUX_KERNEL_RETURN_SAVE_ADDR            (0x1805400C)
//=======================================================================================//

extern void operate_device(int enable);
extern void ali_ip_interrupt_restore(void);
#endif
