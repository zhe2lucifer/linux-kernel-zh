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
#include <linux/kthread.h>
#include <ali_front_panel_common.h>
#include <linux/ali_transport.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include "ali_pan_ch454.h"

#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <ali_front_panel_common.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/ali_kumsgq.h>


#define ALI_PANEL_INFO	"ALi Panel(CH454) Driver"

#define CH454_SYSTEM_ADDR	0x48
#define CH454_SYSTEM_DATA	0x43

#define CH454_DIS_ADDR	0x4a
#define CH454_DIS_DATA	0x00

#define CH454_KEY_ADDR	0x4f


/* bit31-bit0 : 00LK NMJH G2G1FE DCBA */
#define CH454_K_MASK	0x1000	/* bit12 */
#define CH454_L_MASK	0x2000	/* bit13 */
#define CH454_DP_MASK	0xC000	/* bit14 bit15 */

#define CH454_KEY_STATUS_MASK	0x40
#define CH454_KEY_ROW_MASK	0x38
#define CH454_KEY_COLUMN_MASK	0x07

#define CH454_STATUS_UP		0
#define CH454_STATUS_DOWN		1

/* ESC command: 27 (ESC code), PAN_ESC_CMD_xx (CMD type), param1, param2 */
#define PAN_ESC_CMD_LBD			'L'		/* LBD operate command */
#define PAN_ESC_CMD_LBD_FUNCA	0		/* Extend function LBD A */
#define PAN_ESC_CMD_LBD_FUNCB	1		/* Extend function LBD B */
#define PAN_ESC_CMD_LBD_FUNCC	2		/* Extend function LBD C */
#define PAN_ESC_CMD_LBD_FUNCD	3		/* Extend function LBD D */
#define PAN_ESC_CMD_LBD_LEVEL		5		/* Level status LBD, no used */

#define PAN_ESC_CMD_LED			'E'		/* LED operate command */
#define PAN_ESC_CMD_LED_LEVEL		0		/* Level status LED */

#define PAN_ESC_CMD_LBD_ON		1		/* Set LBD to turn on status */
#define PAN_ESC_CMD_LBD_OFF		0		/* Set LBD to turn off status */

#define PAN_MAX_LED_NUM			4

#define PAN_MAX_CHAR_LIST_NUM		70


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

#ifdef ARRAY_SIZE
	#undef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#else
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

struct led_bitmap
{
	u8 character;
	u16 bitmap;
};

enum value_key_press
{
	KEY_RELEASE		= 0,
	KEY_PRESSED		= 1,
	KEY_REPEAT		= 2 
};

#define PAN_KEY_INVALID			0xFFFFFFFF
#define IR_TYPE_UNDEFINE 9
//cara.shi    2016/8/1
struct ali_ch454_com_device_data
{
	//struct	cdev cdev;
	struct kumsgq *ch454_kumsgq;
	struct mutex ch454_mutex;
	//void    *priv;
};

struct ali_ch454_com_device_data ali_ch454_dev;

static struct led_bitmap bitmap_table[PAN_MAX_CHAR_LIST_NUM * 2] = {
	{'0', 0x3f}, {'1', 0x06}, {'2', 0x5b | 0x80}, {'3', 0x4f | 0x80},
	{'4', 0x66 | 0x80}, {'5', 0x6d | 0x80}, {'6', 0x7d | 0x80}, {'7', 0x07},
	{'8', 0x7f | 0x80}, {'9', 0x6f | 0x80}, {'a', 0x5f | 0x80}, {'A', 0x1806  | 0x80},
	{'b', 0x7c | 0x80}, {'B', 0x7c | 0x80}, {'c', 0xd8}, {'C', 0x39},
	{'d', 0x5e | 0x80}, {'D', 0x5e | 0x80}, {'e', 0x7b | 0x80}, {'E', 0x79 | 0x80},
	{'f', 0x71 | 0x80}, {'F', 0x71 | 0x80}, {'g', 0x6f | 0x80}, {'G', 0xbd},
	{'h', 0x74 | 0x80}, {'H', 0x76 | 0x80}, {'i', 0x600}, {'I', 0x609},
	{'j', 0x1e}, {'J', 0x1e},  {'k', 0x3070}, {'K', 0x3070},
	{'l', 0x38}, {'L', 0x38}, {'m', 0x4d4}, {'M', 0x1536},
	{'n', 0x54 | 0x80}, {'N', 0x2136}, {'o', 0x5c | 0x80}, {'O', 0x3f},
	{'p', 0x73 | 0x80}, {'P', 0x73 | 0x80}, {'q', 0x67 | 0x80}, {'Q', 0x67 | 0x80},
	{'r', 0x2073 | 0x80}, {'R', 0x2073 | 0x80}, {'s', 0x6d | 0x80}, {'S', 0x6d | 0x80},
	{'t', 0x601}, {'T', 0x601}, {'u', 0x1c}, {'U', 0x3e},
	{'v', 0x2106}, {'V', 0x2106}, {'w', 0x2814}, {'W', 0x2a36},
	{'x', 0x3900}, {'X', 0x3900}, {'y', 0xee}, {'Y', 0x1500},
	{'z', 0x1809}, {'Z', 0x1809}, {':', 0xc000}, {'.', 0xc000},
	/*{'_', 0x08}, */{' ', 0x00},
};

#define PAN_CH454_CHAR_LIST_NUM sizeof(bitmap_table)/sizeof(struct led_bitmap)
static u8 g_display_backup[(PAN_MAX_LED_NUM + 1) * 2][(PAN_MAX_LED_NUM + 1) * 2] = 
{
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
	{0, 0}, 
};

/*
ali_ch454_key_map.code :
row       : bit0-bit2
column : bit3-bit5
*/
static const struct ali_fp_key_map_t ali_linux_pan_key_map[] =
{	
	{(0xffff0031),		KEY_FN},
	{(0xffff0030),		KEY_POWER},

};

static struct ali_fp_key_map_cfg g_ch454_key_map;



struct input_dev *ch454_input;

struct ch454_device {
	struct timer_list timer;
	struct mutex lock;
	u32	i2c_id;
	u8 	gpio_i2c;
	u8	system_param;	
	u8	dis_param;
	struct task_struct *thread_id;
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

	u8 	lbd_func_flag;

	u32	(*read8)(struct ch454_device *pch454,  u8 dig_addr, u8 *data);
	u32	(*write8)(struct ch454_device *pch454, u8 dig_addr, u8 data);
    u8	cur_bitmap[PAN_MAX_LED_NUM];
};


static unsigned long pan_key_received_count = 0;
static unsigned long pan_key_mapped_count = 0;
static unsigned char pan_debug = 0;
static unsigned long ch454_repeat_delay_rfk = 600;
static unsigned long ch454_repeat_interval_rfk = 350;
static unsigned long ch454_repeat_width_rfk = 0;
static unsigned int ch454_rfk_port = 0;
static struct proc_dir_entry *proc_ch454;
/* 	ch454_i2c_info :
	byte0, I2C ID; 
	byte1, 1 gpio_i2c, 0 i2c 
	byte2, reserve; 
	byte3, reserve; 
*/
static unsigned int  ch454_i2c_info = 0;//use 4th i2c id as default panel config


#define PANEL_PRINTK(fmt, args...)			\
{										\
	if (0 !=  pan_debug)					\
	{									\
		printk(fmt, ##args);					\
	}									\
}

#define PANEL_ERR_PRINTK(fmt, args...)		\
{										\
	printk(fmt, ##args);						\
}


int ali_i2c_scb_write(u32 id, u8 slv_addr, u8 *data, int len);
int ali_i2c_scb_read(u32 id, u8 slv_addr, u8 *data, int len);
static int  ch454_mode_set(struct ch454_device *pch454);
static unsigned char                g_dig0_addr = 0x50;
static unsigned char                g_dig1_addr = 0x52;
 static unsigned char               g_dig2_addr = 0x54;
static unsigned char                g_dig3_addr = 0x56;
static unsigned char                g_dig4_addr = 0x58;

static unsigned char                g_dig0_high_addr = 0x60;
static unsigned char                g_dig1_high_addr = 0x62;
static unsigned char                g_dig2_high_addr = 0x64;
static unsigned char                g_dig3_high_addr = 0x66;
static unsigned char                g_dig4_high_addr = 0x68;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
#include <linux/seq_file.h>
static int ch454_read_proc(struct seq_file *m, void *v)
{
	seq_printf(m, "%s", CH454_DEV_NAME);
	return 0;
}

static int proc_panel_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, ch454_read_proc, PDE_DATA(inode), 256);
}


static const struct file_operations proc_panel_fops = {
	.open		= proc_panel_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#else
static int ch454_read_proc (char *buffer, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;    

	
	if (off > 0)
	{
		return 0;
	}
	len = sprintf(buffer, "%s", CH454_DEV_NAME);
	*start = buffer;	
	*eof = 1;

        	return len;
}
#endif

static int ali_ch454_send_msg(unsigned long  code, int ir_protocol, int ir_state)
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
	
	PANEL_PRINTK("[ %s %d ], rfk port %d send msg :\n", __FUNCTION__, __LINE__, ch454_rfk_port);
	for (i=0; i<sizeof(msg); i++)
	{
		PANEL_PRINTK("%02x ", msg[i]);
	}
	PANEL_PRINTK("\n");	
	
	ret = ali_kumsgq_sendmsg(ali_ch454_dev.ch454_kumsgq, &msg, sizeof(msg));
	if (-1 == ret)
	{
		PANEL_ERR_PRINTK("[ %s %d ], rfk port %d send msg fail!\n", __FUNCTION__, __LINE__, ch454_rfk_port);
	}

	
	return ret;
}


#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
static long ch454_ioctl(struct file * file, unsigned int cmd, unsigned long param)
#else
static int ch454_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long param)
#endif
{
	unsigned long reg[2] = {0, 0};	
	struct ch454_device *pch454 = file->private_data;	
	
	
	PANEL_PRINTK("[ %s ]: cmd %d\n", __FUNCTION__, cmd);	
	switch (cmd)
	{
		case PAN_DRIVER_READ_REG: 			
			if (0 != (copy_from_user(&reg, (void*)param, sizeof(reg))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}
            reg[1]=ioread32(reg[0]);	
			if (0 != copy_to_user((void*)param, &reg, sizeof(reg)))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
        				return -EFAULT;
			}
			
			PANEL_PRINTK("[ %s %d ]: read 0x%08x = 0x%08x\n\n", 
				__FUNCTION__,  __LINE__, (unsigned int)reg[0], (unsigned int)reg[1]);					
			
			break;		

		case PAN_DRIVER_WRITE_REG: 				
			if (0 != (copy_from_user(&reg, (void*)param, sizeof(reg))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}					
			iowrite32(reg[1],reg[0]);			
			PANEL_PRINTK("[ %s %d ]: write 0x%08x = 0x%08x\n", 
				__FUNCTION__,  __LINE__, (unsigned int)reg[0], (unsigned int)( reg[1]));							
			
			break;
			
		case ALI_FP_CONFIG_KEY_MAP:
		{
			unsigned char * map_entry;
			struct ali_fp_key_map_cfg key_map;
			int i;
			
			if (0 != (copy_from_user(&key_map, (void*)param, sizeof(key_map))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}			
			
			PANEL_PRINTK("[ %s %d ], phy_code = %ld\n", __FUNCTION__, __LINE__, key_map.phy_code);			
			
			g_ch454_key_map.phy_code = key_map.phy_code;
			if (2 != g_ch454_key_map.phy_code)
			{
				map_entry = kzalloc(key_map.map_len, GFP_KERNEL);
				if(NULL==map_entry)
				{
					PANEL_ERR_PRINTK("[ %s ]: Fail, not enouth memory!\n", __FUNCTION__);
					return -ENOMEM;				
				}
				
				if (0 != (copy_from_user(map_entry, key_map.map_entry, key_map.map_len)))
				{
					PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
					return -EFAULT;
				}				
				key_map.map_entry = map_entry;
				key_map.unit_num = (key_map.map_len/key_map.unit_len);
				if(key_map.unit_num>KEY_CNT)
				{
					key_map.unit_num = KEY_CNT;
				}
				if(g_ch454_key_map.map_entry)
				{
					kfree(g_ch454_key_map.map_entry);
				}
				g_ch454_key_map.map_entry = NULL;
				
				for (i = 0; i < key_map.unit_num; ++i)
				{
					unsigned short key;

					/*
					phy_code : 0, logic; 1, index.
					*/
					if(1 == key_map.phy_code)
					{
						key = (unsigned short)i;						
					}
					else
					{
						key = key_map.map_entry[i*key_map.unit_len+4];
						key |= (key_map.map_entry[i*key_map.unit_len+5])<<8;
					}					
					
					__set_bit(key%KEY_CNT, ch454_input->keybit);
				}
				
				g_ch454_key_map.map_len = key_map.map_len;
				g_ch454_key_map.unit_len = key_map.unit_len;
				g_ch454_key_map.phy_code = key_map.phy_code;
				g_ch454_key_map.unit_num = key_map.unit_num;
				g_ch454_key_map.map_entry = key_map.map_entry;
				if (0 != pan_debug)
				{
					int i;
					PANEL_PRINTK("\n[ %s ]: \n", __FUNCTION__);
					for(i=0; i<g_ch454_key_map.unit_num; i++){
						unsigned char * buf = &map_entry[i*g_ch454_key_map.unit_len];
						unsigned long phy ;
						unsigned short log;
						phy = buf[0]|(buf[1]<<8)|(buf[2]<<16)|(buf[3]<<24);
						log = buf[4]|(buf[5]<<8);
						PANEL_PRINTK("%08x	%04x\n", (int)phy, log);
					}
					PANEL_PRINTK("\n");
				}
			}			
		}
		break;
		
		case ALI_FP_GET_PAN_KEY_RECEIVED_COUNT:
		{										
			if (0 != copy_to_user((void*)param, &pan_key_received_count, sizeof(pan_key_received_count)))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
        				return -EFAULT;
			}	
			
			PANEL_PRINTK("[ %s ]: pan_key_received_count = %ld\n", __FUNCTION__, pan_key_received_count);	
		}
		break;

		case ALI_FP_GET_PAN_KEY_MAPPED_COUNT:
		{									
			if (0 != copy_to_user((void*)param, &pan_key_mapped_count, sizeof(pan_key_mapped_count)))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
        				return -EFAULT;
			}
			
			PANEL_PRINTK("[ %s ]: pan_key_mapped_count = %ld\n", __FUNCTION__, pan_key_mapped_count);	
		}
		break;

		case ALI_FP_SET_PAN_KEY_DEBUG:
		{					
			if (0 != copy_from_user(&pan_debug, (void*)param, sizeof(pan_debug)))			
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
        				return -EFAULT;
			}	
			
			PANEL_PRINTK("[ %s ]: pan_debug = %d\n", __FUNCTION__, pan_debug);			
		}
		break;		

		case ALI_FP_GET_KUMSGQ:
		{
			int flags = -1;
			int ret = -1;
			mutex_lock(&ali_ch454_dev.ch454_mutex);
			if(copy_from_user(&flags, (int *)param, sizeof(int)))
			{
				PANEL_PRINTK("Err: copy_from_user\n");
				mutex_unlock(&ali_ch454_dev.ch454_mutex);
				return -EFAULT;
			}
			ret  = ali_kumsgq_newfd(ali_ch454_dev.ch454_kumsgq, flags);
			if(ret> 0)
			{
				mutex_unlock(&ali_ch454_dev.ch454_mutex);
				return ret;	
			}	
		}
		break;

		case PAN_DRIVER_SET_I2C:
		{				
			if (0 != (copy_from_user(&ch454_i2c_info, (void*)param, sizeof(ch454_i2c_info))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}						
			
			pch454->i2c_id = ch454_i2c_info & 0xff;
			pch454->gpio_i2c = (u8)((ch454_i2c_info & 0xff00) >> 8);
			
			PANEL_PRINTK("[ %s %d ]: ch454_i2c_info = 0x%08x, i2c_id = %d, gpio_i2c = %d\n", 
				__FUNCTION__, __LINE__, ch454_i2c_info, pch454->i2c_id, pch454->gpio_i2c);	
	
			ch454_mode_set(pch454);
			PANEL_PRINTK("[ %s %d ]: i2c_id = %d\n", __FUNCTION__, __LINE__, pch454->i2c_id);	
		}
		break;	

		case PAN_DRIVER_SET_DIG_ADDR:
		{		
			unsigned int dig_addr = 0;
			
			if (0 != (copy_from_user(&dig_addr, (void*)param, sizeof(dig_addr))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}						
			
			g_dig0_addr= dig_addr & 0xff;
			g_dig1_addr = (u8)((dig_addr & 0xff00) >> 8);
			g_dig2_addr = (u8)((dig_addr & 0xff0000) >> 16);
			g_dig3_addr = (u8)((dig_addr & 0xff000000) >> 24);

			g_display_backup[0][0] = g_dig0_addr;
			g_display_backup[1][0] = g_dig1_addr;
			g_display_backup[2][0] = g_dig2_addr;
			g_display_backup[3][0] = g_dig3_addr;
			
			PANEL_PRINTK("[ %s %d ]: dig_addr = 0x%08x[0x%02x 0x%02x 0x%02x 0x%02x]\n", 
				__FUNCTION__, __LINE__, dig_addr, g_dig0_addr, g_dig1_addr,
				g_dig2_addr, g_dig3_addr);					
		}
		break;	

		case PAN_DRIVER_SET_LED_BITMAP:
		{
			struct ali_fp_bit_map_cfg bit_map;
			u32 i;
			
			if (0 != (copy_from_user(&bit_map, (void*)param, sizeof(bit_map))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}			
			
			PANEL_PRINTK("[ %s %d ], map_len = %ld\n", __FUNCTION__, __LINE__, bit_map.map_len);
			if (bit_map.map_len > (PAN_MAX_CHAR_LIST_NUM * 2))
			{
				pch454->bitmap_len = PAN_MAX_CHAR_LIST_NUM * 2;
				PANEL_PRINTK("[ %s %d ], map_len > %d\n", __FUNCTION__, __LINE__, (PAN_MAX_CHAR_LIST_NUM * 2));
			}
			else
			{
				pch454->bitmap_len = bit_map.map_len;
			}
			PANEL_PRINTK("[ %s %d ], bitmap_len = %d\n", __FUNCTION__, __LINE__, pch454->bitmap_len);

			memset(pch454->bitmap_list, 0x00, sizeof(bitmap_table));
			if (0 != (copy_from_user(pch454->bitmap_list, bit_map.map_entry, pch454->bitmap_len)))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}				
			
			if (0 != pan_debug)
			{
				for (i=0; i<pch454->bitmap_len; i++)
				{
					PANEL_PRINTK("'%c'    %02x\n", 						
						pch454->bitmap_list[i].character,pch454->bitmap_list[i].bitmap);					
				}
				PANEL_PRINTK("\n");
			}				
		}
		break;	

		case ALI_FP_SET_REPEAT_INTERVAL:
		{				
			unsigned long rep[2] = {0, 0};
			
			if (0 != (copy_from_user(&rep, (void*)param, sizeof(rep))))
			{
				PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
				return -EFAULT;
			}

			ch454_repeat_delay_rfk = rep[0];
			ch454_repeat_interval_rfk = rep[1];			
			
			PANEL_PRINTK("[ %s ]: ch454_repeat_delay_rfk = %ld\n", __FUNCTION__, ch454_repeat_delay_rfk);	
			PANEL_PRINTK("[ %s ]: ch454_repeat_interval_rfk = %ld\n", __FUNCTION__, ch454_repeat_interval_rfk);				
		}
		break;		
			
		default:
			return -EPERM;
			break;
	}

	return 0;
}


static u32 read8(struct ch454_device *pch454,  u8 dig_addr, u8 *data)
{	
	u32 re = (u32)ERR_FAILURE;		
	unsigned char offset = dig_addr;
	struct i2c_adapter *adapter;
    	struct i2c_msg msgs[] = {   { .addr = dig_addr>>1, .flags = 0, .len = 0, .buf = &offset },
                                { .addr = dig_addr>>1, .flags	= I2C_M_RD,  .len = 1,  .buf = data    } };
	int result = 0;	
	
	
	mutex_lock(&pch454->lock);

	if (1 == pch454->gpio_i2c)
	{
		adapter = i2c_get_adapter(pch454->i2c_id);	
	    	if(adapter)
	    	{				
	        	if((result = i2c_transfer(adapter, msgs, 2)) != 2)
	        	{
		         PANEL_PRINTK("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
				 	__FUNCTION__, __LINE__, result, adapter->name, adapter->nr, adapter->nr, adapter->class);
		         re = (u32)ERR_FAILURE;
	        	}    
			else
			{
				re = (u32)SUCCESS;
			}	
	    	}  
		else
		{
			PANEL_PRINTK("[ %s %d ], adapter is NULL, i2c_id = %d\n", __FUNCTION__, __LINE__, pch454->i2c_id);
		}	
	}
	else
	{
		/*re = (u32)ali_i2c_scb_read(pch455->i2c_id, dig_addr, data, 1);*/
	}
	

	mutex_unlock(&pch454->lock);

	//PANEL_PRINTK("[ %s ], id(%d), addr(0x%02x), data(0x%02x).\n", __FUNCTION__, pch454->i2c_id, dig_addr, *data);

	return re;
}

static u32 write8(struct ch454_device *pch454, u8 dig_addr, u8 data)
{
	u32 re = (u32)ERR_FAILURE;	
	unsigned char buf[10];
    	struct i2c_adapter *adapter;
    	struct i2c_msg msgs = { .addr = dig_addr>>1, .flags = 0, .len = 1,  .buf = buf };
	int result = 0;

	
	
	PANEL_PRINTK("[ %s ], id = %d, addr = 0x%02x, data = 0x%02x.\n", __FUNCTION__, pch454->i2c_id, dig_addr, data);

	mutex_lock(&pch454->lock);

	if (1 == pch454->gpio_i2c)
	{
		buf[0] = data;	
	    	adapter = i2c_get_adapter(pch454->i2c_id);		
	    	if(adapter)
	    	{						
		        	if((result = i2c_transfer(adapter, &msgs, 1)) != 1)
		        	{
		         		PANEL_PRINTK("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
					 	__FUNCTION__, __LINE__, result, adapter->name, adapter->nr, adapter->nr, adapter->class);
		            	re = (u32)ERR_FAILURE;
		        	}    
			else
			{
				re = (u32)SUCCESS;
			}	
		}
		else
		{
			PANEL_PRINTK("[ %s %d ], adapter is NULL, i2c_id = %d\n", __FUNCTION__, __LINE__, pch454->i2c_id);
		}
	}
	else
	{
	/*re = (u32)ali_i2c_scb_write(pch455->i2c_id, dig_addr, &data, 1);*/
	}

	mutex_unlock(&pch454->lock);

	return re;
}


static int  ch454_mode_set(struct ch454_device *pch454) 
{
	int re = ERR_FAILURE;

	
	if(SUCCESS != (re=pch454->write8(pch454, CH454_SYSTEM_ADDR, pch454->system_param)))
	{
		PANEL_PRINTK("[ %s %d ], Failed, re = %d\n", __FUNCTION__, __LINE__, re);
	}	
	
	if(SUCCESS != (re=pch454->write8(pch454, CH454_DIS_ADDR, pch454->dis_param)))
	{
		PANEL_PRINTK("[ %s %d ], Failed, re = %d\n", __FUNCTION__, __LINE__, re);
	}	

	return re;
}


static int ch454_key_default_mapping (unsigned long ir_code)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(ali_linux_pan_key_map); i++) {
		if (ali_linux_pan_key_map[i].code== ir_code) 
		{
			return ali_linux_pan_key_map[i].key;
		}
	}

	return -1;
}



static int ch454_key_mapping(unsigned long phy_code)
{
	int i;
	int logic_code = 0;

	
	if ((NULL==g_ch454_key_map.map_entry) || (3 == g_ch454_key_map.phy_code))
	{		
		return ch454_key_default_mapping(phy_code);		
	}
	
	for(i = 0; i<g_ch454_key_map.unit_num; i++)
	{
		unsigned long my_code;
		
		
		if((unsigned long)(&g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len]) & 0x3)
		{
			my_code = g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len];			
			my_code |= (g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len+1])<<8;			
			my_code |= (g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len+2])<<16;			
			my_code |= (g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len+3])<<24;			
		}
		else
		{
			my_code = *((unsigned long *)(&g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len]));
		}
		
		if(phy_code == my_code)
		{			
			if(1 == g_ch454_key_map.phy_code)
			{				
				PANEL_PRINTK("[ %s ], phy code index %d\n", __FUNCTION__, i);				
				return i;
			}
			else
			{
				logic_code =  g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len+4]; 				
				logic_code |= (g_ch454_key_map.map_entry[i*g_ch454_key_map.unit_len+5])<<8;									
				PANEL_PRINTK("logic code 0x%08x\n", logic_code);				
				
				return logic_code;
			}
		}			
	}

	
	return -1;
}


int ch454_timer(void *param)
{
	struct ch454_device *pch454 = (struct ch454_device *)param;
	u8 row, column, status;	
	u8 data = 0xff;	
	u32 re = SUCCESS;
	s32 mode_flag = 0;
	s32 key_index = 0;
	unsigned long code = 0;	
	static unsigned long last_repeat_tick = 0;	
	unsigned long last_repeat_width;
	unsigned long repeat_tick = 0;
	u8 i = 0;	
	u8 count = 0;	
	u8 count_fail = 0;
	u8 sleep_flag = 0;


	while (!kthread_should_stop())
	{			
		re = pch454->read8(pch454,  CH454_KEY_ADDR, &data);		/* about 1ms */
		if(SUCCESS != re)
		{
			mode_flag = 1;
			PANEL_PRINTK("CH454: Scan keyboard failed!re = %d.\n", re);
			goto ch454_sleep;
		}
		else
		{				
			//bit 7 should always be 0.
			if(((data & 0x80) != 0) || ((data & 0x04) == 0))
			{
				//PANEL_PRINTK("%s()=>Read bad key code!data = 0x%2x.\n", __FUNCTION__, data);
				goto ch454_sleep;
			}
			else
			{
				/* bit7 = 0 */				
				column = data & pch454->mask_column;		//(bit 0~2)
				row = (data & pch454->mask_row) >> 3;	//(bit 3~5)
				status = (data & pch454->mask_status) >> 6;  // bit 6					
				code = 0xFFFF0000 | (unsigned long)((column << 3) | row);				
			}		
		}
	
	
		if (pch454->bak_status == CH454_STATUS_UP)	// up : 0; down : 1.
		{		
			if (status == CH454_STATUS_UP)
			{
				goto ch454_sleep;
			}
			/* step 1 */
			pan_key_received_count++;

			if(2 != g_ch454_key_map.phy_code)
			{
				key_index = ch454_key_mapping(code);
				if (key_index >= 0)
				{
					pan_key_mapped_count++;
					input_report_key(ch454_input, key_index, KEY_PRESSED);
					input_sync(ch454_input);						
									
					PANEL_PRINTK("[ %s %d ], key 0x%08x(0x%08x) pressed.\n", __FUNCTION__, __LINE__, 
						key_index, (unsigned int)code);
								
				}
				else
				{
					PANEL_ERR_PRINTK("[ %s %d ] : Not matched key for panel code = 0x%08x \n", 
						__FUNCTION__, __LINE__, (unsigned int)code);				
				}
			}
			else
			{					
				last_repeat_tick = jiffies;
				ali_ch454_send_msg(code, IR_TYPE_UNDEFINE, KEY_PRESSED);
			}
			
			pch454->key_cnt ++;
			pch454->bak_status = status;
			pch454->keypress_cnt = 0;
		}
		else			//pch454->bak_status == CH454_STATUS_DOWN
		{		 
			if (status == CH454_STATUS_UP)
			{	
				/* step 2 */
				pch454->key_cnt = 1;
				pch454->bak_status = status;		

				if(2 != g_ch454_key_map.phy_code)
				{
					key_index = ch454_key_mapping(code);
					if (key_index >= 0)
					{						
						input_report_key(ch454_input, key_index, KEY_RELEASE);
						input_sync(ch454_input);								
						
						PANEL_PRINTK("[ %s %d ], key 0x%08x(0x%08x) pressed.\n", __FUNCTION__, __LINE__, 
							key_index, (unsigned int)code);					
										
					}	
					else
					{
						PANEL_ERR_PRINTK("[ %s %d ] : Not matched key for panel code = 0x%08x \n", 
							__FUNCTION__, __LINE__, (unsigned int)code);				
					}	
				}
				else
				{
					ch454_repeat_width_rfk = ch454_repeat_delay_rfk;
					ali_ch454_send_msg(code, IR_TYPE_UNDEFINE, KEY_RELEASE);	
				}			
			}
			else
			{	
				/* step 2, repeat */		
				if(2 == g_ch454_key_map.phy_code)
				{				
					repeat_tick = jiffies;					
					last_repeat_width = (unsigned long)(((long)repeat_tick - (long)last_repeat_tick));							
					if (last_repeat_width > ch454_repeat_width_rfk)
					{		
						last_repeat_tick = repeat_tick;
						ch454_repeat_width_rfk = ch454_repeat_interval_rfk;
						ali_ch454_send_msg(code, IR_TYPE_UNDEFINE, KEY_REPEAT);							
					}
				}
			}
		}
			
		ch454_sleep:

		#if 0
		if ((SUCCESS == re) && (1 == mode_flag))
		{
			ch454_mode_set(pch454);		
			mode_flag = 0;
			
			for (i=0; i < ((PAN_MAX_LED_NUM + 1)* 2); i++)
			{
				if (SUCCESS != (re = pch454->write8(pch454, g_display_backup[i][0], g_display_backup[i][1])))
				{
					PANEL_ERR_PRINTK("[ %s %d ], Display LED failed. re = %d.\n", __FUNCTION__, __LINE__, re);					
				}
			}
		}	

		
		if ((SUCCESS == re)&&((data & 0x80) == 0) &&((data & 0x04) == 1))			/* patch for FD650 */
		{
			sleep_flag = 0;
			
			count++;
			if (count >= (3000/TIMER_DELAY))	/* at least 3s */ 	
			{
				count = 0;
				
				ch454_mode_set(pch454);					
				for (i=0; i < ((PAN_MAX_LED_NUM + 1) * 2); i++)
				{
					if (SUCCESS != (re = pch454->write8(pch454, g_display_backup[i][0], g_display_backup[i][1])))
					{
						PANEL_PRINTK("[ %s %d ], Display LED failed. re = %d.\n", __FUNCTION__, __LINE__, re);					
					}
				}			
			}	
		}
		else if(SUCCESS != re)
		{
			if (1 == sleep_flag)
			{					
				ssleep(5);			/* sleep here to reduce cpu usage */
			}
			else
			{
				count_fail++;				
				if (count_fail >= 2)
				{
					count_fail = 0;
					sleep_flag = 1;							
				}
			}			
		}	
		#endif
		
		
		msleep(TIMER_DELAY);		
	}


	return 0;
}

static int ch454_start_timer(struct ch454_device *pch454)
{	
	pch454->thread_id = kthread_create(ch454_timer, (void *)pch454, "ali_ch454");
	if(IS_ERR(pch454->thread_id)){
		PANEL_ERR_PRINTK("ch454 kthread create fail\n");
		pch454->thread_id = NULL;
		return -1;
	}
	wake_up_process(pch454->thread_id);

	return 0;
}

static u16 ch454_bitmap(struct ch454_device *pch454, u8 c)
{
	u32 len = pch454->bitmap_len;
	struct led_bitmap node;
	u16 bitmap = 0;
	u32 i = 0;

	for (i=0; i<len; i++)
	{
		node = pch454->bitmap_list[i];
		if (node.character == c)
		{
			bitmap = (u16)node.bitmap;
			break;
		}
	}

	if(i == len)
	{
		PANEL_ERR_PRINTK("ch454_bitmap()=>Character not found, c = 0x%x.\n", c);
		return 0;
	}

	return bitmap;	
}


static ssize_t ch454_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct ch454_device *pch454 = file->private_data;
	u8 pdata = 0;
	u16 bitmap = 0;
	u16 bitmap_dig4 = 0;
	u8 temp = 0;
	u32 re;
	u8 dp_flag=0;
	char *data;
	u8 addr[4]={g_dig0_addr, g_dig1_addr, g_dig2_addr, g_dig3_addr};
	u8 high_addr[4]={g_dig0_high_addr, g_dig1_high_addr, g_dig2_high_addr, g_dig3_high_addr};
	s32 i = 0, num = -1;	
	
	data = (char *)kzalloc(count, GFP_KERNEL);
	if (!data)
	{
		PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}	

	/* read list of keychords from userspace */
	if (copy_from_user(data, buffer, count)) {
		kfree(data);
		PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
		return -EFAULT;
	}	

	if (data[pdata] == 27)			/* ESC command */
	{
		switch(data[pdata + 1])
		{
			case PAN_ESC_CMD_LBD:
				temp = data[pdata + 2];
				if(temp == PAN_ESC_CMD_LBD_FUNCA
					||temp == PAN_ESC_CMD_LBD_FUNCB
					||temp == PAN_ESC_CMD_LBD_FUNCC
					||temp == PAN_ESC_CMD_LBD_FUNCD)
				{
					u8 update_led = 0;
					if(data[pdata + 3] == PAN_ESC_CMD_LBD_ON)
					{
						update_led = (pch454->lbd_func_flag & (1<<(temp-PAN_ESC_CMD_LBD_FUNCA))) ? 0:1;
						pch454->lbd_func_flag |= 1<<(temp-PAN_ESC_CMD_LBD_FUNCA);
					}
					else
					{
						update_led = (pch454->lbd_func_flag & (1<<(temp-PAN_ESC_CMD_LBD_FUNCA))) ? 1:0;
						pch454->lbd_func_flag &= ~(1<<(temp-PAN_ESC_CMD_LBD_FUNCA));
					}
                    
					if(update_led)
					{
						u8 da=pch454->cur_bitmap[temp];
						if(data[pdata + 3] == PAN_ESC_CMD_LBD_ON)
						{
							da |= 0x80; 
						}
						
						if (SUCCESS != (re = pch454->write8(pch454, addr[temp], da)))
						{
								kfree(data);
								PANEL_ERR_PRINTK("ch454_write()=>Display LED failed. re = %d.\n", re);
								return -EFAULT;
						}
						g_display_backup[temp-PAN_ESC_CMD_LBD_FUNCA][1] = da;
					}
				}
				break;

			default:
				break;
		}
	}
	else
	{		
		if(count>PAN_MAX_LED_NUM)
		{			
			for(i=0; i<count; i++)
			{
				if(data[i] == ':' || data[i] == '.')
				{					
					dp_flag = 0x03;		/* high DIG4 bit0 and bit1 */
				}
				else
				{
					num++;
					data[num]=data[i];
				}
			}
			
			count = PAN_MAX_LED_NUM;			
		}
		
		while (pdata < count)
		{
			temp = data[pdata];
			bitmap = ch454_bitmap(pch454, temp);
			pch454->cur_bitmap[pdata] = bitmap;

			

			PANEL_PRINTK("[ %s %d ], bitmap = 0x%x\n", __func__, __LINE__, bitmap);
			re = pch454->write8(pch454, addr[pdata], (u8)(bitmap & 0x00ff));
			re |= pch454->write8(pch454, high_addr[pdata], (u8)((bitmap & ~(CH454_K_MASK | CH454_L_MASK | CH454_DP_MASK)) >> 8));
			
			if (0 != (bitmap & CH454_K_MASK))	/* Kn, bit12 */
			{
				bitmap_dig4 |= 1 << pdata;
				PANEL_PRINTK("[ %s %d ], bitmap_dig4 = 0x%x, pdata = %d\n",
					__func__, __LINE__, bitmap_dig4, pdata);
			}
			
			if (0 != (bitmap & CH454_L_MASK))	/* Ln, bit13 */
			{
				bitmap_dig4 |= 0x10 << pdata;
				PANEL_PRINTK("[ %s %d ], bitmap_dig4 = 0x%x, pdata = %d\n",
					__func__, __LINE__, bitmap_dig4, pdata);
			}	
			
			re |= pch454->write8(pch454, g_dig4_addr, bitmap_dig4);	/* set Kn and Ln of low DIG4*/			
			re |= pch454->write8(pch454, g_dig4_high_addr, dp_flag);	/* set DP of high DIG4 */				
			if (SUCCESS != re)
			{
				kfree(data);
				PANEL_ERR_PRINTK("[ %s %d ], Display LED failed. re = %d.\n", __FUNCTION__, __LINE__, re);
				return -EFAULT;
			}
			
			g_display_backup[pdata][1] = (u8)(bitmap & 0x00ff);
			g_display_backup[pdata + 5][1] = (u8)((bitmap & ~(CH454_K_MASK | CH454_L_MASK | CH454_DP_MASK)) >> 8);
			pdata ++;
		}		
	}		
	

	kfree(data);	

	
	return 0;
}

static int g_pch454_started = 0;
struct ch454_device *g_pch454;
static int ch454_open(struct inode *inode, struct file *file)
{
	int i = 0;
	int ret = SUCCESS;

	ali_ch454_dev.ch454_kumsgq = ali_new_kumsgq();
	if (!ali_ch454_dev.ch454_kumsgq)
	{
		goto out0;
	}

	if(0 == g_pch454_started)
	{
		g_pch454 = kzalloc(sizeof(struct ch454_device), GFP_KERNEL);
		if (!g_pch454)
		{
			PANEL_ERR_PRINTK("[ %s %d ], error\n", __FUNCTION__, __LINE__);
			return -ENOMEM;
		}
		g_pch454_started = 1;
	}
	
	//Set i2c type id.
	ch454_i2c_info = 0x00000100 | ch454_i2c_info; // use gpio i2c default.
	g_pch454->i2c_id = ch454_i2c_info & 0xff; 
	g_pch454->gpio_i2c = (u8)((ch454_i2c_info & 0xff00) >> 8);
	//Config CH454 working mode.
	g_pch454->system_param= CH454_SYSTEM_DATA;
	g_pch454->dis_param= CH454_DIS_DATA;
	
	//Set bit mask for keyboard scan.
	g_pch454->mask_status = CH454_KEY_STATUS_MASK;
	g_pch454->mask_row = CH454_KEY_ROW_MASK;
	g_pch454->mask_column = CH454_KEY_COLUMN_MASK;
	g_pch454->key_cnt = 1;
	g_pch454->keypress_cnt = 0;
	
	//Set repeat key interval to 300 ms.
	g_pch454->keypress_intv = 3;
	g_pch454->keypress_bak = PAN_KEY_INVALID;
	
	//Set back status to up.
 	g_pch454->bak_status = CH454_STATUS_UP; 
	g_pch454->bitmap_len = PAN_CH454_CHAR_LIST_NUM;
	g_pch454->bitmap_list = &(bitmap_table[0]);
	
	memset(&g_ch454_key_map, 0x00, sizeof(g_ch454_key_map));
	g_ch454_key_map.phy_code = 3;	/* use default key map*/
	
	for (i = 0; i < ARRAY_SIZE(ali_linux_pan_key_map); i++) 
	{			
		__set_bit(ali_linux_pan_key_map[i].key%KEY_CNT, ch454_input->keybit);
	}	
	
	g_pch454->read8 = read8;
	g_pch454->write8 = write8;
	mutex_init(&g_pch454->lock);
	
	ch454_mode_set(g_pch454);		
	file->private_data = g_pch454;
	ch454_start_timer(g_pch454);	
	for(i=0; i<PAN_MAX_LED_NUM; i++)
	{
		g_pch454->cur_bitmap[i] = ch454_bitmap(g_pch454, ' ');
	}	
	
	g_display_backup[0][0] = g_dig0_addr;
	g_display_backup[1][0] = g_dig1_addr;
	g_display_backup[2][0] = g_dig2_addr;
	g_display_backup[3][0] = g_dig3_addr;	
	g_display_backup[4][0] = g_dig4_addr;
	g_display_backup[5][0] = g_dig0_high_addr;
	g_display_backup[6][0] = g_dig1_high_addr;
	g_display_backup[7][0] = g_dig2_high_addr;	
	g_display_backup[8][0] = g_dig3_high_addr;
	g_display_backup[9][0] = g_dig4_high_addr;
	

	return ret;
out0:
	WARN(1,"False to new ch454 kumsgq!!!!!!");
	return -EFAULT;
}


static int ch454_release(struct inode *inode, struct file *file)
{
	ali_destroy_kumsgq(ali_ch454_dev.ch454_kumsgq);
	ali_ch454_dev.ch454_kumsgq = NULL;
	return 0;
}


static const struct file_operations ch454_fops = {
	.owner		= THIS_MODULE,
	.open		= ch454_open,
	.release	= ch454_release,
	.write		= ch454_write,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
	.unlocked_ioctl = ch454_ioctl,
	#else
	.ioctl			= ch454_ioctl,
	#endif
	
};

static struct miscdevice ch454_misc = {
	.fops		= &ch454_fops,
	.name		= CH454_DEV_NAME,
	.minor		= MISC_DYNAMIC_MINOR,
};

#define KUMSG_TEST 1
#ifdef KUMSG_TEST
#include <linux/version.h> 
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/types.h>
#define TEST_BUF "hello ch454" 
static ssize_t test_kumsg_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *len)
{
	char *buf = NULL;
	buf =kmalloc(count+1, GFP_KERNEL);
	memset(buf, '\n', count+1);
	memcpy(buf, buffer, count);
	ali_kumsgq_sendmsg(ali_ch454_dev.ch454_kumsgq, TEST_BUF,sizeof(TEST_BUF));
 	kfree(buf);
	return count;
}
static struct proc_dir_entry *test_dir, *test_file;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static const struct file_operations test_proc_subdir_fops = {
#warning Need to set some I/O handlers here
	.write = test_kumsg_proc_write,
};
#endif

#endif
static int ali_m36_ch454_init(void)
{
	int ret = 0;	

	//PANEL_PRINTK("Welcome, %s\n", ALI_PANEL_INFO);

	mutex_init(&ali_ch454_dev.ch454_mutex);
	ret = misc_register(&ch454_misc);
	if (ret != 0) {
		PANEL_ERR_PRINTK(KERN_ERR "CH454: cannot register miscdev(err=%d)\n", ret);
		goto fail_misc;
	}

	ch454_input = input_allocate_device();
	if (!ch454_input) {
		PANEL_ERR_PRINTK(KERN_ERR "CH454: not enough memory for input device\n");
		ret = -ENOMEM;
		goto fail_input_alloc;
	}

	ch454_input->name = CH454_DEV_INPUT_NAME;
	ch454_input->phys = CH454_DEV_INPUT_NAME;
	ch454_input->id.bustype = BUS_HOST; //BUS_I2C;
	ch454_input->id.vendor = 0x0001;
	ch454_input->id.product = 0x0003;
	ch454_input->id.version = 0x0100;
	ch454_input->evbit[0] = BIT_MASK(EV_KEY);
	ch454_input->evbit[0] |= BIT_MASK(EV_REP);
	ret = input_register_device(ch454_input);
	if (ret)
	{
		goto fail_input_reg;
	}	

	ch454_input->rep[REP_DELAY] = REPEAT_DELAY;
	ch454_input->rep[REP_PERIOD] = REPEAT_INTERVAL;

	memset((void *)(&g_ch454_key_map), 0, sizeof(g_ch454_key_map));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	proc_create("panel", 0, NULL, &proc_panel_fops);
#else
	if (NULL != (proc_ch454 = create_proc_entry( "panel", 0, NULL )))
		proc_ch454->read_proc = ch454_read_proc;	
#endif
#ifdef KUMSG_TEST
	/* create a file */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))	
	test_file = proc_create("test_ch454", 0644, test_dir, &test_proc_subdir_fops);
#else
	test_file = create_proc_entry("test_ch454", 0644, test_dir);
#endif
	if(test_file == NULL) {
	}
#endif
	return 0;

fail_input_reg:		input_free_device(ch454_input);
fail_input_alloc:	misc_deregister(&ch454_misc);
fail_misc:

	return ret;
}


static void __exit ali_m36_ch454_exit(void)
{
	//PANEL_PRINTK("Goodbye, %s\n", ALI_PANEL_INFO);
	
	input_unregister_device(ch454_input);
	input_free_device(ch454_input);
	ch454_input = NULL;
	
	misc_deregister(&ch454_misc);

	if(g_ch454_key_map.map_entry)
	{
		kfree(g_ch454_key_map.map_entry);
	}

	if (proc_ch454)
	{
		remove_proc_entry( "panel", NULL);
	}
	
	//PANEL_PRINTK("OK!\n");
}

module_init(ali_m36_ch454_init);
module_exit(ali_m36_ch454_exit);

MODULE_AUTHOR("John Chen");
MODULE_DESCRIPTION("ALi CH454 panel driver");
MODULE_LICENSE("GPL");

