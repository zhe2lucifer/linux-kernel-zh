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
 
#ifndef __PORTING_LINUX_HEADER_H__
#define __PORTING_LINUX_HEADER_H__

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <asm/irq.h>
#include <linux/ali_transport.h>
#include <linux/i2c.h>
#include <linux/wait.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/kthread.h>
 
#include <ali_dmx_common.h>
//#include <linux/dvb/ali_i2c_scb_gpio.h>
#include <ali_i2c_scb_gpio.h>

#include <asm/mach-ali/typedef.h>
#include <dvb_frontend_common.h>
#include <ali_reg.h>

#include "basic_types.h"
#include "nim_device.h"

#include "../alii2c/gpio_i2c.h"

 
#include <linux/ali_gpio.h>
 

#define nim_print           	         printk
#define printf                           printk
#define OSAL_INVALID_ID			         0xFFFFFFFF
#define NIM_TUNER_SET_STANDBY_CMD        0xFFFFFFFF
#define TIMER_DELAY			100		/* ms */
//#define NIM_TUNER_SET_LOOPTHROUGH_CMD    NIM_TUNER_SET_STANDBY_CMD - 1

//for request GPIO with initial configuration flags
#define GPIO_F_DIR_OUT		(0 << 0)
#define GPIO_F_DIR_IN		(1 << 0)

#define GPIO_F_INIT_LOW		(0 << 1)
#define GPIO_F_INIT_HIGH	(1 << 1)

#define GPIO_F_IN				 GPIO_F_DIR_IN)
#define GPIO_F_OUT_INIT_LOW		(GPIO_F_DIR_OUT | GPIO_F_INIT_LOW)
#define GPIO_F_OUT_INIT_HIGH	(GPIO_F_DIR_OUT | GPIO_F_INIT_HIGH)

//for ctrl disqec polar use  pinmux or GPIO 
#define DISEQC_POLAR_SET_NO_USE_GPIO -1


#define CHANNEL_CHANGE_ASYNC
#define OSAL_TWF_ORW				0
#define OSAL_TWF_ANDW 				0
#define OSAL_TWF_CLR				0

#define FAST_TIMECST_AGC	        1
#define SLOW_TIMECST_AGC	        0
#define _1ST_I2C_CMD		        0
#define _2ND_I2C_CMD		        1


#define tuner_chip_sanyo            9
#define TUNER_CHIP_CD1616LF_GIH     8
#define tuner_chip_nxp		        7
#define tuner_chip_maxlinear	    6
#define tuner_chip_microtune	    5
#define tuner_chip_quantek	        4
#define tuner_chip_rfmagic          3
#define tuner_chip_alps		        2	//60120-01Angus
#define tuner_chip_philips	        1
#define tuner_chip_infineon	        0


#define comm_malloc(x)				kmalloc((x),GFP_KERNEL)//kmalloc((x), GFP_ATOMIC)
#define comm_memset 				memset
#define comm_free 					kfree
#define free     					kfree
#define malloc(x)                   kmalloc((x),GFP_KERNEL)
#define comm_sleep 					msleep
#define comm_memcpy 				memcpy
#define nim_i2c_read				ali_i2c_read
#define nim_i2c_write				ali_i2c_write
#define nim_i2c_write_read			ali_i2c_write_read
#define gettimeofday(x,y)           do_gettimeofday(x)
#define realloc(x,y)                krealloc((x),(y),GFP_KERNEL)

#define comm_delay(x)				\
	do \
	{\
	   if(x>1000)\
	   {\
	   	  mdelay(x/1000);\
	   }\
	   else if(x)\
	   {\
	   	  udelay(x);\
	   }\
	}while(0)



#define NIM_MUTEX_ENTER(priv)  \
	do \
	{ \
		mutex_lock(&priv->i2c_mutex); \
	}while(0)

#define NIM_MUTEX_LEAVE(priv) \
	do\
	{ \
		mutex_unlock(&priv->i2c_mutex);\
	}while(0)

typedef enum
{
	   NIM_TUNER_M3031_ID =0,	
	   NIM_TUNER_SET_LOOPTHROUGH_CMD,
	   NIM_TUNER_SET_M3031_FREQ_ERR,
	   NIM_TUNER_GET_M3031_FREQ_ERR,
	   NIM_TUNER_GET_M3031_GAIN,
	   NIM_TUNER_GET_M3031_GAIN_FLAG,
}TUN_CMD;




UINT32 osal_get_tick(void);



typedef struct _flag_lock
{
    rwlock_t 		flagid_rwlk;  //Protect the following flag_id
    UINT32 			flag_id; 	  //Asynchronous channel change control flag
}NIM_FLAG_LOCK;

struct sys_reg{
	void __iomem   *addr;
	UINT32         a_fun_bit_clr;
	UINT32         a_fun_bit_set;
	UINT32         b_fun_bit_clr;
	UINT32         b_fun_bit_set;
};
struct debug_i2c{
	UINT8 rw_flag;//0 : r,1: w
	UINT8 reg_addr;
	UINT8 reg_data;
};
UINT32 		nim_flag_read(NIM_FLAG_LOCK *flag_lock, UINT32 T1, UINT32 T2, UINT32 T3);
UINT32 		nim_flag_create(NIM_FLAG_LOCK *flag_lock);
UINT32 		nim_flag_set(NIM_FLAG_LOCK *flag_lock, UINT32 value);
UINT32 		nim_flag_clear(NIM_FLAG_LOCK *flag_lock, UINT32 value);
UINT32 		nim_flag_del(NIM_FLAG_LOCK *flag_lock);


void        nim_print_x(int log_level,const char *fmt, ...);
void        set_log_level(int log_level);

enum{
	ISDBT_TYPE = 0,
	DVBT_TYPE,
	DVBT2_TYPE,
	DVBT2_COMBO
};



/*******************add for nim proc debug***************/
enum{
	TASK_DEFAULT = 0, //task  is not initialized
	TASK_RUN,     //task is runing
	TASK_EXIT	  //task exit
};
struct nim_debug
{
	struct proc_dir_entry *nim_dir;
	struct proc_dir_entry *monitor_control_file;
	struct proc_dir_entry *i2c_file;
	struct proc_dir_entry *monitor_info_file;
	struct proc_dir_entry *dev_info_file;
	//can set more objects of proc_dir_entry in here
	UINT8  monitor_status;//TASK_DEFAULT, TASK_RUN,TASK_EXIT
	int monitor_object;//bit[0]Always print per ;bit[1]print per when not 0bit[2] lock; bit[3] rf; bit[4] cn;bit[5] ssi;  bit[6] sqi;"
};
#endif



