/****************************************************************************(I)(S)
 *  (C)
 *  ALi (Shanghai) Corporation. 2010 Copyright (C)
 *  (C)
 *  File: m36_printf.c
 *  (I)
 *  Description:
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2010.06.03			Sam			Create

*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
*
 ****************************************************************************/
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/screen_info.h>
#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/mach-ali/prom.h>
#include <asm/dma.h>
#include <asm/time.h>
#include <asm/traps.h>
#ifdef CONFIG_VT
#include <linux/console.h>
#endif
#include <asm/mach-ali/typedef.h>
#include <asm/mach-ali/m36_gpio.h>
#include <asm/reboot.h>
#include <ali_soc.h>
#include <asm/mach-ali/chip.h>
/*****************************************************************************************************/

uint32_t strappin_0x70 = 0;
extern unsigned long __G_ALI_BOARD_TYPE;
extern void __init board_setup(void);
extern bool board_is_nand_boot(void);
/*****************************************************************************************************/

int __init m36_init_gpio(void);
extern void sys_ic_enter_standby(unsigned int , unsigned int);
extern void IRQ_DisableInterrupt(void );
/*****************************************************************************************************/

void boot_delay(u32 ms)
{
	u32 i = 0;

	for(i=0; i<ms; i++) 
	{
		ali_udelay(1000);
	} 
}

static void wdt_reboot_from_nand(void)
{
	uint32_t tmp = 0;

	if(ali_sys_ic_get_chip_id() == ALI_C3701)    
	{	    
		*(volatile uint32_t *)(0xB8000074) |= 0x40040000;/*NOTE: just for 3701 chip.*/      
	}
	else if(ali_sys_ic_get_chip_id() == ALI_S3503)
	{
		strappin_0x70 = (*(volatile uint32_t *)(0xB8000070));
		tmp = strappin_0x70;
		tmp |= ((1<<30) | (1<<29) | (1<<24) | (1<<18));
		tmp &= ~((1<<17) | (1<<15));
		*(volatile uint32_t *)(0xb8000074) = tmp;
	}
	else if(ali_sys_ic_get_chip_id() == ALI_C3505)   
	{    
		strappin_0x70 = (*(volatile uint32_t *)(0xB8000070));
		tmp = strappin_0x70;
		tmp |= ((1<<30) | (1<<18));
		*(volatile uint32_t *)(0xb8000074) = tmp;
	}   
	else if(ali_sys_ic_get_chip_id() == ALI_S3821)
	{
		strappin_0x70 = (*(volatile uint32_t *)(0xB8000070));
		tmp = strappin_0x70;
		tmp |= ((1<<30) | (1<<18));
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
}

static void wdt_reboot_from_nor(void)
{
	uint32_t tmp = 0;

	if(ali_sys_ic_get_chip_id() == ALI_C3701)    
	{	  
		*(volatile uint32_t *)(0xb8000074) &= ~(0x1<<18);  
		*(volatile uint32_t *)(0xb8000074) |= 0x40000000; //NOTE: just for 3701 chip            
	}
	else if((ali_sys_ic_get_chip_id() == ALI_S3503) || (ali_sys_ic_get_chip_id() == ALI_C3505))	
	{
		strappin_0x70 = (*(volatile uint32_t *)(0xB8000070));
		tmp = strappin_0x70;
		tmp &= ~(1<<18);
		tmp |= (1<<30);
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
	else if(ali_sys_ic_get_chip_id() == ALI_S3821)	
	{
		strappin_0x70 = (*(volatile uint32_t *)(0xB8000070));
		tmp = strappin_0x70;
		tmp &= ~(1<<18);
		tmp |= (1<<30);
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
}

static void wdt_reboot(void)
{
	uint16_t div;
	uint32_t a, duration_us; 
	uint32_t mem_clk;
	uint32_t timebase;
	spinlock_t wdt_lock;

	duration_us = 1;
	spin_lock(&wdt_lock);
	mem_clk = 27;

	a = 0xffffffff / mem_clk;
	if (duration_us < (a << 5))
	{
		div = 0;
	}
	else if (duration_us < (a << 6))
	{
		div = 1;
	}
	else if (duration_us < (a << 7))
	{
		div = 2;
	}
	else
	{
		div = 3;
	}
	timebase = 0xffffffff - (duration_us / (1 << (5 + div)) * mem_clk);
	*(volatile uint32_t *)(0xB8018500) = timebase; 
	*(volatile uint8_t *)(0xB8018500+4) = 0x64 | div;

	spin_unlock(&wdt_lock);
}

/*
*TODO - use definition For M3606 CHIP.
*For other CHIP or CPU may need to modify.
*/ 
static void prom_restart(char *command)
{
	printk(KERN_EMERG "\nFunction:%s, Line:%d, Watch Dog reset CHIP......\n",
		__FUNCTION__, __LINE__);

	/*implement.*/
	if(board_is_nand_boot())
	{
		printk(KERN_EMERG "\n========>Function:%s, Line:%d, Start to reboot from nand flash......\n",
			__FUNCTION__, __LINE__);
		wdt_reboot_from_nand();
	}
	else
	{
		printk(KERN_EMERG "\n========>Function:%s, Line:%d, Start to reboot from nor flash......\n",
			__FUNCTION__, __LINE__);
		wdt_reboot_from_nor();
	}

	wdt_reboot();
}

static void prom_halt(void)
{
	printk(KERN_NOTICE "\n** System halted.\n");
	IRQ_DisableInterrupt();
	*(volatile unsigned long *)(0xB8000038) =0;
	*(volatile unsigned long *)(0xB800003C) =0;

	if(board_is_nand_boot())
	{
		wdt_reboot_from_nand();
	}
	else
	{
		wdt_reboot_from_nor();
	}

	wdt_reboot();
}


static unsigned int m_early_mem_size;
unsigned int arm_get_early_mem_size(void)
{
	return m_early_mem_size;
}

struct ali_hwbuf_desc
{
    const char *name;
    unsigned long phy_start;
    unsigned long phy_end;
};

extern struct ali_hwbuf_desc* __init ali_get_privbuf_desc(int *hwbuf_desc_cnt);

#ifndef CONFIG_ALI_GET_DTB
static void __init ali_reserve_mem(void)
{
	int i = 0;
	int hwbuf_desc_cnt = 0;
	struct ali_hwbuf_desc *hwbuf_desc;
	unsigned long start = 0;
	unsigned long len = 0;

	/*Reserve memory for HW buffer.*/
	hwbuf_desc = ali_get_privbuf_desc(&hwbuf_desc_cnt);
	for(i=0; i<hwbuf_desc_cnt; i++)
	{
		start = hwbuf_desc[i].phy_start - 0xA0000000;
		len = hwbuf_desc[i].phy_end - hwbuf_desc[i].phy_start;
		add_memory_region(start, len, BOOT_MEM_RAM);
		printk("%s,%d,%d, start:%x,len:%x\n", __FUNCTION__, __LINE__, i,
			(unsigned int)start, (unsigned int)len);
	}
}
#endif

void __init plat_mem_setup(void)
{
	m36_init_gpio();
	
	/*board specific setup: PIN MUX.*/
	board_setup();

	/*
	**_machine_restart, _machine_halt, pm_power_off 
	**associated with  arch/mips/kernel/reset.c.
	*/
	_machine_restart = prom_restart;
	_machine_halt = prom_halt;
	pm_power_off = prom_halt;
	coherentio = 0;/*no DMA cache coherency (may be set by user).*/
	hw_coherentio = 0;/*init to 0 => no HW DMA cache coherency (reflects real HW).*/

#ifndef CONFIG_ALI_GET_DTB
	/*IO/MEM resources.*/
	ali_reserve_mem();
#endif
}

