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
 
/****************************************************************************
 *  File: ali_smartcard_binding.h
 *
 *  Description: This file contains all globe micros and functions declare
 *		             of smartcard reader IC binding.
 *
 *  History:
 *      Date            Author            Version   Comment
 *      ====        ======      =======  =======
 *     
 ****************************************************************************/

#ifndef _ALI_SMARTCARD_BINDING_H_
#define _ALI_SMARTCARD_BINDING_H_

#include "ali_smartcard.h"

/* Compatibility */
#define ALI_M3202				(HW_TYPE_CHIP + 10)
#define ALI_M3101				(HW_TYPE_CHIP + 30)

/* Those function need to be porting from TDS ---- Owen */
#define sys_ic_get_chip_id  ali_sys_ic_get_chip_id
#define sys_ic_get_rev_id   ali_sys_ic_get_rev_id
#define sys_ic_is_M3101()   0
#define sys_ic_is_M3202()   0

/* IC bonding type list */
#define SYS_IC_BONDING_TYPE_1(tp) \
        ((ALI_M3101 == tp->smc_chip_id) || \
         (ALI_M3329E == tp->smc_chip_id && tp->smc_chip_version >= IC_REV_3) || \
         (ALI_S3602F == tp->smc_chip_id) || \
		 (ALI_C3701 == tp->smc_chip_id) || \
		 (ALI_C3921 == tp->smc_chip_id) || \
		 (ALI_S3503 == tp->smc_chip_id) || \
		 (ALI_S3821 == tp->smc_chip_id) || \
		 (ALI_C3505 == tp->smc_chip_id))
#define SYS_IC_BONDING_TYPE_2(tp) \
       ((ALI_M3101 == tp->smc_chip_id) || \
        (ALI_M3329E == tp->smc_chip_id && tp->smc_chip_version >= IC_REV_5) || \
        (ALI_S3602F == tp->smc_chip_id) || \
		(ALI_C3701 == tp->smc_chip_id) || \
		(ALI_C3921 == tp->smc_chip_id) || \
		(ALI_S3503 == tp->smc_chip_id) || \
		(ALI_S3821 == tp->smc_chip_id) || \
		(ALI_C3505 == tp->smc_chip_id))
#define SYS_IC_BONDING_TYPE_3(tp) \
       (tp->smc_chip_id == ALI_S3602 && tp->smc_chip_version >= IC_REV_6)
#define SYS_IC_BONDING_TYPE_4(tp) sys_ic_is_M3101()
#define SYS_IC_BONDING_TYPE_5(tp) (ALI_S3602 == tp->smc_chip_id)
#define SYS_IC_BONDING_TYPE_6(tp) (ALI_M3329E == tp->smc_chip_id)
#define SYS_IC_BONDING_TYPE_7(tp) \
       ((ALI_M3101 == tp->smc_chip_id) || \
		(ALI_S3602F == tp->smc_chip_id) || \
		(ALI_C3701 == tp->smc_chip_id) || \
		(ALI_C3921 == tp->smc_chip_id) || \
        (ALI_S3503 == tp->smc_chip_id) || \
        (ALI_S3821 == tp->smc_chip_id) || \
        (ALI_C3505 == tp->smc_chip_id))
#define SYS_IC_BONDING_TYPE_8(tp) (sys_ic_is_M3202())
#define SYS_IC_BONDING_TYPE_9(tp) (tp->smc_chip_version >= IC_REV_2)
#define SYS_IC_BONDING_TYPE_10(tp) (tp->smc_chip_version >= IC_REV_3)
#define SYS_IC_BONDING_TYPE_11(tp) (tp->smc_chip_version >= IC_REV_5)
#define SYS_IC_BONDING_TYPE_12(tp) (tp->smc_chip_version > IC_REV_2)
#define SYS_IC_BONDING_TYPE_13(tp) \
       ((ALI_S3602 == tp->smc_chip_id) || \
		(ALI_S3602F == tp->smc_chip_id) || \
		(ALI_C3701 == tp->smc_chip_id) || \
		(ALI_C3921 == tp->smc_chip_id) || \
        (ALI_S3503 == tp->smc_chip_id) || \
        (ALI_S3821 == tp->smc_chip_id) || \
        (ALI_C3505 == tp->smc_chip_id))
#define SYS_IC_BONDING_TYPE_14(tp) (ALI_M3101 == tp->smc_chip_id)
#define SYS_IC_BONDING_TYPE_15(tp) (sys_ic_is_M3202() && (tp->smc_chip_version == IC_REV_2))
#define SYS_IC_BONDING_TYPE_16(tp) \
       ((ALI_M3329E == tp->smc_chip_id && tp->smc_chip_version >= IC_REV_5) || \
        (ALI_S3602F == tp->smc_chip_id) || \
        (ALI_M3101 == tp->smc_chip_id) || \
        (ALI_C3701 == tp->smc_chip_id) || \
        (ALI_C3921 == tp->smc_chip_id) || \
        (ALI_S3503 == tp->smc_chip_id) || \
        (ALI_S3821 == tp->smc_chip_id) || \
        (sys_ic_is_M3202() && tp->smc_chip_version > IC_REV_2) || \
        (ALI_C3505 == tp->smc_chip_id))
#define SYS_IC_BONDING_TYPE_17(tp) \
       (((ALI_C3701 == tp->smc_chip_id) && (tp->smc_chip_version == IC_REV_0)) || \
       ((ALI_C3921 == tp->smc_chip_id) && (tp->smc_chip_version == IC_REV_0)))

#endif
 