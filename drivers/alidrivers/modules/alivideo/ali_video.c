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
 *  File: ali_video.c
 *  (I)
 *  Description: ali video player
 *  (S)
 *  History:(M)
 *          Date                    Author          Comment
 *          ====                    ======      =======
 * 0.       2009.12.24          Sam         Create
 ****************************************************************************/
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <ali_soc.h>
#include "ali_video.h"

int ali_video_ape_dbg_on = 0;
struct ali_video_info *ali_video_priv[DEVICE_NUM];

static int request_buf(void *handle, void **buf_start, int *buf_size, ali_vdeo_ctrl_blk *blk)
{
    struct ali_video_info *info = (struct ali_video_info *)handle;
    int ret = ALI_VIDEO_REQ_RET_ERROR;
    int work = 0;

    if(info->status == ALI_VIDEO_WORKING || info->status == ALI_VIDEO_PAUSED)
        work = 1;
    else
        work = 0;

    if(work){
        ret = hld_decv_request_buf(info, NULL, buf_start, buf_size, blk);
    }

    return ret;
}

static void update_buf(void *handle, int buf_size)
{
    hld_decv_update_buf((struct ali_video_info *)handle, NULL, buf_size);
}

static ssize_t ali_video_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return -1;
}

static ssize_t ali_video_write_es(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct ali_video_info *info = (struct ali_video_info *)file->private_data;
    struct av_packet pkt;
    ssize_t write_size = 0;
    INT32 res = 0;

    if (!info->logo_mode) {
        res = hld_decv_logo_init(info);
        if (res != 0) {
            return -EINVAL;
        }
    }

    if (info->sbm_file && info->sbm_file->f_op->write) {
        if (info->write_header) {
            memset(&pkt, 0, sizeof(pkt));
            pkt.size = count;
            pkt.pts  = AV_NOPTS_VALUE;
            write_size = info->sbm_file->f_op->write(info->sbm_file, (const char *)&pkt, sizeof(pkt), NULL);
        } else {
            write_size = sizeof(pkt);
        }

        if (write_size == sizeof(pkt)) {
            info->write_header = 0;
            write_size = info->sbm_file->f_op->write(info->sbm_file, buf, count, ppos);
            if(write_size == count) {
                info->write_header = 1;
            }
        } else {
            write_size = 0;
            VDEC_PRINTF(info->index, "SBM may be full\n");
        }
    } else {
        return -EINVAL;
    }

    return write_size;
}

static ssize_t ali_video_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct ali_video_info *info = (struct ali_video_info *)file->private_data;
    void *req_buf = NULL;
    int req_size = count;
    int req_ret = 0;
    int ret = 0;

    VDEC_PRINTF(info->index, "video %u write data %d\n", info->vdec_type, count);

    if(down_interruptible(&info->sem)){
        VDEC_PRINTF(info->index, "video down sem fail\n");
        return -EINVAL;
    }

    if(info->open_count <= 0) {
        VDEC_PRINTF(info->index, "video is not opened\n");
        goto EXIT;
    }

    if(info->new_write) {
        ret =  ali_video_write_es(file, buf, count, ppos);
        up(&info->sem);
        return ret;
    }

START:
    req_ret = request_buf(info, &req_buf, &req_size, NULL);
    if(req_ret == ALI_VIDEO_REQ_RET_OK) {
        char *user_buf_ptr = (char*)buf, *req_buf_ptr = (char*)req_buf;
        void *tmp_buf = (void *)info->tmp_mem_addr;
        int tmp_size = (int)info->tmp_mem_size, tmp_malloc = 0, write_size = req_size;

        if(tmp_buf == NULL) {
            tmp_buf = kmalloc(write_size, GFP_KERNEL);
            if(tmp_buf == NULL) {
                VDEC_PRINTF(info->index, "kmalloc request buf fail\n");
                ret = -EINVAL;
                goto EXIT;
            } else {
                tmp_size = write_size;
                tmp_malloc = 1;
            }
        }

        do {
            tmp_size = (tmp_size > write_size) ? write_size : tmp_size;

            if(copy_from_user(tmp_buf, user_buf_ptr, tmp_size))
                return -EFAULT;
#if defined(CONFIG_ALI_CHIP_M3921)

#else
            __CACHE_FLUSH_ALI((unsigned long)tmp_buf, req_size);
#endif
            /* Main CPU should use below mempy to transfer the data to private memory */
            hld_dev_memcpy(req_buf_ptr, (void *)__VMTSALI(tmp_buf), tmp_size);
            //memcpy((void*)__VSTMALI(req_buf_ptr), tmp_buf, tmp_size);
            user_buf_ptr += tmp_size;
            req_buf_ptr  += tmp_size;
            write_size   -= tmp_size;
        } while(write_size > 0);

        if(tmp_malloc) {
            kfree(tmp_buf);
        }

        update_buf(info, req_size);
        ret += req_size;
        if(req_size < count) {
            count = req_size = count - req_size;
            buf += req_size;
            goto START;
        }
    }else if(req_ret == ALI_VIDEO_REQ_RET_ERROR) {
        ret = -EINVAL;
    }

EXIT:
    up(&info->sem);

    return ret;
}

static int video_ioctl_internal(struct ali_video_info *info, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    switch(cmd){
        case ALIVIDEOIO_VIDEO_STOP:
        {
            if(info->status == ALI_VIDEO_WORKING || info->status == ALI_VIDEO_PAUSED){
                info->status = ALI_VIDEO_IDLE;
            }
            break;
        }
        case ALIVIDEOIO_VIDEO_PLAY:
        {
            if(info->status == ALI_VIDEO_IDLE){
                info->status = ALI_VIDEO_WORKING;
                VDEC_PRINTF(info->index, "start ali video player done\n");
            }
            break;
        }
        case ALIVIDEOIO_RPC_OPERATION:
        {
            struct ali_video_rpc_pars pars;
            int i = 0;

            if(copy_from_user((void *)&pars, (void *)arg, sizeof(pars)))
                return -EFAULT;
            for(i = 0; i < pars.arg_num; i++) {
                info->rpc_arg_size[i] = pars.arg[i].arg_size;
                if((info->rpc_arg_size[i] > 0) && (pars.arg[i].arg != NULL)) {
                    if(info->rpc_arg[i] == NULL || info->rpc_arg_size[i] > MAX_VIDEO_RPC_ARG_SIZE) {
                        VDEC_PRINTF(info->index, "allocate rpc arg buf fail\n");
                        ret = -ENOMEM;
                        goto RPC_EXIT;
                    }

                    if(copy_from_user((void *)info->rpc_arg[i], pars.arg[i].arg, info->rpc_arg_size[i]))
                        return -EFAULT;
                }
            }

            ret = hld_decv_rpc_operation(info, pars.API_ID);

RPC_EXIT:
            for(i = 0;i < pars.arg_num;i++) {
                if((info->rpc_arg_size[i] > 0) && (pars.arg[i].arg != NULL)) {
                    if(pars.arg[i].out) {
                        if(copy_to_user(pars.arg[i].arg, (void *)info->rpc_arg[i], info->rpc_arg_size[i]))
                            return -EFAULT;
                    }
                }
            }

            break;
        }
        case ALIVIDEOIO_SET_SOCK_PORT_ID:
        {
            info->dst_pid = (int)arg;
            //hld_decv_rpc_init(info);
            break;
        }
        case ALIVIDEOIO_GET_MEM_INFO:
        {
            struct ali_video_mem_info mem_info;
            mem_info.mem_size = info->mem_size;
            mem_info.mem_start = (void *)(info->mem_addr & 0x1FFFFFFF);
            mem_info.priv_mem_size = 0;
            mem_info.priv_mem_start = NULL;
            mem_info.mp_mem_size = info->mp_mem_size;
            mem_info.mp_mem_start = (void *)(info->mp_mem_addr & 0x1FFFFFFF);
            if(copy_to_user((void *)arg, (void *)&mem_info, sizeof(struct ali_video_mem_info)))
                return -EFAULT;
            break;
        }

        case ALIVIDEOIO_ENABLE_DBG_LEVEL:
		{
			UINT32 *rpc_par = (UINT32 *)info->rpc_arg[0];

			if(arg == 2)
			{
				rpc_par[0] = 1;
				rpc_par[1] = 1;
				rpc_par[2] = 0;
				hld_decv_rpc_operation(info, RPC_VIDEO_SET_DBG_FLAG);
			}
			else if(arg == 1)
			{
				info->debug_on = 1;
			}

			break;
		}
		case ALIVIDEOIO_DISABLE_DBG_LEVEL:
		{
			UINT32 *rpc_par = (UINT32 *)info->rpc_arg[0];

			if(arg == 2)
			{
				rpc_par[0] = 1;
				rpc_par[1] = 0;
				rpc_par[2] = 0;
				hld_decv_rpc_operation(info, RPC_VIDEO_SET_DBG_FLAG);
			}
			else if(arg == 1)
			{
				info->debug_on = 0;
			}

			break;
		}
        case ALIVIDEOIO_SET_CTRL_BLK_INFO:
        {
            if(copy_from_user((void *)&info->ctrl_blk, (void *)arg, sizeof(info->ctrl_blk)))
                return -EFAULT;
            info->ctrl_blk_enable = 1;
            break;
        }
        case ALIVIDEOIO_SET_APE_DBG_MODE:
        {
            if(arg)
                ali_video_ape_dbg_on = 1;
            else
                ali_video_ape_dbg_on = 0;

            break;
        }
        case ALIVIDEOIO_GET_APE_DBG_MODE:
        {
            if(!arg)
            break;

            if(copy_to_user((void *)arg, (void *)(&ali_video_ape_dbg_on), sizeof(int)))
                return -EFAULT;
            break;
        }

        default:
            ret = -EINVAL;
            break;
    }

    return ret;
}

static int ali_video_mp_ioctl(struct ali_video_info *info, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    switch(cmd)
    {
        case VDECIO_MP_INITILIZE:
        {
            struct vdec_mp_init_param init_param;

            ret = copy_from_user(&init_param, (void __user *)arg, sizeof(init_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_init(info, &init_param);
            break;
        }

        case VDECIO_MP_RELEASE:
        {
            struct vdec_mp_rls_param rls_param;

            ret = copy_from_user(&rls_param, (void __user *)arg, sizeof(rls_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_rls(info, &rls_param);
            break;
        }

        case VDECIO_MP_FLUSH:
        {
            struct vdec_mp_flush_param flush_param;

            ret = copy_from_user(&flush_param, (void __user *)arg, sizeof(flush_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_flush(info, &flush_param);
            break;
        }

        case VDECIO_MP_EXTRA_DATA:
        {
            struct vdec_mp_extra_data extra_data_param;
            unsigned char *extra_data = NULL;

            ret = copy_from_user(&extra_data_param, (void __user *)arg, sizeof(extra_data_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            extra_data = kmalloc(extra_data_param.extra_data_size, GFP_KERNEL);
            if(extra_data == NULL)
            {
                ret = -EINVAL;
                break;
            }

            ret = copy_from_user(extra_data, (void __user *)(extra_data_param.extra_data), \
                                 extra_data_param.extra_data_size);
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            extra_data_param.extra_data = extra_data;
            ret = hld_decv_mp_extra_data(info, &extra_data_param);
            kfree(extra_data);
            break;
        }

        case VDECIO_MP_GET_STATUS:
        {
            struct vdec_decore_status decore_status;

            memset(&decore_status, 0, sizeof(decore_status));
            ret = hld_decv_mp_get_status(info, &decore_status, NULL);

            ret = copy_to_user((void __user *)arg, &decore_status, sizeof(decore_status));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

        case VDECIO_MP_PAUSE:
        {
            struct vdec_mp_pause_param pause_param;

            ret = copy_from_user(&pause_param, (void __user *)arg, sizeof(pause_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_pause(info, &pause_param);
            break;
        }

        case VDECIO_MP_SET_SBM_IDX:
        {
            struct vdec_mp_sbm_param sbm_param;

            ret = copy_from_user(&sbm_param, (void __user *)arg, sizeof(sbm_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_set_sbm(info, &sbm_param);
            break;
        }

        case VDECIO_MP_SET_SYNC_MODE:
        {
            struct vdec_mp_sync_param sync_param;

            ret = copy_from_user(&sync_param, (void __user *)arg, sizeof(sync_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_set_sync_mode(info, &sync_param);
            break;
        }

        case VDECIO_MP_SET_DISPLAY_RECT:
        {
            struct vdec_display_rect display_rect;

            ret = copy_from_user(&display_rect, (void __user *)arg, sizeof(display_rect));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }
            ret = hld_decv_mp_set_display_rect(info, &display_rect);
            break;
        }

        case VDECIO_MP_SET_QUICK_MODE:
        {
            unsigned long quick_mode = arg;

            ret = hld_decv_mp_set_quick_mode(info, quick_mode);
            break;
        }

        case VDECIO_MP_CAPTURE_FRAME:
        {
            struct vdec_picture picture;

            ret = copy_from_user(&picture, (void __user *)arg, sizeof(picture));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_mp_capture_frame(info, &picture);
            if(ret != 0)
            {
                break;
            }

            ret = copy_to_user((void __user *)arg, &picture, sizeof(picture));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

        case VDECIO_MP_SET_DEC_FRM_TYPE:
        {
            unsigned long type = arg;

            ret = hld_decv_mp_set_dec_frm_type(info, (UINT32)type);
            if(ret != 0)
            {
                ret = -EFAULT;
            }

            break;
        }

        case VDECIO_MP_DYNAMIC_FRAME_ALLOC:
        {
            unsigned long enable = arg;

            ret = hld_decv_mp_ioctl(info, VDEC_DYNAMIC_FRAME_ALLOC, (UINT32)enable);
            if(ret != 0)
            {
                ret = -EFAULT;
            }

            break;
        }

        default:
            ret = video_ioctl_internal(info, cmd, arg);
            break;
    }

    return ret;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long ali_video_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int ali_video_ioctl(struct inode *node,struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    struct ali_video_info *info = (struct ali_video_info *)file->private_data;
    int ret = 0;

    down(&info->sem);

    if(info->open_count <= 0)
    {
        VDEC_PRINTF(info->index, "video is not opened\n");
        goto EXIT;
    }

    if (1)
    {
	    struct vdec_information vdec_stat;
	    struct vdec_decore_status decore_status;
	    unsigned int last_pts;

	    memset(&vdec_stat, 0, sizeof(vdec_stat));
	    memset(&decore_status, 0, sizeof(decore_status));

	    if (info->work_mode)
		    ret = hld_decv_mp_get_status(info, &decore_status, &vdec_stat);
	    else
		    ret = hld_decv_ioctl(info, VDEC_IO_GET_STATUS, (UINT32)&vdec_stat);

	    if (ret == 0)
	    {
		    last_pts = vdec_stat.frame_last_pts;
		    if (last_pts != -1)
			    info->last_pts = last_pts;
	    }
    }

    switch(cmd)
    {
        case VDECIO_START:
        {
            ret = hld_decv_start(info);
            if(ret == 0)
            {
                if(info->status == ALI_VIDEO_IDLE)
                {
                    info->status = ALI_VIDEO_WORKING;
                }
            }
            break;
        }

        case VDECIO_STOP:
        {
            struct vdec_stop_param stop_param;

            ret = copy_from_user(&stop_param, (void __user *)arg, sizeof(stop_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_stop(info, &stop_param);
            if(ret == 0)
            {
                if(info->status == ALI_VIDEO_WORKING || info->status == ALI_VIDEO_PAUSED)
                {
                    info->status = ALI_VIDEO_IDLE;
                }
            }
            break;
        }

        case VDECIO_PAUSE:
            ret = hld_decv_pause(info);
            break;

        case VDECIO_RESUME:
            ret = hld_decv_resume(info);
            break;

        case VDECIO_STEP:
            ret = hld_decv_step(info);
            break;

        case VDECIO_SET_SYNC_MODE:
        {
            struct vdec_sync_param sync_param;

            ret = copy_from_user(&sync_param, (void __user *)arg, sizeof(sync_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_set_sync_mode(info, &sync_param);
            break;
        }

        case VDECIO_SET_PLAY_MODE:
        {
            struct vdec_playback_param playback_param;

            ret = copy_from_user(&playback_param, (void __user *)arg, sizeof(playback_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_set_play_mode(info, &playback_param);
            break;
        }

        case VDECIO_SET_PVR_PARAM:
        {
            struct vdec_pvr_param pvr_param;

            ret = copy_from_user(&pvr_param, (void __user *)arg, sizeof(pvr_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_set_pvr_param(info, &pvr_param);
            break;
        }

        case VDECIO_SELECT_DECODER:
        {
            struct vdec_codec_param codec_param;

            ret = copy_from_user(&codec_param, (void __user *)arg, sizeof(codec_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_select_decoder(info, &codec_param);
            break;
        }

        case VDECIO_GET_CUR_DECODER:
        {
            enum video_decoder_type type = MPEG2_DECODER;

            if (info->work_mode)
                type = info->vdec_type;
            else
                type = hld_decv_get_cur_decoder(info);

            ret = copy_to_user((void __user *)arg, &type, sizeof(type));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

        case VDECIO_SET_OUTPUT_MODE:
        {
            struct vdec_output_param output_param;

            ret = copy_from_user(&output_param, (void __user *)arg, sizeof(output_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_set_output(info, &output_param);
            if(ret != 0)
            {
                break;
            }

            ret = copy_to_user((void __user *)arg, &output_param, sizeof(output_param));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

        case VDECIO_GET_STATUS:
        {
            struct vdec_information vdec_stat;
            struct vdec_decore_status decore_status;
            unsigned int last_pts;

            memset(&vdec_stat, 0, sizeof(vdec_stat));
			memset(&decore_status, 0, sizeof(decore_status));

            if (info->work_mode)
                ret = hld_decv_mp_get_status(info, &decore_status, &vdec_stat);
            else
                ret = hld_decv_ioctl(info, VDEC_IO_GET_STATUS, (UINT32)&vdec_stat);

            if(ret != 0)
            {
                break;
            }

	    last_pts = vdec_stat.frame_last_pts;
	    if (last_pts == -1)
		    vdec_stat.frame_last_pts = info->last_pts;

            ret = copy_to_user((void __user *)arg, &vdec_stat, sizeof(vdec_stat));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

        case VDECIO_FIRST_I_FREERUN:
        {
            int first_i_freerun = (int)arg;

            ret = hld_decv_ioctl(info, VDEC_IO_FIRST_I_FREERUN, first_i_freerun);
            VDEC_PRINTF(info->index, "video set first i free run %d\n", first_i_freerun);
            break;
        }

        case VDECIO_SET_SYNC_DELAY:
        {
            unsigned long sync_delay = arg;

            ret = hld_decv_ioctl(info, VDEC_IO_SET_SYNC_DELAY, sync_delay);
            VDEC_PRINTF(info->index, "video set sync delay %lu\n", sync_delay);
            break;
        }

        case VDECIO_CONTINUE_ON_ERROR:
        {
            unsigned long continue_on_err = arg;

            ret = hld_decv_ioctl(info, VDEC_IO_CONTINUE_ON_ERROR, continue_on_err);
            VDEC_PRINTF(info->index, "video set continue on error %lu\n", continue_on_err);
            break;
        }

        case VDECIO_SET_OUTPUT_RECT:
        {
            struct vdec_display_rect display_rect;
            struct vdec_pipinfo pip_info;

            memset(&pip_info, 0, sizeof(pip_info));

            ret = copy_from_user(&display_rect, (void __user *)arg, sizeof(display_rect));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            pip_info.adv_setting.switch_mode = 1;
            pip_info.src_rect.u_start_x = display_rect.src_x;
            pip_info.src_rect.u_start_y = display_rect.src_y;
            pip_info.src_rect.u_width   = display_rect.src_w;
            pip_info.src_rect.u_height  = display_rect.src_h;
            pip_info.dst_rect.u_start_x = display_rect.dst_x;
            pip_info.dst_rect.u_start_y = display_rect.dst_y;
            pip_info.dst_rect.u_width   = display_rect.dst_w;
            pip_info.dst_rect.u_height  = display_rect.dst_h;
            ret = hld_decv_ioctl(info, VDEC_IO_SET_OUTPUT_RECT, (unsigned long)&pip_info);

            VDEC_PRINTF(info->index, "video set output rect <%ld %ld %ld %ld> => <%ld %ld %ld %ld>\n",
                display_rect.src_x, display_rect.src_y, display_rect.src_w, display_rect.src_h,
                display_rect.dst_x, display_rect.dst_y, display_rect.dst_w, display_rect.dst_h);
            break;
        }

        case VDECIO_CAPTURE_DISPLAYING_FRAME:
        {
            struct vdec_picture picture;

            ret = copy_from_user(&picture, (void __user *)arg, sizeof(picture));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            if (info->work_mode)
                ret = hld_decv_mp_capture_frame(info, &picture);
            else
                ret = hld_decv_ioctl(info, VDEC_IO_CAPTURE_DISPLAYING_FRAME, (UINT32)&picture);

            if(ret != 0)
            {
                break;
            }

            ret = copy_to_user((void __user *)arg, &picture, sizeof(picture));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

        case VDECIO_FILL_FRAME:
        {
            struct vdec_yuv_color yuv_color;
            struct ycb_cr_color color;

            ret = copy_from_user(&yuv_color, (void __user *)arg, sizeof(yuv_color));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            color.u_y  = yuv_color.y;
            color.u_cb = yuv_color.u;
            color.u_cr = yuv_color.v;
            ret = hld_decv_ioctl(info, VDEC_IO_FILL_FRM, (unsigned long)&color);
            VDEC_PRINTF(info->index, "video fill color <0x%x 0x%x 0x%x>\n", yuv_color.y, yuv_color.u, yuv_color.v);
            break;
        }

        case VDECIO_DRAW_COLOR_BAR:
        {
            unsigned long color_bar_addr = arg;

            ret = hld_decv_ioctl(info, VDEC_IO_COLORBAR, 0);
            VDEC_PRINTF(info->index, "video draw color bar %lu\n", color_bar_addr);
            break;
        }

        case VDECIO_SET_DMA_CHANNEL:
        {
            unsigned char channel_num = (unsigned char)arg;

            ret = hld_decv_ioctl(info, VDEC_SET_DMA_CHANNEL, channel_num);
            VDEC_PRINTF(info->index, "video set dma channel %u\n", channel_num);
            break;
        }

        case VDECIO_DTVCC_PARSING_ENABLE:
        {
            int enable = (int)arg;

            ret = hld_decv_ioctl(info, VDEC_DTVCC_PARSING_ENABLE, enable);
            VDEC_PRINTF(info->index, "video set dtvcc parsing enable %d\n", enable);
            break;
        }

        case VDECIO_SAR_ENABLE:
        {
            int enable = (int)arg;

            ret = hld_decv_ioctl(info, VDEC_IO_SAR_ENABLE, enable);
            VDEC_PRINTF(info->index, "video set sar enable %d\n", enable);
            break;
        }

        case VDECIO_SET_DEC_FRM_TYPE:
        {
            unsigned long type = arg;

            ret = hld_decv_ioctl(info, VDEC_IO_SET_DEC_FRM_TYPE, type);
            VDEC_PRINTF(info->index, "video set dec frm type %lu\n", type);
            break;
        }

        case VDECIO_SET_SIMPLE_SYNC:
        {
            int enable = (int)arg;

            ret = hld_decv_ioctl(info, VDEC_IO_SET_SIMPLE_SYNC, enable);
            VDEC_PRINTF(info->index, "video set simple sync %d\n", enable);
            break;
        }

        case VDECIO_SET_TRICK_MODE:
        {
            struct vdec_playback_param playback_param;

            ret = copy_from_user(&playback_param, (void __user *)arg, sizeof(playback_param));
            if(ret != 0)
            {
                ret = -EFAULT;
                break;
            }

            ret = hld_decv_ioctl(info, VDEC_IO_SET_TRICK_MODE, (UINT32)&playback_param);
            VDEC_PRINTF(info->index, "video set trick mode %d %d\n", playback_param.direction, playback_param.rate);
            break;
        }
		case VDECIO_SET_SYNC_REPEAT_INTERVAL:
        {
            int repeat_interval = (int)arg;

            ret = hld_decv_ioctl(info, VDEC_IO_SET_SYNC_REPEAT_INTERVAL, repeat_interval);
            VDEC_PRINTF(info->index, "video set sync repeat interval %d\n", repeat_interval);
            break;
        }
        case VDECIO_VBV_BUFFER_OVERFLOW_RESET:
        {
            int reset = (int)arg;

            ret = hld_decv_ioctl(info, VDEC_VBV_BUFFER_OVERFLOW_RESET, reset);
            VDEC_PRINTF(info->index, "video set vbv overflow reset %d\n", reset);
            break;
        }

        case VDECIO_GET_MEM_INFO:
        {
            struct ali_video_mem_info mem_info;

            mem_info.mem_size  = info->mem_size;
            mem_info.mem_start = (void *)(info->mem_addr & 0x1FFFFFFF);
            mem_info.priv_mem_size  = info->priv_mem_size;
            mem_info.priv_mem_start = (void *)(info->priv_mem_addr & 0x1FFFFFFF);
            mem_info.mp_mem_size  = info->mp_mem_size;
            mem_info.mp_mem_start = (void *)(info->mp_mem_addr & 0x1FFFFFFF);
            mem_info.vbv_mem_size  = info->vbv_mem_size;
            mem_info.vbv_mem_start = (void *)(info->vbv_mem_addr & 0x1FFFFFFF);

            ret = copy_to_user((void __user *)arg, (void *)&mem_info, sizeof(mem_info));
            if(ret != 0)
            {
                ret = -EFAULT;
            }
            break;
        }

		case VDECIO_GET_KUMSGQ:
		{
			int flags = -1;
			if(copy_from_user(&flags, (int *)arg, sizeof(int)))
			{
				printk("Err: copy_from_user\n");
				ret = -EFAULT;
				break;
			}
			ret = ali_kumsgq_newfd(info->video_kumsgq, flags);
            break;
		}

        case VDECIO_REG_CALLBACK:
        {
            struct vdec_io_reg_callback_para cb_param;
            memset(&cb_param, 0, sizeof(cb_param));
            cb_param.e_cbtype = (unsigned long)arg;
            switch(cb_param.e_cbtype)
            {
                case VDEC_CB_FIRST_SHOWED:
                    cb_param.p_cb = info->call_back.pcb_first_showed;
                    break;
                case VDEC_CB_FIRST_HEAD_PARSED:
                    cb_param.p_cb = info->call_back.pcb_first_head_parsed;
                    break;
                case VDEC_CB_FIRST_I_DECODED:
                    cb_param.p_cb = info->call_back.pcb_first_i_decoded;
                    break;
                case VDEC_CB_MONITOR_USER_DATA_PARSED:
                    cb_param.p_cb = info->call_back.pcb_vdec_user_data_parsed;
                    break;
                case VDEC_CB_INFO_CHANGE:
                    cb_param.p_cb = info->call_back.pcb_vdec_info_changed;
                    break;
                case VDEC_CB_ERROR:
                    cb_param.p_cb = info->call_back.pcb_vdec_error;
                    break;
                case VDEC_CB_STATE_CHANGED:
                    cb_param.p_cb = info->call_back.pcb_vdec_state_changed;
                    break;
                case VDEC_CB_MONITOR_VDEC_START:
                    cb_param.p_cb = info->call_back.pcb_vdec_start;
                    break;
                case VDEC_CB_MONITOR_VDEC_STOP:
                    cb_param.p_cb = info->call_back.pcb_vdec_stop;
                    break;
                case VDEC_CB_MONITOR_FRAME_VBV:
                    cb_param.p_cb = info->call_back.pcb_vdec_new_frame;
                    break;
				case VDEC_CB_BACKWARD_RESTART_GOP:
					cb_param.p_cb = info->call_back.pcb_backward_restart_gop;
					break;
				case VDEC_CB_FRAME_DISPLAYED:
					cb_param.p_cb = info->call_back.pcb_frame_displayed;
					break;
				case VDEC_CB_MONITOR_GOP:
					cb_param.p_cb = info->call_back.pcb_vdec_monitor_gop;
					break;
                case VDEC_CB_MODE_SWITCH_OK:
                    cb_param.p_cb = info->call_back.pcb_mode_switch_ok;
                    break;
                default:
                    cb_param.p_cb = NULL;
                    break;
            }
			if (info->work_mode)
				ret = hld_decv_mp_ioctl(info, VDEC_CMD_REG_CB, (UINT32)(&cb_param));
			else
            	ret = hld_decv_ioctl(info, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
            VDEC_PRINTF(info->index, "video register callback %lu\n", arg);
            break;
        }

        case VDECIO_UNREG_CALLBACK:
        {
            struct vdec_io_reg_callback_para cb_param;
            memset(&cb_param, 0, sizeof(cb_param));
            cb_param.e_cbtype = (unsigned long)arg;
            cb_param.p_cb = NULL;
			if (info->work_mode)
				ret = hld_decv_mp_ioctl(info, VDEC_CMD_REG_CB, (UINT32)(&cb_param));
			else
            	ret = hld_decv_ioctl(info, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
            VDEC_PRINTF(info->index, "video unregister callback %lu\n", arg);
            break;
        }

		case VDECIO_DYNAMIC_FRAME_ALLOC:
		{
			if (info->work_mode)
                ret = hld_decv_mp_ioctl(info, VDEC_DYNAMIC_FRAME_ALLOC, arg);
			else
			    ret = hld_decv_ioctl(info, VDEC_IO_SEAMLESS_SWITCH_ENABLE, arg);
			break;
		}

		case VDECIO_SET_DISPLAY_MODE:
		{
            struct vdec_display_param display_param;

            ret = copy_from_user(&display_param, (void __user *)arg, sizeof(display_param));
            if (ret != 0) {
                ret = -EFAULT;
                break;
            }

			ret = hld_decv_ioctl(info, VDEC_IO_SET_DISPLAY_MODE, (UINT32)&display_param);
			break;
		}

		case VDECIO_RESTART:
            ret = hld_decv_ioctl(info, VDEC_IO_RESTART_DECODE, (int)arg);
            VDEC_PRINTF(info->index, "video restart %d\n", (int)arg);
			break;

		case VDECIO_SET_PIP_PARAM:
        {
            struct vdec_pip_param pip_param;

            ret = copy_from_user(&pip_param, (void __user *)arg, sizeof(pip_param));
            if (ret != 0) {
                ret = -EFAULT;
                break;
            }

            if (info->work_mode)
                ret = hld_decv_mp_ioctl(info, VDEC_SET_PIP_PARAM, (UINT32)&pip_param);
            else
			    ret = hld_decv_ioctl(info, VDEC_IO_SET_PIP_PARAM, (UINT32)&pip_param);

            VDEC_PRINTF(info->index, "video set pip param %lu\n", pip_param.layer);
            break;
        }

        case VDECIO_PARSE_AFD:
            ret = hld_decv_ioctl(info, VDEC_IO_PARSE_AFD, (int)arg);
            VDEC_PRINTF(info->index, "video enable parsing AFD %d\n", (int)arg);
            break;

        case VDECIO_FLUSH:
            ret = hld_decv_ioctl(info, VDEC_IO_FLUSH, (int)arg);
            VDEC_PRINTF(info->index, "video flush %d\n", (int)arg);
            break;

        case VDECIO_GET_ALL_USER_DATA:
            ret = hld_decv_ioctl(info, VDEC_IO_GET_ALL_USER_DATA, (int)arg);
            break;

        case VDECIO_GET_USER_DATA_INFO:
            ret = hld_decv_ioctl(info, VDEC_IO_GET_USER_DATA_INFO, (int)arg);
            break;

        default:
            ret = ali_video_mp_ioctl(info, cmd, arg);
            break;
    }
EXIT:
    up(&info->sem);

    return ret;
}

int ali_video_suspend(struct device *dev, pm_message_t state)
{
    struct ali_video_info *info = dev_get_drvdata(dev);

    if(info->status == ALI_VIDEO_WORKING) {
        hld_decv_rpc_suspend(info);
    }

    VDEC_PRINTF(info->index, "video %u suspend, status %u mode %d\n", info->vdec_type, info->status, info->work_mode);

    return 0;
}

int ali_video_resume(struct device *dev)
{
    struct ali_video_info *info = dev_get_drvdata(dev);

    if(info->status == ALI_VIDEO_WORKING) {
        hld_decv_rpc_resume(info);
    }

    VDEC_PRINTF(info->index, "video %u resume, status %u mode %d\n", info->vdec_type, info->status, info->work_mode);

    return 0;
}

static int pts_update_thread(void *data)
{
	struct ali_video_info *info = (struct ali_video_info *)data;

	struct vdec_information vdec_stat;
	struct vdec_decore_status decore_status;
	unsigned int last_pts;
	int ret;

	while (!kthread_should_stop())
	{
		msleep_interruptible(200);

		memset(&vdec_stat, 0, sizeof(vdec_stat));
		memset(&decore_status, 0, sizeof(decore_status));

		if (info->work_mode)
			ret = hld_decv_mp_get_status(info, &decore_status, &vdec_stat);
		else
			ret = hld_decv_ioctl(info, VDEC_IO_GET_STATUS, (UINT32)&vdec_stat);

		if (ret == 0)
		{
			last_pts = vdec_stat.frame_last_pts;
			if (last_pts != -1)
				info->last_pts = last_pts;
		}
	}

	return 0;
}

static int ali_video_open(struct inode *inode, struct file *file)
{
    struct ali_video_info *info;
    int i = 0;

    /* Get the per-device structure that contains this cdev */
    info = container_of(inode->i_cdev, struct ali_video_info, cdev);

    down(&info->sem);

    if (info->mem_addr == 0 || info->mem_size == 0)
        return -1;

    /* Easy access to sbm_devp from rest of the entry points */
    file->private_data = info;

    if(info->open_count == 0) {
	info->pts_task = kthread_create(pts_update_thread, (void *)info, "pts_update_task");
	if(IS_ERR(info->pts_task)){
                VDEC_PRINTF(info->index, "create pts_update_task fail\n");
		info->pts_task = NULL;
		return PTR_ERR(info->pts_task);
	}

	wake_up_process(info->pts_task);

        /* Initialize some fields */
        for(i = 0; i < MAX_VIDEO_RPC_ARG_NUM; i++) {
            info->rpc_arg[i] = kmalloc(MAX_VIDEO_RPC_ARG_SIZE, GFP_KERNEL);
            if(info->rpc_arg[i] == NULL) {
                up(&info->sem);
                VDEC_PRINTF(info->index, "video malloc rpc arg buf fail\n");
                return -1;
            }
        }

        info->video_kumsgq = ali_new_kumsgq();
    	if (!info->video_kumsgq)
    	{
			up(&info->sem);
    		return -EFAULT;
        }

        //vdec_open(info->cur_dev);
        info->status = ALI_VIDEO_IDLE;
    }

    info->open_count++;

    up(&info->sem);

    return 0;
}

static int ali_video_release(struct inode *inode, struct file *file)
{
    struct ali_video_info *info = (struct ali_video_info *)file->private_data;
    int i = 0;

    down(&info->sem);

    info->open_count--;

    if(info->open_count == 0) {
	kthread_stop(info->pts_task);
	info->pts_task = NULL;
        /* Release some fields */
        for(i = 0;i < MAX_VIDEO_RPC_ARG_NUM;i++) {
            if(info->rpc_arg[i] != NULL) {
                kfree((void*)info->rpc_arg[i]);
                info->rpc_arg[i] = NULL;
            }
        }

    	ali_destroy_kumsgq((struct kumsgq *)info->video_kumsgq);
	    info->video_kumsgq = NULL;

        //vdec_close(info->cur_dev);
    }

    up(&info->sem);

    return 0;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations ali_video_fops = {
    .owner    =   THIS_MODULE,           /* Owner */
    .open     =   ali_video_open,        /* Open method */
    .release  =   ali_video_release,     /* Release method */
    .read     =   ali_video_read,        /* Read method */
    .write    =   ali_video_write,       /* Write method */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	.unlocked_ioctl = ali_video_ioctl,
#else
	.ioctl = ali_video_ioctl,
#endif
};

static dev_t ali_video_dev_t;            /* Allotted device number */
static struct class *ali_video_class;    /* Tie with the device model */
static struct device *ali_video_device;

static int ali_video_probe(struct platform_device *pdev)
{
    int ret = 0;
    int i = 0;

	ret = of_get_major_minor(pdev->dev.of_node,&ali_video_dev_t,
			0, DEVICE_NUM, DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto fail;
	}

    /* Populate sysfs entries */
    ali_video_class = class_create(THIS_MODULE, DEVICE_NAME);
    if(ali_video_class == NULL){
        pr_err("Video create class fail\n");
        goto fail;
    }
    ali_video_class->suspend = ali_video_suspend;
    ali_video_class->resume = ali_video_resume;

    for (i = 0; i < DEVICE_NUM; i++) {
        /* Allocate memory for the per-device structure */
        ali_video_priv[i] = kmalloc(sizeof(struct ali_video_info), GFP_KERNEL);
        if (!ali_video_priv[i]) {
            pr_err("Bad Kmalloc\n");
            goto fail;
        }
        memset(ali_video_priv[i], 0, sizeof(struct ali_video_info));

        /* init current device to MPEG2 decoder */
        if (i == 0) {
            ali_video_priv[i]->cur_dev = (struct vdec_device *)hld_dev_get_by_name("DECV_S3601_0");
            ali_video_priv[i]->mem_addr = __G_ALI_MM_VIDEO_START_ADDR;
            ali_video_priv[i]->mem_size = __G_ALI_MM_VIDEO_SIZE;
            ali_video_priv[i]->priv_mem_addr = __G_ALI_MM_VDEC_HW_BUF_START_ADDR;
            ali_video_priv[i]->priv_mem_size = __G_ALI_MM_VDEC_HW_BUF_SIZE;
            ali_video_priv[i]->vbv_mem_addr = __G_ALI_MM_VDEC_VBV_START_ADDR;
            ali_video_priv[i]->vbv_mem_size = __G_ALI_MM_VDEC_VBV_SIZE;
            ali_video_priv[i]->mp_mem_addr = __G_ALI_MM_APE_MEM_START_ADDR;
            ali_video_priv[i]->mp_mem_size = __G_ALI_MM_APE_MEM_SIZE;
            ali_video_priv[i]->capture_mem_addr = __G_ALI_MM_STILL_FRAME_START_ADDR;
            ali_video_priv[i]->capture_mem_size = __G_ALI_MM_STILL_FRAME_SIZE;
            ali_video_priv[i]->tmp_mem_addr = __G_ALI_MM_APE_MEM_START_ADDR;//kmalloc(TMP_BUF_SIZE, GFP_KERNEL);
            ali_video_priv[i]->tmp_mem_size = __G_ALI_MM_APE_MEM_SIZE;
        } else {
            ali_video_priv[i]->cur_dev = (struct vdec_device *)hld_dev_get_by_name("DECV_S3601_1");
            ali_video_priv[i]->mem_addr = __G_ALI_MM_VIDEO_PIP_START_ADDR;
            ali_video_priv[i]->mem_size = __G_ALI_MM_VIDEO_PIP_SIZE;
            ali_video_priv[i]->priv_mem_addr = __G_ALI_MM_VDEC_PIP_HW_BUF_START_ADDR;
            ali_video_priv[i]->priv_mem_size = __G_ALI_MM_VDEC_PIP_HW_BUF_SIZE;
            ali_video_priv[i]->vbv_mem_addr = __G_ALI_MM_VDEC_PIP_VBV_START_ADDR;
            ali_video_priv[i]->vbv_mem_size = __G_ALI_MM_VDEC_PIP_VBV_SIZE;
            ali_video_priv[i]->mp_mem_addr = __G_ALI_MM_APE_PIP_MEM_START_ADDR;
            ali_video_priv[i]->mp_mem_size = __G_ALI_MM_APE_PIP_MEM_SIZE;
            ali_video_priv[i]->capture_mem_addr = __G_ALI_MM_STILL_FRAME_START_ADDR;
            ali_video_priv[i]->capture_mem_size = __G_ALI_MM_STILL_FRAME_SIZE;
            ali_video_priv[i]->tmp_mem_addr = __G_ALI_MM_APE_PIP_MEM_START_ADDR;//kmalloc(TMP_BUF_SIZE, GFP_KERNEL);
            ali_video_priv[i]->tmp_mem_size = __G_ALI_MM_APE_PIP_MEM_SIZE;
        }

        pr_info("video[%d] memory: fb<0x%lx 0x%lx> vbv<0x%lx 0x%lx> priv<0x%lx 0x%lx> mp<0x%lx 0x%lx>\n", i,
            ali_video_priv[i]->mem_addr, ali_video_priv[i]->mem_size,
            ali_video_priv[i]->vbv_mem_addr, ali_video_priv[i]->vbv_mem_size,
            ali_video_priv[i]->priv_mem_addr, ali_video_priv[i]->priv_mem_size,
            ali_video_priv[i]->mp_mem_addr, ali_video_priv[i]->mp_mem_size);

        sprintf(ali_video_priv[i]->name, "ali_video%d", i);

        /* Fill in the video index to correlate this device
           with the corresponding sbm */
        ali_video_priv[i]->index = i;

        /* Connect the file operations with the cdev */
        cdev_init(&ali_video_priv[i]->cdev, &ali_video_fops);
        ali_video_priv[i]->cdev.owner = THIS_MODULE;
        kobject_set_name(&(ali_video_priv[i]->cdev.kobj), "%s", "ali_video");

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&ali_video_priv[i]->cdev, (ali_video_dev_t + i), 1);
        if (ret) {
            pr_err("Video bad cdev\n");
            goto fail;
        }

    	ali_video_device = device_create(ali_video_class, NULL, MKDEV(MAJOR(ali_video_dev_t), i),
                                         ali_video_priv[i], "ali_video%d", i);
    	if(ali_video_device == NULL){
    		pr_err("Video create device fail\n");
    		goto fail;
    	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36))
	    init_MUTEX(&ali_video_priv[i]->sem);
#else
	    sema_init(&ali_video_priv[i]->sem, 1);
#endif

        hld_decv_rpc_init(ali_video_priv[i]);
        ali_video_procfs_init(i, ali_video_priv[i]);
    }

    return 0;

fail:
    if(ali_video_dev_t != 0)
        unregister_chrdev_region(ali_video_dev_t, DEVICE_NUM);
    if(ali_video_device != NULL)
        device_del(ali_video_device);
    if(ali_video_class != NULL)
        class_destroy(ali_video_class);

    for (i = 0; i < DEVICE_NUM; i++) {
        if(ali_video_priv[i] != NULL)
            kfree(ali_video_priv[i]);
    }

    pr_err("video init fail\n");
    return -1;
}

/* Driver Exit */
static int ali_video_remove(struct platform_device *pdev)
{
    int i = 0;

    ali_video_procfs_exit();

    /* Release the major number */
    unregister_chrdev_region(ali_video_dev_t, DEVICE_NUM);

    for (i = 0; i < DEVICE_NUM; i++) {
        device_destroy(ali_video_class, MKDEV(MAJOR(ali_video_dev_t), i));
        /* Remove the cdev */
        cdev_del(&ali_video_priv[i]->cdev);
        kfree(ali_video_priv[i]);
        ali_video_priv[i] = NULL;
    }

    /* Destroy ali_video_class */
    class_destroy(ali_video_class);

    ali_video_dev_t = 0;
    ali_video_device = NULL;
    ali_video_class = NULL;

    return 0;
}

static const struct of_device_id ali_video_of_match[] = {
       { .compatible = "alitech, video", },
       {},
};
MODULE_DEVICE_TABLE(of, ali_video_of_match);

static struct platform_driver ali_video_driver = {
	.driver		= {
		.name	= "ali_video",
		.owner	= THIS_MODULE,
		.of_match_table = ali_video_of_match,
	},
	.probe		= ali_video_probe,
	.remove		= ali_video_remove,
};

module_platform_driver(ali_video_driver);
MODULE_AUTHOR("ALi (Shanghai) Corporation");
MODULE_DESCRIPTION("ali video player driver");
MODULE_LICENSE("GPL");
