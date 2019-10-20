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
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/list.h>

#include "see_bus.h"
#include "see_bus_internals.h"


static inline struct see_client *dev2client(struct device *dev)
{
	return container_of(dev, struct see_client, dev);
}

static inline struct see_client_driver *dev2drv(struct device *dev)
{
	return container_of(dev->driver, struct see_client_driver, driver);
}


static void see_clnt_release(struct device *dev)
{
	kfree(dev2client(dev));
}

static struct device_type see_client_type = {
	.name		= "SEE_bus_client",
	.release	= see_clnt_release,
};


static int see_bus_match(struct device *dev, struct device_driver *drv)
{
	struct see_client *client = dev2client(dev);
	/* First check if we have a device tree match */
	if (of_driver_match_device(dev, drv))
		return 1;

	/* Fallback: Check if the driver and device names match */
	return (strcmp(client->name, drv->name) == 0);
}

static int see_bus_probe(struct device *dev)
{
	struct see_client *client = dev2client(dev);
	struct see_client_driver *drv = dev2drv(dev);
	int ret;
	
	/*check the see min version and see fw version*/
	if (SEE_VERSION_PROTO(drv->see_min_version) !=
	    SEE_VERSION_PROTO(client->bus->see_fw_version)) {
			dev_err(dev, "see min version:0x%08x don not match with see fw version:0x%08x\n",
			drv->see_min_version, client->bus->see_fw_version);
		return -EPROTO;
	} else if (SEE_VERSION_REST(drv->see_min_version) <
	SEE_VERSION_REST(client->bus->see_fw_version)) {
		dev_err(dev, "see min version:0x%08x don not match with see fw version:0x%08x\n",
			drv->see_min_version, client->bus->see_fw_version);
		return -EPROTO;
	}

#if defined(CONFIG_ALI_RPCNG)
	client->service = open_see_service(client->bus, client->service_id);
	if (IS_ERR(client->service))
		return PTR_ERR(client->service);
#endif

	ret = drv->probe(client);
	if (ret)
		goto probe_fail;

	return ret;

probe_fail:
	close_see_service(client->service);
	return ret;
}

static int see_bus_remove(struct device *dev)
{
	struct see_client *client = dev2client(dev);
	struct see_client_driver *drv = dev2drv(dev);

	close_see_service(client->service);

	return drv->remove(client);
}

struct bus_type see_bus_type = {
	.name	= "see",
	.match	= see_bus_match,
	.probe	= see_bus_probe,
	.remove	= see_bus_remove,
};
EXPORT_SYMBOL(see_bus_type);


int __see_client_driver_register(struct see_client_driver *drv,
				struct module *mod)
{
	drv->driver.owner	= mod;
	drv->driver.bus		= &see_bus_type;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(__see_client_driver_register);

void see_client_driver_unregister(struct see_client_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(see_client_driver_unregister);


struct see_client *see_client_alloc(char *name, int service_id)
{
	struct see_client *clnt;

	clnt = kzalloc(sizeof(struct see_client), GFP_KERNEL);
	if (!clnt)
		return ERR_PTR(-ENOMEM);

	clnt->name		= name;
	clnt->service_id	= service_id;
	//clnt->service = kzalloc(sizeof(struct see_service), GFP_ATOMIC);  // should not kmalloc here!!

	return clnt;
}
EXPORT_SYMBOL(see_client_alloc);

int see_client_register(struct see_client *clnt, struct see_bus *bus)
{    
	clnt->bus		= bus;
	clnt->dev.parent	= &bus->pdev->dev;
	clnt->dev.bus		= &see_bus_type;
	clnt->dev.type		= &see_client_type;
	dev_set_name(&clnt->dev, "%02x:%02x", get_rpc_bus_id(bus->rcd),
			clnt->service_id);

	list_add(&clnt->clients, &bus->clients);

	return device_register(&clnt->dev);
}
EXPORT_SYMBOL(see_client_register);

void see_client_unregister(struct see_client *clnt)
{
	list_del(&clnt->clients);

	device_unregister(&clnt->dev);
}
EXPORT_SYMBOL(see_client_unregister);

static struct see_client *of_see_bus_register_client(struct see_bus *bus,
					struct device_node *node)
{
	u32 sid;
	int ret;
	struct see_client *clnt;

	const char *cp;
	int cplen;

	cp = of_get_property(node, "compatible", &cplen);
	if (cp == NULL)
		return 0;

	ret = of_property_read_u32(node, "reg", &sid);
	if (ret)
		return ERR_PTR(ret);

	clnt = see_client_alloc((char *)cp, sid);
	if (IS_ERR(clnt))
		return clnt;

	clnt->dev.of_node = node;
	ret = see_client_register(clnt, bus);
	if (ret)
		goto err;

	dev_info(&clnt->dev, "Register see_client:%d:%s on Ali SEE Bus Okay\n",
		sid, clnt->name);

	return clnt;

err:
	kfree(clnt);
	return ERR_PTR(ret);
}


static void of_see_bus_scan_clients(struct see_bus *bus)
{
	struct platform_device *pdev = bus->pdev;
	struct device_node *node;

	if (!pdev->dev.of_node) {
		dev_dbg(&pdev->dev, "node null.\n");
		return;
	} else {
		dev_dbg(&pdev->dev, "of_node[%p],[%s]\n", pdev->dev.of_node,
			pdev->dev.of_node->name);
	}

	for_each_available_child_of_node(pdev->dev.of_node, node)
		of_see_bus_register_client(bus, node);
}

static int see_platform_probe(struct platform_device *pdev)
{
	struct see_bus *bus;


	bus = kzalloc(sizeof(struct see_bus), GFP_KERNEL);
	if (!bus)
		return -ENOMEM;

	dev_info(&pdev->dev, "probe.\n");

	platform_set_drvdata(pdev, bus);
	bus->pdev = pdev;
	bus->rcd = init_see_connection(pdev);
	bus->see_fw_version = see_fw_version_get();

	dev_info(&pdev->dev, "fetch see_fw_version: proto:%d, major:%d, minor:%d, patch:%d\n",
	(bus->see_fw_version>>24)&0xff, (bus->see_fw_version>>16)&0xff,
	(bus->see_fw_version>>8)&0xff, bus->see_fw_version&0xff);
	INIT_LIST_HEAD(&bus->clients);
	
	of_see_bus_scan_clients(bus);

	return 0;
}

static int see_platform_remove(struct platform_device *pdev)
{
	struct see_bus *bus = platform_get_drvdata(pdev);
	struct see_client *clnt, *_clnt;

	list_for_each_entry_safe(clnt, _clnt, &bus->clients, clients)
		see_client_unregister(clnt);

	kfree(bus);

	return 0;
}

static const struct of_device_id see_bus_dt_ids[] = {
	{ .name	= "see_bus", .compatible = "alitech,see-bus" },
	{ }
};

static struct platform_driver see_bus_driver = {
	.probe	= see_platform_probe,
	.remove	= see_platform_remove,
	.driver	= {
		.name	= "see_bus",
		.of_match_table = of_match_ptr(see_bus_dt_ids),
		.owner	= THIS_MODULE,
	},
};

#include <linux/platform_device.h>


static int __init see_bus_module_init(void)
{
	int ret;

	ret = bus_register(&see_bus_type);
	if (ret)
		return ret;

	ret = platform_driver_register(&see_bus_driver);
	if (ret)
		goto platdrv_fail;

	return ret;

platdrv_fail:
	bus_unregister(&see_bus_type);
	return ret;
}

static void __exit see_bus_module_exit(void)
{
	platform_driver_unregister(&see_bus_driver);

	bus_unregister(&see_bus_type);
}

rootfs_initcall(see_bus_module_init);
module_exit(see_bus_module_exit);
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI See bus driver");
MODULE_LICENSE("GPL");
