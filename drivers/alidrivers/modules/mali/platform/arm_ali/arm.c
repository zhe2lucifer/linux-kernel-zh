/*
 * Copyright (C) 2010, 2012-2017 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for:
 * - Realview Versatile platforms with ARM11 Mpcore and virtex 5.
 * - Versatile Express platforms with ARM Cortex-A9 and virtex 6.
 */
/*ALi platform setting*/

#include "./platform_ali.h"
#include <linux/vmalloc.h>
#include <linux/module.h> /*Added by Allen 20170328, for Thermal freq limitation export function */


//#define ALI_CONFIG_MALI400_MP1


#ifdef	LINUX_PDK

//#include "/tpsa021/usrhome/tommy.chen/p4_zha1/PDK_Release_1.6.2_001_20140912/PDK1.6.2/linux/kernel/alidrivers/include/ali_board_config.h"
//#include <../arch/arm/mach-ali3921/include/mach/ali-s3921.h> //moved to platform_ali.h
#include "ali_board_config.h"

#define	ALi_MALI_DEDICATED_START	(virt_to_phys((const volatile void *)__G_ALI_MM_MALI_DEDICATED_MEM_START_ADDR)&0x7FFFFFFF)
#define	ALi_MALI_DEDICATED_SIZE	0	//__G_ALI_MM_MALI_DEDICATED_MEM_SIZE
#define	ALi_FB0_START	(virt_to_phys((const volatile void *)__G_ALI_MM_FB0_START_ADDR)&0x7FFFFFFF)
//#define	ALi_FB0_START	((__G_ALI_MM_FB0_START_ADDR))
#define	ALi_FB0_SIZE	__G_ALI_MM_FB0_SIZE
#endif

#ifdef LINUX_rosetop

//#include <../arch/arm/mach-ali/include/mach/m3733.h> //moved to platform_ali.h
#include <../arch/arm/mach-ali/include/mach/reserved_mm.h>

#define	ALi_MALI_DEDICATED_START	0	
#define	ALi_MALI_DEDICATED_SIZE	0	//__G_ALI_MM_MALI_DEDICATED_MEM_SIZE
//#define	ALi_FB0_START	((virt_to_phys(__MM_FB0_CMAP_START)+(_MALI_OSK_CPU_PAGE_SIZE-1))&0x7FFFF000)
//#define	ALi_FB0_START	(0x9F400000)
//#define	ALi_FB0_START	(0x8FA00000) //for linux rosetop
#define	ALi_FB0_START	(0x84000000) //for rosetop linux46
//#define	ALi_FB0_START	(vmalloc_to_pfn(0xd0845000))
//#define	ALi_FB0_SIZE	(4*1024*1024)
#define	ALi_FB0_SIZE	(8*640*480)
#endif

#ifdef	LINUX_buildroot_S3922

/*Frame buffer setting*/
#define	ALi_MALI_DEDICATED_START	0	
#define	ALi_MALI_DEDICATED_SIZE	0	//__G_ALI_MM_MALI_DEDICATED_MEM_SIZE

#ifdef	CHIP_S3922
#define	SYSREG_PHY_BASE	0x18000000
#endif

#ifdef	CHIP_S3922C
#define	SYSREG_PHY_BASE	0x18082C00
#endif


#define	SYSREG_PHY_SIZE	0x400

unsigned long	ALi_FB0_START	= (0xA0000000); 
unsigned long	ALi_FB0_SIZE = (8*640*480);


void		*SYSREG_VIRT_BASE ;

//#define	ALI_HWREG_SET_UINT32(val, reg)	((*((volatile u32*)(SYSREG_VIRT_BASE + reg)))=val)
//#define	ALI_HWREG_GET_UINT32(reg)	(*((volatile u32*)(SYSREG_VIRT_BASE + reg)))

#endif


#include <asm/memory.h>
#include <asm/page.h>


#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#include "mali_kernel_linux.h"
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <asm/io.h>
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>

#include "arm_core_scaling.h"
#include "mali_executor.h"

#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)
#include <linux/devfreq_cooling.h>
#include <linux/thermal.h>
#endif


/*ALi's implementation of DFS*/

int ALi_set_freq(int setting_clock_step);
void ALi_report_clock_info(struct mali_gpu_clock **data);
int ALi_get_freq(void);

struct mali_gpu_clock *ALi_clock;

static int mali_core_scaling_enable = 0;

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);
static u32 mali_read_phys(u32 phys_addr);
static void mali_write_phys(u32 phys_addr, u32 value);


#ifdef CONFIG_MALI_DT
int mali_platform_device_init(struct platform_device *device);
int mali_platform_device_deinit(struct platform_device *device);

#else

static void mali_platform_device_release(struct device *device);


#if defined(CONFIG_ARM64)
/* Juno + Mali-450 MP6 in V7 FPGA */
static struct resource mali_gpu_resources_m450_mp6[] = {
	MALI_GPU_RESOURCES_MALI450_MP6_PMU(0x6F040000, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200)
};

static struct resource mali_gpu_resources_m470_mp4[] = {
	MALI_GPU_RESOURCES_MALI470_MP4_PMU(0x6F040000, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200)
};

static struct resource mali_gpu_resources_m470_mp3[] = {
	MALI_GPU_RESOURCES_MALI470_MP3_PMU(0x6F040000, 200, 200, 200, 200, 200, 200, 200, 200, 200)
};

static struct resource mali_gpu_resources_m470_mp2[] = {
	MALI_GPU_RESOURCES_MALI470_MP2_PMU(0x6F040000, 200, 200, 200, 200, 200, 200, 200)
};

static struct resource mali_gpu_resources_m470_mp1[] = {
	MALI_GPU_RESOURCES_MALI470_MP1_PMU(0x6F040000, 200, 200, 200, 200, 200)
};

#else
static struct resource mali_gpu_resources_m450_mp8[] = {
	MALI_GPU_RESOURCES_MALI450_MP8_PMU(0xFC040000, -1, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 68)
};

static struct resource mali_gpu_resources_m450_mp6[] = {
	MALI_GPU_RESOURCES_MALI450_MP6_PMU(0xFC040000, -1, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 70, 68)
};

static struct resource mali_gpu_resources_m450_mp4[] = {
	MALI_GPU_RESOURCES_MALI450_MP4_PMU(0xFC040000, -1, 70, 70, 70, 70, 70, 70, 70, 70, 70, 68)
};

static struct resource mali_gpu_resources_m470_mp4[] = {
	MALI_GPU_RESOURCES_MALI470_MP4_PMU(0xFC040000, -1, 70, 70, 70, 70, 70, 70, 70, 70, 70, 68)
};
#endif /* CONFIG_ARM64 */

/* for ALi implementation*/

static struct resource mali_gpu_resources_m300[] = {
	MALI_GPU_RESOURCES_MALI300_PMU(0xC0000000, -1, -1, -1, -1)
};
/*
static struct resource mali_gpu_resources_m400_mp1[] = {
	MALI_GPU_RESOURCES_MALI400_MP1_PMU(0xC0000000, -1, -1, -1, -1)
};

static struct resource mali_gpu_resources_m400_mp2[] = {
	MALI_GPU_RESOURCES_MALI400_MP2_PMU(0xC0000000, -1, -1, -1, -1, -1, -1)
};
*/
#ifdef	CHIP_C3921
#ifdef	ALI_CONFIG_MALI400_MP1
static struct resource mali_gpu_resources_m400_mp1[] = {
	//MALI_GPU_RESOURCES_MALI400_MP1(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP1(MALI_IO_BASE, 110, 109, 107, 106)
};
#endif

static struct resource mali_gpu_resources_m400_mp2_pmu[] = {
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, 110, 109, 107, 106, 105, 104)
};

#if 0
static struct resource mali_gpu_resources_m400_mp2[] = {
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP2(MALI_IO_BASE, 110, 109, 107, 106, 105, 104)
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, -1, -1, -1, -1, -1, -1)	
};
#endif

#endif
#ifdef	CHIP_S3922
#ifdef	ALI_CONFIG_MALI400_MP1
static struct resource mali_gpu_resources_m400_mp1[] = {
	//MALI_GPU_RESOURCES_MALI400_MP1(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP1(MALI_IO_BASE, 40, 39, 47, 46)
};
#endif

static struct resource mali_gpu_resources_m400_mp2_pmu[] = {
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, 40, 39, 47, 46, 45, 42)	
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, -1, -1, -1, -1, -1, -1)	
};

#if 0 
static struct resource mali_gpu_resources_m400_mp2[] = {
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP2(MALI_IO_BASE, 40, 39, 47, 46, 45, 42)	
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, -1, -1, -1, -1, -1, -1)	
};
#endif


#endif

#ifdef	CHIP_S3922C
#ifdef	ALI_CONFIG_MALI400_MP1
static struct resource mali_gpu_resources_m400_mp1[] = {
	//MALI_GPU_RESOURCES_MALI400_MP1(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP1(MALI_IO_BASE, 116+32, 39, 47, 46)
};
#endif

static struct resource mali_gpu_resources_m400_mp2_pmu[] = {
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, 148, 149, 151, 145, 146, 147)	
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, -1, -1, -1, -1, -1, -1)	
};

/*
static struct resource mali_gpu_resources_m400_mp2[] = {
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(base_addr, gp_irq, gp_mmu_irq, pp0_irq, pp0_mmu_irq, pp1_irq, pp1_mmu_irq) 
	MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, 148, 149, 151, 145, 146, 147)	
	//MALI_GPU_RESOURCES_MALI400_MP2_PMU(MALI_IO_BASE, -1, -1, -1, -1, -1, -1)	
};
*/

#endif

#endif /*CONFIG_MALI_DT*/


#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)

#define FALLBACK_STATIC_TEMPERATURE 55000

static struct thermal_zone_device *gpu_tz;

/* Calculate gpu static power example for reference */
static unsigned long arm_model_static_power(unsigned long voltage)
{
	int temperature, temp;
	int temp_squared, temp_cubed, temp_scaling_factor;
	const unsigned long coefficient = (410UL << 20) / (729000000UL >> 10);
	const unsigned long voltage_cubed = (voltage * voltage * voltage) >> 10;
	unsigned long static_power;

	if (gpu_tz) {
		int ret;

		ret = gpu_tz->ops->get_temp(gpu_tz, &temperature);
		if (ret) {
			MALI_DEBUG_PRINT(2, ("Error reading temperature for gpu thermal zone: %d\n", ret));
			temperature = FALLBACK_STATIC_TEMPERATURE;
		}
	} else {
		temperature = FALLBACK_STATIC_TEMPERATURE;
	}

	/* Calculate the temperature scaling factor. To be applied to the
	 * voltage scaled power.
	 */
	temp = temperature / 1000;
	temp_squared = temp * temp;
	temp_cubed = temp_squared * temp;
	temp_scaling_factor =
		(2 * temp_cubed)
		- (80 * temp_squared)
		+ (4700 * temp)
		+ 32000;

	static_power = (((coefficient * voltage_cubed) >> 20)
			* temp_scaling_factor)
		       / 1000000;

	return static_power;
}

/* Calculate gpu dynamic power example for reference */
static unsigned long arm_model_dynamic_power(unsigned long freq,
		unsigned long voltage)
{
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 *
	 * This function calculates the dynamic power after this formula:
	 * Pdyn (mW) = c (mW/(MHz*mV*mV)) * v (mV) * v (mV) * f (MHz)
	 */
	const unsigned long v2 = (voltage * voltage) / 1000; /* m*(V*V) */
	const unsigned long f_mhz = freq / 1000000; /* MHz */
	const unsigned long coefficient = 3600; /* mW/(MHz*mV*mV) */
	unsigned long dynamic_power;

	dynamic_power = (coefficient * v2 * f_mhz) / 1000000; /* mW */

	return dynamic_power;
}

struct devfreq_cooling_power arm_cooling_ops = {
	.get_static_power = arm_model_static_power,
	.get_dynamic_power = arm_model_dynamic_power,
};
#endif

static struct mali_gpu_device_data mali_gpu_data = {
#ifndef CONFIG_MALI_DT
	.pmu_switch_delay = 0xFF, /* do not have to be this high on FPGA, but it is good for testing to have a delay */
	.shared_mem_size = 256 * 1024 * 1024, /* 256MB */
#endif
	.max_job_runtime = 60000, /* 60 seconds */

/* ALi implementation*/
//	.dedicated_mem_start = 0x10708000, /* Physical start address (use 0xD0000000 for old indirect setup) */
//	.dedicated_mem_size = 0x4000000, /* 256MB */
//	.dedicated_mem_start = 0x00000000, /* Physical start address (use 0xD0000000 for old indirect setup) */
//	.dedicated_mem_size = 0x00000000, /* 256MB */


#if defined(CONFIG_ARM64)
	/* Some framebuffer drivers get the framebuffer dynamically, such as through GEM,
	* in which the memory resource can't be predicted in advance.
	*/
	.fb_start = 0x0,
	.fb_size = 0xFFFFF000,
#else
/* ALi implementation*/
//	.fb_start = 0xe0000000,
//	.fb_size = 0x01000000,
	.fb_start = 0x10000000,
	.fb_size = 0x20000,
#endif
	.control_interval = 1000, /* 1000ms */
	.utilization_callback = mali_gpu_utilization_callback,
	.get_clock_info = ALi_report_clock_info,
	.get_freq = ALi_get_freq,
	.set_freq = ALi_set_freq,
#if defined(CONFIG_ARCH_VEXPRESS) && defined(CONFIG_ARM64)
	.secure_mode_init = mali_secure_mode_init_juno,
	.secure_mode_deinit = mali_secure_mode_deinit_juno,
	.gpu_reset_and_secure_mode_enable = mali_gpu_reset_and_secure_mode_enable_juno,
	.gpu_reset_and_secure_mode_disable = mali_gpu_reset_and_secure_mode_disable_juno,
#else
	.secure_mode_init = NULL,
	.secure_mode_deinit = NULL,
	.gpu_reset_and_secure_mode_enable = NULL,
	.gpu_reset_and_secure_mode_disable = NULL,
#endif
#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)
	.gpu_cooling_ops = &arm_cooling_ops,
#endif
};

#ifndef CONFIG_MALI_DT
static struct platform_device mali_gpu_device = {
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
	.dev.release = mali_platform_device_release,
	.dev.dma_mask = &mali_gpu_device.dev.coherent_dma_mask,
	.dev.coherent_dma_mask = DMA_BIT_MASK(32),

	.dev.platform_data = &mali_gpu_data,
};

int mali_platform_device_register(void)
{
	int err = -1;
	int num_pp_cores = 0;
	u32 m400_gp_version;

MALI_DEBUG_PRINT(2, ("<ALi_DEBUG>,%s called\n", __FUNCTION__));
	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

	/* Detect present Mali GPU and connect the correct resources to the device */

#if defined(CONFIG_ARM64)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
	mali_gpu_device.dev.archdata.dma_ops = &dummy_dma_ops;
#else
	mali_gpu_device.dev.archdata.dma_ops = dma_ops;
#endif
	if ((mali_read_phys(0x6F000000) & 0x00600450) == 0x00600450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP6 device\n"));
		num_pp_cores = 6;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp6);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp6;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00400430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP4 device\n"));
		num_pp_cores = 4;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m470_mp4);
		mali_gpu_device.resource = mali_gpu_resources_m470_mp4;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00300430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP3 device\n"));
		num_pp_cores = 3;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m470_mp3);
		mali_gpu_device.resource = mali_gpu_resources_m470_mp3;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00200430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP2 device\n"));
		num_pp_cores = 2;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m470_mp2);
		mali_gpu_device.resource = mali_gpu_resources_m470_mp2;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00100430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP1 device\n"));
		num_pp_cores = 1;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m470_mp1);
		mali_gpu_device.resource = mali_gpu_resources_m470_mp1;
	}
#else
	if (mali_read_phys(0xFC000000) == 0x00000450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP8 device\n"));
		num_pp_cores = 8;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp8);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp8;
	} else if (mali_read_phys(0xFC000000) == 0x40600450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP6 device\n"));
		num_pp_cores = 6;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp6);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp6;
	} else if (mali_read_phys(0xFC000000) == 0x40400450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP4 device\n"));
		num_pp_cores = 4;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp4);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp4;
	} else if (mali_read_phys(0xFC000000) == 0xFFFFFFFF) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP4 device\n"));
		num_pp_cores = 4;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m470_mp4);
		mali_gpu_device.resource = mali_gpu_resources_m470_mp4;
	}
#endif /* CONFIG_ARM64 */



	m400_gp_version = mali_read_phys(MALI_IO_BASE+0x6C);
	if ((m400_gp_version & 0xFFFF0000) == 0x0C070000) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-300 device\n"));
		num_pp_cores = 1;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m300);
		mali_gpu_device.resource = mali_gpu_resources_m300;
		mali_write_phys(0xC0010020, 0xA); /* Enable direct memory mapping for FPGA */
	} else if ((m400_gp_version & 0xFFFF0000) == 0x0B070000) {
#if 0
		u32 fpga_fw_version = mali_read_phys(0x19010000);
		if (fpga_fw_version == 0x130C008F || fpga_fw_version == 0x110C008F) {
			/* Mali-400 MP1 r1p0 or r1p1 */
			MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP1 device\n"));
			num_pp_cores = 1;
			mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m400_mp1);
			mali_gpu_device.resource = mali_gpu_resources_m400_mp1;
			mali_write_phys(0x19010020, 0xA); /* Enable direct memory mapping for FPGA */
		} else if (fpga_fw_version == 0x130C000F) {
			/* Mali-400 MP2 r1p1 */
			MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP2 device\n"));
			num_pp_cores = 2;
			mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m400_mp2);
			mali_gpu_device.resource = mali_gpu_resources_m400_mp2;
			mali_write_phys(0x19010020, 0xA); /* Enable direct memory mapping for FPGA */
		}
#else
			/* Mali-400 MP2 r1p1 */
			MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP2 device\n"));
	#ifdef ALI_CONFIG_MALI400_MP1
			num_pp_cores = 1;
			err = 0;
			//err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources_m400_mp1_pmu, sizeof(mali_gpu_resources_m400_mp1_pmu) / sizeof(mali_gpu_resources_m400_mp1_pmu[0]));
			mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m400_mp1);
			mali_gpu_device.resource = mali_gpu_resources_m400_mp1;
	#else
			num_pp_cores = 2;
			err = 0;
			//err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources_m400_mp2_pmu, sizeof(mali_gpu_resources_m400_mp2_pmu) / sizeof(mali_gpu_resources_m400_mp2_pmu[0]));			
			mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m400_mp2_pmu);
			mali_gpu_device.resource = mali_gpu_resources_m400_mp2_pmu;
	#endif						
			
			MALI_DEBUG_PRINT(2, ("<ALi_DEBUG>Mali-400 MP2 device %d core(s) Registered \n", num_pp_cores));

#endif /*if 0*/
	}else{
		MALI_DEBUG_PRINT(4, ("Unknown Mali version\n"));
	}

	if (0 == err) 
	{
		mali_gpu_data.shared_mem_size = totalram_pages << PAGE_SHIFT; /* get from linux kernel*/
		printk("<ALi><Mali> shared_mem_size : %08x \n", (unsigned int)mali_gpu_data.shared_mem_size);
		//20131016 yashi add for dedicated memory
		mali_gpu_data.dedicated_mem_start = ALi_MALI_DEDICATED_START;
		mali_gpu_data.dedicated_mem_size = ALi_MALI_DEDICATED_SIZE;
		
		//20131008 yashi add for dedicated memory
		mali_gpu_data.fb_start =  ALi_FB0_START;
		mali_gpu_data.fb_size = ALi_FB0_SIZE;
		printk("<ALi><Mali>in %s:%4d , dedicated_mem_start = %x, dedicated_mem_size= %x, fb_start = %x, fb_size = %x \n", 
	__func__, __LINE__, (unsigned int)mali_gpu_data.dedicated_mem_start, (unsigned int)mali_gpu_data.dedicated_mem_size, (unsigned int)mali_gpu_data.fb_start, (unsigned int)mali_gpu_data.fb_size);


/*printk("<ALi><Mali> mali_gpu_device : %08x mali_gpu_data : %08x\n", (unsigned int)&mali_gpu_device, (unsigned int)&mali_gpu_data);*/
//err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));

		if (0 == err)
		{
			mali_gpu_device.dev.platform_data = &mali_gpu_data;
	/* Register the platform device */
	err = platform_device_register(&mali_gpu_device);
	if (0 == err) {
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
		pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
		pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
		pm_runtime_enable(&(mali_gpu_device.dev));
#endif
		MALI_DEBUG_ASSERT(0 < num_pp_cores);
		mali_core_scaling_init(num_pp_cores);

		return 0;
	}
		}

		platform_device_unregister(&mali_gpu_device);
	}

	return err;
}

void mali_platform_device_unregister(void)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

	mali_core_scaling_term();
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&(mali_gpu_device.dev));
#endif
	platform_device_unregister(&mali_gpu_device);

	platform_device_put(&mali_gpu_device);

	//mali_write_phys(0x19010020, 0x9); /* Restore default (legacy) memory mapping */

}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}

#else /* CONFIG_MALI_DT */
int mali_platform_device_init(struct platform_device *device)
{
	int num_pp_cores = 0;
	int err = -1;
	u32 m400_gp_version;

	printk("<ALi><Mali> LINUX_VERSION_CODE = %x\n", LINUX_VERSION_CODE);
	

#ifdef LINUX_buildroot_S3922	
	/*20161118 Yashi add of_parsing*/
	{
		struct device_node* node;
		node = of_find_compatible_node(NULL, NULL, "alitech,memory-mapping");
		if(NULL == node)
		{
			printk("<ALi><Mali>alitech,memory-mapping node is null\n");
			return -1;
		}
		of_property_read_u32_index(node, "fb0_buff", 0,(u32 *)&ALi_FB0_START);
		of_property_read_u32_index(node, "fb0_buff", 1,(u32 *)&ALi_FB0_SIZE);		
		
		printk("<ALi><Mali>fb0_buff[0] =%x, fb0_buff[1] =%x \n", (unsigned int)ALi_FB0_START, (unsigned int)ALi_FB0_SIZE);

		/*<issue_GPU_20161216> yashi- add &0x7FFFFFFF mask to fit addr from CPU*/
		ALi_FB0_START &= 0x7FFFFFFF;
		printk("<ALi><Mali>setting ALi_FB0_START = %x\n", (unsigned int)ALi_FB0_START);
	}
#endif

	/* Detect present Mali GPU and connect the correct resources to the device */
/*
#if defined(CONFIG_ARM64)
	if ((mali_read_phys(0x6F000000) & 0x00600450) == 0x00600450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP6 device\n"));
		num_pp_cores = 6;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00400430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP4 device\n"));
		num_pp_cores = 4;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00300430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP3 device\n"));
		num_pp_cores = 3;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00200430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP2 device\n"));
		num_pp_cores = 2;
	} else if ((mali_read_phys(0x6F000000) & 0x00F00430) == 0x00100430) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP1 device\n"));
		num_pp_cores = 1;
	}
#else
	if (mali_read_phys(0xFC000000) == 0x00000450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP8 device\n"));
		num_pp_cores = 8;
	} else if (mali_read_phys(0xFC000000) == 0x40400450) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-450 MP4 device\n"));
		num_pp_cores = 4;
	} else if (mali_read_phys(0xFC000000) == 0xFFFFFFFF) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-470 MP4 device\n"));
		num_pp_cores = 4;
	}
#endif

*/

		/*m400_gp_version = mali_read_phys(0xC000006C);*/
		m400_gp_version = mali_read_phys(MALI_IO_BASE+0x6C);
	if ((m400_gp_version & 0xFFFF0000) == 0x0C070000) {
		MALI_DEBUG_PRINT(4, ("Registering Mali-300 device\n"));
		num_pp_cores = 1;
		mali_write_phys(0xC0010020, 0xA); /* Enable direct memory mapping for FPGA */
	} else if ((m400_gp_version & 0xFFFF0000) == 0x0B070000) {
		num_pp_cores = 2;
		MALI_DEBUG_PRINT(4, ("<AliDEBUG, gp =1, pp=2\n"));
#if 0	
			u32 fpga_fw_version = mali_read_phys(MALI_IO_BASE + 0x10000);
		if (fpga_fw_version == 0x130C008F || fpga_fw_version == 0x110C008F) {
			/* Mali-400 MP1 r1p0 or r1p1 */
			MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP1 device\n"));
			num_pp_cores = 1;
				mali_write_phys(MALI_IO_BASE + 0x10020, 0xA); /* Enable direct memory mapping for FPGA */
		} else if (fpga_fw_version == 0x130C000F) {
			/* Mali-400 MP2 r1p1 */
			MALI_DEBUG_PRINT(4, ("Registering Mali-400 MP2 device\n"));
			num_pp_cores = 2;
				mali_write_phys(MALI_IO_BASE + 0x10020, 0xA); /* Enable direct memory mapping for FPGA */
	}
#endif
		}

	//20160720 yashi added
		mali_gpu_data.shared_mem_size = totalram_pages << PAGE_SHIFT; /* get from linux kernel*/
printk("<ALi><Mali> shared_mem_size : %08x \n", (unsigned int)mali_gpu_data.shared_mem_size);
		//20131016 yashi add for dedicated memory
		mali_gpu_data.dedicated_mem_start = ALi_MALI_DEDICATED_START;
		mali_gpu_data.dedicated_mem_size = ALi_MALI_DEDICATED_SIZE;

		//20131008 yashi add for dedicated memory
		mali_gpu_data.fb_start =  ALi_FB0_START;
		mali_gpu_data.fb_size = ALi_FB0_SIZE;
printk("<ALi><Mali>in %s:%4d , dedicated_mem_start = %x, dedicated_mem_size= %x, fb_start = %x, fb_size = %x \n", 
	__func__, __LINE__, (unsigned int)mali_gpu_data.dedicated_mem_start, (unsigned int)mali_gpu_data.dedicated_mem_size, \
	(unsigned int)mali_gpu_data.fb_start, (unsigned int)mali_gpu_data.fb_size);

//20160802 yashi temporaily removed
#if 0
	/* After kernel 3.15 device tree will default set dev
	 * related parameters in of_platform_device_create_pdata.
	 * But kernel changes from version to version,
	 * For example 3.10 didn't include device->dev.dma_mask parameter setting,
	 * if we didn't include here will cause dma_mapping error,
	 * but in kernel 3.15 it include  device->dev.dma_mask parameter setting,
	 * so it's better to set must need paramter by DDK itself.
	 */
	if (!device->dev.dma_mask)
		device->dev.dma_mask = &device->dev.coherent_dma_mask;
	device->dev.archdata.dma_ops = dma_ops;
#endif 
	err = platform_device_add_data(device, &mali_gpu_data, sizeof(mali_gpu_data));

	if (0 == err) {
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
		pm_runtime_set_autosuspend_delay(&(device->dev), 1000);
		pm_runtime_use_autosuspend(&(device->dev));
#endif
		pm_runtime_enable(&(device->dev));
#endif
		MALI_DEBUG_ASSERT(0 < num_pp_cores);
		mali_core_scaling_init(num_pp_cores);
	}

#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)
	/* Get thermal zone */
	gpu_tz = thermal_zone_get_zone_by_name("soc_thermal");
	if (IS_ERR(gpu_tz)) {
		MALI_DEBUG_PRINT(2, ("Error getting gpu thermal zone (%ld), not yet ready?\n",
				     PTR_ERR(gpu_tz)));
		gpu_tz = NULL;

		err =  -EPROBE_DEFER;
	}
#endif

	return err;
}

int mali_platform_device_deinit(struct platform_device *device)
{
	MALI_IGNORE(device);

	MALI_DEBUG_PRINT(4, ("mali_platform_device_deinit() called\n"));

	mali_core_scaling_term();
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&(device->dev));
#endif

	mali_write_phys(0xC0010020, 0x9); /* Restore default (legacy) memory mapping */

	return 0;
}

#endif /* CONFIG_MALI_DT */

static u32 mali_read_phys(u32 phys_addr)
{
	u32 phys_addr_page = phys_addr & 0xFFFFE000;
	u32 phys_offset    = phys_addr & 0x00001FFF;
	u32 map_size       = phys_offset + sizeof(u32);
	u32 ret = 0xDEADBEEF;
	void *mem_mapped = ioremap_nocache(phys_addr_page, map_size);
	if (NULL != mem_mapped) {
		ret = (u32)ioread32(((u8 *)mem_mapped) + phys_offset);
		iounmap(mem_mapped);
	}

	return ret;
}

static void mali_write_phys(u32 phys_addr, u32 value)
{
	u32 phys_addr_page = phys_addr & 0xFFFFE000;
	u32 phys_offset    = phys_addr & 0x00001FFF;
	u32 map_size       = phys_offset + sizeof(u32);
	void *mem_mapped = ioremap_nocache(phys_addr_page, map_size);
	if (NULL != mem_mapped) {
		iowrite32(value, ((u8 *)mem_mapped) + phys_offset);
		iounmap(mem_mapped);
	}
}


static int param_set_core_scaling(const char *val, const struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);

	if (1 == mali_core_scaling_enable) {
		mali_core_scaling_sync(mali_executor_get_num_cores_enabled());
	}
	return ret;
}

static struct kernel_param_ops param_ops_core_scaling = {
	.set = param_set_core_scaling,
	.get = param_get_int,
};

module_param_cb(mali_core_scaling_enable, &param_ops_core_scaling, &mali_core_scaling_enable, 0644);
MODULE_PARM_DESC(mali_core_scaling_enable, "1 means to enable core scaling policy, 0 means to disable core scaling policy");

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{
	if (1 == mali_core_scaling_enable) {
		mali_core_scaling_update(data);
	}
}

#ifdef LINUX_buildroot_S3922
/*20161024 Yashi add SYSREG init*/	
unsigned int ali_sysreg_mapping_init( void)
{
	SYSREG_VIRT_BASE = vmalloc_32( SYSREG_PHY_SIZE );
	SYSREG_VIRT_BASE = ioremap_nocache( SYSREG_PHY_BASE, SYSREG_PHY_SIZE);

	printk("<ALi><Mali>in %s, SYSREG_VIRT_BASE = %x\n", __func__, (unsigned int)SYSREG_VIRT_BASE);
	printk("<ALi><Mali>in %s, read GPU clk(%x) = %x\n", __func__, ALI_GPU_CTRL_REG, ALI_HWREG_GET_UINT32(ALI_GPU_CTRL_REG));

	if(SYSREG_VIRT_BASE)
		return 1;

	return 0;
}

unsigned int ali_read_sysreg( unsigned int reg)
{
	//#define	ALI_HWREG_GET_UINT32(reg)	(*((volatile u32*)(SYSREG_VIRT_BASE + reg)))
	return	(*((volatile u32*)(SYSREG_VIRT_BASE + reg)));
}

void ali_write_sysreg( unsigned int val, unsigned int reg)
{
	//#define	ALI_HWREG_SET_UINT32(val, reg)	((*((volatile u32*)(SYSREG_VIRT_BASE + reg)))=val)
	((*((volatile u32*)(SYSREG_VIRT_BASE + reg)))=val);
}

#endif

/*ALi's implementation of DFS*/
#define MAX_FREQ_IDX 6 /*Added by Allen 20170328, max freq default at idx 6 : 450Mhz */
unsigned int max_freq_internal = MAX_FREQ_IDX;
int ALi_set_freq(int setting_clock_step)
{
	int vol =  ALi_clock->item[setting_clock_step].vol;
	unsigned int ret =  ALI_HWREG_GET_UINT32(ALI_GPU_CTRL_REG);
	
	/*MALI_DEBUG_PRINT(1, ("<ALi><Mali>in %s, setting_clock_step = %d, set vol = %x\n", __FUNCTION__,setting_clock_step, vol));*/
	MALI_DEBUG_PRINT(2, ("<ALi><Mali>in %s, setting_clock_step = %d, set vol = %x value %08x\n", __func__ ,setting_clock_step, vol, ((ret&ALI_GPU_CTRL_mask)|vol)));

	//MALI_DEBUG_PRINT(1,("<ALi_DBG>clock gated(0x64) =%x\n",ALI_HWREG_GET_UINT32(0x64)));
	//mali_dev_pause();

	//ALI_HWREG_SET_UINT32( ALI_HWREG_GET_UINT32(0x64) | (1<<16), 0x064 );

	ALI_HWREG_SET_UINT32( (ret&ALI_GPU_CTRL_mask)|vol, ALI_GPU_CTRL_REG);    
	
	//ALI_HWREG_SET_UINT32( ALI_HWREG_GET_UINT32(0x64)&(~(1<<16)), 0x064 );
	
	//MALI_DEBUG_PRINT(1,("<ALi_DBG>clock gated(0x64) =%x\n",ALI_HWREG_GET_UINT32(0x64)));
	//mali_dev_resume();
	
MALI_DEBUG_PRINT(2, ("<ALi_DEBUG><%s>in %s:%4d \n", "Mali", __func__, __LINE__));	
	return 1;	
}
#if 0

struct mali_gpu_clk_item {
	unsigned int clock; /* unit(MHz) */
	unsigned int vol;
};

struct mali_gpu_clock {
	struct mali_gpu_clk_item *item;
	unsigned int num_of_steps;
};
#endif
void ALi_report_clock_info(struct mali_gpu_clock **data)
{

	int i;
	
	struct mali_gpu_clk_item *item;
	
	if(NULL == *data)
	{
		*data = (struct mali_gpu_clock *)kmalloc(sizeof(struct mali_gpu_clock), GFP_KERNEL);
		ALi_clock = *data;
	}
	MALI_DEBUG_PRINT(4,("<ALi><Mali>clock = %x, data =%x\n", ALi_clock, data));

#ifdef CHIP_C3921		
	ALi_clock->num_of_steps = 3; 
#endif	

#ifdef CHIP_S3922
	ALi_clock->num_of_steps = 5; 
#endif	

#ifdef CHIP_S3922C
	ALi_clock->num_of_steps = 7; 
#endif

	ALi_clock->item = (struct mali_gpu_clk_item *)kmalloc(sizeof(struct mali_gpu_clk_item) * ALi_clock->num_of_steps, GFP_KERNEL);


	item =  (struct mali_gpu_clk_item *)(ALi_clock->item);
	
#ifdef CHIP_C3921	
	item[0].clock = 148;
	item[0].vol = 0;
	
	item[1].clock = 225;
	item[1].vol = 0x00100000;

	item[2].clock = 297;
	item[2].vol = 0x00200000;
#endif

#ifdef CHIP_S3922
	item[0].clock = 148;
	item[0].vol = 0;
	
	item[1].clock = 225;
	item[1].vol = 0x00010000;
	//item[1].vol = 0x0001;

	item[2].clock = 297;
	item[2].vol = 0x00030000;
	//item[2].vol = 0x0003;

	item[3].clock = 396;
	item[3].vol = 0x00050000;

	item[4].clock = 450;
	item[4].vol = 0x00060000;	
#endif

#ifdef CHIP_S3922C
	item[0].clock = 148;
	item[0].vol = 0;
	
	item[1].clock = 225;
	item[1].vol = 0x100;

	item[2].clock = 270;
	item[2].vol = 0x200;

	item[3].clock = 297;
	item[3].vol = 0x300;
	
	item[4].clock = 337;
	item[4].vol = 0x400;

	item[5].clock = 396;
	item[5].vol = 0x500;

	item[6].clock = 450;
	item[6].vol = 0x600;

#endif

	for (i = 0; i < ALi_clock->num_of_steps; i++){
		MALI_DEBUG_PRINT(4,("<ALi><Mali>clock->item[%d].clock = %d, clock->item[%d].vol = %x\n", i, item[i].clock, i, item[i].vol)); 
	}


}

int ALi_get_freq(void)
{
	int i;
	unsigned int ret = ALI_HWREG_GET_UINT32(ALI_GPU_CTRL_REG);


	//ret = (ret>>20)&0x3;	//for clock bit = [20:21]
	for (i = 0; i < ALi_clock->num_of_steps; i++)
	{
		if( ret == ALi_clock->item[i].vol)
		{
			MALI_DEBUG_PRINT(2, ("<ALi><Mali>in %s, current GPU clock step =%d, corresponding clock = %d\n", __FUNCTION__, i, ALi_clock->item[i].clock));
			return i;
		}
	}

	printk("<ALi><Mali>in %s, clock volume not matched! reg read = %d\n", __FUNCTION__, ret);
	return ret;
}

void ALi_Mali_thermal_limit(unsigned int max_freq)
{
	if (max_freq < 0) 			max_freq = 0;
	if (max_freq > MAX_FREQ_IDX) max_freq = MAX_FREQ_IDX;

	max_freq_internal = max_freq;
}

EXPORT_SYMBOL(ALi_Mali_thermal_limit);
