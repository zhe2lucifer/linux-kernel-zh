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
 
/***************************************************************************************************

*    File:    
*		hdmi_io.h
*
*    Description:    
*		This file define data type for hdmi driver processing.
*
*    History:
*	 	Date           Author        	Version     Reason
*	 	============	=============	=========	=================
*
*************************************************************************************************/

#ifndef __HDMI_IO_COMMON_
#define __HDMI_IO_COMMON_

//#include <asm/types.h>
//#include <linux/types.h>
//#include <linux/ioctl.h>

#define HDMI_IOC_MAGIC 'h'

#define HDMI_API_RES_SUPPORT_NUM 14


typedef struct  {
	unsigned int hdmi_status;
} HDMI_ioctl_sw_onoff_state_t;

typedef struct  {
	unsigned int hdcp_status;
} HDMI_ioctl_hdcp_state_t;

typedef struct  {
	unsigned int mem_sel;
} HDMI_ioctl_mem_sel_t;  //add by ze, for ce_load_key

typedef struct  {
	unsigned char scramble;
	unsigned char hdcp_ksv[5];
	unsigned char encrypted_hdcp_keys[280];
} HDMI_ioctl_hdcp_key_info_t;

typedef struct  {
	int link_status;
} HDMI_ioctl_link_status_t;

typedef struct  {
	unsigned char vendor_name[8];
	unsigned char length;
} HDMI_ioctl_vendor_name_t;

typedef struct  {
	unsigned char product_desc[16];
	unsigned char length;
} HDMI_ioctl_product_desc_t;

typedef struct  {
	unsigned int cec_status;
} HDMI_ioctl_cec_state_t;

typedef struct  {
	unsigned short cec_addr;
	int ret;
} HDMI_ioctl_cec_addr_t;

typedef struct  {
	unsigned char message[16];
	unsigned char message_length;
	int ret;
} HDMI_ioctl_cec_msg_t;

typedef struct  {
	enum EDID_AUD_FMT_CODE *aud_fmt;
	int ret;
} HDMI_ioctl_audio_out_t;

typedef struct  {
	unsigned int native_res_index;
	enum HDMI_API_RES video_res_list[HDMI_API_RES_SUPPORT_NUM];
	int  ret;
} HDMI_ioctl_edid_res_list_t;

typedef struct  {
	enum HDMI_API_RES video_res;
	int  ret;
} HDMI_ioctl_video_res_t;

typedef struct  {
	enum pic_fmt format;
	int  ret;
} HDMI_ioctl_video_format_t;

typedef struct {
	unsigned int hdmi_audio_status;
}HDMI_ioctl_hdmi_audio_state_t;

typedef struct {
    int present;
}HDMI_ioctl_3d_status_t;

typedef struct {
    unsigned char manufacturer[4];
    unsigned char monitor[14];
}HDMI_ioctl_edid_vendor_t;

typedef struct {
    int mute;
}HDMI_ioctl_avmute_t;

typedef struct  {
	enum HDMI_API_DEEPCOLOR dp_mode;
} HDMI_ioctl_deep_color_t;

typedef struct  {
	int block_num;
	unsigned char	block[128];
	int length;
	unsigned char edid_buf[8][128];
} HDMI_ioctl_edid_block_data_t;

typedef struct {
	unsigned char onoff;
}HDMI_ioctl_phy_clock_t;

typedef struct {
	enum HDMI_API_COLOR_SPACE color_space;
}HDMI_ioctl_color_space_t;

typedef struct {
	enum EDID_DEEPCOLOR_CODE dp_mode;
}HDMI_ioctl_edid_deep_color_t;

typedef struct {
	unsigned int av_blank_status;
}HDMI_ioctl_av_state_t;

typedef struct {
	unsigned int hdmi_mode;
}HDMI_ioctl_edid_hdmi_mode_t;

typedef struct {
	unsigned int av_mute;
}HDMI_ioctl_av_mute_t;

typedef struct {
	unsigned int transcoding_manual;
}HDMI_ioctl_transcoding_manual_t;

typedef struct HDMI_SHORT_AUDIO_DESCRIPTOR{
	unsigned short audio_format_code;
	unsigned char max_num_audio_channels;
	unsigned char audio_sampling_rate;
	unsigned short max_audio_bit_rate;
	struct HDMI_SHORT_AUDIO_DESCRIPTOR *next;
}HDMI_ioctl_short_audio_descriptor_t;

typedef struct HDMI_SHORT_VIDEO_DESCRIPTOR
{
	unsigned char native_indicator;
	unsigned char video_id_code;
	struct SHORT_VIDEO_DESCRIPTOR * next;
}HDMI_ioctl_short_video_descriptor_t;

typedef struct {
	unsigned char hdcp_cap;
}HDMI_ioctl_hdcp_cap_t;


#define HDMI_IOCINI_HDMIHW			_IOW(HDMI_IOC_MAGIC,	0,  char[286])
//#define HDMI_IOCT_NETPORT			_IO(HDMI_IOC_MAGIC,		1)
#define HDMI_IOCT_HDCPONOFF			_IO(HDMI_IOC_MAGIC,		2)
#define HDMI_IOCQ_MUTESTA			_IOR(HDMI_IOC_MAGIC,	3,	int)
#define HDMI_IOCQ_HDCPSTAT			_IOR(HDMI_IOC_MAGIC,	4,	int)
#define HDMI_IOC_SET8MA				_IO(HDMI_IOC_MAGIC,		5)
#define HDMI_IOC_SET85MA			_IO(HDMI_IOC_MAGIC,		6)
#define HDMI_IOC_SET9MA				_IO(HDMI_IOC_MAGIC,		7)
#define HDMI_IOC_SET95MA			_IO(HDMI_IOC_MAGIC,		8)
#define HDMI_IOC_SET10MA			_IO(HDMI_IOC_MAGIC,		9)
#define HDMI_IOC_SET105MA			_IO(HDMI_IOC_MAGIC,		10)
#define HDMI_IOC_SET11MA			_IO(HDMI_IOC_MAGIC,		11)
#define HDMI_IOC_SET115MA			_IO(HDMI_IOC_MAGIC,		12)
#define HDMI_IOC_SET12MA			_IO(HDMI_IOC_MAGIC,		13)
#define HDMI_IOC_SET125MA			_IO(HDMI_IOC_MAGIC,		14)
#define HDMI_IOC_SET13MA			_IO(HDMI_IOC_MAGIC,		15)
#define HDMI_IOC_SET135MA			_IO(HDMI_IOC_MAGIC,		16)
#define HDMI_IOC_SET14MA		 	_IO(HDMI_IOC_MAGIC,		17)
#define HDMI_IOC_SET145MA			_IO(HDMI_IOC_MAGIC,		18)
#define HDMI_IOC_SET15MA	 		_IO(HDMI_IOC_MAGIC,		19)
#define HDMI_IOC_SET155MA			_IO(HDMI_IOC_MAGIC,		20)
#define HDMI_IOC_SET16MA			_IO(HDMI_IOC_MAGIC,		21)
#define HDMI_IOCG_HDMIMODE			_IOR(HDMI_IOC_MAGIC,	22,	int)
#define HDMI_IOCG_HDMIAUDIO			_IOR(HDMI_IOC_MAGIC,	23,	short)
#define HDMI_IOCG_EDIDRDY			_IOR(HDMI_IOC_MAGIC,	24,	int)
#define HDMI_IOCG_NATIVERES			_IOR(HDMI_IOC_MAGIC,	25,	int)
#define HDMI_IOC_DEVCLOSE			_IO(HDMI_IOC_MAGIC,		26)

#define HDMI_IOCT_SET_ONOFF			_IOW(HDMI_IOC_MAGIC, 	27, HDMI_ioctl_sw_onoff_state_t *)
#define HDMI_IOCT_GET_ONOFF			_IOR(HDMI_IOC_MAGIC, 	28, HDMI_ioctl_sw_onoff_state_t *)
#define HDMI_IOCT_HDCP_GET_STATUS	_IO(HDMI_IOC_MAGIC, 	29)
#define HDMI_IOCT_HDCP_SET_ONOFF	_IOW(HDMI_IOC_MAGIC, 	30, HDMI_ioctl_hdcp_state_t *)
#define HDMI_IOCT_HDCP_GET_ONOFF	_IOR(HDMI_IOC_MAGIC, 	31, HDMI_ioctl_hdcp_state_t *)
#define HDMI_IOCT_HDCP_SET_KEY_INFO	_IOW(HDMI_IOC_MAGIC,	32,	HDMI_ioctl_hdcp_key_info_t *)
#define HDMI_IOCT_CEC_SET_ONOFF		_IOW(HDMI_IOC_MAGIC, 	33, HDMI_ioctl_cec_state_t *)
#define HDMI_IOCT_CEC_GET_ONOFF		_IOR(HDMI_IOC_MAGIC, 	34, HDMI_ioctl_cec_state_t *)
#define HDMI_IOCT_CEC_GET_PA		_IOR(HDMI_IOC_MAGIC, 	35, HDMI_ioctl_cec_addr_t *)
#define HDMI_IOCT_CEC_SET_LA		_IOW(HDMI_IOC_MAGIC, 	36, HDMI_ioctl_cec_addr_t *)
#define HDMI_IOCT_CEC_GET_LA		_IOR(HDMI_IOC_MAGIC, 	37, HDMI_ioctl_cec_addr_t *)
#define HDMI_IOCT_CEC_TRANSMIT		_IOW(HDMI_IOC_MAGIC, 	38, HDMI_ioctl_cec_msg_t *)
#define HDMI_IOCT_GET_VIDEO_FORMAT	_IOR(HDMI_IOC_MAGIC, 	39, HDMI_ioctl_video_format_t *)
#define HDMI_IOCT_GET_EDID_AUD_OUT	_IOR(HDMI_IOC_MAGIC, 	40, HDMI_ioctl_audio_out_t *)
#define HDMI_IOCT_SET_VID_RES		_IOW(HDMI_IOC_MAGIC, 	41, HDMI_ioctl_video_format_t *)
#define HDMI_IOCT_GET_VID_RES		_IOR(HDMI_IOC_MAGIC, 	42, HDMI_ioctl_video_format_t *)
#define HDMI_IOCG_GET_ALL_VID_RES	_IOR(HDMI_IOC_MAGIC,	43,	HDMI_ioctl_edid_res_list_t *)
#define HDMI_IOCT_SET_VENDOR_NAME	_IOW(HDMI_IOC_MAGIC,	44,	HDMI_ioctl_vendor_name_t *)
#define HDMI_IOCT_SET_PRODUCT_DESC	_IOW(HDMI_IOC_MAGIC,	45,	HDMI_ioctl_product_desc_t *)
#define HDMI_IOCT_GET_VENDOR_NAME	_IOR(HDMI_IOC_MAGIC,	46,	HDMI_ioctl_vendor_name_t *)
#define HDMI_IOCT_GET_PRODUCT_DESC	_IOR(HDMI_IOC_MAGIC,	47,	HDMI_ioctl_product_desc_t *)
#define HDMI_IOCT_GET_LINK_ST		_IOR(HDMI_IOC_MAGIC,	48,	HDMI_ioctl_link_status_t *)

#define HDMI_IOCT_HDCP_MEM_SEL		_IOW(HDMI_IOC_MAGIC, 	49, HDMI_ioctl_mem_sel_t *)  //add by ze, for ce_load_key
#define HDMI_IOCT_SET_HDMI_AUDIO_ONOFF  _IOW(HDMI_IOC_MAGIC, 	50, HDMI_ioctl_hdmi_audio_state_t *)
#define HDMI_IOCT_GET_HDMI_AUDIO_ONOFF _IOR(HDMI_IOC_MAGIC, 	51, HDMI_ioctl_hdmi_audio_state_t *)
#define HDMI_IOCT_GET_3D_PRESENT    _IOR(HDMI_IOC_MAGIC,	52,	HDMI_ioctl_3d_status_t *)
#define HDMI_IOCT_GET_EDID_MANUFACTURER _IOR(HDMI_IOC_MAGIC,    53, HDMI_ioctl_edid_vendor_t *)
#define HDMI_IOCT_GET_EDID_MONITOR  _IOR(HDMI_IOC_MAGIC,    54, HDMI_ioctl_edid_vendor_t *)
#define HDMI_IOCT_SET_DEEP_COLOR	_IOW(HDMI_IOC_MAGIC, 	55, HDMI_ioctl_deep_color_t *)
#define HDMI_IOCT_GET_DEEP_COLOR	_IOR(HDMI_IOC_MAGIC, 	56, HDMI_ioctl_deep_color_t *)
#define HDMI_IOCT_GET_EDID_BLOCK  _IOR(HDMI_IOC_MAGIC,    57, HDMI_ioctl_edid_block_data_t *)
#define HDMI_IOCT_SET_PHY_CLOCK  _IOW(HDMI_IOC_MAGIC,    58, HDMI_ioctl_phy_clock_t *)
#define HDMI_IOCT_SET_COLOR_SPACE  _IOW(HDMI_IOC_MAGIC,    59, HDMI_ioctl_color_space_t *)
#define HDMI_IOCT_GET_COLOR_SPACE  _IOR(HDMI_IOC_MAGIC,    60, HDMI_ioctl_color_space_t *)
#define HDMI_IOCT_GET_EDID_DEEP_COLOR	_IOR(HDMI_IOC_MAGIC,    61, HDMI_ioctl_edid_deep_color_t *)
#define HDMI_IOCT_SET_AV_BLANK		_IOW(HDMI_IOC_MAGIC,    62, HDMI_ioctl_av_state_t *)
#define HDMI_IOCT_GET_AV_BLANK		_IOR(HDMI_IOC_MAGIC,    63, HDMI_ioctl_av_state_t *)
#define HDMI_IOCT_GET_EDID_HDMI_MODE	_IOR(HDMI_IOC_MAGIC, 	64, HDMI_ioctl_edid_hdmi_mode_t *)
#define HDMI_IOCT_SET_AV_MUTE		_IOW(HDMI_IOC_MAGIC, 	65, HDMI_ioctl_av_mute_t *)
#define HDMI_IOCT_SET_TRANSCODING_MANUAL	_IOW(HDMI_IOC_MAGIC, 	66, HDMI_ioctl_transcoding_manual_t *)
#define HDMI_IOCT_GET_TRANSCODING_MANUAL	_IOR(HDMI_IOC_MAGIC, 	67, HDMI_ioctl_transcoding_manual_t *)
#define HDMI_IOCT_GET_EDID_LENGTH  _IOR(HDMI_IOC_MAGIC,    68, HDMI_ioctl_edid_block_data_t *)
#define HDMI_IOCT_GET_EDID  _IOR(HDMI_IOC_MAGIC,    69, HDMI_ioctl_edid_block_data_t *)
#define HDMI_IOCT_GET_KUMSGQ		_IOR(HDMI_IOC_MAGIC,	70, int)
#define HDMI_IOCG_GET_AUDIO_CEA			_IOR(HDMI_IOC_MAGIC, 	71,	HDMI_ioctl_short_audio_descriptor_t *)
#define HDMI_IOCG_GET_VIDEO_CEA			_IOR(HDMI_IOC_MAGIC, 	72,	HDMI_ioctl_short_video_descriptor_t *)
#define HDMI_IOCG_GET_AUDIO_CEA_NUM			_IOR(HDMI_IOC_MAGIC, 	73,	int)
#define HDMI_IOCG_GET_VIDEO_CEA_NUM			_IOR(HDMI_IOC_MAGIC, 	74,	int)
#define HDMI_IOCT_GET_HDCP_CAP	_IOR(HDMI_IOC_MAGIC, 	75, HDMI_ioctl_hdcp_cap_t *)
#endif

BOOL ali_hdmi_get_hdcp_onoff(void);
unsigned int api_get_hdmi_state(void);
BOOL ali_hdmi_audio_get_onoff(void);
void ali_hdmi_audio_switch_onoff(BOOL turn_on);
