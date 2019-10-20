/*----------------------------------------------------------------------------
   Solution:  demodulater CXD2856. + tuner MXL603
------------------------------------------------------------------------------
    History:       
	Date                  athor           version                reason 
------------------------------------------------------------------------------
    2017/04/06          leo.liu           v0.1                  create hardware INIT file       
----------------------------------------------------------------------------*/
#include "nim_cxd2856_common.h"
#include <linux/of_gpio.h>
/*****************************************************************

ali_cxd2856_nim_hw_initialize()--->   |
                                                      |-----> nim_cxd2856_hwreset()
                                                      |-----> nim_set_tuner_config()
	           				              |-----> nim_cxd2856_open()
	           				              |
*****************************************************************/

int nim_cxd2856_open(struct nim_device *dev)
{
	sony_integ_t 		* priv 	= (sony_integ_t *)dev->priv;
	sony_demod_t 		* pdemod= priv->pDemod;
	ALI_CXD2856_DATA_T  * user 	= priv->user;
	sony_demod_iffreq_config_t 	iffreqConfig;
	sony_result_t result 		= SUCCESS;

	NIM_TRACE_ENTER();
	// Initialize the demod.
	result = sony_integ_Initialize (priv);
	if(result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("sony_integ_Initialize failed",ERR_DEV_ERROR);
	}
	/********************* config demod *********************/
	
	//step 1:====== Set intermediate frequency of demod
	if(MXL603 == user->tuner_name)
	{
		NIM_TRACE_ENTER();	
		iffreqConfig.configDVBT_5 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);

		iffreqConfig.configDVBT2_1_7= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT2_5 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT2_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT2_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configDVBT2_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);

		iffreqConfig.configDVBC_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configDVBC_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.9);
		iffreqConfig.configDVBC_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.9);
		iffreqConfig.configDVBC2_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configDVBC2_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.9);

		iffreqConfig.configISDBT_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configISDBT_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);
		iffreqConfig.configISDBT_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(5);

		iffreqConfig.configISDBC_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configJ83B_5_06_5_36 = SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configJ83B_5_60= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.75);
	}
	else
	{
		NIM_TRACE_ENTER();		
		iffreqConfig.configDVBT_5 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.6);
		iffreqConfig.configDVBT_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.6);
		iffreqConfig.configDVBT_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.2);
		iffreqConfig.configDVBT_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.8);

		iffreqConfig.configDVBT2_1_7= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.5);
		iffreqConfig.configDVBT2_5 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.6);
		iffreqConfig.configDVBT2_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.6);
		iffreqConfig.configDVBT2_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.2);
		iffreqConfig.configDVBT2_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.8);

		iffreqConfig.configDVBC_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configDVBC_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.9);
		iffreqConfig.configDVBC_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.9);
		iffreqConfig.configDVBC2_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configDVBC2_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.9);

		iffreqConfig.configISDBT_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.55);
		iffreqConfig.configISDBT_7 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.15);
		iffreqConfig.configISDBT_8 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(4.75);

		iffreqConfig.configISDBC_6 	= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configJ83B_5_06_5_36 = SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.7);
		iffreqConfig.configJ83B_5_60= SONY_DEMOD_MAKE_IFFREQ_CONFIG(3.75);
	}
				
	if (sony_demod_SetIFFreqConfig (pdemod, &iffreqConfig)!= SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("sony_demod_SetIFFreqConfig failed.",ERR_DEV_ERROR);	
	}
		
	/*
	 * In default, The setting is optimized for Sony silicon tuners.
	 * If non-Sony tuner is used, the user should call following to
	 * disable Sony silicon tuner optimized setting.
	 */
	//step 2: =====if non-sony is used.config it
	if (sony_demod_SetConfig (pdemod, SONY_DEMOD_CONFIG_TUNER_OPTIMIZE, SONY_DEMOD_TUNER_OPTIMIZE_NONSONY) != SONY_RESULT_OK)
	{
	   
	   NIM_TRACE_RETURN_ERROR("sony_demod_SetConfig (SONY_DEMOD_CONFIG_TUNER_OPTIMIZE) failed.",ERR_DEV_ERROR);
	}
	
	// step 3:====config TS output
	/*************************************config demod tso **********************************/
	
	//Config TS output mode: SSI/SPI. Default is SPI.
	result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_PARALLEL_SEL, 0 );
	if (result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("Unable to configure DEMOD_CONFIG_PARALLEL_SEL.",ERR_DEV_ERROR);
	}
	//Confif Data MSB or LSB pin: 0:LSB , 1:MSB, default:1.
	result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_OUTPUT_SEL_MSB, 1 );
	if (result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("Unable to configure SONY_DEMOD_CONFIG_OUTPUT_SEL_MSB.",ERR_DEV_ERROR);
	}
	//Config TS Sync output.
	result = sony_demod_SetConfig(pdemod,SONY_DEMOD_CONFIG_TSSYNC_ACTIVE_HI, 1);
	if (result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("Unable to configure DEMOD_CONFIG_TSERR_ENABLE.",ERR_DEV_ERROR);
	}
	//Config TS Error output.
	result = sony_demod_SetConfig(pdemod,SONY_DEMOD_CONFIG_TSERR_ACTIVE_HI, 1);
	if (result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("Unable to configure DEMOD_CONFIG_TSERR_ENABLE.",ERR_DEV_ERROR);
	}
	//Confif Data output from demod pin: 0:TS0 of demod (pin13), 1:TS7 of demod(pin20), default:1.
	result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_SER_DATA_ON_MSB, 0 );
	if (result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("Unable to configure DEMOD_CONFIG_PARALLEL_SEL.",ERR_DEV_ERROR);
	}
	
	//step 4 : ========config polarity of IFAGC for tuner;0 : for positive; 1 : for negtive
	if(MXL603 == user->tuner_name)
	{
		result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_IFAGCNEG, 0);
		if (result != SONY_RESULT_OK)
		{
			NIM_TRACE_RETURN_ERROR("Unable to configure IFAGCNEG.",ERR_DEV_ERROR);
		}
	}
	else
	{
		result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_IFAGCNEG, 1);
		if (result != SONY_RESULT_OK)
		{
			NIM_TRACE_RETURN_ERROR("Unable to configure IFAGCNEG.",ERR_DEV_ERROR);
		}
	}
	/*IFAGC ADC range[0-2]	   0 : 1.4Vpp, 1 : 1.0Vpp, 2 : 0.7Vpp*/   
	result = sony_demod_SetConfig(pdemod,SONY_DEMOD_CONFIG_IFAGC_ADC_FS, 1);//DEMOD_CONFIG_TSERR_ENABLE
	if (result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("Unable to configure SONY_DEMOD_CONFIG_IFAGC_ADC_FS",ERR_DEV_ERROR);
	}
	/***************************** special config*************************/
	/*
	Before burning sony demod ,if frequently called function of monitoring(SSI,SQI,RF,PER...).
	sony demod will  be abnormal,resulting in i2c error,so add the special config
	*/
	//Configure the clock frequency for Serial TS in terrestrial and cable active states.
	 result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_TERR_CABLE_TS_SERIAL_CLK_FREQ, 3 );
	 if (result != SONY_RESULT_OK)
	 {
			  NIM_TRACE_RETURN_ERROR("Unable to configure SONY_DEMOD_CONFIG_TERR_CABLE_TS_SERIAL_CLK_FREQ.",ERR_DEV_ERROR);
	 }
	 //Configure Serial TS clock gated on valid TS data or is continuous.
	 result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_TSCLK_CONT, 0);
	 if (result != SONY_RESULT_OK)
	 {
			  NIM_TRACE_RETURN_ERROR("Unable to configure SONY_DEMOD_CONFIG_TSCLK_CONT.",ERR_DEV_ERROR);
	 }

	// Configure the driving current for the TS Clk pin.
     result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_TSCLK_CURRENT, 1 );
     if (result != SONY_RESULT_OK)
     {
              NIM_TRACE_RETURN_ERROR("Unable to configure SONY_DEMOD_CONFIG_TSCLK_CURRENT.",ERR_DEV_ERROR);
     }
     // Configure the driving current for the TS Sync / TS Valid.
     result = sony_demod_SetConfig(pdemod, SONY_DEMOD_CONFIG_TS_CURRENT, 1);
     if (result != SONY_RESULT_OK)
     {
              NIM_TRACE_RETURN_ERROR("Unable to configure SONY_DEMOD_CONFIG_TS_CURRENT.",ERR_DEV_ERROR);
     }
	/**********************************************************************/

	//Ben Debug 140221#1
	//add by AEC for TS error enable 2013-09-09
	 // TSERR output enable from GPIO2 pin
	result = sony_demod_GPIOSetConfig(pdemod, 2, 1, SONY_DEMOD_GPIO_MODE_TS_OUTPUT);
	if(result != SONY_RESULT_OK)
	{
		NIM_TRACE_RETURN_ERROR("sony_demod_GPIOSetConfig for TS error",ERR_DEV_ERROR);
	}
	//end for TS error enable 2013-09-09  
	
	NIM_TRACE_RETURN_SUCCESS();
}

int nim_cxd2856_close(struct nim_device *dev)
{
	sony_integ_t 		* priv 	= (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T  * user 	= priv->user;
	sony_demod_t 		* pdemod= priv->pDemod;
    int result = SUCCESS;

	NIM_TRACE_ENTER();
    // tuner power off
    if (user->tuner_control.nim_tuner_command != NULL)
    {
    	if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
		{
			/* Enable the I2C repeater */			
			sony_demod_I2cRepeaterEnable (pdemod, 0x01);
		}
		
    	user->tuner_control.nim_tuner_command(user->tuner_id, NIM_TUNER_POWER_CONTROL, TRUE);
		
		if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
		{
			/* disable the I2C repeater */
			sony_demod_I2cRepeaterEnable (pdemod, 0x00);
		}
    }
	
    result =  sony_demod_Shutdown(pdemod);//sony_dvb_demod_Finalize
    if (result != SONY_RESULT_OK)
    {
		NIM_TRACE_RETURN_ERROR("error",ERR_FAILUE);
    }
	
	NIM_TRACE_RETURN_SUCCESS();
}

int nim_set_dev_config(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
	sony_integ_t 		* priv = (sony_integ_t *)dev->priv;
	ALI_CXD2856_DATA_T  * user = priv->user;
	TUNER_IO_FUNC		* p_io_func	= NULL;

	NIM_TRACE_ENTER();
	if (NULL==nim_cfg)
	{
		NIM_TRACE_RETURN_ERROR("NULL==nim_cfg!",ERR_DEV_ERROR);
	}
	memcpy(&user->tuner_control.config_data, &nim_cfg->cofdm_data, sizeof(struct COFDM_TUNER_CONFIG_DATA));
	memcpy(&user->tuner_control.tuner_config, &nim_cfg->tuner_config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
	memcpy(&user->tuner_control.ext_dm_config, &nim_cfg->ext_dm_config, sizeof(struct EXT_DM_CONFIG));
	
	user->demod_i2c_type	= nim_cfg->ext_dm_config.i2c_type_id;
	user->demod_addr		= nim_cfg->ext_dm_config.i2c_base_addr;
	//user->tuner_id 			= nim_cfg->tuner_id;
	user->tuner_i2c_type 	= nim_cfg->tuner_config.i2c_type_id;
	user->tuner_addr		= nim_cfg->tuner_config.c_tuner_base_addr;
	pr_info("[%s] line=%d,flag=%d,i2c_type=%d,i2c_addr=0x%x,nim_type=%u\n", __FUNCTION__, __LINE__,
		user->tuner_control.config_data.flag,
		user->tuner_control.tuner_config.i2c_type_id,
		user->tuner_control.tuner_config.c_tuner_base_addr,
		user->nim_type);

	p_io_func = tuner_setup(user->nim_type,user->tuner_name);
	if(NULL != p_io_func)
	{
		user->tuner_control.nim_tuner_init		= (dvbt_tuner_init)p_io_func->pf_init;
		user->tuner_control.nim_tuner_control	= (dvbt_tuner_control)p_io_func->pf_control;
		user->tuner_control.nim_tuner_status  	= (dvbt_tuner_status)p_io_func->pf_status;
		user->tuner_control.nim_tuner_command	= (dvbt_tuner_command)(p_io_func->pf_command);
		user->tuner_control.nim_tuner_close		= (dvbt_tuner_close)p_io_func->pf_close;
		NIM_TRACE_RETURN_SUCCESS();
	}
	else
	{
		NIM_TRACE_RETURN_ERROR("Tuner API is NULL!",ERR_DEV_ERROR);
		
	}
}

int nim_cxd2856_hwreset(struct nim_device *dev)
{
	sony_integ_t       * priv	= NULL;
	ALI_CXD2856_DATA_T * user	= NULL;
	
	NIM_TRACE_ENTER();
	if (NULL == dev)
	{
		NIM_TRACE_RETURN_ERROR("NULL == dev",-EFAULT);
	}

	priv = dev->priv;
	user = priv->user;
	
	pr_info("[%s %d]priv->reset_pin=%d!\n", __FUNCTION__, __LINE__, user->reset_pin);	
	gpio_direction_output(user->reset_pin, 1);//set gpio to high
	msleep(10);	
	gpio_direction_output(user->reset_pin, 0);//set gpio to low
	msleep(100);	 	
	gpio_direction_output(user->reset_pin, 1);//set gpio to high	
	msleep(10);//this step must be set
	NIM_TRACE_RETURN_SUCCESS();
}

int ali_cxd2856_nim_hw_initialize(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
{
	/***********************data relation*************************
	sony_i2c_t	|
				|-------> *user //(ALI_CXD2856_DATA_T)

	************************************************************/

	sony_integ_t 				* priv = NULL;
	ALI_CXD2856_DATA_T 			* user = NULL;
	sony_demod_t 				* pdemod = NULL;
	sony_i2c_t 					* pdemod_I2c=NULL;
	sony_tuner_terr_cable_t   	* tunerTerrCable;//sony tuner data
	struct nim_debug  			* debug_data =  NULL;
	sony_demod_create_param_t 	createParam;//xtal,i2c_addr
	DEM_WRITE_READ_TUNER 		ThroughMode;

	NIM_TRACE_ENTER();
	if (NULL == dev)
	{
		NIM_TRACE_RETURN_ERROR("Tuner Configuration API structure is NULL!", -ENOMEM);
	}
	
	/************************bind data*****************************/
	priv 		= dev->priv;
	pdemod 		= priv->pDemod;
	user 		= priv->user;
	debug_data 	= &user->debug_data;
	/*************************************************************/
	if(user->nim_init)
	{
		return SUCCESS;
	}
	pdemod_I2c = (sony_i2c_t*)kmalloc(sizeof(sony_i2c_t),GFP_KERNEL);
	if(NULL == pdemod_I2c)
	{
		NIM_TRACE_RETURN_ERROR("kmalloc faile!!!!", -ENOMEM);
	}
	tunerTerrCable = (sony_tuner_terr_cable_t*)kmalloc(sizeof(sony_tuner_terr_cable_t),GFP_KERNEL);
	if(NULL == tunerTerrCable)
	{
		goto error1;
	}
	memset(pdemod_I2c, 0, sizeof(sony_i2c_t));
	memset(tunerTerrCable, 0, sizeof(sony_tuner_terr_cable_t));
	memset(&createParam, 0, sizeof(sony_demod_create_param_t));
	/************************bind data*****************************/
	pdemod_I2c->user = user;
	/*************************************************************/
	if(gpio_is_valid(user->reset_pin))
	{
		if(nim_cxd2856_hwreset(dev) != SUCCESS)
		{
			goto error2;
		}
	}
	
	
	
	/*Setup demod I2C && XTAL interfaces.*/
	createParam.xtalFreq 		= SONY_DEMOD_XTAL_24000KHz; /* 24MHz Xtal */
	createParam.i2cAddressSLVT 	= user->demod_addr; /* I2C slave address */
	/*
	   Note: If use the repeater of i2c mode,demod will be abnormal when a long time burn.
	   So,must use the gatewat of i2c mode.
	*/
	if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER)
	{
    	createParam.tunerI2cConfig = SONY_DEMOD_TUNER_I2C_CONFIG_REPEATER; /* I2C repeater is used */
	}
	else if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_GATEWAY)
	{
		createParam.tunerI2cConfig = SONY_DEMOD_TUNER_I2C_CONFIG_GATEWAY;/* I2C getway is used */
	}
	else
	{
		NIM_TRACE_RETURN_ERROR("don't find the tuner i2c mode!",ERR_DEV_ERROR);
		goto error2;
	}
    pdemod_I2c->ReadRegister 	= nim_cxd2856_i2c_CommonReadRegister;
    pdemod_I2c->WriteRegister 	= nim_cxd2856_i2c_CommonWriteRegister;
    pdemod_I2c->WriteOneRegister= nim_cxd2856_i2c_CommonWriteOneRegister;
	
	/* Create demodulator instance */
	if(sony_integ_Create(priv,pdemod,&createParam,pdemod_I2c,tunerTerrCable)!= SONY_RESULT_OK)
	{
		pr_err("[err] create sony demod error!\n");
		goto error2;
	}
	/*
	when sony_integ_Create() was completed,now:
	
	priv->pTunerTerrCable = tunerTerrCable;
	pdemod->pI2c = pdemod_I2c;

	*/
	pdemod->user = user;
	pdemod->state = SONY_DEMOD_STATE_ACTIVE;
	//It must be set after sony_integ_Create(),because sony_integ_Create() will empty all setting of pdemod.
	
	/* Demod tunerOptimize member allows the demod to be optimized internally when connected to Sony RF parts. */
	if(user->tuner_name == CXD2861 || user->tuner_name == CXD2872)
	{
		pdemod->tunerOptimize = SONY_DEMOD_TUNER_OPTIMIZE_SONYSILICON;	
	}
	else
	{
		pdemod->tunerOptimize = SONY_DEMOD_TUNER_OPTIMIZE_NONSONY;
	}

	/* ---------------------------------------------------------------------------------
	* 							Configure the Demodulator
	* ------------------------------------------------------------------------------ */
	
	if(nim_cxd2856_open(dev) != SUCCESS)
	{
		pr_err("set demod config error!\n");
		goto error2;
	}
	/* ---------------------------------------------------------------------------------
	*							     Configure the Tuner
	* ------------------------------------------------------------------------------ */
    if (user->tuner_control.nim_tuner_init == NULL || user->tuner_control.nim_tuner_command == NULL)
    {
    	pr_err( "Error: interface point of tuner Init or Command is NULL!\r\n");
		goto error2;
    	
    }
	else
	{
		if (user->tuner_control.nim_tuner_init(&(user->tuner_id), &(user->tuner_control.tuner_config)) != SUCCESS)
        {
            pr_err("Error: Init Tuner Failure!\r\n");
            goto error2;
        }
		//set the getaway mode of tuner i2c to tuner if config from dts is getway
    	ThroughMode.nim_dev_priv = dev->priv;
	    ThroughMode.Dem_Write_Read_Tuner = (INTERFACE_DEM_WRITE_READ_TUNER)nim_cxd2856_i2c_TunerGateway;
		if(user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_GATEWAY)
		{
			/* I2C getway is used */
			user->tuner_control.nim_tuner_command(user->tuner_id, NIM_TUNER_SET_THROUGH_MODE, (UINT32)&ThroughMode);
		}
	} 
	user->nim_init = TRUE;
#ifdef CONFIG_CXD2856_DEBUG_PER //for this case : need to default to open per printing
	debug_data->monitor_object = 0x02;//print per when per not 0
	debug_data->monitor_status = TASK_RUN;
	schedule_work(&dev->debug_work);//run task
	pr_warning("\nenable cxd2856 monitor\n");
#endif
    NIM_TRACE_RETURN_SUCCESS();

	
error1:
	kfree(pdemod_I2c);
	NIM_TRACE_RETURN_ERROR("error,abnormal exit !!!\n",-ENOMEM);
error2:
	kfree(tunerTerrCable);
	goto error1;
}

