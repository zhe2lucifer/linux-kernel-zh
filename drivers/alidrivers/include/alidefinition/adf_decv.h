#ifndef __ADF_DECV__
#define __ADF_DECV__

/*! @addtogroup DeviceDriver
 *  @{
 */

/*! @addtogroup ALiDECV
 *  @{
 */

#include "adf_basic.h"
#include "adf_media.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VDEC_DEV_MAX                        2

#define VDEC_SYNC_PTS                       0x00    //!<Open audio/video synchronization function.
#define VDEC_SYNC_FREERUN                   0x01    //!<Close audio/video synchronization function.

#define VDEC_SYNC_FIRSTFREERUN              0x01    //!<Free run the first video frame.
#define VDEC_SYNC_I                         0x02    //!<Not supported now.
#define VDEC_SYNC_P                         0x04    //!<Not supported now.
#define VDEC_SYNC_B                         0x08    //!<Not supported now.
#define VDEC_SYNC_HIGH_LEVEL                0x10    //!<Not supported now.

#define VDEC_IO_FILL_FRM                    0x03    //!<Not supported now.
#define VDEC_IO_REG_CALLBACK                0x04    //!<Register callback function.
#define VDEC_IO_GET_STATUS                  0x05    //!<Read DECV module status information.
#define VDEC_IO_GET_MODE                    0x06    //!<Read video source mode.
#define VDEC_IO_GET_FRM                     0x07    //!<Not supported now.
#define VDEC_IO_WHY_IDLE                    0x08    //!<Not supported now.
#define VDEC_IO_GET_FREEBUF                 0x0A    //!<Not supported now.
#define VDEC_IO_GET_FRM_ADVANCE             0x0D    //!<Not supported now.
#define VDEC_IO_FILL_FRM2                   0x10    //!<Not supported now.
#define VDEC_IO_PULLDOWN_ONOFF              0x11    //!<Not supported now.
#define VDEC_IO_COLORBAR                    0x14    //!<Display colorbar
#define VDEC_IO_SET_SCALE_MODE              0x17    //!<Not supported now.
#define VDEC_IO_GET_OUTPUT_FORMAT           0x21    //!<Not supported now.
#define VDEC_IO_MAF_CMD                     0x22    //!<Not supported now.
#define VDEC_SET_DMA_CHANNEL                0x24    //!<Not supported now.
#define VDEC_IO_SWITCH                      0x25    //!<Not supported now.
#define VDEC_VBV_BUFFER_OVERFLOW_RESET      0x28    //!<Whether to reset es buffer in case of buffer overflow.
#define VDEC_IO_FIRST_I_FREERUN             0x2A    ///!<Whether the first picture performs synchronization or not.
#define VDEC_IO_DONT_RESET_SEQ_HEADER       0x2C    //!<Not supported now.
#define VDEC_IO_SET_DROP_FRM                0x2D    //!<Not supported now.
#define VDEC_IO_MULTIVIEW_HWPIP             0x2E    //!<Not supported now.
#define VDEC_DTVCC_PARSING_ENABLE           0x2f    //!<Not supported now.
#define VDEC_IO_PLAYBACK_PS                 0x31    //!<Not supported now.
#define VDEC_IO_PLAY_FROM_STORAGE           0x32    //!<Not supported now.
#define VDEC_IO_CHANCHG_STILL_PIC           0x33    //!<Not supported now.
#define VDEC_IO_SET_SYNC_DELAY              0x34    //!<Not supported now.
#define VDEC_IO_SAR_ENABLE                  0x35    //!<Not supported now.
#define VDEC_IO_SEAMLESS_SWITCH_ENABLE      0x38    //!<Not supported now.
#define VDEC_IO_PAUSE_VIDEO                 0x39    //!<Not supported now.
#define VDEC_IO_CONTINUE_ON_ERROR           0x3a    //!<Whether to continue to display error picture.
#define VDEC_IO_SET_DECODER_MODE            0x40    //!<Not supported now.
#define VDEC_IO_SET_FREERUN_THRESHOLD       0x41    //!<Not supported now.
#define VDEC_IO_SET_OUTPUT_RECT             0x42    //!<Set display rect in preview mode.
#define VDEC_IO_SET_AVSYNC_GRADUALLY        0x43    //!<Not supported now.
#define VDEC_IO_DBLOCK_ENABLE               0x44    //!<Not supported now.
#define VDEC_IO_GET_DECORE_STATUS           0x46    //!<Not supported now.
#define VDEC_IO_SET_DEC_FRM_TYPE            0x47    //!<Not supported now.
#define VDEC_IO_FILL_BG_VIDEO               0x48    //!<Not supported now.
#define VDEC_IO_BG_FILL_BLACK               0x49    //!<Not supported now.
#define VDEC_IO_GET_MULTIVIEW_BUF           0x50    //!<Not supported now.
#define VDEC_IO_SET_MULTIVIEW_WIN           0x51    //!<Not supported now.
#define VDEC_IO_SLOW_PLAY_BEFORE_SYNC       0x52    //!<Not supported now.
#define VDEC_IO_DONT_KEEP_PRE_FRAME         0x53    //!<Not supported now.
#define VDEC_IO_SET_SEND_GOP_GAP            0x54    //!<Not supported now.
#define VDEC_IO_SET_CC_USER_DATA_INFO       0x55    //!<Not supported now.
#define VDEC_IO_GET_CC_USER_DATA_INFO       0x56    //!<Not supported now.
#define VDEC_IO_GET_FRAME_INFO              0x57    //!<Not supported now.
#define VDEC_IO_FLUSH                       0x58    //!<Not supported now.
#define VDEC_IO_SET_SLOW_PLAY_TRD           0x59    //!<Not supported now.
#define VDEC_IO_GET_AVSYNC_PARAM            0x5a    //!<Not supported now.
#define VDEC_IO_SET_AVSYNC_PARAM            0x5b    //!<Not supported now.
#define VDEC_IO_SET_SPEC_RES_AVSYNC_RANGE   0x5c    //!<Not supported now.
#define VDEC_IO_DROP_FREERUN_PIC            0X5d    //!<only support on mpeg2 driver, drop freerun picture before first picture show.
#define VDEC_IO_SET_PLAY_SCENES             0x5e    //!<only used by dmx
#define VDEC_IO_SET_SIMPLE_SYNC             0x5f
#define VDEC_IO_SET_TRICK_MODE              0x60
#define VDEC_IO_RESTART_DECODE              0x61    //!<Not supported now.
#define VDEC_IO_GET_CAPTURE_FRAME_INFO      0x62    //!<Not supported now.
#define VDEC_IO_DYNAMIC_FB_ALLOC            0x63    //!<Not supported now.
#define VDEC_IO_SET_DISPLAY_MODE            0x64    //!<Not supported now
#define VDEC_IO_SET_PIP_PARAM               0x65
#define VDEC_IO_SET_ID                      0x66
#define VDEC_IO_PARSE_AFD                   0x67
#define VDEC_IO_GET_ALL_USER_DATA           0x68
#define VDEC_IO_RELEASE_VFB                 0x69
#define VDEC_IO_GET_USER_DATA_INFO          0x6A
#define VDEC_IO_SET_SYNC_REPEAT_INTERVAL    0x6B

#define VDEC_IO_ELE_BASE                    0x10000
#define VDEC_IO_PLAYBACK_STR                (VDEC_IO_ELE_BASE + 0x01)   //!<Not supported now.
#define VDEC_IO_REG_DROP_FUN                (VDEC_IO_ELE_BASE + 0x02)   //!<Not supported now.
#define VDEC_IO_REST_VBV_BUF                (VDEC_IO_ELE_BASE + 0x03)   //!<Not supported now.
#define VDEC_IO_KEEP_INPUT_PATH_INFO        (VDEC_IO_ELE_BASE + 0x04)   //!<Not supported now.
#define VDEC_IO_PLAY_MEDIA_STR              (VDEC_IO_ELE_BASE + 0x05)   //!<Not supported now.
#define VDEC_IO_CAPTURE_DISPLAYING_FRAME    (VDEC_IO_ELE_BASE + 0x09)   //!<Not supported now.
#define VDEC_IO_REG_SET_SYNC_FLAG_CB        (VDEC_IO_ELE_BASE + 0x0C)   //!<Not supported now.
#define VDEC_IO_SET_DBG_FLAG                (VDEC_IO_ELE_BASE + 0x0D)

#define DTVCC_USER_DATA_LENGTH_MAX          210      //!<Max user data length.

// decode mode
#define VDEC_MODE_NORMAL                    0   //!<Normal decode
#define VDEC_MODE_VOL                       1   //!<Parse all headers above (including) VOP level without VOP reconstuction (for internal use)
#define VDEC_MODE_HEADER                    2   //!<Parse header to get current frame's prediction type (for internal use)
#define VDEC_MODE_SKIP_B_FRAME              3   //!<Skip b frame (for internal use)
#define VDEC_MODE_SKIP_B_P_FRAME            4   //!<Only decode i frame (for internal use)
#define VDEC_MODE_SBM                       5   //!<Decode from sbm
#define VDEC_MODE_SBM_STREAM                6   //!<Decode from sbm

// decoder command
#define VDEC_CMD_INIT                       0   //!<Initialize the decoder (for internal use)
#define VDEC_CMD_RELEASE                    1   //!<Release the decoder (for internal use)
#define VDEC_CMD_DECODE_FRAME               2   //!<Decode a frame (for internal use)
#define VDEC_CMD_SW_RESET                   3   //!<Reset the decoder SW status (for internal use)
#define VDEC_CMD_HW_RESET                   4   //!<Reset the HW to be ready to decode next frame (for internal use)
#define VDEC_CMD_EXTRA_DADA                 5   //!<Decode extra data before decoding frame (for internal use)
#define VDEC_CMD_GET_STATUS                 6   //!<Get decoder status (for internal use)
#define VDEC_CMD_PAUSE_DECODE               7   //!<Pause decode task running (for internal use)
#define VDEC_CMD_REG_CB                     9   //!<Register callback function (for internal use)
#define VDEC_CMD_ESBUF_REQUEST              10
#define VDEC_CMD_ESBUF_UPDATE               11

#define VDEC_DQ_INIT                        20  //!<*pParam1 -- de_index, *pParam2 --, int DQ for de_index (for internal use)
#define VDEC_DQ_ADD_NULL_FRAME              21  //!<*pParam1 -- de_index, *pParam2 --, Add an null frame into DQ (for internal use)
#define VDEC_DQ_PEEK_FRAME                  22  //!<*pParam1 -- de_index, *pParam2 -- the next frame_id in DQ (for internal use)
#define VDEC_DQ_POP_FRAME                   23  //!<*pParam1 -- de_index, *pParam2 -- the frame_id to be poped (for internal use)
#define VDEC_DQ_GET_COUNT                   24  //!<*pParam1 -- de_index, *pParam2 -- the frame number in DQ (for internal use)
#define VDEC_FRAME_PEEK_FREE                25  //!<*pParam1 -- the next free frm_idx, *pParam2 -- the next free pip_idx (for internal use)
#define VDEC_FRAME_GET_INFO                 26  //!<*pParam2 -- frame_id, pParam1 -- struct DisplayInfo * (for internal use)
#define VDEC_FRAME_GET_POC                  27  //!<*pParam2 -- frame_id, pParam1 -- int *p_poc, (for internal use)
#define VDEC_FRAME_SET_FREE                 28  //!<*pParam2 -- frame_id, bit16 must be set or clear, see DEC_LAST_REQ_FRAME_FLAG (for internal use)
#define VDEC_FRAME_REQUESTING               29  //!<*pParam2 -- frame_id, pParam1 -- struct OutputFrmManager *, (for internal use)
#define VDEC_FRAME_REQUEST_OK               30  //!<*pParam2 -- frame_id, *pParam1 -- de_index, (for internal use)
#define VDEC_FRAME_RELEASE_OK               31  //!<*pParam2 -- frame_id, *pParam1 -- de_index, (for internal use)
#define VDEC_CLEAR_SCREEN                   32  //!<*pParam2 -- flag,     *pParam1 -- de_index, (for internal use)
#define VDEC_START_OUTPUT                   33  //!<*pParam2 -- flag,     *pParam1 -- de_index, (for internal use)
#define VDEC_FRAME_GET_PTS                  34  //!<*pParam2 -- frame_id, pParam1 -- UINT32 *pts (for internal use)
#define VDEC_CFG_VIDEO_SBM_BUF              35  //!<*pParam1 -- smb idx for video pkt buf, *pParam2 -- sbm idx for video output queue (for internal use)
#define VDEC_CFG_DISPLAY_SBM_BUF            36  //!<*pParam1 -- smb idx for display queue (for internal use)
#define VDEC_CFG_SYNC_MODE                  37  //!<*pParam1 -- av sync mode (for internal use)
#define VDEC_CFG_SYNC_THRESHOLD             38  //!<*pParam1 -- hold threshold, *pParam2 -- drop threshold (for internal use)
#define VDEC_CFG_SYNC_DELAY                 39  //!<*pParam1 -- av sync delay [-500ms, 500ms] (for internal use)
#define VDEC_CFG_DISPLAY_RECT               40  //!<*pParam1 -- source rect, *pParam2 destination rect (for internal use)
#define VDEC_CFG_DECODE_MODE                41  //!<*pParam1 -- decode mode (for internal use)
#define VDEC_CFG_FREERUN_TRD                42  //!<*pParam1 -- hol2free threshold, *pParam2 -- drop2free threshold (for internal use)
#define VDEC_CFG_SYNC_PARAM                 43  //!<*pParam1 -- 0:hold 1:drop 2:rollback, *pParam2 -- 0:increase 1:decrease (for internal use)
#define VDEC_CFG_QUICK_MODE                 44  //!<*pParam1 -- quick mode (for internal use)
#define VDEC_CAPTURE_FRAME                  45  //!<*pParam1 -- struct vdec_picture (for internal use)
#define VDEC_ROTATE_FRAME                   46  //!<*pParam1 -- frame angle (for internal use)
#define VDEC_GET_CAPTURE_FRAME_INFO         47  //!<*pParam1 -- struct vdec_capture_frm_info
#define VDEC_DYNAMIC_FRAME_ALLOC            48  //!<*pParam1 -- enable frame malloc or not
#define VDEC_STEP_DISPLAY                   49  //!<*pParam1 -- step display one frame or not
#define VDEC_SET_DISPLAY_MODE               50  //!<*pParam1 -- set display mode
#define VDEC_SET_DISPLAY_SLOW_RATIO         51  //!<*pParam1 -- -1 -2 -3
#define VDEC_SET_PIP_PARAM                  52  //!<*pParam1 -- set pip parameter
#define VDEC_SET_MALLOC_DONE                53
#define VDEC_GET_MEM_RANGE                  54

// decode status machine
#define VDEC_NEW_VIDEO_PACKET               1   //!<ready to decode new video packet(picture)
#define VDEC_WAIT_NEW_FB                    2   //!<waiting for free buffer to decode video
#define VDEC_NEW_SLICE                      3   //!<ready to decode new video slice
#define VDEC_ON_GOING                       4   //!<video decoding is on going
#define VDEC_POST_PROCESS                   5   //!<last picture decoding is done, ready to do some post process
#define VDEC_CONTINUE                       6   //!<decore needs to push last decoded P VOP into DQ in B-VOP decoding and continue B-VOP decoding
#define VDEC_REQ_HW                         7   //!<waiting for hardward idle

// decode return values
#define VDEC_SUCCESS                        0   //!<For internal use
#define VDEC_FAILURE                        1   //!<For internal use
#define VDEC_FORBIDDEN_DECODE               2   //!<Cannot decode due to some reason (for internal use)
#define VDEC_INVALID_CMD                    3   //!<For internal use
#define VDEC_FRAME_SKIPED                   4   //!<Frame skipped according to decode mode (for internal use)
#define VDEC_NO_MEMORY                      5   //!<Memory not enough (for internal use)
#define VDEC_BAD_FORMAT                     6   //!<For internal use
#define VDEC_NOT_IMPLEMENTED                7   //!<New feature not supported (for internal use)
#define VDEC_PATCH_PSUDO_MB                 8   //!<Patch IC issue (for internal use)
#define VDEC_CURRENT_CONTINUE               9   //!<Continue decoding current frame (for internal use)
#define VDEC_SEND_NEXT                      10
#define VDEC_BUSY                           11

// decode error type (not supported features)
#define VDEC_ERR_NONE                       0
#define VDEC_ERR_SHAPE                      (1 << 0)  //!<Shape coding, unsupported
#define VDEC_ERR_SPRITE                     (1 << 1)  //!<Static sprite, unsupported
#define VDEC_ERR_NOT8BIT                    (1 << 2)  //!<Video data precision N-bit, unsupported
#define VDEC_ERR_NEWPRED                    (1 << 3)  //!<Newpred mode, unsupported
#define VDEC_ERR_SCALAB                     (1 << 4)  //!<Scalability, unsupported
#define VDEC_ERR_REDUCEDRES                 (1 << 5)  //!<Reduced resolution vop, unsupported
#define VDEC_ERR_3POINTGMC                  (1 << 6)  //!<3-point gmc, video may appear distorted
#define VDEC_ERR_DATA_PARTITION             (1 << 7)  //!<Data partition, unsupported
#define VDEC_ERR_RESOLUTION                 (1 << 8)  //!<Resolution unsupported
#define VDEC_ERR_CODEC                      (1 << 9)  //!<Codec unsupported
#define VDEC_ERR_NOMEMORY                   (1 << 10) //!<Not enough memory

#define VDEC_FLAG_HAS_LICENSED              (1 << 0)  //!<Has full license (for internal use)
#define VDEC_FLAG_AVC1_FORMAT               (1 << 1)  //!<AVC nalu has nalu_size on the head
#define VDEC_FLAG_MPEG4_DECODER             (1 << 2)  //!<It's a general mpeg4 decoder (for internal use)

#define VDEC_MIN_FRAME_BUF_NUM              4   //!<For internal use
#define VDEC_MAX_FRAME_BUF_NUM              6   //!<For internal use

#define AV_NOPTS_VALUE                      ((INT64)(0x8000000000000000LL))    //!<PTS is invalid or no PTS

#define AV_PKT_FLAG_CODEC_DATA              0x10000000    //!<The packet contains codec data
#define AV_PKT_FLAG_EOS                     0x20000000    //!<The last packet
#define AV_PKT_FLAG_ERROR                   0x40000000
#define AV_PKT_FLAG_VIRTUAL                 0x80000000

#define MKTAG(a, b, c, d)                   (a | (b << 8) | (c << 16) | (d << 24))    //!<Make video decoder's ID
#define h264                                MKTAG('h','2','6','4')    //!<H.264 decoder's ID
#define xvid                                MKTAG('x','v','i','d')    //!<MPEG4 decoder's ID
#define mpg2                                MKTAG('m','p','g','2')    //!<MPEG1/2 decoder's ID
#define flv1                                MKTAG('f','l','v','1')    //!<FLV1 decoder's ID
#define vp8                                 MKTAG('v','p','8',' ')    //!<VP8 decoder's ID
#define wvc1                                MKTAG('w','v','c','1')    //!<VC1 decoder's ID
#define wx3                                 MKTAG('w','x','3',' ')    //!<WX3 decoder's ID
#define rmvb                                MKTAG('r','m','v','b')    //!<RV decoder's ID
#define mjpg                                MKTAG('m','j','p','g')    //!<MJPG decoder's ID
#define vc1                                 MKTAG('v','c','1',' ')    //!<VC1 decoder's ID
#define hevc                                MKTAG('h','e','v','c')    //!<H.265 decoder's ID

enum VDEC_OUTPUT_MODE
{
    MP_MODE,                //!<main picture
    PIP_MODE,               //!<Not supported now.
    PREVIEW_MODE,           //!<preview
    HSCALE_MODE,
    DVIEW_MODE,
    MP_DVIEW_MODE,
    HSCALE_DVIEW_MODE,
    AUTO_MODE,
    DUAL_MODE,
    DUAL_PREVIEW_MODE,
    IND_PIP_MODE,
    SW_PASS_MODE,
    HW_DEC_ONLY_MODE,
    MULTI_VIEW_MODE,
    RESERVE_MODE,
    VDEC_OUTPUT_MODE_END
};

/*! @enum video_decoder_type
@brief
DECV decoder type.
*/
enum video_decoder_type
{
    MPEG2_DECODER = 0,
    H264_DECODER,
    AVS_DECODER,
    H265_DECODER,
    VC1_DECODER,
    MPEG4_DECODER,
    VP8_DECODER,
    RV_DECODER,
    MJPG_DECODER,

    INVALID_DECODER = 0xFF,
};

enum vdec_speed
{
    VDEC_SPEED_1_2,
    VDEC_SPEED_1_4,
    VDEC_SPEED_1_8,
    VDEC_SPEED_STEP,
    VDEC_SPEED_1,
    VDEC_SPEED_2,
    VDEC_SPEED_4,
    VDEC_SPEED_8,
    VDEC_SPEED_16,
    VDEC_SPEED_32,
};

enum vdec_direction
{
    VDEC_FORWARD = 0,
    VDEC_BACKWARD
};

/*! @enum VDEC_STATUS
@brief For internal use
*/
enum VDEC_STATUS
{
    VDEC27_ATTACHED,
    VDEC27_STARTED,
    VDEC27_STOPPED,
    VDEC27_PAUSED,
};

/*! @enum vdec_cbtype
@brief
The callback function type of DECV module.
*/
enum vdec_cbtype
{
    VDEC_CB_SETTING_CHG = 0,       //!<Not supported now.
    VDEC_CB_REQ_DATA,              //!<Not supported now.
    VDEC_CB_STOP,                  //!<Not supported now.
    VDEC_CB_FINISH_CUR_PIC,        //!<Not supported now.
    VDEC_CB_FINISH_I_FRAME,        //!<Not supported now.
    VDEC_CB_FINISH_TARGET_FRAME,   //!<Not supported now.
    VDEC_CB_FIRST_SHOWED,          //!<The first frame was displayed successfully.
    VDEC_CB_MODE_SWITCH_OK,        //!<Not supported now.
    VDEC_CB_BACKWARD_RESTART_GOP,  //!<Retransfer video data in DVR backward application.
    VDEC_CB_OUTPUT_MODE_CHECK,     //!<Not supported now.
    VDEC_CB_FIRST_HEAD_PARSED,     //!<Parsed the first header information.
    VDEC_CB_MONITOR_FRAME_VBV,     //!<Not supported now.
    VDEC_CB_MONITOR_VDEC_START,    //!<Vdec started.
    VDEC_CB_MONITOR_VDEC_STOP,     //!<Vdec stopped.
    VDEC_CB_FIRST_I_DECODED,       //!<First I picture decoded.
    VDEC_CB_MONITOR_USER_DATA_PARSED,   //!<Vdec user data parsed.
    VDEC_CB_INFO_CHANGE,           //!<Vdec information changed
    VDEC_CB_ERROR,                 //!<Vdec error
    VDEC_CB_STATE_CHANGED,         //!<Vdec state changed
    VDEC_CB_DECODER_FINISH,
    VDEC_CB_FRAME_DISPLAYED,       //!<Vdec frame displayed
    VDEC_CB_MALLOC_DONE,
	VDEC_CB_MONITOR_GOP,
};

/*! @enum vdec_rotation_angle
@brief
The rotation angle of DECV module.
*/
enum vdec_rotation_angle
{
    VDEC_ANGLE_0,
    VDEC_ANGLE_90,
    VDEC_ANGLE_180,
    VDEC_ANGLE_270,

    VDEC_ANGLE_MAX,
};

/*! @enum vdec_state_flags
@brief Define flags for vdec state
*/
enum vdec_state_flags
{
    VDEC_STATE_NODATA    = 0x0001,    //!<No data state
    VDEC_STATE_DECODING  = 0x0002,    //!<Decoding state
};

/*! @enum vdec_error_flags
@brief Define flags for vdec error
*/
enum vdec_error_flags
{
    VDEC_ERROR_NODATA    = 0x0001,    //!<No data error
    VDEC_ERROR_HARDWARE  = 0x0002,    //!<Decode error
    VDEC_ERROR_SYNC      = 0x0004,    //!<Sync error
    VDEC_ERROR_FRAMEDROP = 0x0008,    //!<Frame drop
    VDEC_ERROR_FRAMEHOLD = 0x0010,    //!<Frame hold
    VDEC_ERROR_GIVEUPSEQ = 0x0020,    //!<Give up sequence
    VDEC_ERROR_INVDATA   = 0x0040,    //!<Invalid data
};

/*! @enum vdec_info_change_flags
@brief Define flags for information changing
*/
enum vdec_info_change_flags
{
    VDEC_CHANGE_DIMENSIONS = 0x0001,    //!<Dimension change
    VDEC_CHANGE_FRAMERATE  = 0x0002,    //!<Frame rate change
    VDEC_CHANGE_AFD        = 0x0004,    //!<AFD change
    VDEC_CHANGE_SAR        = 0x0008,    //!<SAR change
};

/*! @enum vdec_output_mode
@brief Video output mode
*/
enum vdec_output_mode
{
    VDEC_FULL_VIEW,    //!<Full screen display
    VDEC_PREVIEW,      //!<Preview display
    VDEC_SW_PASS,      //!<Do not decode, just consume data
};

enum vdec_playback_rate
{
    VDEC_RATE_1_2,
    VDEC_RATE_1_4,
    VDEC_RATE_1_8,
    VDEC_RATE_STEP,
    VDEC_RATE_1,
    VDEC_RATE_2,
    VDEC_RATE_4,
    VDEC_RATE_8,
    VDEC_RATE_16,
    VDEC_RATE_32,
};

enum vdec_playback_dir
{
    VDEC_PLAY_FORWARD = 0,
    VDEC_PLAY_BACKWARD,
};

enum VDEC_DBG_FLAG
{
    VDEC_DBG_NONE,
    VDEC_DBG_DEFAULT,
};

/*! @enum VdecEventType
@brief Decore notify event to video player engine (for internal use)
*/
enum vdec_event_type
{
    VDEC_EVENT_FRAME_FINISHED,   //!<One frame finished by VE
    VDEC_EVENT_FRAME_OUTPUT,     //!<One frame put into DQ
    VDEC_EVENT_DE_RELEASED,      //!<One frame released by DE
    VDEC_EVENT_DE_REQUESTING,    //!<Requesting frame by DE
    VDEC_EVENT_SLICE_FINISHED,   //!<One slice finished by VE
    VDEC_EVENT_FB_MALLOC_DONE,   //!<Frame buffer malloc done
    VDEC_EVENT_FIELD_PIC_FLAG,   //!<0 -- 1 frame 1 pts; 1 -- 1 field 1 pts
};

/*! @enum av_param_change_flags
@brief Define flags for paramter changing
*/
enum av_param_change_flags
{
    AV_PARAM_CHANGE_CHANNEL_COUNT  = 0x0001,    //!<Temporarily unused
    AV_PARAM_CHANGE_CHANNEL_LAYOUT = 0x0002,    //!<Temporarily unused
    AV_PARAM_CHANGE_SAMPLE_RATE    = 0x0004,    //!<Temporarily unused
    AV_PARAM_CHANGE_DIMENSIONS     = 0x0008,    //!<Dimensions change
};

/*! @typedef VDecCBFunc
@brief
The callback function type of video module.
*/
typedef void (* vdec_cbfunc)(UINT32 u_param1, UINT32 u_param2);
typedef void (* VDEC_BEYOND_LEVEL)(void);
typedef int  (* pfn_on_decode_event)(enum vdec_event_type event, UINT32 param);

//! @typedef FourCC
//! @brief Video four character codes
typedef UINT32 four_cc;

/*! @struct vdec_display_rect
@brief Display rectangle
*/
struct vdec_display_rect
{
    INT32 src_x;    //!<Horizontal start point of source
    INT32 src_y;    //!<Vertical start point of source
    INT32 src_w;    //!<Horizontal size of source
    INT32 src_h;    //!<Vertical size of source
    INT32 dst_x;    //!<Horizontal start point of destination
    INT32 dst_y;    //!<Vertical start point of destination
    INT32 dst_w;    //!<Horizontal size of destination
    INT32 dst_h;    //!<Vertical size of destination
};

/*! @struct vdec_display_mode
@brief Display mode
*/
struct vdec_display_param
{
    enum vdec_output_mode mode;
    struct vdec_display_rect rect;
};

struct vdec_playback_param
{
    enum vdec_playback_dir direction;
    enum vdec_playback_rate rate;
    UINT32 mode;
};

/*! @struct vdec_info_cb_param
@brief Describe video callback parameters for information change
*/
struct vdec_info_cb_param
{
    UINT32 info_change_flags;   //!<Indicate which information changes
    UINT32 pic_width;           //!<Picture width
    UINT32 pic_height;          //!<Picture height
    UINT32 frame_rate;          //!<Frame rate
    UINT8  active_format;       //!<Active format
    UINT32 sar_width;           //!<Sample aspect ratio width
    UINT32 sar_height;          //!<Sample aspect ratio height
};

struct vdec_pip_param
{
    UINT32 layer;
};

/*! @struct user_data_pram
@brief param of VDEC_CB_MONITOR_USER_DATA_PARSED callback.
*/
struct user_data_pram
{
    UINT32 user_data_size;
    UINT8 user_data[DTVCC_USER_DATA_LENGTH_MAX];
};

/*! @struct vdec_capture_frm_info
@brief param of VDEC_IO_GET_CAPTURE_FRAME_INFO.
*/
struct vdec_capture_frm_info
{
    UINT32 pic_height;
    UINT32 pic_width;
    UINT32 pic_stride;
    UINT32 y_buf_addr;
    UINT32 y_buf_size;
    UINT32 c_buf_addr;
    UINT32 c_buf_size;
    UINT8 de_map_mode;
};

/*! @struct vdec_io_reg_callback_para
@brief
Parameters definition of VDEC_IO_REG_CALLBACK, for registering callback function.
*/
struct vdec_io_reg_callback_para
{
    enum vdec_cbtype e_cbtype;  //!<The callback function type to be registered.
    vdec_cbfunc p_cb;           //!<The callback function pointer to be registered.
    UINT32 monitor_rate;
};

struct vdec_callback
{
    vdec_cbfunc pcb_first_showed;
    vdec_cbfunc pcb_mode_switch_ok;
    vdec_cbfunc pcb_backward_restart_gop;
    vdec_cbfunc pcb_first_head_parsed;
    vdec_cbfunc pcb_new_frame_coming;
    vdec_cbfunc pcb_vdec_start;
    vdec_cbfunc pcb_vdec_stop;
    vdec_cbfunc pcb_vdec_user_data_parsed;
    vdec_cbfunc pcb_vdec_decoder_finish;
    vdec_cbfunc pcb_first_i_decoded;
    vdec_cbfunc pcb_vdec_info_changed;
    vdec_cbfunc pcb_vdec_error;
    vdec_cbfunc pcb_vdec_state_changed;
	vdec_cbfunc pcb_frame_displayed;
    vdec_cbfunc pcb_vdec_new_frame;
	vdec_cbfunc pcb_vdec_monitor_gop;
};

struct use_data_header_info
{
    UINT32 top_field_first     :1;
    UINT32 repeat_first_field  :1;
    UINT32 reserve_0           :30;
};

/*! @struct vdec_frm_output_format
@brief
The output format of DECV module.
*/
struct vdec_frm_output_format
{
    // VE config
    BOOL h_scale_enable;
    UINT32 h_scale_factor;     //!<0:reserved, 1: Y h_scale only, 2: Y,C h_scale.

    BOOL dview_enable;
    UINT32 dv_h_scale_factor;  //!<0:no dview, 1: 1/2 dview, 2: 1/4 dview, 3: 1/8 dview.
    UINT32 dv_v_scale_factor;  //!<0:no dview, 1: 1/2 dview, 2: 1/4 dview, 3: 1/8 dview.

    UINT32 dv_mode;

    //DE config
    UINT32 field_src;          //!<0: both fields, 1:only top field.
    UINT32 scaler_src;         //!<0: frame base, 1: field base.
    UINT32 vpp_effort;         //!<0:high, 1: middle, 2: low, 3:very low.
};

struct vdec_dvr_config_param
{
    BOOL is_scrambled;
};

struct adv_setting
{
    UINT8 init_mode;
    enum tvsystem out_sys;
    BOOL bprogressive;
    UINT8 switch_mode;  /* 1: mp<=>preview switch smoothly */
};

struct multi_view_setting
{
    UINT32 window_width;
    UINT32 window_height;
    UINT32 multi_view_buf;
    UINT32 multi_view_buf_addr;
    UINT32 multi_view_buf_size;
};

struct vdec_pipinfo
{
    struct position pip_sta_pos;
    struct rect_size pip_size;
    struct rect_size mp_size;
    BOOL b_use_bg_color;
    struct ycb_cr_color bg_color;
    BOOL buse_sml_buf;
    struct rect src_rect;
    struct rect dst_rect;
    struct adv_setting adv_setting;
    struct multi_view_setting para;
};

/*! @struct vdec_status_info
@brief For internal use
*/
struct vdec_status_info
{
    enum VDEC_STATUS u_cur_status;
    BOOL   u_first_pic_showed;
    BOOL   b_first_header_got;
    UINT16 pic_width;
    UINT16 pic_height;
    UINT16 status_flag;
    UINT32 read_p_offset;
    UINT32 write_p_offset;
    UINT32 display_idx;
    BOOL   use_sml_buf;
    enum VDEC_OUTPUT_MODE  output_mode;
    UINT32 valid_size;
    UINT32 mpeg_format;
    enum asp_ratio aspect_ratio;
    UINT16 frame_rate;
    UINT32 bit_rate;
    BOOL   hw_dec_error;
    BOOL   display_frm;
    UINT8  top_cnt;
    UINT8  play_direction;
    UINT8  play_speed;
    UINT8  api_play_direction;
    UINT8  api_play_speed;
    BOOL   is_support;
    UINT32 vbv_size;
    UINT8  cur_dma_ch;
    BOOL   progressive;
    INT32  top_field_first;
    INT32  first_pic_decoded;
    UINT32 frames_decoded;
    UINT32 frame_last_pts;
    UINT32 sar_width;
    UINT32 sar_height;
    UINT8  active_format;
    UINT8  layer;                  //!<Which display layer deocder associated with
    UINT8  ff_mode;
    UINT8  rect_switch_done;
	UINT16 max_width;
    UINT16 max_height;
    UINT16 max_frame_rate;
};

struct vdec_io_dbg_flag_info
{
    enum VDEC_DBG_FLAG dbg_flag;
    int active_flag;
    int unique_flag;
};

struct vdec_avc_memmap
{
    BOOL support_multi_bank;

    UINT32 frame_buffer_base;
    UINT32 frame_buffer_len;

    UINT32 dv_frame_buffer_base;
    UINT32 dv_frame_buffer_len;

    UINT32 mv_buffer_base;
    UINT32 mv_buffer_len;

    UINT32 mb_col_buffer_base;
    UINT32 mb_col_buffer_len;

    UINT32 mb_neighbour_buffer_base;
    UINT32 mb_neighbour_buffer_len;

    UINT32 cmd_queue_buffer_base;
    UINT32 cmd_queue_buffer_len;

    UINT32 vbv_buffer_base;
    UINT32 vbv_buffer_len;

    UINT32 laf_rw_buf;
    UINT32 laf_rw_buffer_len;

    UINT32 laf_flag_buf;
    UINT32 laf_flag_buffer_len;

    BOOL   support_conti_memory;
    UINT32 avc_mem_addr;
    UINT32 avc_mem_len;
    UINT32 auxp_addr;
};

struct vdec_avc_config_par
{
    struct vdec_avc_memmap memmap;
    UINT32 max_additional_fb_num;
    UINT8 dtg_afd_parsing;
    UINT8 dev_num;
};

/*!
@struct  vdec_hevc_memmap
@brief  Define the memory mapping configuration  for High Efficiency Video decoder
*/
typedef struct
{
    BOOL support_multi_bank;
    BOOL support_conti_memory;        //!< if system support continual memory allocation mode

    UINT32 mp_frame_buffer_base;      //!< Frame buffer base address for main picture video
    UINT32 mp_frame_buffer_len;       //!< Frame buffer length for main picture video

    UINT32 dv_frame_buffer_base;      //!< Frame buffer base address for decimated picture video
    UINT32 dv_frame_buffer_len;       //!< Frame buffer length for decimated picture video

    UINT32 collocated_mv_buffer_base; //!< Collocated MV buffer base address
    UINT32 collocated_mv_buffer_len;  //!< Collocated MV buffer length

    UINT32 ph_buffer_base;            //!< Picture Header Syntax buffer base address
    UINT32 ph_buffer_len;             //!< Picture Header Syntax buffer length

    UINT32 inner_es_buffer_base;      //!< INNER Elementary Stream buffer base address
    UINT32 inner_es_buffer_len;       //!< INNER Elementary Stream buffer length

    UINT32 inner_aux_buffer_base;     //!< INNER Auxilary buffe buffer base address
    UINT32 inner_aux_buffer_len;      //!< INNER Auxilary buffe buffer length

    UINT32 neighbour_buffer_base;     //!< Neighbour buffer base address
    UINT32 neighbour_buffer_len;      //!< Neighbour buffer length

    UINT32 ep_cmd_queue_buffer_base;  //!< EP command queue buffer base address
    UINT32 ep_cmd_queue_buffer_len;   //!< EP command queue buffer length

    UINT32 md_cmd_queue_buffer_base;  //!< MD command queue buffer base address
    UINT32 md_cmd_queue_buffer_len;   //!< MD command queue buffer length

    UINT32 vbv_buffer_base;           //!< VBV buffer base address
    UINT32 vbv_buffer_len;            //!< VBV buffer length

    UINT32 hevc_mem_addr;             //!<  memory base address
    UINT32 hevc_mem_len;              //!< memory base address length
}vdec_hevc_memmap;

struct vdec_hevc_config_par
{
    char *decv_hevc_inst_name;        //!< H265 decoder instance name, support multi-instance based on different name
    vdec_hevc_memmap memmap;          //!< H265 memory mapping of the buffers
    UINT32 max_additional_fb_num;
	UINT8 dtg_afd_parsing;
    UINT8 dev_num;
};

struct vdec_avs_memmap
{
    BOOL support_multi_bank;

    UINT32 frame_buffer_base;
    UINT32 frame_buffer_len;

    UINT32 dv_frame_buffer_base;
    UINT32 dv_frame_buffer_len;

    UINT32 mv_buffer_base;
    UINT32 mv_buffer_len;

    UINT32 mb_col_buffer_base;
    UINT32 mb_col_buffer_len;

    UINT32 mb_neighbour_buffer_base;
    UINT32 mb_neighbour_buffer_len;

    UINT32 cmd_queue_buffer_base;
    UINT32 cmd_queue_buffer_len;

    UINT32 vbv_buffer_base;
    UINT32 vbv_buffer_len;

    UINT32 laf_rw_buf;
    UINT32 laf_rw_buffer_len;

    UINT32 laf_flag_buf;
    UINT32 laf_flag_buffer_len;

    BOOL   support_conti_memory;
    UINT32 avs_mem_addr;
    UINT32 avs_mem_len;
};

struct vdec_avs_config_par
{
    struct vdec_avs_memmap memmap;
};

struct vdec_mem_map
{
    UINT32 frm0_y_size;
    UINT32 frm0_c_size;
    UINT32 frm1_y_size;
    UINT32 frm1_c_size;
    UINT32 frm2_y_size;
    UINT32 frm2_c_size;

    UINT32 frm0_y_start_addr;
    UINT32 frm0_c_start_addr;
    UINT32 frm1_y_start_addr;
    UINT32 frm1_c_start_addr;
    UINT32 frm2_y_start_addr;
    UINT32 frm2_c_start_addr;

    UINT32 dvw_frm_size;
    UINT32 dvw_frm_start_addr;

    UINT32 maf_size;
    UINT32 maf_start_addr;

    UINT32 vbv_size;
    UINT32 vbv_start_addr;
    UINT32 vbv_end_addr;

    UINT32 frm3_y_size;
    UINT32 frm3_c_size;
    UINT32 frm3_y_start_addr;
    UINT32 frm3_c_start_addr;

    UINT32 frm_num;//3
    UINT32 res_bits;
    UINT32 *res_pointer;

    UINT32 ext_maf_buf1;
    UINT32 ext_maf_buf2;
    UINT32 auxp_buf;
};

struct vdec_adpcm
{
    UINT8 adpcm_mode;
    UINT8 adpcm_ratio;
};

struct vdec_sml_frm
{
    UINT8 sml_frm_mode;
    UINT32 sml_frm_size;
};

struct vdec_config_par
{
    UINT8 user_data_parsing;
    UINT8 dtg_afd_parsing;
    UINT8 drop_freerun_pic_before_firstpic_show;
    UINT8 reset_hw_directly_when_stop;
    UINT8 show_hd_service;
    UINT8 still_frm_in_cc;
    UINT8 extra_dview_window;
    UINT8 not_show_mosaic;
    UINT8 return_multi_freebuf;
    UINT8 advance_play;

    struct vdec_adpcm adpcm;
    struct vdec_sml_frm sml_frm;

    UINT8 lm_shiftthreshold;
    UINT8 vp_init_phase;
    UINT8 preview_solution;

    struct vdec_mem_map mem_map;
    UINT32 res_bits;
    UINT32 *res_pointer;
    UINT8 dev_num;
};

/*! @struct vdec_device
@brief
The DECV device type definition.
*/
struct vdec_device
{
    struct vdec_device *next;
    UINT32 type;
    INT8   name[32];
    UINT8  flags;
    UINT8  index;
    void   *top_info;
    void   *priv;

    RET_CODE (*open)(struct vdec_device *);
    RET_CODE (*close)(struct vdec_device *);
    RET_CODE (*start)(struct vdec_device *);
    RET_CODE (*stop)(struct vdec_device *,BOOL,BOOL);
    RET_CODE (*vbv_request)(struct vdec_device *, UINT32, void **, UINT32 *, struct control_block *);
    void     (*vbv_update)(struct vdec_device *, UINT32);
    RET_CODE (*set_output)(struct vdec_device *, enum VDEC_OUTPUT_MODE, struct vdec_pipinfo *, \
                           struct mpsource_call_back *, struct pipsource_call_back *);
    RET_CODE (*sync_mode)(struct vdec_device *,  UINT8,UINT8);
    RET_CODE (*ioctl)(struct vdec_device *, UINT32 , UINT32);
    /* for advanced play */
    RET_CODE (*playmode)(struct vdec_device *, enum vdec_direction , enum vdec_speed);
    RET_CODE (*step)(struct vdec_device *);
    RET_CODE (*dvr_pause)(struct vdec_device *);
    RET_CODE (*dvr_resume)(struct vdec_device *);
    RET_CODE (*profile_level)(struct vdec_device *,  UINT8,VDEC_BEYOND_LEVEL);
    RET_CODE (*dvr_set_param)(struct vdec_device *, struct vdec_dvr_config_param);
    /* end */
    RET_CODE (*internal_set_output)(struct vdec_device *, enum VDEC_OUTPUT_MODE, struct vdec_pipinfo *, \
                                    struct mpsource_call_back *, struct pipsource_call_back *);
    RET_CODE (*internal_set_frm_output_format)(struct vdec_device *, struct vdec_frm_output_format *);
    void     (*de_hw_using)(struct vdec_device *, UINT8, UINT8, UINT8);
};

/*! @struct VdecHWMemConfig
@brief (for internal use)
*/
typedef struct _vdec_hwmem_config
{
    UINT32 fb_y[VDEC_MAX_FRAME_BUF_NUM];
    UINT32 fb_c[VDEC_MAX_FRAME_BUF_NUM];
    UINT32 dv_y[VDEC_MAX_FRAME_BUF_NUM];
    UINT32 dv_c[VDEC_MAX_FRAME_BUF_NUM];
    UINT32 fb_max_stride;
    UINT32 fb_max_height;
    UINT32 dv_max_stride;
    UINT32 dv_max_height;
    UINT32 fb_num;
    UINT32 neighbor_mem;
    UINT32 colocate_mem;
    UINT32 cmdq_base;
    UINT32 cmdq_len;
    UINT32 vbv_start;
    UINT32 vbv_len;
} vdec_hwmem_config;

/*! @struct DViewCtrl
@brief For internal use
*/
typedef struct _dview_ctrl
{
    UINT8 dv_enable;
    UINT8 dv_h_scale_factor;
    UINT8 dv_v_scale_factor;
    UINT8 interlaced:1;
    UINT8 bottom_field_first:1;
}dview_ctrl;

/*! @struct vdec_fbconfig
@brief For internal use
*/
typedef struct _vdec_fbconfig
{
    UINT32 fbstride;
    UINT32 fbmax_height;
    UINT8  *frm_buf;     //!<Used only for SW_DECODE
    UINT8  *disp_buf;    //!<In HW_DISPLAY mode, the first 4 bytes store frame buffer address, while the last 4 bytes store dv buffer address
} vdec_fbconfig;

/*! @struct vdec_format_info
@brief Describe a compressed or uncompressed video format (for internal use)
*/
typedef struct _vdec_format_info
{
    four_cc fourcc;
    INT32 bits_per_pixel;
    INT32 pic_width;
    INT32 pic_height;
    INT32 pic_inverted;
    INT32 pixel_aspect_x;
    INT32 pixel_aspect_y;
    INT32 frame_rate;
    INT32 frame_period;
    BOOL frame_period_const;
} vdec_format_info;

/*! @struct vdec_init
@brief Structure with parameters used to instantiate a decoder (for internal use)
*/
typedef struct _vdec_init
{
    vdec_format_info fmt_out;   //!<Desired output video format.
    vdec_format_info fmt_in;    //!<Given input video format
    const vdec_fbconfig *pfrm_buf;
    const vdec_hwmem_config *phw_mem_cfg;
    pfn_on_decode_event  on_decode_event;
    t_request pfn_decore_de_request;
    t_release pfn_decore_de_release;
    UINT32 decoder_flag;
    UINT32 decode_mode;
    UINT32 preview;
    UINT32 dec_buf_addr;
    UINT32 dec_buf_size;
    UINT32 priv_buf_addr;
    UINT32 priv_buf_size;
    UINT32 vbv_buf_addr;
    UINT32 vbv_buf_size;
    UINT32 vdec_id;
    UINT8  encrypt_mode;        //!0: clear data 1: full sample 2: subsample
} vdec_init;

/*! @enum vdec_frm_array_type
@brief Structure used by display queue management (for internal use)
*/
enum vdec_frm_array_type
{
    VDEC_FRM_ARRAY_MP = 0x01,
    VDEC_FRM_ARRAY_DV = 0x02,

    VDEC_FRM_ARRAY_INVALID = 0xff,
};

/*! @struct output_frm_manager
@brief (for internal use)
*/
struct output_frm_manager
{
    UINT32  display_index[2];

    enum vdec_frm_array_type de_last_request_frm_array[2];
    enum vdec_frm_array_type de_last_release_frm_array[2];

    UINT8  de_last_request_idx[2];
    UINT8  de_last_release_idx[2];

    INT32 last_output_pic_released[2];
    INT32 de_last_release_poc[2];
    INT32 de_last_request_poc[2];

    UINT8 frm_number;
    UINT8 pip_frm_number;
};

/*! @struct av_panscan
@brief specifies the area which should be displayed (temporarily unused)
*/
struct av_panscan
{
    INT32 id;
    INT32 width;
    INT32 height;
    INT16 position[3][2];    //!<Position of the top left corner in 1/16 pel for up to 3 fields/frames
};

/*! @struct av_rational
@brief Rational number numerator/denominator
*/
struct av_rational
{
    INT32 num;    //!<Numerator
    INT32 den;    //!<Denominator
};

struct av_rect
{
    INT32 x;    //!<Horizontal start point.
    INT32 y;    //!<Vertical start point.
    INT32 w;    //!<Horizontal size.
    INT32 h;    //!<Vertical size.
};

/*! @struct av_packet
@brief Describe A/V packet
*/
struct av_packet
{
    INT64 pts;                   //!<Presentation timestamp, can be AV_NOPTS_VALUE if it is not stored in the file
    INT64 dts;                   //!<Decompression timestamp, can be AV_NOPTS_VALUE if it is not stored in the file
    UINT8 *data;                 //!<A/V packet data
    INT32 size;                  //!<A/V packet size
    INT32 stream_index;          //!<Temporarily unused
    INT32 flags;                 //!<Flag to specify the packet data
    UINT16 width;                //!<Picture width
    UINT16 height;               //!<Picture height
    INT32 param_change_flags;    //!<Flag to specify the change of parameter
    INT32 duration;              //!<Duration of this packet (temporarily unused)
    void  (*destruct)(struct av_packet *);  //!<Temporarily unused
    void  *priv;                 //!<Temporarily unused
    INT64 pos;                   //!<Byte position in stream, -1 if unknown (temporarily unused)
    INT64 convergence_duration;  //!<Temporarily unused
    UINT8 nalu_num;              //!<Numbers of NALU in the packet, -1 if unknown
};

/*! @struct av_frame
@brief Describe video frame output by decoder
*/
struct av_frame
{
    UINT8 *data[4];                 //!<Pointer to the picture planes
    INT32 linesize[4];              //!<Size of picture planes
    UINT8 *base[4];                 //!<Pointer to the first allocated byte of the picture
    INT32 key_frame;                //!<1 -> keyframe, 0-> not
    UINT32 pict_type;               //!<Picture type of the frame
    INT64 pts;                      //!<Presentation timestamp
    INT32 coded_picture_number;     //!<Picture number in bitstream order
    INT32 display_picture_number;   //!<Picture number in display order
    INT32 quality;                  //!<Temporarily unused
    INT32 age;                      //!<Frame rate
    INT32 reference;                //!<Is this picture used as reference
    INT8 *qscale_table;             //!<Temporarily unused
    INT32 qstride;                  //!<Temporarily unused
    UINT8 *mbskip_table;            //!<Temporarily unused
    UINT16 (*motion_val[2])[2];     //!<Temporarily unused
    UINT32 *mb_type;                //!<Temporarily unused
    UINT8 motion_subsample_log2;    //!<Temporarily unused
    void *opaque;                   //!<Temporarily unused
    INT64 error[4];                 //!<Temporarily unused
    INT32 type;                     //!<Temporarily unused
    INT32 repeat_pict;              //!<How much the picture must be delayed (temporarily unused)
    INT32 qscale_type;              //!<Temporarily unused
    INT32 interlaced_frame;         //!<Whether the picture is interlaced
    INT32 top_field_first;          //!<Is top field displayed first
    struct av_panscan *pan_scan;    //!<Temporarily unused
    INT32 palette_has_changed;      //!<Temporarily unused
    INT32 buffer_hints;             //!<Codec suggestion on buffer type if != 0 (temporarily unused)
    short *dct_coeff;               //!<Temporarily unused
    INT8 *ref_index[2];             //!<Temporarily unused
    INT64 reordered_opaque;         //!<Temporarily unused
    void *hwaccel_picture_private;  //!<Hardware accelerator private data
    INT64 pkt_pts;                  //!<Presentation timestamp
    INT64 pkt_dts;                  //!<Decode timestamp
    void *owner;                    //!<Temporarily unused
    void *thread_opaque;            //!<Temporarily unused
    INT64 best_effort_timestamp;    //!<Temporarily unused
    INT64 pkt_pos;                  //!<Temporarily unused
    struct av_rational sample_aspect_ratio;  //!<Sample aspect ratio of the frame
    INT32 width, height;            //!<Width and height of the video frame
    INT32 format;                   //!<Temporarily unused
};

/*! @struct vdec_decore_status
@brief Video decoder's status in media player mode
*/
struct vdec_decore_status
{
    UINT32 decode_status;     //!<Decode state
    UINT32 pic_width;         //!<Picture width
    UINT32 pic_height;        //!<Picture height
    UINT32 sar_width;         //!<Sample aspect ratio width
    UINT32 sar_height;        //!<Sample aspect ratio height
    UINT32 frame_rate;        //!<Frame rate
    INT32 interlaced_frame;   //!<Whether the stream is interlaced
    INT32 top_field_first;    //!<Whether the stream is top field first
    INT32 first_header_got;   //!<Whether the first header is parsed
    INT32 first_pic_showed;   //!<Whether the first picture is displayed
    UINT32 frames_decoded;    //!<Number of frames decoded
    UINT32 frames_displayed;  //!<Number of frames displayed
    INT64 frame_last_pts;     //!<PTS of last displayed frame
    UINT32 buffer_size;       //!<Total es buffer size
    UINT32 buffer_used;       //!<Used size of es buffer
    UINT32 decode_error;      //!<Decoder error type
    UINT32 decoder_feature;   //!<Decoder feature
    UINT32 under_run_cnt;     //!<Times of under run
    INT32 first_pic_decoded;  //!<Whether the first picture is decoded
    INT32 output_mode;        //!<Decoder's output mode
    UINT32 frame_angle;       //!<Decoder's output frame angle
    INT64 first_i_frame_pts;  //!<First I frame's pts
    UINT8 layer;
    UINT8 rect_switch_done;
};

/*!
@brief
Open the DECV module.
@param[in] dev       Pointer to DECV module.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_open(struct vdec_device *dev);

/*!@brief
Close the DECV module.
@param[in] dev       Pointer to DECV module.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_close(struct vdec_device *dev);

/*!
@brief
Start decoding of the DECV module.
@param[in] dev       Pointer to DECV module.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_start(struct vdec_device *dev);

/*!
@brief
Stop decoding of the DECV module.
@param[in] dev        Pointer to DECV module.
@param[in] bclosevp   The flag of closing dis module. 1: close; 0: not close.
@param[in] bfillblack The flag of filling black memory. 1: fill black; 0: black.
@return RET_CODE
@retval RET_SUCCESS   Requested successfully.
@retval !RET_SUCCESS  Failed to request due to parameter error or status error.
*/
RET_CODE vdec_stop(struct vdec_device *dev,BOOL bclosevp,BOOL bfillblack);

/*!
@brief
Apply written memeroy area to DECV module.
@param[in] dev            Pointer to DECV module.
@param[in] size_requested The memeroy size requested.
@param[out] v_data        The address of gotten memeroy area.
@param[out] size_got      The actual gotten memory size.
@param[in] ctrl_blk       Control information related with written data, including synchronous information,etc.
@return RET_CODE
@retval RET_SUCCESS       Requested successfully.
@retval !RET_SUCCESS      Failed to request due to parameter error or status error.
*/
RET_CODE vdec_vbv_request(void *dev, UINT32 size_requested, void **v_data, \
                             UINT32 *size_got, struct control_block * ctrl_blk);

/*!
@brief
Update the written data pointer of DECV module.
@param[in] dev       Pointer to DECV module.
@param[in] data_size Length of data written successfully.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
void     vdec_vbv_update(void *dev, UINT32 data_size);

/*!
@brief
Set the screeen output mode of the DECV module, including full screen and preview mode.
@param[in] dev            Pointer to DECV module.
@param[in] mode           The work mode of the DECV module.
@param[in] init_info      The initialization information of work mode.
@param[out] mp_call_back  The callback function related with main picture layer.
@param[out] pip_call_back The callback function related with auxiliary picture layer. Not supported now.
@return RET_CODE
@retval RET_SUCCESS      Requested successfully.
@retval !RET_SUCCESS     Failed to request due to parameter error or status error.
*/
RET_CODE vdec_set_output(struct vdec_device *dev, enum VDEC_OUTPUT_MODE mode,struct vdec_pipinfo *init_info, \
                           struct mpsource_call_back *mp_call_back, struct pipsource_call_back *pip_call_back);

/*!
@brief
 Set the audio/video synchronization mode of the DECV module.
@param[in] dev        Pointer to DECV module.
@param[in] sync_mode  Synchronization mode, including VDEC_SYNC_PTS and VDEC_SYNC_FREERUN.
@param[in] sync_level Synchronization level.
@return RET_CODE
@retval RET_SUCCESS   Requested successfully.
@retval !RET_SUCCESS  Failed to request due to parameter error or status error.
*/
RET_CODE vdec_sync_mode(struct vdec_device *dev, UINT8 sync_mode,UINT8 sync_level);

/*!
@brief
The IO contorl operation of DECV moduel.
@param[in] dev           Pointer to DECV module.
@param[in] io_code       The operation command type. Please refer to definition of VDEC_IO_XX.
@param[in,out] param     The operation parameters , different commands have different parameters.
@return RET_CODE
@retval RET_SUCCESS      Requested successfully.
@retval !RET_SUCCESS     Failed to request due to parameter error or status error.
@note   IO Command dwCmd Introduction
<table class="doxtable"  width="800" border="1" style="border-collapse:collapse;table-layout:fixed;word-break:break-all;" >
  <tr>
    <th width="200">   Command   </th>
    <th width="200">   Parameter   </th>
    <th width="80">   Transmission Characteristics   </th>
    <th width="320">   Comment   </th>
  </tr>

  <tr align="center">
    <td>VDEC_IO_GET_STATUS</td>
    <td>struct vdec_status_info *</td>
    <td>Output   </td>
    <td>Read status information   </td>
  </tr>

  <tr align="center">
    <td>VDEC_IO_GET_MODE</td>
    <td>enum TVSystem *</td>
    <td>Output   </td>
    <td>Read video source mode.  </td>
  </tr>

  <tr align="center">
    <td>VDEC_IO_GET_OUTPUT_FORMAT</td>
    <td>BOOL *</td>
    <td>Output   </td>
    <td>Read line scanning information, progressive or interlace.   </td>
  </tr>

  <tr align="center">
    <td>VDEC_IO_FILL_FRM</td>
    <td>struct YCbCrColor *</td>
    <td>Input   </td>
    <td>Write specified color to video buffer   </td>
  </tr>

  <tr align="center">
    <td>VDEC_IO_REG_CALLBACK</td>
    <td>struct vdec_io_reg_callback_para *</td>
    <td>Input   </td>
    <td>Register callback function   </td>
  </tr>

  <tr align="center">
    <td>VDEC_IO_SET_OUTPUT_RECT</td>
    <td>struct VDecPIPInfo *</td>
    <td> Input   </td>
    <td>Set parameters related with preivew mode   </td>
  </tr>

 <tr align="center">
    <td>VDEC_IO_COLORBAR</td>
    <td>Void   </td>
    <td>Input   </td>
    <td>Write color bar signals to video buffer </td>
  </tr>
*/
RET_CODE vdec_io_control(struct vdec_device *dev, UINT32 io_code, UINT32 param);

/*!
@brief
Check current video decoder type.
@return BOOL
@retval 0  The current video decoder type is not H.264.
@retval !0 The current video decoder type is H.264.
*/
BOOL is_cur_decoder_avc(void);

/*!
@brief
Get the device pointer of current video decoder.
@return struct vdec_device
@retval !NULL  Get the pointer to DECV module.
@retval NULL   Failed to request due to parameter error or status error.
*/
struct vdec_device * get_selected_decoder(void);

/*!
@brief
Set the video decoder type.
@param[in] select      Flag of decoder type. 0: MPEG2; 1: H.264.
@param[in] in_preview  Work mode of DECV module. 1: preview; 0: full screen.
*/
void h264_decoder_select(int select, BOOL in_preview);

/*!
@brief
Set a new interface for video decoder type, supporting MPEG2, H.264, AVS.
@param[in] select     Flag of decoder type. 0: MPEG2; 1: H.264; 2: AVS.
@param[in] in_preview Work mode of DECV module. 1: preview; 0: full screen.
*/
void video_decoder_select(enum video_decoder_type select, BOOL in_preview);

/*!
@brief
Check current video decoder type.
@return enum
@retval  0  The current video decoder type is MPEG2.
@retval  1  The current video decoder type is H.264.
@retval  2  The current video decoder type is AVS.
*/
enum video_decoder_type get_current_decoder(void);

/*!
@brief
Play encryption bitstream in DVR application.
@param[in] dev       Pointer to DECV module.
@param[in] param     The flag of encryption bitstream.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_dvr_set_param(struct vdec_device *dev, struct vdec_dvr_config_param param);

/*!
@brief
Pause decoding of DECV module in DVR application.
@param[in] dev       Pointer to DECV module.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_dvr_pause(struct vdec_device *dev);

/*!
@brief
Resume decoding of DECV module in DVR application.
@param[in] dev       Pointer to DECV module.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_dvr_resume(struct vdec_device *dev);

/*!
@brief
Set work mode of DECV module in DVR application.
@param[in] dev       Pointer to DECV module.
@param[in] direction Direction of video play, forward or backward.
@param[in] speed     Speed of video play.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_playmode(struct vdec_device *dev, enum vdec_direction direction, enum vdec_speed speed);

/*!
@brief
Play video in step way in DVR application.
@param[in] dev       Pointer to DECV module.
@return RET_CODE
@retval RET_SUCCESS  Requested successfully.
@retval !RET_SUCCESS Failed to request due to parameter error or status error.
*/
RET_CODE vdec_step(struct vdec_device *dev);


RET_CODE vdec_select_decoder(INT32 vdec_id, enum video_decoder_type vdec_type);
RET_CODE vdec_get_cur_decoder(INT32 vdec_id, enum video_decoder_type *vdec_type);
RET_CODE vdec_get_cur_device(INT32 vdec_id, UINT32 **vdec_dev);
RET_CODE vdec_get_cur_dev_name(INT32 vdec_id, UINT32 *vdec_name, UINT8 len);

int vdec_decore_ioctl(void *phandle, int cmd, void *param1, void *param2);
void vdec_m36_attach(struct vdec_config_par *pconfig_par);
void vdec_avc_attach(struct vdec_avc_config_par *pconfig_par);
void vdec_hevc_attach(struct vdec_hevc_config_par *pconfig_par);
void vdec_avs_attach(struct vdec_avs_config_par *pconfig_par);

RET_CODE vdec_enable_advance_play(struct vdec_device *dev);

#ifdef __cplusplus
}
#endif
/*!
@}
*/

/*!
@}
*/
#endif

