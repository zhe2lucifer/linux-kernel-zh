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

/*
* ALi GPIO Driver.
*/
#include <asm/io.h>
#include <linux/export.h> /* for EXPORT_SYMBOL */
#include <linux/ali_gpio.h>

#define HAL_GPIO_INT_EN	0
#define HAL_GPIO_INT_DIS	1
#define HAL_GPIO_EDG_EN	1
#define HAL_GPIO_EDG_DIS	0
#define HAL_GPIO_ENABLE  1
#define HAL_GPIO_DISABLE  0
#define HAL_GPIO_I_DIR      0
#define HAL_GPIO_O_DIR      1


typedef unsigned long DWORD; 

//GPIO Accessing.
#define HAL_GPIO_IER_REG	__REGALIRAW((ALI_SOC_BASE + 0x44))
#define HAL_GPIO_REC_REG	__REGALIRAW((ALI_SOC_BASE + 0x48))
#define HAL_GPIO_FEC_REG	__REGALIRAW((ALI_SOC_BASE + 0x4c))
#define HAL_GPIO_ISR_REG	__REGALIRAW((ALI_SOC_BASE + 0x5c))
#define HAL_GPIO_DIR_REG	__REGALIRAW((ALI_SOC_BASE + 0x58))
#define HAL_GPIO_DI_REG	__REGALIRAW((ALI_SOC_BASE + 0x50))
#define HAL_GPIO_DO_REG	__REGALIRAW((ALI_SOC_BASE + 0x54))
#define HAL_GPIO_EN_REG      __REGALIRAW((ALI_SOC_BASE + 0x430))

#define HAL_GPIO1_IER_REG	__REGALIRAW((ALI_SOC_BASE + 0xc4))
#define HAL_GPIO1_REC_REG	__REGALIRAW((ALI_SOC_BASE + 0xc8))
#define HAL_GPIO1_FEC_REG	__REGALIRAW((ALI_SOC_BASE + 0xcc))
#define HAL_GPIO1_ISR_REG	__REGALIRAW((ALI_SOC_BASE + 0xdc))
#define HAL_GPIO1_DIR_REG	__REGALIRAW((ALI_SOC_BASE + 0xd8))
#define HAL_GPIO1_DI_REG	__REGALIRAW((ALI_SOC_BASE + 0xd0))
#define HAL_GPIO1_DO_REG	__REGALIRAW((ALI_SOC_BASE + 0xd4))
#define HAL_GPIO1_EN_REG    __REGALIRAW((ALI_SOC_BASE + 0x434))

#define HAL_GPIO2_IER_REG	__REGALIRAW((ALI_SOC_BASE + 0xe4)) 
#define HAL_GPIO2_REC_REG	__REGALIRAW((ALI_SOC_BASE + 0xe8)) 
#define HAL_GPIO2_FEC_REG	__REGALIRAW((ALI_SOC_BASE + 0xec)) 
#define HAL_GPIO2_ISR_REG	__REGALIRAW((ALI_SOC_BASE + 0xfc)) 
#define HAL_GPIO2_DIR_REG	__REGALIRAW((ALI_SOC_BASE + 0xf8)) 
#define HAL_GPIO2_DI_REG	__REGALIRAW((ALI_SOC_BASE + 0xf0)) 
#define HAL_GPIO2_DO_REG	__REGALIRAW((ALI_SOC_BASE + 0xf4)) 
#define HAL_GPIO2_EN_REG    __REGALIRAW((ALI_SOC_BASE + 0x438))

#define HAL_GPIO3_IER_REG	__REGALIRAW((ALI_SOC_BASE + 0x344))
#define HAL_GPIO3_REC_REG	__REGALIRAW((ALI_SOC_BASE + 0x348))
#define HAL_GPIO3_FEC_REG	__REGALIRAW((ALI_SOC_BASE + 0x34c))
#define HAL_GPIO3_ISR_REG	__REGALIRAW((ALI_SOC_BASE + 0x35c))
#define HAL_GPIO3_DIR_REG	__REGALIRAW((ALI_SOC_BASE + 0x358))
#define HAL_GPIO3_DI_REG	__REGALIRAW((ALI_SOC_BASE + 0x350))
#define HAL_GPIO3_DO_REG	__REGALIRAW((ALI_SOC_BASE + 0x354))
#define HAL_GPIO3_EN_REG    __REGALIRAW((ALI_SOC_BASE + 0x43c))

#define HAL_GPIO4_IER_REG	__REGALIRAW((ALI_SOC_BASE + 0x444))
#define HAL_GPIO4_REC_REG	__REGALIRAW((ALI_SOC_BASE + 0x448))
#define HAL_GPIO4_FEC_REG	__REGALIRAW((ALI_SOC_BASE + 0x44c))
#define HAL_GPIO4_ISR_REG	__REGALIRAW((ALI_SOC_BASE + 0x45c))
#define HAL_GPIO4_DIR_REG	__REGALIRAW((ALI_SOC_BASE + 0x458))
#define HAL_GPIO4_DI_REG	__REGALIRAW((ALI_SOC_BASE + 0x450))
#define HAL_GPIO4_DO_REG	__REGALIRAW((ALI_SOC_BASE + 0x454))
#define HAL_GPIO4_EN_REG    __REGALIRAW((ALI_SOC_BASE + 0x440))


/* The first set of GPIO */
#define HAL_GPIO_READ()				(*(volatile DWORD *)HAL_GPIO_DI_REG)
#define HAL_GPIO_WRITE(val)			(*(volatile DWORD *)HAL_GPIO_DO_REG = (val))
#define HAL_GPIO_DIR_GET()			(*(volatile DWORD *)HAL_GPIO_DIR_REG)
#define HAL_GPIO_DIR_SET(mode)		(*(volatile DWORD *)HAL_GPIO_DIR_REG = (mode))
#define HAL_GPIO_IER_SET(val)		(*(volatile DWORD *)HAL_GPIO_IER_REG = (val))
#define HAL_GPIO_RER_SET(val)		(*(volatile DWORD *)HAL_GPIO_REC_REG = (val))
#define HAL_GPIO_FER_SET(val)		(*(volatile DWORD *)HAL_GPIO_FEC_REG = (val))
#define HAL_GPIO_ISR_GET()			(*(volatile DWORD *)HAL_GPIO_ISR_REG)
#define HAL_GPIO_ISR_SET(val)		(*(volatile DWORD *)HAL_GPIO_ISR_REG = (val))

#define HAL_GPIO1_READ()			(*(volatile DWORD *)HAL_GPIO1_DI_REG)
#define HAL_GPIO1_WRITE(val)		(*(volatile DWORD *)HAL_GPIO1_DO_REG = (val))
#define HAL_GPIO1_DIR_GET()		    (*(volatile DWORD *)HAL_GPIO1_DIR_REG)
#define HAL_GPIO1_DIR_SET(mode)	    (*(volatile DWORD *)HAL_GPIO1_DIR_REG = (mode))
#define HAL_GPIO1_IER_SET(val)		(*(volatile DWORD *)HAL_GPIO1_IER_REG = (val))
#define HAL_GPIO1_RER_SET(val)		(*(volatile DWORD *)HAL_GPIO1_REC_REG = (val))
#define HAL_GPIO1_FER_SET(val)		(*(volatile DWORD *)HAL_GPIO1_FEC_REG = (val))
#define HAL_GPIO1_ISR_GET()			(*(volatile DWORD *)HAL_GPIO1_ISR_REG)
#define HAL_GPIO1_ISR_SET(val)		(*(volatile DWORD *)HAL_GPIO1_ISR_REG = (val))


#define HAL_GPIO2_READ()			(*(volatile DWORD *)HAL_GPIO2_DI_REG)
#define HAL_GPIO2_WRITE(val)		(*(volatile DWORD *)HAL_GPIO2_DO_REG = (val))
#define HAL_GPIO2_DIR_GET()		    (*(volatile DWORD *)HAL_GPIO2_DIR_REG)
#define HAL_GPIO2_DIR_SET(mode)	    (*(volatile DWORD *)HAL_GPIO2_DIR_REG = (mode))
#define HAL_GPIO2_IER_SET(val)		(*(volatile DWORD *)HAL_GPIO2_IER_REG = (val))
#define HAL_GPIO2_RER_SET(val)		(*(volatile DWORD *)HAL_GPIO2_REC_REG = (val))
#define HAL_GPIO2_FER_SET(val)		(*(volatile DWORD *)HAL_GPIO2_FEC_REG = (val))
#define HAL_GPIO2_ISR_GET()			(*(volatile DWORD *)HAL_GPIO2_ISR_REG)
#define HAL_GPIO2_ISR_SET(val)		(*(volatile DWORD *)HAL_GPIO2_ISR_REG = (val))

#define HAL_GPIO3_READ()			(*(volatile DWORD *)HAL_GPIO3_DI_REG)
#define HAL_GPIO3_WRITE(val)		(*(volatile DWORD *)HAL_GPIO3_DO_REG = (val))
#define HAL_GPIO3_DIR_GET()		    (*(volatile DWORD *)HAL_GPIO3_DIR_REG)
#define HAL_GPIO3_DIR_SET(mode)	    (*(volatile DWORD *)HAL_GPIO3_DIR_REG = (mode))
#define HAL_GPIO3_IER_SET(val)		(*(volatile DWORD *)HAL_GPIO3_IER_REG = (val))
#define HAL_GPIO3_RER_SET(val)		(*(volatile DWORD *)HAL_GPIO3_REC_REG = (val))
#define HAL_GPIO3_FER_SET(val)		(*(volatile DWORD *)HAL_GPIO3_FEC_REG = (val))
#define HAL_GPIO3_ISR_GET()			(*(volatile DWORD *)HAL_GPIO3_ISR_REG)
#define HAL_GPIO3_ISR_SET(val)		(*(volatile DWORD *)HAL_GPIO3_ISR_REG = (val))

#define HAL_GPIO4_READ()			(*(volatile DWORD *)HAL_GPIO4_DI_REG)
#define HAL_GPIO4_WRITE(val)		(*(volatile DWORD *)HAL_GPIO4_DO_REG = (val))
#define HAL_GPIO4_DIR_GET()		    (*(volatile DWORD *)HAL_GPIO4_DIR_REG)
#define HAL_GPIO4_DIR_SET(mode)	    (*(volatile DWORD *)HAL_GPIO4_DIR_REG = (mode))
#define HAL_GPIO4_IER_SET(val)		(*(volatile DWORD *)HAL_GPIO4_IER_REG = (val))
#define HAL_GPIO4_RER_SET(val)		(*(volatile DWORD *)HAL_GPIO4_REC_REG = (val))
#define HAL_GPIO4_FER_SET(val)		(*(volatile DWORD *)HAL_GPIO4_FEC_REG = (val))
#define HAL_GPIO4_ISR_GET()			(*(volatile DWORD *)HAL_GPIO4_ISR_REG)
#define HAL_GPIO4_ISR_SET(val)		(*(volatile DWORD *)HAL_GPIO4_ISR_REG = (val))

#define HAL_GPIO_EN_SET(val)            (*(volatile unsigned long *)HAL_GPIO_EN_REG =(val))
#define HAL_GPIO1_EN_SET(val)           (*(volatile unsigned long *)HAL_GPIO1_EN_REG =(val))
#define HAL_GPIO2_EN_SET(val)           (*(volatile unsigned long *)HAL_GPIO2_EN_REG =(val))
#define HAL_GPIO3_EN_SET(val)           (*(volatile unsigned long *)HAL_GPIO3_EN_REG =(val))
#define HAL_GPIO4_EN_SET(val)           (*(volatile unsigned long *)HAL_GPIO4_EN_REG =(val))

#define HAL_GPIO_BIT_ENABLE(pos,en)       \
	        do {\
				((pos < 32)           \
				? HAL_GPIO_EN_SET(((*(volatile unsigned long *)HAL_GPIO_EN_REG)&~(1<<(pos)))|((en)<<(pos)))\
				:((pos < 64)            \
				? HAL_GPIO1_EN_SET(((*(volatile unsigned long *)HAL_GPIO1_EN_REG)&~(1<<(pos - 32)))|((en)<<(pos - 32)))\
				:((pos < 96)            \
				? HAL_GPIO2_EN_SET(((*(volatile unsigned long *)HAL_GPIO2_EN_REG)&~(1<<(pos - 64)))|((en)<<(pos - 64)))\
				:((pos < 128)          \
				? HAL_GPIO3_EN_SET(((*(volatile unsigned long *)HAL_GPIO3_EN_REG)&~(1<<(pos - 96)))|((en)<<(pos - 96)))\
				: HAL_GPIO4_EN_SET(((*(volatile unsigned long *)HAL_GPIO4_EN_REG)&~(1<<(pos - 128)))|((en)<<(pos - 128)))))));\
			}while(0)


#define HAL_GPIO_BIT_GET(pos)			\
			((pos < 32) ? ((HAL_GPIO_READ() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_READ() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_READ() >> (pos - 64)) & 1) \
			: ((pos < 128) ? ((HAL_GPIO3_READ() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_READ() >> (pos - 128)) & 1)))))

#define HAL_GPIO_BIT_DIR_GET(pos)		\
			((pos < 32) ? ((HAL_GPIO_DIR_GET() >> (pos)) & 1) \
			: ((pos < 64) ? ((HAL_GPIO1_DIR_GET() >> (pos - 32)) & 1) \
			: ((pos < 96) ? ((HAL_GPIO2_DIR_GET() >> (pos - 64)) & 1) \
			: ((pos <128) ? ((HAL_GPIO3_DIR_GET() >> (pos - 128)) & 1) \
			: ((HAL_GPIO4_DIR_GET() >> (pos - 128)) & 1)))))

#define HAL_GPIO_BIT_DIR_SET(pos, val)	\
		do { \
			((pos < 32) ? HAL_GPIO_DIR_SET((HAL_GPIO_DIR_GET() & ~(1 << (pos))) | ((val) << (pos))) \
			: ((pos < 64) ? HAL_GPIO1_DIR_SET((HAL_GPIO1_DIR_GET() & ~(1 << (pos - 32))) | ((val) << (pos - 32))) \
			: ((pos < 96) ? HAL_GPIO2_DIR_SET((HAL_GPIO2_DIR_GET() & ~(1 << (pos - 64))) | ((val) << (pos - 64))) \
			: ((pos <128) ? HAL_GPIO3_DIR_SET((HAL_GPIO3_DIR_GET() & ~(1 << (pos - 96))) | ((val) << (pos - 96))) \
			: HAL_GPIO4_DIR_SET((HAL_GPIO4_DIR_GET() & ~(1 << (pos - 128))) | ((val) << (pos - 128)))))));\
		} while (0)

#define HAL_GPIO_BIT_SET(pos, val)		\
		do { \
			((pos < 32)	? HAL_GPIO_WRITE(((*(volatile DWORD *)HAL_GPIO_DO_REG) & ~(1 << (pos))) | ((val) << (pos))) \
			: ((pos < 64) ? HAL_GPIO1_WRITE(((*(volatile DWORD *)HAL_GPIO1_DO_REG) & ~(1 << (pos - 32))) | ((val) << (pos - 32))) \
			: ((pos < 96) ? HAL_GPIO2_WRITE(((*(volatile DWORD *)HAL_GPIO2_DO_REG) & ~(1 << (pos - 64))) | ((val) << (pos - 64))) \
			: ((pos <128) ? HAL_GPIO3_WRITE(((*(volatile DWORD *)HAL_GPIO3_DO_REG) & ~(1 << (pos - 96))) | ((val) << (pos -96))) \
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
			: ((pos <128) ? ((HAL_GPIO3_ISR_GET() >> (pos - 96)) & 1) \
			: ((HAL_GPIO4_ISR_GET() >> (pos - 128)) & 1)))))

#define HAL_GPIO_INT_CLEAR(pos)		\
			((pos < 32) ? (HAL_GPIO_ISR_SET(1 << (pos))) \
			: ((pos < 64) ? (HAL_GPIO1_ISR_SET(1 << (pos-32))) \
			: ((pos < 96) ? (HAL_GPIO2_ISR_SET(1 << (pos-64))) \
			: ((pos <128) ? (HAL_GPIO3_ISR_SET(1 << (pos-96))) \
			: (HAL_GPIO4_ISR_SET(1 << (pos-128)))))))

#define HAL_GPIO_FUNC_ENABLE(pos, en)       HAL_GPIO_BIT_ENABLE(pos,en)

bool gpio_is_valid(int gpio)
{
    return gpio >= 0 && gpio < GPIO_PORT_MAX;
}
EXPORT_SYMBOL(gpio_is_valid);

int gpio_direction_input(unsigned gpio)
{
    if (!gpio_is_valid(gpio))
        return -1;

	HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_I_DIR);
	HAL_GPIO_BIT_ENABLE(gpio, HAL_GPIO_ENABLE);

	return 0;
}
EXPORT_SYMBOL(gpio_direction_input);

int gpio_direction_output(unsigned gpio, int value)
{
    if (!gpio_is_valid(gpio))
        return -1;

    HAL_GPIO_BIT_DIR_SET(gpio, HAL_GPIO_O_DIR);
	HAL_GPIO_BIT_ENABLE(gpio, HAL_GPIO_ENABLE);

    /* if the value is nither 0 nor 1, then don't set the value */
	if(value == 0)
		HAL_GPIO_BIT_SET(gpio, 0);
	else if (value == 1)
		HAL_GPIO_BIT_SET(gpio, 1);

	return 0;
}
EXPORT_SYMBOL(gpio_direction_output);

int gpio_get_value(unsigned gpio)
{
	int val;

    if (!gpio_is_valid(gpio))
        return -1;

	val = HAL_GPIO_BIT_GET(gpio);

	return val;
}
EXPORT_SYMBOL(gpio_get_value);

int gpio_set_value(unsigned gpio, int value)
{
    if (!gpio_is_valid(gpio))
        return -1;

	if(value)
		HAL_GPIO_BIT_SET(gpio, 1);
	else
		HAL_GPIO_BIT_SET(gpio, 0);

    return 0;
}
EXPORT_SYMBOL(gpio_set_value);

int gpio_request(unsigned gpio, const char *label)
{
    if (!gpio_is_valid(gpio))
        return -1;
    
    HAL_GPIO_FUNC_ENABLE(gpio, HAL_GPIO_ENABLE);
    return 0;
}
EXPORT_SYMBOL(gpio_request);

void gpio_free(unsigned gpio)
{
	if (!gpio_is_valid(gpio))
	    return ;

	HAL_GPIO_FUNC_ENABLE(gpio, HAL_GPIO_DISABLE);
	return ;
}
EXPORT_SYMBOL(gpio_free);

int gpio_get_interrupt_status(unsigned gpio)
{
    if( !gpio_is_valid(gpio))
        return -1;

    return HAL_GPIO_INT_STA_GET(gpio);
}
EXPORT_SYMBOL(gpio_get_interrupt_status);

//clear interrupt status
int gpio_clear_interrupt_status(unsigned gpio)
{
    if( !gpio_is_valid(gpio))
        return -1;

    HAL_GPIO_INT_CLEAR(gpio);
	return 0;
}
EXPORT_SYMBOL(gpio_clear_interrupt_status);

int gpio_interrupt_enable(unsigned gpio,int rise_en, int fall_en)
{
    if( !gpio_is_valid(gpio))
        return -1;

    HAL_GPIO_INT_SET(gpio, 1);
    HAL_GPIO_INT_EDG_SET(gpio,rise_en,fall_en);
	return 0;
}
EXPORT_SYMBOL(gpio_interrupt_enable);

int gpio_interrupt_disable(unsigned gpio)
{
    if( !gpio_is_valid(gpio))
        return -1;

    HAL_GPIO_INT_SET(gpio, 0);
	return 0;
}
EXPORT_SYMBOL(gpio_interrupt_disable);


MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI GPIO driver");
MODULE_LICENSE("GPL");

