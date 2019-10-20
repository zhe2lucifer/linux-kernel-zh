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


/*****************************************************************************
*	 Driver for ALi POK
*
*    File:
*
*    Description:
*         
*    History:
*           Date            Athor        Version           Reason
*	    ============	=============	=========	=======================
*	1.	Aug.21.2013       corey@SCT      Ver 0.1      Create file for 3921
*   2.  Dec.22.2016       Hank.Chou      Ver 0.2      Fix for 3821
*
*  	Copyright 2013 ALi Limited
*  	Copyright (C) 2013 ALi Corp.
*****************************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach-ali/m36_gpio.h>
#if defined(CONFIG_ALI_CHIP_M3921)
#include <ali_interrupt.h>
#else
#include <asm/mach-ali/m36_irq.h>
#endif
#include <ali_reg.h>

#define ALI_SOC_REG 0x18000000
#define C3921 0x3921	
#define C3821 0x3821

struct ali_pok_setting {
    unsigned int pgpio_ctl_reg;
    unsigned int pgpio_ier_reg;
    unsigned int pgpio_rec_reg;
    unsigned int pgpio_fec_reg;
    unsigned int pgpio_io_reg;
    unsigned int pgpio_do_reg;
    unsigned int pgpio_dir_reg;
    unsigned int pgpio_isr_reg;
    unsigned int irq_num;
    unsigned int pok_gpio;
    unsigned int palert_gpio;
    unsigned int gpio_offset;
};

static struct ali_pok_setting C3921_setting = 
{
    .pgpio_ctl_reg = 0x1800043C,
    .pgpio_ier_reg = 0x18000344,
    .pgpio_rec_reg = 0x18000348,
    .pgpio_fec_reg = 0x1800034c,
    .pgpio_io_reg = 0x18000350,
    .pgpio_do_reg = 0x18000354,
    .pgpio_dir_reg = 0x18000358,
    .pgpio_isr_reg = 0x1800035c,
#ifdef CONFIG_ALI_CHIP_M3921
    .irq_num = INT_ALI_GPIO,
#endif
    .pok_gpio = 29,
    .palert_gpio = 30,
    .gpio_offset = 96,
};

static struct ali_pok_setting C3821_setting = 
{
    .pgpio_ctl_reg = 0x1800043C,
    .pgpio_ier_reg = 0x18000344,
    .pgpio_rec_reg = 0x18000348,
    .pgpio_fec_reg = 0x1800034c,
    .pgpio_io_reg = 0x18000350,
    .pgpio_do_reg = 0x18000354,
    .pgpio_dir_reg = 0x18000358,
    .pgpio_isr_reg = 0x1800035c,
#ifndef CONFIG_ALI_CHIP_M3921
    .irq_num = M36_IRQ_SYSGPIO,
#endif
    .pok_gpio = 28,
    .palert_gpio = 29,
    .gpio_offset = 96,
};

struct ali_pok_setting *g_pok_setting = NULL;

#define ALI_POK_DRIVER_NAME   "ali_pok"

extern void _cpu_suspend(void);

static irqreturn_t ali_pok_alert_interrupt(int irq, void *dev_id)
{	
    u32 irq_status = __REG32ALI(g_pok_setting->pgpio_isr_reg);
    
    if(irq_status&(1<<g_pok_setting->pok_gpio) ||
       irq_status&(1<<g_pok_setting->palert_gpio)){

        __REG8ALI(0x18018300) = '$';
    	__REG32ALI(0x18000080) |= (1<<14);  //sflash reset assert
	    __REG32ALI(0x18000084) |= (1<<20);  //nand reset assert
	    __REG32ALI(0x1802A00F) &= 0xfffffffe;
	    printk("[%s] entry cpu_suspend\n", __func__);
	    _cpu_suspend();
    }
	
	return IRQ_HANDLED;
}

static void ali_pok_cleanup(void)
{
	if(NULL == g_pok_setting)
	    return;

	/* Free IRQ */
	free_irq(g_pok_setting->irq_num, NULL);
	
	__REG32ALI(g_pok_setting->pgpio_ctl_reg) &= ~(1<<g_pok_setting->pok_gpio);
	__REG32ALI(g_pok_setting->pgpio_ctl_reg) &= ~(1<<g_pok_setting->palert_gpio);
	__REG32ALI(0x18000038) &= ~(1<<0);
	
#if	defined(CONFIG_MIPS)
	write_c0_status(read_c0_status() & (~STATUSF_IP2));
#endif	
} 

static int ali_init_pok(void)
{
	int ret = 0;
	u32 chip_id = 0;
	
	chip_id = (__REG32ALI(ALI_SOC_REG) >> 16) & 0xFFFF;	
	switch (chip_id)
	{
		case C3921:
            g_pok_setting = &C3921_setting;
            break;
        case C3821:
            g_pok_setting = &C3821_setting;
            break;
        default:
            printk("[%s] can't find chip id: 0x%x\n", __func__, chip_id);
            return -1;
    }

#if	defined(CONFIG_MIPS)
	write_c0_status(read_c0_status() | STATUSF_IP2);
#endif	

	__REG32ALI(g_pok_setting->pgpio_ctl_reg) |= ((1<<(g_pok_setting->pok_gpio&0x1f))|(1<<(g_pok_setting->palert_gpio&0x1f))); //Enable gpio interrupt
	
	__REG32ALI(g_pok_setting->pgpio_dir_reg) &= ~(1 << g_pok_setting->palert_gpio);  //GPIO BIT DIR SET in
    __REG32ALI(g_pok_setting->pgpio_ier_reg) |= (1 << g_pok_setting->palert_gpio);   //GPIO INT SET enable
    __REG32ALI(g_pok_setting->pgpio_fec_reg) |= (1 << g_pok_setting->palert_gpio);   //GPIO INT FEDG SET fall
    __REG32ALI(g_pok_setting->pgpio_isr_reg) = (1 << g_pok_setting->palert_gpio);    //GPIO INT CLEAR
    __REG32ALI(g_pok_setting->pgpio_do_reg) |= (1 << g_pok_setting->palert_gpio);

    __REG32ALI(g_pok_setting->pgpio_dir_reg) &= ~(1 << g_pok_setting->pok_gpio);  //GPIO BIT DIR SET in
    __REG32ALI(g_pok_setting->pgpio_ier_reg) |= (1 << g_pok_setting->pok_gpio);   //GPIO INT SET enable
    __REG32ALI(g_pok_setting->pgpio_fec_reg) |= (1 << g_pok_setting->pok_gpio);   //GPIO INT FEDG SET fall
    __REG32ALI(g_pok_setting->pgpio_isr_reg) = (1 << g_pok_setting->pok_gpio);    //GPIO INT CLEAR
    __REG32ALI(g_pok_setting->pgpio_do_reg) |= (1 << g_pok_setting->pok_gpio);

	ret = request_irq(g_pok_setting->irq_num, ali_pok_alert_interrupt, IRQF_SHARED, ALI_POK_DRIVER_NAME, g_pok_setting);
	if (ret) {
		printk("[ERR] ali_pok: pok_startup - Can't get irq\n");
		return ret;
	}

    //enable GPIO irq
	__REG32ALI(0x18000038) |= (1<<0);
	
	return ret;
}

int ali_pok_open(void)
{
	return ali_init_pok();
}
EXPORT_SYMBOL(ali_pok_open);

void ali_pok_close(void)
{
	ali_pok_cleanup();
}
EXPORT_SYMBOL(ali_pok_close);


static int __init ali_pok_init(void)
{
	return ali_init_pok();
}

static void __exit ali_pok_exit(void)
{
	ali_pok_cleanup();
}

module_init(ali_pok_init);
module_exit(ali_pok_exit);

MODULE_AUTHOR("ALi Corp SCT: corey");
MODULE_DESCRIPTION("ALi STB POK driver");
MODULE_LICENSE("GPL");

