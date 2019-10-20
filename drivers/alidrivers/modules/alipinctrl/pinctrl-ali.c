/*
 * pinctrl driver for ALi SoCs
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Dai <david.dai@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/gpio.h>

#include "core.h"
#include "pinctrl-utils.h"

#ifdef CONFIG_ALI_PINCTL_DEBUG
#define ALI_PINCTRL_DEBUG   printk
#else
#define ALI_PINCTRL_DEBUG(fmt...)   do { } while (0)
#endif
#ifdef CONFIG_ALI_STANDBY_TO_RAM
static u32 *ali_save_pmx_registers;
static u32 *ali_save_gpio_registers;
#endif
struct ali_func_mutex {
	const char *name;
	struct mutex mutex;
};

struct ali_pinctrl_head {
	const char *name;
	struct ali_func_mutex *funcmutex;
	unsigned int mcount;
};

struct ali_pinctrl_reg {
	unsigned int offset;
	unsigned int mask;
	unsigned int value;
};

struct ali_pinctrl_group {
	const char *name;
	const unsigned int *pins;
	struct ali_pinctrl_reg *reg;
	unsigned int pcount;
	unsigned int rcount;
};

struct ali_pinctrl_func {
	const char *name;
	const char **groups;
	unsigned int count;
};

struct ali_pinctrl_range {
	const char *name;
	const unsigned int *pins;
	unsigned int count;
	unsigned int size;
	void *base;
	unsigned long phybase;
};

struct ali_pinctrl_private {
	struct ali_pinctrl_head pctrlhead;
	unsigned int phybase;
	void *base;
	unsigned int size;
	struct ali_pinctrl_group *groups;
	unsigned int gcount;
	struct ali_pinctrl_func *funcs;
	unsigned int fcount;
	struct ali_pinctrl_range *range;
	unsigned int rgcount;
	struct mutex mutex;
	struct pinctrl_dev *pctrl;
	unsigned int pinctrl_type;
};

static int ali_get_groups_count(struct pinctrl_dev *pctldev)
{

	struct ali_pinctrl_private *state = pinctrl_dev_get_drvdata(pctldev);

	return state->gcount;
}

static const char *ali_get_group_name(struct pinctrl_dev *pctldev,
				      unsigned group)
{
	struct ali_pinctrl_private *state = pinctrl_dev_get_drvdata(pctldev);

	return state->groups[group].name;
}

static int ali_get_group_pins(struct pinctrl_dev *pctldev, unsigned group,
			      const unsigned **pins, unsigned *num_pins)
{
	struct ali_pinctrl_private *state = pinctrl_dev_get_drvdata(pctldev);

	*pins = state->groups[group].pins;
	*num_pins = state->groups[group].pcount;

	return 0;
}

#ifdef CONFIG_DEBUG_FS
static void ali_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
			     unsigned offset)
{
	seq_puts(s, " ali_pinctrl");
}
#endif

static int ali_dt_node_to_map(struct pinctrl_dev *pctldev,
			      struct device_node *np, struct pinctrl_map **map,
			      unsigned *num_maps)
{
	struct property *prop;
	const char *function;
	const char *group;
	unsigned reserved_maps;
	unsigned reserve;
	int ret;

	ret = of_property_read_string(np, "name", &function);
	if (ret < 0) {
		dev_err(pctldev->dev,
			 "%s could not parse node:%s <name> property\n",
			 __func__, np->name);
		function = NULL;
	}

	ret = of_property_count_strings(np, "groups");
	if (ret < 0) {
		dev_err(pctldev->dev,
			"%s could not parse node:%s <groups> property\n",
			__func__, np->name);
		return ret;
	}

	reserve = ret;
	*map = NULL;
	*num_maps = 0;
	reserved_maps = 0;
	ret = pinctrl_utils_reserve_map(pctldev,
				 map, &reserved_maps, num_maps, reserve);
	if (ret < 0) {
		dev_err(pctldev->dev,
			"%s pinctrl_utils_reserve_map fail for %s\n",
			__func__, np->name);
		return ret;
	}

	of_property_for_each_string(np, "groups", prop, group) {
		ret = pinctrl_utils_add_map_mux(pctldev, map, &reserved_maps,
						 num_maps, group, function);
		if (ret < 0) {
			dev_err(pctldev->dev,
			"%s <pinctrl_utils_add_map_mux> fail for %s:%s\n",
			 __func__, function, group);
			return ret;
		}
	}

	return 0;
}

static struct pinctrl_ops ali_pinctrl_ops = {
	.get_groups_count = ali_get_groups_count,
	.get_group_name	  = ali_get_group_name,
	.get_group_pins   = ali_get_group_pins,
	.dt_node_to_map   = ali_dt_node_to_map,
	.dt_free_map      = pinctrl_utils_dt_free_map,
#ifdef CONFIG_DEBUG_FS
	.pin_dbg_show     = ali_pin_dbg_show,
#endif
};

static int ali_pinctrl_set_mux(struct pinctrl_dev *pctldev, unsigned func,
			       unsigned group)
{
	struct ali_pinctrl_private *state;
	int i;
	u32 value;

	state = pinctrl_dev_get_drvdata(pctldev);
	if (func >= state->fcount || group >= state->gcount)
		return -EINVAL;
	ALI_PINCTRL_DEBUG("[%s],func=%d,group=%d\n", __func__, func, group);
	mutex_lock(&state->mutex);

	for (i = 0; i < state->groups[group].rcount; i++) {
		value = ioread32(state->base +
		state->groups[group].reg[i].offset);
		ALI_PINCTRL_DEBUG("[%s],value=%x,i=%d\n", __func__, value, i);
		value &= ~state->groups[group].reg[i].mask;
		value |= state->groups[group].reg[i].value;
		iowrite32(value,
			state->base + state->groups[group].reg[i].offset);
		ALI_PINCTRL_DEBUG("[%s],value=%x\n", __func__, value);
	}

	mutex_unlock(&state->mutex);

	return 0;
}

static int ali_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct ali_pinctrl_private *state = pinctrl_dev_get_drvdata(pctldev);

	return state->fcount;
}

static const char *ali_get_function_name(struct pinctrl_dev *pctldev,
					 unsigned func)
{
	struct ali_pinctrl_private *state = pinctrl_dev_get_drvdata(pctldev);

	return state->funcs[func].name;
}

static int ali_get_function_groups(struct pinctrl_dev *pctldev, unsigned func,
				   const char * const **groups,
				   unsigned * const num_groups)
{
	struct ali_pinctrl_private *state = pinctrl_dev_get_drvdata(pctldev);

	*groups = state->funcs[func].groups;
	*num_groups = state->funcs[func].count;

	return 0;
}

static int ali_gpio_request_enable(struct pinctrl_dev *pctldev,
				   struct pinctrl_gpio_range *range,
				   unsigned pin)
{
	struct ali_pinctrl_private *state;
	int i, j;
	u32 value;

	ALI_PINCTRL_DEBUG("[%s],pin=%d\n", __func__, pin);
	state = pinctrl_dev_get_drvdata(pctldev);

	for (i = 0; i < state->rgcount; i++) {
		for (j = 0; j < state->range[i].count; j++) {
			if (state->range[i].pins[j] == pin) {
				mutex_lock(&state->mutex);
				if(state->pinctrl_type==1){
				    value = ioread32(state->range[i].base+j*4);
				    value &= 0xff00;
				    iowrite32(value, state->range[i].base+j*4);
				}else{
				    value = ioread32(state->range[i].base);
				    value |= 1 << j;
				    iowrite32(value, state->range[i].base);
				}
				mutex_unlock(&state->mutex);
				return 0;
			}
		}
	}

	return -EINVAL;
}

static void ali_gpio_disable_free(struct pinctrl_dev *pctldev,
				  struct pinctrl_gpio_range *range,
				  unsigned pin)
{
	struct ali_pinctrl_private *state;
	int i, j;
	u32 value;

	ALI_PINCTRL_DEBUG("[%s],pin=%d\n", __func__, pin);
	state = pinctrl_dev_get_drvdata(pctldev);

	for (i = 0; i < state->rgcount; i++) {
		for (j = 0; j < state->range[i].count; j++) {
			if (state->range[i].pins[j] == pin) {
				mutex_lock(&state->mutex);
				if(state->pinctrl_type==1){
				    value = ioread32(state->range[i].base+j);
				    value |= 0xff;
				    iowrite32(value, state->range[i].base+j);
  				}else{
				    value = ioread32(state->range[i].base);
				    value &= ~(1 << j);
				    iowrite32(value, state->range[i].base);
				}
				mutex_unlock(&state->mutex);
				return;
			}
		}
	}
}

static struct pinmux_ops ali_pinmux_ops = {
	.get_functions_count = ali_get_functions_count,
	.get_function_name   = ali_get_function_name,
	.get_function_groups = ali_get_function_groups,
	.enable             = ali_pinctrl_set_mux,
	.gpio_request_enable = ali_gpio_request_enable,
	.gpio_disable_free   = ali_gpio_disable_free,
};

static struct pinctrl_desc ali_pindesc = {
	.name    = "ali_pindesc",
	.owner   = THIS_MODULE,
	.pctlops = &ali_pinctrl_ops,
	.pmxops  = &ali_pinmux_ops,
};

static struct ali_pinctrl_group *of_ali_name_to_group(
				struct platform_device *pdev,
				const char *name)
{
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);
	struct ali_pinctrl_group *group = state->groups;
	int i;

	if (!name)
		return NULL;

	for (i = 0; i < state->gcount; i++) {
		if (strcmp(name, group->name) == 0)
			return group;

		group++;
		continue;
	}

	return NULL;
}

static int ali_pinctrl_dt_to_funcdb(struct platform_device *pdev)
{
	struct device_node *np, *child, *cchild;
	struct device *dev;
	struct ali_pinctrl_func *func;
	struct ali_pinctrl_group *group;
	const char **grpp;
	char *name;
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);

	int total = 0, i, j;

	dev = &pdev->dev;
	np = pdev->dev.of_node;

	child = of_get_child_by_name(np, "functions");
	if (!child) {
		dev_err(dev, "Don't find child <functions> of DTS node<%s>!\n",
			np->name);
		return -ENODEV;
	}

	/* Get all functions */
	state->fcount = of_get_child_count(child);
	func = devm_kzalloc(dev, sizeof(*func)*state->fcount, GFP_KERNEL);
	if (!func) {
		dev_err(dev, "Allocate memory for func database failed!\n");
		return -ENOMEM;
	}

	state->funcs = func;
	/* Travers all children of "functions" node */
	for_each_child_of_node(child, cchild) {
		/* Get group member for this child */
		total += of_property_count_strings(cchild, "groups");
	}

	/* Get memory for saving pointer to group name */
	grpp = devm_kzalloc(dev, sizeof(char *) * total, GFP_KERNEL);
	if (!grpp)
		return -ENOMEM;

	/* Get memory for saving pointer to group name */
	for_each_child_of_node(child, cchild) {
		if (of_property_read_string(cchild, "name", &func->name)) {
			dev_err(dev, "Get DT node:%s <name> failed!\n",
			 cchild->name);
			return -EINVAL;
		}

		func->groups = grpp;
		func->count = of_property_count_strings(cchild, "groups");
		for (i = 0; i < func->count; i++) {
			if (of_property_read_string_index(cchild,
				"groups", i, &func->groups[i])) {
				dev_err(dev,
				"<%s> property <groups> index %d err!\n",
				cchild->name, i);
				return -EINVAL;
			}

			group = of_ali_name_to_group(pdev, func->groups[i]);
			if (!group) {
				dev_err(dev,
				"%s:DT node <%s> not find group <%s>!\n",
				__func__, cchild->name, func->groups[i]);
				return -EINVAL;
			}

			name = kasprintf(GFP_KERNEL, "field-value-%d", i);
			if (!name) {
				dev_err(dev,
				"%s:DT node <%s> <kasprintf> failed!\n",
				 __func__, cchild->name);
				return -EINVAL;
			}

			/* Save reg-value to group reg */
			for (j = 0; j < group->rcount; j++) {
				if (of_property_read_u32_index(cchild,
				 name, j, &group->reg[j].value)) {
					dev_err(dev,
			"%s:DT node <%s> property <%s>  <%d> value failed!\n",
			__func__, cchild->name, name, j);
					kfree(name);
					return -EINVAL;
				}
			}
			kfree(name);
		}
		grpp += func->count;
		func++;
	}

	return 0;
}

static int ali_pinctrl_dt_get_grpinfo(struct platform_device *pdev,
				      struct ali_pinctrl_group *group)
{
	struct device_node *np, *child, *cchild;
	struct device *dev;
	unsigned int *pins;
	struct ali_pinctrl_reg *reg;
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);

	int  childcnt = 0, regcnt = 0, pincnt = 0;
	int i, lngth;

	dev = &pdev->dev;
	np = pdev->dev.of_node;
	child = of_get_child_by_name(np, "groups");

	/* get total pins and regs of all child nodes */
	for_each_child_of_node(child, cchild) {
		lngth = 0;
		if (!of_find_property(cchild, "pins", &lngth)) {
			dev_err(dev, "%s:Get DT node:%s <pins> failed!\n",
				 __func__, cchild->name);
		}
		pincnt += lngth/sizeof(__be32);
		if (!of_find_property(cchild, "reg-offset", &lngth)) {
			dev_err(dev,
				 "%s:Get DT node:%s <reg-offset> failed!\n",
				 __func__, cchild->name);
		}
		regcnt += lngth/sizeof(__be32);
	}

	/* Get memory for saving all pins, which come from all pin groups */
	pins = devm_kzalloc(dev, sizeof(unsigned int)*pincnt, GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	/* Get memory for saving all reg, which come from all pin groups */
	reg = devm_kzalloc(dev, sizeof(*reg)*regcnt, GFP_KERNEL);
	if (!reg)
		return -ENOMEM;

	/* Travers all pin group nodes */
	for_each_child_of_node(child, cchild) {
		lngth = 0;
		/* 1.get pin group name */
		if (of_property_read_string(cchild, "name", &group->name)) {
			dev_err(dev, "%s:Get DT node:%s <name> failed!\n",
				 __func__, cchild->name);
			return -EINVAL;
		}

		/* 2.get pin member of pin group */
		if (!of_find_property(cchild, "pins", &lngth)) {
			dev_err(dev, "%s:Get DT node:%s <pins> failed!\n",
				 __func__, cchild->name);
			return -EINVAL;
		}
		group->pcount = lngth/sizeof(__be32);
		if (group->pcount == 0) {
			dev_err(dev, "%s:Get DT node:%s <pins> failed!\n",
				 __func__, cchild->name);
			return -EINVAL;
		}
		/* 3.get pin number of pin group */
		of_property_read_u32_array(cchild, "pins", pins,
						group->pcount);

		/* 4.get reg-offset of pin group */
		if (of_find_property(cchild, "reg-offset", &lngth)) {

			/* Find group->rcount offsets */
			group->rcount = lngth/sizeof(__be32);
			for (i = 0; i < group->rcount; i++) {

				/* Get values of these offsets */
				of_property_read_u32_index(cchild,
					 "reg-offset", i, &reg[i].offset);

				if (state->size <= reg[i].offset) {
					dev_err(dev,
					 "%s:DT node:%s offset overflow\n",
					 __func__, cchild->name);
					return -EINVAL;
				}

				/* Each offset must have a mask */
				if (of_property_read_u32_index(cchild,
					 "reg-mask", i, &reg[i].mask)) {
					dev_err(dev,
				"%s:DT node:%s <reg-offset> format err!\n",
				 __func__, cchild->name);
					return -EINVAL;
				}
			}
		} else {
			dev_err(dev,
				"%s:Get DT node:%s <reg-offset> failed!\n",
				 __func__, cchild->name);
			return -EINVAL;
		}
		group->pins = pins;
		group->reg = reg;

		pins += group->pcount;
		reg += group->rcount;
		childcnt++;
		group++;
	}

	return childcnt;
}

static int ali_pinctrl_dt_get_gpioinfo(struct platform_device *pdev,
				       struct ali_pinctrl_group *group)
{
	struct device_node *np, *child, *cchild;
	struct device *dev;
	unsigned int *pins;
	struct ali_pinctrl_range *range;
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);

	int  childcnt = 0, pincnt = 0;
	int lngth;

	dev = &pdev->dev;
	np = pdev->dev.of_node;

	/* 1.get gpio-groups node */
	child = of_get_child_by_name(np, "gpio-groups");
	of_property_read_u32_index(child, "pinctrl-type",0, &state->pinctrl_type);
	/* 2.traverse all subnodes of gpio-groups node */
	for_each_child_of_node(child, cchild) {

		lngth = 0;
		/* Get pin member of this gpio range */
		if (!of_find_property(cchild, "pins", &lngth)) {
			dev_err(dev, "%s DT node %s <pins> failed!\n",
				__func__, cchild->name);
			return -EINVAL;
		}
		pincnt += lngth/sizeof(__be32);

		/* All pins for all gpio ranges */
		childcnt++;
	}

	/* 3.allocate memory for all pins */
	pins = devm_kzalloc(dev, sizeof(unsigned int)*pincnt, GFP_KERNEL);

	/* 4.allocate database memory for maintaining gpio-range */
	state->range = devm_kzalloc(dev,
				 sizeof(*state->range) * childcnt,
				GFP_KERNEL);
	state->rgcount = childcnt;

	if (!pins || !state->range) {
		dev_err(dev, "DT node:%s get memory failed\n", child->name);
		return -ENOMEM;
	}
	range = state->range;

	for_each_child_of_node(child, cchild) {
		lngth = 0;
		of_property_read_string(cchild, "name", &group->name);
		if (!of_find_property(cchild, "pins", &lngth)) {
			dev_err(dev,
				 "DT node:%s <pins> failed!\n",
				 cchild->name);
			return -EINVAL;
		}
		group->pcount = lngth/sizeof(__be32);

		range->name = group->name;
		of_property_read_u32_array(cchild, "pins", pins, group->pcount);
		of_property_read_u32_index(cchild, "reg", 0,
					 (unsigned int *)&range->phybase);
		of_property_read_u32_index(cchild, "reg", 1, &range->size);

		range->pins = group->pins = pins;
		range->count = group->pcount;
		pins += group->pcount;
		group++;
		range++;
	}

	return childcnt;
}

static int ali_pinctrl_dt_to_groupdb(struct platform_device *pdev)
{
	struct device_node *np, *child;
	struct ali_pinctrl_group *group;
	struct device *dev;
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);
	int ret;

	np = pdev->dev.of_node;
	dev = &pdev->dev;

	/* group node */
	child = of_get_child_by_name(np, "groups");
	if (!child) {
		dev_err(dev, "Don't find dts node:%s !\n", "groups");
		return -ENODEV;
	}
	state->gcount = of_get_child_count(child);

	/* gpio-groups node */
	child = of_get_child_by_name(np, "gpio-groups");
	if (!child) {
		dev_err(dev, "Don't find dts node:%s !\n", "gpio-groups");
		return -ENODEV;
	}

	/* total groups */
	state->gcount += of_get_child_count(child);

	/* allocate memory for total groups*/
	group = devm_kzalloc(dev, sizeof(*group)*state->gcount, GFP_KERNEL);
	if (!group)
		return -ENOMEM;

	state->groups = group;

	/* get normal-group information */
	ret = ali_pinctrl_dt_get_grpinfo(pdev, group);
	if (ret < 0)
		return ret;

	if (state->gcount < ret) {
		dev_err(dev, "Memory overflow for group database!\n");
		return -ENOMEM;
	}

	/* get gpio-group information */
	group += ret;
	ret = ali_pinctrl_dt_get_gpioinfo(pdev, group);
	if (ret < 0) {
		dev_err(dev, "Memory overflow for group database!\n");
		return -ENOMEM;
	}

	if ((state->gcount - ret) < 0) {
		dev_err(dev, "Memory overflow for group database!\n");
		return -ENOMEM;
	};

	return 0;
}

static int ali_pinctrl_dt_to_pindb(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev;
	struct pinctrl_pin_desc *pins;
	int i;

	dev =  &pdev->dev;
	ali_pindesc.npins = of_property_count_strings(np, "pins");

	pins = devm_kzalloc(dev,
		sizeof(struct pinctrl_pin_desc) * ali_pindesc.npins,
		GFP_KERNEL);
	if (!pins) {
		dev_err(dev,
			"%s: allocate memory for pin database err!\n",
			 __func__);
		return -ENOMEM;
	}

	ali_pindesc.pins = pins;
	for (i = 0; i < ali_pindesc.npins; pins++, i++) {
		if (of_property_read_string_index(np, "pins", i, &pins->name)) {
			dev_err(dev, "%s: DT node %s does't find pin[%d] name\n",
				 __func__, np->name, i);
			return -EINVAL;
		}
		pins->number = i;
	}

	return 0;
}

static int ali_pinctrl_gpio_rangedb(struct platform_device *pdev)
{
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct resource *res;
	int i, j;

	for (i = 0; i < state->rgcount; i++) {
		/* Total pins <= register width  */
		if (state->range[i].size * 8 < state->range[i].count) {
			dev_err(dev, "%s the pin count of %s overflow!\n",
				 __func__, state->range[i].name);
			return -EINVAL;
		}

		/* Check pin number overflow */
		for (j = 0; j < state->range[i].count; j++) {
			if (state->range[i].pins[j] >= ali_pindesc.npins) {
				dev_err(dev,
				"%s the pin%d number is overflow inside %s\n",
				__func__, j, state->range[i].name);
				return -EINVAL;
			}
		}
	}

	/* Map gpio enable register */
	for (i = 0; i < state->rgcount; i++) {
		res = devm_request_mem_region(dev, state->range[i].phybase,
			 state->range[i].size, state->range[i].name);

		if (!res) {
			dev_err(dev, "%s can't request region for %s\n",
			 __func__, state->range[i].name);
			return -EBUSY;
		}
		state->range[i].base = devm_ioremap(dev,
			 state->range[i].phybase, state->range[i].size);

		if (!state->range[i].base) {
			dev_err(dev, "%s ioremap failed for %s\n",
				 __func__, state->range[i].name);
			return -ENOMEM;
		}
	}
	return 0;
}

static int ali_pinctrl_dt_to_funcmutex(struct platform_device *pdev)
{
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);
	struct device_node *np, *child, *cchild;
	struct ali_func_mutex *p;
	struct device *dev;

	np = pdev->dev.of_node;
	dev = &pdev->dev;

	state->pctrlhead.name =  kasprintf(GFP_KERNEL, "funcmutex");
	state->pctrlhead.funcmutex = NULL;
	state->pctrlhead.mcount = 0;

	/* funcmutex node */
	child = of_get_child_by_name(np, "funcmutex");
	if (!child) {
		kfree(state->pctrlhead.name);
		state->pctrlhead.name = NULL;
		dev_err(dev, "Don't find dts node:%s !\n", "funcmutex");
		return -ENODEV;
	}

	/* total subnodes */
	state->pctrlhead.mcount = of_get_child_count(child);

	/* allocate memory for all mutexs*/
	state->pctrlhead.funcmutex = devm_kzalloc(dev,
	sizeof(struct ali_func_mutex) * state->pctrlhead.mcount, GFP_KERNEL);
	dev_info(dev, "state->pctrlhead.funcmutex=%p",
		state->pctrlhead.funcmutex);
	if (!state->pctrlhead.funcmutex) {
		kfree(state->pctrlhead.name);
		state->pctrlhead.name = NULL;
		return -ENOMEM;
	}

	p =  state->pctrlhead.funcmutex;
	for_each_child_of_node(child, cchild) {
		of_property_read_string(cchild, "name", &p->name);
		mutex_init(&p->mutex);
		p++;
	}

	return 0;
}

static void ali_pinctrl_funcmutex_free(struct platform_device *pdev)
{
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);
	int i;

	if (state->pctrlhead.name != NULL)
		kfree(state->pctrlhead.name);

	for (i = 0; i < state->pctrlhead.mcount; i++)
		mutex_destroy(&state->pctrlhead.funcmutex[i].mutex);

	state->pctrlhead.mcount = 0;
}

static int  ali_pinctrl_private_probe(struct platform_device *pdev)
{
	struct ali_pinctrl_private *state;
	struct device *dev;
	struct resource *res;
	int ret = -EINVAL;

	dev = &pdev->dev;
	ALI_PINCTRL_DEBUG("[%s]line=%d\n", __func__, __LINE__);
	state = devm_kzalloc(dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	platform_set_drvdata(pdev, state);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	state->phybase = res->start;
	state->size = resource_size(res);
	state->base = devm_ioremap_resource(dev, res);

	if (IS_ERR(state->base)) {
		dev_err(dev, "Allocate memory for pinctrl err!\n");
		return PTR_ERR(state->base);
	}
	mutex_init(&state->mutex);

	/* pin database */
	ret = ali_pinctrl_dt_to_pindb(pdev);
	if (ret < 0) {
		dev_err(dev, "Create <pin> database failed!\n");
		goto fail;
	}

	/* group database */
	ret = ali_pinctrl_dt_to_groupdb(pdev);
	if (ret < 0) {
		dev_err(dev, "Create <group> database failed!\n");
		goto fail;
	}

	/* function database */
	ret = ali_pinctrl_dt_to_funcdb(pdev);
	if (ret < 0) {
		dev_err(dev, "Create <function> database failed!\n");
		ret = -EINVAL;
		goto fail;
	}

	/* chech gpio range setting */
	ret = ali_pinctrl_gpio_rangedb(pdev);
	if (ret < 0) {
		dev_err(dev, "Create <gpio> database failed!\n");
		goto fail;
	}

	/* register ali pinctrl */
	state->pctrl = pinctrl_register(&ali_pindesc, dev, state);
	if (!state->pctrl) {
		dev_err(dev, "could not register ALi pinmux driver\n");
		ret = -EINVAL;
		goto fail;
	}

	ali_pinctrl_dt_to_funcmutex(pdev);
	return 0;

fail:
	mutex_destroy(&state->mutex);
	ali_pinctrl_funcmutex_free(pdev);
	return ret;
}

static int  ali_pinctrl_private_remove(struct platform_device *pdev)
{
	struct ali_pinctrl_private *state = platform_get_drvdata(pdev);

	mutex_destroy(&state->mutex);
	ali_pinctrl_funcmutex_free(pdev);
	pinctrl_unregister(state->pctrl);

	return 0;
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ali_pinctrl_suspend(struct device  *pdev)
{
    struct ali_pinctrl_private *state = dev_get_drvdata(pdev);
    int i,j;
    int k=0;

    ali_save_pmx_registers=kmalloc(state->size,GFP_KERNEL);
    ali_save_gpio_registers=kmalloc(state->rgcount*32,GFP_KERNEL);
    for (i = 0; i <(state->size)/4; i++) {
        ali_save_pmx_registers[i] = ioread32(state->base + i*4);
    }
    for (i = 0; i < state->rgcount; i++) {
        if(state->pinctrl_type==1){
            for (j = 0; j < state->range[i].count; j++) {
    		    ali_save_gpio_registers[k] = ioread32(state->range[i].base+j*4);
                k++;
            }
    	}else{
    		ali_save_gpio_registers[k] = ioread32(state->range[i].base);
            k++;
    	}
        
        
    }

	return 0;
}

static int ali_pinctrl_resume(struct device *pdev)
{
    struct ali_pinctrl_private *state = dev_get_drvdata(pdev);
    int i,j;
    int k=0;

    for (i = 0; i <(state->size)/4; i++) {
        iowrite32(ali_save_pmx_registers[i], state->base + i*4);
    }

    for (i = 0; i < state->rgcount; i++) {
        if(state->pinctrl_type==1){
            for (j = 0; j < state->range[i].count; j++) {
    		    iowrite32(ali_save_gpio_registers[k],state->range[i].base+j*4);
                k++;
            }
    	}else{
    		iowrite32(ali_save_gpio_registers[k],state->range[i].base);
            k++;
    	}
    }
    kfree(ali_save_pmx_registers);
    kfree(ali_save_gpio_registers);
	return 0;
}


static const struct dev_pm_ops ali_pinctrl_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ali_pinctrl_suspend, ali_pinctrl_resume)
 };

#endif

static const struct of_device_id ali_pinctrl_dt_ids[] = {
	{ .compatible = "alitech,pinctrl", },
	{},
};
MODULE_DEVICE_TABLE(of, ali_pinctrl_dt_ids);

static struct platform_driver ali_pinctrl_private_driver = {
	.driver = {
		.name = "alitech,pinctrl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ali_pinctrl_dt_ids),
#ifdef CONFIG_ALI_STANDBY_TO_RAM
		.pm	= &ali_pinctrl_pm_ops,
#endif
	},
	.probe = ali_pinctrl_private_probe,
	.remove = ali_pinctrl_private_remove,
};

static int __init ali_pinctrl_init(void)
{
	return platform_driver_register(&ali_pinctrl_private_driver);
}
/*arch_initcall(ali_pinctrl_init);*/
postcore_initcall(ali_pinctrl_init);

MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Ali pinctrl driver");
MODULE_AUTHOR("David Dai <david.dai@alitech.com>");
MODULE_LICENSE("GPL v2");
