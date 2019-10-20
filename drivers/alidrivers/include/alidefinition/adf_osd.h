#ifndef __ADF_OSD__
#define __ADF_OSD__

#ifdef __cplusplus
extern "C" {
#endif

#include "adf_basic.h"

/*!  English @addtogroup DeviceDriver
 *  @{
 */
 

/*!  English @addtogroup OSD
 *  @{
 */

/*! @enum OSDSys
@brief 
Output TV system, used with Scale.  
*/
enum OSDSys
{
    OSD_PAL = 0,
    OSD_NTSC
};

/*! @enum OSDColorMode
@brief 
Color format.  
*/
enum OSDColorMode
{
    OSD_4_COLOR =   0,
    OSD_16_COLOR,
    OSD_256_COLOR,
    OSD_16_COLOR_PIXEL_ALPHA,
    OSD_HD_ACLUT88,
    OSD_HD_RGB565,
    OSD_HD_RGB888,
    OSD_HD_RGB555,
    OSD_HD_RGB444,
    OSD_HD_ARGB565,
    OSD_HD_ARGB8888,
    OSD_HD_ARGB1555,
    OSD_HD_ARGB4444,
    OSD_HD_AYCbCr8888,
    OSD_HD_YCBCR888,
    OSD_HD_YCBCR422,
    OSD_HD_YCBCR422MB,
    OSD_HD_YCBCR420MB,
    OSD_COLOR_MODE_MAX
};

/*! @enum CLIPMode
@brief 
CLIP mode.  
*/
enum CLIPMode
{
    CLIP_INSIDE_RECT = 0,
    CLIP_OUTSIDE_RECT,
    CLIP_OFF
};

#define OSDDRV_RGB          0x00   //!< RGB color table. ARGB byte order is: {B, G , R , A}, A in [0, 255].  
#define OSDDRV_YCBCR        0x01   //!< YUV color table. AYCbCr byte order is: {Y, Cb, Cr, A}, A in [0, 15].  

#define OSDDRV_OFF          0x00   //!< OSD layer off.  
#define OSDDRV_ON           0x01   //!< OSD layer on.  


/**************** the position and size of OSD frame buffer********************/
#define OSD_MIN_TV_X        0x20
#define OSD_MIN_TV_Y        0x10
#define OSD_MAX_TV_WIDTH    0x2B0
#define OSD_MAX_TV_HEIGHT   0x1F0   //!< Old value 0x1E0 


#define P2N_SCALE_DOWN      0x00    //!< Scale down  
#define N2P_SCALE_UP        0x01    //!< Scale up  
#define OSD_SCALE_MODE      P2N_SCALE_DOWN


//#define P2N_SCALE_IN_NORMAL_PLAY
#define P2N_SCALE_IN_SUBTITLE_PLAY

// scaler
#define OSD_VSCALE_OFF                0x00
#define OSD_VSCALE_TTX_SUBT           0x01
#define OSD_VSCALE_GAME               0x02
#define OSD_VSCALE_DVIEW              0x03
#define OSD_HDUPLICATE_ON             0x04
#define OSD_HDUPLICATE_OFF            0x05
#define OSD_OUTPUT_1080               0x06   //!< 720x576(720x480)->1920x1080
#define OSD_OUTPUT_720                0x07   //!< 720x576(720x480)->1280x720 
#define OSD_HDVSCALE_OFF              0x08   //!< 1280x720->720x576(720x480) 
#define OSD_HDOUTPUT_1080             0x09   //!< 1280x720->1920x1080 
#define OSD_HDOUTPUT_720              0x0A   //!< 1280x720->1280x720
#define OSD_SET_SCALE_MODE            0x0B   //!<  Filter mode or duplicate mode  
#define OSD_SCALE_WITH_PARAM          0x0C   //!< Suitable for any case. See struct osd_scale_param.  
#define OSD_VSCALE_CC_SUBT            0x0D   //!< ATSC CC for HD output scale. 

#define OSD_SCALE_WITH_PARAM_DEO      0x1000  //!< Set scale parameter for SD output when dual output is enabled. See struct osd_scale_param. 

#define OSD_SOURCE_PAL                0
#define OSD_SOURCE_NTSC               1

#define OSD_SCALE_DUPLICATE           0
#define OSD_SCALE_FILTER              1

#define OSD_SET_ENHANCE_BRIGHTNESS     0x01    //!< value[0, 100], default 50. 
#define OSD_SET_ENHANCE_CONTRAST       0x02    //!< value[0, 100], default 50. 
#define OSD_SET_ENHANCE_SATURATION     0x04    //!< value[0, 100], default 50. 
#define OSD_SET_ENHANCE_SHARPNESS      0x08    //!< value[0, 10], default 5. 
#define OSD_SET_ENHANCE_HUE            0x10    //!< value[0, 100], default 50. 

// io command
#define OSD_IO_UPDATE_PALLETTE         0x00
#define OSD_IO_ADJUST_MEMORY           0x01
#define OSD_IO_SET_VFILTER             0x02
#define OSD_IO_RESPOND_API             0x03
#define OSD_IO_DIS_STATE               0x04
#define OSD_IO_SET_BUF_CACHEABLE       0x05
#define OSD_IO_16M_MODE                0x06
#define OSD_IO_SET_TRANS_COLOR         0x07           //!< Set color value of transparency  
#define OSD_IO_SET_ANTI_FLICK_THRE     0x0F
#define OSD_IO_ENABLE_ANTIFLICK        0x10           //!< Enable antiflicker function  
#define OSD_IO_DISABLE_ANTIFLICK       0x11           //!< Disable antiflicker function  

#define OSD_IO_SWITCH_DEO_LAYER        0x12
#define OSD_IO_SET_DEO_AUTO_SWITCH     0x13

#define OSD_IO_GET_RESIZE_PARAMATER    0x14
#define OSD_IO_SET_RESIZE_PARAMATER    0x15


#define OSD_IO_ELEPHANT_BASE             0x10000
#define OSD_IO_SWITH_DATA_TRANSFER_MODE (OSD_IO_ELEPHANT_BASE + 0x01)
#define OSD_IO_SET_ANTIFLK_PARA         (OSD_IO_ELEPHANT_BASE + 0x02)

#define OSD_IO_SET_GLOBAL_ALPHA         (OSD_IO_ELEPHANT_BASE + 0x03)   //!< Set overall transparency, with the range of [0x00, 0xff].  

#define OSD_IO_GET_ON_OFF               (OSD_IO_ELEPHANT_BASE + 0x04)
#define OSD_IO_SET_AUTO_CLEAR_REGION    (OSD_IO_ELEPHANT_BASE + 0x05)

#define OSD_IO_SET_YCBCR_OUTPUT         (OSD_IO_ELEPHANT_BASE + 0x06)

#define OSD_IO_SET_DISPLAY_ADDR         (OSD_IO_ELEPHANT_BASE + 0x07)
#define OSD_IO_SET_MAX_PIXEL_PITCH      (OSD_IO_ELEPHANT_BASE + 0x08)
#define OSD_IO_WRITE2_SUPPORT_HD_OSD    (OSD_IO_ELEPHANT_BASE + 0x09)
#define OSD_IO_SUBT_RESOLUTION          (OSD_IO_ELEPHANT_BASE + 0x0A)
#define OSD_IO_CREATE_REGION            (OSD_IO_ELEPHANT_BASE + 0x0b)
#define OSD_IO_MOVE_REGION              (OSD_IO_ELEPHANT_BASE + 0x0c)
#define OSD_IO_GET_REGION_INFO          (OSD_IO_ELEPHANT_BASE + 0x0d)   //!< Get region information.  
#define OSD_IO_GET_DISPLAY_RECT         (OSD_IO_ELEPHANT_BASE + 0x0e)
#define OSD_IO_SET_DISPLAY_RECT         (OSD_IO_ELEPHANT_BASE + 0x0f)
#define OSD_IO_SET_ENHANCE_PAR          (OSD_IO_ELEPHANT_BASE + 0x10)

#define OSD_IO_LINUX_API_BASE           (0xF00000)
#define OSD_IO_NO_CLEAR_BUF             (OSD_IO_LINUX_API_BASE + 0x01)
#define OSD_IO_VIDEO_ENHANCE            (OSD_IO_LINUX_API_BASE + 0x02)  //!< Set OSD enhancement value.  
#define OSD_IO_DISABLE_ENHANCE          (OSD_IO_LINUX_API_BASE + 0x03)  //!< Disable enhancement function.  
#define OSD_IO_GET_LAYER_INFO           (OSD_IO_LINUX_API_BASE + 0x04)  //!< Get OSD information of some layer.  

enum OSD_SUBT_RESOLUTION
{
    OSD_SUBT_RESO_720X576 = 1,
    OSD_SUBT_RESO_720X480,
    OSD_SUBT_RESO_1280X720,
    OSD_SUBT_RESO_1920X1080
};

/*! @struct OSDPara
@brief 
 Initialization parameter.  
*/
struct OSDPara
{
    enum OSDColorMode eMode;   //!< Color format.  
    UINT8 uGAlphaEnable;       //!< Whether use overall.  
    UINT8 uGAlpha;             //!< If use overall alpha, tha value is in the range [0x00, 0xff].  
    UINT8 uPalletteSel;        //!< Not implemented.  
};

/*! @struct OSDRect
@brief 
 Rectangle size and location.  
*/
struct OSDRect
{
    INT16 uLeft;          //!< Left  
    INT16 uTop;           //!< Top  
    INT16 uWidth;         //!< Width  
    INT16 uHeight;        //!< Height  
};

/*! @struct tagVScr
@brief 
Virtual region.  
*/
typedef struct tagVScr
{
    struct OSDRect vR;     //!< Rectangle  
    UINT8 *lpbScr;         //!< Buffer address  
    UINT8 bBlockID;        //!< Not implemented.  
    BOOL updatePending;    //!< Refresh flag, only used by APP.  
    UINT8 bColorMode;      //!< Not implemented.  
    UINT8 bDrawMode;       //!< Not implemented.  
}VSCR,*lpVSCR;

/*! @struct _osd_scale_param
@brief 
Scale parameter.  
*/
typedef struct _osd_scale_param
{
    UINT16 tv_sys;        //!< Not implemented.  
    UINT16 h_div;         //!< Horizontal value divides coefficient.  
    UINT16 v_div;         //!< Vertical value divides coefficient.  
    UINT16 h_mul;         //!< Horizontal value multiplies coefficient.  
    UINT16 v_mul;         //!< Vertical value multiplies coefficient.  
} osd_scale_param, *posd_scale_param;
typedef const osd_scale_param *pcosd_scale_param;

typedef struct _osd_resize_param
{
    INT32 h_mode;
    INT32 v_mode;
} osd_resize_param, *posd_resize_param;

typedef struct
{
    UINT8 enable;
    UINT8 layer;
    UINT8 no_temp_buf; //!< Not use temporary buffer.  
    UINT8 reserved;
}osd_clut_ycbcr_out; /*  Output ycbcr to DE��source is clut format.  */

typedef struct
{
    UINT8 region_id;   //!< Region ID  
    UINT8 reserved[3];
    UINT32 disp_addr;  //!< Buffer address to be displayed  
}osd_disp_addr_cfg;

#define OSD_Resize_Param    osd_resize_param
#define POSD_Resize_Param   posd_resize_param

/*! @struct _osd_region_param
@brief 
Region parameter  
*/
typedef struct _osd_region_param
{
    UINT8   region_id;              //!< Region index value  
    UINT8   color_format;           //!< Color format  
    UINT8   galpha_enable;          //!< whether use overall alpha  
    UINT8   global_alpha;           //!< If use overall alpha, the value is in the range [0x00, 0xff].  
    UINT8   pallette_sel;           //!< Not Implemented。  
    UINT16  region_x;               //!< Left of the region  
    UINT16  region_y;               //!< Top of the region  
    UINT16  region_w;               //!< Width of the region  
    UINT16  region_h;               //!< Height of the region  
    UINT32  phy_addr;               //!< Physical address of region buffer  
    UINT32  bitmap_addr;            //!< Virtual address of region buffer  
    UINT32  pixel_pitch;            //!< Row pitch of the region, with the unit of byte.  
    UINT32  bitmap_x;               //!< Left of virtual address in region buffer  
    UINT32  bitmap_y;               //!< Right of virtual address in region buffer  
    UINT32  bitmap_w;               //!< Width of virtual address in region buffer  
    UINT32  bitmap_h;               //!< Height of virtual address in region buffer  
} osd_region_param, *posd_region_param;
typedef const osd_region_param *pcosd_region_param;

/*! @struct _osd_layer_param
@brief 
Display layer parameter.  
*/
typedef struct _osd_layer_param
{
    enum OSDColorMode mode;            //!< Color format.  
    void *mem_start;                   //!< Physical address of buffer.  
    int mem_size;                      //!< Size of buffer.  
    void *virtual_mem_start;           //!< Virtual address of buffer.  
    int virtual_mem_size;              //!< Virtual address size of buffer.  
    int max_logical_width;             //!< Maximum width.  
    int max_logical_height;            //!< Maximum height.  
    int pitch;                         //!< Row pitch, with the unit of byte.  
}osd_layer_param, *posd_layer_param;

/*! @struct osd_io_video_enhance
@brief 
The definition of parameters needed by OSD_IO_VIDEO_ENHANCE. Set video enhancement information.  
*/
struct osd_io_video_enhance
{
    UINT8 changed_flag;               //!< Video enhancement type, OSD_IO_SET_ENHANCE_XX means enhancement type.  
    UINT16 grade;                     //!< Video enhancement quantitative value, with the range of 0~100. Default value 50, means no enhancement.  
};

/*!
@brief 
Release OSD module.  
*/
void HLD_OSDDrv_Dettach(void);

/*!
@brief Open OSD module.  
@param[in] hDev       Pointer pointed to OSD module.  
@param[in] ptPara     Initialization parameter.  
@return RET_CODE。
@retval RET_SUCCESS   Application succeeded.  
@retval !RET_SUCCESS  Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_Open(HANDLE hDev,struct OSDPara *ptPara);

/*!
@brief   Close OSd module.  
@param[in] hDev       Pointer pointed to OSD module.  
@return RET_CODE。
@retval RET_SUCCESS   Application succeeded.  
@retval !RET_SUCCESS  Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_Close(HANDLE hDev);


/*!
@brief   Read initialization parameters.  
@param[in] hDev       Pointer pointed to OSD module.  
@param[in] ptPara     Initialization parameter.  
@return RET_CODE。
@retval RET_SUCCESS   Application succeeded.  
@retval !RET_SUCCESS  Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_GetPara(HANDLE hDev,struct OSDPara* ptPara);

/*!
@brief  Turn on or off display layer.  
@param[in] hDev       Pointer pointed to OSD module.  
@param[in] uOnOff     Flag of on or off. OSDDRV_OFF means off.  
@return RET_CODE。
@retval  RET_SUCCESS  Application succeeded.  
@retval  !RET_SUCCESS Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_ShowOnOff(HANDLE hDev,UINT8 uOnOff);

/*!
@brief  Set color table.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] puPallette  Input pointer of buffer in color table.  
@param[in] uColorNum   Total number of colors. Only support 256 colors.  
@param[in] uType       Color table type. Only supports OSDDRV_YCBCR才OSDDRV_RGB.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_SetPallette(HANDLE hDev,UINT8 *puPallette,UINT16 uColorNum,UINT8 uType);

/*!
@brief   Read color table.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[out] puPallette Input pointer of buffer in color table.  
@param[in] uColorNum   Total number of colors. 
@param[in] uType       Color table type. Only supports OSDDRV_YCBCR and OSDDRV_RGB. 
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_GetPallette(HANDLE hDev,UINT8 *puPallette,UINT16 uColorNum,UINT8 uType);

/*!
@brief   Modify color table.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uIndex      Index value of color table.  
@param[in] uY          Brightness value.  
@param[in] uCb         Blue chromatism value.  
@param[in] uCr         Red chromatism value.  
@param[in] uK          Transparency value.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_ModifyPallette(HANDLE hDev,UINT8 uIndex,UINT8 uY,UINT8 uCb,UINT8 uCr,UINT8 uK);

/*!
@brief   Create OSD region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@param[in] rect        Region size and location information.  
@param[in] pOpenPara   Not implemented. Can set as NULL.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_CreateRegion(HANDLE hDev,UINT8 uRegionId,struct OSDRect* rect,struct OSDPara*pOpenPara);

/*!
@brief   Delete OSD region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_DeleteRegion(HANDLE hDev,UINT8 uRegionId);

/*!
@brief   Set new location of OSD region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@param[in] rect        New location information of region, for modifying location information only.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_SetRegionPos(HANDLE hDev,UINT8 uRegionId,struct OSDRect* rect);

/*!
@brief   Read new location of OSD region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@param[out] rect       Location information of region.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_GetRegionPos(HANDLE hDev,UINT8 uRegionId,struct OSDRect* rect);

/*!
@brief   Write data into region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@param[in] pVscr       Save virtual region of written data.  
@param[in] rect        Write operated rectangle size and location information.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_RegionWrite(HANDLE hDev,UINT8 uRegionId,VSCR *pVscr,struct OSDRect *rect);

/*!
@brief  Read data from region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@param[out] pVscr      Save virtual region of read data.  
@param[in] rect        Read operated rectangle size and location information.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_RegionRead(HANDLE hDev,UINT8 uRegionId,VSCR *pVscr,struct OSDRect *rect);

/*!
@brief   Fill in the specified color to region.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uRegionId   Index value of region.  
@param[in] rect        Write operated rectangle size and location information.  
@param[in] uColorData  Specify color.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
*/
RET_CODE OSDDrv_RegionFill(HANDLE hDev,UINT8 uRegionId,struct OSDRect *rect, UINT32 uColorData);

/*!
@brief   Set scale parameters.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] uScaleCmd   Scale command.  
@param[in] uScaleParam Scale parameter.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
@note   Introduction of scale command uScaleCmd:  
<table class="doxtable"  width="800" border="1" style="border-collapse:collapse;table-layout:fixed;word-break:break-all;" >
  <tr>
    <th width="200">  Command  </th>
    <th width="200">  Parameter  </th>
    <th width="80">  Transfer Characteristic  </th>
    <th width="320">  Notes  </th>
  </tr>

  <tr align="center">
    <td>OSD_VSCALE_OFF</td>
    <td>  Null  </td>
    <td>  Input  </td>
    <td>  Implement 1:1 output  </td>
  </tr>

   <tr align="center">
    <td>OSD_SET_SCALE_MODE</td>
    <td>  Null  </td>
    <td>  Input  </td>
    <td>  Set scale mode. Can be set duplication mode only.  </td>
  </tr>

   <tr align="center">
    <td>OSD_SCALE_WITH_PARAM</td>
    <td>osd_scale_param *</td>
    <td>  Input  </td>
    <td>  Prameter for setting scaling.  </td>
  </tr>
*/
RET_CODE OSDDrv_Scale(HANDLE hDev, UINT32 uScaleCmd,UINT32 uScaleParam);

/*!
@brief   IO control operation of OSD module.  
@param[in] hDev        Pointer pointed to OSD module.  
@param[in] dwCmd       Operated command type. See the definition of OSD_IO_XX.  
@param[in,out] dwParam Operation parameter. Different commands have different parameters.  
@return RET_CODE。
@retval RET_SUCCESS    Application succeeded.  
@retval !RET_SUCCESS   Application failed, with parameter error or status error.  
@note   IO command dwCmd introduction:
<table class="doxtable"  width="800" border="1" style="border-collapse:collapse;table-layout:fixed;word-break:break-all;" >
  <tr>
    <th width="200">  Command  </th>
    <th width="200">  Parameter  </th>
    <th width="80">  Transfer Characteristic  </th>
    <th width="320">  Notes  </th>
  </tr>

  <tr align="center">
    <td>OSD_IO_SET_TRANS_COLOR</td>
    <td>UINT8</td>
    <td>  Inout  </td>
    <td>  Set transparency index value, valid only when color format is CLUT8.  </td>
  </tr>

  <tr align="center">
    <td>OSD_IO_SET_GLOBAL_ALPHA</td>
    <td>UINT8</td>
    <td>  Input  </td>
    <td>  Set transparency of display layer. 0xFF means fully opaque; 0 means fully transparent.  </td>
  </tr>

  <tr align="center">
    <td>OSD_IO_GET_REGION_INFO</td>
    <td>posd_region_param </td>
    <td> Output  </td>
    <td> Read region information  </td>
  </tr>

   <tr align="center">
    <td>OSD_IO_GET_LAYER_INFO</td>
    <td>posd_layer_param</td>
    <td> Output  </td>
    <td> Read display layer parameters  </td>
  </tr>
*/
RET_CODE OSDDrv_IoCtl(HANDLE hDev,UINT32 dwCmd,UINT32 dwParam);

/*!
 * @}
 */


/*!
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

