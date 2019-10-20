#ifndef _ADF_CERT_H_
#define _ADF_CERT_H_

#ifdef __cplusplus
extern "C"
{
#endif


#define CERT_AKL_DATA_SIZE  (32)
/*!< CERT AKL cmd size in bytes (32 bytes).*/

#define CERT_IO_CMD(cmd)  (cmd & 0xFF) 
/*!< Mask the cmd to 8 bits, only 8 bits valid for CERT see ioctl cmd.*/

/*!
 *	 @brief Define the return-value for CERT 
*/
#define CERT_NO_ERROR (0)
#define CERT_ERROR (0x1000)
#define CERT_ERROR_NODEV (0x1001)
#define CERT_ERROR_INVALID_PARAM (0x1002)
#define CERT_ERROR_SAVE_KEY (0x1004)
#define CERT_ERROR_TIMEOUT (0x1008)
#define CERT_ERROR_ALWAYS_BUSY (0x1010)
#define CERT_ERROR_PIDFULL (0x1020)
#define CERT_ERROR_CHANFULL (0x1040)
#define CERT_ERROR_INVALID_CHAN (0x1080)
#define CERT_ERROR_SYNC_BYTE (0x1100)
#define CERT_ERROR_NOT_SUPPORTED (0x1200)
#define CERT_ERROR_NOT_ALLOWED (0x1400)
#define CERT_ERROR_EXCHANGE_STATUS (0x1800)

/*!
 *	 @brief Define the RPC func list for CERT 
*/
enum HLD_CERT_FUNC{
	/* AKL */
	CERT_RPC_AKL_ATTACH,
	CERT_RPC_AKL_DETACH,
	CERT_RPC_AKL_OPEN,
	CERT_RPC_AKL_CLOSE,
	CERT_RPC_AKL_EXCHANGE,
	CERT_RPC_AKL_SAVEKEY,
	CERT_RPC_AKL_ACK,
	CERT_RPC_AKL_ALLOC,
	CERT_RPC_AKL_FREE,

	/* ASA */
	CERT_RPC_ASA_ATTACH,
	CERT_RPC_ASA_DETACH,
	CERT_RPC_ASA_SETFMT,
	CERT_RPC_ASA_OPEN,
	CERT_RPC_ASA_CLOSE,
	CERT_RPC_ASA_ADDPID,
	CERT_RPC_ASA_DELPID,
	CERT_RPC_ASA_DECRYPT,
};

/*!
     @brief Define basic node info for decryption
*/
#define CERT_ASA_NR_NODES (1024)

/*! @struct cert_asa_node
 *	 @brief Define elements for one ASA node.
*/
struct cert_asa_node {
	unsigned long input; /* pointer to the input data */
	unsigned long output; /* pointer to the output data, can be same as input */
	unsigned int size; /* TS data size in bytes */
};

/*!
     @brief Define packet format
*/
#define CERT_ASA_TS188		(1)
	/*!< DVB-MPEG2 TS packet, 188 Bytes */
#define CERT_ASA_TS188_LTSID (2)
	/*!< DVB-MPEG2 TS packet using LTSID replace Sync Byte, 188 Bytes */
#define CERT_ASA_TS200		(3)
	/*!< CCIF2.0 TS packet, 200 Bytes */

/*!
 * @brief Define the AKL key parity for saving key to the corresponding key position.
*/
#define CERT_AKL_ODD_PARITY (1)
/*!< copy the key to even position.*/
#define CERT_AKL_EVEN_PARITY (2)
/*!< copy the key to odd position.*/

/*!
 * @brief Define the AKL key usage.
*/
#define CERT_AKL_FOR_AES (0)
/*!< Specify that the AKL key is for AES.*/
#define CERT_AKL_FOR_TDES (1)
/*!< Specify that the AKL key is for TDES.*/
#define CERT_AKL_FOR_ASA	(8)
/*!< Specify that the AKL key is for ASA.*/
#define CERT_AKL_FOR_CSA3 (9)
/*!< Specify that the AKL key is for CSA3.*/
#define CERT_AKL_FOR_CSA2 (10)
/*!< Specify that the AKL key is for CSA2/CSA1.1.*/

/*! @struct cert_akl_savekey
 *	 @brief Define the algorithm and parity information for storing the AKL key.
*/
struct cert_akl_savekey
{
	int pos;
	/*!< Specify the destination position which will be
	used for saving this key.*/
	int algo;
	/*!< Specify which crypto algorithm will use this AKL key,
	algorithms are #CERT_AKL_FOR_AES, #CERT_AKL_FOR_TDES,
	#CERT_AKL_FOR_ASA, #CERT_AKL_FOR_CSA3 and #CERT_AKL_FOR_CSA2.
	*/
	int parity;
	/*!< Specify the key parity #CERT_AKL_EVEN_PARITY or
	#CERT_AKL_ODD_PARITY to be saved to.*/
};
 

/*! @struct cert_akl_cmd
 *	 @brief Define the parameters for exchanging data with AKL. @~
 */
struct cert_akl_cmd
{
	unsigned char data_in[CERT_AKL_DATA_SIZE];
	/*!< Input command to exchange with AKL.*/ 
	unsigned char data_out[CERT_AKL_DATA_SIZE];
	/*!< Output data of AKL.*/ 
	unsigned char status[4];
	/*!< Output status of AKL.*/ 
	unsigned char opcodes[4];
	/*!< Opcodes of AKL.*/ 
	unsigned int timeout;
	/*!< Timeout type, default is 0.*/ 
};

/*! @struct cert_asa_pid 
 *	 @brief Define the parameters for adding the ASA PID and key information. @~
 */
struct cert_asa_pid
{
	int pos;
	/*!< Specify which key pos will be used for this PID(or PIDs). */ 
	unsigned char ltsid;
	/*!< Specify the LTSID to be added/removed, only valid when 
	TS200 or TS188 with LTSID mode, normal TS188 doesn't need this parameter. */ 
	unsigned char tsc;
	/*!< Specify the Transport Scramble Control value after descrambling, 0~3*/ 
	unsigned short pid; 
	/*!< Specify the PID value.
	*/ 
};


#ifdef __cplusplus
}
#endif

#endif

