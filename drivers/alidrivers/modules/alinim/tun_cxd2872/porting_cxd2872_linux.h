/*
 * Copyright 2014 Sony Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#ifndef PORTING_CXD2872_LINUX_H
#define PORTING_CXD2872_LINUX_H


/* Type definitions. */
/* <PORTING> Please comment out if conflicted */
#include "../porting_linux_header.h"
#include "../basic_types.h"


#define MAX_TUNER_SUPPORT_NUM         1

typedef INT32  (*INTERFACE_DEM_WRITE_READ_TUNER)(void * nim_dev_priv, UINT8 tuner_address, UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
typedef struct
{
    void * nim_dev_priv; //for support dual demodulator.   
    //The tuner can not be directly accessed through I2C,
    //tuner driver summit data to dem, dem driver will Write_Read tuner.
    //INT32  (*Dem_Write_Read_Tuner)(void * nim_dev_priv, UINT8 slv_addr, UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
    INTERFACE_DEM_WRITE_READ_TUNER  Dem_Write_Read_Tuner;
} DEM_WRITE_READ_TUNER;  //Dem control tuner by through mode (it's not by-pass mode).

#endif /* SONY_COMMON_H */
