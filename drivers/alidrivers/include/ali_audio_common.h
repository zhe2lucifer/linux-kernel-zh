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

#ifndef __ALI_AUDIO_COMMON_H
#define __ALI_AUDIO_COMMON_H



#include "ali_basic_common.h"
#include "./alidefinition/adf_basic.h"
#include "./alidefinition/adf_media.h"
#include "./alidefinition/adf_hld_dev.h"            /** Defintions Used by both kernel and see snd **/

#include "./alidefinition/adf_deca.h"           /** Definitions Used by both kernel and see deca **/
#include "./alidefinition/adf_snd.h"            /** Defintions Used by both kernel and see snd **/
#include <linux/dvb/audio.h>                    /** Definitions contained in standard linux kernel used by app and ali driver **/

////////////////////////////////New re-ordered definition //////////////////

/*! @struct ali_audio_ioctl_command
@brief A structure defines IO command for sub command processing.
*/
struct ali_audio_ioctl_command
{
	unsigned long ioctl_cmd; //!< Sub IO command
	unsigned long param; //!< Sub IO parameter
};

/*! @struct ali_audio_rpc_arg
@brief A structure defines the audio RPC transfer argument.
*/
struct ali_audio_rpc_arg
{
	void *arg; //!< Argument pointer.
	int arg_size; //!< Argument size.
	int out; //!< Data transfer direction, 1: two ways (in and out), 0: set in.
};

#define MAX_AUDIO_RPC_ARG_NUM		8 //!< The maximum number for arguments to set.
#define MAX_AUDIO_RPC_ARG_SIZE		2048 //!< The maximum data size for arguments to set.

#define ADEC_PLUGIN_NO_EXIST            4 //!< Not used any more.
#define MSG_FIRST_FRAME_OUTPUT          5 //!<Message of first frame outputed
#define MSG_DECA_MONITOR_NEW_FRAME      6 //!<Message will be sent when one frame was decoded by audio decoder.
#define MSG_DECA_MONITOR_START          7 //!<Message will be sent when audio decoder is in start status.
#define MSG_DECA_MONITOR_STOP           8 //!<Message will be sent when audio decoder is in stop status.
#define MSG_DECA_MONITOR_DECODE_ERR     9 //!<Message will be sent when audio decoder return one err status.
#define MSG_DECA_MONITOR_OTHER_ERR      10 //!<Message will be sent when audio decoder occurred other err.
#define MSG_DECA_STATE_CHANGED          11 //!<Message will be sent when audio decoder state is changed
#define MSG_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD      12 //!<Message will be sent when  sound card dma data is below the threshold
#define MSG_SND_MONITOR_OUTPUT_DATA_END                  13 //!<Message will be sent when sound card dma data is occured the end
#define MSG_SND_MONITOR_ERRORS_OCCURED                   14 //!<Message will be sent when sound card is occured some errors
#define MSG_SND_MONITOR_SBM_MIX_END                      15 //!<Message will be sent when SBM mix end

/*! @struct ali_audio_rpc_pars
@brief A structure defines multiple arguments.
*/
struct ali_audio_rpc_pars
{
	int API_ID; //!< API ID.
	struct ali_audio_rpc_arg arg[MAX_AUDIO_RPC_ARG_NUM]; //!< Argument array.
	int arg_num; //!< Argument number.
};

typedef struct control_block ali_audio_ctrl_blk;

#define AUDIO_SET_VOLUME                                _IOW('o', 21, unsigned char) //!< Set audio volume
#define AUDIO_GET_INPUT_CALLBACK_ROUTINE                _IOR('o', 22, struct ali_dec_input_callback_routine_pars) //!< Not used any more
#define AUDIO_DECA_IO_COMMAND                           _IOWR('o', 23, struct ali_audio_ioctl_command) //!< Audio sub IO command for decoder
#define AUDIO_SND_IO_COMMAND                            _IOWR('o', 24, struct ali_audio_ioctl_command) //!< Audio sub IO command for sound output
#define AUDIO_DECORE_COMMAND                            _IOWR('o', 25, struct ali_audio_rpc_pars) //!< Audio sub IO command for media player
#define AUDIO_GET_VOLUME                                _IOR('o', 26, unsigned char) //!< Get audio volume

#define AUDIO_ASE_INIT                                  _IO('o', 27) //!< Initialize audio special effect function
#define AUDIO_ASE_STR_STOP                              _IO('o', 28) //!< Stop playing by using audio special effect function
#define AUDIO_ASE_DECA_BEEP_INTERVAL                    _IOW('o', 29,unsigned int) //!< Set beep interval for loop
#define AUDIO_ASE_STR_PLAY                              _IOW('o', 30,struct ase_str_play_param) //!< Start playing by using audio special effect function

#define AUDIO_GEN_TONE_VOICE                            _IOW('o', 31, unsigned long)  //!< Start to generate tone volume
#define AUDIO_STOP_TONE_VOICE                           _IO('o', 32) //!< Stop generating tone volume
#define AUDIO_SND_ENABLE_EQ                             _IOW('o',33,struct ali_audio_rpc_pars) //!< Not used any more

#define AUDIO_EMPTY_BS_SET                              _IO('o', 34) //!< Clear bit stream mode flag
#define AUDIO_ADD_BS_SET                                _IO('o', 35) //!< Add an audio stream type to support bit stream mode
#define AUDIO_DEL_BS_SET                                _IO('o', 36) //!< Delete an audio stream type to support bit stream mode
#define AUDIO_SND_START                                 _IO('o', 37) //!< Start sound output
#define AUDIO_SND_STOP                                  _IO('o', 38) //!< Stop sound output
#define AUDIO_SND_STC_INVALID                           _IO('o', 39) //!< Set STC (System timer counter) to invalid status by sound hardware
#define AUDIO_SND_STC_VALID                             _IO('o', 40) //!< Set STC (System timer counter) to valid status by sound hardware
#define AUDIO_RPC_CALL_ADV                              _IO('o', 41) //!< IO command for multiple commands and parameters usage
#define AUDIO_SND_GET_STC                               _IO('o', 42) //!< Set an initial value for sound hardware STC to start counting, must be used by #AUDIO_RPC_CALL_ADV
#define AUDIO_SND_SET_STC                               _IO('o', 43) //!< Get the current sound hardware STC value, must be used by #AUDIO_RPC_CALL_ADV
#define AUDIO_SND_PAUSE_STC                             _IO('o', 44) //!< Set the sound hardware STC to stop counting, must be used by #AUDIO_RPC_CALL_ADV
#define AUDIO_SND_SET_SUB_BLK                           _IO('o', 45) //!< Enable an output interface (CVBS/SPDIF/HDMI), must be used by #AUDIO_RPC_CALL_ADV
#define AUDIO_DECA_IO_COMMAND_ADV                       _IOWR('o', 46, struct ali_audio_rpc_pars) //!< Decoder IO command for multiple commands and parameters usage, must be used by #AUDIO_RPC_CALL_ADV, the same as #AUDIO_DECA_IO_COMMAND
#define AUDIO_SND_IO_COMMAND_ADV                        _IOWR('o', 47, struct ali_audio_rpc_pars) //!< Sound output IO command for multiple commands and parameters usage, must be used by #AUDIO_RPC_CALL_ADV, the same as #AUDIO_SND_IO_COMMAND
#define AUDIO_DECA_PROCESS_PCM_SAMPLES                  _IO('o', 48) //!< Write a PCM sample to output, must be used by #AUDIO_RPC_CALL_ADV
#define AUDIO_DECA_PROCESS_PCM_BITSTREAM                _IO('o', 49) //!< Write a PCM sample or bit stream to output, must be used by #AUDIO_RPC_CALL_ADV
#define RPC_AUDIO_DECORE_IOCTL                          _IO('o', 50) //!< Media player IO command for multiple commands and parameters, must be used by #AUDIO_RPC_CALL_ADV, the same as #AUDIO_DECORE_COMMAND.

#define AUDIO_DECA_SET_DBG_LEVEL                        _IO('o', 51) //!< Set decoder debug log output level
#define AUDIO_SND_SET_DBG_LEVEL                         _IO('o', 52) //!< Set sound output debug log output level

#define AUDIO_SET_CTRL_BLK_INFO                         _IOW('o',53,ali_audio_ctrl_blk) //!< Update the length of live data

#define AUDIO_DECA_IS_BS_MEMBER_CHECK                   _IOR('O',54,unsigned char) //!< Not used any more
#define AUDIO_DECA_HDD_PLAYBACK                         _IOW('O',55,unsigned char) //!< Not used any more. Please use #DECA_HDD_PLAYBACK by #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define AUDIO_DECA_PLAY_SPEED_SET                       _IOW('O',56,enum ADecPlaySpeed) //!< Not used any more
#define AUDIO_DECA_DESC_ENABLE                          _IOW('O',57,unsigned char) //!< Not used any more
#define AUDIO_DECA_AC3_COMP_MODE_SET                    _IOW('O',58,enum DECA_AC3_COMP_MODE) //!< Not used any more
#define AUDIO_DECA_AC3_STEREO_MODE_SET                  _IOW('O',59,enum DECA_AC3_STEREO_MODE) //!< Not used any more
#define AUDIO_DECA_STATE_GET                            _IOR('O',60,unsigned char) //!< Not used any more
#define AUDIO_DECA_DDP_INMOD_GET                        _IOR('O',61,unsigned char) //!< Not used any more
#define AUDIO_DECA_FRM_INFO_GET                         _IOR('O',62,struct cur_stream_info) //!< Not used any more


#define AUDIO_SND_FORCE_SPDIF_TYPE                      _IOR('O',63,enum ASndOutSpdifType) //!< Not used any more. Please use #FORCE_SPDIF_TYPE by AUDIO_SND_IO_COMMAND or (#AUDIO_RPC_CALL_ADV or #AUDIO_SND_IO_COMMAND_ADV).
#define AUDIO_SND_I2S_OUT                               _IOW('O',64,unsigned char) //!< Not used any more. Please use #SND_I2S_OUT by #AUDIO_SND_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_SND_IO_COMMAND_ADV).
#define AUDIO_SND_SPDIF_OUT                             _IOW('O',65,unsigned char) //!< Not used any more. Please use #SND_SPDIF_OUT by AUDIO_SND_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_SND_IO_COMMAND_ADV).
#define AUDIO_SND_HDMI_OUT                              _IOW('O',66,unsigned char) //!< Not used any more. Please use #SND_HDMI_OUT by AUDIO_SND_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_SND_IO_COMMAND_ADV).
#define AUDIO_SND_DESC_VOLUME_OFFSET_SET                _IOW('O',67,long) //!< Not used any more
#define AUDIO_SND_DESC_ENABLE                           _IOW('O',68,unsigned char) //!< Not used any more
#define AUDIO_SND_SYNC_DELAY_SET                        _IOW('O',69,long) //!< Not used any more
#define AUDIO_SND_SYNC_LEVEL_SET                        _IOW('O',70,unsigned char) //!< Not used any more
#define AUDIO_SND_SET_MIX_INFO                          _IOW('O',71,struct snd_mix_info) //!< set snd mix info
#define AUDIO_SND_SET_MIX_END                           _IOW('O',72,unsigned long) //!< set snd mix end
#define AUDIO_SND_GET_MIX_STATE                         _IOR('O',73,unsigned long) //!< get snd mix state
#define AUDIO_GET_UNDERRUN_TIMES                        _IOR('o', 74, unsigned int) //!< Get audio underrun times


#define AUDIO_DECA_DECORE_INIT                          _IOW('O',301,struct audio_config) //!< Not used any more. Please use #DECA_DECORE_INIT by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_RLS                           _IO('O',302) //!< Not used any more. Please use DECA_DECORE_RLS by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_SET_BASE_TIME                 _IOW('O',303,unsigned long) //!< Not used any more. Please use #DECA_DECORE_SET_BASE_TIME by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_GET_PCM_TRD                   _IOR('O',304,unsigned long) //!< Not used any more. Please use #DECA_DECORE_GET_PCM_TRD by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_PAUSE_DECODE                  _IO('O',305) //!< Not used any more. Please use #DECA_DECORE_PAUSE_DECODE by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_FLUSH                         _IO('O',306) //!< Not used any more. Please use #DECA_DECORE_FLUSH by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_SET_QUICK_MODE                _IOW('O',307,unsigned char) //!< Not used any more. Please use #DECA_DECORE_SET_QUICK_MODE by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_SET_SYNC_MODE                 _IOW('O',308,enum AUDIO_AV_SYNC_MODE) //!< Not used any more. Please use #DECA_DECORE_SET_SYNC_MODE by AUDIO_DECORE_COMMAND or (AUDIO_RPC_CALL_ADV and RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_GET_CUR_TIME                  _IOR('O',309,unsigned long) //!< Not used any more. Please use #DECA_DECORE_GET_CUR_TIME by AUDIO_DECORE_COMMAND or (AUDIO_RPC_CALL_ADV and RPC_AUDIO_DECORE_IOCTL).
#define AUDIO_DECA_DECORE_GET_STATUS                    _IOR('O',310,struct audio_decore_status) //!< Not used any more. Please use #DECA_DECORE_GET_STATUS by #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL).

//#define AUDIO_SET_SOCKET_PORT_ID                        _IOR('O',311, int) //!<Set the socket port id.
#define AUDIO_CB_RPC_REGISTER                           _IOR('O',312, struct audio_callback_register_param)//!<Register audio cb
#define AUDIO_CB_RPC_UNREGISTER                         _IOR('O',313, struct audio_callback_register_param)//!<Unregister audio cb
#define AUDIO_INIT_TONE_VOICE                           _IO('O',314) //!< Init tone voice, init adec and snd tone voice.
#define AUDIO_GET_KUMSGQ	    					 	_IOR('O',315, int)

#endif
