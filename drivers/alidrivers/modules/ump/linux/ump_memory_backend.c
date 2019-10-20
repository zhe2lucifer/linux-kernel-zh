/*
 * Copyright (C) 2010, 2013-2014, 2016-2017 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
//#define LINUX_PDK
//#define	LINUX_rosetop
#define LINUX_buildroot_S3922


 #ifdef LINUX_PDK
//#include "/tpsa021/usrhome/tommy.chen/p4_zha1/PDK_Release_1.6.2_001_20140912/PDK1.6.2/linux/kernel/alidrivers/include/ali_board_config.h"
//20141121 yashi change the referring path, the path is defined as ALI_INC_DIR in kbuild
#include "ali_board_config.h"

#define	ALi_UMP_DEDICATED_START	(virt_to_phys(__G_ALI_MM_MALI_UMP_MEM_START_ADDR)&0x7FFFFFFF)
#define	ALi_UMP_DEDICATED_SIZE	0	//__G_ALI_MM_MALI_UMP_MEM_SIZE

#endif

#ifdef LINUX_rosetop
#include <../arch/arm/mach-ali/include/mach/reserved_mm.h>
#define	ALi_UMP_DEDICATED_START	0
#define	ALi_UMP_DEDICATED_SIZE	0	//__G_ALI_MM_MALI_UMP_MEM_SIZE

#endif

#ifdef LINUX_buildroot_S3922
#define	ALi_UMP_DEDICATED_START	0
#define	ALi_UMP_DEDICATED_SIZE	0	//__G_ALI_MM_MALI_UMP_MEM_SIZE

#endif


#include <linux/module.h>            /* kernel module definitions */
#include <linux/ioport.h>            /* request_mem_region */

#include "arch/config.h"             /* Configuration for current platform. The symlink for arch is set by Makefile */

#include "ump_osk.h"
#include "ump_kernel_common.h"
#include "ump_kernel_memory_backend_os.h"
#include "ump_kernel_memory_backend_dedicated.h"

// Added for sourcing linux kernel total size, 20140514 by Tommy Chen
extern unsigned long totalram_pages;

/* Configure which dynamic memory allocator to use */
int ump_backend = ARCH_UMP_BACKEND_DEFAULT;
module_param(ump_backend, int, S_IRUGO); /* r--r--r-- */
MODULE_PARM_DESC(ump_backend, "0 = dedicated memory backend (default), 1 = OS memory backend");

/* The base address of the memory block for the dedicated memory backend */
unsigned int ump_memory_address = ARCH_UMP_MEMORY_ADDRESS_DEFAULT;
module_param(ump_memory_address, uint, S_IRUGO); /* r--r--r-- */
MODULE_PARM_DESC(ump_memory_address, "The physical address to map for the dedicated memory backend");

/* The size of the memory block for the dedicated memory backend */
unsigned int ump_memory_size = ARCH_UMP_MEMORY_SIZE_DEFAULT;
module_param(ump_memory_size, uint, S_IRUGO); /* r--r--r-- */
MODULE_PARM_DESC(ump_memory_size, "The size of fixed memory to map in the dedicated memory backend");

ump_memory_backend *ump_memory_backend_create(void)
{
	ump_memory_backend *backend = NULL;

	/* Create the dynamic memory allocator backend */
	if (0 == ump_backend) {
		ump_memory_address = ALi_UMP_DEDICATED_START;
		ump_memory_size = ALi_UMP_DEDICATED_SIZE;
printk("<UMP> Dedicated memory : start_address %08x, size %08x",ump_memory_address,ump_memory_size);
		DBG_MSG(2, ("Using dedicated memory backend\n"));

		DBG_MSG(2, ("Requesting dedicated memory: 0x%08x, size: %u\n", ump_memory_address, ump_memory_size));
		/* Ask the OS if we can use the specified physical memory */
		if (NULL == request_mem_region(ump_memory_address, ump_memory_size, "UMP Memory")) {
			MSG_ERR(("Failed to request memory region (0x%08X - 0x%08X). Is Mali DD already loaded?\n", ump_memory_address, ump_memory_address + ump_memory_size - 1));
			return NULL;
		}
		backend = ump_block_allocator_create(ump_memory_address, ump_memory_size);
	} else if (1 == ump_backend) {
		// Added for sourcing linux kernel total size, 20140514 by Tommy Chen
		ump_memory_size = totalram_pages << PAGE_SHIFT;
		printk("<UMP>Using OS memory backend, allocation limit: %d\n", ump_memory_size);
		DBG_MSG(2, ("Using OS memory backend, allocation limit: %d\n", ump_memory_size));
		backend = ump_os_memory_backend_create(ump_memory_size);
	}

	return backend;
}

void ump_memory_backend_destroy(void)
{
	if (0 == ump_backend) {
		DBG_MSG(2, ("Releasing dedicated memory: 0x%08x\n", ump_memory_address));
		release_mem_region(ump_memory_address, ump_memory_size);
	}
}
