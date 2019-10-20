#ifndef NIM_CXD2856_DEBUG_H
#define NIM_CXD2856_DEBUG_H
#include <linux/platform_device.h>

/*trace when driver probe*/
#define NIM_TRACE pr_info

/** Trace enter */
#define NIM_TRACE_ENTER() \
	do{\
		pr_debug("\n[enter]%s,%d\n",__FUNCTION__,__LINE__);\
		}while(0);
/** Trace return error */
#define NIM_TRACE_RETURN_ERROR(string,result) \
	do{\
		pr_err("\n[error exit] %s,result=%d,%s,%d\n",string,result,__FUNCTION__, __LINE__); \
		return (result);\
		}while(0);

/** Trace return success */
#define NIM_TRACE_RETURN_SUCCESS() \
	do{\
		pr_debug("\n[success exit]%s,%d\n",__FUNCTION__,__LINE__); \
		return (SUCCESS);\
		}while(0);

#endif 
