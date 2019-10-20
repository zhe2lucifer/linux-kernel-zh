/**
 * dwc3-ali.c - ALi DWC3 Glue layer
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Shih <david.shih@alitech.com>,
 *          Lucas.Lai  <lucas.lai@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk-provider.h>
#include <linux/clkdev.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/usb/otg.h>
#include <linux/usb/usb_phy_gen_xceiv.h>
#include <linux/of.h>
#include <linux/of_platform.h>

/* one-time programming register */
#define LIMIT_START_ADDR      0x0000  /* USB Memory Start Address */
#define LIMIT_END_ADDR        0x0004  /* USB Memory End Address */
#define CFG_USB_LIMIT         0x0008  /* [0]: Enable USB_LIMIT function */
#define ENABLE_USB_LIMIT (1 << 0)

static void usb_dma_limit_dump_reg(void __iomem *base)
{
	pr_info("--- usb_dma_limit_dump_reg ----\n");
	pr_info("LIMIT_START_ADDR(0x%.08x)    = 0x%.08x\n",
		(unsigned int) base+LIMIT_START_ADDR,
		ioread32(base+LIMIT_START_ADDR));
	pr_info("LIMIT_END_ADDR(0x%.08x)      = 0x%.08x\n",
		(unsigned int) base+LIMIT_END_ADDR,
		ioread32(base+LIMIT_END_ADDR));
	pr_info("CFG_USB_LIMIT_SPACE(0x%.08x) = 0x%.08x\n",
		(unsigned int) base+CFG_USB_LIMIT,
		ioread32(base+CFG_USB_LIMIT));
}

static int usb_dma_limit_is_enable(void __iomem *base)
{
	return ((ioread32(base+CFG_USB_LIMIT) & ENABLE_USB_LIMIT)
		== ENABLE_USB_LIMIT);
}

/**
 * usb_dma_limit_enable
 * USB memory limit space function must be in the beginning of USB
 * initialize function. The function allows USB IP to access the target
 * range of memory.
 */
static void usb_dma_limit_enable(void __iomem *base, u32 limit_start_addr,
	u32 limit_end_addr)
{

	if (limit_start_addr > limit_end_addr) {
		pr_err("%s: start addr(0x%.08x) > end addr(0x%.08x) !!\n",
			__func__, limit_start_addr, limit_end_addr);
		return;
	}

	if (usb_dma_limit_is_enable(base)) {
		pr_err("%s: usb dma limit already enable.\n", __func__);
		usb_dma_limit_dump_reg(base);
		return;
	}

	iowrite32(limit_end_addr, base+LIMIT_END_ADDR);
	iowrite32(limit_start_addr, base+LIMIT_START_ADDR);
	iowrite32(ioread32(base+CFG_USB_LIMIT) | ENABLE_USB_LIMIT,
		base+CFG_USB_LIMIT);

	return;
}

struct addr_range {
	u32 start;
	u32 end;
};

static int dwc3_ali_probe(struct platform_device *pdev)
{
	struct device   *dev = &pdev->dev;
	struct resource *res;
	void __iomem    *base;
	struct addr_range dma_limit_range = { .start = 0x00000000,
					      .end = 0xFFFFFFFF };
	const char *clk_string;
	struct clk *clk = NULL;
	int ret = -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "%s: missing memory base resource\n",
			__func__);
		return -EINVAL;
	}

	of_property_read_string(dev->of_node, "clock-names", &clk_string);
	clk = devm_clk_get(dev, clk_string);
	if (IS_ERR(clk)) {
		dev_err(dev, "%s: get clock error\n",
			__func__);
		return -EINVAL;
	}

	clk_prepare(clk);
	clk_enable(clk);

	of_property_read_u32_array(dev->of_node, "dma-limit-range",
		&dma_limit_range, 2);

	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	pr_info("dma limit address = <0x%.08x 0x%.08x>\n",
		dma_limit_range.start, dma_limit_range.end);
	usb_dma_limit_enable(base, dma_limit_range.start, dma_limit_range.end);

	devm_iounmap(dev, base);

	ret = of_platform_populate(dev->of_node, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "failed to add dwc3 core\n");
	  return ret;
	}

	return 0;
}

static int dwc3_ali_remove_child(struct device *dev, void *unused)
{
	struct platform_device *pdev = to_platform_device(dev);

	platform_device_unregister(pdev);
	return 0;
}

static int dwc3_ali_remove(struct platform_device *pdev)
{
	device_for_each_child(&pdev->dev, NULL, dwc3_ali_remove_child);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ali_dwc3_match[] = {
	{ .compatible = "alitech,dwc3" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, ali_dwc3_match);
#endif

static struct platform_driver dwc3_ali_driver = {
	.probe    = dwc3_ali_probe,
	.remove   = dwc3_ali_remove,
	.driver   = {
		.name = "ali-dwc3",
		.of_match_table = of_match_ptr(ali_dwc3_match),
		.pm             = NULL,
	},
};

module_platform_driver(dwc3_ali_driver);

MODULE_ALIAS("platform:ali-dwc3");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 Ali Glue Layer");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("David Shih <david.shih@alitech.com>");
