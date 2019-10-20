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

/****************************************************************************
 *  File: ali_sbm.c
 *
 *  Description: ali share buffer memory for cpu & see access
 *
 *  History:
 *      Date             Author         Version      Comment
 *      ======           ======          =====       =======
 *  1.  2011.08.03       Dylan.Yang     0.1.000     First version Created
 ****************************************************************************/
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
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/ali_transport.h>
#include <ali_sbm_common.h>
#include <linux/version.h>
#include <rpc_hld/ali_rpc_sbm.h>
#include <linux/ali_rpc.h>
#include <ali_cache.h>
#include <ali_shm.h>
#include "ali_sbm.h"

volatile struct sbm_desc *sbm_info[SBM_NUM];
volatile struct sbm_desc_pkt *sbm_info_pkt[SBM_NUM];

static struct semaphore m_sbm_sem;

#if 1

static unsigned int  desc_mem[SBM_NUM];
static unsigned char desc_mem_stat[SBM_NUM];
static unsigned int  desc_pkt_mem[SBM_NUM];
static unsigned char desc_pkt_mem_stat[SBM_NUM];

static void *share_malloc(int cfg, unsigned int size)
{
   unsigned int *pmem = NULL;
   unsigned char *pstat = NULL;
   int i = 0;

   if(cfg == 0) {
       pmem  = desc_mem;
       pstat = desc_mem_stat;
   } else if(cfg == 1) {
       pmem  = desc_pkt_mem;
       pstat = desc_pkt_mem_stat;
   } else {
       return NULL;
   }

   if(pmem == NULL) {
       return NULL;
   }

   while(i < SBM_NUM) {
       if(pstat[i] == 0) {
           pstat[i] = 1;
           return ((void *)CACHE_ADDR(pmem[i]));
       }

       i++;
   }

   return NULL;
}

static void share_free(int cfg, void *ptr)
{
   unsigned int *pmem = NULL;
   unsigned char *pstat = NULL;
   int i = 0;

   if(cfg == 0) {
       pmem  = desc_mem;
       pstat = desc_mem_stat;
   } else if(cfg == 1) {
       pmem  = desc_pkt_mem;
       pstat = desc_pkt_mem_stat;
   } else {
       return;
   }

   while(i < SBM_NUM) {
       if(pmem[i] == (unsigned int)NONCACHE_ADDR(ptr)) {
           pstat[i] = 0;
           return;
       }

       i++;
   }

   return;
}

#define SHM_MALLOC(cfg, size)	    share_malloc(cfg, size)
#define SHM_FREE(cfg, ptr)			share_free(cfg, ptr);

#else

#define SHM_MALLOC(cfg, size)	    kmalloc(size, GFP_KERNEL)
#define SHM_FREE(ptr)				kfree(ptr)

#endif

int ali_rpc_sbm_lock(int sbm_idx)
{
    struct sbm_config *sbm_cfg = NULL;
    int lock_mode, id;

    if(sbm_info[sbm_idx]) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
        id = sbm_ctx->mutex;
	    sbm_cfg = &sbm_ctx->sbm_cfg;
        lock_mode = sbm_cfg->lock_mode;
    } else if(sbm_info_pkt[sbm_idx]) {
        struct sbm_desc_pkt *sbm_ctx_pkt = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
        id = sbm_ctx_pkt->mutex;
	    sbm_cfg = &sbm_ctx_pkt->sbm_cfg;
        lock_mode = sbm_cfg->lock_mode;
    } else {
        SBM_PRF("sbm lock fail\n");
        return -1;
    }

    if(lock_mode == SBM_MUTEX_LOCK) {
        ali_rpc_mutex_lock(id, ALI_RPC_MAX_TIMEOUT);
    } else {
        ali_rpc_spinlock(id);
    }

    return 0;
}

int ali_rpc_sbm_unlock(int sbm_idx)
{
    struct sbm_config *sbm_cfg = NULL;
    int lock_mode, id;

    if(sbm_info[sbm_idx]) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
        id = sbm_ctx->mutex;
	    sbm_cfg = &sbm_ctx->sbm_cfg;
        lock_mode = sbm_cfg->lock_mode;
    } else if(sbm_info_pkt[sbm_idx]) {
        struct sbm_desc_pkt *sbm_ctx_pkt = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
        id = sbm_ctx_pkt->mutex;
	    sbm_cfg = &sbm_ctx_pkt->sbm_cfg;
        lock_mode = sbm_cfg->lock_mode;
    } else {
        SBM_PRF("sbm unlock fail\n");
        return -1;
    }

    if(lock_mode == SBM_MUTEX_LOCK) {
        ali_rpc_mutex_unlock(id);
    } else {
        ali_rpc_spinunlock(id);
    }

    return 0;
}

int ali_sbm_copy(void *to, void *from, unsigned long n)
{
    int ret = 0;
#ifdef CONFIG_MIPS
    void *kernel_addr = (void *)0x80000000;
#else
    void *kernel_addr = (void *)0xC0000000;
#endif

    if (to >= kernel_addr) {
        if (from >= kernel_addr) {
            memcpy(to, from, n);
        } else {
            ret = copy_from_user(to, from, n);
        }
    } else {
        if (from >= kernel_addr) {
            ret = copy_to_user(to, from, n);
        } else {
            return -1;
        }
    }

    return ret;
}

int ali_rpc_sbm_create(int sbm_idx, struct sbm_config sbm_init)
{
    struct sbm_desc *sbm_ctx = NULL;
    struct sbm_desc_pkt *sbm_ctx_pkt = NULL;
    struct sbm_rw_desc *sbm_rw = NULL;
    struct sbm_rw_desc_pkt *sbm_rw_pkt = NULL;
    struct sbm_config sbm_cfg;
    void *ptr = NULL;
    int ret = 0;

    if(sbm_idx < SBM_NUM){
        if(sbm_init.wrap_mode == SBM_MODE_NORMAL && sbm_info[sbm_idx] == NULL){
            /* alloc sbm desc */
            sbm_ctx = kmalloc(sizeof(struct sbm_desc), GFP_KERNEL);
            if(sbm_ctx == NULL) {
                SBM_PRF("malloc sbm desc fail\n");
                return -1;
            }
            sbm_info[sbm_idx] = sbm_ctx;

            /* alloc sbm rw desc */
            ptr = SHM_MALLOC(0, sizeof(struct sbm_rw_desc));
            if(ptr == NULL) {
                kfree(sbm_ctx);
                sbm_info[sbm_idx] = NULL;
                SBM_PRF("malloc sbm rw desc fail\n");
                return -1;
            }
#ifdef CONFIG_MIPS
            __CACHE_INV_ALI((unsigned long)ptr, sizeof(struct sbm_rw_desc));
#endif
            sbm_rw = (struct sbm_rw_desc *)(NONCACHE_ADDR(ptr));
	        sbm_info[sbm_idx]->sbm_rw = (struct sbm_rw_desc *)(__VMTSALI(sbm_rw));

            /* create sbm see */
            sbm_cfg.buffer_addr = sbm_init.buffer_addr;
            sbm_cfg.buffer_size = sbm_init.buffer_size;
            sbm_cfg.block_size = sbm_init.block_size;
            sbm_cfg.reserve_size = sbm_init.reserve_size;
            sbm_cfg.wrap_mode = sbm_init.wrap_mode;
            sbm_cfg.lock_mode = sbm_init.lock_mode;
			sbm_info[sbm_idx]->sbm_cfg = sbm_cfg;

            sbm_rw->read_pos = 0;
            sbm_rw->write_pos = 0;
            sbm_rw->valid_size = 0;
            sbm_ctx->status = SBM_CPU_READY;
            sbm_ctx->mutex = -1;
            if(sbm_init.lock_mode == SBM_MUTEX_LOCK) {
                sbm_ctx->mutex = SBM_MUTEX_CREATE();
                if(sbm_ctx->mutex <= 0){
                    SBM_PRF("create mutex fail\n");
		            return -1;
                }
            }
            ret = sbm_see_create(sbm_idx, sbm_init.wrap_mode, (void *)sbm_ctx);
            if(ret == 0) {
                sbm_ctx->status |= SBM_SEE_READY;
            } else {
                SBM_PRF("create see sbm fail\n");
                return -1;
            }
        }else if(sbm_init.wrap_mode == SBM_MODE_PACKET && sbm_info_pkt[sbm_idx] == NULL){
            /* alloc sbm desc */
            sbm_ctx_pkt = kmalloc(sizeof(struct sbm_desc_pkt), GFP_KERNEL);
            if(sbm_ctx_pkt == NULL) {
                SBM_PRF("malloc sbm pkt desc fail\n");
                return -1;
            }
            sbm_info_pkt[sbm_idx] = sbm_ctx_pkt;

            /* alloc sbm rw desc */
            ptr = SHM_MALLOC(1, sizeof(struct sbm_rw_desc_pkt));
            if(ptr == NULL) {
                kfree(sbm_ctx_pkt);
                sbm_info_pkt[sbm_idx] = NULL;
                SBM_PRF("malloc sbm rw pkt desc fail\n");
                return -1;
            }
#ifdef CONFIG_MIPS
            __CACHE_INV_ALI((unsigned long)ptr, sizeof(struct sbm_rw_desc_pkt));
#endif
            sbm_rw_pkt = (struct sbm_rw_desc_pkt *)(NONCACHE_ADDR(ptr));
	        sbm_info_pkt[sbm_idx]->sbm_rw = (struct sbm_rw_desc_pkt *)(__VMTSALI(sbm_rw_pkt));

            /* create sbm see */
            sbm_cfg.buffer_addr = sbm_init.buffer_addr;
            sbm_cfg.buffer_size = sbm_init.buffer_size;
            sbm_cfg.block_size = sbm_init.block_size;
            sbm_cfg.reserve_size = sbm_init.reserve_size;
            sbm_cfg.wrap_mode = sbm_init.wrap_mode;
            sbm_cfg.lock_mode = sbm_init.lock_mode;
			sbm_info_pkt[sbm_idx]->sbm_cfg = sbm_cfg;

            sbm_rw_pkt->read_pos = sbm_init.buffer_addr;
            sbm_rw_pkt->write_pos = sbm_init.buffer_addr;
            sbm_rw_pkt->tmp_write_pos = sbm_init.buffer_addr;
            sbm_rw_pkt->last_read_pos = sbm_init.buffer_addr;
            sbm_rw_pkt->read_wrap_around = 0;
            sbm_rw_pkt->write_wrap_around = 0;
            sbm_rw_pkt->valid_size = 0;
            sbm_rw_pkt->pkt_num = 0;
            sbm_ctx_pkt->status = SBM_CPU_READY;
            sbm_ctx_pkt->mutex = -1;
            if(sbm_init.lock_mode == SBM_MUTEX_LOCK) {
                sbm_ctx_pkt->mutex = SBM_MUTEX_CREATE();
                if(sbm_ctx_pkt->mutex <= 0){
                    SBM_PRF("create mutex fail\n");
		            return -1;
                }
            }
            ret = sbm_see_create(sbm_idx, sbm_init.wrap_mode, (void *)sbm_ctx_pkt);
            if(ret == 0) {
                sbm_ctx_pkt->status |= SBM_SEE_READY;
            } else {
                SBM_PRF("create see sbm fail\n");
                return -1;
            }
        }else{
            return 0;
        }
    }

    return 0;
}

int ali_rpc_sbm_destroy(int sbm_idx, int sbm_mode)
{
    struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
    struct sbm_desc_pkt *sbm_ctx_pkt = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
    struct sbm_rw_desc *sbm_rw = NULL;
    struct sbm_rw_desc_pkt *sbm_rw_pkt = NULL;

    if(sbm_idx < SBM_NUM){
        if(sbm_mode == SBM_MODE_NORMAL && sbm_ctx){
            if(sbm_ctx->mutex > 0){
                SBM_MUTEX_DELETE(sbm_ctx->mutex);
            }

            sbm_see_destroy(sbm_idx, sbm_mode);
            sbm_ctx->status = 0;

            sbm_rw = (struct sbm_rw_desc *)CACHE_ADDR(sbm_ctx->sbm_rw);
            SHM_FREE(0, (void *)__VSTMALI(sbm_rw));
            sbm_ctx->sbm_rw = NULL;

            kfree(sbm_ctx);
	        sbm_info[sbm_idx] = NULL;
        }else if(sbm_mode == SBM_MODE_PACKET && sbm_ctx_pkt){
            if(sbm_ctx_pkt->mutex > 0){
                SBM_MUTEX_DELETE(sbm_ctx_pkt->mutex);
            }

            sbm_see_destroy(sbm_idx, sbm_mode);
            sbm_ctx_pkt->status = 0;

            sbm_rw_pkt = (struct sbm_rw_desc_pkt *)CACHE_ADDR(sbm_ctx_pkt->sbm_rw);
            SHM_FREE(1, (void *)__VSTMALI(sbm_rw_pkt));
            sbm_ctx_pkt->sbm_rw = NULL;

            kfree(sbm_ctx_pkt);
	        sbm_info_pkt[sbm_idx] = NULL;
        }
    }

    return 0;
}

int ali_rpc_sbm_request_read(int sbm_idx, void **buf_start, int *buf_size)
{
    struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
    struct sbm_config info_fix;
    struct sbm_rw_desc info_rw;
    int read_size = 0;

    if(sbm_ctx == NULL || sbm_idx < 0 || sbm_idx >= SBM_NUM){
        return SBM_REQ_FAIL;
    }

    ali_rpc_sbm_lock(sbm_idx);

    if(sbm_ctx->status != (SBM_CPU_READY|SBM_SEE_READY)){
		ali_rpc_sbm_unlock(sbm_idx);
		return SBM_REQ_FAIL;
	}

	memcpy(&info_rw,  (void *)__VSTMALI(sbm_ctx->sbm_rw), sizeof(struct sbm_rw_desc));
    memcpy(&info_fix, &sbm_ctx->sbm_cfg, sizeof(struct sbm_config));

    ali_rpc_sbm_unlock(sbm_idx);

    if(info_rw.valid_size){
        if(info_rw.read_pos < info_rw.write_pos)
            read_size = info_rw.write_pos - info_rw.read_pos;
        else
            read_size = info_fix.buffer_size - info_rw.read_pos;

        if(read_size < *buf_size){
            *buf_size = read_size;
        }

        *buf_start = (char *)(info_fix.buffer_addr + info_rw.read_pos);

        return SBM_REQ_OK;
    }

    return SBM_REQ_FAIL;
}

void ali_rpc_sbm_update_read(int sbm_idx, int update_size)
{
	struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
    struct sbm_rw_desc *sbm_rw = NULL;
    struct sbm_config *sbm_fix = NULL;

    if(sbm_ctx == NULL || sbm_idx < 0 || sbm_idx >= SBM_NUM){
        return;
    }

    sbm_fix = &sbm_ctx->sbm_cfg;

    sbm_rw = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
    if(sbm_rw == NULL) {
        return;
    }

	ali_rpc_sbm_lock(sbm_idx);

    if(sbm_ctx->status != (SBM_CPU_READY|SBM_SEE_READY)){
        ali_rpc_sbm_unlock(sbm_idx);
		return;
	}

	sbm_rw->read_pos += update_size;
	while(sbm_rw->read_pos >= sbm_fix->buffer_size){
		sbm_rw->read_pos -= sbm_fix->buffer_size;
	}

	sbm_rw->valid_size -= update_size;

    ali_rpc_sbm_unlock(sbm_idx);
}

int ali_rpc_sbm_request_write(int sbm_idx, void **buf_start, int *buf_size)
{
    unsigned int buffer_end = 0;
    unsigned int write_addr = 0;
    int ret = SBM_REQ_OK, free_size = 0, req_size = *buf_size, write_size = 0;

    if(sbm_idx < 0 || sbm_idx >= SBM_NUM) {
        return SBM_REQ_FAIL;
    }

    ali_rpc_sbm_lock(sbm_idx);

    if(sbm_info[sbm_idx]) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)sbm_info[sbm_idx];
        struct sbm_rw_desc *sbm_rw = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
        struct sbm_config *sbm_fix = &sbm_ctx->sbm_cfg;

        if(sbm_ctx->status != (SBM_CPU_READY|SBM_SEE_READY)) {
            ali_rpc_sbm_unlock(sbm_idx);
    		return SBM_REQ_FAIL;
    	}

        free_size = sbm_fix->buffer_size - sbm_rw->valid_size;
    	if(free_size > sbm_fix->reserve_size ) {
    		if(sbm_rw->write_pos >= sbm_rw->read_pos) {
    			write_size = sbm_fix->buffer_size - sbm_rw->write_pos;
    		} else {
    			write_size = sbm_rw->read_pos - sbm_rw->write_pos - sbm_fix->reserve_size;
    		}

    		if(sbm_fix->block_size && write_size > sbm_fix->block_size) {
    			write_size = sbm_fix->block_size;
    		}
    		if(req_size < write_size) {
    			write_size = req_size;
    		}

    		*buf_start = (char *)(sbm_fix->buffer_addr + sbm_rw->write_pos);
            *buf_size = write_size;
    	} else {
            ret = SBM_REQ_BUSY;
    	}
    } else if (sbm_info_pkt[sbm_idx]) {
        struct sbm_desc_pkt *sbm_ctx = (struct sbm_desc_pkt *)sbm_info_pkt[sbm_idx];
        struct sbm_rw_desc_pkt *sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx->sbm_rw);
        struct sbm_config *sbm_fix = &sbm_ctx->sbm_cfg;

        if(sbm_ctx->status != (SBM_CPU_READY|SBM_SEE_READY)) {
            ali_rpc_sbm_unlock(sbm_idx);
    		return SBM_REQ_FAIL;
    	}

        free_size = sbm_fix->buffer_size - sbm_rw_pkt->valid_size;
    	if(free_size > req_size + sbm_fix->reserve_size) {
    		if(sbm_rw_pkt->write_pos >= sbm_rw_pkt->read_pos) {
                buffer_end = sbm_fix->buffer_addr + sbm_fix->buffer_size;
    			write_size = buffer_end - sbm_rw_pkt->write_pos;

        		if(req_size > write_size) {
                    if (sbm_rw_pkt->read_pos >= sbm_fix->buffer_addr + sbm_fix->reserve_size)
        			    write_size = sbm_rw_pkt->read_pos - sbm_fix->buffer_addr - sbm_fix->reserve_size;
                    else
                        write_size = 0;

          		    if(req_size <= write_size) {
        			    write_size = req_size;
                        write_addr = sbm_fix->buffer_addr;
          		    } else {
                        ret = SBM_REQ_BUSY;
                    }
        		} else {
    			    write_size = req_size;
                    write_addr = sbm_rw_pkt->write_pos;
        		}
    		} else {
    		    if (sbm_rw_pkt->read_pos >= sbm_rw_pkt->write_pos + sbm_fix->reserve_size)
    			    write_size = sbm_rw_pkt->read_pos - sbm_rw_pkt->write_pos - sbm_fix->reserve_size;
                else
                    write_size = 0;

                if(req_size > write_size) {
                    ret = SBM_REQ_BUSY;
                } else {
    			    write_size = req_size;
                    write_addr = sbm_rw_pkt->write_pos;
                }
    		}

            if (ret == SBM_REQ_OK) {
        		*buf_start = (char *)write_addr;
                *buf_size = write_size;
            }
    	} else {
            ret = SBM_REQ_BUSY;
    	}
    } else {
        ret = SBM_REQ_FAIL;
    }

    ali_rpc_sbm_unlock(sbm_idx);

    return ret;
}

int ali_rpc_sbm_update_write(int sbm_idx, int update_size)
{
	struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
    struct sbm_desc_pkt *sbm_ctx_pkt = (struct sbm_desc_pkt *)sbm_info_pkt[sbm_idx];
    struct sbm_rw_desc *sbm_rw = NULL;
    struct sbm_rw_desc_pkt *sbm_rw_pkt = NULL;
    struct sbm_config *sbm_fix = NULL;
    unsigned int buffer_end = 0;

    if(sbm_idx < 0 || sbm_idx >= SBM_NUM){
        return SBM_REQ_FAIL;
    }

    if(sbm_info[sbm_idx]) {
        sbm_fix = &sbm_ctx->sbm_cfg;

        sbm_rw = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
        if(sbm_rw == NULL) {
            return SBM_REQ_FAIL;
        }

        ali_rpc_sbm_lock(sbm_idx);

        if(sbm_ctx->status != (SBM_CPU_READY|SBM_SEE_READY)){
            ali_rpc_sbm_unlock(sbm_idx);
    		return SBM_REQ_FAIL;
    	}

    	sbm_rw->write_pos += update_size;
    	while(sbm_rw->write_pos >= sbm_fix->buffer_size){
    		sbm_rw->write_pos -= sbm_fix->buffer_size;
    	}

    	sbm_rw->valid_size += update_size;

        ali_rpc_sbm_unlock(sbm_idx);
    } else if (sbm_info_pkt[sbm_idx]) {
        sbm_fix = &sbm_ctx_pkt->sbm_cfg;

        sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx_pkt->sbm_rw);
        if(sbm_rw_pkt == NULL) {
            return SBM_REQ_FAIL;
        }

        if(sbm_ctx_pkt->status != (SBM_CPU_READY|SBM_SEE_READY)){
    		return SBM_REQ_FAIL;
    	}

        ali_rpc_sbm_lock(sbm_idx);

        buffer_end = sbm_fix->buffer_addr + sbm_fix->buffer_size;

        if(sbm_rw_pkt->write_pos >= sbm_rw_pkt->read_pos) {
            if (sbm_rw_pkt->write_pos + update_size > buffer_end) {
                if (sbm_fix->buffer_addr + update_size <= sbm_rw_pkt->read_pos) {
                    sbm_rw_pkt->write_pos = sbm_fix->buffer_addr + update_size;
                } else {
                    SBM_PRF("sbm update size %d error\n", update_size);
                    ali_rpc_sbm_unlock(sbm_idx);
                    return SBM_REQ_FAIL;
                }
            } else {
                sbm_rw_pkt->write_pos += update_size;
            	while(sbm_rw_pkt->write_pos >= buffer_end) {
            		sbm_rw_pkt->write_pos -= sbm_fix->buffer_size;
            	}
            }
        } else {
            if (sbm_rw_pkt->write_pos + update_size > sbm_rw_pkt->read_pos) {
                SBM_PRF("sbm update size %d error\n", update_size);
                ali_rpc_sbm_unlock(sbm_idx);
                return SBM_REQ_FAIL;
            } else {
                sbm_rw_pkt->write_pos += update_size;
            	while(sbm_rw_pkt->write_pos >= buffer_end) {
            		sbm_rw_pkt->write_pos -= sbm_fix->buffer_size;
            	}
            }
        }

    	sbm_rw_pkt->valid_size += update_size;
        sbm_rw_pkt->pkt_num++;

        ali_rpc_sbm_unlock(sbm_idx);
    } else {
        return SBM_REQ_FAIL;
    }

    return SBM_REQ_OK;
}

int ali_rpc_sbm_write_pkt(int sbm_idx, const char *buf_start, size_t buf_size)
{
    struct sbm_desc_pkt *sbm_ctx = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
    struct sbm_rw_desc_pkt *sbm_rw = NULL, info_rw;
    struct sbm_config *sbm_fix = NULL;
    unsigned int buffer_end = 0, end_addr = 0;
    int ret = 0;

    if(sbm_ctx == NULL || sbm_idx < 0 || sbm_idx >= SBM_NUM){
        return SBM_REQ_FAIL;
    }

    sbm_fix = &sbm_ctx->sbm_cfg;

    sbm_rw = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx->sbm_rw);
    if(sbm_rw == NULL) {
        return SBM_REQ_FAIL;
    }

    ali_rpc_sbm_lock(sbm_idx);

    if(sbm_ctx->status != (SBM_CPU_READY|SBM_SEE_READY)){
        ali_rpc_sbm_unlock(sbm_idx);
        SBM_PRF("sbm not ready: %d\n",sbm_ctx->status);
		return SBM_REQ_FAIL;
	}

	memcpy(&info_rw, sbm_rw, sizeof(struct sbm_rw_desc_pkt));

    ali_rpc_sbm_unlock(sbm_idx);

    buffer_end = sbm_fix->buffer_addr + sbm_fix->buffer_size;

    if(!info_rw.write_wrap_around){
        end_addr = info_rw.read_wrap_around ? info_rw.last_read_pos : buffer_end;
        if(info_rw.write_pos + buf_size <= end_addr){
           ret = ali_sbm_copy((char *)__VSTMALI(info_rw.write_pos), (void *)buf_start, buf_size);
           if(ret) {
               SBM_PRF("%s line %u copy %p=>%x size %u fail %d\n",
                   __func__, __LINE__, buf_start, info_rw.write_pos, buf_size, ret);
           }
#ifdef CONFIG_MIPS
           __CACHE_FLUSH_ALI(__VSTMALI(info_rw.write_pos), buf_size);
#endif

           ali_rpc_sbm_lock(sbm_idx);
           sbm_rw->write_pos += buf_size;
           ali_rpc_sbm_unlock(sbm_idx);
        }else{
            if(!info_rw.read_wrap_around){
                if(sbm_fix->buffer_addr + buf_size + sbm_fix->reserve_size <= info_rw.last_read_pos){
                    ret = ali_sbm_copy((char *)__VSTMALI(sbm_fix->buffer_addr), (void *)buf_start, buf_size);
                    if(ret) {
                        SBM_PRF("%s line %u copy %p=>%x size %u fail %d\n",
                            __func__, __LINE__, buf_start, sbm_fix->buffer_addr, buf_size, ret);
                    }
#ifdef CONFIG_MIPS
                    __CACHE_FLUSH_ALI(__VSTMALI(sbm_fix->buffer_addr), buf_size);
#endif
                    ali_rpc_sbm_lock(sbm_idx);
                    sbm_rw->tmp_write_pos = sbm_fix->buffer_addr + buf_size;
                    sbm_rw->write_wrap_around = 1;
                    ali_rpc_sbm_unlock(sbm_idx);
                }else{
                    return SBM_REQ_BUSY;
                }
            }else{
                return SBM_REQ_BUSY;
            }
        }
    }else{
        if(info_rw.tmp_write_pos + buf_size + sbm_fix->reserve_size <= info_rw.last_read_pos){
            ret = ali_sbm_copy((void *)__VSTMALI(info_rw.tmp_write_pos), (void *)buf_start, buf_size);
            if(ret) {
                SBM_PRF("%s line %u copy %p=>%x size %u fail %d\n",
                    __func__, __LINE__, buf_start, info_rw.tmp_write_pos, buf_size, ret);
            }
#ifdef CONFIG_MIPS
            __CACHE_FLUSH_ALI(__VSTMALI(info_rw.tmp_write_pos), buf_size);
#endif

            ali_rpc_sbm_lock(sbm_idx);
            if(sbm_rw->write_wrap_around) {
                sbm_rw->tmp_write_pos += buf_size;
            } else {
                sbm_rw->write_pos += buf_size;
            }
            ali_rpc_sbm_unlock(sbm_idx);
        }else{
            return SBM_REQ_BUSY;
        }
    }

    ali_rpc_sbm_lock(sbm_idx);
	sbm_rw->valid_size += buf_size;
    sbm_rw->pkt_num++;
    ali_rpc_sbm_unlock(sbm_idx);

    return SBM_REQ_OK;
}

int ali_rpc_sbm_reset(int sbm_idx, int sbm_mode)
{
    if(sbm_idx < 0 || sbm_idx >= SBM_NUM) {
        return -1;
    }

    if(sbm_mode == SBM_MODE_NORMAL) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
        if(sbm_ctx) {
            struct sbm_rw_desc *sbm_rw = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
            ali_rpc_sbm_lock(sbm_idx);
            sbm_rw->read_pos = 0;
            sbm_rw->write_pos = 0;
            sbm_rw->valid_size = 0;
            ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    } else {
        struct sbm_desc_pkt *sbm_ctx = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
        if(sbm_ctx) {
            struct sbm_rw_desc_pkt *sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx->sbm_rw);
            struct sbm_config *sbm_fix = &sbm_ctx->sbm_cfg;
            ali_rpc_sbm_lock(sbm_idx);
            sbm_rw_pkt->read_pos = sbm_fix->buffer_addr;
            sbm_rw_pkt->write_pos = sbm_fix->buffer_addr;
            sbm_rw_pkt->tmp_write_pos = sbm_fix->buffer_addr;
            sbm_rw_pkt->last_read_pos = sbm_fix->buffer_addr;
            sbm_rw_pkt->read_wrap_around = 0;
            sbm_rw_pkt->write_wrap_around = 0;
            sbm_rw_pkt->valid_size = 0;
            sbm_rw_pkt->pkt_num = 0;
            ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    }

    return 0;
}

int ali_rpc_sbm_show_valid_size(int sbm_idx, int sbm_mode, unsigned int *valid_size)
{
    if(sbm_idx < 0 || sbm_idx >= SBM_NUM) {
        return -1;
    }

    if(sbm_mode == SBM_MODE_NORMAL) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
        if(sbm_ctx) {
            struct sbm_rw_desc *sbm_rw = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
            //ali_rpc_sbm_lock(sbm_idx);
            *valid_size = sbm_rw->valid_size;
            //ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    } else {
        struct sbm_desc_pkt *sbm_ctx = (struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
        if(sbm_ctx) {
            struct sbm_rw_desc_pkt *sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx->sbm_rw);
            //ali_rpc_sbm_lock(sbm_idx);
            *valid_size = (sbm_rw_pkt->pkt_num == 0) ? 0 : sbm_rw_pkt->valid_size;
            //ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    }

    return 0;
}

int ali_rpc_sbm_show_free_size(int sbm_idx, int sbm_mode, unsigned int *free_size)
{
    struct sbm_config *sbm_fix = NULL;

    if(sbm_idx < 0 || sbm_idx >= SBM_NUM) {
        return -1;
    }

    if(sbm_mode == SBM_MODE_NORMAL) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)(sbm_info[sbm_idx]);
        if(sbm_ctx) {
            struct sbm_rw_desc *sbm_rw = (struct sbm_rw_desc *)__VSTMALI(sbm_ctx->sbm_rw);
            sbm_fix = &sbm_ctx->sbm_cfg;
            //ali_rpc_sbm_lock(sbm_idx);
            *free_size = sbm_fix->buffer_size - sbm_rw->valid_size;
            if(*free_size < sbm_fix->reserve_size) {
                *free_size  = 0;
            }else{
                *free_size -=  sbm_fix->reserve_size;
            }
            //ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    } else {
        struct sbm_desc_pkt *sbm_ctx = ( struct sbm_desc_pkt *)(sbm_info_pkt[sbm_idx]);
        if(sbm_ctx) {
            struct sbm_rw_desc_pkt *sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx->sbm_rw);
            sbm_fix = &sbm_ctx->sbm_cfg;
            //ali_rpc_sbm_lock(sbm_idx);
            *free_size = sbm_fix->buffer_size - sbm_rw_pkt->valid_size;
            if(*free_size < sbm_fix->reserve_size) {
                *free_size = 0;
            }else{
                *free_size -=  sbm_fix->reserve_size;
            }
            //ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    }

    return 0;
}

int ali_rpc_sbm_show_pkt_num(int sbm_idx, int sbm_mode, unsigned int *pkt_num)
{
    if(sbm_idx < 0 || sbm_idx >= SBM_NUM) {
        return -1;
    }

    if(sbm_mode == SBM_MODE_NORMAL) {
        struct sbm_desc *sbm_ctx = (struct sbm_desc *)sbm_info[sbm_idx];
        if(sbm_ctx) {
            //ali_rpc_sbm_lock(sbm_idx);
            *pkt_num = 0;
            //ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    } else {
        struct sbm_desc_pkt *sbm_ctx = (struct sbm_desc_pkt *)sbm_info_pkt[sbm_idx];
        if(sbm_ctx) {
            struct sbm_rw_desc_pkt *sbm_rw_pkt = (struct sbm_rw_desc_pkt *)__VSTMALI(sbm_ctx->sbm_rw);
            //ali_rpc_sbm_lock(sbm_idx);
            *pkt_num = sbm_rw_pkt->pkt_num;
            //ali_rpc_sbm_unlock(sbm_idx);
        } else {
            return -1;
        }
    }

    return 0;
}

static int sbm_open(struct inode *inode, struct file *file)
{
    struct sbm_dev *sbm_devp;


	if(down_interruptible(&m_sbm_sem)){
		printk("sbm_open down sem fail\n");
		return -1;
	}

    /* Get the per-device structure that contains this cdev */
    sbm_devp = container_of(inode->i_cdev, struct sbm_dev, cdev);

    /* Easy access to sbm_devp from rest of the entry points */
    file->private_data = sbm_devp;

    if(sbm_devp->open_count == 0){
        /* Initialize some fields */
        sbm_devp->status = 0;
    }

    sbm_devp->open_count++;

    //SBM_PRF("%s : done count %d status %d num %d\n", __FUNCTION__, sbm_devp->open_count , sbm_devp->status, sbm_devp->sbm_number);

    up(&m_sbm_sem);

    return 0;
}

static int sbm_release(struct inode *inode, struct file *file)
{
    struct sbm_dev *sbm_devp = file->private_data;

	if(down_interruptible(&m_sbm_sem)){
		printk("sbm_release down sem fail\n");
		return -1;
	}

    sbm_devp->open_count--;

    if(sbm_devp->open_count == 0) {
        if(sbm_devp->status != 0) {
            ali_rpc_sbm_destroy(sbm_devp->sbm_number, sbm_devp->sbm_cfg.wrap_mode);
            sbm_devp->status = 0;
        }
    } else if(sbm_devp->open_count < 0) {
        SBM_PRF("sbm %d open count fail %d\n", sbm_devp->sbm_number, sbm_devp->open_count);
    }

    //SBM_PRF("%s : sbm %d open count %d\n", __FUNCTION__, sbm_devp->sbm_number, sbm_devp->open_count);

    up(&m_sbm_sem);

    return 0;
}

static ssize_t sbm_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct sbm_dev *sbm_devp = file->private_data;
	int ret = 0, req_size = count, read_size = 0;
    char *req_buf = NULL;

    if(down_interruptible(&m_sbm_sem)) {
       SBM_PRF("%s : ali sbm down sem fail\n", __FUNCTION__);
       return -1;
    }

    if(sbm_devp->open_count <= 0) {
    	up(&m_sbm_sem);
        //SBM_PRF("ali sbm%d is not opened\n", sbm_devp->sbm_number);
    	return -1;
    }

    ret = ali_rpc_sbm_request_read(sbm_devp->sbm_number, (void **)&req_buf, &req_size);
    if(ret == SBM_REQ_OK){
	    req_buf = (char *)__VSTMALI(req_buf);
#ifdef CONFIG_MIPS
        __CACHE_INV_ALI((unsigned long)req_buf, req_size);
#endif
        ret = copy_to_user(buf, req_buf, req_size);
        if(ret) {
            SBM_PRF("%s line %u copy %p=>%p size %d fail %d\n", __func__, __LINE__, req_buf, buf, req_size, ret);
            up(&m_sbm_sem);
            return -EFAULT;
        }
        ali_rpc_sbm_update_read(sbm_devp->sbm_number, req_size);

        /* for buffer wrap around, continue read the left data */
        count -= req_size;
        read_size += req_size;
        if(count){
            buf += req_size;
            req_buf = NULL;
            req_size = count;
            ret = ali_rpc_sbm_request_read(sbm_devp->sbm_number, (void **)&req_buf, &req_size);
            if(ret == SBM_REQ_OK){
		        req_buf = (char *)__VSTMALI(req_buf);
#ifdef CONFIG_MIPS
                __CACHE_INV_ALI((unsigned long)req_buf, req_size);
#endif
                ret = copy_to_user(buf, req_buf, req_size);
                if(ret) {
                    SBM_PRF("%s line %u copy %p=>%p size %u fail %d\n",
                        __func__, __LINE__, req_buf, buf, req_size, ret);
                    up(&m_sbm_sem);
                    return -EFAULT;
                }
                ali_rpc_sbm_update_read(sbm_devp->sbm_number, req_size);
                read_size += req_size;
            }
        }
    }

    up(&m_sbm_sem);
    return read_size;
}

static ssize_t sbm_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct sbm_dev *sbm_devp = file->private_data;
	int ret = 0, req_size = count, write_count = 0;
    char *req_buf = NULL;

   if(down_interruptible(&m_sbm_sem)) {
      SBM_PRF("%s : ali sbm down sem fail\n", __FUNCTION__);
      return -1;
   }

    if((sbm_devp->open_count <= 0) || (sbm_devp->status == 0)) {
    	up(&m_sbm_sem);
        //SBM_PRF("ali sbm%d is not opened\n", sbm_devp->sbm_number);
	    return -1;
    }

    do{
        ret = ali_rpc_sbm_request_write(sbm_devp->sbm_number, (void **)&req_buf, &req_size);
        if(ret == SBM_REQ_OK && req_size > 0){
            ret = ali_sbm_copy((void*)__VSTMALI(req_buf), (void*)buf, req_size);
            if(ret) {
                SBM_PRF("%s line %u copy %p=>%p size %d fail %d\n",
                    __func__, __LINE__, buf, req_buf, req_size, ret);
                up(&m_sbm_sem);
                return -EFAULT;
            }
  #ifdef CONFIG_MIPS
            __CACHE_FLUSH_ALI((unsigned long)__VSTMALI(req_buf), req_size);
  #endif
            ali_rpc_sbm_update_write(sbm_devp->sbm_number, req_size);

            buf += req_size;
            write_count += req_size;
            count -= req_size;
            req_size = count;
            sbm_devp->is_full = 0;

            if (sbm_devp->sbm_cfg.wrap_mode == SBM_MODE_PACKET) {
                if (count != 0) {
                    SBM_PRF("%s line %u write packet error", __FUNCTION__, __LINE__);
                    break;
                }
            }
        }else{
            sbm_devp->is_full = (ret == SBM_REQ_BUSY) ? 1 : 0;
            break;
        }
	}while(count > 0);

    up(&m_sbm_sem);

    return write_count;
}

static int sbm_req_buf_info(struct sbm_dev *sbm_devp,struct sbm_req_buf *info)
{
	int ret;
	ret = ali_rpc_sbm_request_write(sbm_devp->sbm_number, (void **)&info->phy_addr, &info->req_size);
	if(ret != SBM_REQ_OK) {
		info->req_size = 0;
	}

	return info->req_size;

}

static int sbm_update_buf_info(struct sbm_dev *sbm_devp, struct sbm_req_buf *info)
{
	ali_rpc_sbm_update_write(sbm_devp->sbm_number, info->req_size);
	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long sbm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int sbm_ioctl(struct inode *node, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    struct sbm_dev *sbm_devp = file->private_data;
    int ret = 0;
    int res = 0;
	if(down_interruptible(&m_sbm_sem)){
		printk("sbm_ioctl down sem fail\n");
		return -1;
	}
    if(sbm_devp->open_count <= 0) {
    	up(&m_sbm_sem);
        //SBM_PRF("ali sbm%d is not opened\n", sbm_devp->sbm_number);
	    return -1;
    }

    switch(cmd){
		case SBMIO_CREATE_SBM:
		{
			struct sbm_config sbm_info;

			if(sbm_devp->status != 0){
				SBM_PRF("sbm%d has been created before\n", sbm_devp->sbm_number);
				goto EXIT;
			}

            res = ali_sbm_copy((void *)&sbm_info, (void *)arg, sizeof(sbm_info));
			if(res) {
                SBM_PRF("%s line %u copy %lx=>%p size %u fail %d\n",
                    __func__, __LINE__, arg, &sbm_info, sizeof(sbm_info), res);
                up(&m_sbm_sem);
                return -EFAULT;
			}
			sbm_devp->sbm_cfg.buffer_size = sbm_info.buffer_size;
			sbm_devp->sbm_cfg.block_size = sbm_info.block_size;
			sbm_devp->sbm_cfg.reserve_size = sbm_info.reserve_size;
			sbm_devp->sbm_cfg.wrap_mode = sbm_info.wrap_mode;
			if(sbm_info.buffer_addr == 0){
#if 1
				SBM_PRF("sbm%d buffer is NULL\n", sbm_devp->sbm_number);
				ret = -1;
				goto EXIT;
#else
				sbm_devp->sbm_cfg.buffer_addr = (unsigned int)kmalloc(sbm_info.buffer_size, GFP_KERNEL);
				if(!sbm_devp->sbm_cfg.buffer_addr){
				SBM_PRF("Kmalloc sbm fail\n"); return -1;
			}
#endif
				sbm_info.buffer_addr = sbm_devp->sbm_cfg.buffer_addr;
			}else{
				sbm_info.buffer_addr = (unsigned int)__CACHE_ADDR_ALI_SEE(sbm_info.buffer_addr);
				sbm_devp->sbm_cfg.buffer_addr = sbm_info.buffer_addr;
			}
			ret = ali_rpc_sbm_create(sbm_devp->sbm_number, sbm_info);
			if(ret < 0){
				SBM_PRF("create sbm%d fail\n", sbm_devp->sbm_number);
				ret = -1;
				goto EXIT;
			}

			//SBM_PRF("%s : create sbm%d done\n", __FUNCTION__, sbm_devp->sbm_number);
			sbm_devp->status = 1;
            sbm_devp->is_full = 0;
			break;
		}

        case SBMIO_RESET_SBM:
        {
            sbm_devp->is_full = 0;
            ret = ali_rpc_sbm_reset(sbm_devp->sbm_number, sbm_devp->sbm_cfg.wrap_mode);
            break;
        }

        case SBMIO_SHOW_VALID_SIZE:
        {
            unsigned int valid_size = 0;
            ret = ali_rpc_sbm_show_valid_size(sbm_devp->sbm_number, sbm_devp->sbm_cfg.wrap_mode, &valid_size);
            res = copy_to_user((void *)arg, (void *)&valid_size, sizeof(int));
            if(res) {
                SBM_PRF("%s line %u copy %p=>%lx size %u fail %d\n",
                    __func__, __LINE__, &valid_size, arg, sizeof(int), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }
            break;
        }

        case SBMIO_SHOW_FREE_SIZE:
        {
            unsigned int free_size = 0;
            ret = ali_rpc_sbm_show_free_size(sbm_devp->sbm_number, sbm_devp->sbm_cfg.wrap_mode, &free_size);
            res = copy_to_user((void *)arg, (void *)&free_size, sizeof(int));
            if(res) {
                SBM_PRF("%s line %u copy %p=>%lx size %u fail %d\n",
                    __func__, __LINE__, &free_size, arg, sizeof(int), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }
            break;
        }

        case SBMIO_SHOW_TOTAL_SIZE:
        {
            unsigned int total_size = sbm_devp->sbm_cfg.buffer_size;
            res = copy_to_user((void *)arg, (void *)&total_size, sizeof(int));
            if(res) {
                SBM_PRF("%s line %u copy %p=>%lx size %u fail %d\n",
                    __func__, __LINE__, &total_size, arg, sizeof(int), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }
            break;
        }

    	case SBMIO_DESTROY_SBM:
    	{
    		if(sbm_devp->status != 0)
    		{
    			ali_rpc_sbm_destroy(sbm_devp->sbm_number, sbm_devp->sbm_cfg.wrap_mode);
    			sbm_devp->status = 0;
                sbm_devp->is_full = 0;
    		}

    		//SBM_PRF("%s : release sbm%d \n", __FUNCTION__, sbm_devp->sbm_number);
    		break;
    	}

        case SBMIO_REQ_BUF_INFO:
    	{
    		struct sbm_req_buf *buf_info = (struct sbm_req_buf *)arg;
    		ret = sbm_req_buf_info(sbm_devp,buf_info);
    		break;
    	}

    	case SBMIO_UPDATE_BUF_INFO:
    	{
    		struct sbm_req_buf *buf_info = (struct sbm_req_buf *)arg;
    		ret = sbm_update_buf_info(sbm_devp,buf_info);
    		break;
    	}

        case SBMIO_SHOW_PKT_NUM:
        {
            unsigned int pkt_num = 0;
            ret = ali_rpc_sbm_show_pkt_num(sbm_devp->sbm_number, sbm_devp->sbm_cfg.wrap_mode, &pkt_num);
            res = copy_to_user((void *)arg, (void *)&pkt_num, sizeof(int));
            if(res) {
                SBM_PRF("%s line %u copy %p=>%lx size %u fail %d\n",
                    __func__, __LINE__, &pkt_num, arg, sizeof(int), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }
            break;
        }

        case SBMIO_REQUEST_WRITE:
        {
            struct sbm_buf write_buffer;
            memset(&write_buffer, 0, sizeof(struct sbm_buf));
            res = copy_from_user((void *)&write_buffer, (void *)arg, sizeof(struct sbm_buf));
            if(res) {
                SBM_PRF("%s line %u copy %lx=>%p size %u fail %d\n",
                    __func__, __LINE__, arg, &write_buffer, sizeof(struct sbm_buf), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }

            ret = ali_rpc_sbm_request_write(sbm_devp->sbm_number, (void**)(&(write_buffer.buf_addr)), (int*)(&(write_buffer.buf_size)));
            write_buffer.buf_addr = (char*)((unsigned int)(write_buffer.buf_addr) & 0x1FFFFFFF);
            res = copy_to_user((void *)arg, (void *)&write_buffer, sizeof(struct sbm_buf));
            if(res) {
                SBM_PRF("%s line %u copy %p=>%lx size %u fail %d\n",
                    __func__, __LINE__, &write_buffer, arg, sizeof(struct sbm_buf), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }
            if(ret == SBM_REQ_BUSY)  ret = -1;
            break;
        }

        case SBMIO_IS_FULL:
        {
            int is_full = sbm_devp->is_full;
            res = copy_to_user((void *)arg, (void *)&is_full, sizeof(int));
            if(res) {
                SBM_PRF("%s line %u copy %p=>%lx size %u fail %d\n",
                    __func__, __LINE__, &is_full, arg, sizeof(int), res);
                up(&m_sbm_sem);
                return -EFAULT;
            }
            break;
        }

        default:
            break;
    }

EXIT:
    up(&m_sbm_sem);
    return ret;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations sbm_fops = {
    .owner    =   THIS_MODULE,     /* Owner */
    .open     =   sbm_open,        /* Open method */
    .release  =   sbm_release,     /* Release method */
    .read     =   sbm_read,        /* Read method */
    .write    =   sbm_write,       /* Write method */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
    .unlocked_ioctl = sbm_ioctl,
#else
    .ioctl    =   sbm_ioctl,       /* Ioctl method */
#endif
};

#define DEVICE_NAME                "ali_sbm"

static dev_t sbm_dev_number;       /* Allotted device number */
static struct class *sbm_class;    /* Tie with the device model */
static struct device *sbm_device;
static struct sbm_dev *sbm_priv[SBM_NUM];

static int ali_sbm_probe(struct platform_device * pdev)
{
    int i, ret;
    int rw_pkt_desc_num = SBM_RW_PKT_DESC_NUM;

	ret = of_get_major_minor(pdev->dev.of_node,&sbm_dev_number, 
			0, SBM_NUM, DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}

    /* Populate sysfs entries */
    sbm_class = class_create(THIS_MODULE, DEVICE_NAME);

    for (i=0; i<SBM_NUM; i++) {
        /* Allocate memory for the per-device structure */
        sbm_priv[i] = kmalloc(sizeof(struct sbm_dev), GFP_KERNEL);
        if (!sbm_priv[i]) {
            SBM_PRF("Bad Kmalloc\n"); return -ENOMEM;
        }
        memset(sbm_priv[i], 0, sizeof(struct sbm_dev));

        sprintf(sbm_priv[i]->name, "ali_sbm%d", i);

        /* Fill in the sbm number to correlate this device
           with the corresponding sbm */
        sbm_priv[i]->sbm_number = i;

	    sbm_priv[i]->status = 0;

        /* Connect the file operations with the cdev */
        cdev_init(&sbm_priv[i]->cdev, &sbm_fops);
        sbm_priv[i]->cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&sbm_priv[i]->cdev, (sbm_dev_number + i), 1);
        if (ret) {
            SBM_PRF("Bad cdev\n");
            return ret;
        }

    	sbm_device = device_create(sbm_class, NULL, MKDEV(MAJOR(sbm_dev_number), i),
                                   NULL, "ali_sbm%d", i);
    	if(sbm_device == NULL){
    		SBM_PRF("sbm create device fail\n");
    		return 1;
    	}

    	sbm_info[i] = NULL;
    	sbm_info_pkt[i] = NULL;

        ali_sbm_procfs_init(i, sbm_priv[i]);
    }

    if (ali_sys_ic_get_chip_id() == ALI_C3921 || ali_sys_ic_get_chip_id() >= ALI_C3505
        || !ali_sys_ic_split_enabled())
        rw_pkt_desc_num = SBM_RW_PKT_DESC_NUM;
    else
        rw_pkt_desc_num = 2;

    desc_pkt_mem[0] = (unsigned int)ali_rpc_malloc_shared_mm(rw_pkt_desc_num * sizeof(struct sbm_rw_desc_pkt));
    SBM_PRF("desc_pkt_mem[%d] = 0x%x\n", 0, desc_pkt_mem[0]);
    if(desc_pkt_mem[0]) {
        for(i = 1; i < rw_pkt_desc_num; i++) {
            desc_pkt_mem[i] = desc_pkt_mem[i - 1] + sizeof(struct sbm_rw_desc_pkt);
            SBM_PRF("desc_pkt_mem[%d] = 0x%x\n", i, desc_pkt_mem[i]);
        }
    }

    if((ali_sys_ic_get_chip_id() == ALI_C3921) || (ali_sys_ic_get_chip_id() >= ALI_C3505)
       || (!ali_sys_ic_split_enabled())) {
        desc_mem[0] = (unsigned int)ali_rpc_malloc_shared_mm(SBM_RW_DESC_NUM * sizeof(struct sbm_rw_desc));
        SBM_PRF("desc_mem[%d] = 0x%x\n", 0, desc_mem[0]);
        if(desc_mem[0]) {
            for(i = 1; i < SBM_RW_DESC_NUM; i++) {
                desc_mem[i] = desc_mem[i - 1] + sizeof(struct sbm_rw_desc);
                SBM_PRF("desc_mem[%d] = 0x%x\n", i, desc_mem[i]);
            }
        }
    }

    sema_init(&m_sbm_sem, 1);

    return 0;
}

/* Driver Exit */
static int ali_sbm_remove(struct platform_device * pdev)
{
    int i;

    ali_sbm_procfs_exit();

    /* Release the major number */
    unregister_chrdev_region(sbm_dev_number, SBM_NUM);

    for (i=0; i<SBM_NUM; i++) {
        device_destroy(sbm_class, MKDEV(MAJOR(sbm_dev_number), i));
        /* Remove the cdev */
        cdev_del(&sbm_priv[i]->cdev);
        kfree(sbm_priv[i]);
    }

    /* Destroy cmos_class */
    class_destroy(sbm_class);

    return 0;
}

static const struct of_device_id ali_sbm_match[] = {
       { .compatible = "alitech, sbm", },
       {},
};

MODULE_DEVICE_TABLE(of, ali_sbm_match);

static struct platform_driver ali_sbm_platform_driver = {
	.probe   = ali_sbm_probe, 
	.remove   = ali_sbm_remove,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = "ali_sbm",
			.of_match_table = ali_sbm_match,
	},
};

module_platform_driver(ali_sbm_platform_driver);
MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("ali share buffer memory driver");
MODULE_LICENSE("GPL");
