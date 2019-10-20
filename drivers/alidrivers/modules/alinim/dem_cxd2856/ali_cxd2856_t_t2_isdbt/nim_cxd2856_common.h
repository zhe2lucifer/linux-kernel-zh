#ifndef NIM_CXD2856_COMMMON_H
#define NIM_CXD2856_COMMMON_H

#include "sony_integ.h"
#include "sony_dtv.h"
#include "sony_demod.h"
#include "sony_i2c.h"
#include "sony_demod_dvbt.h"
#include "sony_demod_dvbt2.h"
#include "sony_demod_dvbt2_monitor.h"
#include "sony_demod_dvbt_monitor.h"
#include "sony_dvbt.h"
#include "sony_dvbt2.h"
#include "sony_integ_dvbt2.h"
#include "sony_integ_dvbt.h"
#include "sony_integ_dvbt_t2.h"
#include "sony_demod_isdbt.h"
#include "sony_demod_isdbt_monitor.h"
#include "sony_integ_isdbt.h"
#include "sony_math.h"
#include "sony_stdlib.h"

#include "porting_cxd2856_linux.h"
#include "nim_cxd2856_debug.h"
#include "nim_cxd2856_proc.h"
#include "nim_cxd2856_monitor.h"
#include "nim_cxd2856_hw_cfg.h"
#include "nim_cxd2856_channel_change.h"

#define INVALID_VALUE 0XFFFFFFF //when need return a invalid value,can use it,NOTE: 7 byte not 8byte

/*****************************************************************************
*sony_result_t nim_cxd2856_i2c_CommonReadRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t* pData, uint32_t size);
* Description: read  values of register
*			
* Arguments: 
*	pI2c:i2c private data
*	deviceAddress:  device addr 
*	subAddress: register addr	
*  	pData:  read data
*	size: read len
* Return Value: SONY_RESULT_OK if ok
*****************************************************************************/
sony_result_t nim_cxd2856_i2c_CommonReadRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t* pData, uint32_t size);

/*****************************************************************************
*sony_result_t nim_cxd2856_i2c_CommonWriteRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, const uint8_t* pData, uint32_t size);
* Description: write  values of register
*			
* Arguments: 
*	pI2c:i2c private data
*	deviceAddress:  device addr 
*	subAddress: register addr	
*  	pData:  write data
*	size: write len
* Return Value: SONY_RESULT_OK if ok
*****************************************************************************/
sony_result_t nim_cxd2856_i2c_CommonWriteRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, const uint8_t* pData, uint32_t size);

/*****************************************************************************
*sony_result_t nim_cxd2856_i2c_CommonWriteOneRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t data);
* Description: write  one value of register
*			
* Arguments: 
*	pI2c:i2c private data
*	deviceAddress:  device addr 
*	subAddress: register addr	
*  	pData:  write data
* Return Value: SONY_RESULT_OK if ok
*****************************************************************************/
sony_result_t nim_cxd2856_i2c_CommonWriteOneRegister(sony_i2c_t* pI2c, uint8_t deviceAddress, uint8_t subAddress, uint8_t data);

/*****************************************************************************
*sony_result_t nim_cxd2856_TunerGateway(void* nim_dev_priv, UINT8	tuner_address , UINT8* wdata , int wlen , UINT8* rdata , int rlen);
* Description: write  or read  values of register with GETWAY (this is sony demod define) mode.
*			
* Arguments: 
*	nim_dev_priv:nim private data
*	tuner_address:  device addr 
*	subAddress: register addr	
*  	wdata:  write data
*	wlen: write len
*	rdata: read data
*	rlen:	 read len
* Return Value: SONY_RESULT_OK if ok
*****************************************************************************/
sony_result_t nim_cxd2856_i2c_TunerGateway(void* nim_dev_priv, UINT8	tuner_address , UINT8* wdata , int wlen , UINT8* rdata , int rlen);

long ali_cxd2856_nim_ioctl_mutex(struct nim_device *dev,unsigned int cmd, unsigned long parg);


#endif 
