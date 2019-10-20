#ifndef	__ALI_IMG_H_
#define	__ALI_IMG_H_

#include <linux/ioctl.h>

#define MP_IMG_DISP_IOC_MAGIC  'k'

#define MP_IMG_DISP_CONFIG       _IOW(MP_IMG_DISP_IOC_MAGIC, 0, int)
#define MP_IMG_DISP_FILL_Y_DATA  _IOW(MP_IMG_DISP_IOC_MAGIC, 1, int)
#define MP_IMG_DISP_FILL_C_DATA  _IOW(MP_IMG_DISP_IOC_MAGIC, 2, int)
#define MP_IMG_DISP_RUN          _IOW(MP_IMG_DISP_IOC_MAGIC, 3, int)

#define MP_IMG_DISP_IOC_MAXNR   3

#define MP_IMG_DISP_LAYER_DECV_FB        0
#define MP_IMG_DISP_LAYER_STILL_PIC_BUF  1

typedef struct mp_img_rect_s
{
	unsigned int u_start_x;
	unsigned int u_start_y;
	unsigned int u_width;
	unsigned int u_height;
}mp_img_rect_t;

struct mp_imginfo
{
	unsigned int width;
	unsigned int height;
    unsigned int stride;
	unsigned char sample_format;
    unsigned char disp_layer;
    unsigned char disp_buffer;
	mp_img_rect_t src_rect;
	mp_img_rect_t dst_rect;
    char *buffer_y_addr;
    char *buffer_c_addr;
	unsigned int buffer_y_len;
	unsigned int buffer_c_len;
};

#endif