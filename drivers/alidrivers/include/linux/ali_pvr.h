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
 
#ifndef __ALI_PVR_H__
#define __ALI_PVR_H__

#include <alidefinition/adf_pvr.h>
#include <alidefinition/adf_dsc.h>

#define PVR_NUM                    1

struct ali_pvr_reencrypt
{
    unsigned short      pid_num;                   //[input] the re-encryption pid number
    unsigned short      pid_list[32];                   //[iutput] the pid count
    unsigned int        source_mode;
    unsigned int        dsc_fd;
};

struct pvr_ca_attr {
	void *sub_dev_see_hdl;
		/*!< Pointer of the sub device handler at see for the current session. */
	unsigned short stream_id;
		/*!< Stream id of the current session. */
	int crypt_mode;
		/*!< Crypt mode of the current session. */        
    unsigned short sub_dev_id;
        /*sub dev id */
};


struct ali_pvr_data_decrypt_param
{
    unsigned int        decrypt_hdl;                //[input] the get decrypt handle
	UINT8               *input;                     //[input] input data block 
	UINT32              length;                     //[input] input data block lenth  
    UINT32              block_index;                  //[input] index of data block 0 ~ 0xFFFFFFFF-1 and 188byte integral multiple
    UINT32              block_vob_indicator;          //[input] block vob attribute:0:block normal,1: block start
                                                      // 2: block end
	enum pvr_ott_data_type	des_flag; /*flag: PVR_OTT_DATA_DMX -> DMX, PVR_OTT_DATA_VBV -> VBV, PVR_OTT_DATA_AUDIO -> AUDIO*/
	UINT32 iv_length;/*Initial vector length,if = 0,needn't update iv,or need update iv to dsc driver*/ 
	UINT32 *iv_data;  /*Initial vector*/
	UINT32 decv_id;	/*it is set to 0 under default,if pip is used,it is set in practice*/
};

struct ali_pvr_capt_decrypt_res_param
{
    unsigned int      decrypt_hdl;                  //[output] the get decrypt handle
    unsigned int      decrypt_dsc_num;              //[input] decrypt dsc num; 
    unsigned int      block_data_size;              //[input] block data size,188byte integral multiple; 
};

enum block_data_mode
{    
    BLOCK_DATA_MODE_PURE_DATA = 0,
    BLOCK_DATA_MODE_TS = 1,
};

enum decrypt_res_iv_param
{    
    KEY_IV_MODE_EVEN = 0,
    KEY_IV_MODE_ODD  = 1,
};


struct ali_pvr_set_decrypt_res_param
{
    unsigned int                decrypt_hdl;                  //[input] the get decrypt handle
    unsigned int                decrypt_index;                //[input] descrypt_index , form  0  to  decrypt_dsc_num - 1
    enum block_data_mode        dsc_mode;                     //[input] pure data mode and ts mode 
    unsigned int                decrypt_switch_block;         //[input] switch block 1 ~ 0xFFFFFFFF
    unsigned int                block_data_size;              //[input] every block size  
    enum decrypt_res_iv_param   iv_parity;                    //[input] iv parity
    unsigned int                iv_lenth;                     //[input] iv length
    unsigned char               *iv_data;                     //[input] iv data  
    unsigned int                key_handle;                   //[input] main dsc key handle 
    unsigned int                dsc_fd;                       //[input] dsc fd;     
};


struct ali_pvr_set_pvr_rec_key
{
    unsigned int                stream_id;                  //[input] the stream id
    unsigned int                key_item_len;               //[input] the stream id
    unsigned int                key_item_num;               //[input] the stream id
    unsigned char               *input_key;                 //[input] the stream id
    unsigned int                block_data_size;            //[input] ts 188 integer multiple
    unsigned int                qn_per_key;					//[input] the stream id	
};

struct ali_pvr_set_pvr_plyback_key
{
    unsigned int                dsc_fd;                    //[input]dsc fd
    unsigned int                kl_fd;                     //[input]kl fd
    struct PVR_BLOCK_ENC_PARAM  pvr_key_param;             //[input/ouput],pvr key parameter 
};


#define PVR_MAGIC                  'p'
#define PVR_IO_DECRYPT                  _IOW(PVR_MAGIC, 0, struct PVR_RPC_RAW_DECRYPT)
#define PVR_IO_UPDATE_ENC_PARAMTOR      _IOW(PVR_MAGIC, 1, struct PVR_BLOCK_ENC_PARAM)
#define PVR_IO_SET_BLOCK_SIZE           _IOW(PVR_MAGIC, 2, UINT32)
#define PVR_IO_FREE_BLOCK               _IOW(PVR_MAGIC, 3, UINT32)
#define PVR_IO_START_BLOCK              _IOW(PVR_MAGIC, 4, struct PVR_BLOCK_ENC_PARAM)
#define PVR_IO_START_REENCRYPT          _IOW(PVR_MAGIC, 5, struct ali_pvr_reencrypt)
#define PVR_IO_STOP_REENCRYPT           _IOW(PVR_MAGIC, 6, UINT32)

#define PVR_IO_UPDATE_ENC_PARAMTOR_EVO  _IOW(PVR_MAGIC, 7, struct PVR_BLOCK_ENC_PARAM)
#define PVR_IO_FREE_BLOCK_EVO           _IOW(PVR_MAGIC, 8, UINT32)
#define PVR_IO_START_BLOCK_EVO          _IOW(PVR_MAGIC, 9, struct PVR_BLOCK_ENC_PARAM)

#define PVR_IO_CAPTURE_DECRYPT_RES              _IOWR(PVR_MAGIC,10, struct ali_pvr_capt_decrypt_res_param)
#define PVR_IO_SET_DECRYPT_RES                  _IOW(PVR_MAGIC,11, struct ali_pvr_set_decrypt_res_param)
#define PVR_IO_RELEASE_DECRYPT_RES              _IOW(PVR_MAGIC,12, UINT32)
#define PVR_IO_DECRYPT_EVO                  _IOW(PVR_MAGIC,13, struct ali_pvr_data_decrypt_param)
#define PVR_IO_DECRYPT_ES_EVO                  _IOW(PVR_MAGIC,14, struct ali_pvr_data_decrypt_param)
#define PVR_IO_CAPTURE_PVR_KEY          _IOW(PVR_MAGIC,15, struct ali_pvr_set_pvr_rec_key)   //for c0100 AS
#define PVR_IO_RELEASE_PVR_KEY          _IOW(PVR_MAGIC,16, UINT32)                   //for c0100 AS
#define PVR_IO_SET_PLAYBACK_CA_PRAM     _IOW(PVR_MAGIC,17, struct ali_pvr_set_pvr_plyback_key) //for c0100 AS
#define PVR_IO_DECRYPT_EVO_SUB     _IOW(PVR_MAGIC,18, struct ali_pvr_data_decrypt_param) //for c0100 AS

int ali_pvr_block_de_encrypt(DEEN_CONFIG *p_DeEn,UINT8 *input,UINT8 *output, UINT32 total_length, PVR_REC_VIDEO_PARAM *video_param);
int ali_pvr_ts_de_encrypt(DEEN_CONFIG *p_deen,UINT8 *input,UINT8 *output, UINT32 total_length, PVR_REC_VIDEO_PARAM *video_param);
int pvr_get_session_attr(int pvr_fd, struct pvr_ca_attr * p_ca_attr);
int pvr_check_is_pvr_fd(int pvr_fd);



#endif

