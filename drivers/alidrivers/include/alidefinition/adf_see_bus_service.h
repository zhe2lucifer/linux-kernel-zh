#ifndef __SEE_BUS_SERVICE_H
#define __SEE_BUS_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

//#define SEE_BUS_INTERNAL_DEBUG  1

//struct see_service_desc is used to describe a see service item
#define FUNCTION_NAME_MAX_CHARS    16
#define FUNCTION_TABLE_MAX_NUMBER  48

typedef struct _func_name{
   unsigned char name[FUNCTION_NAME_MAX_CHARS];
}func_name;

struct see_service {
	unsigned int function_table[FUNCTION_TABLE_MAX_NUMBER];
	func_name function_name_table[FUNCTION_TABLE_MAX_NUMBER];
	/* ... */
};


#define COPY_SEE_SERVICE(service, func_index, func)                         \
do                                                                          \
{                                                                           \
	service->function_table[func_index] = HASH_STR(func);						 \
	strncpy(service->function_name_table[func_index].name, #func, (sizeof(#func) > (FUNCTION_NAME_MAX_CHARS-1))? (FUNCTION_NAME_MAX_CHARS-1) : sizeof(#func));   \
}while(0)

#ifdef __cplusplus
}
#endif

#endif  /*__SEE_BUS_SERVICE_H*/
