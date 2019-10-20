/*
 * Scramber Core driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
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


#include <linux/highmem.h>
#include "ca_scr_sysfs.h"

static ssize_t sysfs_store_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ca_scr_dev *scr = dev_get_drvdata(dev);
	long mode;
	if (kstrtol(buf, 10, &mode))
		return -EINVAL;

	mutex_lock(&scr->mutex);
	scr->mode = (__u32)mode;
	mutex_unlock(&scr->mutex);
	return count;
}

static ssize_t sysfs_show_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_scr_dev *scr = dev_get_drvdata(dev);
	ssize_t count = 0;

	mutex_lock(&scr->mutex);
	count += sprintf(buf, "mode %d\n", scr->mode);
	mutex_unlock(&scr->mutex);
	return count;
}

static ssize_t att_store_info(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static ssize_t att_show_info(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_scr_dev *scr = NULL;
	ssize_t count = 0;

	scr = dev_get_drvdata(dev);
	mutex_lock(&scr->mutex);

	count += sprintf(buf, "total session: %d\n", scr->num_inst);

	mutex_unlock(&scr->mutex);
	return count;
}

static ssize_t att_show_debug_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_scr_dev *scr = NULL;
	ssize_t count = 0;

	scr = dev_get_drvdata(dev);
	mutex_lock(&scr->mutex);

	count += sprintf(buf, "debug_mode: %d\n", scr->debug_mode);

	mutex_unlock(&scr->mutex);
	return count;
}

static ssize_t att_store_debug_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ca_scr_dev *scr = dev_get_drvdata(dev);
	int debug_mode;
	if (kstrtoint(buf, 10, &debug_mode))
		return -EINVAL;

	mutex_lock(&scr->mutex);
	scr->debug_mode = (debug_mode & 0x01);
	mutex_unlock(&scr->mutex);
	return count;
}

#ifdef CONFIG_DEBUG_FS
static ssize_t att_show_not_gothrough_hw(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ca_scr_dev *scr = NULL;
	ssize_t count = 0;

	scr = dev_get_drvdata(dev);
	mutex_lock(&scr->mutex);

	count += sprintf(buf, "not_gothrough_hw: %d\n", scr->not_gothrough_hw);

	mutex_unlock(&scr->mutex);
	return count;
}

static ssize_t att_store_not_gothrough_hw(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ca_scr_dev *scr = dev_get_drvdata(dev);
	int not_gothrough_hw;
	if (kstrtoint(buf, 10, &not_gothrough_hw))
		return -EINVAL;

	mutex_lock(&scr->mutex);
	scr->not_gothrough_hw = (not_gothrough_hw & 0x01);
	mutex_unlock(&scr->mutex);
	return count;
}

static DEVICE_ATTR(not_gothrough_hw, 0666,
	att_show_not_gothrough_hw, att_store_not_gothrough_hw);
#endif

static DEVICE_ATTR(mode, 0660,
	sysfs_show_mode, sysfs_store_mode);
static DEVICE_ATTR(info, 0666,
	att_show_info, att_store_info);
static DEVICE_ATTR(debug_mode, 0666,
	att_show_debug_mode, att_store_debug_mode);

static const struct attribute *sysfs_attrs[] = {
	&dev_attr_mode.attr,
	&dev_attr_info.attr,
	&dev_attr_debug_mode.attr,
#ifdef CONFIG_DEBUG_FS
	&dev_attr_not_gothrough_hw.attr,
#endif
	NULL,
};

int ca_scr_sysfs_create(struct ca_scr_dev *scr)
{
	int ret = sysfs_create_files(&scr->dev->kobj, sysfs_attrs);
	if (ret)
		dev_err(scr->dev, "sysfs create failed\n");

	return ret;
}

void ca_scr_sysfs_remove(struct ca_scr_dev *scr)
{
	sysfs_remove_files(&scr->dev->kobj, sysfs_attrs);
}

