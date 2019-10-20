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

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <ali_video_common.h>
#include "ali_fb.h"
#include "ali_gma.h"
#include "ali_vpo.h"
#include "ali_accel.h"

static ssize_t fb_debug_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t fb_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static struct kobject *example_kobj;

static DEVICE_ATTR(debug, S_IRUGO | S_IWUSR, fb_debug_show, fb_debug_store);

static struct attribute *attrs[] = {
	&dev_attr_debug.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static ssize_t fb_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct fb_info *fbinfo = dev_get_drvdata(dev);
    unsigned int len = 0;
	
    if(strcmp(attr->attr.name, "debug") == 0) {
        len += sprintf(&buf[len], "\nshowing fb%d info\n", fbinfo->node);
        len += sprintf(&buf[len], "line_length = %u\n", fbinfo->fix.line_length);
        len += sprintf(&buf[len], "smem_start = 0x%lx\n", fbinfo->fix.smem_start);
    	len += sprintf(&buf[len], "smem_len = %u\n", fbinfo->fix.smem_len);
    	len += sprintf(&buf[len], "xres = %u\n", fbinfo->var.xres);
    	len += sprintf(&buf[len], "yres = %u\n", fbinfo->var.yres);
        len += sprintf(&buf[len], "xres_virtual = %u\n", fbinfo->var.xres_virtual);
    	len += sprintf(&buf[len], "yres_virtual = %u\n", fbinfo->var.yres_virtual);
        len += sprintf(&buf[len], "xoffset = %u\n", fbinfo->var.xoffset);
        len += sprintf(&buf[len], "yoffset = %u\n", fbinfo->var.yoffset);
    	len += sprintf(&buf[len], "bits_per_pixel = %u\n", fbinfo->var.bits_per_pixel);
    	len += sprintf(&buf[len], "red.length = %u\n", fbinfo->var.red.length);
    	len += sprintf(&buf[len], "red.offset = %u\n", fbinfo->var.red.offset);
    	len += sprintf(&buf[len], "green.length = %u\n", fbinfo->var.green.length);
    	len += sprintf(&buf[len], "green.offset = %u\n", fbinfo->var.green.offset);
    	len += sprintf(&buf[len], "blue.length = %u\n", fbinfo->var.blue.length);
    	len += sprintf(&buf[len], "blue.offset = %u\n\n", fbinfo->var.blue.offset);
    }
    
	return len;
}

static ssize_t fb_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct fb_info *fbinfo = dev_get_drvdata(dev);
    struct alifb_info *info = (struct alifb_info *)fbinfo->par;
    unsigned long value = 0, addr = 0, num = 0;
    char *str = NULL;
    int i = 0, j = 0;

    if(strcmp(attr->attr.name, "debug") == 0) {
        if((str = strstr(buf, "alpha="))) {
            if(sscanf(str, "alpha=%lu", &value) == 1) {
                ali_vpo_set_GMA_alpha(info, (unsigned char)value);
            }
        }

        if((str = strstr(buf, "onoff="))) {
            if(sscanf(str, "onoff=%lu", &value) == 1) {
                ali_vpo_show_GMA_layer(info, (int)value);
            }
        }

        if((str = strstr(buf, "rd="))) {
            if(sscanf(str, "rd=0x%lx", &addr) == 1) {
                #ifdef CONFIG_ARM
                addr |= 0xC0000000;
                #else
                addr |= 0xA0000000;
                #endif
                
                if((str = strstr(buf, "num="))) {
                    if(sscanf(str, "num=%lu", &num) == 1) {
                        printk("\n");

                        for(i = 0, j = 0; i < num; i++) {
                            value = *(unsigned int *)addr;

                            if(j == 0) {
                                printk("%08lx %08lx ", addr, value);
                            } else if(j < 3) {
                                printk("%08lx ", value);
                            } else if(j == 3) {
                                printk("%08lx\n", value);
                            }

                            addr += 4;

                            j++;
                            if(j > 3) {
                                j = 0;
                            }
                        }

                        printk("\n");
                    }
                } else {
                    value = *(unsigned int *)addr;
                    printk("%08lx %08lx\n", addr, value);
                }
            }
        }
    }
    
	return count;
}

int ali_fb_sysfs_init(struct fb_info *fb_info)
{
	int retval;

	//example_kobj = kobject_create_and_add("debug", &fb_info->dev->kobj);
	example_kobj = &fb_info->dev->kobj;
	//if(!example_kobj) {
		//return -ENOMEM;
	//}

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(example_kobj, &attr_group);
	if(retval) {
		//kobject_put(example_kobj);
	}

	return retval;
}

void ali_fb_sysfs_exit(struct fb_info *fb_info)
{
	//kobject_put(example_kobj);
	sysfs_remove_group(&fb_info->dev->kobj, &attr_group);

    return;
}

