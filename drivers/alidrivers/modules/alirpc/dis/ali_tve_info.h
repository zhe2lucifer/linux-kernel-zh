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
 
/***************************************************************************
/**\file
 * \brief bootargs and baseparam partition generate from xml;
 * ali boot info module will replace it
 *
 * \author stu1200+1100 <anystu1200+1100@alitech.com>
 * \date 2014-10-09: create the document
 ***************************************************************************/

#ifndef _ALI_TVE_INFO_H
#define _ALI_TVE_INFO_H

#include <alidefinition/adf_vpo.h>

/*! @addtogroup boot info
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
* Macro definitions
***************************************************************************/

#define VDAC_TYPE_CVBS		0
#define VDAC_TYPE_SVIDEO	1
#define VDAC_TYPE_YUV		2
#define VDAC_TYPE_RGB		3
#define VDAC_TYPE_SCVBS		4
#define VDAC_TYPE_SSV		5
#define VDAC_TYPE_MAX		6
//Detail
#define VDAC_CVBS			(VDAC_TYPE_CVBS<<2|0)
#define VDAC_CVBS_1			(VDAC_TYPE_CVBS<<2|1)
#define VDAC_CVBS_2			(VDAC_TYPE_CVBS<<2|2)
#define VDAC_CVBS_3			(VDAC_TYPE_CVBS<<2|3)

#define VDAC_SVIDEO_Y	(VDAC_TYPE_SVIDEO<<2|0)	// 0x4
#define VDAC_SVIDEO_C	(VDAC_TYPE_SVIDEO<<2|1)	// 0x5
#define VDAC_YUV_Y		(VDAC_TYPE_YUV<<2|0)	// 0x8
#define VDAC_YUV_U		(VDAC_TYPE_YUV<<2|1)	// 0x9
#define VDAC_YUV_V		(VDAC_TYPE_YUV<<2|2)	// 0xA
#define VDAC_RGB_R		(VDAC_TYPE_RGB<<2|0)	// 0xC
#define VDAC_RGB_G		(VDAC_TYPE_RGB<<2|1)	// 0xD
#define VDAC_RGB_B		(VDAC_TYPE_RGB<<2|2)	// 0xE
#define VDAC_SCVBS		(VDAC_TYPE_SCVBS<<2|0)	// 0x10
#define VDAC_SSV_Y		(VDAC_TYPE_SSV<<2|0)	// 0x14
#define VDAC_SSV_C		(VDAC_TYPE_SSV<<2|1)	// 0x15
#define VDAC_NULL		0xFF

//#define CVBS_USE_DAC0

#ifdef CVBS_USE_DAC0
#define DAC0_TYPE   VDAC_CVBS_1
#define DAC1_TYPE   VDAC_NULL
#define DAC2_TYPE   VDAC_NULL
#define DAC3_TYPE   VDAC_CVBS
#else
#define DAC0_TYPE   VDAC_YUV_V
#define DAC1_TYPE   VDAC_YUV_U
#define DAC2_TYPE   VDAC_YUV_Y
#define DAC3_TYPE   VDAC_CVBS
#endif

#define CONFIG_TV_SYSTEM LINE_1080_60
#define CONFIG_HDCP_DISABLE 1
#define CONFIG_MUTE_GPIO 70

#define CB_LEVEL_PAL_SD				0x85
#define CB_LEVEL_NTSC_SD            0x85
#define CR_LEVEL_PAL_SD				0x55
#define CR_LEVEL_NTSC_SD			0x55
#define LUMA_LEVEL_PAL_SD			0x52 //0x53
#define LUMA_LEVEL_NTSC_SD			0x4d //0x4f
#define CHRMA_LEVEL_PAL_SD			0x6
#define CHRMA_LEVEL_NTSC_SD		    0x6
#define FREQ_RESPONSE_PAL_SD		0x102
#define FREQ_RESPONSE_NTSC_SD		0x102

typedef enum
{
	TV_ASPECT_RATIO_43 = 0,
	TV_ASPECT_RATIO_169,
	TV_ASPECT_RATIO_AUTO
} TV_ASPECT_RATIO_TYPE;

typedef enum
{
    DISPLAY_MODE_NORMAL = 0,
    DISPLAY_MODE_LETTERBOX,
    DISPLAY_MODE_PANSCAN,
} DISPLAY_MODE_TYPE;

typedef enum
{
	SCART_CVBS = 0,
	SCART_RGB,
	SCART_SVIDEO,
	SCART_YUV,
} SCART_OUT_TYPE;

typedef enum
{
	SYS_DIGITAL_FMT_BY_EDID = 0,
	SYS_DIGITAL_FMT_RGB,
	SYS_DIGITAL_FMT_RGB_EXPD,
	SYS_DIGITAL_FMT_YCBCR_444,
	SYS_DIGITAL_FMT_YCBCR_422,
} DIGITAL_FMT_TYPE;

typedef enum
{
	SYS_DIGITAL_AUD_BS = 0,
	SYS_DIGITAL_AUD_LPCM,
	SYS_DIGITAL_AUD_AUTO,		//by TV EDID setting
}DIGITAL_AUD_TYPE;

/* tve adjust table for SD output */
static struct tve_adjust g_sd_tve_adjust_table[] = {
    {TVE_ADJ_COMPOSITE_Y_DELAY, SYS_625_LINE, 1}, {TVE_ADJ_COMPOSITE_Y_DELAY, SYS_525_LINE, 1},
    {TVE_ADJ_COMPOSITE_C_DELAY, SYS_625_LINE, 0}, {TVE_ADJ_COMPOSITE_C_DELAY, SYS_525_LINE, 0},
    {TVE_ADJ_COMPONENT_Y_DELAY, SYS_625_LINE, 4}, {TVE_ADJ_COMPONENT_Y_DELAY, SYS_525_LINE, 4},
    {TVE_ADJ_COMPONENT_CB_DELAY, SYS_625_LINE, 3}, {TVE_ADJ_COMPONENT_CB_DELAY, SYS_525_LINE, 3},
    {TVE_ADJ_COMPONENT_CR_DELAY, SYS_625_LINE, 1}, {TVE_ADJ_COMPONENT_CR_DELAY, SYS_525_LINE, 1},
    {TVE_ADJ_BURST_LEVEL_ENABLE, SYS_625_LINE, 1}, {TVE_ADJ_BURST_LEVEL_ENABLE, SYS_525_LINE, 0},
    {TVE_ADJ_BURST_CB_LEVEL, SYS_625_LINE, CB_LEVEL_PAL_SD/**/}, {TVE_ADJ_BURST_CB_LEVEL, SYS_525_LINE, CB_LEVEL_NTSC_SD/**/},
    {TVE_ADJ_BURST_CR_LEVEL, SYS_625_LINE, CR_LEVEL_PAL_SD/**/}, {TVE_ADJ_BURST_CR_LEVEL, SYS_525_LINE, CR_LEVEL_NTSC_SD/**/},
    {TVE_ADJ_COMPOSITE_LUMA_LEVEL, SYS_625_LINE, LUMA_LEVEL_PAL_SD/**/}, {TVE_ADJ_COMPOSITE_LUMA_LEVEL, SYS_525_LINE, LUMA_LEVEL_NTSC_SD/**/},
    {TVE_ADJ_COMPOSITE_CHRMA_LEVEL, SYS_625_LINE, CHRMA_LEVEL_PAL_SD/**/}, {TVE_ADJ_COMPOSITE_CHRMA_LEVEL, SYS_525_LINE, CHRMA_LEVEL_NTSC_SD/**/},
    {TVE_ADJ_PHASE_COMPENSATION, SYS_625_LINE, 0x280}, {TVE_ADJ_PHASE_COMPENSATION, SYS_525_LINE, 0x04a0},
    {TVE_ADJ_VIDEO_FREQ_RESPONSE, SYS_625_LINE, FREQ_RESPONSE_PAL_SD/**/}, {TVE_ADJ_VIDEO_FREQ_RESPONSE, SYS_525_LINE, FREQ_RESPONSE_NTSC_SD/**/},
};

static struct tve_adjust g_sd_tve_adjust_table_adv[] = {
    {TVE_ADJ_ADV_PEDESTAL_ONOFF, SYS_625_LINE, 0},              {TVE_ADJ_ADV_PEDESTAL_ONOFF, SYS_525_LINE, 1},
    {TVE_ADJ_ADV_COMPONENT_LUM_LEVEL, SYS_625_LINE, 0x53},      {TVE_ADJ_ADV_COMPONENT_LUM_LEVEL, SYS_525_LINE, 0x4f},
    {TVE_ADJ_ADV_COMPONENT_CHRMA_LEVEL, SYS_625_LINE, 0x51},    {TVE_ADJ_ADV_COMPONENT_CHRMA_LEVEL, SYS_525_LINE, 0x51},
    {TVE_ADJ_ADV_COMPONENT_PEDESTAL_LEVEL, SYS_625_LINE, 0x0},  {TVE_ADJ_ADV_COMPONENT_PEDESTAL_LEVEL, SYS_525_LINE, 0x9},
    {TVE_ADJ_ADV_COMPONENT_SYNC_LEVEL, SYS_625_LINE, 0x0},      {TVE_ADJ_ADV_COMPONENT_SYNC_LEVEL, SYS_525_LINE, 0x4},
    {TVE_ADJ_ADV_RGB_R_LEVEL, SYS_625_LINE, 0x8f},              {TVE_ADJ_ADV_RGB_R_LEVEL, SYS_525_LINE, 0x92},
    {TVE_ADJ_ADV_RGB_G_LEVEL, SYS_625_LINE, 0x8e},              {TVE_ADJ_ADV_RGB_G_LEVEL, SYS_525_LINE, 0x91},
    {TVE_ADJ_ADV_RGB_B_LEVEL, SYS_625_LINE, 0x8e},              {TVE_ADJ_ADV_RGB_B_LEVEL, SYS_525_LINE, 0x91},
    {TVE_ADJ_ADV_COMPOSITE_PEDESTAL_LEVEL, SYS_625_LINE, 0x0},  {TVE_ADJ_ADV_COMPOSITE_PEDESTAL_LEVEL, SYS_525_LINE, 0x2b},
    {TVE_ADJ_ADV_COMPOSITE_SYNC_LEVEL, SYS_625_LINE, 0x0},      {TVE_ADJ_ADV_COMPOSITE_SYNC_LEVEL, SYS_525_LINE, 0x5},
    {TVE_ADJ_ADV_PLUG_OUT_EN, SYS_625_LINE, 0x0},               {TVE_ADJ_ADV_PLUG_OUT_EN, SYS_525_LINE, 0x0},
    {TVE_ADJ_ADV_PLUG_DETECT_LINE_CNT_SD, SYS_625_LINE, 0x0},      {TVE_ADJ_ADV_PLUG_DETECT_LINE_CNT_SD, SYS_525_LINE, 0x0},
    {TVE_ADJ_ADV_PLUG_DETECT_AMP_ADJUST_SD, SYS_625_LINE, 0x0},    {TVE_ADJ_ADV_PLUG_DETECT_AMP_ADJUST_SD, SYS_525_LINE, 0x0},
};

/*************************** 576I.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_576i[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x3},
	{TVE_COMPOSITE_C_DELAY          ,   0x6},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x51},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x67},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x3},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x2},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x1},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x2},
	{TVE_COMPONENT_CB_DELAY         ,   0x2},
	{TVE_COMPONENT_CR_DELAY         ,   0x2},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x50},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0xb},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x7},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x47},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x47},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x47},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x205},
	{TVE_COMPONENT_R2V              ,   0x198},


	{TVE_BURST_POS_ENABLE           ,   0x1},
	{TVE_BURST_LEVEL_ENABLE         ,   0x1},
	{TVE_BURST_CB_LEVEL             ,   0x96},
	{TVE_BURST_CR_LEVEL             ,   0x6a},
	{TVE_BURST_START_POS            ,   0x55},
	{TVE_BURST_END_POS              ,   0x74},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x61e8},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},

    //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x4},
	{TVE_CB_CR_INSERT_SW            ,   0x0},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,   0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},

};


/*************************** 480I.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_480i[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x3},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x4c},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x60},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x5},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x1},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x3},
	{TVE_COMPONENT_CB_DELAY         ,   0x1},
	{TVE_COMPONENT_CR_DELAY         ,   0x1},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x4c},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0xb},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x7},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x47},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x47},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x47},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x1},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x1},
	{TVE_COMPONENT_PED_ADJUST       ,   0xa8},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x205},
	{TVE_COMPONENT_R2V              ,   0x198},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x1},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x55},
	{TVE_BURST_END_POS              ,   0x74},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x3400},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	//added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x7},
	{TVE_CB_CR_INSERT_SW            ,   0x1},//0x84,28bit	//C3511 ижии3иж1бъ?S3511 ижии3иж0
	{TVE_VBI_LINE21_EN              ,   0x1},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD              ,   0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},
};


/*************************** 576P.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_576p[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x1},
	{TVE_COMPONENT_CB_DELAY         ,   0x1},
	{TVE_COMPONENT_CR_DELAY         ,   0x1},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0xa},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x7},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x205},
	{TVE_COMPONENT_R2V              ,   0x198},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0xe}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 480P.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_480p[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x1},
	{TVE_COMPONENT_CB_DELAY         ,   0x1},
	{TVE_COMPONENT_CR_DELAY         ,   0x1},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0xa},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x7},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x205},
	{TVE_COMPONENT_R2V              ,   0x198},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
     //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0xe}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,   0x60},
    {TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0},
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,


    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},


};


/*************************** 720P_50.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_720p_50[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x0},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 720P_60.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_720p_60[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x7},
	{TVE_COMPONENT_CB_DELAY         ,   0x6},
	{TVE_COMPONENT_CR_DELAY         ,   0x6},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xf},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 1080I_25.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080i_25[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
     //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD              ,   0x60},
    {TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0},
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,


    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},

};


/*************************** 1080I_30.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080i_30[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x51},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
     //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD              ,   0x60},
    {TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0},
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,


    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},

};


/*************************** 1080P_24.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080p_24[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 1080P_25.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080p_25[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
     //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD              ,   0x60},
    {TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0},
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,


    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},


};


/*************************** 1080P_30.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080p_30[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x5},
	{TVE_COMPONENT_CB_DELAY         ,   0x4},
	{TVE_COMPONENT_CR_DELAY         ,   0x4},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x52},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 1152I_25.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1152i_25[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x4d},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 1080IASS.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080IASS[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x4d},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x5},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x4},
	{TVE_CAV_SYNC_HIGH              ,   0x1},
	{TVE_SYNC_HIGH_WIDTH            ,   0x6},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
	{TVE_COMPONENT_PLUG_OUT_EN      ,   0x1}      ,
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x0}      ,
	{TVE_VBI_LINE21_EN              ,   0x0}      ,
	{TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD,0x60},
	{TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0}      ,
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,
	{TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F}   ,
	{TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2}        ,
	{TVE_COMPONENT_VDAC_ENBUF,0x0}                ,


};


/*************************** 1080P_50.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080p_50[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x6},
	{TVE_COMPONENT_CB_DELAY         ,   0x6},
	{TVE_COMPONENT_CR_DELAY         ,   0x6},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x51},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x0},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x2},
	{TVE_CAV_SYNC_HIGH              ,   0x2},
	{TVE_SYNC_HIGH_WIDTH            ,   0x0},
	{TVE_SYNC_LOW_WIDTH             ,   0xd},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
     //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x1},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD              ,   0x60},
    {TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0},
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,


    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},

};


/*************************** 1080P_60.txt *******************************/
static T_TVE_ADJUST_ELEMENT table_1080p_60[TVE_ADJ_FIELD_NUM] =
{
	{TVE_COMPOSITE_Y_DELAY          ,   0x0},
	{TVE_COMPOSITE_C_DELAY          ,   0x2},
	{TVE_COMPOSITE_LUMA_LEVEL       ,   0x50},
	{TVE_COMPOSITE_CHRMA_LEVEL      ,   0x68},
	{TVE_COMPOSITE_SYNC_DELAY       ,   0x0},
	{TVE_COMPOSITE_SYNC_LEVEL       ,   0x4},
	{TVE_COMPOSITE_FILTER_C_ENALBE  ,   0x0},
	{TVE_COMPOSITE_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPOSITE_PEDESTAL_LEVEL   ,   0x0},


	{TVE_COMPONENT_IS_PAL           ,   0x0},
	{TVE_COMPONENT_PAL_MODE         ,   0x0},
	{TVE_COMPONENT_ALL_SMOOTH_ENABLE,   0x0},
	{TVE_COMPONENT_BTB_ENALBE       ,   0x0},
	{TVE_COMPONENT_INSERT0_ONOFF    ,   0x1},
	{TVE_COMPONENT_DAC_UPSAMPLEN    ,   0x1},
	{TVE_COMPONENT_Y_DELAY          ,   0x4},
	{TVE_COMPONENT_CB_DELAY         ,   0x5},
	{TVE_COMPONENT_CR_DELAY         ,   0x5},
	{TVE_COMPONENT_LUM_LEVEL        ,   0x52},
	{TVE_COMPONENT_CHRMA_LEVEL      ,   0x51},
	{TVE_COMPONENT_PEDESTAL_LEVEL   ,   0x0},
	{TVE_COMPONENT_UV_SYNC_ONOFF    ,   0x0},
	{TVE_COMPONENT_SYNC_DELAY       ,   0x0},
	{TVE_COMPONENT_SYNC_LEVEL       ,   0x3},
	{TVE_COMPONENT_R_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_G_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_B_SYNC_ONOFF     ,   0x0},
	{TVE_COMPONENT_RGB_R_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_G_LEVEL      ,   0x49},
	{TVE_COMPONENT_RGB_B_LEVEL      ,   0x49},
	{TVE_COMPONENT_FILTER_Y_ENALBE  ,   0x1},
	{TVE_COMPONENT_FILTER_C_ENALBE  ,   0x1},
	{TVE_COMPONENT_PEDESTAL_ONOFF   ,   0x0},
	{TVE_COMPONENT_PED_RGB_YPBPR_ENABLE,0x0},
	{TVE_COMPONENT_PED_ADJUST       ,   0x0},
	{TVE_COMPONENT_G2Y              ,   0x12a},
	{TVE_COMPONENT_G2U              ,   0x64},
	{TVE_COMPONENT_G2V              ,   0xd0},
	{TVE_COMPONENT_B2U              ,   0x21d},
	{TVE_COMPONENT_R2V              ,   0x1cb},


	{TVE_BURST_POS_ENABLE           ,   0x0},
	{TVE_BURST_LEVEL_ENABLE         ,   0x0},
	{TVE_BURST_CB_LEVEL             ,   0xd4},
	{TVE_BURST_CR_LEVEL             ,   0x0},
	{TVE_BURST_START_POS            ,   0x0},
	{TVE_BURST_END_POS              ,   0x0},
	{TVE_BURST_SET_FREQ_MODE        ,   0x0},
	{TVE_BURST_FREQ_SIGN            ,   0x0},
	{TVE_BURST_PHASE_COMPENSATION   ,   0x0},
	{TVE_BURST_FREQ_RESPONSE        ,   0x0},


	{TVE_ASYNC_FIFO                 ,   0x2},
	{TVE_CAV_SYNC_HIGH              ,   0x2},
	{TVE_SYNC_HIGH_WIDTH            ,   0xb},
	{TVE_SYNC_LOW_WIDTH             ,   0xb},


	{TVE_VIDEO_DAC_FS               ,   0x0},
	{TVE_SECAM_PRE_COEFFA3A2        ,   0x0},
	{TVE_SECAM_PRE_COEFFB1A4        ,   0x0},
	{TVE_SECAM_PRE_COEFFB3B2        ,   0x0},
	{TVE_SECAM_F0CB_CENTER          ,   0x0},
	{TVE_SECAM_F0CR_CENTER          ,   0x0},
	{TVE_SECAM_FM_KCBCR_AJUST       ,   0x0},
	{TVE_SECAM_CONTROL              ,   0x0},
	{TVE_SECAM_NOTCH_COEFB1         ,   0x0},
	{TVE_SECAM_NOTCH_COEFB2B3       ,   0x0},
	{TVE_SECAM_NOTCH_COEFA2A3       ,   0x0},
     //added
    {TVE_COMPONENT_PLUG_OUT_EN      ,   0x1},
	{TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD,0x6}   ,
	{TVE_CB_CR_INSERT_SW            ,   0x1},//0x84,28bit
	{TVE_VBI_LINE21_EN              ,   0x0},//0xc8,14bit
    {TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD              ,   0x60},
    {TVE_SCART_PLUG_DETECT_LINE_CNT_HD, 0x0},
	{TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD,0x0}     ,


    {TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT,0x1F},
    {TVE_COMPONENT_PLUG_DETECT_VCOUNT,0x2},
    {TVE_COMPONENT_VDAC_ENBUF,0x0},

};

//if have no the tv sys ,pls set null,can't delete it.
static T_TVE_ADJUST_TABLE g_tve_adjust_table_total[TVE_SYS_NUM] =
{
    	{SYS_576I , table_576i},
        {SYS_480I , table_480i},
        {SYS_576P , table_576p},
        {SYS_480P , table_480p},
        {SYS_720P_50 , table_720p_50},
        {SYS_720P_60 , table_720p_60},
        {SYS_1080I_25 , table_1080i_25},
        {SYS_1080I_30 , table_1080i_30},
        {SYS_1080P_24 , table_1080p_24 },
        {SYS_1080P_25 , table_1080p_25},
        {SYS_1080P_30 , table_1080p_30 },
        {SYS_1152I_25 , table_1152i_25},
        {SYS_1080IASS , table_1080IASS},
        {SYS_1080P_50 , table_1080p_50},
        {SYS_1080P_60 , table_1080p_60},
};

static ADF_BOOT_AVINFO g_avinfo =
{
	.tvSystem = CONFIG_TV_SYSTEM,
	.progressive = FALSE,
	.tv_ratio = TV_ASPECT_RATIO_AUTO,
	.display_mode = DISPLAY_MODE_LETTERBOX,

	.scart_out = SCART_YUV,
	.vdac_out[0] = DAC0_TYPE,
	.vdac_out[1] = DAC1_TYPE,
	.vdac_out[2] = DAC2_TYPE,
	.vdac_out[3] = DAC3_TYPE,
	.vdac_out[4] = VDAC_NULL,
	.vdac_out[5] = VDAC_NULL,
	.video_format = SYS_DIGITAL_FMT_RGB,

	.audio_output = SYS_DIGITAL_AUD_LPCM,
	.brightness = 50,
	.contrast = 50,
	.saturation = 50,

	.sharpness = 5,
	.hue = 50,
	.snd_mute_gpio = CONFIG_MUTE_GPIO,
	.snd_mute_polar = 0,
	.hdcp_disable = CONFIG_HDCP_DISABLE,
};

#ifdef __cplusplus
}
#endif

/*!
@}
*/

#endif

