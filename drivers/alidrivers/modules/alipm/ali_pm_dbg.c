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
 
#include <linux/debugfs.h>
#include "ali_pm.h"
#include <linux/debugfs.h>
#include "ali_pm.h"
//========================================================================================================//

static struct dentry *ali_pm_debugfs_root;
//========================================================================================================//
#ifdef CONFIG_ALI_STANDBY_TO_RAM
static __s32 ali_pm_cmac_area_size_get(void *data, u64 *val)
{
    //__u32 reg = *(u32 *)data;

    *val = get_default_area_size();
    
    return 0;
}

static __s32 ali_pm_cmac_area_size_set(void *data, u64 val)
{
    //*(u32 *)data = val;

    set_default_area_size((u32)val);
    
    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ali_pm_cmac_area_size_fops, 
    ali_pm_cmac_area_size_get, ali_pm_cmac_area_size_set, "0x%llx\n");

static __s32 ali_pm_calc_cmac(void *data, u64 *val)
{
    *val = (u64)calc_aes_cmac();
    
    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(ali_pm_calc_cmac_fops, 
    ali_pm_calc_cmac, NULL, "0x%llx\n");
#endif

__s32 ali_pm_dbg_exit(void)
{
	debugfs_remove_recursive(ali_pm_debugfs_root);
	return 0;
}

__s32 ali_pm_dbg_init(void)
{
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	struct dentry *fs_entry;

	ali_pm_debugfs_root = debugfs_create_dir("ali_pm", NULL);
	if (!ali_pm_debugfs_root)
	{
		printk("%s,%d  error!!! debugfs_create_dir(ali_pm) failed.\n", __FUNCTION__, __LINE__);
		return(-ENOENT);
	}

	fs_entry = debugfs_create_file("cmac_area_size", 0644, ali_pm_debugfs_root, 
	NULL, &ali_pm_cmac_area_size_fops);
	if (!fs_entry)
	{
		printk("%s,%d  error!!! debugfs_create_u32(cmac_area_size) failed.\n", __FUNCTION__, __LINE__);
		goto Fail;  
	}

	fs_entry = debugfs_create_file("calc_cmac", 0644, ali_pm_debugfs_root, 
	NULL, &ali_pm_calc_cmac_fops);
	if (!fs_entry)
	{
		printk("%s,%d  error!!! debugfs_create_u32(cmac_area_size) failed.\n", __FUNCTION__, __LINE__);
		goto Fail;  
	}
#endif

	return(0);

Fail:
	ali_pm_dbg_exit();
	return(-ENOENT);
}
