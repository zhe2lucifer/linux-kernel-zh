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
*
*    File:    dog_m3327e.c
*
*    Description:    This file contains all globe micros and functions declare
*		             of watchdog timer.
*    History:
*           Date            Athor        Version          Reason
*	    ============	=============	=========	=================
*	1.	May.2.2006       Justin Wu      Ver 0.1    Create file.
*	2.
*****************************************************************************/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <ali_soc.h>
#include <linux/delay.h>
#include <ali_reg.h>
#include <linux/platform_device.h>
/*****************************************************************************************************/

#define PFX "alim36dog: "
#define DOG_M3327E_WDTCNT (0)
#define DOG_M3327E_WDTCTRL (4)
#define DOG_M3327E_NUM (1)
#define WDT_DEBUG (printk) 
//#define WDT_DEBUG do{}while(0)
#define WATCHDOG_TIMEOUT (60)/* 60 sec default timeout */

#if defined(CONFIG_ALI_CHIP_M3921)
#define DOG_WRITE32(id, reg, data) ((__REG32ALI(dog_m3327e_reg[id].reg_base + reg)) = (data))
#define DOG_READ32(id, reg) (__REG32ALI(dog_m3327e_reg[id].reg_base + reg))
#define DOG_WRITE8(id, reg, data) ((__REG8ALI(dog_m3327e_reg[id].reg_base + reg)) = (data))
#define DOG_READ8(id, reg) (__REG8ALI(dog_m3327e_reg[id].reg_base + reg))
#else
#define DOG_WRITE32(id, reg, data) (*((volatile UINT32 *)(dog_m3327e_reg[id].reg_base + reg)) = (data))
#define DOG_READ32(id, reg) (*((volatile UINT32 *)(dog_m3327e_reg[id].reg_base + reg)))
#define DOG_WRITE8(id, reg, data) (*((volatile UINT8 *)(dog_m3327e_reg[id].reg_base + reg)) = (data))
#define DOG_READ8(id, reg) (*((volatile UINT8 *)(dog_m3327e_reg[id].reg_base + reg)))
#endif
/*****************************************************************************************************/

extern bool is_nand_boot;
static spinlock_t wdt_lock;
static unsigned int wdt_count;
static unsigned int wdt_timeleft;
static int nowayout = WATCHDOG_NOWAYOUT;

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static struct
{
	UINT32 dog_cnt;
	unsigned char dog_ctrl;
} dog_pm_cfg;
#endif

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout,
		"Watchdog cannot be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

typedef unsigned long UINT32;
typedef unsigned char UINT8;
typedef unsigned short UINT16; 

#if defined(CONFIG_ALI_CHIP_M3921)
static struct
{
	UINT32 reg_base;
	int irq;
	UINT32 timebase;/*The init value of cnt (we need config dog in us).*/
} dog_m3327e_reg[DOG_M3327E_NUM] = {{0x18018500, 31, 0}};
#else
static struct
{
	UINT32 reg_base;
	int irq;
	UINT32 timebase;/*The init value of cnt (we need config dog in us).*/
} dog_m3327e_reg[DOG_M3327E_NUM] = {{0xb8018500, 31, 0}};
#endif
static UINT32 sys_mem_clk = 27;

uint32_t strappin70;
extern bool board_is_nand_boot(void);
//==========================================================================//

static void wdt_reboot_from_nand(void)
{
	uint32_t tmp = 0;

	if(ali_sys_ic_get_chip_id() == ALI_C3701) 
	{ 
		*(volatile uint32_t *)(0xB8000074) |= 0x40040000;
	}
	else if(ali_sys_ic_get_chip_id() == ALI_S3503)
	{ 
		tmp = strappin70;
		tmp |= ((1<<30) | (1<<29) | (1<<24) | (1<<18));
		tmp &= ~((1<<17) | (1<<15));
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
	else if(ali_sys_ic_get_chip_id() == ALI_C3505)	
	{ 
		tmp = strappin70;
		tmp |= ((1<<30) | (1<<18));
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
	else if(ali_sys_ic_get_chip_id() == ALI_C3921)
	{
		__REG32ALI(0x18000074) = (1<<23) | (1<<18);/*strap pin to nand.*/
	}
	else if(ali_sys_ic_get_chip_id() == ALI_S3821) 
	{ 
		int strappin_0x70 = (*(volatile uint32_t *)(0xB8000070)); 
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
		*(volatile uint32_t *)(0xB8000074) &= ~(0x1<<18);
		*(volatile uint32_t *)(0xB8000074) |= 0x40000000;/*NOTE: just for 3701 chip.*/
	}
	else if((ali_sys_ic_get_chip_id() == ALI_S3503) || (ali_sys_ic_get_chip_id() == ALI_C3505))
	{ 
		strappin70 = (*(volatile uint32_t *)(0xB8000070));
		tmp = strappin70;
		tmp &= ~(1<<18);
		tmp |= (1<<30);
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
	else if(ali_sys_ic_get_chip_id() == ALI_C3921)
	{
		__REG32ALI(0x18000074) = (1<<23)|(0<<18);/* strap pin to nor.*/
	}
	else if(ali_sys_ic_get_chip_id() == ALI_S3821)
	{
		int strappin_0x70 = (*(volatile uint32_t *)(0xB8000070)); 
		tmp = strappin_0x70; 
		tmp &= ~(1<<18); 
		tmp |= (1<<30); 
		*(volatile uint32_t *)(0xB8000074) = tmp;
	}
}

static void m36wdt_init(void)
{
#if defined(CONFIG_ALI_CHIP_M3921)
	dog_m3327e_reg[0].reg_base=0x18018500;
	dog_m3327e_reg[0].irq = 23;
	dog_m3327e_reg[0].timebase=0;
	//sys_mem_clk = ali_sys_ic_get_dram_clock();
	//later will get clock from the right function
	sys_mem_clk = 27;
#else
	dog_m3327e_reg[0].reg_base=0xB8018500;
	dog_m3327e_reg[0].irq = 34;
	dog_m3327e_reg[0].timebase = 0;
	//sys_mem_clk = ali_sys_ic_get_dram_clock();
	//later will get clock from the right function
	sys_mem_clk = 27;
#endif
}

static void m36wdt_get_lefttime(void)
{
	UINT16 div = 0;

	div = DOG_READ8(0, DOG_M3327E_WDTCTRL) & 3;
	div = (1 << (5 + div));
	wdt_timeleft = (0xffffffff - (DOG_READ32(0, DOG_M3327E_WDTCNT)))/sys_mem_clk*div;
}

/*3701c cpu clk from pll clk to 600MHz.*/
static void change_cpu_clk(void)
{
	int i = 0, ret = 0;

	/*cpu clk swith enable.*/
	*((volatile unsigned long *)(0xB8000068)) |= (1<<31);
	for(i=0; i<0x3FF; i++);

	/*cpu clk 600MHz, strap pin control bit[7:9].*/
	*((volatile unsigned long *)(0xB8000074)) |= (1<<31);
	*((volatile unsigned long *)(0xB8000074)) &= ~((1<<8) | (1<<9) | (1<<7));
	for(i=0; i<0x3FF; i++);

	/* cpu clk swith disable */
	*((volatile unsigned long *)(0xB8000068)) &= ~(1<<31);
	for(i=0; i<0x3FF; i++);

	ret = *((volatile unsigned long *)(0xB8000070));
	WDT_DEBUG("[%s] strappin 0x70 = 0x%x \n", __FUNCTION__, ret);
}

static void m36wdt_start(void)
{
	UINT16 div = 0;
	UINT32 a = 0, duration_us = 0;
	unsigned int cpu_clk = 0;

	/*Disable Interrupts.*/
	local_irq_disable();
	if(board_is_nand_boot())
	{
		wdt_reboot_from_nand();
	}
	else
	{
		wdt_reboot_from_nor();
	}

	duration_us = wdt_count; 
	a = 0xffffffff / sys_mem_clk;
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
	spin_lock(&wdt_lock);

	dog_m3327e_reg[0].timebase = 0xffffffff - (duration_us / (1 << (5 + div)) * sys_mem_clk);
	DOG_WRITE32(0, DOG_M3327E_WDTCNT, dog_m3327e_reg[0].timebase);/* It is watchdog mode */

	if(ALI_C3701 == ali_sys_ic_get_chip_id())/*for M3912 dongle.*/
	{
		cpu_clk = *((volatile unsigned long *)(0xB8000070));
		WDT_DEBUG("4[%s] 70 =0x%x end.\n",__FUNCTION__,(unsigned int)cpu_clk);
		cpu_clk = (cpu_clk>>7)&0x07;
		/* cpu clk is pll clk, pll clk have no output after watch dog reboot */
		if(cpu_clk == 0x04)
		{
			/* in order to execute change_cpu_clk() */
			DOG_WRITE32(0, DOG_M3327E_WDTCNT, dog_m3327e_reg[0].timebase-0xfff);/* It is watchdog mode */
			DOG_WRITE8(0, DOG_M3327E_WDTCTRL, 0x64 | div);
			//load_to_icache(change_cpu_clk,0x200); define in ali_soc 
			change_cpu_clk();
		}
	}

	DOG_WRITE8(0, DOG_M3327E_WDTCTRL, 0x64 | div);
	spin_unlock(&wdt_lock);
}

static void m36wdt_stop(void)
{
	spin_lock(&wdt_lock);
	DOG_WRITE32(0, DOG_M3327E_WDTCTRL, 0);
	DOG_WRITE32(0, DOG_M3327E_WDTCNT, 0);
	spin_unlock(&wdt_lock);
}

static void m36wdt_ping(void)
{
	//UINT16 div = 0;
	UINT32 us = 0;

	us = wdt_count; 
	spin_lock(&wdt_lock);

	//keeplive: use the old time out
	//div = DOG_READ8(0, DOG_M3327E_WDTCTRL) & 3;
	//div = (1 << (5 + div));
	//dog_m3327e_reg[0].timebase = 0xffffffff - (us / div * sys_mem_clk);

	DOG_WRITE32(0, DOG_M3327E_WDTCNT, dog_m3327e_reg[0].timebase);
	spin_unlock(&wdt_lock);
}

static int m36wdt_open(struct inode *inode, struct file *file)
{
	//m36wdt_start();
	m36wdt_init();
	return nonseekable_open(inode, file);
}

static int m36wdt_release(struct inode *inode, struct file *file)
{
	//m36wdt_stop();
	return 0;
}

#if 0/*davy.Lee add that remove unused code to clear warning status juring compilation.*/
static ssize_t m36wdt_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	if (len)
	{
		wdt_count = len;
		m36wdt_start();
		m36wdt_ping();
	}

	return len;
}
#endif 

static long m36wdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int options, retval = (-EINVAL);
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	unsigned int reboot_sec = 0;
	unsigned int new_margin = 0;

	static struct watchdog_info ident = {
		.options = WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
		.firmware_version = 0,
		.identity = "Hardware Watchdog for ALI M36",
	};

	switch (cmd)
	{
		case WDIOC_GETSUPPORT:
			if(copy_to_user((struct watchdog_info *)arg, &ident, sizeof(ident)))
			{
				return -EFAULT;
			}
			return 0;

		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			return put_user(0, (int *)arg);

		case WDIOC_SETOPTIONS:
			if(get_user(options, (int *)arg))
			{
				return -EFAULT;
			}

			if(options & WDIOS_DISABLECARD)
			{
				m36wdt_stop();
				retval = 0;
			}

			if(options & WDIOS_ENABLECARD)
			{
				m36wdt_start();
				retval = 0;
			}

			return retval;

		case WDIOC_KEEPALIVE:
			//if (get_user(new_margin, p))
			//return -EFAULT;
			//wdt_count = new_margin;
			//keeplive: use the old time out
			m36wdt_ping();
			return 0;

		case WDIOC_GETTIMEOUT:
			return put_user(WATCHDOG_TIMEOUT*1000000, (int *)arg);

		case WDIOC_SETTIMEOUT://start counting
			if (get_user(new_margin, p))
			{
				return -EFAULT;
			}
			wdt_count = new_margin;
			m36wdt_start();

		case WDIOC_GETTIMELEFT:
			m36wdt_get_lefttime();
			return put_user(wdt_timeleft, (unsigned int*)arg);

		case WDIOC_WDT_REBOOT:
			if(get_user(reboot_sec, p))
			{
				return -EFAULT;
			}

			if(board_is_nand_boot())
			{
				printk(KERN_EMERG "\n====>Function:%s, Line:%d, Start to reboot from nand flash......\n",
					__FUNCTION__, __LINE__);
				wdt_reboot_from_nand();
			}
			else
			{
				printk(KERN_EMERG "\n====>Function:%s, Line:%d, Start to reboot from nor flash......\n",
					__FUNCTION__, __LINE__);
				wdt_reboot_from_nor();
			}

			return 0;

		default:
			return (-ENOTTY);
	}
}

static int m36_notify_sys(struct notifier_block *this, unsigned long code, void *unused)
{
	if(code == SYS_DOWN || code == SYS_HALT)
	{
		m36wdt_stop();/*Turn the WDT off.*/
	}

	return NOTIFY_DONE;
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int dog_m33_suspend(struct device *pdev)
{
	dog_pm_cfg.dog_cnt=DOG_READ32(0, DOG_M3327E_WDTCNT);
	dog_pm_cfg.dog_ctrl= DOG_READ8(0, DOG_M3327E_WDTCTRL);
	return 0;
}

static int dog_m33_resume(struct device *pdev)
{
	DOG_WRITE32(0,DOG_M3327E_WDTCNT, dog_pm_cfg.dog_cnt);
	DOG_WRITE8(0,DOG_M3327E_WDTCTRL, dog_pm_cfg.dog_ctrl);
	return 0;
}


static const struct dev_pm_ops dog_m33_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dog_m33_suspend, dog_m33_resume)
};
static int dog_probe(struct platform_device *pdev)
{	
	return 0;
}

static int dog_remove(struct platform_device *pdev)
{	
	return 0;
}

static struct platform_driver dog_driver = {
	.probe = dog_probe,
	.remove = dog_remove,
	.driver = {
	.name = "watchdog",
		.owner = THIS_MODULE,
		.pm	= &dog_m33_pm_ops,
	},
};
static struct platform_device dog_device = {
	.name = "watchdog",
	.id = 0,
	.dev = {
		.driver = &dog_driver.driver,
	}
};
#endif

/*kernel interface.*/
static const struct file_operations m36wdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.write = NULL, //m36wdt_write , it's recommanded to use ioctl
	.unlocked_ioctl = m36wdt_ioctl,
	.open = m36wdt_open,
	.release = m36wdt_release,
};

static struct miscdevice m36wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &m36wdt_fops,
};

static struct notifier_block m36dog_notifier = {
	.notifier_call = m36_notify_sys,
};

static char banner[] __initdata = KERN_INFO PFX "Hardware Watchdog Timer for ALI M36 Platform\n";

static int __init watchdog_init(void)
{
	int ret = 0;
	spin_lock_init(&wdt_lock);
	ret = register_reboot_notifier(&m36dog_notifier);
	if(ret)
	{
		printk(KERN_ERR PFX
			"cannot register reboot notifier (err=%d)\n", ret);
		return ret;
	}

	ret = misc_register(&m36wdt_miscdev);
	if(ret)
	{
		printk(KERN_ERR PFX
			"cannot register miscdev on minor=%d (err=%d)\n",
							WATCHDOG_MINOR, ret);
		unregister_reboot_notifier(&m36dog_notifier);
		return ret;
	}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
	if(0 != platform_driver_register(&dog_driver))
	{
		printk(KERN_ERR"register driver error!!\n");
	}
	platform_device_register(&dog_device);
#endif
	printk(banner);

	return 0;
}

static void __exit watchdog_exit(void)
{
	misc_deregister(&m36wdt_miscdev);
	unregister_reboot_notifier(&m36dog_notifier);
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	platform_device_unregister(&dog_device);
	platform_driver_unregister(&dog_driver);
#endif
}

module_init(watchdog_init);
module_exit(watchdog_exit);
MODULE_DESCRIPTION("Hardware Watchdog Device for ALI M36 Platform");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
