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

#include "ca_kl_sysfs.h"

static ssize_t sysfs_show_global_info(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_kl_sub_dev *pdev = dev_get_drvdata(dev);
	struct ca_kl_sub_dev *son, *_son;
	struct ca_kl_dev *parent = pdev->parent;
	ssize_t count = 0;

	count += sprintf(buf + count, "\n%25s: %d\n",
		"fixed_addr_mode",
		parent->fixed_addr_mode);

	count += sprintf(buf + count, "%25s: %d\n",
		"fixed_engine_mode",
		parent->fixed_eng_mode);

	count += sprintf(buf + count, "%25s: %d\n",
		"one_kl_one_engine_mode",
		parent->one_kl_one_engine_mode);

	count += sprintf(buf + count, "%25s: %d\n",
		"disable_kdf",
		parent->disable_kdf);

	/*iterate the list*/
	count += sprintf(buf + count,
		"\n\troot_key_addr \tsupport_level \tkl_index\n");
	list_for_each_entry_safe(son, _son, &parent->sub_dev_list, sub_dev) {
		count += sprintf(buf + count, "%s:", son->basename);
		count += sprintf(buf + count, "\t\t0x%x \t\t%d \t%02x\n",
			son->root_key_addr,
			son->support_level,
			son->kl_index);
	}

	return count;
}

static ssize_t sysfs_show_individule_info(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_kl_sub_dev *pdev = dev_get_drvdata(dev);
	ssize_t count = 0;
	int idx;

	char name[7][16] = {
		[0] = "AES",
		[1] = "DES",
		[2] = "TDES",
		[3] = "CSA1",
		[4] = "CSA2",
		[5] = "CSA3",
		[6] = "MULTI2",
	};

	count += sprintf(buf + count, "\n%25s: %d\n",
		"level-sel-otp:",
		pdev->level_1st2nd3rd_sel);
	count += sprintf(buf + count, "%25s: %d\n",
		"level-5-enable-otp:",
		pdev->level_5th_en);
	count += sprintf(buf + count, "%25s: %s\n",
		"one-kl-one-engine-algo:",
		name[pdev->one_kl_one_engine_algo]);

	count += sprintf(buf + count, "%25s: 0x%x\n",
		"root_key_addr", pdev->root_key_addr);
	count += sprintf(buf + count, "%25s: %d\n",
		"support_level", pdev->support_level);
	count += sprintf(buf + count, "%25s: %d\n",
		"kl_index", pdev->kl_index);

	count += sprintf(buf + count, "%25s: ",
			"supported engine");
	for (idx = 0; idx < pdev->fixed_engine_algos_num; idx++) {
		count += sprintf(buf + count, "%s ",
			name[pdev->fixed_engine_algos[idx]]);
	}
	count += sprintf(buf + count, "\n");

	return count;
}

static ssize_t kl_show_debug_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_kl_sub_dev *pdev = dev_get_drvdata(dev);
	ssize_t count = 0;

	count += sprintf(buf, "debug_mode: %d\n", pdev->debug_mode);

	return count;
}

static ssize_t kl_store_debug_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ca_kl_sub_dev *pdev = dev_get_drvdata(dev);
	int debug_mode;
	if (kstrtoint(buf, 10, &debug_mode))
		return -EINVAL;

	pdev->debug_mode = (debug_mode & 0x01);

	return count;
}

static DEVICE_ATTR(global_info, 0660,
	sysfs_show_global_info, NULL);
static DEVICE_ATTR(individule_info, 0660,
	sysfs_show_individule_info, NULL);
static DEVICE_ATTR(debug_mode, 0666,
	kl_show_debug_mode, kl_store_debug_mode);


static const struct attribute *sysfs_attrs[] = {
	&dev_attr_global_info.attr,
	&dev_attr_individule_info.attr,
	&dev_attr_debug_mode.attr,
	NULL,
};

int ca_kl_sysfs_create(struct ca_kl_sub_dev *pdev)
{
	int ret = sysfs_create_files(&pdev->dev->kobj, sysfs_attrs);
	if (ret)
		dev_err(pdev->dev, "sysfs create failed\n");

	return ret;
}

void ca_kl_sysfs_remove(struct ca_kl_sub_dev *pdev)
{
	sysfs_remove_files(&pdev->dev->kobj, sysfs_attrs);
}

