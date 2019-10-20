/*----------------------------------------------------------------------------
   Solution:  demodulater CXD2856. + tuner MXL603
------------------------------------------------------------------------------
    History:       
	Date                  athor           version                reason 
------------------------------------------------------------------------------
    2017/04/06          leo.liu           v0.1                  re-encapsulate the i2c interface for ali       
----------------------------------------------------------------------------*/
#include "nim_cxd2856_common.h"


#define BURST_WRITE_MAX 128 /* Max length of burst write */
#if 1
sony_result_t nim_cxd2856_i2c_CommonReadRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t* pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
	ALI_CXD2856_DATA_T * user = pI2c->user;
    if(!pI2c){
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    mutex_lock(&user->i2c_mutex);
    pData[0] = subAddress;
	pr_debug("[i2c_read] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,pData[0],pData[1],size);
    if ( SUCCESS != nim_i2c_write_read(user->demod_i2c_type, deviceAddress, pData, 1, size) )
    {
		pr_err("[demod read error rp] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,pData[0],pData[1],size);
        result = SONY_RESULT_ERROR_I2C;
    }
	else
	{
		pr_debug("[demod i2c_read rp]  read ok data = 0x%x\n",pData[0]);
	}
    mutex_unlock(&user->i2c_mutex);
    
    SONY_TRACE_I2C_RETURN(result);
}
sony_result_t nim_cxd2856_i2c_CommonWriteRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, const uint8_t* pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
	ALI_CXD2856_DATA_T * user = pI2c->user;
    uint8_t buffer[BURST_WRITE_MAX + 1];

    if(!pI2c){
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    if(size > BURST_WRITE_MAX){
        /* Buffer is too small... */
        pr_err("%s(): buffer[%d] is too small: pData[%d]\r\n", __FUNCTION__, BURST_WRITE_MAX, size);
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    mutex_lock(&user->i2c_mutex);
    
    buffer[0] = subAddress;
    sony_memcpy(&(buffer[1]), pData, size);
	pr_debug("[i2c_write]i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,buffer[0],buffer[1],size);
    /* send the new buffer */
    if ( SUCCESS != nim_i2c_write(user->demod_i2c_type, deviceAddress, buffer, size+1) )
	{
		pr_err("[demod write error rp] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,buffer[0],buffer[1],size);
        result = SONY_RESULT_ERROR_I2C;
    }
	else
	{
    	pr_debug("[demod i2c_write rp] write ok\n");
    //CXD2856_PRINTF(user, result, 1, deviceAddress, buffer, size+1);
	}

    mutex_unlock(&user->i2c_mutex);
    
    SONY_TRACE_I2C_RETURN(result);
}

#else
sony_result_t nim_cxd2856_i2c_CommonReadRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t* pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
	ALI_CXD2856_DATA_T * user = pI2c->user;
    if(!pI2c){
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    mutex_lock(&user->i2c_mutex);
    pData[0] = subAddress;
	pr_debug("[i2c_read] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,pData[0],pData[1],size);
    if ( SUCCESS != nim_i2c_read(user->demod_i2c_type, deviceAddress, pData, 1, size) )
    {
		pr_err("[demod read error rp] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,pData[0],pData[1],size);
        result = SONY_RESULT_ERROR_I2C;
    }
	else
	{
		pr_debug("[demod i2c_read rp]  read ok data = 0x%x\n",pData[0]);
	}
    mutex_unlock(&user->i2c_mutex);
    
    SONY_TRACE_I2C_RETURN(result);
}

sony_result_t nim_cxd2856_i2c_CommonWriteRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, const uint8_t* pData, uint32_t size)
{
    sony_result_t result = SONY_RESULT_OK;
	ALI_CXD2856_DATA_T * user = pI2c->user;
    uint8_t buffer[BURST_WRITE_MAX + 1];

    if(!pI2c){
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    if(size > BURST_WRITE_MAX){
        /* Buffer is too small... */
        pr_err("%s(): buffer[%d] is too small: pData[%d]\r\n", __FUNCTION__, BURST_WRITE_MAX, size);
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    mutex_lock(&user->i2c_mutex);
    
    buffer[0] = subAddress;
    sony_memcpy(&(buffer[1]), pData, size);
	pr_debug("[i2c_write]i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,buffer[0],buffer[1],size);
    /* send the new buffer */
    if ( SUCCESS != nim_i2c_write(user->demod_i2c_type, deviceAddress, buffer, size+1) )
	{
		pr_err("[demod write error rp] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,deviceAddress,buffer[0],buffer[1],size);
        result = SONY_RESULT_ERROR_I2C;
    }
	else
	{
    	pr_debug("[demod i2c_write rp] write ok\n");
    //CXD2856_PRINTF(user, result, 1, deviceAddress, buffer, size+1);
	}

    mutex_unlock(&user->i2c_mutex);
    
    SONY_TRACE_I2C_RETURN(result);
}
#endif
sony_result_t nim_cxd2856_i2c_CommonWriteOneRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t data)
{
    sony_result_t result = SONY_RESULT_OK;;

    if(!pI2c){
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    result = pI2c->WriteRegister(pI2c, deviceAddress, subAddress, &data, 1);
    SONY_TRACE_I2C_RETURN(result);
}

/* For Read-Modify-Write */
sony_result_t nim_cxd2856_SetRegisterBits(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t data, uint8_t mask)
{
    sony_result_t result = SONY_RESULT_OK;



    if(!pI2c){
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
    if(mask == 0x00){
        /* Nothing to do */
        SONY_TRACE_I2C_RETURN(SONY_RESULT_OK);
    }
    
    if(mask != 0xFF){
        uint8_t rdata = 0x00;
        result = pI2c->ReadRegister(pI2c, deviceAddress, subAddress, &rdata, 1);
        if(result != SONY_RESULT_OK){ SONY_TRACE_I2C_RETURN(result); }
        data = (uint8_t)((data & mask) | (rdata & (mask ^ 0xFF)));
    }

    result = pI2c->WriteOneRegister(pI2c, deviceAddress, subAddress, data);
    SONY_TRACE_I2C_RETURN(result);
}

//this is a complex communication mode,so don' t recommend using it
#if 1
sony_result_t nim_cxd2856_i2c_TunerGateway(void* nim_dev_priv, UINT8	tuner_address , UINT8* wdata , int wlen , UINT8* rdata , int rlen)
{
    uint8_t buffer[BURST_WRITE_MAX + 2]={0};//Protect against overflow: GW Sub Adress (1) + Tuner address (1)
    sony_integ_t *priv = (sony_integ_t*)nim_dev_priv;
	ALI_CXD2856_DATA_T * user = priv->user;
    sony_result_t result = SONY_RESULT_OK;


    if( (0 == wlen) && (0 == rlen) )
    {
        pr_err( "%s(): No data for read/write.\r\n", __FUNCTION__);
        SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
    }
	if ( wlen != 0 ) //write to tuner
	{
		if(wlen > BURST_WRITE_MAX)    //Protect against overflow: GW Sub Adress (1) + Tuner address (1)
		{
			pr_err( "%s(): buffer[%d] is too small: Write wdata[%d]\r\n", __FUNCTION__, BURST_WRITE_MAX, wlen);
			SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
		}
		mutex_lock(&user->i2c_mutex);
		//Payload: [GWSub] [TunerAddress] [Data0] ... [DataN]
		//{S} [Addr] [GW Sub] [TunerAddr+Write]
		buffer[0] = 0x09;   //The tuner gateway register of Sony demod.
		buffer[1] = tuner_address&0xFE;   //[TunerAddr+Write]
		sony_memcpy (&(buffer[2]), wdata, wlen);
		if ( SUCCESS != nim_i2c_write(user->demod_i2c_type,user->demod_addr, buffer, wlen+2))
		{
			pr_err("[demod write error gt] i2c_type %lu dev_addr 0x%x reg_addr 0x%x data 0x%x size %u\n",user->demod_i2c_type,tuner_address,buffer[0],buffer[1],wlen);
			result = SONY_RESULT_ERROR_I2C;
		}
		//CXD2837_LOG_I2C(pDemod, result, 1, pDemod->i2cAddressSLVT, buffer, wlen+2);
		mutex_unlock(&user->i2c_mutex);

		SONY_TRACE_I2C_RETURN(result);
	}

	if ( rlen != 0 ) //read from tuner
	{	
		if(rlen > BURST_WRITE_MAX)
		{
			pr_err( "%s(): buffer[%d] is too small: Read rdata[%d]\r\n", __FUNCTION__, BURST_WRITE_MAX, rlen);
			SONY_TRACE_I2C_RETURN(SONY_RESULT_ERROR_ARG);
		}
		//{S} [Addr] [GW Sub] [TunerAddr+Read]
		//hostI2C->Write (hostI2C, pDemod->i2cAddress, data, sizeof (data), SONY_I2C_START_EN)
		//{SR} [Addr+Read] [Read0] [Read1] ... [ReadN] {P} 
		//hostI2C->Read (hostI2C, pI2c->gwAddress, pData, size, SONY_I2C_START_EN | SONY_I2C_STOP_EN);

		mutex_lock(&user->i2c_mutex);
		//{S} [Addr] [GW Sub] [TunerAddr+Read]
		buffer[0] = 0x09;   //The tuner gateway register of Sony demod. 
		buffer[1] = tuner_address|1;   //[TunerAddr+Read]
		//CXD2837_LOG_I2C(pDemod, result, 1, pDemod->i2cAddressSLVT, buffer, 2);
		//if ( SUCCESS != nim_i2c_read(user->demod_i2c_type, user->demod_addr, buffer, 2, rlen) )
		//if ( SUCCESS != i2c_write_read(user->demod_i2c_type,user->demod_addr, buffer, 2, rlen))
		if ( SUCCESS != ali_i2c_write_read(user->demod_i2c_type,user->demod_addr, buffer, 2, rlen))
		{	
			
			pr_err("[demod read error gt] i2c_type %lu demod_addr 0x%x tuner_addr 0x%x size %u\n",user->demod_i2c_type,user->demod_addr,tuner_address,rlen);
			result = SONY_RESULT_ERROR_I2C;
		}
		//CXD2837_LOG_I2C(pDemod, result, 0, pDemod->i2cAddressSLVT, buffer, rlen);
		if (SONY_RESULT_OK == result )
			sony_memcpy(rdata, buffer, rlen);

		mutex_unlock(&user->i2c_mutex);

		SONY_TRACE_I2C_RETURN(result);
	}
	SONY_TRACE_I2C_RETURN(result);
}
#endif

