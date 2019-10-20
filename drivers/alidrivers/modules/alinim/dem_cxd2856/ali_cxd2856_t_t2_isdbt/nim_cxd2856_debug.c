/*----------------------------------------------------------------------------
   Solution:  demodulater CXD2856. + tuner MXL603
------------------------------------------------------------------------------
    History:       
	Date                  athor           version                reason 
------------------------------------------------------------------------------
    2017/04/06          leo.liu           v0.1              create the debug file for cxd2856 driver     
----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kobject.h>

#include "nim_cxd2856_common.h"

char device_name[2][20] ={"cxd2856_0","cxd2856_1"};
static int dev_nu = 0;
struct nim_device * dev[2]= {};
static void cxd2856_task_monitor(struct work_struct *nim_work)
{
	struct nim_device 	* dev = NULL;
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pDemod = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	UINT32 count = 0;
	
	int lock = 0;
	int ssi = 0;
	int sqi = 0;
	int cn = 0;
	int rf_level = 0;
	unsigned long parg;
	if(nim_work == NULL)
	{
		return;
	}
    dev = container_of((void *)nim_work,struct nim_device, work);
	if(dev == NULL)
	{
		return;
	}
	priv= dev->priv;
	pDemod = priv->pDemod;
	user = priv->user;
	
	printk("start sysfs monitor task %d\n",user->monitor_status);
	while(user->monitor_status == TASK_RUN)
	{
		msleep(100);//must set this code
		if(!user->nim_init)//start monitor after hardware init
		{
			continue;
		}
		if(count == 1)
		{
			
			lock = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_LOCK_STATUS,parg);
			rf_level = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_RF_LEVEL,parg);
			ssi = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_AGC,parg);
		 	sqi = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_SNR,parg);
			cn = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_CN_VALUE,parg);
				
			printk("lock = %d cn = %d rf = %d ssi = %d sqi = %d\n",lock,cn,rf_level,ssi,sqi);
			count = 0;
		}
		count ++;
	}
	user->monitor_status = TASK_EXIT;//task has exited
	printk("exit sysfs monitor task\n");
}

static ssize_t cxd2856_monitor_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;
	if(strcmp(device_name[0],kobj->name))
	{
		if(strcmp(device_name[1],kobj->name))
		{
			return 0;
		}
		nim_dev = dev[1];
		
	}
	nim_dev = dev[0];
	priv = nim_dev->priv;
	user = priv->user;
	

	printk("monitor is %s\n",(user->monitor_status == TASK_DEFAULT) ? "disable" : "enable");
	
	return 0;
}
static ssize_t cxd2856_monitor_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
	struct sony_demod_t * pdemod = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;
	if(strcmp(device_name[0],kobj->name))
	{
		if(strcmp(device_name[1],kobj->name))
		{
			return 0;
		}
		nim_dev = dev[1];
		
	}
	nim_dev = dev[0];
	priv = nim_dev->priv;
	pdemod = priv->pDemod;
	user = priv->user;
	switch (buf[0]) 
	{
		case '0':
			if(TASK_DEFAULT == user->monitor_status)
			{
				printk("\ndisable monitor\n");
				break;
			}
			user->monitor_status = TASK_DEFAULT;
			while(user->monitor_status != TASK_EXIT)//wait task exit
			{
				msleep(5);
			}
			cancel_work_sync(&nim_dev->work);//remove task from work queue
			user->monitor_status = TASK_DEFAULT;
			printk("\ndisable monitor\n");
			break;
		case '1':
			if(TASK_RUN == user->monitor_status)
			{
				printk("\nenable monitor\n");
				break;
			}
			user->monitor_status = TASK_RUN;
			schedule_work(&nim_dev->work);
			printk("\nenable monitor\n");
			break;
		case '2':
			if(!user->nim_init)//read chipid after hardware init
			{
				printk("\n  !!!can't read chipid,because hw don't INIT\n\n");
				break;
			}
			ret = sony_demod_ChipID (pdemod, &(pdemod->chipId));
    		if (ret != SONY_RESULT_OK)
			{
       		 	printk("\nread chipid error\n");
    		}
			printk("cxd2856 chipid = 0x%x\n",pdemod->chipId);
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret < 0 ? ret : count;
}
static ssize_t cxd2856_i2c_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
	struct sony_demod_t * pdemod = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;
	if(strcmp(device_name[0],kobj->name))
	{
		if(strcmp(device_name[1],kobj->name))
		{
			return 0;
		}
		nim_dev = dev[1];
		
	}
	nim_dev = dev[0];
	priv = nim_dev->priv;
	pdemod = priv->pDemod;
	user = priv->user;
	if(user->nim_init)//read chipid after hardware init
	{
		if(sony_demod_ChipID (pdemod, &(pdemod->chipId)) !=SONY_RESULT_OK )
		{
		 	printk("\nread chipid error\n");
		}
	}
	else
	{
		printk("\n  !!!can't read chipid,because hw don't INIT\n\n");
	}
	printk("cxd2856 chipid = 0x%x tuner i2c mode is %s\n"
		   "demod i2c id = 0x%lx demod i2c addr = 0x%x \n"
		   "tuner i2c id = 0x%lx tuner i2c addr = 0x%x \n",
		   pdemod->chipId,
		   (user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_GATEWAY) ? "gatway" : "repeater",
		   user->demod_i2c_type,user->demod_addr,
		   user->tuner_i2c_type,user->tuner_addr);
	
	
	return 0;
}
static ssize_t cxd2856_i2c_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
	struct sony_demod_t * pdemod = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;
	if(strcmp(device_name[0],kobj->name))
	{
		if(strcmp(device_name[1],kobj->name))
		{
			return 0;
		}
		nim_dev = dev[1];
		
	}
	nim_dev = dev[0];
	priv = nim_dev->priv;
	pdemod = priv->pDemod;
	user = priv->user;
	printk("=== %s\n",buf);
	
	return ret < 0 ? ret : count;
}

static struct kobj_attribute cxd2856_monitor_attr =
	__ATTR(debug_monitor, 0644, cxd2856_monitor_show, cxd2856_monitor_store);
static struct kobj_attribute cxd2856_i2c_attr =
	__ATTR(debug_i2c, 0644, cxd2856_i2c_show, cxd2856_i2c_store);

#if 0
static struct kobj_attribute cxd2856_info_attr =
	__ATTR(cxd2856_monitor_info, 0644, cxd2856_info_show, cxd2856_info_store);
static struct kobj_attribute cxd2856_per_attr =
	__ATTR(cxd2856_monitor_per, 0644, cxd2856_per_show, cxd2856_per_store);

static struct attribute *attrs[] = {
    (struct attribute *)&cxd2856_switch_attr,
    (struct attribute *)&cxd2856_info_attr,
    (struct attribute *)&cxd2856_per_attr,
    NULL,	/* need to NULL terminate the list of attributes */
};
#endif
#if 0
static struct kobj_attribute edid_info_show_attribute = __ATTR(edid_info_show,  0664, b_show, b_store);
static struct kobj_attribute debug_log_onoff_attribute = __ATTR(debug_log_onoff,  0664, b_show, b_store);
static struct kobj_attribute chip_id_attribute = __ATTR(chip_id,  0664, b_show, b_store);
static struct kobj_attribute hdcp_version_attribute = __ATTR(hdcp_version,  0664, b_show, b_store);
static struct kobj_attribute hdmi_link_status_attribute = __ATTR(hdmi_link_status,  0664, b_show, b_store);

static struct attribute *attrs[] = {
    (struct attribute *)&edid_info_show_attribute,
    (struct attribute *)&debug_log_onoff_attribute,
    (struct attribute *)&chip_id_attribute,
    (struct attribute *)&hdcp_version_attribute,
    (struct attribute *)&hdmi_link_status_attribute,
    NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};
int hdmi_sysfs_init(struct kobject *kobj)
{
    int retval  = -1;

    if (kobj != NULL)
    {   
        retval = sysfs_create_group(kobj, &attr_group);
        hdmi_edid_debug_add();
    }

    
    return retval;

}
#endif
int  nim_cxd2856_sysfs_init(struct platform_device * pdev)
{
	unsigned long 		  ret;
	struct nim_device 	* nim_dev = platform_get_drvdata(pdev);
	sony_integ_t 		* priv = (sony_integ_t *)nim_dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	struct kobject 		* nim_cxd2856_kobject;
	
	if(dev_nu > 1)
	{
		printk(KERN_WARNING "beyond max dev id\n");
		return 0;
	}
	if (!nim_cxd2856_kobject)
		nim_cxd2856_kobject = kobject_create_and_add(device_name[dev_nu], NULL);
	if (!nim_cxd2856_kobject) {
		printk(KERN_WARNING "kobject_create_and_add cxd2856 failed\n");
		return -EINVAL;
	}
	user->nim_cxd2856_kobject = nim_cxd2856_kobject;
	
	//printk("kobject addr = %p %p %p\n",nim_cxd2856_kobject,user->nim_cxd2856_kobject,user);
	ret = sysfs_create_file(nim_cxd2856_kobject, &cxd2856_monitor_attr.attr);
	//ret = sysfs_create_group(nim_cxd2856_kobject, &attr_group);
	if (ret) {
		printk(KERN_WARNING "sysfs_create_file cxd2856_monitor_attr failed\n");
		return ret;
	}
	ret = sysfs_create_file(nim_cxd2856_kobject, &cxd2856_i2c_attr.attr);
	if (ret) {
		printk(KERN_WARNING "sysfs_create_file cxd2856_i2c_attr failed\n");
		return ret;
	}
	INIT_WORK(&nim_dev->work,cxd2856_task_monitor);
	user->monitor_status = TASK_DEFAULT;
	dev[dev_nu] = nim_dev;
	dev_nu ++;
	return 0;
}
int  nim_cxd2856_sysfs_exit(struct platform_device * pdev)
{
	//unsigned long 		  ret;
	struct nim_device 	* nim_dev = platform_get_drvdata(pdev);
	sony_integ_t 		* priv = (sony_integ_t *)nim_dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	struct kobject 		* nim_cxd2856_kobject = user->nim_cxd2856_kobject;

	sysfs_remove_file(nim_cxd2856_kobject,&cxd2856_monitor_attr.attr);
	sysfs_remove_file(nim_cxd2856_kobject,&cxd2856_i2c_attr.attr);
	dev_nu --; 
	return 0;
}


