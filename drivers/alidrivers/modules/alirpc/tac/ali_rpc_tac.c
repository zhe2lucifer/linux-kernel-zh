//for printk
#include <linux/kernel.h>
#include <rpc_hld/ali_rpc_tac.h>
#include <linux/ali_rpc.h>

//M-->S
static UINT32 tac_rpc_call_M2S_pdesc[] =
{	//desc of pointer para
	2, DESC_OUTPUT_STRU(0, 1024 * sizeof(UINT8) * 50),DESC_OUTPUT_STRU(1, sizeof(UINT32)),
	2, DESC_P_PARA(0, 4, 0),DESC_P_PARA(1, 5, 1),
	//desc of pointer ret
	0,
	0,
};

//M-->S
static UINT32 tac_rpc_call_DMDA_M2S_pdesc[] =
{	//desc of pointer para
	2, DESC_OUTPUT_STRU(0, 0),DESC_OUTPUT_STRU(1, sizeof(UINT32)),
	2, DESC_P_PARA(0, 4, 0),DESC_P_PARA(1, 5, 1),
	//desc of pointer ret
	0,
	0,
};

int tac_rpc_M2S(UINT32 clientId, UINT32 contextId,UINT32 main_address_uncache,
				    UINT32 data_len_call, void* msg_ret, UINT32 *data_len_ret)
{
	UINT32 *desc = NULL;
	//printk("[%s %s %d] rpc call tac_rpc_main_call_see.\n",__FILE__,__func__,__LINE__);
	desc = tac_rpc_call_M2S_pdesc;
	jump_to_func(NULL, ali_rpc_call, clientId, (HLD_TAC_MODULE<<24)|(6<<16)|FUNC_TAC_CALL_SEE, desc);
}


void tac_rpc_Init(UINT32 address)
{

	//printk("[%s %s %d] rpc call tac_rpc_Init.\n",__FILE__,__func__,__LINE__);
	jump_to_func(NULL, ali_rpc_call, address, (HLD_TAC_MODULE<<24)|(1<<16)|FUNC_TAC_CALL_INIT, NULL);
}

void tac_rpc_start_tee_client(UINT32 client_id)
{

	//printk("[%s %s %d] rpc call tac_rpc_start_tee_client.\n",__FILE__,__func__,__LINE__);
	jump_to_func(NULL, ali_rpc_call, client_id, (HLD_TAC_MODULE<<24)|(1<<16)|FUNC_TAC_START_CLIENT, NULL);
}

void tac_rpc_DMDA_Init(UINT32 address)
{

	//printk("[%s %s %d] rpc call tac_rpc_DMDA_Init.\n",__FILE__,__func__,__LINE__);
	jump_to_func(NULL, ali_rpc_call, address, (HLD_TAC_MODULE<<24)|(1<<16)|FUNC_TAC_CALL_DMDA_INIT, NULL);
}

int tac_rpc_DMDA_M2S(UINT32 clientId, UINT32 contextId,UINT32 main_address_uncache,
				    UINT32 data_len_call, void* msg_ret, UINT32 *data_len_ret)
{
	UINT32 *desc = NULL;
	//printk("[%s %s %d] rpc call tac_rpc_main_call_see.\n",__FILE__,__func__,__LINE__);
	desc = tac_rpc_call_DMDA_M2S_pdesc;
	jump_to_func(NULL, ali_rpc_call, clientId, (HLD_TAC_MODULE<<24)|(6<<16)|FUNC_TAC_CALL_DMDA_SEE, desc);
}