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
 
#ifndef _CA_SHA_H_
#define _CA_SHA_H_

#define CA_SHA_BASENAME		"sha"
	/*!< SHA device base name. */
#define CA_SHA_PATHNAME		"/dev/" CA_SHA_BASENAME
	/*!< SHA device path name. */
#define CA_SHA_DRVNAME		"ca_sha"
	/*!< SHA device driver name. */
#define CA_SHA_BASE 0xc9
	/*!< SHA ioctl cmd base. */


/*!
     @brief Define SHA type
*/
#define CA_SHA_TYPE_1		(0)
#define CA_SHA_TYPE_224		(1)
#define CA_SHA_TYPE_256		(2)
#define CA_SHA_TYPE_384		(3)
#define CA_SHA_TYPE_512		(4)

struct digest {
	unsigned char *input;
	unsigned int data_len;
	unsigned char *output;
};

/*!
   @brief SHA ioctl list

   All ioctl returns -1 if error occurs and the errno variable contains the
   detailed error.
*/
#define CA_SHA_SET_TYPE        _IOW(CA_SHA_BASE, 1, int /*type*/)
/*!< Sets the SHA information for current session.

    int ioctl(dev, #CA_SHA_SET_TYPE,  &type)

    This command set the type of SHA which can be:
    #CA_SHA_TYPE_1
    #CA_SHA_TYPE_224
    #CA_SHA_TYPE_256
    #CA_SHA_TYPE_384
    #CA_SHA_TYPE_512

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_SHA_DIGEST        _IOW(CA_SHA_BASE, 2, struct digest)

#endif
