/*
 * Key Ladder Core driver
 * Copyright(C) 2014 ALi Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/splice.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/dma-mapping.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/clk-provider.h>
#include <ali_soc.h>

#include <ca_kl.h>
#include <ca_otp_dts.h>
#include "ca_kl_priv.h"
#include "ca_kl_ioctl.h"
#ifdef CONFIG_ALI_VSC
#include "ca_kl_vsc_ioctl.h"
#endif
#include "ca_kl_sysfs.h"
#include "ca_kl_dbgfs.h"
#include "ca_kl_rpc.h"

static struct class *g_kl_class;

static int ca_kl_open(struct inode *inode, struct file *file);
static int ca_kl_release(struct inode *inode, struct file *file);

const static struct file_operations ca_kl_fops = {
	.owner		= THIS_MODULE,
	.open		= ca_kl_open,
	.release	= ca_kl_release,
	.unlocked_ioctl	= ca_kl_ioctl,
};
#ifdef CONFIG_ALI_VSC
const static struct file_operations ca_kl_vsc_fops = {
	.owner		= THIS_MODULE,
	.open		= ca_kl_open,
	.release	= ca_kl_release,
	.unlocked_ioctl	= ca_kl_vsc_ioctl,
};
#endif

static int fetch_kl_key_cell(struct file *file,
	struct kl_key_cell **cell)
{
	struct ca_kl_session *s = NULL;

	s =  file2session(file);
	if (!s)
		return -EBADF;

	if (!cell) {
		dev_dbg(s->pdev->dev, "cell NULL\n");
		return -EINVAL;
	}

	if (!s->cell || !s->cell->valid) {
		dev_dbg(s->pdev->dev, "session cell not ready\n");
		return -EAGAIN;
	}

	*cell = s->cell;
	return 0;
}

static int ca_kl_open(struct inode *inode, struct file *file)
{
	struct ca_kl_session *session = NULL;
	struct ca_kl_sub_dev *son = NULL;

	son = container_of(inode->i_cdev, struct ca_kl_sub_dev, cdev);
	if (!son)
		return -ENODEV;

	session = devm_kzalloc(son->dev, sizeof(struct ca_kl_session),
		GFP_KERNEL);
	if (!session)
		return -ENOMEM;

	mutex_lock(&son->parent->mutex);
	son->sess_num++;
	mutex_unlock(&son->parent->mutex);

	mutex_init(&session->mutex);
	session->pdev = son;
	file->private_data = (void *)session;

	session->id = ida_simple_get(&son->s_ida,
		0, 0, GFP_KERNEL);

	ca_kl_dbgfs_add_session(session);
	return 0;
}

static int ca_kl_release(struct inode *inode, struct file *file)
{
	struct ca_kl_session *session;
	struct ca_kl_sub_dev *son;

	session = file2session(file);
	if (!session)
		return -EBADF;

	son = session->pdev;

	if (son->debug_mode)
		return 0;

	mutex_lock(&session->mutex);
#ifdef CONFIG_DEBUG_FS
	ca_kl_dbgfs_del_session(session);
#endif

	/*put key cell*/
	if (session->cell)
		put_key_cell(session->cell);

	mutex_unlock(&session->mutex);
	mutex_destroy(&session->mutex);

	ida_simple_remove(&son->s_ida, session->id);

	devm_kfree(son->dev, session);
	file->private_data = NULL;

	mutex_lock(&son->parent->mutex);
	son->sess_num--;
	mutex_unlock(&son->parent->mutex);

	return 0;
}

static int ca_kl_probe_sub_dev_dt(struct ca_kl_sub_dev *son,
	struct device_node *node, int type)
{
	int ret, len;
	const char *val;

	struct of_ali_otp_val otp_vals[] = {
		{"level-sel-otp",	NULL},
		{"level-5-enable-otp", NULL},
	};

	ret = of_property_read_u32(node,
		"dev-index", &son->dev_index);
	if (ret)
		goto errout;

	ret = of_property_read_u32(node,
		"root-key-addr", &son->root_key_addr);
	if (ret)
		goto errout;
	
	#ifdef CONFIG_ALI_VSC_SMI
	son->type = KL_TYPE_ETSI;
	#endif
	/* set kl index at see side*/
	switch (son->root_key_addr) {
	case 0x4d:
		son->kl_index = (son->type == KL_TYPE_ETSI) ?
			ETSI_INDEX_0 : KL_INDEX_0;
		break;

	case 0x51:
		son->kl_index = (son->type == KL_TYPE_ETSI) ?
			ETSI_INDEX_1 : KL_INDEX_1;
		break;

	case 0x55:
		son->kl_index = (son->type == KL_TYPE_ETSI) ?
			ETSI_INDEX_2 : KL_INDEX_2;
		break;

	case 0x59:
		son->kl_index = (son->type == KL_TYPE_ETSI) ?
			ETSI_INDEX_3 : KL_INDEX_3;
		son->is_hdcp = 1;
		break;

	case 0x60:
		if (son->parent->one_kl_one_engine_mode) {
			son->kl_index = (son->type == KL_TYPE_ETSI) ?
				ETSI_INDEX_4 : KL_INDEX_4;
		} else {
			son->kl_index = (son->type == KL_TYPE_ETSI) ?
				ETSI_INDEX_2 : KL_INDEX_2;
		}
		break;

	case 0x64:
		son->kl_index = (son->type == KL_TYPE_ETSI) ?
			ETSI_INDEX_3 : KL_INDEX_3;
		break;

	default:
		dev_dbg(&son->parent->clnt->dev,
			"invalid root key addr:%x\n", son->root_key_addr);
		break;
	}
	#ifdef CONFIG_ALI_VSC_SMI
	son->type = type;
	#endif
	
	val = of_get_property(node,
		"fixed-engine-algos", &len);
	if (!val || !len) {
		dev_dbg(&son->parent->clnt->dev,
			"fixed-engine-algos not found");
		goto errout;
	}

	son->fixed_engine_algos_num = len / sizeof(u32);
	if (son->fixed_engine_algos_num > KL_MAX_ENGINE) {
		dev_dbg(&son->parent->clnt->dev,
			"impossible len: %d", len);
		goto errout;
	}

	ret = of_property_read_u32_array(node, "fixed-engine-algos",
		son->fixed_engine_algos, son->fixed_engine_algos_num);
	if (ret) {
		dev_dbg(&son->parent->clnt->dev,
			"get fixed-engine-algos failed");
		goto errout;
	}

	ret = of_property_read_u32(node,
		"one-kl-one-engine-algo", &son->one_kl_one_engine_algo);
	if (ret)
		goto errout;

	if (type == KL_TYPE_VSC)
		goto skip_algo_and_level;

	otp_vals[0].valptr = &son->level_1st2nd3rd_sel;
	otp_vals[1].valptr = &son->level_5th_en;

	ret = of_parse_ali_otp_list(node, otp_vals,
		sizeof(otp_vals)/sizeof(struct of_ali_otp_val));
	if (ret)
		goto errout;

	if (ALI_C3921 == ali_sys_ic_get_chip_id()) {
		/** level-sel-otp[2]
		 *	00 -> 1 level
		 *	01 -> 2 level
		 *	10 -> 3 level
		 *	11 -> 3 level
		 */
		 if (0 == son->level_1st2nd3rd_sel) {
			son->support_level = KL_MAX_LEVEL_1;
		 } else if (1 == son->level_1st2nd3rd_sel) {
			son->support_level = KL_MAX_LEVEL_2;
		 } else if (2 == son->level_1st2nd3rd_sel ||
		 	3 == son->level_1st2nd3rd_sel) {
			son->support_level = KL_MAX_LEVEL_3;
		 }
		 son->max_level = KL_MAX_LEVEL_3;
	} else {
		/* set max gen key level*/
		if (son->level_5th_en) {
			son->support_level = KL_MAX_LEVEL_5;
		} else {
			if (son->level_1st2nd3rd_sel == 0 ||
				son->level_1st2nd3rd_sel == 1) {
				son->support_level = KL_MAX_LEVEL_1;
			} else if (son->level_1st2nd3rd_sel == 2) {
				son->support_level = KL_MAX_LEVEL_2;
			} else if (son->level_1st2nd3rd_sel == 3) {
				son->support_level = KL_MAX_LEVEL_3;
			}
		}

		son->max_level = KL_MAX_LEVEL_5;
	}

skip_algo_and_level:
	return 0;

errout:
	return ret;
}

static int ca_kl_unregister_sub_dev(
	struct ca_kl_sub_dev *son)
{
	struct ca_kl_dev *parent;
	const struct file_operations *fops;

	if (!son)
		return -EINVAL;

	fops = &ca_kl_fops;
	#ifdef CONFIG_ALI_VSC
	if (son->type == KL_TYPE_VSC)
		fops = &ca_kl_vsc_fops;
	#endif
	parent = son->parent;

	ca_kl_dbgfs_remove(son);
	ca_kl_sysfs_remove(son);
	dev_set_drvdata(son->dev, NULL);
	ida_destroy(&son->s_ida);
	device_destroy(g_kl_class, son->devt);
	class_destroy(g_kl_class);
	cdev_del(&son->cdev);
	unregister_chrdev_region(son->devt, 1);
	unregister_kl_callbacks(fops);
	list_del(&son->sub_dev);

	dev_info(&parent->clnt->dev,
		"sub dev %s removed!\n", son->basename);

	devm_kfree(&parent->clnt->dev, son);

	return 0;
}

static int _ca_kl_register_sub_dev(struct ca_kl_dev *parent,
	struct device_node *node, int type)
{
	int ret;
	char dev_base_name[32];
	struct device_node *child;
	struct ca_kl_sub_dev *son;
	char basenames[][6] = { "kl", "etsi", "akl", "vsc" };
	const struct file_operations *fops;

	son = devm_kzalloc(&parent->clnt->dev,
		sizeof(struct ca_kl_sub_dev), GFP_KERNEL);
	if (!son)
		return -ENOMEM;

	son->parent = parent;
	son->type = type;

	ret = ca_kl_probe_sub_dev_dt(son, node, type);
	if (ret < 0)
		goto errout;

	sprintf(son->basename, "%s%d", basenames[type], son->dev_index);
	sprintf(dev_base_name, "%s%s", KL_DEV_BASENAME, son->basename);

	fops = &ca_kl_fops;
	#ifdef CONFIG_ALI_VSC
	if (son->type == KL_TYPE_VSC)
		fops = &ca_kl_vsc_fops;
	#endif

	child = of_get_child_by_name(node, son->basename);
	if (!child) {
		pr_err("Don't find child <functions> of DTS node<%s>!\n",son->basename);
		ret = alloc_chrdev_region(&son->devt, 0, 1, dev_base_name);
		if (ret < 0)
		{
			son->devt =0;
			pr_err("register ali dev_t fail\n");
			goto errout;
		}
	}
	else{
		ret = of_get_major_minor(child,&son->devt, 
				0, 1, dev_base_name);
		if (ret  < 0) {
			pr_err("unable to get major and minor for char devive\n");
			goto errout;
		}
	}

	cdev_init(&son->cdev, fops);
	ret = cdev_add(&son->cdev, son->devt, 1);
	if (ret < 0)
		goto errout1;

	son->dev = device_create(g_kl_class, &parent->clnt->dev,
		son->devt, son, dev_base_name);
	if (IS_ERR(son->dev)) {
		ret = PTR_ERR(son->dev);
		goto errout3;
	}

	son->fd_ops.fetch_key_cell = fetch_kl_key_cell;
	ret = register_kl_callbacks(fops, &son->fd_ops);
	if (ret != 0)
		goto errout4;

	ret = ca_kl_sysfs_create(son);
	if (ret)
		goto errout5;

	ca_kl_dbgfs_create(son);
	dev_set_drvdata(son->dev, son);
	list_add(&son->sub_dev, &parent->sub_dev_list);

	ida_init(&son->s_ida);
	son->debug_mode = 0;

	dev_dbg(son->dev,
		"%s, register sub_dev[%d], root_addr[0x%x], support_level[%d]\n",
		__func__, son->dev_index,
		son->root_key_addr, son->support_level);

	return 0;

errout5:
	unregister_kl_callbacks(fops);
errout4:
	device_destroy(g_kl_class, son->devt);
errout3:
	class_destroy(g_kl_class);
	cdev_del(&son->cdev);
errout1:
	unregister_chrdev_region(son->devt, 1);
errout:
	dev_dbg(&parent->clnt->dev,
		"%s, register sub_dev failed!\n", __func__);
	devm_kfree(&parent->clnt->dev, son);
	return ret;
}

static int ca_kl_register_sub_dev(struct ca_kl_dev *parent,
	struct device_node *node)
{
	int ret, i, num, type;

	int vsc_mode = (of_find_property(node, "vsc-mode", NULL) != NULL);

	if (vsc_mode) {
		/* when in vsc-mode, disable etsi */
		num = 1;
		type = KL_TYPE_VSC;
	} else {
		/*when KDF enable, creating 2 devices (kl && etsi) for a
		 * given node*/
		num = (parent->disable_kdf == 0) ? 2 : 1;
		type = KL_TYPE_ALI;
	}

	for (i = 0; i < num; i++) {
		ret = _ca_kl_register_sub_dev(parent, node, type + i);
		if (ret) {
			dev_dbg(&parent->clnt->dev, "register sub dev err!\n");
			return ret;
		}
	}

	return 0;
}

static int ca_kl_scan_sub_dev(struct ca_kl_dev *parent)
{
	struct device_node *node;
	int ret;

	if (!parent->clnt->dev.of_node) {
		dev_dbg(&parent->clnt->dev, "node null.\n");
		return -ENODEV;
	}

	for_each_available_child_of_node(parent->clnt->dev.of_node, node) {
		ret = ca_kl_register_sub_dev(parent, node);
		if (ret) {
			dev_dbg(&parent->clnt->dev, "Failed to register sub-dev!\n");
			return ret;
		}
	}

	return 0;
}

/* Get parameters from DTS */
static int ca_kl_probe_dt(struct ca_kl_dev *parent)
{
	struct device_node *dn = parent->clnt->dev.of_node;
	int ret;
	
	struct of_ali_otp_val otp_vals[] = {
		{"fixed-addr-mode-otp",	NULL},
		{"fixed-engine-mode-otp", NULL},
		{"one-kl-one-engine-otp", NULL},
		{"disable-kdf-otp", NULL},
	};

	otp_vals[0].valptr = &parent->fixed_addr_mode;
	otp_vals[1].valptr = &parent->fixed_eng_mode;
	otp_vals[2].valptr = &parent->one_kl_one_engine_mode;
	otp_vals[3].valptr = &parent->disable_kdf;

	/* get info from OTP defined in device tree */
	ret = of_parse_ali_otp_list(dn, otp_vals,
				sizeof(otp_vals)/sizeof(struct of_ali_otp_val));
	if (ret)
		return ret;

	dev_dbg(&parent->clnt->dev, "fixed-addr-mode:%d\n",
		parent->fixed_addr_mode);
	dev_dbg(&parent->clnt->dev, "fixed-engine-mode:%d\n",
		parent->fixed_eng_mode);
	dev_dbg(&parent->clnt->dev, "one-kl-one-engine-mode:%d\n",
		parent->one_kl_one_engine_mode);
	dev_dbg(&parent->clnt->dev, "disable-kdf:%d\n",
		parent->disable_kdf);

	return ret;
}


static int ca_kl_probe(struct see_client *clnt)
{
	int ret;
	struct ca_kl_sub_dev *son, *_son;
	struct ca_kl_dev *parent;
	struct device_node *dn = clnt->dev.of_node;

	dev_info(&clnt->dev, "probing KL@%d\n", clnt->service_id);
	if (!dn) {
		dev_dbg(&clnt->dev, "dn invalid\n");
		return -EINVAL;
	}

	parent = devm_kzalloc(&clnt->dev, sizeof(struct ca_kl_dev), GFP_KERNEL);
	if (!parent)
		return -ENOMEM;

	parent->clnt = clnt;
	mutex_init(&parent->mutex);
	INIT_LIST_HEAD(&parent->sub_dev_list);

	/*open see module for ce*/
	ali_m36_ce_see_init();
	parent->see_ce_id = hld_dev_get_by_type(NULL, HLD_DEV_TYPE_CE);
	if (NULL == parent->see_ce_id)
		goto errout;

	if (ALI_C3921 == ali_sys_ic_get_chip_id()) {
	
		parent->legacy_hw = 1;
	} 
	else{
		parent->legacy_hw = 0;
	}
	if (of_have_populated_dt()) {
		ret = ca_kl_probe_dt(parent);
		if (ret < 0) {
			dev_dbg(&clnt->dev, "failed to parse DT\n");
			goto errout;
		}

		ret = ca_kl_scan_sub_dev(parent);
		if (ret < 0) {
			dev_dbg(&clnt->dev, "failed to scan sub devices\n");
			goto errout;
		}
	}

	dev_set_drvdata(&clnt->dev, parent);
	dev_info(&clnt->dev, "driver probed\n");
	return 0;

errout:
	list_for_each_entry_safe(son, _son, &parent->sub_dev_list, sub_dev)
		ca_kl_unregister_sub_dev(son);

	devm_kfree(&clnt->dev, parent);
	return ret;
}

static int ca_kl_remove(struct see_client *clnt)
{
	struct device_node *dn = clnt->dev.of_node;
	const char *clk_name = NULL;
	struct clk *clk;

	struct ca_kl_sub_dev *son, *_son;
	struct ca_kl_dev *parent = dev_get_drvdata(&clnt->dev);

	if (!parent)
		return -ENODEV;

	of_property_read_string(dn, "clock-names", &clk_name);
	clk = devm_clk_get(&parent->clnt->dev, clk_name);
	if (IS_ERR(clk)) {
		dev_info(&parent->clnt->dev, "get clk error\n");
	} else {
		clk_disable(clk);
		clk_unprepare(clk);
	}

	list_for_each_entry_safe(son, _son, &parent->sub_dev_list, sub_dev)
		ca_kl_unregister_sub_dev(son);

	dev_set_drvdata(&clnt->dev, NULL);
	mutex_destroy(&parent->mutex);

	devm_kfree(&clnt->dev, parent);
	dev_info(&clnt->dev, "KL driver removed\n");
	return 0;
}

static int ca_kl_suspend(struct device *dev)
{
	ali_m36_ce_see_suspend();

	return 0;
}

static int ca_kl_resume(struct device *dev)
{
	ali_m36_ce_see_resume();

	return 0;
}


static struct dev_pm_ops kl_drv_pm_ops = {
	.suspend = ca_kl_suspend,
	.resume = ca_kl_resume,
};

static const struct of_device_id see_kl_matchtbl[] = {
	{ .compatible = "alitech,kl" },
	{ }
};

static struct see_client_driver kl_drv = {
	.probe	= ca_kl_probe,
	.remove	= ca_kl_remove,
	.driver	= {
		.name		= "KL",
		.of_match_table	= see_kl_matchtbl,
		.pm = &kl_drv_pm_ops,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};
static int __init kl_init(void)
{
	g_kl_class = class_create(THIS_MODULE, KL_DRV_NAME);
	if (IS_ERR(g_kl_class))
		return PTR_ERR(g_kl_class);

	return see_client_driver_register(&kl_drv);
}

static void __exit kl_exit(void)
{
	see_client_driver_unregister(&kl_drv);
	class_destroy(g_kl_class);
}

module_init(kl_init);
module_exit(kl_exit);

MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi Key Ladder Core");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.3.0");


