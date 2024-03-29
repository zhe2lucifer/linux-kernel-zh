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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>

//replace "hdmi_ioctl.h"
#include <linux/kthread.h>
#include <linux/version.h>
#include <ali_soc.h>

#if defined(CONFIG_ALI_M39)           // OP4 SDK
#include <asm/mach-ali/m39xx.h>
#include "../include/mediatypes.h"

#define IRQ_HDMI    M39_IRQ_HDMI
#else                                 // SHA SDK
#include <asm/mach-ali/m36_irq.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/path.h>
#define SHA_SDK

#endif



#include "hdmi_proc.h"
#include "hdmi_register.h"
#include "hdmi_infoframe.h"
#include "hdmi_interrupt.h"
#include "hdmi_hdcp.h"

#include <alidefinition/adf_vpo.h>
#include <hdmi_io_common.h>

#include "ali_reg.h"

#ifdef CONFIG_CEC_ENABLE_ALI
#include "hdmi_cec.h"
#endif

#include <rpc_hld/ali_rpc_hld.h>

#if defined (CONFIG_ARM)	// For ARM CPU
//#include <../mach-ali3921/include/mach/ali_s3921.h>
//#include <ali_s3921.h>
//#define IRQ_HDMI   (ALI_SYS_IRQ_BASE + 13)
#define IRQ_HDMI   (32 + 13)
#else
#define IRQ_HDMI    M36_IRQ_HDMI
#endif

#define ALI_HDMI_DRIVER_VERSION "ali_hdmi_ver_2015_0602_1"
#define ALI_HDMI_DEVICE_NAME	"ali_hdmi_device"

const char ali_hdmi_driver_version[] = ALI_HDMI_DRIVER_VERSION;
HDMI_PRIVATE_DATA*  hdmi_drv = NULL;
FB2HDMI_INFO*       fb2hdmi_config = NULL;
ALSA2HDMI_INFO*     alsa2hdmi_config = NULL;
struct ali_hdmi_device_data ali_hdmi_dev;
static int hdmi_open_cnt = 0;
static int hdmi_init_cnt = 0;
extern struct VENDOR_PRODUCT_BLOCK     *vendor_product_block;
extern unsigned char monitor_name[14];

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
long ali_hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
int ali_hdmi_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#endif
int ali_hdmi_open(struct inode *inode, struct file *file);
int ali_hdmi_close(struct inode *inode, struct file *file);
static unsigned int ali_hdmi_poll (struct file *file, poll_table *wait);


void ali_hdmi_set_video_info(FB2HDMI_INFO* fb2hdmi_info );
void ali_hdmi_set_audio_info(ALSA2HDMI_INFO* alsa2hdmi_info );



static struct file_operations  ali_hdmi_fops = {
	.owner			= THIS_MODULE,
	.open 			= ali_hdmi_open,
	.release 		= ali_hdmi_close,
	.read			= NULL,
	.write			= NULL,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	.unlocked_ioctl = ali_hdmi_ioctl, 
#else
	.ioctl			= ali_hdmi_ioctl,
#endif	
	.mmap			= NULL,
	.poll 			= ali_hdmi_poll,
};

// For SHA
typedef struct
{
	unsigned char hdmi_sig[4];
	unsigned char setting_valid;
	unsigned char *hdcp; 		//hdcp key addr.
	unsigned char reserved;
} HDMI_BOOT_PARAM_t, *PHDMI_BOOT_PARAM_t;

#if defined(CONFIG_ALI_M39) //different from 39

static int ali_hdmi_major = 240;

#else

static int ali_hdmi_major = 0;
struct class *ali_hdmi_class = NULL;
struct device *ali_hdmi_node = NULL;

#endif

static bool hdmi_sw_onoff_from_user = true;
int hdmi_module_param_hdcp_onoff = 1; // hdmi module param
module_param(hdmi_module_param_hdcp_onoff, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hdmi_module_param_hdcp_onoff, "An integer");


enum HDMI_API_RES api_hdmi_info_video_res = HDMI_RES_INVALID;
HDMI_ioctl_hdcp_key_info_t UserParams;

// For ZHA
int ali_hdmi_hardware_init(void)
{
	hdmi_proc_driver_init();
	if(fb2hdmi_config != NULL) // Frame buffer has been set hdmi before.
	{
		HDMI_DEBUG("Frame buffer has been set hdmi before.\n ");
	    ali_hdmi_set_video_info(fb2hdmi_config);
	    kfree(fb2hdmi_config);
	    fb2hdmi_config = NULL;
	}
	else
	{
		HDMI_DEBUG("Frame buffer has not been set hdmi before.\n ");    	
	}

	if(alsa2hdmi_config != NULL) // Frame buffer has been set hdmi before.
	{
	    ali_hdmi_set_audio_info(alsa2hdmi_config);
	    kfree(alsa2hdmi_config);
	    alsa2hdmi_config = NULL;
	}
	return 0; //success
}

#ifdef CONFIG_HDCP_ENABLE_ALI
// HDCP Feature
/* api function switch hdcp on/off before module init*/
void ali_hdmi_set_module_init_hdcp_onoff(bool bOnOff)
{
	hdmi_module_param_hdcp_onoff = bOnOff;
	HDMI_DEBUG("%s : %s.\n", __FUNCTION__, (bOnOff) ? "on" : "off");
}

// HDCP Feature
/* api function switch hdcp on/off and status */
BOOL ali_hdmi_get_hdcp_onoff(void)
{
	if(hdmi_drv != NULL)
	{
		return hdmi_drv->control.hdcp_enable;
	}
	return false;
}

void ali_hdmi_set_hdcp_onoff(bool bOnOff)
{
	if(hdmi_drv != NULL)
	{
		if(bOnOff && (hdmi_drv->control.hdcp_enable == false))
		{
			HDMI_DEBUG("%s : on\n", __FUNCTION__);
			hdmi_drv->control.hdcp_enable = true;
		}
		else
		{
			HDMI_DEBUG("%s : off\n", __FUNCTION__);
			hdmi_drv->control.hdcp_enable = false;
		}
		hdmi_drv->hdcp.authenticated = false;       // Re-Authentication
	}
}

void ali_hdmi_set_mem_sel(bool mem_sel)    //add by ze, for ce_load_key,1:ce_load_key, 0:sw_load_key
{
	if(hdmi_drv != NULL)
	{
		if(mem_sel)
		{
			HDMI_DEBUG("%s : OTP mode\n", __FUNCTION__);
			hdmi_drv->control.load_key_from_ce = true;
			HDMI_REG_CFG1 |= B_MEM_SEL;
		}
		else
		{
			HDMI_DEBUG("%s : SRAM mode\n", __FUNCTION__);
			hdmi_drv->control.load_key_from_ce = false;
			HDMI_REG_CFG1 &= (~B_MEM_SEL);			
		}
		//hdmi_drv->hdcp.authenticated = false;       // Re-Authentication
	}
}
#endif // end of CONFIG_HDCP_ENABLE_ALI

#ifdef CONFIG_CEC_ENABLE_ALI
// CEC Feature
/* api function switch cec on/off and status */
bool ali_hdmi_get_cec_onoff(void)
{
	if(hdmi_drv != NULL)
		return hdmi_drv->control.cec_enable;
	else
		return false;	
}

void ali_hdmi_set_cec_onoff(bool bOnOff)
{
	if (hdmi_drv != NULL)
		hdmi_drv->control.cec_enable = bOnOff;
}

bool ali_hdmi_set_logical_address(unsigned char logical_address)
{
	if(hdmi_drv == NULL)
		return false;

	if (hdmi_drv->control.cec_enable)
	{
		hdmi_drv->cec.logical_address = logical_address;
		hdmi_set_cec_logic_address(logical_address);
		return true;
	}
	else
		return false;
}

unsigned char ali_hdmi_get_logical_address(void)
{
	return hdmi_get_cec_logic_address();
}

bool ali_hdmi_cec_transmit(unsigned char* message, unsigned char message_length)
{
	if(hdmi_drv == NULL)
		return false;
		
	if (hdmi_drv->control.cec_enable)
	{	
		return hdmi_cec_transmit(message, message_length);
	}
	else
		return false;
}
#endif // end of CONFIG_CEC_ENABLE_ALI

void ali_hdmi_switch_onoff(bool turn_on)
{
    HDMI_DEBUG("%s: turn hdmi %s.\n", __FUNCTION__, (turn_on) ? "on" : "off");
    if(hdmi_drv != NULL)
    {   
        hdmi_drv->control.hdmi_enable = turn_on;
        if(turn_on == true)
        {
        	delay_tick(200);
        }
        hdmi_proc_state_update();
    }
	else
	{
		HDMI_DEBUG("%s error \n", __FUNCTION__);
	}
}

void ali_hdmi_audio_switch_onoff(BOOL turn_on)
{
    HDMI_DEBUG("%s : turn hdmi audio %s.\n", __FUNCTION__, (turn_on) ? "on" : "off");
    if(hdmi_drv != NULL)
    {
        hdmi_drv->control.hdmi_audio_enable = turn_on;
        // Turn on or turn off Audio
        (turn_on) ? (HDMI_REG_CFG6 |= B_AUDIO_CAP_RST) : (HDMI_REG_CFG6 &= (~B_AUDIO_CAP_RST));
    }
}

BOOL ali_hdmi_audio_get_onoff(void)
{
    HDMI_DEBUG("%s : get hdmi audio %s.\n", __FUNCTION__, (hdmi_drv->control.hdmi_audio_enable) ? "on" : "off");

	return hdmi_drv->control.hdmi_audio_enable;
}

void api_set_hdmi_deep_color(enum HDMI_API_DEEPCOLOR deep_color)
{
	HDMI_DEBUG("%s : set deep color %d.\n", __FUNCTION__, deep_color);
	if(hdmi_drv != NULL)
	{
		hdmi_proc_set_avmute(true, 3);
		hdmi_drv->control.deep_color = deep_color;	
		hdmi_proc_hdmi_phy_output();
		hdmi_proc_gcp_update();
		udelay(5);
		hdmi_proc_transmit_infoframe(&hdmi_drv->gcp.infoframe);
		udelay(5);
		hdmi_proc_set_avmute(false, 3);
	}
	else
	{
		HDMI_DEBUG("%s error \n", __FUNCTION__);
	}
}
unsigned char api_get_hdmi_deep_color(void)
{
	if(hdmi_drv != NULL)
		return hdmi_drv->control.deep_color;
	else
		return false;
}
void api_set_hdmi_phy_clk_onoff(bool turn_on)
{
	if(hdmi_drv != NULL)
	{
    		HDMI_DEBUG("%s :  %s.\n", __FUNCTION__, (turn_on) ? "on" : "off");
		hdmi_proc_set_phy_clk_onoff(turn_on);
	}
}
void api_set_hdmi_color_space(enum HDMI_API_COLOR_SPACE color_space)
{
	HDMI_DEBUG("%s : %d.\n", __FUNCTION__, color_space);
	if(hdmi_drv != NULL)
	{
		hdmi_proc_set_avmute(true, 3);
		hdmi_drv->control.color_space = color_space;
		hdmi_proc_config_color_space(color_space);
		hdmi_proc_avi_infoframe_update();
		udelay(50);
		hdmi_proc_transmit_infoframe(&hdmi_drv->video.infoframe);
		udelay(50);
		hdmi_proc_set_avmute(false, 3);
	}
}
unsigned char api_get_hdmi_color_space(void)
{
	if(hdmi_drv != NULL)
		return hdmi_drv->control.color_space;
	else
		return false;
}

unsigned char api_get_edid_deep_color(void)
{
	if(hdmi_drv != NULL)
		return hdmi_drv->edid.deep_color;
	else
		return false;
}

void api_set_hdmi_mono_blank(bool turn_on)
{
	HDMI_DEBUG("%s : turn hdmi mono blank %s.\n", __FUNCTION__, (turn_on) ? "on" : "off");
	(turn_on) ? (HDMI_REG_I2S_UV |= B_NORMAL_MONO_SEL) : (HDMI_REG_I2S_UV &= (~B_NORMAL_MONO_SEL));
}

/* api function to get the hdcp capability of a hdmi sink device */
bool api_get_hdmi_sink_hdcp_cap(void)
{
	bool ret = false;
	unsigned char BKsv[5];

	if (HOT_PLUG_STATE)
	{
		if( hdmi_hdcp_read_from_sink(HDCP_RX_PRI_BKSV, BKsv, sizeof(BKsv)) == true)
		{
			if(hdmi_hdcp_validate_ksv(BKsv) == true)
			{
				ret = true;
			}
			HDMI_DEBUG("\tBKsv: %.2x %.2x %.2x %.2x %.2x\n", BKsv[0], BKsv[1], BKsv[2], BKsv[3], BKsv[4]);
		}
	}
	return ret;			
}

unsigned int api_get_hdmi_state(void)
{
	return hdmi_drv->link_status;
}

void ali_hdmi_avmute(bool avmute)
{
    if(hdmi_drv != NULL)
    {
    	HDMI_DEBUG("%s: avmute = %s.\n", __FUNCTION__, (avmute) ? "TRUE" : "FALSE");    	
		hdmi_proc_set_avmute(avmute, 1);
	}
}

void hdmi_stop_transmit(void)
{
	
	HDMI_REG_CTRL |= B_SRST;
	udelay(5);
	
    HDMI_REG_HDCP_CTRL |= B_CP_RST;
    udelay(5);
    HDMI_REG_HDCP_CTRL &= ~B_CP_RST;
    udelay(5);            
	
	// Stop TMDS clock frequency
	hdmi_proc_set_phy_onoff(false);
}

void ali_hdmi_set_audio_info(ALSA2HDMI_INFO* alsa2hdmi_info )
{
    if(hdmi_drv == NULL)
    {
        // hdmi driver not init yet.
        HDMI_DEBUG("%s: hdmi driver not init yet, record setting first.\n", __FUNCTION__);
	    alsa2hdmi_config = kmalloc(sizeof(ALSA2HDMI_INFO), GFP_KERNEL);
        memcpy(alsa2hdmi_config, alsa2hdmi_info, sizeof(ALSA2HDMI_INFO));
        return;
    }

    if((hdmi_drv->control.valid_aud_info == false)||(memcmp(&hdmi_drv->audio.config, alsa2hdmi_info, sizeof(ALSA2HDMI_INFO))!= 0))
    {
		// Turn off Audio
		HDMI_REG_CFG6 &= (~B_AUDIO_CAP_RST);
		udelay(5);
		HDMI_REG_CTRL &= (~B_AUDIO_EN);
		udelay(5);
		HDMI_REG_CFG5 |= B_I2S_SWITCH;
		HDMI_REG_CFG5 |= B_SPDIF_SWITCH;
		udelay(5);
        // Switch to I2S buffer for draft version
        hdmi_drv->control.valid_aud_info = true;
		HDMI_DEBUG("%s: infoframe update.\n", __FUNCTION__);
        memcpy(&hdmi_drv->audio.config, alsa2hdmi_info, sizeof(ALSA2HDMI_INFO));
        hdmi_proc_audio_interface_config();
        hdmi_proc_audio_infoframe_update();
        hdmi_proc_audio_n_cts_update();
        hdmi_proc_transmit_infoframe(&hdmi_drv->audio.infoframe);
		// Turn on Audio
		(HDMI_REG_CFG1 & B_SPDIF) ? (HDMI_REG_CFG5 &= ~(B_SPDIF_SWITCH)) : (HDMI_REG_CFG5 &= (~B_I2S_SWITCH));
        udelay(5);
        HDMI_REG_CTRL |= B_AUDIO_EN;
        udelay(5);
		if (hdmi_drv->control.hdmi_audio_enable)
        HDMI_REG_CFG6 |= B_AUDIO_CAP_RST;
        udelay(5);
    }
    else
	{
		HDMI_DEBUG("%s: nothing updated.\n", __FUNCTION__);
	}
	
		if(hdmi_drv->control.hdmi_audio_enable)  //fixed #86137
		{
				HDMI_REG_CFG6 |= B_AUDIO_CAP_RST;
	  }
    return;
}

void ali_hdmi_set_video_info(FB2HDMI_INFO* fb2hdmi_info )
{
	if(hdmi_drv == NULL)
    {
        // hdmi driver not init yet.
        HDMI_DEBUG("%s: hdmi driver not init yet, record setting first.\n",__FUNCTION__);
	    fb2hdmi_config = kmalloc(sizeof(FB2HDMI_INFO), GFP_KERNEL);
        memcpy(fb2hdmi_config, fb2hdmi_info, sizeof(FB2HDMI_INFO));
        return;
    }
    
    if((hdmi_drv->control.valid_avi_info == false) ||
		(memcmp(&hdmi_drv->video.config, fb2hdmi_info, sizeof(FB2HDMI_INFO))!= 0))
    {
		HDMI_DEBUG("%s: infoframe update.\n", __FUNCTION__);
		memcpy(&hdmi_drv->video.config, fb2hdmi_info, sizeof(FB2HDMI_INFO));

		hdmi_proc_avi_infoframe_update();
		udelay(50);
		hdmi_proc_transmit_infoframe(&hdmi_drv->video.infoframe);
		udelay(50);
#ifdef CONFIG_ALI_CHIP_M3921
		hdmi_proc_vsi_infoframe_update();
		udelay(50);
		hdmi_proc_transmit_infoframe(&hdmi_drv->vsi.infoframe);
		udelay(50);
#endif
#ifdef CONFIG_CEC_ENABLE_ALI
		if (hdmi_drv->control.cec_enable)
			hdmi_set_biu_frequency();
#endif

	if(hdmi_drv->control.valid_avi_info == false)
        {
            //rick debug, not get valid avi yet, set phy clock
            hdmi_proc_hdmi_phy_output();
            
            hdmi_drv->control.valid_avi_info = true;
	}

    }
	else
	{
 	    HDMI_DEBUG("%s: but nothing updated\n", __FUNCTION__);
	}
}

#ifdef SHA_SDK
void print_snd2hdmi_info(struct snd2Hdmi_audio_infor *audio_info)
{
	HDMI_DEBUG("av_chg_ste = %d\n",			(int)audio_info->av_chg_ste);
	HDMI_DEBUG("user_def_ch_num = %d\n",	(int)audio_info->user_def_ch_num);
	HDMI_DEBUG("pcm_out = %d\n", 			(int)audio_info->pcm_out );
	HDMI_DEBUG("coding_type = %d\n", 		(int)audio_info->coding_type);
	HDMI_DEBUG("max_bit_rate = %d\n",		(int)audio_info->max_bit_rate);
	HDMI_DEBUG("ch_count = %d\n", 			(int)audio_info->ch_count);
	HDMI_DEBUG("sample_rate = %d\n", 		(int)audio_info->sample_rate );
	HDMI_DEBUG("level_shift = %d\n", 		(int)audio_info->level_shift);
	HDMI_DEBUG("spdif_edge_clk = %d\n", 	(int)audio_info->spdif_edge_clk);
	HDMI_DEBUG("ch_status = 0x%x\n", 		(int)audio_info->ch_status);
	HDMI_DEBUG("ch_position = 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x \n", 
		audio_info->ch_position[0], audio_info->ch_position[1], audio_info->ch_position[2], audio_info->ch_position[3], 
		audio_info->ch_position[4], audio_info->ch_position[5], audio_info->ch_position[6], audio_info->ch_position[7] );
	HDMI_DEBUG("bclk_lrck = %d\n", 			(int)audio_info->bclk_lrck);
	HDMI_DEBUG("word_length = %d\n", 		(int)audio_info->word_length);
	HDMI_DEBUG("i2s_edge_clk = %d\n", 		(int)audio_info->i2s_edge_clk);
	HDMI_DEBUG("i2s_format = %d\n", 		(int)audio_info->i2s_format);
	HDMI_DEBUG("lrck_hi_left = %d\n", 		(int)audio_info->lrck_hi_left);

}

/* For SHA TDS Interface backward compatible */
void set_audio_info_to_hdmi(UINT32 param)
{
	struct snd2Hdmi_audio_infor *audio_info = (struct snd2Hdmi_audio_infor *)(param);
    ALSA2HDMI_INFO hdmi_audio_config;

	HDMI_DEBUG("*******************************\n");
	HDMI_DEBUG("%s in: %d\n", __FUNCTION__,audio_info->av_chg_ste);	
	print_snd2hdmi_info(audio_info);
	if(audio_info->av_chg_ste == HDMI_CB_AV_INFO_CHG)
	{
		audio_info->av_chg_ste = HDMI_CB_NOTHING;
        // Test to update Audio
        hdmi_audio_config.user_audio_out    = audio_info->pcm_out;		//AUD_USR_BITSTREAM_OUT
        hdmi_audio_config.coding_type       = audio_info->coding_type;	//AUD_CODING_PCM;
        hdmi_audio_config.channel_count     = audio_info->ch_count;		//2

        switch(audio_info->sample_rate)
        {
            case 0:      hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_UNKNOWN;   break;
            case 32000:  hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_32KHZ;     break;
            case 44100:  hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_44_1KHZ;   break;
            case 88200:  hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_88_2KHZ;   break;
            case 176400: hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_176_4KHZ;  break;
            case 48000:  hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_48KHZ;     break;
            case 96000:  hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_96KHZ;     break;
            case 192000: hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_192KHZ;    break;
			case 768000: hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_768KHZ;    break;
			default:     hdmi_audio_config.sample_rate   = AUD_SAMPLERATE_192KHZ;    break;
        }
		switch(audio_info->i2s_format)
		{
			case 0:	hdmi_audio_config.i2s_format = I2S_FMT_I2S;				break;
			case 1:	hdmi_audio_config.i2s_format = I2S_FMT_LEFT_JUSTIFIED;	break;
			case 2:	hdmi_audio_config.i2s_format = I2S_FMT_RIGHT_JUSTIFIED;	break;
		}
        hdmi_audio_config.lrck_hi_left      = (audio_info->lrck_hi_left) ? true:false;
		switch(audio_info->word_length)
		{
			case 16:	hdmi_audio_config.word_length = I2S_FMT_WLENGTH_16;	break;
			case 24:	hdmi_audio_config.word_length = I2S_FMT_WLENGTH_24;	break;
			case 28:	hdmi_audio_config.word_length = I2S_FMT_WLENGTH_28;	break;
		}
        hdmi_audio_config.channel_status.data_uint32 = audio_info->ch_status;
        ali_hdmi_set_audio_info(&hdmi_audio_config);
	}

    return;
}

void print_de2hdmi_info(struct de2Hdmi_video_infor *video_info)
{

	HDMI_DEBUG("av_chg_ste = %d\n",video_info->av_chg_ste);
	HDMI_DEBUG("tv_mode = %d, %dx%d,\n", video_info->tv_mode,video_info->width,video_info->height);
	HDMI_DEBUG("format = %d\n",video_info->format);	
	HDMI_DEBUG("scan_mode = %s\n",(video_info->scan_mode) ? "TRUE":"FALSE");	
	HDMI_DEBUG("afd_present = %s\n",(video_info->afd_present) ? "TRUE":"FALSE");		
	HDMI_DEBUG("output_aspect_ratio = %d\n",video_info->output_aspect_ratio);	
	HDMI_DEBUG("active_format_aspect_ratio = %d\n",video_info->active_format_aspect_ratio);
	HDMI_DEBUG("_4K_VIC_3D_structure = %d\n",video_info->_4K_VIC_3D_structure);
	HDMI_DEBUG("ext_video_format = %d\n",video_info->ext_video_format);
	HDMI_DEBUG("ext_data = %d\n",video_info->ext_data);
}
void ali_hdmi_init_video_param(struct de2Hdmi_video_infor *video_info)
{
	FB2HDMI_INFO hdmi_video_config;
	HDMI_DEBUG("%s : \n", __FUNCTION__);
	
	//print_de2hdmi_info(video_info);

	switch(video_info->tv_mode)
	{
		case PAL:			hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_576P : TV_MODE_PAL; 	 break;
		case PAL_N: 		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_576P : TV_MODE_PAL; 	 break;
		case PAL_60:		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
		case NTSC:			hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
		case PAL_M: 		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
		case NTSC_443:		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
		case SECAM: 		hdmi_video_config.resolution = TV_MODE_SECAM;									break;
		case LINE_720_25:	hdmi_video_config.resolution = TV_MODE_720P_50; 								break;
		case LINE_720_30:	hdmi_video_config.resolution = TV_MODE_720P_60; 								break;
		case LINE_1080_25:	hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_1080P_25 : TV_MODE_1080I_25;  break;
		case LINE_1080_30:	hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_1080P_30 : TV_MODE_1080I_30;  break;
		case LINE_1080_24:	hdmi_video_config.resolution = TV_MODE_1080P_24;								break;
		case LINE_1080_50:	hdmi_video_config.resolution = TV_MODE_1080P_50;								break;
		case LINE_1080_60:	hdmi_video_config.resolution = TV_MODE_1080P_60;								break;
		case LINE_4096X2160_24: hdmi_video_config.resolution = TV_MODE_4096X2160_24;	break;
		case LINE_3840X2160_24: hdmi_video_config.resolution = TV_MODE_3840X2160_24;	break;
		case LINE_3840X2160_25: hdmi_video_config.resolution = TV_MODE_3840X2160_25;	break;
		case LINE_3840X2160_30: hdmi_video_config.resolution = TV_MODE_3840X2160_30;	break;
		default:			HDMI_DEBUG("%s: tv_mode not support!\n", __FUNCTION__); 						break;
	}
	hdmi_video_config.color_format = video_info->format;
	hdmi_video_config.aspect_ratio = video_info->output_aspect_ratio;
	hdmi_video_config.afd_present  = video_info->afd_present;
#ifdef CONFIG_ALI_CHIP_M3921
	hdmi_video_config._4K_VIC_3D_structure = video_info->_4K_VIC_3D_structure;
	hdmi_video_config.ext_data = video_info->ext_data;
	hdmi_video_config.ext_video_format = video_info->ext_video_format;
#endif
	memcpy(&hdmi_drv->video.config, &hdmi_video_config, sizeof(FB2HDMI_INFO));
}

void set_video_info_to_hdmi(UINT32 param)
{
	struct de2Hdmi_video_infor *video_info = (struct de2Hdmi_video_infor *)(param);    
	FB2HDMI_INFO hdmi_video_config;
	struct de2Hdmi_video_infor info;
	extern struct vpo_device *g_vpo_dev;
	unsigned char change_state = 0x00;
	
	memset(&info, 0, sizeof(info));
	HDMI_DEBUG("*******************************\n");
	//HDMI_DEBUG("%s in: jiffies %lu\n", __FUNCTION__, jiffies);	
	HDMI_DEBUG("%s in: av_chg_state %d \n", __FUNCTION__, video_info->av_chg_ste);	
	vpo_ioctl(g_vpo_dev, VPO_IO_GET_DE2HDMI_INFO, (UINT32) &info);
    
    if(hdmi_drv->control.hdmi_enable == false)
    	info.av_chg_ste = HDMI_CB_CLK_CHG_DONE | HDMI_CB_AV_INFO_CHG;
    
	video_info = &info;
	print_de2hdmi_info(video_info);
	change_state = video_info->av_chg_ste;
	//switch(video_info->av_chg_ste)
	switch(change_state)
	{
		case HDMI_CB_CLR_RDY2CHG:
		case HDMI_CB_CLK_RDY2CHG:
			ali_hdmi_switch_onoff(false);
		break;
		case HDMI_CB_AV_INFO_CHG:
		case HDMI_CB_CLK_CHG_DONE | HDMI_CB_AV_INFO_CHG:
			// Test to update AVI Infoframe
			/*
				not support  MAC, LINE_1152_ASS, LINE_1080_ASS,LINE_576P_50_VESA,
				LINE_720P_60_VESA,LINE_1080P_60_VESA module
			*/
			switch(video_info->tv_mode)
			{
				case PAL:			hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_576P : TV_MODE_PAL;      break;
				case PAL_N:			hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_576P : TV_MODE_PAL;      break;
				case PAL_60:		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
				case NTSC:		    hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
				case PAL_M:		    hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
				case NTSC_443:		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
				case SECAM:			hdmi_video_config.resolution = TV_MODE_SECAM;                                   break;
				case LINE_720_25:	hdmi_video_config.resolution = TV_MODE_720P_50;                                 break;
				case LINE_720_30:	hdmi_video_config.resolution = TV_MODE_720P_60;                                 break;
				case LINE_1080_25:	hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_1080P_25 : TV_MODE_1080I_25;  break;
				case LINE_1080_30:	hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_1080P_30 : TV_MODE_1080I_30;  break;
				case LINE_1080_24:	hdmi_video_config.resolution = TV_MODE_1080P_24;                                break;
				case PAL_NC: 		hdmi_video_config.resolution = (video_info->scan_mode)? TV_MODE_576P : TV_MODE_PAL;      break;
				case LINE_1080_50:	hdmi_video_config.resolution = TV_MODE_1080P_50;                                break;
				case LINE_1080_60:	hdmi_video_config.resolution = TV_MODE_1080P_60;                                break;
#if (defined(CONFIG_ALI_CHIP_M3921))
				case LINE_4096X2160_24:	hdmi_video_config.resolution = TV_MODE_4096X2160_24;	break;
				case LINE_3840X2160_24:	hdmi_video_config.resolution = TV_MODE_3840X2160_24;	break;
				case LINE_3840X2160_25:	hdmi_video_config.resolution = TV_MODE_3840X2160_25;	break;
				case LINE_3840X2160_30:	hdmi_video_config.resolution = TV_MODE_3840X2160_30;	break;				
#endif
				default:	hdmi_video_config.resolution = TV_MODE_1080P_60; printk(KERN_WARNING"%s: tv_mode not support! error error!!\n", __FUNCTION__); break;
			}
			hdmi_video_config.color_format = video_info->format;
			hdmi_video_config.aspect_ratio = video_info->output_aspect_ratio;
			hdmi_video_config.afd_present  = video_info->afd_present;
			hdmi_video_config.afd          = video_info->active_format_aspect_ratio;
#ifdef CONFIG_ALI_CHIP_M3921
			hdmi_video_config._4K_VIC_3D_structure = video_info->_4K_VIC_3D_structure;
			hdmi_video_config.ext_data = video_info->ext_data;
			hdmi_video_config.ext_video_format = video_info->ext_video_format;
#endif
			ali_hdmi_set_video_info(&hdmi_video_config);
			if(video_info->av_chg_ste == (HDMI_CB_CLK_CHG_DONE | HDMI_CB_AV_INFO_CHG))
			{
				hdmi_proc_hdmi_phy_output();
			}
			
			if(hdmi_sw_onoff_from_user)
				ali_hdmi_switch_onoff(true);

			if (video_info->format == RGB_MODE1 || video_info->format == RGB_MODE2)
				HDMI_REG_I2S_UV &= ~B_RGB_YCBCR_MONO;  // RGB
			else
				HDMI_REG_I2S_UV |= B_RGB_YCBCR_MONO;   // YCbCr
				
			break;
		case HDMI_CB_NOTHING:
			ali_hdmi_init_video_param(video_info);
		default:
            break;
	}
	HDMI_DEBUG("exit set_video_info_to_hdmi jiffies %lu\n", jiffies);	
    return;
}
#endif

// bootloader parameter for SHA
/* api function set boot loader parameter */
void ali_hdmi_set_bootloader_param(void *pParam)
{
	HDMI_BOOT_PARAM_t *boot_param = (HDMI_BOOT_PARAM_t *)pParam;
	struct de2Hdmi_video_infor video_info;    
	FB2HDMI_INFO hdmi_video_config;
	extern struct vpo_device *g_vpo_dev;

	memset(&video_info, 0, sizeof(video_info));
	HDMI_DEBUG("%s : \n", __FUNCTION__);
	
	if(hdmi_drv == NULL)
    {
        // hdmi driver not init yet.
        HDMI_DEBUG("%s: hdmi driver not init yet, record setting first.\n", __FUNCTION__);
        return;
    }
	
	if(boot_param->setting_valid)
	{
		hdmi_drv->control.valid_avi_info = true;

		if(boot_param->hdcp)
		{
			memcpy(hdmi_drv->hdcp.key, boot_param->hdcp, 286);
			HDMI_DEBUG("%s: hdmi hdcp key addr = 0x%X\n", __FUNCTION__, (unsigned int)boot_param->hdcp);
		}
		
		vpo_ioctl(g_vpo_dev, VPO_IO_GET_DE2HDMI_INFO, (UINT32) &video_info);
	    
//		print_de2hdmi_info(&video_info);	
	    // Update AVI Infoframe
	    switch(video_info.tv_mode)
		{
	    	case PAL:			hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_576P : TV_MODE_PAL;      break;
	    	case PAL_N:			hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_576P : TV_MODE_PAL;      break;
	    	case PAL_60:		hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
	    	case NTSC:		    hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
	    	case PAL_M:		    hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
	    	case NTSC_443:		hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_480P : TV_MODE_NTSC443;  break;
	    	case SECAM:			hdmi_video_config.resolution = TV_MODE_SECAM;                                   break;
	    	case LINE_720_25:	hdmi_video_config.resolution = TV_MODE_720P_50;                                 break;
	    	case LINE_720_30:	hdmi_video_config.resolution = TV_MODE_720P_60;                                 break;
	    	case LINE_1080_25:	hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_1080P_25 : TV_MODE_1080I_25;  break;
	    	case LINE_1080_30:	hdmi_video_config.resolution = (video_info.scan_mode)? TV_MODE_1080P_30 : TV_MODE_1080I_30;  break;
	        case LINE_1080_24:	hdmi_video_config.resolution = TV_MODE_1080P_24;                                break;
	        case LINE_1080_50:	hdmi_video_config.resolution = TV_MODE_1080P_50;                                break;
	        case LINE_1080_60:	hdmi_video_config.resolution = TV_MODE_1080P_60;                                break;
			case LINE_4096X2160_24:	hdmi_video_config.resolution = TV_MODE_4096X2160_24;	break;
			case LINE_3840X2160_24:	hdmi_video_config.resolution = TV_MODE_3840X2160_24;	break;
			case LINE_3840X2160_25:	hdmi_video_config.resolution = TV_MODE_3840X2160_30;	break;
			case LINE_3840X2160_30:	hdmi_video_config.resolution = TV_MODE_3840X2160_30;	break;
			default:			HDMI_DEBUG("%s: tv_mode not support!\n", __FUNCTION__);							break;
		}
	    hdmi_video_config.color_format = video_info.format;
	    hdmi_video_config.aspect_ratio = video_info.output_aspect_ratio;
	    hdmi_video_config.afd_present  = video_info.afd_present;
	    ali_hdmi_set_video_info(&hdmi_video_config);
	}
	else
	{
		hdmi_drv->control.valid_avi_info = false;
	}
}

static unsigned int ali_hdmi_poll (struct file *file, poll_table *wait)
{
	struct ali_hdmi_device_data *dev = (struct ali_hdmi_device_data*)file->private_data;
	HDMI_PRIVATE_DATA *priv = dev->priv;
	unsigned int mask = 0;
	poll_wait(file, &priv->poll_plug_wq, wait);

	if (priv->plug_status == 0x5a5a5a5a)
	{
		printk("%s: hdmi plug in!\n", __FUNCTION__);
		//clear status.
		priv->plug_status = 0; 
		mask |= POLLIN;
	}
	else if (priv->plug_status == 0xa5a5a5a5)
	{
		printk("%s: hdmi plug out!\n", __FUNCTION__);
		//clear status.
		priv->plug_status = 0;
		mask |= POLLOUT;
	}

	return mask;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
long ali_hdmi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
int ali_hdmi_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{

	struct ali_hdmi_device_data *dev = file->private_data;
	HDMI_PRIVATE_DATA *priv = dev->priv;
	int argu = (int)arg;
	int ret = 0;
	void *pUserParams;
	
	if(hdmi_drv->control.edid_ready == false) 
	{
		hdmi_proc_get_parse_edid();
		if(1==hdmi_drv->edid.block[0][0x12])
		{
			HDMI_DEBUG("%s: HDMI_IOCT_GET_EDID: EDID ready!\n", __FUNCTION__);
		}
		else
		{
			HDMI_DEBUG("%s: HDMI_IOCT_GET_EDID: EDID not ready!\n", __FUNCTION__);
			hdmi_proc_clear_edid();
		}
	}			

	if (mutex_lock_interruptible(&hdmi_drv->hdmi_mutex))
		return -EBUSY;

	switch (cmd)
	{
		default:
			mutex_unlock(&hdmi_drv->hdmi_mutex);
			ret = ENOIOCTLCMD;
			break;

		case HDMI_IOCT_SET_ONOFF:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_sw_onoff_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_sw_onoff_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_sw_onoff_state_t *)arg, sizeof(HDMI_ioctl_sw_onoff_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}

				ali_hdmi_switch_onoff((bool) ((HDMI_ioctl_sw_onoff_state_t *)pUserParams)->hdmi_status);
				hdmi_sw_onoff_from_user = (bool)((HDMI_ioctl_sw_onoff_state_t *)pUserParams)->hdmi_status;
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_GET_ONOFF:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_sw_onoff_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_sw_onoff_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_sw_onoff_state_t *)arg, sizeof(HDMI_ioctl_sw_onoff_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_sw_onoff_state_t *)pUserParams)->hdmi_status = hdmi_drv->control.hdmi_enable;

				if ((ret = copy_to_user((HDMI_ioctl_sw_onoff_state_t *)arg, pUserParams, sizeof(HDMI_ioctl_sw_onoff_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
			
#ifdef CONFIG_HDCP_ENABLE_ALI
		case HDMI_IOCT_HDCP_GET_STATUS:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				if((hdmi_drv->hdcp.authenticated) && (hdmi_drv->control.hdcp_enable == true))
				{
					HDMI_DEBUG("%s authenticated success!\n", __FUNCTION__);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return true;
				}
				else
				{
					HDMI_DEBUG("%s authenticated fail!\n", __FUNCTION__);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return false;				
				}
			}
			break;

		case HDMI_IOCT_HDCP_SET_ONOFF:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_hdcp_state_t UserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_hdcp_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_hdcp_state_t *)arg, sizeof(HDMI_ioctl_hdcp_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				ali_hdmi_set_hdcp_onoff((bool) ((HDMI_ioctl_hdcp_state_t *)pUserParams)->hdcp_status);
				priv->control.hdcp_enable = ((HDMI_ioctl_hdcp_state_t *)pUserParams)->hdcp_status;
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_HDCP_GET_ONOFF:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_hdcp_state_t UserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_hdcp_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_hdcp_state_t *)arg, sizeof(HDMI_ioctl_hdcp_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_hdcp_state_t *)pUserParams)->hdcp_status = ali_hdmi_get_hdcp_onoff();

				if ((ret = copy_to_user((HDMI_ioctl_hdcp_state_t *)arg, pUserParams, sizeof(HDMI_ioctl_hdcp_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_HDCP_SET_KEY_INFO:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				if ((ret = copy_from_user(&UserParams, (HDMI_ioctl_hdcp_key_info_t *)arg, sizeof(HDMI_ioctl_hdcp_key_info_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}

				*(hdmi_drv->hdcp.key) = UserParams.scramble;
				memcpy((hdmi_drv->hdcp.key+1), UserParams.hdcp_ksv, 5);
				memcpy((hdmi_drv->hdcp.key+6), UserParams.encrypted_hdcp_keys, 280);
			}
			break;
			
		case HDMI_IOCT_HDCP_MEM_SEL:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				if(hdmi_open_cnt != 0)	
				{
					break;
				}
				hdmi_open_cnt = 1;
				
				//HDMI_ioctl_mem_sel_t UserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_mem_sel_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_mem_sel_t *)arg, sizeof(HDMI_ioctl_mem_sel_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				ali_hdmi_set_mem_sel((bool) ((HDMI_ioctl_mem_sel_t *)pUserParams)->mem_sel);
				//priv->control.hdcp_enable = UserParams.hdcp_status;
				kfree(pUserParams);
			}
			break;
			
#endif  //CONFIG_HDCP_ENABLE_ALI

#ifdef CONFIG_CEC_ENABLE_ALI

		case HDMI_IOCT_CEC_SET_ONOFF:
			{
				//HDMI_ioctl_cec_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_cec_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_cec_state_t *)arg, sizeof(HDMI_ioctl_cec_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				ali_hdmi_set_cec_onoff((bool) ((HDMI_ioctl_cec_state_t *)pUserParams)->cec_status);
				priv->control.cec_enable = ((HDMI_ioctl_cec_state_t *)pUserParams)->cec_status;
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_CEC_GET_ONOFF:
			{
				//HDMI_ioctl_cec_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_cec_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_cec_state_t *)arg, sizeof(HDMI_ioctl_cec_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_cec_state_t *)pUserParams)->cec_status = ali_hdmi_get_cec_onoff();

				if ((ret = copy_to_user((HDMI_ioctl_cec_state_t *)arg, pUserParams, sizeof(HDMI_ioctl_cec_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_CEC_GET_PA:
			{
				//HDMI_ioctl_cec_addr_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_cec_addr_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_cec_addr_t *)arg, sizeof(HDMI_ioctl_cec_addr_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}

				if(hdmi_drv->control.edid_ready == true)
				{
					((HDMI_ioctl_cec_addr_t *)pUserParams)->cec_addr = hdmi_drv->edid.physical_address;
					((HDMI_ioctl_cec_addr_t *)pUserParams)->ret = true;
				}
				else
				{
					((HDMI_ioctl_cec_addr_t *)pUserParams)->cec_addr = 0;
					((HDMI_ioctl_cec_addr_t *)pUserParams)->ret = false;
				}

				if ((ret = copy_to_user((HDMI_ioctl_cec_addr_t *)arg, pUserParams, sizeof(HDMI_ioctl_cec_addr_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
			
		case HDMI_IOCT_CEC_SET_LA:
			{
				//HDMI_ioctl_cec_addr_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_cec_addr_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_cec_addr_t *)arg, sizeof(HDMI_ioctl_cec_addr_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_cec_addr_t *)pUserParams)->ret = ali_hdmi_set_logical_address(((HDMI_ioctl_cec_addr_t *)pUserParams)->cec_addr);

				if ((ret = copy_to_user((HDMI_ioctl_cec_addr_t *)arg, pUserParams, sizeof(HDMI_ioctl_cec_addr_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;


		case HDMI_IOCT_CEC_GET_LA:
			{
				//HDMI_ioctl_cec_addr_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_cec_addr_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_cec_addr_t *)arg, sizeof(HDMI_ioctl_cec_addr_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_cec_addr_t *)pUserParams)->cec_addr = ali_hdmi_get_logical_address();
				((HDMI_ioctl_cec_addr_t *)pUserParams)->ret = true;

				if ((ret = copy_to_user((HDMI_ioctl_cec_addr_t *)arg, pUserParams, sizeof(HDMI_ioctl_cec_addr_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_CEC_TRANSMIT:
			{
				//HDMI_ioctl_cec_msg_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_cec_msg_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_cec_msg_t *)arg, sizeof(HDMI_ioctl_cec_msg_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_cec_msg_t *)pUserParams)->ret = ali_hdmi_cec_transmit(((HDMI_ioctl_cec_msg_t *)pUserParams)->message, ((HDMI_ioctl_cec_msg_t *)pUserParams)->message_length);

				if ((ret = copy_to_user((HDMI_ioctl_cec_msg_t *)arg, pUserParams, sizeof(HDMI_ioctl_cec_msg_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

#endif  //CONFIG_CEC_ENABLE_ALI

		case HDMI_IOCT_GET_VIDEO_FORMAT:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_video_format_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_video_format_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_video_format_t *)arg, sizeof(HDMI_ioctl_video_format_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				edid_support_video_format(&(((HDMI_ioctl_video_format_t *)pUserParams)->format));

				if ((ret = copy_to_user((HDMI_ioctl_video_format_t *)arg, pUserParams, sizeof(HDMI_ioctl_video_format_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;


		case HDMI_IOCT_GET_EDID_AUD_OUT:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_audio_out_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_audio_out_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_audio_out_t *)arg, sizeof(HDMI_ioctl_audio_out_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_audio_out_t *)pUserParams)->ret = 0;//ali_hdmi_cec_transmit(((HDMI_ioctl_sw_onoff_state_t *)pUserParams)->message, ((HDMI_ioctl_sw_onoff_state_t *)pUserParams)->message_length);

				if ((ret = copy_to_user((HDMI_ioctl_audio_out_t *)arg, pUserParams, sizeof(HDMI_ioctl_audio_out_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;

		case HDMI_IOCT_SET_VID_RES:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_video_format_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_video_format_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_video_format_t *)arg, sizeof(HDMI_ioctl_video_format_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				api_hdmi_info_video_res = ((HDMI_ioctl_video_format_t *)pUserParams)->format;
				((HDMI_ioctl_video_format_t *)pUserParams)->ret = true;
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_GET_VID_RES:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_video_format_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_video_format_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_video_format_t *)arg, sizeof(HDMI_ioctl_video_format_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_video_format_t *)pUserParams)->format = api_hdmi_info_video_res;

				if ((ret = copy_to_user((HDMI_ioctl_video_format_t *)arg, pUserParams, sizeof(HDMI_ioctl_video_format_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
			}
			break;

		case HDMI_IOCG_GET_ALL_VID_RES:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_res_list_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_res_list_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_res_list_t *)arg, sizeof(HDMI_ioctl_edid_res_list_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}

				if(hdmi_drv->control.edid_ready == true)
				{
					edid_get_native_resolution(&(((HDMI_ioctl_edid_res_list_t *)pUserParams)->native_res_index));
					memcpy(((HDMI_ioctl_edid_res_list_t *)pUserParams)->video_res_list, hdmi_drv->edid.resolution_support, sizeof(enum HDMI_API_RES)* HDMI_API_RES_SUPPORT_NUM );
					((HDMI_ioctl_edid_res_list_t *)pUserParams)->ret = true;
				}
				else
					((HDMI_ioctl_edid_res_list_t *)pUserParams)->ret = false;

				if ((ret = copy_to_user((HDMI_ioctl_edid_res_list_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_res_list_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_SET_VENDOR_NAME:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_vendor_name_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_vendor_name_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_vendor_name_t *)arg, sizeof(HDMI_ioctl_vendor_name_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				hdmi_drv->spd.vendor_name_len = ((HDMI_ioctl_vendor_name_t *)pUserParams)->length;
				memset(hdmi_drv->spd.vendor_name, 0, 8);
				memcpy(hdmi_drv->spd.vendor_name, ((HDMI_ioctl_vendor_name_t *)pUserParams)->vendor_name, ((HDMI_ioctl_vendor_name_t *)pUserParams)->length);
				kfree(pUserParams);
			}
			break;
			
		case HDMI_IOCT_SET_PRODUCT_DESC:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_product_desc_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_product_desc_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_product_desc_t *)arg, sizeof(HDMI_ioctl_product_desc_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				hdmi_drv->spd.product_desc_len = ((HDMI_ioctl_product_desc_t *)pUserParams)->length;
				memset(hdmi_drv->spd.product_desc, 0, 16);
				memcpy(hdmi_drv->spd.product_desc, ((HDMI_ioctl_product_desc_t *)pUserParams)->product_desc, ((HDMI_ioctl_product_desc_t *)pUserParams)->length);
				kfree(pUserParams);
			}
			break;
			
		case HDMI_IOCT_GET_VENDOR_NAME:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_vendor_name_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_vendor_name_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_vendor_name_t *)arg, sizeof(HDMI_ioctl_vendor_name_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				((HDMI_ioctl_vendor_name_t *)pUserParams)->length = hdmi_drv->spd.vendor_name_len;
				memset(((HDMI_ioctl_vendor_name_t *)pUserParams)->vendor_name, 0, 8);
				memcpy(((HDMI_ioctl_vendor_name_t *)pUserParams)->vendor_name, hdmi_drv->spd.vendor_name, hdmi_drv->spd.vendor_name_len);

				if ((ret = copy_to_user((HDMI_ioctl_vendor_name_t *)arg, pUserParams, sizeof(HDMI_ioctl_vendor_name_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
			
		case HDMI_IOCT_GET_PRODUCT_DESC:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_product_desc_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_product_desc_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_product_desc_t *)arg, sizeof(HDMI_ioctl_product_desc_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				((HDMI_ioctl_product_desc_t *)pUserParams)->length = hdmi_drv->spd.product_desc_len;
				memset(((HDMI_ioctl_product_desc_t *)pUserParams)->product_desc, 0, 16);
				memcpy(((HDMI_ioctl_product_desc_t *)pUserParams)->product_desc, hdmi_drv->spd.product_desc, hdmi_drv->spd.product_desc_len);

				if ((ret = copy_to_user((HDMI_ioctl_product_desc_t *)arg, pUserParams, sizeof(HDMI_ioctl_product_desc_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_GET_LINK_ST:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_link_status_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_link_status_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_link_status_t *)arg, sizeof(HDMI_ioctl_link_status_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}

				((HDMI_ioctl_link_status_t *)pUserParams)->link_status = hdmi_drv->link_status;

				if ((ret = copy_to_user((HDMI_ioctl_link_status_t *)arg, pUserParams, sizeof(HDMI_ioctl_link_status_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;

            	case HDMI_IOCT_SET_HDMI_AUDIO_ONOFF:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_hdmi_audio_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_hdmi_audio_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_hdmi_audio_state_t *)arg, sizeof(HDMI_ioctl_hdmi_audio_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				ali_hdmi_audio_switch_onoff((bool) ((HDMI_ioctl_hdmi_audio_state_t *)pUserParams)->hdmi_audio_status);
				kfree(pUserParams);
			}
			break;

		case HDMI_IOCT_GET_HDMI_AUDIO_ONOFF:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_hdmi_audio_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_hdmi_audio_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_hdmi_audio_state_t *)arg, sizeof(HDMI_ioctl_hdmi_audio_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_hdmi_audio_state_t *)pUserParams)->hdmi_audio_status = hdmi_drv->control.hdmi_audio_enable;
				if ((ret = copy_to_user((HDMI_ioctl_hdmi_audio_state_t *)arg, pUserParams, sizeof(HDMI_ioctl_hdmi_audio_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_3D_PRESENT:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_3d_status_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_3d_status_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_3d_status_t *)arg, sizeof(HDMI_ioctl_3d_status_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				((HDMI_ioctl_3d_status_t *)pUserParams)->present = hdmi_drv->edid.support_3d;
				if ((ret = copy_to_user((HDMI_ioctl_3d_status_t *)arg, pUserParams, sizeof(HDMI_ioctl_3d_status_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
            
		case HDMI_IOCT_GET_EDID_MANUFACTURER:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_vendor_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_vendor_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_vendor_t *)arg, sizeof(HDMI_ioctl_edid_vendor_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				memset(((HDMI_ioctl_edid_vendor_t *)pUserParams)->manufacturer, 0, 4);
				memcpy(((HDMI_ioctl_edid_vendor_t *)pUserParams)->manufacturer, vendor_product_block->manufacturer_name, sizeof(vendor_product_block->manufacturer_name));
				if ((ret = copy_to_user((HDMI_ioctl_edid_vendor_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_vendor_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;

		case HDMI_IOCT_GET_EDID_MONITOR:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_vendor_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_vendor_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_vendor_t *)arg, sizeof(HDMI_ioctl_edid_vendor_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				memset(((HDMI_ioctl_edid_vendor_t *)pUserParams)->monitor, 0, 14);
				memcpy(((HDMI_ioctl_edid_vendor_t *)pUserParams)->monitor, monitor_name, sizeof(monitor_name));
				if ((ret = copy_to_user((HDMI_ioctl_edid_vendor_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_vendor_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;

		case HDMI_IOCT_SET_DEEP_COLOR:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_deep_color_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_deep_color_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_deep_color_t *)arg, sizeof(HDMI_ioctl_deep_color_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				api_set_hdmi_deep_color(((HDMI_ioctl_deep_color_t *)pUserParams)->dp_mode);
				kfree(pUserParams);
			}	
			break;
		case HDMI_IOCT_GET_DEEP_COLOR:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_deep_color_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_deep_color_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_deep_color_t *)arg, sizeof(HDMI_ioctl_deep_color_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_deep_color_t *)pUserParams)->dp_mode = api_get_hdmi_deep_color();

				if ((ret = copy_to_user((HDMI_ioctl_deep_color_t *)arg, pUserParams, sizeof(HDMI_ioctl_deep_color_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_EDID_BLOCK:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_block_data_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_block_data_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_block_data_t *)arg, sizeof(HDMI_ioctl_edid_block_data_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				memset(((HDMI_ioctl_edid_block_data_t *)pUserParams)->block, 0, 128);
				memcpy(((HDMI_ioctl_edid_block_data_t *)pUserParams)->block, hdmi_drv->edid.block[((HDMI_ioctl_edid_block_data_t *)pUserParams)->block_num], 128);
				if ((ret = copy_to_user((HDMI_ioctl_edid_block_data_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_block_data_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;
		case HDMI_IOCT_SET_PHY_CLOCK:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_phy_clock_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_phy_clock_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_phy_clock_t *)arg, sizeof(HDMI_ioctl_phy_clock_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				api_set_hdmi_phy_clk_onoff(((HDMI_ioctl_phy_clock_t *)pUserParams)->onoff);
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_SET_COLOR_SPACE:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_color_space_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_color_space_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_color_space_t *)arg, sizeof(HDMI_ioctl_color_space_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				api_set_hdmi_color_space(((HDMI_ioctl_color_space_t *)pUserParams)->color_space);
				kfree(pUserParams);
			}	
			break;
		case HDMI_IOCT_GET_COLOR_SPACE:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_color_space_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_color_space_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_color_space_t *)arg, sizeof(HDMI_ioctl_color_space_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_color_space_t *)pUserParams)->color_space= api_get_hdmi_color_space();

				if ((ret = copy_to_user((HDMI_ioctl_color_space_t *)arg, pUserParams, sizeof(HDMI_ioctl_color_space_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_EDID_DEEP_COLOR:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_deep_color_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_deep_color_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_deep_color_t *)arg, sizeof(HDMI_ioctl_edid_deep_color_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_edid_deep_color_t *)pUserParams)->dp_mode = api_get_edid_deep_color();

				if ((ret = copy_to_user((HDMI_ioctl_edid_deep_color_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_deep_color_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;
		case HDMI_IOCT_SET_AV_BLANK:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_av_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_av_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_av_state_t *)arg, sizeof(HDMI_ioctl_av_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				api_set_hdmi_mono_blank((bool)((HDMI_ioctl_av_state_t *)pUserParams)->av_blank_status);
				ali_hdmi_audio_switch_onoff((bool) !(((HDMI_ioctl_av_state_t *)pUserParams)->av_blank_status));
				hdmi_drv->control.hdmi_av_enable = (bool) ((HDMI_ioctl_av_state_t *)pUserParams)->av_blank_status;
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_AV_BLANK:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_av_state_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_av_state_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_av_state_t *)arg, sizeof(HDMI_ioctl_av_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
			
				((HDMI_ioctl_av_state_t *)pUserParams)->av_blank_status = hdmi_drv->control.hdmi_av_enable;

				if ((ret = copy_to_user((HDMI_ioctl_av_state_t *)arg, pUserParams, sizeof(HDMI_ioctl_av_state_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_EDID_HDMI_MODE:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_hdmi_mode_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_hdmi_mode_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_hdmi_mode_t *)arg, sizeof(HDMI_ioctl_edid_hdmi_mode_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
			
				((HDMI_ioctl_edid_hdmi_mode_t *)pUserParams)->hdmi_mode = hdmi_edid_chk_intface();

				if ((ret = copy_to_user((HDMI_ioctl_edid_hdmi_mode_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_hdmi_mode_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;
		case HDMI_IOCT_SET_AV_MUTE:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_av_mute_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_av_mute_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_av_mute_t *)arg, sizeof(HDMI_ioctl_av_mute_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				hdmi_proc_set_avmute((bool)((HDMI_ioctl_av_mute_t *)pUserParams)->av_mute, 2);
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_SET_TRANSCODING_MANUAL:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_transcoding_manual_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_transcoding_manual_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_transcoding_manual_t *)arg, sizeof(HDMI_ioctl_transcoding_manual_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				hdmi_drv->control.audio_transcoding_manual = (bool)((HDMI_ioctl_transcoding_manual_t *)pUserParams)->transcoding_manual;
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_TRANSCODING_MANUAL:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_transcoding_manual_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_transcoding_manual_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_transcoding_manual_t *)arg, sizeof(HDMI_ioctl_transcoding_manual_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
			
				((HDMI_ioctl_transcoding_manual_t *)pUserParams)->transcoding_manual = hdmi_drv->control.audio_transcoding_manual;

				if ((ret = copy_to_user((HDMI_ioctl_transcoding_manual_t *)arg, pUserParams, sizeof(HDMI_ioctl_transcoding_manual_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}
			break;
		case HDMI_IOCT_GET_EDID_LENGTH:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_block_data_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_block_data_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_block_data_t *)arg, sizeof(HDMI_ioctl_edid_block_data_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				((HDMI_ioctl_edid_block_data_t *)pUserParams)->length = 128 * (hdmi_drv->edid.number_of_extension_block + 1);
				if ((ret = copy_to_user((HDMI_ioctl_edid_block_data_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_block_data_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;
		case HDMI_IOCT_GET_EDID:
			if (1==hdmi_drv->edid.block[0][0x12]) {
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_edid_block_data_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_edid_block_data_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_edid_block_data_t *)arg, sizeof(HDMI_ioctl_edid_block_data_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				memcpy(((HDMI_ioctl_edid_block_data_t *)pUserParams)->edid_buf, hdmi_drv->edid.block, sizeof(hdmi_drv->edid.block));
				if ((ret = copy_to_user((HDMI_ioctl_edid_block_data_t *)arg, pUserParams, sizeof(HDMI_ioctl_edid_block_data_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			} else 
				ret = -EFAULT;
			break;
		case HDMI_IOCINI_HDMIHW:
			HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);	
                        if (hdmi_init_cnt != 0)
                            break;
			hdmi_init_cnt = 1;			
			HDMI_DEBUG("%s in:HDMI_IOCINI_HDMIHW\n", __FUNCTION__);
			//ali_hdmi_hardware_init();
			if(hdmi_sw_onoff_from_user)
			{
				ali_hdmi_switch_onoff(true);
			}
#if 0
			if ((ret = copy_from_user(hdmi_drv->hdcp.key, (char *)arg, 286)))
			{
				printk("%s error line%d\n", __FUNCTION__, __LINE__);
				// Invalid user space address
				return false;
			}
			hdmi_drv->hdcp.key[0] = 0;   
#endif

			break;

		case HDMI_IOCT_GET_KUMSGQ:
			{
				int flags = -1;
				if(copy_from_user(&flags, (int *)arg, sizeof(int)))
				{
					HDMI_DEBUG("Err: copy_from_user\n");
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				ret  = ali_kumsgq_newfd(hdmi_drv->hdmi_kumsgq, flags);
				if(ret> 0)
				{
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return ret;	
				}				
			}
			break;
		case HDMI_IOCT_HDCPONOFF:
			HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
			priv->control.hdcp_enable = (int)argu;
			break;
		case HDMI_IOCQ_MUTESTA:
			ret = priv->control.av_mute_state;
			break;
		case HDMI_IOCQ_HDCPSTAT:
			ret = priv->control.hdcp_enable;
			break;
		case HDMI_IOC_SET8MA:
			hdmi_set_txp_ctl0(0x02);hdmi_set_txn_ctl0(0x02);hdmi_set_txp_ctl1(0x02);
			hdmi_set_txn_ctl1(0x02);hdmi_set_txp_ctl2(0x02);hdmi_set_txn_ctl2(0x02);
			hdmi_set_txp_ctl3(0x02);hdmi_set_txn_ctl3(0x02);hdmi_set_emp_en(0x00);
			ret = 0;
			break;
		case HDMI_IOC_SET85MA:
			hdmi_set_txp_ctl0(0x02);hdmi_set_txn_ctl0(0x02);hdmi_set_txp_ctl1(0x02);
			hdmi_set_txn_ctl1(0x02);hdmi_set_txp_ctl2(0x02);hdmi_set_txn_ctl2(0x02);
			hdmi_set_txp_ctl3(0x02);hdmi_set_txn_ctl3(0x02);hdmi_set_emp_en(0x01);
			ret = 0;
			break;
		case HDMI_IOC_SET9MA:
			hdmi_set_txp_ctl0(0x02);hdmi_set_txn_ctl0(0x02);hdmi_set_txp_ctl1(0x02);
			hdmi_set_txn_ctl1(0x02);hdmi_set_txp_ctl2(0x02);hdmi_set_txn_ctl2(0x02);
			hdmi_set_txp_ctl3(0x02);hdmi_set_txn_ctl3(0x02);hdmi_set_emp_en(0x02);
			ret = 0;
			break;
		case HDMI_IOC_SET95MA:
			hdmi_set_txp_ctl0(0x02);hdmi_set_txn_ctl0(0x02);hdmi_set_txp_ctl1(0x02);
			hdmi_set_txn_ctl1(0x02);hdmi_set_txp_ctl2(0x02);hdmi_set_txn_ctl2(0x02);
			hdmi_set_txp_ctl3(0x02);hdmi_set_txn_ctl3(0x02);hdmi_set_emp_en(0x03);
			ret = 0;
			break;
		case HDMI_IOC_SET10MA:
			hdmi_set_txp_ctl0(0x03);hdmi_set_txn_ctl0(0x03);hdmi_set_txp_ctl1(0x03);
			hdmi_set_txn_ctl1(0x03);hdmi_set_txp_ctl2(0x03);hdmi_set_txn_ctl2(0x03);
			hdmi_set_txp_ctl3(0x03);hdmi_set_txn_ctl3(0x03);hdmi_set_emp_en(0x00);
			ret = 0;
			break;
		case HDMI_IOC_SET105MA:
			hdmi_set_txp_ctl0(0x03);hdmi_set_txn_ctl0(0x03);hdmi_set_txp_ctl1(0x03);
			hdmi_set_txn_ctl1(0x03);hdmi_set_txp_ctl2(0x03);hdmi_set_txn_ctl2(0x03);
			hdmi_set_txp_ctl3(0x03);hdmi_set_txn_ctl3(0x03);hdmi_set_emp_en(0x01);
			ret = 0;
			break;
		case HDMI_IOC_SET11MA:
			hdmi_set_txp_ctl0(0x03);hdmi_set_txn_ctl0(0x03);hdmi_set_txp_ctl1(0x03);
			hdmi_set_txn_ctl1(0x03);hdmi_set_txp_ctl2(0x03);hdmi_set_txn_ctl2(0x03);
			hdmi_set_txp_ctl3(0x03);hdmi_set_txn_ctl3(0x03);hdmi_set_emp_en(0x02);
			ret = 0;
			break;
		case HDMI_IOC_SET115MA:
			hdmi_set_txp_ctl0(0x03);hdmi_set_txn_ctl0(0x03);hdmi_set_txp_ctl1(0x03);
			hdmi_set_txn_ctl1(0x03);hdmi_set_txp_ctl2(0x03);hdmi_set_txn_ctl2(0x03);
			hdmi_set_txp_ctl3(0x03);hdmi_set_txn_ctl3(0x03);hdmi_set_emp_en(0x03);
			ret = 0;
			break;
	    case HDMI_IOC_SET12MA:
			hdmi_set_txp_ctl0(0x04);hdmi_set_txn_ctl0(0x04);hdmi_set_txp_ctl1(0x04);
			hdmi_set_txn_ctl1(0x04);hdmi_set_txp_ctl2(0x04);hdmi_set_txn_ctl2(0x04);
			hdmi_set_txp_ctl3(0x04);hdmi_set_txn_ctl3(0x04);hdmi_set_emp_en(0x00);
			ret = 0;
			break;
         case HDMI_IOC_SET125MA:
			hdmi_set_txp_ctl0(0x04);hdmi_set_txn_ctl0(0x04);hdmi_set_txp_ctl1(0x04);
			hdmi_set_txn_ctl1(0x04);hdmi_set_txp_ctl2(0x04);hdmi_set_txn_ctl2(0x04);
			hdmi_set_txp_ctl3(0x04);hdmi_set_txn_ctl3(0x04);hdmi_set_emp_en(0x01);
			ret = 0;
			break;
		case HDMI_IOC_SET13MA:
			hdmi_set_txp_ctl0(0x04);hdmi_set_txn_ctl0(0x04);hdmi_set_txp_ctl1(0x04);
			hdmi_set_txn_ctl1(0x04);hdmi_set_txp_ctl2(0x04);hdmi_set_txn_ctl2(0x04);
			hdmi_set_txp_ctl3(0x04);hdmi_set_txn_ctl3(0x04);hdmi_set_emp_en(0x02);
			break;
		case HDMI_IOC_SET135MA:
			hdmi_set_txp_ctl0(0x04);hdmi_set_txn_ctl0(0x04);hdmi_set_txp_ctl1(0x04);
			hdmi_set_txn_ctl1(0x04);hdmi_set_txp_ctl2(0x04);hdmi_set_txn_ctl2(0x04);
			hdmi_set_txp_ctl3(0x04);hdmi_set_txn_ctl3(0x04);hdmi_set_emp_en(0x03);
			ret = 0;
			break;
		case HDMI_IOC_SET14MA:
			hdmi_set_txp_ctl0(0x05);hdmi_set_txn_ctl0(0x05);hdmi_set_txp_ctl1(0x05);
			hdmi_set_txn_ctl1(0x05);hdmi_set_txp_ctl2(0x05);hdmi_set_txn_ctl2(0x05);
			hdmi_set_txp_ctl3(0x05);hdmi_set_txn_ctl3(0x05);hdmi_set_emp_en(0x00);
			ret = 0;
			break;
		case HDMI_IOC_SET145MA:
			hdmi_set_txp_ctl0(0x05);hdmi_set_txn_ctl0(0x05);hdmi_set_txp_ctl1(0x05);
			hdmi_set_txn_ctl1(0x05);hdmi_set_txp_ctl2(0x05);hdmi_set_txn_ctl2(0x05);
			hdmi_set_txp_ctl3(0x05);hdmi_set_txn_ctl3(0x05);hdmi_set_emp_en(0x01);
			ret = 0;
			break;
		case HDMI_IOC_SET15MA:
			hdmi_set_txp_ctl0(0x05);hdmi_set_txn_ctl0(0x05);hdmi_set_txp_ctl1(0x05);
			hdmi_set_txn_ctl1(0x05);hdmi_set_txp_ctl2(0x05);hdmi_set_txn_ctl2(0x05);
			hdmi_set_txp_ctl3(0x05);hdmi_set_txn_ctl3(0x05);hdmi_set_emp_en(0x02);	
			ret = 0;
			break;
		case HDMI_IOC_SET155MA:
			hdmi_set_txp_ctl0(0x05);hdmi_set_txn_ctl0(0x05);hdmi_set_txp_ctl1(0x05);
			hdmi_set_txn_ctl1(0x05);hdmi_set_txp_ctl2(0x05);hdmi_set_txn_ctl2(0x05);
			hdmi_set_txp_ctl3(0x05);hdmi_set_txn_ctl3(0x05);hdmi_set_emp_en(0x03);
			ret = 0;
			break;
		case HDMI_IOC_SET16MA:
			hdmi_set_txp_ctl0(0x06);hdmi_set_txn_ctl0(0x06);hdmi_set_txp_ctl1(0x06);
			hdmi_set_txn_ctl1(0x06);hdmi_set_txp_ctl2(0x06);hdmi_set_txn_ctl2(0x06);
			hdmi_set_txp_ctl3(0x06);hdmi_set_txn_ctl3(0x06);hdmi_set_emp_en(0x00);
			ret = 0;
			break;
		case HDMI_IOCG_HDMIMODE:
			ret = copy_to_user((unsigned int *)arg, (unsigned int*)&(priv->control.hdmi_dvi_mode),sizeof(unsigned int));
			break;
		case HDMI_IOCG_HDMIAUDIO:
			{
				unsigned short audio_fmt;
				edid_get_prefer_audio_out(&audio_fmt);
				ret = copy_to_user((unsigned short *)arg, &audio_fmt, sizeof(unsigned short));
				ret = 0;
			}
			break;
		case HDMI_IOCG_GET_AUDIO_CEA:
			{
				unsigned short idx = 0;
				SHORT_AUDIO_DESCRIPTOR_t *cea_item = NULL;
				cea_item = kmalloc(sizeof(SHORT_AUDIO_DESCRIPTOR_t), GFP_KERNEL);
				if(NULL == cea_item)
				{
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				if ((ret = copy_from_user(cea_item, (SHORT_AUDIO_DESCRIPTOR_t *)arg, sizeof(SHORT_AUDIO_DESCRIPTOR_t)) ))
				{
					kfree(cea_item);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				idx = cea_item->audio_format_code;
				if(false == edid_get_audio_cea_item(idx, cea_item))
				{
				
					kfree(cea_item);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				ret = copy_to_user((SHORT_AUDIO_DESCRIPTOR_t *)arg, cea_item, sizeof(SHORT_AUDIO_DESCRIPTOR_t));
				ret = 0;
				kfree(cea_item);
			}
			break;
		case HDMI_IOCG_GET_VIDEO_CEA:
			{
				unsigned short idx = 0;
				SHORT_VIDEO_DESCRIPTOR_t *cea_item = NULL;
				cea_item = kmalloc(sizeof(SHORT_VIDEO_DESCRIPTOR_t), GFP_KERNEL);
				if(NULL == cea_item)
				{
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				if ((ret = copy_from_user(cea_item, (SHORT_VIDEO_DESCRIPTOR_t *)arg, sizeof(SHORT_VIDEO_DESCRIPTOR_t)) ))
				{
					kfree(cea_item);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				idx = cea_item->native_indicator;
				if(false == edid_get_video_cea_item(idx, cea_item))
				{
				
					kfree(cea_item);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				ret = copy_to_user((SHORT_VIDEO_DESCRIPTOR_t *)arg, cea_item, sizeof(SHORT_VIDEO_DESCRIPTOR_t));
				ret = 0;
				kfree(cea_item);
			}
			break;
		case HDMI_IOCG_GET_AUDIO_CEA_NUM:
			{
				unsigned char type = AUDIO_TYPE;
				int num = 0;
				edid_get_audio_or_video_cea_item_num(type, &num);
				ret = copy_to_user((int *)arg, &num, sizeof(int));
				ret = 0;
			}
			break;
		case HDMI_IOCG_GET_VIDEO_CEA_NUM:
			{
				unsigned char type = VIDEO_TYPE;
				int num = 0;
				edid_get_audio_or_video_cea_item_num(type, &num);
				ret = copy_to_user((int *)arg, &num, sizeof(int));
				ret = 0;
			}
			break;
		case HDMI_IOCT_GET_HDCP_CAP:
			{
				HDMI_DEBUG("%s in %d\n", __FUNCTION__, __LINE__);		
				//HDMI_ioctl_color_space_t pUserParams;
				pUserParams = kmalloc(sizeof(HDMI_ioctl_hdcp_cap_t), GFP_KERNEL);
				if ((ret = copy_from_user(pUserParams, (HDMI_ioctl_hdcp_cap_t *)arg, sizeof(HDMI_ioctl_hdcp_cap_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				
				((HDMI_ioctl_hdcp_cap_t *)pUserParams)->hdcp_cap = api_get_hdmi_sink_hdcp_cap();

				if ((ret = copy_to_user((HDMI_ioctl_hdcp_cap_t *)arg, pUserParams, sizeof(HDMI_ioctl_hdcp_cap_t)) ))
				{
					printk("%s error line%d\n", __FUNCTION__, __LINE__);
					// Invalid user space address
					kfree(pUserParams);
					mutex_unlock(&hdmi_drv->hdmi_mutex);
					return -EFAULT;
				}
				kfree(pUserParams);
			}	
			break;
		case HDMI_IOCG_EDIDRDY:
			{
				int edid_stat;
				if(hdmi_drv->control.edid_ready == true)
					edid_stat = 1;
				else
					edid_stat = 0;
				ret = copy_to_user((int *)arg, (int *)&edid_stat, sizeof(int));
				ret = 0;
			}
			break;
		case HDMI_IOCG_NATIVERES:
			{
				enum HDMI_API_RES native;
				edid_get_native_resolution(&native);
				HDMI_DEBUG("%s in:init hw.HDMI_IOCG_NATIVERES = 0x%08x\n",__FUNCTION__, native);
				ret = copy_to_user((enum HDMI_API_RES*)arg, (enum HDMI_API_RES*)&native, sizeof(unsigned int));
				ret = 0;
			}
			break;
		case HDMI_IOC_DEVCLOSE:
			{
				HDMI_DEBUG("%s:HDMI_IOC_DEVCLOSE\n", __FUNCTION__);
				hdmi_proc_set_avmute(true, 1);
				hdmi_stop_transmit();
#ifdef CONFIG_HDCP_ENABLE_ALI
// SHA undo
//				if(hdmi_drv->hdmi_thread != NULL)
//				{
//					kthread_stop(hdmi_drv->hdmi_thread);
//					hdmi_drv->hdmi_thread = NULL;
//				}
#endif
			}
			break;
	}
	mutex_unlock(&hdmi_drv->hdmi_mutex);
	
	return ret;
}

int ali_hdmi_open(struct inode *inode, struct file *file)
{
	HDMI_PRIVATE_DATA *priv = hdmi_drv;	
	struct ali_hdmi_device_data *dev = container_of(inode->i_cdev, struct ali_hdmi_device_data, cdev);

	if(priv == NULL)
	{
		printk("%s error line%d\n", __FUNCTION__, __LINE__);
		return -1; // fail
	}
	
	dev->priv = (void*)priv;
	file->private_data = dev;
	
	hdmi_drv->hdmi_kumsgq = NULL;
	hdmi_drv->hdmi_kumsgq = ali_new_kumsgq();
	
	if (!hdmi_drv->hdmi_kumsgq)
	{
		goto out0;
    }
//	hdmi_proc_driver_init();
//	if(request_irq(IRQ_HDMI, (irq_handler_t)hdmi_interrupt_handler, IRQF_DISABLED, "hdmi", NULL))
//	{
//		printk("%s: Could not allocate hdmi interrupt handler\n",__FUNCTION__);
//		asm("sdbbp");
//		return RET_FAILURE;
//	}
//	printk(" i_private=%p private_data=%p  priv=%p \n", inode->i_private, file->private_data, priv);
	return 0; // success
	
out0:
	WARN(1,"False to new hdmi kumsgq!!!!!!");
	return -EFAULT;	
}

int ali_hdmi_close(struct inode *inode, struct file *file)
{
//Ike, 20130422, APP kill process cause hdmi no output issue
	ali_destroy_kumsgq(hdmi_drv->hdmi_kumsgq);
	hdmi_drv->hdmi_kumsgq = NULL;
    return 0;
    
	HDMI_DEBUG("%s: in....\n", __FUNCTION__);
	hdmi_proc_set_avmute(true, 1);
	hdmi_stop_transmit();

#ifdef CONFIG_HDCP_ENABLE_ALI
// SHA undo
//	if(hdmi_drv->hdmi_thread != NULL)
//	{
//		kthread_stop(hdmi_drv->hdmi_thread);
//		hdmi_drv->hdmi_thread = NULL;
//	}
#endif

	return 0; // success
}

static struct proc_dir_entry *hdmi_dir, *hdmi_file;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static const struct file_operations hdmi_proc_subdir_fops = {
//warning Need to set some I/O handlers here
};
#endif

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int hdmi_suspend(struct device *dev)
{
#ifdef CONFIG_HDCP_ENABLE_ALI
		if(hdmi_drv->hdmi_thread != NULL)
		{
			kthread_stop(hdmi_drv->hdmi_thread);
			hdmi_drv->hdmi_thread = NULL;
		}
#endif
	return 0;
}

static int hdmi_resume(struct device *dev)
{
	ali_hdmi_hardware_init();
	set_video_info_to_hdmi(0x00);
	return 0;
}
#endif

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static const struct dev_pm_ops hdmi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(hdmi_suspend, hdmi_resume)
};
#endif

static struct platform_driver hdmi_dev_driver = {
	.driver = {
	.name = "hdmi_device",
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	.pm = &hdmi_pm_ops,
#endif
	}
};

static int ali_hdmi_probe(struct platform_device * pdev)
{
	int ret = 0;
	dev_t dev = MKDEV(ali_hdmi_major, 0);
	int alloc_ret = 0;
	int	major;
	int cdev_err = 0;

	HDMI_DEBUG("ALi HDMI Driver version: %s\n", ali_hdmi_driver_version);
	HDMI_DEBUG("Base Address = 0x%.8x\n", HDMI_REG_BASEADDR);
	hdmi_open_cnt = 0;
	hdmi_init_cnt = 0;
	

	hdmi_drv = kmalloc(sizeof(HDMI_PRIVATE_DATA), GFP_KERNEL);
	if(hdmi_drv == NULL)
	{
		printk("%s error line%d\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	memset(hdmi_drv, 0, sizeof(HDMI_PRIVATE_DATA));
	init_waitqueue_head(&hdmi_drv->poll_plug_wq);


	alloc_ret = of_get_major_minor(pdev->dev.of_node,&dev, 
			0, 1, ALI_HDMI_DEVICE_NAME);
	if (alloc_ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto error0;
	}

	ali_hdmi_major = major = MAJOR(dev);
	cdev_init(&ali_hdmi_dev.cdev, &ali_hdmi_fops);
	ali_hdmi_dev.cdev.owner = THIS_MODULE;
	ali_hdmi_dev.cdev.ops   = &ali_hdmi_fops;
	cdev_err = cdev_add(&ali_hdmi_dev.cdev, MKDEV(ali_hdmi_major, 0), 1);
	if(cdev_err)
	{
		printk("Alloc HDMI device failed, err: %d.\n", cdev_err);
		goto error1;
	}

	ali_hdmi_class = class_create(THIS_MODULE, "ali_m36_hdmi_class");
	if(ali_hdmi_class == NULL)
	{
		printk("ali hdmi create class fail\n");
		goto error2;
	}
	ali_hdmi_node  = device_create(ali_hdmi_class, NULL, dev, &ali_hdmi_dev, ALI_HDMI_DEVICE_NAME);
	if(ali_hdmi_node == NULL)
	{
		printk("ali hdmi create device fail\n");
		goto error3;
	}

	platform_driver_register(&hdmi_dev_driver);

#if defined(CONFIG_ARM) 
	hdmi_drv->chip_info.chip_id        = (unsigned int) ((( __REG32ALI(M36_SYS_REG_BASEADDR)) & 0xFFFF0000) >> 16);
	hdmi_drv->chip_info.chip_version   = (unsigned char) (( __REG32ALI(M36_SYS_REG_BASEADDR)) & 0x000000FF);
	hdmi_drv->chip_info.bonding_option = (unsigned char)((( __REG32ALI(M36_SYS_REG_BASEADDR)) & 0x00000300) >> 8);
	//hdmi_drv->chip_info.chip_version2  = (unsigned char)(( __REG32ALI(M36_SYS_REG_FUNC_VER_ID) & 0x0000FF00) >> 8);
	//hdmi_drv->chip_info.hw_version	   = (unsigned char) ( __REG32ALI(M36_SYS_REG_FUNC_VER_ID) & 0x000000FF);
#else
	hdmi_drv->chip_info.chip_id        = (unsigned int) (((*(volatile unsigned int*)M36_SYS_REG_BASEADDR) & 0xFFFF0000) >> 16);
	hdmi_drv->chip_info.chip_version   = (unsigned char) ((*(volatile unsigned int*)M36_SYS_REG_BASEADDR) & 0x000000FF);
	hdmi_drv->chip_info.bonding_option = (unsigned char)(((*(volatile unsigned int*)M36_SYS_REG_BASEADDR) & 0x00000300) >> 8);
	hdmi_drv->chip_info.chip_version2  = (unsigned char)((M36_SYS_REG_FUNC_VER_ID & 0x0000FF00) >> 8);
	hdmi_drv->chip_info.hw_version	   = (unsigned char) (M36_SYS_REG_FUNC_VER_ID & 0x000000FF);
#endif
	hdmi_drv->control.interrupt_work_queue  = create_workqueue("hdmi_work_queue");
	hdmi_drv->control.hdcp_work_queue       = create_workqueue("hdcp_work_queue");

	HDMI_DEBUG("hdmi_chip_id_ver %x_%x_%x\n", hdmi_drv->chip_info.chip_id,  hdmi_drv->chip_info.chip_version, hdmi_drv->chip_info.chip_version2);
	HDMI_DEBUG("hdmi_chip_bonding %x\n", hdmi_drv->chip_info.bonding_option);

    // SHA do init
	//kinson Register Interrupt handler
	printk("request_irq:%d\n", IRQ_HDMI);
	
	init_waitqueue_head(&hdmi_drv->control.wait_queue);////kinson

#if 0	// Marlboro debug, move request irq later
    ret = request_irq(IRQ_HDMI, (irq_handler_t)hdmi_interrupt_handler, 0, "hdmi", NULL);       	
#endif
	mutex_init(&hdmi_drv->hdmi_mutex);
         	
	hdmi_proc_driver_init();
	
    if(fb2hdmi_config != NULL) // Frame buffer has been set hdmi before.
    {
		HDMI_DEBUG("Frame buffer has been set hdmi before.\n ");
        ali_hdmi_set_video_info(fb2hdmi_config);
        kfree(fb2hdmi_config);
        fb2hdmi_config = NULL;
    }
    else
    {
		HDMI_DEBUG("Frame buffer has not been set hdmi before.\n ");
	}
	
    if(alsa2hdmi_config != NULL) // Frame buffer has been set hdmi before.
    {
        ali_hdmi_set_audio_info(alsa2hdmi_config);
        kfree(alsa2hdmi_config);
        alsa2hdmi_config = NULL;
    }

     //irq here
#if 1	// Marlboro debug, move request irq here
    ret = request_irq(IRQ_HDMI, (irq_handler_t)hdmi_interrupt_handler, 0, "hdmi", NULL);
#endif
     
	/* create a directory */
	hdmi_dir = proc_mkdir("hdmi", NULL);

	if(hdmi_dir == NULL)
	{
		printk("Error create /proc/hdmi\n");
		goto end_dir;
	}
	
	/* create a file */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))	
	hdmi_file = proc_create("support", 0644, hdmi_dir, &hdmi_proc_subdir_fops);
#else
	hdmi_file = create_proc_entry("support", 0644, hdmi_dir);
#endif
	if(hdmi_file == NULL) {
		remove_proc_entry("hdmi", NULL);
		printk("Error create /proc/hdmi/support\n");
		goto end_dir;
	}

//	hdmi_file->read_proc = read_support;
	return 0; // success
end_dir:
	device_del(ali_hdmi_node);
error3:
	class_destroy(ali_hdmi_class);
error2:
	cdev_del(&ali_hdmi_dev.cdev);
error1:
	unregister_chrdev_region(dev, 1);
error0:
	kfree(hdmi_drv);
//error:
	return -1;
}

static int ali_hdmi_remove(struct platform_device * pdev)
{
	dev_t dev = MKDEV(ali_hdmi_major, 0);
	HDMI_DEBUG("%s: in....\n", __FUNCTION__);
#ifdef CONFIG_HDCP_ENABLE_ALI
	if(hdmi_drv->hdmi_thread != NULL)
	{
		kthread_stop(hdmi_drv->hdmi_thread);
		hdmi_drv->hdmi_thread = NULL;
	}
#endif // end of CONFIG_HDCP_ENABLE_ALI
	cdev_del(&ali_hdmi_dev.cdev);
	unregister_chrdev_region(dev, 1);
	platform_driver_unregister(&hdmi_dev_driver);
	free_irq(IRQ_HDMI, NULL);
	kfree(hdmi_drv);
	hdmi_drv = NULL;

	return 0;
}

static const struct of_device_id ali_hdmi_match[] = {
       { .compatible = "alitech, hdmi", },
       {},
};

MODULE_DEVICE_TABLE(of, ali_hdmi_match);

static struct platform_driver ali_hdmi_platform_driver = {
	.probe   = ali_hdmi_probe, 
	.remove   = ali_hdmi_remove,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = "ali_hdmi",
			.of_match_table = ali_hdmi_match,
	},
};

static int __init ali_hdmi_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&ali_hdmi_platform_driver);

	return ret;
}

static void __exit ali_hdmi_exit(void)
{
	platform_driver_unregister(&ali_hdmi_platform_driver);
}

//module_init(ali_hdmi_init);
fs_initcall_sync(ali_hdmi_init);
module_exit(ali_hdmi_exit);

EXPORT_SYMBOL(ali_hdmi_switch_onoff);
EXPORT_SYMBOL(ali_hdmi_audio_switch_onoff);
EXPORT_SYMBOL(ali_hdmi_audio_get_onoff);
EXPORT_SYMBOL(ali_hdmi_set_video_info);
EXPORT_SYMBOL(ali_hdmi_set_audio_info);
EXPORT_SYMBOL(api_get_hdmi_state);
#ifdef CONFIG_HDCP_ENABLE_ALI
EXPORT_SYMBOL(ali_hdmi_set_module_init_hdcp_onoff);
EXPORT_SYMBOL(ali_hdmi_get_hdcp_onoff);
#endif

#ifdef SHA_SDK
EXPORT_SYMBOL(set_video_info_to_hdmi);
EXPORT_SYMBOL(set_audio_info_to_hdmi);
#endif

MODULE_VERSION(ALI_HDMI_DRIVER_VERSION);
MODULE_LICENSE("Proprietary");
//MODULE_LICENSE("ALi Tech. Corp. R4200 Taipei Driver Team");
MODULE_DESCRIPTION("ALi HDMI Driver");
