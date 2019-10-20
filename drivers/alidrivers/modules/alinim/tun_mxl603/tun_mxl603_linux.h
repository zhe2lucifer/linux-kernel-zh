#ifndef __LLD_TUN_MXL603_LINUX_H__
#define __LLD_TUN_MXL603_LINUX_H__


#include "../porting_linux_header.h"
#include "../basic_types.h"
#include "tun_mxl603.h"


#include "MaxLinearDataTypes.h"
//#include "MxL603_TunerApi.h"
//#include "MxL603_TunerCfg.h"


#ifdef __cplusplus
extern "C"
{
#endif




#define SYS_TUN_MODULE             ANY_TUNER



typedef INT32  (*INTERFACE_DEM_WRITE_READ_TUNER)(void * nim_dev_priv, UINT8 tuner_address, \
                 UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
typedef struct
{
    void * nim_dev_priv; //for support dual demodulator.   
    INTERFACE_DEM_WRITE_READ_TUNER  dem_write_read_tuner;
} DEM_WRITE_READ_TUNER;  //Dem control tuner by through mode (it's not by-pass mode).





/******************************************************************************
    Global Variable Declarations
******************************************************************************/
extern void * MxL603_OEM_DataPtr[];




#ifdef __cplusplus
}
#endif

#endif  /* __LLD_TUN_TDA18250_H__ */


