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
 *  File: ali_rpc_dev_init.c
 *  (I)
 *  Description: initialize the remote devices in the SEE CPU
 *  (S)
 *  History:(M)
 *      	Date        			Author         	Comment
 *      	====        			======		=======
 * 0.		2010.06.02			Sam			Create
 ****************************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36))
#include <linux/smp_lock.h>
#endif
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
#include <ali_hdmi_common.h>
#include <ali_soc.h>
#include <ali_otp_common.h>
#include <ali_video_common.h>

#include <rpc_hld/ali_rpc_hld.h>
#include <rpc_hld/ali_rpc_hld_dis.h>
#include <rpc_hld/ali_rpc_hld_decv.h>
//#include <rpc_hld/ali_rpc_hld_deca.h>
//#include <rpc_hld/ali_rpc_hld_snd.h>
#include <rpc_hld/ali_rpc_hld_gma.h>
#include <rpc_hld/ali_rpc_hld_vbi.h>
#include <rpc_hld/ali_rpc_hld_sdec.h>
#include <rpc_hld/ali_rpc_hld_dsc.h>
#include <rpc_hld/ali_rpc_hld_avsync.h>
#include <rpc_hld/ali_rpc_hld_dmx_see_init.h>

#include <ali_reg.h>
#include <ali_cache.h>
#include <ali_shm.h>  //for __VMTSALI
#include <ali_board_config.h>
#include <alidefinition/adf_boot.h>
#include <asm/pgtable.h>
#include <alidefinition/adf_deca.h>
#include <alidefinition/adf_snd.h>
#include <alidefinition/adf_media.h>
#include <linux/ali_rpc.h>


#if 1
#define PRF printk
#else
#define PRF(...) 	do{}while(0)
#endif

extern void set_video_info_to_hdmi(UINT32 param);
extern void set_audio_info_to_hdmi(UINT32 param);

enum HLD_SEE_DEVINIT_FUNC
{
   FUNC_VPO_DEV_ALL_ATTACH = 0,
   FUNC_VDEC_ALL_ATTACH,   
   FUNC_DECA_DEV_ATTACH,
   FUNC_SND_DEV_ATTACH,
   FUNC_VIDEO_CONFIG,
};

void vpo_dev_attach(void)
{
    jump_to_func(NULL, ali_rpc_call, NULL, (HLD_SEE_DEVINIT<<24)|(0<<16)|FUNC_VPO_DEV_ALL_ATTACH, NULL);
}

void vdec_dev_attach(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_SEE_DEVINIT<<24)|(0<<16)|FUNC_VDEC_ALL_ATTACH, NULL);
}

void deca_dev_attach(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_SEE_DEVINIT<<24)|(0<<16)|FUNC_DECA_DEV_ATTACH, NULL);
}

void snd_dev_attach(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_SEE_DEVINIT<<24)|(0<<16)|FUNC_SND_DEV_ATTACH, NULL);
}

void video_config(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, (HLD_SEE_DEVINIT<<24)|(0<<16)|FUNC_VIDEO_CONFIG, NULL);
}

extern unsigned long g_support_afd_wss;

struct vdec_device*  g_decv_dev0 = NULL;
struct vdec_device*  g_decv_avc_dev0 = NULL;
struct vdec_device*  g_decv_avs_dev0 = NULL;
#ifdef CONFIG_RPC_H265
struct vdec_device*  g_decv_hevc_dev0 = NULL;
#endif
struct vpo_device*   g_vpo_dev = NULL;
EXPORT_SYMBOL(g_vpo_dev);
struct vpo_device*   g_sd_vpo_dev = NULL;
EXPORT_SYMBOL(g_sd_vpo_dev);

struct gma_device *g_gma_dev0 = NULL;
struct gma_device *g_gma_dev1 = NULL;

static void otp_read_vfs_value(UINT32 *p_vfs_value)
{
    UINT32 otp_addr = 0;
    UINT32 otp_tve_fs_value  = 0;

    if(ALI_S3503 == ali_sys_ic_get_chip_id())
    {
        otp_addr = 0x85;
    }
    else
    {
        otp_addr = 0xDF;
    }

    ali_otp_hw_init();
    ali_otp_read(otp_addr*4, (unsigned char *)&otp_tve_fs_value, sizeof(__u32));


    if(ALI_S3503 == ali_sys_ic_get_chip_id())
    {
        *p_vfs_value =  otp_tve_fs_value & 0x7;
    }
    else
    {
        *p_vfs_value = (otp_tve_fs_value >> 6) & 0x0000FFFF;
    }
}

#ifdef CONFIG_RPC_HLD_GMA
static void gma_dev_attach(GMA_DMEM_PARS *dmem_pars)
{
    int i = 0;

    for(i = 0; i < 2; i++) {
	    gma_attach_m36f(i, dmem_pars->dmem_start[i], dmem_pars->dmem_size[i]);
    }
	PRF("attach gma done\n");
}
#endif

#ifdef CONFIG_RPC_HLD_AVSYNC
static void avsync_dev_attach(void)
{
	struct avsync_device * avsync_dev;

	avsync_attach();
	avsync_dev = (struct avsync_device*)hld_dev_get_by_type(NULL, HLD_DEV_TYPE_AVSYNC);
	avsync_open(avsync_dev);
	PRF("kernel attach avsync module done\n");
}
#endif


void vdec_dev_attach(void);

static void dev_attach(void)
{
    GMA_DMEM_PARS dmem_pars;
	vpo_dev_attach();
    otp_read_vfs_value((UINT32 *)&g_otp_set_vdac_fs);
	vpo_ioctl((struct vpo_device*) hld_dev_get_by_id(HLD_DEV_TYPE_DIS, 0), VPO_IO_OTP_SET_VDAC_FS, g_otp_set_vdac_fs);

	g_vpo_dev = (struct vpo_device *)hld_dev_get_by_id(HLD_DEV_TYPE_DIS, 0);
	g_sd_vpo_dev = (struct vpo_device *)hld_dev_get_by_id(HLD_DEV_TYPE_DIS, 1);
	PRF("attach vpo devices done hd %x sd %x\n\n", (int)g_vpo_dev, (int)g_sd_vpo_dev);

	vdec_dev_attach();

	g_decv_dev0 = (struct vdec_device *)hld_dev_get_by_id(HLD_DEV_TYPE_DECV, 0);
	g_decv_avc_dev0 = (struct vdec_device *)hld_dev_get_by_name("DECV_AVC_0");
	PRF("attach decv devices done mpg %x avc %x\n\n", (int)g_decv_dev0, (int)g_decv_avc_dev0);

	g_decv_avs_dev0 = (struct vdec_device *)hld_dev_get_by_name("DECV_AVS_0");
	PRF("attach decv devices done avs %x\n\n", (int)g_decv_avs_dev0);

	#ifdef CONFIG_RPC_H265
    g_decv_hevc_dev0 = (struct vdec_device *)hld_dev_get_by_name("DECV_HEVC_0");
	PRF("attach decv devices done hevc %x\n\n", (int)g_decv_hevc_dev0);
  	#endif

	deca_dev_attach();
	snd_dev_attach();
	snd_register_cb_routine();
#ifdef CONFIG_RPC_HLD_GMA
    memset(&dmem_pars, 0, sizeof(dmem_pars));
	dmem_pars.dmem_start[0] = __G_ALI_MM_FB0_START_ADDR;
	dmem_pars.dmem_size[0] = __G_ALI_MM_FB0_SIZE;
	dmem_pars.dmem_start[1] = __G_ALI_MM_FB2_SEE_START_ADDR;
	dmem_pars.dmem_size[1] = __G_ALI_MM_FB2_SIZE;
	gma_dev_attach(&dmem_pars);

	g_gma_dev0 = (struct gma_device *)hld_dev_get_by_id(HLD_DEV_TYPE_GMA, 0);
	if(g_gma_dev0 == NULL) {
		PRF("gma dev attach fail\n");
	} else {
		PRF("gma dev %x\n", (int)g_gma_dev0);
	}

	g_gma_dev1 = (struct gma_device *)hld_dev_get_by_id(HLD_DEV_TYPE_GMA, 1);
	if(g_gma_dev1 == NULL) {
		PRF("gma dev attach fail\n");
	} else {
		PRF("gma dev %x\n", (int)g_gma_dev1);
	}

    if((ali_sys_ic_get_chip_id() == ALI_C3921) || (ali_sys_ic_get_chip_id() == ALI_S3821)
        || (ali_sys_ic_get_chip_id() == ALI_C3505))
	{
		gma_io_control(g_gma_dev0, GMA_IO_SET_DMEM_PAR, (UINT32)&dmem_pars);
        osddrv_attach();
	}
#endif

#ifdef CONFIG_DVB_ALI_M36_DMX
	sed_dmx_attach(0);
#endif

#ifdef CONFIG_RPC_HLD_AVSYNC
	avsync_dev_attach();
#endif

#ifdef CONFIG_ALI_DSC
	ali_m36_dsc_see_init();
#endif
}

#ifdef CONFIG_HDMI_ALI
static void bootmedia_cb(unsigned long type, unsigned long param)
{
	if(param == 0)
	{
	   printk("error: param == NULL!\n");
	   return;
	}

    *(int*)param = TRUE;
    printk("param = %x!\n" ,*(int*)param);
	return ;

}
#endif
void hdmi_config(void)
{
    #ifdef CONFIG_HDMI_ALI
	struct vpo_device* dis = (struct vpo_device*) hld_dev_get_by_id(HLD_DEV_TYPE_DIS, 0);
	struct snd_device* snd = (struct snd_device*) hld_dev_get_by_id(HLD_DEV_TYPE_SND, 0);
	/* register Callback to DIS/SND TDS Driver */
	if(dis != NULL){
		if(vpo_ioctl(dis, VPO_IO_REG_CB_HDMI, (UINT32)set_video_info_to_hdmi) != RET_SUCCESS)
			PRF("register video callback of HDMI failed!\n");

		if( vpo_ioctl(dis, VPO_IO_HDMI_OUT_PIC_FMT, RGB_MODE2) != RET_SUCCESS)
        {
			PRF("%s : %d fail \n", __FUNCTION__, __LINE__);
        }
	}

    #ifdef CONFIG_RPC_HLD_SND
	if(snd != NULL){
		if(snd_io_control(snd, SND_REG_HDMI_CB, (UINT32)set_audio_info_to_hdmi) != RET_SUCCESS)
			PRF("register sound callback of HDMI failed!\n");
	}
    #endif
	ali_rpc_register_callback(ALI_RPC_CB_BOOTMEDIA_HDMI, bootmedia_cb);
	
	#endif
}

static void dev_config(void)
{
 	video_config();
 	vpo_register_cb_routine();
	hdmi_config();
}

void rpc_remote_dev_init(void)
{
	if(g_see_heap_top_addr != 0)
		hld_dev_see_init((void *)g_see_heap_top_addr);

	PRF("%s : start to init remote rpc devices\n", __FUNCTION__);
	dev_attach();
	dev_config();
	PRF("%s : end the init job\n", __FUNCTION__);
}

/* Mali dedicated memory */
EXPORT_SYMBOL(__G_ALI_MM_MALI_DEDICATED_MEM_SIZE);
EXPORT_SYMBOL(__G_ALI_MM_MALI_DEDICATED_MEM_START_ADDR);

/* Mali UMP memory */
EXPORT_SYMBOL(__G_ALI_MM_MALI_UMP_MEM_SIZE);
EXPORT_SYMBOL(__G_ALI_MM_MALI_UMP_MEM_START_ADDR);

/* FB memory */
EXPORT_SYMBOL(__G_ALI_MM_FB0_SIZE);
EXPORT_SYMBOL(__G_ALI_MM_FB0_START_ADDR);
