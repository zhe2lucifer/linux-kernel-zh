#ifndef  _ADF_CE_H_
#define  _ADF_CE_H_

/*!@addtogroup ALiKeyLadder
 *  @{
    @~
 */
 
#define IO_CE_BASE                                          (0xC0<<8) 
//!< KeyLadder ioctl cmd base.
#define IO_OTP_ROOT_KEY_GET                        (IO_CE_BASE + 0) 
//!< Load the specified OTP root key to KEY_0_X, along with 'struct OTP_PARAM'.
#define IO_CRYPT_DEBUG_GET_KEY                   (IO_CE_BASE + 5) 
//!< Reserved for caller to get the KL debug key.
#define IO_CRYPT_POS_IS_OCCUPY                   (IO_CE_BASE + 6) 
//!< Check if the key idex is occupied or not, along with 'struct CE_POS_STS_PARAM'.
#define IO_CRYPT_POS_SET_USED                     (IO_CE_BASE + 7) 
//!< Set the key index status to busy directly, usually used on the application initialization stage.
#define IO_CRYPT_POS_SET_IDLE                      (IO_CE_BASE + 8) 
//!< Set the key index status to idle.
#define IO_CRYPT_FOUND_FREE_POS                 (IO_CE_BASE + 9)
//!< Get an idle key index and set this index status to busy, along with 'struct CE_FOUND_FREE_POS_PARAM'.
#define IO_DECRYPT_PVR_USER_KEY                 (IO_CE_BASE + 10)
//!< Used to decrypt the PVR cipher key to the specified PVR key index, refer to 'struct CE_PVR_KEY_PARAM', only used in DMX driver.

#define  IO_CRYPT_GEN_NLEVEL_KEY       (IO_CE_BASE + 12)
/*!< Generate n level keys, refer to 'struct CE_NLEVEL_PARAM'. HW has only 
one intermediate key_pos for one KL, 
thus driver shall generate all the required levels in one call to ensure the 
thread-safety. */

#define  IO_CRYPT_CW_DERIVE_CW       (IO_CE_BASE + 13)
/*!< Generate CW from CW, refer to 'struct CE_CW_DERIVATION'.*/

#define  IO_CRYPT_ETSI_CHALLENGE       (IO_CE_BASE + 14)
/*!< Perform ETSI challenge*/

#define IO_CE_VSC_OFFSET 0x50
/*! @VSC dediacated key ladder.
 *	 @bried the ioctl cmd for vsc dedicated key ladder.
 */
#define IO_CRYPT_VSC_DECW_KEY					(IO_CE_BASE + IO_CE_VSC_OFFSET + 0)
//!< Send the encrypted CW to the dedicated vsc module, along with 'CE_DECW_KEY_PARAM'.
#define IO_CRYPT_VSC_REC_EN_KEY					(IO_CE_BASE + IO_CE_VSC_OFFSET + 1)
//!< Send the encrypted key for the pvr record, along with 'CE_REC_KEY_PARAM'.
#define IO_CRYPT_VSC_UK_EN_KEY					(IO_CE_BASE + IO_CE_VSC_OFFSET + 2)
//!< Send the encrypted key for the UK, along with 'CE_UK_KEY_PARAM'.
#define IO_CRYPT_VSC_CW_DERIVE_CW					(IO_CE_BASE + IO_CE_VSC_OFFSET + 3)
//!< Send the key position of CACWC,CFCWC,CW to the dedicated vsc module, along with 'CE_CW_DERIVATION'.

#define CE_IO_CMD(cmd)  (cmd & 0xFF) //!< Mask the cmd to 8 bits, only 8 bits valid for KL ioctl cmd.

#define AES_CE_KEY_LEN  16 //!< Indicate the AES cipher block length in KL is 16 bytes.
#define TDES_CE_KEY_LEN  8 //!< Indicate the DES cipher block length in KL is 8 bytes.

#define INVALID_ALI_CE_KEY_POS 0xff  //!< Define the invalid KL key index
#define ALI_INVALID_CRYPTO_KEY_POS   INVALID_ALI_CE_KEY_POS //!< Define the invalid KL key index


/*! @enum CE_OTP_KEY_ADDR
 *   @brief OTP physical address of the KL root keys.
 */
enum CE_OTP_KEY_ADDR
{
	OTP_ADDESS_1 = 0x4d, //!< OTP physical address of secure key 0.
	OTP_ADDESS_2 = 0x51, //!< OTP physical address of secure key 1.
	OTP_ADDESS_3 = 0x55, //!< OTP physical address of secure key 2.
	OTP_ADDESS_4 = 0x59, //!< OTP physical address of secure key 3.
	OTP_ADDESS_5 = 0x60, //!< OTP physical address of secure key 4.	
	OTP_ADDESS_6 = 0x64, //!< OTP physical address of secure key 5.	
};

/*! @enum CRYPTO_STATUS
 *   @brief Return values of the KL functions.
 */
enum CRYPTO_STATUS
{
	ALI_CRYPTO_SUCCESS = 0, //!< The intended operation was executed successfully.	
	ALI_CRYPTO_ERROR, //!< The function was terminated abnormally. The intended operation failed.
	ALI_CRYPTO_WARNING_DRIVER_ALREADY_INITIALIZED, //!< The SEE KL is already initialized.
	ALI_CRYPTO_ERROR_INVALID_PARAMETERS, //!< The passed parameters are invalid.
	ALI_CRYPTO_ERROR_OPERATION_NOT_ALLOWED,//!< The requested operation is not allowed.
	ALI_CRYPTO_ERROR_OPERATION_NOT_SUPPORTED,//!< The requested operation is not supported.
	ALI_CRYPTO_ERROR_INITIALIZATION_FAILED,//!< The SEE KL initialization failed.
	ALI_CRYPTO_ERROR_DRIVER_NOT_INITIALIZED,//!< The SEE KL has not been initialized.
	ALI_CRYPTO_ERROR_INVALID_ADDR,//!< The passed address is invalid.
	ALI_CRYPTO_ERROR_INVALID_DEV,//!< The passed device pointer is invalid.
};

#define ALI_KL_PREFIX (0x00)
#define ETSI_KL_PREFIX (0x10)
/*! @enum CE_OTP_KEY_SEL
 *   @brief Define the KL root key index.
 */
enum CE_OTP_KEY_SEL
{
    OTP_KEY_0_0 = 0, //!< Load root key from OTP 0x4d.
    OTP_KEY_0_1 = 1, //!< Load root key from OTP 0x51.
    OTP_KEY_0_2 = 2, //!< Load root key from OTP 0x55 or OTP 0x60.
    OTP_KEY_0_3 = 3, //!< Load root key from OTP 0x59 or OTP 0x64.
    OTP_KEY_0_4 = 4,
    OTP_KEY_0_5 = 5,

    ALI_KL_0 = 0, //! < choose to use ALi Kl 0, load root key from OTP 0x4d
    ALI_KL_1 = 1, //! < choose to use ALi Kl 1, load root key from OTP 0x51
    ALI_KL_2 = 2, //! < choose to use ALi Kl 2, load root key from OTP 0x55 or 0x60
    ALI_KL_3 = 3, //! < choose to use ALi Kl 3, load root key from OTP 0x59 or 0x64
    ALI_KL_4 = 4, //! < choose to use ALi Kl 4, load root key from OTP 0x60
    ALI_KL_5 = 5, //! < choose to use AKL
    
    ETSI_KL_0 = (ETSI_KL_PREFIX | 0), //!< choose to use ETSI Kl 0, introduced from CAP210 on
    ETSI_KL_1 = (ETSI_KL_PREFIX | 1), //!< choose to use ETSI Kl 1
    ETSI_KL_2 = (ETSI_KL_PREFIX | 2), //!< choose to use ETSI Kl 2
    ETSI_KL_3 = (ETSI_KL_PREFIX | 3), //!< choose to use ETSI Kl 3
    ETSI_KL_4 = (ETSI_KL_PREFIX | 4), //!< choose to use ETSI Kl 4
};

/*! @enum HDCP_DECRYPT_MODE
 *   @brief Define KL SRAM operation mode, which is usually #NOT_FOR_HDCP.
 */
enum  HDCP_DECRYPT_MODE
{
	NOT_FOR_HDCP=0, //!< For KL key operation.
	TARGET_IS_HDCP_KEY_SRAM=(1<<14) //!< For HDCP key operation
};

/*! @enum CE_CRYPT_TARGET
 *   @brief Define the 1st, 2nd, and 3rd level keys. CRYPTO_KEY_X_X is the equivalent to KEY_X_X. 
 *	The first X indicates the key's level, the second X indicates the key's index in current level. \n
 *    CRYPT_KEY_1_X is generated by OTP_KEY_0_X; CRYPT_KEY_2_X is generated by CRYPT_KEY_1_X;\n
 *    CRYPT_KEY_3_X is generated by CRYPT_KEY_2_X;
 */
enum CE_CRYPT_TARGET
{
	CRYPT_KEY_1_0=0x4, //!< 0x4
	CRYPT_KEY_1_1=0x5, //!< 0x5
	CRYPT_KEY_1_2=0x6, //!< 0x6
	CRYPT_KEY_1_3=0x7, //!< 0x7

	CRYPT_KEY_2_0=0x8, //!< 0x8
	CRYPT_KEY_2_1=0x9, //!< 0x9
	CRYPT_KEY_2_2=0xa, //!< 0xA
	CRYPT_KEY_2_3=0xb, //!< 0xB
	CRYPT_KEY_2_4=0xc, //!< 0xC
	CRYPT_KEY_2_5=0xd, //!< 0xD
	CRYPT_KEY_2_6=0xe, //!< 0xE
	CRYPT_KEY_2_7=0xf, //!< 0xF

	CRYPT_KEY_3_0=0x10, //!< 0x10
	CRYPT_KEY_3_1=0x11, //!< 0x11
	CRYPT_KEY_3_2=0x12, //!< 0x12
	CRYPT_KEY_3_3=0x13, //!< 0x13
	CRYPT_KEY_3_4=0x14, //!< 0x14
	CRYPT_KEY_3_5=0x15, //!< 0x15
	CRYPT_KEY_3_6=0x16, //!< 0x16
	CRYPT_KEY_3_7=0x17, //!< 0x17
	CRYPT_KEY_3_8=0x18, //!< 0x18
	CRYPT_KEY_3_9=0x19, //!< 0x19
	CRYPT_KEY_3_10=0x1a, //!< 0x1A
	CRYPT_KEY_3_11=0x1b, //!< 0x1B
	CRYPT_KEY_3_12=0x1c, //!< 0x1C
	CRYPT_KEY_3_13=0x1d, //!< 0x1D
	CRYPT_KEY_3_14=0x1e, //!< 0x1E
	CRYPT_KEY_3_15=0x1f, //!< 0x1F

	CRYPT_KEY_3_16=0x20, //!< 0x20
	CRYPT_KEY_3_17=0x21, //!< 0x21
	CRYPT_KEY_3_18=0x22, //!< 0x22
	CRYPT_KEY_3_19=0x23, //!< 0x23
	CRYPT_KEY_3_20=0x24, //!< 0x24
	CRYPT_KEY_3_21=0x25, //!< 0x25
	CRYPT_KEY_3_22=0x26, //!< 0x26
	CRYPT_KEY_3_23=0x27, //!< 0x27
	CRYPT_KEY_3_24=0x28, //!< 0x28
	CRYPT_KEY_3_25=0x29, //!< 0x29
	CRYPT_KEY_3_26=0x2a, //!< 0x2A
	CRYPT_KEY_3_27=0x2b, //!< 0x2B
	CRYPT_KEY_3_28=0x2c, //!< 0x2C
	CRYPT_KEY_3_29=0x2d, //!< 0x2D
	CRYPT_KEY_3_30=0x2e, //!< 0x2E
	CRYPT_KEY_3_31=0x2f, //!< 0x2F

	CRYPT_KEY_3_32=0x30, //!< 0x30
	CRYPT_KEY_3_33=0x31, //!< 0x31
	CRYPT_KEY_3_34=0x32, //!< 0x32
	CRYPT_KEY_3_35=0x33, //!< 0x33
	CRYPT_KEY_3_36=0x34, //!< 0x34
	CRYPT_KEY_3_37=0x35, //!< 0x35
	CRYPT_KEY_3_38=0x36, //!< 0x36
	CRYPT_KEY_3_39=0x37, //!< 0x37

	CRYPT_KEY_3_40=0x38, //!< 0x38
	CRYPT_KEY_3_41=0x39, //!< 0x39
	CRYPT_KEY_3_42=0x3a, //!< 0x3A
	CRYPT_KEY_3_43=0x3b, //!< 0x3B
	CRYPT_KEY_3_44=0x3c, //!< 0x3C
	CRYPT_KEY_3_45=0x3d, //!< 0x3D
	CRYPT_KEY_3_46=0x3e, //!< 0x3E
	CRYPT_KEY_3_47=0x3f, //!< 0x3F
};

/*! @enum CE_KEY
 *   @brief Define the 1st, 2nd, and 3rd level keys, KEY_X_X is the equivalent to CRYPTO_KEY_X_X. 
 *   The first X indicates the key's level, the second X indicates the key's index in current level.
 */
enum CE_KEY
{
	KEY_0_0=0, //!< 0x0
	KEY_0_1=1, //!< 0x1
	KEY_0_2=2, //!< 0x2
	KEY_0_3=3, //!< 0x3
	KEY_1_0=4, //!< 0x4
	KEY_1_1=5, //!< 0x5
	KEY_1_2=6, //!< 0x6
	KEY_1_3=7, //!< 0x7
	KEY_2_0=8, //!< 0x8
	KEY_2_1=9, //!< 0x9
	KEY_2_2=0xa, //!< 0xA
	KEY_2_3=0xb, //!< 0xB
	KEY_2_4=0xc, //!< 0xC
	KEY_2_5=0xd, //!< 0xD
	KEY_2_6=0xe, //!< 0xE
	KEY_2_7=0xf, //!< 0xF
	KEY_3_0=0x10, //!< 0x10
	KEY_3_1=0x11, //!< 0x11
	KEY_3_2=0x12, //!< 0x12
	KEY_3_3=0x13, //!< 0x13
	KEY_3_4=0x14, //!< 0x14
	KEY_3_5=0x15, //!< 0x15
	KEY_3_6=0x16, //!< 0x16
	KEY_3_7=0x17, //!< 0x17
	KEY_3_8=0x18, //!< 0x18
	KEY_3_9=0x19, //!< 0x19
	KEY_3_10=0x1a, //!< 0x1A
	KEY_3_11=0x1b, //!< 0x1B
	KEY_3_12=0x1c, //!< 0x1C
	KEY_3_13=0x1d, //!< 0x1D
	KEY_3_14=0x1e, //!< 0x1E
	KEY_3_15=0x1f, //!< 0x1F
};

/*! @enum CE_CRYPT_SELECT
 *   @brief Define KL encryption and decryption mode.
 */
enum CE_CRYPT_SELECT
{  
	CE_IS_DECRYPT = 1,//!<Decryption
	CE_IS_ENCRYPT=0//!<Encryption
};

/*! @enum CE_MODULE_SELECT
 *   @brief Define KL algorithm selection.
 */
enum CE_MODULE_SELECT
{
    CE_SELECT_DES = 0,  //!<KL algorithm is DES.
    CE_SELECT_AES = 1,  //!<KL algorithm is AES.
    CE_SELECT_XOR = 2,  //!<KL algorithm is XOR.
    CE_SELECT_AES64BIT = 3, //!<KL algorithm is AES-64Bit.
    CE_SELECT_XOR64BIT = 4, //!<KL algorithm is XOR-64Bit.
};

enum DATA_MODULE_MODE
{
    BIT128_DATA_MODULE = 0,
    BIT64_DATA_MODULE =  1
};

enum DATA_HILO_MODE
{
    LOW_64BITS_DATA = 0,
    HIGH_64BITS_DATA= 1
};

enum KEY_MODULE_FROM
{
    CE_KEY_FROM_SRAM = 0,
    CE_KEY_FROM_CPU  = 1,
};

enum DATA_MODULE_FROM
{
    CE_DATA_IN_FROM_CPU = 0,
    CE_DATA_IN_FROM_SRAM = 1,
};

/*! @enum HDCP_KEY_SELECT
 *   @brief Specify key type which is used for reading the KL debug key.
 */
enum HDCP_KEY_SELECT
{
	CE_KEY_READ=0, //!<Read KL key
	HDCP_KEY_READ=1 //!<Read HDCP key
};

/*! @enum KEY_LEVEL
 *   @brief Define the key level that API will get from KL.
 */
enum KEY_LEVEL
{
	SKIP_LEVEL = 0 ,//!<Internal reserved. 
	ONE_LEVEL,//!<Get an idle key index from 1st, 2nd and 3rd level keys.
	TWO_LEVEL,//!<Get an idle key index from 2nd and 3rd level keys.
	THREE_LEVEL,//!<Get an idle key index from 3rd level keys.

	FIVE_LEVEL = 5, //! <Get an idle key pos from 5th level, support from CAP210 on
};

/*! @struct CE_PVR_KEY_PARAM
  *   @brief Struct used to change the PVR key. Only used in DMX driver when doing playback.
 */
typedef struct CE_PVR_KEY_PARAM
{
	unsigned char *input_addr;//!<Buffer address of the 16 bytes cipher CW
	unsigned int second_pos;  //!<Specify the target key index.
	unsigned int first_pos;   //!<Specify the source key index.
}CE_PVR_KEY_PARAM, *pCE_PVR_KEY_PARAM;

/*! @struct OTP_PARAM
 *   @brief Struct used to load physical OTP root key to KL KEY_0_X.
 */
typedef struct OTP_PARAM
{
	unsigned char otp_addr;//!<OTP root key physical address.
	enum CE_OTP_KEY_SEL otp_key_pos;   //!<KL root key index, KEY_0_X.
}OTP_PARAM, *pOTP_PARAM;

/*! @struct DATA_PARAM
 *  @brief Struct used to specify the data block information.
 */
typedef struct DATA_PARAM
{
	unsigned int crypt_data[4] ; //!<Input data buffer.
	unsigned int data_len ;  //!<Input data length.
}DATA_PARAM, *pDATA_PARAM;

/*! @struct DES_PARAM
 *  @brief Struct used to specify the cryption mode, algorithm and result location(TDES only).
 */
typedef struct DES_PARAM
{
	enum CE_CRYPT_SELECT crypt_mode;//!<Encryption or decryption selection.
	enum CE_MODULE_SELECT aes_or_des;//!<AES or TDES algorithm selection.
	unsigned char des_low_or_high;//!<Select TDES result location in higher 8bytes or lower 8bytes.
}DES_PARAM, *pDES_PARAM;

/*! @struct CE_KEY_PARAM
 *  @brief Define source key index, target key index and KL SRAM operation mode.
 */
typedef struct CE_KEY_PARAM
{
	enum CE_KEY first_key_pos;//!<Source key index.
	enum CE_CRYPT_TARGET second_key_pos;//!<Target key index.
	enum HDCP_DECRYPT_MODE hdcp_mode ;//!<KL SRAM operation mode.

	/*introduced for CAP210 chipset*/
	int use_new_mng_style;
	int kl_index_sel; //! < CE_SEL_KL_0~4, CE_SEL_ETSI_0~4
	int gen_level; //! <generating which level key
	int cw_pos; //! <the cw position. this pos should be get free from driver. when generating level 5 need this pos parameter.
}CE_KEY_PARAM, *pCE_KEY_PARAM;

/*! @struct CE_DEBUG_KEY_INFO
 *   @brief Reserved.
 */
typedef struct CE_DEBUG_KEY_INFO
{
	enum HDCP_KEY_SELECT sel; //!<Read HDCP or not HDCP.
	unsigned int buffer[4]; //!<Buffer to store the debug key.
	unsigned int len;  //!<Length in byte, equal to algorithm block length, AES-16, TDES-8.

	/*introduced for CAP210 chipset*/
    int kl_index_sel; //! < CE_SEL_KL_0~4, CE_SEL_ETSI_0~4
    int pos;
}CE_DEBUG_KEY_INFO, *pCE_DEBUG_KEY_INFO;

/*! @struct CE_DATA_INFO
 *   @brief Struct used for KL to generate the single level key.
 */
typedef struct CE_DATA_INFO
{
	OTP_PARAM otp_info;//!<Load physical OTP root key to KL KEY_0_X.
	DATA_PARAM data_info;//!<Specify the data block information.
	DES_PARAM des_aes_info;//!<Specify the cryption mode, algorithm and result location(TDES only).
	CE_KEY_PARAM key_info; //!<Specify the source key index, target key index and KL SRAM operation mode.
}CE_DATA_INFO, *pCE_DATA_INFO;

/*! @struct CE_POS_STS_PARAM
 *   @brief Struct used to get the status of the specified key index.
 */
typedef struct CE_POS_STS_PARAM
{
	unsigned int pos; //!<Specify the key index.
	unsigned int status; //!<Returned status(busy or idle).
}CE_POS_STS_PARAM, *pCE_POS_STS_PARAM;

/*! @struct CE_FOUND_FREE_POS_PARAM
 *   @brief Struct used to get the idle key index from KL key table.
 */
typedef struct CE_FOUND_FREE_POS_PARAM
{
	unsigned int pos; //!<Idle index returned from key table.
	enum KEY_LEVEL ce_key_level; //!<Specify the initial key level of KL.
	unsigned char number; //!<Specify the key number that caller wants to get, which is usually 1 or 2, with default of 1.
	enum CE_OTP_KEY_SEL root;//!<Specify root key index, and driver will return relevant idle key index.
	/*!<This parameter is valid on M3515B, M3823 and M3733 only.
	*/
}CE_FOUND_FREE_POS_PARAM, *pCE_FOUND_FREE_POS_PARAM;

/*! @struct CE_DECW_KEY_PARAM
 *   @brief Struct used to send the decw key for VSC dedicated key ladder.
 */
typedef struct CE_DECW_KEY_PARAM
{
	char en_key[16]; //!<Specify the encrypted key.
	CE_DATA_INFO ce_data_info; //!<Specify the key ladder generate key information.
}CE_DECW_KEY_PARAM, *pCE_DECW_KEY_PARAM;

/*! @struct CE_REC_KEY_PARAM
 *   @brief Struct used to send the encrypted key for pvr record on VSC dedicated key ladder.
 */
typedef struct CE_REC_KEY_PARAM
{
	char en_key[16]; //!<specify the encrypted key for pvr.
	unsigned int pos; //!<Specify the key index.
}CE_REC_KEY_PARAM, *pCE_REC_KEY_PARAM;

/*! @struct CE_UK_KEY_PARAM
 *   @brief Struct used to send the encrypted key for UK on VSC dedicated key ladder.
 */
typedef struct CE_UK_KEY_PARAM
{
	char en_key[48]; //!<specify the encrypted key for UK.
	unsigned int pos; //!<Specify the key index.
}CE_UK_KEY_PARAM, *pCE_UK_KEY_PARAM;



#define CE_PARITY_EVEN (1<<0)
//!< Only used for #CE_SELECT_DES, generate to even position.
#define CE_PARITY_ODD (1<<1)
//!< Only used for #CE_SELECT_DES, generate to odd position.
#define CE_PARITY_EVEN_ODD (CE_PARITY_EVEN | CE_PARITY_ODD)
//!< Only used for #CE_SELECT_DES, generate both the even and odd key.



/*! @struct CE_NLEVEL_PARAM.
 *   @brief Define the parameters to generate all the levels in one call to ensure the thread-safety.
 *
 * 	Max. 5 levels.
 */
typedef struct ce_generate_nlevel
{
	unsigned int kl_index;
	/*!<Specify the KL index.  #ALI_KL_0  ~ #ALI_KL_3.
	or the ETSI KL index, #ETSI_KL_0  ~ #ETSI_KL_3.
	*/
	unsigned int otp_addr;
	/*!<Specify the root otp key address which will be used for this KeyLadder. 
		#OTP_ADDESS_1  ~ #OTP_ADDESS_6, or 0.
		If otp_addr is 0, driver will not load the otp root key.
	*/
	int algo;
	/*!<Specify the KeyLadder algorithm.
		#CE_SELECT_AES or #CE_SELECT_DES
	*/
	int crypto_mode;
	/*!<#CE_IS_DECRYPT or #CE_IS_ENCRYPT
	*/
	unsigned int pos;
	/*!<Specify the target key position.
	*/
	unsigned char protecting_key[64];
	/*!<Buffer for the protecting keys.
	*/
	unsigned int protecting_key_num; 
	/*!<Number of the protecting keys, 0, 1, 2 or 4. (Shall equal to the KL level subtract 1).
	*/
	unsigned char content_key[16];
	/*!<If the algo is #CE_SELECT_DES, user needs to copy the 64-bit even key to
		 &content_key[8] and copy the 64-bit odd key to &content_key[0].
		If the algo is #CE_SELECT_AES, copy the 128-bit key to &content_key[0].
	*/
	int parity;
	/*!<Because one key position is 128-bit width and the #CE_SELECT_DES 
		block length is 64-bit, so when using #CE_SELECT_DES, user needs to
		specify the target position within this key. Valid values:
		#CE_PARITY_EVEN, #CE_PARITY_ODD or #CE_PARITY_EVEN_ODD.

		When using #CE_SELECT_DES to generate key for DSC AES, TDES or
		CSA3 (algo's key length is 128-bit), the 'parity' must be set to 
		#CE_PARITY_EVEN_ODD and user shall set 128-bit content key to the
		 'content_key' buffer.

		When using #CE_SELECT_AES, this parameter can be ignored.
	*/
} CE_NLEVEL_PARAM;

/*! @struct CE_CW_DERIVATION.
 *   @brief Define the parameters to derive next stage CW using CW.
 *   CW -> CW (#IO_CRYPT_CW_DERIVE_CW)
 */
typedef struct ce_cw_derivation
{
	/**
	 * Derivation algorithm, #CE_SELECT_AES
	 * or #CE_SELECT_XOR.
	 */
	int algo;
	/**
	 * #CE_IS_DECRYPT or #CE_IS_ENCRYPT
	 */
	int crypto_mode;
	/**
	 * Specify the key from where will be used for derivation. 
	 * KL SRAM (#CE_KEY_FROM_SRAM) or buffer 
	 * in DRAM (#CE_KEY_FROM_CPU).
	 */
	int key_src;
	/**
	 * Key is from KL SRAM or data buffer.
	 * key.pos is associated with the KL key SRAM,
	 * key.buf is the buffer contains the key.
	 */
	union {
		int pos;
		unsigned char buf[16];
	} key;
	/**
	 * Specify the data from where will be decrypted. 
	 * KL SRAM (#CE_DATA_IN_FROM_SRAM) or buffer 
	 * in DRAM (#CE_DATA_IN_FROM_CPU).
	 */
	int data_src;
	/**
	 * Data is from KL SRAM or data buffer.
	 * data.pos is associated with the KL SRAM
	 *
	 * data.buf is the buffer for ECW.
	 */
	union {
		int pos;
		unsigned char buf[16];
	} data;
	/**
	 * Target KL SRAM pos.
	 */
	int target_pos;
} CE_CW_DERIVATION;

/*! @struct CE_ETSI_CHALLENGE.
 *   @brief Define the parameters to perform the ETSI challenge.
 *   (#IO_CRYPT_ETSI_CHALLENGE)
 */
typedef struct ce_etsi_challenge
{
	int kl_index;
	/*!<Specify the ETSI KL index. 
	 * #ETSI_KL_0  ~ #ETSI_KL_4.
	*/
	int otp_addr;
	/*!<Specify the root otp key address which will be used for this KeyLadder. 
		#OTP_ADDESS_1  ~ #OTP_ADDESS_6, or 0.
		If otp_addr is 0, driver will not load the otp root key.
	*/
	int algo;
	/*!<
	 * Challenge algorithm, #CE_SELECT_AES
	 * or #CE_SELECT_TDES.
	 */
	unsigned char ek2[16];
	/*!<
	 * ek3(k2)
	 */
	unsigned char nonce[16];
	/*!<
	 * nonce
	 */
	unsigned char da_nonce[16];
	/*!<
	 * Da(Nonce), output parameter.
	 */
} CE_ETSI_CHALLENGE;


/*! @struct CE_DEVICE
 *   @brief KL device struct reserved on HLD layer.
 */
typedef struct CE_DEVICE
{
	struct CE_DEVICE *next;
	int type;
	char name[16];

	void *pCePriv;
	unsigned int base_addr;
	unsigned int interrupt_id;

	unsigned short semaphore_id;
	unsigned short semaphore_id2;

	void (*attach)(void);
	void (*detach)( struct CE_DEVICE *);
	int (*open)( struct CE_DEVICE *);
	int (*close)( struct CE_DEVICE *);
	int (*ioctl)(struct CE_DEVICE *,unsigned int ,unsigned int );
	int (*key_generate)(struct CE_DEVICE *,pCE_DATA_INFO );
	int (*key_load)(struct CE_DEVICE *,pOTP_PARAM);	
	int fd;
}CE_DEVICE, *pCE_DEVICE;


enum ce_funcs {
	ALIRPC_RPC_ce_api_attach = 0,
	ALIRPC_RPC_ce_api_detach,
	ALIRPC_RPC_ce_ioctl,
	ALIRPC_RPC_ce_key_generate,
	ALIRPC_RPC_ce_key_load,
	ALIRPC_RPC_ce_generate_cw_key,
	ALIRPC_RPC_ce_generate_single_level_key,
};

/*!
@}
*/

#endif 

