#ifndef	__ADF_VPO_H_
#define	__ADF_VPO_H_

#include "adf_basic.h"
#include "adf_media.h"

#ifdef __cplusplus
extern "C" {
#endif



#define	VP_S3601_SOURCE_MAX_NUM 4
#define	VP_S3601_WINDOW_MAX_NUM 4
#define	VP_S3601_VIDEO_MAX_NUM	4


#define	VP_SOURCE_MAX_NUM 4
#define	VP_WINDOW_MAX_NUM 4
#define	VP_VIDEO_MAX_NUM	4

//3vpo_zoom
#define PICTURE_WIDTH 720
#define PICTURE_HEIGHT 2880	         //!<  2880 is the lease common multiple of screen height of PAL and NTSC
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 2880

//3vpo_open
#define	VP_DAC_TYPENUM	23
#define DAC0		0x01
#define DAC1		0x02
#define DAC2		0x04
#define DAC3		0x08
#define DAC4		0x10
#define DAC5		0x20

//3vpo_setwinmode
#define	VPO_MAINWIN					0x01
#define	VPO_PIPWIN					0x02
#define	VPO_PREVIEW					0x04
//TVEncoder
#define TTX_START_LINE        7//6
#define TTX_START_LINE_NTSC   8
#define TTX_START_LINE_NTSC_HD 0xB
#define TTX_START_PRESENT_LINE      0x01
#define TTX_BUF_DEPTH        16//18
#define TTX_PRESENT_BYTE_PER_LINE   0x2D
#define TTX_DATA                    0xAA

//tvencoder config bit
#define TVE_CC_BY_VBI               1
#define TVE_TTX_BY_VBI              2
#define TVE_WSS_BY_VBI              4
#define CGMS_WSS_BY_VBI             8
#define YUV_SMPTE                   0x10
#define YUV_BETACAM                 0x20
#define YUV_MII                     0x40
#define TVE_NEW_CONFIG              0x80
#define TVE_FULL_CUR_MODE           0x100
#define TVE_NEW_CONFIG_1            0x200
#define TVE_PLUG_DETECT_ENABLE      0x400

#define	SYS_525_LINE	0
#define	SYS_625_LINE	1
#define VPO_CB_HDMI 0x01
#define VPO_CB_SRCASPRATIO_CHANGE 0x02

#ifndef VIDEO_OPEN_PIP_WIN_FLAG
#define VIDEO_OPEN_PIP_WIN_FLAG   0X80
#endif

// vp_init_info.device_priority
#define VPO_INDEPENDENT_NONE        0   //share the source with sdhd device and share all api setting
#define VPO_INDEPENDENT_IN_OUT      2   //have its own source and its own all api setting
// enable dual-output for S3602F, must call vcap_m36f_attach() before vpo_open();
#define VPO_AUTO_DUAL_OUTPUT        3

#define	VPO_IO_SET_BG_COLOR				    0x01
#define	VPO_IO_SET_VBI_OUT				    0x02
#define	VPO_IO_GET_OUT_MODE				    0x03
#define	VPO_IO_REG_DAC					    0x04
#define	VPO_IO_UNREG_DAC		            0x05
#define VPO_IO_GET_SRC_ASPECT	       	    0x06
#define	VPO_IO_REG_CALLBACK	       		    0x07
#define	VPO_IO_REG_CB_SRCMODE_CHANGE	    0x08
#define	VPO_IO_REG_CB_SRCASPECT_CHANGE	    0x09
#define	VPO_IO_REG_CB_GE				    0x0A
#define	VPO_IO_GET_INFO			            0x0B
#define	VPO_IO_SET_PARAMETER	            0x0C
#define	VPO_IO_PRINTF_HW_INFO	            0x0D
#define	VPO_IO_PRINTF_HW_SCALE_INIT	        0x0E
#define	VPO_IO_PREFRAME_DETECT_ONOFF	    0x0F
#define	VPO_IO_DIGITAL_OUTPUT_MODE		    0x10
#define	VPO_IO_REG_CB_HDMI				    0x10
#define	VPO_IO_HDMI_OUT_PIC_FMT			    0x11
#define VPO_IO_VIDEO_ENHANCE			    0x12
#define VPO_IO_WRITE_TTX                    0x13
#define VPO_IO_WRITE_CC                     0x14
#define VPO_IO_WRITE_WSS                    0x15
#define	VPO_IO_GET_REAL_DISPLAY_MODE	    0x16
#define VPO_IO_MHEG_SCENE_AR	            0x17
#define VPO_IO_MHEG_IFRAME_NOTIFY           0x18
#define VPO_IO_DISAUTO_WIN_ONOFF            0x19
#define VPO_IO_ENABLE_VBI                   0x1A
#define VPO_IO_ENABLE_EXTRA_WINDOW          0x1F
#define VPO_IO_GET_TV_ASPECT	            0x20
#define VPO_IO_DIRECT_ZOOM		            0x21
#define	VPO_IO_DROP_LINE	                0x22
#define VPO_IO_FIX_WSS			            0x23
#define VPO_IO_OUPUT_PIC_DIRECTLY	        0x24
#define VPO_IO_PLAYMODE_CHANGE	            0x25
#define VPO_IO_SET_PROGRES_INTERLC	        0x26
#define VPO_IO_SET_LINEMEET_CNT	            0x27
#define VPO_IO_ADJUST_LM_IN_PREVIEW	        0x28
#define	VPO_IO_DIT_CHANGE		            0x29
#define VPO_IO_SWAFD_ENABLE				    0x30
#define VPO_IO_CHANGE_DEINTERL_MODE	        0x31
#define VPO_IO_704_OUTPUT				    0x32
#define VPO_IO_CHANGE_YC_INIT_PHASE		    0x33
#define VPO_IO_COMPENENT_UPSAMP_REPEAT	    0x34
#define VPO_IO_SET_PREVIEW_MODE			    0x35
#define VPO_IO_SET_DIT_LEVEL			    0x36
#define VPO_IO_CHANGE_CHANNEL               0x37
#define VPO_IO_SET_DIGITAL_OUTPUT_601656    0x38
#define VPO_IO_ADJUST_Y_COMP_FREQRESPONSE	0x39
#define VPO_IO_SD_CC_ENABLE                 0x3a
#define VPO_IO_SET_STILL_PIC				0x40
#define VPO_IO_CB_UPDATE_PALLET				0x41
#define VPO_IO_AFD_CONFIG					0x42
#define VPO_IO_SET_MG_APS_INFO		        0x43
#define VPO_IO_SET_CGMS_INFO				0x44
#define VPO_IO_ADJUST_DIGTAL_YC_DELAY		0x45
#define VPO_IO_TVESDHD_SOURCE_SEL			0x46
#define VPO_IO_CB_AVSYNC_MONITOR			0x47
#define VPO_IO_SET_OSD_SHOW_TIME            0x48    //!<  vpo_osd_show_time_t *
#define VPO_IO_GET_OSD0_SHOW_TIME           0x49    //!<  UINT32 *
#define VPO_IO_GET_OSD1_SHOW_TIME           0x4a    //!<  UINT32 *
#define VPO_IO_GET_CURRENT_DISPLAY_INFO     0x4b   //!< struct vpo_io_get_picture_info *
#define VPO_IO_BACKUP_CURRENT_PICTURE       0x4c    //!< struct vpo_io_get_picture_info *
#define VPO_IO_GET_DISPLAY_MODE             0x4d   //!<  enum display_mode *
#define VPO_IO_ENABLE_DE_AVMUTE_HDMI        0x4e    //!<  BOOL: TRUE/FALSE
#define VPO_IO_SET_DE_AVMUTE_HDMI           0x4f    //!<  BOOL: TRUE/FALSE
#define VPO_IO_GET_DE_AVMUTE_HDMI           0x50    //!< UINT32 * -- output TRUE/FALSE
#define VPO_IO_ALWAYS_OPEN_CGMS_INFO		0x51    //!<  True: always open; False: close it
#define VPO_IO_GET_MP_SCREEN_RECT           0x52    //!<  struct rect *
#define VPO_IO_ENABLE_ICT		            0x53    //!<  True: enable ict; FALSE: disable it
#define VPO_IO_SET_WIN_ONOFF_STATUS_MANUALLY		0x54







#define VPO_IO_GET_MP_INFO                  0x55
#define VPO_IO_SET_SINGLE_OUTPUT            0x56
#define VPO_IO_SET_TVE_CVBS                 0x57
#define VPO_IO_VE_IMPROVE                   0x58
#define VPO_FULL_SCREEN_CVBS                0x59
#define VPO_IO_IMPROVE_GE                   0x60

#define VPO_IO_GET_PIP_USING_ADDR           0x61
#define VPO_IO_SET_MULTIVIEW_MODE           0x62

#define VPO_IO_OTP_SET_VDAC_FS              0x63
#define VPO_IO_SET_VBI_START_LINE           0x64
#define VPO_IO_SET_HDMI_AR_SD4VS3           0x65  //!<Add by ze, for ar 4:3 when sd
#define VPO_IO_SET_MULTIVIEW_BUF            0x66
#define VPO_IO_FILL_GINGA_BUF               0x67
#define VPO_IO_UPDATE_GINGA                 0x68
#define VPO_IO_ENABLE_VDAC_PLUG_DETECT      0x69
#define VPO_IO_SELECT_SCALE_MODE       		0x6a
#define VPO_IO_SET_VER_ACTIVE_LINE          0x6b
#define VPO_IO_GET_AUX_PIC_DISPLAY_INFO     0x6c
#define VPO_IO_BACKUP_AUX_PICTURE           0x6d
#define VPO_IO_SEL_OSD_SHOW_TIME            0x6e
#define VPO_IO_GET_OSD2_SHOW_TIME           0x6f //!< Only support in m3281
#define VPO_IO_SEL_DUPLICATE_MODE           0x70
#define VPO_IO_ENABLE_AUXP_TRANSPARENT      0x71 //!< Only support in m3811
#define VPO_IO_SET_AUXP_TRANSPARENT_AREA    0x72 //!< Only support in m3811
#define VPO_IO_ENABLE_ANTIFLICK             0x73
#define VPO_IO_SET_ANTIFLICK_THR            0x74
#define VPO_IO_BOOT_UP_DONE                 0x75
#define VPO_IO_SET_LOGO_INFO                0x76
#define VPO_IO_SET_PRC_INFO                 0x77
#define VPO_IO_SET_CUTOFF           		0x78
#define VPO_IO_SET_TIMECODE                 0x79
#define VPO_IO_GET_TIMECODE                 0x7a
#define VPO_IO_CANCEL_TIMECODE              0x7b
#define VPO_IO_CLOSE_DEO                    0x7c
#define VPO_IO_GET_DUAL_OUTPUT_DELAY        0x80
#define VPO_IO_GET_VCAP_INFO                0x81
#define VPO_IO_GET_BEST_DESUB				0x82
#define VPO_IO_GET_DEO_DELAY                0x83
#define VPO_IO_CLEAN_CURRENT_STILL_PICTURE	0x84
#define VPO_IO_FREE_BACKUP_PICTURE			0x85
#define VPO_IO_SET_PREVIEW_SAR_MODE			0x86
#define VPO_IO_SET_DISPLAY_STYLE            0X87
#define VPO_IO_EXCHANGE_VIDEO_LAYER         0x88

#define VPO_IO_GET_VBI_TTX_ISR              0x89
#define VPO_IO_WRITE_TTX_PACKET             0x8a
#define VPO_IO_GET_MP_SOURCE_RECT           0x8b
#define VPO_IO_DISABLE_FBC                  0x8c
#define VPO_IO_OTP_SET_VDAC_FS_OFFSET       0x8d
#define VPO_IO_SET_JOINT_MODE               0x8e

#define VPO_IO_CGMS_STATUS_GET              0X8F

#define VPO_IO_ELEPHANT_BASE             	0x10000
#define VPO_IO_CHOOSE_HW_LAYER		        (VPO_IO_ELEPHANT_BASE + 0x01)
#define VPO_IO_GLOBAL_ALPHA			        (VPO_IO_ELEPHANT_BASE + 0x02)
#define VPO_IO_SET_MEMMAP_MODE              (VPO_IO_ELEPHANT_BASE + 0x03)
#define VPO_IO_PILLBOX_CUT_FLAG		        (VPO_IO_ELEPHANT_BASE + 0x04)
#define VPO_IO_SET_LAYER_ORDER              (VPO_IO_ELEPHANT_BASE + 0x05) //!<  enum VP_LAYER_ORDER
#define VPO_IO_GET_LAYER_ORDER              (VPO_IO_ELEPHANT_BASE + 0x08) //!< UINT32 *
#define VPO_IO_SET_ENHANCE_ENABLE           (VPO_IO_ELEPHANT_BASE + 0x07) //!<  TRUE/FALSE, default TRUE=enable
#define VPO_IO_GET_DE2HDMI_INFO             (VPO_IO_ELEPHANT_BASE + 0x06) //!<  struct de2Hdmi_video_infor *
#define VPO_IO_GET_PRE_FB_ADDR              (VPO_IO_ELEPHANT_BASE + 0x09) //!< UINT32 *
//#define VPO_IO_CLOSE_HW_OUTPUT			(VPO_IO_ELEPHANT_BASE + 0x0A)
#define VPO_IO_SET_EP_LEVERL                (VPO_IO_ELEPHANT_BASE + 0x0A) //only for m3281
#define VPO_IO_SET_COLOR_SHIFT              (VPO_IO_ELEPHANT_BASE + 0x0B)
#define VPO_IO_SET_CUTLINE		 			(VPO_IO_ELEPHANT_BASE + 0x0C)
#define VPO_IO_SET_INIT_TV_SYS				(VPO_IO_ELEPHANT_BASE + 0x0D)
#define VPO_IO_DISPLAY_3D_ENABLE		 	(VPO_IO_ELEPHANT_BASE + 0x0E)
#define VPO_IO_SET_3D_ENH               	(VPO_IO_ELEPHANT_BASE + 0x0F)
#define VPO_IO_GET_CURRENT_3D_FB_ADDR		(VPO_IO_ELEPHANT_BASE + 0x10)
#define VPO_IO_DISPLAY_3D_STATUS        	(VPO_IO_ELEPHANT_BASE + 0x11)
//#define VPO_IO_BOOT_UP_DONE			 	(VPO_IO_ELEPHANT_BASE + 0x12)
#define VPO_IO_SET_VBI_STARTLINE			(VPO_IO_ELEPHANT_BASE + 0x12)
#define VPO_IO_GLOBAL_ALPHA_AUXP            (VPO_IO_ELEPHANT_BASE + 0x13)
#define VPO_IO_SUSPEND_DE    			    (VPO_IO_ELEPHANT_BASE + 0x14) //!< NULL
#define VPO_IO_RESUME_DE                    (VPO_IO_ELEPHANT_BASE + 0x15) //!< NULL
#define VPO_IO_ENABLE_VCR_PROTECTION        (VPO_IO_ELEPHANT_BASE + 0x16)
#define VPO_IO_SET_BOOT_LOGO_STATUS			(VPO_IO_ELEPHANT_BASE + 0x18) //!<  Only for SEE CPU
#define VPO_IO_GET_BOOT_LOGO_STATUS			(VPO_IO_ELEPHANT_BASE + 0x19) //!<  Only for SEE CPU
#define VPO_IO_GET_MP_ONOFF					(VPO_IO_ELEPHANT_BASE + 0xFE) //!< Only for SEE CPU
#define VPO_IO_SHOW_VIDEO_SMOOTHLY			(VPO_IO_ELEPHANT_BASE + 0xFF) //!<  Only for SEE CPU 
#define VPO_IO_GET_DAC_INFO					(VPO_IO_ELEPHANT_BASE + 0x100)//!<  To get dac info,including enabled or not and signal type
#define VPO_IO_GET_CGMS_INFO				(VPO_IO_ELEPHANT_BASE + 0x101)//!<  To get dac info,including enabled or not and signal type


#define	VPO_IO_SET_PARA_FETCH_MODE	        0x01
#define	VPO_IO_SET_PARA_DIT_EN		        0x02
#define	VPO_IO_SET_PARA_VT_EN		        0x04
#define	VPO_IO_SET_PARA_V_2TAP		        0x08
#define	VPO_IO_SET_PARA_EDGE_PRESERVE_EN	0x10
#define VPO_IO_SET_ENHANCE_BRIGHTNESS       0x01    //!< value[0, 100], default 50
#define VPO_IO_SET_ENHANCE_CONTRAST         0x02    //!<  value[0, 100], default 50
#define VPO_IO_SET_ENHANCE_SATURATION	    0x04    //!<  value[0, 100], default 50
#define VPO_IO_SET_ENHANCE_SHARPNESS        0x08    //!<  value[0, 10 ], default 5
#define VPO_IO_SET_ENHANCE_HUE              0x10    //!<  value[0, 100], default 50

//tvencoder adjustable register define
#define TVE_ADJ_COMPOSITE_Y_DELAY       0
#define TVE_ADJ_COMPOSITE_C_DELAY       1
#define TVE_ADJ_COMPONENT_Y_DELAY       2
#define TVE_ADJ_COMPONENT_CB_DELAY      3
#define TVE_ADJ_COMPONENT_CR_DELAY      4
#define TVE_ADJ_BURST_LEVEL_ENABLE      5
#define TVE_ADJ_BURST_CB_LEVEL          6
#define TVE_ADJ_BURST_CR_LEVEL          7
#define TVE_ADJ_COMPOSITE_LUMA_LEVEL    8
#define TVE_ADJ_COMPOSITE_CHRMA_LEVEL   9
#define TVE_ADJ_PHASE_COMPENSATION      10
#define TVE_ADJ_VIDEO_FREQ_RESPONSE     11
//secam adjust value
#define TVE_ADJ_SECAM_PRE_COEFFA3A2     12
#define TVE_ADJ_SECAM_PRE_COEFFB1A4     13
#define TVE_ADJ_SECAM_PRE_COEFFB3B2     14
#define TVE_ADJ_SECAM_F0CB_CENTER       15
#define TVE_ADJ_SECAM_F0CR_CENTER       16
#define TVE_ADJ_SECAM_FM_KCBCR_AJUST    17
#define TVE_ADJ_SECAM_CONTROL           18
#define TVE_ADJ_SECAM_NOTCH_COEFB1      19
#define TVE_ADJ_SECAM_NOTCH_COEFB2B3    20
#define TVE_ADJ_SECAM_NOTCH_COEFA2A3    21
#define TVE_ADJ_VIDEO_DAC_FS			22
#define TVE_ADJ_C_ROUND_PAR				23

//advance tvencoder adjustable register define
#define TVE_ADJ_ADV_PEDESTAL_ONOFF              0
#define TVE_ADJ_ADV_COMPONENT_LUM_LEVEL         1
#define TVE_ADJ_ADV_COMPONENT_CHRMA_LEVEL       2
#define TVE_ADJ_ADV_COMPONENT_PEDESTAL_LEVEL    3
#define TVE_ADJ_ADV_COMPONENT_SYNC_LEVEL        4
#define TVE_ADJ_ADV_RGB_R_LEVEL                 5
#define TVE_ADJ_ADV_RGB_G_LEVEL                 6
#define TVE_ADJ_ADV_RGB_B_LEVEL                 7
#define TVE_ADJ_ADV_COMPOSITE_PEDESTAL_LEVEL    8
#define TVE_ADJ_ADV_COMPOSITE_SYNC_LEVEL        9
#define TVE_ADJ_ADV_PLUG_OUT_EN                 10
#define TVE_ADJ_ADV_PLUG_DETECT_LINE_CNT_SD     11
#define TVE_ADJ_ADV_PLUG_DETECT_AMP_ADJUST_SD   12
#define TVE_ADJ_ADV_CLAM_Y_LEVEL_MAX            13
#define TVE_ADJ_ADV_BUFFER_MODE                 14
#define TVE_ADJ_ADV_YFIR_EN                     15
#define TVE_ADJ_ADV_CFIR_EN                     16
#define TVE_ADJ_ADV_SYNC_DELAY                  17

enum vp_display_layer
{
	VPO_LAYER_MAIN = 1,                            //!<  Changed for S3601-6/5.
	VPO_LAYER_GMA1 = 2,
	VPO_LAYER_GMA2,
	VPO_LAYER_CUR,
	VPO_LAYER_AUXP = 0x10,                       //!<  For Auxiliary Picture Layer.
};

enum vp_display_layer_29e
{
    VPO_LAYER_M = 1,
    VPO_LAYER_PIP,
    VPO_LAYER_OSD,
    VPO_LAYER_ST1,  // 3101F
    VPO_LAYER_ST2,  // 3101F
};

enum VP_LAYER_ORDER
{

    // 3101F
    VPO_MP_PIP_OSD_ST1_ST2 = 0x00,
    VPO_MP_PIP_ST1_OSD_ST2 = 0x01,
};

enum VP_AFD_SOLUTION
{
	VP_AFD_COMMON,     //!<  UTG specification.
	VP_AFD_MINGDIG,    //!< MINGDIG specification.
};

enum VP_LAYER_BLEND_ORDER
{
	MP_GMAS_GMAF_AUXP =0,
	MP_GMAS_AUXP_GMAF,
	MP_GMAF_GMAS_AUXP,
	MP_GMAF_AUXP_GMAS,
	MP_AUXP_GMAS_GMAF,
	MP_AUXP_GMAF_GMAS,
	AUXP_MP_GMAS_GMAF,
	AUXP_MP_GMAF_GMAS,
	GMAS_MP_GMAF_AUXP,
	RED_BLUE_3D_LAYER,
};

enum VP_TVESDHD_SOURCE
{
	TVESDHD_SRC_DEN,     //!<  TV Encoder connects to DEN.
	TVESDHD_SRC_DEO,     //!<  TV Encoder connects to DEO.
};

enum vga_mode
{
	VGA_NOT_USE = 0,     //!<  Close VGA interface.
	VGA_640_480,         //!<  Not implemented.
	VGA_800_600          //!<  Not implemented.
};

enum dac_type
{
	CVBS_1 = 0,
	CVBS_2,
	CVBS_3,
	CVBS_4,
	CVBS_5,
	CVBS_6,
	SVIDEO_1,
	SVIDEO_2,
	SVIDEO_3,
	YUV_1,
	YUV_2,
	RGB_1,
	RGB_2,
	SVIDEO_Y_1,
	SECAM_CVBS1,
	SECAM_CVBS2,
	SECAM_CVBS3,
	SECAM_CVBS4,
	SECAM_CVBS5,
	SECAM_CVBS6,
	SECAM_SVIDEO1,
	SECAM_SVIDEO2,
	SECAM_SVIDEO3,
};

enum VPO_DIT_LEVEL
{
	VPO_DIT_LEVEL_HIGH ,
	VPO_DIT_LEVEL_MEDIUM,
	VPO_DIT_LEVEL_LOW,
};


struct vpo_status_info
{
	UINT8	case_index;
	UINT8	status_index;
	UINT32	display_frm_step;
};

/*! @struct dac_index
@brief Configuration information of video interface.
 */
struct dac_index
{
	UINT8 u_dac_first;                //!< The first group configuration interface (CVBS, YC_Y, YUV_Y, RGB_R)
	UINT8 u_dac_second;               //!< The second group configuration interface (YC_C, bYUV_U, RGB_G)
	UINT8 u_dac_third;                //!< The third group configuration interface (YUV_V, RGB_B)
};

/*! @struct vp_dac_info
@brief Configuration information of video interface.
*/
struct vp_dac_info
{
	BOOL					b_enable; //!< Enable flag.
	//enum dac_type 			e_dac_type; //for all
	struct dac_index			t_dac_index;                 //!< Interface configuration information
	enum vga_mode			e_vgamode;                   //!< Not implemented.
	BOOL					b_progressive;                    //!< Progressive output flag
};

typedef enum TVEDacOutput_e
{
		DAC_CAV_Y = 0,
		DAC_CAV_PB,
		DAC_CAV_PR,
		DAC_CAV_RGB_R,
		DAC_CAV_RGB_G,
		DAC_CAV_RGB_B,
		DAC_CVBS,
		DAC_YC_Y,
		DAC_YC_C,
		DAC_SD_DAC1,//9
		DAC_SD_DAC2,//10
		DAC_SD_DAC3//11
}TVEDacOutput_t;
	
struct vp_dac_state
{
	UINT8 dac0_enabled;							// For hd
	TVEDacOutput_t dac0_signal_type; 
				
	UINT8 dac1_enabled;							// For hd
	TVEDacOutput_t dac1_signal_type;
						
	UINT8 dac2_enabled;							// For hd
	TVEDacOutput_t dac2_signal_type;
					
	UINT8 dac3_enabled;							// For sd 
	TVEDacOutput_t dac3_signal_type;
};


/*! @struct vpo_io_reg_dac_para
@brief Parameters definition needed by VPO_IO_REG_DAC, for registering video interface.
*/
struct vp_io_reg_dac_para
{
	enum dac_type e_dac_type; //!< Interface type.
	struct vp_dac_info dac_info; //!< Interface configuration information.
};

typedef void (* vp_cbfunc)(UINT32 u_param1);

enum vp_cbtype
{
    VPO_CB_REPORT_TIMECODE = 0x03,
};

struct vp_io_reg_callback_para
{
    enum vp_cbtype e_cbtype;
    vp_cbfunc p_cb;
};

struct vp_io_afd_para
{
    BOOL b_swscale_afd;
    int  afd_solution;
    int  protect_mode_enable;
};
struct vp_gma_lc_value
{
	int  layer_id;
	int  value;
};

typedef enum
{
    REPORT_TIME_CODE = 1,
    FREEZE_FRAME_AT_TIME_CODE
} time_code_mode;

struct vpo_io_set_time_code
{
   struct time_code_t time_code;
   time_code_mode mode;
};

typedef enum
{
    TIME_OUT_REPORT = 0,
    FREEZE_FRAME,
    CANCEL_FREEZE_FRAME,
    CANCEL_REPORT
} report_type;


typedef void (* VP_SRCMODE_CHANGE)(enum tvsystem);
typedef void (* VP_SRCASPECT_CHANGE)(enum asp_ratio, enum tvmode);
typedef void (* VP_SRCASPRATIO_CHANGE)(UINT8);
typedef BOOL (* vp_mpget_mem_info)(UINT32 *);
typedef void (*VP_CB_VSYNC_CALLBACK)(UINT32 );

struct vpo_callback
{
    VP_SRCASPRATIO_CHANGE    p_src_asp_ratio_change_call_back;
    vp_cbfunc    phdmi_callback;
    vp_cbfunc    report_timecode_callback;
};


/*! @struct vp_init_info
@brief Initialization information of video output.
*/
struct vp_init_info
{
	//api set backgound color
	struct  ycb_cr_color		t_init_color;           			//!< Background color.
	//set advanced control
	UINT8 						b_brightness_value;              	//!<Not implemented.
	BOOL 						f_brightness_value_sign;          	//!<Not implemented.
	UINT16 						w_contrast_value;                	//!<Not implemented.
	BOOL 						f_contrast_sign;                 	//!<Not implemented.
	UINT16 						w_saturation_value;              	//!<Not implemented.
	BOOL						f_saturation_value_sign;          	//!<Not implemented.
	UINT16						w_sharpness;                    	//!<Not implemented.
	BOOL 						f_sharpness_sign;             		//!<Not implemented.
	UINT16 						w_hue_sin;                  		//!<Not implemented.
	BOOL 						f_hue_sin_sign;               		//!<Not implemented.
	UINT16						w_hue_cos;                    		//!<Not implemented.
	BOOL						f_hue_cos_sign;                   	//!<Not implemented.

	enum tvmode 				e_tvaspect; 						//!<  Aspect ratio
	enum display_mode 			e_display_mode; 					//!<  Display mode.
	UINT8 						u_nonlinear_change_point;  			//!<Not implemented.
	UINT8 						u_pan_scan_offset;  				//!<Not implemented.

	struct vp_dac_info			p_dac_config[VP_DAC_TYPENUM]; 		//!<  Configuration information of video interface.
	enum tvsystem 				e_tvsys;                  			//!<  TV system.

	//3config win on/off and mode
	BOOL						b_win_on_off; 						//!<  Open video layer flag.
	UINT8						u_win_mode;  						//!< Screen output mode.
	struct rect					t_src_rect;  						//!< Input rectangle information.
	struct rect					dst_rect;  							//!<  Output rectangle information.

	struct mpsource_call_back	t_mpcall_back;   					//!<Not implemented.
	struct pipsource_call_back	t_pipcall_back;   					//!<Not implemented.
    VP_SRCMODE_CHANGE           p_src_change_call_back;
    UINT8						device_priority;  					//!<Not implemented.
	BOOL 						b_ccir656enable;  					//!<Not implemented.
    BOOL                        b_progressive;
    enum VP_TVESDHD_SOURCE      e_tvesdhd_src;
};
struct vp_ngwm_callback
{
    
    int (*ngwm_ext_set_setting)(int id, int type);
    
    int (*ngwm_ext_set_xposition)(int id, int start, int end);
    
    int (*ngwm_ext_set_yposition)(int id, int start, int end);

    int (*ngwm_ext_set_input_mode)(int id, int mode);

    int (*ngwm_ext_set_frame_type)(int id, int type);

    int (*ngwm_ext_set_frame_rate)(int id, int rate);
    
};
struct vp_feature_config
{
    BOOL b_osd_mulit_layer;
    BOOL b_mheg5enable;
    BOOL b_avoid_mosaic_by_freez_scr;
    BOOL b_support_extra_win;
    BOOL b_overshoot_solution;
    BOOL b_p2ndisable_maf;
    BOOL b_adpcmenable;
    vp_mpget_mem_info p_mpget_mem_info;
    VP_SRCASPRATIO_CHANGE    p_src_asp_ratio_change_call_back;
    BOOL b_disable_multivew_fun;
    BOOL bl_fireware_smoothly;

    BOOL b_malloc_use_reserve_memory;
    UINT32 reserve_memory_addr;
    UINT32 reserve_memory_len;
    
	BOOL   b_config_still_picture_memroy;
    UINT32 still_picture_memory_addr;
    UINT32 still_picture_memory_len;

    struct vp_ngwm_callback ngwm_callback;
    
};
struct vp_direct_output
{
	UINT8	direct_output_array_len;
	UINT8 	reserved;
	UINT16	direct_output_array[5];
};
struct vp_mp_info
{
    UINT32 pic_height;
    UINT32 pic_width;
    UINT32 y_buf_addr;
    UINT32 c_buf_addr;
    UINT8  de_map_mode;
};

struct vpo_io_vcap_info
{
    UINT32 buf_addr;
    UINT32 buf_size;
};





typedef struct vpo_osd_show_time_s

{
    UINT8  show_on_off;     //!< True or False.
    UINT8  layer_id;        //!< 0 or 1
    UINT8  reserved0;
    UINT8  reserved1;
    UINT32 time_in_ms;      // in ms
} vpo_osd_show_time_t, *p_vpo_osd_show_time_t;

//Add for VE pause/slow/ff play mode, DE change DIT mode to increase display quality
enum vp_play_mode
{
	NORMAL_PLAY,
	NORMAL_2_ABNOR,
	ABNOR_2_NORMAL
};

/*! @struct vpo_io_get_info
@brief Parameters definition needed by VPO_IO_GET_INFO, for reading module information.
*/
struct vpo_io_get_info
{
	UINT32 display_index;                              //!< Frame numbers displayed.
	UINT32	api_act;                                  //!< Not implemented.
	BOOL	bprogressive;                               //!< Progressive flag of output information.
	enum tvsystem	tvsys;                             //!< TV system of output signal.
	BOOL	fetch_mode_api_en;                         //!< Not implemented.
	enum deinterlacing_alg	fetch_mode;                  //!< Not implemented.
	BOOL	dit_enable;                                  //!< Dit flag.
	BOOL	vt_enable;                                  //!< VT flag.
	BOOL	vertical_2tap;                            //!< Not implemented.
	BOOL	edge_preserve_enable;                      //!< Not implemented.
	UINT16	source_width;                            //!< Width of video source.
	UINT16	source_height;                          //!< Height of video source.
	UINT16    des_width;                           //!< Width of screen.
	UINT16	des_height;                           //!< Height of screen.
	BOOL	preframe_enable;                         //!< Not implemented.
	BOOL	gma1_onoff;                              //!< Flag of opening GMA1.
	UINT32	reg90;                                 //!< Not implemented.


    BOOL	mp_on_off;                                   //!< Flag of opening main picture.
    UINT32  scart_16_9;                              //!< Whether it is 16:9 or not.
};


struct vpo_io_set_parameter
{
	UINT8	changed_flag;
	BOOL	fetch_mode_api_en;
	enum deinterlacing_alg	fetch_mode;
	BOOL	dit_enable;
	BOOL	vt_enable;
	BOOL	vertical_2tap;
	BOOL	edge_preserve_enable;
};

struct pip_pic_info

{
    UINT8 index;
    UINT32 pic_addr;
};



struct vpo_io_get_picture_info
{
    UINT8   de_index; //!< Input parameter. 0: DE_N; 1: DE_O.
    UINT8   sw_hw;    //!< Input parameter. 0: software register; 1: hardware register.
    UINT8   status;  //!< Input value is initialized to 0.
                     //!<h Output value:

                     //!<h 0: this control command is not implemented;
                    //!< 1: this control command is implemented and no picture is displayed;
                    //!< 2: this control command is implemented and one picture is displayed now.
                    //!< The following parameters contain the information of this picture.
    UINT32  top_y;
    UINT32  top_c;
    UINT32  bot_y;
    UINT32  bot_c;
    UINT32  maf_buffer;
    UINT32  y_buf_size;
    UINT32  c_buf_size;
    UINT32  maf_buf_size;
    UINT32  reserved[10];
};

/*! @struct vpo_io_video_enhance
@brief Parameters definition needed by VPO_IO_VIDEO_ENHANCE, for setting picture enhancement information.
*/
struct vpo_io_video_enhance
{
	UINT8	changed_flag;               //!<Picture enhancement type. VPO_IO_SET_ENHANCE_XX means enhancement type.
	UINT16   grade;                   //!<The picture enhancement value. Range of 0 ~ 100, default 50 means no enhancement.
    UINT16   grade_range_min;          //!<Range of min
    UINT16   grade_range_max;          //!<Range of max
};

struct ali_vbi_ttx_pars
{
	BOOL  flag;
	UINT8 field;
};

struct vpo_io_ttx
{
        UINT8 line_addr;
        UINT8 addr;
        UINT8 data;
};

struct vpo_io_ttx_packet
{
        UINT8 LineAddr[16];
        UINT8 Addr[45];
        UINT8 Data[45];
};

struct vpo_io_cc
{
        UINT8 field_parity;
        UINT16 data;
};

/*! @struct vpo_io_global_alpha
@brief  Parameters definition needed by VPO_IO_GLOBAL_ALPHA, for setting global transparency of video layer.
*/
struct vpo_io_global_alpha
{
	UINT8 value; //!<The transparency value, range of 0~255;0 means full transparency.
	UINT8 valid; //!< Valid flag.
};

struct vpo_io_cgms_info
{
	UINT8 cgms;
	UINT8 aps;
};

struct vpo_io_cutline_pars
{
    	UINT8 top_bottom;             //!<0: Display from top; 1: Display from bottom
    	UINT8 cut_line_number;
};



struct vpo_io_set_ver_active_line

{
   UINT16 odd_start_active_line;
   UINT16 odd_end_active_line;
   UINT16 even_start_active_line;
   UINT16 even_end_active_line;
};

/*! @struct VP_CutOff

@brief  Configuration information of cutoff parameter.

*/

struct vpo_io_set_cutoff
{
    UINT32 reserve;
    UINT32 cutoff_hor;
    UINT32 cutoff_ver;
};

/*
typedef enum
{
    REPORT_TIME_CODE = 1,
    FREEZE_FRAME_AT_TIME_CODE
} time_code_mode;
struct vpo_io_set_time_code
{
   struct time_code_t time_code;
   time_code_mode mode;
};

typedef enum
{
    TIME_OUT_REPORT = 0,
    FREEZE_FRAME,
    CANCEL_FREEZE_FRAME,
    CANCEL_REPORT
} report_type;
*/
struct vpo_io_logo_info
{
	UINT32 pic_width;
	UINT32 pic_height;
    UINT32 src_aspect_ratio;
    enum tvmode hdmi_aspect_ratio;
};
/*
#define SYS_576I	0
#define SYS_480I	1
#define SYS_576P	2
#define SYS_480P	3
#define SYS_720P_50		4
#define SYS_720P_60		5
#define SYS_1080I_25	6
#define SYS_1080I_30	7
*/
typedef enum _e_tv_sys
{
    SYS_576I	,
    SYS_480I	,
    SYS_576P	,
    SYS_480P	,
    SYS_720P_50	,
    SYS_720P_60	,
    SYS_1080I_25,
    SYS_1080I_30,

    SYS_1080P_24,
    SYS_1080P_25,
    SYS_1080P_30,

    SYS_1152I_25,
    SYS_1080IASS,

    SYS_1080P_50,
    SYS_1080P_60,

    TVE_SYS_NUM,
}t_e_tve_sys;
struct vp_src_dst_rect
{
	struct rect src_rect;
	struct rect dst_rect;
};

struct vp_win_config
{
	UINT8	source_index;//!<  Source index.
	enum vp_display_layer	display_layer;//!< Displayed picture layer.
	//UINT8	win_mode;
	void * src_module_devide_handle; //!<Not implemented.
	struct source_callback	src_callback;  //!<Not implemented.
	struct vp_src_dst_rect 	rect;//!<  Source displaying area information.
};
struct vp_source_info
{
	void * src_module_devide_handle;
	struct source_callback	src_callback;
	UINT8	src_path_index;
	UINT8	attach_source_index;
};

struct vp_win_config_para
{
	UINT8		source_number;
	struct vp_source_info source_info[VP_S3601_SOURCE_MAX_NUM];
	UINT8		control_source_index;
	UINT8		mainwin_source_index;
	UINT8		window_number;
	struct vp_win_config window_parameter[VP_S3601_WINDOW_MAX_NUM];
};
typedef enum InputFormat_e
{
	INPUT_2D = 0x0,            //!<For internal use
	INPUT_3D,                  //!<For internal use
}InputFormat_t;

enum OutputFormat_e

{
    OUTPUT_2D = 0x0,
    FRAME_PACKING,
    SIDE_BY_SIDE_HALF,
    SIDE_BY_SIDE_FULL,
    TOP_AND_BOTTOM,
    LINE_ALTERNATIVE,
    RED_AND_BLUE,
    FIELD_ALTERNATIVE,
    FRAME_SEQUENTIAL,
    PRGB,
    SRGB,
};

struct alifbio_3d_pars
{
    int display_3d_enable;
    int side_by_side;
    int top_and_bottom;
    int display_2d_to_3d;
    int depth_2d_to_3d;
    int mode_2d_to_3d;
    int red_blue;
//    int mvc_flag;
    InputFormat_t eInputFormat;
};

struct alifbio_3d_enhance_pars
{
    int enable_3d_enhance;
    int use_enhance_default;
    int set_3d_enhance;
    UINT8 eye_protect_shift;
	int user_true3d_enhance;
	int user_true3d_constant_shift;
	int ver_gradient_ratio;
	int hor_gradient_ratio;
	int pop_for_reduction_ratio;
	int parallax_sign_bias;
};

struct vpo_io_joint_mode
{
    int gp_update;
    int gp_en;
    int vgp_update;
    int vgp_en;
};

struct vpo_device
{
	struct vpo_device  *next;  //!<  Internal use
    UINT32 type;  //!<For internal use
	INT8 name[32];  //!<For internal use
	void *priv;  //!<For internal use
	void *priv_pip;
	UINT32 reserved;

	RET_CODE	(*open)(struct vpo_device *, struct vp_init_info *);
	RET_CODE   	(*close)(struct vpo_device *);
	RET_CODE   	(*ioctl)(struct vpo_device *,UINT32,UINT32);
	RET_CODE   	(*zoom)(struct vpo_device *,struct rect *, struct rect *);
	RET_CODE   	(*aspect_mode)(struct vpo_device *,enum tvmode, enum display_mode);
	RET_CODE 	(*tvsys)(struct vpo_device *, enum tvsystem, BOOL );
	RET_CODE 	(*win_onoff)(struct vpo_device *, BOOL);
	RET_CODE 	(*win_mode)(struct vpo_device *, BYTE, struct mpsource_call_back *, struct pipsource_call_back *);
	RET_CODE 	(*config_source_window)(struct vpo_device *, struct vp_win_config_para *);
	RET_CODE 	(*set_progres_interl)(struct vpo_device *, BOOL);
    RET_CODE   	(*zoom_ext)(struct vpo_device *,struct rect *, struct rect *,enum vp_display_layer);
    RET_CODE 	(*win_onoff_ext)(struct vpo_device *, BOOL, enum vp_display_layer);
};





struct tve_adjust
{
    UINT8 type;
    UINT8 sys;
    UINT32 value;
};

struct tve_adjust_data
{
	BOOL valid;
	UINT32 value;
};

struct tve_adjust_tbl
{
	UINT8 type;
	struct tve_adjust_data tve_data[8]; //!<Configuration parameters for 8 kinds of tv_system.
};


/*! @enum eTVE_ADJ_FILED

@brief Set TV Encoder.

*/
typedef enum e_tve_adj_filed
{

	TVE_COMPOSITE_Y_DELAY    ,
	TVE_COMPOSITE_C_DELAY    ,
	TVE_COMPOSITE_LUMA_LEVEL ,
	TVE_COMPOSITE_CHRMA_LEVEL,
	TVE_COMPOSITE_SYNC_DELAY       ,
	TVE_COMPOSITE_SYNC_LEVEL       ,
	TVE_COMPOSITE_FILTER_C_ENALBE  ,
	TVE_COMPOSITE_FILTER_Y_ENALBE  ,
	TVE_COMPOSITE_PEDESTAL_LEVEL   ,


    TVE_COMPONENT_IS_PAL           ,
    TVE_COMPONENT_PAL_MODE         ,
	TVE_COMPONENT_ALL_SMOOTH_ENABLE,
	TVE_COMPONENT_BTB_ENALBE       ,
	TVE_COMPONENT_INSERT0_ONOFF    ,
	TVE_COMPONENT_DAC_UPSAMPLEN    ,
    TVE_COMPONENT_Y_DELAY    ,
	TVE_COMPONENT_CB_DELAY   ,
	TVE_COMPONENT_CR_DELAY   ,
	TVE_COMPONENT_LUM_LEVEL        ,
	TVE_COMPONENT_CHRMA_LEVEL      ,
	TVE_COMPONENT_PEDESTAL_LEVEL   ,
	TVE_COMPONENT_UV_SYNC_ONOFF    ,
	TVE_COMPONENT_SYNC_DELAY       ,
	TVE_COMPONENT_SYNC_LEVEL       ,
	TVE_COMPONENT_R_SYNC_ONOFF     ,
	TVE_COMPONENT_G_SYNC_ONOFF     ,
	TVE_COMPONENT_B_SYNC_ONOFF     ,
	TVE_COMPONENT_RGB_R_LEVEL      ,
	TVE_COMPONENT_RGB_G_LEVEL      ,
	TVE_COMPONENT_RGB_B_LEVEL      ,
	TVE_COMPONENT_FILTER_Y_ENALBE  ,
	TVE_COMPONENT_FILTER_C_ENALBE  ,
	TVE_COMPONENT_PEDESTAL_ONOFF   ,
	TVE_COMPONENT_PED_RGB_YPBPR_ENABLE ,
	TVE_COMPONENT_PED_ADJUST       ,
	TVE_COMPONENT_G2Y              ,
	TVE_COMPONENT_G2U              ,
	TVE_COMPONENT_G2V              ,
	TVE_COMPONENT_B2U              ,
	TVE_COMPONENT_R2V              ,

    TVE_BURST_POS_ENABLE     ,
    TVE_BURST_LEVEL_ENABLE   ,
	TVE_BURST_CB_LEVEL       ,
	TVE_BURST_CR_LEVEL       ,
	TVE_BURST_START_POS            ,
	TVE_BURST_END_POS              ,
	TVE_BURST_SET_FREQ_MODE        ,
	TVE_BURST_FREQ_SIGN            ,
	TVE_BURST_PHASE_COMPENSATION   ,
    TVE_BURST_FREQ_RESPONSE  ,

    TVE_ASYNC_FIFO           ,
    TVE_CAV_SYNC_HIGH        ,
    TVE_SYNC_HIGH_WIDTH      ,
    TVE_SYNC_LOW_WIDTH       ,

    TVE_VIDEO_DAC_FS		 ,

	TVE_SECAM_PRE_COEFFA3A2  ,
	TVE_SECAM_PRE_COEFFB1A4  ,
	TVE_SECAM_PRE_COEFFB3B2  ,
	TVE_SECAM_F0CB_CENTER    ,
	TVE_SECAM_F0CR_CENTER    ,
	TVE_SECAM_FM_KCBCR_AJUST ,

	TVE_SECAM_CONTROL        ,

	TVE_SECAM_NOTCH_COEFB1   ,

	TVE_SECAM_NOTCH_COEFB2B3 ,

	TVE_SECAM_NOTCH_COEFA2A3 ,

    //added

    TVE_COMPONENT_PLUG_OUT_EN,

    TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,

    TVE_CB_CR_INSERT_SW      ,

    TVE_VBI_LINE21_EN        ,

    TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,

    TVE_SCART_PLUG_DETECT_LINE_CNT_HD,

    TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,

    TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,

    TVE_COMPONENT_PLUG_DETECT_VCOUNT,

    TVE_COMPONENT_VDAC_ENBUF,

    TVE_ADJ_FIELD_NUM,

}t_e_tve_adj_filed;



typedef struct _TVE_ADJUST_ELEMENT
{
    t_e_tve_adj_filed index;
	unsigned int  value;
}T_TVE_ADJUST_ELEMENT;

typedef struct _TVE_ADJ_ADJUST_TABLE
{
    t_e_tve_sys index;
	T_TVE_ADJUST_ELEMENT*  p_table;
}T_TVE_ADJUST_TABLE;

struct tve_feature_config
{
	UINT32 config;
	struct tve_adjust *tve_adjust;
	struct tve_adjust_tbl *tve_tbl;
    T_TVE_ADJUST_TABLE *tve_tbl_all;
	struct tve_adjust *tve_adjust_adv;
};






/*! @

@brief For internal use

*/

struct tve_device

{

	struct tve_device  *next;       //!< Next device

    UINT32 type;

	INT8 name[32];



	void *priv;		/* Used to be 'private' but that upsets C++ */



	RET_CODE (*open)(struct tve_device *);

	RET_CODE (*close)(struct tve_device *);

	RET_CODE (*set_tvsys)(struct tve_device *,enum tvsystem, BOOL );

	RET_CODE (*register_dac)(struct tve_device *,enum dac_type, struct vp_dac_info *);

	RET_CODE (*unregister_dac)(struct tve_device *,enum dac_type);

	RET_CODE (*write_wss)(struct tve_device *,UINT16);

	RET_CODE (*write_cc)(struct tve_device *,UINT8, UINT16);

	RET_CODE (*write_ttx)(struct tve_device *,UINT8, UINT8, UINT8);

	RET_CODE (*reset_async_fifo)(struct tve_device *);

	RET_CODE (*set_wss_enable)(struct tve_device *, BOOL );

	BOOL     (*get_wss_status)(struct tve_device *);

	RET_CODE (*set_cc_config)(struct tve_device *, UINT8 , UINT8 , UINT8 );

	RET_CODE (*set_vbi_startline)(struct tve_device *, UINT8 );

    RET_CODE (*set_plug_detect_enable)(struct tve_device *, BOOL );

    RET_CODE (*set_vdac_fs_value)(struct tve_device *, UINT32 );

    RET_CODE (*set_mg_config)(struct tve_device *,UINT8, UINT8 );

    RET_CODE (*set_cgms_config)(struct tve_device *,UINT8,UINT8 , UINT8 aps);

    RET_CODE (*set_cgms_cc_config)(struct tve_device *,UINT8,UINT8 , UINT8 aps);
    RET_CODE (*set_bootlogo_flag)(struct tve_device *dev,BOOL b_set_logo_flag);

	UINT32     (*get_cgms_status)(struct tve_device *);
	RET_CODE (*get_dac_info)(struct tve_device	*dev,struct vp_dac_state *dac_state);
	RET_CODE (*get_cgms_info)(struct tve_device	*dev,UINT8 *cgms_state);

};



// for S3602F dual-output, need at least 3 extra frame buffers for SD output
typedef struct vcap_attach_s
{
    UINT32 fb_addr;  // 256 bytes aligned frame buffer address
    UINT32 fb_size;  // (736x576 * 2) *3
} vcap_attach_t, *p_vcap_attach_t;



/**************************API SubFunction Begin********************************************/

void dvi_api_enable_interrupt(UINT8 u_dviscan_mode, UINT8 u_interrupt);
void HLD_vpo_attach(void);
void HLD_vpo_dettach(void);



/*!

@brief  Open VPO module.

@param[in] dev  Pointer to VPO module.

@param[in] pInitInfo   Initialization parameter
@return RET_CODE

@retval  RET_SUCCESS        Applied successfully
@retval  !RET_SUCCESS       Failed to apply due to parameter error or status error.
*/

RET_CODE vpo_open(struct vpo_device *dev,struct vp_init_info *init_info);



/*!

@brief  Close VPO module.

@param[in] dev  Pointer to VPO module.
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
*/

RET_CODE vpo_close(struct vpo_device *dev);



/*!

@brief   On/Off display layer.
 @param[in] dev  Pointer to VPO module.
@param[in] bOn On/off flag. TRUE(1): open; FALSE(0): close.
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
*/

RET_CODE vpo_win_onoff(struct vpo_device *dev,BOOL on);
RET_CODE vpo_win_onoff_ext(struct vpo_device *dev,BOOL on,enum vp_display_layer layer);



/*!

@brief   Set work mode of VPO module.
 @param[in] dev  Pointer to VPO module.
@param[in] bWinMode  Work mode, including VPO_MAINWIN and VPO_PREVIEW.
@param[in] pMPCallBack  Callback function related with main picture layer.
@param[in] pPIPCallBack  Callback function related with auxiliary picture layer. Not implemented.
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
*/

RET_CODE vpo_win_mode(struct vpo_device *dev, BYTE win_mode, \
    struct mpsource_call_back *mp_call_back,struct pipsource_call_back *pip_call_back);


/*!

@brief  Set scaling parameters of VPO module.

@param[in] dev  Pointer to VPO module.
@param[in] pSrcRect  Displaying area parameters of data source.
@param[in] pDstRect  Output area parameters of displaying screen.
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
*/

RET_CODE vpo_zoom(struct vpo_device *dev, struct rect *src_rect , struct rect *dst_rect);
RET_CODE vpo_zoom_ext(struct vpo_device *dev, struct rect *src_rect , \
                      struct rect *dst_rect,enum vp_display_layer layer);
/*!

@brief  Set aspect ratio parameters of VPO module.
@param[in] dev  Pointer to VPO module.
@param[in] e_tvaspect  Aspect ratio parameters
@param[in] e169DisplayMode   Display mode:16:9
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
*/

RET_CODE vpo_aspect_mode(struct vpo_device *dev, enum tvmode tv_aspect, enum display_mode e169_display_mode);



/*!

@brief  Set output TV system of VPO module.
@param[in] dev  Pointer to VPO module.
@param[in] e_tvsys  Output TV system

@param[in] bprogressive Flag of progressive output. TRUE(1): progressive; FALSE(0): interlace.
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
*/
RET_CODE vpo_tvsys(struct vpo_device *dev, enum tvsystem tv_sys);
RET_CODE vpo_tvsys_ex(struct vpo_device *dev, enum tvsystem tv_sys,BOOL progressive);



/*!

@brief   IO contorl operation of VPO module.
@param[in] dev  Pointer to VPO module.
@param[in] dwCmd Operation command type. Please refer to definition of VPO_IO_XX.
@param[in,out] dwParam  Operation parameters. Different commands have different parameters.
@return RET_CODE

@retval  RET_SUCCESS       Applied successfully
@retval  !RET_SUCCESS      Failed to apply due to parameter error or status error.
@note  IO Command dwCmd Introduction:
<table class="doxtable"  width="800" border="1" style="border-collapse:collapse;table-layout:fixed;word-break:break-all;" >

  <tr>

    <th width="200"> Command

    <th width="200"> Parameter

    <th width="80"> Transmission Characteristics

    <th width="320"> Comment

  </tr>



  <tr align="center">

    <td>VPO_IO_SET_BG_COLOR</td>

    <td>struct  ycb_cr_color *</td>

    <td>Input</td>

    <td>Set background color</td>

  </tr>



  <tr align="center">

    <td>VPO_IO_REG_DAC</td>

    <td>struct vp_io_reg_dac_para *</td>

    <td>Input</td>

    <td>Register a new video interface</td>

  </tr>



  <tr align="center">

    <td>VPO_IO_UNREG_DAC</td>

    <td>enum dac_type</td>

    <td>Input</td>

    <td>Revoke a video interface</td>

  </tr>



  <tr align="center">

    <td>VPO_IO_VIDEO_ENHANCE</td>

    <td>struct  vpo_io_video_enhance *</td>

    <td>Input</td>

    <td>Set image enhancement parameters</td>

  </tr>



  <tr align="center">

    <td>VPO_IO_GET_OUT_MODE</td>

    <td>enum tvsystem *</td>

    <td>Output</td>

    <td>Read outputted TV system</td>

  </tr>



  <tr align="center">

    <td>VPO_IO_GET_INFO</td>

    <td>struct vpo_io_get_info *</td>

    <td>Output</td>

    <td>Read module information</td>

  </tr>



  <tr align="center">

    <td>VPO_IO_SET_LAYER_ORDER</td>

    <td>bit6-7 bit4-5 bit2-3 bit0-1 *</td>

    <td>Input</td>

    <td>Set the sequence of overlay display layer</td>

  </tr>

  <tr align="center">

    <td>VPO_IO_CLOSE_HW_OUTPUT</td>

    <td>NULL *</td>

    <td>Input</td>

    <td>Turn off hardware video output before entering standby</td>

  </tr>

*/

RET_CODE vpo_ioctl(struct vpo_device *dev, UINT32 cmd, UINT32 param);










RET_CODE vpo_config_source_window(struct vpo_device *dev, struct vp_win_config_para *win_para);

// new added to set logo switch flag

RET_CODE vpo_set_logo_switch(BOOL b_logo_switch);
RET_CODE vpo_set_progres_interl(struct vpo_device *dev, BOOL progressive);




RET_CODE tvenc_open(struct tve_device *dev);

RET_CODE tvenc_close(struct tve_device *dev);

RET_CODE tvenc_set_tvsys(struct tve_device *dev,enum tvsystem tv_sys);
RET_CODE tvenc_set_tvsys_ex(struct tve_device *dev,enum tvsystem tv_sys, BOOL progressive);
RET_CODE tvenc_register_dac(struct tve_device *dev,enum dac_type dac_type, struct vp_dac_info *info);
RET_CODE tvenc_unregister_dac(struct tve_device *dev,enum dac_type dac_type);
RET_CODE tvenc_write_wss(struct tve_device *dev,UINT16 data);
RET_CODE tvenc_write_cc(struct tve_device *dev,UINT8 field_parity, UINT16 data);
RET_CODE tvenc_write_ttx(struct tve_device *dev,UINT8 line_addr, UINT8 addr, UINT8 data);


RET_CODE tvenc_reset_async_fifo(struct tve_device *dev);

RET_CODE tvenc_set_wss_enable(struct tve_device *dev, BOOL b_enable);

BOOL tvenc_get_wss_status(struct tve_device *dev);
UINT32 tvenc_get_cgms_status(struct tve_device *dev);


RET_CODE tvenc_set_vbi_startline(struct tve_device *dev, UINT8 line);
void tve_advance_config(struct tve_adjust *tve_adj_adv);

//void m36g_tve_hd_attach(struct tve_feature_config *pTVECfg);

RET_CODE tvenc_set_plug_detect_enable(struct tve_device *dev, BOOL b_enable);
RET_CODE tvenc_set_vdac_fs_value(struct tve_device *dev, UINT32 n_value);
RET_CODE tvenc_set_mg_config(struct tve_device *dev,UINT8 tv_sys_in , UINT8 aps_setting);
RET_CODE tvenc_set_cgms_config(struct tve_device *dev,UINT8 tv_sys_in ,UINT8 cgms_a, UINT8 aps);
RET_CODE tvenc_set_cgms_cc_config(struct tve_device *dev,UINT8 tv_sys_in ,UINT8 cgms_a, UINT8 aps);
RET_CODE tvenc_dac_info_get(struct tve_device *dev,struct vp_dac_state *dac_state);
RET_CODE tvenc_get_cgms_info(struct tve_device *dev,UINT8 *cgms_state);

void vcap_m36f_attach(const vcap_attach_t *vcap_param);

void vcap_m36f_dettach(void *dev);

void vpo_m36f_attach(struct vp_feature_config * vp_cfg, struct tve_feature_config*tvec_cfg);
void vpo_dev_attach(void);

void vpo_m36f_sd_attach(struct vp_feature_config *vp_cfg, struct tve_feature_config *tvec_cfg);

void m36g_vcap_attach(const vcap_attach_t *vcap_param);

void m36g_vcap_dettach(void *dev);

/*****************************************************************************/
void  vpo_m33_attach(struct vp_feature_config *p_vp_feature_config, struct tve_feature_config *p_tve_feature_config);


void vpo_register_cb_routine(void);



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



