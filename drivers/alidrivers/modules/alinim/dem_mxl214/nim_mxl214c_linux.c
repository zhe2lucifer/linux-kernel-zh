

#include <linux/platform_device.h>
#include "nim_mxl214c.h"


#define ALI_NIM_DEVICE_NAME         "ali_nim_mxl214c"
#define MAX_TUNER_SUPPORT_NUM         4

/*
* redefined
#define NIM_TUNER_SET_STANDBY_CMD    0xffffffff 
*/

struct nim_device 					ali_mxl214c_nim_dev[MAX_TUNER_SUPPORT_NUM];
struct nim_mxl214c_private          *ali_mxl214c_nim_priv[MAX_TUNER_SUPPORT_NUM]={NULL,NULL,NULL,NULL} ;
struct class 						*ali_mxl214c_nim_class =NULL;
struct device 						*ali_mxl214c_nim_dev_node[MAX_TUNER_SUPPORT_NUM] = {NULL,NULL,NULL,NULL};



/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_HRCLS_OEM_Reset
--| 
--| DESCRIPTION   : This function resets MxL_HRCLS through Reset Pin
--| PARAMETERS    : devid - MxL_HRCLS device id
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_Reset(UINT8 devid)
{
    MXL_STATUS_E status = MXL_SUCCESS;

    // !!! FIXME !!!
    // OEM should toggle reset pin of MxL_HRCLS specified by I2C Slave Addr
    
    nim_mxl214c_reset(devid);
    return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_HRCLS_OEM_WriteRegister
--|
--| DESCRIPTION   : This function does I2C write operation.
--| PARAMETERS    : devid - MxL_HRCLS device id
--|                 regaddr - Register address of MxL_HRCLS to write.
--|                 regdata - Data to write to the specified address.
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_WriteRegister(UINT8 devid, UINT16 regaddr, UINT16 regdata)
{
    MXL_STATUS_E    status = MXL_SUCCESS;
    INT32           ret_code = SUCCESS;
    UINT32          i2c_addr = 0;
    UINT32          i2c_type_id = 0;
    UINT8           i2c_data[4] = {0};
	
    // !!! FIXME !!!
    // OEM should implement I2C write protocol that complies with MxL_HRCLS I2C
    // format.

    // 16bit Register Write Protocol:
    // +------+-+-----+-+-+----------+-+----------+-+----------+-+----------+-+-+
    // |MASTER|S|SADDR|W| |regaddr(H)| |regaddr(L)| |regdata(H)| |regdata(L)| |P|
    // +------+-+-----+-+-+----------+-+----------+-+----------+-+----------+-+-+
    // |SLAVE |         |A|          |A|          |A|          |A|          |A| |
    // +------+---------+-+----------+-+----------+-+----------+-+----------+-+-+
    // Legends: SADDR (I2c slave address), S (Start condition), A (Ack), N(NACK), P(Stop condition)

    i2c_addr = ali_mxl214c_nim_priv[devid]->tuner_config_ext.c_tuner_base_addr;
	i2c_type_id =ali_mxl214c_nim_priv[devid]->tuner_config_ext.i2c_type_id;
	
    
    i2c_data[0] = (UINT8)((regaddr >> 8) & 0xFF);
    i2c_data[1] = (UINT8)((regaddr) & 0xFF);
    i2c_data[2] = (UINT8)((regdata >> 8) & 0xFF);
    i2c_data[3] = (UINT8)((regdata) & 0xFF);
    
    ret_code = ali_i2c_write(i2c_type_id,i2c_addr,i2c_data,4);
    if(ret_code != SUCCESS)
    {
        status = MXL_FAILURE;
    }
    
    return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_HRCLS_OEM_ReadRegister
--|
--| DESCRIPTION   : This function does I2C read operation.
--| PARAMETERS    : devid - MxL_HRCLS device id
--|                 regaddr - Register address of MxL_HRCLS to read.
--|                 dataPtr - Data container to return 16 data.
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_ReadRegister(UINT8 devid, UINT16 regaddr, UINT16 *dataPtr)
{
    MXL_STATUS_E status = MXL_SUCCESS;
    INT32           ret_code = SUCCESS;
    UINT32          i2c_addr = 0;
    UINT32          i2c_type_id = 0;
    UINT8           i2c_data[4] = {0};
	
    // !!! FIXME !!!
    // OEM should implement I2C read protocol that complies with MxL_HRCLS I2C
    // format.

    // 16 Register Read Protocol:
    // +------+-+-----+-+-+----+-+----+-+----------+-+----------+-+-+
    // |MASTER|S|SADDR|W| |0xFF| |0xFB| |regaddr(H)| |regaddr(L)| |P|
    // +------+-+-----+-+-+----+-+----+-+----------+-+----------+-+-+
    // |SLAVE |         |A|    |A|    |A|          |A|          |A| |
    // +------+-+-----+-+-+----+-+----+-+----------+-+----------+-+-+
    // +------+-+-----+-+-+-------+--+-------+--+-+
    // |MASTER|S|SADDR|R| |       |MA|       |MN|P|
    // +------+-+-----+-+-+-------+--+-------+--+-+
    // |SLAVE |         |A|Data(H)|  |Data(L)|  | |
    // +------+---------+-+-------+--+----------+-+
    // Legends: SADDR(I2c slave address), S(Start condition), MA(Master Ack), MN(Master NACK), P(Stop condition)

	i2c_addr = ali_mxl214c_nim_priv[devid]->tuner_config_ext.c_tuner_base_addr;
	i2c_type_id =ali_mxl214c_nim_priv[devid]->tuner_config_ext.i2c_type_id;

	
    i2c_data[0] = 0xFF;
    i2c_data[1] = 0xFB;
    i2c_data[2] = (UINT8)((regaddr >> 8) & 0xFF);
    i2c_data[3] = (UINT8)((regaddr) & 0xFF);    
    ret_code = ali_i2c_write_read(i2c_type_id,i2c_addr,i2c_data,4,2);    
    if(ret_code != SUCCESS)
    {
        status = MXL_FAILURE;
    }
    *dataPtr = (UINT16)((i2c_data[0]<<8)|i2c_data[1]);
    return status;
}
  
/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_HRCLS_OEM_WriteBlock
--|
--| DESCRIPTION   : This function does I2C block write operation.
--| PARAMETERS    : devid - MxL_HRCLS device id
--|                 regaddr - Register address of MxL_HRCLS to start a block write.
--|                 bufsize - The number of bytes to write
--|                 bufptr - Data bytes to write to the specified address.
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_WriteBlock(UINT8 devid, UINT16 regaddr, UINT16 bufsize, UINT8 *bufptr)
{
    MXL_STATUS_E    status = MXL_SUCCESS;
    INT32           ret_code = SUCCESS;
    UINT32          i2c_addr = 0;
    UINT32          i2c_type_id = 0;
    UINT8           *i2c_data = NULL;
	
    // !!! FIXME !!!
    // OEM should implement I2C block write protocol that complies with MxL_HRCLS I2C
    // format.

    // Block Write Protocol:
    // +------+-+-----+-+-+----------+-+----------+-+---------+-+---+-----------------+-+-+
    // |MASTER|S|SADDR|W| |regaddr(H)| |regaddr(L)| |bufptr[0]| |   |bufptr[Bufsize-1]| |P|
    // +------+-+-----+-+-+----------+-+----------+-+---------+-+...+-----------------+-+-+
    // |SLAVE |         |A|          |A|          |A|         |A|   |                 |A| |
    // +------+---------+-+----------+-+---- -----+-+---------+-+---+-----------------+-+-+
    // Legends: SADDR(I2c slave address), S(Start condition), A(Ack), P(Stop condition)
    i2c_addr = ali_mxl214c_nim_priv[devid]->tuner_config_ext.c_tuner_base_addr;
	i2c_type_id =ali_mxl214c_nim_priv[devid]->tuner_config_ext.i2c_type_id;
	

    i2c_data = (UINT8 *)comm_malloc(bufsize + 2);
    if((i2c_data != NULL) && (bufptr != NULL))
    {
        i2c_data[0] = (UINT8)((regaddr >> 8) & 0xFF);
        i2c_data[1] = (UINT8)((regaddr) & 0xFF);
        comm_memcpy(i2c_data + 2,bufptr,bufsize);
        ret_code = ali_i2c_write(i2c_type_id,i2c_addr,i2c_data,bufsize + 2);
        if(ret_code != SUCCESS)
        {
            status = MXL_FAILURE;
        }        
    }
    else
    {
        status = MXL_FAILURE;
    }
    if(i2c_data != NULL)
    {
		comm_free(i2c_data);
    }	
    
    return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare_HRCLS_OEM_ReadBlock
--|
--| DESCRIPTION   : This function does I2C block read operation.
--| PARAMETERS    : devid - MxL_HRCLS device id
--|                 regaddr - Register Address to start a block read
--|                 readsize - The number of bytes to read
--|                 bufptr - Container to hold readback data
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_ReadBlock(UINT8 devid, UINT16 regaddr, UINT16 readsize, UINT8 *bufptr)
{
    MXL_STATUS_E status = MXL_SUCCESS;
    INT32        ret_code = SUCCESS;
    UINT32       i2c_addr = 0;
    UINT32       i2c_type_id = 0;
    UINT8        *i2c_data = NULL;
    
    // !!! FIXME !!!
    // OEM should implement I2C block read protocol that complies with MxL_HRCLS I2C
    // format.

    // Block Read Protocol (n bytes of data):
    // +------+-+-----+-+-+----+-+----+-+----------+-+----------+-+-+
    // |MASTER|S|SADDR|W| |0xFF| |0xFD| |regaddr(H)| |regaddr(L)| |P|
    // +------+-+-----+-+-+----+-+----+-+----------+-+----------+-+-+
    // |SLAVE |         |A|    |A|    |A|          |A|          |A| |
    // +------+-+-----+-+-+----+-+----+-+----------+-+----------+-+-+
    // +------+-+-----+-+-------+-+-----+-+-----+-+-----+-+-+
    // |MASTER|S|SADDR|R|       |A|     |A|       |     |N|P|
    // +------+-+-----+-+-+-----+-+-----+-+  ...  +-----+-+-+
    // |SLAVE |         |A|DATA1| |DATA2| |       |DATAn|   |
    // +------+---------+-+-----+-+-----+-+-----+-+-----+---+
    // Legends: SADDR (I2c slave address), S (Start condition), A (Acknowledgement), N(NACK), P(Stop condition)
    i2c_addr = ali_mxl214c_nim_priv[devid]->tuner_config_ext.c_tuner_base_addr;
	i2c_type_id =ali_mxl214c_nim_priv[devid]->tuner_config_ext.i2c_type_id;
	

    i2c_data = (UINT8 *)comm_malloc(readsize + 4);
    if((i2c_data != NULL) && (bufptr != NULL))
    {
        i2c_data[0] = 0xFF;
        i2c_data[1] = 0xFD;        
        i2c_data[2] = (UINT8)((regaddr >> 8) & 0xFF);
        i2c_data[3] = (UINT8)((regaddr) & 0xFF);
        //memcpy(i2c_data + 2,bufptr,bufsize);
        ret_code = ali_i2c_write_read(i2c_type_id,i2c_addr,i2c_data,4,readsize);
        if(ret_code != SUCCESS)
        {
            status = MXL_FAILURE;
        }
        comm_memcpy(bufptr,i2c_data,readsize);
    }
    else
    {
        status = MXL_FAILURE;
    }
    if(i2c_data != NULL)
    {
		comm_free(i2c_data);
    }	
    return status;
}

MXL_STATUS_E MxLWare_HRCLS_OEM_ReadBlockExt(UINT8 devid, UINT16 cmdid, UINT16 offset, UINT16 readsize, UINT8 *bufptr)
{
    MXL_STATUS_E status = MXL_SUCCESS;
    INT32        ret_code = SUCCESS;
    UINT32       i2c_addr = 0;
    UINT32       i2c_type_id = 0;
    UINT8        *i2c_data = NULL;
    
    // !!! FIXME !!!
    // OEM should implement I2C block read protocol that complies with MxL_HRCLS I2C
    // format.

    // Block Read Protocol (n bytes of data):
    // +------+-+-----+-+-+----+-+----+-+--------+-+--------+-+----------+-+---------+-+-+
    // |MASTER|S|SADDR|W| |0xFF| |0xFD| |cmdid(H)| |cmdid(L)| |offset(H) | |offset(L)| |P|
    // +------+-+-----+-+-+----+-+----+-+--------+-+--------+-+----------+-+---------+-+-+
    // |SLAVE |         |A|    |A|    |A|        |A|        |A|          |A|         |A| |
    // +------+-+-----+-+-+----+-+----+-+--------+-+--------+-+----------+-+---------+-+-+
    // +------+-+-----+-+-------+-+-----+-+-----+-+-----+-+-+
    // |MASTER|S|SADDR|R|       |A|     |A|       |     |N|P|
    // +------+-+-----+-+-+-----+-+-----+-+  ...  +-----+-+-+
    // |SLAVE |         |A|DATA1| |DATA2| |       |DATAn|   |
    // +------+---------+-+-----+-+-----+-+-----+-+-----+---+
    // Legends: SADDR (I2c slave address), S (Start condition), A (Acknowledgement), N(NACK), P(Stop condition)
    i2c_addr = ali_mxl214c_nim_priv[devid]->tuner_config_ext.c_tuner_base_addr;
	i2c_type_id =ali_mxl214c_nim_priv[devid]->tuner_config_ext.i2c_type_id;
	

    i2c_data = (UINT8 *)comm_malloc(readsize + 6);
    if((i2c_data != NULL) && (bufptr != NULL))
    {
        i2c_data[0] = 0xFF;
        i2c_data[1] = 0xFD;        
        i2c_data[2] = (UINT8)((cmdid >> 8) & 0xFF);
        i2c_data[3] = (UINT8)((cmdid) & 0xFF);
        i2c_data[4] = (UINT8)((offset >> 8) & 0xFF);
        i2c_data[5] = (UINT8)((offset) & 0xFF);        

        ret_code = ali_i2c_write_read(i2c_type_id,i2c_addr,i2c_data,6,readsize);
        if(ret_code != SUCCESS)
        {
            status = MXL_FAILURE;
        }
        comm_memcpy(bufptr,i2c_data,readsize);
    }
    else
    {
        status = MXL_FAILURE;
    }
    if(i2c_data != NULL)
    {
		comm_free(i2c_data);
    }	
    
    return status;
}

/*------------------------------------------------------------------------------
--|
--| FUNCTION NAME : MxLWare_HRCLS_OEM_LoadNVRAMFile
--|
--| DESCRIPTION   : Load MxLNVRAMFile
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_LoadNVRAMFile(UINT8 devid, UINT8 *bufptr, UINT32 buflen)
{
  MXL_STATUS_E status = MXL_FAILURE;

  // !!! FIXME !!! 
  // To be implemented for customer OEM platform
  status = MXL_SUCCESS;
  return status;
}

/*------------------------------------------------------------------------------
--|
--| FUNCTION NAME : MxLWare_HRCLS_OEM_SaveNVRAMFile
--|
--| DESCRIPTION   : Save MxLNVRAMFile
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS_E MxLWare_HRCLS_OEM_SaveNVRAMFile(UINT8 devid, UINT8 *bufptr, UINT32 buflen)
{
  MXL_STATUS_E status = MXL_FAILURE;
  
  // !!! FIXME !!! 
  // To be implemented for customer OEM platform
  status = MXL_SUCCESS; 
  return status;  
}

/*------------------------------------------------------------------------------
--|
--| FUNCTION NAME : MxLWare_HRCLS_OEM_DelayUsec
--|
--| DESCRIPTION   : Delay in micro-seconds
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

void MxLWare_HRCLS_OEM_DelayUsec(UINT32 usec)
{
  comm_sleep(usec/1000);
}

/*------------------------------------------------------------------------------
--|
--| FUNCTION NAME : MxLWare_HRCLS_OEM_GetCurrTimeInUsec
--|
--| DESCRIPTION   : Get current time in micro-seconds
--|
--| RETURN VALUE  : MXL_SUCCESS or MXL_FAILURE
--|
--|---------------------------------------------------------------------------*/

void MxLWare_HRCLS_OEM_GetCurrTimeInUsec(UINT64* usecptr)
{

	struct timeval currentTime;
    do_gettimeofday(&currentTime);	
    *usecptr = currentTime.tv_usec + (UINT64) ((UINT64) currentTime.tv_sec * 1000000);
}



static INT32 ali_nim_mxl214c_hw_initialize(struct nim_device *dev, struct ali_nim_m3200_cfg *nim_cfg)
{
	struct nim_mxl214c_private *priv = NULL;

	priv = (struct nim_mxl214c_private *)(dev->priv);


	
	//MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter!\n", __FUNCTION__,__LINE__);
	/* tuner configuration function */
	comm_memcpy((void*)&(priv->tuner_config_data), (void*)&(nim_cfg->tuner_config_data), sizeof(struct QAM_TUNER_CONFIG_DATA));
	comm_memcpy((void*)&(priv->tuner_config_ext), (void*)&(nim_cfg->tuner_config_ext), sizeof(struct QAM_TUNER_CONFIG_EXT));
	comm_memcpy((void*)&(priv->ext_dem_config), (void*)&(nim_cfg->ext_dem_config), sizeof(struct EXT_DM_CONFIG));

	priv->qam_mode = nim_cfg->qam_mode;


   MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,demod_i2c=%d[0x%x],tuner_i2c=%d[0x%x]\n", 
   	                      __FUNCTION__, __LINE__,
   	                      priv->ext_dem_config.i2c_type_id,
   	                      priv->ext_dem_config.i2c_base_addr,
   	                      priv->tuner_config_ext.i2c_type_id,
   	                      priv->tuner_config_ext.c_tuner_base_addr);


    return nim_mxl214c_hw_init(dev);

}

static long ali_mxl214c_nim_ioctl(struct file *file, unsigned int cmd, unsigned long parg)
{
    struct nim_device *dev = file->private_data;
    struct nim_mxl214c_private *priv = dev->priv;
    int ret = 0;

    switch(cmd)
    {
	case ALI_NIM_TUNER_SELT_ADAPTION_C:
    {
        struct ali_nim_m3200_cfg nim_param;

        if(copy_from_user(&nim_param, (struct ali_nim_m3200_cfg *)parg, sizeof(struct ali_nim_m3200_cfg))>0)
		{
			MXL214C_PRINTF(NIM_LOG_ERR,"%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
        //ret = ali_m3281_tuner_adaption(dev, &nim_param);

		break;
	}
    case ALI_NIM_HARDWARE_INIT_C:
    {
        struct ali_nim_m3200_cfg nim_param;

        memset((void*)&nim_param,0,sizeof(struct ali_nim_m3200_cfg));
        if(copy_from_user(&nim_param, (struct ali_nim_m3200_cfg *)parg, sizeof(struct ali_nim_m3200_cfg))>0)
		{
			MXL214C_PRINTF(NIM_LOG_ERR,"%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}

        ret = ali_nim_mxl214c_hw_initialize(dev, &nim_param);

        break;
    }
    case ALI_NIM_CHANNEL_CHANGE:
    {
        NIM_CHANNEL_CHANGE_T nim_param;

        MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,nim_param.fec=%d\n", __FUNCTION__, __LINE__, nim_param.fec);

		memset((void*)&nim_param,0,sizeof(NIM_CHANNEL_CHANGE_T));
		
        if(copy_from_user(&nim_param, (NIM_CHANNEL_CHANGE_T *)parg, sizeof(NIM_CHANNEL_CHANGE_T))>0)
		{
			MXL214C_PRINTF(NIM_LOG_ERR,"%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}

        return nim_mxl214c_channel_change(dev, &nim_param);
        break;
    }
    case ALI_NIM_GET_LOCK_STATUS:
    {
        UINT8 lock = 0;
        
        nim_mxl214c_get_lock(dev, &lock);
        ret = lock;
        break;
    }
    case ALI_NIM_READ_QPSK_BER:
    {
        UINT32 ber = 0;
        
        nim_mxl214c_get_ber(dev, &ber);
        ret = ber;
        break;
    }
    case ALI_NIM_READ_RSUB:
    {
        UINT32 per = 0;

        nim_mxl214c_get_per(dev, &per);
        ret = per;
        break;
    }
    case ALI_NIM_DRIVER_READ_SUMPER:
    {
        UINT32 per_sum = 0;


        if(copy_from_user(&per_sum, (UINT32 *)parg, sizeof(UINT32))>0)
		{
			MXL214C_PRINTF(NIM_LOG_ERR,"%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
			
        nim_mxl214c_get_per(dev, &per_sum);
        ret = per_sum;
        break;
    }
    case ALI_NIM_GET_RF_LEVEL:
    {
        UINT16 rf_level = 0;

        nim_mxl214c_get_rf_level(dev, &rf_level);
        ret = rf_level;
        break;
    }
	case ALI_NIM_LOG_LEVEL:
	{
        UINT32 log_level= 0;

		if(copy_from_user(&log_level, (void __user *)parg, sizeof(int))>0)
		{
			printk("%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}
        set_log_level(log_level);
		ret=0;
		
		break;
	}	
    case ALI_NIM_GET_CN_VALUE:
    {
        UINT16 cn_value = 0;

        nim_mxl214c_get_cn_value(dev, &cn_value);
        ret = cn_value;
        break;
    }
    case ALI_NIM_SET_PERF_LEVEL:
    {
      //  ret = nim_mxl214c_set_perf_level(dev, parg);
        break;
    }
    case ALI_NIM_READ_AGC:
    {
        UINT8 agc = 0;

        nim_mxl214c_get_agc(dev, &agc);

        ret = agc;
        break;
    }
    case ALI_NIM_READ_SNR:
    {
        UINT8 snr = 0;

        nim_mxl214c_get_snr(dev, &snr);
        ret = snr;
        break;
    }
    case ALI_NIM_READ_SYMBOL_RATE:
    {
        UINT32 sym = 0;

        nim_mxl214c_get_symbol_rate(dev, &sym);
        ret = sym;
        break;
    }
    case ALI_NIM_READ_FREQ:
    {
        UINT32 freq = 0;

        nim_mxl214c_get_freq(dev, &freq);
        ret = freq;
        break;
    }
    case ALI_NIM_READ_CODE_RATE:
    {
        UINT8 fec = 0;

        nim_mxl214c_get_qam_order(dev, &fec);
        ret = fec;
        break;
    }
    case ALI_NIM_DRIVER_SET_MODE:
    {

        UINT8 mode_cmd[32] ={0};

        MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,18\n", __FUNCTION__, __LINE__);

        if(copy_from_user(mode_cmd, (UINT8 *)parg, 8)>0)
		{
			MXL214C_PRINTF(NIM_LOG_ERR,"%s error line%d\n", __FUNCTION__, __LINE__);
			// Invalid user space address
			return -EFAULT;
		}			
        if (4 == mode_cmd[0])
        {
#ifndef MONITOR_NIM_STATUS
            MXL214C_PRINTF(NIM_LOG_DBG,"[TRCAE] print mode invalid for not start NIM monitor thread\n");
#else
            DEBUG_SHOW_NIM_STATUS = ((0 == DEBUG_SHOW_NIM_STATUS) ? 1 : 0);
            MXL214C_PRINTF(NIM_LOG_DBG,"[TRCAE] print mode %s\n", (1 == DEBUG_SHOW_NIM_STATUS) ? "ON" : "OFF");
#endif
        }
        else
        {
            mode_cmd[1] = ((priv->qam_mode) & 0x01);
            if(copy_to_user((UINT8 *)parg, mode_cmd, 8)>0)
			{
				MXL214C_PRINTF(NIM_LOG_ERR,"%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				return -EFAULT;
			}
						
            if ((mode_cmd[0] < 2) && (mode_cmd[1] != mode_cmd[0]))
            {
                priv->qam_mode &= ~(1 << 0);
                priv->qam_mode |= mode_cmd[0];
                MXL214C_PRINTF(NIM_LOG_DBG,"[TRACE:%d]NIM work mode changed to %s\n", __LINE__, (1 == priv->qam_mode) ? "J83AC" : "J83B");
                //kent
                //nim_s3281_dvbc_set_mode(priv, struct DEMOD_CONFIG_ADVANCED qam_config)

                //  nim_s3202_mode_set4start(priv);
            }
            ret = ((priv->qam_mode) & 0x01);
        }

        break;
    }
    default:
    {
        MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,23\n", __FUNCTION__, __LINE__);
        ret = -ENOIOCTLCMD;
        break;
    }
    }

    return ret;
}

void nim_mxl214_hwreset(void)
{
    //GPIO[77] need pull up to reset demod
    int gpio_index = 13; 
    __REG32ALI(0x18000438) |= 1 << gpio_index;
    __REG32ALI(0x180000f4) |= 1 << gpio_index;
	__REG32ALI(0x180000f8) &= ~(1 << gpio_index);
	MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,reset!\n", __FUNCTION__,__LINE__);
	comm_sleep(300);
	__REG32ALI(0x180000f8) |= 1 << gpio_index;
    comm_sleep(100);
}

static int ali_mxl214c_nim_open(struct inode *inode, struct file *file)
{
    UINT8  confit_flag = 0;
	UINT8   dev_idx = 0;
    
    dev_idx = MINOR(inode->i_rdev);

    MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,dev_idx=%d,enter!\n", __FUNCTION__,__LINE__,dev_idx);
	
	file->private_data=(void*)&ali_mxl214c_nim_dev[dev_idx];

	if(!confit_flag) 
	{
	   nim_mxl214_hwreset();
	   confit_flag = 1;
	}
	
    return RET_SUCCESS;
}

static int ali_mxl214c_nim_release(struct inode *inode, struct file *file)
{

    MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d,enter\n", __FUNCTION__, __LINE__);

    return RET_SUCCESS;
}


static struct file_operations ali_mxl214c_nim_fops =
{
    .owner		= THIS_MODULE,
    .write		= NULL,
    .unlocked_ioctl	= ali_mxl214c_nim_ioctl,
    .open		= ali_mxl214c_nim_open,
    .release	= ali_mxl214c_nim_release,
};

extern int of_get_major_minor(struct device_node *enode, dev_t *dev,
                       unsigned baseminor, unsigned count,const char *name);

static int nim_mxl214c_probe(struct platform_device *pdev)
{
	INT32 i = 0;
	INT32 ret = SUCCESS;
	dev_t devno;

	MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line=%d enter,max_tuner=%d!\n", __FUNCTION__,__LINE__,MAX_TUNER_SUPPORT_NUM);

	ret = of_get_major_minor(pdev->dev.of_node,&devno, 
			0, MAX_TUNER_SUPPORT_NUM, ALI_NIM_DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}
	ali_mxl214c_nim_class = class_create(THIS_MODULE, "ali_mxl214c_nim_class");
	if (IS_ERR(ali_mxl214c_nim_class))
	{
		MXL214C_PRINTF(NIM_LOG_DBG,"[%s]line=%d,class_create error,back!\n", __FUNCTION__,__LINE__);
		ret = PTR_ERR(ali_mxl214c_nim_class);
		return ret;
	}

    for(i = 0; i < MAX_TUNER_SUPPORT_NUM; i++)
    {
        ali_mxl214c_nim_priv[i] = kmalloc(sizeof(struct nim_mxl214c_private), GFP_KERNEL);
        if (!ali_mxl214c_nim_priv[i])
        {
            MXL214C_PRINTF(NIM_LOG_DBG,"kmalloc failed!\n");
            ret = -ENOMEM;
            break;
        }
        comm_memset(ali_mxl214c_nim_priv[i], 0, sizeof(struct nim_mxl214c_private));
        mutex_init(&ali_mxl214c_nim_priv[i]->i2c_mutex);
       
        cdev_init(&ali_mxl214c_nim_dev[i].cdev, &ali_mxl214c_nim_fops);
        ali_mxl214c_nim_dev[i].cdev.owner = THIS_MODULE;
        ali_mxl214c_nim_dev[i].cdev.ops = &ali_mxl214c_nim_fops;

        ali_mxl214c_nim_dev[i].priv = (void *)ali_mxl214c_nim_priv[i];
        ali_mxl214c_nim_priv[i]->dev_idx=i;
		ali_mxl214c_nim_priv[i]->tuner_id = i;
		
        ret = cdev_add(&ali_mxl214c_nim_dev[i].cdev, devno + i, 1);
        if(ret)
        {
            MXL214C_PRINTF(NIM_LOG_DBG,"Alloc NIM device failed, err: %d.\n", (int)ret);
            mutex_destroy(&ali_mxl214c_nim_priv[i]->i2c_mutex);
            kfree(ali_mxl214c_nim_priv[i]);
        }

        ali_mxl214c_nim_dev_node[i] = device_create(ali_mxl214c_nim_class, NULL, MKDEV(MAJOR(devno), i),
                                    &ali_mxl214c_nim_dev[i], "ali_mxl214c_nim%d", (int)i);
        if(IS_ERR(ali_mxl214c_nim_dev_node[i]))
        {
            MXL214C_PRINTF(NIM_LOG_ERR,"device_create() failed!\n");
            ret = PTR_ERR(ali_mxl214c_nim_dev_node[i]);
            cdev_del(&ali_mxl214c_nim_dev[i].cdev);
            mutex_destroy(&ali_mxl214c_nim_priv[i]->i2c_mutex);
            kfree(ali_mxl214c_nim_priv[i]);
        }
    }



    return ret;

}

static int nim_mxl214c_remove(struct platform_device *pdev)
{

    UINT8 i = 0;

    MXL214C_PRINTF(NIM_LOG_DBG,"[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

    if(ali_mxl214c_nim_class != NULL)
    {
		class_destroy(ali_mxl214c_nim_class);
    }	

    for (i = 0; i < MAX_TUNER_SUPPORT_NUM; i++)
    {
        if(ali_mxl214c_nim_dev_node[i] != NULL)
        {
			device_del(ali_mxl214c_nim_dev_node[i]);
        }	


        cdev_del(&ali_mxl214c_nim_dev[i].cdev);
        mutex_destroy(&ali_mxl214c_nim_priv[i]->i2c_mutex);


        kfree(ali_mxl214c_nim_priv[i]);
    }
    MXL214C_PRINTF(NIM_LOG_DBG,"[%s]line=%d,end!\n", __FUNCTION__, __LINE__);
	
	return 0;
}

static const struct of_device_id ali_nim_mxl214c_of_match[] = {
	{ .compatible= "alitech, nim_mxl214c", },
	{},
};

static struct platform_driver nim_mxl214c_platform_driver =
{
	.probe      = nim_mxl214c_probe,
	.remove     = nim_mxl214c_remove,
	.driver     = {
	            .owner = THIS_MODULE,
				.name  = "ali_nim_mxl214c",
				.of_match_table = ali_nim_mxl214c_of_match,
	},
};

module_platform_driver(nim_mxl214c_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kent");
MODULE_DESCRIPTION("Ali MXL214c NIM driver");
