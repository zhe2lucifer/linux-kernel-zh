/* 
* NXP TDA18250 silicon tuner driver 
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

#ifndef _TMBSL_LO_DETUNING_H
#define _TMBSL_LO_DETUNING_H

#ifdef __cplusplus
extern "C"
{
#endif

#define abs(a) ((a)>=0?(a):-(a))

extern tmErrorCode_t tmSystem_LO_Detuning
(
    tmUnitSelect_t          TunerUnit,      /* I: Tuner unit number                   */
    tmTDA18250ChannelType_t ChannelType,    /* I: Channel type                        */
    UInt32                  *uLO            /* I: local oscillator frequency in hertz */
);

#ifdef __cplusplus
}
#endif

#endif /* _TMBSL_LO_DETUNING_H */
