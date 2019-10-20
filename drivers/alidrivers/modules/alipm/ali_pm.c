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
 *  File: ali_pm.c
 *  (I)
 *  Description: ALi power management implementation
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2011.03.07				Owen			Creation
 ****************************************************************************/
#include <linux/platform_device.h>
#include <linux/of.h>
#include <ali_soc.h>
#include "ali_pm.h"
//===========================================================================================================//

extern void ali_suspend_register_ops(void);
extern void ali_suspend_set_resume_key(pm_key_t *pm_key);
extern void ali_suspend_set_standby_param(pm_param_t *p_standby_param);
//===========================================================================================================//

static int ali_pm_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ali_pm_release(struct inode *inode, struct file *file)
{
	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long ali_pm_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
#else
static int ali_pm_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long parg)
#endif			            
{
	unsigned int temp = 0, ret = 0;
	pm_key_t pm_resume_key;
	pm_key_t *p_key = (pm_key_t *)parg;
	pm_param_t pm_standby_param;
	pm_param_t *p_param = (pm_param_t *)parg;

	temp = PM_CMD_SET_RESUME_KEY;
	printk(" PM_CMD_SET_RESUME_KEY:%x\n",temp);

	temp = PM_CMD_SET_STANDBY_PARAM;
	printk(" PM_CMD_SET_STANDBY_PARAM:%x\n",temp);

	printk("ali_pm_ioctl cmd:%x\n",cmd);
	switch (cmd)
	{
	#ifdef CONFIG_ALI_STANDBY_TO_RAM
		case PM_CMD_SET_RESUME_KEY:        
			printk("ali_pm_ioctl PM_CMD_SET_RESUME_KEY\n");

			if(NULL == p_key)
			{
				return -EINVAL;
			}
			get_user(pm_resume_key.standby_key, &p_key->standby_key);
			ret = copy_from_user(pm_resume_key.ir_power, p_key->ir_power, 8*sizeof(unsigned long));
			if(0 != ret)
			{
				printk("PM_CMD_SET_RESUME_KEY copy_from_user() failed, ret:%d\n", ret);
				return(-EINVAL);
			}
			ali_suspend_set_resume_key(&pm_resume_key);

			break;

		case PM_CMD_SET_STANDBY_PARAM:
			printk("ali_pm_ioctl PM_CMD_SET_STANDBY_PARAM\n");

			if(NULL == p_key)
			{
				return -EINVAL;
			}
			get_user(pm_standby_param.board_power_gpio, &p_param->board_power_gpio);
			get_user(pm_standby_param.timeout, &p_param->timeout);
			get_user(pm_standby_param.reboot, &p_param->reboot);
			ali_suspend_set_standby_param(&pm_standby_param);
			break;

		case PM_CMD_SET_STR_CMAC_PROTECTION_SIZE:
			printk("ali_pm_ioctl PM_CMD_SET_STR_CMAC_PROTECTION_SIZE, %ld.\n", parg);
			set_default_area_size(parg);
			break;

		case PM_CMD_CALC_STR_CMAC:
			printk("ali_pm_ioctl PM_CMD_CALC_STR_CMAC.\n");
			calc_aes_cmac();
			break;
	#endif
		default:
			break;
	}

	return 0;
}

static struct file_operations ali_pm_fops = {
	.owner		= THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	.unlocked_ioctl = ali_pm_ioctl,
#else
	.ioctl		= ali_pm_ioctl,
#endif	
	.open		= ali_pm_open,
	.release	= ali_pm_release,
};

static struct pm_device pm_dev = {
	.dev_name = "ali_pm",
};

static int ali_pm_probe(struct platform_device * pdev)
{
	int ret = 0;

	ret = of_get_major_minor(pdev->dev.of_node,&pm_dev.dev_no, 
			0, 1, pm_dev.dev_name);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}
	pm_dev.pm_class = class_create(THIS_MODULE, "ali_pm_class");
	if(IS_ERR(pm_dev.pm_class))
	{
		ret = PTR_ERR(pm_dev.pm_class);
		goto err0;
	}

	cdev_init(&pm_dev.pm_cdev, &ali_pm_fops);
	ret = cdev_add(&pm_dev.pm_cdev, pm_dev.dev_no, 1);
	if (ret < 0)
	{
		goto err1;
	}

	pm_dev.pm_device_node = device_create(pm_dev.pm_class, NULL, pm_dev.dev_no, &pm_dev, pm_dev.dev_name);
	if(IS_ERR(pm_dev.pm_device_node))
	{
		ret = PTR_ERR(pm_dev.pm_device_node);
		goto err2;
	}
    
	ali_suspend_register_ops();
	ali_pm_dbg_init();
	return 0;

err2:
	cdev_del(&pm_dev.pm_cdev);
err1:
	class_destroy(pm_dev.pm_class);
err0:
	unregister_chrdev_region(pm_dev.dev_no, 1);
	return ret;
}

static int ali_pm_remove(struct platform_device * pdev)
{
	ali_pm_dbg_exit();
	return 0;
}

static const struct of_device_id ali_pm_match[] = {
       { .compatible = "alitech, pm", },
       {},
};

MODULE_DEVICE_TABLE(of, ali_pm_match);

static struct platform_driver ali_pm_platform_driver = {
	.probe   = ali_pm_probe, 
	.remove   = ali_pm_remove,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = "ali_pm",
			.of_match_table = ali_pm_match,
	},
};

static int __init ali_pm_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&ali_pm_platform_driver);

	return ret;
}

static void __exit ali_pm_exit(void)
{
	platform_driver_unregister(&ali_pm_platform_driver);
}


subsys_initcall(ali_pm_init);
module_exit(ali_pm_exit);
 
MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("ALi power management implementation");
MODULE_LICENSE("GPL");
