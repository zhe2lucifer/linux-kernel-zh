/* 
* NXP TDA10025 silicon tuner driver 
* 
* Copyright (C) 2016 <V.VRIGNAUD, C.CAZETTES> 
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

#ifndef _TMBSLTDA10025_INSTANCE_H
#define _TMBSLTDA10025_INSTANCE_H

tmErrorCode_t iTDA10025_AllocInstance (tmUnitSelect_t tUnit, ppTDA10025Object_t ppDrvObject);
tmErrorCode_t iTDA10025_DeAllocInstance (tmUnitSelect_t tUnit);
tmErrorCode_t iTDA10025_GetInstance (tmUnitSelect_t tUnit, ppTDA10025Object_t ppDrvObject);
tmErrorCode_t iTDA10025_ResetInstance(pTDA10025Object_t pDrvObject);

extern TDA10025PllConfig_t ES1PllSettings[];
extern TDA10025PllConfig_t ES2PllSettings[];

#endif /* _TMBSLTDA10025_INSTANCE_H */

