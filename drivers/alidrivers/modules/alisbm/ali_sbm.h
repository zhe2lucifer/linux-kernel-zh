/*
 * Share Buffer Manager driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ALI_SBM_H
#define __ALI_SBM_H

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
#include <linux/delay.h>

#include <ali_reg.h>
#include <ali_shm.h>
#include <ali_cache.h>
#include <ali_sbm_common.h>
#include <ali_soc.h>

#if 1
#define SBM_PRF(fmt, args...) \
	    do \
	    { \
		    if(1) \
		    { \
                printk("[ali sbm]"); \
			    printk(fmt, ##args); \
		    } \
	    }while(0)
#else
#define SBM_PRF(...)					do{}while(0)
#endif

#define SBM_NUM                  12

#define SBM_REQ_OK	             0
#define SBM_REQ_FAIL	         1
#define SBM_REQ_BUSY	         2

#define SBM_CLOSED               0
#define SBM_CPU_READY            1
#define SBM_SEE_READY            2

#define SBM_MUTEX_CREATE         ali_rpc_mutex_create
#define SBM_MUTEX_DELETE         ali_rpc_mutex_delete

#define CACHE_ADDR(addr)         (__CACHE_ADDR_ALI(addr))
#define NONCACHE_ADDR(addr)      (__NONCACHE_ADDR_ALI(addr))

#define SBM_RW_DESC_NUM          6
#define SBM_RW_PKT_DESC_NUM      3

/* Per-device (per-sbm) structure */
struct sbm_dev {
    struct cdev cdev;
    int sbm_number;
    char name[10];
    int status;
    int open_count;
    int is_full;
    struct sbm_config sbm_cfg;
    struct semaphore sem;
};

struct sbm_rw_desc {
    unsigned int valid_size;
	unsigned int read_pos;
	unsigned int write_pos;
};

struct sbm_rw_desc_pkt {
	unsigned int read_pos;
	unsigned int write_pos;
	unsigned int tmp_write_pos;
	unsigned int last_read_pos;
	unsigned char write_wrap_around;
    unsigned char read_wrap_around;
    unsigned int pkt_num;
    unsigned int valid_size;
};

struct sbm_desc {
    struct sbm_config sbm_cfg;
    struct sbm_rw_desc *sbm_rw;
    int mutex;
    unsigned char status;
};

struct sbm_desc_pkt {
    struct sbm_config sbm_cfg;
    struct sbm_rw_desc_pkt *sbm_rw;
    int mutex;
    unsigned char status;
};

int ali_sbm_procfs_init(int sbm_idx, struct sbm_dev *sbm_priv);
void ali_sbm_procfs_exit(void);

extern int sbm_see_create(int sbm_idx, int sbm_mode, void *sbm_init);
extern int sbm_see_destroy(int sbm_idx, int sbm_mode);

extern volatile struct sbm_desc *sbm_info[SBM_NUM];
extern volatile struct sbm_desc_pkt *sbm_info_pkt[SBM_NUM];
#endif

