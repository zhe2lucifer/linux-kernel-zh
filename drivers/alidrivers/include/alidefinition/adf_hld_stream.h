/*****************************************************************************
    History:
    2004-10-29 Hudson   First Created
*****************************************************************************/

#ifndef __ADF_HLD_STREAM_H
#define __ADF_HLD_STREAM_H


//#include "common.h"

#if 0   // use libyamad.a
struct stream
{
    uint8 *buffer;
    int nbyte_buffer;
    uint8 *next_frame;
    int nbyte_left;
};

void yamad_init_stream(struct stream *stream);
void yamad_stream_in(struct stream *stream, uint8 *buf, int nbyte);
void yamad_stream_out(struct stream *stream, int nbyte);
int yamad_sync_stream(struct stream *stream);


INLINE
int yamad_stream_query(struct stream *stream)
{
    return stream->nbyte_left;
}

#else   // use mpgdec.a ,  ac3dec.a

#define DATA_PRECISION  24  //16
#if(DATA_PRECISION == 24)
    #define SDATA long
#else
    #define SDATA short
#endif


typedef struct
{
    UINT32 version_num;     // version number
    UINT32 codec_feature;   // codec feature  
    UINT8 reserved[20];     // reserved for future uses 
    UINT8 version_str[100]; // version string, including lib name, build date etc.
} AUD_CODEC_LIB_INFO; // 128 bytes


typedef struct
{
    int offset; //uint is bit
    int bits; //uint is bit
}ANCILLARY_DATA;



/*------------------------------------------------*/
/*                downmix lib                   */
/*------------------------------------------------*/

#define DOWNMIX_CEOF 724 // 0.707 * 2^10

typedef enum
{
    DOWNMIX_OK = 0,                  // Downmix is successful
    DOWNMIX_INVALID_BITSPERSAMPLE,   // not belong to {8, 16, 24, 32}
    DOWNMIX_INVALID_CHANNELS,        // not belong to {1, 2, 3, 4, 5, 6}
    DOWNMIX_ERR_INBUFSZ,             // less than (sample_num) samples in the buffer
    DOWNMIX_ERR_OUTBUFSZ             // no enough space to store downmix result
}DOWNMIX_STATUS;

typedef struct
{
    INT8       *in_buf;          // the input buffer 
    INT8       *out_buf;         // the output buffer
    UINT32     in_buf_sz;        // the size of the input buffer
    UINT32     out_buf_sz;       // the size of the output buffer
    UINT32     sample_num;       // the number of samples in the input buffer
    UINT16     bits_per_sample;  // 
    UINT16     channels;         // 
}DOWNMIX_BITSTREAM_IN;

typedef struct
{
    DOWNMIX_STATUS   status;

}DOWNMIX_BITSTREAM_OUT;


INT32 downmix_bitstream(DOWNMIX_BITSTREAM_IN* p_in_params, 
                        DOWNMIX_BITSTREAM_OUT* p_out_params);


/*------------------------------------------------*/
/*                MPEG2 decoder                   */
/*------------------------------------------------*/


typedef struct
{
    UINT8 status;
    UINT8 bit_depth;
    UINT16 sample_rate;
    UINT16 samp_num;
    UINT16 left_data;
    UINT16 left_extdata;
    UINT16 chan_num;
    UINT16 samprateid;
    UINT16 inmod;
} DECOUTSIP_MPG;


typedef struct
{
    UINT8 *input_ptr;
    UINT16 bs_length;
    INT32 *output_ptr;
    UINT8 *extbs_ptr;
    UINT16 extbs_length;
    UINT8 ismpeg1;
} DECINSIP_MPG;


void mpg_decode_init(void);
DECOUTSIP_MPG mpg_decode_one_frame(DECINSIP_MPG mpg_para);
void mpg_get_ancillary_data(ANCILLARY_DATA *anc_data);
void mpg_decode_finish(void);

void mpg_decode_init_ad(void *temp);
DECOUTSIP_MPG mpg_decode_one_frame_ad(void *temp, DECINSIP_MPG mpg_para);
void mpg_get_ancillary_data_ad(void *temp, ANCILLARY_DATA *anc_data);
void mpg_decode_finish_ad(void *temp);



/*------------------------------------------------*/
/*               AC3 decoder                      */
/*------------------------------------------------*/


typedef struct
{
    UINT8 status;
    UINT8 bit_depth;
    UINT16 sample_rate;
    UINT16 samp_num;
    UINT16 left_data;
    UINT16 chan_num;
    UINT16 samprateid;
    UINT16 inmod;
    UINT16 dsurmod;
} DECOUTSIP_AC3;


typedef struct
{
    UINT8 *input_ptr;
    UINT16 bs_length;
    INT32 *output_ptr;
} DECINSIP_AC3;


void ac3_decode_init(void);
DECOUTSIP_AC3 ac3_decode_one_frame(DECINSIP_AC3 ac3_para);
void ac3_decode_finish(void);
UINT8 ac3_get_bs_info(UINT8 *p_info);
UINT8 ac3_set_pcm_scale(UINT32 ac3_pcmscale);
UINT8 ac3_set_drc_scale_low(UINT32 ac3_scalelow);
UINT8 ac3_set_drc_scale_high(UINT32 ac3_scalehigh);
UINT8 ac3_set_compression_mode(UINT8 ac3_compmod);
UINT8 ac3_set_dual_mode(UINT8 ac3_dualmod);
UINT8 ac3_set_karaoke_mode(UINT8 ac3_karaokemod);
UINT8 ac3_set_kcapable_mode(UINT8 ac3_kcapablemod);
UINT8 ac3_set_output_mode(UINT8 ac3_outputmod);
UINT8 ac3_set_stereo_mode(UINT8 ac3_stereomod);
UINT8 ac3_set_out_lfe(UINT8 ac3_outlfe);

void ac3_decode_init_ad(void *temp);
DECOUTSIP_AC3 ac3_decode_one_frame_ad(void *temp, DECINSIP_AC3 ac3_para);
void ac3_decode_finish_ad(void *temp);
UINT8 ac3_get_bs_info_ad(void *temp, UINT8 *p_info);
UINT8 ac3_set_pcm_scale_ad(void *temp, UINT32 ac3_pcmscale);
UINT8 ac3_set_drc_scale_low_ad(void *temp, UINT32 ac3_scalelow);
UINT8 ac3_set_drc_scale_high_ad(void *temp, UINT32 ac3_scalehigh);
UINT8 ac3_set_compression_mode_ad(void *temp, UINT8 ac3_compmod);
UINT8 ac3_set_dual_mode_ad(void *temp, UINT8 ac3_dualmod);
UINT8 ac3_set_karaoke_mode_ad(void *temp, UINT8 ac3_karaokemod);
UINT8 ac3_set_kcapable_mode_ad(void *temp, UINT8 ac3_kcapablemod);
UINT8 ac3_set_output_mode_ad(void *temp, UINT8 ac3_outputmod);
UINT8 ac3_set_stereo_mode_ad(void *temp, UINT8 ac3_stereomod);
UINT8 ac3_set_out_lfe_ad(void *temp, UINT8 ac3_outlfe);

UINT8 get_ac3dec_lib_info(AUD_CODEC_LIB_INFO *info);



/*------------------------------------------------*/
/*               MP3 decoder                      */
/*------------------------------------------------*/

typedef struct
{
    unsigned char *p_input_bs;       // pointer to the input bitsteam buffer
    int *pcm_out;                    // pointer to the output pcm buffer
    int input_buf_len;               // size in bytes of the input bitsteam buffer
} DECINSIP_MP3;


typedef enum
{
    MP3_SAI_OK = 0,                  // function completed successfully                           
    MP3_SAI_SYNC_LOST = -2147483640, // 0x80000000,  // can't find syncword in bit stream                         
    MP3_SAI_INVALID_MAIN_DATA_BEGIN, // main data begin is nor right to process the current frame 
    MP3_SAI_FREE_BITRATE_FRAME_ERROR,// when bitrate_index=0,can't fine correct syncword          
    MP3_INVALID_INPUT_BUFFER_SIZE,   // invalid input buffer for p_input_bs                       
    MP3_SAI_INVALID_ID,              // invalid id in bit stream                                  
    MP3_SAI_INVALID_LAYER,           // invalid layer in bit stream                               
    MP3_SAI_BAD_BITRATE_INDEX,       // bitrate index is equal to 0xf                             
    MP3_SAI_INVALID_SAMPRATE,        // invalid sample rate in bit stream                         
    MIPS_SAI_MP3_FREE_BITRATE,       // free mode                                                 
    MP3_SAI_BAD_CRC,                 // invalid CRC                                               
    MP3_SAI_INVALID_IN_PTR,          // invalid input buffer pointer                              
    MP3_SAI_INVALID_OUT_PTR,         // invalid output buffer pointer(s)                          
    MP3_SAI_INVALID_OUT_FMT,         // invalid output buffer format                              
    MP3_SAI_NOT_READY,               // decoder not ready to perform operation                    
    MP3_SAI_STATUS_OK,               // decoder operating normally                                
    MP3_SAI_STATUS_FRAME_MUTED,      // non-fatal error resulted in muted frame
    MP3_SAI_STATUS_BR_BUF_ERROR,     // Br buf Store previous main data happens error
    MP3_SAI_GET_FRAME_ERROR          // get frame happens error
} DEC_STATUS;


typedef struct
{
    DEC_STATUS status_mp3;
    int sample_rate;
    int sample_num;
    int chan_num;
    int frame_size;
    int layer;
} DECOUTSIP_MP3;


void mp3_decoder_init(void *mp3_dec);
int mp3_decode_one_frame(DECINSIP_MP3 *p_in_params, void *mp3_dec, DECOUTSIP_MP3 *p_out_params);
unsigned char get_mp3dec_lib_info(AUD_CODEC_LIB_INFO *info);
void get_mepg_ancillary_data(void *temp, ANCILLARY_DATA *anc_data);



/*------------------------------------------------*/
/*               EAC3 decoder                     */
/*------------------------------------------------*/


typedef struct
{
    UINT8    status;
    UINT8    bit_depth;
    UINT16   sample_rate;
    UINT16   samp_num;
    UINT16   left_data;
    UINT16   chan_num;
    UINT16   samplerateid;
    UINT16   inmod;
    UINT16   dsurmod;
    UINT16   output_ac3_bytes;
} DECOUTSIP_EAC3;


typedef struct
{
    UINT8   *input_ptr;
    UINT16  bs_length;
    INT32   *output_ptr;
    UINT8   *output_ac3_ptr;
    UINT8   is_ac3_file;
    short   remain_time;
    short   diff_dmfac;
} DECINSIP_EAC3;


void ec3_decode_init(void);
DECOUTSIP_EAC3 ec3_decode_one_frame(DECINSIP_EAC3 eac3_para);
void ec3_decode_finish(void);
UINT8 ec3_set_dual_mode(UINT8 eac3_dualmod);
UINT8 ec3_set_compression_mode(UINT8 eac3_compmod);
UINT8 ec3_set_out_lfe(UINT8 eac3_outlfe);
UINT8 ec3_set_output_mode(UINT8 eac3_outputmod);
UINT8 ec3_set_stereo_mode(UINT8 eac3_stereomod);
UINT8 ec3_set_pcm_scale(UINT32 eac3_pcmscale);
UINT8 ec3_set_drc_scale_low(UINT32 eac3_scalelow);
UINT8 ec3_set_drc_scale_high(UINT32 eac3_scalehigh);

void ec3_decode_init_ad(void *temp);
DECOUTSIP_EAC3 ec3_decode_one_frame_ad(void *temp, DECINSIP_EAC3 eac3_para);
void ec3_decode_finish_ad(void *temp);
UINT8 ec3_set_dual_mode_ad(void *temp, UINT8 eac3_dualmod);
UINT8 ec3_set_compression_mode_ad(void *temp, UINT8 eac3_compmod);
UINT8 ec3_set_out_lfe_ad(void *temp, UINT8 eac3_outlfe);
UINT8 ec3_set_output_mode_ad(void *temp, UINT8 eac3_outputmod);
UINT8 ec3_set_stereo_mode_ad(void *temp, UINT8 eac3_stereomod);
UINT8 ec3_set_pcm_scale_ad(void *temp, UINT32 eac3_pcmscale);
UINT8 ec3_set_drc_scale_low_ad(void *temp, UINT32 eac3_scalelow);
UINT8 ec3_set_drc_scale_high_ad(void *temp, UINT32 eac3_scalehigh);

UINT8 get_ec3dec_lib_info(AUD_CODEC_LIB_INFO *info);



/*------------------------------------------------*/
/*               AAC decoder                     */
/*------------------------------------------------*/
#if 0

#define AAC_ERR_NONE       0      // decoder one frame successfully
#define AAC_ERR_SYNC       1      // not find SYNC word, so need more data
#define AAC_ERR_CRC        2      // SYNC OK, but CRC error
#define AAC_ERR_LENGTH     3      // SYNC OK, but need more data
#define AAC_ERR_PTR        4      // involid input or output buffer pointer
#define AAC_ERR_DECODE     5      // fatal error when decode one frame


typedef struct
{
    UINT8 status;
    UINT8 bit_depth;
    UINT16 sample_rate;
    UINT16 samp_num;
    UINT16 left_data;
    UINT16 chan_num;
} DECOUTSIP_AAC;


typedef struct
{
    UINT8 *input_ptr;
    UINT16 bs_length;
    INT32 *output_ptr;
    UINT16 fs_index;
    UINT16 channel_num;
    UINT16 stream_type;
    UINT16 optimize_enable;
} DECINSIP_AAC;



UINT8 aac_set_drc_params(UINT32 aac_scalehi, UINT32 aac_scalelo, INT32 aac_reflevel);
UINT8 aac_decode_init(void);
DECOUTSIP_AAC aac_decode_one_frame(DECINSIP_AAC aac_para);
void aac_get_ancillary_data(ANCILLARY_DATA *anc_data);
void aac_decode_finish(void);

UINT8 aac_set_drc_params_ad(void *temp, UINT32 aac_scalehi, UINT32 aac_scalelo, INT32 aac_reflevel);
void aac_decode_init_ad(void *temp);
DECOUTSIP_AAC aac_decode_one_frame_ad(void *temp, DECINSIP_AAC aac_para);
void aac_get_ancillary_data_ad(void *temp, ANCILLARY_DATA *anc_data);
void aac_decode_finish_ad(void *temp);

UINT8 get_aacdec_lib_info(AUD_CODEC_LIB_INFO *info); 
#endif

#endif



typedef struct
{
    enum AudioStreamType type;
    void (* deca_audio_init)(void);
    BYTE (* set_comp_mode)(BYTE);
    BYTE (* set_stereo_mode)(BYTE);
    BOOL  (*frame_decoder)(void* input,void* output);
    BYTE (* set_dual_mode)(BYTE);
    BYTE (* set_out_lfe)(BYTE);
    BYTE (* set_output_mode)(BYTE);
    BYTE (* set_pcm_scale)(UINT32);
    BYTE (* set_dyn_scale_low)(UINT32);
    BYTE (* set_dyn_scale_high)(UINT32);
    BYTE (*aac_set_drc_params)(void *decoder, unsigned long aac_scalehi, unsigned long aac_scalelo, long aac_reflevel);
    BYTE (*get_dec_lib_info)(AUD_CODEC_LIB_INFO *info);
    void (* get_ancillary_data)(void* temp, ANCILLARY_DATA *anc_data);
}deca_audiostream_decoder;


typedef struct
{
    enum AudioStreamType type;
    void (* deca_audio_init)(void*);
    BYTE (* set_comp_mode)(void*decoder,BYTE);
    BYTE (* set_stereo_mode)(void*decoder,BYTE);
    BOOL  (*frame_decoder)(void* input,void* output,void*decoder);
    BYTE (* set_dual_mode)(void*,BYTE);
    BYTE (* set_out_lfe)(void*,BYTE);
    BYTE (* set_output_mode)(void*,BYTE);
    BYTE (* set_pcm_scale)(void*,UINT32);
    BYTE (* set_dyn_scale_low)(void*,UINT32);
    BYTE (* set_dyn_scale_high)(void*,UINT32);
    void (*aac_set_drc_params)(void *decoder, unsigned long aac_scalehi, unsigned long aac_scalelo, long aac_reflevel);
    BYTE (*get_dec_lib_info)(AUD_CODEC_LIB_INFO *info);
    void (* get_ancillary_data)(void* temp, ANCILLARY_DATA *anc_data);
}deca_audiostream_decoder_ad;


#define _deca_audiostream_decoder(x) const deca_audiostream_decoder x __attribute__ ((section(".deca.init")))
#define _deca_audiostream_decoder_ad(x) const deca_audiostream_decoder_ad x __attribute__ ((section(".deca.init")))

//#if defined(_ATSC_CONVERTER_BOX_E_) || defined(_ATSC_CONVERTER_BOX_D_)
//#define _deca_audiostream_decoder(x) const deca_audiostream_decoder x __attribute__ ((section(".deca.plugin")))





#endif /* STREAM_H */

