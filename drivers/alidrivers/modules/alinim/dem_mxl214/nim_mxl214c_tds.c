#include "nim_mxl214c.h"



static unsigned char nim_dev_num = 0;

static char     nim_mxl214c_name[3][HLD_MAX_NAME_SIZE] =
{
    "NIM_MXL214C_0", "NIM_MXL214C_1", "NIM_MXL214C_2"
};



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

    i2c_addr = nim_mxl214c_manager_get_i2c_addr(devid);
    i2c_type_id = nim_mxl214c_manager_get_i2c_type_id(devid);
    i2c_data[0] = (UINT8)((regaddr >> 8) & 0xFF);
    i2c_data[1] = (UINT8)((regaddr) & 0xFF);
    i2c_data[2] = (UINT8)((regdata >> 8) & 0xFF);
    i2c_data[3] = (UINT8)((regdata) & 0xFF);
    
    ret_code = i2c_write(i2c_type_id,i2c_addr,i2c_data,4);
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
    i2c_addr = nim_mxl214c_manager_get_i2c_addr(devid);
    i2c_type_id = nim_mxl214c_manager_get_i2c_type_id(devid);
    i2c_data[0] = 0xFF;
    i2c_data[1] = 0xFB;
    i2c_data[2] = (UINT8)((regaddr >> 8) & 0xFF);
    i2c_data[3] = (UINT8)((regaddr) & 0xFF);    
    ret_code = i2c_write_read_std(i2c_type_id,i2c_addr,i2c_data,4,2);    
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
    MXL_STATUS_E status = MXL_SUCCESS;
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
    i2c_addr = nim_mxl214c_manager_get_i2c_addr(devid);
    i2c_type_id = nim_mxl214c_manager_get_i2c_type_id(devid);
    i2c_data = (UINT8 *)comm_malloc(bufsize + 2);
    if((i2c_data != NULL) && (bufptr != NULL))
    {
        i2c_data[0] = (UINT8)((regaddr >> 8) & 0xFF);
        i2c_data[1] = (UINT8)((regaddr) & 0xFF);
        comm_memcpy(i2c_data + 2,bufptr,bufsize);
        ret_code = i2c_write(i2c_type_id,i2c_addr,i2c_data,bufsize + 2);
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
        free(i2c_data);
    
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
    i2c_addr = nim_mxl214c_manager_get_i2c_addr(devid);
    i2c_type_id = nim_mxl214c_manager_get_i2c_type_id(devid);
    i2c_data = (UINT8 *)comm_malloc(readsize + 4);
    if((i2c_data != NULL) && (bufptr != NULL))
    {
        i2c_data[0] = 0xFF;
        i2c_data[1] = 0xFD;        
        i2c_data[2] = (UINT8)((regaddr >> 8) & 0xFF);
        i2c_data[3] = (UINT8)((regaddr) & 0xFF);
        //memcpy(i2c_data + 2,bufptr,bufsize);
        ret_code = i2c_write_read_std(i2c_type_id,i2c_addr,i2c_data,4,readsize);
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
        free(i2c_data);
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
    i2c_addr = nim_mxl214c_manager_get_i2c_addr(devid);
    i2c_type_id = nim_mxl214c_manager_get_i2c_type_id(devid);
    i2c_data = (UINT8 *)comm_malloc(readsize + 6);
    if((i2c_data != NULL) && (bufptr != NULL))
    {
        i2c_data[0] = 0xFF;
        i2c_data[1] = 0xFD;        
        i2c_data[2] = (UINT8)((cmdid >> 8) & 0xFF);
        i2c_data[3] = (UINT8)((cmdid) & 0xFF);
        i2c_data[4] = (UINT8)((offset >> 8) & 0xFF);
        i2c_data[5] = (UINT8)((offset) & 0xFF);        

        ret_code = i2c_write_read_std(i2c_type_id,i2c_addr,i2c_data,6,readsize);
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
        free(i2c_data);
    
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
  // !!! FIXME !!! 
  // To be implemented for customer OEM platform
  //usleep(usec);
    osal_task_sleep(usec/1000);
    osal_delay(usec%1000);
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
  struct timeval currtime = 0;
  long           ucurtick = 0;
#if __WORDSIZE == 32  
  static unsigned long long sec_count = 0;
  unsigned long long mask = 0;
  unsigned long long incrementvalue = 0;
#endif  
  unsigned long long timeinusec;

  MxL_HRCLS_DEBUG(" MxL_HRCLS_Ctrl_GetTimeTickInUsec ");
  //gettimeofday(&currTime, NULL);
    currtime.tv_sec = osal_get_time();
    ucurtick = read_tsc();
    currtime.tv_usec= ucurtick/( (sys_ic_get_cpu_clock()*1000000 / 2000000));
  
#if __WORDSIZE == 32  
  incrementvalue = ((unsigned long long) 1 << (sizeof(currtime.tv_sec) * 8));
  mask = incrementvalue-1;  

  if ((unsigned long long) currtime.tv_sec < (sec_count & mask))
  {
    sec_count += incrementvalue;
  }
  sec_count &= ~(mask);
  sec_count |= (currtime.tv_sec & mask);
  timeinusec = (unsigned long long)(sec_count * 1000 * 1000) + (unsigned long long) currtime.tv_usec;
#else
  timeinusec = (unsigned long long)((unsigned long long) currtime.tv_sec * 1000 * 1000) + (unsigned long long)currtime.tv_usec;
//printf("sec:%x, usec:%lu, timeInUsec:%Lu\n", (unsigned int) currTime.tv_sec, (unsigned long) currTime.tv_usec, timeInUsec);  
#endif
  *usecptr= (UINT64) timeinusec;

  // return MXL_SUCCESS;
}




static INT32 nim_mxl214c_open(struct nim_device *dev)
{
     return nim_mxl214c_hw_init(dev);
   
}

static INT32 nim_mxl214c_close(struct nim_device *dev)
{
    
}

static INT32 nim_mxl214c_ioctl(struct nim_device *dev, INT32 cmd, UINT32 param)
{
	INT32 rtn = SUCCESS;
	switch( cmd )
	{
	case NIM_DRIVER_READ_QPSK_BER:
	    rtn =  nim_mxl214c_get_ber(dev, (UINT32 *)param);
	    break;
	case NIM_DRIVER_READ_RSUB:
	    rtn =  nim_mxl214c_get_per(dev, (UINT32 *)param);
	    break;
	case NIM_DRIVER_SET_RESET_CALLBACK:
	    MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line = %d,wakeup!\n",__FUNCTION__,__LINE__);
		break;		
	case NIM_TURNER_SET_STANDBY:
	    MXL214C_PRINTF(NIM_LOG_DBG,"[%s] line = %d,standby!\n",__FUNCTION__,__LINE__);
     	break;
	case NIM_DRIVER_GET_AGC:
		rtn =  nim_mxl214c_get_agc(dev, (UINT8 *)param);
		break;        
	default:
	    rtn = SUCCESS;
	    break;
	}
	return rtn;    
}

static INT32 nim_mxl214c_ioctl_ext(struct nim_device *dev, INT32 cmd, void* param_list)
{
	INT32 rtn = SUCCESS;
	switch( cmd )
	{
	case NIM_DRIVER_AUTO_SCAN:			// Do AutoScan Procedure 
		rtn = SUCCESS;
		break;
	case NIM_DRIVER_CHANNEL_CHANGE:		// Do Channel Change 
		rtn = nim_mxl214c_channel_change(dev, (NIM_CHANNEL_CHANGE_T *) (param_list));
		break;
	case NIM_DRIVER_QUICK_CHANNEL_CHANGE:	// Do Quick Channel Change without waiting lock 
		break;
	case NIM_DRIVER_CHANNEL_SEARCH:	// Do Channel Search 
		rtn= SUCCESS;
		break;
	case NIM_DRIVER_GET_RF_LEVEL:
		rtn = nim_mxl214c_get_rf_level(dev, (UINT16 *)param_list);
		break;
	case NIM_DRIVER_GET_CN_VALUE:
		rtn = nim_mxl214c_get_cn_value(dev, (UINT16 *)param_list);
		break;
	case NIM_DRIVER_GET_BER_VALUE:
		rtn = nim_mxl214c_get_ber(dev, (UINT32 *)param_list);
		break;
	case NIM_DRIVER_GET_I2C_INFO:
		break;		
	default:
		rtn = SUCCESS;
	       break;
	}

	return rtn;
    
}



INT32 nim_mxl214c_attach(struct QAM_TUNER_CONFIG_API * ptrqam_tuner)
{
	struct nim_device           *dev        = NULL;
	struct nim_mxl214c_private  *priv_mem   = NULL;

    if(ptrqam_tuner == NULL)
    {
        return ERR_PARA;
    }


    dev = (struct nim_device *)dev_alloc((unsigned char *)nim_mxl214c_name[nim_dev_num], HLD_DEV_TYPE_NIM, 
		                                 sizeof(struct nim_device));
    if (dev == NULL)
    {
    	MXL214C_PRINTF(NIM_LOG_DBG,"Error: Alloc nim device error!\n");
    	return ERR_NO_MEM;
    }

    /* Alloc structure space of private */
    priv_mem = (struct nim_mxl214c_private *)comm_malloc(sizeof(struct nim_mxl214c_private));	
    if ((void*)priv_mem == NULL)
    {
    	dev_free(dev);
    	MXL214C_PRINTF(NIM_LOG_DBG,"Alloc nim device prive memory error!/n");
    	return ERR_NO_MEM;
    }
    comm_memset((void*)priv_mem, 0, sizeof(struct nim_mxl214c_private));

    /* tuner configuration function */
    comm_memcpy((void*)&(priv_mem->qam_tuner_config), (void*)(ptrqam_tuner), sizeof(struct QAM_TUNER_CONFIG_API));

    dev->priv = (void*)priv_mem;
    /* Function point init */
    dev->base_addr = ptrqam_tuner->ext_dem_config.i2c_base_addr;	//0x40
    dev->init = nim_mxl214c_attach;
    dev->open = nim_mxl214c_open;
    dev->stop = nim_mxl214c_close;
    dev->do_ioctl = nim_mxl214c_ioctl;
    dev->do_ioctl_ext = nim_mxl214c_ioctl_ext;
    dev->get_lock = nim_mxl214c_get_lock;
    dev->get_freq = nim_mxl214c_get_freq;
    dev->get_FEC = nim_mxl214c_get_qam_order;	
    dev->get_SNR = nim_mxl214c_get_snr;
    dev->get_sym = nim_mxl214c_get_symbol_rate;
    dev->get_BER = nim_mxl214c_get_ber;
    dev->get_AGC = nim_mxl214c_get_agc;

    
    /* Add this device to queue */
    if (dev_register(dev) != SUCCESS)
    {
    	MXL214C_PRINTF(NIM_LOG_DBG,"Error: Register nim device error!\n");
    	FREE(priv_mem);
    	dev_free(dev);
    	return ERR_NO_DEV;
    }
	nim_dev_num++;
	

    return SUCCESS;    
}


