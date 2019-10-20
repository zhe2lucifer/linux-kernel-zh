/*
 * Conax Virtual Smart Card Core driver
 * Copyright(C) 2016 ALi Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CA_VSC_H__
#define __CA_VSC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>
#include <linux/types.h>

//#define _CAS9_VSC_RAP_ENABLE_

#define VSC_DRVNAME       "ca_vsc"
	/*!< VSC driver name.*/
#define VSC_DEVNAME       "vsc0"
	/*!< VSC device node, used for accessing the VSC-Lib */
#define VSC_STORE_DEVNAME "vsc_store0"
	/*!< VSC device node, used for accessing config upload/download */



/*!< VSC ioctl cmd base.*/
#define VSC_IOCTL_BASE 0xca /* 0xca */

#define VSC_LIB_MAX_LEN      (0x20000)
#define VSC_STORE_DATA_SIZE  (0x10000)
#define VSC_STORE_KEY_SIZE   (16)
#define VSC_STORE_HASH_SIZE  (32)

#define VSC_DATA_SIZE_MAX  (4096)
	/*!< VSC protocol max data transfer size */

#define VSC_KEY_DECW_SIZE  (16)
	/*!< VSC decrypted CW key size*/

#define VSC_CK_PARITY_ODD  (1 << 0)
	/*!< Define the decw key parity to run. Parity odd.*/
#define VSC_CK_PARITY_EVEN (1 << 1)
	/*!< Define the decw key parity to run. Parity even.*/

/* VSC storage buffer configuration */
struct vsc_store {
	__u8 *data;
		/*!< Array containing VSC store data */
	__u32 data_len;
		/*!< data len, max VSC_STORE_DATA_SIZE */
	__u8 random_key[VSC_STORE_KEY_SIZE];
		/*!< Array containing random key */
	__u8 hash[VSC_STORE_HASH_SIZE];
		/*!< Array containing hash */
};

/* VSC-Lib address parameters */
struct vsc_lib_init_params {
	__s32 len;
	/* VSC lib length. If -1 is specified, the max value defined
	   in the DTS is used */
};

/* VSC command data transfer parameters */
struct vsc_cmd_transfer
{
	__u8 session_id;
		/* session id */
	__u8 command[VSC_DATA_SIZE_MAX];
		/* Array containing command for VSC_Lib */
	__s32 num_to_write;
		/* number of bytes to write */
	__u8 response[VSC_DATA_SIZE_MAX];
		/* Array containing response from VSC-Lib */
	__s32 response_len;
		 /* number of response bytes read */
	__u8 sw1;
		/* status word 1 */
	__u8 sw2;
		/* status word 1 */
};

/*! @struct kl_vsc_decw_key
 *   @brief decrypted CW key for running the vsc dedicated key ladder.
 */
struct vsc_decw_key {
	int kl_fd;
		/*!< Specify the destination key fd (allocated with an open() on the /dev/kl/vsc0)
		which will be used for saving this key.*/
	__u16 key_id;
		/*!< Request key via a previously set key_id.
		   The response keys will be kept in private memory */
	char decw_key[VSC_KEY_DECW_SIZE];
		/*!< Array containing decrypted CW key.*/
	int ck_parity;
		/*!< Content key parity mode,
		#VSC_CK_PARITY_EVEN for even parity,
		#VSC_CK_PARITY_ODD for odd parity.*/
};

/*
 * List of IOCTL commands
 */

/*
    All listed IOCTLs returns 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define VSC_STORE_WRITE _IOW(VSC_IOCTL_BASE, 0, struct vsc_store)
/*!<
    This command would fetch VSC storage data to VSC private data area.

    Applicable to character device interface /dev/vsc_store0

    int ioctl(vsc_store, #VSC_STORE_WRITE, struct vsc_store*);

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define VSC_STORE_READ _IOR(VSC_IOCTL_BASE, 1, struct vsc_store)
/*!<
    This command would fetch VSC storage data from VSC private data area.

    Applicable to character device interface /dev/vsc_store0

    int ioctl(vsc_store, #VSC_STORE_READ, struct vsc_store*);

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define VSC_LIB_INIT _IOR(VSC_IOCTL_BASE, 2, struct vsc_lib_init_params)
/*!<
    This command would initialize the VSC-Lib.

    Applicable to character device interface /dev/vsc_store0

    int ioctl(vsc_store, #VSC_LIB_INIT, struct vsc_lib_init_params*);

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define VSC_CMD_DISPATCH _IOW(VSC_IOCTL_BASE, 3, int/*struct vsc_cmd_transfer*/)
/*!<
    This command would initiate smart card data transaction with
    virtual smart card.

    Applicable to character device interface /dev/vsc0

    int ioctl(vsc, #VSC_CMD_DISPATCH, struct vsc_cmd_transfer*);

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define VSC_DECW_KEY      _IOW(VSC_IOCTL_BASE, 4, struct vsc_decw_key)
/*!<
    This command would send the decryted CW and it's parity for
    running the vsc dedicated key ladder.

    Applicable to character device interface /dev/vsc0

    int ioctl(vsc, #VSC_DECW_KEY, struct kl_vsc_decw_key*);

    These resources are released when closing the device.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/
#ifdef _CAS9_VSC_RAP_ENABLE_
#define VSC_LIB_GET_KEY _IOW(VSC_IOCTL_BASE, 5, unsigned short)
/*!<
    This command only use in RAP for get VSC key ID

    Applicable to character device interface /dev/vsc0

    int ioctl(vsc, #ALI_VSC_LIB_GET_KEY, unsigned short);

    These resources are released when closing the device.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define VSC_CLEAR_STORE      _IO(VSC_IOCTL_BASE, 6)
/*!<
    This command only use in RAP clear store memory

    Applicable to character device interface /dev/vsc0

    int ioctl(vsc, #VSC_CLEAR_STORE, 0);

    These resources are released when closing the device.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/
#endif

#ifdef __cplusplus
}
#endif

#endif
