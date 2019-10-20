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
 @file    sony_dvbc.h

          This file provides DVB-C related type definitions.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_DVBC_H
#define SONY_DVBC_H

/*------------------------------------------------------------------------------
 Enumerations
------------------------------------------------------------------------------*/
/**
 @brief DVB-C constellation.
*/
typedef enum {
    SONY_DVBC_CONSTELLATION_16QAM,          /**< 16-QAM */
    SONY_DVBC_CONSTELLATION_32QAM,          /**< 32-QAM */
    SONY_DVBC_CONSTELLATION_64QAM,          /**< 64-QAM */
    SONY_DVBC_CONSTELLATION_128QAM,         /**< 128-QAM */
    SONY_DVBC_CONSTELLATION_256QAM          /**< 256-QAM */
} sony_dvbc_constellation_t;

#endif /* SONY_DVBC_H */
