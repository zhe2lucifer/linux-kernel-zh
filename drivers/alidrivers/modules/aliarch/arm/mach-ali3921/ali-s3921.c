/*
 * arch/arm/mach-ali3921/ali-s3921.c
 *
 * ALi S3921 Development Board Setup
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/version.h> 
#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/export.h>
#include <asm/smp_scu.h>
#include <asm/smp_twd.h>
#include <mach/hardware.h>
#include <mach/ali-s3921.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/irqchip/arm-gic.h>
#else
#include <asm/hardware/gic.h>
#endif
#include <asm/hardware/cache-l2x0.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/timex.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
#else
#include <asm/localtimer.h>
#endif
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <ali_reg.h>
#include <mach/s3921-clock.h>
#include "board_config.h"
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <ali_board_config.h>
#include <linux/mmc/host.h>

/*add for dts*/
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/irqchip.h>
#endif
//==============================================================================//

#define NOR_BOOT (0)
#define NAND_BOOT (1)
#define BOOT_TYPE_SELECTED (__REG32ALI(0x18000070) & ((1<<18) | (1<<17)))
#define SOC_PINMUX_REG1 (0x88)
#define STRAP_PIN_REG (0x74)
#define NF_PIN_SEL (1<<3)
#define SFLASH_FUNC_SEL (1<<19)
#define SOC_PINMUX_REG2 (0x8C)
#define STRAPIN_SEL_ENABLE (1 << 31)
#define NOR_BOOT_SELECT (~((1<<18) | (1<<17)))
#define ALI_M3921_SOC_BASE (0x18000000)
#define NF_SEL_TRIG (1<<23)
#define EMMC_SEL_TRIG (1<<24)
#define FUNC_MODE_TRIG (1<<28)
#define NAND_BOOT_IS_SELECTED (1 << 18)
#define ALI_CHIP_C3921 (0x39210000)
//==============================================================================//

bool is_nand_boot = false;
uint32_t strappin_0x70 = 0;
extern unsigned long l2x0_saved_regs_addr;
extern void ali_hdmi_set_module_init_hdcp_onoff(bool bOnOff);
extern void arm_remap_lowmem_nocache(unsigned int start, unsigned int len);
int __init m36_init_gpio(void);
//extern void (*_machine_halt)(void);//arm/kernel/process.c have no _machine_halt
void ali_s3921_poweroff(void);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
extern void ali_s3921_timer_init(void);
extern struct smp_operations s3921_smp_ops;
#else
struct sys_timer;
extern struct sys_timer ali_s3921_timer;
#endif

/*ali_s3921_init_early.*/
extern ALI_L2X0_Init(unsigned long arg1, unsigned long arg2);

/*Function declaration.*/
void __init ali_s3921_map_io(void);
void ali_s3921_l2_cache_init(void);
void __iomem *ali_m3921_get_l2cache_base(void);
void __iomem *ali_m3921_get_per_base(void);
static void __init ali_s3921_init_early(void);
static int mbx_irq_enable(unsigned int irq);
static void __init ali_s3921_init_irq(void);
void ali_s3921_restart(char mode, const char *cmd);
static void __init ali_s3921_init_machine(void);
void __init ali_s3921_reserve_mem(void);
//==============================================================================//

static volatile unsigned long boot_type = 0;
/* Mutex to protect requesting and releasing pinmux 1 and 2. This is shared by
   Nand Flash, SDIO and Nor Flash */
DEFINE_MUTEX(ali_sto_mutex);
EXPORT_SYMBOL(ali_sto_mutex);
//==============================================================================//

/* Platform-dependent data for ali-mci host */
//struct alimci_host_platform_data;

/*
Define the host switch type
C3921 use the  Pinmux Switch to support the eMMC and SD host type Switch
It means that host controller  can be either eMMC active or SD active in one time
*/
enum alimci_hostswitch_type
{
	FORCE_EMMC_ACTIVE = 0,   /* host switch to eMMC active */
	FORCE_SD_ACTIVE,              /* host switch to SD active */
};

/**
 * struct alimci_host_platform_data - Platform-dependent data for ali-mci host
 * @host_switch_type: Define the host switch type, either to be eMMC active or to be SD active
 * @timeout: clock stretching timeout in jiffies
 */
struct alimci_host_platform_data {
	unsigned int	host_switch_type;
	int		timeout;
};

/*ali_s3921_map_io.*/
static struct map_desc ali_s3921_io_desc[] __initdata = 
{
	{
		.virtual        = VIRT_SYSTEM,
		.pfn            = __phys_to_pfn(PHYS_SYSTEM),
		.length         = SIZE_ALIIO,
		.type           = MT_DEVICE,
	},
   
	{
		.virtual    = VIRT_ARM_PERIPHBASE,
		.pfn            = __phys_to_pfn(PHYS_ARM_PERIPHBASE),
		.length         = SIZE_ARM_PERIPHBASE,
		.type           = MT_DEVICE,
	},
};
static struct map_desc ali_s3921_cache_patch_desc[] __initdata = 
{
    {
        .virtual        = VIRT_CACHE_PATCH_READ_CH1,
        .pfn            = __phys_to_pfn(0x0),
        .length         = SIZE_CACHE_PATCH,
        .type           = MT_DEVICE,
    },  
    {
        .virtual        = VIRT_CACHE_PATCH_READ_CH2,
        .pfn            = __phys_to_pfn(0x0),
        .length         = SIZE_CACHE_PATCH,
        .type           = MT_DEVICE,
    }, 

};
/*ali_s3921_init_machine.*/
 static struct platform_device ali_uart = {
	.name       = "ali_uart",
	.id         = -1,
};

/*OHCI (USB full speed host controller).*/
static struct resource ali_usb_ohci_resources_1[] = {
	[0] = {
		.start  = ALI_USB1_OHCI_PHY_BASE,
		.end    = ALI_USB1_OHCI_PHY_BASE+ ALI_USB1_OHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},

	[1] = {
		.start  = ALI_IRQ_USB1_OHCI,
		.end    = ALI_IRQ_USB1_OHCI,
		.flags  = IORESOURCE_IRQ,
	},

	[2] = {
		.start  = ALI_USB1_OHCI_PCI_PHY_BASE,
		.end    = ALI_USB1_OHCI_PCI_PHY_BASE+ ALI_USB1_OHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct resource ali_usb_ohci_resources_2[] = {
	[0] = {
		.start  = ALI_USB2_OHCI_PHY_BASE,
		.end    = ALI_USB2_OHCI_PHY_BASE+ ALI_USB2_OHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},

	[1] = {
		.start  = ALI_IRQ_USB2_OHCI,
		.end    = ALI_IRQ_USB2_OHCI,
		.flags  = IORESOURCE_IRQ,
	},

	[2] = {
		.start  = ALI_USB2_OHCI_PCI_PHY_BASE,
		.end    = ALI_USB2_OHCI_PCI_PHY_BASE+ ALI_USB2_OHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},
};

/*The dmamask must be set for OHCI to work.*/
static u64 ohci_dmamask = DMA_BIT_MASK(32);
static struct platform_device ali_usb1_ohci_device = {
	.name           = "ali-ohci",
	.id         = 0,
	.dev = {
		.dma_mask           = &ohci_dmamask,
		.coherent_dma_mask  = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(ali_usb_ohci_resources_1),
	.resource           = ali_usb_ohci_resources_1,
};

static struct platform_device ali_usb2_ohci_device = {
	.name           = "ali-ohci",
	.id         = 1,
	.dev = {
		.dma_mask           = &ohci_dmamask,
		.coherent_dma_mask  = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(ali_usb_ohci_resources_2),
	.resource           = ali_usb_ohci_resources_2,
};

/*EHCI (USB high speed host controller).*/
static struct resource ali_usb_ehci_resources_1[] = {
	[0] = {
		.start  = ALI_USB1_EHCI_PHY_BASE,
		.end        = ALI_USB1_EHCI_PHY_BASE + ALI_USB1_EHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},

	[1] = {
		.start  = ALI_IRQ_USB1_EHCI,
		.end        = ALI_IRQ_USB1_EHCI,
		.flags  = IORESOURCE_IRQ,
	},

	[2] = {
		.start  = ALI_USB1_EHCI_PCI_PHY_BASE,
		.end        = ALI_USB1_EHCI_PCI_PHY_BASE + ALI_USB1_EHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},

	[3] = {
		.start  = ALI_USB1_HOST_GENERAL_PURPOSE_BASE,
		.end        = ALI_USB1_HOST_GENERAL_PURPOSE_BASE + ALI_USB1_HOST_GENERAL_PURPOSE_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct resource ali_usb_ehci_resources_2[] = {
	[0] = {
		.start  = ALI_USB2_EHCI_PHY_BASE,
		.end        = ALI_USB2_EHCI_PHY_BASE + ALI_USB2_EHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},

	[1] = {
		.start  = ALI_IRQ_USB2_EHCI,
		.end        = ALI_IRQ_USB2_EHCI,
		.flags  = IORESOURCE_IRQ,
	},

	[2] = {
		.start  = ALI_USB2_EHCI_PCI_PHY_BASE,
		.end        = ALI_USB2_EHCI_PCI_PHY_BASE + ALI_USB1_EHCI_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},

	[3] = {
		.start  = ALI_USB2_HOST_GENERAL_PURPOSE_BASE,
		.end        = ALI_USB2_HOST_GENERAL_PURPOSE_BASE + ALI_USB2_HOST_GENERAL_PURPOSE_LEN - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static u64 ehci_dmamask = DMA_BIT_MASK(32);
static struct platform_device ali_usb1_ehci_device = {
	.name           = "ali-ehci",
	.id         = 0,
	.dev = {
		.dma_mask       = &ehci_dmamask,
		.coherent_dma_mask  = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(ali_usb_ehci_resources_1),
	.resource   = ali_usb_ehci_resources_1,
};

static struct platform_device ali_usb2_ehci_device = {
	.name           = "ali-ehci",
	.id         = 1,
	.dev = {
		.dma_mask       = &ehci_dmamask,
		.coherent_dma_mask  = DMA_BIT_MASK(32),
	},
	.num_resources  = ARRAY_SIZE(ali_usb_ehci_resources_2),
	.resource   = ali_usb_ehci_resources_2,
};

/*gadget (USB high speed dev controller).*/
static u64 gadget_dmamask = DMA_BIT_MASK(32);
static struct resource ali_usb_gadget_resources[] = {
	[0] = {
		.start	= USB_ALI_GADGET_BASE,
		.end	= USB_ALI_GADGET_BASE + USB_ALI_GADGET_LEN - 1,
		.flags	= IORESOURCE_MEM,
	},

	[1] = {
		.start	= ALI_IRQ_USB_GADGET,
		.end	= ALI_IRQ_USB_GADGET,
		.flags	= IORESOURCE_IRQ,
	},

	[2] = {
		.start	= USB_ALI_GADGET_DMA_BASE,
		.end	= USB_ALI_GADGET_DMA_BASE + USB_ALI_GADGET_DMA_LEN - 1,
		.flags	= IORESOURCE_DMA,
	},
};

struct ali_usb_gadget_platform_data {
	int *usb_5v_control_gpio;/*for C3921 BGA GPIO 73 control usb port 0 5V.*/
	int	*usb_5v_detect_gpio;/*for C3921 BGA GPIO 84 control usb port device port 5V detect.*/
};
    
/*Define Platform-dependent data for ali-mci host.*/
static struct ali_usb_gadget_platform_data ali_usb_gadget_data = 
{
	.usb_5v_control_gpio = &g_usb_p0_host_5v_gpio,
	.usb_5v_detect_gpio  = &g_usb_device_5v_detect_gpio,
};

static struct platform_device ali_usb_gadget_device = {
	.name		= "alidev_udc",
	.id		= 0,
	.dev = {
		.dma_mask		= &gadget_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data = &ali_usb_gadget_data,		
	},
	.num_resources	= ARRAY_SIZE(ali_usb_gadget_resources),
	.resource	= ali_usb_gadget_resources,
};

/*Define Platform-dependent data for ali nand driver.*/
struct ali_nand_platform_data {
	long *nand_wp_gpio;
};

/* SDIO/eMMC */
/*
pinmux sel for NF/SFLASH/EMMC

C3921A : SFLASH/ NF /EMMC_BOOT/EMMC_SEL2 /EMMC_SEL3
C3921B : SFLASH/ NF /EMMC_BOOT/EMMC_SEL2 

GPIO pin define C3921A and C3921B is same.
GPIO[x] : use as below:
NF : 93 ~ 104
SFLASH : 98, 100~104

EMMC_BOOT_SEL : 93, 94, 99~107
EMMC_SEL2 : 35, 37, 42~46, 48, 49, 51, 53
EMMC_SEL3 : 11, 12, 41~44 
 */
enum ali_3921_mmc_pin_group
{
	EMMC_BOOT = 0,   /* host switch to eMMC active */
	EMMC_SEL2,
	EMMC_SEL3,
};
	
struct ali_mmc_platform_data {
	unsigned long    	max_frequency;  /* SD_IP_CLK */	
	unsigned long		capability;
	unsigned long		cd_gpios;	/* Card detect GPIO */
	bool			cd_inverted;
	unsigned long		wp_gpios; 	/* Write protect GPIO */
	bool			wp_inverted;
	unsigned long		pin_group;				
};

static struct ali_mmc_platform_data mmc_data = 
{   	
	.max_frequency	= 108000000,			/* SD_IP_CLK 108MHz for M3733 */
	.capability	= MMC_CAP_4_BIT_DATA,
	.cd_gpios	= 113,				/* Card detect GPIO */
	.cd_inverted	= false,
	.wp_gpios	= -1,				/* Write protect GPIO */
	.pin_group	= EMMC_SEL2,
};

static u64 mmc_dmamask = DMA_BIT_MASK(32);
static struct resource mmc_resources[] = {
	[0] = {
		.start	= 0x18014000,
		.end	= 0x18014000 + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},

	[1] = {
		.start	= ALI_IRQ_SDIO,
		.end	= ALI_IRQ_SDIO,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ali_mmc_device = {
	.name		= "ali-mmc",
	.id		= -1,
	.dev		= {
				.dma_mask		= &mmc_dmamask,
				.coherent_dma_mask	= DMA_BIT_MASK(32),
				.platform_data		= &mmc_data,
	},
	.resource	= mmc_resources,
	.num_resources	= ARRAY_SIZE(mmc_resources),
};

/*FB platformat device0 for the GMA1. 
support CLUT8, ARGB1555 and ARGB8888.*/
static struct platform_device ali_dev_fb0 = {
	.name		  = "alifb",
	.id		         = 1,
};

/*FB platformat device1 for the video.*/
static struct platform_device ali_dev_fb1 = {
	.name		  = "alifb",
	.id		         = 2,
};

/*FB platformat device2 for GMA2.*/
static struct platform_device ali_dev_fb2 = {
	.name		  = "alifb",
	.id		         = 3,
};

/*Define I2C GPIO driver platform data as the basic configuration.*/
static struct i2c_gpio_platform_data i2c_data_hdmi = 
{
	.sda_pin = 74,
	.scl_pin = 75,
	.udelay = 6,
	//.timeout = HZ/5,
	.scl_is_open_drain = 0,
	.sda_is_open_drain = 0,
};

/*Define I2C device object using the GPIO PIN  bit-shift emulation adapter.*/
static struct platform_device i2c_gpio_hdmi = 
{
	.name = "i2c-gpio",
	.id = 5, 
	.dev = 
	{
		.platform_data = &i2c_data_hdmi,
	},
};

/*I2S controller.*/
struct platform_device ali_iis_cpu_s3921_device = {
	.name		= "iis_cpu_s3921_dev",
	.id		= -1,
};

/*platform.*/
struct platform_device ali_asoc_iis_platform_s39_device = {
	.name		= "asoc_iis_platform_s39_dev",
	.id		= -1,
};

/*Codec.*/
struct platform_device ali_iis_codec_s3921_device = {
	.name		= "iis_codec_s3921_dev",
	.id		= -1,
};

/*spdif controller.*/

/*platform.*/
struct platform_device ali_spdif_platform_s39xx_dev= {
	.name		= "asoc_spdif_platform_s39_dev",
	.id		= -1,
};

/*CPU.*/
struct platform_device ali_spdif_cpu_s3921_device = {
	.name		= "spdif_cpu_s3921_dev",
	.id		= -1,
};

/*Codec.*/
struct platform_device ali_spdif_codec_s3921_device = {
	.name		= "spdif_codec_s3921_dev",
	.id		= -1,
};

/*ali soundbar device definition.*/
#ifdef CONFIG_ALI_VOLBAR
struct ali_volbar_platform_data {
	int *volbar_volup_gpio;
	int *volbar_voldn_gpio;
};
    
/*Define Platform-dependent data for ali-mci host.*/
static struct ali_volbar_platform_data ali_volbar_gpio_data = 
{
	.volbar_volup_gpio = &g_gpio_vol_up,
	.volbar_voldn_gpio  = &g_gpio_vol_down,
};

static struct resource ali_volbar_resources[] = {   
	[0] = {
		.start = ALI_IRQ_VOLBAR_GPIO,
		.end	= ALI_IRQ_VOLBAR_GPIO,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device ali_volbar_device = {
	.name = "ali_volbar",
	.id = 7,
	.dev = {
		.platform_data = &ali_volbar_gpio_data,
	},
	.num_resources = ARRAY_SIZE(ali_volbar_resources),
	.resource = ali_volbar_resources,
};
#endif

static struct platform_device ali_sflash_device = {
	.name		= "ali_str_sflash",
	.id			= -1,
};

static struct platform_device ali_pmu_device = {
	.name		= "ali_str_pmu",
	.id			= -1,
};

static struct platform_device *devices[] __initdata = {
	&ali_uart,
	//&ali_nand_device,
	&ali_usb_gadget_device,
	&ali_usb1_ohci_device,
	&ali_usb2_ohci_device,
	&ali_usb1_ehci_device,
	&ali_usb2_ehci_device,
	&ali_dev_fb0,/* FB platformat devices */
	&ali_dev_fb1,
	&ali_dev_fb2,
	&i2c_gpio_hdmi,
	&ali_iis_cpu_s3921_device,
	&ali_asoc_iis_platform_s39_device,
	&ali_iis_codec_s3921_device,    
	&ali_spdif_platform_s39xx_dev,
	&ali_spdif_cpu_s3921_device,
	&ali_spdif_codec_s3921_device,
	&ali_mmc_device,
#ifdef CONFIG_ALI_VOLBAR
	&ali_volbar_device,
#endif
	&ali_sflash_device,
	&ali_pmu_device,
};

#ifdef CONFIG_OF   //added by kinson for FDT
static const char *ali_gp_boards_compat[] __initdata = {
	"ali,3921",
	NULL,
};

DT_MACHINE_START(ALI_S3921, "ali 3921 board")
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))    
	.smp		= smp_ops(s3921_smp_ops),
#endif    
	.map_io         = ali_s3921_map_io,
	.init_early     = ali_s3921_init_early,
	.init_irq       = ali_s3921_init_irq,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	.init_time      = ali_s3921_timer_init,
#else
	.timer          = &ali_s3921_timer,
	.handle_irq     = gic_handle_irq,
#endif
	.reserve        = ali_s3921_reserve_mem,//ali_s3921_board_setting,//ali_s3921_reserve_mem,
	.init_machine   = ali_s3921_init_machine,
	.restart	= ali_s3921_restart,
	.dt_compat	= ali_gp_boards_compat,
MACHINE_END
#else//normal.

	/*MACHINE_START.*/
MACHINE_START(ALI_S3921, BOARD_NAME)
	.atag_offset    = 0x100,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))    
	.smp		= smp_ops(s3921_smp_ops),
#endif    
	.map_io         = ali_s3921_map_io,
	.init_early     = ali_s3921_init_early,
	.init_irq       = ali_s3921_init_irq,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
	.init_time      = ali_s3921_timer_init,
#else
	.timer          = &ali_s3921_timer,
	.handle_irq     = gic_handle_irq,
#endif
	.reserve        = ali_s3921_reserve_mem,//ali_s3921_board_setting,//ali_s3921_reserve_mem,
	.init_machine   = ali_s3921_init_machine,
	.restart	= ali_s3921_restart,
MACHINE_END
#endif
//==============================================================================//
unsigned int cache_patch_read_ch1 = 0;
unsigned int cache_patch_read_ch2 = 0;
void __init ali_s3921_map_io(void)
{
    printk("System IO Base Addr = 0x%.08x\n", VIRT_SYSTEM);
    printk("ARM Peripheral Global Timer Base Addr = 0x%.08x\n", (unsigned int) A9_MPCORE_GIT);
    printk("ARM Peripheral GIC CPU Interface Base Addr = 0x%.08x\n", (unsigned int) A9_MPCORE_GIC_CPU);
    printk("ARM Peripheral GIC Distributor Base Addr = 0x%.08x\n",(unsigned int)  A9_MPCORE_GIC_DIST);
    printk("ARM Peripheral PL310 L2Cache Controller Base Addr = 0x%.08x\n",(unsigned int)  A9_L2_BASE_ADDR);
    iotable_init(ali_s3921_io_desc, ARRAY_SIZE(ali_s3921_io_desc));

	if(0x20000000 == memblock.memory.total_size)//512
		ali_s3921_cache_patch_desc[1].pfn = __phys_to_pfn(0x90000000);//0x10000080;
	else if(0x40000000 == memblock.memory.total_size)//1G
		ali_s3921_cache_patch_desc[1].pfn = __phys_to_pfn(0xa0000000);//0x20000080;
	else//256M
		ali_s3921_cache_patch_desc[1].pfn = __phys_to_pfn(0x88000000);//0x8000080;

	iotable_init(ali_s3921_cache_patch_desc, ARRAY_SIZE(ali_s3921_cache_patch_desc));
	cache_patch_read_ch1 = VIRT_CACHE_PATCH_READ_CH1;
	cache_patch_read_ch2 = VIRT_CACHE_PATCH_READ_CH2;
#ifdef CONFIG_ARM_ALIS3921_CPUFREQ
	#if 0
	/*  Setup the s3921 system clock, initialize the ARM CA9 clock to default 1 GHZ  */
	s3921_setup_clocks();
	#endif

	/*Register the system clock.*/
	s3921_register_clocks();
#endif

	printk("%s,%d\n", __FUNCTION__, __LINE__);
	//dump_stack();   
}

void ali_s3921_l2_cache_init(void)
{
#ifdef CONFIG_CACHE_L2X0
	u32 data_rd = 0, aux_ctrl_enable = 0, aux_ctrl_disable = 0;

	/*  top.CHIP.U_CORE.U_T2_CHIPSET.CA9_MISC_CTRL_250H[18:16]
	CACHE_SIZE:
	001: 128KB
	010: 256KB
	011: 512KB
	Other: non-define, don't use
	*/
	printk("L2x0 init before: CPU_REG_CTRL:0x%x, TAG RAM CTRL:0x%x, DATA RAM CTRL:0x%x\n", \
		__REG32ALI(0x18000094), readl_relaxed(A9_L2_BASE_ADDR+L2X0_TAG_LATENCY_CTRL), \
		readl_relaxed(A9_L2_BASE_ADDR+L2X0_DATA_LATENCY_CTRL));
	ALI_L2X0_Init(__REGALIRAW(0x18000094), A9_L2_BASE_ADDR);
	printk("L2x0 init after: CPU_REG_CTRL:0x%x, TAG RAM CTRL:0x%x, DATA RAM CTRL:0x%x\n", \
		__REG32ALI(0x18000094), readl_relaxed(A9_L2_BASE_ADDR+L2X0_TAG_LATENCY_CTRL), \
		readl_relaxed(A9_L2_BASE_ADDR+L2X0_DATA_LATENCY_CTRL));

	/*8-way associativity, Way size - 64KB.*/
	aux_ctrl_enable = (0x1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT) | (0x0 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) | (0x03 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT);
	aux_ctrl_enable |= ((1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) | (1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT) | (1 << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT));
	aux_ctrl_enable |= (0x01 << 23) | 1; //Non-write allocate
	aux_ctrl_enable |= (0x01 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT); //Mali Integration Guide Pg 2-11 enable bit 22      
	aux_ctrl_disable = L2X0_AUX_CTRL_MASK;
	l2x0_init(A9_L2_BASE_ADDR , aux_ctrl_enable, aux_ctrl_disable);

	/*Enhance memory performance on 6/6.*/
	//printk("L2x0 end: CPU_REG_CTRL:0x%x, L2X0_CTRL:0x%x, TAG RAM CTRL:0x%x, DATA RAM CTRL:0x%x\n", __REG32ALI(0x18000094),readl_relaxed(A9_L2_BASE_ADDR+L2X0_CTRL), readl_relaxed(A9_L2_BASE_ADDR+L2X0_TAG_LATENCY_CTRL), readl_relaxed(A9_L2_BASE_ADDR+L2X0_DATA_LATENCY_CTRL));
	printk("L2x0 end: CPU_REG_CTRL:0x%x, L2X0_CTRL:0x%x, TAG RAM CTRL:0x%x, DATA RAM CTRL:0x%x, L2X0_PREFETCH_CTRL:0x%x\n", \
		__REG32ALI(0x18000094),readl_relaxed(A9_L2_BASE_ADDR+L2X0_CTRL), readl_relaxed(A9_L2_BASE_ADDR+L2X0_TAG_LATENCY_CTRL), \
		readl_relaxed(A9_L2_BASE_ADDR+L2X0_DATA_LATENCY_CTRL), readl_relaxed(A9_L2_BASE_ADDR+L2X0_PREFETCH_CTRL));

	data_rd = readl_relaxed(A9_L2_BASE_ADDR+L2X0_PREFETCH_CTRL);
	data_rd |= (1<<23) | (1<<24) | (1<<27) | (1<<28) | (1<<29) | (1<<30);
	data_rd &= (~0x1f);
	writel_relaxed(data_rd,A9_L2_BASE_ADDR+L2X0_PREFETCH_CTRL);
	printk("MOdifiy L2X0_PREFETCH_CTRL value to:0x%x\n", data_rd);   
   	
	/*I have to save regs as cache-l2x0 does, l2x0_init() have saved aux_ctrl.*/
	l2x0_saved_regs.prefetch_ctrl = readl_relaxed(A9_L2_BASE_ADDR+L2X0_PREFETCH_CTRL);
	l2x0_saved_regs.tag_latency = readl_relaxed(A9_L2_BASE_ADDR+L2X0_TAG_LATENCY_CTRL);
	l2x0_saved_regs.data_latency = readl_relaxed(A9_L2_BASE_ADDR+L2X0_DATA_LATENCY_CTRL);
	l2x0_saved_regs.pwr_ctrl = readl_relaxed(A9_L2_BASE_ADDR+L2X0_POWER_CTRL);
	l2x0_saved_regs.phy_base = PHYS_ARM_PERIPHBASE + 0x2000;	
	l2x0_saved_regs_addr = virt_to_phys(&l2x0_saved_regs);
#endif
}

void __iomem *ali_m3921_get_l2cache_base(void)
{
	u32	l2cache_base = 0;

	l2cache_base =A9_L2_BASE_ADDR;
	return l2cache_base;
}

void __iomem *ali_m3921_get_per_base(void)
{
	return PHYS_ARM_PERIPHBASE;
}

static void __init ali_s3921_init_early(void)
{
	printk("%s,%d\n", __FUNCTION__, __LINE__);
	ali_s3921_l2_cache_init();
	mutex_init(&ali_sto_mutex);
	printk("\n\n\n--------------------------------ali_sto_mutex is initilized--------------------------------\n");

	boot_type = __REG32ALI(0x18000070);
	printk(KERN_EMERG "Function:%s, Line:%d, Reg 0x18000070 value: 0x%08X\n",
		__FUNCTION__, __LINE__, boot_type);
	if(NAND_BOOT == BOOT_TYPE_SELECTED)
	{
		printk(KERN_EMERG "Boot Type: Nand!\n");
		is_nand_boot = true;
	}
	else
	{
		printk(KERN_EMERG "Boot Type: Nor!\n");
		is_nand_boot = false;
	}
}

bool board_is_nand_boot(void)
{
	return is_nand_boot;
}

static int mbx_irq_enable(unsigned int irq)
{
	if((irq >=56) && (irq < 60))
	{
		*(u32 *)(SYS_INT_ENABLE2+0x00040000) |= (u32)(1<<(irq - 28));
		return 1;
	}

	return 0;
}

static int mbx_irq_disable(unsigned int irq)
{
	if((irq >=56) && (irq < 60))
	{
		*(u32 *)(SYS_INT_ENABLE2+0x00040000) &= ~((u32)(1<<(irq-28)));
		return 1;
	}

	return 0;
}

static void ali_s3921_irq_enable(struct irq_data *d)
{   
	unsigned int irq_no = 0, irq_reg = 0;

	if((d->irq > (96 + ALI_SYS_IRQ_BASE)) || (d->irq < (ALI_SYS_IRQ_BASE)))
	{
		return;
	}
 
	if(mbx_irq_enable(d->irq -ALI_SYS_IRQ_BASE))
	{
		return;
	}
 
	irq_no = (d->irq -ALI_SYS_IRQ_BASE) % 32;
	irq_reg = (d->irq -ALI_SYS_IRQ_BASE) /32;

	switch(irq_reg)
	{
		case 0:
			*(u32 *)(SYS_INT_ENABLE1) |= (u32) (1 << irq_no);
			break;

		case 1:
			*(u32 *)(SYS_INT_ENABLE2) |= (u32) (1 << irq_no);
			//printk("%s INT_REG%d, bit %x\n", __FUNCTION__, irq_reg+1, irq_no);
			break;

		case 2:         
			*(u32 *)(SYS_INT_ENABLE3) |= (u32) (1 << irq_no);
			//printk("%s INT_REG%d, bit %x\n", __FUNCTION__, irq_reg+1, irq_no);
			break;

		default:
			break;
	}   
}

static void ali_s3921_irq_disable(struct irq_data *d)
{   
	unsigned int irq_no = 0, irq_reg = 0;

	if((d->irq > (96 + 32)) || (d->irq < (32)))
	{
		return;
	}

	if(mbx_irq_disable(d->irq -ALI_SYS_IRQ_BASE))
	{
		return;
	}
		    
	irq_no = (d->irq -32) %32;
	irq_reg = (d->irq -32) /32;

	switch(irq_reg)
	{
		case 0:
			*(u32 *)(SYS_INT_ENABLE1) &= ~(u32) (1 << irq_no);
			break;

		case 1:
			*(u32 *)(SYS_INT_ENABLE2) &= ~(u32) (1 << irq_no);
			break;

		case 2:         
			*(u32 *)(SYS_INT_ENABLE3) &= ~(u32) (1 << irq_no);
			break;

		default:
			break;
	}
	//printk("%s INT_REG%d, bit %x\n", __FUNCTION__, irq_reg+1, irq_no);
}

static void __init ali_s3921_init_irq(void)
{
	gic_arch_extn.irq_ack = ali_s3921_irq_disable;
	gic_arch_extn.irq_eoi = ali_s3921_irq_enable;
	gic_arch_extn.irq_mask = ali_s3921_irq_disable;
	gic_arch_extn.irq_unmask = ali_s3921_irq_enable;
	//gic_arch_extn.irq_retrigger = NULL;
	//gic_arch_extn.flags = IRQCHIP_MASK_ON_SUSPEND | IRQCHIP_SKIP_SET_WAKE;

#ifdef CONFIG_OF
	irqchip_init();
#else
	gic_init(0, GIC_PPI_START, A9_MPCORE_GIC_DIST, A9_MPCORE_GIC_CPU);
#endif
}

static void __init ali_s3921_map_hwbuf(void)
{
	int i = 0;
	int hwbuf_desc_cnt = 0;
	struct ali_hwbuf_desc *hwbuf_desc;
	unsigned long start = 0;
	unsigned long len = 0;
	unsigned long end = 0;

	printk(KERN_EMERG "\nFunction:%s, Line:%d\n", __FUNCTION__, __LINE__);
	hwbuf_desc = ali_get_privbuf_desc(&hwbuf_desc_cnt);
	for(i=0; i<hwbuf_desc_cnt; i++)
	{	
		start = ALI_MEMALIGNDOWN(hwbuf_desc[i].phy_start);
		end = ALI_MEMALIGNUP(hwbuf_desc[i].phy_end);
		len = end - start;
		arm_remap_lowmem_nocache(start, len);
	}
	printk(KERN_EMERG "\nFunction:%s, Line:%d\n", __FUNCTION__, __LINE__);
}

static void __init ali_s3921_init_machine(void)
{
	printk(KERN_EMERG "\nFunction:%s, Line:%d\n", __FUNCTION__, __LINE__);
	platform_add_devices(devices, ARRAY_SIZE(devices));

	ali_s3921_map_hwbuf();
	customize_board_setting();
	pm_power_off = ali_s3921_poweroff;

#ifdef CONFIG_OF
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
#endif
}

/*
added by kinson for bug:#53829
ali dram controller will send wrong access data to 0x800000E0 in following two cases:
(1). If out-of-range happen, the wrong access will be send to address 0x80000E0-0x800000F0.
(2). If any IP access the wrong memory, Dram Controller will send interrupt to NB, and the wrong access will be send to address 0x80000E0-0x800000F0. 
so, we need to reserve this the bottom PAGE, then kmalloc and vmalloc will not use here.
*/
static void ali_s3921_reserve_dram_controller_debug_region(void)
{
	printk(KERN_EMERG "Reserve dram controller debug region:0x80000000-0x80001000\n");
	memblock_reserve(0x80000000, PAGE_SIZE);
}

void __init ali_s3921_reserve_mem(void)
{
	int i = 0;
	int hwbuf_desc_cnt = 0;
	struct ali_hwbuf_desc *hwbuf_desc;
	unsigned long start = 0;
	unsigned long len = 0;

	printk(KERN_EMERG "\nFunction:%s, Line:%d\n", __FUNCTION__, __LINE__);
	/*Reserve memory for HW buffer.*/
	hwbuf_desc = ali_get_privbuf_desc(&hwbuf_desc_cnt);
	for(i=0; i<hwbuf_desc_cnt; i++)
	{
		start = ALI_MEMALIGNDOWN(hwbuf_desc[i].phy_start);
		len = ALI_MEMALIGNUP(hwbuf_desc[i].phy_end) - ALI_MEMALIGNDOWN(hwbuf_desc[i].phy_start);
		memblock_reserve(start, len);
	}
	ali_s3921_reserve_dram_controller_debug_region();
	printk("%s,%d\n", __FUNCTION__, __LINE__);

	return;
}

static void wdt_reboot_from_nand(void)
{
	uint32_t tmp = 0;
	unsigned int chip_id = __REG32ALI(ALI_M3921_SOC_BASE) & 0xFFFF0000;

	if(ALI_CHIP_C3921 == chip_id)
	{
		strappin_0x70 = __REG32ALI(0x18000070);
		tmp = strappin_0x70;
		tmp |= ((1<<23) | (1<<18));
		__REG32ALI(0x18000074) = tmp;
	}
}

static void wdt_reboot_from_nor(void)
{
	uint32_t tmp = 0;
	unsigned int chip_id = __REG32ALI(ALI_M3921_SOC_BASE) & 0xFFFF0000;

	if(ALI_CHIP_C3921 == chip_id)
	{
		strappin_0x70 = __REG32ALI(0x18000070);
		tmp = strappin_0x70;
		tmp &= ~(1<<18);
		tmp &= ~(1<<23);
		__REG32ALI(0x18000074) = tmp;
	}
}

void ali_s3921_restart(char mode, const char *cmd)
{
	volatile unsigned int delay_cnt = 0;

	printk(KERN_EMERG "\nFunction:%s, Line:%d\n", __FUNCTION__, __LINE__);
	local_irq_disable();

	if(board_is_nand_boot())
	{
		printk(KERN_EMERG "========>Function:%s, Line:%d, Start to reboot from nand flash\n",
			__FUNCTION__, __LINE__);
		wdt_reboot_from_nand();
	}
	else
	{
		printk(KERN_EMERG "========>Function:%s, Line:%d, Start to reboot from nor flash\n",
			__FUNCTION__, __LINE__);
		wdt_reboot_from_nor();
	}
	printk(KERN_EMERG "Wait WDT Reboot......\n\n\n\n",
			__FUNCTION__, __LINE__);

	/*Set WDT regs to reboot STB.*/
	for(delay_cnt=0; delay_cnt<0x10000; delay_cnt++);
	__REG32ALI(0x18018500) = 0xFFFFF000;/*watch dog count.*/
	__REG8ALI(0x18018504) = 0x67;/*enable watch dog.*/

	while(1){delay_cnt++; delay_cnt--;};
}

void ali_s3921_poweroff(void)
{
	/*power_down_board.*/
	__REG8ALI(0x1805C101) = 0x01;/*pmu power down.*/

	/*pmu gpio mode.*/
	//__REG8ALI(0x1805c05f) |= (0x01<<3);/*power down gpio sel.*/
	//__REG8ALI(0x1805c05e) |= (0x01<<3);/*power down gpio output enable.*/
	//__REG8ALI(0x1805c05d) &= ~(0x01<<3);/*power down gpio output low.*/

	while(1);
}
