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
 
 *  File: ali_suspend.c
 *  (I)
 *  Description: ALi power management implementation
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2011.03.07				Owen			Creation
 ****************************************************************************/
#include "ali_suspend.h"
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/version.h>
#include <ali_reg.h>
//====================================================================================//

/*.Spinlock for suspend.*/
static DEFINE_SPINLOCK(ali_suspend_lock);
//====================================================================================//

static int ali_suspend_state_valid(suspend_state_t suspend_state);
int ali_suspend_enter(suspend_state_t suspend_state);
extern void standby_in_pmu_ram(void);
extern int ali_str_pm_suspend(void);
extern int early_mcomm_resume(void);
extern int ali_see_enter_standby(void);
extern void ali_see_exit_standby(void);
//====================================================================================//

/*Enable debug feature.*/
#ifdef PM_THREAD_STATUS
static char *pm_task_status[] = {
	"Running", "Interruptible", "Uninterruptible", \
	"Stopped", "Traced", "exit Zombie", "exit Dead", \
	"Dead", "Wakekill", "Unknown"
};
#endif

/*Accept the key.*/
static pm_key_t resume_key = {
	.standby_key = 0,
	.ir_power[0] = IR_POWER_VALUE,
	.ir_power[1] = IR_POWER_VALUE1,
	.ir_power[2] = IR_POWER_VALUE2,
	.ir_power[3] = IR_POWER_VALUE,
	.ir_power[4] = IR_POWER_VALUE,
	.ir_power[5] = IR_POWER_VALUE,
	.ir_power[6] = IR_POWER_VALUE,
	.ir_power[7] = IR_POWER_VALUE,
};

/*Accept the parameter.*/
static pm_param_t standby_param = {
	.board_power_gpio = -1,
	.timeout = 0,
	.reboot = 0,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
static struct platform_suspend_ops ali_suspend_ops = {
	.valid = ali_suspend_state_valid,
	.prepare = ali_see_enter_standby,
	.enter = ali_suspend_enter,
	.wake = ali_see_exit_standby,
};
#else
static struct platform_suspend_ops ali_suspend_ops = {
	.valid = ali_suspend_state_valid,
	.enter = ali_suspend_enter,
};
#endif
//====================================================================================//

#ifdef PM_THREAD_STATUS
static inline int ali_suspend_get_status_bit(int state)
{
	switch (state)
	{
		case TASK_RUNNING:
			return 0;

		case TASK_INTERRUPTIBLE:
			return 1;

		case TASK_UNINTERRUPTIBLE:
			return 2;

		case __TASK_STOPPED:
			return 3;

		case __TASK_TRACED:
			return 4;

		case EXIT_ZOMBIE:
			return 5;

		case EXIT_DEAD:
			return 6;

		case TASK_DEAD:
			return 7;

		case TASK_WAKEKILL:
			return 8;

		default:
			printk("Task status is 0x%x\n", state);
			return 9;
	}
}
#endif

/*Set the resume key value.*/
void ali_suspend_set_resume_key(pm_key_t *pm_key)
{
	unsigned long i = 0;

	if(NULL != pm_key)
	{
		resume_key.standby_key = pm_key->standby_key;
		for(i=0; i<8; i++)
		{
			if((pm_key->ir_power[i] != 0) && ( pm_key->ir_power[i] != 0xffffffff))
			{
				resume_key.ir_power[i] = pm_key->ir_power[i];
			}
			else
			{
				resume_key.ir_power[i] = 0x5a5a55aa;
			}
			printk("ir power key[%ld]:%08lx \n", i, resume_key.ir_power[i]);
		}
	}
}

/*Set the resume key value.*/
void ali_suspend_set_standby_param(pm_param_t *p_standby_param)
{
	if(NULL != p_standby_param)
	{
		standby_param.board_power_gpio = p_standby_param->board_power_gpio;
		standby_param.timeout = p_standby_param->timeout;
		standby_param.reboot = p_standby_param->reboot;
	}
}

/*To valie the status.*/
static int ali_suspend_state_valid(suspend_state_t suspend_state)
{
	switch(suspend_state)
	{
		case PM_SUSPEND_ON:
		case PM_SUSPEND_STANDBY:
		case PM_SUSPEND_MEM:
			return 1;

		default:
			return 0;
	}

	return 0;
}

int ali_suspend_enter(suspend_state_t suspend_state)
{
	struct timeval cur_time; 
	struct timespec tv;
	struct rtc_time utc_time;
	unsigned int sleep_sec_count = 0;
	//31...26 25...22 21...17 16...12 11...6 5...0
	//year       mon     date     hour    min   sec

	memset(&utc_time, 0, sizeof(struct rtc_time));
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step1....\n");
#endif
	
	/*We don't need to wait here, thanks to the loop in the cache.*/
	/*init_waitqueue_head(&suspend_wq);
	wait_event_interruptible(suspend_wq, (1 == wq_ret));.*/

	/*This process must be atomic, or we'll be unknown status.*/
	/*But we don't care about that, due to IRQ is disable when enter state.*/

#ifdef PM_THREAD_STATUS
	for_each_process(p)
	{
		printk("name: %s status: %d %s task struct %p\n", p->comm, p->state, \
			pm_task_status[ali_suspend_get_status_bit(p->state)], p);
	}
#endif

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step2....\n");
#endif

	operate_device(PM_DISABLE_DEVICE);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step3....\n");
#endif

	standby_param.board_power_gpio = 68;
	mdelay(10);
	mdelay(10);

	ali_str_pm_suspend();
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step4....\n");
#endif

	if(standby_param.reboot == 1)
	{
		while(1);
	}
	tv.tv_sec = cur_time.tv_sec + sleep_sec_count;
	tv.tv_nsec = 0;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step5....\n");
#endif

	operate_device(PM_ENABLE_DEVICE);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step6....\n");
#endif

#ifdef PM_THREAD_STATUS
	for_each_process(p)
	{
		printk("name: %s status: %d %s task struct %p\n", p->comm, p->state, \
			pm_task_status[ali_suspend_get_status_bit(p->state)], p);
	}
#endif

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_enter step7....\n");
#endif
	return 0;
}

void ali_suspend_register_ops(void)
{
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_register_ops step1....\n");
#endif
	spin_lock_init(&ali_suspend_lock);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_register_ops step2....\n");
#endif
	suspend_set_ops(&ali_suspend_ops);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "ali_suspend_register_ops step3....\n");
#endif
}
