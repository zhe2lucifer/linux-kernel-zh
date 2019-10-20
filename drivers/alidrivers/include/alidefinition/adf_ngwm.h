/*****************************************************************************
*	 Copyright (c) 2017 ALi Corp. All Rights Reserved
*	 This source is confidential and is ALi's proprietary information.
*	 This source is subject to ALi License Agreement, and shall not be 
	 disclosed to unauthorized individual.	  
*	 File: adf_ngwm.h
*	 Description: this file is used to define some macros and structures 
*				  for NexGuard embedded watermarking
*	 THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
	  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	  PARTICULAR PURPOSE.
*****************************************************************************/

#ifndef _ADF_NGWM_H_
#define _ADF_NGWM_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 *	 @brief Define the return-value for NGWM 
*/
#define NGWM_NO_ERROR (0)
#define NGWM_ERROR (0x1000)
#define NGWM_ERROR_NODEV (0x1001)
#define NGWM_ERROR_INVALID_PARAM (0x1002)
#define NGWM_ERROR_INVALID_SESS (0x1004)
#define NGWM_ERROR_NOT_SUPPORTED (0x1008)
#define NGWM_ERROR_NOT_ALLOWED (0x1010)
#define NGWM_ERROR_NO_KEYIN (0x1020)
#define NGWM_ERROR_NO_SETTING (0x1040)

/*!
 *	 @brief Define the NGWM core ID
*/
#define NGWM_HDMI_ID (0x0)
#define NGWM_CVBS_ID (0x1)
#define NGWM_SPP1_ID (0x2)
#define NGWM_SPP2_ID (0x3)

/*!
 *	 @brief Define the NGWM ioctl commands 
*/
#define NGWM_IO_SET_SEED_KEYIN (0x0)
/*!< Set the seed/KeyIn.*/
#define NGWM_IO_SET_OPERATOR_ID (0x1)
/*!< Set the operator ID.*/
#define NGWM_IO_SET_SETTING_ARRAY (0x2)
/*!< Set the setting array for this session.*/
#define NGWM_IO_SET_SUBSCRIBER_ID (0x3)
/*!< Update the subscriber ID/UniqueID.*/
#define NGWM_IO_SET_TIMECODE (0x4)
/*!< Update the timecode.*/
#define NGWM_IO_ENABLE_SERVICE (0x5)
/*!< on/off.*/
#define NGWM_IO_ENABLE_DEBUG (0x10)
/*!< on/off the stub debug.*/


/*!
 *	 @brief Define the NGWM setting type 
*/
#define NGWM_SDR8 (0x1)
/*!< The watermark is to be applied on 8 bit SDR display output.*/
#define NGWM_SDR10 (0x2)
/*!< The watermark is to be applied on 10 bit SDR display output.*/
#define NGWM_HDR10 (0x3)
/*!< The watermark is to be applied on HDR10 display output.*/
#define NGWM_HLG (0x4)
/*!< The watermark is to be applied on HLD display output.*/
#define NGWM_DOLBY (0x5)
/*!< The watermark is to be applied on DolbyVision display output.*/

/*! @struct ngwm_subscriber_id
 *	 @brief Define the structure for UniqueID/SubscriberID.
*/
struct ngwm_subscriber_id
{
    unsigned int id;
};

/*! @struct ngwm_setting
 *	 @brief Define the setting along with the setting type.
*/
struct ngwm_setting
{
    int type;
    /*!< Specify the setting type,
    which are #NGWM_SDR8, #NGWM_SDR10 and #NGWM_HDR10.
    */
    unsigned char setting[21];
    /*!< Specify the setting payload. 
    The setting is a 165-bit value Setting[164:0]
    */
};

#ifdef __cplusplus
}
#endif

#endif

