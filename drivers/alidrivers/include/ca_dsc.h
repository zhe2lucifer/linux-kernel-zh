#ifndef __CA_DSC_H__
#define __CA_DSC_H__

/*! @page p_history Changes history

    - <b> 0.0.2 - 19-Aug-2014 Romain Baeriswyl </b>
      - More description for DSC

    - <b> 0.0.1 - 08-Aug-2014 Romain Baeriswyl </b>
      - Initial Release
*/

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/*! @mainpage Overview
    - @subpage p_history
    - @subpage p_preface

    <hr>Copyright &copy; 2014 ALi Corporation. All rights reserved.\n
    ZhuHai, China\n
    Tel: +86 756 3392000 \n
    http://www.alitech.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    All trademarks and registered trademarks are the property of
    their respective owners.

    The information contained in this document is subject to
    change without notice.
*/

/* -------------------------------------------------------------------------- */

/*! @page p_preface Preface
    <h2>Objectives</h2>
    This document specifies the Secure Chipset API that gives access to ALi
    security features of the advanced security chipset.

    It includes standard cryptographic functions such as symmetric block
    ciphers(TDES, AES, CSA). Cryptographic operations are based on clear
    text keys, secret keys protected by hardware key ladders.


    <hr><h2>Audience</h2>
    This document is intended for developers in charge of
    implementing/maintaining the Conditional Access System(CAS)
    and as well as other security applications.

   <hr><h2>References</h2>
    - [DSC/KL] ALi DSC/KL API Programming Guide, Version x.x.x\n
 */

/*! @addtogroup DSC
    @{
    @~
*/

#define CA_DSC_BASENAME		"dsc"
	/*!< DSC device base name. */
#define CA_DSC_PATHNAME		"/dev/" CA_DSC_BASENAME
	/*!< DSC device path name. */
#define CA_DSC_DRVNAME			"ca_dsc"
	/*!< DSC device driver name. */
#define CA_DSC_BASE 0xc1
	/*!< DSC ioctl cmd base. */

#define CA_KEY_SIZE_MAX 32
	/*!< DSC max key size. */
#define CA_IV_SIZE_MAX 16
	/*!< DSC max IV size. */

#define CA_PID_MAX 128
	/*!< max number of PIDs in #CA_ADD_PID and #CA_DEL_PID ioctl. */

/*!
     @brief Define packet format
*/
#define CA_FORMAT_RAW		(0)
	/*!< Raw mode, whole packet processed */
#define CA_FORMAT_TS188	(1)
	/*!< DVB-MPEG2 TS packet, 188 Bytes */
#define CA_FORMAT_TS188_LTSID	(2)
	/*!< DVB-MPEG2 TS packet using LTSID replace Sync Byte, 188 Bytes */
#define CA_FORMAT_TS200	(3)
	/*!< CC2.0 TS packet, 200 Bytes */


#define CA_FORMAT_MASK			(0xFF)
#define CA_FORMAT_TS_CHAINING		(1<<16)
	/*!< To enable inter-packet chaining. */
#define CA_FORMAT_CLEAR_UNTOUCHED	(1<<17)
	/*!< To enable the scrambling mode in which clear packet remains
	  untouched. Packet with ODD resp. EVEN scrambling bits are scrambled
	  with ODD resp. EVEN key.
	 */
#define CA_FORMAT_AUTO_TERMINATION	(1<<18)
	/*!< To enable inter-packet chaining with automatic termination. */


/*!
     @brief Define CA DSC operation mode (crypt_mode).
*/
#define CA_ENCRYPT		(1<<0)
	/*!< Indicates the mode is encryption. */
#define CA_DECRYPT		(1<<1)
	/*!< Indicates the mode is decryption. */

/*!
     @brief Define the DSC Crypto algorithm (algo).
*/
#define CA_ALGO_AES	(0)
	/*!< Indicates the algorithm is AES. */
#define CA_ALGO_DES	(1)
	/*!< Indicates the algorithm is DES. */
#define CA_ALGO_TDES	(2)
	/*!< Indicates the algorithm is TDES. */
#define CA_ALGO_CSA1	(3)
	/*!< Indicates the algorithm is CSA1
	     64 bits supports only TS payload data mode. */
#define CA_ALGO_CSA2	(4)
	/*!< Indicates the algorithm is CSA2.
	     64 bits supports only TS payload data mode. */
#define CA_ALGO_CSA3	(5)
	/*!< Indicates the algorithm is CSA3.
	     128 bits support both raw and TS payload data mode. */

/*!
     @brief Define the Crypto chaining modes between blocks (chaining_mode).
*/
#define CA_MODE_ECB	(0)
	/*!< Electronic codeboock, no IV. */
#define CA_MODE_CBC	(1)
	/*!< Cipher-Block chaining. */
#define CA_MODE_OFB	(2)
	/*!< Output Feedback mode. */
#define CA_MODE_CFB	(3)
	/*!< Cipher Feedback mode. */
#define CA_MODE_CTR	(4)
	/*!< Counter mode, 16 bytes IV contains initial counter. */
#define CA_MODE_CTR8	(5)
	/*!< Counter mode, 8 bytes IV contains initial counter. */


/*!
     @brief Define residue block handling mode (residue_mode).
*/
#define CA_RESIDUE_CLEAR	(0)
	/*!< Do not process residue block.
	  Residue block data is same as input. */
#define CA_RESIDUE_AS_ATSC	(1)
	/*!< Process residue block as ANSI SCT 52 standard. */
#define CA_RESIDUE_HW_CTS	(2)
	/*!< The residue block handling uses cipher stealing method. */
#define CA_RESIDUE_CTR_HDL	(3)
	/*! < The CTR residue handle*/

/*!
     @brief Define extra flags for the scrambling/descrambling
     In raw format the tsc flag is ignored.
*/
#define CA_TSC_FLAG_CLEAR	(0x0)
	/*!< scr bits are set to unscrambled after scrambling/descrambling */
#define CA_TSC_FLAG_EVEN	(0x2)
	/*!< scr bits are set to even after scrambling/descrambling */
#define CA_TSC_FLAG_ODD	(0x3)
	/*!< scr bits are set to odd after scrambling/descrambling */
#define CA_TSC_FLAG_AUTO	(1<<2)
	/*!< When set, the scr_bits values above (CA_TSC_FLAG_CLEAR,
	     CA_TSC_FLAG_EVEN, CA_TSC_FLAG_ODD) are ignored.
	    In ca_pid TSC flag: fall back to the respective key's TSC flag.
		In key TSC flag this is only valid in CLEAR_UNTOUCHED mode:
			Set TSC flag following the input packet's TSC.
	*/


/*!
     @brief The following flags are used to indicate in valid_mask to indicate
     which fields of an argument structure are valid.
*/
#define CA_VALID_KEY_ODD	(1<<0)
	/*!< key_odd (and key_size) fields are valid */
#define CA_VALID_KEY_EVEN	(1<<1)
	/*!< key_even (and key_size) fields are valid */
#define CA_VALID_IV_ODD	(1<<2)
	/*!< iv_odd field is valid */
#define CA_VALID_IV_EVEN	(1<<3)
	/*!< iv_even field is valid */
#define CA_VALID_CRYPT_MODE	(1<<4)
	/*!< crypt_mode field is valid */
#define CA_VALID_CHAINING_MODE	(1<<5)
	/*!< chaining_mode field is valid */
#define CA_VALID_RESIDUE_MODE	(1<<6)
	/*!< residue_mode field is valid */
#define CA_VALID_PARITY	(1<<7)
	/*!< parity field is valid */
#define CA_VALID_TSC_FLAG	(1<<8)
	/*!< tsc_flag field is valid. Not valid in raw format */
#define CA_VALID_KL_FD	(1<<9)
	/*!< kl_fd field is valid */


/*!
     @brief Define the key and IV parities to be used for (de)scrambling
     In raw format only the even parity can be used.
*/
#define CA_PARITY_AUTO	(0)
	/*!< Automatically determine parity from each TS packet's TSC flag.

		If TS packet's TSC is even, use even key and iv;
		If TS packet's TSC is odd, use odd key and iv;
		If TS packet's TSC is clear, do not handle this packet.

		This flag only be valid in descrambling or in scrambling with
		CLEAR_UNTOUCHED
	*/
#define CA_PARITY_ODD	(1)
	/*!< Force to use odd key and IV for scrambling.
		This flag only be valid in scrambling without CLEAR_UNTOUCHED
	*/
#define CA_PARITY_EVEN	(2)
	/*!< Force to use even key and IV for scrambling.
		This flag only be valid in scrambling without CLEAR_UNTOUCHED
		and in raw mode.
	*/

/*!
     @brief Define OTP key mode.
*/
#define CA_OTP_KEY_6	(0)
	/*!< OTP key from secret key-6. */
#define CA_OTP_KEY_7	(1)
	/*!< OTP key from secret key-7. */
#define CA_OTP_KEY_FP	(2)
	/*!< OTP key from secret Flash-Protect root key. */

/*!
     @brief Define key size.
*/
#define CA_KEY_SIZE_BYTE_8 (8)
#define CA_KEY_SIZE_BYTE_16 (16)
#define CA_KEY_SIZE_BYTE_24 (24)
#define CA_KEY_SIZE_BYTE_32 (32)


/*!
     @brief Define key_handle that be used with ioctl CA_UPDATE_PARAMS to
     update all keys at the same time.
*/
#define CA_ALL_KEYS (-1)

/*! @struct ca_create_kl_key
     @brief Parameters defining a key generated by KL.
     Note that with current implementation all keys must have the same crypto
     mode and the same parity.
*/
struct ca_create_kl_key {
	int kl_fd;
		/*!< Key ladder file descriptor. */
	int algo;
		/*!< Algorithm set with #CA_ALGO_AES, #CA_ALGO_DES,
		   #CA_ALGO_TDES, #CA_ALGO_CSA1, #CA_ALGO_CSA2
		   or #CA_ALGO_CSA3. */
	int crypt_mode;
		/*!< Operation mode set with #CA_ENCRYPT or #CA_DECRYPT. */
	int chaining_mode;
		/*!< Chaining mode set with #CA_MODE_ECB, #CA_MODE_CBC,
		     #CA_MODE_OFB, #CA_MODE_CFB or #CA_MODE_CTR. */
	int residue_mode;
		/*!< Residue mode which can be #CA_RESIDUE_CLEAR,
		     #CA_RESIDUE_AS_ATSC or #CA_RESIDUE_HW_CTS. */
	int parity;
		/*!< Key parity to use for scrambling/descrambling:
		     #CA_PARITY_AUTO: use parity defined in each packet's TSC
					bits for (de)scrambling
		     #CA_PARITY_ODD: Always use odd parity
		     #CA_PARITY_EVEN: Always use even parity */
	int tsc_flag;
		/*!< extra flag for input/output packets for PIDs with bits
		     Allowed    : #CA_TSC_FLAG_AUTO, #CA_TSC_FLAG_EVEN,
		                  #CA_TSC_FLAG_ODD, #CA_TSC_FLAG_CLEAR.
		     Not allowed: #CA_TSC_FLAG_PES */
	unsigned char iv_odd[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned char iv_even[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned int valid_mask;
		/*!< Mask to indicate which fields of this struct are valid.
		     crypt_mode, chaining_mode, residue_mode, parity and
		     tsc_flag must always be valid in this structure and the
		     respective bits must be set. */
};

/*! @struct ca_create_clear_key
     @brief Parameters defining a clear key.
     Note that with current implementation all keys must have the same crypto
     mode and the same parity.
*/
struct ca_create_clear_key {
	int algo;
		/*!< Algorithm set with #CA_ALGO_AES, #CA_ALGO_DES,
		   #CA_ALGO_TDES, #CA_ALGO_CSA1, #CA_ALGO_CSA2
		   or #CA_ALGO_CSA3. */
	int crypt_mode;
		/*!< Operation mode set with #CA_ENCRYPT or #CA_DECRYPT. */
	int chaining_mode;
		/*!< Chaining mode set with #CA_MODE_ECB, #CA_MODE_CBC,
		   #CA_MODE_OFB, #CA_MODE_CFB or #CA_MODE_CTR. */
	int residue_mode;
		/*!< Residue mode which can be #CA_RESIDUE_CLEAR,
		   #CA_RESIDUE_AS_ATSC or #CA_RESIDUE_HW_CTS. */
	int parity;
		/*!< Key parity to use for scrambling/descrambling:
		     #CA_PARITY_AUTO: use parity defined in each packet's TSC
					bits for (de)scrambling
		     #CA_PARITY_ODD: Always use odd parity
		     #CA_PARITY_EVEN: Always use even parity */
	int tsc_flag;
		/*!< extra flag for input/output packets for PIDs with bits
		     Allowed    : #CA_TSC_FLAG_AUTO, #CA_TSC_FLAG_EVEN,
		                  #CA_TSC_FLAG_ODD, #CA_TSC_FLAG_CLEAR.
		     Not allowed: #CA_TSC_FLAG_PES */
	unsigned char iv_odd[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned char iv_even[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned char key_odd[CA_KEY_SIZE_MAX];
		/*!< Array containing the odd key. */
	unsigned char key_even[CA_KEY_SIZE_MAX];
		/*!< Array containing the even key. */
	int key_size;
		/*!< Size in bytes of the key. Size can be #CA_KEY_SIZE_BYTE_8,
		#CA_KEY_SIZE_BYTE_16, #CA_KEY_SIZE_BYTE_24 or
		#CA_KEY_SIZE_BYTE_32. */
	unsigned int valid_mask;
		/*!< Mask to indicate which fields of this struct are valid.
		     crypt_mode, chaining_mode, parity, tsc_flag and residue_mode
		     must always be valid in this structure and
		     the respective bits must be set. */
};

/*! @struct ca_create_clear_key
     @brief Parameters defining an OTP key.
*/
struct ca_create_otp_key {
	int algo;
		/*!< Algorithm set with #CA_ALGO_AES, #CA_ALGO_TDES*/
	int crypt_mode;
		/*!< Operation mode set with #CA_ENCRYPT or #CA_DECRYPT. */
	int chaining_mode;
		/*!< Chainging mode set with #CA_MODE_ECB, #CA_MODE_CBC,
		     #CA_MODE_OFB, #CA_MODE_CFB or #CA_MODE_CTR. */
	int residue_mode;
		/*!< Residue mode which can be #CA_RESIDUE_CLEAR,
		     #CA_RESIDUE_AS_ATSC or #CA_RESIDUE_HW_CTS. */
	unsigned char iv_even[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. For OTP keys, only the even IV is
		     used and the odd IV is ignored. */
	int otp_key_select;
		/*!< OTP key selector, can be #CA_OTP_KEY_6, #CA_OTP_KEY_7
		     or #CA_OTP_KEY_FP. */
};

/*! @struct ca_pid
     @brief Parameters defining the PID to be added or deleted.
*/
struct ca_pid {
	int key_handle;
	/*!< Key handle*/
	unsigned char ltsid;
	/*!< the LTSID to add or remove in the key handle.
	  When #CA_FORMAT_TS200 mode is used*/
	unsigned char tsc_flag;
	/*!< the user defined flags for the scrambling/descrambling.

        If CA_TSC_FLAG_AUTO bit is set, then the scrambling bits are set
        automatically with the parity of the key used. Otherwise,
        #CA_TSC_FLAG_CLEAR, #CA_TSC_FLAG_EVEN, #CA_TSC_FLAG_ODD set the
        scrambling bits of processed packet.
        if CA_TSC_FLAG_PES bit is set, the scrambling/descrambling is at
        PES level
	*/
	unsigned short pid;
	/*!< the PID to add or remove in the key handle.*/
};


/*! @struct ca_update_params
     @brief Cryptographic parameters to be updated
*/
struct ca_update_params {
	int key_handle;
		/*!< Key handle or CA_ALL_KEYS */
	int kl_fd;
		/*!< Key ladder file descriptor. */
	int crypt_mode;
		/*!< Operation mode set with #CA_ENCRYPT or #CA_DECRYPT. */
	int chaining_mode;
		/*!< Chainging mode set with #CA_MODE_ECB, #CA_MODE_CBC,
		     #CA_MODE_OFB, #CA_MODE_CFB or #CA_MODE_CTR. */
	int residue_mode;
		/*!< Residue mode which can be #CA_RESIDUE_CLEAR,
		     #CA_RESIDUE_AS_ATSC or #CA_RESIDUE_HW_CTS. */
	unsigned char iv_odd[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned char iv_even[CA_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	int parity;
		/*!< Key parity to use for scrambling/descrambling:
		     #CA_PARITY_AUTO: use parity defined in each packet's TSC
					bits for (de)scrambling
		     #CA_PARITY_ODD: Always use odd parity
		     #CA_PARITY_EVEN: Always use even parity */
	int tsc_flag;
		/*!< extra flag for input/output packets for PIDs with bits
		     Allowed    : #CA_TSC_FLAG_AUTO, #CA_TSC_FLAG_EVEN,
		                  #CA_TSC_FLAG_ODD, #CA_TSC_FLAG_CLEAR.
		     Not allowed: #CA_TSC_FLAG_PES */
	unsigned int valid_mask;
		/*!< Mask to indicate which of the fields of this structure
		     are valid. Only parameters with their respective bits
		     set to 1 in this field will be updated */
};

/*! @struct ca_update_clear_key
     @brief key to be updated as clear key
*/
struct ca_update_clear_key {
	int key_handle;
		/*!< Key handle. */
	char key_odd[CA_KEY_SIZE_MAX];
		/*!< Array containing the odd key. */
	char key_even[CA_KEY_SIZE_MAX];
		/*!< Array containing the even key. */
	unsigned int valid_mask;
		/*!< Mask to indicate which of the fields of this structure
		     are valid. Only keys with their respective bits set to
		     one will be updated */
};


/*! @brief define if DIO ioctl should operate synchronously or asynchronously */
#define DIO_FLAG_SYNC  (0<<0)
	/*!< Synchronous operation (ioctl returns when processing is done) */
#define DIO_FLAG_ASYNC (1<<0)
	/*!< Asynchronous operation (ioctl returns immediately and end of
	   processing is signalled through eventfd) */

struct ca_dio_write_read {
	int length;
		/*!< Buffer length in bytes */
	char *input;
		/*!< Pointer to input buffer */
	char *output;
		/*!< Pointer to output buffer. Can be identical to input for
		     in-place operation */
	int crypt_mode;
		/*<! operation mode set with %CA_ENCRYPT or %CA_DECRYPT */
	int mode;
		/*!< DIO_FLAG_SYNC or DIO_FLAG_ASYNC */
	int eventfd;
		/*!< In DIO_FLAG_ASYNC mode, this must be a file descriptor
		   returned by eventfd(). An event is generated on this file
		   descriptor when the operation is finished.
		   This field is ignored in DIO_FLAG_SYNC mode. */
};

/*! @struct ca_calc_cmac
     @brief Calculate the cmac of the specified dram area.
*/
struct ca_calc_cmac {
	char *area_start;
		/*!< Pointer of the area. */
	int size;
		/*!< Size of the area. */
	char *cmac_addr;
		/*!< Output pointer of the cmac calculate. */
};

/*! @struct ca_dsc_ram_mon
 *   @brief Specify the parameters for memory content monitor. 
    If the memory content under monitoring has been changed, the system goes to crash.
 */
struct ca_dsc_ram_mon {
    unsigned int start_addr;
    	/* !< Start address to be monitored. */
    unsigned int end_addr;
    	/* !< End address to be monitored. */
};

/*! @brief Flags for CA_SET_OPT ioctl */
#define CA_SET_CORK   (1<<0)
	/*< Enable chaining in between subsequent write() operations */
#define CA_SET_UNCORK (0<<0)
	/*< Disable chaining in between subsequent write() operations */

/*!
   @brief DSC ioctl list

   All ioctl returns -1 if error occurs and the errno variable contains the
   detailed error.
*/
#define CA_SET_FORMAT        _IOW(CA_DSC_BASE, 1, int /*format*/)
/*!< Sets the format for current session.

    int ioctl(dev, #CA_SET_FORMAT, &format)

    This command set the format of the TS packet which can be:
    #CA_FORMAT_RAW:
	scramble whole packet.
    #CA_FORMAT_TS188:
	scramble payload of DVB-MPEG TS packet of 188 bytes.
    #CA_FORMAT_TS188_LTSID:
	scramble payload of DVB-MPEG TS packet of 188 bytes where LTSID
	replaces SYNC.
    #CA_FORMAT_TS200:
	scramble payload of CC2.0 TS packet of 200 bytes.

    The flag #CA_FORMAT_TS_CHAINING allows to enable inter-packet chaining
    in #CA_FORMAT_TS188, #CA_FORMAT_TS188_LTSID and #CA_FORMAT_TS200 formats.

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_CREATE_KL_KEY     _IOW(CA_DSC_BASE, 2, struct ca_create_kl_key)
/*!< Associates with CA instance a key coming from the key ladder.

    int ioctl(dev, #CA_CREATE_KL_KEY, struct *ca_set_kl_key)

    Returns a positive or zero key handle if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_CREATE_CLEAR_KEY  _IOW(CA_DSC_BASE, 3, struct ca_create_clear_key)
/*!< Associates with the CA instance a clear key.

    int ioctl(dev, #CA_CREATE_CLEAR_KEY, struct *ca_set_clear_key)

    Returns a positive or zero key handle if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_ADD_PID          _IOW(CA_DSC_BASE, 4, struct ca_pid)
/*!< Adds pid in a key handle.

    int ioctl(dev, #CA_ADD_PID, struct *ca_pid)

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.

    Note that all pids associated to a key handle are sharing the same crypto
    key, IV and crypto parameters.
*/

#define CA_DEL_PID           _IOW(CA_DSC_BASE, 5, struct ca_pid)
/*!< Removes pid from a key handle.

    int ioctl(dev, #CA_DEL_PID, struct *ca_pid)

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_UPDATE_CLEAR_KEY  _IOW(CA_DSC_BASE, 6, struct ca_update_clear_key)
/*!< Updates a clear key.

    int ioctl(dev, #CA_UPDATE_CLEAR_KEY, struct *ca_update_clear_key).

    This command can be used on key handles returned by CA_CREATE_CLEAR_KEY
    and CA_CREATE_KL_KEY.

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_UPDATE_PARAMS     _IOW(CA_DSC_BASE, 7, struct ca_update_params)
/*!< Updates the IV, parity and other parameters.

    int ioctl(dev, #CA_UPDATE_PARAMS, struct *ca_update_params)

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_DELETE_KEY        _IOW(CA_DSC_BASE, 8, int /*key_handle*/)
/*!< Removes a key handle from a device instance.

    int ioctl(dev, #CA_DELETE_KEY, int key_handle)

    All associated PIDs are removed as well.
    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_SET_OPT           _IOW(CA_DSC_BASE, 9, int /* option flag */)
/*!< Changes options of a session.

    int ioctl(dev, #CA_SET_OPT, int flag)

    flag #CA_SET_CORK enables chaining between successive write operations.
    flag #CA_SET_UNCORK disables chaining of successive write operations.
    Cork/uncork is used to apply chaining termination only on the last TS
    packet of a sample buffer (ISOBMFF).

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_DIO_WRITE_READ   _IOW(CA_DSC_BASE, 10, struct ca_dio_write_read)
/*!< Performs ram2ram encryption or decryption in synchronous or asynchronous
     mode directly using the user space buffers (direct IO). The kernel
     directly uses these buffers, i.e. they are not copied in the kernel
     space.

     int ioctl(dev, #CA_DIO_WRITE_READ, struct *ca_dio_enc_decrypt)

   If ca_dio_enc_decrypt.flag is DIO_FLAG_SYNC then ioctl returns when
   operation is done and result is available in ca_dio_enc_decrypt.output.

   If ca dio_enc_decrypt.flag is DIO_FLAG_SYNC then ioctl returns immediately
   and result is available when event is generated through the eventfd.

   Returns 0 if successful and -1 if error.
   The variable errno contains the detailed error.
 */

#define CA_CREATE_OTP_KEY     _IOW(CA_DSC_BASE, 11, struct ca_create_otp_key)
/*!< Associates with CA instance a key coming from the OTP directly.

    int ioctl(dev, #CA_CREATE_OTP_KEY, struct *ca_create_otp_key)

    Returns a positive or zero key handle if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_CALC_CMAC     _IOW(CA_DSC_BASE, 12, struct ca_calc_cmac)
/*!< Calculate the cmac of the specified dram area.

    int ioctl(dev, #CA_CALC_CMAC, struct *ca_calc_cmac)

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_INIT_SESSION     _IOW(CA_DSC_BASE, 13, int)
/*!< init the current session, fetch the stream_id and sub_device form SEE. Before 
	you set current session's fd to DMX, you can invok this API to fetch see resource.
	Should be invoked after CA_SET_FORMAT and before CA_CREATE_(CLEAR|KL)_KEY. 

    int ioctl(dev, #CA_INIT_SESSION, int algo)

    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

#define CA_DSC_IO_TRIG_RAM_MON  _IOW(IO_DSC_BASE, 14, struct ca_dsc_ram_mon)                                                                                                  
/*!< Trigger the memory monitor, along with struct ca_dsc_ram_mon.
    int ioctl(dev, #CA_DSC_IO_TRIG_RAM_MON, NULL)
    Returns 0 if successful and -1 if error.
    The variable errno contains the detailed error.
*/

/*!
@}
*/

#endif
