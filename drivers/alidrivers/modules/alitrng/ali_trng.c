/*
 * ALi True Random Number Generator driver
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/hw_random.h>
#include <linux/platform_device.h>

#include "ali_trng_priv.h"

static int ca_trng_read(struct hwrng *rng, void *buf,
	size_t max, bool wait)
{
	struct ca_trng *trng;
	int ret = -1;
	u8 *data = (u8 *)buf;
	int cnt = max/8;
	int residue = max%8;
	u8 tmp[8];

	trng = container_of(rng, struct ca_trng, rng);

	ret = ca_trng_get_64bits(trng, data, cnt);
	if (residue) {
		ret |= ca_trng_get_64bits(trng, tmp, 1);
		memcpy(buf+max-residue, tmp, residue);
	}

	if (0 != ret)
		return 0;

	return max;
}

static int ca_trng_probe(struct see_client *clnt)
{
	struct ca_trng *trng;
	int ret;

	dev_info(&clnt->dev, "probe.\n");

	trng = devm_kzalloc(&clnt->dev, sizeof(*trng), GFP_KERNEL);
	if (!trng)
		return -ENOMEM;

	trng->clnt = clnt;
	trng->rng.name = clnt->name;
	trng->rng.read = ca_trng_read;

	ret = hwrng_register(&trng->rng);
	if (ret)
		goto err_register;

	dev_set_drvdata(&clnt->dev, trng);

	return 0;

err_register:
	return ret;
}

static int ca_trng_remove(struct see_client *clnt)
{
	struct ca_trng *trng = dev_get_drvdata(&clnt->dev);

	hwrng_unregister(&trng->rng);
	dev_set_drvdata(&clnt->dev, NULL);

	return 0;
}

static const struct of_device_id see_trng_matchtbl[] = {
	{ .compatible = "alitech,trng" },
	{ }
};

static struct see_client_driver trng_drv = {
	.probe	= ca_trng_probe,
	.remove	= ca_trng_remove,
	.driver	= {
		.name		= "TRNG",
		.of_match_table	= see_trng_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

module_see_client_driver(trng_drv);

MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("True Random Data Generator");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.0");


