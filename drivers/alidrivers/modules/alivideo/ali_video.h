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

#ifndef __MEDIA_VIDEO_ALI_VIDEO_H
#define __MEDIA_VIDEO_ALI_VIDEO_H

#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/version.h>
#include <linux/semaphore.h>
#include <linux/ali_rpc.h>
#include <linux/ali_transport.h>
#include <rpc_hld/ali_rpc_hld_decv.h>
#include <rpc_hld/ali_rpc_hld_dis.h>

#include <ali_shm.h>
#include <ali_reg.h>
#include <ali_video_common.h>
#include <ali_sbm_common.h>
#include <ali_decv_plugin_common.h>
#include <ali_soc.h>

#include <ali_cache.h>
#include "fb_rotation.h"
#include <linux/ali_kumsgq.h>
#if 1
#define VDEC_PRINTF(idx, fmt, args...) \
	    do \
	    { \
		    if(ali_video_priv[(int)idx]->debug_on) \
		    { \
                printk("\033[34m[ali video%d]\033[0m", (int)idx); \
			    printk(fmt, ##args); \
		    } \
	    }while(0)
#else
#define VDEC_PRINTF(...)					do{}while(0)
#endif

#define DEVICE_NAME                         "ali_video"
#define DEVICE_NUM                          2

#define TMP_BUF_SIZE                        (128 * 1024)

/* status of this module */
enum ali_video_status
{
	ALI_VIDEO_NULL,
	ALI_VIDEO_IDLE,
	ALI_VIDEO_WORKING,
	ALI_VIDEO_PAUSED,
	ALI_VIDEO_ERROR,
};

/* private information about the this module */
struct ali_video_info {
    struct cdev cdev;
    int index;
    char name[10];
	enum ali_video_status status;
    struct task_struct * pts_task;
    unsigned int last_pts;
    struct vdec_device *cur_dev;
	struct semaphore sem;
    unsigned long mem_addr;
    unsigned long mem_size;
    unsigned long priv_mem_addr;
    unsigned long priv_mem_size;
    unsigned long vbv_mem_addr;
    unsigned long vbv_mem_size;
    unsigned long mp_mem_addr;
    unsigned long mp_mem_size;
    unsigned long capture_mem_addr;
    unsigned long capture_mem_size;
    unsigned long tmp_mem_addr;
    unsigned long tmp_mem_size;
    volatile unsigned long *rpc_arg[MAX_VIDEO_RPC_ARG_NUM];
    volatile int rpc_arg_size[MAX_VIDEO_RPC_ARG_NUM];
    struct vdec_callback call_back;
    int dst_pid;
	ali_vdeo_ctrl_blk ctrl_blk;
	int ctrl_blk_enable;
	int logo_mode;
    int write_header;
    struct file *sbm_file;
    int new_write;
	int open_count;
    int work_mode;
    int pause_decode;
    int pause_display;
    unsigned long codec_tag;
    void *codec_handle;
    enum video_decoder_type vdec_type;
    int debug_on;
	struct kumsgq *video_kumsgq;
};

extern unsigned long __G_ALI_MM_VIDEO_SIZE;
extern unsigned long __G_ALI_MM_VIDEO_START_ADDR;

extern unsigned long __G_ALI_MM_VIDEO_PIP_SIZE;
extern unsigned long __G_ALI_MM_VIDEO_PIP_START_ADDR;

extern unsigned long __G_ALI_MM_VDEC_HW_BUF_SIZE;
extern unsigned long __G_ALI_MM_VDEC_HW_BUF_START_ADDR;

extern unsigned long __G_ALI_MM_VDEC_PIP_HW_BUF_SIZE;
extern unsigned long __G_ALI_MM_VDEC_PIP_HW_BUF_START_ADDR;

extern unsigned long __G_ALI_MM_VDEC_VBV_SIZE;
extern unsigned long __G_ALI_MM_VDEC_VBV_START_ADDR;

extern unsigned long __G_ALI_MM_VDEC_PIP_VBV_SIZE;
extern unsigned long __G_ALI_MM_VDEC_PIP_VBV_START_ADDR;

extern unsigned long __G_ALI_MM_APE_MEM_SIZE;
extern unsigned long __G_ALI_MM_APE_MEM_START_ADDR;

extern unsigned long __G_ALI_MM_APE_PIP_MEM_SIZE;
extern unsigned long __G_ALI_MM_APE_PIP_MEM_START_ADDR;

extern unsigned long __G_ALI_MM_STILL_FRAME_SIZE;
extern unsigned long __G_ALI_MM_STILL_FRAME_START_ADDR;

extern unsigned long __G_ALI_MM_FB2_SIZE;
extern unsigned long __G_ALI_MM_FB2_START_ADDR;

extern struct ali_video_info *ali_video_priv[DEVICE_NUM];

extern struct vpo_device *g_vpo_dev;

void   hld_dev_memcpy(void *dest, const void *src, unsigned int len);
int    hld_decv_request_buf(struct ali_video_info *info, void *instance,void **buf_start,
                               int *buf_size, struct ctrl_blk *blk);
void   hld_decv_update_buf(struct ali_video_info *info, void *instance, int buf_size);
int    hld_decv_rpc_operation(struct ali_video_info *info, int API_idx);
void   hld_decv_rpc_release(struct ali_video_info *info, int force);
void   hld_decv_rpc_init(struct ali_video_info *info);
void   hld_decv_rpc_suspend(struct ali_video_info *info);
void   hld_decv_rpc_resume(struct ali_video_info *info);
INT32  hld_decv_start(struct ali_video_info *info);
INT32  hld_decv_stop(struct ali_video_info *info, struct vdec_stop_param *stop_param);
INT32  hld_decv_pause(struct ali_video_info *info);
INT32  hld_decv_resume(struct ali_video_info *info);
INT32  hld_decv_step(struct ali_video_info *info);
enum vdec_type hld_decv_get_cur_decoder(struct ali_video_info *info);
INT32  hld_decv_set_sync_mode(struct ali_video_info *info, struct vdec_sync_param *sync_param);
INT32  hld_decv_set_play_mode(struct ali_video_info *info, struct vdec_playback_param *playback_param);
INT32  hld_decv_set_pvr_param(struct ali_video_info *info, struct vdec_pvr_param *pvr_param);
INT32  hld_decv_select_decoder(struct ali_video_info *info, struct vdec_codec_param *codec_param);
INT32  hld_decv_set_output(struct ali_video_info *info, struct vdec_output_param *output_param);
INT32  hld_decv_ioctl(struct ali_video_info *info, UINT32 cmd, UINT32 param);
INT32  hld_decv_mp_init(struct ali_video_info *info, struct vdec_mp_init_param *init_param);
INT32  hld_decv_mp_rls(struct ali_video_info *info, struct vdec_mp_rls_param *rls_param);
INT32  hld_decv_mp_flush(struct ali_video_info *info, struct vdec_mp_flush_param *flush_param);
INT32  hld_decv_mp_extra_data(struct ali_video_info *info, struct vdec_mp_extra_data *extra_data_param);
INT32  hld_decv_mp_get_status(struct ali_video_info *info, struct vdec_decore_status *decore_status, struct vdec_information *vdec_stat);
INT32  hld_decv_mp_pause(struct ali_video_info *info, struct vdec_mp_pause_param *pause_param);
INT32  hld_decv_mp_set_sbm(struct ali_video_info *info, struct vdec_mp_sbm_param *sbm_param);
INT32  hld_decv_mp_set_sync_mode(struct ali_video_info *info, struct vdec_mp_sync_param *sync_param);
INT32  hld_decv_mp_set_display_rect(struct ali_video_info *info, struct vdec_display_rect *display_rect);
INT32  hld_decv_mp_set_quick_mode(struct ali_video_info *info, UINT32 quick_mode);
INT32  hld_decv_mp_capture_frame(struct ali_video_info *info, struct vdec_picture *picture);
INT32  hld_decv_mp_set_dec_frm_type(struct ali_video_info *info, UINT32 frm_type);
INT32  hld_decv_mp_ioctl(struct ali_video_info *info, UINT32 cmd, UINT32 param);
INT32  hld_decv_logo_init(struct ali_video_info *info);
INT32  hld_decv_logo_rls(struct ali_video_info *info);

void convert_h264_de2yuv(const unsigned char *y_addr, const unsigned char *c_addr, \
                             unsigned int pic_width, unsigned int pic_height, \
                             unsigned char *yuv_y, unsigned char *yuv_u, \
                             unsigned char *yuv_v, unsigned int stride_y, unsigned int stride_uv);
void convert_mpeg2_de2yuv(const unsigned char *y_addr, const unsigned char *c_addr, \
                               unsigned int pic_width, unsigned int pic_height, \
                               unsigned char *yuv_y, unsigned char *yuv_u, \
                               unsigned char *yuv_v, unsigned int stride_y, unsigned int stride_uv);

int  ali_video_procfs_init(int index, struct ali_video_info *info);
void ali_video_procfs_exit(void);

int vdec_display_mode(struct ali_video_info *info, struct vdec_display_param *pparam);

#endif
