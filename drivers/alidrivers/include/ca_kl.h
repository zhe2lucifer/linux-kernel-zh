#ifndef __CA_KL_H__
#define __CA_KL_H__

/*!@addtogroup KeyLadder
 *  @{
    @~
 */


#include <linux/ioctl.h>

#define KL_DRV_NAME "ca_kl"

#define KL_DEV_BASENAME "kl/"

#define KL_DEV_BASE 0xc0
/*!< KL ioctl cmd base.*/

#define KL_LEVEL_MAX 5
/*!< Number of level of the key ladder.*/

#define KL_KEY_SIZE_MAX 16
/*!< KL maximum key size*/

#define KL_HDCP_PKEY_NUM 18
/*!< Number of hdcp key.*/



#define KL_CK_KEY64 (0)
	/*!< Define Key Ladder content key size. 64 bits.*/
#define KL_CK_KEY128 (1)
	/*!< Define Key Ladder content key size. 128 bits.*/

#define KL_ENCRYPT (0)
	/*!< Define CA Key Ladder operation mode. Indicates the mode is encryption.*/
#define KL_DECRYPT (1)
	/*!< Define CA Key Ladder operation mode. Indicates the mode is decryption.*/

#define KL_ALGO_TDES (0)
	/*!< Define the KL Crypto algorithm. Indicates the algorithm is TDES.*/
#define KL_ALGO_AES (1)
	/*!< Define the KL Crypto algorithm. Indicates the algorithm is AES.*/
#define KL_ALGO_XOR (2)
	/*!< Define the KL Crypto algorithm. Indicates the algorithm is XOR.*/

#define KL_KEY_HW (0)
	/*!< Define the key source for KL derivation.
	Indicates the key is from HW KL SRAM.*/
#define KL_KEY_SW (1)
	/*!< Define the key source for KL derivation.
	Indicates the key is from SW buffer.*/

#define KL_DATA_SW (0)
	/*!< Define the data source for KL derivation.
	Indicates the data is from SW buffer.*/
#define KL_DATA_HW (1)
	/*!< Define the data source for KL derivation.
	Indicates the data is from HW KL SRAM.*/

#define KL_CK_PARITY_ODD_EVEN (0)
	/*!< Define the KL key parity to run. Parity not indicated. Auto parity*/
#define KL_CK_PARITY_ODD (1)
	/*!< Define the KL key parity to run. Parity odd.*/
#define KL_CK_PARITY_EVEN (2)
	/*!< Define the KL key parity to run. Parity even.*/

 /*! @struct kl_config_key
 *   @brief Parameters for configuring a key.
 */
struct kl_config_key {
	int level;
	/*!< Number of key derivation to process (1,2, 3 or 5).*/
	int ck_size;
	/*!< Content key size set with #KL_CK_KEY64 or #KL_CK_KEY128.*/
	int ck_parity;
	/*!< Content key parity mode,
	0 for even parity, 1 for odd parity, 2 for auto parity.*/
	int crypt_mode;
	/*!< Operation mode set with #KL_ENCRYPT or #KL_DECRYPT.*/
	int algo;
	/*!< Algorithm set with #KL_ALGO_TDES, #KL_ALGO_AES or #KL_ALGO_XOR.*/
};

/*! @struct kl_gen_key
 *   @brief parameters for running the key ladder.
 */
struct kl_gen_key {
	int run_parity;
	/*!<
	Parity to run set with #KL_CK_PARITY_ODD, #KL_CK_PARITY_EVEN
	or #KL_CK_PARITY_ODD_EVEN.

	This value is valid only if parity is set with #KL_CONFIG_KEY ioctl.
	*/
	char key_odd[KL_KEY_SIZE_MAX];
	/*!<
	Array containing odd content key, this key is used when parity is not
	set with #KL_CONFIG_KEY ioctl.
	*/
	char key_even[KL_KEY_SIZE_MAX];
	/*!< Array containing even content key.*/
	char pk[KL_LEVEL_MAX-1][KL_KEY_SIZE_MAX];
	/*!< Multiple arrays containing protecting keys.*/
};

/*! @struct kl_gen_hdcp_key
 *   @brief parameters for generate hdcp key.
 */
struct kl_gen_hdcp_key {
	char hdcp_pk[KL_HDCP_PKEY_NUM][KL_KEY_SIZE_MAX];
	/*!< Multiple arrays containing protecting keys for hdcp.*/
};


/*! @struct kl_cw_derivation
 *   @brief Derive the next stage element (CW, key or data),
 *	only support #KL_ALGO_AES or #KL_ALGO_XOR.
 */
struct kl_cw_derivation {
	int key_src;
	/*!<
	 * Specify the key from where will be used for derivation.
	 * KL SRAM (#KL_KEY_HW) or buffer
	 * in DRAM (#KL_KEY_SW).
	 */
	union {
		struct {
			int fd;
			int parity;
		};
		unsigned char buf[16];
	} key;
	/*!<
	 * Key is from KL SRAM or data buffer.
	 * key.fd contains the key for derivation.
	 * key.parity is KL_CK_PARITY_ODD or
	 * #KL_CK_PARITY_EVEN. only valid when this fd 
	 * has two SRAMs (even + odd).
	 *
	 * key.buf is the buffer contains the key.
	 */
	int data_src;
	/*!<
	 * Specify the data from where will be decrypted.
	 * KL SRAM (#KL_DATA_HW) or buffer
	 * in DRAM (#KL_DATA_SW).
	 */
	union {
		struct {
			int fd;
			int parity;
		};
		unsigned char buf[16];
	} data;
	/*!<
	 * Data is from KL fd or data buffer.
	 * data.fd is associated with the KL SRAM,
	 * data.parity is KL_CK_PARITY_ODD or
	 * #KL_CK_PARITY_EVEN. only valid when this fd 
	 * has two SRAMs (even + odd).

	 * data.buf is the buffer for ECW.
	 */
	int target_fd;
	/*!<
	 * Target fd.
	 * One target_fd is associated with one set of
	 * even/odd KL SRAM.
	 */
	int target_parity;
	/*!<
	 * Target parity, #KL_CK_PARITY_ODD or #KL_CK_PARITY_EVEN.
	 */
};

/*! @struct kl_etsi_challenge.
 *   @brief Define the parameters to perform the ETSI challenge.
 *   (#KL_ETSI_CHALLENGE)
 */
struct kl_etsi_challenge {
	int algo;
	/*!<
	 * Challenge algorithm, #KL_ALGO_AES
	 * or #KL_ALGO_TDES.
	 */
	unsigned char ek2[KL_KEY_SIZE_MAX];
	/*!<
	 * ek3(k2)
	 */
	unsigned char nonce[KL_KEY_SIZE_MAX];
	/*!<
	 * nonce
	 */
	unsigned char da_nonce[KL_KEY_SIZE_MAX];
	/*!<
	 * Da(Nonce), output.
	 */
} ;

/* Used for KL ioctl
*/
#define KL_CONFIG_KEY      _IOW(KL_DEV_BASE, 1, struct kl_config_key)
/*!<
    This command allocates all the resources in the key ladder
    for running the defined number of levels.

    int ioctl(dev, #KL_CONFIG_KEY, struct *kl_config_key);

    These resources are released when closing the device.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define KL_GEN_KEY         _IOW(KL_DEV_BASE, 2, struct kl_gen_key)
/*!< This command runs the requested level of the key ladder.

    int ioctl(dev, #KL_GEN_KEY, struct *kl_gen_key);

    The generated keys are hidden.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define KL_GEN_HDCP_KEY         _IOW(KL_DEV_BASE, 3, struct kl_gen_hdcp_key)
/*!< This command generate the hdcp key.

    int ioctl(dev, #KL_GEN_HDCP_KEY, struct *kl_gen_hdcp_key);

    The generated keys are stored into the HDCP SRAM .

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define KL_DERIVE_CW         _IOW(KL_DEV_BASE, 4, struct kl_cw_derivation)
/*!< This command derives the next stage element.

    int ioctl(dev, #KL_DERIVE_CW, struct *kl_cw_derivation);

    The derived CW is stored into the secret KL SRAM.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define KL_ETSI_CHALLENGE         _IOW(KL_DEV_BASE, 5, struct kl_etsi_challenge)
/*!< This command performs the challenge/response on ETSI KLs.
	Only support 3-stage ETSI KL and the ETSI KL crypto_mode is
	enforced to #KL_DECRYPT.

    int ioctl(dev, #KL_ETSI_CHALLENGE, struct *kl_etsi_challenge);

    User does not need to call #KL_CONFIG_KEY before using this ioctl.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

/*!
@}
*/

/******************************************************************************/
/******************DO NOT CARE. JUST FOR DEBUG PURPOSE****************************/
/******************************************************************************/
#define KL_LEGACY_CMD_OFF 0x10

/*! @struct kl_dbg_key
 *   @brief parameters for getting back the key ladder sram KEY_0_0.
 */
struct kl_dbg_key {
    unsigned int buffer[4];
    /*!<
	Array containing KEY_0_0 key data.
    */
};

#define KL_DBG_KEY _IOW(KL_DEV_BASE, KL_LEGACY_CMD_OFF+0, struct kl_dbg_key)
/*!< This command if for Nagra Sed Extended API usage.

    It requires to get back the KL SRAM<KEY_0_0> value from MAIN side.

    int ioctl(dev, #KL_DBG_KEY, struct *kl_dbg_key);

    The KEY_0_0 data will be returned back to user land if the OTP bit is allowed.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/
/*!
@}
*/

#endif
