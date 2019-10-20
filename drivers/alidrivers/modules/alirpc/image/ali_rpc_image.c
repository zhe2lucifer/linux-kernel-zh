/****************************************************************************(I)(S)
 *  (C)
 *  ALi (Shanghai) Corporation, All Rights Reserved. 2009 Copyright (C)
 *  (C)
 *  File: ali_image_rpc.c
 *  (I)
 *  Description: rpc opeartion
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2012.04.09			Blady		Create
 ****************************************************************************/
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
//#include <linux/ali_image.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/div64.h>
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
#include <linux/ali_rpc.h>

#include <rpc_hld/ali_rpc_image.h>
#include "../ali_rpc.h"

enum HLD_SEE_DEVINIT_FUNC
{
   FUNC_MP_IMAGE_DISPLAY = 0,
};

static UINT32 mp_image_disp[] =
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct mp_imgdisp_info)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};//bysteve

RET_CODE mp_image_display(struct vpo_device *dev, struct mp_imgdisp_info *img_info)
{
    jump_to_func(NULL, ali_rpc_call, dev, (LIB_MP_IMAGE_DISP<<24)|(2<<16)|FUNC_MP_IMAGE_DISPLAY, mp_image_disp);
}
EXPORT_SYMBOL(mp_image_display);//bysteve

