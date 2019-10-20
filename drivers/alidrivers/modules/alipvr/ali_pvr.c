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

#include <linux/version.h>
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
#include <linux/version.h>
#include <linux/semaphore.h>
#include <linux/file.h>
#include <linux/ali_rpc.h>
//#include <ali_rpcng.h>
#include <linux/ali_pvr.h>
#include <ali_board_config.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include <ali_cache.h>
#include <ali_dsc_common.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <rpc_hld/ali_rpc_hld_pvr.h>
#include <linux/ali_dsc.h>
#include <alidefinition/adf_dsc.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <ali_soc.h>
#ifdef CONFIG_ALI_SEC
#include <linux/ali_sec.h>
#endif


#define DSC_NPARA(x) ((HLD_PVR_MODULE << 24)|(x << 16))


#ifndef TS_PACKET_SIZE
#define TS_PACKET_SIZE 188
#else
#undef  TS_PACKET_SIZE
#define TS_PACKET_SIZE 188
#endif

#define MAX_SUPPORT_PVR_PLAY_NUM          3
#define PVR_MMAP_SIZE (256*1024)

struct pvr_vm_node
{
    void *vm_kaddr;
    unsigned long vm_start;
    unsigned long vm_end;
    size_t vm_size;
    long vm_owner;
    struct vm_area_struct *vma;
};

/* Per-device (per-pvr) structure */
struct pvr_dev
{
    struct cdev cdev;
    int pvr_number;
    char name[10];
    int status;
    int open_count;
    struct semaphore sem;/*protect variable list_head*/
    struct list_head list_head;
    char *p_mmap;
    struct pvr_vm_node vm_area_node;
};

struct pvr_enc_param
{
    struct list_head list_head;
    void *only_flag;/* represent for which fd it is owned*/
    unsigned int indicator; /*PVR_IO_START_BLOCK:PVR_IO_START_BLOCK create ,PVR_IO_START_REENCRYPT :PVR_IO_START_REENCRYPT create,PVR_IO_START_BLOCK_EVO:PVR_IO_START_BLOCK_EVO create, */
    PVR_BLOCK_ENC_PARAM enc_param;
};

struct pvr_key_item
{
    unsigned int    switch_block_count;/*switch key block count*/
    PVR_RPC_RAW_DECRYPT_EVO decrypt_evo_info;/*decrypt dsc infomation*/
};

struct pvr_decrypt_res_param
{
    struct list_head list_head;
    void *only_flag;                        /* represent for which fd it is owned*/
    unsigned int    decrypt_hdl;            /* decrypt_hdl*/
    unsigned char   magic[4];               /*magic number */

    unsigned int    block_data_size;        /* block data size */
    unsigned char   *block_data_buf;        /* block data buffer */
    unsigned int    dsc_handle_num;         /*decrypt dsc_num*/
    struct pvr_key_item *key_item_info;     /*decrypt dsc infomation*/
};


struct decrypt_buffer_item
{
    unsigned int  buffer;
    unsigned int  length;
    unsigned int  state;    /*0,free;1,using*/
};

struct decrypt_buffer_mgr
{
    unsigned int                max_block_item_support;
    struct decrypt_buffer_item  *buf_item;
};


#define ALGO_AES_IV_LENGTH    16
#define ALGO_DES_IV_LENGTH    8
#define DEDAULT_BLOCK_SIZE (0xBC00) //384*TS_PACKET_SIZE

#if 0
//#define PVR_PRF printk

#define PVR_PRF(fmt, args...) \
    do { \
        printk(KERN_ERR"\t"fmt, ##args); \
    } while (0)
#else
#define PVR_PRF(...)   do{}while(0)
#endif

#define DEVICE_NAME                "ali_pvr"

#define QUANTUM_SIZE    (47 * 1024)
static dev_t pvr_dev_number;       /* Allocated device number */
static struct class *pvr_class;    /* Tie with the device model */
static struct device *pvr_device;
static struct pvr_dev *pvr_priv[PVR_NUM];
static struct semaphore m_pvr_sem;
static char *pvr_decrypt_buffer = NULL;
static int pvr_decrypt_buffer_size = QUANTUM_SIZE;
static char decrypt_res_magic[4] =
{
    'A',
    'P',
    'V',
    'R',
};
static struct decrypt_buffer_mgr   *g_decrypt_buffer_mgr = NULL;

extern unsigned long __G_ALI_MM_PVR_BUF_MEM_SIZE;            /*pvr block buffer share mediaplay pe buffer,256kb */
extern unsigned long __G_ALI_MM_PVR_BUF_MEM_START_ADDR;      /*pvr block buffer share mediaplay pe buffer */
static UINT32 pvr_g_block_size = DEDAULT_BLOCK_SIZE;
static unsigned char *pvr_get_decrypt_block_buffer(int block_size);
static  int pvr_free_decrypt_block_buffer(const unsigned char *free_buf);
int ali_pvr_release_pvr_key
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
);
static int ali_pvr_release_decrypt_res
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
);
static int pvr_open(struct inode *inode, struct file *file)
{
    struct pvr_dev *pvr_dev_p;

    down(&m_pvr_sem);

    /* Get the per-device structure that contains this cdev */
    pvr_dev_p = container_of(inode->i_cdev, struct pvr_dev, cdev);

    /* Easy access to pvr_dev_p from the entry points */
    file->private_data = pvr_dev_p;

    if(pvr_dev_p->open_count == 0)
    {
        /* Initialize some fields */
        pvr_dev_p->status = 0;
    }

    pvr_dev_p->open_count++;

    up(&m_pvr_sem);

    return 0;
}

static int pvr_release(struct inode *inode, struct file *file)
{
    struct pvr_dev *pvr_dev_p = file->private_data;
    struct list_head *pos = NULL;
    struct pvr_enc_param *temp = NULL;
    PVR_RPC_RAW_DECRYPT decrypt_info;
    struct pvr_decrypt_res_param  *res_param = NULL;    
    unsigned int cmd = 0;
    int ret = 0;
    char release_flag = 0;
    
    down(&m_pvr_sem);

    pvr_dev_p->open_count--;

    if(pvr_dev_p->open_count == 0)
    {
        if(pvr_dev_p->status != 0)
        {
            pvr_dev_p->status = 0;
        }
    }
    else if(pvr_dev_p->open_count < 0)
    {
        PVR_PRF("PVR driver[%d] open count fail %d.\n", pvr_dev_p->pvr_number, pvr_dev_p->open_count);
    }

    down(&pvr_dev_p->sem);
    list_for_each(pos,&pvr_dev_p->list_head)
    {
        temp = container_of(pos,struct pvr_enc_param,list_head);
        PVR_PRF("\n%s temp->only_flag:(0x%08x),file:(0x%08x) \n",__FUNCTION__,temp->only_flag,file);
        if((temp != NULL) && (temp->only_flag == file))
        {
            memset(&decrypt_info,0,sizeof(PVR_RPC_RAW_DECRYPT));
            if ((temp->indicator == PVR_IO_START_BLOCK_EVO) 
                || (temp->indicator == PVR_IO_START_BLOCK)
                || (temp->indicator == PVR_IO_START_REENCRYPT))  //for ap abort exit,release record relative resource
            {   
                decrypt_info.stream_id = (UINT32)temp->enc_param.stream_id;
                if(temp->enc_param.reencrypt_type == 1)
                {
                    cmd = PVR_IO_RELEASE_PVR_KEY;   
                    PVR_PRF("decrypt_info.stream_id:(%d)\n",decrypt_info.stream_id);
                    ali_pvr_release_pvr_key(pvr_dev_p,file,cmd,decrypt_info.stream_id);
                }
                release_flag = 1;
                PVR_PRF("pvr_release PVR_RPC_IO_FREE_BLOCK: decrypt_info.stream_id:%d\n",decrypt_info.stream_id);
                cmd = PVR_RPC_IO_FREE_BLOCK;
#if !defined(CONFIG_ALI_RPCNG)
                ret = pvr_free_resources(cmd, &decrypt_info);
#else
                RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
                RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_PVR_decrypt, sizeof(PVR_decrypt), &decrypt_info);
                ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif 
                if(ret!= RET_SUCCESS){
                    PVR_PRF("%s,line:%d, pvr free resource fail!!\n", __FUNCTION__, __LINE__);
                    ret = -1;
                }
                list_del(pos);
                kfree(temp);
                break;
            }    
        }
        res_param = container_of(pos,struct pvr_decrypt_res_param,list_head);
        if((res_param != NULL) && (res_param->only_flag == file) &&
           (!memcmp(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic)))
          )  //for ap abort exit,release playack relative resource
        {
            PVR_PRF("pvr_release PVR_IO_RELEASE_DECRYPT_RES: res_param->decrypt_hdl:(0x%08x)\n",res_param->decrypt_hdl);
            cmd = PVR_IO_RELEASE_DECRYPT_RES;
            ali_pvr_release_decrypt_res(pvr_dev_p,file,cmd,res_param->decrypt_hdl);
            break;
        }

    }
    if (!release_flag)
    {
    /*
    * Here do not really to free block resource, so stream_id
    * is set to invalid. It just to reset/free other necessary parameters
    * for playback or other scene
    */
        memset(&decrypt_info,0,sizeof(PVR_RPC_RAW_DECRYPT));
        decrypt_info.stream_id = -1;

        cmd = PVR_RPC_IO_FREE_BLOCK;
    #if !defined(CONFIG_ALI_RPCNG)
        ret = pvr_free_resources(cmd, &decrypt_info);
    #else
        RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
        RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_PVR_decrypt, sizeof(PVR_decrypt), &decrypt_info);
        ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
    #endif 
        if(ret!= RET_SUCCESS){
            PVR_PRF("%s,line:%d, pvr free resource fail!!\n", __FUNCTION__, __LINE__);
            ret = -1;
        }
    
    }

    up(&pvr_dev_p->sem);

    up(&m_pvr_sem);

    return ret;
}
static int ali_pvr_DeEncrypt_ex(pDEEN_CONFIG p_DeEn,UINT8 *input, UINT8 *output ,
									UINT32 total_length, UINT8 is_block_mode, PVR_REC_VIDEO_PARAM *video_param)
{

    int ret = RET_FAILURE;
#if !defined(CONFIG_ALI_RPCNG)
    (void)is_block_mode;
    ret = pvr_block_de_encrypt(p_DeEn,input,output,total_length,video_param);
    return ret;
#else
    Param p1;
    Param p2;
    Param p3;
    Param p4;
	Param p5;

    RPC_PARAM_UPDATE(p1, PARAM_INOUT, PARAM_DeEncrypt_config_rpc, sizeof(DeEncrypt_config_rpc), (void *)p_DeEn);
    RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_UINT32, sizeof(UINT32), (void *)&input);
    RPC_PARAM_UPDATE(p3, PARAM_IN, PARAM_UINT32, sizeof(UINT32), (void *)&output);
    RPC_PARAM_UPDATE(p4, PARAM_IN, PARAM_UINT32, sizeof(UINT32), (void *)&total_length);
	RPC_PARAM_UPDATE(p5, PARAM_INOUT, PARAM_PVR_REC_VIDEO_PARAM, sizeof(PVR_REC_VIDEO_PARAM), (void *)video_param);

    ret =  RpcCallCompletion(RPC_PVR_BlockDeEncrypt,&p1,&p2,&p3,&p4,&p5,NULL);
    if(ret!= RET_SUCCESS)
    {
        PVR_PRF("RpcCallCompletion(RPC_PVR_BlockDeEncrypt) ret :%d\n",ret);
        PVR_PRF("*");
    }
    else
    {
        PVR_PRF("-");
    }
    return ret;
#endif
}
static int ali_pvr_umemcpy(void *dest, const void *src, UINT32 n)
{
    int ret = 0;
    int sflag = access_ok(VERIFY_READ, (void __user *)src, n);
    int dflag = access_ok(VERIFY_WRITE, (void __user *)dest, n);

    if(segment_eq(get_fs(), USER_DS))
    {
        if(sflag && !dflag)
        {
            ret = copy_from_user(dest, (void __user *)src, n);
        }
        else    if(dflag && !sflag)
        {
            ret = copy_to_user(dest, src, n);
        }
        else if(!sflag && !dflag)
        {
            memcpy(dest, src, n);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        memcpy(dest, src, n);
    }
    return ret;
}
static void pvr_deal_cache_before_dma(UINT8 *in, UINT8 *out, UINT32 data_length)
{
    __CACHE_FLUSH_ALI((in),  (data_length));

    if ((UINT32)in != (UINT32)out)
    {
        __CACHE_CLEAN_ALI((out),  (data_length));
    }
}

static void pvr_deal_cache_after_dma(UINT8 *out, UINT32 data_length)
{
    __CACHE_INV_ALI((out),  (data_length));
}

int ali_pvr_block_de_encrypt(DEEN_CONFIG *p_deen,UINT8 *input,UINT8 *output,
							UINT32 total_length, PVR_REC_VIDEO_PARAM *video_param)
{
    int ret = RET_FAILURE;
    DEEN_CONFIG deen;

    if(!p_deen || !input || !output || !total_length || !video_param)
    {
        return ret;
    }

    ali_pvr_umemcpy(&deen, p_deen, sizeof(DEEN_CONFIG));
#if 0
    if((UINT32)deen.dec_dev < VIRTUAL_DEV_NUM)
    {
        if(CSA == deen.Decrypt_Mode)
        {
            deen.dec_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_CSA, (UINT32)deen.dec_dev);
        }
        else if(AES == deen.Decrypt_Mode)
        {
            deen.dec_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_AES, (UINT32)deen.dec_dev);
        }
        else if((DES == deen.Decrypt_Mode) || (TDES == deen.Decrypt_Mode))
        {
            deen.dec_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_DES, (UINT32)deen.dec_dev);
        }
    }
    if((UINT32)deen.enc_dev < VIRTUAL_DEV_NUM)
    {
        if(AES == deen.Encrypt_Mode)
        {
            deen.enc_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_AES, (UINT32)deen.enc_dev);
        }
        else if((DES == deen.Decrypt_Mode) || (TDES == deen.Decrypt_Mode))
        {
            deen.enc_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_DES, (UINT32)deen.enc_dev);
        }
    }

    if(!deen.dec_dev || !deen.enc_dev)
#endif
    if((!deen.dec_dev) && (!deen.enc_dev))
    {
        printk(KERN_ERR "Invalid dec/enc dev, should be see pointer/ID\n");
        return ret;
    }

    pvr_deal_cache_before_dma(input,output,(total_length*TS_PACKET_SIZE));
    ret = ali_pvr_DeEncrypt_ex(&deen,input,output,total_length, 1, video_param);
    pvr_deal_cache_after_dma(output,(total_length*TS_PACKET_SIZE));

    memcpy(p_deen,&deen,sizeof(DEEN_CONFIG));

    return ret;
}

int ali_pvr_ts_de_encrypt(DEEN_CONFIG *p_deen,UINT8 *input,UINT8 *output,
						UINT32 total_length, PVR_REC_VIDEO_PARAM *video_param)
{
    int ret = RET_FAILURE;
    DEEN_CONFIG deen;

    if(!p_deen || !input || !output || !total_length || !video_param)
    {
        return ret;
    }

    ali_pvr_umemcpy(&deen, p_deen, sizeof(DEEN_CONFIG));
    //if((UINT32)deen.dec_dev < VIRTUAL_DEV_NUM)
    //{
    //  if(CSA == deen.Decrypt_Mode)
    //  {
    //      deen.dec_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_CSA, (UINT32)deen.dec_dev);
    //  }
    //  else if(AES == deen.Decrypt_Mode)
    //  {
    //      deen.dec_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_AES, (UINT32)deen.dec_dev);
    //  }
    //  else if((DES == deen.Decrypt_Mode) || (TDES == deen.Decrypt_Mode))
    //  {
    //      deen.dec_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_DES, (UINT32)deen.dec_dev);
    //  }
    //}
    //if((UINT32)deen.enc_dev < VIRTUAL_DEV_NUM)
    //{
    //   if(AES == deen.Encrypt_Mode)
    //  {
    //      deen.enc_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_AES, (UINT32)deen.enc_dev);
    //  }
    //  else if((DES == deen.Decrypt_Mode) || (TDES == deen.Decrypt_Mode))
    //  {
    //      deen.enc_dev = (void *)hld_dev_get_by_id(HLD_DEV_TYPE_DES, (UINT32)deen.enc_dev);
    //  }
    //}

    if((!deen.dec_dev) && (!deen.enc_dev))
    {
        printk(KERN_ERR "Invalid dec/enc dev, should be see pointer/ID\n");
        return ret;
    }

    pvr_deal_cache_before_dma(input,output,(total_length*TS_PACKET_SIZE));
    ret = ali_pvr_DeEncrypt_ex(&deen,input,output,total_length, 1, video_param);
    pvr_deal_cache_after_dma(output,(total_length*TS_PACKET_SIZE));

    memcpy(p_deen,&deen,sizeof(DEEN_CONFIG));

    return ret;
}


static int pvr_decrypt_buffer_allocate(int size)
{
    if (size <= 0)
    {
        return -1;
    }
#if 0
    if (NULL != pvr_decrypt_buffer)
    {
        kfree(pvr_decrypt_buffer);
        pvr_decrypt_buffer = NULL;
    }
    pvr_decrypt_buffer = (char *)kmalloc(size, GFP_KERNEL);
    if (NULL == pvr_decrypt_buffer)
    {
        return -1;
    }
#else
    pvr_decrypt_buffer = pvr_get_decrypt_block_buffer(size);
    if (NULL == pvr_decrypt_buffer)
    {
        return -1;
    }
#endif
    return 0;
}

static void pvr_decrypt_buffer_free(void)
{
    if (NULL != pvr_decrypt_buffer)
    {
#if 0        
        kfree(pvr_decrypt_buffer);
#else
        pvr_free_decrypt_block_buffer(pvr_decrypt_buffer);
#endif 
        pvr_decrypt_buffer = NULL;
    }
    return;
}


static int pvr_block_decrypt_buffer_init(void)
{
    int ret = 0;
    if(NULL == g_decrypt_buffer_mgr)
    {
        g_decrypt_buffer_mgr = (struct decrypt_buffer_mgr *)kmalloc(sizeof(struct decrypt_buffer_mgr), GFP_KERNEL);
        if(g_decrypt_buffer_mgr != NULL)
        {
            memset(g_decrypt_buffer_mgr,0x00,sizeof(struct decrypt_buffer_mgr));
            g_decrypt_buffer_mgr->max_block_item_support = MAX_SUPPORT_PVR_PLAY_NUM;
            g_decrypt_buffer_mgr->buf_item = (struct decrypt_buffer_item *)kmalloc(sizeof(struct decrypt_buffer_item) * g_decrypt_buffer_mgr->max_block_item_support, GFP_KERNEL);
            if(NULL == g_decrypt_buffer_mgr->buf_item)
            {
                kfree(g_decrypt_buffer_mgr);
                g_decrypt_buffer_mgr = NULL;
                ret = -ENOMEM;
            }
            else
            {
                memset(g_decrypt_buffer_mgr->buf_item,0x00,sizeof(struct decrypt_buffer_item) * g_decrypt_buffer_mgr->max_block_item_support);
            }
            ret = 0;
        }
        else
        {
            ret = -ENOMEM;
        }
    }
    return ret;
}

static int pvr_block_decrypt_buffer_uninit(void)
{
    int ret = 0;
    if(NULL != g_decrypt_buffer_mgr)
    {
        if (g_decrypt_buffer_mgr->buf_item)
        {
            kfree(g_decrypt_buffer_mgr->buf_item);
            g_decrypt_buffer_mgr->buf_item = NULL;
        }
        kfree(g_decrypt_buffer_mgr);
        g_decrypt_buffer_mgr = NULL;
    }
    else
    {
        ret = -EINVAL;
    }
    return ret;
}

static unsigned char *pvr_get_decrypt_block_buffer(int block_size)
{
    int i = 0;
    unsigned char *ret_buf = NULL;
    unsigned int  last_pos = 0;
    int           find_pos = -1;

    if(g_decrypt_buffer_mgr != NULL)
    {
        for(i = 0; i < g_decrypt_buffer_mgr->max_block_item_support; i++)
        {
            if(0 == g_decrypt_buffer_mgr->buf_item[i].state)
            {
                find_pos = i;
                break;
            }
        }
        if(find_pos < 0)
        {
            return ret_buf;
        }

        for(i = 0; i < g_decrypt_buffer_mgr->max_block_item_support; i++)
        {
            if((1 == g_decrypt_buffer_mgr->buf_item[i].state) &&
               (last_pos < (g_decrypt_buffer_mgr->buf_item[i].buffer + g_decrypt_buffer_mgr->buf_item[i].length)))
            {
                last_pos = g_decrypt_buffer_mgr->buf_item[i].buffer + g_decrypt_buffer_mgr->buf_item[i].length;
            }
        }
        if((last_pos + block_size ) > __G_ALI_MM_PVR_BUF_MEM_SIZE)   //buffer rewind
        {
            last_pos = 0;
            for(i = 0; i < g_decrypt_buffer_mgr->max_block_item_support; i++)
            {
                if((1 == g_decrypt_buffer_mgr->buf_item[i].state) &&
                   (last_pos >= g_decrypt_buffer_mgr->buf_item[i].buffer)
                   && (last_pos < (g_decrypt_buffer_mgr->buf_item[i].buffer +  g_decrypt_buffer_mgr->buf_item[i].length))
                  )
                {
                    last_pos = g_decrypt_buffer_mgr->buf_item[i].buffer + g_decrypt_buffer_mgr->buf_item[i].length;
                }
            }
        }
        g_decrypt_buffer_mgr->buf_item[find_pos].state  = 1;
        g_decrypt_buffer_mgr->buf_item[find_pos].buffer = last_pos;
        g_decrypt_buffer_mgr->buf_item[find_pos].length = block_size;
        ret_buf = (unsigned char *)(__G_ALI_MM_PVR_BUF_MEM_START_ADDR + last_pos);
    }
    return ret_buf;
}

static  int pvr_free_decrypt_block_buffer(const unsigned char *free_buf)
{
    int     ret = 0;
    int     find_pos = -1;
    int     i = 0;
    unsigned long    free_buf_pos = 0;
    if(g_decrypt_buffer_mgr != NULL)
    {
        free_buf_pos =  (((unsigned long)(free_buf)) - __G_ALI_MM_PVR_BUF_MEM_START_ADDR);
        for(i = 0; i < g_decrypt_buffer_mgr->max_block_item_support; i++)
        {
            if((1 == g_decrypt_buffer_mgr->buf_item[i].state) &&
               (free_buf_pos == g_decrypt_buffer_mgr->buf_item[i].buffer)
              )
            {
                find_pos = i;
                break;
            }
        }
        if((find_pos > 0)&& (find_pos < g_decrypt_buffer_mgr->max_block_item_support))
        {
            g_decrypt_buffer_mgr->buf_item[i].state  = 0;
            g_decrypt_buffer_mgr->buf_item[i].buffer = 0;
            g_decrypt_buffer_mgr->buf_item[i].length = 0;
        }
        else
        {
            ret = -2;
        }
    }
    else
    {
        ret = -1;
    }
    return ret;
}

static int ali_pvr_capture_decrypt_res
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int ret = 0;
    struct ali_pvr_capt_decrypt_res_param capt_decrypt_param;
    struct pvr_decrypt_res_param  *res_param = NULL;

    memset(&capt_decrypt_param,0x00,sizeof(struct ali_pvr_capt_decrypt_res_param));
    ret = copy_from_user((void *)&capt_decrypt_param, (void *)arg, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    if((NULL == pvr_dev) || (NULL == file))
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    if((capt_decrypt_param.block_data_size % TS_PACKET_SIZE) != 0)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EINVAL);
    }

    res_param = (struct pvr_decrypt_res_param  *)kmalloc(sizeof(struct pvr_decrypt_res_param), GFP_KERNEL);

    if(NULL == res_param)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-ENOMEM);
    }
    memset(res_param,0x00,sizeof(struct pvr_decrypt_res_param));
    res_param->decrypt_hdl = (unsigned int)(res_param);
    memcpy(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic));
    res_param->dsc_handle_num = capt_decrypt_param.decrypt_dsc_num;
    res_param->block_data_size = capt_decrypt_param.block_data_size;
    res_param->block_data_buf = NULL;
    res_param->key_item_info = NULL;

    res_param->key_item_info = (struct pvr_key_item *)kmalloc(sizeof(struct pvr_key_item)*res_param->dsc_handle_num, GFP_KERNEL);
    if(NULL == res_param->key_item_info)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        kfree(res_param);
        return(-ENOMEM);
    }

    //res_param->block_data_buf = (unsigned char   *)kmalloc(res_param->block_data_size, GFP_KERNEL);
    if(pvr_g_block_size != 0)/*for OTT certifcation*/
    {
        res_param->block_data_buf = (unsigned char   *)pvr_get_decrypt_block_buffer(res_param->block_data_size);
    }
    if(NULL == res_param->block_data_buf && pvr_g_block_size != 0)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        kfree(res_param->key_item_info);
        kfree(res_param);
        return(-ENOMEM);
    }

    down(&pvr_dev->sem);
    list_add(&res_param->list_head,&pvr_dev->list_head);
    up(&pvr_dev->sem);
    capt_decrypt_param.decrypt_hdl = res_param->decrypt_hdl;
    ret = copy_to_user((void *)arg,(void *)&capt_decrypt_param, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    return ret;
}


static int ali_pvr_release_decrypt_res
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int ret = 0;
    unsigned int decrypt_handle;
    struct pvr_decrypt_res_param  *res_param = NULL;
    struct list_head *pos = NULL;
    int find= 0;
    decrypt_handle  = arg;

    if((NULL == pvr_dev) || (NULL == file))
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }

    down(&pvr_dev->sem);
    list_for_each(pos,&pvr_dev->list_head)
    {
        res_param = container_of(pos,struct pvr_decrypt_res_param,list_head);
        if((res_param != NULL) && (res_param->decrypt_hdl == decrypt_handle) &&
           (!memcmp(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic)))
          )
        {
            list_del(pos);
            kfree(res_param->key_item_info);
            res_param->key_item_info = NULL;
            //kfree(res_param->block_data_buf);
            if(res_param->block_data_buf) /*for OTT certifcation*/
            {
                pvr_free_decrypt_block_buffer(res_param->block_data_buf);
                res_param->block_data_buf = NULL;
            }
            kfree(res_param);
            res_param = NULL;
            find = 1;
            break;
        }
    }
    up(&pvr_dev->sem);

    if(1 != find)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        ret = -1;
    }
    return ret;
}


static int _ali_pvr_set_decrytp_res_to_key_item
(
    struct ali_pvr_set_decrypt_res_param *res_param,
    struct pvr_key_item  *key_item
)
{
    struct ca_session_attr  dsc_ca_attr;
    struct ca_key_attr      dsc_key_attr;
#ifdef CONFIG_ALI_SEC
    struct sec_ca_attr sec_attr;
    struct sec_ca_key_attr sec_key_attr;
#endif
    int  ret = 0;
    int  ret2 = 0;
    if(res_param != NULL && key_item != NULL)
    {
        key_item->switch_block_count = res_param->decrypt_switch_block;
#ifdef CONFIG_ALI_SEC
        if(sec_check_is_sec_fd((__s32)res_param->dsc_fd))
        {
            memset(&sec_attr,0x00,sizeof(struct sec_ca_attr));
            ret = sec_get_session_attr(res_param->dsc_fd,&sec_attr);
            memset(&sec_key_attr,0x00,sizeof(struct sec_ca_key_attr));
            ret2 = sec_get_key_attr(res_param->dsc_fd,&sec_key_attr);
           
            if(ret == 0 && ret2 == 0)
            {

                key_item->decrypt_evo_info.algo         =  sec_attr.crypt_mode;
                key_item->decrypt_evo_info.stream_id    = sec_attr.stream_id;
                key_item->decrypt_evo_info.dev          = sec_attr.sub_dev_see_hdl;
                key_item->switch_block_count            = res_param->decrypt_switch_block;

                key_item->decrypt_evo_info.dsc_dma_mode = PURE_DATA_MODE;

                key_item->decrypt_evo_info.key_handle = sec_key_attr.key_handle;

                if(AES == key_item->decrypt_evo_info.algo)
                {
                    key_item->decrypt_evo_info.iv_length =16;
                }
                else if(DES == key_item->decrypt_evo_info.algo || TDES == key_item->decrypt_evo_info.algo)
                {
                    key_item->decrypt_evo_info.iv_length = 8;
                }
                else
                {
                    PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
                    ret = -7;
                }
                

                memcpy(key_item->decrypt_evo_info.iv_data,sec_key_attr.iv_ctr, key_item->decrypt_evo_info.iv_length);
                key_item->decrypt_evo_info.iv_parity = KEY_IV_MODE_EVEN;

            }
            else
            {
                PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
                ret = -6;
            }            
        }
        else
#endif
        {
        memset(&dsc_ca_attr,0x00,sizeof(struct ca_session_attr));
        ret = ca_dsc_get_session_attr(res_param->dsc_fd,&dsc_ca_attr);
        memset(&dsc_key_attr,0x00,sizeof(struct ca_key_attr));
        ret2 = ca_dsc_get_key_attr(res_param->dsc_fd,res_param->key_handle,&dsc_key_attr);



        if(ret == 0 && ret2 == 0)
        {

            key_item->decrypt_evo_info.algo         =  dsc_ca_attr.crypt_mode;
            key_item->decrypt_evo_info.stream_id    = dsc_ca_attr.stream_id;
            key_item->decrypt_evo_info.dev          = dsc_ca_attr.sub_dev_see_hdl;
            key_item->switch_block_count            = res_param->decrypt_switch_block;
            if(res_param->dsc_mode == BLOCK_DATA_MODE_PURE_DATA)
            {
                key_item->decrypt_evo_info.dsc_dma_mode    = res_param->dsc_mode;
            }
            else
            {
                key_item->decrypt_evo_info.dsc_dma_mode    = 1<<24;
            }
            key_item->decrypt_evo_info.key_handle = dsc_key_attr.key_handle;

            if(AES == key_item->decrypt_evo_info.algo)
            {
                if(res_param->iv_lenth > ALGO_AES_IV_LENGTH)
                {
                    PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
                    return -4;
                }
            }
            else if(DES == key_item->decrypt_evo_info.algo || TDES == key_item->decrypt_evo_info.algo)
            {
                if(res_param->iv_lenth > ALGO_DES_IV_LENGTH)
                {
                    PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
                    return -3;
                }
            }

            if(res_param->iv_data != NULL)
            {
                memcpy(key_item->decrypt_evo_info.iv_data,res_param->iv_data,
                       (res_param->iv_lenth > sizeof(key_item->decrypt_evo_info.iv_data)?
                        sizeof(key_item->decrypt_evo_info.iv_data):res_param->iv_lenth)
                      );
            }
            key_item->decrypt_evo_info.iv_length =
                (res_param->iv_lenth > sizeof(key_item->decrypt_evo_info.iv_data)?
                 sizeof(key_item->decrypt_evo_info.iv_data):res_param->iv_lenth);
            key_item->decrypt_evo_info.iv_parity = res_param->iv_parity;

        }
        else
        {
            PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
            ret = -2;
        }
    }
    }
    else
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        ret = -1;
    }
    return ret;

}


static int ali_pvr_set_decrypt_res
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int ret = 0;
    struct ali_pvr_set_decrypt_res_param set_decrypt_param;
    struct pvr_decrypt_res_param  *res_param = NULL;
    unsigned char *k_buf = NULL;
    struct list_head *pos = NULL;
    int set_ok = 0;
    int i;
    if((NULL == pvr_dev) || (NULL == file))
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    memset(&set_decrypt_param,0x00,sizeof(struct ali_pvr_capt_decrypt_res_param));
    ret = copy_from_user((void *)&set_decrypt_param, (void *)arg, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    k_buf = (unsigned char *)kmalloc(set_decrypt_param.iv_lenth + 1,GFP_KERNEL);
    if(k_buf != NULL)
    {
        if((set_decrypt_param.iv_lenth > 0) && (set_decrypt_param.iv_data != NULL))
        {
            ret = copy_from_user((void *)k_buf, (void *)(set_decrypt_param.iv_data), set_decrypt_param.iv_lenth);
            if(0 != ret)
            {
                PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
                kfree(k_buf);
                k_buf = NULL;
                return(-EFAULT);
            }
            set_decrypt_param.iv_data = k_buf;
        }

    }
    down(&pvr_dev->sem);
    list_for_each(pos,&pvr_dev->list_head)
    {
        res_param = container_of(pos,struct pvr_decrypt_res_param,list_head);
        if((res_param != NULL) && (res_param->decrypt_hdl == set_decrypt_param.decrypt_hdl) &&
           (!memcmp(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic)))
          )
        {
            for(i = 0; i < res_param->dsc_handle_num; i++)
            {
                if(i == set_decrypt_param.decrypt_index)
                {
                    ret = _ali_pvr_set_decrytp_res_to_key_item(&set_decrypt_param,res_param->key_item_info + i);
                    if(ret == 0)
                    {
                        set_ok = 1;
                    }
                    break;
                }
            }
            break;
        }
    }
    up(&pvr_dev->sem);

    if(1 != set_ok)
    {
        ret = -1;
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
    }
    if(NULL != k_buf)
    {
        kfree(k_buf);
        k_buf = NULL;
    }
    return ret;
}

int ali_pvr_decrytp_raw_evo
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int rpc_cmd = PVR_RPC_IO_RAW_DECRYPT_EVO;
    int ret = 0;
    struct ali_pvr_data_decrypt_param  raw_decrypt_param;
    struct pvr_key_item   *p_item = NULL;
    struct pvr_decrypt_res_param  *res_param = NULL;
    struct list_head *pos = NULL;
    int i = 0,use_key_pos = 0xFFFFFFFF;
    unsigned int block_dif = 0xFFFFFFFF;
    struct pvr_vm_node *p_vm_node = &pvr_dev->vm_area_node;
#if defined(CONFIG_ALI_RPCNG)
    Param p1,p2;
#endif
    PVR_decrypt_evo decrypt_evo;

    memset(&raw_decrypt_param,0x00,sizeof(struct ali_pvr_data_decrypt_param));
    ret = copy_from_user((void *)&raw_decrypt_param, (void *)arg, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }

    if((NULL == pvr_dev) || (NULL == file))
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    down(&pvr_dev->sem);
    list_for_each(pos,&pvr_dev->list_head)
    {
        res_param = container_of(pos,struct pvr_decrypt_res_param,list_head);
        if((res_param != NULL) && (res_param->decrypt_hdl == raw_decrypt_param.decrypt_hdl) &&
           (!memcmp(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic))))
        {
            if(res_param->dsc_handle_num> 0)
            {
                for(i = 0; i < res_param->dsc_handle_num; i++)
                {
                    p_item = res_param->key_item_info + i;
                    if(((raw_decrypt_param.block_index < p_item->switch_block_count)
                        && ((p_item->switch_block_count - raw_decrypt_param.block_index) < block_dif))
                       || ((p_item->switch_block_count == 0xFFFFFFFF ) && (raw_decrypt_param.block_index == 0)))   //speical case.
                    {
                        block_dif = (p_item->switch_block_count - raw_decrypt_param.block_index);
                        use_key_pos = i;
                    }
                }
                if(use_key_pos != 0xFFFFFFFF)
                {
                    break;
                }
            }
            else
            {
                ret = -EINVAL;
                break;
            }
        }
    }
    up(&pvr_dev->sem);
    if((use_key_pos == 0xFFFFFFFF) || (use_key_pos > res_param->dsc_handle_num))
    {
        ret = -EINVAL;
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return ret;
    }
    if(((raw_decrypt_param.length > res_param->block_data_size) && res_param->block_data_size != 0)
       || (((raw_decrypt_param.length % TS_PACKET_SIZE) != 0) && (raw_decrypt_param.des_flag == PVR_OTT_DATA_DMX)
          && (pvr_g_block_size != 0))
       || (raw_decrypt_param.length == 0 ) )
    {
        ret = -EINVAL;
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return ret;
    }
    /*mmap buffer*/
    if((p_vm_node->vma != NULL))
    {
        if (((unsigned long)raw_decrypt_param.input >= p_vm_node->vm_start)
            && (((unsigned long)raw_decrypt_param.input + (unsigned long)raw_decrypt_param.length) <= p_vm_node->vm_end))
        {
            decrypt_evo.input = (u8 *)(p_vm_node->vm_kaddr + (unsigned long)raw_decrypt_param.input - p_vm_node->vm_start);
        }
        else
        {
            return -EINVAL;
        }
    }
    else if (res_param->block_data_buf != NULL)  /*malloc buffer*/
    {
        ret = copy_from_user((void *)res_param->block_data_buf , (void *)(raw_decrypt_param.input), raw_decrypt_param.length);
        if(0 != ret)
        {
            PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
            return(-EFAULT);
        }
        decrypt_evo.input = res_param->block_data_buf;
    }
    else
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    p_item = res_param->key_item_info + use_key_pos;
    decrypt_evo.algo      = p_item->decrypt_evo_info.algo;
    decrypt_evo.data_mode = p_item->decrypt_evo_info.dsc_dma_mode;
    decrypt_evo.dev       = p_item->decrypt_evo_info.dev;
    memcpy(decrypt_evo.iv_data,p_item->decrypt_evo_info.iv_data,p_item->decrypt_evo_info.iv_length);
    decrypt_evo.iv_length = p_item->decrypt_evo_info.iv_length;
    if(decrypt_evo.iv_length > 0)
    {
        __CACHE_FLUSH_ALI((decrypt_evo.iv_data), (decrypt_evo.iv_length));
    }
    decrypt_evo.iv_parity = p_item->decrypt_evo_info.iv_parity;
    decrypt_evo.length    = raw_decrypt_param.length;
    decrypt_evo.stream_id = p_item->decrypt_evo_info.stream_id;
    decrypt_evo.key_handle= p_item->decrypt_evo_info.key_handle;
    decrypt_evo.block_indicator = raw_decrypt_param.block_vob_indicator;
    decrypt_evo.decv_id = raw_decrypt_param.decv_id;
    if (pvr_g_block_size > 0)
    {
        decrypt_evo.des_flag = PVR_OTT_DATA_DMX;
        memcpy(decrypt_evo.iv_data,p_item->decrypt_evo_info.iv_data,p_item->decrypt_evo_info.iv_length);
        decrypt_evo.iv_length = p_item->decrypt_evo_info.iv_length;
    }
    else
    {
        decrypt_evo.des_flag = raw_decrypt_param.des_flag;
        if(raw_decrypt_param.iv_length > 0)
        {
            memcpy(decrypt_evo.iv_data,raw_decrypt_param.iv_data,raw_decrypt_param.iv_length);
        }
        decrypt_evo.iv_length = raw_decrypt_param.iv_length;
    }
    __CACHE_FLUSH_ALI((decrypt_evo.input), (decrypt_evo.length));

    cmd = rpc_cmd;
#if !defined(CONFIG_ALI_RPCNG)
    ret = pvr_decrytp_raw_evo(cmd,&decrypt_evo);
#else
    RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
    RPC_PARAM_UPDATE(p2, PARAM_IN, PRRAM_PVR_DECRYPT_EVO, sizeof(PVR_decrypt_evo), &decrypt_evo);
    ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif

    return ret;
}

int ali_pvr_decrytp_raw_evo_sub
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int rpc_cmd = PVR_RPC_IO_RAW_DECRYPT_EVO_SUB;
    int ret = 0;
    struct ali_pvr_data_decrypt_param  raw_decrypt_param;
    struct pvr_key_item   *p_item = NULL;
    struct pvr_decrypt_res_param  *res_param = NULL;
    struct list_head *pos = NULL;
    int i = 0,use_key_pos = 0xFFFFFFFF;
    unsigned int block_dif = 0xFFFFFFFF;
    struct pvr_vm_node *p_vm_node = &pvr_dev->vm_area_node;
#if defined(CONFIG_ALI_RPCNG)
    Param p1,p2;
#endif
    PVR_decrypt_evo decrypt_evo;
    memset(&raw_decrypt_param,0x00,sizeof(struct ali_pvr_data_decrypt_param));
    ret = copy_from_user((void *)&raw_decrypt_param, (void *)arg, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        printk("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }

    if((NULL == pvr_dev) || (NULL == file))
    {
        printk("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    down(&pvr_dev->sem);
    list_for_each(pos,&pvr_dev->list_head)
    {
        res_param = container_of(pos,struct pvr_decrypt_res_param,list_head);
        if((res_param != NULL) && (res_param->decrypt_hdl == raw_decrypt_param.decrypt_hdl) &&
           (!memcmp(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic))))
        {
            if(res_param->dsc_handle_num> 0)
            {
                for(i = 0; i < res_param->dsc_handle_num; i++)
                {
                    p_item = res_param->key_item_info + i;
                    if(((raw_decrypt_param.block_index < p_item->switch_block_count)
                        && ((p_item->switch_block_count - raw_decrypt_param.block_index) < block_dif))
                       || ((p_item->switch_block_count == 0xFFFFFFFF ) && (raw_decrypt_param.block_index == 0)))   //speical case.
                    {
                        block_dif = (p_item->switch_block_count - raw_decrypt_param.block_index);
                        use_key_pos = i;
                    }
                }
                if(use_key_pos != 0xFFFFFFFF)
                {
                    break;
                }
            }
            else
            {
			    printk("%s,line:%d\n", __FUNCTION__, __LINE__);
                ret = -EINVAL;
                break;
            }
        }
    }
    up(&pvr_dev->sem);
    if((use_key_pos == 0xFFFFFFFF) || (use_key_pos > res_param->dsc_handle_num))
    {
        ret = -EINVAL;
        printk("%s,line:%d\n", __FUNCTION__, __LINE__);
        return ret;
    }
    if(((raw_decrypt_param.length > res_param->block_data_size) && res_param->block_data_size != 0)
       || (((raw_decrypt_param.length % TS_PACKET_SIZE) != 0) && (raw_decrypt_param.des_flag == PVR_OTT_DATA_DMX)
          && (pvr_g_block_size != 0))
       || (raw_decrypt_param.length == 0 ) )
    {
        ret = -EINVAL;
        printk("%s,line:%d\n", __FUNCTION__, __LINE__);
        return ret;
    }
    /*mmap buffer*/
    if((p_vm_node->vma != NULL))
    {
        if (((unsigned long)raw_decrypt_param.input >= p_vm_node->vm_start)
            && (((unsigned long)raw_decrypt_param.input + (unsigned long)raw_decrypt_param.length) <= p_vm_node->vm_end))
        {
            decrypt_evo.input = (u8 *)(p_vm_node->vm_kaddr + (unsigned long)raw_decrypt_param.input - p_vm_node->vm_start);
        }
        else
        {
            printk(KERN_ERR "%s,line:%d,p_vm_node:%p,p_vm_node->vm_kaddr:%p,vma->vm_end:%p,vma->vm_start:%p :%p :%lu\n", __FUNCTION__, __LINE__,
                                 (void *)p_vm_node ,(void *)p_vm_node->vm_kaddr,(void *)p_vm_node->vm_end,(void *)p_vm_node->vm_start,raw_decrypt_param.input,raw_decrypt_param.length);
            return -EINVAL;
        }
    }
    else if (res_param->block_data_buf != NULL)  /*malloc buffer*/
    {
        ret = copy_from_user((void *)res_param->block_data_buf , (void *)(raw_decrypt_param.input), raw_decrypt_param.length);
        if(0 != ret)
        {
            printk("%s,line:%d\n", __FUNCTION__, __LINE__);
            return(-EFAULT);
        }
        decrypt_evo.input = res_param->block_data_buf;
    }
    else
    {
        printk("%s,line:%d\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    p_item = res_param->key_item_info + use_key_pos;
    decrypt_evo.algo      = p_item->decrypt_evo_info.algo;
    decrypt_evo.data_mode = p_item->decrypt_evo_info.dsc_dma_mode;
    decrypt_evo.dev       = p_item->decrypt_evo_info.dev;
    memcpy(decrypt_evo.iv_data,p_item->decrypt_evo_info.iv_data,p_item->decrypt_evo_info.iv_length);
    decrypt_evo.iv_length = p_item->decrypt_evo_info.iv_length;
    if(decrypt_evo.iv_length > 0)
    {
        __CACHE_FLUSH_ALI((decrypt_evo.iv_data), (decrypt_evo.iv_length));
    }
    decrypt_evo.iv_parity = p_item->decrypt_evo_info.iv_parity;
    decrypt_evo.length    = raw_decrypt_param.length;
    decrypt_evo.stream_id = p_item->decrypt_evo_info.stream_id;
    decrypt_evo.key_handle= p_item->decrypt_evo_info.key_handle;
    decrypt_evo.block_indicator = raw_decrypt_param.block_vob_indicator;
    decrypt_evo.decv_id = raw_decrypt_param.decv_id;
    if (pvr_g_block_size > 0)
    {
        decrypt_evo.des_flag = PVR_OTT_DATA_DMX;
        memcpy(decrypt_evo.iv_data,p_item->decrypt_evo_info.iv_data,p_item->decrypt_evo_info.iv_length);
        decrypt_evo.iv_length = p_item->decrypt_evo_info.iv_length;
    }
    else
    {
        decrypt_evo.des_flag = raw_decrypt_param.des_flag;
        if(raw_decrypt_param.iv_length > 0)
        {
            memcpy(decrypt_evo.iv_data,raw_decrypt_param.iv_data,raw_decrypt_param.iv_length);
        }
        decrypt_evo.iv_length = raw_decrypt_param.iv_length;
    }
    __CACHE_FLUSH_ALI((decrypt_evo.input), (decrypt_evo.length));

    cmd = rpc_cmd;

#if !defined(CONFIG_ALI_RPCNG)
    ret = pvr_decrytp_raw_evo_sub(cmd,&decrypt_evo);
#else
    RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
    RPC_PARAM_UPDATE(p2, PARAM_IN, PRRAM_PVR_DECRYPT_EVO, sizeof(PVR_decrypt_evo), &decrypt_evo);
    ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif

    return ret;
}

#if 0
int ali_pvr_decrytp_raw_es_evo
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int rpc_cmd = PVR_RPC_IO_RAW_DECRYPT_EVO;
    int ret = 0;
    struct ali_pvr_data_decrypt_param  raw_decrypt_param;
    struct pvr_key_item   *p_item = NULL;
    struct pvr_decrypt_res_param  *res_param = NULL;
    struct list_head *pos = NULL;
    int use_key_pos = 0xFFFFFFFF;
    struct pvr_vm_node *p_vm_node = NULL;
    int malloc_flag = 0;
#if defined(CONFIG_ALI_RPCNG)
    Param p1,p2;
#endif
    PVR_decrypt_evo decrypt_evo;

    memset(&raw_decrypt_param,0x00,sizeof(struct ali_pvr_data_decrypt_param));
    ret = copy_from_user((void *)&raw_decrypt_param, (void *)arg, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }

    if((NULL == pvr_dev) || (NULL == file))
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    down(&pvr_dev->sem);
    list_for_each(pos,&pvr_dev->list_head)
    {
        res_param = container_of(pos,struct pvr_decrypt_res_param,list_head);
        if((res_param != NULL) && (res_param->decrypt_hdl == raw_decrypt_param.decrypt_hdl) &&
           (!memcmp(res_param->magic,decrypt_res_magic,sizeof(decrypt_res_magic)))
          )
        {
            if(res_param->dsc_handle_num == 1)
            {
                use_key_pos = 0;
            }
            else
            {
                ret = -EINVAL;
                break;
            }
        }
    }
    up(&pvr_dev->sem);
    if((use_key_pos == 0xFFFFFFFF) || (use_key_pos > res_param->dsc_handle_num))
    {
        ret = -EINVAL;
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return ret;
    }
#if 0
    if((raw_decrypt_param.length > PVR_MMAP_SIZE) || ((raw_decrypt_param.length % TS_PACKET_SIZE) != 0)
       || (raw_decrypt_param.length == 0 ) )
    {
        ret = -EINVAL;
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return ret;
    }
#endif
    /*mmap buffer*/
    if((p_vm_node->vma != NULL))
        if (((unsigned long)raw_decrypt_param.input >= p_vm_node->vm_start)
            && (((unsigned long)raw_decrypt_param.input + (unsigned long)raw_decrypt_param.length) <= p_vm_node->vm_end))
        {
            decrypt_evo.input = (u8 *)(p_vm_node->vm_kaddr + (unsigned long)raw_decrypt_param.input - p_vm_node->vm_start);
        }
        else
        {
            return -EINVAL;
        }
    else
    {
        decrypt_evo.input = kmalloc(raw_decrypt_param.length, GFP_KERNEL);
        if(decrypt_evo.input)
        {
            return -ENOMEM;
        }
        malloc_flag = 1;
        ret = copy_from_user((void *)decrypt_evo.input, (void *)(raw_decrypt_param.input), raw_decrypt_param.length);
        if (ret)
        {
            PVR_PRF("copy user data error!\n");
            kfree(decrypt_evo.input);
        }
    }
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }
    p_item = res_param->key_item_info + use_key_pos;
    decrypt_evo.algo      = p_item->decrypt_evo_info.algo;
    decrypt_evo.data_mode = p_item->decrypt_evo_info.dsc_dma_mode;
    decrypt_evo.dev       = p_item->decrypt_evo_info.dev;

    decrypt_evo.iv_length = raw_decrypt_param.iv_length;
    if(decrypt_evo.iv_length > 0)
    {
        memcpy((void*)decrypt_evo.iv_data, (void*)raw_decrypt_param.iv_data, raw_decrypt_param.iv_length);
        __CACHE_FLUSH_ALI((decrypt_evo.iv_data), (decrypt_evo.iv_length));
    }
    decrypt_evo.iv_parity = p_item->decrypt_evo_info.iv_parity;
    decrypt_evo.length    = raw_decrypt_param.length;
    decrypt_evo.stream_id = p_item->decrypt_evo_info.stream_id;
    decrypt_evo.key_handle= p_item->decrypt_evo_info.key_handle;
    decrypt_evo.block_indicator = raw_decrypt_param.block_vob_indicator;
    decrypt_evo.des_flag = raw_decrypt_param.des_flag;
    decrypt_evo.decv_id = raw_decrypt_param.decv_id;
    __CACHE_FLUSH_ALI((decrypt_evo.input), (decrypt_evo.length));

    cmd = rpc_cmd;
#if !defined(CONFIG_ALI_RPCNG)
    ret = pvr_decrytp_raw_evo(cmd,&decrypt_evo);
#else
    RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
    RPC_PARAM_UPDATE(p2, PARAM_IN, PRRAM_PVR_DECRYPT_EVO, sizeof(PVR_decrypt_evo), &decrypt_evo);
    ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
    if(malloc_flag)
    {
        kfree(decrypt_evo.input);
    }
    return ret;
}
#endif

int ali_pvr_capture_pvr_key
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int rpc_cmd = PVR_RPC_IO_CAPTURE_PVR_KEY;
    int ret = 0;
    struct pvr_enc_param *temp = NULL;
    struct pvr_enc_param *pvalid = NULL;
    struct list_head *pos = NULL;
    int ret_num = 0;
#if defined(CONFIG_ALI_RPCNG)
	Param p1,p2;
#endif
    PVR_KEY_PARAM   key_param;
    struct ali_pvr_set_pvr_rec_key    pvr_set_key;
    
    memset(&key_param,0x00,sizeof(PVR_KEY_PARAM));
    memset(&pvr_set_key,0x00,sizeof(struct ali_pvr_set_pvr_rec_key));
    
    ret = copy_from_user((void *)&pvr_set_key, (void *)arg, _IOC_SIZE(cmd));
    if((pvr_set_key.key_item_len == 0) || (pvr_set_key.key_item_num == 0)) 
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);        
    }
    
    if((pvr_set_key.block_data_size % 188) != 0)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);             
    }
    down(&pvr_dev->sem);
    list_for_each(pos,&pvr_dev->list_head) 
    {
        temp = container_of(pos,struct pvr_enc_param,list_head);
        if(temp->only_flag == file) 
        {
            pvalid = temp;    
        }
    }
    up(&pvr_dev->sem);
    
    if(pvalid == NULL)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);           
    }    
    key_param.input_key = kmalloc(pvr_set_key.key_item_len * pvr_set_key.key_item_num, GFP_KERNEL);
    if(key_param.input_key == NULL)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);           
    }
    ret = copy_from_user((void *)key_param.input_key , (void *)(pvr_set_key.input_key), pvr_set_key.key_item_len * pvr_set_key.key_item_num);
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        if(key_param.input_key != NULL)
        {
            kfree(key_param.input_key);
        }
        return(-EFAULT);
    }

    __CACHE_FLUSH_ALI((key_param.input_key), (pvr_set_key.key_item_len * pvr_set_key.key_item_num));
    key_param.valid_key_num         = pvr_set_key.key_item_num;
    key_param.pvr_key_length        = pvr_set_key.key_item_len;
    key_param.qn_per_key            = pvr_set_key.qn_per_key;
    key_param.quantum_size          = pvr_set_key.block_data_size;
    key_param.stream_id             = pvalid->enc_param.stream_id;
    
    
#if !defined(CONFIG_ALI_RPCNG)
	ret = pvr_capture_pvr_key(rpc_cmd,&key_param);
#else
    //RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
    //RPC_PARAM_UPDATE(p2, PARAM_IN, PRRAM_PVR_DECRYPT_EVO, sizeof(PVR_decrypt_evo), &decrypt_evo);
    //ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        if(key_param.input_key != NULL)
        {
            kfree(key_param.input_key);
        }
        return(-EFAULT);        
    }
    PVR_PRF("========%s,line:%d ret_num:%d\n", __FUNCTION__, __LINE__,pvr_set_key.stream_id);
    pvr_set_key.stream_id = pvalid->enc_param.stream_id;
    PVR_PRF("%s,line:%d pvr_set_key.stream_id:%d\n", __FUNCTION__, __LINE__,pvr_set_key.stream_id);
    ret_num = copy_to_user((void *)arg, (void *)(&pvr_set_key),_IOC_SIZE(cmd));
    PVR_PRF("%s,line:%d ret_num:%d stream_id:%d\n", __FUNCTION__, __LINE__,ret_num,pvr_set_key.stream_id);
    if(key_param.input_key != NULL)
    {
        kfree(key_param.input_key);
    }    
    return ret;
}



int ali_pvr_release_pvr_key
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int rpc_cmd = PVR_RPC_IO_RELEASE_PVR_KEY;
    int ret = 0;
    
#if defined(CONFIG_ALI_RPCNG)
	Param p1,p2;
#endif
   

    
#if !defined(CONFIG_ALI_RPCNG)
	ret = pvr_release_pvr_key(rpc_cmd,arg);
#else
    //RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
    //RPC_PARAM_UPDATE(p2, PARAM_IN, PRRAM_PVR_DECRYPT_EVO, sizeof(PVR_decrypt_evo), &decrypt_evo);
    //ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
    return ret;
}


int ali_pvr_playback_set_key
(
    struct pvr_dev *pvr_dev,
    struct file *file,
    unsigned int cmd,
    unsigned long arg
)
{
    int rpc_cmd = PVR_RPC_IO_PVR_PLAYBCK_SET_KEY;
    int ret  = 0;
    int ret1 = 0;
    int ret2 = 0;
#if defined(CONFIG_ALI_RPCNG)
	Param p1,p2;
#endif

    struct ali_pvr_set_pvr_plyback_key  pvr_playback_key;
    struct PVR_BLOCK_ENC_PARAM pvr_key_param;
    struct ca_key_attr         ca_key;
    struct ca_session_attr     ca_session;
    
    memset(&pvr_key_param,0x00,sizeof(struct PVR_BLOCK_ENC_PARAM));
    memset(&pvr_playback_key,0x00,sizeof(struct ali_pvr_set_pvr_plyback_key));
    memset(&ca_key,0x00,sizeof(struct ca_key_attr));
    memset(&ca_session,0x00,sizeof(struct ca_session_attr));
    
    ret = copy_from_user((void *)&pvr_playback_key, (void *)arg, _IOC_SIZE(cmd));
    if(0 != ret)
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);
    }    
    
    ret1 = ca_dsc_get_session_attr(pvr_playback_key.dsc_fd,&ca_session);
    if((pvr_playback_key.pvr_key_param.root_key_pos != 0xff) &&(pvr_playback_key.kl_fd != -1))
    {
        ret2 = ca_dsc_get_key_attr(pvr_playback_key.dsc_fd,pvr_playback_key.pvr_key_param.key_handle,&ca_key);
    }    
    if((ret1 != 0) || (ret2 != 0))
    {
        PVR_PRF("%s,line:%d\n", __FUNCTION__, __LINE__);
        return(-EFAULT);        
    }
    memcpy(&pvr_key_param,&pvr_playback_key.pvr_key_param,sizeof(struct PVR_BLOCK_ENC_PARAM));
    pvr_key_param.key_handle = ca_key.key_handle;
    pvr_key_param.target_key_pos = ca_key.key_pos;
    
    

#if !defined(CONFIG_ALI_RPCNG)
	ret = pvr_playback_set_key(rpc_cmd,&pvr_key_param);
#else
    //RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
    //RPC_PARAM_UPDATE(p2, PARAM_IN, PRRAM_PVR_DECRYPT_EVO, sizeof(PVR_decrypt_evo), &decrypt_evo);
    //ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
    return ret;    
}


#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
static long pvr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
static int pvr_ioctl(struct inode *node, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    struct pvr_dev *pvr_dev_p = file->private_data;
    int ret = 0;
    unsigned long retNum = 0;
    PVR_BLOCK_ENC_PARAM input_enc_param;
    struct ca_session_attr decrypt_ca_session_attr;
    struct pvr_enc_param *temp = NULL,*pvalid = NULL;
    struct list_head *pos = NULL;
#if defined(CONFIG_ALI_RPCNG)
    Param p1,p2;
#endif

    //int i = 0;
    if(pvr_dev_p == NULL)
    {
        PVR_PRF("file not  be opened.\n");
        return -EBADF;
    }
    down(&m_pvr_sem);
    if(pvr_dev_p->open_count <= 0)
    {
        up(&m_pvr_sem);

        PVR_PRF("PVR driver don't be opened.\n");
        return -1;
    }

    switch(cmd)
    {
        case PVR_IO_DECRYPT:
        {
            PVR_RPC_RAW_DECRYPT decrypt_info;
            retNum = copy_from_user((void *)&decrypt_info, (void *)arg, sizeof(decrypt_info));

            if((decrypt_info.dev != 0)&& (decrypt_info.stream_id == 0))
            {
            #ifdef CONFIG_ALI_SEC
                if(sec_check_is_sec_fd((__s32)(decrypt_info.dev)))
                {
                    struct sec_ca_attr decrypt_ca_session_attr;
                    memset(&decrypt_ca_session_attr,0,sizeof(decrypt_ca_session_attr));
                    sec_get_session_attr((__s32)(decrypt_info.dev), &decrypt_ca_session_attr);
                    PVR_PRF("PVR_IO_DECRYPT fd=(%d), algo(%d), stream_id(%d), dev(0x%08x) length(0x%08x) \n",
                        (int)decrypt_info.dev,
                        decrypt_ca_session_attr.crypt_mode,
                        decrypt_ca_session_attr.stream_id,
                        (unsigned int)decrypt_ca_session_attr.sub_dev_see_hdl,
                        (unsigned int)decrypt_info.length);
                     
                    decrypt_info.algo=decrypt_ca_session_attr.crypt_mode;
                    decrypt_info.stream_id = decrypt_ca_session_attr.stream_id;
                    decrypt_info.dev = decrypt_ca_session_attr.sub_dev_see_hdl;
                }
                else
            #endif
                {
                struct ca_session_attr decrypt_ca_session_attr;
                memset(&decrypt_ca_session_attr,0x00,sizeof(struct ca_session_attr));

                ca_dsc_get_session_attr((__s32)(decrypt_info.dev), &decrypt_ca_session_attr);
                PVR_PRF("PVR_IO_DECRYPT fd=(%d), stream_id(%d), dev(0x%08x) length(0x%08x) \n",
                        (int)decrypt_info.dev,
                        decrypt_ca_session_attr.stream_id,
                        (unsigned int)decrypt_ca_session_attr.sub_dev_see_hdl,
                        (unsigned int)decrypt_info.length);

                decrypt_info.stream_id = decrypt_ca_session_attr.stream_id;
                decrypt_info.dev = decrypt_ca_session_attr.sub_dev_see_hdl;
                }

            }
            if (0 != retNum)
            {
                PVR_PRF("Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }
            if (pvr_decrypt_buffer_size != decrypt_info.length)
            {
                PVR_PRF("Error decrypt length [%u]\n", (__u32)decrypt_info.length);
                ret = -1;
                goto EXIT;
            }
            retNum = copy_from_user((void *)pvr_decrypt_buffer, (void *)decrypt_info.input, decrypt_info.length);
            if (0 != retNum)
            {
                PVR_PRF("Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }

            /* convert to the kernel address */
            decrypt_info.input = pvr_decrypt_buffer;
            //printk(KERN_ALERT "input addr [0x%08x]\n", decrypt_info.input);
            __CACHE_FLUSH_ALI((decrypt_info.input), (decrypt_info.length));
            cmd = PVR_RPC_IO_RAW_DECRYPT;
#if !defined(CONFIG_ALI_RPCNG)
            ret = pvr_raw_decrypt_main(cmd,&decrypt_info);
#else
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_PVR_decrypt, sizeof(PVR_decrypt), &decrypt_info);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
            break;
        }
        case PVR_IO_UPDATE_ENC_PARAMTOR:
        {

            PVR_PRF("PVR_RPC_IO_UPDATE_ENC_PARAMTOR command is set!\n");

            memset(&input_enc_param,0, sizeof(PVR_BLOCK_ENC_PARAM));
            retNum = copy_from_user((void *)&input_enc_param, (void *)arg, sizeof(PVR_BLOCK_ENC_PARAM));
            input_enc_param.kl_level = PVR_KL_THREE;
            if (0 != retNum)
            {
                PVR_PRF("Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }
            cmd = PVR_RPC_IO_UPDATE_ENC_PARAMTOR;
#if !defined(CONFIG_ALI_RPCNG)
            ret = pvr_update_paras(cmd,&input_enc_param);
#else
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_INOUT, PRRAM_PVR_UPDATE_ENC, sizeof(PVR_BLOCK_ENC_PARAM), &input_enc_param);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
            retNum = copy_to_user((void *)arg, (void *)&input_enc_param, sizeof(PVR_BLOCK_ENC_PARAM));
#endif
            PVR_PRF("RpcCallCompletion ,ret = %d.\n", ret);
            break;
        }
        case PVR_IO_SET_BLOCK_SIZE:
        {
            PVR_PRF("PVR_RPC_IO_SET_BLOCK_SIZE command is set!\n");
            if (( arg%188 != 0) && (((arg/188) % 64) != 0))
            {
                printk(KERN_ERR"PVR_RPC_IO_SET_BLOCK_SIZE block size is not quantum size, failed!\n");
                ret = -1;
                goto EXIT;
            }
            if((pvr_decrypt_buffer_size != arg) && (arg != 0))
            {
                pvr_decrypt_buffer_size = arg;
                pvr_decrypt_buffer_free();
                if(pvr_decrypt_buffer_allocate(pvr_decrypt_buffer_size)  != 0)
                {
                    printk(KERN_ERR"PVR_RPC_IO_SET_BLOCK_SIZE malloc buffer,failed!\n");
                    ret = -1;
                    goto EXIT;
                }
            }
            else if(arg == 0)
            {
                pvr_decrypt_buffer_size = 0;
            }
            cmd = PVR_RPC_IO_SET_BLOCK_SIZE;
            pvr_g_block_size = arg;
#if !defined(CONFIG_ALI_RPCNG)
            ret = pvr_set_block_size(cmd,arg);
#else
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_UINT32, 4, &arg);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
            break;
        }
        case PVR_IO_FREE_BLOCK:
        {

            PVR_RPC_RAW_DECRYPT decrypt_info;
            memset(&decrypt_info,0,sizeof(PVR_RPC_RAW_DECRYPT));
            decrypt_info.stream_id = (UINT32)arg;
            cmd = PVR_RPC_IO_FREE_BLOCK;
#if !defined(CONFIG_ALI_RPCNG)
            ret = pvr_free_resources(cmd, &decrypt_info);
#else
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_PVR_decrypt, sizeof(PVR_decrypt), &decrypt_info);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
            down(&pvr_dev_p->sem);
            list_for_each(pos,&pvr_dev_p->list_head) 
            {
                temp = container_of(pos,struct pvr_enc_param,list_head);
                if(temp->only_flag == file)
                {
                    list_del(pos);
                    kfree(temp);
                    break;
                }
            }
            up(&pvr_dev_p->sem);
            break;

        }
        case PVR_IO_START_BLOCK:
        {
            struct ca_key_attr  p_key_attr;
            PVR_PRF("PVR_RPC_IO_START_BLOCK command is set!\n");
            memset(&input_enc_param,0, sizeof(PVR_BLOCK_ENC_PARAM));
            memset(&decrypt_ca_session_attr,0x00,sizeof(struct ca_session_attr));
            memset(&p_key_attr,0xff,sizeof(struct ca_key_attr));
            retNum = copy_from_user((void *)&input_enc_param, (void *)arg, sizeof(PVR_BLOCK_ENC_PARAM));
            if (0 != retNum)
            {
                PVR_PRF("Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }

            ca_dsc_get_key_attr((__s32)(input_enc_param.sub_device_id),(__s32)(input_enc_param.key_handle), &p_key_attr);
            input_enc_param.key_handle = p_key_attr.key_handle;
            input_enc_param.target_key_pos = p_key_attr.key_pos;
            if((0 != input_enc_param.sub_device_id)&& (0xff == input_enc_param.stream_id))
            {
                ca_dsc_get_session_attr((__s32)(input_enc_param.sub_device_id), &decrypt_ca_session_attr);
                input_enc_param.stream_id = decrypt_ca_session_attr.stream_id;
                input_enc_param.sub_device_id = decrypt_ca_session_attr.sub_dev_id;
            }
			input_enc_param.kl_level = PVR_KL_THREE;
#if 0
            printk(KERN_ERR"PVR_IO_START_BLOCK parametor is :\n");

            printk(KERN_ERR"pid_num:(%d)\n",input_enc_param.pid_num);
            for(i=0; i<input_enc_param.pid_num; i++) {
                printk(KERN_ERR"\t %d-st pid:(%d)\n",i,input_enc_param.pid_list[i]);
            }

            printk(KERN_ERR"\t input_iv is ");
            for(i=0; i<16; i++) {
                printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_iv[i]);
            }
            printk(KERN_ERR"\n");


            printk(KERN_ERR"\t input_key is ");
            for(i=0; i<48; i++) {
                printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_key[i]);
            }
            printk(KERN_ERR"\n");

            printk(KERN_ERR"\t dsc_sub_device:(0x%08x)\n",input_enc_param.dsc_sub_device);
            printk(KERN_ERR"\t key_handle:(0x%08x)\n",input_enc_param.key_handle);
            printk(KERN_ERR"\t key_mode:(0x%08x)\n",input_enc_param.key_mode);
            printk(KERN_ERR"\t kl_mode:(0x%08x)\n",input_enc_param.kl_mode);
            printk(KERN_ERR"\t residue_mode:(0x%08x)\n",input_enc_param.residue_mode);
            printk(KERN_ERR"\t root_key_pos:(0x%08x)\n",input_enc_param.root_key_pos);
            printk(KERN_ERR"\t source_mode:(0x%08x)\n",input_enc_param.source_mode);
            printk(KERN_ERR"\t stream_id:(0x%08x)\n",input_enc_param.stream_id);
            printk(KERN_ERR"\t sub_device_id:(0x%08x)\n",input_enc_param.sub_device_id);
            printk(KERN_ERR"\t target_key_pos:(0x%08x)\n",input_enc_param.target_key_pos);
            printk(KERN_ERR"\t work_mode:(0x%08x)\n",input_enc_param.work_mode);
            printk(KERN_ERR"\t request_res:(0x%08x)\n",input_enc_param.request_res);
#endif
#if !defined(CONFIG_ALI_RPCNG)
            cmd = PVR_RPC_IO_START_BLOCK;
            ret = pvr_start_block_rec(cmd, &input_enc_param);
            if(ret != 0)
            {
                PVR_PRF("pvr start block record error!\n");
                ret = -1;
                goto EXIT;
            }
            cmd = PVR_RPC_IO_UPDATE_ENC_PARAMTOR;   //update the key right now
            ret |= pvr_update_paras(cmd,&input_enc_param);
#else
            cmd = PVR_RPC_IO_START_BLOCK;
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_OUT, PRRAM_PVR_UPDATE_ENC, sizeof(PVR_BLOCK_ENC_PARAM), &input_enc_param);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
            PVR_PRF("ret of the PVR_RPC_IO_START_BLOCK is %d\n ",ret );
            cmd = PVR_RPC_IO_UPDATE_ENC_PARAMTOR;   //update the key right now
            ret |= RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
            PVR_PRF("ret of the PVR_RPC_IO_UPDATE_ENC_PARAMTOR is %d\n ",ret );

            temp = kmalloc(sizeof(struct pvr_enc_param),GFP_KERNEL);
            if (temp == NULL) {
                ret = -ENOMEM;
                goto EXIT;
            }
            memcpy(&temp->enc_param,&input_enc_param,sizeof(temp->enc_param));
            temp->only_flag = file;
            temp->indicator = PVR_IO_START_BLOCK;
            PVR_PRF("\n PVR_IO_START_BLOCK temp->only_flag:(0x%08x),file:(0x%08x) \n",temp->only_flag,file);
            down(&pvr_dev_p->sem);
            list_add(&temp->list_head,&pvr_dev_p->list_head);
            up(&pvr_dev_p->sem);

            break;
        }
        case PVR_IO_START_REENCRYPT:
        {
            struct ali_pvr_reencrypt reencrypt_param;

            memset(&reencrypt_param,0x00,sizeof(struct ali_pvr_reencrypt));
            memset(&input_enc_param,0x00, sizeof(PVR_BLOCK_ENC_PARAM));
            memset(&decrypt_ca_session_attr,0x00,sizeof(struct ca_session_attr));

            retNum = copy_from_user((void *)&reencrypt_param, (void *)arg, sizeof(struct ali_pvr_reencrypt));


            memcpy(input_enc_param.pid_list,reencrypt_param.pid_list,2*reencrypt_param.pid_num);

            input_enc_param.pid_num = reencrypt_param.pid_num;
            PVR_PRF("PVR_IO_START_REENCRYPT dsc_fd:%d \n",reencrypt_param.dsc_fd);
            ca_dsc_get_session_attr((__s32)(reencrypt_param.dsc_fd), &decrypt_ca_session_attr);
            input_enc_param.stream_id = decrypt_ca_session_attr.stream_id;
            input_enc_param.dsc_sub_device = decrypt_ca_session_attr.crypt_mode;
            input_enc_param.sub_device_id = decrypt_ca_session_attr.sub_dev_id;
            PVR_PRF("PVR_IO_START_REENCRYPT stream_id:%d \n",input_enc_param.stream_id);

            input_enc_param.source_mode = reencrypt_param.source_mode;
            input_enc_param.kl_level = PVR_KL_THREE;
            PVR_PRF("PVR_IO_START_REENCRYPT src_mode:%u \n", (__u32)input_enc_param.source_mode);
            if (0 != retNum)
            {
                PVR_PRF("Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }

#if 0
            printk(KERN_ERR"PVR_IO_START_REENCRYPT parametor is :\n");
            printk(KERN_ERR"pid_num:(%d)\n",input_enc_param.pid_num);
            for(i=0; i<input_enc_param.pid_num; i++) {
                printk(KERN_ERR"\t %d-st pid:(%d)\n",i,input_enc_param.pid_list[i]);
            }

            printk(KERN_ERR"\t input_iv is ");
            for(i=0; i<16; i++) {
                printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_iv[i]);
            }
            printk(KERN_ERR"\n");


            printk(KERN_ERR"\t input_key is ");
            for(i=0; i<48; i++) {
                printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_key[i]);
            }
            printk(KERN_ERR"\n");

            printk(KERN_ERR"\t dsc_sub_device:(0x%08x)\n",input_enc_param.dsc_sub_device);
            printk(KERN_ERR"\t key_handle:(0x%08x)\n",input_enc_param.key_handle);
            printk(KERN_ERR"\t key_mode:(0x%08x)\n",input_enc_param.key_mode);
            printk(KERN_ERR"\t kl_mode:(0x%08x)\n",input_enc_param.kl_mode);
            printk(KERN_ERR"\t residue_mode:(0x%08x)\n",input_enc_param.residue_mode);
            printk(KERN_ERR"\t root_key_pos:(0x%08x)\n",input_enc_param.root_key_pos);
            printk(KERN_ERR"\t source_mode:(0x%08x)\n",input_enc_param.source_mode);
            printk(KERN_ERR"\t stream_id:(0x%08x)\n",input_enc_param.stream_id);
            printk(KERN_ERR"\t sub_device_id:(0x%08x)\n",input_enc_param.sub_device_id);
            printk(KERN_ERR"\t target_key_pos:(0x%08x)\n",input_enc_param.target_key_pos);
            printk(KERN_ERR"\t work_mode:(0x%08x)\n",input_enc_param.work_mode);
            printk(KERN_ERR"\t request_res:(0x%08x)\n",input_enc_param.request_res);
#endif
            cmd = PVR_RPC_IO_START_BLOCK;
#if !defined(CONFIG_ALI_RPCNG)
            ret = pvr_start_block_rec(cmd, &input_enc_param);
#else
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_OUT, PRRAM_PVR_UPDATE_ENC, sizeof(PVR_BLOCK_ENC_PARAM), &input_enc_param);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif

            temp = kmalloc(sizeof(struct pvr_enc_param),GFP_KERNEL);
            if (temp == NULL) {
                ret = -ENOMEM;
                goto EXIT;
            }
            memcpy(&temp->enc_param,&input_enc_param,sizeof(temp->enc_param));
            temp->only_flag = file;
            temp->indicator = PVR_IO_START_REENCRYPT;
            PVR_PRF("\n PVR_IO_START_BLOCK_EVO temp->only_flag:(0x%08x),file:(0x%08x) \n",temp->only_flag,file);
            down(&pvr_dev_p->sem);
            list_add(&temp->list_head,&pvr_dev_p->list_head);
            up(&pvr_dev_p->sem);

            break;
        }
        case PVR_IO_STOP_REENCRYPT:
        {
            PVR_RPC_RAW_DECRYPT decrypt_info;
            UINT32  fd;

            memset(&decrypt_ca_session_attr,0,sizeof(struct ca_session_attr));
            memset(&decrypt_info,0,sizeof(PVR_RPC_RAW_DECRYPT));
            fd = (UINT32)arg;
            PVR_PRF("PVR_IO_STOP_REENCRYPT dsc_fd %u!\n",(__u32)fd);
            ca_dsc_get_session_attr((__s32)fd, &decrypt_ca_session_attr);
            decrypt_info.stream_id = (UINT32)decrypt_ca_session_attr.stream_id;
            PVR_PRF("PVR_IO_STOP_REENCRYPT stream_id %u!\n",(__u32)fd);
            cmd = PVR_RPC_IO_FREE_BLOCK;
#if !defined(CONFIG_ALI_RPCNG)
            ret = pvr_free_resources(cmd, &decrypt_info);
#else
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_PVR_decrypt, sizeof(PVR_decrypt), &decrypt_info);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
            down(&pvr_dev_p->sem);
            list_for_each(pos,&pvr_dev_p->list_head) 
            {
                temp = container_of(pos,struct pvr_enc_param,list_head);
                if(temp->only_flag == file)
                {
                    list_del(pos);
                    kfree(temp);
                    break;
                }
            }
            up(&pvr_dev_p->sem);
            break;
        }
        case PVR_IO_UPDATE_ENC_PARAMTOR_EVO:
        {
            PVR_PRF("PVR_RPC_IO_UPDATE_ENC_PARAMTOR command is set!\n");

            memset(&input_enc_param,0, sizeof(PVR_BLOCK_ENC_PARAM));
            retNum = copy_from_user((void *)&input_enc_param, (void *)arg, sizeof(PVR_BLOCK_ENC_PARAM));
            if (0 != retNum)
            {
                PVR_PRF("Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }
            input_enc_param.request_res = 1;
            down(&pvr_dev_p->sem);
            list_for_each(pos,&pvr_dev_p->list_head)
            {
                temp = container_of(pos,struct pvr_enc_param,list_head);
                if(temp->only_flag == file)
                {
                    pvalid = temp;
                    input_enc_param.stream_id = (UINT32)pvalid->enc_param.stream_id;
                    input_enc_param.sub_device_id = pvalid->enc_param.sub_device_id;
                    input_enc_param.key_handle = pvalid->enc_param.key_handle;
                    input_enc_param.target_key_pos = pvalid->enc_param.target_key_pos;
#if 0
                    printk(KERN_ERR"PVR_IO_UPDATE_ENC_PARAMTOR_EVO parametor is :\n");

                    printk(KERN_ERR"pid_num:(%d)\n",input_enc_param.pid_num);
                    for(i=0; i<input_enc_param.pid_num; i++) {
                        printk(KERN_ERR"\t %d-st pid:(%d)\n",i,input_enc_param.pid_list[i]);
                    }

                    printk(KERN_ERR"\t input_iv is ");
                    for(i=0; i<16; i++) {
                        printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_iv[i]);
                    }
                    printk(KERN_ERR"\n");


                    printk(KERN_ERR"\t input_key is ");
                    for(i=0; i<48; i++) {
                        printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_key[i]);
                    }
                    printk(KERN_ERR"\n");

                    printk(KERN_ERR"\t dsc_sub_device:(0x%08x)\n",input_enc_param.dsc_sub_device);
                    printk(KERN_ERR"\t key_handle:(0x%08x)\n",input_enc_param.key_handle);
                    printk(KERN_ERR"\t key_mode:(0x%08x)\n",input_enc_param.key_mode);
                    printk(KERN_ERR"\t kl_mode:(0x%08x)\n",input_enc_param.kl_mode);
                    printk(KERN_ERR"\t residue_mode:(0x%08x)\n",input_enc_param.residue_mode);
                    printk(KERN_ERR"\t root_key_pos:(0x%08x)\n",input_enc_param.root_key_pos);
                    printk(KERN_ERR"\t source_mode:(0x%08x)\n",input_enc_param.source_mode);
                    printk(KERN_ERR"\t stream_id:(0x%08x)\n",input_enc_param.stream_id);
                    printk(KERN_ERR"\t sub_device_id:(0x%08x)\n",input_enc_param.sub_device_id);
                    printk(KERN_ERR"\t target_key_pos:(0x%08x)\n",input_enc_param.target_key_pos);
                    printk(KERN_ERR"\t work_mode:(0x%08x)\n",input_enc_param.work_mode);
                    printk(KERN_ERR"\t request_res:(0x%08x)\n",input_enc_param.request_res);
#endif
                    cmd = PVR_RPC_IO_UPDATE_ENC_PARAMTOR;
#if !defined(CONFIG_ALI_RPCNG)
                    ret = pvr_update_paras(cmd,&input_enc_param);
#else
                    RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
                    RPC_PARAM_UPDATE(p2, PARAM_INOUT, PRRAM_PVR_UPDATE_ENC, sizeof(PVR_BLOCK_ENC_PARAM), &input_enc_param);
                    ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
                    if(ret)
                    {
                        up(&pvr_dev_p->sem);
                        ret = -EINVAL;
                        goto EXIT;
                    }
                    memcpy(&pvalid->enc_param,&input_enc_param,sizeof(input_enc_param));
                    retNum = copy_to_user((void *)arg, (void *)&input_enc_param, sizeof(PVR_BLOCK_ENC_PARAM));
                    break;
                }
            }
            up(&pvr_dev_p->sem);
            if(pvalid == NULL)
            {
                ret = -EINVAL;
                goto EXIT;
            }

            PVR_PRF("RpcCallCompletion ,ret = %d.\n", ret);
            break;
        }
        case PVR_IO_START_BLOCK_EVO:
        {
            struct ca_key_attr  p_key_attr;
            PVR_PRF("PVR_RPC_IO_START_BLOCK command is set!\n");

            memset(&input_enc_param,0, sizeof(PVR_BLOCK_ENC_PARAM));
            memset(&decrypt_ca_session_attr,0x00,sizeof(struct ca_session_attr));
            memset(&p_key_attr,0xff,sizeof(struct ca_key_attr));
            retNum = copy_from_user((void *)&input_enc_param, (void *)arg, sizeof(PVR_BLOCK_ENC_PARAM));
            if (0 != retNum)
            {
                //printk(KERN_ERR"Fail to copy %d.\n", (int)retNum);
                ret = -1;
                goto EXIT;
            }
            input_enc_param.request_res = 1;
#if 0
            printk(KERN_ERR"PVR_IO_START_BLOCK_EVO parametor is :\n");

            printk(KERN_ERR"pid_num:(%d)\n",input_enc_param.pid_num);
            for(i=0; i<input_enc_param.pid_num; i++) {
                printk(KERN_ERR"\t %d-st pid:(%d)\n",i,input_enc_param.pid_list[i]);
            }

            printk(KERN_ERR"\t input_iv is ");
            for(i=0; i<16; i++) {
                printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_iv[i]);
            }
            printk(KERN_ERR"\n");


            printk(KERN_ERR"\t input_key is ");
            for(i=0; i<48; i++) {
                printk(KERN_ERR"0x%02x, ",(unsigned int)input_enc_param.input_key[i]);
            }
            printk(KERN_ERR"\n");

            printk(KERN_ERR"\t dsc_sub_device:(0x%08x)\n",input_enc_param.dsc_sub_device);
            printk(KERN_ERR"\t key_handle:(0x%08x)\n",input_enc_param.key_handle);
            printk(KERN_ERR"\t key_mode:(0x%08x)\n",input_enc_param.key_mode);
            printk(KERN_ERR"\t kl_mode:(0x%08x)\n",input_enc_param.kl_mode);
            printk(KERN_ERR"\t residue_mode:(0x%08x)\n",input_enc_param.residue_mode);
            printk(KERN_ERR"\t root_key_pos:(0x%08x)\n",input_enc_param.root_key_pos);
            printk(KERN_ERR"\t source_mode:(0x%08x)\n",input_enc_param.source_mode);
            printk(KERN_ERR"\t stream_id:(0x%08x)\n",input_enc_param.stream_id);
            printk(KERN_ERR"\t sub_device_id:(0x%08x)\n",input_enc_param.sub_device_id);
            printk(KERN_ERR"\t target_key_pos:(0x%08x)\n",input_enc_param.target_key_pos);
            printk(KERN_ERR"\t work_mode:(0x%08x)\n",input_enc_param.work_mode);
            printk(KERN_ERR"\t request_res:(0x%08x)\n",input_enc_param.request_res);
#endif
#if !defined(CONFIG_ALI_RPCNG)
            cmd = PVR_RPC_IO_START_BLOCK;
            ret = pvr_start_block_rec(cmd, &input_enc_param);
            PVR_PRF("ret of the PVR_RPC_IO_START_BLOCK is %d\n ",ret );
            if(ret != 0)
            {
                PVR_PRF("pvr start block record error!\n");
                ret = -1;
                goto EXIT;
            }
            cmd = PVR_RPC_IO_UPDATE_ENC_PARAMTOR;   //update the key right now
            ret |= pvr_update_paras(cmd,&input_enc_param);
            PVR_PRF("ret of the PVR_RPC_IO_UPDATE_ENC_PARAMTOR is %d\n ",ret );
#else
            cmd = PVR_RPC_IO_START_BLOCK;
            RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
            RPC_PARAM_UPDATE(p2, PARAM_INOUT, PRRAM_PVR_UPDATE_ENC, sizeof(PVR_BLOCK_ENC_PARAM), &input_enc_param);
            ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
            PVR_PRF("ret of the PVR_RPC_IO_START_BLOCK is %d\n ",ret );
            cmd = PVR_RPC_IO_UPDATE_ENC_PARAMTOR;   //update the key right now
            ret |= RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
            PVR_PRF("ret of the PVR_RPC_IO_UPDATE_ENC_PARAMTOR is %d\n ",ret );
#endif
            if(ret)
            {
                goto EXIT;
            }
            temp = kmalloc(sizeof(*temp),GFP_KERNEL);
            if (temp == NULL)
            {
                ret = -ENOMEM;
                goto EXIT;
            }
            memcpy(&temp->enc_param,&input_enc_param,sizeof(temp->enc_param));
            temp->only_flag = file;
            temp->indicator = PVR_IO_START_BLOCK_EVO;
            PVR_PRF("\n PVR_IO_START_BLOCK_EVO temp->only_flag:(0x%08x),file:(0x%08x) \n",temp->only_flag,file);
            down(&pvr_dev_p->sem);
            list_add(&temp->list_head,&pvr_dev_p->list_head);
            up(&pvr_dev_p->sem);
            break;
        }
        case PVR_IO_FREE_BLOCK_EVO:
        {
            PVR_RPC_RAW_DECRYPT decrypt_info;

            memset(&decrypt_info,0,sizeof(PVR_RPC_RAW_DECRYPT));
            down(&pvr_dev_p->sem);
            list_for_each(pos,&pvr_dev_p->list_head)
            {
                temp = container_of(pos,struct pvr_enc_param,list_head);
                if(temp->only_flag == file)
                {
                    decrypt_info.stream_id = (UINT32)temp->enc_param.stream_id;
                    cmd = PVR_RPC_IO_FREE_BLOCK;
#if !defined(CONFIG_ALI_RPCNG)
                    ret = pvr_free_resources(cmd, &decrypt_info);
#else
                    RPC_PARAM_UPDATE(p1, PARAM_IN, PARAM_UINT32, 4, &cmd);
                    RPC_PARAM_UPDATE(p2, PARAM_IN, PARAM_PVR_decrypt, sizeof(PVR_decrypt), &decrypt_info);
                    ret = RpcCallCompletion(RPC_pvr_rpc_ioctl,&p1,&p2,NULL);
#endif
                    list_del(pos);
                    kfree(temp);
                    break;
                }
            }
            up(&pvr_dev_p->sem);
            break;
        }
        case PVR_IO_CAPTURE_DECRYPT_RES:
        {
//(struct file *file, unsigned int cmd, unsigned long arg)
            ret = ali_pvr_capture_decrypt_res(pvr_dev_p,file,cmd,arg);
            break;
        }
        case PVR_IO_RELEASE_DECRYPT_RES:
        {
//
            ret= ali_pvr_release_decrypt_res(pvr_dev_p,file,cmd,arg);
            break;
        }
        case PVR_IO_SET_DECRYPT_RES:
        {
            ret= ali_pvr_set_decrypt_res(pvr_dev_p,file,cmd,arg);
            break;
        }
        case PVR_IO_DECRYPT_EVO:
        {
            ret= ali_pvr_decrytp_raw_evo(pvr_dev_p,file,cmd,arg);
            break;
        }
#if 0
        case PVR_IO_DECRYPT_ES_EVO:
        {
            ret= ali_pvr_decrytp_raw_es_evo(pvr_dev_p,file,cmd,arg);
            break;
        }
#endif
        case PVR_IO_CAPTURE_PVR_KEY:
        {
            ret = ali_pvr_capture_pvr_key(pvr_dev_p,file,cmd,arg);
            break;
        }
        case PVR_IO_RELEASE_PVR_KEY:
        {
            ret = ali_pvr_release_pvr_key(pvr_dev_p,file,cmd,arg);
            break;
        }
        case PVR_IO_SET_PLAYBACK_CA_PRAM:
        {
            ret = ali_pvr_playback_set_key(pvr_dev_p,file,cmd,arg);
            break;
        }
        case PVR_IO_DECRYPT_EVO_SUB:
        {
            ret= ali_pvr_decrytp_raw_evo_sub(pvr_dev_p,file,cmd,arg);
            break;
        }
        default:
            break;
    }

EXIT:
    up(&m_pvr_sem);
    return ret;
}

static void pvr_munmap(struct vm_area_struct *vma)
{
    struct pvr_dev *pvr_dev_p = vma->vm_private_data;

    if (!pvr_dev_p)
    {
        return;
    }

    if (pvr_dev_p->vm_area_node.vma != vma)
    {
        return;
    }

    down(&m_pvr_sem);
    memset(&pvr_dev_p->vm_area_node, 0, sizeof(pvr_dev_p->vm_area_node));
    up(&m_pvr_sem);
}

static const struct vm_operations_struct pvr_vmops =
{
    .close = pvr_munmap,
};

static int pvr_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct pvr_dev *pvr_dev_p = file->private_data;
    int ret = -1;
    size_t size = vma->vm_end - vma->vm_start;
    struct pvr_vm_node *p_vm_node = NULL;

    if (!pvr_dev_p)
    {
        return -EBADF;
    }

    if (pvr_dev_p->vm_area_node.vma)
    {
        dev_info(pvr_device, "device is already mmaped\n");
        return -EBUSY;
    }

    if (!size || (size > __G_ALI_MM_PVR_BUF_MEM_SIZE))
    {
        dev_info(pvr_device, "size not support, size=0x%x, kbuf max:%ldKb\n",
                 size, __G_ALI_MM_PVR_BUF_MEM_SIZE);
        return -ENOMEM;
    }

    size = (size >= PAGE_SIZE) ? size : PAGE_SIZE;

    down(&m_pvr_sem);

    /* map vma->vm_start to kaddr('s page frame num) in size area */
    ret = remap_pfn_range(vma, vma->vm_start,
                          virt_to_phys((void *)pvr_dev_p->p_mmap) >> PAGE_SHIFT,
                          size,  pgprot_noncached(PAGE_SHARED));
    if (ret != 0)
    {
        dev_info(pvr_device, "Kernel error - remap_pfn_range failed\n");
        up(&m_pvr_sem);
        return -EAGAIN;
    }

    p_vm_node = &pvr_dev_p->vm_area_node;

    p_vm_node->vm_kaddr = (void *)pvr_dev_p->p_mmap;
    p_vm_node->vm_start = vma->vm_start;
    p_vm_node->vm_end = vma->vm_end;
    p_vm_node->vm_size = size;
    p_vm_node->vm_owner = current->pid;
    p_vm_node->vma = vma;

    vma->vm_private_data = pvr_dev_p;
    vma->vm_ops = &pvr_vmops;

    up(&m_pvr_sem);

    return 0;
}



/* File operations structure. Defined in linux/fs.h */
static struct file_operations pvr_fops =
{
    .owner    =   THIS_MODULE,     /* Owner */
    .open     =   pvr_open,        /* Open method */
    .release  =   pvr_release,     /* Release method */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
    .unlocked_ioctl = pvr_ioctl,
#else
    .ioctl    =   pvr_ioctl,       /* Ioctl method */
#endif
    .mmap = pvr_mmap,

};

int pvr_get_session_attr(int pvr_fd, struct pvr_ca_attr *p_ca_attr)
{
    __s32 ret = 0;
    struct pvr_dev *s = NULL;
    struct fd k_fd;
    struct pvr_enc_param *temp = NULL,*pvalid = NULL;
    struct list_head *pos = NULL;

    if (!p_ca_attr)
    {
        return -EINVAL;
    }

    k_fd = fdget(pvr_fd);
    if(!k_fd.file)
    {
        ret = -EBADF;
        goto ERR2;
    }

    s = (struct pvr_dev *)k_fd.file->private_data;
    if (!s)
    {
        ret = -EFAULT;
        goto ERR2;
    }

    down(&s->sem);
    list_for_each(pos,&s->list_head)
    {
        temp = container_of(pos,struct pvr_enc_param,list_head);
        if(temp->only_flag == (void*)k_fd.file)
        {
            pvalid = temp;
            break;
        }
    }
    if(pvalid == NULL)
    {
        ret = -EINVAL;
        goto ERR1;
    }
    p_ca_attr->stream_id = pvalid->enc_param.stream_id;
    p_ca_attr->crypt_mode = pvalid->enc_param.dsc_sub_device;
    p_ca_attr->sub_dev_id = pvalid->enc_param.sub_device_id;
    p_ca_attr->sub_dev_see_hdl = (void *)00000001;//notify:see pvr record don't use this valure,only check this function.
    up(&s->sem);

    //p_ca_attr->sub_dev_see_hdl = (void*)(s->sub_dev_see_hdl);

    //p_ca_attr->sub_dev_id = s->sub_dev_id;

ERR1:
    //mutex_unlock(&s->dsc->mutex);
ERR2:
    fdput(k_fd);

    return ret;
}
EXPORT_SYMBOL(pvr_get_session_attr);

int pvr_check_is_pvr_fd(int pvr_fd)
{
    __s32 ret = 0;
    struct fd k_fd;
    k_fd = fdget(pvr_fd);
    if(!k_fd.file)
    {
        ret = -EBADF;
        goto ERR1;
    }
    if(k_fd.file->f_op == &pvr_fops)
    {
        ret = 1;
    }
ERR1:
    fdput(k_fd);
    return ret;
}
EXPORT_SYMBOL(pvr_check_is_pvr_fd);

static int ali_pvr_probe(struct platform_device * pdev)
{
    int i, ret;

	ret = of_get_major_minor(pdev->dev.of_node,&pvr_dev_number, 
			0, PVR_NUM, DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}

    /* Populate sysfs entries */
    pvr_class = class_create(THIS_MODULE, DEVICE_NAME);

    for (i=0; i<PVR_NUM; i++)
    {
        /* Allocate memory for the per-device structure */
        pvr_priv[i] = kmalloc(sizeof(struct pvr_dev), GFP_KERNEL);
        if (!pvr_priv[i])
        {
            PVR_PRF("Bad Kmalloc\n");
            return -ENOMEM;
        }
        memset(pvr_priv[i], 0, sizeof(struct pvr_dev));

        sprintf(pvr_priv[i]->name, "ali_pvr%d", i);

        pvr_priv[i]->pvr_number = i;
        pvr_priv[i]->status = 0;

        /* Connect the file operations with the cdev */
        cdev_init(&pvr_priv[i]->cdev, &pvr_fops);
        pvr_priv[i]->cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&pvr_priv[i]->cdev, (pvr_dev_number + i), 1);
        if (ret)
        {
            PVR_PRF("Bad cdev.\n");
            return ret;
        }

        pvr_device = device_create(pvr_class, NULL, MKDEV(MAJOR(pvr_dev_number), i),
                                   NULL, "ali_pvr%d", i);
        if(pvr_device == NULL)
        {
            PVR_PRF("PVR create device fail.\n");
            return 1;
        }
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
        sema_init(&pvr_priv[i]->sem, 1);
#else
        init_MUTEX(&pvr_priv[i]->sem);
#endif
        INIT_LIST_HEAD(&pvr_priv[i]->list_head);
        if (pvr_priv[i]->p_mmap == NULL)
        {
#if 0
            pvr_priv[i]->p_mmap = kmalloc(PVR_MMAP_SIZE, GFP_KERNEL);
#endif
            pvr_priv[i]->p_mmap = (char *)__G_ALI_MM_PVR_BUF_MEM_START_ADDR;
            if(pvr_priv[i]->p_mmap ==NULL)
            {
                return -ENOMEM;
            }
        }

    }

    if (pvr_block_decrypt_buffer_init() < 0) {
        PVR_PRF("PVR allocate block decrypt buffer fail.\n");
        return -1;
    }
    if (-1 == pvr_decrypt_buffer_allocate(pvr_decrypt_buffer_size)) {
        PVR_PRF("PVR allocate decrypt buffer fail.\n");
        return -1;
    }
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
    sema_init(&m_pvr_sem, 1);
#else
    init_MUTEX(&m_pvr_sem);
#endif

    return 0;

}

/* Driver Exit */
static int ali_pvr_remove(struct platform_device * pdev)
{
    int i;

    /* Release the major number */
    unregister_chrdev_region(pvr_dev_number, PVR_NUM);

    for (i=0; i<PVR_NUM; i++)
    {
        device_destroy(pvr_class, MKDEV(MAJOR(pvr_dev_number), i));
        /* Remove the cdev */
        cdev_del(&pvr_priv[i]->cdev);
        kfree(pvr_priv[i]);
    }

    /* Destroy class */
    class_destroy(pvr_class);

    pvr_decrypt_buffer_free();
    pvr_block_decrypt_buffer_uninit();

    return 0;
}

static const struct of_device_id ali_pvr_match[] = {
       { .compatible = "alitech, pvr", },
       {},
};

MODULE_DEVICE_TABLE(of, ali_pvr_match);

static struct platform_driver ali_pvr_platform_driver = {
	.probe   = ali_pvr_probe, 
	.remove   = ali_pvr_remove,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = "ali_pvr",
			.of_match_table = ali_pvr_match,
	},
};

module_platform_driver(ali_pvr_platform_driver);
MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("ali PVR driver");
MODULE_LICENSE("GPL");
