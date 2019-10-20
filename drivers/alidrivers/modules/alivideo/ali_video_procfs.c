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
 
#include <ali_avsync_common.h>
#include "ali_video.h"

#define PROC_DIR         "alivideo"
#define PROC_DEBUG_FILE  "debug"

static struct proc_dir_entry *ali_video_dir = NULL;
static struct proc_dir_entry *ali_video_debug_file[DEVICE_NUM] = {NULL};

static ssize_t ali_video_debug_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
static int ali_video_debug_open(struct inode *inode, struct file *file);

static const struct file_operations ali_video_debug_fops = {
    .owner   = THIS_MODULE,
    .open    = ali_video_debug_open,
	.read    = seq_read,
	.write   = ali_video_debug_write,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int video_preview(struct ali_video_info *info, int preview)
{
	struct vdec_display_param display_param;
	int ret = 0;

	down(&info->sem);

	display_param.rect.src_x = 0;
	display_param.rect.src_y = 0;
	display_param.rect.src_w = 720;
	display_param.rect.src_h = 2880;

	if (preview) {
		display_param.rect.dst_x = 413;
		display_param.rect.dst_y = 560;
		display_param.rect.dst_w = 176;
		display_param.rect.dst_h = 740;
		display_param.mode = VDEC_PREVIEW;
	} else {
		display_param.rect.dst_x = 0;
		display_param.rect.dst_y = 0;
		display_param.rect.dst_w = 720;
		display_param.rect.dst_h = 2880;
		display_param.mode = VDEC_FULL_VIEW;
	}

	ret = vdec_display_mode(info, &display_param);
	ret = (ret == RET_SUCCESS) ? 0 : -1;

	up(&info->sem);

	return ret;
}

static int video_debug(struct ali_video_info *info, u32 debug_level)
{
    struct vdec_io_dbg_flag_info dbg_info;
    struct vdec_io_reg_callback_para cb_param;

	down(&info->sem);

	switch(debug_level) {
        case 0:
            info->debug_on       = 0;
            dbg_info.dbg_flag    = 1;
            dbg_info.active_flag = 0;
            dbg_info.unique_flag = 0;
            vdec_io_control(info->cur_dev, VDEC_IO_SET_DBG_FLAG, (UINT32)&dbg_info);
            pr_info("disable video debug info\n");
            break;
        case 1:
            info->debug_on        = 1;
            cb_param.monitor_rate = 0;
            cb_param.p_cb = (vdec_cbfunc)1;
            cb_param.e_cbtype = VDEC_CB_INFO_CHANGE;
            vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
            cb_param.e_cbtype = VDEC_CB_ERROR;
            vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
            cb_param.e_cbtype = VDEC_CB_STATE_CHANGED;
            vdec_io_control(info->cur_dev, VDEC_IO_REG_CALLBACK, (UINT32)(&cb_param));
            pr_info("enable video%d kernel debug info\n", info->index);
            break;
        case 2:
            dbg_info.dbg_flag    = 1;
            dbg_info.active_flag = 1;
            dbg_info.unique_flag = 0;
            vdec_io_control(info->cur_dev, VDEC_IO_SET_DBG_FLAG, (UINT32)&dbg_info);
            pr_info("enable video see debug info\n");
            break;
        default:
            break;
    }

	up(&info->sem);

	return 0;
}

 UINT32 last_stc_value = 0, last = 0;
static ssize_t ali_video_debug_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct seq_file *sf = file->private_data;
    struct ali_video_info *info = (struct ali_video_info *)sf->private;
    struct vdec_mp_pause_param pause_param;
    struct vdec_playback_param playback_param;
    struct vdec_information vdec_stat;
    struct vdec_decore_status decore_status;
    struct avsync_device *avsync_dev = NULL;
    struct vpo_device *vpo_dev = NULL;
    UINT32 addr = 0, num = 0, value = 0;
    UINT32 debug_level = 0, sync_mode = 0, step_mode = 0, pause = 0, logo_mode = 0;
    char *temp = NULL, *str = NULL;
    int ret = 0;
    int i = 0, j = 0;

    if(count <= 0) {
	    return 0;
    }

    temp = kmalloc(count, GFP_KERNEL);
    if(temp == NULL) {
        return 0;
    }

	if(copy_from_user(temp, buf, count)) {
        kfree(temp);
	    return -EFAULT;
	}

    if((str = strstr(temp, "rd="))) {
        if(sscanf(str, "rd=0x%lx", &addr) == 1) {
            if((str = strstr(temp, "num="))) {
                if(sscanf(str, "num=%lu", &num) == 1) {
                    pr_info("\n");

                    for(i = 0, j = 0; i < num; i++) {
                        value = __REG32ALI(addr);
                        if(j == 0) {
                            printk("%08lx %08lx ", addr, value);
                        } else if(j < 3) {
                            printk("%08lx ", value);
                        } else if(j == 3) {
                            printk("%08lx\n", value);
                        }

                        addr += 4;

                        j++;
                        if(j > 3) {
                            j = 0;
                        }
                    }

                    pr_info("\n");
                }
            } else {
                value = __REG32ALI(addr);
                pr_info("read register 0x%08lx value 0x%08lx\n", addr, value);
            }
        }
    }

    str = strstr(temp, "wr=");
    if(str) {
        ret = sscanf(str, "wr=0x%lx", &addr);
        if(ret == 1) {
            str = strstr(temp, "val=");
            if(str) {
                ret = sscanf(str, "val=0x%lx", &value);
                if(ret == 1) {
                    __REG32ALI(addr) = value;
                    pr_info("write register 0x%08lx value 0x%08lx\n", addr, value);
                }
            }
        }
    }

    if((str = strstr(temp, "level="))) {
        if(sscanf(str, "level=%lu", &debug_level) == 1) {
			video_debug(info, debug_level);
        }
    }

    if((str = strstr(temp, "pause="))) {
        if(sscanf(str, "pause=%lu", &pause) == 1) {
            down(&info->sem);

            if(info->work_mode) {
                pause_param.pause_decode = 0xFF;
                pause_param.pause_display = pause;
                hld_decv_mp_pause(info, &pause_param);
            } else {
                if(pause) {
                    hld_decv_pause(info);
                } else {
                    hld_decv_resume(info);
                }
            }
            pr_info("set pause %lu\n", pause);

            up(&info->sem);
        }
    }

    if((str = strstr(temp, "step="))) {
        if(sscanf(str, "step=%lu", &step_mode) == 1) {
            down(&info->sem);

            if(info->work_mode) {
                hld_decv_mp_ioctl(info, VDEC_STEP_DISPLAY, step_mode);
            } else {
                if(step_mode) {
                    hld_decv_step(info);
                } else {
                    hld_decv_resume(info);
                }
            }
            pr_info("set step display %lu\n", step_mode);

            up(&info->sem);
        }
    }

#ifdef CONFIG_RPC_HLD_AVSYNC
    if((str = strstr(temp, "sync="))) {
        if(sscanf(str, "sync=%lu", &sync_mode) == 1) {
            avsync_dev = (struct avsync_device*)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_AVSYNC);
            if(avsync_dev) {
                avsync_set_syncmode(avsync_dev, sync_mode);
                pr_info("set avsync mode %lu\n", sync_mode);
            }
        }
    }
#endif

    if((str = strstr(temp, "logo="))) {
        if(sscanf(str, "logo=%lu", &logo_mode) == 1) {
            info->new_write = logo_mode;
            pr_info("set logo mode %lu\n", logo_mode);
        }
    }

    if((str = strstr(temp, "speed="))) {
        if(sscanf(str, "speed=%lu", &value) == 1) {
            playback_param.direction = VDEC_PLAY_FORWARD;
            playback_param.rate = value;
            vdec_io_control(info->cur_dev, VDEC_IO_SET_TRICK_MODE, (UINT32)&playback_param);
            pr_info("set speed %lu\n", value);
        }
    }

    if((str = strstr(temp, "conceal="))) {
        if(sscanf(str, "conceal=%lu", &value) == 1) {
            vdec_io_control(info->cur_dev, VDEC_IO_CONTINUE_ON_ERROR, (value?FALSE:TRUE));
            pr_info("set conceal %lu\n", value);
        }
    }

    if ((str = strstr(temp, "preview="))) {
        if (sscanf(str, "preview=%lu", &value) == 1) {
			video_preview(info, value);
            pr_info("set preview %lu\n", value);
        }
    }

#ifdef CONFIG_RPC_HLD_DIS
    if((str = strstr(temp, "onoff="))) {
        if(sscanf(str, "onoff=%lu", &value) == 1) {
            vpo_dev = (struct vpo_device *)hld_dev_get_by_id(HLD_DEV_TYPE_DIS, 0);
            if(vpo_dev) {
                memset(&decore_status, 0, sizeof(decore_status));
                memset(&vdec_stat, 0, sizeof(vdec_stat));

                if(info->work_mode)
                    ret = hld_decv_mp_get_status(info, &decore_status, &vdec_stat);
                else
                    ret = hld_decv_ioctl(info, VDEC_IO_GET_STATUS, (UINT32)&vdec_stat);

                vpo_win_onoff_ext(vpo_dev, value, (vdec_stat.layer==VPO_PIPWIN)?VPO_LAYER_AUXP:VPO_LAYER_MAIN);
                pr_info("set vpo layer %u onoff %lu\n", vdec_stat.layer, value);
            }
        }
    }
#endif

    kfree(temp);

	return count;
}

static int ali_video_debug_show(struct seq_file *sf, void *unused)
{
    struct ali_video_info *info = (struct ali_video_info *)sf->private;
    struct vdec_information vdec_stat;
    struct vdec_decore_status decore_status;
    INT32 ret = 0;

    down(&info->sem);

    if(info->work_mode) {
        memset(&decore_status, 0, sizeof(decore_status));
        ret = hld_decv_mp_get_status(info, &decore_status, NULL);
        if(ret == 0) {
            seq_printf(sf, "\nmedia player info\n");
            seq_printf(sf, "decoder type %u\n", info->vdec_type);
            seq_printf(sf, "buffer size %lu\n", decore_status.buffer_size);
            seq_printf(sf, "buffer used %lu\n", decore_status.buffer_used);
            seq_printf(sf, "frame rate %lu\n", decore_status.frame_rate);
            seq_printf(sf, "width %lu\n", decore_status.pic_width);
            seq_printf(sf, "height %lu\n", decore_status.pic_height);
            seq_printf(sf, "sar width %lu\n", decore_status.sar_width);
            seq_printf(sf, "sar height %lu\n", decore_status.sar_height);
            seq_printf(sf, "interlaced %ld\n", decore_status.interlaced_frame);
            seq_printf(sf, "frames decoded %lu\n", decore_status.frames_decoded);
            seq_printf(sf, "frames displayed %lu\n", decore_status.frames_displayed);
            seq_printf(sf, "frame last pts %llx\n", decore_status.frame_last_pts);
            seq_printf(sf, "first header parsed %ld\n", decore_status.first_header_got);
            seq_printf(sf, "first pic decoded %ld\n", decore_status.first_pic_decoded);
            seq_printf(sf, "first pic showed %ld\n", decore_status.first_pic_showed);
			seq_printf(sf, "display mode %ld\n", decore_status.output_mode);
            seq_printf(sf, "pause decode %d\n", info->pause_decode);
            seq_printf(sf, "pause display %d\n", info->pause_display);
            seq_printf(sf, "layer %u\n\n", decore_status.layer);
        }
    } else {
        memset(&vdec_stat, 0, sizeof(vdec_stat));
        ret = hld_decv_ioctl(info, VDEC_IO_GET_STATUS, (UINT32)&vdec_stat);
        if(ret == 0) {
            seq_printf(sf, "\nlive play info\n");
            seq_printf(sf, "decoder type %u\n", info->vdec_type);
            seq_printf(sf, "buffer size %lu\n", vdec_stat.buffer_size);
            seq_printf(sf, "buffer used %lu\n", vdec_stat.buffer_used);
            seq_printf(sf, "frame rate %lu\n", vdec_stat.frame_rate);
            seq_printf(sf, "width %u\n", vdec_stat.pic_width);
            seq_printf(sf, "height %u\n", vdec_stat.pic_height);
            seq_printf(sf, "sar width %lu\n", vdec_stat.sar_width);
            seq_printf(sf, "sar height %lu\n", vdec_stat.sar_height);
            seq_printf(sf, "interlaced %ld\n", vdec_stat.interlaced_frame);
            seq_printf(sf, "frames decoded %lu\n", vdec_stat.frames_decoded);
            seq_printf(sf, "frames displayed %lu\n", vdec_stat.frames_displayed);
            seq_printf(sf, "frame last pts %llx\n", vdec_stat.frame_last_pts);
            seq_printf(sf, "first header parsed %d\n", vdec_stat.first_header_parsed);
            seq_printf(sf, "first pic decoded %d\n", vdec_stat.first_pic_decoded);
            seq_printf(sf, "first pic showed %d\n", vdec_stat.first_pic_showed);
			seq_printf(sf, "display mode %u\n", vdec_stat.output_mode);
            seq_printf(sf, "layer %u\n\n", vdec_stat.layer);
        }
    }

    up(&info->sem);

    return 0;
}

static int ali_video_debug_open(struct inode *inode, struct file *file)
{
    return single_open(file, ali_video_debug_show, PDE_DATA(inode));
}

int ali_video_procfs_init(int index, struct ali_video_info *pvideo_priv)
{
    char proc_debug_file[8] = {0};

    if (ali_video_dir == NULL) {
        ali_video_dir = proc_mkdir(PROC_DIR, NULL);
        if(ali_video_dir == NULL) {
            pr_info("create dir /proc/%s fail\n", PROC_DIR);
            return -1;
        }
    }

    sprintf(proc_debug_file, "debug%d", index);

    ali_video_debug_file[index] = proc_create_data(proc_debug_file, 0644, ali_video_dir, \
                                            &ali_video_debug_fops, pvideo_priv);
    if(ali_video_debug_file[index] == NULL) {
        remove_proc_entry(PROC_DIR, NULL);
        pr_info("create file /proc/%s/%s fail\n", PROC_DIR, proc_debug_file);
        return -1;
    }

    return 0;
}

void ali_video_procfs_exit(void)
{
    char proc_debug_file[8] = {0};
    int i = 0;

    for(i = 0; i < DEVICE_NUM; i++) {
        sprintf(proc_debug_file, "debug%d", i);
        remove_proc_entry(proc_debug_file, ali_video_dir);
    }

    ali_video_dir = NULL;
}

