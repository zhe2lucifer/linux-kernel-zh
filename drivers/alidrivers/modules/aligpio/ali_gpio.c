/* ALi Systems MODULE DESCRIPTION
 *
 * Copyright (C) ALi Systems 2015
 *
 * Authors: Dennis.Dai <Dennis.Dai@alitech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/pinctrl/consumer.h>
#include <linux/version.h>

#define ALI_GPIO_DIR_IN					(0x00000000)
#define ALI_GPIO_DIR_OUT				(0x00000001)
#define OFFSET_TO_REG_INT_EN				(ali_gpio->offsets[0])
#define OFFSET_TO_REG_EDGE_RISING			(ali_gpio->offsets[1])
#define OFFSET_TO_REG_EDGE_FALLING			(ali_gpio->offsets[2])
#define OFFSET_TO_REG_INT_STATUS			(ali_gpio->offsets[3])
#define OFFSET_TO_REG_SET_DIR				(ali_gpio->offsets[4])
#define OFFSET_TO_REG_SET_OUTPUT			(ali_gpio->offsets[5])
#define OFFSET_TO_REG_INPUT_STATUS			(ali_gpio->offsets[6])

#define OFFSETS_SIZE 7
// offsets of past project such as C3505
static const u32 PAST_OFFSETS[OFFSETS_SIZE] = {0x0, 0x4, 0x8, 0x18, 0x14, 0x10, 0xc};

/**
 * @spinlock: used for atomic read/modify/write of registers
 * @base: register base address
 * @domain: IRQ domain of GPIO generated interrupts managed by this controller
 * @irq: Interrupt line of parent interrupt controller
 * @gc: gpio_chip structure associated to this GPIO controller
 */
struct ali_gpio {
	spinlock_t spinlock;
	void __iomem *base;
	struct irq_domain *domain;
	int irq;
	struct gpio_chip gc;
	char label[16];

	// DTS: offsets = <0x0 0x4 0x8 0xc 0x10 0x14 0x18>;
	u32 offsets[OFFSETS_SIZE];
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	void *ram_cache_regs;// registers content cache in RAM
#endif
};

static inline u32 ali_reg_read(struct ali_gpio *gpio, unsigned int offs)
{
	return ioread32(gpio->base + offs);
}

static inline void ali_reg_write(struct ali_gpio *gpio, unsigned int offs, u32 val)
{
	iowrite32(val, gpio->base + offs);
}

static inline void ali_set_bits(struct ali_gpio *gpio, unsigned int offs, u32 mask, u32 val)
{
	u32 r;
	unsigned long flags;

	spin_lock_irqsave(&gpio->spinlock, flags);

	r = ali_reg_read(gpio, offs);
	r = (r & ~mask) | (val & mask);

	ali_reg_write(gpio, offs, r);
	spin_unlock_irqrestore(&gpio->spinlock, flags);
}

static inline struct ali_gpio *to_ali_gpio(struct gpio_chip *chip)
{
	return container_of(chip, struct ali_gpio, gc);
}

static int ali_gpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
	struct ali_gpio *ali_gpio = to_ali_gpio(chip);
	int mask = BIT(offset);
	int val = ALI_GPIO_DIR_IN << offset;
	ali_set_bits(ali_gpio, OFFSET_TO_REG_SET_DIR, mask, val);

	return 0;
}

static int ali_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct ali_gpio *ali_gpio = to_ali_gpio(chip);
	int val;
	val = ali_reg_read(ali_gpio, OFFSET_TO_REG_INPUT_STATUS);

	if (val & BIT(offset))
		return 1;
	else
		return 0;
}

static void ali_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ali_gpio *ali_gpio = to_ali_gpio(chip);
	int mask = BIT(offset);
	int val = value << offset;
	ali_set_bits(ali_gpio, OFFSET_TO_REG_SET_OUTPUT, mask, val);
}

static int ali_gpio_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ali_gpio *ali_gpio = to_ali_gpio(chip);
	int mask = BIT(offset);
	int val = ALI_GPIO_DIR_OUT << offset;
	ali_gpio_set(chip, offset, value);
	ali_set_bits(ali_gpio, OFFSET_TO_REG_SET_DIR, mask, val);

	return 0;
}

static int ali_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	int ret = 0;
	ret = pinctrl_request_gpio(chip->base + offset);
	return ret;
}

static void ali_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	pinctrl_free_gpio(chip->base + offset);
}

static int ali_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct ali_gpio *ali_gpio = to_ali_gpio(chip);
	return irq_create_mapping(ali_gpio->domain, offset);
}

static int ali_gpio_irq_set_type(struct irq_data *data, unsigned int type)
{
	struct ali_gpio *ali_gpio = data->domain->host_data;
	unsigned int offset = irqd_to_hwirq(data);
	int mask = BIT(offset);
	u32 r, m;
	unsigned long flags;

	if (!(type & IRQ_TYPE_EDGE_BOTH)) {
		pr_err("Only falling,rising and both edge supported.\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&ali_gpio->spinlock, flags);

	r = ali_reg_read(ali_gpio, OFFSET_TO_REG_EDGE_RISING);
	m = ali_reg_read(ali_gpio, OFFSET_TO_REG_EDGE_FALLING);

	if (type & IRQ_TYPE_EDGE_RISING)
		ali_reg_write(ali_gpio, OFFSET_TO_REG_EDGE_RISING, r | mask);
	else
		ali_reg_write(ali_gpio, OFFSET_TO_REG_EDGE_RISING, r & (~mask));

	if (type & IRQ_TYPE_EDGE_FALLING)
		ali_reg_write(ali_gpio, OFFSET_TO_REG_EDGE_FALLING, m | mask);
	else
		ali_reg_write(ali_gpio, OFFSET_TO_REG_EDGE_FALLING, m & (~mask));

	spin_unlock_irqrestore(&ali_gpio->spinlock, flags);

	irqd_set_trigger_type(data, type);

	return IRQ_SET_MASK_OK;
}

static irqreturn_t ali_gpio_irq_cascade(int irq, void *data)
{
	struct ali_gpio *ali_gpio = data;
	u32 r = ali_reg_read(ali_gpio, OFFSET_TO_REG_INT_STATUS);
	u32 m = ali_reg_read(ali_gpio, OFFSET_TO_REG_INT_EN);
	const unsigned long bits = r & m;
	int i = 0;

	for_each_set_bit(i, &bits, 32)
		generic_handle_irq(irq_find_mapping(ali_gpio->domain, i));

	return IRQ_HANDLED;
}

static int ali_gpio_interrupt_controller_init(struct platform_device *pdev)
{
	struct ali_gpio *ali_gpio = platform_get_drvdata(pdev);
	struct device_node *dn = pdev->dev.of_node;
	struct irq_chip_generic *gc;
	int ret;

	ali_gpio->irq = platform_get_irq(pdev, 0);
	if (0 > ali_gpio->irq) {
		dev_err(&pdev->dev, "No interrupt specified.\n");
		return -1;
	}

	ret = devm_request_irq(&pdev->dev, ali_gpio->irq, ali_gpio_irq_cascade, 
			       IRQF_TRIGGER_NONE | IRQF_SHARED, dev_name(&pdev->dev), ali_gpio);
	if (ret) {
		dev_err(&pdev->dev, "Request irq(%d) fail.\n", ali_gpio->irq);
		return -1;
	}

	ali_gpio->domain = irq_domain_add_linear(dn, ali_gpio->gc.ngpio, &irq_generic_chip_ops, ali_gpio);
	if (!ali_gpio->domain) {
		dev_err(&pdev->dev, "Alloc linear irq domain fail.\n");
		return -1;
	}

	ret = irq_alloc_domain_generic_chips(ali_gpio->domain, ali_gpio->gc.ngpio, 1, ali_gpio->gc.label, 
					     handle_edge_irq, IRQ_NOREQUEST, IRQ_NOPROBE, IRQ_GC_INIT_MASK_CACHE);
	if (ret) {
		dev_err(&pdev->dev, "Allocate generic chips for an irq domain fail.\n");
		irq_domain_remove(ali_gpio->domain);
		return -1;
	}

	gc = ali_gpio->domain->gc->gc[0];
	gc->reg_base = ali_gpio->base;
	gc->chip_types[0].type = IRQ_TYPE_EDGE_BOTH;
	gc->chip_types[0].chip.irq_ack = irq_gc_ack_set_bit;
	gc->chip_types[0].chip.irq_mask = irq_gc_mask_clr_bit;
	gc->chip_types[0].chip.irq_unmask = irq_gc_mask_set_bit;
	gc->chip_types[0].chip.irq_set_type = ali_gpio_irq_set_type;
	gc->chip_types[0].regs.ack = OFFSET_TO_REG_INT_STATUS;
	gc->chip_types[0].regs.mask = OFFSET_TO_REG_INT_EN;
	return 0;
}

static int ali_gpio_probe(struct platform_device *pdev)
{
	struct ali_gpio *ali_gpio;
	struct resource *mem;
	struct device_node *dn = pdev->dev.of_node;
	int ret;
	u32 ngpio = 0;
	u32 gpio_base = 0;

	if (!dn)
		return -EINVAL;

	if (of_property_read_u32(dn, "ngpio", &ngpio)) {
		dev_err(&pdev->dev, "%s: ngpio property missing\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_u32(dn, "gpio-base", &gpio_base)) {
		dev_err(&pdev->dev, "%s: gpio-base property missing\n", __func__);
		return -EINVAL;
	}

	ali_gpio = devm_kzalloc(&pdev->dev, sizeof (*ali_gpio), GFP_KERNEL);
	if (NULL == ali_gpio)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ali_gpio->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(ali_gpio->base)) {
		return PTR_ERR(ali_gpio->base);
	}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
	ali_gpio->ram_cache_regs = devm_kzalloc(&pdev->dev, resource_size(mem), GFP_KERNEL);
	if (NULL == ali_gpio->ram_cache_regs) {
		return -ENOMEM;
	}
#endif

	ali_gpio->gc.label = of_node_full_name(dn);
	ali_gpio->gc.dev = &pdev->dev;
	ali_gpio->gc.owner = THIS_MODULE;
	ali_gpio->gc.direction_input = ali_gpio_direction_in;
	ali_gpio->gc.get = ali_gpio_get;
	ali_gpio->gc.direction_output = ali_gpio_direction_out;
	ali_gpio->gc.set = ali_gpio_set;
	ali_gpio->gc.request = ali_gpio_request;
	ali_gpio->gc.free = ali_gpio_free;
	ali_gpio->gc.base = gpio_base;
	ali_gpio->gc.ngpio = ngpio;
	ali_gpio->gc.can_sleep = false;

	if (of_find_property(dn, "offsets", NULL)) {
		ret = of_property_read_u32_array(dn, "offsets", ali_gpio->offsets, OFFSETS_SIZE);
		if (ret) {
			dev_err(&pdev->dev, "Offsets property format incorrect.ret=%d\n", ret);
			return ret;
		}
	} else {
		memcpy(ali_gpio->offsets, PAST_OFFSETS, sizeof (ali_gpio->offsets));
	}

	ret = gpiochip_add(&ali_gpio->gc);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not add gpiochip.\n");
		return ret;
	}

	spin_lock_init(&ali_gpio->spinlock);
	platform_set_drvdata(pdev, ali_gpio);

	if (of_find_property(dn, "interrupt-controller", NULL)) {
		ret = ali_gpio_interrupt_controller_init(pdev);
		if (ret) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
			gpiochip_remove(&ali_gpio->gc);
#else
			int ret2;
			ret2 = gpiochip_remove(&ali_gpio->gc);
#endif
			return ret;
		}
		ali_gpio->gc.to_irq = ali_gpio_to_irq;
	}

	return 0;
}

static int __exit ali_gpio_remove(struct platform_device *pdev)
{
	struct ali_gpio *ali_gpio = platform_get_drvdata(pdev);

	if (ali_gpio->gc.to_irq) {
		irq_remove_generic_chip(ali_gpio->domain->gc->gc[0], BIT(ali_gpio->gc.ngpio) - 1, 0, 0);
		kfree(ali_gpio->domain->gc);
		irq_domain_remove(ali_gpio->domain);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	gpiochip_remove(&ali_gpio->gc);
	return 0;
#else
	return gpiochip_remove(&ali_gpio->gc);
#endif
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ali_gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ali_gpio *ali_gpio = platform_get_drvdata(pdev);
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	memcpy_fromio(ali_gpio->ram_cache_regs, ali_gpio->base, resource_size(mem));
	return 0;
}

static int ali_gpio_resume(struct platform_device *pdev)
{
	struct ali_gpio *ali_gpio = platform_get_drvdata(pdev);
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	memcpy_toio(ali_gpio->base, ali_gpio->ram_cache_regs, resource_size(mem));
	return 0;
}
#endif

static const struct of_device_id ali_gpio_dt_ids[] = {
	{.compatible = "alitech,ali-gpio"},
	{}
};

MODULE_DEVICE_TABLE(of, ali_gpio_dt_ids);

static struct platform_driver ali_gpio_driver = {
	.probe = ali_gpio_probe,
	.remove = ali_gpio_remove,
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	.suspend = ali_gpio_suspend,
	.resume = ali_gpio_resume,
#endif
	.driver = {
		.name = "ali-gpio",
		.of_match_table = ali_gpio_dt_ids,
	}
};

static int __init ali_gpio_init(void)
{
	return platform_driver_register(&ali_gpio_driver);
}

arch_initcall(ali_gpio_init);
MODULE_AUTHOR("Dennis Dai,Ziv Gu,Billy Zhou");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ALi GPIO Driver");
MODULE_VERSION("1.1.1");
