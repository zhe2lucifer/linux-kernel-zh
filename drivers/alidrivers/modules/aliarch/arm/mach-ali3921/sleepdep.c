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
#include <linux/version.h> 
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/system_misc.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/smp_scu.h>
#include <asm/pgalloc.h>
#include <asm/suspend.h>
#include <asm/hardware/cache-l2x0.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#else
#include <asm/hardware/gic.h>
#endif
#include "sleepdep.h"
#include "m3921_sleep.h"
#include "ali_standby_bin.h"
#include "ali_pmu_bin_3921.h"
#include <ali_reg.h>
#include "sleepdep.h"
#include <asm/sections.h>
#include <linux/syscalls.h>
#include <asm/io.h>
#include "ca_dsc.h"
//=================================================================================//

unsigned int g_default_area_addr = 0;
unsigned int g_default_area_len = 0;
unsigned int g_mac_addr = 0;
static unsigned int wakeup_power_key[8] = {ALI_NEC_IR_POWER_KEY, 0, 0, 0, 0, 0, 0, 0};
unsigned int ali_3921_irq_flags_38 = 0;
unsigned int ali_3921_irq_flags_3c = 0;
unsigned int pinmux88 = 0;
unsigned int pinmux8c = 0;
unsigned int sys_timer4_cnt_reg = 0;
unsigned int sys_timer4_cmp_reg = 0;
unsigned int sys_timer4_ctrl_reg = 0;
unsigned int sys_timer4_restore_flag = 0;
//=================================================================================//

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
//=================================================================================//

extern void ali_3921_resume(void);
int ali_3921_enter_lowpower(unsigned int cpu, unsigned int power_state);
void standby_in_pmu_ram(void);
void ali_s3921_timer_enable(void);
void ali_s3921_l2_cache_pm_init(void);
void Backup_DDR_PHY1_Setting(void);
void Backup_DDR_PHY2_Setting(void);
extern int early_mcomm_resume(void);
extern unsigned int see_standby(unsigned int status);
//=================================================================================//

void str_delay_ms(unsigned int count)
{
	unsigned int i = 0, j = 0;
	for(i=0; i<count; i++)
	{
		for(j=0; j<0x1000; j++);
	}
}

#if 0/*Add for debug.*/
void Check_Retention_OTP_Bit(void)
{
	unsigned int otp_addr =0, OTP_Value = 0;

	otp_addr = 0xC;
	__REG32ALI(STR_OTP_BASE_ADDR+STR_OTP_ADDR_REG) = otp_addr;
	__REG32ALI(STR_OTP_BASE_ADDR+STR_OTP_OP_TRIG_REG) |= (1<<STR_READ_BIT_OFFSET);

	while(STR_OTP_READ_BUSY == (STR_OTP_READ_STATUS_REG & (1<<8)));
	OTP_Value = __REG32ALI(STR_OTP_BASE_ADDR+STR_OTP_VALUE_REG);

	if(0 == (OTP_Value & (1<<STR_RETENTION_BIT_OFFSET)))
	{
		printk("\n================>OTP03 value is 0x%x, DDR retention otp bit is not enabled!!!!\n", OTP_Value);
		printk("Start to burn OTP DDR retention bit....\n");
		__REG32ALI(STR_OTP_BASE_ADDR+STR_OTP_WRITE_VALUE_REG) = (1<<STR_RETENTION_BIT_OFFSET);
		__REG32ALI(STR_OTP_BASE_ADDR+STR_OTP_OP_TRIG_REG) |= (1<<STR_WRITE_BIT_OFFSET);
	}
	else
	{
		printk("\n================>OTP03 value is 0x%x, DDR retention otp bit is enabled!!!!\n", OTP_Value);
	}
}
EXPORT_SYMBOL(Check_Retention_OTP_Bit);
#endif

/*Function is used to output one char.*/
void suspend_output_char(unsigned char c)
{
#ifdef ALI_STR_PRINT_ENABEL
	while((__REG8ALI(0x18018305)&0x40) == 0x00);
	__REG8ALI(0x18018300) = c;
#endif
}

/*Function is used to output one string.*/
void ali_suspend_output_string(unsigned char *string)
{
#ifdef ALI_STR_PRINT_ENABEL
	suspend_output_char(0x0d);
	suspend_output_char(0x0a);
	while(*string)
	{
		suspend_output_char(*string++);
	}
	suspend_output_char(0x0d);
	suspend_output_char(0x0a);
#endif
}
EXPORT_SYMBOL(ali_suspend_output_string);

/*Timer4 is used as the system timer in linux kernel,
so it must be backup and restore in STR flow.
*/
void Backup_Sys_Timer(void)
{
	sys_timer4_cnt_reg = __REG32ALI(SB_TIMER4_CNT_REG);
	sys_timer4_cmp_reg = __REG32ALI(SB_TIMER4_CMP_REG);
	sys_timer4_ctrl_reg = __REG32ALI(SB_TIMER4_CTRL_REG);
	sys_timer4_restore_flag = 0x1;
}
EXPORT_SYMBOL(Backup_Sys_Timer);

void Restore_Sys_Timer(void)
{
	if(0 != sys_timer4_restore_flag)
	{
		__REG32ALI(ALI_SBTIMER_BASE+0x8) = 0x8;/*disable Time0 and Time0 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x18) = 0x8;/*disable Time1 and Time1 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x28) = 0x8;/*disable Time2 and Time2 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x38) = 0x8;/*disable Time3 and Time3 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x48) = 0x8;/*disable Time4 and Time4 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x58) = 0x8;/*disable Time5 and Time5 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x68) = 0x8;/*disable Time6 and Time6 interrupt.*/
		__REG32ALI(ALI_SBTIMER_BASE+0x78) = 0x8;/*disable Time7 and Time7 interrupt.*/

		__REG32ALI(SB_TIMER4_CNT_REG) = sys_timer4_cnt_reg;
		__REG32ALI(SB_TIMER4_CMP_REG) = sys_timer4_cmp_reg;
		__REG32ALI(SB_TIMER4_CTRL_REG) = sys_timer4_ctrl_reg;

		__REG32ALI(0x18000038) = ali_3921_irq_flags_38;/*close all interrupt.*/
		__REG32ALI(0x1800003c) = ali_3921_irq_flags_3c;/*close all interrupt.*/

		asm volatile ("mrs r0, cpsr");
		asm volatile ("bic r0,r0,#0xC0");/*enable interrupt, CPSR bit 5/6 interrupt mask.*/
		asm volatile ("msr cpsr_cxsf,r0");
		sys_timer4_restore_flag = 0x1;

		ali_suspend_output_string("============>Sys Timer4 is restored!!!!");
	}
}
EXPORT_SYMBOL(Restore_Sys_Timer);

void pm_standby_prepare(int enter)
{
	return;
}

void operate_device(int enable)
{
	return;
}
	
int ali_3921_pm_suspend(void)
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
	ali_3921_enter_lowpower(cpu_id, PWRDM_POWER_OFF);

	return 0;
}

/**
* omap4_enter_lowpower: OMAP4 MPUSS Low Power Entry Function
* The purpose of this function is to manage low power programming
* of OMAP4 MPUSS subsystem
* @cpu : CPU ID
* @power_state: Low power state.
*
* MPUSS states for the context save:
* save_state =
*	0 - Nothing lost and no need to save: MPUSS INACTIVE
*	1 - CPUx L1 and logic lost: MPUSS CSWR
*	2 - CPUx L1 and logic lost + GIC lost: MPUSS OSWR
*	3 - CPUx L1 and logic lost + GIC + L2 lost: DEVICE OFF
*/
int ali_3921_enter_lowpower(unsigned int cpu, unsigned int power_state)
{
	unsigned int save_state = 0;
	unsigned int reg38_save = 0, reg3c_save = 0;
	unsigned int  reg60_save = 0;
	unsigned int reg64_save = 0;	
	unsigned int reg88_save = 0;
	unsigned int reg90_save = 0;
	unsigned int reg94_save = 0;
	unsigned int reg68_save = 0;
	unsigned int reg6c_save = 0;
	unsigned int reg80_save = 0;
	unsigned int reg8c_save = 0;
	unsigned int regb0_save = 0;
	unsigned int reg20d4_save = 0;
	unsigned int reg20d8_save = 0;
	unsigned int reg28421_save = 0;
	//unsigned int reg8084_save = 0;
	unsigned int reg3d810_save = 0;
	unsigned int reg410_save = 0;
	unsigned int reg414_save = 0;
	unsigned int reg418_save = 0;
	unsigned int reg41c_save = 0;
	
	reg38_save =__REG32ALI(0x18000038);
	reg3c_save = __REG32ALI(0x1800003c);
	reg60_save = __REG32ALI(0x18000060);
	reg64_save = __REG32ALI(0x18000064);	
	reg90_save = __REG32ALI(0x18000090);
	reg88_save = __REG32ALI(0x18000088);
	reg94_save = __REG32ALI(0x18000094);
	reg68_save = __REG32ALI(0x18000068);
	reg6c_save = __REG32ALI(0x1800006c);
	reg80_save = __REG32ALI(0x18000080);
	reg8c_save = __REG32ALI(0x1800008c);
	regb0_save = __REG32ALI(0x180000b0);
	reg20d4_save = __REG32ALI(0x180020d4);
	reg20d8_save = __REG32ALI(0x180020d8);
	reg28421_save = __REG32ALI(0x18028421);
	reg28421_save = __REG32ALI(0x18008084);
	reg3d810_save = __REG32ALI(0x1803d810);
	reg410_save = __REG32ALI(0x18000410);
	reg414_save = __REG32ALI(0x18000414);
	reg418_save = __REG32ALI(0x18000418);
	reg41c_save = __REG32ALI(0x1800041c);

	__REG32ALI(0x180020d4) |= (7<<24);/*alg dac mute pattern.*/
	__REG32ALI(0x180020d4) |= (3<<8);/*disable l/r chl clock.*/
	__REG32ALI(0x180020d4) |= (3<<6);/*disable l/r chl data.*/
	__REG32ALI(0x180020d4) |= (0X1F<<10);/*power down dac.*/
	__REG32ALI(0x180020d4) &= ~(7<<24);
	__REG32ALI(0x180020d4) |= (8<<24);/*DAC_ALG_REG.*/
	__REG32ALI(0x180020d8) &= ~(1<<7);/*disable dig dac.*/
	__REG32ALI(0x180020d8) &= ~(1<<21);/*disable data input.*/

	/*Call low level function  with targeted low power state.*/
	cpu_suspend(save_state, standby_in_pmu_ram);

	__REG32ALI(0x18006010) |= (0x1<<0);/*fb register  VHD plane.*/
	__REG32ALI(0x18006014) |= (0x1<<0);/*fb register VSD.*/
	__REG32ALI(0x18000410) = reg410_save;
	__REG32ALI(0x18000414) = reg414_save;
	__REG32ALI(0x18000418) = reg418_save;
	__REG32ALI(0x1800041c) = reg41c_save;
	__REG32ALI(0x18000060) =  reg60_save;
	__REG32ALI(0x18000064) = reg64_save;	
	__REG32ALI(0x18000090) = reg90_save;
	__REG32ALI(0x18000088) = reg88_save;
	__REG32ALI(0x1800008c) = reg8c_save;
	__REG32ALI(0x18000094) = reg94_save;
	__REG32ALI(0x18000068) = reg68_save;
	__REG32ALI(0x1800006c) = reg6c_save;
	__REG32ALI(0x18000080) = reg80_save;
	__REG32ALI(0x180000b0) = regb0_save;
	__REG32ALI(0x180020d4) = reg20d4_save;
	__REG32ALI(0x180020d8) = reg20d8_save;
	__REG32ALI(0x18008084) = 0x14800000;//reg8084_save;	
	//__REG32ALI(0x18028421) = reg28421_save;//QAM?
	//__REG32ALI(0x1803d810) = reg3d810_save;//usb?
	__REG32ALI(0x18000038)  = reg38_save;/*close all interrupt.*/
	__REG32ALI(0x1800003c)  = reg3c_save;/*close all interrupt.*/

	early_mcomm_resume();
	ali_s3921_timer_enable();

	return 0;
}

static void operate_nonboot_cpu(int standby)
{
	see_standby(standby);
	if(standby)
	{
		str_delay_ms(400);
	}
}

int ali_see_enter_standby(void)
{
	operate_nonboot_cpu(SEE_ENTER_STANDBY_CMD);
	asm volatile ("mrs r0, cpsr");
	asm volatile ("orr r0, r0, #0xC0");/*disable interrupt, CPSR bit 5/6 interrupt mask.*/
	asm volatile ("msr cpsr_cxsf, r0");

	ali_3921_irq_flags_38 = __REG32ALI(0x18000038);
	ali_3921_irq_flags_3c = __REG32ALI(0x1800003c);
	__REG32ALI(0x18000038) = 0;/*close all interrupt.*/
	__REG32ALI(0x1800003c) = 0;/*close all interrupt.*/

	return 0;
}

int ali_see_exit_standby(void)
{
	return 0;
}

unsigned int mips_to_mcs51(unsigned int  tmp)
{
	return (((tmp&0xff)<<24) | (((tmp>>8)&0xff)<<16) | (((tmp>>16)&0xff)<<8) | (((tmp>>24)&0xff)));
}

void pmu_mcu_wakeup_ir_power_key(unsigned int *pmu_ir_key)
{
	unsigned int i = 0;
	unsigned int pmu_ir_key_51[8] = {0x0};

	for(i=0; i<8; i++)
	{
		if(pmu_ir_key[i] == 0xFFFFFFFF)
		{
			pmu_ir_key_51[i] = 0x5a5a5a5a;
		}
		else
		{
			pmu_ir_key_51[i] = mips_to_mcs51(pmu_ir_key[i]);
		}
	}

	for(i=0; i<8; i++)
	{
		__REG32ALI(0x18050000+0x3fc0+i*4) = pmu_ir_key_51[i];
	}
}

void pmu_mcu_panel_power_key_en(unsigned char key_en)
{
	__REG8ALI(MAILBOX_SET_PANEL_KEY_EN) = (unsigned char)(key_en & 0xff);
}

#if 0//Add for debug.
static void __naked get_arm_mode_regs(void)
{
	asm volatile (
	"mrs	r1, cpsr\n"
	"msr	cpsr_c, #0xd3 @(SVC_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"stmia	r0!, {r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xd7 @(ABT_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"stmia	r0!, {r2, r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xdb @(UND_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"stmia	r0!, {r2, r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xd2 @(IRQ_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"stmia	r0!, {r2, r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xd1 @(FIQ_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"stmia	r0!, {r2, r8 - r14}\n"
	"mrs	r2, spsr\n"
	"stmia	r0!, {r2}\n"
	"msr	cpsr_c, r1\n"
	"bx	lr\n");
}

static void __naked set_arm_mode_regs(void)
{
	asm volatile (		
	"mov	 r3, lr\n"
	"mrs	r1, cpsr\n"
	"msr	cpsr_c, #0xd3 @(SVC_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"ldmia	r0!, {r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xd7 @(ABT_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"ldmia	r0!, {r2, r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xdb @(UND_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"ldmia	r0!, {r2, r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xd2 @(IRQ_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"ldmia	r0!, {r2, r13 - r14}\n"
	"mrs	r2, spsr\n"
	"msr	cpsr_c, #0xd1 @(FIQ_MODE | PSR_I_BIT | PSR_F_BIT)\n"
	"ldmia	r0!, {r2, r8 - r14}\n"
	"mrs	r2, spsr\n"
	"ldmia	r0!, {r2}\n"
	"msr	cpsr_c, r1\n"	
	"mov	lr,r3 \n"
	"bx	lr\n");
}
#endif

void standby_in_pmu_ram(void)
{
	unsigned int i = 0, len = 0;
	unsigned int save_addr = 0, reg_addr = 0, alias_save_addr = 0;
	struct STR_CFG *pconfig = NULL;

	/*Close WDT.*/
	__REG32ALI(0x18018504) = 0;

	/*Reset MCU.*/
	__REG32ALI(0x18000320) &= ~(1 << 17);

	/*Copy standbybin code to pmu sram.*/
	len = sizeof(ali_standby_bin);
	for(i=0; i<len; i++)
	{
		__REG8ALI(PMU_RAM_ADDR+0x5000+i) = *(unsigned char *)(ali_standby_bin+i);
	}

  	/*Copy standby code to pmu sram.*/
	len = sizeof(ali_pmu_bin);
	for(i=0; i<len; i++)
	{
		__REG8ALI(PMU_RAM_ADDR+i) = *(unsigned char *)(ali_pmu_bin+i);
	}

	pmu_mcu_wakeup_ir_power_key(wakeup_power_key);

	/******************Backup DDR/cpu regs start******************/
	/*Save MEM_CLK (0xb8000070[7:5]) and ODT Value 0xb8001033[7].*/
	save_addr = PMU_RAM_ADDR + PMU_STANDBY_SAVE_OFFSET;
	reg_addr = DDR_DM_CTRL_BASE_ADDR;
	__REG32ALI(save_addr) = 0xDEADBEEF;
	save_addr +=4;

	printk(KERN_EMERG "return addr save at :0x%08x\n",save_addr);
	//__REG32ALI(save_addr) = virt_to_phys(&&wakeup_return);
	__REG32ALI(save_addr) = (unsigned int)(&&wakeup_return);
	save_addr +=4;

	printk(KERN_EMERG "mem clk save at :0x%08x\n",save_addr);
	__REG8ALI(save_addr) = __REG8ALI(0x18000070)&(0x07<<5);	
	save_addr ++;

	printk(KERN_EMERG "ODT save at :0x%08x\n",save_addr);
	__REG8ALI(save_addr) = __REG8ALI(0x18001033)&0x80;	
	save_addr +=3;	
	__REG32ALI(save_addr) = virt_to_phys(&ali_3921_resume);
	
	/*Save special registers.*/
	save_addr = PMU_RAM_ADDR + PMU_STANDBY_SAVE_OFFSET + 0x10;
	__REG8ALI(save_addr) = __REG32ALI(0x18000110);

	/*STR save DRAM parameters for AS Flow. The BootRom--Aux_Code will parse it.*/
	/*ALIAS save mem  parameters with its own format.*/
	printk(KERN_EMERG "\nSTR_CFG start:0x%08x\n", MAIN_CMAC_BACKUP_ADDR);
	pconfig = (struct STR_CFG *)(__REGALIRAW(MAIN_CMAC_BACKUP_ADDR));
	pconfig->Default_Area_Addr = g_default_area_addr;
	pconfig->Default_Area_Len = g_default_area_len;
	pconfig->cmac_start = MAIN_CMAC_SAVE_ADDR;//virt_to_phys((void *)cmac_buffer);
	pconfig->STREP = virt_to_phys(&ali_3921_resume);
	pconfig->Memcfg_ODT = __REG32ALI(0x18001030)&0xFF000000;
	pconfig->the_whole_area_sign = 0xc00f4200-0x20;
	printk(KERN_EMERG "Default_Area_Addr: 0x%x\nDefault_Area_Len: 0x%x\ncmac_start: 0x%x\n\n", \
		pconfig->Default_Area_Addr, pconfig->Default_Area_Len, pconfig->cmac_start);

	alias_save_addr = DDR_PARA_BACKUP_ADDR + sizeof(struct STR_CFG);
	/*Recover MEM_CLK 0xb800_0074[6:4] and 0xb8000074[21] (trigger bit).*/
	__REG32ALI(alias_save_addr) = (0x18000074&0xfffff) | (0x0f<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (__REG32ALI(0x18000070) & (0x07<<4)) | (0x1<<21); 
	alias_save_addr += 4;

	/*Save Dram Controller Register Value 0xb8001000~0xb8001114 if Chip will
	Power down or Reset. If Chip always Power On not reset, this step is not need.*/
	reg_addr = DDR_DM_CTRL_BASE_ADDR;
	printk(KERN_EMERG "DDR_DM_CTRL start: 0x%08x\n", alias_save_addr);
	for(i=0; i<0x114; i=i+4)
	{
		__REG32ALI(alias_save_addr) = (reg_addr&0xfffff) | (0xf<<24);
		alias_save_addr += 4;

		__REG32ALI(alias_save_addr) = __REG32ALI(reg_addr);
		alias_save_addr += 4;
		reg_addr += 4;
	}

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	printk(KERN_EMERG "DDR freq trigger flag saved at: 0x%08x \n", alias_save_addr);
	__REG32ALI(alias_save_addr) = (0x18001000&0xfffff) | (0xf<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x18001000) | (0x1<<28);
	alias_save_addr += 4;

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*Save DDR3PHY Register Value 0xb803e000~0xb803e07f ;  0xb803f000~0xb803f07f.
	if Chip will Power down or Reset. If Chip always Power On not reset, this step is not need.*/
	reg_addr = DDR_PHY1_BASE_ADDR;
	printk(KERN_EMERG "DDR_PHY1 start: 0x%08x \n", alias_save_addr);
	for(i=0; i<0x80; i=i+4)
	{
		__REG32ALI(alias_save_addr) = (reg_addr&0xfffff) | (0xf<<24);
		alias_save_addr += 4;

		__REG32ALI(alias_save_addr) = __REG32ALI(reg_addr);
		alias_save_addr += 4;
		reg_addr += 4;
	}
	printk(KERN_EMERG "DDR_PHY1 end: 0x%08x\n", alias_save_addr);

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*Trigger for update the PHY DDR clock QDL.*/
	//SET_WORD(0xb803E03C, ( READ_WORD(0xb803E03C) | (0x1<<30)));
	//nop_sleep(0xff);
	//SET_WORD(0xb803E03C, ( READ_WORD(0xb803E03C) & (~(0x1<<30))));
	__REG32ALI(alias_save_addr) = (0x1803E03C&0xfffff) | (0xf<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x1803E03C) | (0x1<<30);
	alias_save_addr += 4;

	__REG32ALI(alias_save_addr) = (0x1803E03C&0xfffff) | (0x0f<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x1803E03C) & (~(0x1<<30));
	alias_save_addr += 4;

	/*DDR PHY2 restore process is closed because aux code only support 4K para area.*/
	reg_addr = DDR_PHY2_BASE_ADDR;
	printk(KERN_EMERG "DDR_PHY2 start: 0x%08x\n", alias_save_addr);
	for(i=0; i<0x80; i=i+4)
	{
		__REG32ALI(alias_save_addr) = (reg_addr&0xfffff) | (0xf<<24);
		alias_save_addr += 4;

		__REG32ALI(alias_save_addr) = __REG32ALI(reg_addr);
		alias_save_addr += 4;
		reg_addr += 4;
	}
	printk(KERN_EMERG "DDR_PHY2 end: 0x%08x\n", alias_save_addr);

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*Trigger for update the PHY DDR clock QDL.*/
	//SET_WORD(0xb803F03C, ( READ_WORD(0xb803F03C) | (0x1<<30)));
	//nop_sleep(0xff);
	//SET_WORD(0xb803F03C, ( READ_WORD(0xb803F03C) & (~(0x1<<30))));
	__REG32ALI(alias_save_addr) = (0x1803F03C&0xfffff) | (0xf<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x1803F03C) | (0x1<<30);
	alias_save_addr += 4;

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	__REG32ALI(alias_save_addr) = (0x1803F03C&0xfffff) | (0x0f<<24); 
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = __REG32ALI(0x1803F03C) & (~(0x1<<30));
	alias_save_addr += 4;

	/*read register for delay.*/
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = (0x4f<<24) | (0x00);
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;
	__REG32ALI(alias_save_addr) = 0;
	alias_save_addr += 4;

	/*store parameters for exit from self-refresh.*/
	__REG32ALI(alias_save_addr) = EXIT_DDR_SELF_REFRESH_FLAG;
#ifdef ALI_STR_PRINT_ENABLE
	printk(KERN_EMERG "\nFunction:%s, Line:%d, alias_save end addr=0x%x\n__REG32ALI(0x%x)=0x%08X\n", \
		__FUNCTION__, __LINE__, alias_save_addr, alias_save_addr, __REG32ALI(alias_save_addr));
#endif
	/******************Backup DDR/cpu regs end******************/

	Backup_Sys_Timer();
	__REG32ALI(0x18000038) = 0;/*close all interrupt.*/
	__REG32ALI(0x1800003c) = 0;/*close all interrupt.*/
	__REG8ALI(0x1805D204) = 0xFF;
	__REG8ALI(0x1805D210) |= (1<<0);
	__REG8ALI(0x1805D211) |= (1<<0);

	/*Backup pinmux setting.*/
	pinmux88 = __REG32ALI(0x18000088);
	pinmux8c = __REG32ALI(0x1800008c);

	/*flush tlb.*/
	local_flush_tlb_all();

	ali_3921_finish_suspend(3);

	/*Restore pinmux setting.*/
	__REG32ALI(0x18000088) = pinmux88;
	__REG32ALI(0x1800008c) = pinmux8c;

wakeup_return:
	return;
}

noinline void arch_suspend_enable_irqs(void)
{
	local_irq_enable();
	return;
}

noinline void arch_suspend_disable_irqs(void)
{
	local_irq_disable();
	return;
}

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
	return;
}
EXPORT_SYMBOL(standby_dump_reg);

void Backup_DDR_PHY1_Setting(void)
{
	unsigned int index = 0;

	__REG32ALI(0x18000320) &= ~(1 << 17);/*reset pmu MCU.*/
	for(index=0; index<0x80; index++)
	{
		__REG8ALI(DDR_PHY1_BACKUP_ADDR+index) = __REG8ALI(DDR_PHY1_REG_ADDR+index);
	}

	ali_suspend_output_string("============>Function DDR PHY1 backup done!");
#ifdef ALI_STR_PRINT_ENABEL
	//standby_dump_reg(DDR_PHY1_REG_ADDR, 0x90);
#endif
}
EXPORT_SYMBOL(Backup_DDR_PHY1_Setting);

void Backup_DDR_PHY2_Setting(void)
{
	unsigned int index = 0;

	__REG32ALI(0x18000320) &= ~(1 << 17);/*reset pmu MCU.*/
	for(index=0; index<0x80; index++)
	{
		__REG8ALI(DDR_PHY2_BACKUP_ADDR+index) = __REG8ALI(DDR_PHY2_REG_ADDR+index);
	}

	ali_suspend_output_string("============>Function DDR PHY2 backup done!");
#ifdef ALI_STR_PRINT_ENABEL
	//standby_dump_reg(DDR_PHY2_REG_ADDR, 0x90);
#endif
}
EXPORT_SYMBOL(Backup_DDR_PHY2_Setting);

void Backup_DDR_Control_Setting(void)
{
	unsigned int index = 0;

	__REG32ALI(0x18000320) &= ~(1 << 17);/*reset pmu MCU.*/
	for(index=0; index<0x114; index++)
	{
		__REG8ALI(DDR_CTRL_BACKUP_ADDR+index) = __REG8ALI(DDR_CTRL_REG_ADDR+index);
	}

	ali_suspend_output_string("============>Function DDR CTRL backup done!");
#ifdef ALI_STR_PRINT_ENABEL
	//standby_dump_reg(DDR_CTRL_REG_ADDR, 0x114);
#endif
}
EXPORT_SYMBOL(Backup_DDR_Control_Setting);

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
	unsigned int default_area_start = 0;
	unsigned int default_area_end = 0;
	int ret = -1;
	struct file * fd_dsc = NULL;
	struct ca_calc_cmac cmac_info;
#ifdef ALI_STR_PRINT_ENABLE
	unsigned char *temp_buffer;
	unsigned char index = 0;
#endif

	g_mac_addr = (int)ioremap(MAIN_CMAC_SAVE_ADDR, 0x100);
	default_area_start = ((unsigned int)_stext);
	default_area_end = ((unsigned int)_etext);
	g_default_area_addr = virt_to_phys((void *)default_area_start);

	/*If user don't set the default area length, then the default area length will be equal to the whole kernel text stage.*/
	if(0 == g_default_area_len) 
	{
		g_default_area_len = default_area_end - default_area_start;
	}

	fd_dsc = filp_open("/dev/dsc0", O_RDWR, 0);
	if(NULL == fd_dsc)
	{
		printk("filp_open fd_dsc failed!\n");
		return 2;
	}

	/*calc main cmac.*/
	cmac_info.area_start = (char*)g_default_area_addr;
	cmac_info.size = (int)g_default_area_len;
	cmac_info.cmac_addr = (char*)(MAIN_CMAC_SAVE_ADDR);

	ret = fd_dsc->f_op->unlocked_ioctl(fd_dsc, CA_CALC_CMAC, (unsigned int)&cmac_info);
	if(ret)
	{
		printk(KERN_EMERG "error! MAIN ioctl CA_CALC_CMAC ret:%x\n", ret);
		return ret;
	}
	filp_close(fd_dsc, 0);

#ifdef ALI_STR_PRINT_ENABLE
	/*Dump MAIN CMAC info.*/
	temp_buffer = (unsigned char *)(g_mac_addr);
	printk(KERN_EMERG "\nMAIN CMAC:\n");
	for(index=0; index<16; index++)
	{
		printk(KERN_EMERG "0x%02x,", temp_buffer[index]);
	}
	
	printk(KERN_EMERG "\n\n");
#endif

	return 0;
}
EXPORT_SYMBOL(calc_aes_cmac);

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