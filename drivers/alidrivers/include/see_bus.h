#include <linux/device.h>
#include <alidefinition/adf_see_bus_service.h>

struct see_bus;

struct see_client {
	char *name;
	int service_id;
	struct see_service *service;
	struct see_bus *bus;
	struct device dev;
	struct list_head clients;
};

struct see_client_driver {
	int (*probe)(struct see_client *);
	int (*remove)(struct see_client *);
	struct device_driver driver;
	int see_min_version;
	/* add other callbacks here, e.g. for RPC call completion and events */
};


extern struct bus_type see_bus_type;

struct see_client *see_client_alloc(char *name, int service_id);
int see_client_register(struct see_client *clnt, struct see_bus *bus);
void see_client_unregister(struct see_client *clnt);

int __see_client_driver_register(struct see_client_driver *clnt,
				struct module *mod);
void see_client_driver_unregister(struct see_client_driver *clnt);

#define see_client_driver_register(CLNT) \
	__see_client_driver_register(CLNT, THIS_MODULE)

#define module_see_client_driver(__see_client_driver) \
	module_driver(__see_client_driver, see_client_driver_register, \
			see_client_driver_unregister)

//SEE FW Version
#define SEE_MIN_VERSION(proto,major,minor,patch)   (((proto) << 24) + ((major) << 16) + ((minor) << 8) + (patch))

#define SEE_VERSION_PROTO(version)               ((version>>24)&0xff)
#define SEE_VERSION_MAJOR(version)               ((version>>16)&0xff)
#define SEE_VERSION_MINOR(version)               ((version>>8)&0xff)
#define SEE_VERSION_PATCH(version)               ((version)&0xff)
#define SEE_VERSION_REST(version)                (version&0xffffff)


struct see_rpc {
	/* ... */
};

int see_call_rpc(struct see_client *clnt, int function_id, ...);
int see_call_rpc_completion(struct see_client *clnt, int function_id, ...);
int see_query_rpc_completion(struct see_rpc *rpc);
int see_wait_rpc_completion(struct see_rpc *rpc);


/*synchronously calls a function of a service*/
#define __rpc_service_call_completion(clnt, func_id, arg...) \
	__rpc_call_completion(NULL, clnt->service->function_table[func_id], ##arg)


/* SBM function prototypes */
