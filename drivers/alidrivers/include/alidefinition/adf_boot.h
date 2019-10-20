
#ifndef __ADF_BOOT__
#define __ADF_BOOT__

/*! @addtogroup DeviceDriver
 *  @{
 */

/*! @addtogroup Boot Configuration
 *  @{
 */

#define MEDIA_BLOCK_MAXCOUNT 0x8
#define MEDIA_PLAYLIST_MAXCOUNT 64

#define TVE_SYSTEM_NUM              20
#define TVE_ADJ_FIELD_NUMBER    100
#define TVE_ADJ_REG_NUM             100
#define TVE_ADV_ADJ_REG_NUM        100
#define BOOT_MAGIC_NUM 0xAA5555AA

/* Define for VDAC configuration */
#define VDAC_NUM_MAX		6
#define VDAC_TYPE_NUM		6
//Type
#define VDAC_TYPE_CVBS		0
#define VDAC_TYPE_SVIDEO	1
#define VDAC_TYPE_YUV		2
#define VDAC_TYPE_RGB		3
#define VDAC_TYPE_SCVBS		4
#define VDAC_TYPE_SSV		5
#define VDAC_TYPE_MAX		6
//Detail
#define VDAC_CVBS			(VDAC_TYPE_CVBS<<2|0)
#define VDAC_CVBS_1			(VDAC_TYPE_CVBS<<2|1)
#define VDAC_CVBS_2			(VDAC_TYPE_CVBS<<2|2)
#define VDAC_CVBS_3			(VDAC_TYPE_CVBS<<2|3)

#define VDAC_SVIDEO_Y	(VDAC_TYPE_SVIDEO<<2|0)	// 0x4
#define VDAC_SVIDEO_C	(VDAC_TYPE_SVIDEO<<2|1)	// 0x5
#define VDAC_YUV_Y		(VDAC_TYPE_YUV<<2|0)	// 0x8
#define VDAC_YUV_U		(VDAC_TYPE_YUV<<2|1)	// 0x9
#define VDAC_YUV_V		(VDAC_TYPE_YUV<<2|2)	// 0xA
#define VDAC_RGB_R		(VDAC_TYPE_RGB<<2|0)	// 0xC
#define VDAC_RGB_G		(VDAC_TYPE_RGB<<2|1)	// 0xD
#define VDAC_RGB_B		(VDAC_TYPE_RGB<<2|2)	// 0xE
#define VDAC_SCVBS		(VDAC_TYPE_SCVBS<<2|0)	// 0x10
#define VDAC_SSV_Y		(VDAC_TYPE_SSV<<2|0)	// 0x14
#define VDAC_SSV_C		(VDAC_TYPE_SSV<<2|1)	// 0x15
#define VDAC_NULL		0xFF

/*! @struct ADF_BOOT_HDCP
@brief HDCP Key value information.
*/
typedef struct
{
	unsigned char hdcp[288];//!<HDCP key value.
}ADF_BOOT_HDCP;

/*! @struct ADF_BOOT_AVINFO
@brief Audio and video information.
*/
typedef struct
{
	unsigned char tvSystem;
	unsigned char progressive;
	unsigned char tv_ratio;
	unsigned char display_mode;

	unsigned char scart_out;
	unsigned char vdac_out[6];
	unsigned char video_format;

	unsigned char audio_output;
	unsigned char brightness;
	unsigned char contrast;
	unsigned char saturation;

	unsigned char sharpness;
	unsigned char hue;
	unsigned char snd_mute_gpio;
	unsigned char snd_mute_polar;
	unsigned char hdcp_disable;
	unsigned char resv[7];
}ADF_BOOT_AVINFO;

/*! @struct ADF_BOOT_MAC_PHYADDR
@brief Mac physical address information.
*/
typedef struct
{
	unsigned char phyaddr1[8];
	unsigned char phyaddr2[8];
	unsigned char phyaddr3[8];
	unsigned char phyaddr4[8];
}ADF_BOOT_MAC_PHYADDR;

/*! @struct ADF_BOOT_MEMMAPINFO
@brief Memory distribution information.
*/
typedef struct
{
	unsigned int kernel_start;
	unsigned int kernel_len;

	unsigned int boot_see_start;
	unsigned int boot_see_len;

	unsigned int see_start;
	unsigned int see_len;

	unsigned int ae_start;
	unsigned int ae_len;

	unsigned int mcapi_start;
	unsigned int mcapi_len;

	unsigned int fb0_start;
	unsigned int fb0_len;

	unsigned int fb2_start;
	unsigned int fb2_len;

	unsigned int vbv_start;
	unsigned int vbv_len;

	unsigned int decv_fb_start;//Video frame buffer.
	unsigned int decv_fb_len;

	unsigned int decv_hw_buff_start;
	unsigned int decv_hw_buff_len;

	unsigned int osd_fb_start;
	unsigned int osd_fb_len;

	unsigned int boot_de_param_buff_start;
	unsigned int boot_de_param_buff_len;

	unsigned int ramdisk_start;
	unsigned int ramdisk_len;

	unsigned int cmd_queue_buffer_start;
	unsigned int cmd_queue_buffer_len;

	unsigned int vcap_buffer_start;
	unsigned int vcap_buffer_len;

	unsigned int boot_media_mkv_buf_start; //!< buffer for decoding bootmedia mkv
	unsigned int boot_media_mkv_buf_len;

    unsigned int decv_pip_fb_start;
    unsigned int decv_pip_fb_len;

    unsigned int decv_pip_hw_start;
    unsigned int decv_pip_hw_len;

    unsigned int decv_pip_vbv_start;
    unsigned int decv_pip_vbv_len;

	unsigned int boot_media_buf_start; //!< use to store bootmedia source
	unsigned int boot_media_buf_len;

    unsigned int still_picture_buf_start;
    unsigned int still_picture_buf_len;
    unsigned int reserve[758];
	unsigned int dd_support_buff_start;
	unsigned int dd_support_buff_len;

    unsigned int pcm_ring_buff_start;
    unsigned int pcm_ring_buff_len;

    /* For i2so & spo & ddp_spo dma buffer */
    unsigned int snd_dma_buffer_start;
    unsigned int snd_dma_buffer_len;
}ADF_BOOT_MEMMAPINFO; // Maximun is 800X4 bytes.

#define MAX_REGISTER_NUM		64
#define REGISTER_VALID_MAGIC		0x78AC88EF

/*! @struct REGISTER_SETTING
@brief Boot configuration register value information.
*/
struct REGISTER_SETTING
{
	unsigned int magic;
	unsigned int addr;
	unsigned int bits_offset;
	unsigned int bits_size;
	unsigned int bits_value;
};

/*! @struct ADF_REGISTERINFO
@brief Boot configuration register setting information.
*/
typedef struct
{
	unsigned int valid_count;
	struct REGISTER_SETTING unit[MAX_REGISTER_NUM];
}ADF_REGISTERINFO;

struct tve_adjust_element
{
	unsigned int field_index;
	unsigned int field_value;
};

struct tve_adjust_table
{
	unsigned int index;
	struct tve_adjust_element field_info[TVE_ADJ_FIELD_NUMBER];
};

struct sd_tve_adjust_table
{
	unsigned char type;
	unsigned char sys;
	unsigned int value;
};

/*! @struct ADF_BOOT_TVEINFO
@brief TVE setting information.
*/
typedef struct
{
	struct tve_adjust_table tve_adjust_table_info[TVE_SYSTEM_NUM];       //804 * 20
	struct sd_tve_adjust_table sd_tve_adj_table_info[TVE_ADJ_REG_NUM];      //8 * 100
	struct sd_tve_adjust_table sd_tve_adv_adj_table_info[TVE_ADV_ADJ_REG_NUM];    // 8 * 100
	unsigned char reserve[8944];
}ADF_BOOT_TVEINFO; // maximun is 26k = 26 X 1024 =  26624 bytes

enum ADF_BOOT_MEDIA_CMD
{
	BOOT_MEDIA_IDLE = 1,
	BOOT_MEDIA_START,
	BOOT_MEDIA_PLAYING,
	BOOT_MEDIA_FINISHED,
};

enum ADF_BOOT_MEDIA_STATUS
{
	MEDIA_FILE_IDLE = 1,
	MEDIA_FILE_START,
	MEDIA_FILE_PLAYING,
	MEDIA_FILE_FINISHED,
};

/*! @struct ADF_BOOT_MEDIAINFO
@brief Boot media information.
*/
typedef struct
{
	unsigned int play_enable;
	enum ADF_BOOT_MEDIA_CMD start_cmd;
	enum ADF_BOOT_MEDIA_CMD finish_cmd;
	enum ADF_BOOT_MEDIA_STATUS jpeg_show_status;
	enum ADF_BOOT_MEDIA_STATUS mpeg2_show_status;
	enum ADF_BOOT_MEDIA_STATUS mkv_show_status;
	unsigned int smart_output_enable;
    unsigned int play_delay;
}ADF_BOOT_MEDIAINFO;


/*! @struct ADF_BOOT_GMA_INFO
@brief Boot GMA information.
*/
typedef struct
{
	int gma_enable;
	int gma_layer_id;

	int format;//See adf_gma.h enum GMA_FORMAT

	int x, y;
	int w, h;

	unsigned int gma_buffer;
	unsigned int gma_pitch;

	unsigned int pallett[256];
	int full_screen;
}ADF_BOOT_GMA_INFO;

/*! @struct ADF_SEE_HEART_BEAT
@brief SEE CPU heart beat information when booting.
*/
typedef struct
{
	int live_flag;
	int live_tick;
}ADF_SEE_HEART_BEAT;

/*! @struct ADF_BOOT_BOARD_INFO
@brief Boot configuration information.
*/
typedef struct
{
	ADF_BOOT_AVINFO avinfo;
	ADF_BOOT_MEMMAPINFO memmap_info;
	ADF_BOOT_TVEINFO tve_info;
	ADF_BOOT_MEDIAINFO	media_info;
	ADF_BOOT_GMA_INFO gma_info;
	ADF_SEE_HEART_BEAT heart_beat;
}ADF_BOOT_BOARD_INFO; // The maximun size is 128K

#ifdef CONFIG_SUPPORT_DEVICE_TREE
ADF_BOOT_BOARD_INFO *alisee_boot_info_p;

#ifdef _S3922_
#define DTB_BLOB_ADDR (*(volatile UINT32 *)(0xB8000004))  /* instore DTB addr into scratch register */
#else
#if (defined _C3922_ || defined _M3702_)
#define DTB_BLOB_ADDR (*(volatile UINT32 *)(0xB8087400))  /* instore DTB addr into scratch register */
#else
#define DTB_BLOB_ADDR (*(volatile UINT32 *)(0xB8000208))  /* instore DTB addr into mailbox register */
#endif
#endif

#else
#define PRE_DEFINED_ADF_BOOT_START                     (0x84000000 - 0x20000)
#endif


//#define PRE_DEFINED_ADF_BOOT_START			(0x84000000 - 0x20000)//!< The memory address of boot configuration.

/*! @enum ADF_BOOT_MEDIA_TYPE
@brief Type of boot media information.
*/
enum ADF_BOOT_MEDIA_TYPE
{
	BOOT_MEDIA_TYPE_MIN = 10,

	// logo
	BOOT_MEDIA_MPEG2_Logo,
	BOOT_MEDIA_JPEG_Logo,

	// video
	BOOT_MEDIA_MKV_Video = 110,

	BOOT_MEDIA_TYPE_MAX,
};

#define ADF_BOOT_MEDIA_MAGIC		"adfbootmedia"
#define ADF_BOOT_MEDIA_MAGIC_LEN 	16

/*! @struct ADF_BOOT_MEDIA_BLOCK_HEADER
@brief Boot media block header information.
*/
typedef struct
{
	unsigned int block_offset;//!< Media file offset.
	unsigned int block_len;//!< Media file length.
	enum ADF_BOOT_MEDIA_TYPE media_type;//!< Media information type.
	unsigned int play_time; //!< Play time.
}ADF_BOOT_MEDIA_BLOCK_HEADER;

/*! @struct ADF_BOOT_MEDIA_BLOCK_HEADER
@brief Boot media header information.
*/
typedef struct
{
	unsigned char magic[ADF_BOOT_MEDIA_MAGIC_LEN];//!<Magic number.
	unsigned short media_block_count;//!< Included media block count.
	unsigned short play_count;//!< Actully played media block count.
	unsigned char play_select_index[MEDIA_PLAYLIST_MAXCOUNT];//!< Select media No. to play.
	ADF_BOOT_MEDIA_BLOCK_HEADER block_hdr[MEDIA_BLOCK_MAXCOUNT];//!< Each media block information.
	unsigned char reserve[44];
}ADF_BOOT_MEDIA_HEADER;

/*!
 * @}
 */

/*!
 * @}
 */

#endif

