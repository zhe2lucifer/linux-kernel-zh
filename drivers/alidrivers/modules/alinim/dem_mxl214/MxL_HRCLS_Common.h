#ifndef __MXL_HRCLS_COMMON_H__
#define __MXL_HRCLS_COMMON_H__

#include "nim_mxl214c.h"




#include "MaxLinearDataTypes.h"
#include "MxL_HRCLS_Features.h"    // SKU-dependent feature configurations (depending on MXL_HRCLS_PROD_ID, which is defined in MxL_HRCLS_ProductId.h)
#include "MxL_HRCLS_OEM_Defines.h" // Platform-dependent configurations

#include "MaxLinearDebug.h"






MXL_STATUS_E MxLWare_HRCLS_OEM_Reset(UINT8 devId);
MXL_STATUS_E MxLWare_HRCLS_OEM_WriteRegister(UINT8 devId, UINT16 regAddr, UINT16 regData);
MXL_STATUS_E MxLWare_HRCLS_OEM_ReadRegister(UINT8 devId, UINT16 regAddr,/*@out@*/  UINT16 *dataPtr);
MXL_STATUS_E MxLWare_HRCLS_OEM_ReadBlock(UINT8 devId, UINT16 regAddr, UINT16 readSize, /*@out@*/ UINT8 *bufPtr);
MXL_STATUS_E MxLWare_HRCLS_OEM_ReadBlockExt(UINT8 devId, UINT16 cmdId, UINT16 offset, UINT16 readSize, /*@out@*/ UINT8* bufPtr);
MXL_STATUS_E MxLWare_HRCLS_OEM_WriteBlock(UINT8 devId, UINT16 regAddr, UINT16 bufSize, UINT8* bufPtr);
MXL_STATUS_E MxLWare_HRCLS_OEM_ReadBlockExt(UINT8 devId, UINT16 cmdId, UINT16 offset, UINT16 readSize, UINT8 *bufPtr);
MXL_STATUS_E MxLWare_HRCLS_OEM_LoadNVRAMFile(UINT8 devId, UINT8 *bufPtr, UINT32 bufLen);
MXL_STATUS_E MxLWare_HRCLS_OEM_SaveNVRAMFile(UINT8 devId, UINT8 *bufPtr, UINT32 bufLen);
void MxLWare_HRCLS_OEM_DelayUsec(UINT32 usec);
void MxLWare_HRCLS_OEM_GetCurrTimeInUsec(/*@out@*/ UINT64* usecPtr);





#endif

