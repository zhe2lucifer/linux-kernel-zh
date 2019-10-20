/****************************************************************************(I)(S)
 *  File: ali_img.c
 *  (I)
 *  Description: image configure(main cpu) for displaying(see cpu)
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	==========        		==========		=======
 * 0.0		2016.11.23				Steve.Chen		Create
 ****************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/ali_cache.h>
#include <alidefinition/adf_image.h>
#include "ali_img.h"

MODULE_LICENSE("Dual BSD/GPL");

typedef struct aliimg_dev_s
{
	int open_state;
	int imgdisp_config_flag;
	char *imgbuffer_y_ptr;
    char *imgbuffer_c_ptr;
	struct mp_imgdisp_info imgdisp_info;
} aliimg_dev_t;

static aliimg_dev_t *g_aliimg_dev_ptr;

#define MP_IMG_DISP_MPEG2_Y_MAXLEN (1920*1088+1024)
#define MP_IMG_DISP_MPEG2_C_MAXLEN ((1920*1088)/2+1024)

#define UNCACHE_ADDR(addr)  ((void *)((((UINT32)(addr))&0x1FFFFFFF)|0xa0000000))

int aliimg_open(struct inode *inode, struct file *filp)
{
	if (g_aliimg_dev_ptr->open_state == 0)
	{
        printk("kenel:aliimg open!\n");
        g_aliimg_dev_ptr->imgbuffer_y_ptr = kmalloc(MP_IMG_DISP_MPEG2_Y_MAXLEN, GFP_KERNEL);
        g_aliimg_dev_ptr->imgbuffer_c_ptr = kmalloc(MP_IMG_DISP_MPEG2_C_MAXLEN, GFP_KERNEL);
        if((!g_aliimg_dev_ptr->imgbuffer_y_ptr) || (!g_aliimg_dev_ptr->imgbuffer_c_ptr))
        {
            printk("kmalloc failly\n");
            return -ENOMEM;
        }
        memset(g_aliimg_dev_ptr->imgbuffer_y_ptr, 0, MP_IMG_DISP_MPEG2_Y_MAXLEN);
        memset(g_aliimg_dev_ptr->imgbuffer_c_ptr, 0, MP_IMG_DISP_MPEG2_C_MAXLEN);
		g_aliimg_dev_ptr->open_state = 1;
		return 0;
	}
	printk("aliimg has been opened!\n");
	return -1;
}

int aliimg_release(struct inode *inode, struct file *filp)
{
	if (g_aliimg_dev_ptr->open_state == 1)
	{
        printk("kenel:aliimg release!\n");
        kfree(g_aliimg_dev_ptr->imgbuffer_y_ptr);
        kfree(g_aliimg_dev_ptr->imgbuffer_c_ptr);
        g_aliimg_dev_ptr->imgbuffer_y_ptr = NULL;
        g_aliimg_dev_ptr->imgbuffer_c_ptr = NULL;
		g_aliimg_dev_ptr->open_state = 0;
		return 0;
	}
	printk("aliimg has not been opened yet!\n");
	return -1;
}

static ssize_t aliimg_read(struct file *filp, char __user *buf, size_t count, loff_t *fpos)
{
	int num = 0;

	//num = copy_to_user(buf, g_aliimg_dev_ptr->imgbuffer_ptr, count);

	if(0 != num)
		printk("aliimg read failly\n");

	return 0;
}

static ssize_t aliimg_write(struct file *filp,const char __user *buf, size_t count, loff_t *fpos)
{
	int num = 0;

	//num = copy_from_user(g_aliimg_dev_ptr->imgbuffer_y_ptr, buf, count);

	if(0 != num)
		printk("aliimg write failly\n");

	return 0;
}

long aliimg_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct mp_imginfo imginfo;
	//unsigned long __G_ALI_MM_VIDEO_START_ADDR = 0xA70B2000;

	if (_IOC_TYPE(cmd) != MP_IMG_DISP_IOC_MAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) > MP_IMG_DISP_IOC_MAXNR)
		return -EINVAL;

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EINVAL;

	copy_from_user(&imginfo,(struct mp_imginfo *)arg,sizeof(struct mp_imginfo));

	switch (cmd)
	{
	case MP_IMG_DISP_CONFIG:
        if(0 == g_aliimg_dev_ptr->imgdisp_config_flag)
        {
    		g_aliimg_dev_ptr->imgdisp_config_flag = 1;
    		g_aliimg_dev_ptr->imgdisp_info.width  = imginfo.width;
    		g_aliimg_dev_ptr->imgdisp_info.height = imginfo.height;
    		g_aliimg_dev_ptr->imgdisp_info.stride = imginfo.stride;
    		g_aliimg_dev_ptr->imgdisp_info.sample_format = imginfo.sample_format;
            g_aliimg_dev_ptr->imgdisp_info.disp_layer    = imginfo.disp_layer;
            g_aliimg_dev_ptr->imgdisp_info.disp_buffer   = imginfo.disp_buffer;

    		g_aliimg_dev_ptr->imgdisp_info.src_rect.u_start_x = imginfo.src_rect.u_start_x;
    		g_aliimg_dev_ptr->imgdisp_info.src_rect.u_start_y = imginfo.src_rect.u_start_y;
    		g_aliimg_dev_ptr->imgdisp_info.src_rect.u_width   = imginfo.src_rect.u_width;
    		g_aliimg_dev_ptr->imgdisp_info.src_rect.u_height  = imginfo.src_rect.u_height;

    		g_aliimg_dev_ptr->imgdisp_info.dst_rect.u_start_x = imginfo.dst_rect.u_start_x;
    		g_aliimg_dev_ptr->imgdisp_info.dst_rect.u_start_y = imginfo.dst_rect.u_start_y;
    		g_aliimg_dev_ptr->imgdisp_info.dst_rect.u_width   = imginfo.dst_rect.u_width;
    		g_aliimg_dev_ptr->imgdisp_info.dst_rect.u_height  = imginfo.dst_rect.u_height;
        }
        else
        {
            printk("kernel:aliimg has been configured image infomation\n");
        }
		break;
	case MP_IMG_DISP_FILL_Y_DATA:
		if((imginfo.buffer_y_len <= MP_IMG_DISP_MPEG2_Y_MAXLEN) && (1 == g_aliimg_dev_ptr->imgdisp_config_flag))
		{
            g_aliimg_dev_ptr->imgdisp_config_flag = 2;
            copy_from_user(g_aliimg_dev_ptr->imgbuffer_y_ptr, imginfo.buffer_y_addr, imginfo.buffer_y_len);
    		g_aliimg_dev_ptr->imgdisp_info.imgbuf.image_y_addr     = UNCACHE_ADDR(g_aliimg_dev_ptr->imgbuffer_y_ptr);
            g_aliimg_dev_ptr->imgdisp_info.imgbuf.image_y_addr_len = imginfo.buffer_y_len;
            //printk("kernel: image infomation v %x p %x\n", g_aliimg_dev_ptr->imgbuffer_y_ptr, UNCACHE_ADDR(g_aliimg_dev_ptr->imgbuffer_y_ptr));
		}
		else
		{
			printk("kernel: no configure image infomation or imginfo.buffer_y_len out of boundary\n");
		}
		break;
	case MP_IMG_DISP_FILL_C_DATA:
		if((imginfo.buffer_c_len <= MP_IMG_DISP_MPEG2_C_MAXLEN) && (2 == g_aliimg_dev_ptr->imgdisp_config_flag))
		{
            g_aliimg_dev_ptr->imgdisp_config_flag = 3;
            copy_from_user(g_aliimg_dev_ptr->imgbuffer_c_ptr, imginfo.buffer_c_addr, imginfo.buffer_c_len);
    		g_aliimg_dev_ptr->imgdisp_info.imgbuf.image_c_addr     = UNCACHE_ADDR(g_aliimg_dev_ptr->imgbuffer_c_ptr);
            g_aliimg_dev_ptr->imgdisp_info.imgbuf.image_c_addr_len = imginfo.buffer_c_len;
            //printk("kernel: image infomation v %x p %x\n", g_aliimg_dev_ptr->imgbuffer_c_ptr, UNCACHE_ADDR(g_aliimg_dev_ptr->imgbuffer_c_ptr));
		}
		else
		{
			printk("kernel: no MP_IMG_DISP_FILL_Y_DATA or imginfo.buffer_c_len out of boundary\n");
		}
		break;
	case MP_IMG_DISP_RUN:
		if(3 == g_aliimg_dev_ptr->imgdisp_config_flag)
		{
            g_aliimg_dev_ptr->imgdisp_config_flag = 0;
            __CACHE_FLUSH_ALI((unsigned long)(g_aliimg_dev_ptr->imgbuffer_y_ptr), g_aliimg_dev_ptr->imgdisp_info.imgbuf.image_y_addr_len);
            __CACHE_FLUSH_ALI((unsigned long)(g_aliimg_dev_ptr->imgbuffer_c_ptr), g_aliimg_dev_ptr->imgdisp_info.imgbuf.image_c_addr_len);
			mp_image_display(NULL, &g_aliimg_dev_ptr->imgdisp_info);
		}
		else
		{
			printk("kernel:no configure image infomation\n");
		}
		break;
	default:
		break;
	}

	return 0;
}

struct file_operations fops_aliimg =
{
	.owner          = THIS_MODULE,
	.open           = aliimg_open,
	.write          = aliimg_write,
	.read           = aliimg_read,
	.release        = aliimg_release,
	.unlocked_ioctl = aliimg_ioctl,
};

struct miscdevice dev_aliimg =
{
	.minor    = MISC_DYNAMIC_MINOR,
	.fops     = &fops_aliimg,
	.name     = "aliimg",
	.nodename = "ali_img"
};

static int __init aliimg_init(void)
{
	int ret = 0;

	g_aliimg_dev_ptr = kmalloc(sizeof(aliimg_dev_t),GFP_KERNEL);
	if(!g_aliimg_dev_ptr)
	{
		return -ENOMEM;
	}
	memset(g_aliimg_dev_ptr, 0, sizeof(aliimg_dev_t));

	ret = misc_register(&dev_aliimg);
	if(!ret)
		return 0;
	else
		return ret;
}

static void __exit aliimg_exit(void)
{
	printk(KERN_ALERT"aliimg exit!\n");
	kfree(g_aliimg_dev_ptr);

	misc_deregister(&dev_aliimg);
}

module_init(aliimg_init);
module_exit(aliimg_exit);

