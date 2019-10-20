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

/*------------------------------------------------------------------------------
	History:	   
	Date				  athor 		  version				 reason 
------------------------------------------------------------------------------
	2017/04/06		leo.liu              v0.1               create ali cxd2856 driver 
											   to support dvbt/t2/isdbt	  
-------------------------------------------------------------------------------
NOTE:
cxd2856 support repeater and getway mode of i2c to w/r tuner.this driver has supportted the two mode,
but repteater is simple.so,I suggest you use the repeater mode of i2c,but you must enable repeater mode 
before w/r tuner,and disable repeater mode after w/r tuner .use the function: 
		sony_demod_I2cRepeaterEnable (pdemod, 0x01); // Enable the I2C repeater
		sony_demod_I2cRepeaterEnable (pdemod, 0x00); //	disable the I2C repeater
-------------------------------------------------------------------------------*/


#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include "nim_cxd2856_common.h"

#define DRV_VERSION     				"1.0.0"
#define ALI_CXD2856_DEVICE_NAME      	"ali_cxd2856"
#define ALI_CXD2856_DEV_COMPATIBLE_NAME "alitech,cxd2856"


#define MAX_TUNER_SUPPORT_NUM        1

static struct class                  *ali_cxd2856_nim_class;
static struct device                 *ali_cxd2856_nim_dev_node;
bool cxd2856_class_create_flag = true; 


static long ali_cxd2856_nim_ioctl_common(struct nim_device *dev,unsigned int cmd, unsigned long parg)
{
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
    int ret = SUCCESS;
	
    switch(cmd)
	{
		case ALI_NIM_TUNER_SELT_ADAPTION_C:
	    {
	        struct ali_nim_mn88436_cfg nim_param;
	        if(copy_from_user(&nim_param, (struct ali_nim_mn88436_cfg *)parg, sizeof(struct ali_nim_mn88436_cfg))>0)
			{
				return -EFAULT;
			}
			break;
		}
	    case ALI_NIM_HARDWARE_INIT_T:
	    {
	        struct ali_nim_mn88436_cfg nim_param;
	        memset((void*)&nim_param,0,sizeof(struct ali_nim_mn88436_cfg));
	        if(copy_from_user(&nim_param, (struct ali_nim_mn88436_cfg *)parg, sizeof(struct ali_nim_mn88436_cfg))>0)
			{
				NIM_TRACE_RETURN_ERROR("error",-EFAULT);
			}	
	        ret = ali_cxd2856_nim_hw_initialize(dev, &nim_param);
	        break;
	    }
		case ALI_NIM_DRIVER_STOP_ATUOSCAN:
	        user->autoscan_stop_flag = parg;
	        break;
	    case ALI_NIM_CHANNEL_CHANGE:
	    {
	        NIM_CHANNEL_CHANGE_T nim_param;
			memset((void*)&nim_param,0,sizeof(NIM_CHANNEL_CHANGE_T));
	        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
			{
				NIM_TRACE_RETURN_ERROR("error",-EFAULT);
			}
			if(SUCCESS != nim_cxd2856_channel_change(dev, &nim_param))
			{
				pr_err("[%s] line=%d,nim_cxd2856_channel_change failed!\n", __FUNCTION__, __LINE__);
			}

	        if(copy_to_user((NIM_CHANNEL_CHANGE_T *)parg, &nim_param, sizeof(NIM_CHANNEL_CHANGE_T))>0)
			{
				NIM_TRACE_RETURN_ERROR("error",-EFAULT);
			}
	        break;
	    }

		case ALI_NIM_DRIVER_T2_SIGNAL_ONLY:
	    {
			user->search_t2_only = parg;
			
			break;
		}
		case ALI_NIM_READ_RSUB:
		{
			UINT32 per = 0;
			nim_cxd2856_get_per(dev,&per);
			ret = per;
			break;
		}
	    case ALI_NIM_GET_LOCK_STATUS:
	    {
	        UINT8 lock = 0;
			nim_cxd2856_get_lock(dev, &lock);
	        ret = lock;
	        break;
	    }
		case ALI_NIM_GET_RF_LEVEL:
	    {
	        UINT32 rf_level = 0;
	        nim_cxd2856_get_rf_level(dev, &rf_level);
	        ret = rf_level;
	        break;
	    }
		case ALI_NIM_GET_CN_VALUE:
		{
			UINT32 cn = 0;
			nim_cxd2856_get_cn(dev, &cn);
			pr_debug("cn = %lu\n",cn);
			ret = cn;
			break;
		}
	    case ALI_NIM_READ_AGC:
	    {
	        UINT8 agc = 0;
	        nim_cxd2856_get_SSI(dev,&agc);
			pr_debug("agc = %d\n",agc);
	        ret = agc;
	        break;
	    }
		case ALI_NIM_READ_SNR:
		{
			UINT8 snr = 0;
			nim_cxd2856_get_SQI(dev,&snr);
			pr_debug("snr = %d\n",snr);
			ret=snr;
			break;
		}
		case ALI_NIM_LOG_LEVEL:
		{
	        int log_level= 0;
			if(copy_from_user(&log_level, (int *)parg, sizeof(int))>0)
			{
				NIM_TRACE_RETURN_ERROR("error",-EFAULT);
			}
	        set_log_level(log_level);
			ret = SUCCESS;
			break;
		}	
		case ALI_NIM_READ_CODE_RATE:
		{
	        UINT8 code_rate = 0;
			//nim_cxd2837_get_FEC(dev, &code_rate);
			ret=code_rate;
			break;
		}
		
		case ALI_NIM_READ_QPSK_BER:
		{
	        UINT32 ber = 0;    
	        nim_cxd2856_get_ber(dev, &ber);
	        ret = ber;
			break;
		}
		
		case ALI_NIM_READ_FREQ:
	    {
	        UINT32 freq = 0;
	        nim_cxd2856_get_freq(dev, &freq);
	        ret = freq;
	        break;
	    }	
		
	    case ALI_NIM_DRIVER_GET_FFT_MODE:
        {
			UINT8 fftmode = 0;
			nim_cxd2856_get_fftmode(dev,&fftmode);
			ret = fftmode;
			 
	        break;  
	    }
		
	    case ALI_NIM_DRIVER_GET_MODULATION:
		{
			UINT8 modulation = 0;
	    	nim_cxd2856_get_modulation(dev,&modulation);
			ret = modulation;
			break;
	    }
		
		case ALI_NIM_GET_GUARD_INTERVAL:
		{
			UINT8 guard_interval = 0;	
			 
			//nim_cxd2837_get_GI(dev,&guard_interval);
			ret = guard_interval;
			break;
		}
		case ALI_NIM_DRIVER_SET_RESET_CALLBACK:
		{
			user->m_pfn_reset_cxd2856 = (pfn_nim_reset_callback ) parg;
		}
		break;
		
		case ALI_NIM_TURNER_SET_STANDBY:
		{
			pr_debug("tuner power off\n");
			// tuner power off
			/*if (user->tuner_control.nim_tuner_command != NULL)
			{
				user->tuner_control.nim_tuner_command(user->tuner_id, NIM_TUNER_POWER_CONTROL, TRUE);
			}*/
			break;
		}
	    default:
	    {
	        pr_warning("[%s %d]default cmd = %d, ERROR!\n", __FUNCTION__, __LINE__,cmd);
	        ret = -ENOIOCTLCMD;
	        break;
	    }
	 }
    return ret;
}
long ali_cxd2856_nim_ioctl_mutex(struct nim_device *dev,unsigned int cmd, unsigned long parg)
{
	sony_integ_t 		* priv = NULL;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	int ret = SUCCESS;
	if(dev == NULL)
		return -EFAULT;
	priv = dev->priv;
	if(priv == NULL)
		return -EFAULT;
	
	user = priv->user;
	
	mutex_lock(&user->ioctl_mutex);
	ret = ali_cxd2856_nim_ioctl_common(dev,cmd,parg);
	mutex_unlock(&user->ioctl_mutex);
	return ret;
}

static long ali_cxd2856_nim_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
	struct nim_device 	* dev = NULL;
	int ret = SUCCESS;
	if(file == NULL)
		goto ERROR;
	dev = file->private_data;
	
	ret = ali_cxd2856_nim_ioctl_mutex(dev,cmd,parg);
	
	return ret;
ERROR:
	return -EFAULT;
}

static int ali_cxd2856_nim_release(struct inode *inode, struct file *file)
{
	struct nim_device 	* dev = file->private_data;
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	struct nim_debug  	* debug_data = &user->debug_data;
	int result = SUCCESS;
	NIM_TRACE_ENTER();
	
	user->nim_used--;
	if(user->nim_used < 0)//close() nu > open() nu
	{
		NIM_TRACE_RETURN_ERROR("close error",-EFAULT);
	}
	else if(user->nim_used > 0)//close() nu < open() nu
	{
		pr_warning("don't nedd release device\n");
		return SUCCESS;
	}
	//close() nu ==  open() nu
	user->nim_init = FALSE;
	/******exit debug task****/
	if(TASK_RUN == debug_data->monitor_status)//determine whether task has quit)
	{
		debug_data->monitor_status = TASK_DEFAULT;//stop task
		while(TASK_EXIT != debug_data->monitor_status)//wait task exit
		{
			/*This is a good warning. must use msleep() here ,don't use the  msleep_interruptible().
			if you use msleep_interruptible(),when you call this api when the signal appears,here will
			die cycle*/
			msleep(100);
		}
		cancel_work_sync(&dev->debug_work);//remove task from work queue
		debug_data->monitor_status = TASK_DEFAULT;
	}
	//close demod && tuner
	//close tuner
	if (user->tuner_control.nim_tuner_close != NULL)
	{
		result = user->tuner_control.nim_tuner_close();
	}
	
	if(SUCCESS != nim_cxd2856_close(dev))
	{
		NIM_TRACE_RETURN_ERROR("nim_cxd2856_close error",-EFAULT);
	}

	
	NIM_TRACE_RETURN_SUCCESS();
}

static int ali_cxd2856_nim_open(struct inode *inode, struct file *file)
{    
	struct nim_device   * dev = NULL;
	sony_integ_t 		* priv = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;

	NIM_TRACE_ENTER();
	dev = container_of(inode->i_cdev, struct nim_device, cdev);
	if (NULL == dev)
	{
		NIM_TRACE_RETURN_ERROR("get dev error!",-EFAULT);
	}
	
	priv = dev->priv;
	user = priv->user;
	if (NULL == priv)
	{
		NIM_TRACE_RETURN_ERROR("get priv error!",-EFAULT);
	} 
	
	file->private_data = (void *)dev;

	user->nim_used++;
	pr_debug( "%s %d priv->nim_used=%d!\n", __FUNCTION__, __LINE__, user->nim_used);
	NIM_TRACE_RETURN_SUCCESS();
}

static const struct file_operations ali_cxd2856_nim_fops =
{
    .owner		    = THIS_MODULE,
    .unlocked_ioctl	= ali_cxd2856_nim_ioctl,
    .open		    = ali_cxd2856_nim_open,
    .release	    = ali_cxd2856_nim_release,
};

static int ali_cxd2856_init(struct platform_device *pdev)
{
/***************************Data relation***************************
	
nim_device->|
		      |->void * priv = sony_integ_t -->|->void *user = user //ali data:ALI_CXD2856_DATA_T
		      |				                     |->sony_demod_t

******************************data********************************/
    int 				ret = 0;
    dev_t 				devno;
	static int 			dev_num = 0;
	struct nim_device	*dev = NULL;
	sony_integ_t 		*priv = NULL;
	sony_demod_t 		*pdemod = NULL;
	ALI_CXD2856_DATA_T 	*user = NULL;
/******************************************************************/
    NIM_TRACE_ENTER();

	dev = kmalloc(sizeof(struct nim_device), GFP_KERNEL);
    if (!dev)
    {
		NIM_TRACE_RETURN_ERROR("kmalloc dev fail",-ENOMEM);
    }
	memset(dev, 0, sizeof(struct nim_device));
	
    priv = kmalloc(sizeof(struct sony_integ_t), GFP_KERNEL);
    if (!priv)
    {
    	kfree(dev);
		NIM_TRACE_RETURN_ERROR("kmalloc priv fail",-ENOMEM);
    }	
    memset(priv, 0, sizeof(struct sony_integ_t));
	
	pdemod = (sony_demod_t*)kmalloc(sizeof(sony_demod_t),GFP_KERNEL);
	if(NULL == pdemod)
	{
		kfree(priv);
		kfree(dev);
		NIM_TRACE_RETURN_ERROR("kmalloc pdemod fail",-ENOMEM);
	}
	user = (ALI_CXD2856_DATA_T*)kmalloc(sizeof(ALI_CXD2856_DATA_T),GFP_KERNEL);
	if(NULL == user)
	{
		kfree(priv);
		kfree(dev);
		kfree(pdemod);
		NIM_TRACE_RETURN_ERROR("kmalloc user fail",-ENOMEM);
	}
	memset(user, 0, sizeof(ALI_CXD2856_DATA_T));

	/*********************bind private data*******************/
	dev->priv 		= priv;
	priv->user 		= user;
	priv->pDemod 	= pdemod;
	/******************************************************/
    ret = alloc_chrdev_region(&devno, 0, 1, ALI_CXD2856_DEVICE_NAME);
    if (ret < 0)
    {
		//mutex_destroy(&priv->demodMode_mutex_id);
		kfree(priv);
		kfree(dev);
		kfree(pdemod);
		kfree(user);
		NIM_TRACE_RETURN_ERROR("Alloc device region failed",ret);
    }
	if (cxd2856_class_create_flag)
	{
	    ali_cxd2856_nim_class = class_create(THIS_MODULE, "ali_cxd2856_nim_class");
	    if (IS_ERR(ali_cxd2856_nim_class))
	    {
	        ret = PTR_ERR(ali_cxd2856_nim_class);
			pr_err("[err]ali_cxd2856_nim_class create failed, err: %d.\n",(int)ret);
	        goto error1;
	    }
		cxd2856_class_create_flag = false;
	}
	
    cdev_init(&dev->cdev, &ali_cxd2856_nim_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &ali_cxd2856_nim_fops;
	dev->class = ali_cxd2856_nim_class;

    ret = cdev_add(&dev->cdev, devno, 1);
    if (ret)
    {
        pr_err("[err] Alloc NIM device failed, err: %d.\n", (int)ret);
        goto error1;
    }
		
    ali_cxd2856_nim_dev_node = device_create(ali_cxd2856_nim_class, NULL, devno, dev,
                                           "ali_cxd2856_nim%d", dev_num);
    if (IS_ERR(ali_cxd2856_nim_dev_node))
    {
        pr_err( "[err]device_create() failed!\n");

        ret = PTR_ERR(ali_cxd2856_nim_dev_node);

        goto error2;
    }
	user->demod_id = dev_num;
	dev_num++;
	user->nim_init = FALSE;//hw init flag
	user->nim_used = 0;//support multithreading
	
	mutex_init(&user->nim_mutex);
	mutex_init(&user->i2c_mutex);
	mutex_init(&user->ioctl_mutex);
	platform_set_drvdata(pdev, (void *)dev); //add device private data to platform_device data
	NIM_TRACE_RETURN_SUCCESS();
	
error2:
    cdev_del(&dev->cdev);
	goto error1;
error1:
	cdev_del(&dev->cdev);
	kfree(pdemod);
    kfree(priv);
	kfree(dev);
	kfree(user);
	return ret;
}

/* Get parameters from DTS */
static int ali_cxd2856_get_dts(struct platform_device *pdev)
{
	struct nim_device 	* nim_dev = platform_get_drvdata(pdev);
	sony_integ_t 		* priv = (sony_integ_t *)nim_dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	struct device_node 	*dev_node = pdev->dev.of_node;
	struct device_node 	*adapter_np = NULL;
    struct i2c_adapter 	*adapter = NULL;
	struct ali_nim_mn88436_cfg dts_nim_cfg;
	int ret = 0;
	if (NULL == dev_node)
	{
		pr_err("[err]get dev_node fail!\n");
		goto error;
	}
	pr_info("[enter] %s %d\n",__FUNCTION__, __LINE__);
	memset(&dts_nim_cfg, 0, sizeof(dts_nim_cfg));	
	if (of_property_read_u32(dev_node, "nim_type", &(user->nim_type)))
	{
		pr_err("[err] get nim_type error!\n");
		goto error;
	}
	pr_info( "nim_type =%u\n",user->nim_type);
	adapter_np = of_parse_phandle(dev_node, "demod_i2c_bus", 0);
	if (!adapter_np) 
	{
		pr_err("[err]parse demod_i2c_bus error!\n");
		goto error;
	} 
	else
	{
		adapter = of_find_i2c_adapter_by_node(adapter_np);
		of_node_put(adapter_np);
		if (!adapter)
		{
			pr_err( "[err]%s %d get i2c adapter error!\n", __FUNCTION__, __LINE__);
			goto error;
		}
		else
		{
			dts_nim_cfg.ext_dm_config.i2c_type_id = adapter->nr;
		}            
	}
	if (of_property_read_u32(dev_node, "demod_i2c_addr", &dts_nim_cfg.ext_dm_config.i2c_base_addr))
	{
		pr_err("[err]get demod_i2c_addr error!\n");
		goto error;
	}
	pr_info( "[%s %d]dts_nim_cfg.ext_dm_config.i2c_type_id=0x%x dts_nim_cfg.ext_dm_config.i2c_base_addr = 0x%x\n", 
		__FUNCTION__, __LINE__, dts_nim_cfg.ext_dm_config.i2c_type_id, dts_nim_cfg.ext_dm_config.i2c_base_addr);
	if(of_property_read_u32(dev_node,"nim_id",&user->nim_id))
	{
		pr_err("[err] nim_id property missing\n");
		goto error;
	}
	pr_info( "user->nim_id = %d\n", user->nim_id);
	if (of_property_read_u32(dev_node, "tuner_name", &user->tuner_name))
	{
		pr_err("[err]get tuner_name error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.tuner_name = %u\n", user->tuner_name);
	
	if (of_property_read_u32(dev_node, "tuner_id", &user->tuner_id))
	{
		pr_err("[err]get tuner_id error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.tuner_id = 0x%x\n", user->tuner_id);
	
	adapter_np = of_parse_phandle(dev_node, "tuner_i2c_bus", 0);
	if (!adapter_np) 
	{
		pr_err( "[err]parse tuner_i2c_bus error!\n");
		goto error;
	} 
	else
	{
		adapter = of_find_i2c_adapter_by_node(adapter_np);
		of_node_put(adapter_np);
		if (!adapter)
		{
			pr_err( "[err]%s %d get i2c adapter error!\n", __FUNCTION__, __LINE__);
			goto error;
		}
		else
		{
			dts_nim_cfg.tuner_config.i2c_type_id = adapter->nr;
		}            
	}
	if (of_property_read_u8(dev_node, "tuner_i2c_addr", &(dts_nim_cfg.tuner_config.c_tuner_base_addr)))
	{
		pr_err("[err]get tuner_i2c_addr error!\n");
		goto error;
	}
	pr_info( "[%s %d]dts_nim_cfg.tuner_config.i2c_type_id=0x%x, dts_nim_cfg.tuner_config.c_tuner_base_addr = 0x%x\n", 
		__FUNCTION__, __LINE__, dts_nim_cfg.tuner_config.i2c_type_id, dts_nim_cfg.tuner_config.c_tuner_base_addr);
	if (of_property_read_u32(dev_node, "tuner_i2c_communication_mode", &(user->tuner_i2c_communication_mode)))
	{
		pr_err("[err]get tuner_i2c_communication_mode error!\n");
		goto error;
	}
	user->tuner_i2c_communication_mode = 2;
	pr_info( "user->tuner_i2c_communication_mode =%d\n",user->tuner_i2c_communication_mode);

	if (of_property_read_u8(dev_node, "tuner_connection_config", &(dts_nim_cfg.cofdm_data.connection_config)))
	{
		pr_err("[err]get tuner_connection_config error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.cofdm_data.connection_config=0x%x\n", dts_nim_cfg.cofdm_data.connection_config);

	if (of_property_read_u16(dev_node, "tuner_cofdm_config", &(dts_nim_cfg.cofdm_data.cofdm_config)))
	{
		pr_err("[err]get tuner_cofdm_config error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.cofdm_data.cofdm_config=0x%x\n", dts_nim_cfg.cofdm_data.cofdm_config);

	if (of_property_read_u8(dev_node, "tuner_agc_ref", &(dts_nim_cfg.cofdm_data.AGC_REF)))
	{
		pr_err("[err]get tuner_agc_ref error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.cofdm_data.AGC_REF=0x%x\n",dts_nim_cfg.cofdm_data.AGC_REF);

	if (of_property_read_u8(dev_node, "tuner_if_agc_max", &(dts_nim_cfg.cofdm_data.IF_AGC_MAX)))
	{
		pr_err("[err]get tuner_if_agc_max error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.cofdm_data.IF_AGC_MAX=0x%x\n",dts_nim_cfg.cofdm_data.IF_AGC_MAX);

	if (of_property_read_u8(dev_node, "tuner_if_agc_min", &(dts_nim_cfg.cofdm_data.IF_AGC_MIN)))
	{
		pr_err("[err]get tuner_if_agc_min error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.cofdm_data.IF_AGC_MIN=0x%x\n",dts_nim_cfg.cofdm_data.IF_AGC_MIN);

	if (of_property_read_u16(dev_node, "tuner_c_tuner_crystal", &(dts_nim_cfg.tuner_config.c_tuner_crystal)))
	{
		pr_err("[err]get tuner_c_tuner_crystal error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.tuner_config.c_tuner_crystal=%d\n",dts_nim_cfg.tuner_config.c_tuner_crystal);
	
	if (of_property_read_u16(dev_node, "tuner_w_tuner_if_freq", &(dts_nim_cfg.tuner_config.w_tuner_if_freq)))
	{
		pr_err("[err]get tuner_w_tuner_if_freq error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.tuner_config.w_tuner_if_freq=%d\n",dts_nim_cfg.tuner_config.w_tuner_if_freq);
	if (of_property_read_u8(dev_node, "tuner_c_tuner_agc_top", &(dts_nim_cfg.tuner_config.c_tuner_agc_top)))
	{
		pr_err("[err]get tuner_c_tuner_agc_top error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.tuner_config.c_tuner_agc_top=%d\n", dts_nim_cfg.tuner_config.c_tuner_agc_top);

	if (of_property_read_u8(dev_node, "tuner_c_chip", &(dts_nim_cfg.tuner_config.c_chip)))
	{
		pr_err("[err]get tuner_c_chip error!\n");
		goto error;
	}
	pr_info( "dts_nim_cfg.tuner_config.c_chip=%d\n", dts_nim_cfg.tuner_config.c_chip);

	user->reset_pin = of_get_gpio(dev_node, 0);//get the reset pin from dts
	pr_info( "reset_pin=%d!\n",user->reset_pin);
	if(gpio_is_valid(user->reset_pin))
	{
		ret = devm_gpio_request(&pdev->dev, user->reset_pin, "reset_pin");	
		if (ret) 
		{		
			NIM_TRACE_RETURN_ERROR("request reset_pin fail!",-EINVAL);
		}
	}
	
	
	/*************************set dts para to dev private data******************/
	nim_set_dev_config(nim_dev, &dts_nim_cfg);
	NIM_TRACE_RETURN_SUCCESS();
   
error:
	mutex_destroy(&user->nim_mutex);
	mutex_destroy(&user->i2c_mutex);
	mutex_destroy(&user->ioctl_mutex);
	cdev_del(&nim_dev->cdev);
	kfree(user);
	kfree(priv);
	kfree(nim_dev);
	return -EINVAL;
}

static int ali_cxd2856_probe(struct platform_device * pdev)
{
	
	pr_info("[enter] %s %d\n",__FUNCTION__, __LINE__);
	if (ali_cxd2856_init(pdev) != 0)
	{
		pr_err("device_create() failed!\n");
		return -EFAULT;
	}
	if (of_have_populated_dt()) 
	{
		if (ali_cxd2856_get_dts(pdev) != SUCCESS) 
		{
			pr_err( "Failed to parse DT\n");
			return -EFAULT;
		}
	}
	
	nim_cxd2856_proc_init(pdev);
	pr_info("[leave]%s %d pdev->id = 0x%x\n", __FUNCTION__, __LINE__, pdev->id);
	return SUCCESS;
}

static int ali_cxd2856_remove(struct platform_device * pdev)
{	
	struct nim_device 		*dev = NULL;
	struct sony_integ_t 	*priv = NULL;
	ALI_CXD2856_DATA_T 		*user = NULL;
	struct sony_demod_t 	*pdemod = NULL;
	NIM_TRACE_ENTER();
	if (NULL == pdev)
	{
		NIM_TRACE_RETURN_ERROR("pdev is NULL",-EFAULT);
	}
	
	dev = platform_get_drvdata(pdev);
	if (NULL == dev)
	{
		NIM_TRACE_RETURN_ERROR("get dev fail",-EFAULT);
	}
	
	priv 	= dev->priv;
	user 	= priv->user;
	pdemod 	= priv->pDemod;
	if (dev->ali_nim_dev_node != NULL)
	{
		device_del(dev->ali_nim_dev_node);
	}   
	
	if (ali_cxd2856_nim_class != NULL)
	{
		class_destroy(ali_cxd2856_nim_class);
	}   
	cdev_del(&dev->cdev);
	mutex_destroy(&user->nim_mutex);
	mutex_destroy(&user->i2c_mutex);
	mutex_destroy(&user->ioctl_mutex);
	nim_cxd2856_proc_exit(pdev);
	kfree(dev);
	kfree(priv);
	kfree(pdemod);
	kfree(user);
	
	cxd2856_class_create_flag = true;
	NIM_TRACE_RETURN_SUCCESS();
	return SUCCESS;
}

static int ali_cxd2856_suspend(struct platform_device * pdev, pm_message_t state)
{	
	INT32 tuner_standby = 1;
	struct nim_device 	* dev = NULL;
	sony_integ_t 		* priv = NULL;
	sony_demod_t 		* pdemod = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;
	
	NIM_TRACE_ENTER();
	if (NULL == pdev)
	{
		NIM_TRACE_RETURN_ERROR("pdev is NULL",-EFAULT);
	}
	
	dev = platform_get_drvdata(pdev);
	if (NULL == dev)
	{
		NIM_TRACE_RETURN_ERROR("get dev fail",-EFAULT);
	}

	priv 	= dev->priv;
	pdemod	= priv->pDemod;
	user 	= priv->user;
	
	if(!user->nim_init)
	{	
		pr_warning( "[err] %s %d cxd2856 have not init\n", __FUNCTION__, __LINE__);
		return SUCCESS;
	}

	if (user->tuner_control.nim_tuner_command != NULL)
	{
		//sleep tuner
		if(user->tuner_control.nim_tuner_command(user->tuner_id,NIM_TUNER_POWER_CONTROL,tuner_standby)<0)
		{
			NIM_TRACE_RETURN_ERROR("cxd2856 nim_suspned tuner sleep fail",-EFAULT);
		}
	}
	//sleep demod
    if (sony_demod_Shutdown(pdemod) != SONY_RESULT_OK)//sony_dvb_demod_Finalize
    {
		NIM_TRACE_RETURN_ERROR("sony_demod_Shutdown fail",-EFAULT);
    }
	NIM_TRACE_RETURN_SUCCESS();
}

static int ali_cxd2856_resume(struct platform_device * pdev)
{
	INT32 tuner_standby = 0;
	struct nim_device 	* dev = NULL;
	sony_integ_t 		* priv = NULL;
	ALI_CXD2856_DATA_T 	* user = NULL;

	NIM_TRACE_ENTER();
	if (NULL == pdev)
	{
		NIM_TRACE_RETURN_ERROR("pdev is NULL",-EFAULT);
	}
	
	dev = platform_get_drvdata(pdev);
	if (NULL == dev)
	{
		NIM_TRACE_RETURN_ERROR("get dev fail",-EFAULT);
	}

	priv 	= dev->priv;
	user 	= priv->user;
	if(!user->nim_init)
	{
		pr_warning( "[%s %d]priv->nim_init=%d\n", __FUNCTION__, __LINE__, user->nim_init);
		return SUCCESS;
	}
	
	//resume tuner
	if (user->tuner_control.nim_tuner_command != NULL)
	{
		if(user->tuner_control.nim_tuner_command(user->tuner_id, NIM_TUNER_POWER_CONTROL, tuner_standby)<0)
		{
			NIM_TRACE_RETURN_ERROR("cxd2856 nim_suspned tuner sleep fail",-EFAULT);
		}
	}
	NIM_TRACE_RETURN_SUCCESS();
}

static const struct of_device_id ali_cxd2856_of_match[] = {
	{ .compatible = ALI_CXD2856_DEV_COMPATIBLE_NAME, },
};
MODULE_DEVICE_TABLE(of, ali_cxd2856_of_match);

static struct platform_driver ali_cxd2856_driver = {
	.driver 	= {
		.name = ALI_CXD2856_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ali_cxd2856_of_match,
	},
	.probe   	= ali_cxd2856_probe, 
	.remove   	= ali_cxd2856_remove,
	.suspend  	= ali_cxd2856_suspend,
	.resume   	= ali_cxd2856_resume,
};

static int __init ali_cxd2856_driver_init(void)
{
	return platform_driver_register(&ali_cxd2856_driver);
	
}

static void __exit ali_cxd2856_driver_exit(void)
{
	platform_driver_unregister(&ali_cxd2856_driver);
}

module_init(ali_cxd2856_driver_init);
module_exit(ali_cxd2856_driver_exit);

MODULE_DESCRIPTION("Alitech CXD2837 NIM Driver");
MODULE_AUTHOR("Robin.gan");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

