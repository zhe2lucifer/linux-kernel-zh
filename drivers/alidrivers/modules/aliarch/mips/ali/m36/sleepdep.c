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
#include <linux/version.h> 
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <ali_reg.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <linux/syscalls.h>
#include "ca_dsc.h"
#include "sleepdep.h"
#include "ali_standby_bin.h"
#if defined(CONFIG_ALI_CHIP_M3627)
#include "ali_pmu_bin_3505.h"
#else
#include "ali_pmu_bin_3505.h"
#endif
//====================================================================================//

unsigned int pinmux_488 = 0;
unsigned int pinmux_48c = 0;
unsigned int pinmux_490 = 0;
unsigned int pinmux_494 = 0;
unsigned int reg38_save = 0;
unsigned int reg3c_save = 0;
unsigned int g_default_area_addr = 0;
unsigned int g_default_area_len = 0;
unsigned int g_mac_addr = 0;
unsigned char ali_uart_setting[12] = {0};
//====================================================================================//

extern void ali_str_resume(void);
extern void ali_str_finish_suspend(void);
int ali_str_enter_lowpower(unsigned int cpu, unsigned int power_state);
extern void clear_C0_EBase(void);
void standby_in_pmu_ram(void);
extern int early_mcomm_resume(void);
extern unsigned int see_standby(unsigned int status);
//====================================================================================//

struct STR_CFG
{
	unsigned int Default_Area_Addr;
	unsigned int Default_Area_Len;
	unsigned int STREP;
	unsigned int Default_Area_Sign;
	unsigned int Memcfg_ODT;
	unsigned int cmac_start;
	unsigned int the_whole_area_sign;
	unsigned int Reserved2;
};

/*Add for backup and restore some paras for seebl.*/
struct SEE_BL_PARAM
{
	unsigned int see_bl_location_address;
	unsigned int see_bl_len_address;
	unsigned int see_bl_real_run_address;
	unsigned int see_signatrue_location_address;
	unsigned int see_software_enter_address;
	unsigned int see_enter_address;
}see_bl_param;
//====================================================================================//

/*Function is used to output one char.*/
void suspend_output_char(unsigned char c)
{
#ifdef ALI_STR_DDR_CHECK_ENABLE
	while((__REG8ALI(0x18018305)&0x40) == 0x00);
	__REG8ALI(0x18018300) = c;
#endif
}

unsigned int ali_get_otp_value(unsigned int otp_addr)
{
	unsigned int otp_value = 0;

	__REG32ALI(ALI_OTP_BASE+OTP_ADDR_REG) = (otp_addr*4);
	__REG32ALI(ALI_OTP_BASE+OTP_READ_TRIG_REG) |= (1<<8);
	while(OTP_READ_BUSY == (GET_OTP_READ_STATUS & (1<<8)));
	otp_value = __REG32ALI(ALI_OTP_BASE+OTP_VALUE_REG);

	return otp_value;
}
EXPORT_SYMBOL(ali_get_otp_value);

void operate_device(int enable)
{
	return;
}

int ali_str_pm_suspend(void)
{
	u32 cpu_id = smp_processor_id();

	/*
	* For MPUSS to hit power domain retention(CSWR or OSWR),
	* CPU0 and CPU1 power domains need to be in OFF or DORMANT state,
	* since CPU power domain CSWR is not supported by hardware
	* Only master CPU follows suspend path. All other CPUs follow
	* CPU hotplug path in system wide suspend. On OMAP4, CPU power
	* domain CSWR is not supported by hardware.
	* More details can be found in OMAP4430 TRM section 4.3.4.2.
	*/
	ali_str_enter_lowpower(cpu_id, PWRDM_POWER_OFF);

	return 0;
}
extern void set_cpu_clk_by_product_id(void);
int ali_str_enter_lowpower(unsigned int cpu, unsigned int power_state)
{
	reg38_save = __REG32ALI(STR_INTERRUPT_ENABLE_REG1);
	reg3c_save = __REG32ALI(STR_INTERRUPT_ENABLE_REG2);
	__REG32ALI(STR_INTERRUPT_ENABLE_REG1) = 0x0;
	__REG32ALI(STR_INTERRUPT_ENABLE_REG2) = 0x0;

	/*Call low level function with targeted low power state.*/
	standby_in_pmu_ram();
	set_cpu_clk_by_product_id();
	early_mcomm_resume();

	return 0;
}

static inline void operate_nonboot_cpu(int standby)
{
	see_standby(standby);

	if(standby)
	{
		mdelay(400);
	}
}

int ali_see_enter_standby(void)
{
	operate_nonboot_cpu(0x10);

	return 0;
}

int ali_see_exit_standby(void)
{
	__REG32ALI(STR_INTERRUPT_ENABLE_REG1) = 0;/*close all interrupt.*/
	__REG32ALI(STR_INTERRUPT_ENABLE_REG2) = 0;/*close all interrupt.*/
	return 0;
}

void standby_in_pmu_ram(void)
{
	unsigned int i = 0, len = 0, alias_save_addr = 0, save_addr = 0, reg_addr = 0;
	struct STR_CFG *pconfig = NULL;

	__REG32ALI(STR_PMU_IP_RESET_REG) |= (1 << 25);/*reset MCU.*/
	DISABLE_ALI_WATCHDOG;

	/*Copy standbybin code to pmu sram.*/
	len = sizeof(ali_standby_bin);
	for(i=0; i<len; i++)
	{
		__REG8ALI(STR_STANDBYBIN_BASE_ADDR+i) = *(unsigned char *)(ali_standby_bin+i);
	}

	/*Copy standby code to pmu sram.*/
	len = sizeof(str_ali_pmu_bin);
	for(i=0; i<len; i++)
	{
		__REG8ALI(PMU_SRAM_BASE_ADDR+i) = *(unsigned char *)(str_ali_pmu_bin+i);
	}

	/*Save MEM_CLK (0xb8000070[7:5]) and ODT Value 0xb8001033[7].*/
	save_addr = PMU_SRAM_BASE_ADDR + PMU_STANDBY_SAVE_OFFSET;
	reg_addr = DDR_DM_CTRL_BASE_ADDR;
	__REG32ALI(save_addr) = 0xDEADBEEF;
	save_addr += 4;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>return addr save at: 0x%08X\n",(unsigned int)save_addr);
#endif
	//__REG32ALI(save_addr) = virt_to_phys(&&wakeup_return);
	__REG32ALI(save_addr) = (unsigned int)(&&wakeup_return);
	save_addr += 4;

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>mem clk save at: 0x%08X\n",(unsigned int)save_addr);
#endif
	__REG8ALI(save_addr) = __REG8ALI(0x18000070)&(0xF<<4);
	save_addr++;

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>ODT save at: 0x%08X\n", (unsigned int)save_addr);
#endif
	__REG8ALI(save_addr) = __REG8ALI(DDR_ODT_1033)&(0x80);
	save_addr += 3;
	__REG32ALI(save_addr) = (unsigned int)&ali_str_resume;//virt_to_phys(&ali_str_resume);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>linux kernel return addr is saved at: 0x%08X\n", save_addr);
#endif

	/*Save special registers.*/
	save_addr = PMU_SRAM_BASE_ADDR + PMU_STANDBY_SAVE_OFFSET + 0x10;
	__REG8ALI(save_addr) = __REG32ALI(0x18000110);

	/*save see bl launch paramters.*/
	see_bl_param.see_bl_location_address = __REG32ALI(SEEBL_LOCATION_ADDR_REG);
	see_bl_param.see_bl_len_address = __REG32ALI(SEEBL_LEN_REG);
	see_bl_param.see_bl_real_run_address = __REG32ALI(SEEBL_RUNNING_ADDR_REG);
	see_bl_param.see_signatrue_location_address = __REG32ALI(SEEBL_SIGN_ADDR_REG);
	see_bl_param.see_software_enter_address = __REG32ALI(SEE_SW_ENETR_ADDR_REG);
	see_bl_param.see_enter_address = __REG32ALI(SEE_ENTER_ADDR_REG);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>see_bl_location_address: 0x%08X\n", see_bl_param.see_bl_location_address);
	printk(KERN_EMERG "<Linux Kernel>see_bl_len_address: 0x%08X\n", see_bl_param.see_bl_len_address);
	printk(KERN_EMERG "<Linux Kernel>see_bl_real_run_address: 0x%08X\n", see_bl_param.see_bl_real_run_address);
	printk(KERN_EMERG "<Linux Kernel>see_signatrue_location_address: 0x%08X\n", see_bl_param.see_signatrue_location_address);
	printk(KERN_EMERG "<Linux Kernel>see_software_enter_address: 0x%08X\n", see_bl_param.see_software_enter_address);
	printk(KERN_EMERG "<Linux Kernel>see_enter_address: 0x%08X\n", see_bl_param.see_enter_address);
#endif

	/*STR save STR structure, and Aux_Code will parse it.*/
	pconfig = (struct STR_CFG *)(__REGALIRAW(MAIN_CMAC_BACKUP_ADDR));
	pconfig->Default_Area_Addr = g_default_area_addr;
	pconfig->Default_Area_Len = g_default_area_len;
	pconfig->cmac_start = g_mac_addr;
	pconfig->STREP = (unsigned int)&ali_str_resume;
	pconfig->Memcfg_ODT = __REG32ALI(DDR_ODT_REG)&0xFF000000;
	pconfig->the_whole_area_sign = WHOLE_AREA_SIGN_ADDR;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>STR_CFG parameters is saved at: 0x%08X\n", (unsigned int)(pconfig));
	printk(KERN_EMERG "<Linux Kernel>Default_Area_Add: 0x%08X\n", g_default_area_addr);
	printk(KERN_EMERG "<Linux Kernel>Default_Area_Len: 0x%08X\n", g_default_area_len);
	printk(KERN_EMERG "<Linux Kernel>cmac_start: 0x%08X\n", (unsigned int)g_mac_addr);
	printk(KERN_EMERG "<Linux Kernel>pconfig->STREP: 0x%08X\n", pconfig->STREP);
#endif

	/*STR save DDR init parameters, and Aux_Code will parse it.*/
	alias_save_addr = DDR_PARA_BACKUP_ADDR + sizeof(struct STR_CFG);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>Function:%s, Line:%d, DDR init parameters is saved at: 0x%08X\n",
		__FUNCTION__, __LINE__, alias_save_addr);
#endif

	/*Recover MEM_CLK 0xb800_0074[6:4] and 0xb8000074[21](trigger bit).*/
	__REG32ALI(alias_save_addr) = (0x18000074&0xFFFFF) | (0x0F<<24);
	alias_save_addr += 4;
	/*Bit4~7:DRAM clock selection; Bit8~10:CPU clock selection; Bit21:MEM_CLK_SEL TRIG; BIT22:CPU_CLK PLL M TRIG.*/
	__REG32ALI(alias_save_addr) = (__REG32ALI(STRAP_INFO_REG) & (MEM_CLK_SETTING_BIT)) | MEM_CLK_TRRIGER;
	alias_save_addr += 4;

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*Re-configure Dram Controller Register Value 0xb8001000~0xB8001140.*/
	reg_addr = DDR_DM_CTRL_BASE_ADDR;
	for(i=0; i<0x140; i+=4)
	{
		__REG32ALI(alias_save_addr) = (reg_addr&0xFFFFF) | (0xF<<24);
		alias_save_addr += 4;

		__REG32ALI(alias_save_addr) = __REG32ALI(reg_addr);
		alias_save_addr += 4;
		reg_addr += 4;
	}

	/*DLL Update Command issue to MEMBUS(Add for DDR pic weason's suggestion).*/
	__REG32ALI(alias_save_addr) = (0x18001004&0xFFFFF) | (0xF<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x18001004) & (~(1<<28));
	alias_save_addr += 4;

	/*Re-configure DDR3PHY Register Value 0xb803E000~0xb803E07F.*/
	reg_addr = DDR_PHY1_BASE_ADDR;
	for(i=0; i<0x80; i+=4)
	{
		__REG32ALI(alias_save_addr) = (reg_addr&0xFFFFF) | (0xF<<24);
		alias_save_addr += 4;

		__REG32ALI(alias_save_addr) = __REG32ALI(reg_addr);
		alias_save_addr += 4;
		reg_addr += 4;
	}

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*Trigger for update the PHY DDR clock QDL.*/
	//SET_WORD(0xB803E03C, (READ_WORD(0xB803E03C) | (1<<30)));
	//nop_sleep(0xff);
	//SET_WORD(0xB803E03C, (READ_WORD(0xB803E03C) & (~(1<<30))));
	__REG32ALI(alias_save_addr) = (0x1803E03C&0xFFFFF) | (0xF<<24);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x1803E03C) | (1<<30);
	alias_save_addr += 4;

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	__REG32ALI(alias_save_addr) = (0x1803E03C&0xFFFFF) | (0x0F<<24);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x1803E03C) & (~(0x1<<30));
	alias_save_addr += 4;

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4F<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*store parameters for exit from self-refresh.*/
	__REG32ALI(alias_save_addr) = EXIT_DDR_SELF_REFRESH_FLAG;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>Function:%s, Line:%d, alias_save end addr=0x%08X\n", __FUNCTION__, __LINE__, alias_save_addr);
#endif

	__REG32ALI(STR_INTERRUPT_ENABLE_REG1) = 0;/*close all interrupt.*/
	__REG32ALI(STR_INTERRUPT_ENABLE_REG2) = 0;/*close all interrupt.*/

	/*flush tlb.*/
	local_flush_tlb_all();

	/*Backup pinmux setting.*/
	if(M3505_CHIP_ID == (__REG32ALI(IO_BASE_ADDR) & 0xFFFF0000))
	{
		pinmux_488 = __REG32ALI(0x18000488);
		pinmux_48c = __REG32ALI(0x1800048C);
		pinmux_490 = __REG32ALI(0x18000490);
		pinmux_494 = __REG32ALI(0x18000494);
		__REG32ALI(ALI_SB_TIMER_BASE+0x8) = 0x8;/*disable Time0 and Time0 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x18) = 0x8;/*disable Time1 and Time1 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x28) = 0x8;/*disable Time2 and Time2 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x38) = 0x8;/*disable Time3 and Time3 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x48) = 0x8;/*disable Time4 and Time4 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x58) = 0x8;/*disable Time5 and Time5 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x68) = 0x8;/*disable Time6 and Time6 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x78) = 0x8;/*disable Time7 and Time7 interrupt.*/
	}

	clear_C0_EBase();

	/*Backup ali uart setting.*/
#if 0/*Reseved for debug, and don't delete it.*/
	for(i=0; i<12; i++)
	{
		ali_uart_setting[i] = __REG32ALI(ALI_UART_BASE_ADDR+i);
	}
#endif
	printk(KERN_EMERG "<Linux Kernel>Function:%s, Line:%d, STB enter standby mode!\n", __FUNCTION__, __LINE__);

	/*Enter standby.*/
	ali_str_finish_suspend();

	DISABLE_ALI_WATCHDOG;
	__REG32ALI(STR_INTERRUPT_ENABLE_REG1) = 0;/*close all interrupt.*/
	__REG32ALI(STR_INTERRUPT_ENABLE_REG2) = 0;/*close all interrupt.*/

	/*Restore ali uart setting.*/
#if 0/*Reseved for debug, and don't delete it.*/
	for(i=0; i<12; i++)
	{
		__REG32ALI(ALI_UART_BASE_ADDR+i) = ali_uart_setting[i];
	}
#endif

	/*Restore pinmux setting.*/
	if(M3505_CHIP_ID == (__REG32ALI(IO_BASE_ADDR) & 0xFFFF0000))
	{
		__REG32ALI(0x18000488) = pinmux_488;
		__REG32ALI(0x1800048c) = pinmux_48c;
		/*Bit10:Sflash and NandFlash PINMUX select enable at BGA Package.*/
		__REG32ALI(0x18000490) = pinmux_490 | (1<<10);
		__REG32ALI(0x18000494) = pinmux_494;
		__REG32ALI(ALI_SB_TIMER_BASE+0x8) = 0x8;/*disable Time0 and Time0 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x18) = 0x8;/*disable Time1 and Time1 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x28) = 0x8;/*disable Time2 and Time2 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x38) = 0x8;/*disable Time3 and Time3 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x48) = 0x8;/*disable Time4 and Time4 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x58) = 0x8;/*disable Time5 and Time5 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x68) = 0x8;/*disable Time6 and Time6 interrupt.*/
		__REG32ALI(ALI_SB_TIMER_BASE+0x78) = 0x8;/*disable Time7 and Time7 interrupt.*/
	}

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>Function:%s, Line:%d, STB enter standby mode!\n", __FUNCTION__, __LINE__);
#endif
	/*Reset PMU and MCU.*/
	__REG32ALI(STR_PMU_IP_RESET_REG) |= PMU_SW_RESET_ENABLE;/*reset pmu.*/
	for(i=0; i<0x100; i++);
	__REG32ALI(STR_PMU_IP_RESET_REG) &= PMU_SW_RESET_DISABLE;
	__REG32ALI(STR_PMU_IP_RESET_REG) |= PMU_MCU_RESET_ENABLE;/*Suspend MCU.*/

wakeup_return:
	return;
}

noinline void arch_suspend_disable_irqs(void)
{
	local_irq_disable();
}

noinline void arch_suspend_enable_irqs(void)
{
	DISABLE_ALI_WATCHDOG;
	__REG32ALI(STR_INTERRUPT_ENABLE_REG1) = 0;/*close all interrupt.*/
	__REG32ALI(STR_INTERRUPT_ENABLE_REG2) = 0;/*close all interrupt.*/
	local_irq_enable();
	ali_ip_interrupt_restore();
}

#if 0/*Reserved for debug, do not delete it.*/
void standby_dump_reg(unsigned long addr, unsigned long len)
{
	unsigned long i = 0, j = 0;
	char *ascii = "0123456789ABCDEF";
	unsigned char index = 0;

	for(i=0; i<len; i++)
	{
		if(i%16 == 0)
		{
			suspend_output_char(0x0d);
			suspend_output_char(0x0a);
			for(j=0; j<8; j++)
			{
				suspend_output_char(ascii[((addr+i)>>(4*(7-j)))&0xF]);
			}
			suspend_output_char(':');
		}
		index = __REG8ALI(addr+i);
		suspend_output_char(ascii[(index>>4)&0xF]);
		suspend_output_char(ascii[index&0xF]);
	}
	suspend_output_char(0x0d);
	suspend_output_char(0x0a);
}
#endif

#if 0/*Add only for debug.*/
void Backup_DDR_PHY1_Setting(void)
{
	unsigned int index = 0;

	__REG32ALI(0x18000084) |= (1 << 25);//reset pmu MCU.
	for(index=0; index<0x80; index++)
	{
		__REG8ALI(DDR_PHY1_BACKUP_ADDR+index) = __REG8ALI(DDR_PHY1_REG_ADDR+index);
	}

#ifdef ALI_STR_DDR_CHECK_ENABLE
	standby_dump_reg(DDR_PHY1_REG_ADDR, 0x90);
#endif
}
EXPORT_SYMBOL(Backup_DDR_PHY1_Setting);

void Backup_DDR_Ctrl_Setting(void)
{
	unsigned int index = 0;

	__REG32ALI(0x18000084) |= (1 << 25);//reset pmu MCU.
	for(index=0; index<0x114; index++)
	{
		__REG8ALI(DDR_CTRL_BACKUP_ADDR+index) = __REG8ALI(DDR_CTRL_REG_ADDR+index);
	}

#ifdef ALI_STR_DDR_CHECK_ENABLE
	standby_dump_reg(DDR_CTRL_REG_ADDR, 0x114);
#endif
}
EXPORT_SYMBOL(Backup_DDR_Ctrl_Setting);
#endif

/*Add for CMAC verification.*/
void set_default_area_size(unsigned long size)
{
	g_default_area_len = size;
}
EXPORT_SYMBOL(set_default_area_size);

unsigned int get_default_area_size(void)
{
	return g_default_area_len;
}
EXPORT_SYMBOL(get_default_area_size);

int calc_aes_cmac(void)
{
	unsigned long default_area_start = 0;
	unsigned long default_area_end = 0;
	int ret = (-1);
	struct file * fd_dsc = NULL;
	struct ca_calc_cmac cmac_info;

	g_mac_addr = MAIN_CMAC_SAVE_ADDR;
	default_area_start = ((unsigned int)_stext);
	default_area_end = ((unsigned int)_etext);
	g_default_area_addr = default_area_start;

	/*If user don't set the default area length, then the default area length will be equal to the whole kernel text stage.*/
	if(0 == g_default_area_len)
	{
		g_default_area_len = default_area_end - default_area_start;
	}

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "<Linux Kernel>[g_default_area_addr: 0x%08X\n", (unsigned int)g_default_area_addr);
	printk(KERN_EMERG "<Linux Kernel>see_default_area_end: 0x%08X\n", (unsigned int)default_area_end);
	printk(KERN_EMERG "<Linux Kernel>g_default_area_len: 0x%08X\n", (unsigned int)g_default_area_len);
	printk(KERN_EMERG "<Linux Kernel>g_mac_addr: 0x%08X\n", (unsigned int)g_mac_addr);
#endif

	fd_dsc = filp_open("/dev/dsc0", O_RDWR, 0);
	if(NULL == fd_dsc)
	{
		printk(KERN_EMERG "<Linux Kernel>filp_open fd_dsc failed!\n");
		return 2;
	}

	/*calc main cmac.*/
	cmac_info.area_start = (char*)g_default_area_addr;
	cmac_info.size = (int)g_default_area_len;
	cmac_info.cmac_addr = (char*)g_mac_addr;
	ret = fd_dsc->f_op->unlocked_ioctl(fd_dsc, CA_CALC_CMAC, (unsigned int)&cmac_info);
	if(ret)
	{
		printk(KERN_EMERG "<Linux Kernel>error! main ioctl CA_CALC_CMAC ret:%x\n", ret);
		return ret;
	}
	filp_close(fd_dsc, 0);

	return 0;
}
EXPORT_SYMBOL(calc_aes_cmac);

void ali_ip_interrupt_restore(void)
{
	if(M3505_CHIP_ID == (__REG32ALI(IO_BASE_ADDR) & 0xFFFF0000))
	{
		__REG32ALI(STR_INTERRUPT_ENABLE_REG1) = reg38_save;
		__REG32ALI(STR_INTERRUPT_ENABLE_REG2) = reg3c_save & (0x0FFFFFFF);
	}
}
EXPORT_SYMBOL(ali_ip_interrupt_restore);