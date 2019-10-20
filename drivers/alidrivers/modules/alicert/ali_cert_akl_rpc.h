/*
 * RPC interface for CERT AKL
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */


#ifndef _ALI_CERT_AKL_RPC_H
#define _ALI_CERT_AKL_RPC_H

struct cert_akl_see_ops {
	int (*open)(void);
	int (*close)(int sess);
	int (*exchange)(int sesss, void *data);
	int (*savekey)(int sess, int algo, int pos, int parity);
	void (*ack)(int sess);
	int (*alloc)(int kl_sel, int nr);
	int (*free)(int kl_sel, int pos);
};

int see_cert_akl_register(void *data);
int see_cert_akl_unregister(void *data);

#endif

