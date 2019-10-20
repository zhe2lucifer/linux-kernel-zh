/*
 * Copyright 2012 Sony Corporation Inc. All Rights Reserved.
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

#include "sony_common.h"

/* Utility function */
#define MASKUPPER(n) (((n) == 0) ? 0 : (0xFFFFFFFFU << (32 - (n))))
#define MASKLOWER(n) (((n) == 0) ? 0 : (0xFFFFFFFFU >> (32 - (n))))
/* Convert N (<32) bit 2's complement value to 32 bit signed value */
int32_t cxd2837_Convert2SComplement(uint32_t value, uint32_t bitlen)
{
    if((bitlen == 0) || (bitlen >= 32)){
        return (int32_t)value;
    }

    if(value & (uint32_t)(1 << (bitlen - 1))){
        /* minus value */
        return (int32_t)(MASKUPPER(32 - bitlen) | value);
    }
    else{
        /* plus value */
        return (int32_t)(MASKLOWER(bitlen) & value);
    }
}
