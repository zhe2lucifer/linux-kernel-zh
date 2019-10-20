/*
 * RPC interface for CERT AKL
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>

#include "ali_cert_akl_rpc.h"
#include "ali_cert_akl_priv.h"

static struct see_client *cert_akl_see_client;

static inline struct see_client *clnt(void)
{
	return cert_akl_see_client;
}

static int see_cert_akl_open_rpc(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		CERT_NPARA(0)|CERT_RPC_AKL_OPEN, \
		NULL);
}

static int see_cert_akl_open(void)
{
	int ret = -1;

	ret = see_cert_akl_open_rpc();
	return cert_uret(ret);
}

static int see_cert_akl_close_rpc(int sess)
{
	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(1)|CERT_RPC_AKL_CLOSE, \
		NULL);
}

static int see_cert_akl_close(int sess)
{
	int ret = -1;

	ret = see_cert_akl_close_rpc(sess);
	return cert_uret(ret);
}

static int see_cert_akl_exchange_rpc(int sess, void *data)
{
	u32 cert_akl_exchange_desc[] =
	{ //desc of pointer para
		1, DESC_OUTPUT_STRU(0, sizeof(struct cert_akl_cmd)),
		1, DESC_P_PARA(0, 1, 0),
		//desc of pointer ret
		0,
		0,
	};

	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(2)|CERT_RPC_AKL_EXCHANGE, \
		cert_akl_exchange_desc);
}

static int see_cert_akl_exchange(int sess, void *data)
{
	int ret = -1;

	ret = see_cert_akl_exchange_rpc(sess, data);
	return cert_uret(ret);
}

static int see_cert_akl_savekey_rpc(int sess, void *param)
{
	u32 cert_akl_savekey_desc[] =
	{ //desc of pointer para
		1, DESC_STATIC_STRU(0, sizeof(struct cert_akl_savekey)),
		1, DESC_P_PARA(0, 1, 0),
		//desc of pointer ret
		0,
		0,
	};

	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(2)|CERT_RPC_AKL_SAVEKEY, \
		cert_akl_savekey_desc);
}

static int see_cert_akl_savekey(int sess, int algo,
		int pos, int parity)
{
	int ret = -1;
	struct cert_akl_savekey param;

	memset(&param, 0x00, sizeof(param));

	param.pos = pos;
	param.algo = algo;
	param.parity = parity;

	ret = see_cert_akl_savekey_rpc(sess, (void *)&param);

	return cert_uret(ret);
}

static void see_cert_akl_ack(int sess)
{
	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(1)|CERT_RPC_AKL_ACK, \
		NULL);
}

static int see_cert_akl_alloc_rpc(int kl_sel, int nr)
{
	jump_to_func(NULL, ali_rpc_call, kl_sel, \
		CERT_NPARA(2)|CERT_RPC_AKL_ALLOC, \
		NULL);
}

static int see_cert_akl_alloc(int kl_sel, int nr)
{
	int ret = -1;

	ret = see_cert_akl_alloc_rpc(kl_sel, nr);
	return cert_uret(ret);
}

static int see_cert_akl_free_rpc(int kl_sel, int pos)
{
	jump_to_func(NULL, ali_rpc_call, kl_sel, \
		CERT_NPARA(2)|CERT_RPC_AKL_FREE, \
		NULL);
}

static int see_cert_akl_free(int kl_sel, int pos)
{
	int ret = -1;

	ret = see_cert_akl_free_rpc(kl_sel, pos);
	return cert_uret(ret);
}

static const struct cert_akl_see_ops see_cert_akl_ops = {
	.open = see_cert_akl_open,
	.close = see_cert_akl_close,
	.exchange = see_cert_akl_exchange,
	.savekey = see_cert_akl_savekey,
	.ack = see_cert_akl_ack,
	.alloc = see_cert_akl_alloc,
	.free = see_cert_akl_free,
};

static int see_cert_akl_register_rpc(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		CERT_NPARA(0)|CERT_RPC_AKL_ATTACH, \
		NULL);
}

int see_cert_akl_register(void *data)
{
	int ret = -1;
	struct cert_akl_drv *drv = data;

	drv->see_ops = &see_cert_akl_ops;
	cert_akl_see_client = drv->see_clnt;

	ret = see_cert_akl_register_rpc();

	return cert_uret(ret);
}

static int see_cert_akl_unregister_rpc(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		CERT_NPARA(0)|CERT_RPC_AKL_DETACH, \
		NULL);
}

int see_cert_akl_unregister(void *data)
{
	int ret = -1;
	struct cert_akl_drv *drv = data;

	ret = see_cert_akl_unregister_rpc();

	drv->see_ops = NULL;
	cert_akl_see_client = NULL;

	return cert_uret(ret);
}

