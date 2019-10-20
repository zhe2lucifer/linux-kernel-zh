/*
 * Copyright 2012 Sony Corporation. All Rights Reserved.
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

/**
 @file    sony_dtv.h

          This file provides DTV system specific definitions.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_DTV_H
#define SONY_DTV_H

/*------------------------------------------------------------------------------
  Enumerations
------------------------------------------------------------------------------*/
/**
 @brief System (DVB-T/T2/C/C2/S/S2) 
*/
typedef enum {
    SONY_DTV_SYSTEM_UNKNOWN,        /**< Unknown. */
    SONY_DTV_SYSTEM_DVBT,           /**< DVB-T. */
    SONY_DTV_SYSTEM_DVBT2,          /**< DVB-T2. */
    SONY_DTV_SYSTEM_DVBC,           /**< DVB-C. */
    SONY_DTV_SYSTEM_DVBC2,          /**< DVB-C2. */
    SONY_DTV_SYSTEM_DVBS,           /**< DVB-S. */
    SONY_DTV_SYSTEM_DVBS2,          /**< DVB-S2. */
    SONY_DTV_SYSTEM_ANY             /**< Used for multiple system scanning / blind tuning */
} sony_dtv_system_t;

#endif /* SONY_DTV_H */
