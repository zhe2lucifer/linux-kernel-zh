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

#include "ali_sbm.h"

#define PROC_DIR         "alisbm"
#define PROC_DEBUG_FILE  "sbm"

static struct proc_dir_entry *ali_sbm_dir = NULL;
static struct proc_dir_entry *ali_sbm_debug_file[SBM_NUM] = {NULL};

static ssize_t ali_sbm_debug_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos);
static int ali_sbm_debug_open(struct inode *inode, struct file *file);

static const struct file_operations ali_sbm_debug_fops = {
    .owner   = THIS_MODULE,
    .open    = ali_sbm_debug_open,
	.read    = seq_read,
	.write   = ali_sbm_debug_write,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static ssize_t ali_sbm_debug_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
//    struct seq_file *sf = file->private_data;
//    struct sbm_dev *sbm_priv = (struct sbm_dev *)sf->private;
    
	return count;
}

static int ali_sbm_debug_show(struct seq_file *sf, void *unused)
{
    struct sbm_dev *sbm_priv = (struct sbm_dev *)sf->private;
    struct sbm_desc *sbm_ctx = NULL;
    struct sbm_desc_pkt *sbm_ctx_pkt = NULL;
    struct sbm_config *sbm_cfg = NULL;
    struct sbm_rw_desc *sbm_rw = NULL;
    struct sbm_rw_desc_pkt *sbm_rw_pkt = NULL;
    int sbm_idx = sbm_priv->sbm_number;

    if((sbm_idx < 0) || (sbm_idx >= SBM_NUM)) {
        return 0;
    }

    seq_printf(sf, "\n*****************************\n");
    seq_printf(sf, "sbm%u info\n", sbm_idx);

    seq_printf(sf, "open count: %u\n", sbm_priv->open_count);
    seq_printf(sf, "is full: %u\n", sbm_priv->is_full);
    
    if(sbm_info[sbm_idx]) {
        sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
        sbm_cfg = &sbm_ctx->sbm_cfg;
        sbm_rw  = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
        
        seq_printf(sf, "buffer addr: 0x%x\n", sbm_cfg->buffer_addr);
        seq_printf(sf, "buffer size: %u\n", sbm_cfg->buffer_size);
        seq_printf(sf, "block size: %u\n", sbm_cfg->block_size);
        seq_printf(sf, "reserve size: %u\n", sbm_cfg->reserve_size);
        seq_printf(sf, "wrap mode: %u\n", sbm_cfg->wrap_mode);
        seq_printf(sf, "lock mode: %u\n", sbm_cfg->lock_mode);

        seq_printf(sf, "valid size: %u\n", sbm_rw->valid_size);
        seq_printf(sf, "read pos: %u\n", sbm_rw->read_pos);
        seq_printf(sf, "write pos: %u\n", sbm_rw->write_pos);

        seq_printf(sf, "status: 0x%x\n", sbm_ctx->status);
        seq_printf(sf, "mutex: %d\n", sbm_ctx->mutex);
    } else if (sbm_info_pkt[sbm_idx]) {
        sbm_ctx_pkt = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
	    sbm_cfg = &sbm_ctx_pkt->sbm_cfg;
        sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx_pkt->sbm_rw);
        
        seq_printf(sf, "buffer addr: 0x%x\n", sbm_cfg->buffer_addr);
        seq_printf(sf, "buffer size: %u\n", sbm_cfg->buffer_size);
        seq_printf(sf, "block size: %u\n", sbm_cfg->block_size);
        seq_printf(sf, "reserve size: %u\n", sbm_cfg->reserve_size);
        seq_printf(sf, "wrap mode: %u\n", sbm_cfg->wrap_mode);
        seq_printf(sf, "lock mode: %u\n", sbm_cfg->lock_mode);

        seq_printf(sf, "valid size: %u\n", sbm_rw_pkt->valid_size);
        seq_printf(sf, "read pos: 0x%x\n", sbm_rw_pkt->read_pos);
        seq_printf(sf, "write pos: 0x%x\n", sbm_rw_pkt->write_pos);
        seq_printf(sf, "last read pos: 0x%x\n", sbm_rw_pkt->last_read_pos);
        seq_printf(sf, "tmp write pos: 0x%x\n", sbm_rw_pkt->tmp_write_pos);
        seq_printf(sf, "read wrap around: %u\n", sbm_rw_pkt->read_wrap_around);
        seq_printf(sf, "write wrap around: %u\n", sbm_rw_pkt->write_wrap_around);
        seq_printf(sf, "pkt num: %u\n", sbm_rw_pkt->pkt_num);

        seq_printf(sf, "status: 0x%x\n", sbm_ctx_pkt->status);
        seq_printf(sf, "mutex: %d\n", sbm_ctx_pkt->mutex);
    } else {
        seq_printf(sf, "sbm%u has not been created\n", sbm_idx);
    }
    
    seq_printf(sf, "*****************************\n\n");
    
    return 0;
}

static int ali_sbm_debug_open(struct inode *inode, struct file *file)  
{       
    return single_open(file, ali_sbm_debug_show, PDE_DATA(inode));
}

int ali_sbm_procfs_init(int sbm_idx, struct sbm_dev *sbm_priv)
{
    char proc_debug_file[8] = {0};

    if(ali_sbm_dir == NULL) {
        ali_sbm_dir = proc_mkdir(PROC_DIR, NULL);
        if(ali_sbm_dir == NULL) {
            printk("create dir /proc/%s fail\n", PROC_DIR);
            return -1;
        }
    }

    sprintf(proc_debug_file, "sbm%d", sbm_idx);

    ali_sbm_debug_file[sbm_idx] = proc_create_data(proc_debug_file, 0644, ali_sbm_dir, \
                                                   &ali_sbm_debug_fops, sbm_priv);
    if(ali_sbm_debug_file[sbm_idx] == NULL) {
        printk("create file /proc/%s/%s fail\n", PROC_DIR, proc_debug_file);
    }

    return 0;
}

void ali_sbm_procfs_exit(void)
{
    char proc_debug_file[8] = {0};
    int i = 0;

    for(i = 0; i < SBM_NUM; i++) {
        sprintf(proc_debug_file, "sbm%d", i);
        remove_proc_entry(proc_debug_file, ali_sbm_dir);
    }
    
    remove_proc_entry(PROC_DIR, NULL);
    ali_sbm_dir = NULL;
}

