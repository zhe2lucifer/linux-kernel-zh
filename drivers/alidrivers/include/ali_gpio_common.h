#ifndef _ALI_GPIO_COMMON_H_
#define _ALI_GPIO_COMMON_H_


#include <ali_basic_common.h>

typedef struct gpio_info {
    unsigned int gpio;
    unsigned int status;
}gpio_info;




#define GPIO_MAGIC                                      'G'

#define GPIO_REQUEST                                  _IOR(GPIO_MAGIC, 0, int)
#define GPIO_RELEASE                                  _IOR(GPIO_MAGIC, 1, int)
#define GPIO_GET_STATUS                               _IOR(GPIO_MAGIC, 2, struct gpio_info)
#define GPIO_SET_STATUS                               _IOW(GPIO_MAGIC, 3, struct gpio_info)
#define GPIO_SET_DIR                                  _IOW(GPIO_MAGIC, 4, struct gpio_info)

#define GPIO_SET_IRQ_ENABLE                           _IOW(GPIO_MAGIC, 5, int)

#define GPIO_SET_IRQ_DISENABLE                        _IOW(GPIO_MAGIC, 6, int)

#define GPIO_GET_IRQ_ENABLE                           _IOWR(GPIO_MAGIC, 7, struct gpio_info)

#define GPIO_IRQ_CLEAR                                _IOW(GPIO_MAGIC, 8, int)

#define GPIO_SET_IRQ_TYPE                             _IOW(GPIO_MAGIC, 9, struct gpio_info)

#define GPIO_GET_IRQ_STATUS                           _IOWR(GPIO_MAGIC, 10, struct gpio_info)

#define GPIO_SET_TRANSPORT_ID                         _IOW(GPIO_MAGIC, 15, int)                              

#define GPIO_GET_KUMSGQ                         	  _IOW(GPIO_MAGIC, 16, int) 


#endif /* _ALI_GPIO_COMMON_H_ */

