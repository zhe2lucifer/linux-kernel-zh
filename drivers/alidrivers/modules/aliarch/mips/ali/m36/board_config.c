/*                                                                                                                      
*      Alitech
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License
*   as published by the Free Software Foundation; either version
*   2 of the License, or (at your option) any later version.
*/
#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/pgtable.h>
#include <ali_reg.h>
#include "tve_config.h"
#include "board_config.h"
#include <alidefinition/adf_boot.h>
#define DEFINE_BORAD_VARIABLES
#include <ali_board_config.h>
#include <ali_soc.h>
#include <alidefinition/adf_sysdef.h>

typedef enum 
{
	I2C_DEVICE_ID = 4,
    I2C_DEVICE_HDMI_ID = 5,
    I2C_DEVICE_UNKNOW_ID = 0xff,
} ALI_I2C_DEVICE_ID;

#define DRAM_SPLIT_CTRL_BASE 0xb8041000
#define SHR_S_ADDR 0x18
#define SHR_E_ADDR 0x1c
#define GET_DWORD(i)           (*(volatile UINT32 *)(i))

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

/*
return Enable/Disable security bootrom.
0x03[1] BOOT_SECURITY_EN 
return 1, enable
return 0, disable
*/
#ifndef CONFIG_ALI_GET_DTB
static unsigned int AS_bootrom_is_enable(void)
{
	unsigned int OTP_value = 0;
	unsigned int ret = 0;
	OTP_value = (*(volatile 	unsigned int *)(0xb8042044)); 
	ret = (OTP_value & 0x2) >> 1;
	return ret;
}

static struct ali_hwbuf_desc ali_hwbuf_desc_public_256M[] =
{
    {
        .name      = "Ali public mem1",
        .phy_start = 0xA0000000,
        .phy_end   = __MM_ALI_RESERVE_MEM_START - 1,
    }, 
    
#ifndef   CONFIG_SUPPORT_512MB_MEMORY
    {
        .name      = "Ali public mem2",
        .phy_start = __MM_ALI_RESERVE_MEM_END,
        .phy_end   = 0xAFFFFFFF,
    },
#else 		
    {
        .name      = "Ali public mem2",
        .phy_start = __MM_ALI_RESERVE_MEM_END,
        .phy_end   = 0xB7FFFFFF,
    },
    
#if defined(CONFIG_ALI_CHIP_CAP210)
    {
	    .name 	 = "Ali public highmem",      //CAP210: high mem: 0x90000000--0XA0000000
		.phy_start = 0x90000000 + 0xA0000000,
		.phy_end   = 0x9FFFFFFF + 0XA0000000,
	},
#elif defined(CONFIG_ALI_M3505)
	{
	    .name 	 = "Ali public highmem",      //M3529: high mem: 0x98000000--0XA0000000
		.phy_start = 0x98000000 + 0xA0000000,
		.phy_end   = 0x9FFFFFFF + 0XA0000000,
	},
#elif defined(CONFIG_ALI_CHIP_M3823)

	{
	    .name 	 = "Ali public highmem",      //M3823: high mem: 0X38000000--0X4000000
		.phy_start = 0x38000000 + 0xA0000000,
		.phy_end   = 0x3FFFFFFF + 0XA0000000,
	},
#else
	{
	    .name 	 = "Ali public highmem",      //others: high mem: 0X38000000--0X4000000
		.phy_start = 0x38000000 + 0xA0000000,
		.phy_end   = 0x3FFFFFFF + 0XA0000000,
	},
#endif
#endif	
};
#endif 

int get_memory_mapping(void)
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

	of_property_read_u32_index(node, "init_ramfs_buff", 0,(u32 *)&__G_ALI_MM_INIT_RAMFS_ADDR);
	of_property_read_u32_index(node, "init_ramfs_buff", 1,(u32 *)&__G_ALI_MM_INIT_RAMFS_SIZE);

	of_property_read_u32_index(node, "vcap_buff", 0,(u32 *)&__G_ALI_MM_VCAP_FB_START_ADDR);
	of_property_read_u32_index(node, "vcap_buff", 1,(u32 *)&__G_ALI_MM_VCAP_FB_SIZE);

	of_property_read_u32_index(node, "decv_hw_buff", 0,(u32 *)&__G_ALI_MM_VDEC_HW_BUF_START_ADDR);
	of_property_read_u32_index(node, "decv_hw_buff", 1,(u32 *)&__G_ALI_MM_VDEC_HW_BUF_SIZE);

	of_property_read_u32_index(node, "decv_pip_hw_buff", 0,(u32 *)&__G_ALI_MM_VDEC_PIP_HW_BUF_START_ADDR);
	of_property_read_u32_index(node, "decv_pip_hw_buff", 1,(u32 *)&__G_ALI_MM_VDEC_PIP_HW_BUF_SIZE);

	of_property_read_u32_index(node, "decv_vbv_buff", 0,(u32 *)&__G_ALI_MM_VDEC_VBV_START_ADDR);
	of_property_read_u32_index(node, "decv_vbv_buff", 1,(u32 *)&__G_ALI_MM_VDEC_VBV_SIZE);

	of_property_read_u32_index(node, "decv_pip_vbv_buff", 0,(u32 *)&__G_ALI_MM_VDEC_PIP_VBV_START_ADDR);
	of_property_read_u32_index(node, "decv_pip_vbv_buff", 1,(u32 *)&__G_ALI_MM_VDEC_PIP_VBV_SIZE);

	of_property_read_u32_index(node, "decv_fb_buff", 0,(u32 *)&__G_ALI_MM_VIDEO_START_ADDR);
	of_property_read_u32_index(node, "decv_fb_buff", 1,(u32 *)&__G_ALI_MM_VIDEO_SIZE);

	of_property_read_u32_index(node, "decv_pip_fb_buff", 0,(u32 *)&__G_ALI_MM_VIDEO_PIP_START_ADDR);
	of_property_read_u32_index(node, "decv_pip_fb_buff", 1,(u32 *)&__G_ALI_MM_VIDEO_PIP_SIZE);

	of_property_read_u32_index(node, "share_mem_buff", 0,(u32 *)&__G_ALI_MM_SHARED_MEM_START_ADDR);
	of_property_read_u32_index(node, "share_mem_buff", 1,(u32 *)&__G_ALI_MM_SHARED_MEM_SIZE);
	
	of_property_read_u32_index(node, "nim_dvbt2_buff", 0,(u32 *)&__G_ALI_MM_NIM_PARAM_BUF_ADDR);
	of_property_read_u32_index(node, "nim_dvbt2_buff", 1,(u32 *)&__G_ALI_MM_NIM_PARAM_BUF_SIZE);

	of_property_read_u32_index(node, "fb0_buff", 0,(u32 *)&__G_ALI_MM_FB0_START_ADDR);
	of_property_read_u32_index(node, "fb0_buff", 1,(u32 *)&__G_ALI_MM_FB0_SIZE);

	of_property_read_u32_index(node, "fb2_buff", 0,(u32 *)&__G_ALI_MM_FB2_START_ADDR);
	of_property_read_u32_index(node, "fb2_buff", 1,(u32 *)&__G_ALI_MM_FB2_SIZE);

	of_property_read_u32_index(node, "dsc_buff", 0,(u32 *)&__G_ALI_MM_DSC_MEM_START_ADDR);
	of_property_read_u32_index(node, "dsc_buff", 1,(u32 *)&__G_ALI_MM_DSC_MEM_SIZE);

	of_property_read_u32_index(node, "capture_frame_buff", 0,(u32 *)&__G_ALI_MM_STILL_FRAME_START_ADDR);
	of_property_read_u32_index(node, "capture_frame_buff", 1,(u32 *)&__G_ALI_MM_STILL_FRAME_SIZE);

	of_property_read_u32_index(node, "image_mem_buff", 0,(u32 *)&__G_ALI_MM_IMAGE_DECODER_MEM_START_ADDR);
	of_property_read_u32_index(node, "image_mem_buff", 1,(u32 *)&__G_ALI_MM_IMAGE_DECODER_MEM_SIZE);

	of_property_read_u32_index(node, "ge_cmd_buff", 0,(u32 *)&__G_ALI_MM_GE_CMD_START_ADDR);
	of_property_read_u32_index(node, "ge_cmd_buff", 1,(u32 *)&__G_ALI_MM_GE_CMD_SIZE);

	of_property_read_u32_index(node, "ape_mem_buff", 0,(u32 *)&__G_ALI_MM_APE_MEM_START_ADDR);
	of_property_read_u32_index(node, "ape_mem_buff", 1,(u32 *)&__G_ALI_MM_APE_MEM_SIZE);


	of_property_read_u32_index(node, "tac_buff", 0,(u32 *)&__G_ALI_MM_TAC_MEM_START_ADDR);
	of_property_read_u32_index(node, "tac_buff", 1,(u32 *)&__G_ALI_MM_TAC_MEM_SIZE);

	of_property_read_u32_index(node, "ape_pip_mem_buff", 0,(u32 *)&__G_ALI_MM_APE_PIP_MEM_START_ADDR);
	of_property_read_u32_index(node, "ape_pip_mem_buff", 1,(u32 *)&__G_ALI_MM_APE_PIP_MEM_SIZE);

	of_property_read_u32_index(node, "tsg_mem_buff", 0,(u32 *)&__G_ALI_MM_TSG_BUF_START_ADDR);
	of_property_read_u32_index(node, "tsg_mem_buff", 1,(u32 *)&__G_ALI_MM_TSG_BUF_SIZE);
	__G_ALI_MM_TSG_BUF_START_ADDR |= 0xA0000000;

	of_property_read_u32_index(node, "dmx_mem_buff", 0,(u32 *)&__G_ALI_MM_DMX_MEM_START_ADDR);
	of_property_read_u32_index(node, "dmx_mem_buff", 1,(u32 *)&__G_ALI_MM_DMX_MEM_SIZE);
	__G_ALI_MM_DMX_MEM_START_ADDR |= 0xA0000000;

	of_property_read_u32_index(node, "dmx_pip_mem_buff", 0,(u32 *)&__G_ALI_MM_DMX_PIP_MEM_START_ADDR);
	of_property_read_u32_index(node, "dmx_pip_mem_buff", 1,(u32 *)&__G_ALI_MM_DMX_PIP_MEM_SIZE);
	__G_ALI_MM_DMX_PIP_MEM_START_ADDR |= 0xA0000000;

	of_property_read_u32_index(node, "nim_j83b_buff", 0,(u32 *)&__G_ALI_MM_NIM_J83B_MEM_START_ADDR);
	of_property_read_u32_index(node, "nim_j83b_buff", 1,(u32 *)&__G_ALI_MM_NIM_J83B_MEM_SIZE);

	of_property_read_u32_index(node, "fb0_cmap_buff", 0,(u32 *)&__G_ALI_MM_FB0_CMAP_START_ADDR);
	of_property_read_u32_index(node, "fb0_cmap_buff", 1,(u32 *)&__G_ALI_MM_FB0_CMAP_SIZE);

	of_property_read_u32_index(node, "fb2_cmap_buff", 0,(u32 *)&__G_ALI_MM_FB2_CMAP_START_ADDR);
	of_property_read_u32_index(node, "fb2_cmap_buff", 1,(u32 *)&__G_ALI_MM_FB2_CMAP_SIZE);

	of_property_read_u32_index(node, "audio_dec_buff", 0,(u32 *)&__G_ALI_MM_DECA_MEM_START_ADDR);
	of_property_read_u32_index(node, "audio_dec_buff", 1,(u32 *)&__G_ALI_MM_DECA_MEM_SIZE);

	of_property_read_u32_index(node, "boot_cmd_buff", 0,(u32 *)&__G_ALI_MM_BOOT_COMMAND_START_ADDR);
	of_property_read_u32_index(node, "boot_cmd_buff", 1,(u32 *)&__G_ALI_MM_BOOT_COMMAND_SIZE);

	of_property_read_u32_index(node, "deca_mem_buff", 0,(u32 *)&__G_ALI_MM_DECA_MEM_START_ADDR);
	of_property_read_u32_index(node, "deca_mem_buff", 1,(u32 *)&__G_ALI_MM_DECA_MEM_SIZE);

	of_property_read_u32_index(node, "pvr_buff", 0,(u32 *)&__G_ALI_MM_PVR_BUF_MEM_START_ADDR);
	of_property_read_u32_index(node, "pvr_buff", 1,(u32 *)&__G_ALI_MM_PVR_BUF_MEM_SIZE);

	return 0;
}

/* Init HW buffer addresses & feature configs.
*/
static void set_global_setting(void)
{	
	UINT32 chip_id = ali_sys_ic_get_chip_id();	

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

	  __G_ALI_MM_FB0_SIZE = (FB0_MEM_SIZE);
	  __G_ALI_MM_FB2_SIZE = (FB2_MEM_SIZE);

	/* Standard FB or not
	*/
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
	g_tve_dac_rgb_r = DAC2;

	if (ALI_S3503 == chip_id)
	{
		g_tve_dac_rgb_g = DAC1;
		g_tve_dac_rgb_b = DAC0;
	}
	else
	{
		g_tve_dac_rgb_g = DAC0;
		g_tve_dac_rgb_b = DAC1;
	}
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
	return;
}


static void __init alissi_setting(void)
{
	UINT32 chip_id = ali_sys_ic_get_chip_id();
	unsigned int value = 0;
	unsigned int mask = 0;

	if (ALI_S3821 == chip_id)	/* M3S23 */
	{
		value = __REG32ALI(0x1800043c);
		mask = (1<<12)|(1<<15)|(1<<17)|(1<<19)|(1<<21) |(1<<26);
		value = value & (~mask);
		__REG32ALI(0x1800043c) = value;

		/* enable 48ch[10] [9] [8]  for M3S23 ASSI3 */
		value = __REG32ALI(0x1800048c);
		value |= (1<<8) | (1<<9) | (1<<10);
		__REG32ALI(0x1800048c) = value;
	}
	else if (ALI_S3503 == chip_id)
	{
		value = __REG32ALI(0x1800043c);
		mask = (1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22) |(1<<23);
		value = value & (~mask);
		__REG32ALI(0x1800043c) = value;

		/* enable 0x18000088[28], disable 0x18000088[2] for M3515*/
		value = __REG32ALI(0x18000088);
		value |= (1<<28);
		__REG32ALI(0x18000088) = value;

		value = __REG32ALI(0x18000088);
		mask = (1<<2);
		value = value & (~mask);
		__REG32ALI(0x18000088) = value;
	}
	else if (ALI_C3505 == chip_id)
	{
	#ifdef CONFIG_ALI_M3529_PINMUX//M3529	
		value = __REG32ALI(0x1800048c);
		value |= (1<<8)|(1<<9)|(1<<10) | (1<<12) | (1<<15);//enable assi3(8,9,10), enable assi4(12,15)
		__REG32ALI(0x1800048c) = value;
	#elif defined  CONFIG_ALI_M3528//M3528
		value = __REG32ALI(0x1800048c);
		mask  = (1<<12) | (1<<15); //enable assi4
		value = value | mask;
		__REG32ALI(0x1800048c) = value;
	#else				
		value = __REG32ALI(0x1800048c);
		value |= (1<<5)|(1<<6)|(1<<7);
		__REG32ALI(0x1800048c) = value;
	#endif
	}

}

static void __init ali_nand_setting(void)
{
	UINT32 chip_id = ali_sys_ic_get_chip_id();
	unsigned int value = 0;

	if (ALI_S3821 == chip_id)
	{
		unsigned int value = 0;

		// gpio setting.
		g_nand_wp_gpio = -1;

		// pinmux setting.
		value = __REG32ALI(0x18000490);
		value |= ALI_PINMUX_CTRL_BIT3 | ALI_PINMUX_CTRL_BIT26;
		__REG32ALI(0x18000490) = value;
	}
	else if (ALI_S3503 == chip_id)
	{
		g_nand_wp_gpio = 56;

		// pinmux setting.
		value = __REG32ALI(0x18000434);
		value |= (1 << 24);
		__REG32ALI(0x18000434) = value;
		value = __REG32ALI(0x180000D8);
		value |= (1 << 24);
		__REG32ALI(0x180000D8) = value;
		value = __REG32ALI(0x180000D4);
		value |= (1 << 24);
		__REG32ALI(0x180000D4) = value;
	}
	else if (ALI_C3505 == chip_id)
	{
	//todo
	}
}

void ali_smc_pinmux_setting(void)
{
	unsigned int value = 0;
	unsigned int mask = 0;
	UINT32 chip_id = ali_sys_ic_get_chip_id();

	if (ALI_S3821 == chip_id)	/* M3S23 */
	{
		value = __REG32ALI(0x18000438);
		mask = (1<<(81-64)) | (1<<(82-64)) | (1<<(88-64)) | (1<<(95-64));
		value = value & (~mask);
		__REG32ALI(0x18000438) = value;
        value = __REG32ALI(0x1800043c);
		mask = (1<<(96-96));
		value = value & (~mask);
		__REG32ALI(0x1800043c) = value;

        __REG32ALI(0x18000488) |= __REG32ALI(0x18000488) | (1<<19);

        gpio_enable_pin(80);
        ali_gpio_set_value(80, 0);
        gpio_enable_pin(79);
        ali_gpio_set_value(79, 1);
	}
	else if (ALI_S3503 == chip_id)
	{
		__REG32ALI(0x18000088) &= ~(1<<15);  /* disable PK256_CA1_SEL1 */
		__REG32ALI(0x18000088) &= ~(1<<0);  /* disable PK256_CI_SEL */
		/* disable gpio 79 - 83 */
		value = __REG32ALI(0x18000438);
		mask = (1<<15)|(1<<16)|(1<<17)|(1<<18)|(1<<19);
		value = value & (~mask);
		__REG32ALI(0x18000438) = value;
		__REG32ALI(0x18000088) |= (1<<19); /* enable PK256_CA1_SEL2*/
		gpio_enable_pin(34);
		ali_gpio_set_value(34, 1);
		gpio_enable_pin(85);
		ali_gpio_set_value(85, 0);
	}
	else if (ALI_C3505 == chip_id)
	{	
	#ifdef CONFIG_ALI_M3529_PINMUX//M3529	
		value = __REG32ALI(0x18000438);
		mask = (1<<24)|(1<<25)|(1<<26)|(1<<27)|(1<<28);  
		value = value & (~mask);
		__REG32ALI(0x18000438) = value;	// disable gpio 25 - 28
				
		__REG32ALI(0x18000488) |= (1<<20); // enable XSC3_SEL	

		gpio_enable_pin(7);
		ali_gpio_set_value(7, 1);    
		//gpio_enable_pin(6);
		//ali_gpio_set_value(6, 0);
	#else
		value = __REG32ALI(0x18000438);
		mask = (1<<11)|(1<<12)|(1<<13)|(1<<14)|(1<<15);  
		value = value & (~mask);
		__REG32ALI(0x18000438) = value;	// disable gpio 75 - 79

		__REG32ALI(0x1800048C) &= ~(1<<9);  //disable ASSI3

		__REG32ALI(0x18000488) |= (1<<3); // enable XSC_SEL

		gpio_enable_pin(72);
		ali_gpio_set_value(72, 1);
		gpio_enable_pin(74);
		ali_gpio_set_value(74, 0);    
	#endif
	}	
	g_smc_invert_detect = 1;
}

void ali_cic_pinmux_setting(void)
{
	unsigned int value = 0, mask = 0;
    printk("\n\n\n\n%s\n", "Enter ali_cic_pinmux_setting");
	//set gpio enable control register(IOBASE + 0430h) for ci
	//	GPIO[31:0] :
	//		0: GPIO function disable(default)
	//		1: GPIO function enable
	value = __REG32ALI(0x18000430);
	/*printk("[MODULE:CI] %s : read gpio 0~31 value from 0x%08x\n",  __FUNCTION__, value);*/
	mask = 0x1fffffff;//disable 29 ~ 31
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
	mask = 0xf000f840;//disable 64 ~ 91 except 70,75,76,77,78,79
	value = value & mask;
	__REG32ALI(0x18000438) = value;
	/*printk("[MODULE:CI] %s : set gpio 64~95 value to 0x%08x\n", __FUNCTION__, value);*/

    value = __REG32ALI(0x18000488);
	mask = 0xffffffd7;//clear bit 3,5
	value = value & mask;
	__REG32ALI(0x18000488) = value;

	value = __REG32ALI(0x1800048c);
	mask = 0xffffff0f;//clear bit 4,5,6,7
	value = value & mask;
	__REG32ALI(0x1800048c) = value;

	value = __REG32ALI(0x18000490);
	mask = 0xfffffffd;//clear bit 1
	value = value & mask;
	__REG32ALI(0x18000490) = value;
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
	#if 0
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
	#endif
	//set pinmux register(IOBASE + 00490h ) for ci
	//	XCI_SEL[bit-1]
	//		0 : Other Function (Default)
	//		1 : Enable (*)
	value = __REG32ALI(0x18000490);
	/*printk("[MODULE:CI] %s : read pinmux-490 value from 0x%08x\n", __FUNCTION__, value);*/
	mask = (1 << 5);//set 1
	value = value | mask;
	__REG32ALI(0x18000490) = value;
	//printk("[MODULE:CI] %s : set pinmux-490 value to 0x%08x\n", __FUNCTION__, value);
}

void nim_setting(void)
{
	UINT32 chip_id = ali_sys_ic_get_chip_id();
	if (ALI_S3821 == chip_id)
	{
		__REG32ALI(0x18000488) |= 1<<26;	  // IF_AGC_T2_SEL
		__REG32ALI(0x18000488) |= 1<< 17 ;	  //QAM_IF_AGC_SEL
		__REG32ALI(0x18000488) |= 1<< 13 ;	  //I2C4_SEL
		__REG32ALI(0x18000488) |= 1<< 11 ;	  //I2C4_SEL
	}
	else if (ALI_S3503 == chip_id)
	{
		int index=2;
	    __REG32ALI(0x18000430) |= 1<<index;
	    __REG32ALI(0x18000058) |= 1<<index;
	    __REG32ALI(0x18000054) &= ~(1<<index);
	    __REG32ALI(0x18000054) |= 1<<index;
	    __REG32ALI(0x1800008c) |= 1<<1;      // RF_AGC_SEL
	    __REG32ALI(0x1800008c) |= 1<<0;      // I2C1_SEL
	    __REG32ALI(0x18000430) |= 1<< 6 ;      //gpio 6
	    __REG32ALI(0x18000430) |= 1<< 7 ;      //gpio 7
	    __REG32ALI(0x18000088) |= 1<< 8 ;   //STRAP_PIN_SEL_EN
	    __REG32ALI(0x1800008C) |= 1<< 3 ;  //PK256_DISEQC_HV_SE
	    __REG32ALI(0x1800008C) |= 1<< 4 ;  //PK256_DISEQC_OUT_SEL
	}
	else if (ALI_C3505 == chip_id)
	{
	    #ifndef CONFIG_S3281
		__REG32ALI(0x18000488) |= 1 << 9;   //enable XIF_AGC_S2_SEL
			#ifdef CONFIG_ALI_M3528
			//When use DVB-S in Demoboard_M3528,the clk of QAM will disturb the clk of S2.
			//So it must close the clk of QAM,in order to improve S2 ACI.
			//In this situation,QAM can not be used in Demoboard_M3528.
			__REG32ALI(0x18000604) |= 1 << 8;
			#endif
	    #else
		__REG32ALI(0x18000488) &= ~(1 << 9);   //disable XIF_AGC_S2_SEL
	    __REG32ALI(0X18000488) |= 1 << 17;  //enable XIF_AGC_QAM_SEL
	    //__REG32ALI(0X1800043c) &= 0xfffffffe;  //disable GPIO[96]
	    __REG32ALI(0X18000488) &= ~(1 << 15); //disable XTUN_AGC_QAM_SEL
	    //__REG32ALI(0X18000488) |= (1 << 6);
		//__REG32ALI(0X18000488) |= (1 << 7);
	    __REG32ALI(0X1800048C) |= 1 << 18;  //enable XIF_AGC_QAM2_SEL
	    //__REG32ALI(0X1800048C) |= 1 << 20;
		//__REG32ALI(0X1800048c) &= ~(1 << 5);
		//__REG32ALI(0X1800048c) &= ~(1 << 6);
		//__REG32ALI(0X1800048c) &= ~(1 << 7);
		//__REG32ALI(0X1800006c) &= 0xFF9FFFFF;
		//__REG32ALI(0X1800006c) |= 0x00A00000;
	    
		#endif
		

	    __REG32ALI(0x18000488) |= 1 << 18;	//enable XI2C2_SEL
	    __REG32ALI(0x18000488) |= 1 << 5;   //enable XI2C_SEL
	    __REG32ALI(0x18000488) |= 1 << 16;	//enable XI2C_SEL4
	    __REG32ALI(0x1800048c) |= 1 << 20;	//enable XDISEQC_HV_BGA_SEL
	    __REG32ALI(0x18000488) |= 1 << 10;	//enable XDISEQC_OUT_SEL
	}
}

static void __init board_setting_ali(void)
{
	UINT32 chip_id = ali_sys_ic_get_chip_id();

	 // disable all GPIO for M3S23
    __REG32ALI(0x18000430) = 0;
    __REG32ALI(0x18000434) = 0;
    __REG32ALI(0x18000438) = 0;
    __REG32ALI(0x1800043c) = 0;

	nim_setting();

    /* Setting DVFS I2C device id number : 4, using Linux kernel i2c interface*/
    g_dvfs_i2c_device_id = I2C_DEVICE_UNKNOW_ID;

	/* TODO: Set GPIO here.
	*/

	alissi_setting();

	if (ALI_S3821 == chip_id)
	{
		/* usb gadget */
		//g_usb_device_5v_detect_gpio = 84;
	  	//g_usb_p0_host_5v_gpio = 0;
	    g_snd_mute_gpio = 118; // this gpio pin is set in see side
	    g_snd_chip_type = 1;
	    /* gpio i2c for M3S23 panel, sda:gpio 129, scl:gpio 128 */
		g_gpio_i2c_panel_id = I2C_DEVICE_ID;
		g_gpio_i2c_panel_sda = 129;
		g_gpio_i2c_panel_scl = 128;
		__REG32ALI(0x18000440) |= (1<<0) | (1<<1);
	}
	else if (ALI_S3503 == chip_id)
	{
		g_usb_device_5v_detect_gpio = 84;
	    g_usb_p0_host_5v_gpio = 0;

	    /* SND GPIO.
	    */
	    g_snd_mute_gpio = 19; // this gpio pin is set in see side
	    g_snd_chip_type = 1;

	    /* gpio i2c for M3515 panel, sda:gpio 134, scl:gpio135 */
		g_gpio_i2c_panel_id = I2C_DEVICE_ID;
		g_gpio_i2c_panel_sda = 134;
		g_gpio_i2c_panel_scl = 135;
		__REG32ALI(0x18000440) |= (1<<6) | (1<<7);
	}
	else if (ALI_C3505 == chip_id)
	{
	    g_snd_mute_gpio = 70; // this gpio pin is set in see side
	    g_snd_chip_type = 1;
		g_gpio_i2c_panel_id = I2C_DEVICE_ID;
		g_gpio_i2c_panel_sda = 108;
		g_gpio_i2c_panel_scl = 107;
#ifdef CONFIG_ALI_M3529_PANEL_PATCH 
		g_gpio_i2c_panel_sda = 107;
		g_gpio_i2c_panel_scl = 108;
#endif
		__REG32ALI(0x1800043C) |= (1<<11) | (1<<12);  
	}

	ali_nand_setting();
	//panel CH455 LED address
	g_dig0_addr = 0x68;
	g_dig1_addr = 0x6a;
	g_dig2_addr = 0x6c;
	g_dig3_addr = 0x6e;

	return;
}

#ifndef CONFIG_ALI_GET_DTB
struct ali_hwbuf_desc* __init ali_get_privbuf_desc
(
    int *hwbuf_desc_cnt
)
{
	struct ali_hwbuf_desc *desc = NULL;

	desc = ali_hwbuf_desc_public_256M;
	*hwbuf_desc_cnt = ARRAY_SIZE(ali_hwbuf_desc_public_256M);
	return(desc);
}
#endif

#ifdef CONFIG_ALI_CHIP_M3823
void S3821_ddr_set_de_latency(void)
{
    __REG32ALI(0x1800f904) = 0x20304080 ;
    __REG32ALI(0x1800f90c) = 0x20304080 ;
    __REG32ALI(0x1800f94c) = 0x02020202 ;
    __REG32ALI(0x1800F934) = 0x77777777;
    __REG32ALI(0x1800F954) = 0x77777777;
    __REG32ALI(0x1800f93c) = 0x18181818;

    __REG32ALI(0x18001010) = 0xfffffffd;
    __REG32ALI(0x18001018) = 0xffffff77;
    __REG32ALI(0x18001078) = 0xffffffff;
    __REG32ALI(0x180010FC) = 0x88880d00;
    __REG32ALI(0x18001104) = 0xffff4eff;
    __REG32ALI(0x1800f944) =0x4e4e4e4e;
}

void S3821_ddr_1066_set_ve_latency(void)
{
    __REG32ALI(0x1800f914) =0xffffffff;
    __REG32ALI(0x1800f91C) =0xffffffff;
}

void S3821_ddr_set_cpu_latency(void)
{
    __REG32ALI(0x18000224) |= 0x0000FFFF;
}

void board_ddr_priority_init(void)
{
	//S3821_ddr_set_cpu_latency();
	S3821_ddr_set_de_latency();
	S3821_ddr_1066_set_ve_latency();
}
#endif

void customize_board_setting(void)
{
	unsigned long ul_reg_val = 0;
	UINT32 chip_id = ali_sys_ic_get_chip_id();

	set_global_setting();
	board_setting_ali();
	ali_smc_pinmux_setting();

    #ifdef CONFIG_ALI_M36_CIC
	ali_cic_pinmux_setting();
    #endif
#ifdef CONFIG_ALI_CHIP_M3823
	board_ddr_priority_init();
#endif

	if (ALI_S3821 == chip_id)
	{
		g_gpio_i2c_hdmi_id = I2C_DEVICE_HDMI_ID;
		g_gpio_i2c_hdmi_sda = 27;
		g_gpio_i2c_hdmi_scl = 28;
		ul_reg_val = __REG32ALI(0x18000430);
		ul_reg_val |= (1<<(27-0));
		ul_reg_val |= (1<<(28-0));
		__REG32ALI(0x18000430) = ul_reg_val;
	}
	else if (ALI_S3503 == chip_id)
	{
		/* enable gpio 22, 23 for M3515 hdmi gpio i2c */
		g_gpio_i2c_hdmi_id = I2C_DEVICE_HDMI_ID;
		g_gpio_i2c_hdmi_sda = 22;
		g_gpio_i2c_hdmi_scl = 23;
		ul_reg_val = __REG32ALI(0x18000430);
		ul_reg_val |= (1<<(22-0));
		ul_reg_val |= (1<<(23-0));
		__REG32ALI(0x18000430) = ul_reg_val;
	}
	else if (ALI_C3505 == chip_id)
	{
		g_gpio_i2c_hdmi_id = I2C_DEVICE_HDMI_ID;
		g_gpio_i2c_hdmi_sda = 3;
		g_gpio_i2c_hdmi_scl = 4;
		ul_reg_val = __REG32ALI(0x18000430);
		ul_reg_val |= (1<<(3-0));
		ul_reg_val |= (1<<(4-0));
		__REG32ALI(0x18000430) = ul_reg_val;
	}
}

