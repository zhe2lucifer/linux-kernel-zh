#ifndef __ADF_MEDIA__
#define __ADF_MEDIA__

#include "adf_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! @addtogroup OSAL
 *  @{
 */

enum tvsystem
{
    PAL = 0,        //PAL4.43(==PAL_BDGHI)        (Fh=15.625,fv=50)
    NTSC,           //NTSC3.58                    (Fh=15.734,Fv=59.94)
    PAL_M,          //PAL3.58                     (Fh=15.734,Fv=59.94)
    PAL_N,          //PAL4.43(changed PAL mode)   (Fh=15.625,fv=50)
    PAL_60,         //                            (Fh=15.734,Fv=59.94)
    NTSC_443,       //NTSC4.43                    (Fh=15.734,Fv=59.94)
    SECAM,
    MAC,
    LINE_720_25,
    LINE_720_30,
    LINE_1080_25,
    LINE_1080_30,
    LINE_1080_50,
    LINE_1080_60,
    LINE_1080_24,
    LINE_1152_ASS,
    LINE_1080_ASS,
    PAL_NC,         //PAL3.58(changed PAL mode)    (Fh=15.625,fv=50)
    LINE_576P_50_VESA,
    LINE_720P_60_VESA,
    LINE_1080P_60_VESA,
    LINE_4096X2160_24,
    LINE_3840X2160_24,
    LINE_3840X2160_25,
    LINE_3840X2160_30,

    LINE_480_60,
    LINE_480_120,
    LINE_480_240,
    LINE_576_50,
    LINE_576_100,
    LINE_576_200,
    LINE_720_24,
    LINE_720_25_EXT,    //720p25
    LINE_720_30_EXT,    //720p30
    LINE_720_100,
    LINE_720_120,
    LINE_1080_100_I,
    LINE_1080_100,
    LINE_1080_120_I,
    LINE_1080_120,
    LINE_3840X2160_50,
    LINE_3840X2160_60,
    LINE_4096X2160_25,
    LINE_4096X2160_30,
    LINE_4096X2160_50,
    LINE_4096X2160_60,   



    TV_SYS_INVALID = 0xFF,
};

enum audio_stream_type
{
    AUDIO_INVALID,
    AUDIO_MPEG1,        //MPEG I
    AUDIO_MPEG2,        //MPEG II
    AUDIO_MPEG_AAC,
    AUDIO_AC3,          //AC-3
    AUDIO_DTS,          //DTS audio for DVD-Video
    AUDIO_PPCM,         //Packet PCM for DVD-Audio
    AUDIO_LPCM_V,       //Linear PCM audio for DVD-Video
    AUDIO_LPCM_A,       //Linear PCM audio for DVD-Audio
    AUDIO_PCM,          //PCM audio
    AUDIO_BYE1,         //BYE1 audio
    AUDIO_RA8,          //Real audio 8
    AUDIO_MP3,          //MP3 audio
    AUDIO_MPEG_ADTS_AAC,
    AUDIO_OGG,
    AUDIO_EC3,
    AUDIO_MP3_L3,       //for TS MPEG Layer 3
	AUDIO_MP3_2,
	AUDIO_MP2_2,
	AUDIO_PCM_RAW,      //PCM DECODE PARAMS SET BY APP
    AUDIO_DTSHD,
    AUDIO_DOLBY_PULSE,
    AUDIO_ADPCM,
    AUDIO_APE,
    AUDIO_FLAC,
    AUDIO_BYE1PRO,
    AUDIO_VORBIS,
    AUDIO_AMR,
    AUDIO_ALAC,
    AUDIO_STREAM_TYPE_END,
};

/** New fefined for normal used audio sample rate  **/
enum audio_sample_rate
{
    AUDIO_SAMPLE_RATE_INVALID = 1,    // Invalid sample rate
    AUDIO_SAMPLE_RATE_8,        // 8 KHz
    AUDIO_SAMPLE_RATE_11,        // 11.025 KHz
    AUDIO_SAMPLE_RATE_12,        // 12 KHz
    AUDIO_SAMPLE_RATE_16,        // 16 KHz
    AUDIO_SAMPLE_RATE_22,        // 22.05 KHz
    AUDIO_SAMPLE_RATE_24,        // 24 KHz
    AUDIO_SAMPLE_RATE_32,          // 32 KHz
    AUDIO_SAMPLE_RATE_44,          // 44.1 KHz
    AUDIO_SAMPLE_RATE_48,        // 48 KHz
    AUDIO_SAMPLE_RATE_64,        // 64 KHz
    AUDIO_SAMPLE_RATE_88,        // 88.2 KHz
    AUDIO_SAMPLE_RATE_96,        // 96 KHz
    AUDIO_SAMPLE_RATE_128,        // 128 KHz
    AUDIO_SAMPLE_RATE_176,        // 176.4 KHz
    AUDIO_SAMPLE_RATE_192,        // 192 KHz
    AUDIO_SAMPLE_RATE_END,        //for hld input param check
};

/** New defined for quantization type  **/
enum audio_quantization
{
    AUDIO_QWLEN_INVALID = 1,
    AUDIO_QWLEN_8,            // 8 Bits
    AUDIO_QWLEN_12,            // 12 Bits
    AUDIO_QWLEN_16,            // 16 Bits
    AUDIO_QWLEN_20,            // 20 Bits
    AUDIO_QWLEN_24,            // 24 Bits
    AUDIO_QWLEN_32,            // 32 Bits
    AUDIO_QWLEN_END,           //for hld input param check
};

enum asp_ratio
{
	DAR_FORBIDDEN = 0,  //aspect ratio forbidden
	SAR, 				//sample aspect ratio
	DAR_4_3,			//Display aspect ratio 3/4
	DAR_16_9,			//Display aspect ratio 9/16
	DAR_221_1			//Display aspect ratio 1/2.11
};

enum tvmode
{
	TV_4_3 = 0,
	TV_16_9,
	TV_AUTO	            //060517 yuchun for GMI Aspect Auto
};

enum display_mode
{
	PANSCAN = 0,		// default panscan is 16:9 source on 4:3 TV.
	PANSCAN_NOLINEAR,	// non-linear pan&scan
	LETTERBOX,
	TWOSPEED,
	PILLBOX,
	VERTICALCUT,
	NORMAL_SCALE,
	LETTERBOX149,
	AFDZOOM,
	PANSCAN43ON169,		// 4:3 source panscan on 16:9 TV.
	COMBINED_SCALE,
	DONT_CARE,
    VERTICALCUT_149,
};

enum pic_fmt
{
	YCBCR_411,
	YCBCR_420,
	YCBCR_422,
	YCBCR_444,
	RGB_MODE1,		    //rgb (16-235)
	RGB_MODE2,		    //rgb (0-255)
    Y_ONLY,
    C_ONLY,
    HW_CFG,
    RGB888,
    ARGB8888,
    RGB4444,
    ARGB4444,
    RGB555,
    ARGB1555,
    RGB565,
    CLUT2,
    CLUT4,
    CLUT8,
    ACLUT88,
    TILE_MEM,
    YCbCr888,
    AYCbCr8888,
    YCbCr4444,
    AYCbCr4444,
    YCbCr555,
    AYCbCr1555,
    YCbCr565,
    PACKET_422
};

enum deinterlacing_alg
{
	NORMAL_WEAVE = 0,
	HALF_PHASE_WEAVE,
	NORMAL_BOB,
	SINGLE_FIELD_BOB,
	NORMAL_MAF,
	SINGLE_FIELD_MAF,
	NORMAL_BOB_CONV,
	DI_WEAVE,
	PURE_BOB,
	EPEC_PMF,
	EPEC_BOB,
	EPEC_MAF,
	EPEC_PMF_BOB,
	EPEC_PMF_WEAVE
};

enum output_frame_ret_code
{
	ret_output_success,
	ret_no_decoded_frame
};
typedef enum _MAPMPING_MODE
{
    MAPPING_MODE_MPEG2,
    MAPPING_MODE_H264,
    MAPPING_MODE_RASTER,
    MAPPING_MODE_H265_INTERLACE,
    MAPPING_MODE_H265_PROGRESSIVE,
    MAPPING_MODE_SEMI_420,
    MAPPING_MODE_SEMI_422,
}t_e_mapping_mode;

typedef enum output_frame_ret_code(* t_mprequest)(void *display_info);
typedef BOOL (* t_mprelease)(UINT8, UINT8);

typedef enum output_frame_ret_code(* t_request_ext)(void *,void *);
typedef BOOL (* t_release_ext)(void*,UINT8, UINT8);

typedef enum output_frame_ret_code(* t_request)(void *dev, void *display_info, void *request_info);
typedef RET_CODE (* t_release)(void *, void *release_info);

typedef RET_CODE (* t_vblanking)(void *, void *vblanking_info);

typedef void (* t_vbirequest)(UINT8 field_polar);

struct position
{
    UINT16 u_x;  //!< Horizontal position.
    UINT16 u_y;  //!< Vertical position.
};

struct rect_size
{
    UINT16 u_width;  //!< Horizontal size.
    UINT16 u_height; //!< Vertical size.
};

struct rect
{
#if (defined(_MHEG5_SUPPORT_) || defined(_MHEG5_V20_ENABLE_))
    INT16 u_start_x;    //!< Horizontal start point.
    INT16 u_start_y;    //!< Vertical start point.
#else
    UINT16 u_start_x;   //!< Horizontal start point.
    UINT16 u_start_y;   //!< Vertical start point.
#endif
    UINT16 u_width;     //!< Horizontal size.
    UINT16 u_height;    //!< Vertical size.
};

struct ycb_cr_color
{
    UINT8 u_y;
    UINT8 u_cb;
    UINT8 u_cr;
};

#define     VOB_START_MASK              0x01    // used with control_block.vob_start
#define     VOB_END_MASK                0x02    // used with control_block.vob_start
typedef struct control_block
{
    UINT8 stc_id_valid;   // 1: valid, 0: invalid
    UINT8 pts_valid;      // 1:valid, 0:invalid
    UINT8 data_continue;  // 1:not continue, 0: continue
    UINT8 ctrlblk_valid;  // 1:valid, 0: invalid
    UINT8 instant_update; // provided by dec, 1:need update instantly, 0: NOT
    UINT8 vob_start;
    UINT8 pts_start;      // 1:Identify one new pts reach
	UINT8 reserve;
    UINT8 stc_id;
    UINT32 pts;
	UINT8 bstream_run_back;
	UINT8 stc_offset_idx;
}st_control_block;

struct mpsource_call_back
{
	struct vdec_device *handler;
	t_mprequest	request_callback;
	t_mprelease	release_callback;
	t_vblanking vblanking_callback;
};

struct pipsource_call_back
{
	t_request_ext request_callback;
	t_release_ext release_callback;
	t_vblanking vblanking_callback;
    struct vdec_device *handler;
};

struct source_callback
{
	struct vdec_device *handler;
	t_request request_callback;
	t_release release_callback;
	t_vblanking vblanking_callback;
};

struct time_code_t
{
    UINT8 hour;
    UINT8 minute;
    UINT8 second;
    UINT8 frame;
};

/*!
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif

