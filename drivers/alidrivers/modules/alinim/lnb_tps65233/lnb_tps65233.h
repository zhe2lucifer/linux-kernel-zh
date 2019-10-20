#ifndef __TPS65233_H__
#define __TPS65233_H__

#include <linux/mutex.h>
#include <linux/types.h>
#include "dvb_frontend_common.h"
#include "../basic_types.h"

//#define LNB_DEBUG
//#define RD_LNB_DEBUG
#ifdef LNB_DEBUG
#define LNB_PRINTF   printk
#else
#define LNB_PRINTF(...)
#endif

#define REG_CONTROL_1	0x00
#define REG_CONTROL_2	0x01
#define REG_STATUS		0x02
#define POWER_ON    0x01
#define POWER_OFF   0x00
#define CURRENT_LIMIT_ON  0x01
#define CURRENT_LIMIT_OFF 0x00

#define MAX_LNB_TPS65233   2



struct LNB_TPS65233_CONTROL
{
    UINT32 i2c_base_addr;
    UINT32 i2c_type_id;
    struct mutex  lnb_mutex;
    struct mutex  i2c_mutex;
    UINT8   int_gpio_en;
    UINT8   gpio_polar;
    UINT8   gpio_num;
    UINT8   current_polar;
    UINT8   power_en;
	UINT8   init_flag;
};

INT32 lnb_tps65233_command(UINT32 *lnb_id, UINT32 cmd, UINT32 param);



#endif
