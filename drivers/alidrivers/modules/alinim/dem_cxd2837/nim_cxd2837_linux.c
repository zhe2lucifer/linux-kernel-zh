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

/*
*    File: nim_cxd2837_linux.c
*    Description: cxd2837 nim driver for linux api
*/
#include <linux/platform_device.h>
#include <ali_soc.h>
#include "porting_cxd2837_linux.h"
#include "nim_cxd2837.h"
#include "sony_demod.h"
#include "sony_demod_integ.h"


#define SONY_NIM_DEVICE_NAME         "ali_nim_cxd2837"

#define MAX_TUNER_SUPPORT_NUM        1
//#define NIM_TUNER_SET_STANDBY_CMD    0xffffffff
#define sony_demod_Create            cxd2837_demod_Create

static struct nim_device             ali_cxd2837_nim_dev;
struct sony_demod_t                  *ali_cxd2837_nim_priv = NULL;
static struct class                  *ali_cxd2837_nim_class;
static struct device                 *ali_cxd2837_nim_dev_node;
static sony_i2c_t                    demodI2c;
static INT32                         g_is_rest =1;

static void nim_cxd2837_set_config(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
    struct sony_demod_t 	    *priv = dev->priv;
    TUNER_IO_FUNC               *p_io_func=NULL;

	
    memcpy(&priv->tuner_control.config_data, &nim_cfg->cofdm_data, sizeof(struct COFDM_TUNER_CONFIG_DATA));
    memcpy(&priv->tuner_control.tuner_config, &nim_cfg->tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
    memcpy(&priv->tuner_control.ext_dm_config, &nim_cfg->ext_dm_config, sizeof(struct EXT_DM_CONFIG));
    priv->tuner_control.tuner_id = nim_cfg->tuner_id;

    CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,flag=%d,i2c_type=%d,i2c_addr=0x%x\n", __FUNCTION__, __LINE__,
		                              priv->tuner_control.config_data.flag,
		                              priv->tuner_control.tuner_config.i2c_type_id,
		                              priv->tuner_control.tuner_config.c_tuner_base_addr);

	p_io_func = tuner_setup(NIM_DVBT,nim_cfg->tuner_id);
	if(NULL != p_io_func)
	{
		priv->tuner_control.nim_tuner_init    = (dvbt_tuner_init)p_io_func->pf_init;
		priv->tuner_control.nim_tuner_control = (dvbt_tuner_control)p_io_func->pf_control;
		priv->tuner_control.nim_tuner_status  = (dvbt_tuner_status)p_io_func->pf_status;
		priv->tuner_control.nim_tuner_close   = (dvbt_tuner_close)p_io_func->pf_close;
		priv->tuner_control.nim_tuner_command = (dvbt_tuner_command)(p_io_func->pf_command);
	}
	else
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"[%s]line=%d,Tuner API is NULL!\n",__FUNCTION__,__LINE__);
	}
}


/*****************************************************************************
* INT32 ali_m3281_nim_hw_initialize(struct nim_device *dev)
* Description: S3202 open
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/

static INT32 ali_cxd2837_nim_hw_initialize(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{

    sony_demod_t *priv = dev->priv;
    DEM_WRITE_READ_TUNER ThroughMode;
	if(priv->nim_init)//state of initation sucessfully,hw can only be initialized  once;
	{
		return SUCCESS;
	}
	if((NULL == nim_cfg) || (NULL == dev))
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Tuner Configuration API structure is NULL!/n");
		return ERR_NO_DEV;
	}
	CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,start:\n", __FUNCTION__, __LINE__);

  if(priv->m_pfn_reset_cxd2837)
		priv->m_pfn_reset_cxd2837(priv->tuner_id);
	
	//Setup demod I2C interfaces.
	demodI2c.i2c_type_id      = nim_cfg->ext_dm_config.i2c_type_id;
	demodI2c.ReadRegister     = cxd2837_i2c_CommonReadRegister;
	demodI2c.WriteRegister    = cxd2837_i2c_CommonWriteRegister;
	demodI2c.WriteOneRegister = cxd2837_i2c_CommonWriteOneRegister;

    CXD2837_PRINTF(NIM_LOG_DBG,"ext_dm_config.i2c_base_addr = 0x%x\n",nim_cfg->ext_dm_config.i2c_base_addr);
    /* Create demodulator instance */
	if (sony_demod_Create (priv, SONY_DEMOD_XTAL_20500KHz, nim_cfg->ext_dm_config.i2c_base_addr, &demodI2c) != SONY_RESULT_OK)
	{
	    CXD2837_PRINTF(NIM_LOG_DBG,"sony_demod_Create error!\n");
		return ERR_NO_DEV;
	}
	nim_cxd2837_set_config(dev,nim_cfg);
	CXD2837_PRINTF(NIM_LOG_DBG,"i2cAddressSLVX = 0x%x\n",priv->i2cAddressSLVX);
	/* ---------------------------------------------------------------------------------
	* Configure the Demodulator
	* ------------------------------------------------------------------------------ */
	/* DVB-T demodulator IF configuration for terrestrial / cable tuner */
	priv->iffreqConfig.configDVBT_5 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT_5MHz_IF);
	priv->iffreqConfig.configDVBT_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT_6MHz_IF);
	priv->iffreqConfig.configDVBT_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT_7MHz_IF);
	priv->iffreqConfig.configDVBT_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT_8MHz_IF);
	
	/* DVB-T2 demodulator IF configuration for terrestrial / cable tuner */
	priv->iffreqConfig.configDVBT2_1_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT2_1_7MHz_IF);
	priv->iffreqConfig.configDVBT2_5 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT2_5MHz_IF);
	priv->iffreqConfig.configDVBT2_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT2_6MHz_IF);
	priv->iffreqConfig.configDVBT2_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT2_7MHz_IF);
	priv->iffreqConfig.configDVBT2_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBT2_8MHz_IF);

	/* DVB-C demodulator IF configuration for terrestrial / cable tuner */
	priv->iffreqConfig.configDVBC_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBC_6MHz_IF);
	priv->iffreqConfig.configDVBC_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBC_7MHz_IF);
	priv->iffreqConfig.configDVBC_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_DVBC_8MHz_IF);
	if(CXD2837 == nim_cfg->tuner_id)
	{
		priv->tunerOptimize = SONY_DEMOD_TUNER_OPTIMIZE_ASCOT3;
	}
	else
	{
		priv->tunerOptimize = SONY_DEMOD_TUNER_OPTIMIZE_UNKNOWN;
	}
	/*   transfer the system signal type to private data    */
    CXD2837_PRINTF(NIM_LOG_DBG,"[%s]:line=%d,system:%d\r\n",__FUNCTION__,__LINE__,priv->system);
	
    if(priv->tuner_control.nim_tuner_init != NULL)
    {
    	if(priv->tuner_control.nim_tuner_init(&(priv->tuner_id), &(priv->tuner_control.tuner_config)) != SUCCESS)
        {
            CXD2837_PRINTF(NIM_LOG_DBG,"Error: Init Tuner Failure!\r\n");
            return ERR_NO_DEV;
        }
    	ThroughMode.nim_dev_priv = dev->priv;
    	ThroughMode.Dem_Write_Read_Tuner = (INTERFACE_DEM_WRITE_READ_TUNER)cxd2837_i2c_TunerGateway;
		if (priv->tuner_control.nim_tuner_command != NULL)
		{
			priv->tuner_control.nim_tuner_command(priv->tuner_id, NIM_TUNER_SET_THROUGH_MODE, (UINT32)&ThroughMode);
		}
    	//tun_cxd_ascot3_command(priv->tuner_id, NIM_TUNER_SET_THROUGH_MODE, (UINT32)&ThroughMode);
    }
	else
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"Error: Init Tuner is NULL!\r\n");
	}
    if (SONY_RESULT_OK != nim_cxd2837_open(&ali_cxd2837_nim_dev))
    {
        CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,nim_cxd2837_open failed!\n", __FUNCTION__, __LINE__);
		return ERR_FAILURE;
	}
    CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,end!\n", __FUNCTION__, __LINE__);
	priv->nim_init = TRUE;
    return SUCCESS;
}

#if 0
static INT32 ali_cxd2837_tuner_adaption(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
	struct nim_cxd2837_private *priv = dev->priv;
	UINT8 data = 0;
	
	nim_cxd2837_set_config(dev,nim_cfg);

    if (SUCCESS == nim_cxd2837_read(dev, 
		                        priv->tuner_control.tuner_config.c_tuner_base_addr, 
		                        &data, 
		                        1))
    {	
		CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,adaption success!,i2c_type=%d,i2c_addr=0x%x\n", 
			      __FUNCTION__, __LINE__,
			      priv->tuner_control.tuner_config.i2c_type_id,
			      priv->tuner_control.tuner_config.c_tuner_base_addr);

		return SUCCESS;
    }

   return -1;	
}
#endif
static long ali_cxd2837_nim_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
    struct nim_device *dev = file->private_data;
	sony_demod_t *priv = dev->priv;
    int ret = 0;
	mutex_lock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
    switch(cmd)
	{
		case ALI_NIM_TUNER_SELT_ADAPTION_C:
	    {
	        struct ali_nim_mn88436_cfg nim_param;

	        if(copy_from_user(&nim_param, (struct ali_nim_mn88436_cfg *)parg, sizeof(struct ali_nim_mn88436_cfg))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				mutex_unlock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
				return -EFAULT;
			}
	        //ret = ali_cxd2837_tuner_adaption(dev, &nim_param);

			break;
		}
	    case ALI_NIM_HARDWARE_INIT_T:
	    {
	        struct ali_nim_mn88436_cfg nim_param;
	        memset((void*)&nim_param,0,sizeof(struct ali_nim_mn88436_cfg));
	        if(copy_from_user(&nim_param, (struct ali_nim_mn88436_cfg *)parg, sizeof(struct ali_nim_mn88436_cfg))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				mutex_unlock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
				// Invalid user space address
				return -EFAULT;
			}
				
	        ret = ali_cxd2837_nim_hw_initialize(dev, &nim_param);

	        break;
	    }
		case ALI_NIM_DRIVER_STOP_ATUOSCAN:
	        priv->autoscan_stop_flag = parg;
	        break;
	    case ALI_NIM_CHANNEL_CHANGE:
	    {
	        NIM_CHANNEL_CHANGE_T nim_param;

	        CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,nim_param.fec=%d\n", __FUNCTION__, __LINE__, nim_param.fec);

			memset((void*)&nim_param,0,sizeof(NIM_CHANNEL_CHANGE_T));
			
	        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				mutex_unlock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
				return -EFAULT;
			}
			if(SUCCESS != nim_cxd2837_channel_change_smart(dev, &nim_param))
			{
				CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,nim_cxd2837_channel_change_smart failed!\n", __FUNCTION__, __LINE__);
			}

	        if(copy_to_user((NIM_CHANNEL_CHANGE_T *)parg, &nim_param, sizeof(NIM_CHANNEL_CHANGE_T))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				mutex_unlock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
				return -EFAULT;
			}
			
	        break;
	    }

		case ALI_NIM_DRIVER_T2_SIGNAL_ONLY:
	    {	
			priv->search_t2_only = parg;
			
			break;
		}
	    case ALI_NIM_GET_LOCK_STATUS:
	    {
	        UINT8 lock = 0;
			nim_cxd2837_get_lock(dev, &lock);
	        ret = lock;
	        break;
	    }

	    case ALI_NIM_READ_AGC:
	    {
	        UINT8 agc = 0;
	        nim_cxd2837_get_SSI(dev, &agc);
	        ret = agc;
	        break;
	    }
		case ALI_NIM_LOG_LEVEL:
		{
	        int log_level= 0;

			if(copy_from_user(&log_level, (int *)parg, sizeof(int))>0)
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				mutex_unlock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
				return -EFAULT;
			}
	        set_log_level(log_level);

			ret = SUCCESS;
			break;
		}		
		case ALI_NIM_READ_SNR:
		{
			UINT8 snr = 0;
			nim_cxd2837_get_SQI(dev, &snr);
			ret=snr;
			
			break;
		}
		case ALI_NIM_READ_CODE_RATE:
		{
	        UINT8 code_rate = 0;
			
			nim_cxd2837_get_FEC(dev, &code_rate);

			ret=code_rate;
			break;
		
		}
		case ALI_NIM_READ_QPSK_BER:
		{
	        UINT32 ber = 0;
	        
	        nim_cxd2837_get_BER(dev, &ber);
			
	        ret = ber;

			break;
		}
		case ALI_NIM_READ_FREQ:
	    {
	        UINT32 freq = 0;
	     
	        nim_cxd2837_get_freq(dev, &freq);
	        ret = freq;
	        break;
	    }	
	    case ALI_NIM_DRIVER_GET_FFT_MODE:
        {
			UINT8 fftmode = 0;
			 
			nim_cxd2837_get_fftmode(dev,&fftmode);
			ret = fftmode;
			 
	        break;  
	    }
	    case ALI_NIM_DRIVER_GET_MODULATION:
		{
			UINT8 modulation = 0;
			 
	    	nim_cxd2837_get_modulation(dev,&modulation);
			ret = modulation;
			break;
	    }
		case ALI_NIM_GET_GUARD_INTERVAL:
		{
			UINT8 guard_interval = 0;	
			 
			nim_cxd2837_get_GI(dev,&guard_interval);
			ret = guard_interval;
			break;
		}
		case ALI_NIM_GET_SPECTRUM_INV:

			 break;
		case ALI_NIM_DRIVER_SET_RESET_CALLBACK:
		{
			sony_demod_t * pDemod = (sony_demod_t *)dev->priv;
			pDemod->m_pfn_reset_cxd2837 = (pfn_nim_reset_callback ) parg;
		}
		break;
		case ALI_NIM_TURNER_SET_STANDBY:

			CXD2837_PRINTF(NIM_LOG_DBG,"tuner power off\n");
			// tuner power off
			if (priv->tuner_control.nim_tuner_command != NULL)
			{
				priv->tuner_control.nim_tuner_command(priv->tuner_id, NIM_TUNER_POWER_CONTROL, TRUE);
			}
			//tun_cxd_ascot3_command(priv->tuner_id, NIM_TUNER_POWER_CONTROL, TRUE);
			break;
		case ALI_NIM_GET_CN_VALUE:
		{
				
			UINT32 cn_value = 0;
			nim_cxd2837_get_SNR(dev,&cn_value);
			ret = cn_value;
			break;
		}
		case ALI_NIM_GET_RF_LEVEL:
		{
			INT32 rf_level = 0;
			nim_cxd2837_get_RF_LEVEL(dev,&rf_level);
			ret = rf_level;
			break;
		}
	    default:
	    {
	        CXD2837_PRINTF(NIM_LOG_DBG,"ERROR-->[%s]line=%d,23\n", __FUNCTION__, __LINE__);
	        ret = -ENOIOCTLCMD;
	        break;
	    }
	 }
	mutex_unlock(&ali_cxd2837_nim_priv->demodMode_mutex_id);
    return ret;
}

static int ali_cxd2837_nim_release(struct inode *inode, struct file *file)
{
	INT32 result = SUCCESS;
	struct nim_device *dev = file->private_data;
	 struct sony_demod_t *priv = dev->priv;
	 priv->nim_used --; //use the counter record the num of user
	 if(priv->nim_used)
	 {
	 	return RET_SUCCESS;
	 }
	 priv->nim_init = FALSE;
	 priv->search_t2_only = 0;
    //close demod && tuner
	result = nim_cxd2837_close(dev);
	if(SUCCESS != result)
	   return result;
	
	//close tuner
	result = tun_cxd_ascot3_release();
	return result;
 
}

static int match(struct gpio_chip *chip,void *data)
{
    if (0 == strcmp(chip->label, data))
    {
        return 1;
    }
	return 0;
}
/*****************************************************************************
* static void nim_cxd2838_hwreset(void)
* Description: cxd2838 device reset
*
* Arguments:
*  
*
* Return Value: success
*****************************************************************************/

static void nim_cxd2837_hwreset(void)
{

	 struct gpio_chip *gpio_chip;
	 int gpio_index  = 84;	//gpio[84]
 
	 gpio_chip = gpiochip_find("m36", match);

	 //set gpio to high
     gpio_chip->direction_output(gpio_chip,gpio_index,1);
	 msleep(10);
     //set gpio to low
	 gpio_chip->direction_output(gpio_chip,gpio_index,0);
	 msleep(10);	 
	 //set gpio to high
	 gpio_chip->direction_output(gpio_chip,gpio_index,1);
}



static int ali_cxd2837_nim_open(struct inode *inode, struct file *file)
{
   /* struct sony_demod_t *priv = ali_cxd2837_nim_priv;
    CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);
	
    ali_cxd2837_nim_dev.priv = (void *)priv;*/
    file->private_data = (void *) &ali_cxd2837_nim_dev;
	if(!g_is_rest)
    {
		nim_cxd2837_hwreset();
		g_is_rest = 1;
    }
	ali_cxd2837_nim_priv->nim_used ++;
	return SUCCESS;

}

static const struct file_operations ali_cxd2837_nim_fops =
{
    .owner		    = THIS_MODULE,
    .unlocked_ioctl	= ali_cxd2837_nim_ioctl,
    .open		    = ali_cxd2837_nim_open,
    .release	    = ali_cxd2837_nim_release,
};

static int ali_nim_suspend(struct platform_device * pdev,pm_message_t state)
{	
	INT32 tuner_standby = 1;
	INT32 result = SUCCESS;
	struct nim_device *dev = pdev->dev.platform_data;
	sony_demod_t *priv = dev->priv;
	if(!priv->nim_init)
	{
	#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		printk(KERN_EMERG "cxd3827 nim_suspned have not init\n");
	#endif
		return SUCCESS;
	}

	//sleep tuner
	if (priv->tuner_control.nim_tuner_command != NULL)
	{
		//if(tun_cxd_ascot3_command(priv->tuner_id,NIM_TUNER_POWER_CONTROL,tuner_standby)<0)
		if(priv->tuner_control.nim_tuner_command(priv->tuner_id,NIM_TUNER_POWER_CONTROL,tuner_standby)<0)
		{
		#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
			printk(KERN_EMERG "cxd3827 nim_suspned tuner sleep fail");
		#endif
			CXD2837_PRINTF("[%s] cxd3827 nim_suspned tuner sleep fail\n",__FUNCTION__);
			return -1;
		}
	}

	//sleep demod
	if (cxd2837_demod_Shutdown(priv) != SONY_RESULT_OK)//sony_dvb_demod_Finalize
	{
		result = ERR_FAILUE;
	}

	/*result = cxd2837_demod_Shutdown(priv);//sony_dvb_demod_Finalize
	if (result != SONY_RESULT_OK)
	{
	#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		printk(KERN_EMERG "cxd3827 nim_suspned shutdown fail");
	#endif
		result = ERR_FAILUE;
	}*/

	/*if(sony_demod_SleepT_C (priv)<0) //just sleep;not shoutdown
	{
		CXD2837_PRINTF("[%s] demod cxd2837 fail\n",__FUNCTION__);
		return (-1);
	}*/

#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "cxd3827 nim tuner sleep..\n");
#endif

	return SUCCESS;
}
static int ali_nim_resume(struct platform_device * pdev)
{
	INT32 tuner_standby = 0;
	struct nim_device *dev = pdev->dev.platform_data;
	sony_demod_t *priv = dev->priv;
	if(!priv->nim_init)
	{
	#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		printk(KERN_EMERG "Function nim_resume have not init");
	#endif
		return SUCCESS;
	}

	//resume demod
	if(sony_demod_InitializeT_C(priv)<0)
	{
		CXD2837_PRINTF("[%s] demod cxd2837 fail\n",__FUNCTION__);
		return -1;
	}
	msleep(10);//wait 10ms
	//resume tuner
	if (priv->tuner_control.nim_tuner_command != NULL)
	{
		//if(tun_cxd_ascot3_command(priv->tuner_id,NIM_TUNER_POWER_CONTROL,tuner_standby)<0)
		if(priv->tuner_control.nim_tuner_command(priv->tuner_id,NIM_TUNER_POWER_CONTROL,tuner_standby)<0)
		{
			CXD2837_PRINTF("[%s] tuner cxd2872 fail\n",__FUNCTION__);
			return -1;
		}
	}
	CXD2837_PRINTF("[%s] cxd3827 nim tuner wakeup..\n",__FUNCTION__);
	return SUCCESS;
}

static int ali_nim_probe(struct platform_device * pdev)
{
	INT32 ret = 0;
	dev_t devno;
	struct sony_demod_t *priv = NULL;

	CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);
	ali_cxd2837_nim_priv = kmalloc(sizeof(struct sony_demod_t), GFP_KERNEL);
	if (!ali_cxd2837_nim_priv)
	{
		return (-ENOMEM);
	}
	memset(ali_cxd2837_nim_priv, 0, sizeof(struct sony_demod_t));
	ali_cxd2837_nim_priv->t2_signal = 0xff;
	//mutex_init(&ali_cxd2837_nim_priv->i2c_mutex_id);
	mutex_init(&ali_cxd2837_nim_priv->demodMode_mutex_id);
	//mutex_init(&ali_cxd2837_nim_priv->flag_id);

	//ali_cxd2837_nim_priv->flag_lock.flagid_rwlk = __RW_LOCK_UNLOCKED(ali_cxd2837_nim_priv->flagid_rwlk);
	//mutex_init(&ali_cxd2837_nim_priv->i2c_mutex_id);

	ret = of_get_major_minor(pdev->dev.of_node,&devno, 
			0, 1, SONY_NIM_DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		mutex_destroy(&ali_cxd2837_nim_priv->demodMode_mutex_id);
		kfree(ali_cxd2837_nim_priv);
		return ret;
	}
	ali_cxd2837_nim_priv->nim_init = FALSE;
	ali_cxd2837_nim_priv->nim_used = 0;

	cdev_init(&ali_cxd2837_nim_dev.cdev, &ali_cxd2837_nim_fops);
	ali_cxd2837_nim_dev.cdev.owner = THIS_MODULE;
	ali_cxd2837_nim_dev.cdev.ops = &ali_cxd2837_nim_fops;
	ret = cdev_add(&ali_cxd2837_nim_dev.cdev, devno, 1);
	if (ret)
	{
	   CXD2837_PRINTF(NIM_LOG_DBG,"[NIMTRACE] Alloc NIM device failed, err: %d.\n", (int)ret);
		goto error1;
	}
	priv = ali_cxd2837_nim_priv;
	ali_cxd2837_nim_dev.priv = (void *)priv;
	CXD2837_PRINTF(NIM_LOG_DBG,"register NIM device end.\n");

	ali_cxd2837_nim_class = class_create(THIS_MODULE, "ali_cxd2837_nim_class");

	if (IS_ERR(ali_cxd2837_nim_class))
	{
		ret = PTR_ERR(ali_cxd2837_nim_class);
		goto error2;
	}

	ali_cxd2837_nim_dev_node = device_create(ali_cxd2837_nim_class, NULL, devno, &ali_cxd2837_nim_dev,
										   "ali_cxd2837_nim0");
	if (IS_ERR(ali_cxd2837_nim_dev_node))
	{
		CXD2837_PRINTF(NIM_LOG_DBG,"device_create() failed!\n");

		ret = PTR_ERR(ali_cxd2837_nim_dev_node);

		goto error3;
	}

	pdev->dev.platform_data = (void *) &ali_cxd2837_nim_dev;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "[%s]line=%d,enter! id = %d\n", __FUNCTION__, __LINE__, pdev->id);
#endif

	mutex_destroy(&ali_cxd2837_nim_priv->demodMode_mutex_id);
	return SUCCESS;
error3:
	class_destroy(ali_cxd2837_nim_class);
error2:
	cdev_del(&ali_cxd2837_nim_dev.cdev);
error1:
	//mutex_destroy(&ali_cxd2837_nim_priv->i2c_mutex_id);
	mutex_destroy(&ali_cxd2837_nim_priv->demodMode_mutex_id);
	//mutex_destroy(&ali_cxd2837_nim_priv->flag_id);
	kfree(ali_cxd2837_nim_priv);

	return ret;
}
static int ali_nim_remove(struct platform_device * pdev)
{

	CXD2837_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);
    if (ali_cxd2837_nim_dev_node != NULL)
    {
		device_del(ali_cxd2837_nim_dev_node);
    }

    if (ali_cxd2837_nim_class != NULL)
    {
		class_destroy(ali_cxd2837_nim_class);
    }	
    cdev_del(&ali_cxd2837_nim_dev.cdev);
	//mutex_destroy(&ali_cxd2837_nim_priv->i2c_mutex_id);
	mutex_destroy(&ali_cxd2837_nim_priv->demodMode_mutex_id);
	//mutex_destroy(&ali_cxd2837_nim_priv->flag_id);
    kfree(ali_cxd2837_nim_priv);

	return SUCCESS;
}


static const struct of_device_id sony_cx2837_nim_of_match[] = {
       { .compatible = "alitech, sony_cx2837_nim", },
       {},
};

MODULE_DEVICE_TABLE(of, sony_cx2837_nim_of_match);

static struct platform_driver sony_cx2837_nim_platform_driver = {
	.probe   = ali_nim_probe, 
	.remove   = ali_nim_remove,
	.suspend  = ali_nim_suspend,
	.resume   = ali_nim_resume,
	.driver  = {
			.owner  = THIS_MODULE,
			.name   = "sony_cx2837_nim",
			.of_match_table = sony_cx2837_nim_of_match,
			},
};

module_platform_driver(sony_cx2837_nim_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dennis");
MODULE_DESCRIPTION("SONY CXD2837 NIM driver");
