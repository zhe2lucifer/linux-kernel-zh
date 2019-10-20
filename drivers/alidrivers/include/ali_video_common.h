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
 *  File: ali_video.h
 *  (I)
 *  Description: ali video header
 *  (S)
 *  History:(M)
 *          Date                    Author          Comment
 *          ====                    ======      =======
 * 0.       2009.12.22              Sam     Create
 ****************************************************************************/

#ifndef __ALI_VIDEO_COMMON_H
#define __ALI_VIDEO_COMMON_H

/*! @addtogroup DeviceDriver
 *  @{
 */

/*! @addtogroup ALiVideo
 *  @{
*/

#include "ali_basic_common.h"
#include <alidefinition/adf_basic.h>
#include <alidefinition/adf_media.h>
#include <alidefinition/adf_vpo.h>
#include <alidefinition/adf_decv.h>

/* video output dac information */
#define MAX_VIDEO_DAC_TYPENUM   23
#define VIDEO_DAC0              0x01
#define VIDEO_DAC1              0x02
#define VIDEO_DAC2              0x04
#define VIDEO_DAC3              0x08
#define VIDEO_DAC4              0x10
#define VIDEO_DAC5              0x20


/*! @enum Video_DacType
@brief DAC type (for internal use)
*/
enum Video_DacType
{
    Video_CVBS_1 = 0,
    Video_CVBS_2,
    Video_CVBS_3,
    Video_CVBS_4,
    Video_CVBS_5,
    Video_CVBS_6,
    Video_SVIDEO_1,
    Video_SVIDEO_2,
    Video_SVIDEO_3,
    Video_YUV_1,
    Video_YUV_2,
    Video_RGB_1,
    Video_RGB_2,
    Video_SVIDEO_Y_1,
    Video_SECAM_CVBS1,
    Video_SECAM_CVBS2,
    Video_SECAM_CVBS3,
    Video_SECAM_CVBS4,
    Video_SECAM_CVBS5,
    Video_SECAM_CVBS6,
    Video_SECAM_SVIDEO1,
    Video_SECAM_SVIDEO2,
    Video_SECAM_SVIDEO3,
};

/*! @enum Video_VgaMode
@brief VGA mode (for internal use)
*/
enum Video_VgaMode
{
    Video_VGA_NOT_USE = 0,
    Video_VGA_640_480,
    Video_VGA_800_600
};

/*! @struct Video_DacIndex
@brief DAC index (for internal use)
*/
struct Video_DacIndex
{
    uint8 u_dac_first;     // For all (CVBS, YC_Y,YUV_Y,RGB_R)
    uint8 u_dac_second;    // For SVideo & YUV & RGB (YC_C ,YUV_U,RGB_G)
    uint8 u_dac_third;     // For YUV & RGB (YUV_V,RGB_B)
};

/*! @struct Video_DacInfo
@brief DAC information (for internal use)
*/
struct Video_DacInfo
{
    int enable;
    enum Video_DacType type;
    enum Video_VgaMode mode;
    struct Video_DacIndex index;
    int progressive;
};

/*! @enum Video_TVSystem
@brief tv system (for internal use)
*/
enum Video_TVSystem
{
    Video_PAL = 0,       // PAL4.43(==PAL_BDGHI)        (Fh=15.625,fv=50)
    Video_NTSC,          // NTSC3.58                    (Fh=15.734,Fv=59.94)
    Video_PAL_M,         // PAL3.58                     (Fh=15.734,Fv=59.94)
    Video_PAL_N,         // PAL4.43(changed PAL mode)   (Fh=15.625,fv=50)
    Video_PAL_60,        //                             (Fh=15.734,Fv=59.94)
    Video_NTSC_443,      // NTSC4.43                    (Fh=15.734,Fv=59.94)
    Video_SECAM,
    Video_MAC,
    Video_LINE_720_25,   //Added for s3601
    Video_LINE_720_30,   //Added for s3601
    Video_LINE_1080_25,  //Added for s3601
    Video_LINE_1080_30,  //Added for s3601_
    Video_LINE_1080_50,  //Added for s3602f
    Video_LINE_1080_60,  //Added for s3602f
    Video_LINE_1080_24,  //Added for s3602f
    Video_LINE_1152_ASS, //Added for s3602f
    Video_LINE_1080_ASS, //Added for s3602f
    Video_PAL_NC,


    Video_LINE_800_600_VESA,
    Video_LINE_1024_768_VESA,
    Video_LINE_1360_768_VESA,
};


/*! @enum Video_TVAspect
@brief TV aspect ratio (for internal use)
*/
enum Video_TVAspect
{
    Video_TV_4_3 = 0,
    Video_TV_16_9,
    Video_TV_AUTO   //060517 yuchun for GMI Aspect Auto
};

/*! @enum Video_DisplayMode
@brief TV display mode (for internal use)
*/
enum Video_DisplayMode
{
    Video_PANSCAN = 0,
    Video_PANSCAN_NOLINEAR, //Non-linear pan&scan
    Video_LETTERBOX,
    Video_TWOSPEED, //Added by t2
    Video_PILLBOX,
    Video_VERTICALCUT,
    Video_NORMAL_SCALE,
    Video_LETTERBOX149,
    Video_DONT_CARE,
    Video_AFDZOOM,
    //BOTH
};

#define VIDEO_PIC_WIDTH  720
#define VIDEO_PIC_HEIGHT 2880   //!>2880 is the lease common multiple of screen height of Pal and ntsc
#define VIDEO_SCR_WIDTH  720
#define VIDEO_SCR_HEIGHT 2880

/*! @struct Video_Rect
@brief Display coordinate and resolution
*/
struct Video_Rect
{
    int32 x;    //!<Horizontal start point
    int32 y;    //!<Vertical start point
    int32 w;    //!<Horizontal size
    int32 h;    //!<Vertical size
};

/*! @struct Video_Pos
@brief Display coordinate and resolution
*/
struct Video_Pos
{
    int32 x;    //!<Horizontal start point
    int32 y;    //!<Vertical start point
    int32 w;    //!<Horizontal size
    int32 h;    //!<Vertical size
};

/*! @struct Video_YCbCrColor
@brief Parameter of video color space
*/
struct  Video_YCbCrColor
{
    uint8 Y;
    uint8 Cb;
    uint8 Cr;
};

enum Video_PicFmt
{
    //YCbCr Format
    Video_PicFmt_YCBCR_411,
    Video_PicFmt_YCBCR_420,
    Video_PicFmt_YCBCR_422,
    Video_PicFmt_YCBCR_444,

    //RGB format
    Video_PicFmt_RGB_MODE1,     //rgb (16-235)
    Video_PicFmt_RGB_MODE2,     //rgb (0-255)
};

#define DIS_FORMAT_CLUT2                        0x1    //!<Temporarily unused
#define DIS_FORMAT_CLUT4                        0x2    //!<Temporarily unused
#define DIS_FORMAT_ACLUT88                      0x4    //!<Temporarily unused
#define DIS_FORMAT_ARGB4444                     0x6    //!<Temporarily unused
#define DIS_FORMAT_RGB444                       0x8    //!<Temporarily unused
#define DIS_FORMAT_RGB555                       0xa    //!<Temporarily unused

#define DIS_FORMAT_CLUT8                        0      //!Input data format CLUT8

#define DIS_FORMAT_MC420                        10     //!<Temporarily unused
#define DIS_FORMAT_MC422                        (DIS_FORMAT_MC420 + 1)    //!<Temporarily unused
#define DIS_FORMAT_MC444                        (DIS_FORMAT_MC420 + 2)    //!<Temporarily unused
#define DIS_FORMAT_AYCBCR8888                   (DIS_FORMAT_MC444 + 3)    //!<Temporarily unused

#define DIS_FORMAT_ARGB1555                     20     //!Input data format ARGB1555
#define DIS_FORMAT_RGB565                       (DIS_FORMAT_ARGB1555 + 1) //!Input data format RGB565

#define DIS_FORMAT_ARGB8888                     40     //!Input data format ARGB8888
#define DIS_FORMAT_RGB888                       41     //!Input data format RGB888

#define VIDEO_MAX_GMA_REGION_NUM                6      //!<For internal use
#define VIDEO_MAX_GMA_WIDTH                     4095   //!<For internal use
#define VIDEO_MAX_GMA_HEIGHT                    4095   //!<For internal use

/*! @enum ALIFB_DE_LAYER
@brief Video/Graphic layers (for internal use)
*/
enum ALIFB_DE_LAYER
{
    DE_LAYER0,    //!<Video main picture
    DE_LAYER1,    //!<Temporarily unused
    DE_LAYER2,    //!<GMA1
    DE_LAYER3,    //!<GMA2
    DE_LAYER4,    //!<Temporarily unused
    DE_LAYER5,    //!<Temporarily unused
};

/*! @enum ALIFB_DE_LAYER
@brief Video/Graphic color space (for internal use)
*/
enum ALIFB_COLOR_TYPE
{
    ALIFB_COLOR_YUV,    //!<YUV color space
    ALIFB_COLOR_RGB,    //!<RGB color space
};

#define FBIO_GET_FBINFO                         0x460001    //!<Get fb information defined in struct fb_info
#define FBIO_SET_OUTPUT_FRAME_PATH              0x460002    //!<Not used any more
#define FBIO_OUTPUT_FRAME                       0x460003    //!<Not used any more
#define FBIO_CHECK_FRAME_FREE                   0x460004    //!<Not used any more
#define FBIO_GET_FBINFO_DATA                    0x460005    //!<Get fb information defined in struct alifbio_fbinfo_data_pars
#define FBIO_SET_COLOR_FMT                      0x460009    //!<Not used any more
#define FBIO_SET_TVSYS                          0x460010    //!<For internal use
#define FBIO_GET_TVSYS                          0x460011    //!<For internal use
#define FBIO_WIN_ZOOM                           0x460012    //!<For internal use
#define FBIO_SET_ASPECT_MODE                    0x460013    //!<For internal use
#define FBIO_GET_ASPECT_MODE                    0x460014    //!<For internal use
#define FBIO_UNREG_DAC                          0x460015    //!<For internal use
#define FBIO_REG_DAC                            0x460016    //!<For internal use
#define FBIO_WIN_ONOFF                          0x460017    //!<For internal use
#define FBIO_SET_PALLETTE                       0x460018    //!<Set palette for clut input format
#define FBIO_GET_PALLETTE                       0x460019    //!<Get palette for clut input format
#define FBIO_DELETE_GMA_REGION                  0x460020    //!<Delete gma region
#define FBIO_CREATE_GMA_REGION                  0x460021    //!<Create gma region
#define FBIO_SET_GMA_SCALE_INFO                 0x460022    //!<Scale gma region
#define FBIO_GET_GMA_REG_INFO                   0x460023    //!<Not used any more
#define FBIO_FLUSH_GMA_RECT                     0x460024    //!<Not used any more
#define FBIO_MOVE_REGION                        0x460025    //!<Move gma region
#define FBIO_GET_WINONOFF_STATE                 0x460028    //!<For internal use
#define FBIO_SET_BG_COLOR                       0x460030    //!<For internal use
#define FBIO_REG_HDMI_CALLBACK_ROUTINE          0x460031    //!<For internal use
#define FBIO_SET_HDMI_PIC_FRMT                  0x460032    //!<For internal use
#define FBIO_FILL_COLOR                         0x460033    //!<For internal use
#define FBIO_SET_DE_LAYER                       0x460040    //!<For internal use
#define FBIO_RPC_OPERATION                      0x460041    //!<For internal use
#define FBIO_SET_GLOBAL_ALPHA                   0x460042    //!<Set global alpha
#define FBIO_GMA_SCALE                          0x460043    //!<Not used any more
#define FBIO_DISABLE_GMA_ENHANCE                0x460044    //!<Not used any more
#define FBIO_SET_REGION_SIDE_BY                 0x460045    //!<Not used any more
#define FBIO_VIDEO_ENHANCE                      0x460046    //!<Set enhance function
#define FBIO_SET_MIX_ORDER                      0x46004f    //!<Temporarily unused
#define FBIO_SET_PREMULTIPLY                    0x460050    //!<Temporarily unused

#define FBIO_SET_GE_CMDQ_BUF                    0x460100    //!<Not used any more
#define FBIO_GET_UI_RSC_MAP                     0x460101    //!<Get infomation of memory allocated to fb
#define FBIO_GET_DISPLAY_RECT                   0X460102    //!<Not used any more
#define FBIO_SET_DISPLAY_RECT                   0X460103    //!<Not used any more
#define FBIO_SET_UI_CACHE_FLUSH                 0x460104    //!<Not used any more
#define FBIO_GET_GE_CMDQ_BUF_ADDR_SIZE          0x460105    //!<Not used any more
#define FBIO_GE_SYNC_TIMEOUT                    0x460106    //!<Not used any more
#define FBIO_SET_GMA_ANTIFLICK                  0x460107    //!<Temporarily unused
#define FBIO_SET_GMA_DBG_FLAG                   0x460108    //!<Enable/disable gma debug info

#define FBIO_REGISTER_ISR                       0x460200    //!<Not used any more
#define FBIO_UNREGISTER_ISR                     0x460201    //!<Not used any more
#define FBIO_FLAG_WAIT                          0x460202    //!<Not used any more

#define VPO_SET_WIN_ONOFF                       0x480001    //!<Open/close main picture layer
#define VPO_SET_WIN_ONOFF_EX                    0x480002    //!<Temporarily unused
#define VPO_WIN_ZOOM                            0x480003    //!<Zoom main picture display rect
#define VPO_WIN_ZOOM_EX                         0x480004    //!<Temporarily unused
#define VPO_SET_TVSYS                           0x480005    //!<Set TV system, only support interlaced TV system
#define VPO_SET_TVSYS_EX                        0x480006    //!<Set TV system
#define VPO_SET_ASPECT_MODE                     0x480007    //!<Set TV aspect ratio
#define VPO_CONFIG_SOURCE_WINDOW                0x480008    //!<Configure source window
#define VPO_SET_WIN_MODE                        0x480009    //!<Set vpo working mode
#define VPO_SET_BG_COLOR                        0x480010    //!<Set background color space
#define VPO_REG_DAC                             0x480011    //!<Registering DAC
#define VPO_SET_PARAMETER                       0x480012    //!<Not used any more
#define VPO_VIDEO_ENHANCE                       0x480013    //!<Set video enhance param
#define VPO_SET_CGMS_INFO                       0x480014    //!<Set CGMS information
#define VPO_AFD_CONFIG                          0x480015    //!<Config sw/hw AFD param
#define VPO_GET_CURRENT_DISPLAY_INFO            0x480016    //!<Get current display picture information
#define VPO_BACKUP_CURRENT_PICTURE              0x480017    //!<Backup current display picture
#define VPO_FREE_BACKUP_PICTURE                 0x480018    //!<Temporarily unused
#define VPO_SET_OSD_SHOW_TIME                   0x480019    //!<Set osd show time
#define VPO_GET_MP_SCREEN_RECT                  0x480020    //!<Get main picture layer screen rect
#define VPO_GET_MP_INFO                         0x480021    //!<Get main picture layer information
#define VPO_GET_REAL_DISPLAY_MODE               0x480022    //!<Not used any more
#define VPO_GET_TV_ASPECT                       0x480023    //!<Get TV aspect ratio
#define VPO_GET_SRC_ASPECT                      0x480024    //!<Get source aspect ratio
#define VPO_GET_DISPLAY_MODE                    0x480025    //!<Get TV display mode
#define VPO_GET_OSD0_SHOW_TIME                  0x480026    //!<Get OSD1 show time
#define VPO_GET_OSD1_SHOW_TIME                  0x480027    //!<Get OSD2 show time
#define VPO_SET_VBI_OUT                         0x480028    //!<Set vbi output callback
#define VPO_WRITE_WSS                           0x480029    //!<For internal use, write wss data,
#define VPO_UNREG_DAC                           0x480030    //!<Unregistering DAC
#define VPO_MHEG_SCENE_AR                       0x480031    //!<Not used any more
#define VPO_MHEG_IFRAME_NOTIFY                  0x480032    //!<Not used any more
#define VPO_DISAUTO_WIN_ONOFF                   0x480033    //!<Disable DE driver auto open main picture layer when showing the first picture
#define VPO_ENABLE_VBI                          0x480034    //!<Enable vbi output
#define VPO_PLAYMODE_CHANGE                     0x480035    //!<Notify play mode change
#define VPO_DIT_CHANGE                          0x480036    //!<Not used any more
#define VPO_SWAFD_ENABLE                        0x480037    //!<Enable software AFD
#define VPO_704_OUTPUT                          0x480038    //!<For internal use
#define VPO_SET_PREVIEW_MODE                    0x480039    //!<Notify display in preview mode
#define VPO_HDMI_OUT_PIC_FMT                    0x480040    //!<Get HDMI output picture format
#define VPO_ALWAYS_OPEN_CGMS_INFO               0x480041    //!<Always enable CGMS
#define VPO_SET_LAYER_ORDER                     0x480042    //!<Set display layer order
#define VPO_TVESDHD_SOURCE_SEL                  0x480043    //!<HD tvencoder connect DEN or DEO
#define VPO_SD_CC_ENABLE                        0x480044    //!<Enable SD tvencoder closecaption function
#define VPO_SET_PREVIEW_SAR_MODE                0x480045    //!<Not used any more
#define VPO_SET_FULL_SCREEN_CVBS                0x480046    //!<Not used any more
#define VPO_GET_OUT_MODE                        0x480047    //!<Get current TV system
#define VPO_ENABLE_VDAC_PLUG_DETECT             0x480048    //!<Enable video dac plug detect
#define VPO_DISPLAY_3D_ENABLE                   0x480049
#define VPO_GET_BOOT_LOGO_STATUS		        0x48004A
#define VPO_ENABLE_ANTIFLICK                    0x48004B
#define VPO_SET_ANTIFLICK_THR                   0x48004C
#define VPO_EXCHANGE_VIDEO_LAYER                0x48004D
#define VPO_CLOSE_DEO                           0x48004E
#define VPO_SET_CUTOFF                          0x48004F
#define VPO_GET_VBI_TTX_ISR                     0x480050
#define VPO_WRITE_TTX_PACKET                    0x480051
#define VPO_SET_VBI_STARTLINE                   0x480052
#define VPO_GET_MP_SOURCE_RECT                  0x480053    //!<Get main picture layer source rect
#define VPO_SELECT_SCALE_MODE                   0x480054
#define VPO_GET_DAC_INFO						0x480055
#define VPO_GET_CGMS_INFO						0x480056
#define VPO_CGMS_STATUS_GET						0x480057


#define FB_SET_SCALE_PARAM                      0x05        //!<Not used any more
#define FB_GET_SCALE_PARAM                      0x06        //!<Not used any more

/*! @struct alifbio_cache_flush
@brief Parameter of FBIO_SET_UI_CACHE_FLUSH (Not used any more)
*/
struct alifbio_cache_flush
{
    unsigned long mem_start;
    int mem_size;
};

/*! @struct alifbio_cmd_buf_addr_size
@brief Parameter of FBIO_GET_GE_CMDQ_BUF_ADDR_SIZE (Not used any more)
*/
struct alifbio_cmd_buf_addr_size
{
    unsigned addr;
    unsigned size;
};

/*! @enum ALIFB_OUTPUT_FRAME_PATH
@brief Not used any more
*/
enum ALIFB_OUTPUT_FRAME_PATH
{
    FRAME_FROM_KERNEL,
    FRAME_FROM_USER,
};

/*! @struct alifbio_fbinfo_data_pars
@brief Parameter of FBIO_GET_FBINFO_DATA
*/
struct alifbio_fbinfo_data_pars
{
    unsigned long mem_start;    //!<Memory start address
    int mem_size;               //!<Memory size

    int xres_virtual;           //!<Virtual horizontal resolution
    int yres_virtual;           //!<Virtual vertical resolution
    int line_length;            //!<Length of a line in bytes
};

/*! @struct alifbio_tvsys_pars
@brief Parameter of FBIO_SET_TVSYS (for internal use)
*/
struct alifbio_tvsys_pars
{
    int progressive;
    enum Video_TVSystem tvsys;
};

/*! @struct alifbio_zoom_pars
@brief Parameter of FBIO_WIN_ZOOM (for internal use)
*/
struct alifbio_zoom_pars
{
    int user_screen_xres;
    int user_screen_yres;
    struct Video_Rect src_rect;
    struct Video_Rect dst_rect;
};

/*! @struct alifbio_aspect_pars
@brief Parameter of FBIO_ASPECT_MODE (for internal use)
*/
struct alifbio_aspect_pars
{
    enum Video_TVAspect aspect;
    enum Video_DisplayMode display_mode;
};

/*! @enum ALIFB_PLT_TYPE
@brief Palette color space
*/
enum ALIFB_PLT_TYPE
{
    PLT_RGB, //!<RGB type: B[0-7] G[8-15] R[16-23] A[24-31]
    PLT_YUV, //!<YUV type: V[0-7] U[8-15] Y[16-23] A[24-31]
};

/*! @enum ALIFB_PLT_ALPHA_LEVEL
@brief Palette type
*/
enum ALIFB_PLT_ALPHA_LEVEL
{
    PLT_ALPHA_16,   //!<16 color palette
    PLT_ALPHA_128,  //!<128 color palette
    PLT_ALPHA_256,  //!<256 color palette
};

/*! @struct alifbio_plt_pars
@brief Parameter of FBIO_SET_PALLETTE
*/
struct alifbio_plt_pars
{
    enum ALIFB_PLT_TYPE type;   //!<Palette color space
    enum ALIFB_PLT_ALPHA_LEVEL level;    //!<Palette type

    void *pallette_buf;         //!<Palette buffer
    int color_num;              //!<Palette color number

    unsigned char rgb_order;    //!<Temporarily unused
    unsigned char alpha_range;  //!<Temporarily unused
    unsigned char alpha_pol;    //!<Temporarily unused
};

/*! @struct alifbio_reg_pars
@brief Parameter of FBIO_CREATE_GMA_REGION
*/
struct alifbio_reg_pars
{
    int index;                  //!<Region ID
    struct Video_Rect rect;     //!<Region displaying coordinate and resolution
    uint32 dis_format;          //!<Region input color format

    void *mem_start;            //!<Region memory start address
    int mem_size;               //!<Region memory size
    int pitch;                  //!<Length of a line in bytes
};

/*! @enum ALIFB_GMA_SCALE_MODE
@brief gma scale mode
*/
enum ALIFB_GMA_SCALE_MODE
{
    GMA_RSZ_DISABLE = 0,
    GMA_RSZ_DIRECT_RESIZE = 4,  //!<Copy pixels directly
    GMA_RSZ_ALPHA_ONLY = 5,     //!<Filter active only on alpha channel
    GMA_RSZ_COLOR_ONLY = 6,     //!<Filter active only on color channel
    GMA_RSZ_ALPHA_COLOR = 7     //!<Filter active on alpha and color channel
};

/*! @struct alifbio_gma_scale_info_pars
@brief param of FBIO_SET_GMA_SCALE_INFO
*/
struct alifbio_gma_scale_info_pars
{
    enum ALIFB_GMA_SCALE_MODE scale_mode;   //!<Scale mode
    int x_off;
    int y_off;
    int h_dst;                  //!<Horizontal size of destination
    int h_src;                  //!<Horizontal size of source
    int v_dst;                  //!<Vertical size of destination
    int v_src;                  //!<Vertical size of source
    int tv_sys;                 //!<Temporarily unused
    unsigned long uScaleCmd;    //!<Temporarily unused
    unsigned long uScaleParam;  //!<Temporarily unused
};

#if 1
typedef enum
{
    //SD
    TV_MODE_FB_AUTO = 0,    // Switch by source
    TV_MODE_FB_PAL,
    TV_MODE_FB_PAL_M,       // PAL3.58
    TV_MODE_FB_PAL_N,
    TV_MODE_FB_NTSC358,     // NTSC3.58
    TV_MODE_FB_NTSC443,
    TV_MODE_FB_SECAM,
    //HD
    TV_MODE_FB_576P,
    TV_MODE_FB_480P,
    TV_MODE_FB_720P_50,
    TV_MODE_FB_720P_60,
    TV_MODE_FB_1080I_25,
    TV_MODE_FB_1080I_30,
    TV_MODE_FB_1080P_50,
    TV_MODE_FB_1080P_60,
    TV_MODE_FB_1080P_25,
    TV_MODE_FB_1080P_30,
    TV_MODE_FB_1080P_24,
}FB_TV_OUT_MODE;

enum TVSystem_FB
{
    PAL_FB = 0,         //PAL4.43(==PAL_BDGHI)        (Fh=15.625,fv=50)
    NTSC_FB,            //NTSC3.58                    (Fh=15.734,Fv=59.94)
    PAL_M_FB,           //PAL3.58                     (Fh=15.734,Fv=59.94)
    PAL_N_FB,           //PAL4.43(changed PAL mode)   (Fh=15.625,fv=50)
    PAL_60_FB,          //                            (Fh=15.734,Fv=59.94)
    NTSC_443_FB,        //NTSC4.43                    (Fh=15.734,Fv=59.94)
    SECAM_FB,
    MAC_FB,
    LINE_720_25_FB,     //Added for s3601
    LINE_720_30_FB,     //Added for s3601
    LINE_1080_25_FB,    //Added for s3601
    LINE_1080_30_FB,    //Added for s3601

    LINE_1080_50_FB,    //Added for s3602f
    LINE_1080_60_FB,    //Added for s3602f
    LINE_1080_24_FB,    //Added for s3602f
    LINE_1152_ASS_FB,   //Added for s3602f
    LINE_1080_ASS_FB,   //Added for s3602f
    PAL_NC_FB,
};
#endif

/*! @struct alifbio_gma_scale_pars
@brief Parameter of FBIO_GMA_SCALE (Not used any more)
*/
struct alifbio_gma_scale_pars
{
    FB_TV_OUT_MODE tv_mode;
    int x_off;
    int y_off;
    int h_dst;
    int h_src;
    int v_dst;
    int v_src;
};

/*! @struct alifbio_flush_GMA_rect_pars
@brief Parameter of FBIO_FLUSH_GMA_RECT (Not used any more)
*/
struct alifbio_flush_GMA_rect_pars
{
    int region_id;
    void *in_start;
    int in_pitch;
    struct Video_Rect rect;
};

/*! @struct alifbio_move_region_pars
@brief Parameter of FBIO_MOVE_REGION
*/
struct alifbio_move_region_pars
{
    int region_id;           //!<Region id
    struct Video_Pos pos;    //!<Coordinate of the region
};

/*! @struct alifbio_fill_color_pars
@brief Parameter of FBIO_FILL_COLOR (Not used any more)
*/
struct alifbio_fill_color_pars
{
    enum ALIFB_COLOR_TYPE type;
    uint32 color;
};

/*! @struct alifbio_cmdq_buf
@brief Parameter of FBIO_SET_GE_CMDQ_BUF (Not used any more)
*/
struct alifbio_cmdq_buf
{
    int cmdq_index;     //!<0 - HQ, 1 - LQ
    uint32 *cmdq_buf;   //!<Should be a virtual address
    int cmdq_size;      //!<In bytes
};

/* FBIO_RPC_OPERATION :
It does Remote Process Call operation to the dis */
/*RPC API index definition */
#define RPC_FB_OPEN                 1   //!<For internal use
#define RPC_FB_CLOSE                2   //!<For internal use
#define RPC_FB_WIN_ON_OFF           3   //!<For internal use
#define RPC_FB_WIN_MODE             4   //!<For internal use
#define RPC_FB_ZOOM                 5   //!<For internal use
#define RPC_FB_ASPECT_MODE          6   //!<For internal use
#define RPC_FB_TVSYS                7   //!<For internal use
#define RPC_FB_TVSYS_EX             8   //!<For internal use
#define RPC_FB_IOCTL                9   //!<For internal use
#define RPC_FB_CONFIG_SOURCE_WINDOW 10  //!<For internal use
#define RPC_FB_SET_PROGRES_INTERL   11  //!<For internal use
#define RPC_FB_WIN_ON_OFF_EX        12  //!<For internal use
#define RPC_FB_ZOOM_EX              13  //!<For internal use

#define MAX_FB_RPC_ARG_NUM          4   //!<For internal use
#define MAX_FB_RPC_ARG_SIZE         512 //!<For internal use

/*! @struct ali_fb_rpc_arg
@brief For internal use
*/
struct ali_fb_rpc_arg
{
    void *arg;
    int arg_size;
    int out;
};

/*! @struct ali_fb_rpc_pars
@brief For internal use
*/
struct ali_fb_rpc_pars
{
    int hd_dev; /* 0 : SD output 1 : HD output*/
    int API_ID;
    struct ali_fb_rpc_arg arg[MAX_FB_RPC_ARG_NUM];
    int arg_num;
};

/*! @struct ali_fb_rsc_mem_map
@brief Define parameter of FBIO_GET_UI_RSC_MAP
*/
struct ali_fb_rsc_mem_map
{
    void *mem_start;           //!<Memory start address
    unsigned long mem_size;    //!<Memory size
};

#define FBIO_SET_ENHANCE_BRIGHTNESS 0x01    //!<Brightness enhancement, value[0, 100], default 50
#define FBIO_SET_ENHANCE_CONTRAST   0x02    //!<Contrast enhancement, value[0, 100], default 50
#define FBIO_SET_ENHANCE_SATURATION 0x04    //!<Saturation enhancement, value[0, 100], default 50
#define FBIO_SET_ENHANCE_SHARPNESS  0x08    //!<Sharpness enhancement, value[0, 10], default 5
#define FBIO_SET_ENHANCE_HUE        0x10    //!<Hue enhancement, value[0, 100], default 50

/*! @struct ali_fb_video_enhance_pars
@brief Parameter of FBIO_VIDEO_ENHANCE
*/
struct ali_fb_video_enhance_pars
{
    unsigned char changed_flag;    //!<Enhancement type
    unsigned short grade;          //!<Enhancement value
};


typedef struct ctrl_blk ali_vdeo_ctrl_blk;

//! @typedef ali_video_request_buf
//! @brief For internal use
typedef ali_dec_request_buf ali_video_request_buf;

//! @typedef ali_video_update_buf
//! @brief For internal use
typedef ali_dec_update_buf ali_video_update_buf;

/*! @enum vdec_output_mode
@brief Video synchronization mode
*/
enum vdec_avsync_mode
{
    VDEC_AVSYNC_PTS = 0,    //!<Do synchronization
    VDEC_AVSYNC_FREERUN,    //!<Don't do synchronization
};

/*! @enum vdec_type
@brief Video decoder type
*/
enum vdec_type
{
    VDEC_MPEG,    //!<MPEG1/2 decoder
    VDEC_AVC,     //!<H.264 decoder
    VDEC_AVS,     //!<AVS decoder
    VDEC_HEVC,    //!<H.265 decoder
};

/*! @struct vdec_stop_param
@brief Define parameter of VDECIO_STOP
*/
struct vdec_stop_param
{
    int32 close_display;    //!<Whether to close vpo
    int32 fill_black;       //!<Whether to fill frame buffer black
};

/*! @enum vdec_status
@brief Video status
*/
enum vdec_status
{
    VDEC_STARTED = 1,    //!<Started state
    VDEC_STOPPED,        //!<Stopped state
    VDEC_PAUSED,         //!<Paused state
};

/*! @struct vdec_yuv_color
@brief Video yuv color
*/
struct vdec_yuv_color
{
    uint8 y;    //!<Y
    uint8 u;    //!<U
    uint8 v;    //!<V
};

/*! @struct vdec_sync_param
@brief Define parameter of VDECIO_SET_SYNC_MODE
*/
struct vdec_sync_param
{
    enum vdec_avsync_mode sync_mode;    //!<Synchronization mode
};

/*! @struct ali_video_mem_info
@brief Define parameter of VDECIO_GET_MEM_INFO
*/
struct ali_video_mem_info
{
    void *mem_start;               //!<Start address of memory allocated to video in Main CPU
    unsigned long mem_size;        //!<Size of memory allocated to video in Main CPU
    void *priv_mem_start;          //!<Start address of memory allocated to video in SEE CPU
    unsigned long priv_mem_size;   //!<Size of memory allocated to video in SEE CPU
    void *mp_mem_start;            //!<Start address of memory allocated to media player
    unsigned long mp_mem_size;     //!<Size of memory allocated to media player
    void *vbv_mem_start;           //!<Start address of memory allocated to VBV buffer
    unsigned long vbv_mem_size;    //!<Size of memory allocated to VBV buffer
};

/*! @struct vdec_pvr_param
@brief Define parameter of VDECIO_SET_PVR_PARAM
*/
struct vdec_pvr_param
{
    int32 is_scrambled;    //!<Whether the stream is scrambled
};

/*! @struct vdec_codec_param
@brief Define parameter of VDECIO_SELECT_DECODER
*/
struct vdec_codec_param
{
    enum vdec_type type;    //!<Video decoder type
    int32 preview;          //!<Whether in preview
};

/*! @struct vdec_information
@brief Define parameter of VDECIO_GET_STATUS
*/
struct vdec_information
{
    enum vdec_status status;      //!<Video decoder's state
    int8   first_header_parsed;   //!<Whether the first header is parsed
    int8   first_pic_decoded;     //!<Whether the first picture is decoded
    int8   first_pic_showed;      //!<Whether the first picture is displayed
    uint16 pic_width;             //!<Picture width
    uint16 pic_height;            //!<Picture height
    enum asp_ratio aspect_ratio;  //!<Aspect ratio of the stream
    uint32 frames_displayed;      //!<Number of displayed frames
    uint32 frames_decoded;        //!<Number of decoded frames
    int64  frame_last_pts;        //!<PTS of last displayed frame
    int32  show_frame;            //!<Whether to display frame
    uint8  queue_count;           //!<Number of frames in display queue
    uint32 buffer_size;           //!<Total es buffer size
    uint32 buffer_used;           //!<Used size of es buffer
    uint32 frame_rate;            //!<Frame rate of the stream
    int32  interlaced_frame;      //!<Whether the stream is interlaced
    int32  top_field_first;       //!<Whether the stream is top field first
    int32  hw_dec_error;          //!<Decoder error
    int32  is_support;            //!<Whether the stream is supported
    enum vdec_output_mode output_mode;               //!<Video decoder's output mode
    struct vdec_playback_param playback_param;       //!<Video decoder's playback info
    struct vdec_playback_param api_playback_param;   //!<Playback info set by user
    uint32 sar_width;             //!<Sample aspect horizontal ratio
	uint32 sar_height;            //!<Sample apsect vertical ratio
	uint8 active_format;          //!<Active format
	uint8 layer;                  //!<Which display layer deocder associated with
	uint8 ff_mode;                //!<Whether only decode i frame in trick mode
	uint8 rect_switch_done;       //!<Whether set display rect done
	uint16 max_width;
    uint16 max_height;
    uint16 max_frame_rate;
};

/*! @struct ali_video_out_info_pars
@brief For internal use
*/
struct ali_video_out_info_pars
{
    int dev_idx;
    uint32 flag;
    int started;
    int width;
    int height;
    int frame_rate;
    int progressive;
    int display_idx;
    uint32 read_p_offset;
    uint32 write_p_offset;
    int valid_size;
    int is_support;
};
/*! @struct vdec_output_param
@brief Define parameter of VDECIO_SET_OUTPUT_MODE
*/
struct vdec_output_param
{
    enum vdec_output_mode output_mode;      //!<Video decoder's output mode
    int32 smooth_switch;                    //!<Mode of full screen and preview switching
    enum tvsystem tv_sys;                   //!<Current TV system
    int32 progressive;                      //!<Current TV system
    struct mpsource_call_back mp_callback;   //!<Main picture callback
    struct pipsource_call_back pip_callback; //!<PIP picture callback


};

/*! @struct ali_vpo_win_config_pars
@brief Define parameter of VPO_CONFIG_SOURCE_WINDOW
*/
struct ali_vpo_win_config_pars
{
    int hd_dev;
    struct vp_win_config_para win_config_para;
};

/*! @struct ali_vpo_winmode_pars
@brief Define parameter of VPO_SET_WIN_MODE
*/
struct ali_vpo_winmode_pars
{
    int hd_dev;                              //!<0: SD output; 1: HD output
    uint32 win_mode;                         //!<VPO working mode
    struct mpsource_call_back mp_callback;    //!<Main picture callback
    struct pipsource_call_back pip_callback;  //!<PIP picture callback
};

/*! @struct ali_vpo_tvsys_pars
@brief Define parameter of VPO_SET_TVSYS_EX
*/
struct ali_vpo_tvsys_pars
{
    int hd_dev;             //!<0: SD output; 1: HD output
    int progressive;        //!<Whether TV system is progressive
    enum tvsystem tvsys;    //!<Current TV system
};

/*! @struct ali_vpo_zoom_pars
@brief Define parameter of VPO_WIN_ZOOM
*/
struct ali_vpo_zoom_pars
{
    int hd_dev;                     //!<0: SD output; 1: HD output
    struct rect src_rect;           //!<Video source coordinate and resolution
    struct rect dst_rect;           //!<Display coordinate and resolution
    enum vp_display_layer layer;    //!<Video/Graphic layers
};

/*! @struct ali_vpo_aspect_pars
@brief Define parameter of VPO_SET_ASPECT_MODE
*/
struct ali_vpo_aspect_pars
{
    int hd_dev;                     //!<0: SD output; 1: HD output
    enum tvmode aspect;             //!<Aspect ratio of the TV
    enum display_mode display_mode;  //!<Display mode of the TV
};

/*! @struct ali_vpo_win_status_pars
@brief define parameter of VPO_SET_WIN_ONOFF
*/
struct ali_vpo_win_status_pars
{
    int hd_dev;                     //!<0: SD output; 1: HD output
    int on;                         //!<0: off; 1: on
    enum vp_display_layer layer;    //!<Video/Graphic layers
};

/*! @struct ali_vpo_bgcolor_pars
@brief Define parameter of VPO_SET_BG_COLOR
*/
struct ali_vpo_bgcolor_pars
{
    int hd_dev;                     //!<0: SD output; 1: HD output
    struct ycb_cr_color yuv_color;    //!<Background color space
};

/*! @struct ali_vpo_dac_pars
@brief Define parameter of VPO_REG_DAC
*/
struct ali_vpo_dac_pars
{
    int hd_dev;                             //!<0: SD output; 1: HD output
    struct vp_io_reg_dac_para dac_param;
};

/*! @struct ali_vpo_parameter_pars
@brief Define parameter of VPO_SET_PARAMETER
*/
struct ali_vpo_parameter_pars
{
    int hd_dev;
    struct vpo_io_set_parameter param;
};

/*! @struct ali_vpo_video_enhance_pars
@brief Define parameter of VPO_VIDEO_ENHANCE
*/
struct ali_vpo_video_enhance_pars
{
    int hd_dev;
    struct vpo_io_video_enhance video_enhance_param;
};

/*! @struct ali_vpo_cgms_info_pars
@brief Define parameter of VPO_SET_CGMS_INFO
*/
struct ali_vpo_cgms_info_pars
{
    int hd_dev;
    struct vpo_io_cgms_info cgms_info;
};

/*! @struct ali_vpo_afd_pars
@brief Define parameter of VPO_AFD_CONFIG
*/
struct ali_vpo_afd_pars
{
    int hd_dev;
    struct vp_io_afd_para afd_param;
};

/*! @struct ali_vpo_display_info_pars
@brief Define parameter of VPO_GET_CURRENT_DISPLAY_INFO
*/
struct ali_vpo_display_info_pars
{
    int hd_dev;
    struct vpo_io_get_picture_info display_info;
};

/*! @struct ali_vpo_dac_state_pars
@brief Define parameter of VPO_IO_GET_DAC_INFO
*/
struct ali_vpo_dac_state_pars
{
    int hd_dev;
    struct vp_dac_state dac_state;
};


/*! @struct ali_vpo_osd_show_time_pars
@brief Define parameter of VPO_SET_OSD_SHOW_TIME
*/
struct ali_vpo_osd_show_time_pars
{
    int hd_dev;
    vpo_osd_show_time_t osd_show_time;
};

/*! @struct ali_vpo_screem_rect_pars
@brief Define parameter of VPO_GET_MP_SCREEN_RECT
*/
struct ali_vpo_screem_rect_pars
{
    int hd_dev;
    struct rect mp_screem_rect;
};

/*! @struct ali_vpo_mp_info_pars
@brief Define parameter of VPO_GET_MP_INFO
*/
struct ali_vpo_mp_info_pars
{
    int hd_dev;
    struct vpo_io_get_info mp_info;
};

/*! @struct ali_vpo_ioctrl_pars
@brief Define parameter of io control
*/
struct ali_vpo_ioctrl_pars
{
    int hd_dev;
    uint32 param;
};

/*! @struct ali_vpo_cutoff_pars
@brief Define parameter of VPO_SET_CUTOFF
*/
struct ali_vpo_cutoff_pars
{
    int hd_dev;                      //!<0: SD output; 1: HD output
    struct vpo_io_set_cutoff param;
};

struct ali_vpo_vbittx_info_pars
{
    int hd_dev;                     //!<0: SD output; 1: HD output
    struct ali_vbi_ttx_pars param;    //!<vbittx info
};

struct ali_vpo_vbittx_packet_pars
{
    int hd_dev;                     //!<0: SD output; 1: HD output
    struct vpo_io_ttx_packet param;    //!<vbittx packet
};


#define ALIVIDEOIO_VIDEO_STOP                           0x550001    //!<For internal use
#define ALIVIDEOIO_VIDEO_PLAY                           0x550002    //!<For internal use

#define ALIVIDEOIO_GET_OUT_INFO                         0x550024    //!<For internal use
#define ALIVIDEOIO_RPC_OPERATION                        0x550040    //!<For internal use
#define ALIVIDEOIO_SET_SOCK_PORT_ID                     0x550041    //!<For internal use
#define ALIVIDEOIO_GET_MEM_INFO                         0x550042    //!<For internal use
#define ALIVIDEOIO_SET_CTRL_BLK_INFO                    0x550043    //!<For internal use
#define ALIVIDEOIO_GET_BOOTMEDIA_INFO                   0x550044    //!<For internal use
#define ALIVIDEO_ROTATION                               0x550045    //!<For internal use
#define ALIVIDEOIO_SET_MODULE_INFO                      0x550046    //!<For internal use
#define ALIVIDEOIO_GET_BOOTMEDIA_TIME                   0x550047    //!<For internal use

#define ALIVIDEOIO_DBG_BASE                             0x55f000    //!<For internal use
#define ALIVIDEOIO_ENABLE_DBG_LEVEL                     (ALIVIDEOIO_DBG_BASE + 0x0001)    //!<Enable kernel/see debug info (for internal use)
#define ALIVIDEOIO_DISABLE_DBG_LEVEL                    (ALIVIDEOIO_DBG_BASE + 0x0002)    //!<Disable kernel/see debug info (for internal use)
#define ALIVIDEOIO_SET_SEE_TEST_MODE                    (ALIVIDEOIO_DBG_BASE + 0x0003)    //!<For internal use
#define ALIVIDEOIO_SET_APE_DBG_MODE                     (ALIVIDEOIO_DBG_BASE + 0x0004)    //!<Enable/Disable ape debug info (for internal use)
#define ALIVIDEOIO_GET_APE_DBG_MODE                     (ALIVIDEOIO_DBG_BASE + 0x0005)    //!<Get ape debug info (for internal use)

#define VDEC_MAGIC                                      0x56

#define VDECIO_START                                    _IO(VDEC_MAGIC, 0)    //!<Start video decoder
#define VDECIO_PAUSE                                    _IO(VDEC_MAGIC, 1)    //!<Pause display
#define VDECIO_RESUME                                   _IO(VDEC_MAGIC, 2)    //!<Resume diplay
#define VDECIO_STEP                                     _IO(VDEC_MAGIC, 3)    //!<Step play
#define VDECIO_DRAW_COLOR_BAR                           _IO(VDEC_MAGIC, 4)    //!<Display color bar
#define VDECIO_RESTART                                  _IO(VDEC_MAGIC, 5)    //!<Restart decode

#define VDECIO_STOP                                     _IOR(VDEC_MAGIC, 0, struct vdec_stop_param)      //!<Stop video decoder
#define VDECIO_GET_CUR_DECODER                          _IOR(VDEC_MAGIC, 1, enum vdec_type)              //!<Get current video decoder's type
#define VDECIO_GET_STATUS                               _IOR(VDEC_MAGIC, 2, struct vdec_information)     //!<Get video's current status
#define VDECIO_GET_MEM_INFO                             _IOR(VDEC_MAGIC, 3, struct ali_video_mem_info)   //!<Get info of memory allocated to video

#define VDECIO_SET_SYNC_MODE                            _IOW(VDEC_MAGIC, 0, struct vdec_sync_param)      //!<Set synchronization mode
#define VDECIO_SET_PLAY_MODE                            _IOW(VDEC_MAGIC, 1, struct vdec_playback_param)  //!<Set playback mode
#define VDECIO_SET_PVR_PARAM                            _IOW(VDEC_MAGIC, 2, struct vdec_pvr_param)       //!<Set pvr parameter
#define VDECIO_SELECT_DECODER                           _IOW(VDEC_MAGIC, 3, struct vdec_codec_param)     //!<Select video decoder
#define VDECIO_FIRST_I_FREERUN                          _IOW(VDEC_MAGIC, 4, int)                         //!<Whether the first picture does synchronization or not
#define VDECIO_SET_SYNC_DELAY                           _IOW(VDEC_MAGIC, 5, unsigned long)               //!<Set synchronization delay [-500, 500]ms
#define VDECIO_CONTINUE_ON_ERROR                        _IOW(VDEC_MAGIC, 6, unsigned long)               //!<Whether to continue to display error picture
#define VDECIO_SET_OUTPUT_RECT                          _IOW(VDEC_MAGIC, 7, struct vdec_display_rect)    //!<Set display rectangle
#define VDECIO_FILL_FRAME                               _IOW(VDEC_MAGIC, 8, struct vdec_yuv_color)       //!<Fill frame buffer with specified color
#define VDECIO_SET_DMA_CHANNEL                          _IOW(VDEC_MAGIC, 9, unsigned char)               //!<Set dma channel number
#define VDECIO_DTVCC_PARSING_ENABLE                     _IOW(VDEC_MAGIC, 10, int)                        //!<Whether to parse dtv cc
#define VDECIO_SAR_ENABLE                               _IOW(VDEC_MAGIC, 11, int)                        //!<Whether to enable sample aspect ratio
#define VDECIO_SET_DEC_FRM_TYPE                         _IOW(VDEC_MAGIC, 12, unsigned long)              //!<Set frame type to decode
//#define VDECIO_SET_SOCK_PORT_ID                         _IOW(VDEC_MAGIC, 13, int)                        //!<Set sock port id
#define VDECIO_VBV_BUFFER_OVERFLOW_RESET                _IOW(VDEC_MAGIC, 14, int)                        //!<Whether to reset es buffer when buffer overflow
#define VDECIO_SET_SIMPLE_SYNC                          _IOW(VDEC_MAGIC, 15, int)                        //!<Set simple synchronization mode
#define VDECIO_SET_TRICK_MODE                           _IOW(VDEC_MAGIC, 16, struct vdec_playback_param) //!<Set trick play mode for playback
#define VDECIO_REG_CALLBACK                             _IOW(VDEC_MAGIC, 17, unsigned long)              //!<Register callback function
#define VDECIO_UNREG_CALLBACK                           _IOW(VDEC_MAGIC, 18, unsigned long)              //!<Unregister callback function
#define VDECIO_DYNAMIC_FRAME_ALLOC                      _IOW(VDEC_MAGIC, 19, int)                        //!<Set dynamic frame buffer allocation
#define VDECIO_SET_DISPLAY_MODE                         _IOW(VDEC_MAGIC, 20, struct vdec_display_param)  //!<Set display parameter
#define VDECIO_SET_PIP_PARAM                            _IOW(VDEC_MAGIC, 21, struct vdec_pip_param)      //!<Set pip parameter
#define VDECIO_PARSE_AFD                                _IOW(VDEC_MAGIC, 22, int)                        //!<Whether to enable parsing AFD
#define VDECIO_FLUSH                                    _IOW(VDEC_MAGIC, 23, int)                        //!<Flush the last decoded picure display queue
#define VDECIO_GET_ALL_USER_DATA                        _IOW(VDEC_MAGIC, 24, int)                        //!<Get all user data
#define VDECIO_GET_USER_DATA_INFO                       _IOW(VDEC_MAGIC, 25, int)                        //!<Get user associated frame information
#define VDECIO_SET_SYNC_REPEAT_INTERVAL					_IOW(VDEC_MAGIC, 26, int)                        //!<set interval to force repeat display video frame

#define VDECIO_SET_OUTPUT_MODE                          _IOWR(VDEC_MAGIC, 0, struct vdec_output_param)   //!<Set video's output mode
#define VDECIO_CAPTURE_DISPLAYING_FRAME                 _IOWR(VDEC_MAGIC, 1, struct vdec_picture)        //!<Capture displaying frame

#define VDECIO_MP_GET_STATUS                            _IOR(VDEC_MAGIC, 200, struct vdec_decore_status) //!<Get status in mp mode

#define VDECIO_MP_INITILIZE                             _IOW(VDEC_MAGIC, 200, struct vdec_mp_init_param) //!<Video initilization in mp mode
#define VDECIO_MP_RELEASE                               _IOW(VDEC_MAGIC, 201, struct vdec_mp_rls_param)  //!<Video deinitilization in mp mode
#define VDECIO_MP_FLUSH                                 _IOW(VDEC_MAGIC, 202, struct vdec_mp_flush_param)//!<Video flush in mp mode
#define VDECIO_MP_EXTRA_DATA                            _IOW(VDEC_MAGIC, 203, struct vdec_mp_extra_data) //!<Decode extra data in mp mode
#define VDECIO_MP_PAUSE                                 _IOW(VDEC_MAGIC, 204, struct vdec_mp_pause_param)//!<Pause decode/display in mp mode
#define VDECIO_MP_SET_SBM_IDX                           _IOW(VDEC_MAGIC, 205, struct vdec_mp_sbm_param)  //!<Set sbm to video
#define VDECIO_MP_SET_SYNC_MODE                         _IOW(VDEC_MAGIC, 206, struct vdec_mp_sync_param) //!<Set synchronizaion in mp mode
#define VDECIO_MP_SET_DISPLAY_RECT                      _IOW(VDEC_MAGIC, 207, struct vdec_display_rect)  //!<Set display rectangle in mp mode
#define VDECIO_MP_SET_QUICK_MODE                        _IOW(VDEC_MAGIC, 208, unsigned long)             //!<Set quick mode in mp mode
#define VDECIO_MP_SET_DEC_FRM_TYPE                      _IOW(VDEC_MAGIC, 209, unsigned long)             //!<Set frame type to decode in mp mode, 0: normal 1: first I
#define VDECIO_MP_DYNAMIC_FRAME_ALLOC                   _IOW(VDEC_MAGIC, 210, unsigned long)             //!<Set dynamic frame buffer allocation

#define VDECIO_MP_CAPTURE_FRAME                         _IOWR(VDEC_MAGIC, 200, struct vdec_picture)      //!<Capture displaying frame in mp mode
#define VDECIO_GET_KUMSGQ								_IOR(VDEC_MAGIC, 211, int)

#define ALI_VIDEO_REQ_RET_FAIL                          ALI_DECV_REQ_RET_FAIL    //!<For internal use
#define ALI_VIDEO_REQ_RET_OK                            ALI_DECV_REQ_RET_OK      //!<For internal use
#define ALI_VIDEO_REQ_RET_ERROR                         ALI_DECV_REQ_RET_ERROR   //!<For internal use

#define RPC_VIDEO_OPEN                1    //!<For internal use
#define RPC_VIDEO_CLOSE               2    //!<For internal use
#define RPC_VIDEO_START               3    //!<For internal use
#define RPC_VIDEO_STOP                4    //!<For internal use
#define RPC_VIDEO_VBV_REQUEST         5    //!<For internal use
#define RPC_VIDEO_VBV_UPDATE          6    //!<For internal use
#define RPC_VIDEO_SET_OUT             7    //!<For internal use
#define RPC_VIDEO_SYNC_MODE           8    //!<For internal use
#define RPC_VIDEO_PROFILE_LEVEL       9    //!<For internal use
#define RPC_VIDEO_IO_CONTROL          10   //!<For internal use
#define RPC_VIDEO_PLAY_MODE           11   //!<For internal use
#define RPC_VIDEO_DVR_SET_PAR         12   //!<For internal use
#define RPC_VIDEO_DVR_PAUSE           13   //!<For internal use
#define RPC_VIDEO_DVR_RESUME          14   //!<For internal use
#define RPC_VIDEO_DVR_STEP            15   //!<For internal use
#define RPC_VIDEO_SELECT_DEC          16   //!<For internal use
#define RPC_VIDEO_GET_DECODER         17   //!<For internal use
#define RPC_VIDEO_IS_AVC              18   //!<For internal use
#define RPC_VIDEO_SWITCH_DEC          19   //!<For internal use
#define RPC_VIDEO_GET_CURRENT_DEC     20   //!<For internal use
#define RPC_VIDEO_DECORE_IOCTL        21   //!<For internal use
#define RPC_VIDEO_DECODER_SELECT_NEW  22   //!<For internal use

#define RPC_VIDEO_INTENAL             0xF0000000    //!<For internal use
#define RPC_VIDEO_SET_DBG_FLAG        (RPC_VIDEO_INTENAL + 0x0001)    //!<For internal use

#define MAX_VIDEO_RPC_ARG_NUM         4    //!<For internal use
#define MAX_VIDEO_RPC_ARG_SIZE        2048 //!<For internal use

/*! @struct ali_video_rpc_arg
@brief For internal use
*/
struct ali_video_rpc_arg
{
    void *arg;
    int arg_size;
    int out;
};

/*! @struct ali_video_rpc_pars
@brief For internal use
*/
struct ali_video_rpc_pars
{
    int API_ID;
    struct ali_video_rpc_arg arg[MAX_VIDEO_RPC_ARG_NUM];
    int arg_num;
};

/*! @struct vdec_reserve_buf
@brief For internal use
*/
struct vdec_reserve_buf
{
    uint32 buf_addr;
    uint32 buf_size;
};

#define MSG_FIRST_SHOWED          1     //!<Message of first picture displayed
#define MSG_MODE_SWITCH_OK        2     //!<Temporarily unused
#define MSG_BACKWARD_RESTART_GOP  3     //!<Message of restarting GOP in backward play
#define MSG_FIRST_HEADRE_PARSED   4     //!<Message of first header parsed
#define MSG_UPDATE_OUT_INFO       5     //!<For internal use
#define MSG_FIRST_I_DECODED       6     //!<Message of first I picture decoded
#define MSG_FF_FB_SHOWED          7     //!<Message of first picture displayed in ff/fb play
#define MSG_USER_DATA_PARSED      8     //!<Message of user data parsed
#define MSG_INFO_CHANGED          9     //!<Message of information changed
#define MSG_ERROR                 10    //!<Message of error
#define MSG_STATE_CHANGED         11    //!<Message of state changed
#define MSG_NEW_FRAME             12    //!<Message of new frame
#define MSG_STARTED               13    //!<Message of start
#define MSG_STOPPED               14    //!<Message of stop
#define MSG_FRAME_DISPLAYED       15    //!<Message of frame displayed
#define MSG_MONITOR_GOP           16    //!<Message of got GOP info

/*! @enum vdec_out_pic_type
@brief YUV storage format that video outputs
*/
enum vdec_out_pic_type
{
    VDEC_PIC_IMC1,    //!<Not used any more
    VDEC_PIC_IMC2,    //!<Not used any more
    VDEC_PIC_IMC3,    //!<Not used any more
    VDEC_PIC_IMC4,    //!<Not used any more
    VDEC_PIC_YV12     //!<YU12 storage format
};

/*! @struct vdec_picture
@brief Define parameter of VDECIO_CAPTURE_DISPLAYING_FRAME
*/
struct vdec_picture
{
    enum vdec_out_pic_type type;    //!<Output YUV data format
    uint8 *out_data_buf;            //!<Output YUV data
    uint32 out_data_buf_size;       //!<Size of input buffer
    uint32 out_data_valid_size;     //!<Valid size of output YUV data
    uint32 pic_width;               //!<Picture width
    uint32 pic_height;              //!<Picture height
};

/*! @struct av_config
@brief Configuration of decoder's initialization
*/
struct av_config
{
   int32 av_sync_mode;           //!<A/V synchronization mode
   int32 hold_threshold;         //!<Hold threshold
   int32 drop_threshold;         //!<Drop threshold
   int32 av_sync_delay;          //!<A/V synchronization delay [-500, 500]ms
   int32 av_sync_unit;           //!<A/V synchronization unit
   int32 preview;                //!<Whether in preview
   struct av_rect src_rect;   //!<Display rectangle of source
   struct av_rect dst_rect;   //!<Display rectangle of destination
   int32 pause_decode;           //!<Whether to pause decode
   int32 pause_output;           //!<Whether to pause display
   int32 enable_output;          //!<Whether to enable decoder's output
   int32 high_threshold;         //!<High threshold of es buffer
   int32 low_threshold;          //!<Low threshold of es buffer
   int32 hold2free_trd;          //!<Hold to free run threshold
   int32 drop2free_trd;          //!<Drop to free run threshold
   uint32 quick_mode;            //!<Whether to enable quick mode
   uint32 decode_mode;           //!<Decode mode
};

/*! @enum av_sync_mode
@brief A/V synchronization mode
*/
enum av_sync_mode
{
    AV_SYNC_NONE,         //!<Don't do sync
    AV_SYNC_AUDIO,        //!<Sync to audio
    AV_SYNC_VIDEO,        //!<Sync to video (temporarily unused)
    AV_SYNC_EXTERNAL,     //!<Sync to an external clock (temporarily unused)
    AV_SYNC_AUDIO_N,      //!<Sync to audio, using new av sync module
    AV_SYNC_VIDEO_N,      //!<Sync to video, using new av sync module (temporarily unused)
    AV_SYNC_EXTERNAL_N,   //!<Sync to external clock, using new av sync module (temporarily unused)
};

/*! @struct rotation_rect
@brief (for internal use)
*/
struct rotation_rect
{
    int width;
    int height;
    unsigned char *src_y_addr;
    unsigned char *src_c_addr;
    int angle;
    unsigned char *dst_y_addr;
    unsigned char *dst_c_addr;
};
/*! @struct vdec_mp_init_param
@brief Define parameter of VDECIO_MP_INITILIZE
*/
struct vdec_mp_init_param
{
    uint32 codec_tag;      //!<Specified decoder's type
    uint32 decode_mode;    //!<Decode mode
    uint32 decoder_flag;   //!<Decode flag
    uint32 preview;        //!<Whether in preview
    uint32 frame_rate;     //!<Frame rate
    int32  pic_width;      //!<Picture width
    int32  pic_height;     //!<Picture height
    int32  pixel_aspect_x; //!<Pixel aspect ratio width
    int32  pixel_aspect_y; //!<Pixel aspect ratio height
    uint32 dec_buf_addr;   //!<Frame buffer start address
    uint32 dec_buf_size;   //!<Frame buffer total size
    uint8  encrypt_mode;   //!<0: clear data 1: full sample 2: subsample
};

/*! @struct vdec_mp_rls_param
@brief Define parameter of VDECIO_MP_RELEASE
*/
struct vdec_mp_rls_param
{
    int32 reserved;    //!<Temporarily unused
};

/*! @struct vdec_mp_flush_param
@brief Define parameter of VDECIO_MP_FLUSH
*/
struct vdec_mp_flush_param
{
    uint32 flush_flag;    //!<Setting to 3 if flushed to FF/FB and setting to 1 if flushed to normal play
};

/*! @struct vdec_mp_extra_data
@brief Define parameter of VDECIO_MP_EXTRA_DATA
*/
struct vdec_mp_extra_data
{
    uint8 *extra_data;      //!<Extra data buffer
    uint32 extra_data_size; //!<Extra data size
};

/*! @struct vdec_mp_pause_param
@brief Define parameter of VDECIO_MP_PAUSE
*/
struct vdec_mp_pause_param
{
    uint8 pause_decode;    //!<Whether to pause decode
    uint8 pause_display;   //!<Whether to pause display
};

/*! @struct vdec_mp_sbm_param
@brief Define parameter of VDECIO_MP_SET_SBM_IDX
*/
struct vdec_mp_sbm_param
{
    uint8 packet_header;   //!<SBM to hold packet header
    uint8 packet_data;     //!<SBM to hold packet data
    uint8 decode_output;   //!<SBM to hold decoder's output frames
    uint8 display_input;   //!<SBM to hold frames to display
};

/*! @struct vdec_mp_sync_param
@brief Define parameter of VDECIO_MP_SET_SYNC_MODE
*/
struct vdec_mp_sync_param
{
    enum av_sync_mode sync_mode;   //!<Synchronization mode
    uint8 sync_unit;               //!<Synchronization unit
};

/*! @enum GE_GMA_PIXEL_FORMAT
@brief For internal use
*/
enum GE_GMA_PIXEL_FORMAT
{
    // used by 3602
    GE_GMA_PF_RGB565        = 0x00,
    GE_GMA_PF_RGB888        = 0x01,
    GE_GMA_PF_RGB555        = 0x02,
    GE_GMA_PF_RGB444        = 0x03,
    GE_GMA_PF_ARGB565       = 0x04,
    GE_GMA_PF_ARGB8888      = 0x05,
    GE_GMA_PF_ARGB1555      = 0x06,
    GE_GMA_PF_ARGB4444      = 0x07,
    GE_GMA_PF_CLUT1         = 0x08,
    GE_GMA_PF_CLUT2         = 0x09,
    GE_GMA_PF_CLUT4         = 0x0A,
    GE_GMA_PF_CLUT8         = 0x0B,
    GE_GMA_PF_ACLUT88       = 0x0C,
    GE_GMA_PF_YCBCR888      = 0x10,
    GE_GMA_PF_YCBCR422      = 0x12,
    GE_GMA_PF_YCBCR422MB    = 0x13,
    GE_GMA_PF_YCBCR420MB    = 0x14,
    GE_GMA_PF_AYCBCR8888    = 0x15,
    GE_GMA_PF_A1            = 0x18,
    GE_GMA_PF_A8            = 0x19,
    GE_GMA_PF_CK_CLUT2      = 0x89,
    GE_GMA_PF_CK_CLUT4      = 0x8A,
    GE_GMA_PF_CK_CLUT8      = 0x8B,
    GE_GMA_PF_ABGR1555      = 0x90,
    GE_GMA_PF_ABGR4444      = 0x91,
    GE_GMA_PF_BGR565        = 0x92,
    GE_GMA_PF_ACLUT44       = 0x93,
    GE_GMA_PF_YCBCR444      = 0x94,
    GE_GMA_PF_YCBCR420      = 0x95,
    GE_GMA_PF_MASK_A1       = GE_GMA_PF_A1,
    GE_GMA_PF_MASK_A8       = GE_GMA_PF_A8,
};

/*! @struct gma_pal_attr_t
@brief For internal use
*/
typedef struct _gma_pal_attr_t
{
    unsigned char pal_type;         //!<GE_PAL_RGB or GE_PAL_YCBCR
    unsigned char rgb_order;        //!<enum GE_RGB_ORDER
    unsigned char alpha_range;      //!<enum GE_ALPHA_RANGE
    unsigned char alpha_pol;        //!<enum GE_ALPHA_POLARITY
} gma_pal_attr_t, *pgma_pal_attr_t;
typedef const gma_pal_attr_t *pcgma_pal_attr_t;

/*! @struct gma_scale_param_t
@brief For internal use
*/
typedef struct _gma_scale_param_t
{
    unsigned short tv_sys;
    unsigned short h_div;
    unsigned short v_div;
    unsigned short h_mul;
    unsigned short v_mul;
} gma_scale_param_t, *pgma_scale_param_t;
typedef const gma_scale_param_t *pcgma_scale_param_t;

/*! @struct gma_layer_config_t
@brief For internal use
*/
typedef struct _gma_layer_config_t
{
    unsigned long mem_base;
    unsigned long mem_size;
    unsigned char  hw_layer_id;         //!<Application can switch hw layer id
    unsigned char  color_format;        //!<Default region color format, enum GMA_PIXEL_FORMAT
    unsigned short width, height;       //!<Default region width and height
    unsigned short pixel_pitch;         //!<Default region pixel pitch

    unsigned long bScaleFilterEnable       :1;   //!<Enable/disable GMA scale filter
    unsigned long bP2NScaleInNormalPlay    :1;   //!<Enable/disable PAL/NTSC scale in normal play mode
    unsigned long bP2NScaleInSubtitlePlay  :1;   //!<Enable/disable PAL/NTSC scale in subtitle play mode
    unsigned long bDirectDraw              :1;   //!<For CPU direct draw, no GE draw
    unsigned long bCacheable               :1;   //!<For CPU direct draw, no GE draw
    unsigned long reserved                 :29;  //!<Reserved for future use
} gma_layer_config_t, *pgma_layer_config_t;

/*! @struct alifb_gma_region_t
@brief For internal use
*/
typedef struct _alifb_gma_region_t
{
    unsigned char  color_format;   // enum GMA_PIXEL_FORMAT
    unsigned char  galpha_enable;  // 0 - use color by color alpha; 1 - enable global alpha for this region
    unsigned char  global_alpha;   // If global alpha enable, please set global_alpha [0x00, 0xff]
    unsigned char  pallette_sel;   // pallette index for CLUT4

    unsigned short region_x;       // x offset of the region, from screen top_left pixel
    unsigned short region_y;       // y offset of the region, from screen top_left pixel
    unsigned short region_w;
    unsigned short region_h;

    unsigned long  bitmap_addr;    // 0 - use uMemBase(internal memory) which is set by ge_attach(gma_layer_config_t *);
                                   // bitmap_addr not 0 - use external memory address as region bitmap addr
    unsigned long  pixel_pitch;    // pixel pitch(not byte pitch) for internal memory or bitmap_addr

                                   // ge_create_region(): bitmap_addr and pixel_pitch determines the region bitmap address, total 4 cases:
                                   // Case 1: if bitmap_addr is 0, and pixel_pitch is 0, it will use region_w as pixel_pitch,
                                   //     and region bitmap addr will be allocated from uMemBase dynamically.
                                   // Case 2: if bitmap_addr is 0, and pixel_pitch is not 0, the region bitmap addr will be fixed:
                                   //     uMemBase + (pixel_pitch * bitmap_y + bitmap_x) * byte_per_pixel

                                   // Case 3: if bitmap_addr is not 0, and pixel_pitch is 0, the region bitmap addr will be:
                                   //     bitmap_addr + (bitmap_w * bitmap_y + bitmap_x) * byte_per_pixel
                                   // Case 4: if bitmap_addr is not 0, and pixel_pitch is not 0, the region bitmap addr will be:
                                   //     bitmap_addr + (pixel_pitch * bitmap_y + bitmap_x) * byte_per_pixel

                                   // ge_move_region(): region using internal memory can only change region_x, region_y, pal_sel;
                                   // ge_move_region(): region using external memory can change everyting in ge_region_t;

    unsigned long  bitmap_x;       // x offset from the top_left pixel in bitmap_addr or internal memory
    unsigned long  bitmap_y;       // y offset from the top_left pixel in bitmap_addr or internal memory
    unsigned long  bitmap_w;       // bitmap_w must >= bitmap_x + region_w, both for internal memory or external memory
    unsigned long  bitmap_h;       // bitmap_h must >= bitmap_y + region_h, both for internal memory or external memory
    unsigned long  region_index;
} alifb_gma_region_t, *pgma_gma_region_t;
typedef const alifb_gma_region_t *pcgma_region_t;

/*!
@}
*/

/*!
@}
*/

#endif

