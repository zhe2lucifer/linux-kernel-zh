/*
 * VSC Driver for Conax Virtual Smart Card
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/pagemap.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include "ca_vsc_priv.h"


#define VSC_DRIVER_VERSION "1.2.0"


int ca_vsc_smc_probe(struct ca_vsc_drv *drv);
int ca_vsc_smc_remove(struct ca_vsc_drv *drv);
int ca_vsc_store_probe(struct ca_vsc_drv *drv);
int ca_vsc_store_remove(struct ca_vsc_drv *drv);
void ca_vsc_store_clean(void);
void ca_vsc_process_lib(void);

/* Get parameters from DTS */
static int vsc_probe_dt(struct see_client *clnt)
{
	struct device_node *dn = clnt->dev.of_node;
	struct ca_vsc_plat_data *pdata = NULL;
	int ret = 0;

	(void) dn;

	pdata = devm_kzalloc(&clnt->dev,
			sizeof(struct ca_vsc_plat_data),
			GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	ret |= of_property_read_u32_index(
			dn, "lib-addr-params", 0, &pdata->lib_addr);
	ret |= of_property_read_u32_index(
			dn, "lib-addr-params", 1, &pdata->lib_len);

	if (pdata->lib_len > VSC_LIB_MAX_LEN)
		return -EINVAL;

	if (pdata->lib_len < 1)
		return -EINVAL;

	dev_dbg(&clnt->dev, "%s,%d: lib-params 0x%x,0x%x\n",
			__func__, __LINE__, pdata->lib_addr, pdata->lib_len);

	clnt->dev.platform_data = pdata;
	return ret;
}

int ca_vsc_probe(struct see_client *clnt)
{
	int ret = -1;
	struct ca_vsc_plat_data *pdata = NULL;
	struct ca_vsc_drv *drv = NULL;

	if (of_have_populated_dt()) {
		ret = vsc_probe_dt(clnt);
		if (ret < 0) {
			dev_info(&clnt->dev, "Failed to parse DT\n");
			return ret;
		}
	}

	pdata = clnt->dev.platform_data;
	if (unlikely(!pdata)) {
		dev_info(&clnt->dev, "No configuration data\n");
		return -ENXIO;
	}

	drv = devm_kzalloc(&clnt->dev,
			sizeof(struct ca_vsc_drv),
			GFP_KERNEL);
	if (!drv) {
		devm_kfree(&clnt->dev, pdata);
		return -ENOMEM;
	}

	drv->clnt = clnt;
	drv->pdata = pdata;
	drv->see_ce_id = hld_dev_get_by_type(NULL, HLD_DEV_TYPE_CE);

	dev_set_drvdata(&clnt->dev, drv);
	mutex_init(&drv->mutex);

	ret = ca_vsc_smc_probe(drv);
	if (ret != 0) {
		devm_kfree(&clnt->dev, drv);
		dev_info(&clnt->dev, "probing VSC SMC failed\n");
		return ret;
	}

	ret = ca_vsc_store_probe(drv);
	if (ret != 0) {
		devm_kfree(&clnt->dev, drv);
		dev_info(&clnt->dev, "probing VSC STORE failed\n");
		return ret;
	}
	
	ca_vsc_store_clean();
	ca_vsc_process_lib();//only work on C1903A
	
	dev_info(&clnt->dev, "probed ver %s\n", VSC_DRIVER_VERSION);

	return 0;
}

int ca_vsc_remove(struct see_client *clnt)
{
	struct ca_vsc_drv *drv = dev_get_drvdata(&clnt->dev);

	if (!drv)
		return 0;

	dev_set_drvdata(&clnt->dev, NULL);

	ca_vsc_smc_remove(drv);
	ca_vsc_store_remove(drv);

	devm_kfree(&clnt->dev, drv->pdata);
	devm_kfree(&clnt->dev, drv);

	dev_info(&drv->clnt->dev, "driver removed\n");
	return 0;
}

/**
ALi VSC DTS node description:

Required properties :
- compatible : should be "alitech,conaxvsc"
- reg : unique index within the see_bus

Example:

*/
static const struct of_device_id see_vsc_matchtbl[] = {
	{ .compatible = "alitech,conaxvsc" },
	{ }
};

static struct see_client_driver vsc_driver = {
	.probe	= ca_vsc_probe,
	.remove	= ca_vsc_remove,
	.driver	= {
		.name = VSC_DRVNAME,
		.of_match_table = see_vsc_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 0, 0, 0),
};

module_see_client_driver(vsc_driver);

MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("Conax VSC Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(VSC_DRIVER_VERSION);
