/*
 * CERT Driver
 *
 * CERT top layer driver, parsing the
 * global platform configurations and
 * probing the sub-devices ASA and AKL.
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
#include <linux/clk-provider.h>

#include "ali_cert_akl_priv.h"
#include "ali_cert_asa_priv.h"
#include "ca_otp_dts.h"


/* Get parameters from DTS */
static int cert_probe_dt(struct see_client *clnt)
{
	struct device_node *dn = clnt->dev.of_node;
	struct cert_plat_data *pdata = NULL;
	int ret = -1;

	pdata = devm_kzalloc(&clnt->dev, sizeof(struct cert_plat_data),
			     GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	/* get info from OTP defined in device tree */
	ret = of_parse_ali_otp(dn, "cert-disable",
				&pdata->is_cert_disabled);
	if (ret) {
		devm_kfree(&clnt->dev, pdata);
		return ret;
	}

	if (!pdata->is_cert_disabled) {
		ret = of_parse_ali_otp(dn, "asa-disable",
				&pdata->is_asa_disabled);
		if (ret) {
			devm_kfree(&clnt->dev, pdata);
			return ret;
		}
	}

	clnt->dev.platform_data = pdata;
	return ret;
}

static int cert_probe(struct see_client *clnt)
{
	int ret = -1;
	struct cert_plat_data *pdata = NULL;
	struct cert_driver *drv = NULL;

	if (of_have_populated_dt()) {
		ret = cert_probe_dt(clnt);
		if (ret < 0) {
			dev_dbg(&clnt->dev, "Failed to parse DT\n");
			return ret;
		}
	}

	pdata = clnt->dev.platform_data;
	if (unlikely(!pdata)) {
		dev_dbg(&clnt->dev, "No configuration data\n");
		return -ENXIO;
	}

	if (pdata->is_cert_disabled) {
		dev_dbg(&clnt->dev, "No CERT HW device\n");
		return -ENXIO;
	}

	drv = devm_kzalloc(&clnt->dev,
			sizeof(struct cert_driver),
			GFP_KERNEL);
	if (!drv) {
		devm_kfree(&clnt->dev, pdata);
		return -ENOMEM;
	}

	drv->clnt = clnt;
	ret = cert_akl_probe(drv);
	if (ret != 0) {
		devm_kfree(&clnt->dev, drv);
		dev_dbg(&clnt->dev, "probing CERT AKL err\n");
		return ret;
	}

	if (!pdata->is_asa_disabled) {
		ret = cert_asa_probe(drv);
		if (ret != 0) {
			dev_dbg(&clnt->dev, "probing CERT ASA err\n");
			cert_akl_remove(drv);
			devm_kfree(&clnt->dev, drv);
			return ret;
		}
	}

	drv->pdata = pdata;
	dev_set_drvdata(&clnt->dev, drv);

	return 0;
}

static int cert_remove(struct see_client *clnt)
{
	struct cert_driver *drv = dev_get_drvdata(&clnt->dev);

	if (!drv)
		return 0;

	dev_set_drvdata(&clnt->dev, NULL);

	cert_akl_remove(drv);
	if (!drv->pdata->is_asa_disabled)
		cert_asa_remove(drv);

	clk_prepare(drv->pdata->clk);
	clk_disable(drv->pdata->clk);
	devm_clk_put(&clnt->dev, drv->pdata->clk);
	devm_kfree(&clnt->dev, drv->pdata);
	devm_kfree(&clnt->dev, drv);

	return 0;
}

/**
ALi CERT DTS node description:

Required properties :
- compatible : should be "alitech,cert"
- reg : unique index within the see_bus
- cert-disable : OTP to deactivate the CERT ASA and AKL hardware
- asa-disable : OTP to deactivate the ASA hardware (not affect the AKL)
- clocks : clock source selection
- clock-names : clock name selection

Example:
	CERT@6 {
			compatible = "alitech,cert";
			reg = <6>;
			cert-disable = <&otp 0x8c 19 1>;
			asa-disable = <&otp 0x8c 0 1>;
			clocks = <&cap210_clks_gate1 1>;
			clock-names = "cert_ip_clk";
	};
*/
static const struct of_device_id see_cert_matchtbl[] = {
	{ .compatible = "alitech,cert" },
	{ }
};

static struct see_client_driver cert_driver = {
	.probe	= cert_probe,
	.remove	= cert_remove,
	.driver	= {
		.name = "CERT",
		.of_match_table = see_cert_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

module_see_client_driver(cert_driver);

MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("CERT Driver for Advanced Security");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.0");

