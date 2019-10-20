#ifndef _ADF_SCR_H_
#define _ADF_SCR_H_


/** @page p_history Changes history
 *
 *  - <b> 1.0 - 09-May-2015 </b>
 *    - Initial Release
 *    - Ray.Zhang
*/

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/** @mainpage Overview
 *  - @subpage p_history
 *  - @subpage p_preface
 *
 *  <hr>Copyright &copy; 2013 ALi Corporation. All rights reserved.\n
 *  ZhuHai, China\n
 *  Tel: +86 756 3392000 \n
 *  http://www.alitech.com
 *
 *  All trademarks and registered trademarks are the property of their respective
 *  owners.
 *
 *  This document is supplied with an understanding that the notice(s) herein or
 *  any other contractual agreement(s) made that instigated the delivery of a
 *  hard copy, electronic copy, facsimile or file transfer of this document are
 *  strictly observed and maintained.
 *
 *  The information contained in this document is subject to change without notice.
 *
 *  <b>Security Policy of ALi Corporation</b>\n
 *  Any recipient of this document, without exception, is subject to a
 *  Non-Disclosure Agreement (NDA) and access authorization.
*/

/* -------------------------------------------------------------------------- */

/** @page p_preface Preface
 *  <h2>Objectives</h2>
 *  This document specifies the Secure Chipset API that gives access to ALi 
 *  security features of the advanced security chipset.
 *  
 *  It includes standard cryptographic functions such as symmetric block ciphers 
 *  (TDES, AES), digest functions (CRC32). Cryptographic operations are based on 
 *  cleartext keys, secret keys protected by hardware key ladders.
 *  
 *
 *  <hr><h2>Audience</h2>
 *  This document is intended for developers in charge of implementing/maintaining the Conditional 
 *  Access System (CAS) and as well as other security applications.
 *
 * <hr><h2>References</h2>
 *  - [SCR] ALi SCR API Programming Guide\n
 *  - [KL] ALi Key Ladder API Programming Guide\n
 *  - [TRNG] ALi TRNG API Programming Guide\n
*/

 
/*! @addtogroup ALiSCR
 *  @{
    @~
 */
#define SCR_INVALID_KEY_HANDLE (0xFFFFFFFF)
#define SCR_KEY_SIZE_MAX (32)
#define SCR_IV_SIZE_MAX (16)


/*! @struct scr_dev
 *   @brief Reserved for HLD DSC device structure.
 */
struct scr_dev {
	int type;
	char name[16];
	void *priv;
	int (*attach)(void);
	int (*detach)(void);
	int (*ioctl)(struct scr_dev*, unsigned int, unsigned int);
};

/*! @struct scr_sess_create
     @brief Parameters creating a SCR session.
*/
struct scr_sess_create {
	unsigned int format;
		/*!< data format set with #SCR_PURE_DATA, #SCR_TS_188,
			#SCR_TS_188_LTSID or #SCR_TS_200

			#define SCR_PURE_DATA		(0)
			#define SCR_TS_188			(1)
			#define SCR_TS_188_LTSID	(2)
			#define SCR_TS_200			(3)
		*/
	unsigned int algo;
		/*!< Algorithm set with #SCR_ALGO_AES, #SCR_ALGO_DES,
		   #SCR_ALGO_TDES, #SCR_ALGO_CRC32

			#define SCR_ALGO_AES (0)
			#define SCR_ALGO_DES (1)
			#define SCR_ALGO_TDES (2)
			#define SCR_ALGO_CRC32 (3)
		  */
	unsigned int parity;
		/*!< Packet parity to process which can be #SCR_PARITY_AUTO,
		     #SCR_PARITY_ODD or #SCR_PARITY_EVEN */
	unsigned int residue;
		/*!< Residue mode which can be #SCR_RESIDUE_CLEAR,
		     #SCR_RESIDUE_AS_ATSC, #SCR_RESIDUE_HW_CTS,
		     or #SCR_RESIDUE_CTR_HDL. 

			#define SCR_RESIDUE_CLEAR (0)
			#define SCR_RESIDUE_AS_ATSC (1)
			#define SCR_RESIDUE_HW_CTS (2)
			#define SCR_RESIDUE_CTR_HDL (3)
		*/
	unsigned int chaining;
		/*!< Chaining mode set with #SCR_CHAINING_ECB, #SCR_CHAINING_CBC,
		   #SCR_CHAINING_OFB, #SCR_CHAINING_CFB or #SCR_CHAINING_CTR. */
	unsigned int crypto;
		/*!< crypto mode set with #SCR_CRYPTO_EN, #SCR_CRYPTO_DE.

			#define SCR_CRYPTO_EN (1 << 0)
			#define SCR_CRYPTO_DE (1 << 1)
		*/
	unsigned int continuous;
		/*!< continuous mode set with #SCR_INDEPENDENT_MODE,
		   #SCR_CONTINUOUS_MODE1, or #SCR_CONTINUOUS_MODE2

			#define SCR_INDEPENDENT_MODE (0)
			#define SCR_CONTINUOUS_MODE1 (1)
			#define SCR_CONTINUOUS_MODE2 (2)
		*/
	unsigned int sc_mode;
		/*! < sc_mode set with #SCR_SCRAMBLE_CTRL_MODE0, or
		   #SCR_SCRAMBLE_CTRL_MODE1

			#define SCR_SCRAMBLE_CTRL_MODE0 (0)
			#define SCR_SCRAMBLE_CTRL_MODE1 (1)
		*/
};

/*!
	scr #scr_sess_key key_from
*/
#define SCR_KEY_FROM_SRAM (0)
#define SCR_KEY_FROM_KL (1)

/*! @struct scr_key
     @brief Parameters setting a session key.
*/
struct scr_key {
	unsigned int key_handle;
		/*!< OUT parameter, returns key_handle to user. If fail, #SCR_INVALID_KEY_HANDLE returns*/
	unsigned int key_from;
		/*!< set with #SCR_KEY_FROM_SRAM, #SCR_KEY_FROM_KL*/
	unsigned char key_odd[SCR_KEY_SIZE_MAX];
		/*!< odd key, used when key_from set with #SCR_KEY_FROM_SRAM*/
	unsigned char key_even[SCR_KEY_SIZE_MAX];
		/*!< even key, used when key_from set with #SCR_KEY_FROM_SRAM */
	unsigned char key_size;
		/*!< Size in bytes of the key. Size can be #SCR_KEY_SIZE_BYTE_8, #SCR_KEY_SIZE_BYTE_16,
		#SCR_KEY_SIZE_BYTE_24 or #SCR_KEY_SIZE_BYTE_32.

			#define SCR_KEY_SIZE_BYTE_8 (0)
			#define SCR_KEY_SIZE_BYTE_16 (1)
			#define SCR_KEY_SIZE_BYTE_24 (2)
			#define SCR_KEY_SIZE_BYTE_32 (3)
		*/
	unsigned char iv_odd[SCR_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned char iv_even[SCR_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned int key_pos;
		/*!< KL key pos, used when key_from set with #SCR_KEY_FROM_KL*/
};

/*! @struct scr_update_info
     @brief Parameters updating a session key.
*/
/*!
	scr #scr_update_info even_locate && odd_locate
*/
#define SCR_KEY_LOCATE_SRAM (0)
#define SCR_KEY_LOCATE_KL (1)
/*!
	scr #scr_update_info up_mask
*/
#define SCR_UP_KEY_ODD (1 << 0)
#define SCR_UP_KEY_EVEN (1 << 1)
#define SCR_UP_IV_ODD (1 << 2)
#define SCR_UP_IV_EVEN (1 << 3)
#define SCR_UP_KEY_POS (1 << 4)
#define SCR_UP_PARITY (1 << 5)
#define SCR_UP_RESIDUE (1 << 6)
#define SCR_UP_CHAINING (1 << 7)
#define SCR_UP_TSC_FLAG (1 << 8)

struct scr_update_info {
	unsigned int key_handle;
		/*!< IN parameter.*/
	unsigned char key_odd[SCR_KEY_SIZE_MAX];
		/*!< odd key*/
	unsigned char key_even[SCR_KEY_SIZE_MAX];
		/*!< even key*/
	unsigned char iv_odd[SCR_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned char iv_even[SCR_IV_SIZE_MAX];
		/*!< Array containing the IV. */
	unsigned int key_pos;
		/*!< KL key pos*/
	unsigned int no_even;
		/*! < 1: not update even key or even iv*/
	unsigned int no_odd;
		/*! < 1: not update odd key or odd iv*/
	unsigned int even_locate;
		/*! < key location, set with #SCR_KEY_LOCATE_SRAM or
		#SCR_KEY_LOCATE_KL */
	unsigned int odd_locate;
		/*! < key location, set with #SCR_KEY_LOCATE_SRAM or
		#SCR_KEY_LOCATE_KL */
	unsigned int chaining;
		/*!< Chaining mode set with #SCR_CHAINING_ECB, #SCR_CHAINING_CBC,
		   #SCR_CHAINING_OFB, #SCR_CHAINING_CFB or #SCR_CHAINING_CTR. */
	unsigned int residue;
		/*!< Residue mode which can be #SCR_RESIDUE_CLEAR,
		     #SCR_RESIDUE_AS_ATSC, #SCR_RESIDUE_HW_CTS,
		     or #SCR_RESIDUE_CTR_HDL. 

			#define SCR_RESIDUE_CLEAR (0)
			#define SCR_RESIDUE_AS_ATSC (1)
			#define SCR_RESIDUE_HW_CTS (2)
			#define SCR_RESIDUE_CTR_HDL (3)
		*/
	unsigned int parity;
		/*!< Packet parity to process which can be #SCR_PARITY_AUTO,
		     #SCR_PARITY_ODD or #SCR_PARITY_EVEN */
	unsigned int up_mask;
		/*! < indicate which field need to update*/
	unsigned char tsc_flag;
};

/*! @struct scr_pid
     @brief Parameters handling the PIDs of a session key.
*/
struct scr_pid {
	unsigned int key_handle;
		/*!< IN parameter, indicates the key index*/
	unsigned short pid;
		/*!< PID to add or remove in the key handle. */
	unsigned char ltsid;
		//!< Used when the stream format is TS-200
	unsigned char tsc_flag;
		/*!< the user defined TSC bit configuration flags */
};

/*!
	scr #scr_sess_dio_crypto crypt_mode
*/
#define SCR_ENCRYPT (0)
#define SCR_DECRYPT (1)

/*! @struct scr_sess_dio_crypto
     @brief Parameters for the session to encrypt or decrypt
*/
struct scr_sess_dio_crypto {
	int length;
		/*!< Buffer length in bytes */
	char *input;
		/*!< Pointer to input buffer */
	char *output;
		/*!< Pointer to output buffer. Can be identical to input for
		in-place operation */
	int crypt_mode;
		/*<! operation mode set with %SCR_ENCRYPT or %SCR_DECRYPT*/
};

/*!
	scr #scr_sess_dio_crc32 region_mode
*/
#define SCR_CRC32_NORMAL_MODE (0)
#define SCR_CRC32_SCATTER_MODE (1)

/*! @struct scr_sess_dio_crc32
     @brief Parameters for the session to calculate CRC32
*/
struct scr_sess_dio_crc32 {
	int length;
		/*!< Buffer length in bytes */
	char *input;
		/*!< Pointer to input buffer */
};

enum scr_funcs {
	ALIRPC_RPC_scr_api_attach = 0,
	ALIRPC_RPC_scr_api_detach,
	ALIRPC_RPC_scr_api_ioctl,
	ALIRPC_RPC_scr_session_create,
	ALIRPC_RPC_scr_session_delete,
	ALIRPC_RPC_scr_session_add_key,
	ALIRPC_RPC_scr_session_del_key,
	ALIRPC_RPC_scr_session_update_key,
	ALIRPC_RPC_scr_session_add_pid,
	ALIRPC_RPC_scr_session_del_pid,
	ALIRPC_RPC_scr_session_contns_renew,
	ALIRPC_RPC_scr_session_crypto,
	ALIRPC_RPC_scr_session_crc32_update,
	ALIRPC_RPC_scr_session_crc32_final,
	ALIRPC_RPC_scr_create_sbm_task,
	ALIRPC_RPC_scr_delete_sbm_task,
	ALIRPC_RPC_crc32_create_sbm_task,
	ALIRPC_RPC_crc32_delete_sbm_task,
};

#define SCR_SBM_NR_NODES (512)
#define CRC32_SBM_NR_NODES (256)

struct scr_sbm_packet {
	unsigned char *input;
	unsigned char *output;
	unsigned int len;
	unsigned char crypto;
	unsigned int sess_id;
	unsigned char type; /*0: raw, 188: ts188; 200: ts200*/
};

struct crc32_sbm_packet {
	unsigned int sess_id;
	unsigned char *input;
	unsigned int len;
};


/*!
@}
*/

#endif 

