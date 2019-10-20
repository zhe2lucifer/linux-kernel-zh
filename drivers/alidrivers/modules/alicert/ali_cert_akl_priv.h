/*
 * Driver structures for CERT AKL
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_AKL_PRIV_H
#define _ALI_CERT_AKL_PRIV_H

#include "ali_cert_akl_rpc.h"
#include "ali_cert_utils.h"
#include "ali_cert_priv.h"

#define CERT_AKL_DEV "ca_akl"
#define CERT_AKL_KEY_DEV "ca_akl_key"
#define CERT_AKL_INVALID_VALUE ((int)-1)

#define CERT_AKL_KEY_MAX (32)

struct cert_akl_drv {
	struct semaphore sem;
	dev_t devt;
	struct device *dev;
	struct class *dev_class;
	struct cdev cdev;
	int see_sess; /* handle/chan for operating the SEE of cert*/
	struct see_client *see_clnt;
	const struct cert_akl_see_ops *see_ops;
	struct kl_fd_operations fd_ops;
	atomic_t key_c; /* opening akl_key count */
	struct cert_driver *parent;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
#endif
};

struct cert_akl_key {
	int algo;

	struct kl_key_cell *cell;
	struct cert_akl_drv *drv;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
#endif
};

int cert_akl_probe(struct cert_driver *parent);
int cert_akl_remove(struct cert_driver *parent);

#endif

