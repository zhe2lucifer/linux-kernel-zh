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

#include <linux/io.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <ali_uart_io_common.h>
#include <linux/delay.h>
#include <ali_reg.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/slab.h>  /* IBU */
#include <ali_soc.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#if 1
#define ALI_UART_DEBUG(...)	do{}while(0)
#else
#define ALI_UART_DEBUG	printk
#endif

#define MAX_ALI_UART_PARAS 	8

#ifdef CONFIG_ALI_UART_IO
char uart_buffer[SCI_16550UART_RX_BUF_SIZE];
unsigned int uart_header;
unsigned int uart_tail;
unsigned int uart_sci_mode;
#endif

static unsigned int sys_reg_addr ;
static unsigned int uart_reg_addr ;
//static unsigned int chip_id ;

extern int of_get_major_minor(struct device_node *enode, dev_t *dev,
		unsigned baseminor, unsigned count,const char *name);

struct ali_uart_io_dev
{
	struct mutex ioctl_mutex;
	dev_t dev_id;
	struct cdev cdev;
};

struct ali_uart_io_dev g_ali_uart_io_device;


#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
long ali_uart_ioctl(struct file *filp,unsigned int cmd,unsigned long arg);
#else
__s32 ali_uart_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
__s32 ali_uart_open(struct inode *inode, struct file *file);
__s32 ali_uart_release(struct inode *inode, struct file *file);

struct file_operations g_ali_uart_fops = 
{
	.owner = THIS_MODULE,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
	.unlocked_ioctl = ali_uart_ioctl,
	#else
	.ioctl = ali_uart_ioctl,
	#endif
	.open = ali_uart_open,
	.release = ali_uart_release,
};

struct class *g_ali_uart_io_class;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
long ali_uart_ioctl
(	
	struct file	    *filp, 
	unsigned int    cmd,
	unsigned long   arg
)

#else
__s32 ali_uart_ioctl
(
	struct inode    *inode,
	struct file	*filp,
	unsigned int    cmd,
	unsigned long   arg
)
#endif
{
	__s32			ret;
	void __user *argp = (void __user *)arg;	
	unsigned long	paras[MAX_ALI_UART_PARAS];
	ret = 0;

	switch(cmd)
	{
		case ALI_UART_IO_READ:
		{
			unsigned char ch = 0;

			if(!uart_sci_mode) break;
			ALI_UART_DEBUG("[kernel] uart_buffer %d: %d\n", uart_header, uart_tail);
			while(uart_header == uart_tail)
			{
				msleep(2);
			}

			ch = uart_buffer[uart_tail++];
			ALI_UART_DEBUG("[kernel]           tail: %d = %c\n", uart_tail, ch);
			uart_tail %= SCI_16550UART_RX_BUF_SIZE;
			
			ALI_UART_DEBUG("[kernel] uart_header=%d, uart_tail=%d, ch=%d\n", uart_header, uart_tail, ch);
			
			ret = copy_to_user(argp, &ch, sizeof(unsigned char));
			
			ALI_UART_DEBUG("[%s]line=%d, ret=%d, recv character is :%c", __FUNCTION__, __LINE__, ret, ch);
			if (ret)
		  	{
				ALI_UART_DEBUG("[kernel] [%s]line=%d,ali_uart_ioctl copy_from_user fail\n",__FUNCTION__, __LINE__);
				return -EFAULT;
			}
			ALI_UART_DEBUG("[kernel] ALI_UART_IO_READ over\n");
			break;
		}
		case ALI_UART_IO_READ_TIMEOUT:	/* IBU */
		{
			struct uart_pars pars;
			unsigned long	timeout =0;
			
			if(!uart_sci_mode) break;
			ret=copy_from_user(&pars, (void __user *)arg, sizeof(struct uart_pars));
            		if (ret)
            		{
            			ALI_UART_DEBUG("[kernel] [%s]line=%d,ali_uart_ioctl copy_from_user fail\n",__FUNCTION__, __LINE__);
            			return -EFAULT;
           		}
			timeout = pars.tm;
			while((uart_header == uart_tail))
			{
				if (timeout <= 0)
				{
					return -1;
				}
				msleep(2);
				timeout--;
			}
			pars.ch = uart_buffer[uart_tail++];
			uart_tail %= SCI_16550UART_RX_BUF_SIZE;
			
			ret = copy_to_user((void __user*)arg, &pars, sizeof(struct uart_pars));
			ALI_UART_DEBUG("[%s]line=%d, ret=%d, recv character is :%c", __FUNCTION__, __LINE__, ret, pars.ch);
			if (ret)
		  	{
				ALI_UART_DEBUG("[kernel] [%s]line=%d,ali_uart_ioctl copy_from_user fail\n",__FUNCTION__, __LINE__);
				return -EFAULT;
			}
			break;
		
		}
		case ALI_UART_IO_READ_TM: //READ_TM = READ + READ_TIMEOUT
		{			
			struct uart_pars pars;
			unsigned long	timeout =0;
			
			if(!uart_sci_mode) break;
			ret=copy_from_user(&pars, (void __user *)arg, sizeof(struct uart_pars));
            		if (ret)
            		{
            			ALI_UART_DEBUG("[kernel] [%s]line=%d,ali_uart_ioctl copy_from_user fail\n",__FUNCTION__, __LINE__);
                		return -EFAULT;
            		}
			timeout = pars.tm;

			if(0 == timeout)
			{
				while(uart_header == uart_tail)
				{
					msleep(2);
				}
			}
			else
			{
				while (timeout >0)
				{
					if (uart_header == uart_tail)
					{
						msleep(2);
						timeout--;
					}
					else
					{
						break;
					}
				}
				if(timeout <= 0) return -1;
			}
			pars.ch = uart_buffer[uart_tail++];
			uart_tail %= SCI_16550UART_RX_BUF_SIZE;

			ret = copy_to_user((void __user*)arg, &pars, sizeof(struct uart_pars));
			ALI_UART_DEBUG("[%s]line=%d, ret=%d, recv character is :%c", __FUNCTION__, __LINE__, ret, pars.ch);
			if (ret)
			{
				ALI_UART_DEBUG("[kernel] [%s]line=%d,ali_uart_ioctl copy_from_user fail\n",__FUNCTION__, __LINE__);
				return -EFAULT;	
			}
 			break;
		}		
		case ALI_UART_IO_WRITE:
		{
			unsigned char ch = 0;	
			volatile unsigned char status;
			int retry = 100;
			if(!uart_sci_mode) break;
				
			if (0 != copy_from_user(&paras, (void __user *)arg, _IOC_SIZE(cmd)))
			{
				ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
				ALI_UART_DEBUG("[kernel] ali_uart_ioctl copy_from_user fail\n");
				return (-EFAULT);
			}
			ch = (unsigned char)*paras;
			/* Wait Tx empty until 1s*/
			while(retry){
				status = __REG8ALI((uart_reg_addr+0x05));
				if(status & 0x40)
					break;
				msleep(10);
				retry --;
			}
			
			if(0 != retry){
				__REG8ALI((uart_reg_addr+0x00)) = (ch);
				ret = 0;
			}
			else{
				ret = -EFAULT;
			}
			
			break;
		}
		case ALI_UART_IO_SET:
		{
			__REG8ALI((uart_reg_addr+0x01)) = 0x00;
			__REG8ALI((uart_reg_addr+0x03)) = 0x9b;
			__REG8ALI((uart_reg_addr+0x00)) = 0x01;
			__REG8ALI((uart_reg_addr+0x01)) = 0x00;
			__REG8ALI((uart_reg_addr+0x03)) = 0x1b;
			__REG8ALI((uart_reg_addr+0x02)) = 0x47;
			__REG8ALI((uart_reg_addr+0x05)) = 0x00;
			__REG8ALI((uart_reg_addr+0x04)) = 0x03;
			__REG8ALI((uart_reg_addr+0x01)) = 0x05;
			break;
		}
		case ALI_UART_DISABLE:
		{
			if (0 != copy_from_user(&paras, (void __user *)arg, _IOC_SIZE(cmd)))
			{
				ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
				ALI_UART_DEBUG("[kernel] ali_uart_ioctl copy_from_user fail\n");
				return (-EFAULT);
			}
			ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d,board_type=0x%x!\n",__LINE__,__REG32ALI(sys_reg_addr));
			if ( ALI_S3503 == ali_sys_ic_get_chip_id())
			{
				unsigned char idx = (unsigned char)*paras;
				unsigned long data = __REG32ALI((sys_reg_addr+0x88));
				if (1 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data &= ~(1 << 23);
					data &= ~(1 << 12);
					__REG32ALI((sys_reg_addr + 0x88)) = data;
				}
				else if (2 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data &= ~(1 << 14);	// uart2 tx disabled by pinmux
					data &= ~(1 << 13);
					data &= ~(1 << 24); // uart2 tx disabled by pinmux
					data &= ~(1 << 21); 
					__REG32ALI((sys_reg_addr + 0x88)) = data;
				}
			}
			else if(ALI_S3821 == ali_sys_ic_get_chip_id())
			{
				unsigned char idx = (unsigned char)*paras;
				unsigned long data = __REG32ALI((sys_reg_addr+0x488));
				if (1 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data &= ~(1 << 1);
					data &= ~(1 << 2);
					__REG32ALI((sys_reg_addr + 0x488)) = data;
				}
				else if (2 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data &= ~(1 << 7);	// uart2 tx disabled by pinmux
					data &= ~(1 << 6);
					__REG32ALI((sys_reg_addr + 0x488)) = data;
				}
			}
			break;
		}
		case ALI_UART_ENABLE:
		{
			if (0 != copy_from_user(&paras, (void __user *)arg, _IOC_SIZE(cmd)))
			{
				ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
				ALI_UART_DEBUG("[kernel] ali_uart_ioctl copy_from_user fail\n");
				return (-EFAULT);
			}
			ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d,board_type=0x%x!\n",__LINE__,__REG32ALI(sys_reg_addr));
			if ( ALI_S3503 == ali_sys_ic_get_chip_id())
			{
				unsigned char idx = (unsigned char)*paras;
				unsigned long data = __REG32ALI((sys_reg_addr+0x88));
				if (1 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data |= (1 << 23);
					data |= (1 << 12);
					__REG32ALI((sys_reg_addr+0x88)) = data;
				}
				else if (2 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data |= (1 << 14);	// uart2 tx disabled by pinmux
					data |= (1 << 13);
					data |= (1 << 24);  // uart2 tx disabled by pinmux
					data |= (1 << 21); 
					__REG32ALI((sys_reg_addr+0x88)) = data;
				}
			}
			else if(ALI_S3821 == ali_sys_ic_get_chip_id())
			{
				unsigned char idx = (unsigned char)*paras;
				unsigned long data = __REG32ALI((sys_reg_addr+0x488));
				if (1 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data |= (1 << 1);
					data |= (1 << 2);
					__REG32ALI((sys_reg_addr+0x488)) = data;
				}
				else if (2 == idx)
				{
					ALI_UART_DEBUG("[kernel] ali_uart_ioctl line=%d!\n",__LINE__);
					data |= (1 << 7);	// uart2 tx disabled by pinmux
					data |= (1 << 6);
					__REG32ALI((sys_reg_addr+0x488)) = data;
				}
			}
			break;
		}
		case ALI_UART_IO_CLEAR_SCI:
		{
			uart_header = uart_tail = 0;
      			uart_sci_mode = 0;
			ALI_UART_DEBUG("\nClose UART SCI mode:\n");
			break;
		}
		case ALI_UART_IO_SET_SCI:
		{
			uart_header = uart_tail = 0;			
			uart_sci_mode = 1;
			ALI_UART_DEBUG("\nOpen UART SCI mode:\n");
			break;
		}
        case ALI_UART_IO_SET_BACKLOOP:
		{   volatile unsigned char mode;
            mode= __REG8ALI((uart_reg_addr+0x04));
            mode |= (1 << 4);
            __REG8ALI((uart_reg_addr+0x04)) = mode;
			ALI_UART_DEBUG("\n set backloop mode:\n");
			break;
		}
        case ALI_UART_IO_CLOSE_BACKLOOP:
		{   volatile unsigned char mode;
            mode= __REG8ALI((uart_reg_addr+0x04));
            mode &= ~(1 << 4);
            __REG8ALI((uart_reg_addr+0x04)) = mode;
			ALI_UART_DEBUG("\n close backloop mode:\n");
			break;
		}
		default:
		{
			break;
		}		
	}	
	return ret;
}

__s32 ali_uart_open
(
	struct inode *inode,
	struct file *file
)
{
	ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
	return (0);
}

__s32 ali_uart_release
(
	struct inode *inode,
	struct file *file
)
{
	int				ret;
	ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
	uart_header = uart_tail = 0;
	uart_sci_mode = 0;
	ret = 0;
	return (ret);
}

static int ali_uart_io_probe(struct platform_device *pdev)
{
	int 			result = 0;
	struct device			*clsdev;
	struct ali_uart_io_dev	*uart;

	sys_reg_addr  = 0x18000000 ;
	uart_reg_addr = 0x18018300 ;
	
	ALI_UART_DEBUG("sys_reg_addr = 0x%08x\n",sys_reg_addr);
	ALI_UART_DEBUG("uart_reg_addr = 0x%08x\n",uart_reg_addr);
	
	ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
		
	uart = &g_ali_uart_io_device;

	mutex_init(&uart->ioctl_mutex);

	result = of_get_major_minor(pdev->dev.of_node,&uart->dev_id, 
			0, 1, "ali_uart_io");
	if (result  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return result;
	}

	ALI_UART_DEBUG("%s, dev_id:%d.\n", __FUNCTION__, uart->dev_id);

	cdev_init(&(uart->cdev), &g_ali_uart_fops);
	uart->cdev.owner = THIS_MODULE;
	result = cdev_add(&uart->cdev, uart->dev_id, 1);
	if (result)
	{
		ALI_UART_DEBUG("cdev_add() failed, result:%d\n", result);
		goto fail;
	}

	g_ali_uart_io_class = class_create(THIS_MODULE, "ali_uart_io_class");
	if (IS_ERR(g_ali_uart_io_class))
	{
		result = PTR_ERR(g_ali_uart_io_class);
		goto fail;
	}
	ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);

	clsdev = device_create(g_ali_uart_io_class, NULL, uart->dev_id,
		uart, "ali_uart_io");
	if (IS_ERR(clsdev))
	{
		ALI_UART_DEBUG(KERN_ERR "device_create() failed!\n");
		result = PTR_ERR(clsdev);
		goto fail;
	}

	ALI_UART_DEBUG("%s, %d.\n", __FUNCTION__, __LINE__);
	return (0);
fail:
	return (-1);
}

static int  ali_uart_io_remove(struct platform_device *pdev)
{
	ALI_UART_DEBUG("%s\n", __FUNCTION__);
	mutex_destroy(&g_ali_uart_io_device.ioctl_mutex);
    cdev_del(&g_ali_uart_io_device.cdev);
	if (NULL != g_ali_uart_io_class)
    {
        class_destroy(g_ali_uart_io_class);
        g_ali_uart_io_class = NULL;
    }

    unregister_chrdev_region(g_ali_uart_io_device.dev_id, 1);

	return 0;
}

static const struct of_device_id ali_uart_io_of_match[] = {
       { .compatible = "alitech, uart_io", },
       {},
};
MODULE_DEVICE_TABLE(of, ali_uart_io_of_match);

static struct platform_driver ali_uart_io_driver = {
	.driver		= {
		.name	= "ali_uart_io",
		.owner	= THIS_MODULE,
		.of_match_table = ali_uart_io_of_match,
	},
	.probe		= ali_uart_io_probe,
	.remove		= ali_uart_io_remove,
};

module_platform_driver(ali_uart_io_driver);
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI uart io driver");
MODULE_LICENSE("GPL");
