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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <asm/io.h>
//#include <ali_interrupt.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/delay.h>

#include <linux/mm.h>

#include <ali_soc.h>
#include "dmx_see_interface.h"
#include "dmx_stack.h"


enum DMX_DRV_STATE g_dmx_drv_state = DMX_DRV_EXIT;
struct platform_device *pdev_dt = NULL;
extern int dmx_debug_procfs_init(void);
static int dmx_init(void)
{ 
    __u32 chip_id;
	
    DMX_INIT_DEBUG("%s, %d\n", __FUNCTION__, __LINE__);

    /* Step 1: Init SEE part.
	*/
	dmx_see_init();


	/* Step 2: Init HW independent part.
	*/
    dmx_pid_flt_module_init();

    dmx_ts_flt_module_init();

    dmx_sec_flt_module_init();

    dmx_data_buf_module_init();

    dmx_stream_module_init();

    dmx_mutex_module_init();

	
	/* Step 3: Init HW dependent part. 
	 */
	/*
	 * We abstract dmx data source buffer and it's control mechanism as a "hw_interface", which contains a
	 * chunk of physically continuous SRAM buffer and a bounch of fuctions to manupulate it.
	 * Each of this "hw_interface" is assigned an ID for identification.
	 * A "hw_interface" could be bounded to a "data_engine" to retrive data from it;
	 * A "hw_interface" could also be bounded to a "linux_interface" to communicate with userspace.
	 */
	 
    /* Init HW DMX 0, assign hw_interface ID 0 to it. 
	 */
    dmx_hw_interface_init(ALI_HWDMX0_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 0);

    /* Init data engine for dev ID 0, this engine will fetch data from hw_interface 0.
	*/
#ifndef DMX_HWDMX_DATA_ENGINE_DELAY	
    dmx_data_engine_module_init_kern(ALI_HWDMX0_OUTPUT_HWIF_ID, ALI_HWDMX0_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
#endif

	/* Init linux interface for dev ID 0, user space dev name defined by ALI_HWDMX0_OUTPUT_NAME.
	*/
    dmx_linux_output_interface_init(ALI_HWDMX0_OUTPUT_HWIF_ID, ALI_HWDMX0_OUTPUT_NAME);

    dmx_subt_if_init(ALI_HWDMX0_OUTPUT_HWIF_ID, "ali_hwdmx0_subt");


    /* Init HW DMX 1, assign hw_interface ID 1 to it. 
	 */
    dmx_hw_interface_init(ALI_HWDMX1_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 1);

    /* Init data engine for hw_interface 1, this engine will fetch data from hw_interface 1.
	*/
#ifndef DMX_HWDMX_DATA_ENGINE_DELAY		
    dmx_data_engine_module_init_kern(ALI_HWDMX1_OUTPUT_HWIF_ID, ALI_HWDMX1_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
#endif

	chip_id = ali_sys_ic_get_chip_id();

    //if ((ALI_C3921 == chip_id) || (ALI_S3821 == chip_id))
	//if (ALI_C3921 == chip_id)	
    {
    	/* Init linux interface for hw_interface 1, user space could access serivce provided by 
    	 * this interface by dev name defined by ALI_HWDMX1_OUTPUT_NAME.
    	 */	
        dmx_linux_output_interface_init(ALI_HWDMX1_OUTPUT_HWIF_ID, ALI_HWDMX1_OUTPUT_NAME);
    	
        dmx_subt_if_init(ALI_HWDMX1_OUTPUT_HWIF_ID, "ali_hwdmx1_subt");
    
        /* Init HW DMX 2, assign hw_interface ID 2 to it. 
    	 */
        dmx_hw_interface_init(ALI_HWDMX2_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 2);
    
        /* Init data engine for hw_interface 2, this engine will fetch data from hw_interface 2.
    	*/
#ifndef DMX_HWDMX_DATA_ENGINE_DELAY	    	
        dmx_data_engine_module_init_kern(ALI_HWDMX2_OUTPUT_HWIF_ID, ALI_HWDMX2_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
#endif
	
    	/* Init linux interface for hw_interface 2, user space could access serivce provided by 
    	 * this interface by dev name defined by ALI_HWDMX2_OUTPUT_NAME.
    	 */	
        dmx_linux_output_interface_init(ALI_HWDMX2_OUTPUT_HWIF_ID, ALI_HWDMX2_OUTPUT_NAME);
    
        dmx_subt_if_init(ALI_HWDMX2_OUTPUT_HWIF_ID, "ali_hwdmx2_subt");
    	
    
        /* Init HW DMX 3, assign hw_interface ID 3 to it. 
    	 */
        dmx_hw_interface_init(ALI_HWDMX3_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 3);
  
        /* Init data engine for hw_interface 3, this engine will fetch data from hw_interface 3.
    	*/
#ifndef DMX_HWDMX_DATA_ENGINE_DELAY	     	
        dmx_data_engine_module_init_kern(ALI_HWDMX3_OUTPUT_HWIF_ID, ALI_HWDMX3_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);
#endif

    	/* Init linux interface for hw_interface 3, user space could access serivce provided by 
    	 * this interface by dev name defined by ALI_HWDMX3_OUTPUT_NAME.
    	 */	
        dmx_linux_output_interface_init(ALI_HWDMX3_OUTPUT_HWIF_ID, ALI_HWDMX3_OUTPUT_NAME);
    
        dmx_subt_if_init(ALI_HWDMX3_OUTPUT_HWIF_ID, "ali_hwdmx3_subt");
	}

    /* Init SW DMX 0, assign ALI_SWDMX0_OUTPUT_HWIF_ID to it. 
	 */
    dmx_hw_interface_init(ALI_SWDMX0_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_USR, 0);
    dmx_hw_interface_init(ALI_SWDMX1_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_USR, 1);

#ifndef DMX_SWDMX_DATA_ENGINE_DELAY
	dmx_data_engine_module_init_usr(ALI_SWDMX0_OUTPUT_HWIF_ID, ALI_SWDMX0_ENGINE_NAME, DMX_DATA_ENGINE_SRC_VIRTUAL_HW);

	dmx_data_engine_module_init_usr(ALI_SWDMX1_OUTPUT_HWIF_ID, ALI_SWDMX1_ENGINE_NAME, DMX_DATA_ENGINE_SRC_VIRTUAL_HW);
#endif

    dmx_linux_output_interface_init(ALI_SWDMX0_OUTPUT_HWIF_ID, ALI_SWDMX0_OUTPUT_NAME);
    dmx_linux_output_interface_init(ALI_SWDMX1_OUTPUT_HWIF_ID, ALI_SWDMX1_OUTPUT_NAME);
	
    dmx_linux_input_interface_init(ALI_SWDMX0_OUTPUT_HWIF_ID, ALI_SWDMX0_INPUT_NAME);
    dmx_linux_input_interface_init(ALI_SWDMX1_OUTPUT_HWIF_ID, ALI_SWDMX1_INPUT_NAME);

    dmx_subt_if_init(ALI_SWDMX0_OUTPUT_HWIF_ID, "ali_swdmx0_subt");
    dmx_subt_if_init(ALI_SWDMX1_OUTPUT_HWIF_ID, "ali_swdmx1_subt");
				
#ifndef DMX_SEE2MAINDMX_DATA_ENGINE_DELAY
	/* Enable internal engine, which will fetch data from SEE to main.
	*/
	dmx_data_engine_module_init_kern(ALI_SEETOMAIN_BUF_HWIF_ID, ALI_DMX_SEE2MAIN0_ENGINE_NAME, DMX_DATA_ENGINE_SRC_REAL_HW);	
#endif

#if 0
    dmx_hw_interface_module_init(4, DMX_HW_INTERFACE_TYPE_VIRTUAL, 1);
    
    dmx_data_engine_module_init(4, "ali_dmx_3_output", DMX_DATA_ENGINE_SRC_VIRTUAL_HW);
    
    dmx_linux_interface_module_init(4, "ali_dmx_3_output", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 4);
    
    dmx_linux_interface_module_init(4, "ali_dmx_3_input", DMX_LINUX_INTERFACE_TYPE_INPUT, 1);
#endif

    /* Enable internal engine, which will fetch data from SEE to main.
	*/
    dmx_hw_interface_init(ALI_SEETOMAIN_BUF_HWIF_ID, DMX_HW_INTERFACE_TYPE_SEE, 0);

#if 1
    /* Support linux legacy interface.
     * legacy input interface and output interface are independent to 
     * main line input interface and output interface, hence their interface_id
     * are stand by there own.
     */
    dmx_channel_module_legacy_init();
    
    dmx_linux_interface_module_legacy_init(ALI_HWDMX0_OUTPUT_HWIF_ID, "ali_m36_dmx_0", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 0);

    dmx_linux_interface_module_legacy_init(ALI_HWDMX1_OUTPUT_HWIF_ID, "ali_m36_dmx_1", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 1);

	dmx_linux_interface_module_legacy_init(ALI_HWDMX2_OUTPUT_HWIF_ID, "ali_m36_dmx_2", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 2);

	dmx_linux_interface_module_legacy_init(ALI_HWDMX3_OUTPUT_HWIF_ID, "ali_m36_dmx_3", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 3);

    dmx_linux_interface_module_legacy_init(ALI_SWDMX0_OUTPUT_HWIF_ID, "ali_dmx_pb_0_out", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 4);

	dmx_linux_interface_module_legacy_init(ALI_SWDMX1_OUTPUT_HWIF_ID, "ali_dmx_pb_1_out", DMX_LINUX_INTERFACE_TYPE_OUTPUT, 5);

    dmx_linux_interface_module_legacy_init(ALI_SWDMX0_OUTPUT_HWIF_ID, "ali_dmx_pb_0_in", DMX_LINUX_INTERFACE_TYPE_INPUT, 0);

	dmx_linux_interface_module_legacy_init(ALI_SWDMX1_OUTPUT_HWIF_ID, "ali_dmx_pb_1_in", DMX_LINUX_INTERFACE_TYPE_INPUT, 1);

    //dmx_self_test_init();
#endif

    dmx_statistic_show_init();
    dmx_debug_procfs_init();

    DMX_INIT_DEBUG("%s, %d\n", __FUNCTION__, __LINE__);

    return(0);
}


static void __exit dmx_exit(void)
{
    DMX_INIT_DEBUG(KERN_ALERT "Goodbye, cruel world\n");

    return;
}


/*
 *  Initialisation
 */
static int alidmx_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	pdev_dt = pdev;
	
	dmx_init();
	
	g_dmx_drv_state = DMX_DRV_INIT;

	return ret;
}


/*
 *  Removement
 */
static int alidmx_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	g_dmx_drv_state = DMX_DRV_EXIT;

	dmx_exit();

	return 0;
}


static int alidmx_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "%s, %d\n", __FUNCTION__, __LINE__);
#endif

	g_dmx_drv_state = DMX_DRV_SUSPEND;

	dmx_channel_module_legacy_deinit();

	/* DeInit HW DMX 0
	 */
	dmx_hw_interface_deinit(ALI_HWDMX0_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 0);


	/* DeInit HW DMX 1
	 */
	dmx_hw_interface_deinit(ALI_HWDMX1_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 1);

	/* DeInit HW DMX 2
	 */
	dmx_hw_interface_deinit(ALI_HWDMX2_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 2);

	/* DeInit HW DMX 3
	 */
	dmx_hw_interface_deinit(ALI_HWDMX3_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 3);

	return 0;
}


static int alidmx_resume(struct platform_device *pdev)
{
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "%s, %d\n", __FUNCTION__, __LINE__);
#endif

	/*Resume HW DMX0.*/
	dmx_hw_interface_init(ALI_HWDMX0_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 0);

	/*Resume HW DMX1.*/
	dmx_hw_interface_init(ALI_HWDMX1_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 1);

    /*Resume HW DMX2.*/
    dmx_hw_interface_init(ALI_HWDMX2_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 2);

    /*Resume HW DMX3.*/
    dmx_hw_interface_init(ALI_HWDMX3_OUTPUT_HWIF_ID, DMX_HW_INTERFACE_TYPE_HW, 3);

	g_dmx_drv_state = DMX_DRV_RESUME;

	return 0;
}

static const struct of_device_id ali_demux_dt_ids[] = {
	{ .compatible = "alitech,ali-demux" },
	{ }
};
MODULE_DEVICE_TABLE(of, ali_demux_dt_ids);

static struct platform_driver alidmx_driver = {
	.probe	 = alidmx_probe,
	.remove  = alidmx_remove,
	.suspend = alidmx_suspend,
	.resume  = alidmx_resume,
	.driver  =
	{
		.name	= "alidmx",
		.owner	= THIS_MODULE,
		.of_match_table = ali_demux_dt_ids,
	},
};

static int __init alidmx_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&alidmx_driver);

	printk("%s, %d, ret:%d\n", __FUNCTION__, __LINE__, ret);

	return ret;
}


static void __exit alidmx_exit(void)
{
	platform_driver_unregister(&alidmx_driver);
}


module_init(alidmx_init);
module_exit(alidmx_exit);
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI DMX driver");
MODULE_LICENSE("GPL");

