#ifndef __ALI_RPC_SEC_H__
#define __ALI_RPC_SEC_H__

#include "ali_rpc_hld.h"
#include <alidefinition/adf_dsc.h>
#include <linux/ali_sec.h>

typedef struct
{
    enum WORK_SUB_MODULE dsc_algo;// CSA , AES, TDES ...
    enum WORK_MODE work_mode;// CBC, ECB, CTR ...
    enum KEY_TYPE key_type;
    enum DMA_MODE data_type;// TS, PURE DATA
    enum RESIDUE_BLOCK residue_mode;
    enum PARITY_MODE parity_mode;
    enum CRYPT_SELECT en_de_crypt;
    enum CSA_VERSION csa_version;
    void *sub_dev_hdl;//---------------------------> just pointer value, no memory copy!!
    unsigned long sub_device_id;
    unsigned long stream_id;
    unsigned long key_handle;
    unsigned long key_pos;
    unsigned long key_size;// key size
    unsigned char content_key[32];
    unsigned char iv_ctr[32];
    unsigned short pid_lists[32];
    unsigned long pid_cnt;
    unsigned int rec_block_count;
}S_DSC_HDL,*P_DSC_HDL;


enum HLD_SEC_FUNC{
	FUNC_BC_SET_DVB_CA = 0,
	FUNC_BC_SET_DVR_PID = 1,
	FUNC_GET_DSC_STATUS = 2,
	FUNC_SEC_TEST=3,
};

int rpc_m2s_bc_set_dvb_ca_descriptor(unsigned char bServiceIdx,\
							unsigned char ca_device,\
							unsigned char ca_mode,\
							unsigned char iv_value[32]);

int rpc_m2s_bc_set_dvr_pid(unsigned char bServiceIdx, \
							unsigned short pid_number, \
							unsigned short pid_table[32]);

int rpc_m2s_find_dsc_status_by_serviceIdx(unsigned char bServiceIdx,\
							S_DSC_HDL *dsc_status);

int rpc_m2s_sec_test(unsigned char bServiceIdx, SEC_TEST_PARAM *sec_test_param);

#endif
