/* 
* NXP TDA10025 silicon tuner driver 
* 
* Copyright (C) 2016 <insert developer name and email> 
* 
*    This program is free software; you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation; either version 2 of the License, or 
*    (at your option) any later version. 
* 
*    This program is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
* 
*    You should have received a copy of the GNU General Public License along 
*    with this program; if not, write to the Free Software Foundation, Inc., 
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
*/ 

#pragma once
#ifndef _TMBSL_HCDP_INSTANCE_H
#define _TMBSL_HCDP_INSTANCE_H

#ifdef __cplusplus
extern "C"
{
#endif


tmErrorCode_t
iHCDP_AllocInstance(tmUnitSelect_t tUnit, ppHCDPObject_t ppDrvObject);

tmErrorCode_t
iHCDP_DeAllocInstance(pHCDPObject_t pDrvObject);

tmErrorCode_t
iHCDP_GetInstance(tmUnitSelect_t tUnit, ppHCDPObject_t ppDrvObject);

tmErrorCode_t
iHCDP_ResetInstance(pHCDPObject_t pDrvObject);


#ifdef __cplusplus
}
#endif

#endif /* _TMBSL_HCDP_INSTANCE_H */

