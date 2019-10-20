/*
* ALi GPIO Driver.
*/
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <asm/errno.h>
#include <asm/mach-ali/m36_gpio.h>
#include <ali_interrupt.h>

#include <alidefinition/adf_basic.h>
#include <linux/ali_reg.h>

#ifdef CONFIG_M36_GPIO_ENABLE_INTERRUPT
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/export.h>
#include <ali_gpio_common.h>
#include <linux/workqueue.h>
//#include <linux/ali_transport.h>
#include <uapi/asm-generic/ioctl.h>
#include <asm/uaccess.h>
#include <linux/compiler.h>
#include <linux/ali_kumsgq.h>
#endif
//#define ENABLE_M3701C_JTAG_DEBG

#define GPIO_CTL_REG        __REGALIRAW(0x18000430)  /* GPIO  */
#define GPIOA_CTL_REG       __REGALIRAW(0x18000434)  /* GPIO2 */
#define GPIOB_CTL_REG       __REGALIRAW(0x18000438)  /* GPIO3 */
#define GPIOC_CTL_REG       __REGALIRAW(0x1800043c)  /* GPIO4 */
#define GPIOD_CTL_REG		__REGALIRAW(0x18000440)	 /* GPIO5 */

#define HAL_GPIO_IER_REG    __REGALIRAW(0x18000044)
#define HAL_GPIO_REC_REG    __REGALIRAW(0x18000048)
#define HAL_GPIO_FEC_REG    __REGALIRAW(0x1800004c)
#define HAL_GPIO_ISR_REG    __REGALIRAW(0x1800005c)
#define HAL_GPIO_DIR_REG    __REGALIRAW(0x18000058)
#define HAL_GPIO_DI_REG     __REGALIRAW(0x18000050)
#define HAL_GPIO_DO_REG     __REGALIRAW(0x18000054)

#define HAL_GPIO1_IER_REG	__REGALIRAW(0x180000c4)
#define HAL_GPIO1_REC_REG	__REGALIRAW(0x180000c8)
#define HAL_GPIO1_FEC_REG	__REGALIRAW(0x180000cc)
#define HAL_GPIO1_ISR_REG	__REGALIRAW(0x180000dc)
#define HAL_GPIO1_DIR_REG	__REGALIRAW(0x180000d8)
#define HAL_GPIO1_DI_REG	__REGALIRAW(0x180000d0)
#define HAL_GPIO1_DO_REG	__REGALIRAW(0x180000d4)

#define HAL_GPIO2_IER_REG	__REGALIRAW(0x180000e4)
#define HAL_GPIO2_REC_REG	__REGALIRAW(0x180000e8)
#define HAL_GPIO2_FEC_REG	__REGALIRAW(0x180000ec)
#define HAL_GPIO2_ISR_REG	__REGALIRAW(0x180000fc)
#define HAL_GPIO2_DIR_REG	__REGALIRAW(0x180000f8)
#define HAL_GPIO2_DI_REG	__REGALIRAW(0x180000f0)
#define HAL_GPIO2_DO_REG	__REGALIRAW(0x180000f4)

#define HAL_GPIO3_IER_REG   __REGALIRAW(0x18000344)
#define HAL_GPIO3_REC_REG   __REGALIRAW(0x18000348)
#define HAL_GPIO3_FEC_REG   __REGALIRAW(0x1800034c)
#define HAL_GPIO3_ISR_REG   __REGALIRAW(0x1800035c)
#define HAL_GPIO3_DIR_REG   __REGALIRAW(0x18000358)
#define HAL_GPIO3_DI_REG    __REGALIRAW(0x18000350)
#define HAL_GPIO3_DO_REG    __REGALIRAW(0x18000354)

#define HAL_GPIO4_IER_REG   __REGALIRAW(0x18000444)
#define HAL_GPIO4_REC_REG   __REGALIRAW(0x18000448)
#define HAL_GPIO4_FEC_REG   __REGALIRAW(0x1800044c)
#define HAL_GPIO4_ISR_REG   __REGALIRAW(0x1800045c)
#define HAL_GPIO4_DIR_REG   __REGALIRAW(0x18000458)
#define HAL_GPIO4_DI_REG    __REGALIRAW(0x18000450)
#define HAL_GPIO4_DO_REG    __REGALIRAW(0x18000454)


#define HAL_GPIO_I_DIR      0
#define HAL_GPIO_O_DIR      1
#define HAL_GPIO_INT_EN     0
#define HAL_GPIO_INT_DIS    1
#define HAL_GPIO_EDG_EN     1
#define HAL_GPIO_EDG_DIS    0
#define HAL_GPIO_ENABLE     1
#define HAL_GPIO_DISABLE     0



/* The first set of GPIO */

#define HAL_GPIO_READ()             (*(volatile DWORD *)HAL_GPIO_DI_REG)

#define HAL_GPIO_WRITE(val)         (*(volatile DWORD *)HAL_GPIO_DO_REG = (val))
#define HAL_GPIO_DO_READ()          (*(volatile DWORD *)HAL_GPIO_DO_REG)
#define HAL_GPIO_DIR_GET()          (*(volatile DWORD *)HAL_GPIO_DIR_REG)
#define HAL_GPIO_DIR_SET(mode)      (*(volatile DWORD *)HAL_GPIO_DIR_REG = (mode))
#define HAL_GPIO_IER_SET(val)       (*(volatile DWORD *)HAL_GPIO_IER_REG = (val))
#define HAL_GPIO_IER_GET()          (*(volatile DWORD *)HAL_GPIO_IER_REG)
#define HAL_GPIO_RER_SET(val)       (*(volatile DWORD *)HAL_GPIO_REC_REG = (val))
#define HAL_GPIO_FER_SET(val)       (*(volatile DWORD *)HAL_GPIO_FEC_REG = (val))
#define HAL_GPIO_ISR_GET()          (*(volatile DWORD *)HAL_GPIO_ISR_REG)
#define HAL_GPIO_ISR_SET(val)       (*(volatile DWORD *)HAL_GPIO_ISR_REG = (val))

#define HAL_GPIO1_READ()            (*(volatile DWORD *)HAL_GPIO1_DI_REG)
#define HAL_GPIO1_WRITE(val)        (*(volatile DWORD *)HAL_GPIO1_DO_REG = (val))
#define HAL_GPIO1_DO_READ()         (*(volatile DWORD *)HAL_GPIO1_DO_REG)
#define HAL_GPIO1_DIR_GET()         (*(volatile DWORD *)HAL_GPIO1_DIR_REG)
#define HAL_GPIO1_DIR_SET(mode)     (*(volatile DWORD *)HAL_GPIO1_DIR_REG = (mode))
#define HAL_GPIO1_IER_SET(val)      (*(volatile DWORD *)HAL_GPIO1_IER_REG = (val))
#define HAL_GPIO1_IER_GET()         (*(volatile DWORD *)HAL_GPIO1_IER_REG)
#define HAL_GPIO1_RER_SET(val)      (*(volatile DWORD *)HAL_GPIO1_REC_REG = (val))
#define HAL_GPIO1_FER_SET(val)      (*(volatile DWORD *)HAL_GPIO1_FEC_REG = (val))
#define HAL_GPIO1_ISR_GET()         (*(volatile DWORD *)HAL_GPIO1_ISR_REG)
#define HAL_GPIO1_ISR_SET(val)      (*(volatile DWORD *)HAL_GPIO1_ISR_REG = (val))


#define HAL_GPIO2_READ()            (*(volatile DWORD *)HAL_GPIO2_DI_REG)
#define HAL_GPIO2_WRITE(val)        (*(volatile DWORD *)HAL_GPIO2_DO_REG = (val))
#define HAL_GPIO2_DO_READ()         (*(volatile DWORD *)HAL_GPIO2_DO_REG)
#define HAL_GPIO2_DIR_GET()         (*(volatile DWORD *)HAL_GPIO2_DIR_REG)
#define HAL_GPIO2_DIR_SET(mode)     (*(volatile DWORD *)HAL_GPIO2_DIR_REG = (mode))
#define HAL_GPIO2_IER_SET(val)      (*(volatile DWORD *)HAL_GPIO2_IER_REG = (val))
#define HAL_GPIO2_IER_GET()         (*(volatile DWORD *)HAL_GPIO2_IER_REG)
#define HAL_GPIO2_RER_SET(val)      (*(volatile DWORD *)HAL_GPIO2_REC_REG = (val))
#define HAL_GPIO2_FER_SET(val)      (*(volatile DWORD *)HAL_GPIO2_FEC_REG = (val))
#define HAL_GPIO2_ISR_GET()         (*(volatile DWORD *)HAL_GPIO2_ISR_REG)
#define HAL_GPIO2_ISR_SET(val)      (*(volatile DWORD *)HAL_GPIO2_ISR_REG = (val))

#define HAL_GPIO3_READ()            (*(volatile DWORD *)HAL_GPIO3_DI_REG)
#define HAL_GPIO3_WRITE(val)        (*(volatile DWORD *)HAL_GPIO3_DO_REG = (val))
#define HAL_GPIO3_DO_READ()         (*(volatile DWORD *)HAL_GPIO3_DO_REG)
#define HAL_GPIO3_DIR_GET()         (*(volatile DWORD *)HAL_GPIO3_DIR_REG)
#define HAL_GPIO3_DIR_SET(mode)     (*(volatile DWORD *)HAL_GPIO3_DIR_REG = (mode))
#define HAL_GPIO3_IER_SET(val)      (*(volatile DWORD *)HAL_GPIO3_IER_REG = (val))
#define HAL_GPIO3_IER_GET()         (*(volatile DWORD *)HAL_GPIO3_IER_REG)
#define HAL_GPIO3_RER_SET(val)      (*(volatile DWORD *)HAL_GPIO3_REC_REG = (val))
#define HAL_GPIO3_FER_SET(val)      (*(volatile DWORD *)HAL_GPIO3_FEC_REG = (val))
#define HAL_GPIO3_ISR_GET()         (*(volatile DWORD *)HAL_GPIO3_ISR_REG)
#define HAL_GPIO3_ISR_SET(val)      (*(volatile DWORD *)HAL_GPIO3_ISR_REG = (val))

#define HAL_GPIO4_READ()            (*(volatile DWORD *)HAL_GPIO4_DI_REG)
#define HAL_GPIO4_WRITE(val)        (*(volatile DWORD *)HAL_GPIO4_DO_REG = (val))
#define HAL_GPIO4_DO_READ()         (*(volatile DWORD *)HAL_GPIO4_DO_REG)
#define HAL_GPIO4_DIR_GET()         (*(volatile DWORD *)HAL_GPIO4_DIR_REG)
#define HAL_GPIO4_DIR_SET(mode)     (*(volatile DWORD *)HAL_GPIO4_DIR_REG = (mode))
#define HAL_GPIO4_IER_SET(val)      (*(volatile DWORD *)HAL_GPIO4_IER_REG = (val))
#define HAL_GPIO4_IER_GET()         (*(volatile DWORD *)HAL_GPIO4_IER_REG)
#define HAL_GPIO4_RER_SET(val)      (*(volatile DWORD *)HAL_GPIO4_REC_REG = (val))
#define HAL_GPIO4_FER_SET(val)      (*(volatile DWORD *)HAL_GPIO4_FEC_REG = (val))
#define HAL_GPIO4_ISR_GET()         (*(volatile DWORD *)HAL_GPIO4_ISR_REG)
#define HAL_GPIO4_ISR_SET(val)      (*(volatile DWORD *)HAL_GPIO4_ISR_REG = (val))

/*  End  */

// get input gpio value
#define HAL_GPIO_BIT_GET(pos)			\
			((pos < 32) ? ((HAL_GPIO_READ() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_READ() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_READ() >> (pos - 64)) & 1) \
			: ((pos < 128) ? ((HAL_GPIO3_READ() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_READ() >> (pos - 128)) & 1)))))
			
#define HAL_GPIO_BIT_DO_GET(pos)        \
            			((pos < 32) ? ((HAL_GPIO_DO_READ() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_DO_READ() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_DO_READ() >> (pos - 64)) & 1) \
			: ((pos < 128) ? ((HAL_GPIO3_DO_READ() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_DO_READ() >> (pos - 128)) & 1)))))
#define HAL_GPIO_BIT_DIR_GET(pos)		\
			((pos < 32) ? ((HAL_GPIO_DIR_GET() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_DIR_GET() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_DIR_GET() >> (pos - 64)) & 1) \
			: ((pos < 128) ? ((HAL_GPIO3_DIR_GET() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_DIR_GET() >> (pos - 128)) & 1))))

#define HAL_GPIO_BIT_DIR_SET(pos, val)	\
		do { \
			((pos < 32) ? HAL_GPIO_DIR_SET((HAL_GPIO_DIR_GET() & ~(1 << (pos))) | ((val) << (pos))) \
			: ((pos < 64) ? HAL_GPIO1_DIR_SET((HAL_GPIO1_DIR_GET() & ~(1 << (pos - 32))) | ((val) << (pos - 32))) \
			: ((pos < 96) ? HAL_GPIO2_DIR_SET((HAL_GPIO2_DIR_GET() & ~(1 << (pos - 64))) | ((val) << (pos - 64))) \
			: ((pos < 128) ? HAL_GPIO3_DIR_SET((HAL_GPIO3_DIR_GET() & ~(1 << (pos - 96))) | ((val) << (pos - 96))) \
			: HAL_GPIO4_DIR_SET((HAL_GPIO4_DIR_GET() & ~(1 << (pos - 128))) | ((val) << (pos - 128)))))));\
		} while (0)

#define HAL_GPIO_BIT_SET(pos, val)		\
		do { \
			((pos < 32)	? HAL_GPIO_WRITE(((*(volatile DWORD *)HAL_GPIO_DO_REG) & ~(1 << (pos))) | ((val) << (pos))) \
			: ((pos < 64) ? HAL_GPIO1_WRITE(((*(volatile DWORD *)HAL_GPIO1_DO_REG) & ~(1 << (pos - 32))) | ((val) << (pos - 32))) \
			: ((pos < 96) ? HAL_GPIO2_WRITE(((*(volatile DWORD *)HAL_GPIO2_DO_REG) & ~(1 << (pos - 64))) | ((val) << (pos - 64))) \
			: ((pos < 128) ? HAL_GPIO3_WRITE(((*(volatile DWORD *)HAL_GPIO3_DO_REG) & ~(1 << (pos - 96))) | ((val) << (pos - 96))) \
			: HAL_GPIO4_WRITE(((*(volatile DWORD *)HAL_GPIO4_DO_REG) & ~(1 << (pos - 128))) | ((val) << (pos - 128))))))); \
		} while (0)

#define HAL_GPIO_INT_SET(pos, en)		\
		do { \
			((pos < 32)					\
			? HAL_GPIO_IER_SET(((*(volatile DWORD *)HAL_GPIO_IER_REG) & ~(1 << (pos))) | ((en) << (pos))) \
			: ((pos < 64) 				\
			? HAL_GPIO1_IER_SET(((*(volatile DWORD *)HAL_GPIO1_IER_REG) & ~(1 << (pos - 32))) | ((en) << (pos - 32))) \
			: ((pos < 96) 				\
			? HAL_GPIO2_IER_SET(((*(volatile DWORD *)HAL_GPIO2_IER_REG) & ~(1 << (pos - 64))) | ((en) << (pos - 64))) \
			: ((pos < 128) 				\
			? HAL_GPIO3_IER_SET(((*(volatile DWORD *)HAL_GPIO3_IER_REG) & ~(1 << (pos - 96))) | ((en) << (pos - 96))) \
			: HAL_GPIO4_IER_SET(((*(volatile DWORD *)HAL_GPIO4_IER_REG) & ~(1 << (pos - 128))) | ((en) << (pos - 128))))))); \
		} while (0)

#define HAL_GPIO_INT_REDG_SET(pos, rise)	\
		do { \
			((pos < 32)					\
			? HAL_GPIO_RER_SET(((*(volatile DWORD *)HAL_GPIO_REC_REG) & ~(1 << (pos))) | ((rise) << (pos))) \
			: ((pos < 64) 				\
			? HAL_GPIO1_RER_SET(((*(volatile DWORD *)HAL_GPIO1_REC_REG) & ~(1 << (pos - 32))) | ((rise) << (pos - 32))) \
			: ((pos < 96) 				\
			? HAL_GPIO2_RER_SET(((*(volatile DWORD *)HAL_GPIO2_REC_REG) & ~(1 << (pos - 64))) | ((rise) << (pos - 64))) \
			: ((pos < 128) 				\
			? HAL_GPIO3_RER_SET(((*(volatile DWORD *)HAL_GPIO3_REC_REG) & ~(1 << (pos - 96))) | ((rise) << (pos - 96))) \
			: HAL_GPIO4_RER_SET(((*(volatile DWORD *)HAL_GPIO4_REC_REG) & ~(1 << (pos - 128))) | ((rise) << (pos - 128))))))); \
		} while (0)

#define HAL_GPIO_INT_FEDG_SET(pos, fall)	\
		do { \
			((pos < 32)					\
			? HAL_GPIO_FER_SET(((*(volatile DWORD *)HAL_GPIO_FEC_REG) & ~(1 << (pos))) | ((fall) << (pos))) \
			: ((pos < 64)				\
			? HAL_GPIO1_FER_SET(((*(volatile DWORD *)HAL_GPIO1_FEC_REG) & ~(1 << (pos - 32))) | ((fall) << (pos - 32))) \
			: ((pos < 96) 				\
			? HAL_GPIO2_FER_SET(((*(volatile DWORD *)HAL_GPIO2_FEC_REG) & ~(1 << (pos - 64))) | ((fall) << (pos - 64))) \
			: ((pos < 128) 				\
			? HAL_GPIO3_FER_SET(((*(volatile DWORD *)HAL_GPIO3_FEC_REG) & ~(1 << (pos - 96))) | ((fall) << (pos - 96))) \
			: HAL_GPIO4_FER_SET(((*(volatile DWORD *)HAL_GPIO4_FEC_REG) & ~(1 << (pos - 128))) | ((fall) << (pos - 128))))))); \
		} while (0)

#define HAL_GPIO_INT_EDG_SET(pos, rise, fall)	\
		do { \
			((pos < 32)					\
			? (HAL_GPIO_RER_SET(((*(volatile DWORD *)HAL_GPIO_REC_REG) & ~(1 << (pos))) | ((rise) << (pos))), \
			  HAL_GPIO_FER_SET(((*(volatile DWORD *)HAL_GPIO_FEC_REG) & ~(1 << (pos))) | ((fall) << (pos)))) \
			: ((pos < 64)				\
			? (HAL_GPIO1_RER_SET(((*(volatile DWORD *)HAL_GPIO1_REC_REG) & ~(1 << (pos - 32))) | ((rise) << (pos - 32))), \
			  HAL_GPIO1_FER_SET(((*(volatile DWORD *)HAL_GPIO1_FEC_REG) & ~(1 << (pos - 32))) | ((fall) << (pos - 32)))) \
			: ((pos < 96)				\
			?  (HAL_GPIO2_RER_SET(((*(volatile DWORD *)HAL_GPIO2_REC_REG) & ~(1 << (pos - 64))) | ((rise) << (pos - 64))), \
			  HAL_GPIO2_FER_SET(((*(volatile DWORD *)HAL_GPIO2_FEC_REG) & ~(1 << (pos - 64))) | ((fall) << (pos - 64)))) \
			  : ((pos < 128)				\
			?  (HAL_GPIO3_RER_SET(((*(volatile DWORD *)HAL_GPIO3_REC_REG) & ~(1 << (pos - 96))) | ((rise) << (pos - 96))), \
			  HAL_GPIO3_FER_SET(((*(volatile DWORD *)HAL_GPIO3_FEC_REG) & ~(1 << (pos - 96))) | ((fall) << (pos - 96)))) \
			:  (HAL_GPIO4_RER_SET(((*(volatile DWORD *)HAL_GPIO4_REC_REG) & ~(1 << (pos - 128))) | ((rise) << (pos - 128))), \
			  HAL_GPIO4_FER_SET(((*(volatile DWORD *)HAL_GPIO4_FEC_REG) & ~(1 << (pos - 128))) | ((fall) << (pos - 128)))))))); \
		} while (0)

#define HAL_GPIO_INT_STA_GET(pos)		\
			((pos < 32) ? ((HAL_GPIO_ISR_GET() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_ISR_GET() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_ISR_GET() >> (pos - 64)) & 1) \
			: ((pos < 128) ? ((HAL_GPIO3_ISR_GET() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_ISR_GET() >> (pos - 128)) & 1)))))

#define HAL_GPIO_INT_IER_GET(pos)		\
			((pos < 32) ? ((HAL_GPIO_IER_GET() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_IER_GET() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_IER_GET() >> (pos - 64)) & 1) \
			: ((pos < 128) ? ((HAL_GPIO3_IER_GET() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_IER_GET() >> (pos - 128)) & 1)))))
			
#define HAL_GPIO_INT_CLEAR(pos)		\
			((pos < 32) ? (HAL_GPIO_ISR_SET(1 << (pos))) \
			: ((pos < 64) ? (HAL_GPIO1_ISR_SET(1 << (pos-32))) \
			: ((pos < 96) ? (HAL_GPIO2_ISR_SET(1 << (pos-64))) \
			: ((pos < 128) ? (HAL_GPIO3_ISR_SET(1 << (pos-96))) \
			: (HAL_GPIO4_ISR_SET(1 << (pos-128)))))))


/*add by martin.zhu 2011-06-29 */
/*Some pinmux pin default value is 0, some is 1, define the  enable value .
  Use to enable other Function ( GPIO)
*/
#define HAL_PINMUX_DEFALT_VALUE        0x4E461CD3
#define HAL_PINMUX1_DEFALT_VALUE       0x100793C8

#define HAL_PINMUX_CTRL_REG            __REGALIRAW(0x18000088)
#define HAL_PINMUX_CTRL1_REG           __REGALIRAW(0x1800008c)
// PINMUX REG4 is revert

#define HAL_PINMUX_SET(val)           (*(volatile DWORD *)HAL_PINMUX_CTRL_REG = (val))
#define HAL_PINMUX1_SET(val)          (*(volatile DWORD *)HAL_PINMUX_CTRL1_REG = (val))

#define GET_PINMAX_BIT(val, pos)      (((val >> (pos)) & 1) << pos)
#define GET_BYTE(addr)            		 (*(volatile unsigned char *)(addr))
#define SET_BITE(addr, pos, val)       *(volatile unsigned long *)(addr) = (((*(volatile DWORD *)addr) & ~(1 << (pos))) | ((val) << (pos)))
#define WRITE_DATA_OR(add, val)        *(volatile unsigned long *)(add) |= (unsigned long)(val)
#define WRITE_DATA_AND(add, val)       *(volatile unsigned long *)(add) &= (unsigned long)(val)
#define WRITE_DATA_EQUAL(addr, val)    *(volatile unsigned long *)(addr)= (unsigned long)(val)


#define HAL_PINMUX_GPIO_ENABLE(pos)       \
    do { \
        ((pos < 32)                 \
        ? HAL_PINMUX_SET(((*(volatile DWORD *)HAL_PINMUX_CTRL_REG) & ~(1 << (pos))) | (GET_PINMAX_BIT(HAL_PINMUX_DEFALT_VALUE , pos) )) \
        : ((pos < 64)               \
        ? HAL_PINMUX1_SET(((*(volatile DWORD *)HAL_PINMUX_CTRL1_REG) & ~(1 << (pos - 32))) | (GET_PINMAX_BIT(HAL_PINMUX1_DEFALT_VALUE , pos))) \
        : 1) );\
    } while (0)

#define HAL_GPIO_CTL_SET(val)       (*(volatile DWORD *)GPIO_CTL_REG = (val))
#define HAL_GPIOA_CTL_SET(val)       (*(volatile DWORD *)GPIOA_CTL_REG = (val))
#define HAL_GPIOB_CTL_SET(val)       (*(volatile DWORD *)GPIOB_CTL_REG = (val))
#define HAL_GPIOC_CTL_SET(val)       (*(volatile DWORD *)GPIOC_CTL_REG = (val))
#define HAL_GPIOD_CTL_SET(val)       (*(volatile DWORD *)GPIOD_CTL_REG = (val))

//enable/disable  GPIO function
#define HAL_GPIO_FUNC_ENABLE(pos, en)       \
        do { \
            ((pos < 32)                 \
            ? HAL_GPIO_CTL_SET(((*(volatile DWORD *)GPIO_CTL_REG) &~(1<<(pos))) | (en << (pos))) \
            : ((pos < 64)               \
            ? HAL_GPIOA_CTL_SET(((*(volatile DWORD *)GPIOA_CTL_REG) &~(1<<(pos - 32)))  | (en << (pos - 32))) \
            : ((pos < 96)               \
            ? HAL_GPIOB_CTL_SET(((*(volatile DWORD *)GPIOB_CTL_REG) &~(1<<(pos - 64))) | (en << (pos - 64))) \
             : ((pos < 128)               \
            ? HAL_GPIOC_CTL_SET(((*(volatile DWORD *)GPIOC_CTL_REG) &~(1<<(pos - 96)))  | (en << (pos - 96))) \
		: HAL_GPIOD_CTL_SET(((*(volatile DWORD *)GPIOD_CTL_REG) &~(1<<(pos - 128))) | (en << (pos - 128))))))); \
		} while (0)
        

#define HAL_GPIO_POINT_INT_CLEAR(pos)		\
			((pos < 32) ? (HAL_GPIO_ISR_SET(HAL_GPIO_ISR_GET())) \
			: ((pos < 64) ? (HAL_GPIO1_ISR_SET(HAL_GPIO1_ISR_GET())) \
			: ((pos < 96) ? (HAL_GPIO2_ISR_SET(HAL_GPIO2_ISR_GET())) \
			: ((pos < 128) ? (HAL_GPIO3_ISR_SET(HAL_GPIO3_ISR_GET())) \
			: (HAL_GPIO4_ISR_SET(HAL_GPIO4_ISR_GET()))))))		
extern u32 ali_i2c_gpio_init(void);

static spinlock_t m36_gpio_lock = __SPIN_LOCK_UNLOCKED(m36_gpio_lock);


#ifndef CONFIG_GPIOLIB
int gpio_is_valid(int gpio)
{
    return gpio >= 0 && gpio < GPIO_PORT_MAX;
}

int gpio_request(unsigned gpio, const char *label)
{
    printk(KERN_ALERT "gpio_request: GPIO number(%d), label %s\n", gpio, label);
    return 0;
}

void gpio_free(unsigned gpio)
{
}
#endif


int gpio_enable_pin(int number)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;
	
//    HAL_PINMUX_GPIO_ENABLE(number);
	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_FUNC_ENABLE(number, HAL_GPIO_ENABLE);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}

int gpio_disable_pin(int number)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;
	
//    HAL_PINMUX_GPIO_ENABLE(number);
	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_FUNC_ENABLE(number, HAL_GPIO_DISABLE);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}


int enable_gpio_interrupt_pin(int number)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

    HAL_GPIO_INT_SET(number, 1);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}		

int disable_gpio_interrupt_pin(int number)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

    HAL_GPIO_INT_SET(number, 0);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}	

//set gpio interrupt as rising edge
int set_gpio_rising_ir_pin(int number, int val)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;
	
	spin_lock_irqsave(&m36_gpio_lock, flags);

    HAL_GPIO_INT_REDG_SET(number, val);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}	

//set gpio interrupt as falling edge
int set_gpio_falling_ir_pin(int number, int val)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;
	
	spin_lock_irqsave(&m36_gpio_lock, flags);

    HAL_GPIO_INT_FEDG_SET(number, val);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}	

//clear interrupt status
int clear_gpio_interrupt_status(int number)
{
	unsigned long flags;

	if( !gpio_is_valid(number))
        return -1;
	
	spin_lock_irqsave(&m36_gpio_lock, flags);

    HAL_GPIO_INT_CLEAR(number);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}

int clear_gpio_point_isr(int number)
{
	int val;	
	unsigned long flags;
	
	if( !gpio_is_valid(number))
        return -1;
	
	spin_lock_irqsave(&m36_gpio_lock, flags);
	
	val = HAL_GPIO_POINT_INT_CLEAR(number);

    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return val;
}

//get interrupt status
int get_gpio_interrupt_status(int number)
{
	int val;
	unsigned long flags;
	
	if( !gpio_is_valid(number))
        return -1;
	
	spin_lock_irqsave(&m36_gpio_lock, flags);

    val = HAL_GPIO_INT_STA_GET(number);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return val;
}

int gpio_set_output(unsigned char gpio)
{
	unsigned long flags;

	if( !gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

    HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

    return 0;
}


/*  End  martin.zhu 2011-06-29  */
int ali_gpio_direction_input(unsigned char gpio)
{
	unsigned long flags;

	if( !gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio))
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	}
	else
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	}
	
    spin_unlock_irqrestore(&m36_gpio_lock, flags);
	
    return 0;
}

int ali_gpio_direction_output(unsigned char gpio, int value)
{
	unsigned long flags;

	if( !gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio))
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	}
	else
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	}
    
    HAL_GPIO_BIT_SET(gpio, (value)? 1 : 0);

    spin_unlock_irqrestore(&m36_gpio_lock, flags);
	
    return 0;
}


int ali_gpio_get_value(unsigned char gpio)
{	
	int val;
	unsigned long flags;
	
    if( !gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio))
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	}
	else
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	}
	
	/* GPIO can never have been requested or set as {in,out}put */
	val = HAL_GPIO_BIT_GET(gpio);
	
	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return val;
}


int ali_gpio_set_value(unsigned char gpio, int value)
{	
	unsigned long flags;

	if( !gpio_is_valid(gpio))
		return -1;

	spin_lock_irqsave(&m36_gpio_lock, flags);

	/* fix IC bug */
	if ((68 == gpio) || (69 == gpio) || (70 == gpio) || (145 == gpio) || (146 == gpio))
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	}
	else
	{
		HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	}
    	
	/* GPIO can never have been requested or set as output */
	HAL_GPIO_BIT_SET(gpio, (value)? 1 : 0);
	
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
	
	return 0;
}


static int m36_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned long flags;
	
	//printk("%s->%d\n", __FUNCTION__, offset);	
	
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return 0;
#endif	

	spin_lock_irqsave(&m36_gpio_lock, flags);
	/* fix IC bug */
	if ((__REG16ALI(0x18000002) == 0x3701) 
		&& ((68 == offset) || (69 == offset) || (70 == offset) || (145 == offset) || (146 == offset)))
	{
		HAL_GPIO_BIT_DIR_SET(offset, HAL_GPIO_O_DIR);		
	}
	else
	{
		HAL_GPIO_BIT_DIR_SET(offset, HAL_GPIO_I_DIR);
	}

	HAL_GPIO_FUNC_ENABLE(offset, HAL_GPIO_ENABLE);
	
	spin_unlock_irqrestore(&m36_gpio_lock, flags);

	return 0;
}


static int m36_direction_output(struct gpio_chip *chip, unsigned int offset, int value)
{
	unsigned long flags;

	//printk("%s->%d val->%d\n", __FUNCTION__, offset, value);
	
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return 0;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);
	/* fix IC bug */
	if ((__REG16ALI(0x18000002) == 0x3701) 
		&& ((68 == offset) || (69 == offset) || (70 == offset) || (145 == offset) || (146 == offset)))
	{
		HAL_GPIO_BIT_DIR_SET(offset, HAL_GPIO_I_DIR);		
	}
	else
	{
		HAL_GPIO_BIT_DIR_SET(offset, HAL_GPIO_O_DIR);
	}

	HAL_GPIO_FUNC_ENABLE(offset, HAL_GPIO_ENABLE);

	if(value)
		HAL_GPIO_BIT_SET(offset, 1);
	else
		HAL_GPIO_BIT_SET(offset, 0);
	
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
	
	return 0;
}


static int m36_gpio_get(struct gpio_chip *chip, unsigned offset)
{	
	unsigned long flags;
	int val;
	
	//printk("%s->%d\n", __FUNCTION__, offset);

#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return 0;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);


	/* fix IC bug */
	if ((__REG16ALI(0x18000002) == 0x3701) && (69 == offset))
	{		
		__REG32ALI(0x1805c057) |= (1<<1);		/*enable XPMU_GPIO[1] as PMU internal GPIO */
		val = (__REG32ALI(0x1805c054) >> 1) & 1;	/* read bit 1 */
		__REG32ALI(0x1805c057) &= ~(1<<1);		/*enable XPMU_GPIO[1] as PMU internal GPIO */		
	}
	else
	{
		val = HAL_GPIO_BIT_GET(offset);
	}
	
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
	
	return val;
}

static void m36_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	unsigned long flags;

	//printk("%s->%d val->%d\n", __FUNCTION__, offset,val);

#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);

	if(val)
		HAL_GPIO_BIT_SET(offset, 1);
	else
		HAL_GPIO_BIT_SET(offset, 0);
		
	spin_unlock_irqrestore(&m36_gpio_lock, flags);

}


#ifdef CONFIG_MIPS
int gpio_to_irq(unsigned gpio)
{
	return INT_ALI_GPIO;
}


int irq_to_gpio(unsigned irq)
{
	printk(KERN_ERR "m36_irq_to_gpio: not support(irq = %d)\n",irq);
	return -EINVAL;
}
#endif

void gpio_irq_enable(unsigned offset)
{
	unsigned long flags;
	
	//printk("%s->%d\n", __FUNCTION__, offset);

#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_INT_SET(offset, 1);	
		
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
}

void gpio_irq_disable(unsigned offset)
{
	unsigned long flags;
	
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return;
#endif

	//printk("%s->%d\n", __FUNCTION__, offset);

	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_INT_SET(offset, 0);	
		
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
}	

void gpio_irq_type(unsigned offset, unsigned trigger)
{
	unsigned long flags;

	//printk("%s->%d type->0x%x\n", __FUNCTION__, offset, trigger);

#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);

	if(trigger & IRQ_TYPE_EDGE_RISING)
		HAL_GPIO_INT_REDG_SET(offset, 1);
	else
		HAL_GPIO_INT_REDG_SET(offset, 0);

	if(trigger & IRQ_TYPE_EDGE_FALLING)
		HAL_GPIO_INT_FEDG_SET(offset, 1);
	else
		HAL_GPIO_INT_FEDG_SET(offset, 0);
	
	if(trigger == IRQ_TYPE_EDGE_BOTH)
	{
		//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&] %d\n",__func__,__LINE__,IRQ_TYPE_EDGE_BOTH);
		HAL_GPIO_INT_REDG_SET(offset, 1);
		HAL_GPIO_INT_FEDG_SET(offset, 1);
	}
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
}

void gpio_irq_clear(unsigned offset)
{
	unsigned long flags;

	//printk("%s->%d\n", __FUNCTION__, offset);
	
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);

	HAL_GPIO_INT_CLEAR(offset);	
		
	spin_unlock_irqrestore(&m36_gpio_lock, flags);
}


int gpio_irq_get_status(unsigned offset)
{
    unsigned long flags;
    int irq_status = 0;
    
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return 0;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);
    irq_status = HAL_GPIO_INT_STA_GET(offset);
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

    return irq_status;
}
int gpio_irq_get_enable(unsigned offset)
{
	unsigned long flags;
    int irq_enable = 0;
    
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return 0;
#endif

    spin_lock_irqsave(&m36_gpio_lock, flags);
    irq_enable = HAL_GPIO_INT_IER_GET(offset);
    spin_unlock_irqrestore(&m36_gpio_lock, flags);
	return irq_enable;
}

int gpio_do_get_status(unsigned offset)
{
	unsigned long flags;
    int val = 0;
    
#ifdef ENABLE_M3701C_JTAG_DEBG
	if((offset == 17) || (offset == 18)
		|| (offset == 4) || (offset == 1)
		|| (offset == 32))
	return 0;
#endif

	spin_lock_irqsave(&m36_gpio_lock, flags);
    val = HAL_GPIO_BIT_DO_GET(offset);
    spin_unlock_irqrestore(&m36_gpio_lock, flags);

    return val;
}

#ifdef CONFIG_M36_GPIO_ENABLE_INTERRUPT
struct ali_gpio {
	void __iomem *base;
	struct irq_domain *domain;
	int irq;
	struct gpio_chip gc;
	struct miscdevice miscdev;
	char label[16];
	
	struct workqueue_struct *gpio_notification_workqueue;
    struct work_struct gpio_notification_work;
};
struct ali_gpio_com_device_data
{
	//struct	cdev cdev;
	struct kumsgq *gpio_kumsgq;
	struct mutex gpio_mutex;
	//void    *priv;
};

#define ALI_GPIO_DEVICE_NAME 	"ali_gpio"
#define GPIO_SUCCESS (0)
#define GPIO_FAILED  (-1)
static int g_init_flag = -1; 
struct ali_gpio_com_device_data gpio_com_dev;


static struct gpio_info g_irq_gpio[5]={{0,0},{0,0},{0,0},{0,0},{0,0}};

static void set_g_irq_gpio(int offset)
{
   int i = 0;
   for(i = 0 ;i<5;i++)
   {
   	 if(g_irq_gpio[i].gpio == 0)
	 	{
			g_irq_gpio[i].gpio= offset;
			//if(gpio_get_value(offset))
			//if(gpio_do_get_status(offset))  //check GPIO Output Register
			if(1 == ali_gpio_get_value(offset))//check GPIO Input Status Register
			{
				//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]\n",__func__,__LINE__);
				g_irq_gpio[i].status = IRQ_TYPE_EDGE_RISING;//if 1,IRQ_TYPE_EDGE_RISING
			}
			else
			{
				//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]\n",__func__,__LINE__);
				g_irq_gpio[i].status = IRQ_TYPE_EDGE_FALLING;//if 0,IRQ_TYPE_EDGE_FALLING
			}
			break;
   	     }
   	}
}
static int m36_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]\n",__func__,__LINE__);
	return  gpio_enable_pin(offset);
}

static void m36_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]\n",__func__,__LINE__);
	gpio_disable_pin(offset);
}
/* Workqueue to notify application */
static void gpio_irq_happen_status_notification(struct work_struct *work)
{
	int i = 0;
	unsigned long flags;
	struct gpio_info p_gpio_info;
	
	for(i = 0 ;i < 5 ; i++)
	{
		if((g_irq_gpio[i].gpio > 0) && (1 == gpio_irq_get_enable(g_irq_gpio[i].gpio)))
		{
		   p_gpio_info.gpio = g_irq_gpio[i].gpio;
		   p_gpio_info.status = g_irq_gpio[i].status;
		   spin_lock_irqsave(&m36_gpio_lock, flags);
           ali_kumsgq_sendmsg(gpio_com_dev.gpio_kumsgq,&p_gpio_info,sizeof(p_gpio_info));
		   spin_unlock_irqrestore(&m36_gpio_lock, flags);
		   g_irq_gpio[i].gpio = 0;
		   g_irq_gpio[i].status = 0;
		   //printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]irq_gpio:%d status:%d\n",__func__,__LINE__,p_gpio_info.gpio,p_gpio_info.status);
		}			   
	}
	//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]port:%x\n",__func__,__LINE__,(unsigned int)g_com_port);
}
static irqreturn_t ali_gpio_irq_handle(int irq, void *data)
{
	struct ali_gpio *ali_gpio = (struct ali_gpio *)data;
	int i = 0;
    /*Clear Irq*/
	for(i = 0 ;i < GPIO_PORT_MAX+1 ; i++)
	{
	  if(gpio_irq_get_status(i))
	  {
	  	//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]gpio_id:%d\n",__func__,__LINE__,i);
        set_g_irq_gpio(i);
		gpio_irq_clear(i);
		break;
	  }
	}
    /*WorkQueue*/
	if(g_init_flag < 0)
	{
		g_init_flag = 0;
        INIT_WORK(&ali_gpio->gpio_notification_work, \
                  gpio_irq_happen_status_notification);
	}
	else
	{
		PREPARE_WORK(&ali_gpio->gpio_notification_work, \
                     gpio_irq_happen_status_notification);
	}
	if (NULL != ali_gpio->gpio_notification_workqueue)
    {
	 	queue_work(ali_gpio->gpio_notification_workqueue, \
                   &ali_gpio->gpio_notification_work);
	}

	
	return IRQ_HANDLED;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
static long ali_gpio_ioctl(struct file * file, unsigned int cmd, unsigned long param)
#else
static int ali_gpio_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long param)
#endif
{
	//struct ali_gpio *ali_gpio = file->private_data;
	//unsigned long flags;
	int ret = GPIO_SUCCESS;
	unsigned int offset = 0;
	gpio_info p_gpio_info;

	//*((unsigned int *)0xb8000208) = param;
	//printk(KERN_INFO "GPIO:In %s line:%d cmd = %d\n",__func__,__LINE__, cmd);
	switch(cmd)
	{
		case GPIO_SET_IRQ_ENABLE:
			{
				offset = (unsigned int)param;
				gpio_irq_enable(offset);
				//printk(KERN_INFO "[&&&&&&&&&&%s,%d&&&&&&&&&&] SET_IRQ_ENABLE_ID %d\n",__func__,__LINE__,(unsigned int)offset);
             	break;
		    }
		case GPIO_GET_KUMSGQ:
			{
				int flags = -1;
				mutex_lock(&gpio_com_dev.gpio_mutex);	
				if(copy_from_user(&flags, (int *)param, sizeof(int)))
				{
					printk("Err: copy_from_user\n");
					mutex_unlock(&gpio_com_dev.gpio_mutex);
					return -EFAULT;
				}
				ret  = ali_kumsgq_newfd(gpio_com_dev.gpio_kumsgq, flags);
				if(ret> 0)
				{
					mutex_unlock(&gpio_com_dev.gpio_mutex);
					return ret;	
				}	
				break;
				//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]port:%x\n",__func__,__LINE__,(unsigned int)g_com_port);
		     }
             
		case GPIO_SET_IRQ_DISENABLE:
			{
				offset = (unsigned int)param;
				gpio_irq_disable(offset);
				//printk(KERN_INFO "GPIO:In %s line:%d SET_IRQ_DISENABLE_ID %d\n",__func__,__LINE__,(unsigned int)offset);
				break;
		    }
		case GPIO_GET_IRQ_ENABLE:
			{
				if (0!=copy_from_user(&p_gpio_info, (gpio_info __user *)param, sizeof(gpio_info)))
			    {
				  return -EFAULT;
			    }
				
				p_gpio_info.status = gpio_irq_get_enable(p_gpio_info.gpio);
				//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]gpio:%d\n",__func__,__LINE__,(unsigned int)p_gpio_info.gpio);
				if (0 != copy_to_user((gpio_info __user *)param, &p_gpio_info, sizeof(gpio_info)))
                {
            		return -EFAULT;
            	}
				break;
			}
		case GPIO_IRQ_CLEAR:
			{
				offset = (unsigned int)param;
				gpio_irq_clear(offset);
				break;
			}
		case GPIO_SET_IRQ_TYPE:
			{
				if (0!=copy_from_user(&p_gpio_info, (gpio_info __user *)param, sizeof(gpio_info)))
			    {
				  return -EFAULT;
			    }
				gpio_irq_type(p_gpio_info.gpio,p_gpio_info.status);
				//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]gpio:%d\n",__func__,__LINE__,(unsigned int)p_gpio_info.gpio);
				break;
			}
		case GPIO_GET_IRQ_STATUS:
			{
				if (0!=copy_from_user(&p_gpio_info, (gpio_info __user *)param, sizeof(gpio_info)))
			    {
				  return -EFAULT;
			    }
				p_gpio_info.status = gpio_irq_get_status(p_gpio_info.gpio);
				//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]gpio:%d\n",__func__,__LINE__,(unsigned int)p_gpio_info.gpio);
				if (0 != copy_to_user((gpio_info __user *)param, &p_gpio_info, sizeof(gpio_info)))
                {
            		return -EFAULT;
            	}
				break;
			}

			
	}
	return ret;
}
static const struct file_operations g_gpio_fops = {
	.owner		= THIS_MODULE,
	.open		= NULL,	
	.release		= NULL,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
	.unlocked_ioctl = ali_gpio_ioctl,
	#else
	.ioctl			= ali_gpio_ioctl,
	#endif
};

 void gpio_dev_workqueue_request(struct ali_gpio *ali_gpio)
{
	ali_gpio->gpio_notification_workqueue = create_workqueue(ALI_GPIO_DEVICE_NAME);
	if(NULL == ali_gpio->gpio_notification_workqueue)
	{
		printk(KERN_ERR "GPIO DEV:In %s  %dWarning -Work queue creat fail!",__func__,__LINE__);
	}
	
}

void gpio_dev_workqueue_release(struct ali_gpio *ali_gpio)
{
	
    if (NULL != ali_gpio->gpio_notification_workqueue)
    {
        destroy_workqueue(ali_gpio->gpio_notification_workqueue);
        ali_gpio->gpio_notification_workqueue = NULL;
    }
	
}
static int ali_gpio_probe(struct platform_device *pdev)
{
	struct ali_gpio *ali_gpio;
	int ret = 0;

	ali_gpio = devm_kzalloc(&pdev->dev, sizeof(*ali_gpio), GFP_KERNEL);
	if (ali_gpio == NULL)
		return -ENOMEM;
    //creat workqueue
    gpio_dev_workqueue_request(ali_gpio);

	//add aligpio as gpiochip
	ali_gpio->gc.label		= ALI_GPIO_DEVICE_NAME;
	ali_gpio->gc.dev		= &pdev->dev;
	ali_gpio->gc.owner		= THIS_MODULE;
	ali_gpio->gc.direction_input	= m36_direction_input;
	ali_gpio->gc.get		= m36_gpio_get;
	ali_gpio->gc.direction_output	= m36_direction_output;
	ali_gpio->gc.set		= m36_gpio_set;
	ali_gpio->gc.request		= m36_gpio_request;
	ali_gpio->gc.free		= m36_gpio_free;
	ali_gpio->gc.base		= 0;
	ali_gpio->gc.ngpio		= GPIO_PORT_MAX + 1;
	ali_gpio->gc.can_sleep		= false; 
	ret = gpiochip_add(&ali_gpio->gc);
	if (ret < 0) 
	{
		dev_err(&pdev->dev, "Could not add gpiochip.\n");
		goto fail_gpiochip_registration;
	}
    
    //request irq
	ali_gpio->irq =  INT_ALI_GPIO;
	ret = devm_request_irq(&pdev->dev, ali_gpio->irq, ali_gpio_irq_handle,
				IRQF_TRIGGER_RISING | IRQF_SHARED|IRQF_TRIGGER_FALLING,
				dev_name(&pdev->dev), ali_gpio);
	if (ret != 0)goto fail_request_irq;
	
	//creat aligpio misc for ioctl
	ali_gpio->miscdev.fops = &g_gpio_fops;
	ali_gpio->miscdev.name = ALI_GPIO_DEVICE_NAME;
	ali_gpio->miscdev.minor = MISC_DYNAMIC_MINOR;
	ret = misc_register(&ali_gpio->miscdev);
	if (ret != 0) {
		printk(KERN_ERR " cannot register ali gpio miscdev(err=%d)\n", ret);
		goto fail_misc;
	}
	
	/* request kernel-userspace message queue creation */
	gpio_com_dev.gpio_kumsgq = (struct kumsgq *)ali_new_kumsgq();
	if (!gpio_com_dev.gpio_kumsgq)
	{
		goto fail_misc;
    }
	mutex_init(&gpio_com_dev.gpio_mutex);
	return 0;
fail_misc:
	gpio_dev_workqueue_release(ali_gpio);
	misc_deregister(&ali_gpio->miscdev);
fail_request_irq:
	ret = gpiochip_remove(&ali_gpio->gc);
fail_gpiochip_registration:
	devm_kfree(&pdev->dev, ali_gpio);
	return ret;
}

static int __exit ali_gpio_remove(struct platform_device *pdev)
{
  int ret = 0; 
  struct ali_gpio *ali_gpio = platform_get_drvdata(pdev);

  gpio_dev_workqueue_release(ali_gpio);
  devm_free_irq(&pdev->dev, ali_gpio->irq, ali_gpio);
  misc_deregister(&ali_gpio->miscdev);
  ret = gpiochip_remove(&ali_gpio->gc);
  if (ret)return ret;
  devm_kfree(&pdev->dev, ali_gpio);

  /* release kernel-userspace message queue */
  ali_destroy_kumsgq(gpio_com_dev.gpio_kumsgq);
  gpio_com_dev.gpio_kumsgq = NULL;
  mutex_destroy(&gpio_com_dev.gpio_mutex);
  return ret;
}


static struct platform_driver ali_gpio_driver = {
	.probe		= ali_gpio_probe,
	.remove		= ali_gpio_remove,
	.driver = {
		.name	= ALI_GPIO_DEVICE_NAME,
		.owner= THIS_MODULE,
	},
};

static int __init ali_gpio_init(void)
{
	struct platform_device *pd;
	//printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]\n",__func__,__LINE__);
	
	pd = platform_device_alloc(ALI_GPIO_DEVICE_NAME, 0);
	if (pd == NULL) 
	{
		printk(KERN_ERR "ali_gpio: failed to allocate device id %d\n",0);
		return -ENODEV;
	}
	pd->dev.platform_data = NULL;
	platform_device_add(pd);
	
	return platform_driver_probe(&ali_gpio_driver, ali_gpio_probe);
}

static void __exit ali_gpio_exit(void)
{
	platform_driver_unregister(&ali_gpio_driver);
}

int __init m36_init_gpio(void)
{
	printk(KERN_INFO"[&&&&&&&&&&%s,%d&&&&&&&&&&]m36_init_gpio not implete\n",__func__,__LINE__);
	return 1;//gpiochip_add(&m36_gpio_chip);
}

module_init(ali_gpio_init);//must be module because msic is subsys_initcall
module_exit(ali_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ALi gpio driver");
MODULE_VERSION("1.0.0");

#else
static struct gpio_chip m36_gpio_chip = {
	.label			= "m36",
	.direction_input	= m36_direction_input,
	.direction_output	= m36_direction_output,
	.set			= m36_gpio_set,
	.get			= m36_gpio_get,
	.base			= 0,
	.ngpio			= GPIO_PORT_MAX + 1,
};

int __init m36_init_gpio(void)
{
	return gpiochip_add(&m36_gpio_chip);
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0))
#else
arch_initcall(m36_init_gpio);
#endif
#endif



