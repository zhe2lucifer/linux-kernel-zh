/*****************************************************************************
*    Copyright (C)2017 Ali Corporation. All Rights Reserved.
*
*    File:    lnb_a8304.c
*
*    Description: a8304 lnb driver
*    History:
*                   Date                      Athor            Version                 Reason
*        ============       =========     ======     ================
*    1.     2017-02-20          	  Leo.liu           Ver 1.0                Create file.
*   
*****************************************************************************/
#include "lnb_common.h"
#include "porting_linux_header.h"

static struct LNB_IO_FUN lnb_array[] =
{
#ifdef CONFIG_LNB_A8304
	{
		LNB_A8304,
		(lnb_command)lnb_a8304_command
	},
#endif
#ifdef CONFIG_LNB_TPS65233
	{
		LNB_TPS65233,
		(lnb_command)lnb_tps65233_command
	},
#endif
};

struct LNB_IO_FUN  *lnb_setup(UINT32 lnb_type)
{
	UINT8 lnb_nu = 0,i = 0;
	lnb_nu = sizeof(lnb_array)/sizeof(struct LNB_IO_FUN);
	for(i = 0;i<lnb_nu;i++)
	{
		if(lnb_array[i].lnb_type == lnb_type)
		{
			return &lnb_array[i];
		}
	}
	
	return NULL;
};



