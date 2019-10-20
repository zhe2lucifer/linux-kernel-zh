/*
* Copyright 2014 Ali Corporation Inc. All Rights Reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA 02110-1301, USA.
*/
 //======================================================================================================//
 
#include "ali_pmu.h"
#include <linux/of.h>
#include <linux/of_platform.h>
#include <ali_soc.h>

//======================================================================================================//
#define ALISL_PWR_STANDBY (0x57414B45)

PMU_EXIT_FLAG get_powerup_status(void);
static void output_string(unsigned char *string);
static void pmu_delay(void);
static int ali_pmu_init(struct platform_device *pdev);
void set_mcu_panel_show_type(unsigned char show_type);
//======================================================================================================//

static unsigned char pmu_read8(struct ali_pmu_device *pmu_dev, unsigned int reg)
{
	return ioread8(pmu_dev->ali_pmu_base_addr + reg);
}

static void pmu_write8(struct ali_pmu_device *pmu_dev, unsigned int reg, unsigned char val)
{
	iowrite8(val, pmu_dev->ali_pmu_base_addr + reg);
}

static unsigned int pmu_read32(struct ali_pmu_device *pmu_dev, unsigned int reg)
{
	return ioread32(pmu_dev->ali_pmu_base_addr + reg);
}

static void pmu_write32(struct ali_pmu_device *pmu_dev, unsigned int reg, unsigned int val)
{
	iowrite32(val, pmu_dev->ali_pmu_base_addr + reg);
}

#if 0/*Reserved for debug.*/
static unsigned char pmu_get_bit(struct ali_pmu_device *pmu_dev, unsigned int reg, unsigned char bit)
{
	unsigned char temp_data = 0;

	temp_data = ioread8(pmu_dev->ali_pmu_base_addr + reg);
	temp_data &= (1<<bit);

	return temp_data;
}

static unsigned char pmu_set_bit(struct ali_pmu_device *pmu_dev, unsigned int reg, unsigned char bit)
{
	unsigned char temp_data = 0;

	temp_data = ioread8(pmu_dev->ali_pmu_base_addr + reg);
	temp_data |= (1<<bit);

	return temp_data;
}
#endif

static unsigned int soc_read8(struct ali_pmu_device *pmu_dev, unsigned int reg)
{
	return ioread8(pmu_dev->ali_soc_base_addr + reg);
}

static void soc_write8(struct ali_pmu_device *pmu_dev, unsigned int reg, unsigned int val)
{
	iowrite8(val, pmu_dev->ali_soc_base_addr + reg);
}

static unsigned int soc_read32(struct ali_pmu_device *pmu_dev, unsigned int reg)
{
	return ioread32(pmu_dev->ali_soc_base_addr + reg);
}

static void soc_write32(struct ali_pmu_device *pmu_dev, unsigned int reg, unsigned int val)
{
	iowrite32(val, pmu_dev->ali_soc_base_addr + reg);
}

static void ali_pmu_get_chip_info(void)
{
	ali_chip_info = soc_read32(g_pmu_dev, 0x0000);
	ali_chip_id = (ali_chip_info & 0xFFFF0000);
	ali_chip_rev = (ali_chip_info & 0x0000FFFF);

#ifdef CONFIG_ALI_PMU_DEBUG
	printk(KERN_EMERG "Get chip info: 0x%08X\n", ali_chip_info);
#endif
}

static void ali_disable_all_ip_interrupt(void)
{
#if defined CONFIG_ALI_CHIP_3922
	soc_write32(g_pmu_dev, M3922_INT_ENABLE_REG1, 0x0);
	soc_write32(g_pmu_dev, M3922_INT_ENABLE_REG2, 0x0);
	soc_write32(g_pmu_dev, M3922_INT_ENABLE_REG3, 0x0);
	soc_write32(g_pmu_dev, M3922_INT_ENABLE_REG4, 0x0);
#else
	soc_write32(g_pmu_dev, ALI_INT_ENABLE_REG1, 0x0);
	soc_write32(g_pmu_dev, ALI_INT_ENABLE_REG2, 0x0);
#endif
}

static void Cut_Down_IP_Power(void)
{
	unsigned int value = 0;

	if(0)//(ALI_PMU_M3503 == ali_chip_id)
	{
		/*RXADC.*/
		value = soc_read8(g_pmu_dev, M35X_RXADC_REG_OFFSET);
		value |= M3503_RXADC_DIS;
		soc_write8(g_pmu_dev, M35X_RXADC_REG_OFFSET, value);

		/*RXADC_5V.*/
		value = soc_read32(g_pmu_dev, M35X_RXADC5V_REG_OFFSET);
		value |= M3503_RXADC_5V_DIS;
		soc_write32(g_pmu_dev, M35X_RXADC5V_REG_OFFSET, value);

		/*VDAC.*/
		value = soc_read32(g_pmu_dev, M35X_VDAC_REG_OFFSET);
		value |= M3503_VDAC_DIS;
		soc_write32(g_pmu_dev, M35X_VDAC_REG_OFFSET, value);

		/*VDAC Sleep.*/
		value = soc_read32(g_pmu_dev, IC_NB_BASE_H + M35X_VDACSLEEP_REG_OFFSET);
		value |= M3503_VDAC_SLEEP_DIS;
		soc_write32(g_pmu_dev, M35X_VDACSLEEP_REG_OFFSET, value);

		/*ADAC.*/
		value = soc_read32(g_pmu_dev, M35X_ADAC_REG_OFFSET);
		value |= M3503_ADAC_DIS;
		soc_write32(g_pmu_dev, IC_NB_BASE_H + M35X_ADAC_REG_OFFSET, value);

		/*USB PLL/BGR/IVREF/VREF.*/
		value = soc_read32(g_pmu_dev, M35X_USBPOWER_REG_OFFSET);
		value |= M3503_USBPLL_BGR_IVREF_VREF_DIS;
		soc_write32(g_pmu_dev, IC_NB_BASE_H + M35X_USBPOWER_REG_OFFSET, value);

		/*HDMI.*/
		value = soc_read32(g_pmu_dev, M35X_HDMIPOWER_REG_OFFSET);
		value |= M3503_HDMI_DIS;
		soc_write32(g_pmu_dev, M35X_HDMIPOWER_REG_OFFSET, value);

		/*DVB_S2.*/
		value = soc_read32(g_pmu_dev, M35X_DVBS2_REG_OFFSET);
		value |= M3503_DVBS2_DIS;
		soc_write32(g_pmu_dev, IC_NB_BASE_H + M35X_DVBS2_REG_OFFSET, value);

		/*VIDEO_CHIPSET.*/
		value = soc_read32(g_pmu_dev, M35X_VIDEO_CHIPSET_REG_OFFSET);
		value |= M3503_VIDEO_CHIPSET_DIS;
		soc_write32(g_pmu_dev, M35X_VIDEO_CHIPSET_REG_OFFSET, value);

		/*HDMI I2C(GPIO22, GPIO23) -> GPIO Output 0.*/
		value = soc_read32(g_pmu_dev, 0x430);
		value |= M3503_HDMI_I2C_DIS;
		soc_write32(g_pmu_dev, 0x430, value);
		value = soc_read32(g_pmu_dev, 0x58);
		value |= M3503_HDMI_I2C_DIS;
		soc_write32(g_pmu_dev, 0x58, value);
		value = soc_read32(g_pmu_dev, 0x54);
		value &= ~(M3503_HDMI_I2C_DIS);
		soc_write32(g_pmu_dev, 0x54, value);

		/*UART(GPIO35, GPIO36) -> GPIO Output 0.*/
		value = soc_read32(g_pmu_dev, 0x434);
		value |= M3503_UART_GPIO1_DIS;
		soc_write32(g_pmu_dev, 0x434, value);
		value = soc_read32(g_pmu_dev, 0xD8);
		value |= M3503_UART_GPIO1_DIS;
		soc_write32(g_pmu_dev, 0xD8, value);
		value = soc_read32(g_pmu_dev, 0xD4);
		value &= ~(M3503_UART_GPIO1_DIS);
		soc_write32(g_pmu_dev, 0xD4, value);

		/*UART(GPIO110, GPIO111) -> GPIO Output 0.*/
		value = soc_read32(g_pmu_dev, 0x43c);
		value |= M3503_UART_GPIO2_DIS;
		soc_write32(g_pmu_dev, 0x43c, value);
		value = soc_read32(g_pmu_dev, 0x358);
		value |= M3503_UART_GPIO2_DIS;
		soc_write32(g_pmu_dev, 0x358, value);
		value = soc_read32(g_pmu_dev, 0x354);
		value &= ~(M3503_UART_GPIO2_DIS);
		soc_write32(g_pmu_dev, 0x354, value);

		/*Cut down all PLL power.*/
		/*APLL.*/
		value = soc_read32(g_pmu_dev, M35X_APLL_REG_OFFSET);
		value &= M3503_APLL_DIS;
		soc_write32(g_pmu_dev, M35X_APLL_REG_OFFSET, value);

		/*RxPLL.*/
		value = soc_read32(g_pmu_dev, M35X_RXPLL_REG_OFFSET);
		value &= M3503_RXPLL_DIS;
		soc_write32(g_pmu_dev, M35X_RXPLL_REG_OFFSET, value);

		/*DPLL.*/
		value = soc_read32(g_pmu_dev, M35X_DPLL_REG_OFFSET);
		value &= M3503_DPLL_DIS;
		soc_write32(g_pmu_dev, M35X_DPLL_REG_OFFSET, value);

		/*CPLL, SPLL.*/
		value = soc_read32(g_pmu_dev, M35X_CPLL_SPLL_REG_OFFSET);
		value &= M3503_CPLL_SPLL_DIS;
		soc_write32(g_pmu_dev, M35X_CPLL_SPLL_REG_OFFSET, value);
	}
	else if(ALI_PMU_M3821 == ali_chip_id)
	{
		/*VDAC, VDAC Sleep.*/
		value = soc_read32(g_pmu_dev, M382X_VDAC_REG_OFFSET);
		value |= M3821_VDAC_VDACSLEEP_DIS;
		soc_write32(g_pmu_dev, M382X_VDAC_REG_OFFSET, value);

		/*ADAC.*/
		value = soc_read32(g_pmu_dev, M382X_ADAC_REG_OFFSET);
		value |= M3821_ADAC_DIS;
		soc_write32(g_pmu_dev, M382X_ADAC_REG_OFFSET, value);

		/*HDMI.*/
		value = soc_read32(g_pmu_dev, M382X_HDMIPOWER_REG_OFFSET);
		value |= M3821_HDMI_DIS;
		soc_write32(g_pmu_dev, M382X_HDMIPOWER_REG_OFFSET, value);

		/*RXADC.*/
		value = soc_read32(g_pmu_dev, M382X_RXADC_REG_OFFSET);
		value |= M3821_RXADC_DIS;
		soc_write32(g_pmu_dev, M382X_RXADC_REG_OFFSET, value);

		/*USB PLL/BGR/IVREF/VREF.*/
		value = soc_read32(g_pmu_dev, M382X_USBPOWER_REG_OFFSET);
		value |= M3821_USBPLL_BGR_IVREF_VREF_DIS;
		soc_write32(g_pmu_dev, M382X_USBPOWER_REG_OFFSET, value);

		/*Ethernet Phy.*/
		value = soc_read32(g_pmu_dev, M382X_ETHERNET_REG_OFFSET);
		value |= M3821_ETHERNET_PHY_DIS;
		soc_write32(g_pmu_dev, M382X_ETHERNET_REG_OFFSET, value);

		/*Cut down all PLL power.*/
		/*APLL.*/
		value = soc_read32(g_pmu_dev, M382X_APLL_REG_OFFSET);
		value &= M3821_APLL_DIS;
		soc_write32(g_pmu_dev, M382X_APLL_REG_OFFSET, value);

		/*RxPLL.*/
		value = soc_read32(g_pmu_dev, M382X_RXPLL_REG_OFFSET);
		value &= M3821_RXPLL_DIS;
		soc_write32(g_pmu_dev, M382X_RXPLL_REG_OFFSET, value);

		/*DPLL.*/
		//value = soc_read32(g_pmu_dev, M382X_DPLL_REG_OFFSET);
		//value &= M3821_DPLL_DIS;
		//soc_write32(g_pmu_dev, M382X_DPLL_REG_OFFSET, value);

		/*SPLL.*/
		//value = soc_read32(g_pmu_dev, M382X_SPLL_REG_OFFSET);
		//value &= M3821_SPLL_DIS;
		//soc_write32(g_pmu_dev, M382X_SPLL_REG_OFFSET, value);

		/*MPLL.*/
		value = soc_read32(g_pmu_dev, M382X_MPLL_REG_OFFSET);
		value &= M3821_MPLL_DIS;
		soc_write32(g_pmu_dev, M382X_MPLL_REG_OFFSET, value);

		/*Clock Gating.*/
		soc_write32(g_pmu_dev, M3821_IP_CLK_GATE_REG1, M3821_IP_CLK_GATE_REG1_VALUE);
		soc_write32(g_pmu_dev, M3821_IP_CLK_GATE_REG2, M3821_IP_CLK_GATE_REG2_VALUE);

		output_string("IP power all are cut down before enter standby!");
	}
	else if(ALI_PMU_M3505 == ali_chip_id)
	{
		/*Set CLK gated.*/
		pmu_delay();
		soc_write32(g_pmu_dev, M3505_IP_CLK_GATE_REG1, M3505_IP_CLK_GATE_REG1_VALUE);
		pmu_delay();

		soc_write32(g_pmu_dev, M3505_IP_CLK_GATE_REG2, M3505_IP_CLK_GATE_REG2_VALUE);
		pmu_delay();
	}
}

void pmu_set_value_and_set_waketime(void)
{
	volatile unsigned char Temp_Current_Month = 0, Temp_Current_Day = 0;
	volatile unsigned char Temp_Current_Hour = 0, Temp_Current_Min = 0;
	volatile unsigned char Temp_Current_Sec = 0, Tenp_Current_Year_H = 0;
	volatile unsigned char Tenp_Current_Year_L = 0, Temp_Wake_Month = 0;
	volatile unsigned char Temp_Wake_day = 0, Temp_Wake_hour = 0, Temp_Wake_min = 0;
	volatile unsigned char Temp_Wake_sec = 0;

	do{
		/*current time.*/
		pmu_write8(g_pmu_dev, MAILBOX_SET_SEC, g_bak_set_sec);
		pmu_write8(g_pmu_dev, MAILBOX_SET_MIN, g_bak_set_min);
		pmu_write8(g_pmu_dev, MAILBOX_SET_HOUR, g_bak_set_hour);
		pmu_write8(g_pmu_dev, MAILBOX_SET_DAY, g_bak_set_day);
		pmu_write8(g_pmu_dev, MAILBOX_SET_MONTH, g_bak_set_month);
		pmu_write8(g_pmu_dev, MAILBOX_SET_YEAR_L, g_bak_set_year_l);
		pmu_write8(g_pmu_dev, MAILBOX_SET_YEAR_H, g_bak_set_year_h);

		/*wakeup time.*/
		pmu_write8(g_pmu_dev, MAILBOX_WAKE_SECOND, g_bak_wake_sec);
		pmu_write8(g_pmu_dev, MAILBOX_WAKE_MIN, g_bak_wake_min);
		pmu_write8(g_pmu_dev, MAILBOX_WAKE_HOUR, g_bak_wake_hour);
		pmu_write8(g_pmu_dev, MAILBOX_WAKE_DAY, g_bak_wake_day);
		pmu_write8(g_pmu_dev, MAILBOX_WAKE_MONTH, g_bak_wake_month);

		Tenp_Current_Year_H= pmu_read8(g_pmu_dev, MAILBOX_SET_YEAR_H);
		Tenp_Current_Year_L = pmu_read8(g_pmu_dev, MAILBOX_SET_YEAR_L);
		Temp_Current_Month = pmu_read8(g_pmu_dev, MAILBOX_SET_MONTH);
		Temp_Current_Day = pmu_read8(g_pmu_dev, MAILBOX_SET_DAY);
		Temp_Current_Hour = pmu_read8(g_pmu_dev, MAILBOX_SET_HOUR);
		Temp_Current_Min = pmu_read8(g_pmu_dev, MAILBOX_SET_MIN);
		Temp_Current_Sec = pmu_read8(g_pmu_dev, MAILBOX_SET_SEC);

		Temp_Wake_Month = pmu_read8(g_pmu_dev, MAILBOX_WAKE_MONTH);
		Temp_Wake_day = pmu_read8(g_pmu_dev, MAILBOX_WAKE_DAY);
		Temp_Wake_hour = pmu_read8(g_pmu_dev, MAILBOX_WAKE_HOUR);
		Temp_Wake_min = pmu_read8(g_pmu_dev, MAILBOX_WAKE_MIN);
		Temp_Wake_sec = pmu_read8(g_pmu_dev, MAILBOX_WAKE_SECOND);
	}while((Tenp_Current_Year_H != g_bak_set_year_h) || (Tenp_Current_Year_L != g_bak_set_year_l) \
		|| (Temp_Current_Month != g_bak_set_month) || (Temp_Current_Day != g_bak_set_day) \
		|| (Temp_Current_Hour != g_bak_set_hour) || (Temp_Current_Min != g_bak_set_min) \
		|| (Temp_Current_Sec != g_bak_set_sec) || (Temp_Wake_Month != g_bak_wake_month) \
		|| (Temp_Wake_day != g_bak_wake_day) || (Temp_Wake_hour != g_bak_wake_hour) \
		|| (Temp_Wake_min != g_bak_wake_min) || (Temp_Wake_sec != g_bak_wake_sec));

	printk(KERN_EMERG "\n====>Current time before enter standby:[year_h:%d], [year_l:%d], [month:%d], [date:%d], [hour:%d], [min:%d], [sec:%d]\n", \
		pmu_read8(g_pmu_dev, MAILBOX_SET_YEAR_H), pmu_read8(g_pmu_dev, MAILBOX_SET_YEAR_L), pmu_read8(g_pmu_dev, MAILBOX_SET_MONTH), \
		pmu_read8(g_pmu_dev, MAILBOX_SET_DAY), pmu_read8(g_pmu_dev, MAILBOX_SET_HOUR), pmu_read8(g_pmu_dev, MAILBOX_SET_MIN), \
		pmu_read8(g_pmu_dev, MAILBOX_SET_SEC));

	printk(KERN_EMERG "====>Wakeup Time:[Month:%d], [date:%d], [hour:%d], [min:%d], [sec:%d]\n",\
		pmu_read8(g_pmu_dev, MAILBOX_WAKE_MONTH), pmu_read8(g_pmu_dev, MAILBOX_WAKE_DAY), pmu_read8(g_pmu_dev, MAILBOX_WAKE_HOUR), \
		pmu_read8(g_pmu_dev, MAILBOX_WAKE_MIN), pmu_read8(g_pmu_dev, MAILBOX_WAKE_SECOND));
}

static void pmu_delay(void)
{
	volatile unsigned int cnt1 = 0, temp_data = 0;
	for(cnt1=0; cnt1<0x100000; cnt1++)
	{
		temp_data = soc_read8(g_pmu_dev, IC_NB_BASE_H);
	}
}

static void wait_for_standby(void)
{
	volatile unsigned int cnt2 = 0;
	for(cnt2=0; cnt2<0x2000; cnt2++);
	{
		pmu_delay();
	}
	output_string("7");

	/*Release pmu sram to main CPU, and main cpu boot mcu once again.*/
	pmu_write8(g_pmu_dev, PMU_RAM_SWITCH_REG, pmu_read8(g_pmu_dev, PMU_RAM_SWITCH_REG) | PMU_SRAM_RELEASE_TO_CPU_ENABLE);
	pmu_delay();
}

static void output_char(unsigned char c)
{
	volatile unsigned char uart_status = 0;

	pmu_delay();
	do{
		soc_write8(g_pmu_dev, UART_OUTPUT_REG, c);
		uart_status = soc_read8(g_pmu_dev, ALI_UART_STATUS_REG);
		uart_status &= UART_STATUS_MASK;
	}while(TRANSMIT_IDLE != uart_status);
}

static void output_string(unsigned char *string)
{
	while(*string)
	{
		output_char(*string++);
	}
	output_char(0x0D);
	output_char(0x0A);
}

#if 0/*Reserved for debug.*/
static char ascii[] = "0123456789ABCDEF";
static void dump_reg(unsigned int addr, unsigned int len)
{
	unsigned int i = 0, j = 0;
	unsigned char index = 0;
	for(i=0; i<len; i++)
	{
		if((i%16) == 0)
		{
			output_char(0x0d);
			output_char(0x0a);
			for(j=0; j<8; j++)
			{
				output_char(ascii[((addr+i)>>(4*(7-j)))&0xF]);
			}
			output_char(':');
		}
		index = *(unsigned char *)(addr+i);
		output_char(ascii[(index>>4)&0xF]);
		output_char(ascii[index&0xF]);
		output_char(' ');
	}
	output_char(0x0D);
	output_char(0x0A);
	return;
}
#endif

static unsigned char Now_Is_Leap_Year(unsigned int year)
{
	if((year%4 == 0) && (year%100 != 0))
	{
		return 1;/*is leap.*/
	}
	else if(year%400 == 0)
	{
		return 1;/*is leap.*/
	}
	else
	{
		return 0;/*not leap.*/
	}
}

static void time_accumulate(struct rtc_base_time *base_time)
{
	rtc_sec += 1;
	if(rtc_sec > 59)
	{
		rtc_min += 1;
		rtc_sec = 0;
	}

	if(rtc_min > 59)
	{
		rtc_hour += 1;
		rtc_min = 0;
	}

	if(rtc_hour > 23)
	{
		rtc_date += 1;
		rtc_hour = 0;
	}

	if((rtc_month == 1) || (rtc_month == 3) || (rtc_month == 5) \
		|| (rtc_month == 7) || (rtc_month == 8) || (rtc_month == 10) \
		|| (rtc_month == 12))
	{
		if(rtc_date > 31)
		{
			rtc_month += 1;
			rtc_date = 1;
		}
	}
	else
	{
		if(rtc_month == 2)/*month 2.*/
		{
			if(Now_Is_Leap_Year(rtc_year) == 0)/*not a leap year.*/
			{
				if(rtc_date > 28)
				{
					rtc_month += 1;
					rtc_date = 1;
				}
			}
			else/*leap year.*/
			{
				if(rtc_date > 29)
				{
					rtc_month += 1;
					rtc_date = 1;
				} 
			}
		}
		else
		{
			if(rtc_date > 30)
			{
				rtc_month += 1;
				rtc_date = 1;
			}
		}
	}

	if(rtc_month > 12)
	{
		/*a new year begins.*/
		rtc_year += 1;
		rtc_month = 1;
	}
}

int ali_pmu_get_time(unsigned int *time_cur)
{
	unsigned char c_year_h = 1;
	unsigned char c_year_l = 1;
	unsigned int c_year = 1;
	unsigned char c_month = 1;
	unsigned char c_day = 1;
	unsigned char c_hour = 1;
	unsigned char c_min = 1;
	unsigned char c_sec = 1;
	unsigned int c_time = 0;

	c_year_h = pmu_read8(g_pmu_dev, MAILBOX_GET_YEAR_H);
	c_year_l = pmu_read8(g_pmu_dev, MAILBOX_GET_YEAR_L);
	c_month = pmu_read8(g_pmu_dev, MAILBOX_GET_MONTH);
	c_day = pmu_read8(g_pmu_dev, MAILBOX_GET_DAY);
	c_hour = pmu_read8(g_pmu_dev, MAILBOX_GET_HOUR);
	c_min = pmu_read8(g_pmu_dev, MAILBOX_GET_MIN);
	c_sec = pmu_read8(g_pmu_dev, MAILBOX_GET_SEC);
	c_year = (c_year_h*100)+c_year_l;
	c_time = (c_sec ) | (c_min << 6 ) | (c_hour << 12 ) | (c_day << 17 ) | ((c_month & 0xF) << 22 ) | (c_year_l << 26);

	if((c_month > 12) || (c_day > 31) || (c_min > 60) || (c_sec > 60))
	{
		*time_cur = 0;
		return 1;
	}
	else
	{
		*time_cur = c_time;
	}

	return 0;
}
EXPORT_SYMBOL(ali_pmu_get_time);

void main_cpu_read_time_init(void)
{
	g_year_h = pmu_read8(g_pmu_dev, MAILBOX_GET_YEAR_H);
	g_year_l = pmu_read8(g_pmu_dev, MAILBOX_GET_YEAR_L);
	g_month = pmu_read8(g_pmu_dev, MAILBOX_GET_MONTH);
	g_day = pmu_read8(g_pmu_dev, MAILBOX_GET_DAY);
	g_hour = pmu_read8(g_pmu_dev, MAILBOX_GET_HOUR);
	g_min = pmu_read8(g_pmu_dev, MAILBOX_GET_MIN);
	g_sec = pmu_read8(g_pmu_dev, MAILBOX_GET_SEC);
	g_year = (g_year_h*100) + g_year_l;
#ifdef CONFIG_ALI_PMU_DEBUG
	printk(KERN_EMERG "\n============>MCU Current Time:[year:%d], [month:%d], [day:%d], [hour:%d], [min:%d], [sec:%d]\n", \
		g_year, g_month, g_day, g_hour, g_min, g_sec);
#endif
}

void enter_standby_reset_mcu(void)
{
#ifdef CONFIG_ALI_CHIP_M3921
	soc_write32(g_pmu_dev, PMU_IP_RESET_REG, soc_read32(g_pmu_dev, PMU_IP_RESET_REG) & PMU_MCU_RESET_ENABLE);
#else
	soc_write32(g_pmu_dev, PMU_IP_RESET_REG, soc_read32(g_pmu_dev, PMU_IP_RESET_REG) | PMU_MCU_RESET_ENABLE);
#endif
}

void set_pmu_clksel(unsigned char clksel)
{
	pmu_write8(g_pmu_dev, PMU_CFG_SET, (pmu_read8(g_pmu_dev, PMU_CFG_SET) & 0xc0) | clksel);
}

void pan_pmu_power_key_init(unsigned int type1, unsigned int type2)
{
	unsigned char clk_sel = 0x5;
	unsigned char reg = 0;

	pmu_write8(g_pmu_dev, PMU_CFG_SET, pmu_read8(g_pmu_dev, PMU_CFG_SET) & (~(0x1<<7)));/*key pmu_disable.*/
	set_pmu_clksel(clk_sel);/*clk_sel=5.*/

	/*about 0.2s==>>>0x3e80 (UINT8)(4000*4/256)*/
	pmu_write8(g_pmu_dev, PMU_PRS_KEY_CFG, 0x24);/*about 0.1s==>>>0x1f40 (UINT8)(4000*2/256).*/
	pmu_write8(g_pmu_dev, PMU_PRS_KEY_CFG+1, 0xf4);
	pmu_write8(g_pmu_dev, PMU_PRS_KEY_CFG+2, 0x0);
	pmu_write8(g_pmu_dev, PMU_PRS_KEY_CFG+3, pmu_read8(g_pmu_dev, PMU_PRS_KEY_CFG+3)&0xf0);

	/*KEY_DISABLE_VAL=n us*1.5/(4*(5+1)).*/
	if(type2)
	{
		pmu_write8(g_pmu_dev, KEY_DISABLE_VAL_LOW0, 0x6a);
		pmu_write8(g_pmu_dev, KEY_DISABLE_VAL_LOW1, 0x18);/*0.5 second.*/
		pmu_write8(g_pmu_dev, KEY_DISABLE_VAL_LOW2, 0x0);
		pmu_write8(g_pmu_dev, KEY_DISABLE_EN, 0x80);

		reg = pmu_read8(g_pmu_dev, MCU_SYS_IPR);/*MCU_Polar.*/
		//reg |= ((1<<3) | (1<<6));/*KEY_INT_STANDBY and KEY_INT_NORM.*/
		reg |= (1<<3);
		pmu_write8(g_pmu_dev, MCU_SYS_IPR, reg);
		reg = pmu_read8(g_pmu_dev, MCU_SYS_IER);/*MCU_INT_EN.*/
		//reg |= ((1<<3) | (1<<6));/*KEY_INT_STANDBY and KEY_INT_NORM.*/
		reg |= (1<<3);
		pmu_write8(g_pmu_dev, MCU_SYS_IER, reg);
		pmu_write8(g_pmu_dev, PMU_PRS_KEY_CFG+3, pmu_read8(g_pmu_dev, PMU_PRS_KEY_CFG+3) | (0x1<<7));
	}
	else
	{
		pmu_write8(g_pmu_dev, PMU_PRS_KEY_CFG+3, pmu_read8(g_pmu_dev, PMU_PRS_KEY_CFG+3) & (~(0x1<<7)));/*key pmu_disable.*/
	}
}

void pmu_hw_init(void)
{
	if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3503 == ali_chip_id) \
		|| (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id) \
		|| (ALI_PMU_M3922 == ali_chip_id))
	{
		main_cpu_read_time_init();
		enter_standby_reset_mcu();
		pmu_write8(g_pmu_dev, PANEL_POWER_KEY1_ADDR, (u8)g_pmu_dev->panel_power_key);
		set_mcu_panel_show_type(MCU_SHOW_OFF);/*Default show " OFF" in pmu standby.*/
		pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_READ_ADDR, pmu_read32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR));
		pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR, ALI_WDT_REBOOT);
	}
}

/*when STB system enter standby,call pmu_mcu_enter_stby_timer_set_value function to set pmu time.*/
void pmu_mcu_enter_stby_timer_set_value(struct rtc_time_pmu *base_time)
{
	pmu_write8(g_pmu_dev, MAILBOX_SET_SEC, base_time->sec);
	pmu_write8(g_pmu_dev, MAILBOX_SET_MIN, base_time->min);
	pmu_write8(g_pmu_dev, MAILBOX_SET_HOUR, base_time->hour);
	pmu_write8(g_pmu_dev, MAILBOX_SET_DAY, base_time->date);
	pmu_write8(g_pmu_dev, MAILBOX_SET_MONTH, base_time->month);
	pmu_write8(g_pmu_dev, MAILBOX_SET_YEAR_L, (unsigned char)(base_time->year%100));
	pmu_write8(g_pmu_dev, MAILBOX_SET_YEAR_H, (unsigned char)(base_time->year/100));

	g_bak_set_sec = base_time->sec;
	g_bak_set_min = base_time->min;
	g_bak_set_hour = base_time->hour;
	g_bak_set_day = base_time->date;
	g_bak_set_month = base_time->month;
	g_bak_set_year_h = ((unsigned char)(base_time->year/100));
	g_bak_set_year_l = ((unsigned char)(base_time->year%100));

#ifdef CONFIG_ALI_PMU_DEBUG
	printk(KERN_EMERG "\n========>Received current time is [year_h:%d], [year_l:%d], [mon:%d], [day:%d], [hour:%d], [min:%d], [sec:%d]\n", \
		pmu_read8(g_pmu_dev, MAILBOX_SET_YEAR_H), pmu_read8(g_pmu_dev, MAILBOX_SET_YEAR_L), \
		pmu_read8(g_pmu_dev, MAILBOX_SET_MONTH), pmu_read8(g_pmu_dev, MAILBOX_SET_DAY), \
		pmu_read8(g_pmu_dev, MAILBOX_SET_HOUR), pmu_read8(g_pmu_dev, MAILBOX_SET_MIN), \
		pmu_read8(g_pmu_dev, MAILBOX_SET_SEC));
#endif
}

/*When STB system work in norm mode, call rtc_set_value funciton to set current time.*/
void rtc_set_value(struct rtc_time_pmu *base_time, enum SB_TIMER timer_flag)
{
	spin_lock(&rtc_lock);

	if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3503 == ali_chip_id) \
		|| (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id) \
		|| (ALI_PMU_M3922 == ali_chip_id))
	{
		rtc_upper = 0xFFFFFFFF;
		rtc_onesec = 1000000*SB_CLK_RTC/RTC_DIV[RTC_DIV_32];
		rtc_onesec = rtc_upper -rtc_onesec;
		soc_write32(g_pmu_dev, SB_TIMER3_CNT, (rtc_onesec & 0xFFFFFFFF));
		soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, 0x0);

		/*RTC starts to count with 32 divisor clock frequency, and enable interrupt.*/
		/*interrupt (enable) and start countings.*/
		soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) | ALI_SB_TIMER_ENABLE);

		rtc_sec = base_time->sec;
		rtc_min = base_time->min;
		rtc_hour = base_time->hour;
		rtc_date = base_time->date;
		rtc_month = base_time->month;
		rtc_year = base_time->year;

		printk(KERN_EMERG "\n============>APP io-ctrl call [Function: %s, Line %d] set current time:[year:%d], [month:%d], [day:%d], [hour:%d], [min:%d], [sec:%d]\n", \
			__FUNCTION__, __LINE__, rtc_year, rtc_month, rtc_date, rtc_hour, rtc_min, rtc_sec);
	}

	spin_unlock(&rtc_lock);
}

void rtc_init(void)
{
	if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3503 == ali_chip_id) \
		|| (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id) \
		|| (ALI_PMU_M3922 == ali_chip_id))
	{
		rtc_upper = 0xFFFFFFFF;
		rtc_onesec = 1000000*SB_CLK_RTC/RTC_DIV[RTC_DIV_32];
		rtc_onesec = rtc_upper - rtc_onesec;
		soc_write32(g_pmu_dev, SB_TIMER3_CNT, (rtc_onesec & 0xFFFFFFFF));
		soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, 0x0);

		soc_write8(g_pmu_dev, SB_TIMER0_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER0_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
		soc_write8(g_pmu_dev, SB_TIMER0_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER0_CTL_REG) | (1<<3));
		soc_write8(g_pmu_dev, SB_TIMER1_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER1_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
		soc_write8(g_pmu_dev, SB_TIMER1_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER1_CTL_REG) | (1<<3));
		soc_write8(g_pmu_dev, SB_TIMER2_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER2_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
		soc_write8(g_pmu_dev, SB_TIMER2_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER2_CTL_REG) | (1<<3));
		soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, (soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) | ALI_SB_TIMER_ENABLE));/*interrupt3 (enable).*/
		soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) | (1<<3));

		/*Masked by tony because linux kernel will use timer4 as the system timer.*/
		if((ALI_PMU_M3921 != ali_chip_id) || (ALI_PMU_M3922 == ali_chip_id))
		{
			soc_write8(g_pmu_dev, SB_TIMER4_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER4_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
			soc_write8(g_pmu_dev, SB_TIMER4_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER4_CTL_REG) | (1<<3));
		}
		soc_write8(g_pmu_dev, SB_TIMER5_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER5_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
		soc_write8(g_pmu_dev, SB_TIMER5_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER5_CTL_REG) | (1<<3));
		soc_write8(g_pmu_dev, SB_TIMER6_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER6_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
		soc_write8(g_pmu_dev, SB_TIMER6_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER6_CTL_REG) | (1<<3));
		soc_write8(g_pmu_dev, SB_TIMER7_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER7_CTL_REG) & (~((0x1 << 2) | (0x1 <<4))));
		soc_write8(g_pmu_dev, SB_TIMER7_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER7_CTL_REG) | (1<<3));
	}
}

void rtc_time_read_value(enum SB_TIMER timer_flag, struct rtc_time_pmu *g_rtc_read)
{
	spin_lock(&rtc_lock);
	if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3503 == ali_chip_id) \
		|| (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id) \
		|| (ALI_PMU_M3922 == ali_chip_id))
	{
		g_rtc_read->year = rtc_year;
		g_rtc_read->month = rtc_month;
		g_rtc_read->date = rtc_date;
		g_rtc_read->hour = rtc_hour;
		g_rtc_read->min = rtc_min;
		g_rtc_read->sec = rtc_sec;
	#ifdef CONFIG_ALI_PMU_DEBUG
		printk(KERN_EMERG "\n========>[Function %s, Line %d] : [year:%d], [month:%d], [date:%d], [hour:%d], [min:%d], [sec:%d]\n", \
			__FUNCTION__, __LINE__, g_rtc_read->year, g_rtc_read->month, g_rtc_read->date, g_rtc_read->hour, \
			g_rtc_read->min, g_rtc_read->sec);
	#endif
	}

	spin_unlock(&rtc_lock);
}

/*
when STB system enter normer mode from standby mode(standby mode->normer mode), \
call rtc_time_init_value function to set STB system time.
*/
struct rtc_time_pmu rtc_time_init_value(void)/*from standby mode=>norm mode.*/
{
	struct rtc_time_pmu base_time;

	memset(&base_time, 0, sizeof(struct rtc_time_pmu));
	if((ALI_PMU_M3503 == ali_chip_id) || (ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3821 == ali_chip_id) \
		|| (ALI_PMU_M3505 == ali_chip_id) || (ALI_PMU_M3922 == ali_chip_id))
	{
		base_time.year = g_year;
		base_time.month = g_month;
		base_time.date = g_day;
		base_time.day = 0;
		base_time.hour = g_hour;
		base_time.min = g_min;
		base_time.sec = g_sec;
	}

	return base_time;
}

/*
when STB system enter standby mode from normer mode(normer mode->standby mode), \
call pmu_mcu_wakeup_timer_set_min_alarm function to set STB system wakeup time.
*/
void pmu_mcu_wakeup_timer_set_min_alarm(struct min_alarm *alarm, unsigned char num)
{
	if((1 > alarm->month) || (12 < alarm->month) || (1 > alarm->date) \
		|| (31 < alarm->date) || (0 > alarm->hour) || (23 < alarm->hour) \
		|| (0 > alarm->min) || (59 < alarm->min) || (0 > alarm->second) \
		|| (59 < alarm->second))
	{
		printk(KERN_EMERG "\n================>The time that you input is not valid!!!!\n");
	}

	g_bak_wake_month = alarm->month;
	g_bak_wake_day = alarm->date;
	g_bak_wake_hour = alarm->hour;
	g_bak_wake_min = alarm->min;
	g_bak_wake_sec = alarm->second;

	pmu_write8(g_pmu_dev, MAILBOX_WAKE_MONTH, alarm->month);
	pmu_write8(g_pmu_dev, MAILBOX_WAKE_DAY, alarm->date);
	pmu_write8(g_pmu_dev, MAILBOX_WAKE_HOUR, alarm->hour);
	pmu_write8(g_pmu_dev, MAILBOX_WAKE_MIN, alarm->min);
	pmu_write8(g_pmu_dev, MAILBOX_WAKE_SECOND, alarm->second);

#ifdef CONFIG_ALI_PMU_DEBUG
	printk(KERN_EMERG "\nSet wakeup time:[month:%d], [date:%d], [hour:%d], [min:%d], [sec:%d]\n", \
		pmu_read8(g_pmu_dev, MAILBOX_WAKE_MONTH), pmu_read8(g_pmu_dev, MAILBOX_WAKE_DAY), pmu_read8(g_pmu_dev, MAILBOX_WAKE_HOUR), \
		pmu_read8(g_pmu_dev, MAILBOX_WAKE_MIN), pmu_read8(g_pmu_dev, MAILBOX_WAKE_SECOND));
#endif
}

static void pmu_mcu_wakeup_ir_power_key(unsigned int *pmu_ir_key)
{
	unsigned int i = 0;
	unsigned int pmu_ir_key_51[8] = {0};

	for(i=0; i<8; i++)
	{
		if(pmu_ir_key[i] == PAN_KEY_INVALID)
		{
			pmu_ir_key_51[i] = DEFAULT_IR_DATA;
		}
		else
		{
			pmu_ir_key_51[i] = pmu_ir_key[i];
		}
	}

	for(i=0; i<8; i++)
	{
		pmu_write32(g_pmu_dev, (IR_BUFFER_BASE+i*4), pmu_ir_key_51[i]);
	}
}

void pmu_wakeup_keyval(unsigned int power_key)
{
	pmu_write8(g_pmu_dev, NEC_IR_KEY_SARM_LOW0, (unsigned char)(power_key&0xff));
	pmu_write8(g_pmu_dev, NEC_IR_KEY_SARM_LOW1, (unsigned char)((power_key>>8)&0xff));
	pmu_write8(g_pmu_dev, NEC_IR_KEY_SARM_LOW2, (unsigned char)((power_key>>16)&0xff));
	pmu_write8(g_pmu_dev, NEC_IR_KEY_SARM_LOW3, (unsigned char)((power_key>>24)&0xff));
}

void set_mcu_panel_show_type(unsigned char show_type)
{
	pmu_write8(g_pmu_dev, STANDBY_SHOW_TIMR_SARM, show_type);
}

void cpu_release_sram_to_mcu(void)
{
	/*Reference programming guide, after this 3 register setting, MCU should be reset and running up.*/
#ifndef CONFIG_ALI_CHIP_M3921
	soc_write32(g_pmu_dev, PMU_IP_RESET_REG, soc_read32(g_pmu_dev, PMU_IP_RESET_REG) | PMU_MCU_RESET_ENABLE);
#else
	soc_write32(g_pmu_dev, PMU_IP_RESET_REG, soc_read32(g_pmu_dev, PMU_IP_RESET_REG) & PMU_MCU_RESET_ENABLE);
#endif
	/*Release pmu ram to MCU.*/
	pmu_write8(g_pmu_dev, PMU_RAM_SWITCH_REG, (pmu_read8(g_pmu_dev, PMU_RAM_SWITCH_REG) & PMU_SRAM_RELEASE_TO_MCU_ENABLE));

#ifndef CONFIG_ALI_CHIP_M3921
	soc_write32(g_pmu_dev, PMU_IP_RESET_REG, soc_read32(g_pmu_dev, PMU_IP_RESET_REG) & PMU_MCU_RESET_DISABLE);/*reset MCU.*/
#else
	soc_write32(g_pmu_dev, PMU_IP_RESET_REG, soc_read32(g_pmu_dev, PMU_IP_RESET_REG) | PMU_MCU_RESET_DISABLE);/*reset MCU.*/
#endif
}

static void soc_power_off_before_standby(void)
{
	/*Close unused IP power.*/
	if(ALI_PMU_M3821 == ali_chip_id)
	{
		Cut_Down_IP_Power();
	}
	else if(ALI_PMU_M3505 == ali_chip_id)
	{
		/*bit2:MAIN_OCP_ERROR_RESP_ENABLE.
		*bit1:Error Access request record enables.
		*bit0:Enable Device IO OT function.
		*/
		soc_write32(g_pmu_dev, NB_ERROR_ACCESS_HANDING_REG,
			soc_read32(g_pmu_dev, NB_ERROR_ACCESS_HANDING_REG)
			| M3505_ENABLE_TIMEOUT_HANDLING);

		/*IP reset.*/
		soc_write32(g_pmu_dev, ALI_IP_RESET_REG1,
			soc_read32(g_pmu_dev, ALI_IP_RESET_REG1) | C3505_RESET_REG1_MASK);
		pmu_delay();
		soc_write32(g_pmu_dev, ALI_IP_RESET_REG2,
			soc_read32(g_pmu_dev, ALI_IP_RESET_REG2) | C3505_RESET_REG2_MASK);
		pmu_delay();
	}
}

void pmu_enter_standby(void)
{
	/*Close watch dog.*/
	soc_write32(g_pmu_dev, ALI_WDT_REG, 0x0);

	/*Close PMU gpios.*/
	pmu_write8(g_pmu_dev, PMU_GPIO_CTRL_REG1, 0x0);
	pmu_write8(g_pmu_dev, PMU_GPIO_CTRL_REG2, 0x0);
	output_string("1");

	/*Clear WDT reboot flag.*/
	pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_READ_ADDR, 0x0);
	pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR, 0x0);

	/*Disable cpu interrupt.*/
	ali_disable_all_ip_interrupt();
	output_string("2");
	local_irq_disable();

	enter_standby_reset_mcu();
	output_string("3");

	soc_power_off_before_standby();
	output_string("4");

	while(1)
	{
		memcpy_toio(g_pmu_dev->ali_pmu_base_addr, (u8*)g_ali_pmu_bin, sizeof(g_ali_pmu_bin));
		ali_disable_all_ip_interrupt();
		pmu_delay();
		output_string("5");

		pmu_mcu_wakeup_ir_power_key(wakeup_power_key);
		pmu_set_value_and_set_waketime();
		pmu_delay();
		output_string("6");

		cpu_release_sram_to_mcu();
		pmu_delay();
		output_string("7");

		/*Set CPU to MCU interrupt enable, later will send interrupt to MCU.*/
		pmu_write8(g_pmu_dev, CPU_TO_MCU_INTERRUPT_ENABLE_REG,
			(pmu_read8(g_pmu_dev, CPU_TO_MCU_INTERRUPT_ENABLE_REG)
			| INTERRUPT_ENABLE));

		/*
		*set interrupt to MCU, after MCU receive will cut main IC power.
		*You can reference PMU source code: extern_int2() in sys.c.
		*/
		pmu_write8(g_pmu_dev, CPU_TO_MCU_INTERRUPT_STATUS_REG,
			(pmu_read8(g_pmu_dev, CPU_TO_MCU_INTERRUPT_STATUS_REG)
			| INTERRUPT_STATUS_CLEAR));
		output_string("8");

		/*Wait MCU to close power.*/
		wait_for_standby();
		output_string("9");
	}
}

static void pmu_mcu_uart_enable(void)
{
	if (ali_chip_id == ALI_PMU_M3821) {
		/*bit2:XUART_TX_SEL.*/
		soc_write32(g_pmu_dev, 0x488,
			(soc_read32(g_pmu_dev, 0x488)
			& ~(1<<2)));
		/*bit17:PMU DOCD Test Pin Select;
		*bit16:PMU Uart Test pin Select.
		*/
		soc_write32(g_pmu_dev, 0x494,
			(soc_read32(g_pmu_dev, 0x494)
			| (1<<17) | (1<<16)));
		/*uart from xpmu_gpio[0] and xpmu_gpio[1].*/
		pmu_write32(g_pmu_dev, MCU_UART_SELECT_REG,
			(pmu_read32(g_pmu_dev, MCU_UART_SELECT_REG)
			| XPMU_GPIO_UART_SELECTED));
		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_MODE_ENABLE));
		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			| MCU_ICE_MODE_TRIGGER));
		pmu_delay();
		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_TRIGGER_CLEAR));
	} else if (ali_chip_id == ALI_PMU_M3503) {
		/*bit24:PK144_UART_2TX_SEL;
		*bit23:PK144_UART1_SEL.
		*/
		soc_write32(g_pmu_dev, 0x88,
			(soc_read32(g_pmu_dev, 0x88)
			& ~((1<<24) | (1<<23))));

		soc_write32(g_pmu_dev, 0x8C,
			(soc_read32(g_pmu_dev, 0x8C)
			| (1<<22)));/*MCU_DEBUG_SYS_EN bit.*/

		/*uart from xpmu_gpio[0] and xpmu_gpio[1].*/
		pmu_write32(g_pmu_dev, MCU_UART_SELECT_REG,
			(pmu_read32(g_pmu_dev, MCU_UART_SELECT_REG)
			| XPMU_GPIO_UART_SELECTED));

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_MODE_ENABLE));

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			| MCU_ICE_MODE_TRIGGER));
		pmu_delay();
		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_TRIGGER_CLEAR));

		/*GPIO[110] shared with XMCU_UART_TXD pin.*/
		soc_write32(g_pmu_dev, 0x43c,
			(soc_read32(g_pmu_dev, 0x43c) & ~(1<<14)));
	} else if (ali_chip_id == ALI_PMU_M3505) {
		/*bit7:XUART2_RX_SEL; bit6:XUART2_TX_SEL.*/
		soc_write32(g_pmu_dev, 0x488,
			(soc_read32(g_pmu_dev, 0x488)
			& ~((1<<7) | (1<<6))));

		/*bit2:XUART_TX_SEL; bit1:XUART_RX_SEL.*/
		soc_write32(g_pmu_dev, 0x488,
			(soc_read32(g_pmu_dev, 0x488)
			& ~((1<<2) | (1<<1))));

		/*uart from xpmu_gpio[0] and xpmu_gpio[1].*/
		pmu_write32(g_pmu_dev, MCU_UART_SELECT_REG,
			(pmu_read32(g_pmu_dev, MCU_UART_SELECT_REG)
			| XPMU_GPIO_UART_SELECTED));

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_MODE_ENABLE));

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			| MCU_ICE_MODE_TRIGGER));
		pmu_delay();

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_TRIGGER_CLEAR));
	} else if (ali_chip_id == ALI_PMU_M3921) {
		/*Disable function that shared with DOCD.*/
		soc_write32(g_pmu_dev, 0x88,
			(soc_read32(g_pmu_dev, 0x88)
			& ~((1<<16) | (1<<15) | (1<<13))));

		/*MCU_UART_SEL.*/
		soc_write32(g_pmu_dev, 0x8C,
			(soc_read32(g_pmu_dev, 0x8C) | (1<<10)));

		/*GPIO[1] shared with MCU_DOCE_DATA1 pin;
		*GPIO[2] shared with MCU_DOCE_DATA0 pin.
		*/
		soc_write32(g_pmu_dev, 0x430,
			(soc_read32(g_pmu_dev, 0x430)
			& ~((1<<2) | (1<<1))));

		/*GPIO[114] shared with MCU_DOCD_CLK pin.*/
		soc_write32(g_pmu_dev, 0x43C,
			(soc_read32(g_pmu_dev, 0x43C)
			& ~(1<<18)));

		/*uart from xpmu_gpio[0] and xpmu_gpio[1].*/
		pmu_write32(g_pmu_dev, MCU_UART_SELECT_REG,
			(pmu_read32(g_pmu_dev, MCU_UART_SELECT_REG)
			| XPMU_GPIO_UART_SELECTED));

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& (MCU_ICE_MODE_ENABLE)));

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			| MCU_ICE_MODE_TRIGGER));
		pmu_delay();

		pmu_write32(g_pmu_dev, ICE_MODE_SET_REG,
			(pmu_read32(g_pmu_dev, ICE_MODE_SET_REG)
			& MCU_ICE_TRIGGER_CLEAR));
	} 
}

long ali_pmu_ioctl(struct file *file, unsigned int cmd, unsigned long param)
{
	struct rtc_time_pmu base_time;
	struct min_alarm_num min_alm_num;
	unsigned char read_status = 0;
	void __user *argp = (void __user *)param;
	struct rtc_time_pmu rtc_read;
	struct rtc_time_pmu rtc_read_init;
	enum MCU_SHOW_PANNEL flag;
	u8 ret = 0;
	u8 powerup_status = 0;
	unsigned int wdt_reboot_status = 0;

    printk(KERN_ALERT "[ %s %d ]: cmd %d\n", __FUNCTION__, __LINE__, cmd-ALI_PMU_IO_COMMAND_BASE);
	ali_pmu_get_chip_info();
	if((ALI_PMU_M3503 == ali_chip_id) || (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id))
	{
		switch(cmd)
		{
			case ALI_PMU_SET_SHOW_PANEL_TYPE:
				if(get_user(flag, (char __user *)argp))
				{
					return -EFAULT;
				}
				set_mcu_panel_show_type((u8)flag);
				break;

			case ALI_PMU_RTC_SET_VAL:/*when norm mode use south rtc.*/
				ret = copy_from_user(&base_time, (struct rtc_time_pmu *)param, sizeof(struct rtc_time_pmu));
				rtc_set_value(&base_time, SB_TIMER_0);
				break;

			case ALI_PMU_RTC_TIMER3_SET_VAL:
				ret = copy_from_user(&base_time, (struct rtc_time_pmu *)param, sizeof(struct rtc_time_pmu));
				rtc_set_value(&base_time, SB_TIMER_3);
				break;

			case ALI_PMU_RTC_RD_VAL:
				rtc_time_read_value(SB_TIMER_0, &rtc_read);
				ret = copy_to_user(argp, &rtc_read, sizeof(struct rtc_time_pmu));
				break;

			case ALI_PMU_RTC_TIMER3_GET_VAL:
				rtc_time_read_value(SB_TIMER_3, &rtc_read);
				ret = copy_to_user(argp, &rtc_read, sizeof(struct rtc_time_pmu));
				break;

			case ALI_PMU_MCU_ENTER_STANDBY:
				pmu_mcu_wakeup_ir_power_key(wakeup_power_key);

				printk(KERN_EMERG "\nFunction:%s Line:%d, enter standby!\n", __FUNCTION__, __LINE__);
				pmu_enter_standby();
				break;

			case ALI_PMU_MCU_SET_TIME:/*through mailbox set system time, when enter standby mode.*/
				ret = copy_from_user(&base_time, (struct rtc_time_pmu *)param, sizeof(struct rtc_time_pmu));
				pmu_mcu_enter_stby_timer_set_value(&base_time);
				break;

			case ALI_PMU_MCU_READ_TIME:/* from standy mode -->normer mode, read mcu rtc init value.*/
				rtc_read_init = rtc_time_init_value();
				ret = copy_to_user(argp, &rtc_read_init, sizeof(struct rtc_time_pmu));
				break;

			case ALI_PMU_MCU_WAKEUP_TIME:/*when normer mode ->standby mode ,set wake up time, through mailbox.*/
				ret = copy_from_user(&min_alm_num, (struct min_alarm_num *)param, sizeof(struct min_alarm_num));
				pmu_mcu_wakeup_timer_set_min_alarm(&(min_alm_num.min_alm), min_alm_num.num);
				break;

			case ALI_PMU_EXIT_STANDBY_STATUS:
				read_status = g_exit_standby_sts;
				ret = copy_to_user((unsigned int *)param, &read_status, sizeof(unsigned char));
				break;

			case ALI_PMU_IR_PROTOL_NEC:
			case ALI_PMU_IR_KEY_SET:
				ret = copy_from_user(wakeup_power_key, (unsigned int *)param, 8*sizeof(unsigned int));
				pmu_mcu_wakeup_ir_power_key(wakeup_power_key);
				break;

			case ALI_PMU_REPORT_EXIT_TYPE:
				powerup_status = get_powerup_status();
				ret = copy_to_user((unsigned int *)param, &powerup_status, sizeof(unsigned char));
				break;

			case ALI_PMU_SAVE_WDT_REBOOT_FLAG:
				ret = copy_from_user(&wdt_reboot_status, (unsigned int *)param, sizeof(unsigned int));
                printk(KERN_ALERT "[%s %d ]:ALI_PMU_SAVE_WDT_REBOOT_FLAG:  wdt_reboot_status 0x%x\n", __FUNCTION__, __LINE__, wdt_reboot_status);
				pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR, wdt_reboot_status);
				break;

			case ALI_PMU_GET_WDT_REBOOT_FLAG:
				wdt_reboot_status = pmu_read32(g_pmu_dev, WDT_REBOOT_FLAG_READ_ADDR);
                printk(KERN_ALERT "[%s %d ]:ALI_PMU_GET_WDT_REBOOT_FLAG:  wdt_reboot_status 0x%x\n", __FUNCTION__, __LINE__, wdt_reboot_status);
				ret = copy_to_user((unsigned int *)param, &wdt_reboot_status, sizeof(unsigned int));
				break;

			case ALI_PMU_MCU_UART_ENABLE:
				pmu_mcu_uart_enable();
				break;

			default:
				printk(KERN_EMERG "\nFunction:%s Line:%d, invalid cmd %x !\n", __FUNCTION__, __LINE__, cmd);
				break;
		}
	}
	else if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3922 == ali_chip_id))
	{
		switch(cmd)
		{
			case ALI_PMU_SET_SHOW_PANEL_TYPE:
				if(get_user(flag, (char __user *)argp))
				{
					return -EFAULT;
				}
				set_mcu_panel_show_type((u8)flag);
				break;

			case ALI_PMU_RTC_SET_VAL:/*when norm mode use south rtc.*/
				ret = copy_from_user(&base_time, (struct rtc_time_pmu *)param, sizeof(struct rtc_time_pmu));
				rtc_set_value( &base_time, SB_TIMER_0);
				break;

			case ALI_PMU_RTC_RD_VAL:/*when norm mode use south rtc.*/
				rtc_time_read_value(SB_TIMER_0, &rtc_read);
				ret = copy_to_user(argp, &rtc_read, sizeof(struct rtc_time_pmu));
				break;

			case ALI_PMU_MCU_ENTER_STANDBY:
				pmu_mcu_wakeup_ir_power_key(wakeup_power_key);

				printk(KERN_EMERG "\nFunction:%s Line:%d, enter standby!\n", __FUNCTION__, __LINE__);
				pmu_enter_standby();
				break;

			case ALI_PMU_MCU_SET_TIME:/*through mailbox set system time, when enter standby mode.*/
				ret = copy_from_user(&base_time, (struct rtc_time_pmu *)param, sizeof(struct rtc_time_pmu));
				pmu_mcu_enter_stby_timer_set_value(&base_time);
				break;

			case ALI_PMU_MCU_READ_TIME:/*from standy mode -->normer mode, read mcu rtc init value.*/
				rtc_read_init = rtc_time_init_value();
				ret = copy_to_user(argp, &rtc_read_init, sizeof(struct rtc_time_pmu));
				break;

			case ALI_PMU_MCU_WAKEUP_TIME:/*when normer mode ->standby mode, set wake up time,through mailbox.*/
				ret = copy_from_user(&min_alm_num, (struct min_alarm_num *)param, sizeof(struct min_alarm_num));
				pmu_mcu_wakeup_timer_set_min_alarm(&(min_alm_num.min_alm), min_alm_num.num);
				break;

			case ALI_PMU_IR_PROTOL_NEC:
			case ALI_PMU_IR_KEY_SET:
				ret = copy_from_user(wakeup_power_key, (unsigned int *)param, 8*sizeof(unsigned int));
				pmu_mcu_wakeup_ir_power_key(wakeup_power_key);
				#ifdef CONFIG_ALI_PMU_DEBUG
				//printk("\nwakeup_power_key[0]=0x%08X", wakeup_power_key[0]);
				//printk("\nwakeup_power_key[1]=0x%08X", wakeup_power_key[1]);
				//printk("\nwakeup_power_key[2]=0x%08X", wakeup_power_key[2]);
				//printk("\nwakeup_power_key[3]=0x%08X", wakeup_power_key[3]);
				//printk("\nwakeup_power_key[4]=0x%08X", wakeup_power_key[4]);
				//printk("\nwakeup_power_key[5]=0x%08X", wakeup_power_key[5]);
				//printk("\nwakeup_power_key[6]=0x%08X", wakeup_power_key[6]);
				//printk("\nwakeup_power_key[7]=0x%08X\n", wakeup_power_key[7]);
				#endif
				break;

			case ALI_PMU_REPORT_EXIT_TYPE:
				powerup_status = get_powerup_status();
				ret = copy_to_user((unsigned int *)param, &powerup_status, sizeof(unsigned char));
				break;

			case ALI_PMU_SAVE_WDT_REBOOT_FLAG:
				ret = copy_from_user(&wdt_reboot_status, (unsigned int *)param, sizeof(unsigned int));
                printk(KERN_ALERT "[%s %d ]:ALI_PMU_SAVE_WDT_REBOOT_FLAG:  wdt_reboot_status 0x%x\n", __FUNCTION__, __LINE__, wdt_reboot_status);
				pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR, wdt_reboot_status);
				break;

			case ALI_PMU_GET_WDT_REBOOT_FLAG:
				wdt_reboot_status = pmu_read32(g_pmu_dev, WDT_REBOOT_FLAG_READ_ADDR);
                printk(KERN_ALERT "[%s %d ]:ALI_PMU_GET_WDT_REBOOT_FLAG:  wdt_reboot_status 0x%x\n", __FUNCTION__, __LINE__, wdt_reboot_status);
				ret = copy_to_user((unsigned int *)param, &wdt_reboot_status, sizeof(unsigned int));
				break;

			default:
				printk(KERN_EMERG "\nFunction:%s Line:%d, invalid cmd %x !\n", __FUNCTION__, __LINE__, cmd);
				break;
		}
	}

	return 0;
}

static irqreturn_t ali_pmu_interrupt(int irq, void *dev_id)
{
	if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3503 == ali_chip_id) \
		|| (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id) \
		|| (ALI_PMU_M3922 == ali_chip_id))
	{
		if(soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) & RTC_TOV)/*SB-Time3 enbale.*/
		{
			soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, (soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) & ALI_SB_TIME_DIS));/*clear interrupt(disable).*/
			time_accumulate(NULL);
			soc_write32(g_pmu_dev, SB_TIMER3_CNT, rtc_onesec & 0xFFFFFFFF);
			soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, 0x00);

			/*RTC starts to count with 32 divisor clock frequency, and enable interrupt.*/
			soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, (soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) | ALI_SB_TIMER_ENABLE));/*interrupt again(enable).*/
		}
	}

	return IRQ_HANDLED;
}

PMU_EXIT_FLAG get_powerup_status(void)
{
	PMU_EXIT_FLAG pmu_exit_flag = UNKOWN_WAKEUP_TYPE;
	unsigned char pmu_exit_standby_sts = 0;
#ifdef CONFIG_ALI_PMU_DEBUG
	unsigned char temp_data1 = 0;
	unsigned char temp_data2 = 0;
	unsigned char temp_data3 = 0;
	printk(KERN_EMERG "\nFunction:%s, Line:%d", __FUNCTION__, __LINE__);
#endif

	if((ALI_PMU_M3921 == ali_chip_id) || (ALI_PMU_M3503 == ali_chip_id) \
		|| (ALI_PMU_M3821 == ali_chip_id) || (ALI_PMU_M3505 == ali_chip_id) \
		|| (ALI_PMU_M3922 == ali_chip_id))
	{
	#ifdef CONFIG_ALI_PMU_DEBUG
		temp_data1 = pmu_read8(g_pmu_dev, MAILBOX_GET_EXIT_STANDBY_STATUS0);
		temp_data2 = pmu_read8(g_pmu_dev, MAILBOX_GET_EXIT_STANDBY_STATUS1);
		temp_data3 = pmu_read8(g_pmu_dev, MAILBOX_GET_EXIT_STANDBY_STATUS2);
		pmu_exit_standby_sts = pmu_read8(g_pmu_dev, EXIT_STANDBY_TYPE_REG);
		printk(KERN_EMERG "\ntemp_data1: 0x%02X", temp_data1);
		printk(KERN_EMERG "\ntemp_data2: 0x%02X", temp_data2);
		printk(KERN_EMERG "\ntemp_data3: 0x%02X", temp_data3);
		printk(KERN_EMERG "\npmu_exit_standby_sts: 0x%02X\n", pmu_exit_standby_sts);
	#endif
		if((pmu_read8(g_pmu_dev, MAILBOX_GET_EXIT_STANDBY_STATUS2) != 'U') \
			|| (pmu_read8(g_pmu_dev, MAILBOX_GET_EXIT_STANDBY_STATUS1) != 'M') \
			|| (pmu_read8(g_pmu_dev, MAILBOX_GET_EXIT_STANDBY_STATUS0) != 'P'))
		{
			pmu_exit_flag = E_PMU_COLD_BOOT;
			printk(KERN_EMERG "\n<Linux Kernel>Platform boot type is not standby!!!!\n");
			return pmu_exit_flag;
		}
	}

	pmu_exit_standby_sts = pmu_read8(g_pmu_dev, EXIT_STANDBY_TYPE_REG);
	if(EXIT_STANDBY_TYPE_PANEL == pmu_exit_standby_sts)
	{
		pmu_exit_flag = E_PMU_KEY_EXIT;
		printk(KERN_EMERG "\n<Linux Kernel>Platform exit by panel key in the last test process!!!!\n");
	}
	else if(EXIT_STANDBY_TYPE_IR == pmu_exit_standby_sts)
	{
		pmu_exit_flag = E_PMU_IR_EXIT;
		printk(KERN_EMERG "\n<Linux Kernel>Platform exit by ir key in the last test process!!!!\n");
	}
	else if(EXIT_STANDBY_TYPE_RTC == pmu_exit_standby_sts)
	{
		pmu_exit_flag = E_PMU_RTC_EXIT;
		printk(KERN_EMERG "\n<Linux Kernel>Platform exit by RTC in the last test process!!!!\n");
	}

	return pmu_exit_flag;
}

int ali_pmu_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ali_pmu_close(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations ali_pmu_fops =
{
	.owner = THIS_MODULE,
	.open = ali_pmu_open,
	.release = ali_pmu_close,
	.unlocked_ioctl = ali_pmu_ioctl,
};

static int ali_pmu_probe(struct platform_device *pdev)
{
	/*Add for device tree.*/
	struct device *dev = &pdev->dev;
	struct resource *res;
	void __iomem *base = NULL;
	int ret = 0;
	struct ali_pmu_device *pmu_dev;

	printk(KERN_EMERG "\nEnter function:%s", __FUNCTION__);
	pmu_dev = devm_kzalloc(&pdev->dev, sizeof(struct ali_pmu_device), GFP_KERNEL);
	if(!pmu_dev)
	{
		return (-ENOMEM);
	}
	g_pmu_dev = pmu_dev;
	pmu_dev->dev_name = "ali_pmu";

	/*get pmu reg.*/
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res)
	{
		dev_err(&pdev->dev, "failed %s (%d)\n", __func__, __LINE__);
		ret = -ENXIO;
		goto out_free;
	}

	/*add for device tree.*/
	pmu_dev->ali_pmu_base_addr = devm_ioremap_resource(dev, res);
	/*set soc base address ioremap.*/
	pmu_dev->ali_soc_base_addr = ioremap(ALI_SOC_BASE_ADDR, ALI_SOC_BASE_ADDR_SIZE);
	printk(KERN_EMERG "\nali_soc_base_addr: 0x%08X, ali_pmu_base_addr: 0x%08X", (unsigned int)(pmu_dev->ali_soc_base_addr), \
		(unsigned int)(pmu_dev->ali_pmu_base_addr));

	ali_pmu_get_chip_info();

	if(IS_ERR(pmu_dev->ali_pmu_base_addr))
	{
		printk(KERN_EMERG "\nFunction:%s, Line:%d, error return!!!!", __FUNCTION__, __LINE__);
		return PTR_ERR(base);
	}

	/*get irq.*/
	pmu_dev->irq = platform_get_irq(pdev, 0);
	printk(KERN_EMERG "\npmu_dev->irq: %d\n", pmu_dev->irq);
	if(pmu_dev->irq < 0)
	{
		printk(KERN_EMERG "\nFunction:%s, Line:%d, error return!!!!", __FUNCTION__, __LINE__);
		goto out_free;
	}

	rtc_init();
	ret = request_irq(pmu_dev->irq, (irq_handler_t)ali_pmu_interrupt, 0, dev_name(&pdev->dev), NULL);
	if(ret != 0)
	{
		printk(KERN_EMERG "\nFunction:%s, Line:%d, error return!!!!", __FUNCTION__, __LINE__);
		goto out_free;
	}

	spin_lock_init(&pmu_dev->lock);
	INIT_LIST_HEAD(&pmu_dev->queue);
	pmu_dev->pdev = pdev;
	platform_set_drvdata(pdev, pmu_dev);

	if(ali_pmu_init(pdev) < 0)
	{
		return -EFAULT;
	}

out_free:
	return ret;
}

static int ali_pmu_init(struct platform_device *pdev)
{
	int ret = 0;
	struct ali_pmu_device *pmu_dev = platform_get_drvdata(pdev);
#ifdef PANEL_KEY_DETECT_TEST
	unsigned char temp_data1 = 0, temp_data2= 0, temp_data3 = 0, temp_data4 = 0;
#endif

	if(NULL == pmu_dev)
	{
		printk(KERN_EMERG "\n\n\nPlatform_get_drvdata ERROR %s\n", "ALi PMU Driver");
		return -1;
	}

	spin_lock_init(&rtc_lock);

#ifdef PANEL_KEY_DETECT_TEST
	temp_data1 = pmu_read8(g_pmu_dev, 0x3F00);
	temp_data2 = pmu_read8(g_pmu_dev, 0x3F01);
	temp_data3 = pmu_read8(g_pmu_dev, 0x3F02);
	temp_data4 = pmu_read8(g_pmu_dev, 0x3F03);

#ifdef CONFIG_ALI_PMU_DEBUG
	printk(KERN_EMERG "\ntemp_data1=0x%x, temp_data2=0x%x, temp_data3=0x%x, temp_data4=0x%x\n", temp_data1, temp_data2, temp_data3, temp_data4);
#endif
#endif
	ret = of_property_read_u32(pdev->dev.of_node, (const char *)"panel-power-key", &(pmu_dev->panel_power_key));
	if (ret) {
		dev_dbg(&pdev->dev, "get panel-power-key error\n");
		return -EINVAL;
	}
	else
	{
#ifdef CONFIG_ALI_PMU_DEBUG
		printk(KERN_EMERG "panel_power_key = %x\n", pmu_dev->panel_power_key);
#endif
	}

	pmu_hw_init();

	ret = of_get_major_minor(pdev->dev.of_node,&pmu_dev->dev_no, 0, 
			1, pmu_dev->dev_name);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}

	pmu_dev->pmu_class = class_create(THIS_MODULE, "ali_pmu_class");
	if(IS_ERR(pmu_dev->pmu_class))
	{
		ret = PTR_ERR(pmu_dev->pmu_class);
		goto err0;
	}

	cdev_init(&pmu_dev->pmu_cdev, &ali_pmu_fops);
	ret = cdev_add(&pmu_dev->pmu_cdev, pmu_dev->dev_no, 1);
	if(ret < 0)
	{
		goto err1;
	}

	pmu_dev->pmu_device_node = device_create(pmu_dev->pmu_class, NULL, pmu_dev->dev_no, pmu_dev, pmu_dev->dev_name);
	if(IS_ERR(pmu_dev->pmu_device_node))
	{
		ret = PTR_ERR(pmu_dev->pmu_device_node);
		goto err2;
	}

	return 0;

err2:
	cdev_del(&pmu_dev->pmu_cdev);

err1:
	class_destroy(pmu_dev->pmu_class);

err0:
	unregister_chrdev_region(pmu_dev->dev_no, 1);
	return 0;
}

static int ali_pmu_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ali_pmu_suspend(struct device *dev)
{
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "\nFunction:%s, Line:%d", __FUNCTION__, __LINE__);
#endif
    pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR, ALISL_PWR_STANDBY);
	return 0;
}

int ali_pmu_resume(struct device *dev)
{
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "\nFunction:%s, Line:%d", __FUNCTION__, __LINE__);
#endif
	g_year_h = pmu_read8(g_pmu_dev, MAILBOX_GET_YEAR_H);
	g_year_l = pmu_read8(g_pmu_dev, MAILBOX_GET_YEAR_L);
	g_month = pmu_read8(g_pmu_dev, MAILBOX_GET_MONTH);
	g_day = pmu_read8(g_pmu_dev, MAILBOX_GET_DAY);
	g_hour = pmu_read8(g_pmu_dev, MAILBOX_GET_HOUR);
	g_min = pmu_read8(g_pmu_dev, MAILBOX_GET_MIN);
	g_sec = pmu_read8(g_pmu_dev, MAILBOX_GET_SEC);
	g_year = (g_year_h*100) + g_year_l;

	rtc_sec = g_sec;
	rtc_min = g_min;
	rtc_hour = g_hour;
	rtc_date = g_day;
	rtc_month = g_month;
	rtc_year = g_year;

	rtc_upper = 0xFFFFFFFF;
	rtc_onesec = 1000000*SB_CLK_RTC/RTC_DIV[RTC_DIV_32];
	rtc_onesec = rtc_upper -rtc_onesec;
	soc_write32(g_pmu_dev, SB_TIMER3_CNT, (rtc_onesec & 0xFFFFFFFF));
	soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, 0x0);

	/*RTC starts to count with 32 divisor clock frequency, and enable interrupt.*/
	/*interrupt (enable) and start countings.*/
	soc_write8(g_pmu_dev, SB_TIMER3_CTL_REG, soc_read8(g_pmu_dev, SB_TIMER3_CTL_REG) | ALI_SB_TIMER_ENABLE);
    pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_READ_ADDR, \
                pmu_read32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR));
    pmu_write32(g_pmu_dev, WDT_REBOOT_FLAG_SAVE_ADDR, ALI_WDT_REBOOT);
    pmu_write8(g_pmu_dev, MAILBOX_WAKE_SECOND, 0xFF);
    pmu_write8(g_pmu_dev, MAILBOX_WAKE_MIN, 0xFF);
    pmu_write8(g_pmu_dev, MAILBOX_WAKE_HOUR, 0xFF);
    pmu_write8(g_pmu_dev, MAILBOX_WAKE_DAY, 0xFF);
    pmu_write8(g_pmu_dev, MAILBOX_SET_SEC, 0);
    pmu_write8(g_pmu_dev, MAILBOX_SET_MIN, 0);
    pmu_write8(g_pmu_dev, MAILBOX_SET_HOUR, 0);
    pmu_write8(g_pmu_dev, MAILBOX_SET_DAY, 0);
    pmu_write8(g_pmu_dev, MAILBOX_SET_MONTH, 0);
    pmu_write8(g_pmu_dev, MAILBOX_SET_YEAR_L, 0);
    pmu_write8(g_pmu_dev, MAILBOX_SET_YEAR_H, 0);
    pmu_write8(g_pmu_dev, STANDBY_SHOW_TIMR_SARM, 0);
	return 0;
}

static const struct dev_pm_ops ali_pmu_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ali_pmu_suspend, ali_pmu_resume)
};
#endif

/*Add for pmu device tree support.*/
static const struct of_device_id ali_pmu_of_match[] = {
	{ .compatible = "alitech,pmu", },
	{},
};
MODULE_DEVICE_TABLE(of, ali_pmu_of_match)

static struct platform_driver ali_pmu_driver = {
	.probe = ali_pmu_probe,
	.remove = ali_pmu_remove,
	.driver = {
		.name = ALI_PMU_DEVICE_NAME,
		.owner = THIS_MODULE,
	#ifdef CONFIG_ALI_STANDBY_TO_RAM
		.pm = &ali_pmu_pm_ops,
	#endif
		.of_match_table = ali_pmu_of_match,
	},
};

module_platform_driver(ali_pmu_driver);
MODULE_AUTHOR("Chuhua Tang");
MODULE_DESCRIPTION("ALI PMU Driver");
MODULE_LICENSE("GPL");
