#ifndef __ADF_SND_H
#define __ADF_SND_H

#include "adf_hld_dev.h"
/////////////////////New re-ordered snd used defination/////////////

///////Only tds used defination start--->//////
#define STC_DELAY       0x400       //!< STC delay threshold.

#define SND_STATE_DETACH   0
#define SND_STATE_ATTACH   1
#define SND_STATE_IDLE      2
#define SND_STATE_PLAY       4
#define SND_STATE_PAUSE       8

#define SND_SUB_STATE_BUSY       1
#define SND_SUB_STATE_NO_DATA    2
#define SND_SUB_STATE_NO_BUFF     4

#define SND_SUPPORT_AD      0x01
#define SND_SUPPORT_DDPLUS  0x02

enum av_sync_accuracy_level
{
	COMMON_ACCURACY = 0,
	DEFAULT_HIGH_ACCURACY,
	CONFIG_HIGH_ACCURACY,
};

enum snd_sync_mode
{
    SND_SYNC_MODE_PCR = 0,
    SND_SYNC_MODE_AUDIO,
    SND_SYNC_MODE_VIDEO,
    SND_SYNC_MODE_FREERUN
};

struct audio_avsync_param
{
	enum av_sync_accuracy_level av_sync_level;
	enum snd_sync_mode aud_sync_mode;	//Read/Write. SND_SYNC_MODE_PTS/SND_SYNC_MODE_FREERUN
	UINT32 drop_threshold;		    //Read/Write APP should not modify the value
	UINT32 wait_threshold;		    //Read/Write APP should not modify the value
	UINT32 hold_threshold;		    //Read/Write APP should not modify the value
	UINT32 dma_remain_threshold;	//Read/Write APP should not modify the value
	INT32 aud_pts_stc_diff; 	    //Only Read APP can not modify this value Unit:1/45ms
	UINT32 aud_pts_stc_diff_valid;	//Indicate the value of aud_pts_stc_diff whether is valid, 1=vallid, 0=invalid
	INT32 aud_stc_offset;		    //Read/Write. Unit: ms. 1. stc_offset>0, Audio will output early; 2.stc_offset<0, Audio will delay output. the value belong to -500~+1000.
};

struct snd_spdif_scms
{
    UINT8 copyright;      //!< Stream copyright.
    UINT8 reserved;       //!< Reserved bits.
    UINT8 l_bit;          //!< Linear PCM smaples flag or Non-PCM.
    UINT8 category_code;  //!< Category code.
    UINT16 reserved16;      //!< Reserved bits.
};

enum snd_tone_status
{
    SND_STREAM_STATUS = 0, //!< None.
    SND_TONE_STATUS,       //!< Left channel.
};

enum snd_channel
{
    SND_CH_NONE = 0,    //!< None.
    SND_CH_L,           //!< Left channel.
    SND_CH_R,           //!< Right channel.
    SND_CH_LS,          //!< Left surround channel.
    SND_CH_RS,          //!< Right surround channel.
    SND_CH_C,           //!< Center channel.
    SND_CH_LFE,         //!< Low frequency effect channel.
    SND_CH_DML,         //!< Downmix L channel.
    SND_CH_DMR          //!< Downmix R channel.
};

enum snd_down_mix_channel
{
    SND_DOWNMIXCHANNEL_DM, //!< Not used any more.
    SND_DOWNMIXCHANNEL_LR  //!< Not used any more.
};

enum snd_down_mix_mode
{
    SND_DOWNMIXMODE_51 = 1, //!< Not used any more.
    SND_DOWNMIXMODE_LORO,   //!< Not used any more.
    SND_DOWNMIXMODE_LTRT,   //!< Not used any more.
    SND_DOWNMIXMODE_VIR     //!< Not used any more.
};

/*! @enum snd_desc_output_channel
@brief Not used any more.
*/
enum snd_desc_output_channel
{
    SND_FORWARD_CH = 1, //!< Not used any more.
    SND_SURROUND_CH,    //!< Not used any more.
    SND_LFE_C_CH,       //!< Not used any more.
    SND_DOWNMIX_CH      //!< Not used any more.
};

/*! @enum snd_equalizer
@brief Not used any more.
*/
enum snd_equalizer
{
    SND_EQ_NONE = 0,        //!< Disable equalizer mode.
    SND_EQ_CLASSIC,         //!< Classsic mode.
    SND_EQ_ROCK,            //!< Rock mode.
    SND_EQ_JAZZ,            //!< Jazz mode.
    SND_EQ_POP,             //!< Pop mode
    SND_EQ_BASS,            //!< Bass mode.
    SND_EQ_USER             //!< User define mode.
};

/*! @enum snd_reverb
@brief Not used any more.
*/
enum snd_reverb
{
    SND_REVERB_OFF = 0, //!< Not used any more.
    SND_REVERB_CHURCH,  //!< Not used any more.
    SND_REVERB_CINEMA,  //!< Not used any more.
    SND_REVERB_CONCERT, //!< Not used any more.
    SND_REVERB_HALL,    //!< Not used any more.
    SND_REVERB_LIVE,    //!< Not used any more.
    SND_REVERB_ROOM,    //!< Not used any more.
    SND_REVERB_STADIUM, //!< Not used any more.
    SND_REVERB_STANDARD //!< Not used any more.
};

/*! @enum snd_speaker_size
@brief Not used any more.
*/
enum snd_speaker_size
{
    SND_SPEAKER_SIZE_OFF= 0,    //!< Not used any more.
    SND_SPEAKER_SIZE_ON,        //!< Not used any more.
    SND_SPEAKER_SIZE_SMALL,     //!< Not used any more.
    SND_SPEAKER_SIZE_BIG        //!< Not used any more.
};


/*! @enum SndDRC
@brief Not used any more.
*/
enum snd_drc
{
    SND_DRC_OFF = 0,    //!< Not used any more.
    SND_DRC_CUSTOM_A,   //!< Not used any more.
    SND_DRC_CUSTOM_D,   //!< Not used any more.
    SND_DRC_LINE_OUT,   //!< Not used any more.
    SND_DRC_RF_REMOD    //!< Not used any more.
};

enum asnd_out_mode
{
    SND_OUT_GEN = 0,
    SND_OUT_DIG,
    SND_OUT_DIGGEN
};


enum snd_msg_cb_type
{
    SND_MSG_CB_FIRST_FRM_OUTPUT = 20,
    SND_MSG_CB_OUTPUT_DATA_END,
    SND_MONITOR_SBM_MIX_END, // Moniter SBM mix end.
};

typedef void(* snd_cbfunc)(UINT32 uparam1, UINT32 uparam2);
struct snd_msg_callback
{
    snd_cbfunc pcb_snd_first_frm_output;
    snd_cbfunc pcb_snd_output_data_end;
    snd_cbfunc pcb_snd_moniter_sbm_mix_end;
};


#if 0
/*! @enum audio_cb_type
@brief
The callback function type of sound module.
*/
enum audio_cb_type
{
    AUDIO_CB_DECA_MONITOR_NEW_FRAME, //!<One frame was decoded by audio decoder.
    AUDIO_CB_DECA_MONITOR_START, //Audio decoder is in start status.
    AUDIO_CB_DECA_MONITOR_STOP, //Audio decoder is in stop status.
    AUDIO_CB_DECA_MONITOR_DECODE_ERR, //Audio decoder return one err status.
    AUDIO_CB_DECA_MONITOR_OTHER_ERR, //Audio decoder occurred other err.
    AUDIO_CB_DECA_STATE_CHANGED,  //Audio decoder state is changed.
    AUDIO_CB_DECA_CB_ASE_DATA_END,

    AUDIO_CB_SND_MONITOR_REMAIN_DATA_BELOW_THRESHOLD, // Moniter Sound card dma data is below the threshold.
    AUDIO_CB_SND_MONITOR_OUTPUT_DATA_END, // Moniter Sound card dma data is occured the end.
    AUDIO_CB_SND_MONITOR_ERRORS_OCCURED, // Moniter Sound card is occured some errors.
    AUDIO_CB_FIRST_FRAME_OUTPUT, //!<The first frame was outputed successfully.
};
#endif /* 0 */

/*! @typedef audio_cb_func
@brief
The callback function type of audio module.
*/
typedef void (*audio_cb_func)(UINT32 uparam1, UINT32 uparam2);


/*! @struct audio_io_reg_callback_para
@brief
Parameters definition of SND_IO_REG_CALLBACK, for registering callback function.
*/
struct audio_io_reg_callback_para       // Defined for hld_rpc to see
{
    UINT32 e_cb_type; //!<The callback function type to be registered.
    audio_cb_func p_cb; //!<The callback function pointer to be registered.
    UINT32 monitor_rate; //!<The monitor rate to be registered.
    UINT32 timeout_threshold;//!<The timeout threshold to be registered.
    UINT32 reversed; //!<Thereverse to be registered.
};

/*! @struct audio_callback_register_param
@brief
Parameters definition of SND_IO_REG_CALLBACK, for registering callback function.
*/
struct audio_callback_register_param   // Defined for app use
{
    UINT32 e_cb_type; //!<The callback function type to be registered.
    UINT32 monitor_rate; //!<The monitor rate to be registered.
    UINT32 timeout_threshold;//!<The timeout threshold to be registered.
    UINT32 reversed; //!<The reversed param to be registered.
};

/*!
@ snd sbm mix info struct
*/
struct snd_mix_info
{
    UINT32      cmd;
    UINT32      ch_num;
    UINT32      sample_rate;
    UINT32      bit_per_samp;
    UINT32      sbm_id;
};


enum spdif_output_data_type
{
	SPDIF_OUT_PCM = 0,//!<The data format is pcm.
	SPDIF_OUT_DD,//!<The data format is dd.
	SPDIF_OUT_BS,
	SPDIF_OUT_INVALID
};

/*! @enum hdmi_output_data_type
@brief A enum defines the spdif interface output data type
*/
enum hdmi_output_data_type
{
    HDMI_OUT_PCM = 0,//!<The data format is pcm.
    HDMI_OUT_DD,//!<The data format is dd by the trancoding.
    HDMI_OUT_BS,//!<The data format is same as the bitstream format.
    HDMI_OUT_AUTO,//!<Mode added for hdmi auto detect function
    HDMI_OUT_INVALID
};

/*! @enum snd_spo_output_src_type
@brief A enum defines the HDMI output source type.
*/
enum snd_spo_output_src_type
{
    SND_OUT_SRC_DDPSPO = 0, //!< HDMI output from DDP SPO DMA.
    SND_OUT_SRC_SPO = 1     //!< HDMI output from SPO DMA.
};

enum LLD_SND_M36G_FUNC
{
    FUNC_SND_M36G_ATTACH = 0,
    FUNC_SND_M36G_INIT_TONE_VOICE,
    FUNC_SND_M36G_INIT_SPECTRUM,
};

enum HLD_SND_FUNC
{
    FUNC_GET_STC,
    FUNC_SET_STC,
    FUNC_GET_STC_DIVISOR,
    FUNC_SET_STC_DIVISOR,
    FUNC_STC_PAUSE,
    FUNC_STC_INVALID,
    FUNC_STC_VALID,
    FUNC_SND_OUTPUT_CONFIG,
    FUNC_SND_OPEN,
    FUNC_SND_CLOSE,
    FUNC_SND_SET_MUTE,
    FUNC_SND_SET_VOLUME,
    FUNC_SND_GET_VOLUME,
    FUNC_SND_DATA_ENOUGH,
    FUNC_SND_REQUEST_PCM_BUFF,
    FUNC_SND_WRITE_PCM_DATA,
    FUNC_SND_WRITE_PCM_DATA2,
    FUNC_SND_IO_CONTROL,
    FUNC_SND_SET_SUB_BLK,
    FUNC_SND_SET_DUPLICATE,
    FUNC_SND_SET_SPDIF_TYPE,
    FUNC_SND_CONFIG,
    FUNC_SND_START,
    FUNC_SND_STOP,
    FUNC_SND_GEN_TONE_VOICE,
    FUNC_SND_STOP_TONE_VOICE,
    FUNC_SND_ENA_PP_8CH,
    FUNC_SND_SET_PP_DELAY,
    FUNC_SND_ENABLE_VIRTUAL_SURROUND,
    FUNC_SND_ENABLE_EQ,
    FUNC_SND_ENABLE_BASS,
    FUNC_SND_PAUSE,
    FUNC_SND_RESUME,
    FUNC_SND_GET_PLAY_TIME,
    FUNC_SND_GET_UNDERRUN_TIMES,
	FUNC_SND_SET_DBG_LEVEL,
	FUNC_SND_SET_PCM_CAPTURE_BUFF_INFO,
	FUNC_SND_GET_PCM_CAPTURE_BUFF_INFO,
	FUNC_MP_IO_CONTROL,
};

///////Only tds used defination end<-----///////
#define AUD_MIX_STOP            0x00000000
#define AUD_MIX_START           0x00000001
#define AUD_MIX_PAUSE           0x00000002
#define AUD_MIX_RESUME          0x00000004

#define MUTE_BY_GPIO                0           //!< Control mute function by GPIO.
#define MUTE_BY_SCART               1           //!< Control mute function by SCART.
#define MUTE_BY_EXT_GPIO            2

#define SND_IO                                  0x0000000F   //!< Sound output device ioctl command base.
#define IS_SND_RUNNING                          (SND_IO + 1) //!< Check whether sound device is in PLAY state.
#define IS_SND_MUTE                             (SND_IO + 2) //!< Check whether sound device is in MUTE state.
#define SND_CC_MUTE                             (SND_IO + 3) //!< Set volume to zero and close internal DAC.
#define SND_CC_MUTE_RESUME                      (SND_IO + 4) //!< Resume vlume to last setting and open internal DAC.
#define SND_SET_FADE_SPEED                      (SND_IO + 5) //!< Set volume fade in and out speed.
#define IS_PCM_EMPTY                            (SND_IO + 6) //!< Check wheather the I2SO DMA buffer is empty.
#define SND_PAUSE_MUTE                          (SND_IO + 7) //!< Set volume to zero when the sound device is not mute.
#define SND_SPO_ONOFF                           (SND_IO + 8) //!< Open or close the SPO/DDP_SPO interface.
#define SND_REQ_REM_DATA                        (SND_IO + 9) //!< Get the remain pcm data size still not play yet.
#define SND_SPECTRUM_START                      (SND_IO + 10) //!< Start to spectrum play.
#define SND_SPECTRUM_STOP                       (SND_IO + 11) //!< Stop to spectrum play.
#define SND_SPECTRUM_CLEAR                      (SND_IO + 12) //!< Clear spectrum play data.
#define SND_BYPASS_VCR                          (SND_IO + 13) //!< Set bypass VCR.
#define FORCE_SPDIF_TYPE                        (SND_IO + 14) //!< Set the spdif output type, strongly recommend call this command in channel change task.
#define SND_DAC_MUTE                            (SND_IO + 15) //!< Do not use any more.
#define SND_CHK_SPDIF_TYPE                      (SND_IO + 16) //!< Check SPDIF output type.
#define SND_CHK_DAC_PREC                        (SND_IO + 17) //!< Do not use any more.
#define SND_CHK_PCM_BUF_DEPTH                   (SND_IO + 18) //!< Get the pcm DMA buffer size in frame unit.
#define SND_POST_PROCESS_0                      (SND_IO + 19) //!< Not used any more.
#define SND_SPECIAL_MUTE_REG                    (SND_IO + 20) //!< Not used any more.
#define STEREO_FUN_ON                           (SND_IO + 21) //!< Not used any more.
#define SND_REQ_REM_PCM_DATA                    (SND_IO + 22) //!< Get the remain pcm data size still not play yet, equal to SND_REQ_REM_DATA.
#define SND_SPECTRUM_STEP_TABLE                 (SND_IO + 23) //!< Set spectrum step table.
#define SND_SPECTRUM_VOL_INDEPEND               (SND_IO + 24) //!< Set spectrum volume independent.
#define SND_SPECTRUM_CAL_COUNTER                (SND_IO + 25) //!< Set spectrum caculate counter.
#define SND_SET_SYNC_DELAY                      (SND_IO + 26) //!< Set audio sync delay time.
#define SND_REQ_REM_PCM_DURA                    (SND_IO + 27) //!< Get the remain data duration playing time.
#define SND_SET_SYNC_LEVEL                      (SND_IO + 28) //!< Set audio sync level.
#define SND_GET_SPDIF_TYPE                      (SND_IO + 29) //!< Get the SPDIF output type.
#define SND_SET_BS_OUTPUT_SRC                   (SND_IO + 30) //!< Set DDP_SPO(HDMI) output source.
#define SND_SET_MUTE_TH                         (SND_IO + 31) //!< Not used any more.
#define SND_GET_MUTE_TH                         (SND_IO + 32) //!< Not used any more.
#define SND_SET_SPDIF_SCMS                      (SND_IO + 33) //!< struct snd_spdif_scms *
#define SND_GET_SAMPLES_REMAIN                  (SND_IO + 34) //!< Get the remain PCM samples size still not output yet.
#define SND_SECOND_DECA_ENABLE                  (SND_IO + 35) //!< Enable second DECA for audio description.
#define SND_SET_DESC_VOLUME_OFFSET              (SND_IO + 36) //!< Set audio description voume offset.
#define SND_GET_TONE_STATUS                     (SND_IO + 37) //!< Get tone volume playing status.
#define SND_DO_DDP_CERTIFICATION                (SND_IO + 38) //!< Enable sound device do DDP certifictation function.
#define SND_AUTO_RESUME                         (SND_IO + 39) //!< Enable sound device auto resume function when error occurs.
#define SND_SET_SYNC_PARAM                      (SND_IO + 40) //!< Set synchronization parameters.
#define SND_RESET_DMA_BUF                       (SND_IO + 41) //!< Reset sound dma buffer to drop some frames. Only used for SEE side. Please do not use it.
#define SND_I2S_OUT                             (SND_IO + 42) //!< Enable I2S output audio.
#define SND_HDMI_OUT                            (SND_IO + 43) //!< Enable HDMI output audio.
#define SND_SPDIF_OUT                           (SND_IO + 44) //!< Enable SPDIF output audio.
#define SND_SET_FRAME_SHOW_PTS_CALLBACK         (SND_IO + 45) //!< Not used any more.
#define SND_MPEG_M8DB_ENABLE                    (SND_IO + 46) //!< Enable MPEG M8DB ajustment for pcm data, only used for SEE side. Please do not use it.
#define SND_HDMI_ENABLE                         (SND_IO + 47) //!< Only used for SEE side. Please do not use it.
#define SND_GET_SYNC_PARAM                      (SND_IO + 48)
#define SND_RESTART                             (SND_IO + 49)
#define SND_STOP_IMMD                           (SND_IO + 50)
#define SND_DMX_SET_VIDEO_TYPE                  (SND_IO + 51)
#define SND_DO_DDP_CERTIFICATION_EX             (SND_IO + 52)
#define SND_BUF_DATA_REMAIN_LEN                 (SND_IO + 53)
#define SND_STC_DELAY_GET                       (SND_IO + 54)
#define SND_EABLE_INIT_TONE_VOICE               (SND_IO + 55) //beeptone
#define SND_IO_REG_CALLBACK                     (SND_IO + 56)
#define SND_IO_SET_FADE_ENBALE                  (SND_IO + 57) //param:0 disable; 1 enable
#define SND_ONLY_SET_SPDIF_DELAY_TIME           (SND_IO + 58) //!< Set SPDIF delay time (0ms-250ms).
#define SND_HDMI_CONFIG_SPO_CLK                 (SND_IO + 59) //!< HDMI config DDP_SPO clock to SPO clock.

#define SND_ENABLE_DROP_FRAME                   (SND_IO + 60) //!< Only used for SEE side. Please do not use it.
#define SND_REG_GET_SYNC_FLAG_CB                (SND_IO + 61) //!< Get AV synchronization flag control block from avsync module, only used for SEE side. Please do not use it.
#define SND_SET_HW_HOLD_THRESHOLD               (SND_IO + 62) //!< Hardware hold DMA threshold by avsync module, only used for SEE side. Please do not use it.
#define SND_GET_RESET_PARAM                     (SND_IO + 63) //!< Only used for SEE side. Please do not use it.
#define SND_SET_RESET_PARAM                     (SND_IO + 64) //!< Only used for SEE side. Please do not use it.
#define SND_GET_STATUS                          (SND_IO + 65) //!< Get sound device status.
#define SND_GET_RAW_PTS                         (SND_IO + 66) //!< Get raw PTS.
#define SND_IO_PAUSE_SND                        (SND_IO + 67) //!< Pause sound device, only used for SEE side. Please do not use it.
#define SND_IO_RESUME_SND                       (SND_IO + 68) //!< Resume sound device, only used for SEE side. Please do not use it.

#define SND_SET_AUD_AVSYNC_PARAM     	        (SND_IO + 69)
#define SND_GET_AUD_AVSYNC_PARAM     	        (SND_IO + 70)
#define SND_SET_UPDATE_PTS_TO_DMX_CB 	        (SND_IO + 71)

//this CMD will change the time when the HW output the sound
#define SND_IO_SET_CC_MUTE_RESUME_FRAME_COUNT_THRESHOLD (SND_IO + 72)

#define SND_IO_SPO_INTF_CFG                     (SND_IO + 73) //!< SND SPDIF output config
#define SND_IO_DDP_SPO_INTF_CFG                 (SND_IO + 74) //!< SND HDMI output config
#define SND_IO_SPO_INTF_CFG_GET                 (SND_IO + 75)
#define SND_IO_DDP_SPO_INTF_CFG_GET             (SND_IO + 76)

#define SND_IO_GET_PLAY_PTS                     (SND_IO + 77)//!< Get current playing PTS
#define SND_ONLY_GET_SPDIF_DELAY_TIME           (SND_IO + 78)//!< Get SPDIF delay time (0ms-250ms).
#define SND_SET_DESC_VOLUME_OFFSET_NEW          (SND_IO + 79) //!< Set audio description voume offset.--20161012 add for new AD control.
#define SND_GET_AD_DYNAMIC_EN                   (SND_IO + 80) //!< Get audio description voume state.--20161012 add for new AD control.
#define SND_SET_AD_DYNAMIC_EN                   (SND_IO + 81) //!< Set audio description voume state.--20161012 add for new AD control.

#define SND_IO_SET_MIX_INFO                     (SND_IO + 82) //!< Set snd mix info
#define SND_IO_SET_MIX_END                      (SND_IO + 83) //!< Set sbm audio mix end
#define SND_IO_GET_MIX_STATE                    (SND_IO + 84) //!< Get sbm audio mix state

#define SND_IO_SET_DIGITAL_AUDIO_OUTPUT_ONOFF   (SND_IO + 85) //!< set digital audio output
#define SND_IO_GET_DIGITAL_AUDIO_OUTPUT_ONOFF   (SND_IO + 86) //!< get digital audio output
#define SND_IO_GET_MUTE_STATE                   (SND_IO + 87) //!< get SND mute state
#define SND_IO_GET_CHAN_STATE                   (SND_IO + 88) //!< get SND mute state

#define SND_ADV_IO                              (SND_IO + 0x200) //!< Sound sub ioctl command base.
#define SND_BASS_TYPE                           (SND_ADV_IO + 1) //!< Not used any more.
#define SND_REG_HDMI_CB                         (SND_ADV_IO + 2) //!< Set the HDMI callback for sound device.
//#define SND_ENABLE_DROP_FRAME                   (SND_ADV_IO + 3)
#define SND_PCM_DUMP_ON                         (SND_ADV_IO + 4) //!< Not used any more.
#define SND_PCM_DUMP_OFF                        (SND_ADV_IO + 5) //!< Not used any more.
#define SND_GET_DBG_INFO                        (SND_ADV_IO + 6) //!< get debug info. Only used for inside.
#define SND_GET_PCM_CAPTURE_BUFF_INFO           (SND_ADV_IO + 7) //!< get pcm capture buffer info.
#define SND_SET_PCM_CAPTURE_BUFF_RD				(SND_ADV_IO + 8) //!< set pcm capture buffer info.

//added by kinson for kalaok
#define SND_I2SIN_MIC0_GET_ENABLE        (SND_ADV_IO + 20)
#define SND_I2SIN_MIC0_SET_ENABLE        (SND_ADV_IO + 21)
#define SND_I2SIN_MIC0_GET_VOLUME        (SND_ADV_IO + 22)
#define SND_I2SIN_MIC0_SET_VOLUME        (SND_ADV_IO + 23)
#define SND_I2SIN_MIC0_START             (SND_ADV_IO + 24)
#define SND_I2SIN_MIC0_STOP              (SND_ADV_IO + 25)

#define SND_I2SIN_MIC1_GET_ENABLE        (SND_ADV_IO + 26)
#define SND_I2SIN_MIC1_SET_ENABLE        (SND_ADV_IO + 27)
#define SND_I2SIN_MIC1_GET_VOLUME        (SND_ADV_IO + 28)
#define SND_I2SIN_MIC1_SET_VOLUME        (SND_ADV_IO + 29)
#define SND_I2SIN_MIC1_START             (SND_ADV_IO + 30)
#define SND_I2SIN_MIC1_STOP              (SND_ADV_IO + 31)


#define CODEC_I2S (0x0<<1)      //!< I2S codec format.
#define CODEC_LEFT (0x1<<1)     //!< Left codec format.
#define CODEC_RIGHT (0x2<<1)    //!< Right codec format.


// SPDIF raw data coding type
#define SPDO_SRC_FLR                0x00 //!< Not used any more.
#define SPDO_SRC_SLR                0x01 //!< Not used any more.
#define SPDO_SRC_CSW                0x02 //!< Not used any more.
#define SPDO_SRC_DMLR               0x03 //!< Not used any more.
#define SPDO_SRC_EXLR               0x04 //!< Not used any more.
#define SPDO_SRC_BUF                0x07 //!< Not used any more.
#define SPDO_SRC_LFEC               0x01 //!< Not used any more.


enum snd_dup_channel
{
    SND_DUP_NONE,
    SND_DUP_L,
    SND_DUP_R,
    SND_DUP_MONO,
    SND_DUP_CHANNEL_END  //for hld input param check
};


struct snd_output_cfg
{
    UINT8 mute_num; //mute circuit gpio number.
    UINT8 mute_polar; //the polarity which will cause circuit mute
    UINT8 dac_precision;//24bit or 16bit
    UINT8 dac_format;//CODEC_I2S (0x0<<1), CODEC_LEFT (0x1<<1), CODEC_RIGHT (0x2<<1)
    UINT8 is_ext_dac; //for M3329 serial, always should be 1. 0: means embedded dac.
    UINT8 reserved8;
    UINT16 gpio_mute_circuit; //FALSE: no mute circuit; TRUE: exists mute circuit controlled by GPIO
    UINT16 ext_mute_mode;
    UINT16 enable_hw_accelerator;     //FALSE: do not enable M3202 audio HW accelerator;
                                    //TRUE: Enable M3202 audio HW accelerator;
    UINT8 chip_type_config;      //1:QFP.0:BGA.
    UINT16 reservedss;
    UINT8 mute_ext_gpio_clock;
    UINT8 mute_ext_gpio_data;
};

struct snd_feature_config
{
    struct snd_output_cfg output_config;
    UINT8 support_spdif_mute;
    UINT8 swap_lr_channel;
    UINT8 conti_clk_while_ch_chg;
    UINT8 support_desc;
    UINT8 ad_static_mem_flag;
    UINT32 ad_static_mem_addr;
    UINT32 ad_static_mem_size;
};

/** Used by IO_CTRL RPC: **/
/** SND_SET_SYNC_PARAM **/
typedef struct
{
    UINT32 drop_threshold;          //!< The drop frame threshold.
    UINT32 wait_threshold;          //!< The wait frame threshold.
    UINT32 delay_video_sthreshold;  //!< The delay video frame threshold.
    UINT32 hold_threshold;          //!< The hold frame threshold.
    UINT32 dma_remain_threshold;    //!< The DMA remain frame threshold.
} snd_sync_param;


/** Used by IO_CTRL RPC: **/
/** SND_SPECTRUM_START **/
typedef struct
{
    void (*spec_call_back) (INT32 *);   //!< The spectrum callback.
    UINT32 collumn_num;                 //!< The column number.
} spec_param;

/** Used by IO_CTRL RPC: **/
/** SND_SPECTRUM_STEP_TABLE **/
typedef struct
{
    UINT32 column_num;  //!< The column number.
    UINT8 *ptr_table;   //!< The spectrum step table address.
} spec_step_table;


/** Used by IO_CTRL RPC: **/
/** SND_GET_STATUS **/
struct snd_dev_status
{
    UINT8 flags;            //!< Sound device flags.
    UINT32 volume;          //!< The volume sound device output.
    UINT32 in_mute;         //!< The flag wheather sound device mute.
    UINT8 spdif_out;        //!< The flag wheather spdif data valid.
    UINT8 trackmode;        //!< The channel mode, refer to enum SndDupChannel.

    UINT32 samp_rate;       //!< The sample rate of input audio stream.
    UINT32 samp_num;        //!< The sample number of input audio stream.
    UINT32 ch_num;          //!< The channel number of input audio stream.

    UINT32 drop_cnt;        //!< The frame count droped by avsync module.
    UINT32 play_cnt;        //!< The frame count let to play by avsync module.
    UINT32 pcm_out_frames;  //!< The frame count have written in sound device DMA.

    UINT32 pcm_dma_base;    //!< The DMA buffer stored PCM data.
    UINT32 pcm_dma_len;     //!< The length of DMA buffer.
    UINT32 pcm_rd;          //!< The read index of DMA buffer.
    UINT32 pcm_wt;          //!< The write index of DMA buffer.
    UINT32 underrun_cnts;   //!< The underrun counts of DMA buffer.

    UINT32 pcm_dump_addr;   //!< The buffer address dumped pcm data.
    UINT32 pcm_dump_len;    //!< The length of dump pcm buffer.

    UINT8 spdif_mode;       //!< The output mode of SPDIF interface.
    UINT32 spdif_user_bit;  //!< Not used any more.
};

/** Used by RPC api: **/
/** RET_CODE snd_set_volume(struct snd_device * dev, enum SndSubBlock sub_blk, UINT8 volume)**/
enum snd_sub_block
{
    SND_SUB_PP = 0x01,        // Audio post-process.
    SND_SUB_IN = 0x02,        // General audio input interface.
    SND_SUB_OUT = 0x04,        // General audio output interface.
    SND_SUB_MIC0 = 0x08,    // Micro phone 0 input interface.
    SND_SUB_MIC1 = 0x10,    // Micro phone 1 input interface.
    SND_SUB_SPDIFIN = 0x20,    // SPDIF input interface.
    SND_SUB_SPDIFOUT = 0x40,// SPDIF output interface.
    SND_SUB_SPDIFOUT_DDP = 0x80,
    SND_SUB_ALL    = 0xff        // All IO enabled.
};

/** Used by RPC api: **/
/** RET_CODE snd_set_spdif_type(struct snd_device *dev, enum asnd_out_spdif_type type)**/
enum asnd_out_spdif_type
{
    SND_OUT_SPDIF_INVALID = -1,
    SND_OUT_SPDIF_PCM = 0,
    SND_OUT_SPDIF_BS = 1,
    SND_OUT_SPDIF_FORCE_DD = 2,
    SND_OUT_SPDIF_END,//for hld input param check
};

/** Just added for compile,No used info in linux **/
struct pcm_output
{
    UINT32 ch_num ;
    UINT32 ch_mod;
    UINT32 samp_num ;
    UINT32 sample_rata_id;
    UINT32 inmode;
    UINT32 *ch_left ;
    UINT32 *ch_right ;
    UINT32 *ch_sl ;
    UINT32 *ch_sr ;
    UINT32 *ch_c ;
    UINT32 *ch_lfe ;
    UINT32 *ch_dl ;
    UINT32 *ch_dr ;
    UINT32 *ch_left_m8db ;             // Added for mpeg pcm data -31db for spdif output in bs out mode
    UINT32 *ch_right_m8db ;
    UINT32 *ch_sl_m8db ;
    UINT32 *ch_sr_m8db ;
    UINT32 *ch_c_m8db ;
    UINT32 *ch_lfe_m8db ;
    UINT32 *ch_dl_m8db ;
    UINT32 *ch_dr_m8db ;
    UINT8 *raw_data_start;
    UINT32 raw_data_len;
    UINT32 iec_pc;

    UINT8 *raw_data_ddp_start; //KwunLeung
    UINT32 raw_data_ddp_len;
    UINT8 iec_pc_ddp;
};

/** Just added for compile, No used info in linux **/
enum EQ_TYPE
{
    EQ_SLIGHT=0,
    EQ_CLASSIC=1,
    EQ_ELECTRONIC=2,
    EQ_DANCE=3,
    EQ_LIVE=4,
    EQ_POP=5,
    EQ_ROCK=6,
};

struct snd_get_pts_param
{
	BOOL pts_valid;
	UINT32 pts;
};

/*
 * Buffer for capturing pcm data from see to main by DSC module.
 * param:
 * buff_base: The address of the buffer.
 * buff_len: The buffer len of buff_base.
 * buff_wt: The address for writing pcm data.
 * buff_wt_skip: The address of skip writing pcm data from here to the end of the buffer.
 * buff_rd: The address for reading pcm data.
 * status: The status of buffer.
 * sample_rate: sample rate of the pcm data.
 */
struct pcm_capture_buff
{
	unsigned int  *buff_base;
	UINT16  buff_len;
	UINT16  buff_wt;
	UINT16  buff_wt_skip;
	UINT16  buff_rd;
	UINT16  status;
	unsigned int  sample_rate;
};

struct snd_device
{
    struct snd_device  *next;  /*next device */
    /*struct module *owner;*/
    INT32 type;
    INT8 name[HLD_MAX_NAME_SIZE];
    INT32 flags;

    INT32 hardware;
    INT32 busy;
    INT32 minor;

    void *priv;        /* Used to be 'private' but that upsets C++ */
    UINT32 base_addr;

    void      (*attach)(void);
    void      (*detach)(struct snd_device **);
    RET_CODE   (*open)(struct snd_device *);
    RET_CODE   (*close)(struct snd_device *);
    RET_CODE   (*set_mute)(struct snd_device *, enum snd_sub_block, UINT8);
    RET_CODE   (*set_volume)(struct snd_device *, enum snd_sub_block, UINT8);
    RET_CODE   (*set_sub_blk)(struct snd_device *, UINT8 , UINT8);
    RET_CODE   (*set_duplicate)(struct snd_device *, enum snd_dup_channel);
    RET_CODE   (*request_pcm_buff)(struct snd_device *, UINT32);
    RET_CODE   (*data_enough)(struct snd_device *);
    RET_CODE   (*config)(struct snd_device *, UINT32, UINT16, UINT8);
    RET_CODE   (*set_spdif_type)(struct snd_device *, enum asnd_out_spdif_type);
    RET_CODE   (*ioctl)(struct snd_device *, UINT32 , UINT32);
    void (*write_pcm_data)(struct snd_device*,struct pcm_output*,UINT32*);
    void (*write_pcm_data2)(struct snd_device *, UINT32 *, UINT32 *, UINT32 *, UINT32, UINT32);
    RET_CODE (*snd_get_stc)(UINT32, UINT32 *, UINT8);
    void (*snd_set_stc)(UINT32, UINT32, UINT8);
    void (*snd_get_divisor)(UINT32, UINT16 *, UINT8);
    void (*snd_set_divisor)(UINT32, UINT16, UINT8);
    void (*snd_stc_pause)(UINT32, UINT8, UINT8);
    void (*snd_invalid_stc)(void);
    void (*snd_valid_stc)(void);
    void (*start)(struct snd_device *);
    void (*stop)(struct snd_device *);
    UINT8 (*get_volume)(struct snd_device *);
    RET_CODE (*ena_pp_8ch)(struct snd_device *,UINT8);
    RET_CODE (*set_pp_delay)(struct snd_device *,UINT8);
    RET_CODE (*enable_virtual_surround)(struct snd_device *,UINT8);
    RET_CODE (*enable_eq)(struct snd_device *,UINT8 ,enum EQ_TYPE);
    RET_CODE (*enable_bass)(struct snd_device *,UINT8);
    int (*gen_tone_voice)(struct snd_device *, struct pcm_output* , UINT8); //tone voice
    void (*stop_tone_voice)(struct snd_device *);  //tone voice
    void (*output_config)(struct snd_device *, struct snd_output_cfg *);
    RET_CODE (*spectrum_cmd)(struct snd_device *, UINT32 , UINT32);
	RET_CODE (*set_pcm_capture_buff_info)(struct snd_device *dev, UINT32 info, UINT8 flag);
	RET_CODE (*get_pcm_capture_buff_info)(struct snd_device *dev, struct pcm_capture_buff *info);

    RET_CODE (*request_desc_pcm_buff)(struct snd_device *, UINT32);
    void (*write_desc_pcm_data)(struct snd_device*,struct pcm_output*,UINT32*);
    RET_CODE   (*pause)(struct snd_device *);
    RET_CODE   (*resume)(struct snd_device *);
    UINT32   (*get_play_time)(struct snd_device *);
    UINT32   (*get_underrun_times)(struct snd_device *);
    RET_CODE (*set_dbg_level) (struct snd_device *, UINT32);
};

struct snd_dbg_info
{
    UINT32 state;
    UINT32 sub_state;
    UINT32 ad_en;
    UINT32 sync_buff_pcm_len;
    UINT32 sync_buff_pcm_rm;
    UINT32 sync_buff_desc_pcm_len;
    UINT32 sync_buff_desc_pcm_rm;
    UINT32 sync_buff_dd_len;
    UINT32 sync_buff_dd_rm;
    UINT32 sync_buff_ddp_len;
    UINT32 sync_buff_ddp_rm;
    UINT32 dma_buff_pcm_len;
    UINT32 dma_buff_pcm_rm;
    UINT32 dma_buff_dd_len;
    UINT32 dma_buff_dd_rm;
    UINT32 dma_buff_ddp_len;
    UINT32 dma_buff_ddp_rm;
};
struct deca_dbg_info
{
    UINT32 state;
    UINT32 sub_state;
    UINT32 prog_bs_buff_len;
    UINT32 prog_bs_buff_rm;
    UINT32 prog_cb_buff_len;
    UINT32 prog_cb_buff_rm;
    UINT32 desc_bs_buff_len;
    UINT32 desc_bs_buff_rm;
    UINT32 desc_cb_buff_len;
    UINT32 desc_cb_buff_rm;
};
struct audio_dbg_info
{
    struct deca_dbg_info deca;
    struct snd_dbg_info snd;
};

RET_CODE snd_open(struct snd_device *dev);
RET_CODE snd_close(struct snd_device *dev);
RET_CODE snd_set_mute(struct snd_device *dev, enum snd_sub_block sub_blk, UINT8 enable);
RET_CODE snd_set_volume(struct snd_device *dev, enum snd_sub_block sub_blk, UINT8 volume);
UINT8 snd_get_volume(struct snd_device *dev);
UINT32 snd_get_underrun_times(struct snd_device *dev);
RET_CODE snd_io_control(struct snd_device *dev, UINT32 cmd, UINT32 param);
RET_CODE snd_set_spdif_type(struct snd_device *dev, enum asnd_out_spdif_type type);
RET_CODE snd_set_duplicate(struct snd_device *dev, enum snd_dup_channel type);

void snd_start(struct snd_device *dev);
void snd_stop(struct snd_device *dev);
RET_CODE snd_pause(struct snd_device *dev);
RET_CODE snd_resume(struct snd_device *dev);

RET_CODE get_stc(UINT32 *stc_msb32, UINT8 stc_num);
void set_stc(UINT32 stc_msb32, UINT8 stc_num);
void get_stc_divisor(UINT16 *stc_divisor, UINT8 stc_num);
void set_stc_divisor(UINT16 stc_divisor, UINT8 stc_num);
void stc_invalid(void);
void stc_valid(void);
void stc_pause(UINT8 pause, UINT8 stc_num);


void snd_m36_attach(struct snd_feature_config *config);
void snd_m36_init_tone_voice(struct snd_device *dev);
void snd_init_spectrum(struct snd_device *dev);

void snd_register_cb_routine(void);
RET_CODE snd_set_dbg_level(struct snd_device *dev,UINT32 option);
RET_CODE snd_set_pcm_capture_buff_info(struct snd_device *dev, UINT32 info, UINT8 flag);
RET_CODE snd_get_pcm_capture_buff_info(struct snd_device *dev, struct pcm_capture_buff *info);

#endif


