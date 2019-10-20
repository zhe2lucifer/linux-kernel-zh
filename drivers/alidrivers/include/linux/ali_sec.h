#ifndef __ALI_SEC_H_
#define __ALI_SEC_H_

struct sec_ca_key_attr {
	unsigned int key_handle;
		/*!< Key handle at see for the current key. */
	int kl_sel;
		/*!< KeyLadder key sel for the current key. */
	int key_pos;
		/*!< Key pos for the current key. */
       unsigned char iv_ctr[32];
};

struct sec_ca_attr{
	void *sub_dev_see_hdl;
		/*!< Pointer of the sub device handler at see for the current session. */
	unsigned short stream_id;
		/*!< Stream id of the current session. */
	int crypt_mode;
		/*!< Crypt mode of the current session. */        
      unsigned short sub_dev_id;
        /*sub dev id */
};

typedef struct{
	unsigned char bServiceIdx;//[in]
	unsigned char ca_device;//[in]
	unsigned char ca_mode;//[in]
	unsigned char iv_value[32];//[in]
}CA_ALGO_PARAM;

typedef struct{
	unsigned char bServiceIdx;//[in]
	unsigned short pid_number;//[in]
	unsigned short pid_table[32];//[in]
}PID_INFO;

typedef struct{
	unsigned char bServiceIdx;//[in]
	unsigned int rec_block_count;//[out]
}BC_DSC_STATUS;


typedef struct
{
    unsigned long dsc_algo; //[in] CSA , AES, TDES ...
    unsigned long work_mode;           //[in] CBC, ECB, CTR ...
    unsigned long data_type;          //[in] TS, puredata mode
    unsigned long residue_mode;    //[in] process residue block
    unsigned char en_de_crypt;  //[in] 0:DSC_ENCRYPT or 1:DSC_DECRYPT
    unsigned char root_key_pos;         //[in] User need to configure the root key pos. 1 0xFF is clear key mode, 2 OTP_KEY_0_X is keyladder key mode.
    unsigned char kl_mode;                  //[in] key ladder generation mode (CE_SELECT_DES 0:TDES ECB, CE_SELECT_AES 1:AES ECB)
    unsigned char kl_level;				//[in] kl generated level (only support 5 level)
    unsigned char protecting_key[64];       //[in] [0-15] is first stage key data, [16-31] is second stage key data, [32-47] is third stage key data or clear key.[48-63] for fourth stage
    unsigned char content_key[2][16];        //[in] content_key[0] for even clear key or last even kl key, content_key[1] for odd clear key or last odd kl key
    unsigned char iv_ctr[2][16];               //[in] iv_ctr[0] for even iv, iv_ctr[1] for odd iv.
    unsigned long key_size;         //[in] key size (8 bytes or 16 bytes)
    unsigned char key_pattern;                    //[in] 1:even key , 2:odd key, 3:both even and odd key
    unsigned short pid_num;                   //[in] the pid number
    unsigned short pid_list[32];                   //[in] the pid list     
    unsigned char cmd;                    //[in] 1:start create dsc resource, 2:release dsc resource, 0:update parameters
    unsigned char bServiceIdx;      //[in]
}SEC_TEST_PARAM;

#define SEC_MAGIC (0x23)
#define IO_SEC_SET_BC_CA_ALGO	 _IOW(SEC_MAGIC, 0, CA_ALGO_PARAM) //set CA algorithm to see
#define IO_SEC_SET_BC_DVR_PID    _IOW(SEC_MAGIC, 1, PID_INFO) //set dvr pid information to see
#define IO_SEC_GET_BC_DSC_STATUS _IOWR(SEC_MAGIC, 2, BC_DSC_STATUS) //get DSC STATUS from see
#define IO_SEC_GET_KUMSGQ		 _IOR(SEC_MAGIC, 3, unsigned int) // get kumsgq fd
#define IO_SEC_RPC_RET   		 _IO(SEC_MAGIC, 4) // userspace informs kernel it has finished rpc call.
#define IO_SEC_TEST_DSC              _IOW(SEC_MAGIC, 5, SEC_TEST_PARAM) //set dvr pid information to see

int sec_check_is_sec_fd(int sec_fd);
int sec_get_session_attr(int sec_fd, struct sec_ca_attr *p_ca_attr);
int sec_get_key_attr(int sec_fd, struct sec_ca_key_attr *p_ca_key_attr);

#endif
