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

#ifndef _SHARP_VZ7306_H_
#define _SHARP_VZ7306_H_



#include "../basic_types.h"
#include "../porting_linux_header.h"


#define REF_OSC_FREQ	4000 /* 4MHZ */
  
#ifdef __cplusplus
extern "C"
{
#endif



INT32 ali_nim_vz7306_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config);
INT32 ali_nim_vz7306_control(UINT32 tuner_id, UINT32 freq, UINT32 sym);
INT32 ali_nim_vz7306_status(UINT32 tuner_id, UINT8 *lock);
INT32 ali_nim_vz7306_close(void);



#ifdef __cplusplus
}
#endif

#endif  /* _SHARP_VZ7306_H_ */

