/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#include <linux/slab.h>
#include "see_bus.h"
#include "see_bus_internals.h"

#if defined(CONFIG_ALI_RPCNG)
#include <ali_rpcng.h>
#else
#include <linux/ali_rpc.h>
#endif

#if !defined(CONFIG_ALI_RPCNG)
enum HLD_SEEBUS_FUNC
{
   FUNC_SEEBUS_CLIENT_SERVICE_GET = 0,
   FUNC_SEEBUS_SEE_FW_VERSION_GET,
   FUNC_SEEBUS_SEE_FW_TAG_GET,
};

#define SEEBUS_HLD_NPARA(x) ((HLD_SEEBUS_MODULE<<24)|(x<<16))
#endif

#if defined(CONFIG_ALI_RPCNG)
extern int mcomm_ali_modinit(void);
#else
extern int ali_rpc_init(struct platform_device *pdev);
#endif

int init_see_connection(struct platform_device *pdev)
{
#if defined(CONFIG_ALI_RPCNG)
	return mcomm_ali_modinit();
#else
	return ali_rpc_init(pdev);
#endif
}

int get_rpc_bus_id(int rcd)
{
	return 0;
}

struct see_service *open_see_service(struct see_bus *bus, int service_id)
{
	int ret;
	struct see_service *srv;

	srv = kzalloc(sizeof(struct see_service), GFP_KERNEL);
	if (!srv)
		return ERR_PTR(-ENOMEM);

	ret = see_client_service_get(service_id, srv);
	if (ret) {
		dev_err(&bus->pdev->dev, "%s:%d - see client %d get service failed! ret:%d\n",
			__func__, __LINE__, service_id, ret);
	}

	return srv;
}

void close_see_service(struct see_service *srv)
{
	/* ... */
	kfree(srv);
}

int see_call_rpc(struct see_client *clnt, int function_id, ...)
{
	/* ret = __rpc_call(clnt->bus->rcd,
	 *		clnt->service->function_table[function_id], ...) */
	/* ... */
	return 0;
}

int see_call_rpc_completion(struct see_client *clnt, int function_id, ...)
{
	/* ... */
	/* ret = __rpc_call_completion(clnt->bus->rcd,
	 *		clnt->service->function_table[function_id], ...) */
	/* ... */
	return 0;
}

int see_query_rpc_completion(struct see_rpc *rpc)
{
	/* ... */
	return 0;
}

int see_wait_rpc_completion(struct see_rpc *rpc)
{
	/* ... */
	return 0;
}

int see_client_service_get(int service_id, struct see_service *service)
{    
#if defined(CONFIG_ALI_RPCNG)

	RPC_PARAM_CREATE(p1, PARAM_IN, PARAM_INT32, sizeof(int), &service_id);
   	RPC_PARAM_CREATE(p2, PARAM_OUT, PARAM_SEE_SERVICE_PARS, sizeof(struct see_service), service);

	return RpcCallCompletion(RPC_see_client_service_get,&p1,&p2,NULL);
#else
	unsigned int desc_see_client_service_get[] = 
	{ 
	     //desc of pointer para
		1, DESC_OUTPUT_STRU(0, sizeof(struct see_service)),
		1, DESC_P_PARA(0, 1, 0), 
	     //desc of pointer ret
	     0,                          
	     0,
	};

	jump_to_func(NULL, ali_rpc_call, service_id, SEEBUS_HLD_NPARA(2)|FUNC_SEEBUS_CLIENT_SERVICE_GET, desc_see_client_service_get);
#endif
}


/*
*get see fw version from see cpu for checking
*/
int see_fw_version_get(void)
{
#if defined(CONFIG_ALI_RPCNG)
	return RpcCallCompletion(RPC_see_fw_version_get, NULL);
#else
    jump_to_func(NULL, ali_rpc_call, NULL, SEEBUS_HLD_NPARA(0)|FUNC_SEEBUS_SEE_FW_VERSION_GET, NULL);
#endif
}
