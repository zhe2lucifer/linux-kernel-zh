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
#include <linux/signal.h>

#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

static const struct of_device_id ali_ohci_of_match[] = {
	{ .compatible = "alitech,ohci"},
	{ },
};

struct ali_ohci_hcd {
    void __iomem *pci_regs;
    void __iomem *phy_regs;
};

static int __devinit ohci_ali_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	int ret;

	ohci_dbg(ohci, "ohci_ali_start, ohci:%p", ohci);

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		//err ("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}
	return 0;
}

static const struct hc_driver ohci_ali_hc_driver = {
	.description =		"ohci hcd",
	.product_desc =		"ALi OHCI",
	.hcd_priv_size =	sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			ohci_irq,
	.flags =		HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.start =			ohci_ali_start,
	.stop =			ohci_stop,
	.shutdown =		ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ohci_urb_enqueue,
	.urb_dequeue =		ohci_urb_dequeue,
	.endpoint_disable =	ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ohci_hub_status_data,
	.hub_control =		ohci_hub_control,
#ifdef	CONFIG_PM
	.bus_suspend =		ohci_bus_suspend,
	.bus_resume =		ohci_bus_resume,
#endif
	.start_port_reset =	ohci_start_port_reset,
};

#if 0
static void ali_start_ohc(struct platform_device *pdev)
{
	/* enable host controller */

	/* enable EHCI mmio */
	u64 rsrc_start, rsrc_len;
	void __iomem		*pci_regs;

	rsrc_start = pdev->resource[2].start;
	rsrc_len = pdev->resource[2].end - pdev->resource[2].start + 1;
	
	if (!request_mem_region(rsrc_start, rsrc_len, "ALi-OHCI (PCI config regs)")) {
		pr_debug("request_mem_region failed");
	}

	pci_regs = ioremap(rsrc_start, rsrc_len);
	if (!pci_regs) {
		pr_debug("ioremap failed");
	}
	
	writel(0x02900157, (pci_regs + 0x04));
	//writel(0x00002008, (pci_regs + 0x0c)); //0x00008010;
	writel(pdev->resource[0].start, (pci_regs + 0x10));
	//writel(0x1000041E, (pci_regs + 0x44));	
	
#if 0
	if ((readl(0xB8000000) & 0xFFFF0000) != 0x37010000)
	{
		(*(volatile u32*)0xB803D810) = (*(volatile u32*)0xB803D810) | 0x0049AB64;
	}

	if ((readl(0xB8000000) & 0xFFFF0000) == 0x37010000)
	{
		
		//(*(volatile u32*)0xB803D814) =  0x0005C9D4;;	//20120808 change GPSR14 register 
		writel(0x0005C9D4, (gadget_regs + 0x14));
		//(*(volatile u32*)0xB803D810) &= 0x80FFFFFF;
		writel(readl(gadget_regs + 0x14) & 0x80FFFFFF, (gadget_regs + 0x14));
		//(*(volatile u32*)0xB803D810) |= 0x87FFFFFF;
		writel(readl(gadget_regs + 0x14) | 0x87FFFFFF, (gadget_regs + 0x14));
	}
#endif	
}
#endif

static void ali_stop_ohc(struct platform_device *pdev)
{
	/* Disable mem */
	u64 rsrc_start, rsrc_len;

	rsrc_start = pdev->resource[2].start;
	rsrc_len = pdev->resource[2].end - pdev->resource[2].start + 1;
	
	release_mem_region(rsrc_start, rsrc_len);
	
	/* Disable clock */
}

static void __iomem *ali_ohci_get_virt_regs_by_dev(struct platform_device *pdev, 
void* prop, void* name)
{
    u32 len;
    u32 regs;
    void __iomem *virt = NULL;

    
    if(of_property_read_u32_index(pdev->dev.of_node, prop, 0, &regs)){
        printk("[%s] get reg failed...\n", __func__);
        return NULL;
    }
    if(of_property_read_u32_index(pdev->dev.of_node, prop, 1, &len)){
        printk("[%s] get reg len failed...\n", __func__);
        return NULL;
    }

    if (!devm_request_mem_region(&pdev->dev, regs, len, name)) {
        printk("EHCI 0x%x request_mem_region failed", regs);
        return NULL;
    }

    virt = devm_ioremap(&pdev->dev, regs, len);
    if (!virt) {
        printk("EHCI 0x%x Failed to remap I/O memory\n", regs);
        return NULL;
    }

    return virt;
}

static int ali_ohci_set_pci(struct platform_device *pdev, void __iomem *pci_regs){
    
    writel(0x02900157, (pci_regs + 0x04));
    //writel(0x00002008, (pci_regs + 0x0c)); //0x00008010;
    writel(pdev->resource[0].start, (pci_regs + 0x10));
    
    return 0;
}

static int ohci_hcd_ali_dts_drv_probe(struct platform_device *pdev)
{

    const struct of_device_id *match;
    struct usb_hcd *hcd;
    void __iomem *pci_regs, *phy_regs;
    struct ohci_hcd	*ohci;
    struct ali_ohci_hcd *ali;
    int irq = 0;

    match = of_match_device(ali_ohci_of_match, &pdev->dev);
    if (!match) {
        dev_err(&pdev->dev, "Error: No device match found\n");
        return -ENODEV;
    }

    if (!pdev->dev.dma_mask)    
        pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
    if (!pdev->dev.coherent_dma_mask)
        pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

    hcd = usb_create_hcd(&ohci_ali_hc_driver, &pdev->dev, "ALi-OHCI-HCD");
    if (!hcd)
	return -ENOMEM;

    phy_regs = ali_ohci_get_virt_regs_by_dev(pdev, "reg", "OHCI-PHY");
    if(!phy_regs){
        dev_err(&pdev->dev, "Error: Can't get dts phy_reg regs.\n");
        return -ENOMEM;
    }

    pci_regs = ali_ohci_get_virt_regs_by_dev(pdev, "pci_reg", "OHCI-PCI");
    if(!pci_regs){
        dev_err(&pdev->dev, "Error: Can't get dts reg regs.\n");
        return -ENOMEM;
    }

    hcd->rsrc_start = pdev->resource[0].start;
    hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;
    hcd->regs = phy_regs;
    
    printk("rsrc_start:0x%x, len:0x%x, reg:0x%x\n", (u32)hcd->rsrc_start, (u32)hcd->rsrc_len, (u32)hcd->regs);

    platform_set_drvdata(pdev, hcd);

    ohci = hcd_to_ohci(hcd);
    ali = (struct ali_ohci_hcd *)ohci->priv;
    ali->pci_regs = pci_regs;
    ali->phy_regs = phy_regs;

    ali_ohci_set_pci(pdev, pci_regs);
    ohci_hcd_init(ohci);

    irq = platform_get_irq(pdev, 0);
    if (!irq) {
        dev_err(&pdev->dev, "Failed to get IRQ\n");
        return -ENODEV;
    }

    printk("[%s] irq:%d, pci_regs:0x%x, phy_regs:0x%x\n", __func__, irq, (u32)pci_regs, (u32)phy_regs);

    if(usb_add_hcd(hcd, irq, IRQF_DISABLED) < 0){
        dev_err(&pdev->dev, "Error can't add hcd\n");
        return -ENOMEM;
    }

    return 0;
}

#if 0
static int ohci_hcd_ali_drv_probe(struct platform_device *pdev)
{
	int ret;
	struct usb_hcd *hcd;

	printk("%s\n",__func__);
	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ\n");
		return -ENOMEM;
	}

	hcd = usb_create_hcd(&ohci_ali_hc_driver, &pdev->dev, "ALi");
	if (!hcd)
		return -ENOMEM;

	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		pr_debug("request_mem_region failed\n");
		ret = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		pr_debug("ioremap failed\n");
		ret = -ENOMEM;
		goto err2;
	}

	ali_start_ohc(pdev);
	ohci_hcd_init(hcd_to_ohci(hcd));

	ret = usb_add_hcd(hcd, pdev->resource[1].start,
			  IRQF_DISABLED /*| IRQF_SHARED*/);
	if (ret == 0) {
		platform_set_drvdata(pdev, hcd);
		return ret;
	}

	ali_stop_ohc(pdev);
	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
	return ret;
}
#endif

static int ohci_hcd_ali_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	//ali_stop_ohc(pdev);
	//iounmap(hcd->regs);
	//release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int ohci_hcd_ali_drv_suspend(struct platform_device *dev, pm_message_t message)
{
	struct ohci_hcd	*ohci = hcd_to_ohci(platform_get_drvdata(dev));

	if(time_before(jiffies, ohci->next_statechange))
	{
		mdelay(5);
	}
	ohci->next_statechange = jiffies;

	//ali_stop_ohc(dev);
	ohci_to_hcd(ohci)->state = HC_STATE_SUSPENDED;
	return 0;
}

static int ohci_hcd_ali_drv_resume(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
        struct ali_ohci_hcd *ali = (struct ali_ohci_hcd*)ohci->priv;

        printk("entry %s\n", __func__);

	if(time_before(jiffies, ohci->next_statechange))
	{
		mdelay(5);
	}
	ohci->next_statechange = jiffies;

        ali_ohci_set_pci(dev, ali->pci_regs);
	//ali_start_ohc(dev);
	ohci_resume(hcd, false);
	//ohci_finish_controller_resume(hcd);
	return 0;
}
#endif

static struct platform_driver ohci_hcd_ali_driver = {
	.probe		= ohci_hcd_ali_dts_drv_probe,
	.remove		= ohci_hcd_ali_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
#ifdef	CONFIG_PM
	.suspend		= ohci_hcd_ali_drv_suspend,
	.resume		= ohci_hcd_ali_drv_resume,
#endif
	.driver		= {
		.name	= "Ali-ohci",
		.owner	= THIS_MODULE,
		.of_match_table = ali_ohci_of_match,
	},
};

MODULE_ALIAS("platform:ali-ohci");
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI ali-ohci");
MODULE_LICENSE("GPL");