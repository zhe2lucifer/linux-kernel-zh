#ifndef	__ADF_SOC_H_
#define	__ADF_SOC_H_


/*! @addtogroup DeviceDriver
 *  @{
    */

/*! @addtogroup ALiSOC
 *  @{
     */



#ifdef __cplusplus
extern "C" {
#endif




/*! @struct soc_op_paras
@brief Register access operation parameters.
*/
struct soc_op_paras {
    unsigned char * to;
    unsigned char *from;
    int len;
};                    

struct reboot_timer{
    unsigned long *time_exp;
    unsigned long *time_cur;    
};

struct boot_timer{
    unsigned long time_exp;
    unsigned long time_cur;  
};

/*! @struct soc_usb_port
@brief USB port number information. 
*/
struct soc_usb_port {
    unsigned long usb_port;
};

/*! @struct soc_opt_paras8
@brief Register access operation parameters, with the unit of 8 bits. 
*/
struct soc_opt_paras8 {
    unsigned long addr;
    unsigned char data;    
};      

/*! @struct soc_opt_paras16
@brief Register access operation parameters, with the unit of 16 bits. 
*/
struct soc_opt_paras16 {
    unsigned long addr;
    unsigned short data;    
};      

/*! @struct soc_opt_paras32
@brief Register access operation parameters, with the unit of 32 bits. 
*/
struct soc_opt_paras32 {
    unsigned long addr;
    unsigned long data;    
};      

/*! @struct debug_level_paras
@brief Debugging level of SOC module. 
*/
struct debug_level_paras {
    unsigned long level;   
};      

/*! @struct soc_memory_map
@brief Memory mapping table of each module in the system. 
*/
struct soc_memory_map
{	
	unsigned long main_start;		
	unsigned long main_end;	
	unsigned long fb_start;
	unsigned long osd_bk;	
	unsigned long see_dmx_src_buf_start;
	unsigned long see_dmx_src_buf_end;
	unsigned long see_dmx_decrypto_buf_start;
	unsigned long see_dmx_decrypto_buf_end;
	unsigned long dmx_start;
	unsigned long dmx_top;
	unsigned long see_start;
	unsigned long see_top;
	unsigned long video_start;
	unsigned long video_top;
	unsigned long frame;
	unsigned long frame_size;
	unsigned long vcap_fb;
	unsigned long vcap_fb_size;
	unsigned long vdec_vbv_start;
	unsigned long vdec_vbv_len;
	unsigned long shared_start;
	unsigned long shared_top;	

	unsigned long reserved_mem_addr;
	unsigned long reserved_mem_size;

	unsigned long media_buf_addr;
	unsigned long media_buf_size;

	unsigned long mcapi_buf_addr;
	unsigned long mcapi_buf_size;
};

/*! @struct soc_opt_see_ver
@brief SEE CPU version information. 
*/
struct soc_opt_see_ver {
    unsigned char *buf;
};  

/*! @enum boot_type
@brief Boot type.
*/
enum boot_type {
    ALI_SOC_BOOT_TYPE_NOR,
    ALI_SOC_BOOT_TYPE_NAND,    
};
struct soc_read_write_reg{
    int operation;    //0: read; 1: write
    int mode; //0: 32bits; 1:16bits; 2:8bits
    unsigned int addr;
    unsigned int value;
};


#ifdef __cplusplus
}

/*!
 * @}
 */

/*!
 * @}
 */
 
 
#endif
#endif

