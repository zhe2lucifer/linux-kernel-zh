#ifndef __ALI_TAC_H_
#define __ALI_TAC_H_

struct tac_rpc_call{
	int clientId;         //[in].  main call see 那个callback函数
	int contextId;		   //[in].  main call see 那个callback函数
	int dataSize_call;     //[in].  main call see 的raw data 数据长度 
	int dataSize_return;  //[out]. main call see,  see rpc返回的数据长度
};


struct tac_rpc_ret{
	int clientId;         //see call main 那个callback函数
	int contextId;		   // see call main 那个callback函数
	int dataSize_ret;     //see call main，main返回的数据长度 
};

struct tac_see_rpc_call {
	int clientId ;         //[in].  see call main 那个callback函数
	int contextId ;		   // see call main 那个callback函数
	int dataSize_call;     //[in].  see call main 的raw data 数据长度 
};

struct tac_ota_mem_info{
       unsigned long addr;
       unsigned long size;
};

#define TAC_MAGIC (0x13)
#define IO_TAC_GET_KUMSGQ	 _IOR(TAC_MAGIC, 0, unsigned int)
#define IO_TAC_GET_SHM_SIZE  _IOR(TAC_MAGIC, 1, unsigned int)
#define IO_TAC_RPC_CALL 	 _IOWR(TAC_MAGIC, 2, struct tac_rpc_call)
#define IO_TAC_RPC_RET		 _IOW(TAC_MAGIC, 3, struct tac_rpc_ret)
#define IO_TAC_RPC_INIT		 _IOR(TAC_MAGIC, 4, unsigned int)
#define IO_TAC_START_TEE_CLIENT	 _IOW(TAC_MAGIC, 5, unsigned int)
#define IO_TAC_GET_ALL_SHM_SIZE  _IOR(TAC_MAGIC, 6, unsigned int)
#define IO_TAC_RPC_DMDA_CALL 	 _IOWR(TAC_MAGIC, 7, struct tac_rpc_call)  //dynamic memory direct access
#define IO_TAC_RPC_DMDA_INIT		 _IOR(TAC_MAGIC, 8, unsigned int)
#define IO_TAC_GET_OTA_MEM_INFO	 _IOR(TAC_MAGIC, 9, unsigned int)

int tac_rpc_M2S(UINT32 clientId, UINT32 contextId,UINT32 main_address_uncache,
				    UINT32 data_len_call, void* msg_ret, UINT32 *data_len_ret);

int tac_rpc_DMDA_M2S(UINT32 clientId, UINT32 contextId,UINT32 main_address_uncache,
				    UINT32 data_len_call, void* msg_ret, UINT32 *data_len_ret);

#endif
