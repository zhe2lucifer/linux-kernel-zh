#ifndef _NIM_RT720_H
#define _NIM_RT720_H

#include "../basic_types.h"
#include "../porting_linux_header.h"
#include "RT710.h"

#ifdef __cplusplus
extern "C"
{
#endif

INT32 tun_rt7x0_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 tun_rt7x0_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
INT32 tun_rt7x0_status(UINT32 tuner_id, UINT8 *lock);
INT32 tun_rt7x0_command(UINT32 tuner_id, INT32 cmd, INT32 *param);
INT32 tun_rt7x0_Standby(UINT32 tuner_id, RT710_LoopThrough_Type RT710_LTSel, RT710_ClockOut_Type RT710_CLKSel);
INT32 tun_rt7x0_get_rf_rssi(UINT32 tuner_id, struct ali_nim_agc *tuner_agc);

RT710_Err_Type rt7x0_i2c_write(UINT32 tuner_id, I2C_TYPE *I2C_Info);
RT710_Err_Type rt7x0_i2c_write_len(UINT32 tuner_id, I2C_LEN_TYPE *I2C_Info);
RT710_Err_Type rt7x0_i2c_read_len(UINT32 tuner_id, I2C_LEN_TYPE *I2C_Info);

#ifdef __cplusplus
}
#endif

#endif  /* _NIM_RT720_H */

