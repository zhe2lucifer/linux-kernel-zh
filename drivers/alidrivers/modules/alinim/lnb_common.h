#ifndef __LNB_COMMON__H__
#define __LNB_COMMON__H__

#include "basic_types.h" 

#ifdef CONFIG_LNB_TPS65233
#include "lnb_tps65233/lnb_tps65233.h"
#endif

#ifdef CONFIG_LNB_A8304
#include "lnb_a8304/lnb_a8304.h"
#endif

/*******************lnb param area**********************/
typedef  INT32(*lnb_command) (UINT32*,UINT32,UINT32);

struct LNB_IO_FUN
{
	UINT32 lnb_type;
	lnb_command pf_command;

};
/***************lnb common interface fun**************/
struct LNB_IO_FUN *lnb_setup(UINT32 lnb_type);


/*****************lnb cmd define*****************/
#define LNB_CMD_BASE			0xf0
#define LNB_CMD_ALLOC_ID		(LNB_CMD_BASE+1)
#define LNB_CMD_INIT_CHIP		(LNB_CMD_BASE+2)
#define LNB_CMD_SET_POLAR		(LNB_CMD_BASE+3)
#define LNB_CMD_POWER_ONOFF		(LNB_CMD_BASE+4)
#define LNB_CMD_RELEASE_CHIP    (LNB_CMD_BASE+5)
#define LNB_CMD_GET_OCP         (LNB_CMD_BASE+6)
#define LNB_CMD_CURRENT_LIMIT_CONTROL	(LNB_CMD_BASE+7)

#define LNB_A8304_REG_OCP        0x04
#define OCP_HAPPEN 				 0x02

/**********************************************/




#endif
