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
 * 
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
//=====================================================================================//

/* Default value of resume key.*/
#define IR_POWER_VALUE                                   0x60df708f//ali demo stb power key.
#define IR_POWER_VALUE1                                 0x10efeb14//nmp power key1.
#define IR_POWER_VALUE2                                 0x00ff00ff//nmp power key2.
//#define PM_THREAD_STATUS

//=====================================================================================//

/*Debug the suspend state.*/
#ifdef PM_SUSPEND_STATUS_DEBUG
volatile e_pm_state __pm_state = -1;
#endif

#ifdef PM_THREAD_STATUS
static char *pm_task_status[] = { \
    "Running", "Interruptible", "Uninterruptible", \
    "Stopped", "Traced", "exit Zombie", "exit Dead", \
    "Dead", "Wakekill", "Unknown"
};
#endif
//=====================================================================================//

extern void standby_in_pmu_ram(void);
extern int ali_3921_pm_suspend(void);
extern void ali_suspend_output_string(unsigned char *string);
extern int ali_see_enter_standby(void);
extern int ali_see_exit_standby(void);
extern int ali_3921_pm_suspend(void);
//=====================================================================================//

#ifdef PM_THREAD_STATUS
static inline int ali_suspend_get_status_bit(int state)
{
	switch(state)
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
			printk("ir power key[%ld]:%08lx\n", i, resume_key.ir_power[i]);
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
	//year   mon    date     hour    min   sec
	memset(&utc_time, 0, sizeof(struct rtc_time));

	ali_suspend_output_string("============>Function ali_suspend_enter step1......");
#ifdef PM_THREAD_STATUS
	for_each_process(p)
	{
		printk("name: %s   status: %d %s task struct %p\n", p->comm, p->state, \
			pm_task_status[ali_suspend_get_status_bit(p->state)], p);
	}
#endif

#ifdef PM_SUSPEND_STATUS_DEBUG
    __pm_state = PM_IRQ_DISABLE;
#endif
	ali_suspend_output_string("============>Function ali_suspend_enter step2......");
	pm_standby_prepare(PM_ENTER_STANDBY);
	operate_device(PM_DISABLE_DEVICE);

#if 0//Add for debug.
	do_gettimeofday(&cur_time); 
	rtc_time_to_tm(cur_time.tv_sec,&utc_time);
	printk("UTC time :%d-%d-%d %d:%d:%d \n" ,utc_time.tm_year+1900,utc_time.tm_mon, utc_time.tm_mday,utc_time.tm_hour,utc_time.tm_min,utc_time.tm_sec);
	time= (utc_time.tm_sec & 0x3F ) | ((utc_time.tm_min & 0x3F )<<6)  | ((utc_time.tm_hour & 0x1F )<<12) | ((utc_time.tm_mday & 0x1F)<<17)
		| ((utc_time.tm_mon & 0xF) << 22) | (((utc_time.tm_year % 100) & 0x3F)<<26);
#endif

	ali_suspend_output_string("============>Function ali_suspend_enter step3......");
#ifdef PM_SUSPEND_STATUS_DEBUG
	__pm_state = PM_ENTER_CACHE;
#endif

	standby_param.board_power_gpio = 68;
	//standby_in_pmu_ram();
	ali_3921_pm_suspend();
#ifdef PM_SUSPEND_STATUS_DEBUG
    __pm_state = PM_LEAVE_CACHE;
#endif

	//err = rtc_read_time(rtc, &tm);
	//rtc_tm_to_time(&tm, &tv.tv_sec);
	tv.tv_sec = cur_time.tv_sec + sleep_sec_count;
	tv.tv_nsec = 0;
	//do_settimeofday(&tv);
	operate_device(PM_ENABLE_DEVICE);
	pm_standby_prepare(PM_EXIT_STANDBY);
	ali_suspend_output_string("============>Function ali_suspend_enter step4......");

#ifdef PM_SUSPEND_STATUS_DEBUG
    __pm_state = PM_IRQ_ENABLE;
#endif
	ali_suspend_output_string("============>Function ali_suspend_enter step5, run in memory......");

#ifdef PM_THREAD_STATUS
	for_each_process(p)
	{
		printk("name: %s   status: %d %s task struct %p\n", p->comm, p->state, \
			pm_task_status[ali_suspend_get_status_bit(p->state)], p);
	}
#endif
	ali_suspend_output_string("============>Function ali_suspend_enter step6, suspend leave......");

	return 0;
}

static struct platform_suspend_ops ali_suspend_ops = {
	.valid = ali_suspend_state_valid,
	.prepare = ali_see_enter_standby,
	.enter = ali_suspend_enter,
	.wake = ali_see_exit_standby,
};

void ali_suspend_register_ops(void)
{
	ali_suspend_output_string("============>Function ali_suspend_register_ops step1......");
	suspend_set_ops(&ali_suspend_ops);
	ali_suspend_output_string("============>Function ali_suspend_register_ops step2......");
}
