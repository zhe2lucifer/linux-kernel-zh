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
 
#ifndef _CA_AKL_H
#define _CA_AKL_H


/* -------------------------------------------------------------------------- */

/*! @page p_history_akl AKL Changes history
 *
 *	- <b> 1.2 - 16-Jul-2015 Youri Zhang </b>
 *	  - Update the Programming Guide Version
 *	- <b> 1.1 - 16-Jul-2015 Youri Zhang </b>
 *	  - Update the parity definition
 *	- <b> 1.0 - 4-May-2015 Youri Zhang </b>
 *	  - Initial Release
*/

/* -------------------------------------------------------------------------- */

/** @mainpage Overview
 *	- @subpage p_history_akl
 *	- @subpage p_history_asa
 *	- @subpage p_preface
 *
 *	<hr>Copyright &copy; 2015 ALi Corporation. All rights reserved.\n
 *	ZhuHai, China\n
 *	Tel: +86 756 3392000 \n
 *	http://www.alitech.com
 *
 *	All trademarks and registered trademarks are the property of their
 *	respective owners.
 *
 *	This document is supplied with an understanding that the notice(s)
 *	herein or any other contractual agreement(s) made that instigated
 *	the delivery of a hard copy, electronic copy, facsimile or file transfer
 *	of this document are strictly observed and maintained.
 *
 *	The information contained in this document is subject to change without
 *	notice.
 *
 *	<b>Security Policy of ALi Corporation</b>\n
 *	Any recipient of this document, without exception, is subject to a
 *	Non-Disclosure Agreement (NDA) and access authorization.
*/

/* -------------------------------------------------------------------------- */

/** @page p_preface Preface
 *	<h2>Objectives</h2>
 *	This document specifies the CERT API that gives access to the Nagra 3.0
 *	IP block Concurrent Embedded Root of Trust (CERT).
 *
 *	It includes Nagra proprietary cryptographic function such as the
 *	symmetric block cipher (ASA) and secret key derivation block
 *	Advanced Key Ladder (AKL).
 *
 *
 *	<hr><h2>Audience</h2>
 *	This document is intended for developers in charge of implementing or
 *	maintaining the NOCS 3.0 CERT API and as well as the developers that
 *	intend to develop the CERT related applications.
 * <hr><h2>References</h2>
 *	- [CERT] CERT-AKL Programming Guide, Version 1.2\n
 *	- [CERT] CERT-ASA Programming Guide, Version 1.2\n
*/

/*!@addtogroup CA_AKL
 *	@{
	@~
 */

#define CA_AKL_IO_BASE (0xC0<<8)
/*!< CERT IO cmd base in kernel.*/
#define CA_AKL_DEV "/dev/ca_akl"
/*!< CERT-AKL device node, used for accessing the CERT-AKL logic.*/
#define CA_AKL_KEY_DEV "/dev/ca_akl_key"
/*!< CERT-AKL key device node, used for accessing the CERT-AKL key.*/
#define CA_AKL_DATA_SIZE  (32)
/*!< CERT-AKL cmd size in bytes (32 bytes).*/


/*!
 *	 @brief Define the AKL key parity for saving key to the corresponding key position.
*/
#define CA_AKL_ODD_PARITY (1)
/*!< copy the key to ODD position.*/
#define CA_AKL_EVEN_PARITY (2)
/*!< copy the key to EVEN position.*/

/*!
 *	 @brief Define the AKL key usage.
*/
#define CA_AKL_FOR_AES (0)
/*!< Specify that the AKL key is for AES.*/
#define CA_AKL_FOR_TDES (1)
/*!< Specify that the AKL key is for TDES.*/
#define CA_AKL_FOR_ASA	(8)
/*!< Specify that the AKL key is for ASA.*/
#define CA_AKL_FOR_CSA3 (9)
/*!< Specify that the AKL key is for CSA3.*/
#define CA_AKL_FOR_CSA2 (10)
/*!< Specify that the AKL key is for CSA2/CSA1.1.*/


/*! @struct ca_akl_cmd
 *	@brief Define the command elements for #CA_IO_AKL_EXCHANGE, byte
 *	order is MSB. @~
 */
struct ca_akl_cmd {
	unsigned char data_in[CA_AKL_DATA_SIZE];
	/*!< Input data to exchange with AKL.*/
	unsigned char data_out[CA_AKL_DATA_SIZE];
	/*!< Output data of AKL.*/
	unsigned char status[4];
	/*!< Output status of AKL.*/
	unsigned char opcodes[4];
	/*!< Opcodes of AKL.*/
	unsigned int timeout;
	/*!< Timeout type, default is 0.*/
};

/*! @struct ca_akl_key
 *	 @brief Define the algorithm and parity information for #CA_IO_AKL_SAVEKEY
*/
struct ca_akl_key {
	int keyfd;
	/*!< Specify the destination akl key fd which will be
	used for saving this key.*/
	int algo;
	/*!< Specify which crypto algorithm will use this AKL key,
	algorithms are #CA_AKL_FOR_AES, #CA_AKL_FOR_TDES,
	#CA_AKL_FOR_ASA, #CA_AKL_FOR_CSA3 and #CA_AKL_FOR_CSA2.
	Once the algo is set for one AKL key fd, it can't be changed any more,
	otherwise the ioctl() will return -EEXIST.
	*/
	int parity;
	/*!< Specify the key parity #CA_AKL_EVEN_PARITY or
	#CA_AKL_ODD_PARITY to be saved to.*/
};


#define  CA_IO_AKL_EXCHANGE _IOW(CA_AKL_IO_BASE, 1, struct ca_akl_cmd)
/*!< Feed the command and data to AKL. Refer to the struct ca_akl_cmd

	int ioctl(cert, CA_IO_AKL_EXCHANGE, {exchange cmd/data});

	It returns 0 if successful and -1 if error.
	Variable errno contains the detail error information.
*/
#define  CA_IO_AKL_SAVEKEY _IOW(CA_AKL_IO_BASE, 2, struct ca_akl_key)
/*!< Trigger to copy the AKL key to the corresponding secret SRAM position,
	Refer to the struct ca_akl_key.

	One key fd is only associated with one set of EVEN/ODD key,
	the corresponding secret SRAM position will be allocated after
	the algorithm is speficied for this key fd.

	int ioctl(cert, CA_IO_AKL_SAVEKEY, {keyfd, algorithm, parity});

	It returns 0 if successful and -1 if error.
	Variable errno contains the detail error information.
*/
#define  CA_IO_AKL_ACK _IO(CA_AKL_IO_BASE, 3)
/*!< Acknowledge the AKL IP. If the AKL key has been copied to secret SRAM
	or user needs to discard this AKL key, this acknowledge to AKL IP must
	be sent,	otherwise the AKL is unable to output another key.

	int ioctl(cert, CA_IO_AKL_ACK, NULL);

	It returns 0 if successful and -1 if error.
	Variable errno contains the detail error information.
*/

/*!
@}
*/

#endif
