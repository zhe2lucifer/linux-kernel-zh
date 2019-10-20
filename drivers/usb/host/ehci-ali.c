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
 
#include <linux/platform_device.h>

#ifdef CONFIG_ARM
#include <mach/ali-s3921.h>
#endif
#include "ali_otp_common.h"
#include "ali_reg.h"
#include <asm/mach-ali/m36_gpio.h>
#include <ali_board_config.h>

#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#define ALI_USB_EHCI_IO1 0x1803A000
#define C3921	0x3921	
#define C3503 0x3503
#define C3821 0x3821
#define C3701 0x3701

#ifdef CONFIG_ARM
#define ALI_OTP_BASE                                                                           0x18042000
#define OTP_ADDR_REG                                                                          0x4
#define OTP_READ_TRIG_REG                                                                  0xc
#define OTP_READ_STATUS_REG                                                             0x10
#define OTP_VALUE_REG                                                                        0x18
#define GET_OTP_READ_STATUS                                                              __REG32ALI(ALI_OTP_BASE+OTP_READ_STATUS_REG)
#define OTP_READ_BUSY                                                                        0x100
#else
#define ALI_OTP_BASE                                                                           0xb8042000
#define OTP_ADDR_REG                                                                          0x4
#define OTP_READ_TRIG_REG                                                                  0xc
#define OTP_READ_STATUS_REG                                                             0x10
#define OTP_VALUE_REG                                                                        0x18
#define GET_OTP_READ_STATUS                                                              __REG32ALI(ALI_OTP_BASE+OTP_READ_STATUS_REG)
#define OTP_READ_BUSY                                                                        0x100
#endif
//==================================================================================================================//

struct ali_usb_setting {
	int reg_addr;   	
	int reg_val;    	 	
};

/* Define Platform-dependent data for ali-usb host */
/* c39_pkg_ver_setting
*/	
#if 0
static struct ali_usb_setting C3921_XXX_V01_setting [6] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x83000000,},	/* pll */
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x36e836e8,},	/* phy */    
    [3] ={.reg_addr = 0x24,    .reg_val  = 0x00080000,},	/* disconnect count */
    [4] ={.reg_addr = 0x20,    .reg_val  = 0x01000000,},	/* New CDR */
    [5] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
};    

static struct ali_usb_setting C3503_BGA_XXX_setting [5] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x07000000,},	/* pll */
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x001F4A74,},	/* phy */    
    [3] ={.reg_addr = 0x24,    .reg_val  = 0x00080000,},	/* disconnect count */    
    [4] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
};    

static struct ali_usb_setting C3503_QFP_XXX_setting [5] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x07000000,},	/* pll */
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x001D4954,},	/* phy */    
    [3] ={.reg_addr = 0x24,    .reg_val  = 0x00080000,},	/* disconnect count */    
    [4] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
};    

static struct ali_usb_setting C3821_QFP128_XXX_setting [8] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x03000000,},	/* pll */
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x32E832E8,},	/* phy */    
    [3] ={.reg_addr = 0x18,    .reg_val  = 0x00000000,},	/*  */	
    [4] ={.reg_addr = 0x20,    .reg_val  = 0x01000000,},	/* New CDR */	
    [5] ={.reg_addr = 0x24,    .reg_val  = 0x00080000,},	/* disconnect count */        
    [6] ={.reg_addr = 0x2C,    .reg_val  = 0x00000000,},	/* TRIM_CTRL */	
    [7] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
};    

static struct ali_usb_setting C3821_QFP156_XXX_setting [8] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x07000000,},	/* pll */
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x32083208,},	/* phy */    
    [3] ={.reg_addr = 0x18,    .reg_val  = 0x00000000,},	/*  */	
    [4] ={.reg_addr = 0x20,    .reg_val  = 0x01000000,},	/* New CDR */	
    [5] ={.reg_addr = 0x24,    .reg_val  = 0x00080000,},	/* disconnect count */        
    [6] ={.reg_addr = 0x2C,    .reg_val  = 0x00000000,},	/* TRIM_CTRL */	
    [7] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
}; 

static struct ali_usb_setting C3821_BGA_XXX_setting [8] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x03000000,},	/* pll */
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x32E832E8,},	/* phy */    
    [3] ={.reg_addr = 0x18,    .reg_val  = 0x00000000,},	/*  */	
    [4] ={.reg_addr = 0x20,    .reg_val  = 0x01000000,},	/* New CDR */	
    [5] ={.reg_addr = 0x24,    .reg_val  = 0x00080000,},	/* disconnect count */        
    [6] ={.reg_addr = 0x2C,    .reg_val  = 0x00000000,},	/* TRIM_CTRL */	
    [7] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
};   

static struct ali_usb_setting C3701_XXX_V01_setting [4] = 
{
    [0] ={.reg_addr = 0x04,    .reg_val  = 0x00000000,},	/* Host Mode */
    [1] ={.reg_addr = 0x10,    .reg_val  = 0x87000000,},	/* pll */    
    [2] ={.reg_addr = 0x14,    .reg_val  = 0x00054954,},	/* phy */    
    [3] ={.reg_addr = -1,    	 .reg_val  = -1,},	/* end */
};   
#endif
bool patch_s3921 = 0;
//==================================================================================================================//

static const struct of_device_id ali_ehci_of_match[] = {
	{ .compatible = "alitech,ehci"},
	{ },
};

struct ali_ehci_hcd {
    u32 pci_phy_regs;
    u32 pci_len;
    u32 cap_phy_regs;
    u32 cap_len;
    u32 phy_phy_regs;
    u32 phy_len;
    u32 *phy_addr_array;
    u32 *phy_value_array;
    u32 phy_setting_num;
    void __iomem *pci_regs;
    void __iomem *cap_regs;
    void __iomem *phy_regs;
};


unsigned int get_otp_value(unsigned int otp_addr)
{
//	unsigned int read_addr = ((otp_addr*4) & 0x3ff);
	unsigned int otp_value = 0;

	__REG32ALI(ALI_OTP_BASE+OTP_ADDR_REG) = otp_addr;
	__REG32ALI(ALI_OTP_BASE+OTP_READ_TRIG_REG) |= (1<<8);
	while(OTP_READ_BUSY == (GET_OTP_READ_STATUS & (1<<8)));
	otp_value = __REG32ALI(ALI_OTP_BASE+OTP_VALUE_REG);

	return otp_value;
}

static int ali_ehci_hc_reset(struct usb_hcd *hcd)
{
	int result;
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	result = ehci_setup(hcd);
	if (result)
	{
		printk("%s, init\n",__func__);
		return result;
	}	

	ehci_reset(ehci);
	return result;
}

#ifdef CONFIG_USB_EHCI_ALI_IF_TEST
#define TEST_PORT_RESET 10
#define TEST_SUSPEND_RESUME 11
#define TEST_PORT_REG(x) (0x64+x*4)
#define TEST_J_MASK             0b0001 << 16
#define TEST_K_MASK             0b0010 << 16
#define TEST_SE0_NAK_MASK       0b0011 << 16
#define TEST_PACKET_MASK        0b0100 << 16
#define TEST_FORCE_EN_MASK      0b0101 << 16
#define TEST_PORT_RESET_MASK    1 << 8
#define TEST_SUSPEND_MASK       1 << 7
#define TEST_RESUME_MASK        1 << 6
#define TEST_RESTORE_MASK       0b111 | 1<<12

static int ali_usb_if_test(struct usb_hcd *hcd, int port, int testmode, int restore)
{
    struct ehci_hcd *ehci = hcd_to_ehci(hcd);
    struct ali_ehci_hcd *ali;
    u32 reg = 0;

    ali = (struct ali_ehci_hcd *)ehci->priv;

    reg = readl(ali->cap_regs+TEST_PORT_REG(port));

    pr_debug("[%s] ori reg:0x%x\n", __func__, reg);

    if(restore){
        reg &= TEST_RESTORE_MASK;
        writel(reg, ali->cap_regs+TEST_PORT_REG(port));
        msleep(20);
        reg = readl(ali->cap_regs+TEST_PORT_REG(port));
        pr_debug("[%s] reg:0x%x\n", __func__, reg);
        return 0;
    }

    if(TEST_J == testmode)
        reg |= TEST_J_MASK;
    else if(TEST_K == testmode)
        reg |= TEST_K_MASK;
    else if(TEST_SE0_NAK == testmode)
        reg |= TEST_SE0_NAK_MASK;
    else if(TEST_PACKET == testmode)
        reg |= TEST_PACKET_MASK;
    else if(TEST_FORCE_EN == testmode)
        reg |= TEST_FORCE_EN_MASK;
    else if(TEST_PORT_RESET == testmode)
        reg |= TEST_PORT_RESET_MASK;
    else if(TEST_SUSPEND_RESUME == testmode)
        reg |= TEST_SUSPEND_MASK;
    else
        return -1;
    
    writel(reg, ali->cap_regs+TEST_PORT_REG(port));
    msleep(20);
    reg = readl(ali->cap_regs+TEST_PORT_REG(port));
    pr_debug("[%s] read reg:0x%x\n", __func__, reg);

    if(TEST_PORT_RESET == testmode){
        pr_debug("sleep 20ms\n");
        msleep(20);
        reg &= ~(TEST_PORT_RESET_MASK);
        writel(reg, ali->cap_regs+TEST_PORT_REG(port));
        msleep(20);
        reg = readl(ali->cap_regs+TEST_PORT_REG(port));
        pr_debug("[%s] read reg:0x%x\n", __func__, reg);
    }
    if(TEST_SUSPEND_RESUME == testmode){
        msleep(5000);
        reg &= ~(TEST_SUSPEND_MASK);
        reg |= TEST_RESUME_MASK;
        writel(0x00001045, ali->cap_regs+TEST_PORT_REG(port));
        reg = readl(ali->cap_regs+TEST_PORT_REG(port));
        pr_debug("[%s] read reg:0x%x\n", __func__, reg);
        msleep(20);
        writel(0x00001005, ali->cap_regs+TEST_PORT_REG(port));
        reg = readl(ali->cap_regs+TEST_PORT_REG(port));
        pr_debug("[%s] read reg:0x%x\n", __func__, reg);
    }

    return 0;
}
#endif

static const struct hc_driver ehci_ali_hc_driver = {
	.description		= "ehci hcd",
	.product_desc		= "ALi EHCI",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset			= ali_ehci_hc_reset,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,
#ifdef CONFIG_USB_EHCI_ALI_IF_TEST
    .usb_if_test = ali_usb_if_test,
#endif
	//.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

static const struct hc_driver ehci_ali_hc_driver_2 = {
	.description		= "ehci hcd 2",
	.product_desc		= "ALi EHCI 2",
	.hcd_priv_size		= sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ehci_irq,
	.flags			= HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 *
	 * FIXME -- ehci_init() doesn't do enough here.
	 * See ehci-ppc-soc for a complete implementation.
	 */
	.reset			= ali_ehci_hc_reset,
	.start			= ehci_run,
	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.bus_suspend		= ehci_bus_suspend,
	.bus_resume		= ehci_bus_resume,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	.clear_tt_buffer_complete	= ehci_clear_tt_buffer_complete,
};

#if 0
static void ali_start_ehc(struct platform_device *pdev)
{
	struct ali_usb_setting *hw_setting;
	u64 rsrc_start, rsrc_len;
	void __iomem	*pci_regs;
	void __iomem	*phy_regs;
	bool probe_1st = 0;
	u32 chip_id = 0;
	u32 chip_package = 0;
	u32 chip_ver = 0;	
	u32 trim = 0;
	int idx = 0;
	
	chip_id = (__REG32ALI(0x18000000) >> 16) & 0xFFFF;	
	chip_package = (__REG32ALI(0x18000000) >> 8) & 0xFF;	
	chip_ver = __REG32ALI(0x18000000) & 0xFF;	
		
	printk("Chip %04x, Package %02x, Version %02x\n", chip_id, chip_package, chip_ver);
	
	rsrc_start = pdev->resource[2].start;
	rsrc_len = pdev->resource[2].end - pdev->resource[2].start + 1;
	
	if (!request_mem_region(rsrc_start, rsrc_len, "ALi-EHCI (PCI config regs)")) {
		pr_debug("request_mem_region failed");
	}

	pci_regs = ioremap(rsrc_start, rsrc_len);
	if (!pci_regs) {
		pr_debug("pci ioremap failed");
	}
	printk("%s  pci_regs=(0x%llx, 0x%p)\n",__func__, rsrc_start, pci_regs);
	
	/* enable USB Host mode*/
	if (ALI_USB_EHCI_IO1 == rsrc_start)
	{	
		probe_1st = 1;		
		printk("probe 1st USB Host\n");	
	}
	else
	{
		printk("probe 2nd USB Host\n");	
	}
					
	writel(0x02900157, (pci_regs + 0x04));
	writel(0x00008008, (pci_regs + 0x0c)); 	
	writel(pdev->resource[0].start, (pci_regs + 0x10));	
	writel(0x1000041E, (pci_regs + 0x44));
	writel(0xC4000071, (pci_regs + 0x40)); 
	rsrc_start = pdev->resource[3].start;
	rsrc_len = pdev->resource[3].end - pdev->resource[3].start + 1;
	
	if (!request_mem_region(rsrc_start, rsrc_len, "ALi-EHCI (PHY regs)")) {
		pr_debug("request_mem_region failed");
	}

	phy_regs = ioremap(rsrc_start, rsrc_len);
	if (!phy_regs) {
		pr_debug("phy ioremap failed");
	}

	printk("%s  phy_regs=(0x%llx, 0x%p)\n",__func__, rsrc_start, phy_regs);		
	switch (chip_id)
	{
		case C3921:
			hw_setting = (struct ali_usb_setting*)&C3921_XXX_V01_setting;
			if (0x01 == (chip_package & 0x01))/*BGA package.*/
			{
				/*turn on 5V*/
				if (probe_1st)
				{
					if (g_usb_p0_host_5v_gpio > 0)
					{	
						gpio_enable_pin(g_usb_p0_host_5v_gpio);
						/*ali_gpio_direction_output(g_usb_p0_host_5v_gpio, 1);*/
						ali_gpio_set_value(g_usb_p0_host_5v_gpio, 1);
					}	
				}
			}

			trim = get_otp_value(0xE1);
			if (probe_1st)
			{	
				trim &= 0x003f0000;//[18:16][21:19].
			}	
			else
			{
				trim &= 0x00fc0000;//[24:22][27:15].
			}
			writel((readl(phy_regs + 0x14) | trim), (phy_regs + 0x14));
			break;
		
		case C3503:					
			if ((chip_package == 0x10) && (chip_ver == 0x02))
			{
				hw_setting = (struct ali_usb_setting*)&C3503_BGA_XXX_setting;
			}
			else	
			{
				hw_setting = (struct ali_usb_setting*)&C3503_QFP_XXX_setting;
			}
			break;
		
		case C3821:
			if ((0x01 == chip_package) || (0x00 == ((chip_package >>3) & 0x03)))/* BGA pin*/
			{
				hw_setting = (struct ali_usb_setting*)&C3821_BGA_XXX_setting;
			}
			else if (0x02 == ((chip_package >>3) & 0x03) )/* QFP156 pin*/
			{
				hw_setting = (struct ali_usb_setting*)&C3821_QFP156_XXX_setting;
			}
			else	/*QFP 128 pin*/
			{
				hw_setting = (struct ali_usb_setting*)&C3821_QFP128_XXX_setting;
			}
			break;	
		
		case C3701:
			hw_setting = (struct ali_usb_setting*)&C3701_XXX_V01_setting;
			break;
			
		default:
			printk("Not Support Chip !!\n");	
			break;	
	}

	if(hw_setting)
	{
		while(1)
		{
			if (-1 == hw_setting[idx].reg_addr)
			{
				break;
			}
			writel(hw_setting[idx].reg_val, (phy_regs + hw_setting[idx].reg_addr));	
			printk("Wr USB setting (0x%02x ,0x%08x) (rd back 0x%08x)\n", hw_setting[idx].reg_addr, 
					hw_setting[idx].reg_val, readl(phy_regs + hw_setting[idx].reg_addr));	
			idx++;		
		}
	}
}
#endif

static void ali_stop_ehc(struct platform_device *pdev)
{
	/* Disable mem */
//	u64 rsrc_start, rsrc_len;

/*
	rsrc_start = pdev->resource[2].start;
	rsrc_len = pdev->resource[2].end - pdev->resource[2].start + 1;	
	release_mem_region(rsrc_start, rsrc_len);	
*/
	/* Disable EHC clock. If the HS PHY is unused disable it too. */
}

static int ali_ehci_set_ehci(struct ehci_hcd *ehci)
{
    struct ali_ehci_hcd *ali;
    ali = (struct ali_ehci_hcd *)ehci->priv;

    ehci->caps = ali->cap_regs;
    ehci->regs = ali->cap_regs + HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));

    /* ali patch setting */
    writel((readl(&ehci->caps->hcc_params)&0xFFFE), &ehci->caps->hcc_params);   //32bits mode
    writel(((readl(&ehci->caps->hcs_params)&0xFFF0)|0x3), &ehci->caps->hcs_params);   //three ports in M3901
    /* cache this readonly data; minimize chip reads */
    ehci->hcs_params = readl(&ehci->caps->hcs_params);
    
    return 0;
}

static int ali_ehci_set_phy(struct ali_ehci_hcd *ali)
{
    int idx = 0;

    if(NULL == ali)
        return -1;

    if(NULL == ali->phy_addr_array || NULL == ali->phy_value_array)
        return 0;

    for(idx = 0; idx < ali->phy_setting_num; idx++)
    {
        writel(ali->phy_value_array[idx], (ali->phy_regs + ali->phy_addr_array[idx]));
        printk("Wr USB setting (0x%02x ,0x%08x) (rd back 0x%08x)\n", 
                    ali->phy_addr_array[idx], ali->phy_value_array[idx], 
                        readl(ali->phy_regs + ali->phy_addr_array[idx]));
    }
    return 0;
}

static int ali_ehci_set_pci(struct ali_ehci_hcd *ali)
{
    if(NULL == ali)
        return -1;
    
    writel(0x02900157, (ali->pci_regs + 0x04));
    writel(0x00008008, (ali->pci_regs + 0x0c));
    writel(ali->cap_phy_regs, (ali->pci_regs + 0x10));
    writel(0x1000041E, (ali->pci_regs + 0x44));
    writel(0xC4000071, (ali->pci_regs + 0x40)); 
    return 0;
}


#ifdef CONFIG_PM

static int ehci_ali_drv_resume(struct platform_device *dev)
{
    struct usb_hcd	*hcd = platform_get_drvdata(dev);
    struct ehci_hcd	*ehci = hcd_to_ehci(hcd);
    struct ali_ehci_hcd *ali = (struct ali_ehci_hcd *)ehci->priv;

    //ali_start_ehc(dev);
    if(ali_ehci_set_pci(ali))
        return -1;
    if(ali_ehci_set_phy(ali))
        return -1;
	
    ehci_resume(hcd, false);
    return 0;
}

static int ehci_ali_drv_suspend(struct platform_device *dev, pm_message_t message)
{
    struct ehci_hcd	*ehci = hcd_to_ehci(platform_get_drvdata(dev));

	ali_stop_ehc(dev);
	ehci_to_hcd(ehci)->state = HC_STATE_SUSPENDED;
    return 0;
}

#endif

static void __iomem *ali_get_ioremap_regs(struct platform_device *pdev, u32 
phys, unsigned long size, char*name)
{
    void __iomem *virt;

    if(NULL == pdev)
        return NULL;
    
    if (!devm_request_mem_region(&pdev->dev, phys, size, name)) {
        printk("EHCI 0x%x request_mem_region failed", phys);
        return NULL;
    }

    virt = devm_ioremap(&pdev->dev, phys, size);
    if (!virt) {
        printk("EHCI 0x%x Failed to remap I/O memory\n", phys);
        return NULL;
    }

    return virt;
}

static int ali_ehci_map_regs(struct platform_device *pdev, struct ali_ehci_hcd *ali)
{

    if(NULL == ali || NULL == pdev)
      return -1;

    ali->pci_regs = ali_get_ioremap_regs(pdev, ali->pci_phy_regs, ali->pci_len, "ALi-EHCI (PCI regs)");
    if(!ali->pci_regs){
        dev_err(&pdev->dev, "Failed to get EHCI pci regs.\n");
        return -1;
    }

    ali->cap_regs = ali_get_ioremap_regs(pdev, ali->cap_phy_regs, ali->cap_len, "ALi-EHCI (Capability and Operation regs)");
    if(!ali->cap_regs){
        dev_err(&pdev->dev, "Failed to get EHCI cap regs.\n");
        return -1;
    }

    ali->phy_regs = ali_get_ioremap_regs(pdev, ali->phy_phy_regs, ali->phy_len, "ALi-EHCI (PHY regs)");
    if(!ali->phy_regs){
        dev_err(&pdev->dev, "Failed to get EHCI phy regs.\n");
        return -1;
    }
    
    return 0;
}

static int ali_ehci_parse_dts_regs(const struct device_node *np, struct ali_ehci_hcd *ali)
{
    struct property *t = NULL;
    u32 *array = NULL;
    int i = 0;

    if(of_property_read_u32_index(np, "reg", 0, &ali->pci_phy_regs)){
        printk("get reg failed...\n");
        return -1;
    }
    if(of_property_read_u32_index(np, "reg", 1, &ali->pci_len)){
        printk("get reg len failed...\n");
        return -1;
    }
    
    if(of_property_read_u32_index(np, "cmd_reg", 0, &ali->cap_phy_regs)){
        printk("get cmd_reg failed...\n");
        return -1;
    }
    if(of_property_read_u32_index(np, "cmd_reg", 1, &ali->cap_len)){
        printk("get cmd len failed...\n");
        return -1;
    }
    
    if(of_property_read_u32_index(np, "phy_reg", 0, &ali->phy_phy_regs)){
        printk("get phy_reg failed...\n");
        return -1;
    }
    if(of_property_read_u32_index(np, "phy_reg", 1, &ali->phy_len)){
        printk("get phy len failed...\n");
        return -1;
    }
    
    ali->phy_addr_array = NULL;
    ali->phy_value_array = NULL;
    ali->phy_setting_num = 0;
    t = of_find_property(np, "phy-setting", NULL);
    if(NULL == t){
        printk("EHCI has no phy-setting.\n");
        return 0;
    }        

    ali->phy_setting_num = t->length/sizeof(u32)/2;
    ali->phy_addr_array = kmalloc(ali->phy_setting_num*sizeof(u32), GFP_KERNEL);
    ali->phy_value_array = kmalloc(ali->phy_setting_num*sizeof(u32), GFP_KERNEL);

    array = kmalloc(t->length, GFP_KERNEL);
    i = 0;
    if(of_property_read_u32_array(np, "phy-setting", array, t->length/sizeof(u32))){
        printk("get phy-setting failed...\n");
        return 0;
    }
    for(i = 0;i < ali->phy_setting_num; i++){
        ali->phy_addr_array[i] = array[2*i];
        ali->phy_value_array[i] = array[2*i+1];
    }
    kfree(array);
    array = NULL;
    
    return 0;
}


static int ehci_hcd_ali_drv_probe(struct platform_device *pdev)
{
    struct usb_hcd *hcd;
    struct ehci_hcd *ehci;
    int ret = 0;
    int err = 0;
    int irq = 0;
    const struct of_device_id *match;
    struct ali_ehci_hcd *ali;

	printk("%s\n",__func__);	

    match = of_match_device(ali_ehci_of_match, &pdev->dev);
    if (!match) {
        dev_err(&pdev->dev, "Error: No device match found\n");
        return -ENODEV;
	}
       
    if (!pdev->dev.dma_mask)    
        pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
    if (!pdev->dev.coherent_dma_mask)
        pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	hcd = usb_create_hcd(&ehci_ali_hc_driver, &pdev->dev, "ALi HCD");		
	if (!hcd)
		return -ENOMEM;

    platform_set_drvdata(pdev, hcd);
    ehci = hcd_to_ehci(hcd);
    ali = (struct ali_ehci_hcd *)ehci->priv;

    if(ali_ehci_parse_dts_regs(pdev->dev.of_node, ali)){
        dev_err(&pdev->dev, "Failed to get EHCI regs.\n");
        return -ENOMEM;
	}

    if(ali_ehci_map_regs(pdev, ali)){
        dev_err(&pdev->dev, "Failed to map EHCI regs.\n");
        return -ENOMEM;
	}

    if(ali_ehci_set_pci(ali))
        return -ENOMEM;
    if(ali_ehci_set_phy(ali))
        return - ENOMEM;
    if(ali_ehci_set_ehci(ehci))
        return -ENOMEM;

    hcd->regs = ali->cap_regs;
	/* ali patch setting */
    irq = platform_get_irq(pdev, 0);
    if (!irq) {
        dev_err(&pdev->dev, "Failed to get IRQ\n");
        err = -ENODEV;
        return err;
    }
	/* cache this readonly data; minimize chip reads */
    printk("pci_reg:0x%x, cap_reg:0x%x, phy_reg:0x%x, irq:%d\n", (u32)ali->pci_regs, 
                    (u32)ali->cap_regs, (u32)ali->phy_regs, irq);

    err = usb_add_hcd(hcd, irq, IRQF_SHARED);
    if (err) {
        dev_err(&pdev->dev, "Failed to add USB HCD\n");
    }

	return ret;

}

static int ehci_hcd_ali_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	//release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	ali_stop_ehc(pdev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver ehci_hcd_ali_driver = {
	.probe		= ehci_hcd_ali_drv_probe,
	.remove		= ehci_hcd_ali_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
#ifdef	CONFIG_PM
	.resume     = ehci_ali_drv_resume,
	.suspend    = ehci_ali_drv_suspend,
#endif
	.driver = {
		.name	= "Ali-ehci",
		.owner	= THIS_MODULE,
		.of_match_table = ali_ehci_of_match,
	}
};

MODULE_ALIAS("platform:ali-ehci");
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI ali-ehci");
MODULE_LICENSE("GPL");
