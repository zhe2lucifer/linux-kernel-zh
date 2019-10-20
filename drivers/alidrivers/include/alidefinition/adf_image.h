#ifndef __ADF_IMAGE__
#define __ADF_IMAGE__

#ifdef __cplusplus
extern "C" {
#endif
#include "adf_basic.h"
#include "adf_media.h"
#include "adf_vpo.h"
typedef struct mp_img_frm_yc_s
{
    char*  image_y_addr;
    int    image_y_addr_len;
    char*  image_c_addr;
    int    image_c_addr_len;
} mp_img_frm_yc_t;

struct mp_imgdisp_info
{
    int width;
    int height;
    int stride;
    unsigned char sample_format;
    unsigned char disp_layer;
    unsigned char disp_buffer;
    struct rect src_rect;
    struct rect dst_rect;
    mp_img_frm_yc_t imgbuf;
};//bysteve

RET_CODE mp_image_display(struct vpo_device *dev, struct mp_imgdisp_info *img_info);

#ifdef __cplusplus
}
#endif

#endif

