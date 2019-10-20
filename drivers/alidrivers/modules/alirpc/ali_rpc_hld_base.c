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
 *  File: ali_rpc_hld_base.c
 *  (I)
 *  Description: it is a virtual module to be compitalbe with TDS prj. disable all the
 * 			callback function
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2010.04.01			Sam			Create
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
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <ali_cache.h>

#include "ali_rpc.h"

static struct remote_hld_device *remote_hld_device_base = NULL;

#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID
static struct hld_device_ext *hld_device_base_ext = NULL;
static struct remote_hld_device_ext *remote_hld_device_base_ext = NULL;
#endif


#ifdef CONFIG_RPC_HANDSHAKE_TEST
INT32 rpc_remote_handshake_test_see(unsigned long flag1, unsigned long flag2,unsigned long flag3,unsigned long flag4,unsigned long flag5, unsigned long flag6,unsigned long flag7,unsigned long flag8)
{
   //printk("rpc_remote_handshake_test_see, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",(unsigned int)flag1,(unsigned int)flag2,(unsigned int)flag3,(unsigned int)flag4);
   //printk("rpc_remote_handshake_test_see, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",(unsigned int)flag5,(unsigned int)flag6,(unsigned int)flag7,(unsigned int)flag8);
   
   return SUCCESS;
}
#endif


#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID

void *real_dev_get_by_vir_id(UINT32 virtual_dev_id)
{
	struct hld_device_ext *dp_ext;
	
	if(hld_device_base_ext)
    {
        for (dp_ext = hld_device_base_ext; dp_ext != NULL; dp_ext = dp_ext->next)
        {
            if (dp_ext->virtual_dev->virtual_dev_id == virtual_dev_id)
            {
                return (void*)dp_ext->real_dev;
            }
        }
     }

	return NULL;
}


void *hld_dev_get_by_name(INT8 *name)
{  
   struct remote_hld_device_ext *remote_vir_dev = NULL;
    
   if(NULL == name)
   {
       RPC_PRF("error: device name is NULL!\n");
       return NULL;
   }
   
	if(remote_hld_device_base_ext)
	{
	    for (remote_vir_dev = remote_hld_device_base_ext; remote_vir_dev != NULL; remote_vir_dev = remote_vir_dev->next)
	    {
	        if (0 == strcmp(remote_vir_dev->name, name))
	        {
	            return (void *)remote_vir_dev->remote_dev_id; //return virtual dev id
	        }
	    }
	}
	
    return NULL;
}


void *hld_dev_get_by_type(void *sdev, UINT32 type)
{
    struct remote_hld_device_ext *remote_vir_dev = NULL;

    for (remote_vir_dev = remote_hld_device_base_ext; remote_vir_dev != NULL; remote_vir_dev = remote_vir_dev->next)
    {
        if ((remote_vir_dev->type & HLD_DEV_TYPE_MASK) == type)
        {
            return (void *)remote_vir_dev->remote_dev_id; //return virtual dev id
        }
    }

    return NULL;
}



void *hld_dev_get_by_id(UINT32 type, UINT16 id)
{
   struct remote_hld_device_ext *remote_vir_dev = NULL;

    for (remote_vir_dev = remote_hld_device_base_ext; remote_vir_dev != NULL; remote_vir_dev = remote_vir_dev->next)
    {
        if (remote_vir_dev->type == (type | id))
        {
            return (void *)remote_vir_dev->remote_dev_id; //return virtual dev id
        }
    }

     return NULL;
}


static INT32 remote_hld_dev_add(struct hld_virtual_device *vir_dev)
{
	UINT32 virtual_dev_num = 0;
	struct remote_hld_device_ext *dev = NULL;
	struct remote_hld_device_ext *dp = NULL;
	
	if(remote_hld_device_base_ext == NULL)
	{
	    dev = (struct remote_hld_device_ext *)kmalloc(sizeof(struct remote_hld_device_ext), GFP_KERNEL);
		if (NULL == dev)
		{
			return !SUCCESS;
		}
		memcpy(dev, vir_dev, sizeof(struct hld_virtual_device));
		dev->next = NULL;
		remote_hld_device_base_ext = dev;

		return SUCCESS;
	}
    else
	{					
		for (dp = remote_hld_device_base_ext, virtual_dev_num = 0; dp->next != NULL; dp = dp->next)
		{
			virtual_dev_num++;
		}
		
		if ((virtual_dev_num + 1) > HLD_MAX_DEV_NUMBER)
		{
			RPC_PRF("\n hld_dev_add error: virtual device number beyond the maximum value =	%d !\n", HLD_MAX_DEV_NUMBER);
			return ERR_QUEUE_FULL;
		}
		
		dev = (struct remote_hld_device_ext *)kmalloc(sizeof(struct remote_hld_device_ext), GFP_KERNEL);
		if (NULL == dev)
		{
			return !SUCCESS;
		}
		memcpy(dev, vir_dev, sizeof(struct hld_virtual_device));
		dev->next = NULL;
		dp->next = dev;	
    }

	return SUCCESS;
}


static INT32 remote_hld_dev_remove(struct hld_virtual_device *vir_dev)
{
    struct remote_hld_device_ext *dp_ext = NULL;
    struct remote_hld_device_ext *dp_tmp_ext = NULL;

    if(NULL == vir_dev)
    {
		RPC_PRF("error: vir_dev is NULL!\n");
        return !SUCCESS;
    }
    /* If dev in dev_queue, delete it from queue, else free it directly */
    if (remote_hld_device_base_ext != NULL)
    {
        if (0 == strcmp(remote_hld_device_base_ext->name, vir_dev->name))
        {
            /* Store the remote_hld_device_base in the temperary variable before being modified */
            dp_tmp_ext = remote_hld_device_base_ext;
            remote_hld_device_base_ext = remote_hld_device_base_ext->next;

            /* free the memory of this node */
            kfree(dp_tmp_ext);
        }
        else
        {
            for (dp_ext = (struct remote_hld_device_ext *)remote_hld_device_base; dp_ext->next != NULL; dp_ext = dp_ext->next)
            {
                if (0 == strcmp(dp_ext->next->name, vir_dev->name))
                {
                    /* Store the dp->next in the temperary variable before being modified */
                    dp_tmp_ext = dp_ext->next;
                    dp_ext->next = dp_ext->next->next;

                    /* free the memory of this node */
                    kfree(dp_tmp_ext);
                    break;
                }
            }
        }
    }

    return SUCCESS;
}

#else
/*
 * 	Name		:   hld_dev_get_by_name()
 *	Description	:   Get a device from device link list by device name.
 *	Parameter	:   INT8 *name					: Device name
 *	Return		:	void *						: Device founded
 *
 */
void *hld_dev_get_by_name(INT8 *name)
{
    	struct remote_hld_device *remote_dev;

	/* Remote device */
	if(remote_hld_device_base)
	{
		for (remote_dev = remote_hld_device_base; remote_dev != NULL; 
			remote_dev = (struct remote_hld_device *)remote_dev->next)
		{
			/* Find the device */
			if (strcmp(remote_dev->name, name) == 0)
			{
				return remote_dev->remote;
			}
		}
	}
	
	return NULL;
}

/*
 * 	Name		:   hld_dev_get_by_type()
 *	Description	:   Get a device from device link list by device type.
 *	Parameter	:   INT32 type					: Device type
 *					void *sdev					: Start search nod
 *	Return		:	void *						: Device founded
 *
 */
void *hld_dev_get_by_type(void *sdev, UINT32 type)
{
    	struct remote_hld_device *remote_dev;

	/* Remote device */
	if(remote_hld_device_base)
	{
		for (remote_dev = remote_hld_device_base; remote_dev != NULL; 
			remote_dev = (struct remote_hld_device *)remote_dev->next)
		{
			/* Find the device */
			if ((remote_dev->type & HLD_DEV_TYPE_MASK) == type)
			{
				return remote_dev->remote;
			}
		}
	}
	
	return NULL;
}

/*
 * 	Name		:   hld_dev_get_by_id()
 *	Description	:   Get a device from device link list by device ID.
 *	Parameter	:   UINT32 type					: Device type
 *					UINT16 id					: Device id
 *	Return		:	void *						: Device founded
 *
 */
void *hld_dev_get_by_id(UINT32 type, UINT16 id)
{
    	struct remote_hld_device *remote_dev;

	/* Remote device */
	if(remote_hld_device_base)
	{
		for (remote_dev = remote_hld_device_base; remote_dev != NULL; 
			remote_dev = (struct remote_hld_device *)remote_dev->next)
		{
			if (remote_dev->type == (type | id))
				return remote_dev->remote;
		}
	}
	
	return NULL;
}

static INT32 remote_hld_dev_add(struct hld_device *buf, UINT32 dev_addr)
{
	UINT32 count;
	struct remote_hld_device *dev, *dp;

	dev = (struct remote_hld_device *)kmalloc(sizeof(struct remote_hld_device), GFP_KERNEL);

	memcpy(dev, buf, sizeof(struct hld_device));
	dev->remote = (struct hld_device *)(dev_addr);
	dev->next = NULL;
	if(remote_hld_device_base == NULL)
	{
		remote_hld_device_base = dev;
	}
	else
	{
		if (hld_dev_get_by_name(dev->name) != NULL)
		{
			RPC_PRF("hld_dev_add error: device %s same name!\n", dev->name);
			return -1;
		}

		//printk(KERN_EMERG "remote_hld_dev_add %s\n", dev->name);
		
		/* Add this device to device list */
		/* Move to tail */
		for (dp = remote_hld_device_base, count = 0; dp->next != NULL; 
			dp = (struct remote_hld_device *)dp->next)
		{
			count++;
		}

		if (count >= HLD_MAX_DEV_NUMBER)
		{
			RPC_PRF("too many hld dev \n");
			return -1;
		}

		dp->next = (struct remote_hld_device *)dev;
	}

	/*if(remote_hld_device_base)
	{
		dp = remote_hld_device_base;
		do
		{
			libc_printf("dev name: %s, type: 0x%x, dev: 0x%x, remote: 0x%x\n", dp->name, dp->type, dp, dp->remote);
			dp = dp->next;
		}while (dp != NULL);
	}*/

	RPC_PRF("%s : ok dev %x remote dev %x\n", __FUNCTION__, (int)dev, (int)dev_addr);
	return 0;
}

static INT32 remote_hld_dev_remove(struct hld_device *dev)
{
	struct remote_hld_device *dp;

	/* If dev in dev_queue, delete it from queue, else free it directly */
	if (remote_hld_device_base != NULL)
	{
		if (strcmp(remote_hld_device_base->name, dev->name) == 0)
		{
			remote_hld_device_base = (struct remote_hld_device *)dev->next;
		} 
		else
		{
			for (dp = remote_hld_device_base; dp->next != NULL; dp = (struct remote_hld_device *)dp->next)
			{
				if (strcmp(dp->next->name, dev->name) == 0)
				{
					dp->next = (struct remote_hld_device *)dev->next;
					break;
				}
			}
		}
	}

	return 0;
}

#endif

static void remote_hld_dev_memcpy(void *dest, const void *src, unsigned int len)
{
	// keep it NULL. it is only active in the SEE cpu
	do{}while(0);
}

static void remote_hld_see_init(void *addr)
{
    // keep it NULL. it is only active in the SEE cpu
	do{}while(0);
}

#if 0
enum HLD_DEV_FUNC{
    FUNC_HLD_DEV_ADD = 0,   
    FUNC_HLD_DEV_REMOVE,   
    FUNC_HLD_MEM_CPY,
    FUNC_SEE_STANDBY,
    FUNC_HLD_GET_SEE_VER,
    FUNC_HLD_DISABLE_SEE_PRINTF,
    FUNC_HLD_HIT_SEE_HEART,
    FUNC_HLD_ENABLE_SEE_EXCEPTION,
    FUNC_HLD_SHOW_SEE_PLUGIN_INFO,
    FUNC_HLD_SEE_INIT,
    FUNC_HLD_VDEC_CB,
    FUNC_HLD_VPO_CB,
    FUNC_HLD_VPO_HDMI_CB,
    FUNC_HLD_SND_HDMI_CB,
    FUNC_HLD_SND_SPEC_CB,
    FUNC_HLD_IMG_CB,
    FUNC_HLD_VDE_CB,
    FUNC_HLD_MUS_CB,
};
#endif

static ali_rpc_cb_routine m_vdec_cb = NULL;
static void hld_vdec_cb(UINT32 uParam)
{
	if(m_vdec_cb)
		m_vdec_cb(0, uParam);
}

static ali_rpc_cb_routine m_vdec_spec_cb = NULL;
static void hld_vdec_spec_cb(UINT32 param)
{
	if(m_vdec_spec_cb)
		m_vdec_spec_cb(0, param);
}

static ali_rpc_cb_routine m_vdec_info_cb = NULL;
static void hld_vdec_info_cb(UINT32 param)
{
	if(m_vdec_info_cb)
		m_vdec_info_cb(0, param);
}

static ali_rpc_cb_routine m_vpo_cb = NULL;
static void hld_vpo_cb(UINT32 uParam)
{
	if(m_vpo_cb)
		m_vpo_cb(0, uParam);
}

static ali_rpc_cb_routine m_vpo_hdmi_cb = NULL;
static void hld_vpo_hdmi_cb(UINT32 uParam)
{
	if(m_vpo_hdmi_cb)
		m_vpo_hdmi_cb(0, uParam);
}

static ali_rpc_cb_routine m_snd_cb = NULL;


static void hld_deca_msg_cb(UINT32 uparam)  // For deca callback
{
	if(m_snd_cb)
		m_snd_cb(uparam, 0);
}

static void hld_snd_msg_cb(UINT32 uparam)   // For snd callback
{
	if(m_snd_cb)
		m_snd_cb(uparam, 0);
}

static ali_rpc_cb_routine m_snd_hdmi_cb = NULL;
static void hld_snd_hdmi_cb(UINT32 uParam)
{
	if(m_snd_hdmi_cb)
		m_snd_hdmi_cb(0, uParam);
}

static ali_rpc_cb_routine m_snd_spec_cb = NULL;
static void hld_snd_spec_cb(UINT32 uParam)
{
	if(m_snd_spec_cb)
		m_snd_spec_cb(0, uParam);
}

static ali_rpc_cb_routine m_img_cb = NULL;
static void hld_img_cb(unsigned long type, unsigned long param)
{
	if(m_img_cb)
		m_img_cb(type, param);
}	

static ali_rpc_cb_routine m_vde_cb = NULL;
static void hld_vde_cb(unsigned long type, unsigned long param)
{
	if(m_vde_cb)
		m_vde_cb(type, param);
}

static ali_rpc_cb_routine m_mus_cb = NULL;
static void hld_mus_cb(unsigned long type, unsigned long param)
{
	if(m_mus_cb)
		m_mus_cb(type, param);
}

static void empty(void)
{
	do{}while(0);
}

#if 0
static UINT32 hld_dev_entry[] = 
{ 
#ifdef CONFIG_RPC_HANDSHAKE_TEST
    (UINT32)rpc_remote_handshake_test_see,
#endif
	(UINT32)remote_hld_dev_add,
	(UINT32)remote_hld_dev_remove,
	(UINT32)remote_hld_dev_memcpy,
	(UINT32)empty,
	(UINT32)empty,
	(UINT32)empty,
	(UINT32)empty,
	(UINT32)empty,
	(UINT32)empty,
       (UINT32)remote_hld_see_init,
	(UINT32)hld_vdec_cb,
	(UINT32)hld_vpo_cb,
	(UINT32)hld_vpo_hdmi_cb,
	(UINT32)hld_snd_hdmi_cb,
	(UINT32)hld_snd_spec_cb,	
	(UINT32)hld_img_cb,
	(UINT32)hld_vde_cb,
	(UINT32)hld_mus_cb,	
};
#endif

UINT32 rpc_ci_test(UINT32 para1, UINT32 para2,UINT32 para3,UINT32 para4,
                      UINT32 para5, UINT32 para6,UINT32 para7,UINT32 para8)
{
	UINT32 ret = 0;
	
	//printk("kernel rpc_ci_test 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",para1,para2,para3,para4);
	//printk("kernel rpc_ci_test 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",para5,para6,para7,para8);

	if(para1 != 0xffffffff || para2 != 0xeeeeeeee || para3 != 0xdddddddd || para4 != 0xcccccccc ||
       para5 != 0x55555555 || para6 != 0x66666666 || para7 != 0x77777777 || para8 != 0x88888888 )
	{

		ret = RET_FAILURE;
		printk("check data error! \n");
		return ret;
	}

	return SUCCESS;
}

static void hld_cache_flush(UINT32 addr, UINT32 data_length)
{
    __CACHE_FLUSH_ALI((void *)addr,  data_length);
}
static void hld_cache_inv(UINT32 addr, UINT32 data_length)
{
   __CACHE_INV_ALI((void *)addr,  data_length);
}


static UINT32 hld_dev_entry[] =
{
#ifdef CONFIG_RPC_HANDSHAKE_TEST
    (UINT32)rpc_remote_handshake_test_see,
#endif
    (UINT32)remote_hld_dev_add,
    (UINT32)remote_hld_dev_remove,
    (UINT32)remote_hld_dev_memcpy,
    (UINT32)remote_hld_see_init,
	(UINT32)empty,//remote_cpy_from_priv_mem
	(UINT32)empty,//remote_hld_notify_see_trig
	(UINT32)empty,//remote_otp_get_mutex
	(UINT32)empty,//remote_enable_hd_decoder
	(UINT32)hld_vdec_cb,
	(UINT32)hld_vpo_cb,
	(UINT32)hld_vpo_hdmi_cb,
	(UINT32)hld_snd_hdmi_cb,
	(UINT32)hld_snd_spec_cb,	
	(UINT32)hld_img_cb,
	(UINT32)hld_vde_cb,
	(UINT32)hld_mus_cb,
	(UINT32)hld_deca_msg_cb,
	(UINT32)hld_cache_flush,
	(UINT32)hld_cache_inv,
	(UINT32)hld_vdec_spec_cb,
    (UINT32)hld_snd_msg_cb,
	(UINT32)hld_vdec_info_cb,
	(UINT32)empty, //hld_dmx_spec_cb
	(UINT32)empty, //hld_subtitle_cb
	(UINT32)empty, //hld_ts_data_cb  
	(UINT32)empty,// remote_hld_dev_see_printf_to_main_with_rpc
	(UINT32)rpc_ci_test,
#ifdef CONFIG_ALI_TAC	
	(UINT32)tac_rpc_S2M,
#endif
#ifdef CONFIG_ALI_SEC
	(UINT32)rpc_s2m_bc_generate_cwc,
	(UINT32)rpc_s2m_bc_hdmi_cmd,
	(UINT32)rpc_s2m_bc_set_dsc_status,
#endif
};


static UINT32 desc_hld_dev[] = 
{ //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct hld_device)),
	1, DESC_P_PARA(0, 0, 0), 
	//desc of pointer ret
	0,                          
	0,
};

void ali_rpc_hld_base_callee(UINT8 *msg)
{
    	ali_rpc_ret((unsigned long)hld_dev_entry, msg);
}

void ali_rpc_register_callback(enum ALI_RPC_CB_TYPE type, void *cb_func)
{
	switch(type)
	{
		case ALI_RPC_CB_VDEC:
			m_vdec_cb = (ali_rpc_cb_routine)cb_func;
			break;
		case ALI_RPC_CB_VPO:
			m_vpo_cb = (ali_rpc_cb_routine)cb_func;
			break;			
		case ALI_RPC_CB_VPO_HDMI:
			m_vpo_hdmi_cb = (ali_rpc_cb_routine)cb_func;
			break;
		case ALI_RPC_CB_SND_HDMI:
			m_snd_hdmi_cb = (ali_rpc_cb_routine)cb_func;
			break;
		case ALI_RPC_CB_SNC_SPC:
			m_snd_spec_cb = (ali_rpc_cb_routine)cb_func;
			break;
		case ALI_RPC_CB_IMG:
			m_img_cb = (ali_rpc_cb_routine)cb_func;
			break;			
		case ALI_RPC_CB_VDE:
			m_vde_cb = (ali_rpc_cb_routine)cb_func;			
			break;	
		case ALI_RPC_CB_MUS:
			m_mus_cb = (ali_rpc_cb_routine)cb_func;			
			break;	
        case ALI_RPC_CB_VDEC_SPEC:
            m_vdec_spec_cb = (ali_rpc_cb_routine)cb_func;
            break;
        case ALI_RPC_CB_VDEC_INFO:
            m_vdec_info_cb = (ali_rpc_cb_routine)cb_func;
            break;			
        case ALI_RPC_CB_SND_FIRST_FRAME_OUTPUT:
        case ALI_RPC_CB_SND_MONITOR_OUTPUT_DATA_END:
        case ALI_RPC_CB_SND_MONITOR_SBM_MIX_END:
        case ALI_RPC_CB_DECA_MONITOR_NEW_FRAME:
        case ALI_RPC_CB_DECA_MONITOR_START:
        case ALI_RPC_CB_DECA_MONITOR_STOP:
        case ALI_RPC_CB_DECA_MONITOR_DECODE_ERR:
        case ALI_RPC_CB_DECA_MONITOR_OTHER_ERR:
        case ALI_RPC_CB_DECA_STATE_CHANGED:
            m_snd_cb = (ali_rpc_cb_routine)cb_func;
            break;

		default:
			break;
	}
}

#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID

static UINT32 desc_hld_virtual_dev[] =
{ //desc of pointer para
  1, DESC_STATIC_STRU(0, sizeof(struct hld_virtual_device)),
  1, DESC_P_PARA(0, 0, 0),
  0,
  0,
};

INT32 hld_dev_add_remote(struct hld_virtual_device *vir_dev)
{
    jump_to_func(NULL, ali_rpc_call, vir_dev, (HLD_BASE_MODULE<<24)|(1<<16)|FUNC_HLD_DEV_ADD, desc_hld_virtual_dev);
}

INT32 hld_dev_remove_remote(struct hld_virtual_device *vir_dev)
{

	jump_to_func(NULL, ali_rpc_call, vir_dev, (HLD_BASE_MODULE<<24)|(1<<16)|FUNC_HLD_DEV_REMOVE, desc_hld_dev);

}

#else
INT32 hld_dev_add_remote(struct hld_device *dev, UINT32 dev_addr)
{
    
 	jump_to_func(NULL, ali_rpc_call, dev, (HLD_BASE_MODULE<<24)|(2<<16)|FUNC_HLD_DEV_ADD, desc_hld_dev);
	
}

INT32 hld_dev_remove_remote(struct hld_device *dev)
{

	jump_to_func(NULL, ali_rpc_call, dev, (HLD_BASE_MODULE<<24)|(1<<16)|FUNC_HLD_DEV_REMOVE, desc_hld_dev);

}

#endif




void hld_dev_memcpy_ex(void *dest, const void *src, unsigned int len)
{
    	jump_to_func(NULL, ali_rpc_call, dest, (HLD_BASE_MODULE<<24)|(3<<16)|FUNC_HLD_MEM_CPY, NULL);
}

void hld_dev_memcpy(void *dest, const void *src, unsigned int len)
{
	#ifdef CONFIG_MIPS
    __CACHE_FLUSH_ALI((unsigned long)src, len);
	
    if(*(unsigned char *)(src+len-1) != *(volatile unsigned char*)((UINT32)(src+len-1)|0xa0000000))
    {
        //make sure data is flushed into cache before send to SEE
        asm volatile(".word 0x7000003f; nop; nop");
    }
	#else
	#endif
    hld_dev_memcpy_ex(dest, src, len);
}


void hld_dev_see_init(void *addr)
{
	jump_to_func(NULL, ali_rpc_call, addr, (HLD_BASE_MODULE << 24) | (1 << 16) | FUNC_HLD_SEE_INIT, NULL);
}

UINT32 see_standby(UINT32 status)
{
	jump_to_func(NULL, ali_rpc_call, status, (HLD_BASE_MODULE<<24) | (1<<16) | FYNC_HLD_SEE_STANDBY, NULL);
}


INT32 ali_rpc_ci_test(UINT32 para1,UINT32 para2,UINT32 para3,UINT32 para4,UINT32 para5,UINT32 para6,UINT32 para7,UINT32 para8)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_BASE_MODULE<<24)|(8<<16)|FUNC_RPC_CI_TEST_CASE, NULL);
}

#if 0

UINT32 hld_get_see_version(UINT8 *dest)
{
	
    	jump_to_func(NULL, ali_rpc_call, dest, (HLD_BASE_MODULE << 24) | (1<< 16) | FUNC_HLD_GET_SEE_VER, 
			NULL);

}

void hld_disable_see_printf(unsigned long disable)
{
	jump_to_func(NULL, ali_rpc_call, disable, (HLD_BASE_MODULE<<24)|(1<<16)|FUNC_HLD_DISABLE_SEE_PRINTF, NULL);
}

void hld_hit_see_heart(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_BASE_MODULE<<24)|(0<<16)|FUNC_HLD_HIT_SEE_HEART, NULL);
}

void hld_enable_see_exception(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_BASE_MODULE<<24)|(0<<16)|FUNC_HLD_ENABLE_SEE_EXCEPTION, NULL);
}

void hld_show_see_plugin_info(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_BASE_MODULE<<24)|(1<<16)|FUNC_HLD_SHOW_SEE_PLUGIN_INFO, 
		NULL);

}

#endif

