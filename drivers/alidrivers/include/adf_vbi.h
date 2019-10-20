#ifndef	__ADF_VBI_H_
#define	__ADF_VBI_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "alidefinition/adf_basic.h"

typedef enum {

    TTX_VBI_TYPE=0,
    CC_VBI_TYPE=1,
    VPS_VBI_TYPE=2,
    WSS_VBI_TYPE=3,    
    
}VBI_SOURCE_TYPE;


struct vbi_data_array_t{
  
    UINT8 vbi_data[46];
};

void vbi_see_start(VBI_SOURCE_TYPE type);


void vbi_see_stop(void);

UINT8 write_ttx_packet(struct vbi_data_array_t *p_data, UINT32 size);


RET_CODE ttx_vbi_ioctl(UINT32 cmd, UINT32 param);


#ifdef __cplusplus

}

#endif

/*!

@}

*/



/*!

@}

*/





#endif



