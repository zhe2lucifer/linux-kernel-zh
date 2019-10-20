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

//
// $Id: board_config.c,v 1.0 2013/07/13 17:02:53 $
//
//////////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
/**
*
* @file board_config.h
*
* The XTemac driver. Functions in this file are the minimum required functions
* for this driver. See xtemac.h for a detailed description of the driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00a joy  2013/07/13 First release
* </pre>
******************************************************************************/

#ifndef ALI_BOARD_CFG
#define ALI_BOARD_CFG

#include <asm/pgtable.h>
#include <mach/ali-s3921.h>

#define BOARD_NAME	"ALi s3921 Development Board"

#ifdef CONFIG_DVB_ALI_M36_DMX_CACHE
#define CONFIG_DMX_CACHE
#endif

/**
 * mapping table description for 1G solution
 * ----------------------------------
 * |       Kernel                   |
 * ----------------------------------
 * |       MALI                     |
 * ----------------------------------
 * |       FB0/2                    |
 * ----------------------------------   0x0dc00000
 * |       Video Encoder            |
 * ----------------------------------   0x0bc00000
 * |       Video Decoder            |
 * ----------------------------------   0x06200000
 * |       DSC                      |
 * ----------------------------------   0x06000000  <---Fixed Area End
 * |       MCAPI/Share Memory       |
 * ----------------------------------   0x05ffd000
 * |       Private Memory           |
 * ----------------------------------   0x04000000  <---Fixed Area Start
 * |       User Data                |
 * ----------------------------------   0x03fe0000
 * |       Still Frame              |
 * ----------------------------------   0x03ce0000
 * |       Image                    |
 * ----------------------------------   0x03ae0000
 * |       Meida Player             |
 * ----------------------------------   0x030e0000
 * |       TSG                      |
 * ----------------------------------   0x030a0000
 * |       DMX                      |
 * ----------------------------------   0x02aa0000
 * |       NIM                      |
 * ----------------------------------   0x028a0000
 * |       FB0 CMAP                 |
 * ----------------------------------   0x0289fc00
 * |       FB2 CMAP                 |
 * ----------------------------------   0x0289f800
 * |       Boot Command             |
 * ----------------------------------   0x0279f800
 * |       Deca                     |
 * ----------------------------------   0x0277f800
 * |       File Signature           |
 * ----------------------------------
 * |       Kernel/Code              |
 * ----------------------------------   0x00000000
 */

/** Private Memory Map
 * ----------------------------------   0x05ffd000
 * |       AUDIO CPU                |
 * ----------------------------------   0x05efd000
 * |       VDEC VBV                 |
 * ----------------------------------   0x058fd000
 * |       VDEC CMDQ/LAF            |
 * ----------------------------------   0x0573d800
 * |       VCAP                     |
 * ----------------------------------   0x05401000
 * |       FB2                      |
 * ----------------------------------   0x05206000
 * |       Subtitle                 |
 * ----------------------------------   0x05148000
 * |       TTX                      |
 * ----------------------------------   0x05001a98
 * |       SEE Heap/Code            |
 * ----------------------------------   0x04000000
*/


/***************************************************************************
 *                       Define the memory size                            *
 ***************************************************************************/

/* FB0 Size */
#define FB0_VIRTUAL_WIDTH               1280
#define FB0_VIRTUAL_HEIGHT              1440
#define FB0_WIDTH                       1280
#define FB0_HEIGHT                      720
#define FB0_BPP                         4//bytes per pixel
#define FB0_PITCH                       FB0_VIRTUAL_WIDTH

#define FB0_MEM_SIZE                    (((FB0_VIRTUAL_HEIGHT * FB0_PITCH * FB0_BPP) + 0xFFF) & 0xFFFFF000)
#define FB0_CMAP_MEM_SIZE               (1024)

/* FB2 Size */
#define FB2_VIRTUAL_WIDTH               1920
#define FB2_VIRTUAL_HEIGHT              1080
#define FB2_WIDTH                       1920
#define FB2_HEIGHT                      1080
#define FB2_BPP                         1// bytes per pixel
#define FB2_PITCH                       FB2_VIRTUAL_WIDTH

#define FB2_MEM_SIZE                    (((FB2_VIRTUAL_HEIGHT * FB2_PITCH * FB2_BPP) + 0xFFF) & 0xFFFFF000)
#define FB2_CMAP_MEM_SIZE               (1024)

/* Video Size */
#define VDEC_FB_MEM_SIZE                (0x02634000) //9 * (Frame Buffer + DVIEW + MV)

#ifdef CONFIG_ALI_PIP
#define VDEC_VBV_MEM_SIZE               (0x400000)
#else
#define VDEC_VBV_MEM_SIZE               (0x600000)
#endif

#define VCAP_FB_MEM_SIZE                (736*576*2*3)
#define STILL_FB_MEM_SIZE               (0x300000)

#define VDEC_CMD_QUEUE_MEM_SIZE         (0x10000)
#define VDEC_LAF_FLAG_MEM_SIZE          (0x11000)
#define VDEC_LAF_RW_MEM_SIZE            (0x17E800) //MPEG2 laf/laf rw needs 0x19A400
#define VDEC_MB_NEI_SIZE                (0x20000)
#define VDEC_HW_BUF_SIZE                (VDEC_CMD_QUEUE_MEM_SIZE + VDEC_LAF_FLAG_MEM_SIZE + VDEC_LAF_RW_MEM_SIZE + VDEC_MB_NEI_SIZE)

/* MCAPI Size */
#define MCAPI_MEM_SIZE                  (0x2000)

/* Define the DSC Size (DSC+AES+DES/TDES+CSA+SHA) */
#define ALI_DSC_KERNEL_BUFFER_SIZE      (0x100000)
#define SHARED_MEM_SIZE                 (4096)
#define DSC_MEM_SIZE                    (ALI_DSC_KERNEL_BUFFER_SIZE*2) //DSC , input 1M + output 1M

/* Audio Size */
#define AUDIO_MEM_SIZE                  (0x100000)

/* User Confg Size*/
#define USER_DATA_MEM_SIZE              (0x20000)	// 4k

/* Boot Logo Size */
#define BOOTLOGO_DATA_MEM_SIZE          (0x100000)

/*Dmx Size*/
#define DMX_SEE_BUF_SIZE                (0xBC000)
#define DMX_MAIN_BUF_SIZE               (0x00100000)

#ifdef CONFIG_ALI_MEM_256MB
#define DMX_MEM_SIZE                    (DMX_MAIN_BUF_SIZE*2+DMX_SEE_BUF_SIZE*2)
#else
#ifdef CONFIG_DMX_CACHE
#define DMX_MEM_SIZE                    (0x02000000)
#else
#define DMX_MEM_SIZE                    (0x00600000)
#endif
#endif

/* TSG Size */
#define TSG_MEM_SIZE                    (0x40000) /* 256K */

/* Media Player Size */
#define MP_MEM_SIZE                     (0xA00000)

#ifdef CONFIG_S3281_J83B
#define J83B_MEM_SIZE                   (0x00100000)
#else
#define J83B_MEM_SIZE                   (0)
#endif

/* Image Decoder Size */
#define IMAGE_DECODER_MEM_SIZE          (0x00200000)

#ifdef CONFIG_MALI
/* Mali Dedicated Size */
#define MALI_DEDICATED_MEM_SIZE         (0)

/* Mali UMP Size */
#define MALI_UMP_MEM_SIZE               (0)
#else
/* Mali Dedicated Size */
#define MALI_DEDICATED_MEM_SIZE         (0)

/* Mali UMP Size */
#define MALI_UMP_MEM_SIZE               (0)
#endif

/* DECA */
#define ALI_DECA_MEM_SIZE               (0x20000)

/* VENC */
#define ALI_VENC_MEM_SIZE               (0x2000000)

#ifdef CONFIG_FILE_SIGNATURE_ALIASIX
#define __MM_FILE_SIGNATURE_LEN         (0x00601000)
#else
#define __MM_FILE_SIGNATURE_LEN         (0)
#endif

#define SEE_MEM_SIZE                    (0x02000000)

/***************************************************************************
 *                       Define the memory size end                        *
 ***************************************************************************/


/***************************************************************************
 *                       Define the memory address                         *
 ***************************************************************************/

/* SEE memory map */
#define __MM_SEE_MEM_START          	(0x84000000)
#define __MM_SEE_MEM_END				(__MM_SEE_MEM_START + SEE_MEM_SIZE)

#define __MM_SEE_MAIN_SHARED_MEM_END    (__MM_SEE_MEM_END)
#define __MM_SEE_MAIN_SHARED_MEM_START  ((__MM_SEE_MAIN_SHARED_MEM_END - SHARED_MEM_SIZE) & 0xFFFFF000)

#define __MM_MCAPI_MEM_END              (__MM_SEE_MAIN_SHARED_MEM_START)
#define __MM_MCAPI_MEM_START            ((__MM_MCAPI_MEM_END - MCAPI_MEM_SIZE) & 0xFFFFF000)

#define __MM_AUDIO_ENGINE_END           (__MM_MCAPI_MEM_START)
#define __MM_AUDIO_ENGINE_START         ((__MM_AUDIO_ENGINE_END - AUDIO_MEM_SIZE) & 0xFFFFF000)

#define __MM_VBV_BUF_END                (__MM_AUDIO_ENGINE_START)
#define __MM_VBV_BUF_START              ((__MM_VBV_BUF_END - VDEC_VBV_MEM_SIZE) & 0xFFFFFF00)

#ifdef CONFIG_ALI_PIP
#define __MM_VDEC_PIP_VBV_BUF_END       (__MM_VBV_BUF_START)
#define __MM_VDEC_PIP_VBV_BUF_START     ((__MM_VDEC_PIP_VBV_BUF_END - VDEC_VBV_MEM_SIZE) & 0xFFFFFF00)
#else
#define __MM_VDEC_PIP_VBV_BUF_END       (__MM_VBV_BUF_START)
#define __MM_VDEC_PIP_VBV_BUF_START     (__MM_VBV_BUF_START)
#endif

#define __MM_VDEC_HW_BUF_END            (__MM_VDEC_PIP_VBV_BUF_START)
#define __MM_VDEC_HW_BUF_START          ((__MM_VDEC_HW_BUF_END - VDEC_HW_BUF_SIZE) & 0xFFFFFC00)

#ifdef CONFIG_ALI_PIP
#define __MM_VDEC_PIP_HW_BUF_END        (__MM_VDEC_HW_BUF_START)
#define __MM_VDEC_PIP_HW_BUF_START      ((__MM_VDEC_PIP_HW_BUF_END - VDEC_HW_BUF_SIZE) & 0xFFFFFC00)

#define __MM_SEE_VDEC_START             (__MM_VDEC_PIP_HW_BUF_START)
#else
#define __MM_VDEC_PIP_HW_BUF_END        (__MM_VDEC_HW_BUF_START)
#define __MM_VDEC_PIP_HW_BUF_START      (__MM_VDEC_HW_BUF_START)

#define __MM_VCAP_FB_END                (__MM_VDEC_PIP_HW_BUF_START)
#define __MM_VCAP_FB_START              ((__MM_VCAP_FB_END - VCAP_FB_MEM_SIZE) & 0xFFFFF000)

#define __MM_SEE_VDEC_START             (__MM_VCAP_FB_START)
#endif

#define __MM_SEE_PRIVATE_END            (__MM_SEE_VDEC_START)
#define __MM_SEE_PRIVATE_START          (__MM_SEE_MEM_START)
/* SEE memory map end */


/* Up from SEE memory end address */
#define __MM_DSC_MEM_START              (__MM_SEE_MEM_END)
#define __MM_DSC_MEM_END                (__MM_DSC_MEM_START + DSC_MEM_SIZE)

#define __MM_VIDEO_BUF_START            ((__MM_DSC_MEM_END + 0xFFF) & 0xFFFFF000)
#define __MM_VIDEO_BUF_END              (__MM_VIDEO_BUF_START + VDEC_FB_MEM_SIZE)

#ifdef CONFIG_ALI_PIP
#define __MM_VIDEO_PIP_BUF_START        ((__MM_VIDEO_BUF_END + 0xFFF) & 0xFFFFF000)
#define __MM_VIDEO_PIP_BUF_END          (__MM_VIDEO_PIP_BUF_START + VDEC_FB_MEM_SIZE)

#define __MM_VCAP_FB_START              ((__MM_VIDEO_PIP_BUF_END + 0xFFF) & 0xFFFFF000)
#define __MM_VCAP_FB_END                (__MM_VCAP_FB_START + VCAP_FB_MEM_SIZE)

#define __MM_VDEC_MEM_END               __MM_VCAP_FB_END
#else
#define __MM_VIDEO_PIP_BUF_START        (__MM_VIDEO_BUF_END)
#define __MM_VIDEO_PIP_BUF_END          (__MM_VIDEO_BUF_END)

#define __MM_VDEC_MEM_END               __MM_VIDEO_PIP_BUF_END
#endif

#ifdef CONFIG_ALI_MEM_256MB
#define BOOT_MEDIA_FILE_SIZE            (0x800000)
#define __MM_BOOT_MEDIA_FILE_BUF_START  (__MM_VDEC_MEM_END)
#define __MM_BOOT_MEDIA_FILE_BUF_END    (__MM_BOOT_MEDIA_FILE_BUF_START + BOOT_MEDIA_FILE_SIZE)

#define __MM_VENC_DATA_START            (__MM_BOOT_MEDIA_FILE_BUF_END)
#define __MM_VENC_DATA_END              (__MM_BOOT_MEDIA_FILE_BUF_END)
#else
#define __MM_VENC_DATA_START            (__MM_VDEC_MEM_END)
#define __MM_VENC_DATA_END              (__MM_VENC_DATA_START + ALI_VENC_MEM_SIZE)
#endif

#define __MM_FB2_MEM_START         		(__MM_VENC_DATA_END)
#define __MM_FB2_MEM_END           		(__MM_FB2_MEM_START + FB2_MEM_SIZE)

#define __MM_FB_MEM_START          		(__MM_FB2_MEM_END)
#define __MM_FB_MEM_END            		(__MM_FB_MEM_START + FB0_MEM_SIZE)

#ifdef CONFIG_MALI
#define __MM_MALI_MEM_START        		((__MM_FB_MEM_END + 0xFF) & 0xFFFFFF00)
#define __MM_MALI_MEM_END          		(__MM_MALI_MEM_START + MALI_DEDICATED_MEM_SIZE)

#define __MM_MALI_UMP_MEM_START   		((__MM_MALI_MEM_END + 0xFF) & 0xFFFFFF00)
#define __MM_MALI_UMP_MEM_END     		(__MM_MALI_UMP_MEM_START + MALI_UMP_MEM_SIZE)
#else
#define __MM_MALI_MEM_START        		(__MM_FB_MEM_END)
#define __MM_MALI_MEM_END				(__MM_FB_MEM_END)

#define __MM_MALI_UMP_MEM_START   		(__MM_MALI_MEM_END)
#define __MM_MALI_UMP_MEM_END			(__MM_MALI_MEM_END)
#endif
/* Up from SEE memory end address end */


/* Down from SEE memory start address */
#define __MM_USER_DATA_END              (__MM_SEE_MEM_START)
#define __MM_USER_DATA_START            (__MM_USER_DATA_END - USER_DATA_MEM_SIZE)

#define __MM_STILL_FRAME_END            (__MM_USER_DATA_START)
#define __MM_STILL_FRAME_START          ((__MM_STILL_FRAME_END - STILL_FB_MEM_SIZE) & 0xFFFFF000)


#ifdef CONFIG_ALI_MEM_256MB
#define __MM_IMAGE_DECODER_MEM_END      (__MM_STILL_FRAME_START)
#define __MM_IMAGE_DECODER_MEM_START    (__MM_STILL_FRAME_START)
#else
#define __MM_IMAGE_DECODER_MEM_END      (__MM_STILL_FRAME_START)
#define __MM_IMAGE_DECODER_MEM_START    ((__MM_IMAGE_DECODER_MEM_END - IMAGE_DECODER_MEM_SIZE) & 0xFFFFFF00)
#endif

#define __MM_MP_MEM_END                 (__MM_IMAGE_DECODER_MEM_START)
#define __MM_MP_MEM_START               (__MM_MP_MEM_END - MP_MEM_SIZE)

#ifdef CONFIG_ALI_PIP
#define __MM_MP_PIP_MEM_END             (__MM_MP_MEM_START)
#define __MM_MP_PIP_MEM_START           (__MM_MP_PIP_MEM_END - MP_MEM_SIZE)
#else
#define __MM_MP_PIP_MEM_END             (__MM_MP_MEM_START)
#define __MM_MP_PIP_MEM_START           (__MM_MP_MEM_START)
#endif

#define __MM_TSG_MEM_END                (__MM_MP_PIP_MEM_START)
#define __MM_TSG_MEM_START              ((__MM_TSG_MEM_END - TSG_MEM_SIZE) & 0xFFFFFF00)

#define __MM_DMX_MEM_END                (__MM_TSG_MEM_START)
#define __MM_DMX_MEM_START              ((__MM_DMX_MEM_END - DMX_MEM_SIZE) & 0xFFFFFF00)

#ifdef CONFIG_ALI_PIP
#define __MM_DMX_PIP_MEM_END            (__MM_DMX_MEM_START)
#define __MM_DMX_PIP_MEM_START          ((__MM_DMX_PIP_MEM_END - 2 * DMX_SEE_BUF_SIZE) & 0xFFFFFF00)
#else
#define __MM_DMX_PIP_MEM_END            (__MM_DMX_MEM_START)
#define __MM_DMX_PIP_MEM_START          (__MM_DMX_MEM_START)
#endif

#define __MM_NIM_J83B_MEM_END           (__MM_DMX_PIP_MEM_START)
#define __MM_NIM_J83B_MEM_START         (__MM_NIM_J83B_MEM_END - J83B_MEM_SIZE)

#define __MM_FB0_CMAP_END               (__MM_NIM_J83B_MEM_START)
#define __MM_FB0_CMAP_START             ((__MM_FB0_CMAP_END - FB0_CMAP_MEM_SIZE) & 0xFFFFFFE0)

#define __MM_FB2_CMAP_END               (__MM_FB0_CMAP_START)
#define __MM_FB2_CMAP_START             ((__MM_FB2_CMAP_END - FB2_CMAP_MEM_SIZE) & 0xFFFFFFE0)

#ifdef CONFIG_ALI_MEM_256MB
#define __MM_BOOT_COMMAND_DATA_END      (__MM_FB2_CMAP_START)
#define __MM_BOOT_COMMAND_DATA_START    (__MM_FB2_CMAP_START)
#else
#define __MM_BOOT_COMMAND_DATA_END      (__MM_FB2_CMAP_START)
#define __MM_BOOT_COMMAND_DATA_START    (__MM_BOOT_COMMAND_DATA_END - BOOTLOGO_DATA_MEM_SIZE)
#endif

#define __MM_DECA_DATA_END              (__MM_BOOT_COMMAND_DATA_START)
#define __MM_DECA_DATA_START            (__MM_DECA_DATA_END - ALI_DECA_MEM_SIZE)

#ifdef CONFIG_FILE_SIGNATURE_ALIASIX
#define __MM_FILE_SIGNATURE_END_ADDR    (__MM_DECA_DATA_START)
#define __MM_FILE_SIGNATURE_START_ADDR  (__MM_FILE_SIGNATURE_END_ADDR - __MM_FILE_SIGNATURE_LEN)
#else
#define __MM_FILE_SIGNATURE_END_ADDR    (__MM_DECA_DATA_START)
#define __MM_FILE_SIGNATURE_START_ADDR	(__MM_DECA_DATA_START)
#endif
/* Down from SEE memory start address end */


#define __MM_RESERVED_ADDR_START        (__MM_FILE_SIGNATURE_START_ADDR)
#define __MM_RESERVED_ADDR_END          (__MM_MALI_UMP_MEM_END)


#define INIT_RAMFS_SIZE                 (0x02000000)
#define __MM_RAMFS_START                __MM_MALI_UMP_MEM_END
#define __MM_RAMFS_END                  __MM_MALI_UMP_MEM_END + INIT_RAMFS_SIZE

#define __MM_ALI_PRIVATE_MEM_START  	ALI_MEMALIGNDOWN(__MM_RESERVED_ADDR_START)
#define __MM_ALI_PRIVATE_MEM_END     	ALI_MEMALIGNUP(__MM_RESERVED_ADDR_END)

#define __MM_ALI_PRIVATE_MEM_END1     	ALI_MEMALIGNUP(__MM_RAMFS_END)


/***************************************************************************
 *                       Define the memory address end                     *
 ***************************************************************************/


/* Function Prototype Need To Be Implemented By Each Board Start */
struct ali_hwbuf_desc* ali_get_privbuf_desc(int * hwbuf_desc_cnt);
void customize_board_setting(void);


/* Define the I2C GPIO info  */
#if defined CONFIG_ALI_PAN_CH455 || defined CONFIG_ALI_PAN_CH454
#define I2C_GPIO_SDA_PIN                87
#define I2C_GPIO_SCL_PIN                12
#endif

enum
{
    I2C_DEVICE_ID = 4,
    I2C_DEVICE_HDMI_ID = 5,
};

/* Define AFD */
#ifdef CONFIG_ALI_SW_AFD
#define AFD_SW_SUPPORT
#endif
#ifdef CONFIG_ALI_HW_AFD
#define AFD_HW_SUPPORT
#endif

#if( defined (AFD_SW_SUPPORT)) || ( defined(AFD_HW_SUPPORT))
#define SUPPORT_AFD_PARSE
#define SUPPORT_AFD_SCALE
#define SUPPORT_AFD_WSS_OUTPUT
#endif

#endif

