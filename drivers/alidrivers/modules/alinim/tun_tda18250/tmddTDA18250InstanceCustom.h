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

#ifndef _TMDD_TDA18250_INSTANCE_CUSTOM_H
#define _TMDD_TDA18250_INSTANCE_CUSTOM_H


#ifdef __cplusplus
extern "C"
{
#endif


/*============================================================================*/
/* Custom Driver Instance Parameters: (Path 0)                                */
/*============================================================================*/

#define TMDD_TDA18250_INSTANCE_CUSTOM_0                                                                     \
    tmddTDA18250_PowerStandbyWithXtalOn,     /* curPowerState */                                            \
    {                                               /* Config */                                            \
        {                                           /* MAIN_PLL_Map */                                      \
            {33000000, 0x57, 0xF0},                                                                         \
            {36000000, 0x56, 0xE0},                                                                         \
            {38000000, 0x55, 0xD0},                                                                         \
            {42000000, 0x54, 0xC0},                                                                         \
            {45000000, 0x53, 0xB0},                                                                         \
            {50000000, 0x52, 0xA0},                                                                         \
            {56000000, 0x51, 0x90},                                                                         \
            {63000000, 0x50, 0x80},                                                                         \
            {67000000, 0x47, 0x78},                                                                         \
            {72000000, 0x46, 0x70},                                                                         \
            {77000000, 0x45, 0x68},                                                                         \
            {84000000, 0x44, 0x60},                                                                         \
            {91000000, 0x43, 0x58},                                                                         \
            {100000000, 0x42, 0x50},                                                                        \
            {112000000, 0x41, 0x48},                                                                        \
            {126000000, 0x40, 0x40},                                                                        \
            {134000000, 0x37, 0x3C},                                                                        \
            {144000000, 0x36, 0x38},                                                                        \
            {155000000, 0x35, 0x34},                                                                        \
            {168000000, 0x34, 0x30},                                                                        \
            {183000000, 0x33, 0x2C},                                                                        \
            {201000000, 0x32, 0x28},                                                                        \
            {224000000, 0x31, 0x24},                                                                        \
            {252000000, 0x30, 0x20},                                                                        \
            {268000000, 0x27, 0x1E},                                                                        \
            {288000000, 0x26, 0x1C},                                                                        \
            {310000000, 0x25, 0x1A},                                                                        \
            {336000000, 0x24, 0x18},                                                                        \
            {366000000, 0x23, 0x16},                                                                        \
            {403000000, 0x22, 0x14},                                                                        \
            {448000000, 0x21, 0x12},                                                                        \
            {504000000, 0x20, 0x10},                                                                        \
            {537000000, 0x17, 0x0F},                                                                        \
            {576000000, 0x16, 0x0E},                                                                        \
            {620000000, 0x15, 0x0D},                                                                        \
            {672000000, 0x14, 0x0C},                                                                        \
            {733000000, 0x13, 0x0B},                                                                        \
            {806000000, 0x12, 0x0A},                                                                        \
            {896000000, 0x11, 0x09},                                                                        \
            {1008000000, 0x10, 0x08}                                                                       \
        }                                                                                                  \
    }                                                                                                       \

/*============================================================================*/ 
/* Custom Driver Instance Parameters: (Path 1)                                */
/*============================================================================*/

#define TMDD_TDA18250_INSTANCE_CUSTOM_1                                                                     \
    tmddTDA18250_PowerStandbyWithXtalOn,    /* curPowerState */                                             \
    {                                               /* Config */                                            \
        {                                           /* MAIN_PLL_Map */                                      \
            {33000000, 0x57, 0xF0},                                                                         \
            {36000000, 0x56, 0xE0},                                                                         \
            {38000000, 0x55, 0xD0},                                                                         \
            {42000000, 0x54, 0xC0},                                                                         \
            {45000000, 0x53, 0xB0},                                                                         \
            {50000000, 0x52, 0xA0},                                                                         \
            {56000000, 0x51, 0x90},                                                                         \
            {63000000, 0x50, 0x80},                                                                         \
            {67000000, 0x47, 0x78},                                                                         \
            {72000000, 0x46, 0x70},                                                                         \
            {77000000, 0x45, 0x68},                                                                         \
            {84000000, 0x44, 0x60},                                                                         \
            {91000000, 0x43, 0x58},                                                                         \
            {100000000, 0x42, 0x50},                                                                        \
            {112000000, 0x41, 0x48},                                                                        \
            {126000000, 0x40, 0x40},                                                                        \
            {134000000, 0x37, 0x3C},                                                                        \
            {144000000, 0x36, 0x38},                                                                        \
            {155000000, 0x35, 0x34},                                                                        \
            {168000000, 0x34, 0x30},                                                                        \
            {183000000, 0x33, 0x2C},                                                                        \
            {201000000, 0x32, 0x28},                                                                        \
            {224000000, 0x31, 0x24},                                                                        \
            {252000000, 0x30, 0x20},                                                                        \
            {268000000, 0x27, 0x1E},                                                                        \
            {288000000, 0x26, 0x1C},                                                                        \
            {310000000, 0x25, 0x1A},                                                                        \
            {336000000, 0x24, 0x18},                                                                        \
            {366000000, 0x23, 0x16},                                                                        \
            {403000000, 0x22, 0x14},                                                                        \
            {448000000, 0x21, 0x12},                                                                        \
            {504000000, 0x20, 0x10},                                                                        \
            {537000000, 0x17, 0x0F},                                                                        \
            {576000000, 0x16, 0x0E},                                                                        \
            {620000000, 0x15, 0x0D},                                                                        \
            {672000000, 0x14, 0x0C},                                                                        \
            {733000000, 0x13, 0x0B},                                                                        \
            {806000000, 0x12, 0x0A},                                                                        \
            {896000000, 0x11, 0x09},                                                                        \
            {1008000000, 0x10, 0x08}                                                                        \
        }                                                                                                   \
    }                                                                                                       \



#ifdef __cplusplus
}
#endif

#endif /* _TMDD_TDA18250_INSTANCE_CUSTOM_H */

