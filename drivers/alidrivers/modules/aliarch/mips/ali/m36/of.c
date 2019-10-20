/*********************************************************************
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version
* 2 of the License, or (at your option) any later version.
**********************************************************************/
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/bootmem.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <linux/memblock.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include "prom.h"

void __init device_tree_init(void)
{
	unsigned long base, size;
	void *fdt_copy;

	if (!initial_boot_params)
		return;

	base = virt_to_phys((void *)initial_boot_params);
	size = be32_to_cpu(initial_boot_params->totalsize);

	/* Before we do anything, lets reserve the dt blob */
	reserve_bootmem(base, size, BOOTMEM_DEFAULT);

	fdt_copy = alloc_bootmem(size);
	memcpy(fdt_copy, initial_boot_params, size);
	initial_boot_params = fdt_copy;

	unflatten_device_tree();

	/* free the space reserved for the dt blob */
//	free_bootmem(base, size);
}

static int __init ali_plat_of_setup(void)
{
	if (!of_have_populated_dt())
		panic("device tree not present");

	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
	return 0;
}

arch_initcall(ali_plat_of_setup);
