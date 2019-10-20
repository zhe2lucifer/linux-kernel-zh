#ifndef __ALI_RPC_TAC_H__
#define __ALI_RPC_TAC_H__

#include "ali_rpc_hld.h"

enum HLD_TAC_FUNC{
	FUNC_TAC_CALL_SEE = 0,
	FUNC_TAC_CALL_INIT = 1,
	FUNC_TAC_START_CLIENT =2,
	FUNC_TAC_CALL_DMDA_SEE = 3,
	FUNC_TAC_CALL_DMDA_INIT = 4,
};


int tac_rpc_M2S(UINT32 clientId, UINT32 contextId,UINT32 main_address_uncache,
				    UINT32 data_len_call, void* msg_ret, UINT32 *data_len_ret);

void tac_rpc_Init(UINT32 address);
void tac_rpc_start_tee_client(UINT32 client_id);

int tac_rpc_DMDA_M2S(UINT32 clientId, UINT32 contextId,UINT32 main_address_uncache,
					UINT32 data_len_call, void* msg_ret, UINT32 *data_len_ret);

void tac_rpc_DMDA_Init(UINT32 address);
#endif
