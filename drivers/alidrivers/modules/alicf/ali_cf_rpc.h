/*
 * RPC interface for CF
 *
 * Copyright (c) 2016 ALi Corp
 *
 * This file is released under the GPLv2
 */


#ifndef _ALI_CF_RPC_H
#define _ALI_CF_RPC_H

#define CF_NPARA(x) ((HLD_CF_MODULE<<24)|(x<<16))

enum CF_RFUNC
{
	FUNC_CF_ATTACH = 0,
	FUNC_CF_DETACH,
	FUNC_CF_OPEN,
	FUNC_CF_CLOSE,
	FUNC_CF_IOCTL,
	FUNC_CF_WRITE,
	FUNC_CF_READ,
	FUNC_CF_SET_TARGET,
/*
	FUNC_CF_SUSPEND,
	FUNC_CF_RESUME,
*/
};

struct cf_see_ops {
	int (*open)(void);
	int (*close)(int chan);
	int (*ioctl)(int chan, int cmd, void *param);
	int (*write)(int chan, void *buf, int count);
	int (*read)(int chan, void *buf, int count);
	int (*set_target)(int pos);
};

int cf_see_register(void *data);
int cf_see_unregister(void *data);

#endif

