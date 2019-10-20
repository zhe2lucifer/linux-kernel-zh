
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/ali_reg.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>

#include <ali_soc.h>
#include <adf_vbi.h>

#include <ali_interrupt.h>
#include <ali_cache.h>
#include <ali_shm.h>
#include <linux/miscdevice.h>
#include <rpc_hld/ali_rpc_hld_vbi.h>


static bool vbi_open_flag = FALSE;
static VBI_SOURCE_TYPE vbi_type=TTX_VBI_TYPE;

static void set_vbi_type(VBI_SOURCE_TYPE type)
{
	vbi_type=type;
}
static bool get_vbi_type(void)
{
	return vbi_type;
}

static void set_vbi_open_flag(bool flag)
{
	vbi_open_flag=flag;
}
static bool get_vbi_open_flag(void)
{
	return vbi_open_flag;
}

static int ali_vbi_open(struct inode *inode, struct file *file)
{

	return 0;
}

static int  ali_vbi_release(struct inode *inode, struct file *file)
{
	set_vbi_open_flag(FALSE);
	vbi_see_stop();
	return 0;
	
}

static int ali_vbi_write(struct file *  filp, const char __user *arg, size_t size, loff_t * ppos)
{
		
	int ret = 0;		
	
	
	struct vbi_data_array_t ttx_data;

	memset(&ttx_data,0,sizeof(struct vbi_data_array_t));
	
	if(size!=46)
	{	
		printk("%s:%d\n",__FUNCTION__,__LINE__);
		return 1;
	}
	
	if (copy_from_user(&ttx_data, arg, size))  
	{
		printk("%s:%d\n",__FUNCTION__,__LINE__);
		return 1;
	}
	
   	if(get_vbi_open_flag()==FALSE)
   	{
   		printk("%s:%d\n",__FUNCTION__,__LINE__);
   		vbi_see_start(TTX_VBI_TYPE);
		set_vbi_type(TTX_VBI_TYPE);		
		set_vbi_open_flag(TRUE);
	}
	if(get_vbi_type()!=TTX_VBI_TYPE&&(get_vbi_open_flag()==TRUE))
	{
		printk("%s:%d\n",__FUNCTION__,__LINE__);
   		vbi_see_start(TTX_VBI_TYPE);
		set_vbi_type(TTX_VBI_TYPE);		
		set_vbi_open_flag(TRUE);
	}	
		
	ret=write_ttx_packet(&ttx_data,size);

	set_vbi_open_flag(TRUE);

	//printk("%s:ret=%d\n",__FUNCTION__,ret);	
	return ret;

}

static int alivbi_hld_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = 0;  

    switch(cmd)
    {
   
        case IO_VBI_SET_VPO_HD_SD_PARAM:
		{
			ret = ttx_vbi_ioctl(IO_VBI_SET_VPO_HD_SD_PARAM, arg);
            break;
		}
		case IO_VBI_SELECT_OUTPUT_DEVICE:
		{
			ret = ttx_vbi_ioctl(IO_VBI_SELECT_OUTPUT_DEVICE, arg);
            break;
		}
        case IO_VBI_CHECK_TTX_TASK_START:
        {
			//printk("%s-cmd=0x%x\n",__FUNCTION__,cmd);
            ret = ttx_vbi_ioctl(IO_VBI_CHECK_TTX_TASK_START, arg);
			break;
        }
        case IO_VBI_SET_CC_TYPE:
        {
			//printk("%s-cmd=0x%x\n",__FUNCTION__,cmd);
            ret = ttx_vbi_ioctl(IO_VBI_SET_CC_TYPE, arg);
			break;
        }
        case IO_VBI_GET_CC_TYPE:
        {
			//printk("%s-cmd=0x%x\n",__FUNCTION__,cmd);
            ret = ttx_vbi_ioctl(IO_VBI_GET_CC_TYPE, arg);
			break;
        }
		default:
            break;
	}
			
	return ret;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long ali_vbi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int ali_vbi_ioctl(struct inode *node,struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
		
	int ret = 0;

	switch(cmd)
	{
		case IO_VBI_SET_VPO_HD_SD_PARAM:		
		case IO_VBI_CHECK_TTX_TASK_START:
		case IO_VBI_SELECT_OUTPUT_DEVICE:
		case IO_VBI_SET_CC_TYPE:
		case IO_VBI_GET_CC_TYPE:
		{
			//printk("%s-cmd=0x%x\n",__FUNCTION__,cmd);
            ret = alivbi_hld_ioctl(cmd, arg);
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }
		case IO_VBI_SET_SOURCE_TYPE:
			printk("%s-cmd=0x%x\n",__FUNCTION__,cmd);
			set_vbi_open_flag(TRUE);
			vbi_see_start(arg);			
			set_vbi_type(arg);	
			break;
		
		case IO_VBI_TTX_STOP:			
			set_vbi_open_flag(FALSE);
			vbi_see_stop();			
			break;
		default:
			break;
		
	}
	return ret;

}




static const struct file_operations vbi_fops = {
	.owner		= THIS_MODULE,
	.open		= ali_vbi_open,
	.write		= ali_vbi_write,	
	.release	= ali_vbi_release,	
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	.unlocked_ioctl = ali_vbi_ioctl,
#else
	.ioctl = ali_vbi_ioctl,
#endif
};

static struct miscdevice vbi_misc = {
	.fops		= &vbi_fops,
	.name		= "ali_vbi",
	.minor		= MISC_DYNAMIC_MINOR,
};

static int __init ali_m36_vbi_init(void)
{
	int ret = 0;	

	ret = misc_register(&vbi_misc);
	if (ret != 0) {
		printk(KERN_ERR "VBI: cannot register miscdev(err=%d)\n", ret);
		goto fail_misc;
	}
	
fail_misc:

	return ret;
}

static void __exit ali_m36_vbi_exit(void)
{
	misc_deregister(&vbi_misc);
}


module_init(ali_m36_vbi_init);
module_exit(ali_m36_vbi_exit);

MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("Framebuffer driver for ali's chipset");
MODULE_LICENSE("GPL");
