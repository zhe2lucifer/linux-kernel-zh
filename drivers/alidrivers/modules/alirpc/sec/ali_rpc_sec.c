//for printk
#include <linux/kernel.h>
#include <rpc_hld/ali_rpc_sec.h>
#include <linux/ali_rpc.h>

//M1
static UINT32 bc_set_dvb_ca_pdesc[] =
{	//desc of pointer para
	1, DESC_STATIC_STRU(0, 32),
	1, DESC_P_PARA(0, 3, 0),
	//desc of pointer ret
	0,
	0,
};

int rpc_m2s_bc_set_dvb_ca_descriptor(unsigned char bServiceIdx,\
							unsigned char ca_device,\
							unsigned char ca_mode,\
							unsigned char iv_value[32])
{
	UINT32 *desc = NULL;
	//printk("[%s %s %d]\n",__FILE__,__func__,__LINE__);
	desc = bc_set_dvb_ca_pdesc;
	jump_to_func(NULL, ali_rpc_call, bServiceIdx, (HLD_SEC_MODULE<<24)|(4<<16)|FUNC_BC_SET_DVB_CA, desc);
}

// M2
static UINT32 bc_set_dvr_pid_pdesc[] =
{	//desc of pointer para
	1, DESC_STATIC_STRU(0, 32*sizeof(short)),
	1, DESC_P_PARA(0, 2, 0),
	//desc of pointer ret
	0,
	0,
};

int rpc_m2s_bc_set_dvr_pid(unsigned char bServiceIdx, \
							unsigned short pid_number, \
							unsigned short pid_table[32])
{
	UINT32 *desc = NULL;
	//printk("[%s %s %d]\n",__FILE__,__func__,__LINE__);
	desc = bc_set_dvr_pid_pdesc;
	jump_to_func(NULL, ali_rpc_call, bServiceIdx, (HLD_SEC_MODULE<<24)|(3<<16)|FUNC_BC_SET_DVR_PID, desc);
}

//M3
static UINT32 find_dsc_status_by_serviceIdx_pdesc[] =
{	//desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(S_DSC_HDL)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};

int rpc_m2s_find_dsc_status_by_serviceIdx(unsigned char bServiceIdx, S_DSC_HDL *dsc_status)
{
	UINT32 *desc = NULL;
	//printk("[%s %s %d]\n",__FILE__,__func__,__LINE__);
	desc = find_dsc_status_by_serviceIdx_pdesc;
	jump_to_func(NULL, ali_rpc_call, bServiceIdx, (HLD_SEC_MODULE<<24)|(2<<16)|FUNC_GET_DSC_STATUS, desc);

}

static UINT32 sec_test_pdesc[] =
{
	//desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(SEC_TEST_PARAM)),
	1, DESC_P_PARA(0, 1, 0),
	//desc of pointer ret
	0,
	0,
};

int rpc_m2s_sec_test(unsigned char bServiceIdx, SEC_TEST_PARAM *sec_test_param)
{
	UINT32 *desc = NULL;
	//printk("[%s %s %d]\n",__FILE__,__func__,__LINE__);
	desc = sec_test_pdesc;
	jump_to_func(NULL, ali_rpc_call, bServiceIdx, (HLD_SEC_MODULE<<24)|(2<<16)|FUNC_SEC_TEST, desc);

}
