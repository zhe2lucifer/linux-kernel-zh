/*
 * RPC interface for CERT ASA
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
#include <linux/dma-mapping.h>

#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>

#include "ali_cert_asa_rpc.h"
#include "ali_cert_asa_priv.h"

static struct see_client *cert_asa_see_client;

static inline struct see_client *clnt(void)
{
	return cert_asa_see_client;
}

static int see_cert_asa_open_rpc(int sbm_id)
{
	jump_to_func(NULL, ali_rpc_call, sbm_id, \
		CERT_NPARA(1)|CERT_RPC_ASA_OPEN, \
		NULL);
}

static int see_cert_asa_open(int sbm_id)
{
	int ret = -1;

	ret = see_cert_asa_open_rpc(sbm_id);

	return cert_uret(ret);
}

static int see_cert_asa_close_rpc(int sess)
{
	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(1)|CERT_RPC_ASA_CLOSE, \
		NULL);
}

static int see_cert_asa_close(int sess)
{
	int ret = -1;

	ret = see_cert_asa_close_rpc(sess);

	return cert_uret(ret);
}

static int see_cert_asa_setfmt_rpc(int sess, int fmt)
{
	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(2)|CERT_RPC_ASA_SETFMT, \
		NULL);
}

static int see_cert_asa_setfmt(int sess, int fmt)
{
	int ret = -1;

	ret = see_cert_asa_setfmt_rpc(sess, fmt);

	return cert_uret(ret);
}

static int see_cert_asa_addpid_rpc(int sess, struct cert_asa_pid *see_pid)
{
	u32 cert_asa_addpid_desc[] =
	{ //desc of pointer para
		1, DESC_STATIC_STRU(0, sizeof(struct cert_asa_pid)),
		1, DESC_P_PARA(0, 1, 0),
		//desc of pointer ret
		0,
		0,
	};

	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(2)|CERT_RPC_ASA_ADDPID, \
		cert_asa_addpid_desc);
}

static int see_cert_asa_addpid(int sess, int pos, __u8 ltsid, __u16 pid)
{
	int ret = -1;
	struct cert_asa_pid see_pid;

	memset(&see_pid, 0x00, sizeof(see_pid));
	see_pid.pos = pos;
	see_pid.pid = pid;
	see_pid.ltsid = ltsid;

	ret = see_cert_asa_addpid_rpc(sess, &see_pid);

	return cert_uret(ret);
}

static int see_cert_asa_delpid_rpc(int sess, struct cert_asa_pid *see_pid)
{
	u32 cert_asa_delpid_desc[] =
	{ //desc of pointer para
		1, DESC_STATIC_STRU(0, sizeof(struct cert_asa_pid)),
		1, DESC_P_PARA(0, 1, 0),
		//desc of pointer ret
		0,
		0,
	};

	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(2)|CERT_RPC_ASA_DELPID, \
		cert_asa_delpid_desc);
}

static int see_cert_asa_delpid(int sess, __u8 ltsid, __u16 pid)
{
	int ret = -1;
	struct cert_asa_pid see_pid;

	memset(&see_pid, 0x00, sizeof(see_pid));
	see_pid.pid = pid;
	see_pid.ltsid = ltsid;

	ret = see_cert_asa_delpid_rpc(sess, &see_pid);

	return cert_uret(ret);
}

static int see_cert_asa_decrypt_rpc(int sess, __u8 *input,
	__u8 *output, __u32 length)
{
	u32 cert_asa_decrypt_desc[] =
	{ //desc of pointer para
		2, DESC_STATIC_STRU(0, length), DESC_OUTPUT_STRU(0, length),
		2, DESC_P_PARA(0, 1, 0), DESC_P_PARA(0, 2, 1),
		//desc of pointer ret
		0,
		0,
	};

	jump_to_func(NULL, ali_rpc_call, sess, \
		CERT_NPARA(4)|CERT_RPC_ASA_DECRYPT, \
		cert_asa_decrypt_desc);
}

static int see_cert_asa_decrypt(int sess, __u8 *input,
	__u8 *output, __u32 length)
{
	int ret = -1;

	get_dma_ops(NULL)->sync_single_for_device(NULL,
						virt_to_phys(input), length,
						DMA_TO_DEVICE);

	get_dma_ops(NULL)->sync_single_for_device(NULL,
						virt_to_phys(output), length,
						DMA_BIDIRECTIONAL);

	ret = see_cert_asa_decrypt_rpc(sess, input,
		output, length);

	return cert_uret(ret);
}

static const struct cert_asa_see_ops see_cert_asa_ops = {
	.open = see_cert_asa_open,
	.close = see_cert_asa_close,
	.setfmt = see_cert_asa_setfmt,
	.addpid = see_cert_asa_addpid,
	.delpid = see_cert_asa_delpid,
	.decrypt = see_cert_asa_decrypt,
};

int see_cert_asa_register_rpc(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		CERT_NPARA(0)|CERT_RPC_ASA_ATTACH, \
		NULL);
}

int see_cert_asa_register(void *data)
{
	int ret = -1;
	struct cert_asa_drv *drv = (struct cert_asa_drv *)data;

	drv->see_ops = &see_cert_asa_ops;
	cert_asa_see_client = drv->see_clnt;

	ret = see_cert_asa_register_rpc();

	return cert_uret(ret);
}

int see_cert_asa_unregister_rpc(void)
{
	jump_to_func(NULL, ali_rpc_call, NULL, \
		CERT_NPARA(0)|CERT_RPC_ASA_DETACH, \
		NULL);
}

int see_cert_asa_unregister(void *data)
{
	int ret = -1;
	struct cert_asa_drv *drv = (struct cert_asa_drv *)data;

	ret = see_cert_asa_unregister_rpc();
	drv->see_ops = NULL;
	cert_asa_see_client = NULL;

	return cert_uret(ret);
}

