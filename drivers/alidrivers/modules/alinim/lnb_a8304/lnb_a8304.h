#ifndef __LNB_A8304_H__
#define __LNB_A8304_H__

#include <linux/mutex.h>
#include <linux/types.h>
#include "dvb_frontend_common.h"
#include "../basic_types.h"


//#define LNB_DEBUG
//#define RD_LNB_DEBUG
#ifdef LNB_DEBUG
#define lnb_printk 	printk
#else
#define lnb_printk(...)
#endif 
/****************************************/
/*  control and status reg is same address :0x0 */
/*          0x0 is reg of control when written       */
/*          0x0 is reg of status when read           */
/****************************************/
#define CONTROL_REG 0x0
#define STATUS_REG  0x0
#define POWER_ON    0x01
#define POWER_OFF   0x00

struct LNB_A8304_CONTROL
{
    UINT32 			i2c_base_addr;
    UINT32 			i2c_type_id;
    struct mutex 	lnb_mutex;
    struct mutex 	i2c_mutex;
    UINT8   		gpio_en;
    UINT8   		gpio_polar;
    UINT8   		gpio_num;
    UINT8   		current_polar;
    UINT8   		power_en;
	UINT8           init_flag;
};

//interface
INT32 lnb_a8304_command(UINT32* lnb_ld,UINT32 cmd,UINT32 param);

#endif
