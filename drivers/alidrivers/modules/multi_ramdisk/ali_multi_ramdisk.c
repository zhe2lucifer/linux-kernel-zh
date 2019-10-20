/**@file
 *  (c) Copyright 2013-2999  ALi Corp. ZHA Linux SDK Team (alitech.com)
 *  All rights reserved 
 *
 *  @file               ali_multi_ramdisk.c
 *  @brief              
 *
 *  @version            1.0
 *  @date               08/24/2017 04:55:45 PM
 *  @revision           none
 *
 *  @author             Stephen Xiao <stephen.xiao@alitech.com>
 */
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/memblock.h>

#define MRD_NAME          "ali.mrd"
#define RAMFS2_BUF_NAME   "init_ramfs2_buff"

#define MRD_RAMDISK_DIR "/dev/block"

static int __init create_ramdisk(char *name, int n)
{
	sys_unlink(name);
	sys_mkdir(MRD_RAMDISK_DIR, 0600);
	return sys_mknod(name, S_IFBLK|0600, new_encode_dev(MKDEV(RAMDISK_MAJOR, n)));
}

static int __init init_2nd_ramdisk(void)
{
	int fd_out;
	int ret = -1;
	unsigned long offset = 0, count, start, size;
	struct device_node* node;

	node = of_find_compatible_node(NULL, NULL, "alitech,memory-mapping");
	if (NULL == node) {
		pr_err("alitech,memory-mapping node is null\n");
		return ret;
	}

	of_property_read_u32_index(node, RAMFS2_BUF_NAME, 0, (u32 *)&start);
	of_property_read_u32_index(node, RAMFS2_BUF_NAME, 1, (u32 *)&size);

	ret = create_ramdisk(MRD_RAMDISK_DIR"/ram", 1);
	if (ret) {
		pr_err("create ramdisk %s device %d failed.\n", "/dev/ram", 1);
		return ret;
	}

	fd_out = sys_open(MRD_RAMDISK_DIR"/ram", O_RDWR, 0);
	if (fd_out < 0) 
	{
		pr_err("open %s failed.\n", "/dev/ram");
		goto OUT;
	}

	count = BLOCK_SIZE;
	do {
		if ((offset + BLOCK_SIZE) > size)
		{
				count = size - offset;
		}
		ret = sys_write(fd_out, (char *)(start + offset), count);
		if (ret != count) 
		{
			pr_err("write fail");
			dump_stack();
			break;
		}
		offset += BLOCK_SIZE;
	} while (offset < size);

	free_reserved_area((void *)start, (void*)(start + size), 0, MRD_NAME);
	ret = 0;

OUT:
	sys_close(fd_out);
	sys_unlink(MRD_RAMDISK_DIR"/ram");
	return ret;
}

late_initcall(init_2nd_ramdisk);
