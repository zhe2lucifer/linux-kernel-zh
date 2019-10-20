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
 *  File: ali_rpc_sbm.c
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
//#include <linux/smp_lock.h>
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
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_sbm.h>
#include <linux/ali_rpc.h>

static UINT32 desc_sbm_create[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct sbm_desc_rpc)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

int sbm_see_create(int sbm_idx, int sbm_mode, void *sbm_init)
{
	register RET_CODE ret asm("$2");
    UINT32 *desc = desc_sbm_create;

	jump_to_func(NULL, ali_rpc_call, sbm_idx, (LLD_SBM_MODULE<<24)|(3<<16)|FUNC_SBM_SEE_CREATE, desc);

	return ret;
}

int sbm_see_destroy(int sbm_idx, int sbm_mode)
{
	register RET_CODE ret asm("$2");

	jump_to_func(NULL, ali_rpc_call, sbm_idx, (LLD_SBM_MODULE<<24)|(2<<16)|FUNC_SBM_SEE_DESTROY, NULL);

	return ret;
}

