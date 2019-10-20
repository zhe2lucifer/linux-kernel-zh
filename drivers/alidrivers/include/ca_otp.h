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
 

#ifndef _CA_OTP_H__
#define _CA_OTP_H__

/* -------------------------------------------------------------------------- */
/**
 * Modification History:
 * 2014-12-17: Ray.Zhang -- First draft
 * 2015-01-12: Ray.Zhang -- Remove struct of_ali_otp_val
 *
*/

/* -------------------------------------------------------------------------- */

/** @page p_preface Preface
 *  <h2>Objectives</h2>
 *  This document specifies APIs that gives access to ALi
 *  One Time Programming(OTP) device of the advanced security chipset.
 *
 *  <hr><h2>Audience</h2>
 *  This document is intended for developers in charge of
 *  implementing/maintaining the Conditional Access System(CAS)
 *  and as well as other security applications.
 *
 * <hr><h2>References</h2>
 *  - [OTP] ALi OTP API Programming Guide\n
*/

/*!@addtogroup OTP
 *  @{
    @~
 */	

#include <linux/ioctl.h>
#include <linux/types.h>

#define ALI_OTP_DEVNAME "otp0"
/*!< OTP device name.*/
#define ALI_OTP_BASE 0xcc
/*!< OTP ioctl cmd base.*/


/*! @struct otp_paras
 *   @brief Parameters defining OTP read/write parameters.
 */
struct otp_paras {
    unsigned long offset;
    /*!< OTP offset/address(0x00-0xFF).*/
    unsigned char *buf;
    /*!< Buffer pointer containing the OTP value.*/
    int len;
    /*!< Buffer size.*/
};

/*! @struct flash_protect
 *   @brief Parameters defining OTP read flash protecting KEY parameters.
 */
struct flash_protect {
    unsigned long buf[4];   
};

/*! @struct otp_get_mrkcn
 *   @brief Parameters defining OTP get master root key check number parameters.
 */
struct otp_get_mrkcn{
    unsigned int index;
    /*!<Index of Master root key check number.*/
    unsigned int cn;
    /*!<Value of Master root key check number.*/
};

/*
 * OTP ioctl list
 *
 * All ioctl returns -1 if error occurs and the errno variable contains the
 * detailed error.
*/
#define ALI_OTP_READ        _IOW(ALI_OTP_BASE, 1, struct otp_paras)
/*!< This command reads the OTP values from the specified address.

    int ioctl(otp_fd, #ALI_OTP_READ, struct otp_paras)

    It returns 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define ALI_OTP_WRITE        _IOW(ALI_OTP_BASE, 2, struct otp_paras)
/*!< This command writes the OTP values to the specified address.

    int ioctl(otp_fd, #ALI_OTP_WRITE, struct otp_paras)

    It returns 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define ALI_OTP_GET_FP_KEY        _IOW(ALI_OTP_BASE, 3, struct flash_protect)
/*!< This command reads the flash protecting KEY values to the specified address.

    int ioctl(otp_fd, #ALI_OTP_GET_FP_KEY, struct flash_protect)

    It returns 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define ALI_OTP_GET_OTP_MRKCN        _IOW(ALI_OTP_BASE, 4, struct otp_get_mrkcn)
/*!< This command reads the master root key check number values to the specified address.

    int ioctl(otp_fd, #ALI_OTP_GET_OTP_MRKCN, struct otp_get_mrkcn)

    It returns 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#endif /*_CA_OTP_H__*/

