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

/*****************************************************************************/
/**
*
* @file board_config.c
*
* The XTemac driver. Functions in this file are the minimum required functions
* for this driver. See xtemac.h for a detailed description of the driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00a joy  2013/07/13 First release
* </pre>
******************************************************************************/

#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/pgtable.h>

#include <mach/ali-s3921.h>
#include <ali_reg.h>

#include "tve_config.h"
#include "board_config.h"

#include <alidefinition/adf_boot.h>
#define DEFINE_BORAD_VARIABLES

#include <ali_board_config.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

extern unsigned int arm_get_early_mem_size(void);

#ifdef SUPPORT_AFD_WSS_OUTPUT
unsigned long g_support_afd_wss = 1;
#else
unsigned long g_support_afd_wss = 0;
#endif

#ifdef SUPPORT_AFD_SCALE
unsigned long   g_support_afd_scale = 1;
#else
unsigned long   g_support_afd_scale = 0;
#endif

#define ENABLE_SHOW_MEMAP


static struct ali_hwbuf_desc ali_hwbuf_desc[] =
{
    {
	  	.name      = "Ali priv mem",
        .phy_start = 0x84000000,
        .phy_end   = 0x8c000000,
    },
};

/*****************************************************************
return Enable/Disable security bootrom.
0x03[1] BOOT_SECURITY_EN
return 1, enable
return 0, disable
*****************************************************************/
static UINT32 AS_bootrom_is_enable(void)
{
	UINT32 OTP_value = 0;
	UINT32 ret = 0;
	OTP_value = __REG32ALI(0x18042044);
	ret = (OTP_value & 0x2) >> 1;
	return ret;
}
#define SEE_REBOOT_COMMAND_ADDR 0x18000280
#define DRAM_SPLIT_CTRL_BASE 0x18041000
#define DRAM_TRNG_CTRL_BASE 0x1803a400
#define PVT_S_ADDR 0x10
#define PVT_E_ADDR 0x14
#define SHR_S_ADDR 0x18
#define SHR_E_ADDR 0x1c
#define GET_DWORD(i)           (*(volatile UINT32 *)(i))


static int get_memory_map(void)
{
	struct device_node* node;

	node = of_find_compatible_node(NULL, NULL, "alitech,memory-mapping");
	if(NULL == node)
	{
		printk("alitech,memory-mapping node is null\n");
		return -1;
	}

	of_property_read_u32_index(node, "alisee_area_buff", 0,(u32 *)&__G_ALI_MM_PRIVATE_AREA_START_ADDR);
	of_property_read_u32_index(node, "alisee_area_buff", 1,(u32 *)&__G_ALI_MM_PRIVATE_AREA_SIZE);
	__G_ALI_MM_PRIVATE_AREA_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_PRIVATE_AREA_START_ADDR);

	of_property_read_u32_index(node, "init_ramfs_buff", 0,(u32 *)&__G_ALI_MM_INIT_RAMFS_ADDR);
	of_property_read_u32_index(node, "init_ramfs_buff", 1,(u32 *)&__G_ALI_MM_INIT_RAMFS_SIZE);
	__G_ALI_MM_INIT_RAMFS_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_INIT_RAMFS_ADDR);

	of_property_read_u32_index(node, "vcap_buff", 0,(u32 *)&__G_ALI_MM_VCAP_FB_START_ADDR);
	of_property_read_u32_index(node, "vcap_buff", 1,(u32 *)&__G_ALI_MM_VCAP_FB_SIZE);
	__G_ALI_MM_VCAP_FB_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VCAP_FB_START_ADDR);

	of_property_read_u32_index(node, "decv_hw_buff", 0,(u32 *)&__G_ALI_MM_VDEC_HW_BUF_START_ADDR);
	of_property_read_u32_index(node, "decv_hw_buff", 1,(u32 *)&__G_ALI_MM_VDEC_HW_BUF_SIZE);
	__G_ALI_MM_VDEC_HW_BUF_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VDEC_HW_BUF_START_ADDR);

	of_property_read_u32_index(node, "decv_pip_hw_buff", 0,(u32 *)&__G_ALI_MM_VDEC_PIP_HW_BUF_START_ADDR);
	of_property_read_u32_index(node, "decv_pip_hw_buff", 1,(u32 *)&__G_ALI_MM_VDEC_PIP_HW_BUF_SIZE);
	__G_ALI_MM_VDEC_PIP_HW_BUF_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VDEC_PIP_HW_BUF_START_ADDR);

	of_property_read_u32_index(node, "decv_vbv_buff", 0,(u32 *)&__G_ALI_MM_VDEC_VBV_START_ADDR);
	of_property_read_u32_index(node, "decv_vbv_buff", 1,(u32 *)&__G_ALI_MM_VDEC_VBV_SIZE);
	__G_ALI_MM_VDEC_VBV_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VDEC_VBV_START_ADDR);

	of_property_read_u32_index(node, "decv_pip_vbv_buff", 0,(u32 *)&__G_ALI_MM_VDEC_PIP_VBV_START_ADDR);
	of_property_read_u32_index(node, "decv_pip_vbv_buff", 1,(u32 *)&__G_ALI_MM_VDEC_PIP_VBV_SIZE);
	__G_ALI_MM_VDEC_PIP_VBV_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VDEC_PIP_VBV_START_ADDR);

	of_property_read_u32_index(node, "decv_fb_buff", 0,(u32 *)&__G_ALI_MM_VIDEO_START_ADDR);
	of_property_read_u32_index(node, "decv_fb_buff", 1,(u32 *)&__G_ALI_MM_VIDEO_SIZE);
	__G_ALI_MM_VIDEO_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VIDEO_START_ADDR);

	of_property_read_u32_index(node, "decv_pip_fb_buff", 0,(u32 *)&__G_ALI_MM_VIDEO_PIP_START_ADDR);
	of_property_read_u32_index(node, "decv_pip_fb_buff", 1,(u32 *)&__G_ALI_MM_VIDEO_PIP_SIZE);
	__G_ALI_MM_VIDEO_PIP_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_VIDEO_PIP_START_ADDR);

	of_property_read_u32_index(node, "share_mem_buff", 0,(u32 *)&__G_ALI_MM_SHARED_MEM_START_ADDR);
	of_property_read_u32_index(node, "share_mem_buff", 1,(u32 *)&__G_ALI_MM_SHARED_MEM_SIZE);
	__G_ALI_MM_SHARED_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_SHARED_MEM_START_ADDR);

	of_property_read_u32_index(node, "nim_dvbt2_buff", 0,(u32 *)&__G_ALI_MM_NIM_PARAM_BUF_ADDR);
	of_property_read_u32_index(node, "nim_dvbt2_buff", 1,(u32 *)&__G_ALI_MM_NIM_PARAM_BUF_SIZE);
	__G_ALI_MM_NIM_PARAM_BUF_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_NIM_PARAM_BUF_ADDR);

	of_property_read_u32_index(node, "fb0_buff", 0,(u32 *)&__G_ALI_MM_FB0_START_ADDR);
	of_property_read_u32_index(node, "fb0_buff", 1,(u32 *)&__G_ALI_MM_FB0_SIZE);
	__G_ALI_MM_FB0_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_FB0_START_ADDR);

	of_property_read_u32_index(node, "fb2_buff", 0,(u32 *)&__G_ALI_MM_FB2_START_ADDR);
	of_property_read_u32_index(node, "fb2_buff", 1,(u32 *)&__G_ALI_MM_FB2_SIZE);
	__G_ALI_MM_FB2_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_FB2_START_ADDR);

	of_property_read_u32_index(node, "dsc_buff", 0,(u32 *)&__G_ALI_MM_DSC_MEM_START_ADDR);
	of_property_read_u32_index(node, "dsc_buff", 1,(u32 *)&__G_ALI_MM_DSC_MEM_SIZE);
	__G_ALI_MM_DSC_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_DSC_MEM_START_ADDR);

	of_property_read_u32_index(node, "capture_frame_buff", 0,(u32 *)&__G_ALI_MM_STILL_FRAME_START_ADDR);
	of_property_read_u32_index(node, "capture_frame_buff", 1,(u32 *)&__G_ALI_MM_STILL_FRAME_SIZE);
	__G_ALI_MM_STILL_FRAME_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_STILL_FRAME_START_ADDR);

	of_property_read_u32_index(node, "image_mem_buff", 0,(u32 *)&__G_ALI_MM_IMAGE_DECODER_MEM_START_ADDR);
	of_property_read_u32_index(node, "image_mem_buff", 1,(u32 *)&__G_ALI_MM_IMAGE_DECODER_MEM_SIZE);
	__G_ALI_MM_IMAGE_DECODER_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_IMAGE_DECODER_MEM_START_ADDR);

	of_property_read_u32_index(node, "ge_cmd_buff", 0,(u32 *)&__G_ALI_MM_GE_CMD_START_ADDR);
	of_property_read_u32_index(node, "ge_cmd_buff", 1,(u32 *)&__G_ALI_MM_GE_CMD_SIZE);
	__G_ALI_MM_GE_CMD_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_GE_CMD_START_ADDR);

	of_property_read_u32_index(node, "ape_mem_buff", 0,(u32 *)&__G_ALI_MM_APE_MEM_START_ADDR);
	of_property_read_u32_index(node, "ape_mem_buff", 1,(u32 *)&__G_ALI_MM_APE_MEM_SIZE);
	__G_ALI_MM_APE_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_APE_MEM_START_ADDR);

	of_property_read_u32_index(node, "ape_pip_mem_buff", 0,(u32 *)&__G_ALI_MM_APE_PIP_MEM_START_ADDR);
	of_property_read_u32_index(node, "ape_pip_mem_buff", 1,(u32 *)&__G_ALI_MM_APE_PIP_MEM_SIZE);
	__G_ALI_MM_APE_PIP_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_APE_PIP_MEM_START_ADDR);

	of_property_read_u32_index(node, "tsg_mem_buff", 0,(u32 *)&__G_ALI_MM_TSG_BUF_START_ADDR);
	of_property_read_u32_index(node, "tsg_mem_buff", 1,(u32 *)&__G_ALI_MM_TSG_BUF_SIZE);
	__G_ALI_MM_TSG_BUF_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_TSG_BUF_START_ADDR);

	of_property_read_u32_index(node, "dmx_mem_buff", 0,(u32 *)&__G_ALI_MM_DMX_MEM_START_ADDR);
	of_property_read_u32_index(node, "dmx_mem_buff", 1,(u32 *)&__G_ALI_MM_DMX_MEM_SIZE);
	__G_ALI_MM_DMX_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_DMX_MEM_START_ADDR);

	of_property_read_u32_index(node, "dmx_pip_mem_buff", 0,(u32 *)&__G_ALI_MM_DMX_PIP_MEM_START_ADDR);
	of_property_read_u32_index(node, "dmx_pip_mem_buff", 1,(u32 *)&__G_ALI_MM_DMX_PIP_MEM_SIZE);
	__G_ALI_MM_DMX_PIP_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_DMX_PIP_MEM_START_ADDR);

	of_property_read_u32_index(node, "nim_j83b_buff", 0,(u32 *)&__G_ALI_MM_NIM_J83B_MEM_START_ADDR);
	of_property_read_u32_index(node, "nim_j83b_buff", 1,(u32 *)&__G_ALI_MM_NIM_J83B_MEM_SIZE);
	__G_ALI_MM_NIM_J83B_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_NIM_J83B_MEM_START_ADDR);

	of_property_read_u32_index(node, "fb0_cmap_buff", 0,(u32 *)&__G_ALI_MM_FB0_CMAP_START_ADDR);
	of_property_read_u32_index(node, "fb0_cmap_buff", 1,(u32 *)&__G_ALI_MM_FB0_CMAP_SIZE);
	__G_ALI_MM_FB0_CMAP_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_FB0_CMAP_START_ADDR);

	of_property_read_u32_index(node, "fb2_cmap_buff", 0,(u32 *)&__G_ALI_MM_FB2_CMAP_START_ADDR);
	of_property_read_u32_index(node, "fb2_cmap_buff", 1,(u32 *)&__G_ALI_MM_FB2_CMAP_SIZE);
	__G_ALI_MM_FB2_CMAP_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_FB2_CMAP_START_ADDR);

	of_property_read_u32_index(node, "audio_dec_buff", 0,(u32 *)&__G_ALI_MM_DECA_MEM_START_ADDR);
	of_property_read_u32_index(node, "audio_dec_buff", 1,(u32 *)&__G_ALI_MM_DECA_MEM_SIZE);
	__G_ALI_MM_DECA_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_DECA_MEM_START_ADDR);

	of_property_read_u32_index(node, "boot_cmd_buff", 0,(u32 *)&__G_ALI_MM_BOOT_COMMAND_START_ADDR);
	of_property_read_u32_index(node, "boot_cmd_buff", 1,(u32 *)&__G_ALI_MM_BOOT_COMMAND_SIZE);
	__G_ALI_MM_BOOT_COMMAND_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_BOOT_COMMAND_START_ADDR);

	of_property_read_u32_index(node, "deca_mem_buff", 0,(u32 *)&__G_ALI_MM_DECA_MEM_START_ADDR);
	of_property_read_u32_index(node, "deca_mem_buff", 1,(u32 *)&__G_ALI_MM_DECA_MEM_SIZE);
	__G_ALI_MM_DECA_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_DECA_MEM_START_ADDR);

	of_property_read_u32_index(node, "pvr_buff", 0,(u32 *)&__G_ALI_MM_PVR_BUF_MEM_START_ADDR);
	of_property_read_u32_index(node, "pvr_buff", 1,(u32 *)&__G_ALI_MM_PVR_BUF_MEM_SIZE);
	__G_ALI_MM_PVR_BUF_MEM_START_ADDR = (unsigned long)(phys_to_virt)(__G_ALI_MM_PVR_BUF_MEM_START_ADDR);

	__G_ALI_MM_MALI_DEDICATED_MEM_START_ADDR = (unsigned long)(((__G_ALI_MM_FB0_START_ADDR + __G_ALI_MM_FB0_SIZE) + 0xFFF) & 0xFFFFF000);
	__G_ALI_MM_MALI_UMP_MEM_START_ADDR = (unsigned long)(((__G_ALI_MM_FB0_START_ADDR + __G_ALI_MM_FB0_SIZE) + 0xFFF) & 0xFFFFF000);

	return 0;
}

/* Init HW buffer addresses & feature configs.
*/
static void set_global_setting(void)
{
	/*<==================== BOARD IDENTIFICATION START ====================>*/
	/* hardware board definition start */
	/* please review the below board type when you
	want to porting new board. if it belongs to a exist
	type, just allocate a new version for this new board.

		board type description :
			type 				value
			C3701_DEMO			0x00003000
			C3921_DEMO			0x00004000

		board version description :
			version		value
			invalid            0
			ver1   		1
			...
			vern			n
	*/

	/*
	define 3921 board type as flow:

	0x00004001 : BGA445
	0x00004002 : QFP256
	*/
	if(__REG8ALI(0x18000001) & 0x01)
		__G_ALI_BOARD_TYPE = 0x00004001;
	else
		__G_ALI_BOARD_TYPE = 0x00004002;

	get_memory_map();

	/* FRAME BUFFER */
	g_fb0_max_width = (FB0_VIRTUAL_WIDTH);
	g_fb0_max_height = (FB0_VIRTUAL_HEIGHT);
	g_fb0_width = (FB0_WIDTH);
	g_fb0_height = (FB0_HEIGHT);
	g_fb0_pitch = (FB0_PITCH);
	g_fb0_bpp = (FB0_BPP);

	g_fb2_max_width = (FB2_VIRTUAL_WIDTH);
	g_fb2_max_height = (FB2_VIRTUAL_HEIGHT);
	g_fb2_width = (FB2_WIDTH);
	g_fb2_height = (FB2_HEIGHT);
	g_fb2_pitch = (FB2_PITCH);
	g_fb2_bpp = (FB2_BPP);


	g_support_standard_fb = 1;

	/* DAC.
	*/
	/* default scart output
	0 : CVBS
	1 : RGB
	2 : SVIDEO
	3 : YUV
	*/
	g_tve_default_scart_output = 3;

	/* TV ENCODER.
	*/
	g_tve_hd_default_tv_mode = LINE_1080_60;
	g_tve_sd_default_tv_mode = NTSC;

	/* whether use the CVBS output. the invalid value is -1.
	*/
	g_tve_dac_use_cvbs_type = CVBS_1;

	/* whether use the SVIDEO output. the invalid value is -1.
	*/
	g_tve_dac_use_svideo_type = -1;

	/* whether use the RGB output. the invalid value is -1.
	*/
	g_tve_dac_use_rgb_type = RGB_1;

	/* whether use the YUV output. the invalie value is -1.
	*/
	g_tve_dac_use_yuv_type = YUV_1;

	/* CVBS dac definition.
	*/
	g_tve_dac_cvbs = DAC3;

	/* SVIDEO dac definition.
	*/
	g_tve_dac_svideo_y = 0;
	g_tve_dac_svideo_c = 0;

	/* RGB dac definition.
	*/
	g_tve_dac_rgb_r = DAC0;
	g_tve_dac_rgb_g = DAC2;
	g_tve_dac_rgb_b = DAC1;

	/* YUV dac definition.
	*/
	g_tve_dac_yuv_y = DAC2;
	g_tve_dac_yuv_u = DAC1;
	g_tve_dac_yuv_v = DAC0;

	/* feature list.
	*/
	g_support_isdbt_cc = 0;

	/* HDMI.
	*/
	g_hdmi_hdcp_enable = 0;

	/*<==================== DRVIER FEATURE CONFIG END ====================>*/

	return;
}

static void __init alissi_setting(void)
{
	unsigned int value;
	unsigned int mask;
	unsigned int chip_id_reg = 0x0;
	/*************          check  chip_id         *****************/
	chip_id_reg = __REG32ALI(0X18000000);
	chip_id_reg = chip_id_reg >> 12;
	chip_id_reg = 0x0;
	/***********Config the PinMUX for ASSI1_2, ASSI2, ASSI3_1, ASSI4**************/
	value = __REG32ALI(0x18000088);
	//mask = (1<<4)|(1<<21)|(1<<22);  //clear SPI,ALISSI2 and ALISSI3_1
	mask = (1<<4);
	/*(alissi4) mutual exclusion*/
	if(chip_id_reg == 0x3921b)
	{
		mask |= (1<<7)|(1<<23);
	}
	value = value&(~mask);
	mask = (1<<21)|(1<<22);
	value = value|mask;
	__REG32ALI(0x18000088) = value;

	value = __REG32ALI(0x1800008C);
	mask = (1<<6)|(1<<9)|(1<<14)|(1<<16)|(1<<17)|(1<<19);  //clear QAM_SPI,ALISSI4,ALISSI2,QAM_SSI
	/*(alissi4) mutual exclusion*/
	if(chip_id_reg == 0x3921b)
	{
		mask |= (1<<15)|(1<<21)|(1<<25);
	}
	value = value&(~mask);

	/* Bit 22(alissi4) mutual exclusion with SD pin.
	*/
	/*(alissi4) mutual exclusion*/
	if(chip_id_reg == 0x3921b)
	{
		mask = (1<<8)|(1<<22);   //ALISSI1_2
	}
	else
	{
		mask = (1<<8);   //ALISSI1_2
	}
	value = value|mask;
	__REG32ALI(0x1800008C) = value;

	/****************Cfg the GPIO **********************/
	/*GPIO for ASSI1_1*/
	value = __REG32ALI(0x18000430);
	mask = (1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9);
	value = value&(~mask);
	__REG32ALI(0x18000430) = value;

	/*GPIO for ASSI2*/
	value = __REG32ALI(0x18000430);
	mask = (1<<13)|(1<<14)|(1<<15);
	value = value&(~mask);
	__REG32ALI(0x18000430) = value;

	value = __REG32ALI(0x18000438);
	mask = (1<<18)|(1<<19)|(1<<20);
	value = value&(~mask);
	__REG32ALI(0x18000438) = value;

	/* GPIO for ASSI3_1 */
	value = __REG32ALI(0x1800043c);
	mask = (1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<23)|(1<<26);
	value = value&(~mask);
	__REG32ALI(0x1800043c) = value;

	/* Bit 22(alissi4) mutual exclusion with SD pin.
	*/
	/*(alissi4) mutual exclusion*/
	if(chip_id_reg == 0x3921b)
	/* GPIO for ASSI4 */
	{
		value = __REG32ALI(0x18000430);
		mask = (1<<0)|(1<<3)|(1<<10)|(1<11)|(1<12);
		value = value&(~mask);
		__REG32ALI(0x18000430) = value;

		value = __REG32ALI(0x18000438);
		mask = (1<<23);
		value = value&(~mask);
		__REG32ALI(0x18000438) = value;
	}
}

static void __init ali_nand_setting(void)
{
	// gpio setting.
	g_nand_wp_gpio = -1;

	// pinmux setting.
}

/*
		第一组CA：
    	ca1_1:reg 0x18000088[0]
	ca1_2:reg 0x1800008c[23]

		第二组CA：
	ca2_1:reg 0x1800008c[4]
	ca2_2:reg 0x1800008c[1]
*/
void ali_smc_pinmux_setting(void)
{
	unsigned int value = 0;
	unsigned int mask = 0;

	if(__G_ALI_BOARD_TYPE == 0x00004001)	/* bga445 */
	{
			/* ca2_1 */
		__REG32ALI(0x1800008c) |= __REG32ALI(0x1800008c) | 0x10;
		value = __REG32ALI(0x18000438);
		mask = (1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7);
		value = value & (~mask);
		__REG32ALI(0x18000438) = value;

		/* ca1 */
		__REG32ALI(0x18000088) |= __REG32ALI(0x18000088) | 0x1;
		//__REG32ALI(0x1800008C) |= __REG32ALI(0x1800008C) | (0x1<<23);
		printk("in %s, 0x18000088=0x%x, 0x1800008C=0x%x", __FUNCTION__, __REG32ALI(0x18000088), __REG32ALI(0x1800008C));

		gpio_enable_pin(20);
		ali_gpio_set_value(20, 1);
		gpio_enable_pin(22);
		ali_gpio_set_value(22, 0);
	}
	else if(__G_ALI_BOARD_TYPE == 0x00004002)	/* qfp256 */
	{
		/* ca2 */
		__REG32ALI(0x1800008c) |= __REG32ALI(0x1800008c) | 0x2;
		/*	ca2_2
			XSC2_CLK		gpio[40]
			XSC2_POWENJ	gpio[41]
			XSC2_DATA		gpio[42]
			XSC2_RST		gpio[43]
			XSC2_PRESJ		gpio[44]
		*/
		value = __REG32ALI(0x18000434);
		mask = (1<<8)|(1<<9)|(1<<10)|(1<<11)|(1<<12);
		value = value & (~mask);
		__REG32ALI(0x18000434) = value;

		/* ca1 */
		__REG32ALI(0x18000088) |= __REG32ALI(0x18000088) | 0x1;

		/*gpio_enable_pin(20);
		ali_gpio_set_value(20, 1);
		gpio_enable_pin(22);
		ali_gpio_set_value(22, 0);*/
	}

	g_smc_invert_detect = 1;//0;
}


#ifdef CONFIG_ALI_VOLBAR
static void ali_volbar_gpio_setting(void)
{
	unsigned int value = 0, mask = 0;

	/* set gpio enable control register(IOBASE + 0434h) for soundbar */
	/*	GPIO[63:32] :
			0: GPIO function disable(default)
			1: GPIO function enable	*/
	value = __REG32ALI(0x18000434);
	mask = (1 << 1) | (1 << 0);/* set 32,33 */
	value = value | mask;
	__REG32ALI(0x18000434) = value;

	/* set gpio output control register(IOBASE + 00d8h) for soundbar */
	/*	GPIO[63:32] :
			0: input (default) (*)
			1: output	*/
	value = __REG32ALI(0x180000d8);
	mask = 0xfffffffc;/* clear 32,33 */
	value = value & mask;
	__REG32ALI(0x180000d8) = value;

	/* set gpio interrupt enable register(IOBASE + 00C4h) for soundbar */
	/*	GPIO[32:63] :
			0: interrupt diabled (default)
			1: interrupt enable (*)	*/
	value = __REG32ALI(0x180000c4);
	mask = (1 << 1) | (1 << 0);/* set 32,33 */
	value = value | mask;
	__REG32ALI(0x180000c4) = value;

	/* set gpio rising edge interrupt control register(IOBASE + 00C8h) for soundbar */
	/*	GPIO[32:63] :
			0: rising edge interrupt disable
			1: rising edge interrupt enable (default) */
	/*value = __REG32ALI(0x180000c8);*/
	/*mask = (1 << 1) | (1 << 0);*//* set 32,33 */
	/*value = value | mask;*/
	/*__REG32ALI(0x180000c8) = value;*/

	/* set gpio falling edge interrupt control register(IOBASE + 00CCh) for soundbar */
	/*	GPIO[32:63] :
			0: falling edge interrupt disable (default)
			1: falling edge interrupt enable (*) */
	value = __REG32ALI(0x180000cc);
	mask = (1 << 1) | (1 << 0);/* set 32,33 */
	value = value | mask;
	__REG32ALI(0x180000cc) = value;

	g_gpio_vol_down = 32;
	g_gpio_vol_up = 33;
}
#endif


void ali_ci_pinmux_setting(void)
{
	unsigned int value = 0, mask = 0;

	//set gpio enable control register(IOBASE + 0430h) for ci
	//	GPIO[31:0] :
	//		0: GPIO function disable(default)
	//		1: GPIO function enable
	value = __REG32ALI(0x18000430);
	/*printk("[MODULE:CI] %s : read gpio 0~31 value from 0x%08x\n",  __FUNCTION__, value);*/
	mask = 0x0000ffff;//disable 16 ~ 31
	value = value & mask;
	__REG32ALI(0x18000430) = value;
	/*printk("[MODULE:CI] %s : set gpio 0~31 value to 0x%08x\n",  __FUNCTION__, value);*/

	//set gpio enable control register(IOBASE + 0434h) for ci
	//	GPIO[32:63] :
	//		0: GPIO function disable(default)
	//		1: GPIO function enable
	value = __REG32ALI(0x18000434);
	/*printk("[MODULE:CI] %s : read gpio 32~63 value from 0x%08x\n",  __FUNCTION__, value);*/
	mask = 0x00000000;//disable 32 ~ 63
	value = value & mask;
	__REG32ALI(0x18000434) = value;
	/*printk("[MODULE:CI] %s : set gpio 32~63 value to 0x%08x\n", __FUNCTION__, value);*/

	//set gpio enable control register(IOBASE + 0438h) for ci
	//	GPIO[64:95] :
	//		0: GPIO function disable(default)
	//		1: GPIO function enable
	value = __REG32ALI(0x18000438);
	/*printk("[MODULE:CI] %s : read gpio 64~95 value from 0x%08x\n",  __FUNCTION__, value);*/
	mask = 0xfffffe00;//disable 64 ~ 72
	value = value & mask;
	__REG32ALI(0x18000438) = value;
	/*printk("[MODULE:CI] %s : set gpio 64~95 value to 0x%08x\n", __FUNCTION__, value);*/

	//set pinmux register(IOBASE + 0088h ) for ci
	//	EMMC_SEL3[bit-5]
	//		0 : Other Function (*)
	//		1 : Enable(Default)
	//	XVIN0_SEL[bit-17]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	XVOUT0_SEL[bit-18]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	value = __REG32ALI(0x18000088);
	/*printk("[MODULE:CI] %s : read pinmux-88 value from 0x%08x\n", __FUNCTION__, value);*/
	mask = 0xfff9ffdf;//clear 5,17,18
	value = value & mask;
	__REG32ALI(0x18000088) = value;
	/*printk("[MODULE:CI] %s : clear pinmux-88 value to 0x%08x\n", __FUNCTION__, value);*/

	//set pinmux register(IOBASE + 008Ch ) for ci
	//	XSC2_SEL2[bit-1]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	XSC2_SEL1[bit-4]
	//		0 : Other Function (*)
	//		1 : Enable(Default)
	//	AUD_I2SIO_SEL2[bit-11]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	PCM_SEL2[bit-13]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	CA9_DEBUG_SEL[bit-18]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	IP_DEBUG_SEL[bit-20]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	ELMB_SEL[bit-24]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	QAM_SSI_SEL2[bit-26]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	//	EMMC_SEL2[bit-28]
	//		0 : Other Function (Default) (*)
	//		1 : Enable
	value = __REG32ALI(0x1800008c);
	/*printk("[MODULE:CI] %s : read pinmux-8C value from 0x%08x\n", __FUNCTION__, value);*/
	mask = 0xeaebd7ed;//clear 1,4,11,13,18,20,24,26,28
	value = value & mask;
	__REG32ALI(0x1800008c) = value;
	/*printk("[MODULE:CI] %s : clear pinmux-8C value to 0x%08x\n", __FUNCTION__, value);*/

	//set pinmux register(IOBASE + 0088h ) for ci
	//	XCI_SEL[bit-1]
	//		0 : Other Function (Default)
	//		1 : Enable (*)
	value = __REG32ALI(0x18000088);
	/*printk("[MODULE:CI] %s : read pinmux-88 value from 0x%08x\n", __FUNCTION__, value);*/
	mask = (1 << 1);//set 1
	value = value | mask;
	__REG32ALI(0x18000088) = value;
	/*printk("[MODULE:CI] %s : set pinmux-88 value to 0x%08x\n", __FUNCTION__, value);*/
}

#define C3921_BGA_DVB_COMBO

void nim_setting(void)
{
  	//AGC
	__REG32ALI(0x18000088) |= 1<<7;// XIF_AGC_PDM_SEL

 	//__REG32ALI(0x1800043c) != (1<<25)|(1<<26);
	//__REG32ALI(0x1800043c) &= (0<<25)|(0<<26);

	if(__G_ALI_BOARD_TYPE == 0x00004001)	/* bga445 */
	{
		__REG32ALI(0x18000088) |= 1<<8;
		__REG32ALI(0x18000088) |= 0<<24;
		__REG32ALI(0x18000088) |= 0<<26;
		//__REG32ALI(0x18000438) |= (0<<25)|(0<<26);
		//i2c-1 enable
#ifdef C3921_BGA_DVB_COMBO
		__REG32ALI(0x18000088) |= 1<<12;
		//gpio 91-92
		__REG32ALI(0x18000438) |= (0<<27)|(0<<28);
#endif

	}
	else if(__G_ALI_BOARD_TYPE == 0x00004002)	/* qfp256 */
	{
		__REG32ALI(0x18000430) |= 1<<0; //for fullnim i2c
    __REG32ALI(0x18000430) |= 1<<3;

	}
}

/* board configutation.
*/
static void __init board_setting_bga445(void)
{
	/* TODO: Set pinmux here.
	*/
	 // disable all GPIO
    __REG32ALI(0x18000430) = 0;
    __REG32ALI(0x18000434) = 0;
    __REG32ALI(0x18000438) = 0;
    __REG32ALI(0x1800043c) = 0;


	__REG32ALI(0x18000088) |= 1<<9;// i2c1

	nim_setting();

    /* Setting DVFS I2C device id number : 4, using Linux kernel i2c interface*/
    g_dvfs_i2c_device_id = 4;

	/* TODO: Set GPIO here.
	*/

	alissi_setting();

	/* SND GPIO.
	*/
	g_snd_mute_gpio = 80;
    g_snd_chip_type = 1;


    /* SND pimmux.
	*/
	__REG32ALI(0x18000088) |= 1<<13; // XSPDIF_SEL1

	/* nand pinmux & gpio setting.
	*/
	ali_nand_setting();

	/* for C3921 BGA GPIO 73 control usb port 0 5V*/
	g_usb_p0_host_5v_gpio = 73;

	/* for C3921 BGA GPIO 84 control usb port device port 5V detect*/
    g_usb_device_5v_detect_gpio = 84;



#ifdef C3921_BGA_DVB_COMBO   //kent(for dvbc+dvbs work together)
    g_enet_link_gpio = -1;
    g_enet_speed_gpio = -1;
#else
    g_enet_link_gpio = 121;
    g_enet_speed_gpio = 92;
#endif
	return;
}

static void __init board_setting_gfp256(void)
{
	unsigned int value = 0;
	unsigned int mask = 0;


	/* TODO: Set pinmux here.
	*/
	 // disable all GPIO
    __REG32ALI(0x18000430) = 0;
    __REG32ALI(0x18000434) = 0;
    __REG32ALI(0x18000438) = 0;
    __REG32ALI(0x1800043c) = 0;

	//__REG32ALI(0x18000088) |= 1<<7;// XIF_AGC_PDM_SEL
	__REG32ALI(0x18000088) |= 1<<9;// i2c1

	nim_setting();

    /* Setting DVFS I2C device id number : I2C_TYPE_SCB1,   useing the scb i2c */
    g_dvfs_i2c_device_id = 1;

	/* TODO: Set GPIO here.
	*/

	/* SND GPIO.
	*/
	g_snd_mute_gpio = 92;
    g_snd_chip_type = 0;

    /* SND pimmux.
	*/
	__REG32ALI(0x18000088) |= 1<<13; // XSPDIF_SEL1

#if 1
	value = __REG32ALI(0x1800008C);
	//mask = (1<<6)|(1<<9)|(1<<14)|(1<<16)|(1<<17)|(1<<19);  //clear QAM_SPI,ALISSI4,ALISSI2,QAM_SSI
	//value = value&(~mask);
	mask = (1<<8);   //ALISSI1_2
	value = value|mask;
	__REG32ALI(0x1800008C) = value;

	value = __REG32ALI(0x18000430);
	mask = (1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9);
	value = value&(~mask);
	__REG32ALI(0x18000430) = value;
#endif


    g_enet_link_gpio = 6;
    g_enet_speed_gpio = 5;
	return;
}

/* david dai 20150909*/
static void  ali_uart2_pimux_setting(void)
{
	unsigned int value;
	unsigned int mask;
	/* ************************UART2_SEL1 muxed with alissi******************
	value = __REG32ALI(0x18000088);
	mask = (1<<6);
	value |= mask;
//	mask = (1<<27);
//	value &= ~mask;
	__REG32ALI(0x18000088) = value;

	// disable tevnc_sync_sel3
	value = __REG32ALI(0x18000088);
	mask = (1<<24);
	value &= ~mask;
	__REG32ALI(0x18000088) = value;

    //disable alissi_sel2
	value = __REG32ALI(0x1800008c);
	mask = (1<<8);
	value &= ~mask;
	__REG32ALI(0x1800008c) = value;

	// disable QAM_ssi_256_sel2
	value = __REG32ALI(0x1800008c);
	mask = (1<<16);
	value &= ~mask;
	__REG32ALI(0x1800008c) = value;

	//disabel usb_debu_sel
	value = __REG32ALI(0x1800008c);
	mask = (1<<21);
	value &= ~mask;
	__REG32ALI(0x1800008c) = value;



	// disable gpio 87
	value = __REG32ALI(0x18000438);
	mask = (1<<23);
	value &= ~mask;
	__REG32ALI(0x18000438) = value;
	// disable gpio 118
	value = __REG32ALI(0x1800043c);
	mask = (1<<22);
	value &= ~mask;
	__REG32ALI(0x1800043c) = value;
	*/


	/* ************************UART2_SEL2 mexed with i2c_2 ******************/
	value = __REG32ALI(0x18000088);
	mask = (1<<27);
	value |= mask;
	__REG32ALI(0x18000088) = value;

	// disable UART2_SEL1
	value = __REG32ALI(0x18000088);
	mask = (1<<6);
	value &= ~mask;
	__REG32ALI(0x18000088) = value;

	// disable xi2c_2_sel
	value = __REG32ALI(0x18000088);
	mask = (1<<9);
	value &= ~mask;
	__REG32ALI(0x18000088) = value;


	//disable gpio 89 90
	value = __REG32ALI(0x18000438);
	mask = (3<<25);
	value &= ~mask;
	__REG32ALI(0x18000438) = value;

}

u64 __init dt_next_cell(int s, __be32 **cellp)
{
	__be32 *p = *cellp;
	*cellp = p + s;
	return of_read_number(p, s);
}

static int __init platform_get_memory_info(unsigned long node,
	const char *uname, int depth, void *data)
{
	char *type = of_get_flat_dt_prop(node, "device_type", NULL);
	__be32 *reg, *endp;
	unsigned long l;
	int i = 0;
	if (type == NULL) {
		/*
		 * The longtrail doesn't have a device_type on the
		 * /memory node, so look for the node called /memory@0.
		 */
		if (depth != 1 || strcmp(uname, "memory-reserved") != 0)
			return 0;
	} else if (strcmp(type, "memory-reserved") != 0)
		return 0;

	reg = of_get_flat_dt_prop(node, "reg", &l);
	if (reg == NULL)
		return 0;
	endp = reg + (l / sizeof(__be32));

	while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {

		ali_hwbuf_desc[i].phy_start = dt_next_cell(dt_root_addr_cells, &reg);
		ali_hwbuf_desc[i].phy_end = dt_next_cell(dt_root_size_cells, &reg);
	}

	return 0;
}

struct ali_hwbuf_desc* __init ali_get_privbuf_desc
(
    int *hwbuf_desc_cnt
)
{
	of_scan_flat_dt(platform_get_memory_info, NULL);

	/* set CPU phy address to CPU virtural address */
	ali_hwbuf_desc[0].phy_start = (ali_hwbuf_desc[0].phy_start & 0x1FFFFFFF) | 0x80000000;
	ali_hwbuf_desc[0].phy_end   = (ali_hwbuf_desc[0].phy_end & 0x1FFFFFFF) | 0x80000000;

	printk("ali_hwbuf_desc[0].phy_start = %x ali_hwbuf_desc[0].phy_end %x \n",
			ali_hwbuf_desc[0].phy_start, ali_hwbuf_desc[0].phy_end);

	/* there is only one reserved hw buff for ALi platform */
	*hwbuf_desc_cnt = 1;
	return(ali_hwbuf_desc);
}

void board_ddr_priority_init(void)
{

  	__REG32ALI(0x1800F914) = 0x030d1f3f;/*fixed vini overflow*/


}

void customize_board_setting(void)
{
	unsigned long ul_reg_val = 0;

	set_global_setting();
	if(__G_ALI_BOARD_TYPE == 0x00004001)
	{
		board_setting_bga445();
	}
	else if(__G_ALI_BOARD_TYPE == 0x00004002)
	{
		board_setting_gfp256();
	}

#ifdef CONFIG_ALI_M36_SMARTCARD
	ali_smc_pinmux_setting();
#endif

#ifdef CONFIG_ALI_M36_CIC
	ali_ci_pinmux_setting();
#endif

	board_ddr_priority_init();
	/* enable gpio 74, 75 for hdmi gpio i2c */
	ul_reg_val = __REG32ALI(0x18000438);
	ul_reg_val |= (1<<(74-64));
	ul_reg_val |= (1<<(75-64));
	__REG32ALI(0x18000438) = ul_reg_val;

	/*panel gpio i2c set*/
	#if defined CONFIG_ALI_PAN_CH455 || defined CONFIG_ALI_PAN_CH454
	g_gpio_i2c_panel_sda = I2C_GPIO_SDA_PIN;
	g_gpio_i2c_panel_scl = I2C_GPIO_SCL_PIN;
	g_gpio_i2c_panel_id = I2C_DEVICE_ID;
	#endif

    #ifdef CONFIG_ALI_PAN_CH454
		//panel CH454 LED address
		g_dig0_addr = 0x50;
		g_dig1_addr = 0x52;
		g_dig2_addr = 0x54;
		g_dig3_addr = 0x56;
		g_dig4_addr = 0x58;

		g_dig0_high_addr = 0x60;
		g_dig1_high_addr = 0x62;
		g_dig2_high_addr = 0x64;
		g_dig3_high_addr = 0x66;
		g_dig4_high_addr = 0x68;
	#endif

	#ifdef CONFIG_M3921_GPIO_DETECT
	/* enable gpio 87 for upgrade key */
	ul_reg_val = __REG32ALI(0x18000438);
	ul_reg_val |= (1<<(87-64));
	__REG32ALI(0x18000438) = ul_reg_val;
	#endif

	#ifdef	CONFIG_SERIAL_ALI_UART2_AVAIL
	ali_uart2_pimux_setting();
	#endif

	#ifdef	CONFIG_SERIAL_8250_NR_UARTS
    if(CONFIG_SERIAL_8250_NR_UARTS>=2){
	    ali_uart2_pimux_setting();
    }
	#endif

	#ifdef CONFIG_ALI_PAN_CH455
	//panel CH455 LED address
	g_dig0_addr = 0x68;
	g_dig1_addr = 0x6e;
	g_dig2_addr = 0x6c;
	g_dig3_addr = 0x6a;
	#endif

	#ifdef CONFIG_ALI_VOLBAR
	ali_volbar_gpio_setting();
	#endif
}
