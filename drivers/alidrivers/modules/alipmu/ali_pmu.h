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

/****************************************************************************(I)(S)
*  History:(M)
*      	  Date        			Author         	Comment
*      	 ====        			======		=======
* 0.		2014.09.21			Chuhua		Creation
****************************************************************************/

#ifndef __INCLUDE_KERNEL_ALI_PMU_H____
#define __INCLUDE_KERNEL_ALI_PMU_H____

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <ali_pmu_common.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>

#if defined(CONFIG_ALI_CHIP_M3921)
#include "ali_pmu_bin_3921.h"
#else
	#if defined(CONFIG_ALI_CHIP_M3515)
	#include "ali_pmu_bin_3503.h"
	#else
		#if defined(CONFIG_ALI_CHIP_M3627)
			#include "ali_pmu_bin_3505.h"
		#else
			#if defined(CONFIG_ALI_CHIP_3922)
				#include "ali_pmu_bin_3922.h"
			#else
				#include "ali_pmu_bin_3821.h"
			#endif
		#endif
	#endif
#endif

#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <ali_pmu_common.h>
//======================================================================================================//

#define PMU_PRS_KEY_CFG                                                                                       (0xE010)
#define KEY_DISABLE_VAL_LOW0                                                                              (0xE018)
#define KEY_DISABLE_VAL_LOW1                                                                              (0xE019)
#define KEY_DISABLE_VAL_LOW2                                                                              (0xE01A)
#define KEY_DISABLE_EN                                                                                          (0xE01B)
#define PMU_CFG_SET                                                                                               (0xE000)
#define MCU_SYS_IPR                                                                                               (0xC021)
#define MCU_SYS_IER                                                                                               (0xC022)

/*S3921C PMU  from pmu mcu => main cpu.*/
#define MAILBOX_GET_EXIT_STANDBY_STATUS2                                                        (0x3FE9)
#define MAILBOX_GET_EXIT_STANDBY_STATUS1                                                        (0x3FE8)
#define MAILBOX_GET_EXIT_STANDBY_STATUS0                                                        (0x3FE7)
#define MAILBOX_GET_YEAR_H                                                                                 (0x3FE6)
#define MAILBOX_GET_YEAR_L                                                                                  (0x3FE5)
#define MAILBOX_GET_MONTH                                                                                  (0x3FE4)
#define MAILBOX_GET_DAY                                                                                       (0x3FE3)
#define MAILBOX_GET_HOUR                                                                                    (0x3FE2)
#define MAILBOX_GET_MIN                                                                                       (0x3FE1)
#define MAILBOX_GET_SEC                                                                                       (0x3FE0)

/*main cpu  => pmu mcu.*/
#define MAILBOX_WAKE_MONTH                                                                                (0xD20F)
#define MAILBOX_WAKE_DAY                                                                                    (0xD20E)
#define MAILBOX_WAKE_HOUR                                                                                  (0xD20D)
#define MAILBOX_WAKE_MIN                                                                                    (0xD20C)
#define MAILBOX_WAKE_SECOND                                                                              (0xD20B)

#define MAILBOX_SET_YEAR_H                                                                                  (0xD20A)
#define MAILBOX_SET_YEAR_L                                                                                  (0xD209)
#define MAILBOX_SET_MONTH                                                                                   (0xD208)
#define MAILBOX_SET_DAY                                                                                       (0xD207)
#define MAILBOX_SET_HOUR                                                                                     (0xD206)
#define MAILBOX_SET_MIN                                                                                       (0xD205)
#define MAILBOX_SET_SEC                                                                                       (0xD204)

#define NEC_IR_KEY_SARM_LOW3                                                                             (0xD203)
#define NEC_IR_KEY_SARM_LOW2                                                                             (0xD202)
#define NEC_IR_KEY_SARM_LOW1                                                                             (0xD201)
#define NEC_IR_KEY_SARM_LOW0                                                                             (0xD200)
#define CPU_TO_MCU_INTERRUPT_ENABLE_ADDR                                                       (0xD211)
#define CPU_TO_MCU_INTERRUPT_STATUS_ADDR                                                        (0xD210)
#define STANDBY_SHOW_TIMR_SARM                                                                        (0x3FFF)
#define SHOW_TYPE_SRAM                                                                                       (0x3FFE)
#define EXIT_STANDBY_TYPE_REG                                                                             (0x3FFD)
#define EXIT_STANDBY_TYPE_PANEL                                                                         (0x1)
#define EXIT_STANDBY_TYPE_IR                                                                               (0x2)
#define EXIT_STANDBY_TYPE_RTC                                                                             (0x3)

#define PMU_BASE                                                                                                   (0x0000)
#define PMU_IRQ                                                                                                      (20+8+32)
#define C3921_PMU_RTC0_IRQ                                                                                  (115)
#define S3922_PMU_RTC0_IRQ                                                                                  (115)
#define INT_ALI_IRQPMU                                                                                           (108)
#define SB_CLK                                                                                                        (12000000L)
#define RC_CLK                                                                                                        (2000000L)
#define SB_CNT                                                                                                        (8192L)
#define M36_IRQ_RTC                                                                                               (8+22)

/*PMU IR_Decode only can operate by byte.*/
#define PMU_CFG                                                                                                     (0x00)
#define IR_RC_ADJUST                                                                                              (0x01)
#define IR_RC_ADJUST1                                                                                            (0x02)
#define TIMETHR                                                                                                       (0x03)
#define NOISETHR                                                                                                     (0x04)

/*IR1 config.*/
#define IR1_THR0                                                                                                     (0x08)
#define IR1_THR_NUM                                                                                               (0x18)
#define IR1_DECODE_NUM                                                                                         (0x19)
#define IR1_POL                                                                                                       (0x1c)
#define IR1_DONCA                                                                                                  (0x28)
#define IR1_THR_SEL                                                                                                (0x34)

/*IR2 config.*/
#define IR2_THR_NUM                                                                                               (0x64)
#define IR2_DECODE_NUM                                                                                         (0x65)
#define IR2_POL                                                                                                       (0x68)
#define IR2_DONCA                                                                                                   (0x74)
#define IR2_THR_SEL                                                                                                (0x80)
#define IR2_THR0                                                                                                      (0xb8)
#define PRS_KEY_CFG                                                                                               (0xb0)
#define ANALOG_SEL                                                                                                 (0xb4)
#define PMU_DEBUG_REG                                                                                           (0xb6)

#define BASE_ADD	                                                                                               (PMU_BASE)
#define RTC_BASE                                                                                                     (0x18A00)
#define WR_RTC                                                                                                        (RTC_BASE+0x00)
#define RD_RTC                                                                                                         (RTC_BASE+0x04)
#define RD_RTC_MS                                                                                                   (RTC_BASE+0x08)
#define RD_EXIT_STANDBY                                                                                        (RTC_BASE+0x06)
#define TICK_MSK                                                                                                     (RTC_BASE+0x0a)
#define IIR			                                                                                               (RTC_BASE+0x0c)/*interrupt identify register.*/
#define CONFIG0                                                                                                       (RTC_BASE+0x10)
#define PMU_RTC_EPRTC                                                                                            (RTC_BASE+0x40)

/*RTC.*/
#define WR_RTC_L32                                                                                                 (RTC_BASE+0x00)
#define WR_RTC_H8                                                                                                  (RTC_BASE+0x04)
#define RTC_CTL_REG                                                                                                (RTC_BASE+0x08)

#define SB_TIMER0_CTL_REG                                                                                     (RTC_BASE+0x08)
#define SB_TIMER1_CTL_REG                                                                                     (RTC_BASE+0x18)
#define SB_TIMER2_CTL_REG                                                                                     (RTC_BASE+0x28)
#define SB_TIMER3_CTL_REG                                                                                     (RTC_BASE+0x38)
#define SB_TIMER4_CTL_REG                                                                                     (RTC_BASE+0x48)
#define SB_TIMER5_CTL_REG                                                                                     (RTC_BASE+0x58)
#define SB_TIMER6_CTL_REG                                                                                     (RTC_BASE+0x68)
#define SB_TIMER7_CTL_REG                                                                                     (RTC_BASE+0x78)
#define SB_TIMER3_CNT                                                                                            (RTC_BASE+0x30)
#define SB_CLK_RTC                                                                                                 (27)/*27MHz.*/

#define ALARM_NUM	                                                                                              (0x0A)
#define PMU_RTC_IRQ                                                                                               (21+8+32)
#define CUR_MONTH	                                                                                              (0x1e000000)
#define CUR_DATE	                                                                                              (0x01f00000)
#define CUR_DAY		                                                                                              (0x000e0000)
#define CUR_HOUR	                                                                                              (0x0001f000)
#define CUR_MIN		                                                                                              (0x00000fc0)
#define CUR_SEC		                                                                                              (0x0000003f)
#define PMU_SRAM_SIZE                                                                                          (0x4000)
#define INTERRUPT_ENABLE_REG1                                                                             (0x38)
#define INTERRUPT_ENABLE_REG2                                                                             (0x3c)
#if defined CONFIG_ALI_CHIP_M3921
#define PMU_IP_RESET_REG                                                                                      (0x320)
#elif defined CONFIG_ALI_CHIP_3922
#define PMU_IP_RESET_REG                                                                                      (0x278)
#else
#define PMU_IP_RESET_REG                                                                                      (0x84)
#endif

#define PMU_RAM_SWITCH_REG                                                                                (0xD100)
#define PMU_SRAM_BASE_ADDR                                                                                (0x0000)
#define UART_OUTPUT_REG                                                                                       (0x18300)
#define INTERRUPT_INIT_DONE                                                                                  (0x54494e49)/*'T', "I', 'N', 'I'.*/
#define WATCHDOG_ENABLE_REG_ADDR                                                                    (0x18504)

#if defined CONFIG_ALI_CHIP_M3921
#define PMU_MCU_RESET_ENABLE                                                                             (~(1<<17))
#define PMU_SW_RESET_ENABLE                                                                               (~(1<<16))
#define PMU_MCU_RESET_DISABLE                                                                            (1<<17)
#define PMU_SW_RESET_DISABLE                                                                              (1<<16)
#elif defined CONFIG_ALI_CHIP_3922
#define PMU_MCU_RESET_ENABLE                                                                             (1<<1)
#define PMU_SW_RESET_ENABLE                                                                               (1<<0)
#define PMU_MCU_RESET_DISABLE                                                                            (~(1<<1))
#define PMU_SW_RESET_DISABLE                                                                              (~(1<<0))
#else
#define PMU_MCU_RESET_ENABLE                                                                             (1<<25)
#define PMU_SW_RESET_ENABLE                                                                               (1<<24)
#define PMU_MCU_RESET_DISABLE                                                                            (~(1<<25))
#define PMU_SW_RESET_DISABLE                                                                              (~(1<<24))
#endif

#define PMU_SRAM_RELEASE_TO_MCU_ENABLE                                                          (~(1<<0))
#define MCU_START_SUCCESS                                                                                   (0x03020100)
#define CPU_TO_MCU_INTERRUPT_STATUS_REG                                                          (0xd210)
#define INTERRUPT_STATUS_CLEAR                                                                            (1<<0)
#define CPU_TO_MCU_INTERRUPT_ENABLE_REG                                                          (0xd211)
#define INTERRUPT_ENABLE                                                                                       (1<<0)
#define PMU_SRAM_RELEASE_TO_CPU_ENABLE                                                           (1<<0)
#define PANEL_POWER_KEY1_ADDR                                                                           (0x3FFB)
#define PANEL_POWER_KEY2_ADDR                                                                           (0x3FFC)
#define ALI_NEC_IR_KEY                                                                                           (0x60df708f)
#define ALI_WDT_REG                                                                                               (0x18504)
#define PAN_KEY_INVALID                                                                                        (0xFFFFFFFF)

/*Cut down power of all IP.*/
#define IC_NB_BASE_H                                                                                             (0x0000)
#define M35X_RXADC_REG_OFFSET                                                                            (0x280a2)
#define M35X_RXADC5V_REG_OFFSET                                                                        (0xa8)
#define M35X_VDAC_REG_OFFSET                                                                             (0x8084)
#define M35X_VDACSLEEP_REG_OFFSET                                                                     (0x78)
#define M35X_ADAC_REG_OFFSET                                                                             (0x20d4)
#define M35X_USBPOWER_REG_OFFSET                                                                     (0x3D810)
#define M35X_HDMIPOWER_REG_OFFSET                                                                   (0x6c)
#define M35X_DVBS2_REG_OFFSET                                                                            (0x4e000)
#define M35X_VIDEO_CHIPSET_REG_OFFSET                                                              (0x4e010)
#define M35X_APLL_REG_OFFSET                                                                               (0xb0)
#define M35X_RXPLL_REG_OFFSET                                                                             (0xb4)
#define M35X_DPLL_REG_OFFSET                                                                               (0xa0)
#define M35X_CPLL_SPLL_REG_OFFSET                                                                      (0xc0)

#define M382X_VDAC_REG_OFFSET                                                                            (0x8084)
#define M382X_ADAC_REG_OFFSET                                                                            (0x20d4)
#define M382X_HDMIPOWER_REG_OFFSET                                                                  (0x630)
#define M382X_RXADC_REG_OFFSET                                                                          (0x620)
#define M382X_USBPOWER_REG_OFFSET                                                                    (0x3D810)
#define M382X_ETHERNET_REG_OFFSET                                                                     (0x640)
#define M382X_APLL_REG_OFFSET                                                                             (0x610)
#define M382X_RXPLL_REG_OFFSET                                                                           (0x604)
#define M382X_DPLL_REG_OFFSET                                                                             (0x600)
#define M382X_SPLL_REG_OFFSET                                                                             (0x608)
#define M382X_MPLL_REG_OFFSET                                                                            (0x60C)
#define ALI_IP_CLK_GATE_REG1                                                                                (0x60)
#define ALI_IP_CLK_GATE_REG2                                                                                (0x64)
#define ALI_IP_RESET_REG1                                                                                      (0x80)
#define ALI_IP_RESET_REG2                                                                                      (0x84)
#define NB_ERROR_ACCESS_HANDING_REG                                                                (0x24)
#define M3505_ENABLE_TIMEOUT_HANDLING                                                             ((1<<2) | (1<<1) | (1<<0))
#define C3505_RESET_REG1_MASK                                                                            (0x0636FFFF)
#define C3505_RESET_REG2_MASK                                                                            (0x3493E045)
#define UNKOWN_WAKEUP_TYPE                                                                               (0xFF)
//#define PANEL_KEY_DETECT_TEST
#define ALI_UART_STATUS_REG                                                                                 (0x18305)
#define TRANSMIT_IDLE                                                                                           (0)
#define UART_STATUS_MASK                                                                                     ((1<<6) | (1<<5))
#define WDT_REBOOT_FLAG_SAVE_ADDR                                                                   (0x3FAC)
#define WDT_REBOOT_FLAG_READ_ADDR                                                                   (0x3FA8)
#define ALI_SOC_BASE_ADDR			                                                                    (0x18000000)
#define ALI_SOC_BASE_ADDR_SIZE		                                                                     (0x50000)
#define ALI_PMU_DEVICE_NAME                                                                                "ali_pmu"
#define ALI_PMU_M3811                                                                                           (0x38110000)
#define ALI_PMU_M3701                                                                                           (0x37010000)
#define ALI_PMU_M3503                                                                                           (0x35030000)
#define ALI_PMU_M3821                                                                                           (0x38210000)
#define ALI_PMU_M3505                                                                                           (0x35050000)
#define ALI_PMU_M3921                                                                                           (0x39210000)
#define ALI_PMU_M3922                                                                                           (0x39220000)
#define M3922_INT_ENABLE_REG1                                                                             (0x40)
#define M3922_INT_ENABLE_REG2                                                                             (0x44)
#define M3922_INT_ENABLE_REG3                                                                             (0x48)
#define M3922_INT_ENABLE_REG4                                                                             (0x4C)
#define ALI_INT_ENABLE_REG1                                                                                  (0x38)
#define ALI_INT_ENABLE_REG2                                                                                  (0x3C)
#define M3503_RXADC_DIS                                                                                       ((1<<2) | (1<<1) | (1<<0))
#define M3503_RXADC_5V_DIS                                                                                  ((1<<31) | (1<<30) | (1<<29) | (1<<28))
#define M3503_VDAC_DIS                                                                                         ((1<<11) | (1<<10) | (1<<9) | (1<<8))
#define M3503_VDAC_SLEEP_DIS                                                                               (1<<7)
#define M3503_ADAC_DIS                                                                                         (0x7FD0)
#define M3503_USBPLL_BGR_IVREF_VREF_DIS                                                           ((1<<17) | (1<<16) | (1<<15))
#define M3503_HDMI_DIS                                                                                         ((1<<18) | (1<<17) | (1<<16))
#define M3503_DVBS2_DIS                                                                                       (1<<1)
#define M3503_VIDEO_CHIPSET_DIS                                                                          (1<<1)
#define M3503_HDMI_I2C_DIS                                                                                  ((1<<23) | (1<<22))
#define M3503_UART_GPIO1_DIS                                                                               ((1<<4) | (1<<3))
#define M3503_UART_GPIO2_DIS                                                                               ((1<<15) | (1<<14))
#define M3503_APLL_DIS                                                                                          (~(1<<20))
#define M3503_RXPLL_DIS                                                                                        (~(1<<12))
#define M3503_DPLL_DIS                                                                                          (~(1<<15))
#define M3503_CPLL_SPLL_DIS                                                                                 (~((1<<16) | (1<<0)))
#define M3821_VDAC_VDACSLEEP_DIS                                                                      ((1<<31) | (1<<11) | (1<<10) | (1<<9) | (1<<8))
#define M3821_ADAC_DIS                                                                                         (0x7FD0)
#define M3821_HDMI_DIS                                                                                         (1<<1)
#define M3821_RXADC_DIS                                                                                       ((1<<6) | (1<<5) | (1<<4) | (1<<2) | (1<<1) | (1<<0))
#define M3821_USBPLL_BGR_IVREF_VREF_DIS                                                           ((1<<17) | (1<<16) | (1<<15))
#define M3821_ETHERNET_PHY_DIS                                                                           ((1<<8) | (1<<0))
#define M3821_APLL_DIS                                                                                          (~(1<<0))
#define M3821_RXPLL_DIS                                                                                        (~(1<<0))
#define M3821_DPLL_DIS                                                                                          (~(1<<0))
#define M3821_SPLL_DIS                                                                                          (~(1<<0))
#define M3821_MPLL_DIS                                                                                         (~(1<<0))
#define M3821_IP_CLK_GATE_REG1                                                                           (0x60)
#define M3821_IP_CLK_GATE_REG1_VALUE                                                                (0x0EFE3FFF)
#define M3821_IP_CLK_GATE_REG2                                                                           (0x64)
#define M3821_IP_CLK_GATE_REG2_VALUE                                                                (0xCFFFFFFF)
#define M3505_IP_CLK_GATE_REG1                                                                           (0x60)
#define M3505_IP_CLK_GATE_REG1_VALUE                                                                (0x0E3E1FFF)
#define M3505_IP_CLK_GATE_REG2                                                                           (0x64)
#define M3505_IP_CLK_GATE_REG2_VALUE                                                                (0xEFFFFFFF)
#define M3505_ADAC_CLK_RESTORE                                                                          (~(1<<15))
#define M3505_ADAC_CLK_REG                                                                                 (0x20d4)
#define M3505_ADAC_DIS                                                                                         (0x7FD0)
#define M3505_VDAC_CLK_REG                                                                                 (0x8084)
#define M3505_VDAC_DIS                                                                                         ((1<<11) | (1<<10) | (1<<9) | (1<<8))
#define M3505_VDAC_SLEEP_CLK_REG                                                                      (0x78)
#define M3505_VDAC_SLEEP_DIS                                                                              (1<<7)
#define M3505_HDMI_CLK_REG                                                                                 (0x630)
#define M3505_HDMI_DIS                                                                                         ((1<<8) | (1<<7) | (1<<0))
#define M3505_RXADC_CLK_REG1                                                                             (0x2806A)
#define M3505_RXADC_CLK_REG2                                                                             (0x3006A)
#define M3505_RXADC_CLK_REG3                                                                             (0x4C0A2)
#define M3505_RXADC_DIS                                                                                       ((1<<2) | (1<<1) | (1<<0))
#define M3505_USB_CLK_REG                                                                                   (0x3D810)
#define M3505_USB_DIS                                                                                           ((1<<17) | (1<<16) | (1<<15))
#define M3505_RXPLL_CLK_REG                                                                                (0x604)
#define M3505_RXPLL_DIS                                                                                        (1<<0)
#define M3505_APLL_CLK_REG                                                                                  (0x60C)
#define M3505_APLL_DIS                                                                                          (1<<0)
#define M3505_CPU_FREQ_REG                                                                                 (0xC0)
#define M3505_CPU_FREQ_DIV_ENABLE                                                                    ((1<<9) | (1<<8))
#define M3505_CPLL_CLK_REG                                                                                  (0xC0)
#define M3505_CPLL_DIS                                                                                          (1<<0)
#define M3505_DPLL_CLK_REG                                                                                  (0x600)
#define M3505_DPLL_DIS                                                                                          ((1<<16) | (1<<0))
#define M3505_PAD_CLK_REG1                                                                                 (0x410)
#define M3505_PAD_CLK_REG2                                                                                 (0x414)
#define M3505_PAD_CLK_REG3                                                                                 (0x418)
#define M3505_PAD_CLK_REG4                                                                                 (0x41C)
#define M3505_PAD_CLK_REG5                                                                                 (0x420)
#define M3505_PAD_CLK_REG6                                                                                 (0x424)
#define M3505_USB_DEVICE_CLK_REG                                                                      (0x6c)
#define M3505_USB_DEVICE_DIS                                                                              (1<<24)
#define ALI_SB_TIME_DIS                                                                                        (0xEF)
#define ALI_SB_TIMER_ENABLE                                                                                (0x14)
#define IR_BUFFER_BASE                                                                                         (0x3FC0)
#define DEFAULT_IR_DATA                                                                                       (0x5A5A5A5A)
#define PMU_GPIO_CTRL_REG1                                                                                (0xC057)
#define PMU_GPIO_CTRL_REG2                                                                                (0xC05F)

#define ICE_MODE_SET_REG  (0xC0A4)
#define MCU_ICE_MODE_ENABLE (~(1<<0))
#define MCU_ICE_MODE_TRIGGER (1<<24)
#define MCU_ICE_TRIGGER_CLEAR (~(1<<24))
#define MCU_UART_SELECT_REG (0xC0B0)
#define XPMU_GPIO_UART_SELECTED (1<<0)
//======================================================================================================//

unsigned int wakeup_power_key[8] = {0x0};
unsigned char wakeup_panel_key[2] = {0x0};
static spinlock_t rtc_lock;
static unsigned int RTC_DIV[4] = {32, 64, 128, 256};
static unsigned long long rtc_upper = 0xFFFFFFFFFF;
static unsigned long long rtc_onesec = 0xFFFFF32019;
struct rtc_base_time base_time_0 = {30, 30, 11, 16, 8, 0, 1, 1, 1, 1, 1, 1, 1, 1};
struct rtc_base_time base_time_3 = {30, 30, 11, 16, 8, 0, 1, 1, 1, 1, 1, 1, 1, 1};
unsigned char rtc_sec = 30;
unsigned char rtc_min = 30;
unsigned char rtc_hour = 11;
unsigned char rtc_date = 16;
unsigned char rtc_month = 8;
unsigned int  rtc_year = 0; 
unsigned char g_year_h = 1;
unsigned char g_year_l = 1;
unsigned int g_year = 1;
unsigned char g_month = 1;
unsigned char g_day = 1;
unsigned char g_hour = 1;
unsigned char g_min = 1;
unsigned char g_sec = 1;
unsigned char g_exit_standby_sts = 0;
unsigned char g_bak_set_sec = 0;
unsigned char g_bak_set_min = 0;
unsigned char g_bak_set_hour = 0;
unsigned char g_bak_set_day = 0;
unsigned char g_bak_set_month = 0;
unsigned char g_bak_set_year_h = 0;
unsigned char g_bak_set_year_l = 0;
unsigned char g_bak_wake_month = 0;
unsigned char g_bak_wake_min = 0;
unsigned char g_bak_wake_hour = 0;
unsigned char g_bak_wake_day = 0;
unsigned char g_bak_wake_sec = 0;
unsigned int ali_chip_info = 0;
unsigned int ali_chip_id = 0;
unsigned int ali_chip_rev = 0;
struct ali_pmu_device *g_pmu_dev = NULL;
//======================================================================================================//

struct pmu_private
{
	unsigned long reserved;
};

enum RTC_CTL_REG_CONTENT
{
	RTC_TOV = (1 << 3),
	RTC_DIV_32 = 0,
	RTC_DIV_64 = 1,
	RTC_DIV_128 = 2,
	RTC_DIV_256 = 3,
}; 

enum SB_TIMER
{
	SB_TIMER_0 = 0,
	SB_TIMER_1,
	SB_TIMER_2,
	SB_TIMER_3,
	SB_TIMER_4,
	SB_TIMER_5,
	SB_TIMER_6,
	SB_TIMER_7,
};

typedef struct pmu_ir_key
{
	unsigned short ir_thr[8];
	unsigned char ir_thr_cnt; 
	unsigned char ir_decode_cnt;
	unsigned char ir_pol[12];
	unsigned char ir_pol_cnt;
	unsigned char ir_donca[12];
	unsigned char ir_donca_cnt;
	unsigned char ir_thr_sel[48];
	unsigned char ir_thr_sel_cnt;
	unsigned char flg;
	unsigned char type;
} PMU_IR;

enum irp_type
{
	IR_TYPE_NEC = 0,
	IR_TYPE_LAB,
	IR_TYPE_50560,
	IR_TYPE_KF,
	IR_TYPE_LOGIC,
	IR_TYPE_SRC,
	IR_TYPE_NSE,
	IR_TYPE_RC5,
	IR_TYPE_RC5_X,
	IR_TYPE_RC6,
};

struct ali_pmu_device{
	/*Common.*/
	char *dev_name;

	/*Hardware privative structure.*/
	struct pmu_private *priv;	
	struct device *pmu_device_node;
	struct cdev pmu_cdev;
	struct class *pmu_class;
	dev_t dev_no;

	spinlock_t lock;
	unsigned long flags;
	struct platform_device *pdev;
	u8 stopping;
	int irq;
	struct list_head queue;
	struct spi_transfer *current_transfer;
	unsigned long current_remaining_bytes;
	struct spi_transfer *next_transfer;
	unsigned long next_remaining_bytes;
	void __iomem *ali_soc_base_addr;
	void __iomem *ali_pmu_base_addr;
	u8 *buf;
	u32	chip_id;
	u32 panel_power_key;
};
#endif
