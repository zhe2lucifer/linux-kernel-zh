/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
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

#ifndef __PORTING_CXD2837_LINUX_H__
#define __PORTING_CXD2837_LINUX_H__

#include "../porting_linux_header.h"
#include "../basic_types.h"
#include "../tun_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define CXD2837_PRINTF   nim_print_x
#else
#define CXD2837_PRINTF(...)
#endif


#define SYS_FUNC_ON				0x00000001	/* Function on */
#define i2c_read				ali_i2c_read
#define i2c_write				ali_i2c_write
#define i2c_write_read			ali_i2c_write_read      

#define comm_sleep 				msleep

typedef void (*nim_cxd2837_string_out_fnp_t)(char *string);

typedef struct
{

    UINT8    fecrates;
    UINT8    modulation;
    UINT8    mode;            /* (ter) */
    UINT8    guard;           /* (ter) */
    UINT8    hierarchy;       /* (ter) */
    UINT8    spectrum;
    UINT8    channel_bw;       /* (ter) */
    UINT32   frequency;
    INT32    freq_offset;  /* (ter) */
} cxd2837_lock_info;

typedef INT32  (*INTERFACE_DEM_WRITE_READ_TUNER)(void * nim_dev_priv, UINT8 tuner_address, UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
typedef struct
{
    void * nim_dev_priv; //for support dual demodulator.   
    //The tuner can not be directly accessed through I2C,
    //tuner driver summit data to dem, dem driver will Write_Read tuner.
    //INT32  (*Dem_Write_Read_Tuner)(void * nim_dev_priv, UINT8 slv_addr, UINT8 *wdata, int wlen, UINT8* rdata, int rlen);
    INTERFACE_DEM_WRITE_READ_TUNER  Dem_Write_Read_Tuner;
} DEM_WRITE_READ_TUNER;  //Dem control tuner by through mode (it's not by-pass mode).

typedef struct NIM_CHANNEL_CHANGE    NIM_CHANNEL_CHANGE_T;

#ifdef __cplusplus
}
#endif

#endif

