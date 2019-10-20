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

/****************************************************************************(I)(S)
 *  File: ali_rpc_hld_decv.c
 *  (I)
 *  Description: hld decv remote process call api
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2010.04.08			Sam			Create
 ****************************************************************************/
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
//#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/ali_rpc.h>

//#include "ali_rpc_hld.h"
#include <rpc_hld/ali_rpc_hld_snd.h>

#include <alidefinition/adf_snd.h>

#include "../ali_rpc.h"

//////////////////////////New re-oredered code///////////////////

/** Have checked with rpcng, No need change **/
RET_CODE get_stc(UINT32 *stc_msb32, UINT8 stc_num)
{
    UINT32 desc[] =
    { //desc of pointer para
      1, DESC_OUTPUT_STRU(0, 4),
      1, DESC_P_PARA(0, 0, 0),
      //desc of pointer ret
      0,
      0,
    };
    jump_to_func(NULL, ali_rpc_call, stc_msb32, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_GET_STC, desc);
}

/** Have checked with rpcng, No need change **/
void set_stc(UINT32 stc_msb32, UINT8 stc_num)
{
    jump_to_func(NULL, ali_rpc_call, stc_msb32, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_SET_STC, NULL);
}

/** Have checked with rpcng, No need change **/
void get_stc_divisor(UINT16 * stc_divisor, UINT8 stc_num)
{
    UINT32 desc[] =
    { //desc of pointer para
      1, DESC_OUTPUT_STRU(0, 4),
      1, DESC_P_PARA(0, 0, 0),
      //desc of pointer ret
      0,
      0,
    };
    jump_to_func(NULL, ali_rpc_call, stc_divisor, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_GET_STC_DIVISOR, desc);
}

/** Have checked with rpcng, No need change **/
void set_stc_divisor(UINT16 stc_divisor, UINT8 stc_num)
{
    jump_to_func(NULL, ali_rpc_call, stc_divisor, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_SET_STC_DIVISOR, NULL);
}

/** Have checked with rpcng, No need change **/
void stc_pause(UINT8 pause, UINT8 stc_num)
{
	jump_to_func(NULL, ali_rpc_call, null, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_STC_PAUSE, NULL);
}

/** Have checked with rpcng, No need change **/
void stc_invalid(void)
{
	jump_to_func(NULL, ali_rpc_call, null, (HLD_SND_MODULE<<24)|(0<<16)|FUNC_STC_INVALID, NULL);
}

/** Have checked with rpcng, No need change **/
void stc_valid(void)
{
    jump_to_func(NULL, ali_rpc_call, null, (HLD_SND_MODULE<<24)|(0<<16)|FUNC_STC_VALID, NULL);
}

/** Have checked with rpcng, No need change **/
RET_CODE snd_open(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_OPEN, NULL);
}

/** Have checkedd with rpcng, No need change **/
RET_CODE snd_close(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_CLOSE, NULL);
}

/** Have checked with rpcng, No need change **/
RET_CODE snd_set_mute(struct snd_device *dev, enum snd_sub_block sub_blk, UINT8 enable)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_SET_MUTE, NULL);
}

/** Have checked with rpcng, No need change **/
RET_CODE snd_set_volume(struct snd_device *dev, enum snd_sub_block sub_blk, UINT8 volume)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_SET_VOLUME, NULL);
}

/** Have checked with rpcng, No need change **/
UINT8 snd_get_volume(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_GET_VOLUME, NULL);
}

/** Have checked with rpcng, No need change **/
UINT32 snd_get_underrun_times(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_GET_UNDERRUN_TIMES, NULL);
}

/** merge and added **/
RET_CODE snd_set_sub_blk(struct snd_device *dev, UINT8 sub_blk, UINT8 enable)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_SET_SUB_BLK, NULL);
}

/** Have checked with rpcng, No need change **/
RET_CODE snd_set_spdif_type(struct snd_device *dev, enum asnd_out_spdif_type type)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_SND_SET_SPDIF_TYPE, NULL);
}

RET_CODE snd_set_duplicate(struct snd_device *dev, enum snd_dup_channel type)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_SND_SET_DUPLICATE, NULL);
}

/** Have checked with rpcng, No need change **/
void snd_start(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_START, NULL);
}
/** Have checked with rpcng, No need change **/
void snd_stop(struct snd_device*dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_STOP, NULL);
}

/** Only alirpc **/
RET_CODE snd_pause(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_PAUSE, NULL);
}

/** Only alirpc **/
RET_CODE snd_resume(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_RESUME, NULL);
}

static UINT32 gen_tone_voice_desc[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(struct pcm_output)),
  1, DESC_P_PARA(0, 1, 0),
  //desc of pointer ret
  0,
  0,
};

/** Have checked with rpcng,No need change **/
void snd_gen_tone_voice(struct snd_device * dev, struct pcm_output*pcm, UINT8 init)  //tone voice
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_GEN_TONE_VOICE, gen_tone_voice_desc);
}

/** Have checked with rpcng,No need change **/
void snd_stop_tone_voice(struct snd_device * dev)  //tone voice
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(1<<16)|FUNC_SND_STOP_TONE_VOICE, NULL);
}

/** Have checked with rpcng,No need change **/
RET_CODE snd_set_dbg_level(struct snd_device *dev, UINT32 option)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_SND_SET_DBG_LEVEL, NULL);
}

static struct snd_callback g_snd_cb;

static void snd_hdmi_cb(UINT32 type, UINT32 uParam)
{
	if(g_snd_cb.phdmi_snd_cb)
	{
		g_snd_cb.phdmi_snd_cb(uParam);
	}
}

void snd_register_cb_routine(void)
{
	ali_rpc_register_callback(ALI_RPC_CB_SND_HDMI, snd_hdmi_cb);
}
EXPORT_SYMBOL(snd_register_cb_routine);


static UINT32 desc_snd_get_status[] =
{ //desc of pointer para
  1, DESC_OUTPUT_STRU(0, sizeof(struct snd_dev_status)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 req_rem_desc[] =
{ //desc of pointer para
  1, DESC_OUTPUT_STRU(0, 4),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_spec_param[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(spec_param)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_spec_step_table[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(spec_step_table)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_sync_param_desc[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(snd_sync_param)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_spdif_scms_desc[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(struct snd_spdif_scms)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_io_reg_call_back_desc[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(struct audio_io_reg_callback_para)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_io_dbg_info[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(struct snd_dbg_info)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 snd_set_mix_info[] =
{
  //desc of pointer para
  1, DESC_OUTPUT_STRU(0, sizeof(struct snd_mix_info)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

static UINT32 desc_snd_get_pts[] =
{ //desc of pointer para
  1, DESC_OUTPUT_STRU(0, sizeof(struct snd_get_pts_param)),
  1, DESC_P_PARA(0, 2, 0),
  //desc of pointer ret
  0,
  0,
};

RET_CODE snd_io_control(struct snd_device *dev, UINT32 cmd, UINT32 param)
{
    switch(cmd)
    {
        case IS_SND_RUNNING:
        case IS_SND_MUTE:
        case SND_CC_MUTE:
        case SND_DAC_MUTE:
        case SND_CC_MUTE_RESUME:
        case SND_SPO_ONOFF:
        case SND_SET_FADE_SPEED:
        case IS_PCM_EMPTY:
        case SND_BYPASS_VCR:
        case FORCE_SPDIF_TYPE:
        case SND_CHK_SPDIF_TYPE:
        case SND_BASS_TYPE:
        case SND_PAUSE_MUTE:
        case SND_SET_DESC_VOLUME_OFFSET:
        case SND_SET_DESC_VOLUME_OFFSET_NEW:
        case SND_SET_BS_OUTPUT_SRC:
        case SND_SECOND_DECA_ENABLE:
        case SND_DO_DDP_CERTIFICATION:
        case SND_POST_PROCESS_0:
        case SND_SPECIAL_MUTE_REG:
        case SND_SET_SYNC_DELAY:
        case SND_SET_SYNC_LEVEL:
        case SND_SET_MUTE_TH:
        case SND_AUTO_RESUME:
        case SND_I2S_OUT:
        case SND_SPDIF_OUT:
        case SND_HDMI_OUT:
        case SND_IO_PAUSE_SND:                  // Merge ok -->need to implement
        case SND_IO_RESUME_SND:
        case SND_IO_SPO_INTF_CFG:
        case SND_IO_DDP_SPO_INTF_CFG:
        case SND_ONLY_SET_SPDIF_DELAY_TIME:
        case SND_SET_AD_DYNAMIC_EN:
        case SND_IO_SET_MIX_END:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, NULL);
            break;
        }
        case SND_REG_HDMI_CB:
        {
            g_snd_cb.phdmi_snd_cb = (OSAL_T_HSR_PROC_FUNC_PTR)(param);
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, NULL);
            break;
        }
        case SND_GET_STATUS:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, desc_snd_get_status);
            break;
        }
        case SND_CHK_DAC_PREC:
        case SND_GET_RAW_PTS:
        case SND_REQ_REM_DATA:
        case SND_GET_TONE_STATUS:
        case SND_CHK_PCM_BUF_DEPTH:
        case SND_GET_SAMPLES_REMAIN:
        case SND_REQ_REM_PCM_DATA:
        case SND_REQ_REM_PCM_DURA:
        case SND_GET_SPDIF_TYPE:
        case SND_GET_MUTE_TH:
        case SND_ONLY_GET_SPDIF_DELAY_TIME:
        case SND_IO_SPO_INTF_CFG_GET:
        case SND_IO_DDP_SPO_INTF_CFG_GET:
        case SND_GET_AD_DYNAMIC_EN:
        case SND_IO_GET_MIX_STATE:
        case SND_IO_GET_MUTE_STATE:
        case SND_IO_GET_CHAN_STATE:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, req_rem_desc);
            break;
        }
        case SND_IO_GET_PLAY_PTS:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, desc_snd_get_pts);
            break;
        }
        case SND_SPECTRUM_START:
        {
            g_snd_cb.spec_call_back = ((spec_param*)param)->spec_call_back;
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_spec_param);
            break;
        }
		case SND_SPECTRUM_STEP_TABLE:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_spec_step_table);
            break;
		}
        case SND_SPECTRUM_STOP:
        case SND_SPECTRUM_CLEAR:
        case SND_SPECTRUM_VOL_INDEPEND:
        case SND_SPECTRUM_CAL_COUNTER:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, NULL);
            break;
        }
        case SND_SET_SPDIF_SCMS:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_spdif_scms_desc);
            break;
        }
        case SND_SET_SYNC_PARAM:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_sync_param_desc);
            break;
        }
        case SND_IO_REG_CALLBACK:       // Need to be implemented
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_io_reg_call_back_desc);
            break;
        }
        case SND_GET_DBG_INFO:
        {
            jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_io_dbg_info);
            break;
        }
        case SND_IO_SET_MIX_INFO:
        {
            jump_to_func(NULL, ali_rpc_call, dev, \
                (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_IO_CONTROL, snd_set_mix_info);
            break;
        }
        default:
            break;

    }
	return SUCCESS;
}

static UINT32 desc_snd_m36_attach[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(struct snd_feature_config)),
  1, DESC_P_PARA(0, 0, 0),
  //desc of pointer ret
  0,
  0,
};

/** Have checked with rpcng ,No need change **/
void snd_m36_attach(struct snd_feature_config * config)
{
    jump_to_func(NULL, ali_rpc_call, config, (LLD_SND_M36F_MODULE<<24)|(1<<16)|FUNC_SND_M36G_ATTACH, desc_snd_m36_attach);
}

/** Have checked with rpcng, No need change **/
void snd_m36_init_tone_voice(struct snd_device *dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (LLD_SND_M36F_MODULE<<24)|(1<<16)|FUNC_SND_M36G_INIT_TONE_VOICE, NULL);
}

/** Have checked with rpcng, No need change **/
void snd_init_spectrum(struct snd_device * dev)
{
    jump_to_func(NULL, ali_rpc_call, dev, (LLD_SND_M36F_MODULE<<24)|(1<<16)|FUNC_SND_M36G_INIT_SPECTRUM, NULL);
}

/** Have checked with rpcng, No need change **/
RET_CODE snd_set_pcm_capture_buff_info(struct snd_device *dev, UINT32 info, UINT8 flag)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(3<<16)|FUNC_SND_SET_PCM_CAPTURE_BUFF_INFO, NULL);
}

static UINT32 get_pcm_capture_buff_info_desc[] =
{ //desc of pointer para
  1, DESC_OUTPUT_STRU(0, sizeof(struct pcm_capture_buff)),
  1, DESC_P_PARA(0, 1, 0),
  //desc of pointer ret
  0,
  0,
};

/** Have checked with rpcng, No need change **/
RET_CODE snd_get_pcm_capture_buff_info(struct snd_device *dev, struct pcm_capture_buff *info)
{
    jump_to_func(NULL, ali_rpc_call, dev, (HLD_SND_MODULE<<24)|(2<<16)|FUNC_SND_GET_PCM_CAPTURE_BUFF_INFO, get_pcm_capture_buff_info_desc);
}
