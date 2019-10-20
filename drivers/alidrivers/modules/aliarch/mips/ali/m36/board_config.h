/*
*      Alitech
*
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License
*   as published by the Free Software Foundation; either version
*   2 of the License, or (at your option) any later version.
*/

#ifndef ALI_BOARD_CFG
#define ALI_BOARD_CFG

/**
 * mapping table
 * ----------------------------------
 * |       Kernel                   |
 * ----------------------------------
 * |       MALI                     |
 * ----------------------------------
 * |       FB0                      |
 * ----------------------------------   0x20000000
 * |       Kernel                   |
 * ----------------------------------
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
 * ----------------------------------   0x03FE0000
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
 * ----------------------------------   0x05Afd000
 * |       VDEC CMDQ/LAF/NEI        |
 * ----------------------------------   0x0593D800
 * |       VCAP                     |
 * ----------------------------------   0x05601000
 * |       FB2                      |
 * ----------------------------------   0x05406000
 * |       Subtitle                 |
 * ----------------------------------   0x05348000
 * |       TTX                      |
 * ----------------------------------   0x05200a98
 * |       SEE Heap/Code            |
 * ----------------------------------   0x04000000
*/


#define CONFIG_ALI_OTT_DVB_256M_MAPPING

/* FB0 Size */
#define FB0_VIRTUAL_WIDTH               1280
#define FB0_VIRTUAL_HEIGHT              (720 * 4)
#define FB0_WIDTH                       1280
#define FB0_HEIGHT                      720
#define FB0_BPP                         4//bytes per pixel
#define FB0_PITCH                       FB0_VIRTUAL_WIDTH

#define DFB_MEM_SIZE                    (0x01200000)
#define FB0_MEM_SIZE                    (((FB0_VIRTUAL_HEIGHT * FB0_PITCH * FB0_BPP) + DFB_MEM_SIZE + 0xFFF) & 0xFFFFF000)
#define FB0_CMAP_MEM_SIZE               (1024)

/* FB2 Size */
#define FB2_VIRTUAL_WIDTH               1920
#define FB2_VIRTUAL_HEIGHT              1080
#define FB2_WIDTH                       1920
#define FB2_HEIGHT                      1080

#define FB2_BPP                         1// bytes per pixel
#define FB2_PITCH                       FB2_VIRTUAL_WIDTH

#define FB2_MEM_SIZE                    (((FB2_VIRTUAL_HEIGHT * FB2_PITCH * FB2_BPP) + 0xFFF) & 0xFFFFF000)
#define FB2_CMAP_MEM_SIZE				(1024)

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

#define AUDIO_MEM_SIZE                  (0x100000)
/* User Confg Size*/
#define USER_DATA_MEM_SIZE              (0x20000)        // 128k
/* Boot Logo Size */

/*Dmx Size*/
#define DMX_SEE_BUF_SIZE                (0xBC000)
#define DMX_MAIN_BUF_SIZE               (0x100000)
#define DMX_MEM_SIZE                    (DMX_MAIN_BUF_SIZE * 2 + DMX_SEE_BUF_SIZE * 2)

/* TSG Size */
#define TSG_MEM_SIZE                    (0x40000) /* 256K */
#define MP_MEM_SIZE                     (0xA00000)

#ifdef CONFIG_S3281_J83B
#define J83B_MEM_SIZE                   (0x00100000)
#else
#define J83B_MEM_SIZE                   (0)
#endif

#ifdef CONFIG_S3821_DVBT2_ISDBT
#define NIM_MEM_SIZE                    (0x0800000)
#else
#define NIM_MEM_SIZE                    (0)
#endif

#ifdef CONFIG_ALI_INIT_RAMFS
#define INIT_RAMFS_SIZE                 (0x02000000)
#else
#define INIT_RAMFS_SIZE                 (0)
#endif

#define GE_CMDQ_BUF_LEN 		        (0x3c000) /* 240K */
#define PAGE_SZ 0x1000                 /* FIXME: should be page table size */

#define ALI_DECA_MEM_SIZE               (0x20000)

#define SEE_MEM_SIZE                    (0x02000000)

#define __MM_SEE_MEM_START              (0xA4000000)
#define __MM_SEE_MEM_END				(__MM_SEE_MEM_START + SEE_MEM_SIZE)

/*  0xa600_0000 ~ 0xaa84_4000  +++++++++++++++++++++++++++++++++++++++*/
#define __MM_DSC_MEM_START              (__MM_SEE_MEM_START + SEE_MEM_SIZE)
#define __MM_DSC_MEM_END                (__MM_DSC_MEM_START + DSC_MEM_SIZE)

#define __MM_VIDEO_BUF_START            ((__MM_DSC_MEM_END + 0xFFF) & 0xFFFFF000)
#define __MM_VIDEO_BUF_END              (__MM_VIDEO_BUF_START + VDEC_FB_MEM_SIZE)

#ifdef CONFIG_SUPPORT_AS_VFB

#ifdef CONFIG_ALI_PIP
#define __MM_VIDEO_PIP_BUF_START        ((__MM_VIDEO_BUF_END + 0xFFF) & 0xFFFFF000)
#define __MM_VIDEO_PIP_BUF_END          (__MM_VIDEO_PIP_BUF_START + VDEC_FB_MEM_SIZE)

#define __MM_VDEC_HW_BUF_START          ((__MM_VIDEO_PIP_BUF_END + 0x3FF) & 0xFFFFFC00)
#define __MM_VDEC_HW_BUF_END            (__MM_VDEC_HW_BUF_START + VDEC_HW_BUF_SIZE)

#define __MM_VDEC_PIP_HW_BUF_START      ((__MM_VDEC_HW_BUF_END + 0x3FF) & 0xFFFFFC00)
#define __MM_VDEC_PIP_HW_BUF_END        (__MM_VDEC_PIP_HW_BUF_START + VDEC_HW_BUF_SIZE)

#define __MM_VCAP_FB_START              ((__MM_VDEC_PIP_HW_BUF_END + 0xFFF) & 0xFFFFF000)
#define __MM_VCAP_FB_END                (__MM_VCAP_FB_START + VCAP_FB_MEM_SIZE)
#else
#define __MM_VDEC_HW_BUF_START          ((__MM_VIDEO_BUF_END + 0x3FF) & 0xFFFFFC00)
#define __MM_VDEC_HW_BUF_END            (__MM_VDEC_HW_BUF_START + VDEC_HW_BUF_SIZE)

#define __MM_VCAP_FB_START              ((__MM_VDEC_HW_BUF_END + 0xFFF) & 0xFFFFF000)
#define __MM_VCAP_FB_END                (__MM_VCAP_FB_START + VCAP_FB_MEM_SIZE)
#define __MM_VIDEO_PIP_BUF_START        (__MM_VIDEO_BUF_END)
#define __MM_VIDEO_PIP_BUF_END          (__MM_VIDEO_BUF_END)
#define __MM_VDEC_PIP_HW_BUF_END        (__MM_VDEC_HW_BUF_START)
#define __MM_VDEC_PIP_HW_BUF_START      (__MM_VDEC_HW_BUF_START)

#endif
#define __MM_VDEC_MEM_END               __MM_VCAP_FB_END


#else /* CONFIG_SUPPORT_AS_VFB */

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

#endif /* CONFIG_SUPPORT_AS_VFB */

#define __MM_VENC_DATA_START            (__MM_VDEC_MEM_END)
#define __MM_VENC_DATA_END              (__MM_VDEC_MEM_END)

#define __MM_FB2_MEM_START              (__MM_VENC_DATA_END)
#define __MM_FB2_MEM_END                (__MM_FB2_MEM_START + FB2_MEM_SIZE)

#define __MM_FB_MEM_START               (__MM_FB2_MEM_END)
#define __MM_FB_MEM_END                 (__MM_FB_MEM_START + FB0_MEM_SIZE)

#define __MM_NIM_DATA_START             ((__MM_FB_MEM_END + 0xFF) & 0xFFFFFF00)
#define __MM_NIM_DATA_END               (__MM_NIM_DATA_START + NIM_MEM_SIZE)

#define __MM_INIT_RAMFS_START           (__MM_NIM_DATA_END)
#define __MM_INIT_RAMFS_END             (__MM_INIT_RAMFS_START + INIT_RAMFS_SIZE)


/*  end 0xa600_0000 ~ 0xaa84_4000  +++++++++++++++++++++++++++++++++++++++*/

/* see memory  0xa600_0000  ~  0xa400_0000 -----------------------------------------*/
#define __MM_SEE_MAIN_SHARED_MEM_START  ((__MM_SEE_MEM_END - SHARED_MEM_SIZE) & 0xFFFFF000) /* 0x5FFF000 */

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

#ifdef CONFIG_SUPPORT_AS_VFB
#define __MM_SEE_VDEC_START             (__MM_VDEC_PIP_VBV_BUF_START)

#else /* CONFIG_SUPPORT_AS_VFB */

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

#endif /* CONFIG_SUPPORT_AS_VFB */

#define __MM_SEE_PRIVATE_END            (__MM_SEE_VDEC_START)
#define __MM_SEE_PRIVATE_START          (__MM_SEE_MEM_START)

/* end see memory  0xa600_0000  ~  0xa400_0000 --------------------------------------------------------*/

/*bottom memory  0xa400_0000  ~  0xa2e0_e800 -----------------------------------------*/
#define __MM_USER_DATA_START            (__MM_SEE_MEM_START - USER_DATA_MEM_SIZE)
/* STILL FB
*/
#define __MM_STILL_FRAME_START          ((__MM_USER_DATA_START - STILL_FB_MEM_SIZE) & 0xFFFFF000)
#define __MM_STILL_FRAME_END            (__MM_STILL_FRAME_START + STILL_FB_MEM_SIZE)

/* Image Decoder
*/
#define __MM_IMAGE_DECODER_MEM_START    (__MM_STILL_FRAME_START)
#define __MM_IMAGE_DECODER_MEM_END      (__MM_STILL_FRAME_START)

/* GE
*/
#define __MM_GE_CMDQ_START              (__MM_IMAGE_DECODER_MEM_START - GE_CMDQ_BUF_LEN)
#define __MM_GE_CMDQ_END                (__MM_GE_CMDQ_START + GE_CMDQ_BUF_LEN)

/* Media Player (Can share with TSG/DMX/NIM) APE
*/
#define __MM_MP_MEM_START               (__MM_GE_CMDQ_START - MP_MEM_SIZE)
#define __MM_MP_MEM_END                 (__MM_MP_MEM_START + MP_MEM_SIZE)

#ifdef CONFIG_ALI_PIP
#define __MM_MP_PIP_MEM_END             (__MM_MP_MEM_START)
#define __MM_MP_PIP_MEM_START           (__MM_MP_PIP_MEM_END - MP_MEM_SIZE)
#else
#define __MM_MP_PIP_MEM_END             (__MM_MP_MEM_START)
#define __MM_MP_PIP_MEM_START           (__MM_MP_MEM_START)
#endif

/* TSG
*/
#define __MM_TSG_MEM_START              ((__MM_MP_PIP_MEM_START - TSG_MEM_SIZE) & 0xFFFFFF00)
#define __MM_TSG_MEM_END                (__MM_TSG_MEM_START + TSG_MEM_SIZE)

/* DMX
*/
#define __MM_DMX_MEM_START              ((__MM_TSG_MEM_START - DMX_MEM_SIZE) & 0xFFFFFF00)
#define __MM_DMX_MEM_END                (__MM_DMX_MEM_START + DMX_MEM_SIZE)

#ifdef CONFIG_ALI_PIP
#define __MM_DMX_PIP_MEM_START          ((__MM_DMX_MEM_START - 2 * DMX_SEE_BUF_SIZE) & 0xFFFFFF00)
#define __MM_DMX_PIP_MEM_END            (__MM_DMX_PIP_MEM_START + 2 * DMX_SEE_BUF_SIZE)
#else
#define __MM_DMX_PIP_MEM_START          (__MM_DMX_MEM_START)
#define __MM_DMX_PIP_MEM_END            (__MM_DMX_MEM_START)
#endif

/* NIM J83B
*/
#define __MM_NIM_J83B_MEM_START         (__MM_DMX_PIP_MEM_START - J83B_MEM_SIZE)
#define __MM_NIM_J83B_MEM_END           (__MM_NIM_J83B_MEM_START + J83B_MEM_SIZE)

/* FB0 CMP ADDR
*/
#define __MM_FB0_CMAP_START             ((__MM_NIM_J83B_MEM_START - FB0_CMAP_MEM_SIZE) & 0xFFFFFFE0)
#define __MM_FB0_CMAP_END               (__MM_FB0_CMAP_START + FB0_CMAP_MEM_SIZE)

/* FB2 CMP ADDR
*/
#define __MM_FB2_CMAP_START             ((__MM_FB0_CMAP_START - FB2_CMAP_MEM_SIZE) & 0xFFFFFFE0)
#define __MM_FB2_CMAP_END               (__MM_FB2_CMAP_START + FB2_CMAP_MEM_SIZE)

/* Boot Logo
*/
#define __MM_BOOT_COMMAND_DATA_START    (__MM_FB2_CMAP_START)
#define __MM_BOOT_COMMAND_DATA_END      (__MM_BOOT_COMMAND_DATA_START)

/* DECA
*/
#define __MM_DECA_DATA_START            (__MM_BOOT_COMMAND_DATA_START - ALI_DECA_MEM_SIZE)
#define __MM_DECA_DATA_END              (__MM_DECA_DATA_START + ALI_DECA_MEM_SIZE)
/*bottom memory  0xa400_0000  ~  0xa2e0_e800 -----------------------------------------*/

#define __MM_ALI_RESERVE_MEM_END      	ALI_MEMALIGNUP(__MM_INIT_RAMFS_END)
#define __MM_ALI_RESERVE_MEM_START      ALI_MEMALIGNDOWN(__MM_DECA_DATA_START)

struct ali_hwbuf_desc* ali_get_privbuf_desc(int * hwbuf_desc_cnt);
void customize_board_setting(void);

/* define AFD */
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

#define ALI_MEMALIGNDOWN(a) (a & (~(~PAGE_MASK - 1)))
#define ALI_MEMALIGNUP(a)   ((a + ~PAGE_MASK - 1) & (~ (~PAGE_MASK - 1)))

struct ali_hwbuf_desc
{
    const char *name;
    unsigned long phy_start;
    unsigned long phy_end;
};
#endif

