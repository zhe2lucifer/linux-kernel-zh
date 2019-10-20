/*
 * ali pan driver for ALi SoCs
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Dai <david.dai@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/leds.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/ali_transport.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include "ali_pan_ch455.h"
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <ali_front_panel_common.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/ali_kumsgq.h>
#define ALI_PANEL_INFO	"ALi Panel(CH455) Driver"

/*0	[INTENS]	[7SEG]	[SLEEP]	0	[ENA]*/
/*0	000		0		0		0	1*/
#define CH455_MODE	0x01

#define SETING_ADDR	0x48
#define KEY_ADDR	0x4f

#define CH455_KEY_STATUS_MASK	0x40
#define CH455_KEY_ROW_MASK		0x38
#define CH455_KEY_COLUMN_MASK	0x03

#define CH455_STATUS_UP		0
#define CH455_STATUS_DOWN	1

/* ESC command: 27 (ESC code), PAN_ESC_CMD_xx (CMD type), param1, param2 */
#define PAN_ESC_CMD_LBD		'L'	/* LBD operate command */
#define PAN_ESC_CMD_LBD_FUNCA	0	/* Extend function LBD A */
#define PAN_ESC_CMD_LBD_FUNCB	1	/* Extend function LBD B */
#define PAN_ESC_CMD_LBD_FUNCC	2	/* Extend function LBD C */
#define PAN_ESC_CMD_LBD_FUNCD	3	/* Extend function LBD D */
#define PAN_ESC_CMD_LBD_LEVEL	5	/* Level status LBD, no used */

#define PAN_ESC_CMD_LED			'E'	/* LED operate command */
#define PAN_ESC_CMD_LED_LEVEL	0	/* Level status LED */

#define PAN_ESC_CMD_LBD_ON		1	/* Set LBD to turn on status */
#define PAN_ESC_CMD_LBD_OFF		0	/* Set LBD to turn off status */

#define PAN_MAX_LED_NUM		4

#define PAN_MAX_CHAR_LIST_NUM	70


#define REPEAT_DELAY			300		/* ms */
#define REPEAT_INTERVAL			250		/* ms */
#define TIMER_DELAY			100		/* ms */
#ifdef SUCCESS
	#undef SUCCESS
	#define SUCCESS 0
#else
	#define SUCCESS 0
#endif

#ifdef ERR_FAILURE
	#undef ERR_FAILURE
	#define ERR_FAILURE -1
#else
	#define ERR_FAILURE -1
#endif

struct led_bitmap {
	u8 character;
	u8 bitmap;
};

enum {
	KEY_RELEASE		= 0,
	KEY_PRESSED		= 1,
	KEY_REPEAT		= 2
};

#define PAN_KEY_INVALID			0xFFFFFFFF
#define IR_TYPE_UNDEFINE 9
/*cara.shi    2016/8/1*/
struct ali_ch455_com_device_data {
	/*struct	cdev cdev;*/
	struct kumsgq *ch455_kumsgq;
	struct mutex ch455_mutex;
	/*void    *priv;*/
};

struct ali_ch455_com_device_data ali_ch455_dev;
static struct led_bitmap bitmap_table[PAN_MAX_CHAR_LIST_NUM * 2] = {
	{'.', 0x80}, /* Let's put the dot bitmap into the table */
	{'0', 0x3f}, {'1', 0x06}, {'2', 0x5b}, {'3', 0x4f},
	{'4', 0x66}, {'5', 0x6d}, {'6', 0x7d}, {'7', 0x07},
	{'8', 0x7f}, {'9', 0x6f}, {'a', 0x5f}, {'A', 0x77},
	{'b', 0x7c}, {'B', 0x7c}, {'c', 0x39}, {'C', 0x39},
	{'d', 0x5e}, {'D', 0x5e}, {'e', 0x79}, {'E', 0x79},
	{'f', 0x71}, {'F', 0x71}, {'g', 0x6f}, {'G', 0x3d},
	{'h', 0x76}, {'H', 0x76}, {'i', 0x04}, {'I', 0x30},
	{'j', 0x0e}, {'J', 0x0e}, {'l', 0x38}, {'L', 0x38},
	{'n', 0x54}, {'N', 0x37}, {'o', 0x5c}, {'O', 0x3f},
	{'p', 0x73}, {'P', 0x73}, {'q', 0x67}, {'Q', 0x67},
	{'r', 0x50}, {'R', 0x77}, {'s', 0x6d}, {'S', 0x6d},
	{'t', 0x78}, {'T', 0x31}, {'u', 0x3e}, {'U', 0x3e},
	{'y', 0x6e}, {'Y', 0x6e}, {'z', 0x5b}, {'Z', 0x5b},
	{':', 0x80}, {'-', 0x40}, {'_', 0x08}, {' ', 0x00},
};

#define PAN_CH455_CHAR_LIST_NUM (sizeof(bitmap_table)/sizeof(struct led_bitmap))
static u8 g_display_backup[PAN_MAX_LED_NUM][PAN_MAX_LED_NUM] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};

/*
*ali_ch455_key_map.code :
*row       : bit0-bit2
*column : bit3-bit4
*/
static const struct ali_fp_key_map_t ali_linux_pan_key_map[] = {
	{(PAN_ALI_HKEY_UP),			KEY_UP},
	{(PAN_ALI_HKEY_LEFT),		KEY_LEFT},
	{(PAN_ALI_HKEY_ENTER),		KEY_ENTER},
	{(PAN_ALI_HKEY_RIGHT),		KEY_RIGHT},
	{(PAN_ALI_HKEY_DOWN),		KEY_DOWN},
	{(PAN_ALI_HKEY_MENU),		KEY_MENU},
#if defined(CONFIG_M3921_GPIO_DETECT)
	{(PAN_ALI_HKEY_UPGRADE),	KEY_F12},
#else
	{(PAN_ALI_HKEY_POWER),		KEY_POWER},
#endif
};

static struct ali_fp_key_map_cfg g_ch455_key_map;
struct input_dev *ch455_input;
struct mutex ch455_lock;
struct ch455_device {
	struct timer_list timer;
	struct mutex lock;
	u32	i2c_id;
	u8 gpio_i2c;
	u8	mode;

    struct delayed_work worker;
	int run;
	struct led_bitmap	*bitmap_list;
	u32 bitmap_len;

	u8	mask_status;			/* key status bit mask */
	u8	mask_row;			/* key code row bit mask */
	u8	mask_column;		/* key code column bit mask */

	u32	key_cnt;
	u32	keypress_cnt;
	u32	keypress_intv;		/* Continue press key interval */
	u32	keypress_bak;		/* Pressed key saver */
	u8	bak_status;

	u8	lbd_func_flag;

	u32	(*read8)(struct ch455_device *pch455,  u8 dig_addr, u8 *data);
	u32	(*write8)(struct ch455_device *pch455, u8 dig_addr, u8 data);
	u8	cur_bitmap[PAN_MAX_LED_NUM];
};

static unsigned long pan_key_received_count;
static unsigned long pan_key_mapped_count;
static unsigned char pan_debug;
static unsigned long ch455_repeat_delay_rfk = 600;
static unsigned long ch455_repeat_interval_rfk = 350;
static unsigned long ch455_repeat_width_rfk;
static unsigned int ch455_rfk_port;
static struct proc_dir_entry *proc_ch455;
static unsigned int  ch455_i2c_info;/*use 4th i2c id as default panel config*/

static struct ch455_device *g_pch455;

#define PANEL_PRINTK(fmt, args...)			\
{									\
	if (pan_debug != 0) {			\
		pr_info(fmt, ##args);		\
	}								\
}

#define PANEL_ERR_PRINTK(fmt, args...)	\
{								\
	pr_info(fmt, ##args);		\
}



static int  ch455_mode_set(struct ch455_device *pch455);
static unsigned char  g_dig_addr0;
static unsigned char  g_dig_addr1;
static unsigned char  g_dig_addr2;
static unsigned char  g_dig_addr3;
#include <linux/seq_file.h>
static int ch455_read_proc(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", CH455_DEV_NAME);
	return 0;
}

static int proc_panel_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, ch455_read_proc, PDE_DATA(inode), 256);
}


static const struct file_operations proc_panel_fops = {
	.open		= proc_panel_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


static int ali_ch455_send_msg(unsigned long code, int ir_protocol, int ir_state)
{
	unsigned char msg[16];
	unsigned char i = 0;
	int ret = -1;


	memset(msg, 0x00, sizeof(msg));
	msg[4] = (unsigned char)(code >> 24);
	msg[5] = (unsigned char)(code >> 16);
	msg[6] = (unsigned char)(code >> 8);
	msg[7] = (unsigned char)(code);

	msg[8] = (unsigned char)(ir_protocol >> 24);
	msg[9] = (unsigned char)(ir_protocol >> 16);
	msg[10] = (unsigned char)(ir_protocol >> 8);
	msg[11] = (unsigned char)(ir_protocol);

	msg[12] = (unsigned char)(ir_state >> 24);
	msg[13] = (unsigned char)(ir_state >> 16);
	msg[14] = (unsigned char)(ir_state >> 8);
	msg[15] = (unsigned char)(ir_state);

	PANEL_PRINTK("[ %s %d ], rfk port %d send msg :\n",
				__func__, __LINE__, ch455_rfk_port);
	for (i = 0; i < sizeof(msg); i++)
		PANEL_PRINTK("%02x ", msg[i]);

	PANEL_PRINTK("\n");

	ret = ali_kumsgq_sendmsg(ali_ch455_dev.ch455_kumsgq, &msg, sizeof(msg));
	if (-1 == ret)
		PANEL_ERR_PRINTK("[ %s %d ], rfk port %d send msg fail!\n",
					__func__, __LINE__, ch455_rfk_port);



	return ret;
}



static long ch455_ioctl(struct file *file, unsigned int cmd,
						unsigned long param)
{
	unsigned long reg[2] = {0, 0};
	struct ch455_device *pch455 = file->private_data;
	int tmp, tmp2;

	if (!pch455)
		return -EFAULT;


	PANEL_PRINTK("[ %s ]: cmd %d\n", __func__, cmd);
	switch (cmd) {
	case PAN_DRIVER_READ_REG:
			if ((copy_from_user(&reg, (void *)param,
					sizeof(reg))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}
			reg[1] = ioread32((void __iomem *)reg[0]);
			if (copy_to_user((void *)param, &reg,
					sizeof(reg)) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			PANEL_PRINTK("[ %s %d ]: read 0x%08x = 0x%08x\n\n",
			__func__,  __LINE__, (unsigned int)reg[0],
			(unsigned int)reg[1]);

			break;

	case PAN_DRIVER_WRITE_REG:
			if ((copy_from_user(&reg, (void *)param,
					sizeof(reg))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}
			iowrite32(reg[1], (void __iomem *)reg[0]);
			PANEL_PRINTK("[ %s %d ]: write 0x%08x = 0x%08x\n",
			__func__,  __LINE__, (unsigned int)reg[0],
			(unsigned int)(reg[1]));

			break;

	case ALI_FP_CONFIG_KEY_MAP:
		{
			unsigned char *map_entry;
			struct ali_fp_key_map_cfg key_map;
			int i;

			if ((copy_from_user(&key_map, (void *)param,
			sizeof(key_map))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			PANEL_PRINTK("[ %s %d ], phy_code = %ld\n",
			__func__, __LINE__, key_map.phy_code);

			g_ch455_key_map.phy_code = key_map.phy_code;
			if (g_ch455_key_map.phy_code != 2) {
				map_entry = kzalloc(key_map.map_len,
				GFP_KERNEL);
				if (map_entry == NULL) {
					PANEL_ERR_PRINTK("[ %s %d ], error\n",
					 __func__, __LINE__);
					return -ENOMEM;
				}

				if ((copy_from_user(map_entry,
				key_map.map_entry, key_map.map_len)) != 0) {
					PANEL_ERR_PRINTK("[ %s %d ], error\n",
					 __func__, __LINE__);
					return -EFAULT;
				}
				key_map.map_entry = map_entry;
				key_map.unit_num =
				(key_map.map_len/key_map.unit_len);
				if (key_map.unit_num > KEY_CNT)
					key_map.unit_num = KEY_CNT;
				kfree(g_ch455_key_map.map_entry);

				g_ch455_key_map.map_entry = NULL;

				for (i = 0; i < key_map.unit_num; ++i) {
					unsigned short key;

					/*phy_code : 0, logic; 1, index.*/
					if (key_map.phy_code == 1) {
						key = (unsigned short)i;
					} else {
						tmp = i*key_map.unit_len;
						key = key_map.map_entry[tmp+4];
						tmp = key_map.map_entry[tmp+5];
						key |= tmp<<8;
					}

					__set_bit(key%KEY_CNT,
						ch455_input->keybit);
				}

				g_ch455_key_map.map_len = key_map.map_len;
				g_ch455_key_map.unit_len = key_map.unit_len;
				g_ch455_key_map.phy_code = key_map.phy_code;
				g_ch455_key_map.unit_num = key_map.unit_num;
				g_ch455_key_map.map_entry = key_map.map_entry;
				if (pan_debug != 0) {
					int i;

					PANEL_PRINTK("\n[ %s ]:\n", __func__);
					tmp = g_ch455_key_map.unit_num;
					for (i = 0; i < tmp; i++) {
						unsigned char *buf;
						unsigned long phy;
						unsigned short log;

						tmp2 = g_ch455_key_map.unit_len;
						buf = &map_entry[tmp2*i];
						phy = buf[0]|(buf[1]<<8)|
						(buf[2]<<16)|(buf[3]<<24);
						log = buf[4]|(buf[5]<<8);
						PANEL_PRINTK("%08x	%04x\n",
						(int)phy, log);
					}
					PANEL_PRINTK("\n");
				}
			}
		}
		break;

	case ALI_FP_GET_PAN_KEY_RECEIVED_COUNT:
		{
			if (copy_to_user((void *)param, &pan_key_received_count,
			sizeof(pan_key_received_count)) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
		}

		PANEL_PRINTK("[ %s ]: pan_key_received_count = %ld\n",
		__func__, pan_key_received_count);
		}
		break;

	case ALI_FP_GET_PAN_KEY_MAPPED_COUNT:
		{
			if (copy_to_user((void *)param, &pan_key_mapped_count,
			sizeof(pan_key_mapped_count)) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			PANEL_PRINTK("[ %s ]: pan_key_mapped_count = %ld\n",
			 __func__, pan_key_mapped_count);
		}
		break;

	case ALI_FP_SET_PAN_KEY_DEBUG:
		{
			if (copy_from_user(&pan_debug,
			(void *)param, sizeof(pan_debug)) != 0) {
			PANEL_ERR_PRINTK("[ %s %d ], error\n",
			__func__, __LINE__);
			return -EFAULT;
			}

			PANEL_PRINTK("[ %s ]: pan_debug = %d\n",
			__func__, pan_debug);
		}
		break;

	case ALI_FP_GET_KUMSGQ:
		{
			int flags = -1;
			int ret = -1;

			mutex_lock(&ali_ch455_dev.ch455_mutex);
			if (copy_from_user(&flags,
			(int *)param, sizeof(int)) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",__func__, __LINE__);
				mutex_unlock(&ali_ch455_dev.ch455_mutex);
				return -EFAULT;
			}
			ret  = ali_kumsgq_newfd(ali_ch455_dev.ch455_kumsgq,
									flags);
			if (ret > 0) {
				mutex_unlock(&ali_ch455_dev.ch455_mutex);
				return ret;
			}
		}
		break;

	case PAN_DRIVER_SET_I2C:
		{
			if ((copy_from_user(&ch455_i2c_info,
			(void *)param, sizeof(ch455_i2c_info))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			mutex_lock(&ch455_lock);
			pch455->i2c_id = ch455_i2c_info & 0xff;
			pch455->gpio_i2c = (u8)((ch455_i2c_info & 0xff00) >> 8);

			PANEL_PRINTK("ch455_i2c_info = 0x%08x\n",
			ch455_i2c_info);
			PANEL_PRINTK("i2c_id = %d\n", pch455->i2c_id);
			PANEL_PRINTK("gpio_i2c = %d\n", pch455->gpio_i2c);

			ch455_mode_set(pch455);
			PANEL_PRINTK("[ %s %d ]: i2c_id = %d\n",
			__func__, __LINE__, pch455->i2c_id);
			mutex_unlock(&ch455_lock);
		}
		break;

	case PAN_DRIVER_SET_DIG_ADDR:
		{
			unsigned int dig_addr = 0;

			if ((copy_from_user(&dig_addr, (void *)param,
						sizeof(dig_addr))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			g_dig_addr0 = dig_addr & 0xff;
			g_dig_addr1 = (u8)((dig_addr & 0xff00) >> 8);
			g_dig_addr2 = (u8)((dig_addr & 0xff0000) >> 16);
			g_dig_addr3 = (u8)((dig_addr & 0xff000000) >> 24);

			g_display_backup[0][0] = g_dig_addr0;
			g_display_backup[1][0] = g_dig_addr1;
			g_display_backup[2][0] = g_dig_addr2;
			g_display_backup[3][0] = g_dig_addr3;

			PANEL_PRINTK("[%s %d]:[0x%02x 0x%02x 0x%02x 0x%02x]\n",
			__func__, __LINE__, g_dig_addr0, g_dig_addr1,
			g_dig_addr2, g_dig_addr3);
		}
		break;

	case PAN_DRIVER_SET_LED_BITMAP:
		{
			struct ali_fp_bit_map_cfg bit_map;
			u32 i;

			if ((copy_from_user(&bit_map, (void *)param,
			sizeof(bit_map))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			mutex_lock(&ch455_lock);

			PANEL_PRINTK("[ %s %d ], map_len = %ld\n",
			__func__, __LINE__, bit_map.map_len);
			if (bit_map.map_len > (PAN_MAX_CHAR_LIST_NUM * 2)) {
				pch455->bitmap_len = PAN_MAX_CHAR_LIST_NUM * 2;
				PANEL_PRINTK("[%s %d], map_len> %d\n", __func__,
				__LINE__, (PAN_MAX_CHAR_LIST_NUM * 2));
			} else {
				pch455->bitmap_len = bit_map.map_len;
			}
			PANEL_PRINTK("[ %s %d ], bitmap_len = %d\n", __func__,
			__LINE__, pch455->bitmap_len);

			memset(pch455->bitmap_list, 0x00, sizeof(bitmap_table));
			if ((copy_from_user(pch455->bitmap_list,
			bit_map.map_entry, pch455->bitmap_len)) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				mutex_unlock(&ch455_lock);
				return -EFAULT;
			}

			if (pan_debug != 0) {
				for (i = 0; i < pch455->bitmap_len; i++) {
					PANEL_PRINTK("'%c' %02x\n",
					pch455->bitmap_list[i].character,
					pch455->bitmap_list[i].bitmap);
				}
				PANEL_PRINTK("\n");
			}

			mutex_unlock(&ch455_lock);
		}
		break;

	case ALI_FP_SET_REPEAT_INTERVAL:
		{
			unsigned long rep[2] = {0, 0};

			if ((copy_from_user(&rep, (void *)param,
							sizeof(rep))) != 0) {
				PANEL_ERR_PRINTK("[ %s %d ], error\n",
				__func__, __LINE__);
				return -EFAULT;
			}

			ch455_repeat_delay_rfk = rep[0];
			ch455_repeat_interval_rfk = rep[1];

			PANEL_PRINTK("[%s]:ch455_repeat_delay_rfk= %ld\n",
			__func__, ch455_repeat_delay_rfk);
			PANEL_PRINTK("[%s]:ch455_repeat_interval_rfk= %ld\n",
			__func__, ch455_repeat_interval_rfk);
		}
		break;

	default:
			return -EPERM;
	}

	return 0;
}


static u32 read8(struct ch455_device *pch455,  u8 dig_addr, u8 *data)
{
	u32 re = (u32)ERR_FAILURE;
	unsigned char offset = dig_addr;
	struct i2c_adapter *adapter;
	struct i2c_msg msgs[] = {
		{ .addr = dig_addr>>1, .flags = 0, .len = 0, .buf = &offset},
		{ .addr = dig_addr>>1, .flags = I2C_M_RD, .len = 1, .buf = data}
	};
	int result = 0;

	if (!pch455 && !data)
		return -EFAULT;

	mutex_lock(&pch455->lock);

	if (pch455->gpio_i2c == 1) {
		adapter = i2c_get_adapter(pch455->i2c_id);
		if (adapter) {
			result = i2c_transfer(adapter, msgs, 2);
			if (result != 2) {
				PANEL_PRINTK("[ %s %d ], i2c_transfer fail\n",
				__func__, __LINE__);
				PANEL_PRINTK("result= %d, name= %s, id= %d\n",
				result, adapter->name, adapter->nr);
				PANEL_PRINTK("nr = %d, class = 0x%08x\n",
				adapter->nr, adapter->class);
				re = (u32)ERR_FAILURE;
			} else {
					re = (u32)SUCCESS;
			}
		} else {
			PANEL_PRINTK("[%s %d],adapter is NULL, i2c_id = %d\n",
			__func__, __LINE__, pch455->i2c_id);
		}
	} else {
		/*re = ali_i2c_scb_read(pch455->i2c_id, dig_addr, data, 1);*/
	}


	mutex_unlock(&pch455->lock);

	/*PANEL_PRINTK("[ %s ], id(%d), addr(0x%02x), data(0x%02x).\n",*/
	/*__func__, pch455->i2c_id, dig_addr, *data);*/

	return re;
}

static u32 write8(struct ch455_device *pch455, u8 dig_addr, u8 data)
{
	u32 re = (u32)ERR_FAILURE;
	unsigned char buf[10];
	struct i2c_adapter *adapter;
	struct i2c_msg msgs = { .addr = dig_addr>>1,
		.flags = 0, .len = 1,  .buf = buf };
	int result = 0;

	if (!pch455)
		return -EFAULT;

	/*PANEL_PRINTK("[ %s ], id = %d, addr = 0x%02x, data = 0x%02x.\n",*/
	/* __func__, pch455->i2c_id, dig_addr, data);*/

	mutex_lock(&pch455->lock);

	if (pch455->gpio_i2c == 1) {
		buf[0] = data;
		adapter = i2c_get_adapter(pch455->i2c_id);
		if (adapter) {
			result = i2c_transfer(adapter, &msgs, 1);
			if (result != 1) {
				PANEL_PRINTK("[ %s %d ], i2c_transfer fail\n",
				__func__, __LINE__);
				PANEL_PRINTK("result= %d, name= %s, id= %d\n",
				result, adapter->name, adapter->nr);
				PANEL_PRINTK("nr = %d, class = 0x%08x\n",
				adapter->nr, adapter->class);
				re = (u32)ERR_FAILURE;
			} else {
				re = (u32)SUCCESS;
			}
		} else {
			PANEL_PRINTK("[%s %d],adapter is NULL, i2c_id = %d\n",
			__func__, __LINE__, pch455->i2c_id);
		}
	} else {
		/*re = ali_i2c_scb_write(pch455->i2c_id, dig_addr, &data, 1);*/
	}

	mutex_unlock(&pch455->lock);

	return re;
}


static int  ch455_mode_set(struct ch455_device *pch455)
{
	int re = ERR_FAILURE;

	if (!pch455)
		return -EFAULT;
	re = pch455->write8(pch455, SETING_ADDR, pch455->mode);
	if (re != SUCCESS)
		PANEL_PRINTK("[ %s ], Failed, re = %d\n", __func__, re);

	return re;
}


static int ch455_key_default_mapping(unsigned long ir_code)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(ali_linux_pan_key_map); i++) {
		if (ali_linux_pan_key_map[i].code == ir_code)
			return ali_linux_pan_key_map[i].key;
	}

	return -1;
}



static int ch455_key_mapping(unsigned long phy_code)
{
	int i, tmp;
	int logic_code = 0;

	if ((g_ch455_key_map.map_entry == NULL)
	|| (g_ch455_key_map.phy_code == 3))
		return ch455_key_default_mapping(phy_code);

	for (i = 0; i < g_ch455_key_map.unit_num; i++) {
		unsigned long my_code;

		tmp = i*g_ch455_key_map.unit_len;
		if ((unsigned long)(&g_ch455_key_map.map_entry[tmp]) & 0x3) {
			my_code = g_ch455_key_map.map_entry[tmp];
			my_code |= (g_ch455_key_map.map_entry[tmp+1])<<8;
			my_code |= (g_ch455_key_map.map_entry[tmp+2])<<16;
			my_code |= (g_ch455_key_map.map_entry[tmp+3])<<24;
		} else {
			my_code = *((unsigned long *)
			(&g_ch455_key_map.map_entry[tmp]));
		}

		if (phy_code == my_code) {
			if (g_ch455_key_map.phy_code == 1) {
				PANEL_PRINTK("[ %s ], phy code index %d\n",
				__func__, i);
			} else {
				logic_code =  g_ch455_key_map.map_entry[tmp+4];
				logic_code |=
				(g_ch455_key_map.map_entry[tmp+5])<<8;
				PANEL_PRINTK("logic code 0x%08x\n", logic_code);

				return logic_code;
			}
			return i;
		}
	}


	return -1;
}

static void ch455_worker(struct work_struct *work)
{
    struct ch455_device *pch455 = container_of(container_of(work, struct delayed_work, work),struct ch455_device, worker);
	u8 row, column, status;
	u8 data = 0xff;
	u32 re = SUCCESS;
	s32 mode_flag = 0;
	s32 key_index = 0;
	unsigned long code = 0;
	static unsigned long last_repeat_tick;
	unsigned long last_repeat_width;
	unsigned long repeat_tick = 0;
	u8 i = 0;
	u8 count = 0;
	u8 count_fail = 0;
	u8 sleep_flag = 0;
    int delay = TIMER_DELAY;

	if (!pch455)
		return;

	/*mutex_lock(&ch455_lock);*/
	/* about 1ms */
	re = pch455->read8(pch455,  KEY_ADDR, &data);
	if (re != SUCCESS) {
		mode_flag = 1;
		PANEL_PRINTK("CH455: Scan keyboard failed!re = %d.\n",
			re);
        return;//goto ch455_sleep;
	} else {
		/*bit 7 should always be 0, bit 2 should always be 1.*/
		if (((data & 0x80) != 0) || ((data & 0x04) == 0)) {
			goto ch455_sleep;
		} else {
			/* bit7 = 0, bit2 = 1 */
			/*(bit 0~1)*/
			column = data & pch455->mask_column;
			/*(bit 3~5)	*/
			row = (data & pch455->mask_row) >> 3;
			/*bit 6*/
			status = (data & pch455->mask_status) >> 6;
			code = 0xFFFF0000 |
			(unsigned long)((column << 3) | row);
		}
	}

	/* up : 0; down : 1.*/
	if (pch455->bak_status == CH455_STATUS_UP) {
		if (status == CH455_STATUS_UP)
			goto ch455_sleep;
		/* step 1 */
		pan_key_received_count++;

		if (g_ch455_key_map.phy_code != 2) {
			key_index = ch455_key_mapping(code);
			if (key_index >= 0) {
				pan_key_mapped_count++;
				input_report_key(ch455_input,
				key_index, KEY_PRESSED);
				input_sync(ch455_input);

				PANEL_PRINTK("[%s],key (0x%08x)\n",
				__func__, (unsigned int)code);

			} else {
				PANEL_ERR_PRINTK("No code=0x%08x\n",
				(unsigned int)code);
			}
		} else {
			last_repeat_tick = jiffies;
			ali_ch455_send_msg(code,
			IR_TYPE_UNDEFINE, KEY_PRESSED);
		}

		pch455->key_cnt++;
		pch455->bak_status = status;
		pch455->keypress_cnt = 0;
	} else {
		/*pch455->bak_status == CH455_STATUS_DOWN*/

		if (status == CH455_STATUS_UP) {
			/* step 2 */
			pch455->key_cnt = 1;
			pch455->bak_status = status;

			if (g_ch455_key_map.phy_code != 2) {
				key_index = ch455_key_mapping(code);
				if (key_index >= 0) {
					input_report_key(ch455_input,
					key_index, KEY_RELEASE);
					input_sync(ch455_input);

					PANEL_PRINTK("key(0x%08x)\n",
					(unsigned int)code);

				} else {
					PANEL_ERR_PRINTK("key(%08x)\n",
					(unsigned int)code);

				}
			} else {
				ch455_repeat_width_rfk =
				ch455_repeat_delay_rfk;
				ali_ch455_send_msg(code,
				IR_TYPE_UNDEFINE, KEY_RELEASE);
			}
		} else {
			/* step 2, repeat */
			if (g_ch455_key_map.phy_code == 2) {
				repeat_tick = jiffies;
				last_repeat_width =
				(unsigned long)
				(repeat_tick - last_repeat_tick);
				if (last_repeat_width >
					ch455_repeat_width_rfk) {
					last_repeat_tick = repeat_tick;
					ch455_repeat_width_rfk =
					ch455_repeat_interval_rfk;
					ali_ch455_send_msg(code,
					IR_TYPE_UNDEFINE, KEY_REPEAT);
				}
			}
		}
	}

ch455_sleep:

	if ((re == SUCCESS) && (mode_flag == 1)) {
		ch455_mode_set(pch455);
		mode_flag = 0;

		for (i = 0; i < PAN_MAX_LED_NUM; i++) {
			re = pch455->write8(pch455,
				g_display_backup[i][0],
				g_display_backup[i][1]);
			if (re != SUCCESS)
				PANEL_ERR_PRINTK("[ %s %d ]re= %d\n",
				__func__, __LINE__, re);
		}
	}
	/* patch for FD650 */
	if ((re == SUCCESS) && ((data & 0x80) == 0) &&
		((data & 0x04) == 1)) {
		sleep_flag = 0;

		count++;
		/* at least 3s */
		if (count >= (3000/TIMER_DELAY)) {
			count = 0;

			ch455_mode_set(pch455);
			for (i = 0; i < PAN_MAX_LED_NUM; i++) {
				re = pch455->write8(pch455,
				g_display_backup[i][0],
				g_display_backup[i][1]);
				if (re != SUCCESS)
					PANEL_PRINTK("[%s %d],re=%d\n",
					__func__, __LINE__, re);
			}
		}
	} else if (re != SUCCESS) {
		if (sleep_flag == 1) {
			/* sleep here to reduce cpu usage */
            delay += 5000;
		} else {
			count_fail++;
			if (count_fail >= 2) {
				count_fail = 0;
				sleep_flag = 1;
			}
		}
	}
    schedule_delayed_work(&pch455->worker, delay * HZ / 1000);
}

static u8 ch455_bitmap(struct ch455_device *pch455, u8 c)
{
	u32 len = 0;
	struct led_bitmap node;
	u8 bitmap = 0;
	u32 i = 0;


	if (!pch455)
		return -EFAULT;

	len = pch455->bitmap_len;

	for (i = 0; i < len; i++) {
		node = pch455->bitmap_list[i];
		if (node.character == c) {
			bitmap = (u8)node.bitmap;
			break;
		}
	}

	if (i == len) {
		PANEL_ERR_PRINTK("ch455_bitmap()=>Character not found.\n");
		return 0;
	}

	return bitmap;
}


static ssize_t ch455_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct ch455_device *pch455 = file->private_data;
	u8 pdata = 0;
	u8 bitmap, temp, lbd_tmp, ldb_tmp2;
	u32 re;
	u8 flag = 0;
	char *data;
	u8 addr[4] = {g_dig_addr0, g_dig_addr1, g_dig_addr2, g_dig_addr3};
	s32 i = 0, num = -1;

	if (!pch455)
		return -EFAULT;

	mutex_lock(&ch455_lock);


	data = kmalloc(count, GFP_KERNEL);
	memset(data, '0', count);
	if (!data) {
		PANEL_ERR_PRINTK("[ %s %d ], error\n", __func__, __LINE__);
		mutex_unlock(&ch455_lock);
		return -ENOMEM;
	}

	/* read list of keychords from userspace */
	if (copy_from_user(data, buffer, count)) {
		kfree(data);
		PANEL_ERR_PRINTK("[ %s %d ], error\n", __func__, __LINE__);
		mutex_unlock(&ch455_lock);
		return -EFAULT;
	}

	if (data[pdata] == 27) {			/* ESC command */
		switch (data[pdata + 1]) {
		case PAN_ESC_CMD_LBD:
			temp = data[pdata + 2];
			if (temp == PAN_ESC_CMD_LBD_FUNCA
			|| temp == PAN_ESC_CMD_LBD_FUNCB
			|| temp == PAN_ESC_CMD_LBD_FUNCC
			|| temp == PAN_ESC_CMD_LBD_FUNCD) {
			u8 update_led = 0;

			if (data[pdata + 3] == PAN_ESC_CMD_LBD_ON) {
				lbd_tmp = 1<<(temp-PAN_ESC_CMD_LBD_FUNCA);
				ldb_tmp2 = pch455->lbd_func_flag;
				update_led = (ldb_tmp2 & lbd_tmp) ? 0:1;
				pch455->lbd_func_flag |= lbd_tmp;
			} else {
				lbd_tmp = 1<<(temp-PAN_ESC_CMD_LBD_FUNCA);
				ldb_tmp2 = pch455->lbd_func_flag;
				update_led = (ldb_tmp2 & lbd_tmp) ? 1:0;
				pch455->lbd_func_flag &= ~lbd_tmp;
			}

			if (update_led) {
				u8 da = pch455->cur_bitmap[temp];

				if (data[pdata + 3] == PAN_ESC_CMD_LBD_ON)
					da |= 0x80;
				re = pch455->write8(pch455, addr[temp], da);
				if (re != SUCCESS) {
					kfree(data);
					PANEL_ERR_PRINTK("CH455:failed\n");
					mutex_unlock(&ch455_lock);
					return -EFAULT;
				}
				lbd_tmp = temp-PAN_ESC_CMD_LBD_FUNCA;
				g_display_backup[lbd_tmp][1] = da;
			}
		}
		break;

		default:
			break;
		}
	} else {
		if (count > PAN_MAX_LED_NUM) {
			for (i = 0; i < count; i++) {
				if (data[i] == ':' || data[i] == '.') {
					if (num >= 0)
						flag |= 1<<num;
				} else {
					num++;
					data[num] = data[i];
				}
			}
			count = PAN_MAX_LED_NUM;
		}
		while (pdata < count) {
			temp = data[pdata];
			bitmap = ch455_bitmap(pch455, temp);
			pch455->cur_bitmap[pdata] = bitmap;

			if (flag & (1<<pdata))
				bitmap |= 0x80;
			else {
				temp = pch455->lbd_func_flag & (1<<pdata);
				bitmap |= temp ? 0x80 : 0;
			}
			re = pch455->write8(pch455, addr[pdata], bitmap);
			if (re != SUCCESS) {
				kfree(data);
				PANEL_ERR_PRINTK("CH455:failed=%d\n", re);
				mutex_unlock(&ch455_lock);
				return -EFAULT;
			}

			g_display_backup[pdata][1] = bitmap;
			pdata++;
		}
	}

	kfree(data);
	mutex_unlock(&ch455_lock);
	return 0;
}


static int ch455_open(struct inode *inode, struct file *file)
{
	file->private_data = g_pch455;

	ali_ch455_dev.ch455_kumsgq = ali_new_kumsgq();
	if (!ali_ch455_dev.ch455_kumsgq)
		goto out0;
    schedule_delayed_work(&g_pch455->worker, TIMER_DELAY * HZ / 1000);
	return 0;
out0:
	WARN(1, "False to new ir kumsgq!!!!!!");
	return -EFAULT;
}


static int ch455_release(struct inode *inode, struct file *file)
{
	ali_destroy_kumsgq(ali_ch455_dev.ch455_kumsgq);
	ali_ch455_dev.ch455_kumsgq = NULL;
	return 0;
}


static const struct file_operations ch455_fops = {
	.owner		= THIS_MODULE,
	.open		= ch455_open,
	.release	= ch455_release,
	.write		= ch455_write,
	.unlocked_ioctl = ch455_ioctl,


};

static struct miscdevice ch455_misc = {
	.fops		= &ch455_fops,
	.name		= CH455_DEV_NAME,
	.minor		= MISC_DYNAMIC_MINOR,
};


static int ch455_init(void)
{
	int i = 0;
	int ret = SUCCESS;


	mutex_init(&ali_ch455_dev.ch455_mutex);
	g_pch455 = kzalloc(sizeof(struct ch455_device), GFP_KERNEL);
	if (!g_pch455) {
		PANEL_ERR_PRINTK("[ %s %d ], error\n", __func__, __LINE__);
		mutex_unlock(&ch455_lock);
		return -ENOMEM;
	}

	/*Set i2c type id.*/
	/*use gpio i2c default.*/
	ch455_i2c_info = 0x00000100 | ch455_i2c_info;
	g_pch455->i2c_id = ch455_i2c_info & 0xff;
	g_pch455->gpio_i2c = (u8)((ch455_i2c_info & 0xff00) >> 8);
	/*Config CH455 working mode.*/
	g_pch455->mode = CH455_MODE;

	/*Set bit mask for keyboard scan.*/
	g_pch455->mask_status = CH455_KEY_STATUS_MASK;
	g_pch455->mask_row = CH455_KEY_ROW_MASK;
	g_pch455->mask_column = CH455_KEY_COLUMN_MASK;
	g_pch455->key_cnt = 1;
	g_pch455->keypress_cnt = 0;

	/*Set repeat key interval to 300 ms.*/
	g_pch455->keypress_intv = 3;
	g_pch455->keypress_bak = PAN_KEY_INVALID;

	/*Set back status to up.*/
	g_pch455->bak_status = CH455_STATUS_UP;
	g_pch455->bitmap_len = PAN_CH455_CHAR_LIST_NUM;
	g_pch455->bitmap_list = &(bitmap_table[0]);

	memset(&g_ch455_key_map, 0x00, sizeof(g_ch455_key_map));
	/* use default key map*/
	g_ch455_key_map.phy_code = 3;

	for (i = 0; i < ARRAY_SIZE(ali_linux_pan_key_map); i++)
		__set_bit(ali_linux_pan_key_map[i].key%KEY_CNT,
					ch455_input->keybit);


	g_pch455->read8 = read8;
	g_pch455->write8 = write8;
	mutex_init(&g_pch455->lock);

	ch455_mode_set(g_pch455);
    INIT_DELAYED_WORK(&g_pch455->worker, ch455_worker);
	for (i = 0; i < PAN_MAX_LED_NUM; i++)
		g_pch455->cur_bitmap[i] = ch455_bitmap(g_pch455, ' ');

	g_dig_addr0 = 0x68;
	g_dig_addr1 = 0x6a;
	g_dig_addr2 = 0x6c;
	g_dig_addr3 = 0x6e;

	g_display_backup[0][0] = g_dig_addr0;
	g_display_backup[1][0] = g_dig_addr1;
	g_display_backup[2][0] = g_dig_addr2;
	g_display_backup[3][0] = g_dig_addr3;

	return ret;
}


static int ch455_uninit(void)
{
	int ret = SUCCESS;

	if (!g_pch455)
		return -EFAULT;

    cancel_delayed_work_sync(&g_pch455->worker);

	kfree(g_pch455);
	g_pch455 = NULL;

	return ret;
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ch455_suspend(struct device *pdev)
{
    cancel_delayed_work_sync(&g_pch455->worker);
	return 0;
}

static int ch455_resume(struct device *pdev)
{
	int i = 0;
	int re = 0;

	ch455_mode_set(g_pch455);
	for (i = 0; i < PAN_MAX_LED_NUM; i++) {
		re = g_pch455->write8(g_pch455, g_display_backup[i][0],
				g_display_backup[i][1]);
		if (re != SUCCESS)
			PANEL_ERR_PRINTK("CH455:Display failed.re = %d.\n", re);
	}

	schedule_delayed_work(&g_pch455->worker, TIMER_DELAY * HZ / 1000);
	return 0;
}

static const struct dev_pm_ops ch455_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ch455_suspend, ch455_resume)
};
#endif

static int ch455_probe(struct platform_device *pdev)
{
	struct device_node *panel_np = NULL;
        struct device_node *adapter_np = NULL;
        struct i2c_adapter *adapter = NULL;

        panel_np = pdev->dev.of_node;

        adapter_np = of_parse_phandle(panel_np, "i2c-parent", 0);
        if (!adapter_np) {
                pr_err("%s, Cannot parse i2c-parent\n",__FUNCTION__);
                return -EINVAL;
        }

        adapter = of_find_i2c_adapter_by_node(adapter_np);
        of_node_put(adapter_np);
        if (!adapter) {
                pr_err("%s, Cannot get i2c adapter\n",__FUNCTION__);
                return -EINVAL;
        }

        ch455_i2c_info = adapter->nr;
        pr_info("%s, get i2c adapter, id %d\n",__FUNCTION__, adapter->nr);
        put_device(&adapter->dev);
		
	return 0;
}

static int ch455_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id ali_panel_match[] = {
	{ .compatible = "alitech,panel" },
	{},
};

MODULE_DEVICE_TABLE(of, ali_panel_match);

static struct platform_driver ch455_driver = {
	.probe		= ch455_probe,
	.remove		= ch455_remove,
	.driver		= {
		.name	= CH455_DEV_NAME,
		.owner	= THIS_MODULE,
	#ifdef CONFIG_ALI_STANDBY_TO_RAM
		.pm	= &ch455_pm_ops,
	#else
		.pm	= NULL,
	#endif
		.of_match_table	= of_match_ptr(ali_panel_match),
	},
};

static int ali_m36_ch455_init(void)
{
	int ret = 0;

	mutex_init(&ch455_lock);

	ret = misc_register(&ch455_misc);
	if (ret != 0) {
		PANEL_ERR_PRINTK(KERN_ERR "CH455:(err=%d)\n", ret);
		goto fail_misc;
	}

	ch455_input = input_allocate_device();
	if (!ch455_input) {
		PANEL_ERR_PRINTK(KERN_ERR "CH455: not enough memory\n");
		ret = -ENOMEM;
		goto fail_input_alloc;
	}

	ch455_input->name = CH455_DEV_INPUT_NAME;
	ch455_input->phys = CH455_DEV_INPUT_NAME;
	/*BUS_I2C;*/
	ch455_input->id.bustype = BUS_HOST;
	ch455_input->id.vendor = 0x0001;
	ch455_input->id.product = 0x0003;
	ch455_input->id.version = 0x0100;
	ch455_input->evbit[0] = BIT_MASK(EV_KEY);
	ch455_input->evbit[0] |= BIT_MASK(EV_REP);
	ret = input_register_device(ch455_input);
	if (ret)
		goto fail_input_reg;

	ch455_input->rep[REP_DELAY] = REPEAT_DELAY;
	ch455_input->rep[REP_PERIOD] = REPEAT_INTERVAL;

	memset((void *)(&g_ch455_key_map), 0, sizeof(g_ch455_key_map));
	proc_create("panel", 0, NULL, &proc_panel_fops);

	if (platform_driver_register(&ch455_driver) != 0)
		PANEL_ERR_PRINTK(KERN_ERR "register driver error!!\n");

	ch455_init();
	return 0;


fail_input_reg:		input_free_device(ch455_input);
			ch455_input = NULL;
fail_input_alloc:	misc_deregister(&ch455_misc);
fail_misc:
	mutex_unlock(&ch455_lock);

	return ret;
}


static void __exit ali_m36_ch455_exit(void)
{
	ch455_uninit();

	input_unregister_device(ch455_input);
	input_free_device(ch455_input);
	ch455_input = NULL;

	misc_deregister(&ch455_misc);

	kfree(g_ch455_key_map.map_entry);
	g_ch455_key_map.map_entry = NULL;


	if (proc_ch455)
		remove_proc_entry("panel", NULL);

	platform_driver_unregister(&ch455_driver);
	mutex_destroy(&ch455_lock);
}


module_init(ali_m36_ch455_init);
module_exit(ali_m36_ch455_exit);

MODULE_AUTHOR("Mao Feng");
MODULE_DESCRIPTION("ALi CH455 panel driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

