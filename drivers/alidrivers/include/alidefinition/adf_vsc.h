#ifndef _ADF_CERT_H_
#define _ADF_CERT_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <alidefinition/adf_basic.h>

#define VSC_IO_CMD(cmd)  (cmd & 0xFF) 
/*!< Mask the cmd to 8 bits, only 8 bits valid for VSC see ioctl cmd.*/

/*!
 *	 @brief Define the return-value for VSC 
*/
#define VSC_NO_ERROR (0)
#define VSC_ERROR (0x1000)
#define VSC_ERROR_NODEV (0x1001)
#define VSC_ERROR_INVALID_PARAM (0x1002)
#define VSC_ERROR_SAVE_KEY (0x1004)
#define VSC_ERROR_TIMEOUT (0x1008)
#define VSC_ERROR_ALWAYS_BUSY (0x1010)
#define VSC_ERROR_PIDFULL (0x1020)
#define VSC_ERROR_CHANFULL (0x1040)
#define VSC_ERROR_INVALID_CHAN (0x1080)
#define VSC_ERROR_SYNC_BYTE (0x1100)
#define VSC_ERROR_NOT_SUPPORTED (0x1200)
#define VSC_ERROR_NOT_ALLOWED (0x1400)
#define VSC_ERROR_EXCHANGE_STATUS (0x1800)

#define VSC_DATA_LEN 0x10000
#define MAX_VSC_CMD_PKT_LENGTH 128
#define MAX_VSC_BUFFER_LENGTH 4096
#define LENGTH_SHA_256 32
#define VSC_STO_NUM 2
#define VSC_STO_KEY_SIZE 16
#define VSC_TAG 0x56534300
#define KEY_SIZE 16

enum DATA_DIRECTION {
	VSC_DATA_IN,
	VSC_DATA_OUT
};
/*
typedef struct vsc_transfer_prm {
	unsigned int direction;
	unsigned char bufData[128];
}Vsc_transfer_prm;
*/
/**
 * @brief Dispatch command packet
 */
typedef struct vsc_dispatch_cmd_packet
{
   UINT8 session_id;
   UINT8 pkt_index;
   UINT16 valid_data_length;
   UINT16 total_data_length;
   UINT8 data[MAX_VSC_CMD_PKT_LENGTH];
}VSC_PKT;

typedef struct vsc_store_config
{
    	UINT8 index;
    	UINT8 random_key [VSC_STO_NUM][VSC_STO_KEY_SIZE];
    	UINT8 hash [VSC_STO_NUM][LENGTH_SHA_256];
    	UINT32 tag;
}VSC_STORE_CONFIG;

#ifdef __cplusplus
}
#endif

#endif

