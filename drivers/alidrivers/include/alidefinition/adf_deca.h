#ifndef __ADF_DECA_H
#define __ADF_DECA_H


////////////////Only tds start --->///////////////

#define DECA_STATE_DETACH   0
#define DECA_STATE_ATTACH   1
#define DECA_STATE_IDLE        2
#define DECA_STATE_PLAY        4
#define DECA_STATE_PAUSE     8

#define DECA_SUB_STATE_BUSY           1
#define DECA_SUB_STATE_NO_DATA    2
#define DECA_SUB_STATE_NO_BUFF     4

enum deca_state_flags
{
    DECA_STATE_NODATA    = 0x0001,    //!<No data state
    DECA_STATE_DECODING  = 0x0002,    //!<Decoding state
};

/*! @enum deca_cbtype
@brief Not used any more.
*/
enum deca_msg_cb_type
{
    DECA_MSG_CB_DECODE_NEW_FRAME = 0,   // Indicator decode one frame sucessfully
    DECA_MSG_CB_DECODE_START,           // Indicator decode start
    DECA_MSG_CB_DECODE_STOP,            // Indicator decode stop
    DECA_MSG_CB_DECODE_FAIL,            // Indicator decode fail
    DECA_MSG_CB_DECODE_DATA_INVALID,    // Indicator decode data state invalid(no data or data invalid)
	DECA_MSG_CB_DECODE_STATE_SWITCH,    // Indicator decode state switch(decoding->no data or no data->decoding)
};

typedef void(*deca_cbfunc)(UINT32 uparam1, UINT32 uparam2);
/*! @struct deca_callback
@brief Not used any more.
*/
struct deca_msg_callback
{
    deca_cbfunc pcb_deca_decode_new_frame;
    deca_cbfunc pcb_deca_decode_start;
    deca_cbfunc pcb_deca_decode_stop;
    deca_cbfunc pcb_deca_decode_fail;
    deca_cbfunc pcb_deca_decode_data_invalid;
	deca_cbfunc pcb_deca_decode_state_switch;
};

enum deca_ase_end_cb_type
{
    NORMAL_TO_END = 0,
    FROM_UI_FORCE_TO_STOP,
    RESERVED,
};

#if 0
/*! @struct deca_io_reg_callback_para
@brief Not used any more.
*/
struct deca_io_reg_callback_para
{
    enum deca_cbtype e_cbtype;
    deca_cbfunc p_cb;
    void *pv_param;
    UINT32 monitor_rate;
    enum deca_state_flags state_flag;
    //if threshold is zero, use the value 400ms
    //union is ms
    UINT32 state_change_timeout_threshold;
    enum deca_ase_end_cb_type ase_end_cb_type;
};
#endif /* 0 */

/*! @struct deca_av_eventlisten
@brief Not used any more.
*/
struct deca_av_eventlisten
{
    UINT32 command;     //!< Not used any more
    UINT32 param1;      //!< Not used any more
    UINT32 reserved;    //!< Not used any more
};

//use for send some audio frame data from see to main
struct deca_data_callback
{
    deca_cbfunc pcb_deca_send_announcement_switching_data_field;
};

struct ancillary_data
{
    UINT16 announcement_switching_data_field1;
    UINT16 announcement_switching_data_field2;
};

#if 0
struct DECA_FRAME_INFO
{
    struct deca_io_reg_callback_para reg_cb_para;
    struct ancillary_data anci_data;
};//this is used to storage the information in every needed frame
#endif /* 0 */

/*! @struct tag_blue_ray_pcm_head_init
@brief Only used for SEE side. Please do not use it.
*/
typedef struct tag_blue_ray_pcm_head_init
{
    UINT32 ul_pcm_frame_len;    //the pcm frame length parse from pes pack.it used by audio decoder
    UINT32 ul_pcm_head;        //the pcm head,and simulate it act as the sync word
}st_blue_ray_pcm_head_init,*pst_blue_ray_pcm_head_init;

#define DATA_LEN_FOR_EXAMINATION     4096     //We need 4096 bytes' mp3 data to examine whether a mp3 file can be decoded.

#define DECA_AC3COMMODE_LINE    2   //!< AC3 compare mode: LINE. Only used for SEE side. Please do not use it.
#define DECA_AC3COMMODE_RF      3   //!< AC3 compare mode: RF. Only used for SEE side. Please do not use it.

#define DECA_AC3_AUTO_MODE      0   //!< AC3 mode: AUTO. Only used for SEE side. Please do not use it.
#define DECA_AC3_LR_MODE        1   //!< AC3 mode: LR. Only used for SEE side. Please do not use it.
#define DECA_AC3_LORO_MODE      2   //!< AC3 mode: LORO. Only used for SEE side. Please do not use it.

#define DEFAULT_BEEP_INTERVAL   (120*11)    //!< Only used for SEE side. Please do not use it.
#define MAX_ASE_RES_ID          5           //!< Only used for SEE side. Please do not use it.

#define ASE_ID_0                0x06f90100  //!< Only used for SEE side. Please do not use it.
#define ASE_ID_1                0x06f90200  //!< Only used for SEE side. Please do not use it.
#define ASE_ID_2                0x06f90300  //!< Only used for SEE side. Please do not use it.
#define ASE_ID_3                0x06f90400  //!< Only used for SEE side. Please do not use it.
#define ASE_ID_4                0x06f90500  //!< Only used for SEE side. Please do not use it.


/*! @struct ase_res_param
@brief Not used any more.
*/
struct ase_res_param
{
    UINT8 *src; //!< Stream start address
    UINT32 len; //!< Stream data length
};

/*! @enum adec_buf_mode
@brief Not used any more.
*/
enum adec_buf_mode
{
    ADEC_LIVE_MODE = 0,     //!< Not used any more
    ADEC_PS_MODE            //!< Not used any more
};

/*! @enum audio_type_internal
@brief Only used for SEE side. Please do not use it.
*/
enum audio_type_internal
{
    AUDIO_MPEG_AAC_2 = 0x2000,  //!< MPEG_AAC decoder ID
    AUDIO_AC3_2,                //!< AC3 decoder ID
    AUDIO_MPEG_ADTS_AAC_2,      //!< ADTS_AAC decoder ID
    AUDIO_EC3_2,                //!< EC3 decoder ID
};

/*! @enum adec_desc_channel_enable
@brief Only used for SEE side. Please do not use it.
*/
enum adec_desc_channel_enable
{
    ADEC_DESC_CHANNEL_DISABLE = 0, //!< Disable narrator channel
    ADEC_DESC_CHANNEL_ENABLE       //!< Enable narrator channel
};


#define AUDIO_DESC_LEVEL_MAX    ((INT32)16)
#define AUDIO_DESC_LEVEL_MIN    ((INT32)-16)

/*! @enum adec_play_speed
@brief  Only used for SEE side. Please do not use it.
*/
enum adec_play_speed
{
    ADEC_SPEED_S_32X = 1,  //!< Audio decode speed -32x
    ADEC_SPEED_S_16X,      //!< Audio decode speed -16x
    ADEC_SPEED_S_8X,       //!< Audio decode speed -8x
    ADEC_SPEED_S_4X,       //!< Audio decode speed -4x
    ADEC_SPEED_S_2X,       //!< Audio decode speed -2x
    ADEC_SPEED_F_1X,       //!< Audio decode speed 1x
    ADEC_SPEED_F_2X,       //!< Audio decode speed 2x
    ADEC_SPEED_F_4X,       //!< Audio decode speed 4x
    ADEC_SPEED_F_8X,       //!< Audio decode speed 8x
    ADEC_SPEED_F_16X,      //!< Audio decode speed 16x
    ADEC_SPEED_F_32X       //!< Audio decode speed 32x
};

/*! @enum adec_down_mix_type
@brief Not used any more.
*/
enum adec_down_mix_type
{
    ADEC_DOWNMIX_TABLE_DV, //!< DVD-Video
    ADEC_DOWNMIX_TABLE_DA  //!< DVD-Audio
};


/*! @enum AUDIO_MODE
@brief Only used for SEE side. Please do not use it.
*/
enum AUDIO_MODE
{
    STEREO = 0,     //!< Only used for SEE side. Please do not use it.
    JOINT_STEREO,   //!< Only used for SEE side. Please do not use it.
    DUAL_CHANNEL,   //!< Only used for SEE side. Please do not use it.
    SINGLE_CHANNEL  //!< Only used for SEE side. Please do not use it.
};

/*! @struct adec_down_mix_table
@brief Not used any more.
*/

struct adec_down_mix_table
{
    enum adec_down_mix_type e_type;
    union coef_table_
    {
        UINT8 dvd_video[24];
        UINT8 dvd_audio[18];
    }coef_table;
};


/*! @struct AUDIO_INFO
@brief A structure defines audio stream information
*/
struct AUDIO_INFO
{
    UINT32 bit_rate;        //!< The audio bit rate
    UINT32 sample_freq;     //!< The audio sample rate
    enum AUDIO_MODE mode;   //!< The audio channel mode
    UINT8 layer;            //!< The MPEG layer
    UINT8 id:4;             //!< The audio stream id
    UINT8 mpeg25:4;         //!< The flag if is MPEG2.5 layer
    UINT16 frm_size;        //!< The frame size
};

/*! @struct SongInfo
@brief Only used for SEE side. Please do not use it.
*/
typedef struct  _SONG_INFO
{
    char title[30];            /*Title*/
    char artist[30];        /*Aurthor/Artist*/
    char album[30];            /*Album*/
    char year[4];            /*republished time(year)*/
    char comment[30];        /*comment*/
    char genre;                /*type*/
    char track;
    char vbr;
    unsigned long time;
    unsigned long    bit_rate;                //bps
    unsigned long    sample_rate;            //KHz
    unsigned long    channel_mode;

    unsigned long    bye1_packet_size;
    unsigned long    bye1_total_packet;
    unsigned long     bye1_data_packet_offset;

    unsigned long   bye1_block_align;
    unsigned long format_tag;
    unsigned long   average_number_of_bytes_per_second;
    unsigned long   bits_per_sample;

    unsigned int    codec_specific_data_size;
    unsigned char   codec_specific_data[32];
    unsigned long   samples_per_block;
    unsigned int    encode_options;
    unsigned char   stream_number;
}song_info;

/*! @struct reverb_param
@brief Only used for SEE side. Please do not use it.
*/
struct reverb_param
{
    UINT16 enable;          //!< Only used for SEE side. Please do not use it.
    UINT16 reverb_mode;     //!< Only used for SEE side. Please do not use it.
};

/*! @struct pl_ii_param
@brief Only used for SEE side. Please do not use it.
*/
struct pl_ii_param
{
    UINT16 enable;          //!< Only used for SEE side. Please do not use it.

    short abaldisable;      //!< Only used for SEE side. Please do not use it.
    short chanconfig;       //!< Only used for SEE side. Please do not use it.
    short dimset;           //!< Only used for SEE side. Please do not use it.
    short surfiltenable;    //!< Only used for SEE side. Please do not use it.
    short modeselect;       //!< Only used for SEE side. Please do not use it.
    short panoramaenable;   //!< Only used for SEE side. Please do not use it.
    short pcmscalefac;      //!< Only used for SEE side. Please do not use it.
    short rsinvenable;      //!< Only used for SEE side. Please do not use it.
    short cwidthset;        //!< Only used for SEE side. Please do not use it.
};

/*! @struct ddp_certification_param
@brief Not used any more.
*/
struct ddp_certification_param
{
    UINT32 stream_type_switch;  //!< Not used any more
    UINT32 stream_rollback;     //!< Not used any more
    UINT32 frame_double_write;  //!< Not used any more
};

typedef struct
{
    UINT32 sampling_rate;
    UINT32 sample_size;
    //for none-byte-align

    //according the add to bytes-align or subtract to bytes-align
    UINT32 final_sample_size;
    //left add the 0 or right add the 0
    UINT32 sample_justification;

    UINT32 endian;
    UINT32 channel_num;
    UINT32 channel_layout;//channel sort
    UINT32 sign_flag;
    UINT32 frame_len;
    UINT32 reserved;
}hld_pcm_audio_attributes;




////////////////Only tds end <----///////////////


/** New defined for audio stream types **/

#define DECA_CMD_BASE                           0x00                //!< Audio decoder device ioctl command base
#define DECA_SET_STR_TYPE                       (DECA_CMD_BASE + 1) //!< Set audio stream type
#define DECA_GET_STR_TYPE                       (DECA_CMD_BASE + 2) //!< Get audio stream type
#define DECA_SET_DOLBY_ONOFF                    (DECA_CMD_BASE + 3) //!< Not used any more
#define DECA_AUDIO_KEY                          (DECA_CMD_BASE + 4) //!< Not used any more
#define DECA_BEEP_START                         (DECA_CMD_BASE + 5) //!< Not used any more. Please use DECA_STR_PLAY.
#define DECA_BEEP_STOP                          (DECA_CMD_BASE + 6) //!< Not used any more. Please use DECA_STR_STOP.
#define DECA_BEEP_INTERVAL                      (DECA_CMD_BASE + 7) //!< Set beep interval for loop, in main side must be used together with #DECA_STR_PLAY or (#DECA_STR_PAUSE and #DECA_STR_RESUME and #DECA_STR_STOP).
#define DECA_SET_PLAY_SPEED                     (DECA_CMD_BASE + 8) //!< Not used any more
#define DECA_HDD_PLAYBACK                       (DECA_CMD_BASE + 9) //!< Set HDD playback mode
#define DECA_STR_PLAY                           (DECA_CMD_BASE + 10) //!<Play a slice of audio bitstream in SDRAM
#define DECA_SET_MULTI_CH                       (DECA_CMD_BASE + 11) //!< Not used any more
#define DECA_STR_PAUSE                          (DECA_CMD_BASE + 12) //!< Not used any more
#define DECA_STR_RESUME                         (DECA_CMD_BASE + 13) //!< Not used any more
#define DECA_STR_STOP                           (DECA_CMD_BASE + 14) //!< Sub command for AUDIO_ASE_STR_STOP
#define DECA_GET_AUDIO_INFO                     (DECA_CMD_BASE + 15) //!< Get audio stream information
#define DECA_GET_HIGHEST_PTS                    (DECA_CMD_BASE + 16) //!< Get PTS highest threshold
#define DECA_MP3DEC_INIT                        (DECA_CMD_BASE + 17) //!< Not used any more
#define DECA_MP3DEC_CLOSE                       (DECA_CMD_BASE + 18) //!< Not used any more
#define DECA_MP3_CAN_DECODE                     (DECA_CMD_BASE + 19) //!< Not used any more
#define DECA_MP3_GET_ELAPSE_TIME                (DECA_CMD_BASE + 20) //!< Not used any more
#define DECA_MP3_JUMP_TIME                      (DECA_CMD_BASE + 21) //!< Not used any more
#define DECA_MP3_SET_TIME                       (DECA_CMD_BASE + 22) //!< Not used any more
#define DECA_MP3_IS_PLAY_END                    (DECA_CMD_BASE + 23) //!< Not used any more
#define DECA_PCM_FRM_LATE                       (DECA_CMD_BASE + 24) //!< Not used any more
#define DECA_SET_AV_SYNC_LEVEL                  (DECA_CMD_BASE + 25) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_REGISTER_CB                (DECA_CMD_BASE + 26) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_INIT                       (DECA_CMD_BASE + 27) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_CLOSE                      (DECA_CMD_BASE + 28) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_JUMP_TIME                  (DECA_CMD_BASE + 29) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_SET_TIME                   (DECA_CMD_BASE + 30) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_IS_PLAY_END                (DECA_CMD_BASE + 31) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_INIT2                      (DECA_CMD_BASE + 32) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_CLOSE2                     (DECA_CMD_BASE + 33) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_CAN_DECODE2                (DECA_CMD_BASE + 34) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_GET_ELAPSE_TIME2           (DECA_CMD_BASE + 35) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_GET_MUSIC_INFO2            (DECA_CMD_BASE + 36) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_JUMP_TIME2                 (DECA_CMD_BASE + 37) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_IS_PLAY_END2               (DECA_CMD_BASE + 38) //!< Only used for SEE side. Please do not use it.
#define DECA_SOFTDEC_REGISTER_CB2               (DECA_CMD_BASE + 39) //!< Not used any more
#define DECA_PLAY_MEDIA_STR                     (DECA_CMD_BASE + 40) //!< Not used any more
#define DECA_EMPTY_BS_SET                       (DECA_CMD_BASE + 41) //!< Clear bit stream mode flag
#define DECA_ADD_BS_SET                         (DECA_CMD_BASE + 42) //!< Add an audio stream type to support bit stream mode.
#define DECA_DEL_BS_SET                         (DECA_CMD_BASE + 43) //!< Delete an audio stream type to support bit stream mode.
#define DECA_IS_BS_MEMBER                       (DECA_CMD_BASE + 44) //!< Check the bit stream type if supporting bypass.
#define DECA_AUDIO_PTS_SYNC_STC                 (DECA_CMD_BASE + 45) //!< Not used any more
#define DECA_REG_PCM_PROCESS_FUNC               (DECA_CMD_BASE + 46) //!< Only used for SEE side. Please do not use it.
#define DECA_SYNC_BY_SOFT                       (DECA_CMD_BASE + 47) //!< Not used any more
#define DECA_DOLBYPLUS_CONVERT_ONOFF            (DECA_CMD_BASE + 48) //!< Set dolby plus convert enable flag
#define DECA_DOLBYPLUS_CONVERT_STATUS           (DECA_CMD_BASE + 49) //!< Get dolby plus convert status
#define DECA_RESET_BS_BUFF                      (DECA_CMD_BASE + 50) //!< Reset audio BS buffer
#define DECA_REG_PCM_BS_PROCESS_FUNC            (DECA_CMD_BASE + 51) //!< Only used for SEE side. Please do not use it.
#define DECA_GET_AUDIO_DECORE                   (DECA_CMD_BASE + 52) //!< Only used for SEE side. Please do not use it.
#define DECA_DOLBYPLUS_DEMO_ONOFF               (DECA_CMD_BASE + 53) //!< Set enable flag of dolby plus demo
#define DECA_SET_BUF_MODE                       (DECA_CMD_BASE + 54) //!< Not used any more
#define DECA_GET_BS_FRAME_LEN                   (DECA_CMD_BASE + 55) //!< Get the frame length of BS buffer in DVB mode
#define DECA_INDEPENDENT_DESC_ENABLE            (DECA_CMD_BASE + 56) //!< Set enable flag of audio narrator function in DVB mode
#define DECA_GET_DESC_STATUS                    (DECA_CMD_BASE + 57) //!< Get audio narrator function status in DVB mode
#define DECA_GET_DECODER_HANDLE                 (DECA_CMD_BASE + 58) //!< Get audio decoder handle
#define DECA_SYNC_NEXT_HEADER                   (DECA_CMD_BASE + 59) //!< Set next synchronization header flag
#define DECA_DO_DDP_CERTIFICATION               (DECA_CMD_BASE + 60) //!< Enable DDP certification flag
#define DECA_DYNAMIC_SND_DELAY                  (DECA_CMD_BASE + 61) //!< Not used any more
#define DECA_GET_DDP_INMOD                      (DECA_CMD_BASE + 62) //!< Get EAC3 stream inmod
#define DECA_GET_DECA_STATE                     (DECA_CMD_BASE + 63) //!< Get DECA device state
#define DECA_GET_DDP_PARAM                      (DECA_CMD_BASE + 64) //!< Not used any more
#define DECA_SET_DDP_PARAM                      (DECA_CMD_BASE + 65) //!< Not used any more
#define DECA_CONFIG_BS_BUFFER                   (DECA_CMD_BASE + 66) //!< Configure audio BS buffer
#define DECA_CONFIG_BS_LENGTH                   (DECA_CMD_BASE + 67) //!< Set audio BS buffer length
#define DECA_BS_BUFFER_RESUME                   (DECA_CMD_BASE + 68) //!< Resume audio BS buffer
#define DECA_PTS_DELAY                          (DECA_CMD_BASE + 69) //!< Not used any more
#define DECA_DOLBY_SET_VOLUME_DB                (DECA_CMD_BASE + 70) //!< Enable new dd+ certification flag
#define DECA_GET_PLAY_PARAM                     (DECA_CMD_BASE + 71) //!< Get audio play parameters, used with struct #cur_stream_info
#define DECA_MPEG_M8DB_ENABLE                   (DECA_CMD_BASE + 72) //!< Only used for SEE side. Please do not use it.
#define DECA_EABLE_INIT_TONE_VOICE              (DECA_CMD_BASE + 73) //!< Not used any more
#define DECA_EABLE_DVR_ENABLE                   (DECA_CMD_BASE + 74) //!< Not used any more
#define DECA_PCM_SIGNED_SET                     (DECA_CMD_BASE + 75)
#define DECA_GET_HDD_PLAYBACK                   (DECA_CMD_BASE + 76) //!< Only used for SEE side. Please do not use it.
#define DECA_PCM_HEAD_INFO_INIT                 (DECA_CMD_BASE + 77) //!< Only used for SEE side. Please do not use it.
#define SND_GET_DECA_CTRL_BLOCK                 (DECA_CMD_BASE + 78) //!< Only used for SEE side. Please do not use it.
#define DECA_GET_TS_MP3_INIT                    (DECA_CMD_BASE + 79) //!< Only used for SEE side. Please do not use it.
#define DECA_BEEP_TONE_MUTE_INTERVAL            (DECA_CMD_BASE + 80) //!< beep tone[beep==>mute==>beep==>mute...] mute time(ms)
#define DECA_BEEP_TONE_INTERVAL                 (DECA_CMD_BASE + 81) // !< beep tone[beep==>mute==>beep==>mute...] beep time(ms)
#define DECA_IO_REG_CALLBACK                    (DECA_CMD_BASE + 82) //!< Not used any more
#define DECA_CREATE_GET_CPU_DATA_TASK           (DECA_CMD_BASE + 83) //!< Not used any more
#define DECA_START_GET_CPU_DATA_TASK            (DECA_CMD_BASE + 84) //!< Not used any more
#define DECA_STOP_GET_CPU_DATA_TASK             (DECA_CMD_BASE + 85) //!< Not used any more

#define DECA_SET_BEY1_STREAM_NUMBER             (DECA_CMD_BASE + 86)
#define DECA_SOFTDEC_GET_CURRENT_INDEX          (DECA_CMD_BASE + 87)
#define DECA_SET_PCM_DECODER_PARAMS             (DECA_CMD_BASE + 88)
#define DECA_DATA_IO_REG_CALLBACK               (DECA_CMD_BASE + 89)
#define DECA_SET_DESC_STREAM_TYPE               (DECA_CMD_BASE + 99) //!< Set audio description stream type


#define DECA_GET_ES_BUFF_STATE                  (DECA_CMD_BASE + 100) //!< Get bit stream es buffer state
#define DECA_SET_CACHE_INVALID_FLAG             (DECA_CMD_BASE + 101) //!< Set PCM buffer data cacheable invalid flag
#define DECA_SET_QUICK_PLAY_MODE                (DECA_CMD_BASE + 102) //!< Set audio quick play mode

#define DECA_SET_BYPASS_INFO                    (DECA_CMD_BASE + 103) //!< Not used any more
#define DECA_PCM_DUMP_ON                        (DECA_CMD_BASE + 104) //!< Not used any more
#define DECA_PCM_DUMP_OFF                       (DECA_CMD_BASE + 105) //!< Not used any more
#define DECA_CHK_AUD_ENGINE_STATUS              (DECA_CMD_BASE + 106) //!< Get the audio CPU status. Only for 3921 in SEE side. Please do not use it.
#define DECA_INIT_AUD_ENGINE                    (DECA_CMD_BASE + 107) //!< Init audio CPU to decode. Only for 3921 in SEE side. Please do not use it.
#define DECA_START_AUD_ENGINE                   (DECA_CMD_BASE + 108) //!< Start audio CPU to decode. Only for 3921 in SEE side. Please do not use it.
#define DECA_DTS_FALL_BACK                      (DECA_CMD_BASE + 109) //!< DVB live play dts stream 7.1 fall back to 5.1 flag
#define DECA_CHANGE_AUD_TRACK                   (DECA_CMD_BASE + 110) //!< DVB live play change audio track flag
#define DECA_GET_DESC_STREAM_TYPE               (DECA_CMD_BASE + 111) //!< Get audio description stream type
#define DECA_GET_AUDIO_PCM_BUFF_START           (DECA_CMD_BASE + 112)
#define DECA_GET_AUDIO_PCM_BUFF_LEN             (DECA_CMD_BASE + 113)
#define DECA_GET_AUDIO_SUPPORT_STATUS           (DECA_CMD_BASE + 114)
#define DECA_SET_CAPTURE_DATA_TYPE				(DECA_CMD_BASE + 115)
#define DECA_GET_ENDIAN_FLAG					(DECA_CMD_BASE + 116)
#define DECA_SET_HE_AAC_ENABLE                  (DECA_CMD_BASE + 117)
#define DECA_GET_HE_AAC_ENABLE                  (DECA_CMD_BASE + 118)
#define DECA_GET_AAC_OPTIMIZATION_ENABLE        (DECA_CMD_BASE + 119)

#define DECA_ADV_IO                             (DECA_CMD_BASE + 0x200) //!< DECA ioctl command base
#define DECA_SET_REVERB                         (DECA_ADV_IO + 1) //!< Not used any more
#define DECA_SET_PL_II                          (DECA_ADV_IO + 2) //!< Not used any more
#define DECA_SET_AC3_MODE                       (DECA_ADV_IO + 3) //!< Set AC3 decoder mode, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define DECA_SET_AC3_STR_MODE                   (DECA_ADV_IO + 4) //!< Only used for SEE side. Please do not use it.
#define DECA_GET_AC3_BSMOD                      (DECA_ADV_IO + 5) //!< Get AC3 bit stream mode, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define SET_PASS_CI                             (DECA_ADV_IO + 6) //!< Set pass CI mode, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define DECA_CHECK_DECODER_COUNT                (DECA_ADV_IO + 7) //!< Get decode count, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define DECA_SET_DECODER_COUNT                  (DECA_ADV_IO + 8) //!< Set decode count, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define DECA_SET_AC3_COMP_MODE                  (DECA_ADV_IO + 9) //!< Set AC3 compare mode, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and #AUDIO_DECA_IO_COMMAND_ADV)
#define DECA_SET_AC3_STEREO_MODE                (DECA_ADV_IO + 10) //!< Set AC3 stereo mode, in main side must be used together with #AUDIO_DECA_IO_COMMAND or (#AUDIO_RPC_CALL_ADV and A#UDIO_DECA_IO_COMMAND_ADV)

#define DECA_SET_AVSYNC_MODE                    (DECA_ADV_IO + 11) //!< Not used any more
#define DECA_SET_AVSYNC_TEST                    (DECA_ADV_IO + 12) //!< Not used any more
#define DECA_SET_PTS_SHIFT                      (DECA_ADV_IO + 13) //!< Not used any more

#define DECA_GET_FREE_BSBUF_SIZE                (DECA_ADV_IO + 14) //!< Only used for SEE side. Please do not use it.
#define DECA_GET_BSBUF_SIZE                     (DECA_ADV_IO + 15) //!< Only used for SEE side. Please do not use it.
#define DECA_IO_GET_INPUT_CALLBACK_ROUTINE      (DECA_ADV_IO + 16) //!< Not used any more

#define DECA_GET_DBG_INFO                       (DECA_ADV_IO + 17) //!< get debug info. Only used for inside.

/*ALI audio sub IO command for MP*/
#define DECA_DECORE_IO                          (DECA_CMD_BASE  + 0x300) //!< Media player ioctl command base
#define DECA_DECORE_INIT                        (DECA_DECORE_IO + 1) //!< Initialize media player audio decoder, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_RLS                         (DECA_DECORE_IO + 2) //!< Release media player audio decoder, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_SET_BASE_TIME               (DECA_DECORE_IO + 3) //!< Set the audio play base time for media player, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_GET_PCM_TRD                 (DECA_DECORE_IO + 4) //!< Get the threshold value of pcm data size in PCM buffer
                                                                     //!< which can output to play, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_PAUSE_DECODE                (DECA_DECORE_IO + 5) //!< Pause or Resume media player to play audio, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_FLUSH                       (DECA_DECORE_IO + 6) //!< Reset media player audio decoder and sbm, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_SET_QUICK_MODE              (DECA_DECORE_IO + 7) //!< Set media player quick mode, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_SET_SYNC_MODE               (DECA_DECORE_IO + 8) //!< Set media player synchronization mode, in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_GET_CUR_TIME                (DECA_DECORE_IO + 9) //!< Get current audio play time of media player , in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_GET_STATUS                  (DECA_DECORE_IO + 10) //!< Get current audio play status of media player , in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)
#define DECA_DECORE_GET_SUPPORT_STATUS          (DECA_DECORE_IO + 11) //!< Get supported audio format of media player , in main side must be used together with #AUDIO_DECORE_COMMAND or (#AUDIO_RPC_CALL_ADV and #RPC_AUDIO_DECORE_IOCTL)


/*! @enum adec_sync_mode
@brief An enum defines synchronization modes of audio decoder.
*/
enum adec_sync_mode
{
    ADEC_SYNC_FREERUN = 1,    //Audio decoder just decode and send decoded frame to OUTPUT, not caring APTS and STC
    ADEC_SYNC_PTS,     //!< Audio decoder free run, but it will modify STC frequency according to the difference between STC value and APTS at output. And decoder needs to compare APTS of first audio frame with STC, and then to decide when to decode and send it to output.
    ADEC_SYNC_END,      //!< for hld input param check
};

/*! @enum adec_stop_mode
@brief An enum defines stop mode of audio decoder.
*/

enum adec_stop_mode
{
    ADEC_STOP_IMM = 1,    // Audio decoder stop immediately
    ADEC_STOP_PTS,        // Audio decoder stop according to PTS
    ADEC_STOP_END,        // No more data will be sent from parser to decoder,
                        // and decoder will stop automatically after handle
                        // all data
    ADEC_STOP_MODE_END,//for hld input param check
};


////////////////For media player/////////////////////

/*! @struct audio_config
@brief A structure defines the audio configuration information for audio decoder.
*/
struct audio_config
{
    INT32 decode_mode; //!< Audio decode mode.
    INT32 sync_mode; //!< Audio synchronization mode.
    INT32 sync_unit; //!< Synchronization unit.
    INT32 deca_input_sbm; //!< Audio input SBM ID.
    INT32 deca_output_sbm; //!< Audio output SBM ID.
    INT32 snd_input_sbm; //!< Sound input SBM ID.
    INT32 pcm_sbm; //!< PCM SBM ID.
    INT32 codec_id; //!< Audio codec ID.
    INT32 bits_per_coded_sample; //!< Bit numbers per one sample.
    INT32 sample_rate; //!< Sample rate of audio stream.
    INT32 channels; //!< Channel number of audio stream.
    INT32 bit_rate; //!< Bit rate of audio stream.
    UINT32 pcm_buf; //!< Buffer address storing PCM data.
    UINT32 pcm_buf_size; //!< Size of the PCM buffer.
    INT32 block_align; //!< Block align.
    UINT8 extra_data[512]; //!< Extradata for audio decoder.
    UINT32 codec_frame_size; //!< One frame data size.
    UINT32 extradata_size; //!< Extradata size.
    UINT8 extradata_mode; //!< Extradata mode.
    UINT8 cloud_game_prj; //!< Used for cloud game flag.
	UINT8 encrypt_mode;
};

/*! @struct audio_frame
@brief A structure defines the information of one frame data.
*/
struct audio_frame
{
    INT64 pts; //!< PTS of the frame.
    UINT32 size; //!< Frame size.
    UINT32 pos; //!< Frame position in the whole stream.
    UINT32 stc_id; //!< STC ID of the frame.
    UINT32 delay; //!< Frame delay time.
};


/*! @struct audio_decore_status
@brief A structure defines current audio decode status.
*/
struct audio_decore_status
{
    UINT32 sample_rate; //!< Audio sample rate.
    UINT32 channels; //!< Audio channel numbers.
    UINT32 bits_per_sample; //!< Bit numbers per one sample.
    INT32 first_header_got; //!< Not used any more.
    INT32 first_header_parsed; //!< Not used any more.
    UINT32 frames_decoded; //!< The frame numbers have been decoded.
};

/*! @enum AUDIO_AV_SYNC_MODE
@brief A structure defines audio AV synchronization mode. Not used any more.
*/
enum AUDIO_AV_SYNC_MODE
{
    AUDIO_AV_SYNC_NONE, //!< Not used any more.
    AUDIO_AV_SYNC_AUDIO, //!< Not used any more.
    AUDIO_AV_SYNC_VIDEO, //!< Not used any more.
    AUDIO_AV_SYNC_EXTERNAL, //!< Not used any more.
    AUDIO_AV_SYNC_AUDIO_N, //!< Not used any more.
    AUDIO_AV_SYNC_VIDEO_N, //!< Not used any more.
    AUDIO_AV_SYNC_EXTERNAL_N, //!< Not used any more.
};


/*! @struct deca_feature_config
@brief A structure defines the DECA device feature to be configured.
*/
struct deca_feature_config
{
    UINT8 detect_sprt_change;/*=1: if sample rate changed, audio decoder can detected it and re-config sound HW.*/
    UINT8 bs_buff_size  :3;    // power of bs buffer size = (1024 * 8) * (2^n)
    UINT8 support_desc  :1;
    UINT8 reserved      :4;
    UINT16 reserved16;
    UINT8 ad_static_mem_flag;
    UINT32 ad_static_mem_addr;
    UINT32 ad_static_mem_size;

    UINT32 dd_support_buff_addr;
    UINT32 dd_support_buff_size;

    UINT32 pcm_ring_buff_start;
    UINT32 pcm_ring_buff_len;
};

/*! @struct ase_str_play_param
@brief A structure is specially defined for DECA_STR_PLAY, The address of this structure address must be transferred to audio decoder in upper layer by IO control.
*/
struct ase_str_play_param
{
    UINT8 * src;
    UINT32 len;
    UINT32 loop_cnt;                //!< Play the stream (loop_cnt+1) time(s)
    UINT32 loop_interval;           //!< Play the stream repeatedly with the interval (loop_interval)ms
    UINT32 async_play: 1;           //!< =1, Call stream play and return immediately
    UINT32 reserved: 31;            //!< Reserved bits
    UINT32 need_notify;             //!< =1: After finishing playing stream, audio decoder will call the call_back_function to notify upper layer.
    void (*registered_cb) (void);   //!< Callback for control block
};


/*! @struct cur_stream_info
@brief A structure defines current stream information for DVB live play.
*/
struct cur_stream_info
{
    UINT8 str_type;             //!< Audio stream type
    UINT8 bit_depth;            //!< Audio stream bit depth
    UINT32 sample_rate;         //!< Audio stream sample rate
    UINT32 samp_num;            //!< Audio stream sample number
    UINT32 chan_num;            //!< Audio stream channel number
    UINT32 frm_cnt;             //!< Audio stream frame count
    UINT32 desc_frm_cnt;        //!< Audio description stream frame count
    UINT32 reserved1;           //!< Reserved bits
    UINT32 reserved2;           //!< Reserved bits
    UINT32 input_ts_cnt;        //!< TS packets input count
    UINT32 sync_error_cnt;      //!< Count of frame parsing errors
    UINT32 sync_success_cnt;    //!< Count of frames successfully parsed
    UINT32 sync_frm_len;        //!< Audio frame length
    UINT32 decode_error_cnt;    //!< Count of audio frame decoding errors
    UINT32 decode_success_cnt;  //!< Count of audio frames decoded successfully
    UINT32 cur_frm_pts;         //!< Audio stream current frame PTS
};

/*! @struct deca_buf_info
@brief A structure defines DECA buffer information.
*/
struct deca_buf_info
{
    UINT32 buf_base_addr;   //!< Real BS buffer address
    UINT32 buf_len;         //!< BS buffer length
    UINT32 used_len;        //!< Length used already
    UINT32 remain_len;      //!< Remaining length which has not been used yet
    UINT32 cb_rd;           //!< Read index for control block
    UINT32 cb_wt;           //!< Write index for control block
    UINT32 es_rd;           //!< Read index of BS buffer
    UINT32 es_wt;           //!< Write index of BS buffer
};

struct ali_audio_ioctl_tone_voice
{
    UINT32 buffer_add;//tone voice data address
    UINT32 buffer_len;//tone voice data length
};


struct deca_device
{
    struct deca_device  *next;  /*next device */
    /*struct module *owner;*/
    INT32 type;
    INT8 name[HLD_MAX_NAME_SIZE];
    INT32 flags; //This field used to record current running status

    INT32 hardware;
    INT32 busy;   //This field used to record sub state of DECA_STATE_PLAY, could be: busy, no data, no buffer.
    INT32 minor;//This field used to record previous running status

    void *priv;        /* Used to be 'private' but that upsets C++ */
    UINT32 base_addr;
    INT32 ase_flags; //This field used to record current running status of ASE task
    UINT32 standby_cmd;
    UINT32 ae_standby_status;
	UINT8 stop_cnt;

    void             (*attach)(struct deca_feature_config *);
    void             (*detach)(struct deca_device **);
    RET_CODE   (*open)(struct deca_device *, enum audio_stream_type, \
                       enum audio_sample_rate, enum audio_quantization, \
               UINT8, UINT32, const UINT8 *);
    RET_CODE   (*close)(struct deca_device *);
    RET_CODE   (*start)(struct deca_device *, UINT32);
    RET_CODE   (*stop)(struct deca_device *, UINT32, enum adec_stop_mode);
    RET_CODE   (*pause)(struct deca_device *);
    RET_CODE   (*set_sync_mode)(struct deca_device *, enum adec_sync_mode);
    RET_CODE   (*ioctl)(struct deca_device *, UINT32 , UINT32);
    RET_CODE   (*request_write)(struct deca_device *, UINT32, void **, UINT32 *, struct control_block *);
    void       (*update_write)(struct deca_device *, UINT32);
    void (*pcm_buf_resume)(struct deca_device *);
    void (*tone_voice)(struct deca_device *, UINT32, UINT32);   //tone voice
    void (*stop_tone_voice)(struct deca_device *);
    RET_CODE   (*ase_cmd)(struct deca_device *, UINT32 , UINT32);
    /* add for audio description*/
    RET_CODE   (*request_desc_write)(struct deca_device *, UINT32, void **, UINT32 *, struct control_block *);
    void             (*update_desc_write)(struct deca_device *, UINT32);
    void    (*ase_init)(struct deca_device *);
    UINT32    (*standby)(struct deca_device *, UINT32);

    RET_CODE   (*set_dbg_level)      (struct deca_device *,UINT32);
    // add by jacket for s3921
    RET_CODE   (*ae_decode)          (struct deca_device *, void *, void *);
    RET_CODE   (*ae_reset)           (struct deca_device *);
};

typedef struct st_deca_inject
{
    struct deca_device *pst_dev_deca;
    int i_sbm_idx;

} deca_inject, *pdeca_inject;


RET_CODE deca_set_dd_plugin_addr(struct deca_device * dev, const UINT8 *dd_addr);
RET_CODE deca_open(struct deca_device *dev, enum audio_stream_type stream_type,
                                enum audio_sample_rate samp_rate, enum audio_quantization quan,
                                UINT8 channel_num, UINT32 info_struct);
RET_CODE deca_close(struct deca_device *dev);
RET_CODE deca_start(struct deca_device *dev, UINT32 high32_pts);
RET_CODE deca_stop(struct deca_device *dev, UINT32 high32_pts, enum adec_stop_mode mode);
RET_CODE deca_pause(struct deca_device *dev);
RET_CODE deca_io_control(struct deca_device *dev, UINT32 cmd, UINT32 param);
RET_CODE deca_set_sync_mode(struct deca_device *dev, enum adec_sync_mode mode);
void deca_tone_voice(struct deca_device *dev, UINT32 snr, UINT32 init);  //tone voice
void deca_stop_tone_voice(struct deca_device *dev);  //tone voice
void deca_init_ase(struct deca_device *dev);
UINT32 deca_standby(struct deca_device *dev, UINT32 status);

RET_CODE deca_decore_ioctl(struct deca_device *dev, UINT32 cmd, void *param1, void *param2);
UINT32 deca_standby(struct deca_device *dev, UINT32 status);
RET_CODE deca_set_dbg_level(struct deca_device *dev,UINT32 option);

void deca_m36_attach(struct deca_feature_config *config);
void deca_m36_dvr_enable(struct deca_device *dev);
void deca_m36_ext_dec_enable(struct deca_device *dev, struct deca_feature_config *config);
void deca_m36_init_tone_voice(struct deca_device *dev);

//////////////////New re-ordered definition for DECA part ///////////////////////



#endif

