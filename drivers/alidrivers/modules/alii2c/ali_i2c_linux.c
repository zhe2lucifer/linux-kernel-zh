/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
//#define DEBUG
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk-provider.h>
#include <linux/version.h>
#include "ali_i2c_common.h"

#ifdef CONFIG_ALI_CHIP_3922
#define S3922C_I2C_INT_MERGE
#endif

#ifdef S3922C_I2C_INT_MERGE
#define MAX_I2C_NUM 4
static struct ali_i2c *i2c_array[MAX_I2C_NUM];
static int i2c_array_size = 0;
#endif

static irqreturn_t ali_i2c_isr(int irq, void *dev_id)
{
#ifdef S3922C_I2C_INT_MERGE
	int i;
	for (i=0; i<i2c_array_size; ++i) {
		__ali_i2c_isr(i2c_array[i]);
	}
#else	
	__ali_i2c_isr((struct ali_i2c *)dev_id);
#endif
	return IRQ_HANDLED;
}

static int ali_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct ali_i2c *i2c = i2c_get_adapdata(adap);
	int err;

	__ali_i2c_lock(i2c);
	i2c->msgs = msgs;
	i2c->nmsgs = num;
	err = __ali_i2c_start_xfer(i2c);	
	__ali_i2c_unlock(i2c);
	return (0 == err) ? num : err;
}

static u32 ali_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm ali_algorithm = {
	.master_xfer = ali_i2c_xfer,
	.functionality = ali_i2c_func,
};

static struct i2c_adapter ali_adapter = {
	.owner = THIS_MODULE,
	.name = DRIVER_NAME,
	.class = I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.algo = &ali_algorithm,
};

static int ali_i2c_probe(struct platform_device *pdev)
{
	struct ali_i2c *i2c;
	struct resource *mem;

	i2c = devm_kzalloc(&pdev->dev, sizeof (*i2c), GFP_KERNEL);
	if (NULL == i2c) {
		I2C_ERR("out of memory\n");
		return -ENOMEM;
	}

	i2c->dev = &pdev->dev;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(i2c->base)) {
		I2C_ERR("ioremap fail\n");
		return PTR_ERR(i2c->base);
	}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
	i2c->ram_cache_regs = devm_kzalloc(i2c->dev, resource_size(mem), GFP_KERNEL);
	if (NULL == i2c->ram_cache_regs) {
		I2C_ERR("alloc memory for STR fail\n");
		return -ENOMEM;
	}
#endif

	/*get clock from DTS */
	if (of_property_read_u32(i2c->dev->of_node, "clock-frequency", &i2c->bus_clk_rate)) {
		i2c->bus_clk_rate = 100000;	/* default clock rate */
	}
	if (i2c->bus_clk_rate > 400000) {
		I2C_ERR("invalid clock rate: %d\n", i2c->bus_clk_rate);
		return -EINVAL;
	}

	if (of_property_read_u32(i2c->dev->of_node, "is_std_read", &i2c->is_std_read)) {
		i2c->is_std_read = 0;
	}
	if (0 != i2c->is_std_read) {
		i2c->is_std_read = 1;
	}

	/* hook up driver to tree */
	platform_set_drvdata(pdev, i2c);

	i2c->adap = ali_adapter;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->adap.dev.of_node = pdev->dev.of_node;
	i2c_set_adapdata(&i2c->adap, i2c);

	__ali_i2c_init(i2c);

	//-EPROBE_DEFER, -ENXIO;
	i2c->irq = platform_get_irq(pdev, 0);
	if (-EPROBE_DEFER == i2c->irq) {
		return -EPROBE_DEFER;
	} else if (0 <= i2c->irq) {
		if (devm_request_irq(i2c->dev, i2c->irq, ali_i2c_isr, 0, dev_name(i2c->dev), i2c)) {
			dev_err(i2c->dev, "Cannot claim IRQ\n");
			return -1;
		}
		i2c->enable_int = 1;
	} else {
		I2C_INFO("No interrupt specified.\n");
	}

	/* add i2c adapter to i2c tree */
	if (i2c_add_adapter(&i2c->adap)) {
		I2C_ERR("Failed to add adapter\n");
		return -1;
	}

#ifdef S3922C_I2C_INT_MERGE
	i2c_array[i2c_array_size++] = i2c;
#endif

	I2C_INFO("i2C_adap_id: %d, i2c->irq: %d, i2c->is_std_read: %d, i2c->base: 0x%x, probe success!\n",
		 i2c->adap.nr, i2c->irq, i2c->is_std_read, (int)i2c->base);
	return 0;
}

static int ali_i2c_remove(struct platform_device *pdev)
{
	struct ali_i2c *i2c = platform_get_drvdata(pdev);

	i2c_del_adapter(&i2c->adap);
	__ali_i2c_deinit(i2c);
	return 0;
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ali_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ali_i2c *ali_i2c = platform_get_drvdata(pdev);
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	memcpy_fromio(ali_i2c->ram_cache_regs, ali_i2c->base, resource_size(mem));
	return 0;
}

static int ali_i2c_resume(struct platform_device *pdev)
{
	struct ali_i2c *ali_i2c = platform_get_drvdata(pdev);
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	memcpy_toio(ali_i2c->base, ali_i2c->ram_cache_regs, resource_size(mem));
	return 0;
}
#endif

static const struct of_device_id ali_of_match[] = {
	{.compatible = "alitech,i2c",},
	{},
};

MODULE_DEVICE_TABLE(of, ali_of_match);

static struct platform_driver ali_i2c_driver = {
	.probe = ali_i2c_probe,
	.remove = ali_i2c_remove,
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	.suspend = ali_i2c_suspend,
	.resume = ali_i2c_resume,
#endif
	.driver = {
		.owner = THIS_MODULE,
		.name = DRIVER_NAME,
		.of_match_table = ali_of_match,
	},
};

//rick, modify ali_i2c to be probe at same level as i2c_gpio and earlier than normal device_initcall
#if 1
static int __init ali_i2c_driver_init(void)
{
	int ret;

	ret = platform_driver_register(&ali_i2c_driver);
	if (ret) {
		printk(KERN_ERR "ali_i2c: probe failed: %d\n", ret);
	}

	return ret;
}

// make sure HW I2C device nodes are created in front of SW I2C
//subsys_initcall(ali_i2c_driver_init);
arch_initcall(ali_i2c_driver_init);

static void __exit ali_i2c_driver_exit(void)
{
	platform_driver_unregister(&ali_i2c_driver);
}

module_exit(ali_i2c_driver_exit);
#else
module_platform_driver(ali_i2c_driver);
#endif

MODULE_AUTHOR("Dennis Dai,Ziv Gu,Billy Zhou");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ALi I2C Driver");
MODULE_VERSION("1.1.1");
