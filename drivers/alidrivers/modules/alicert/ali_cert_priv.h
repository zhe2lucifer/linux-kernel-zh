/*
 * Platform structures for ASA and AKL
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_PRIV_H
#define _ALI_CERT_PRIV_H

#include <alidefinition/adf_cert.h>
#include <see_bus.h>
#include <ca_asa.h>
#include <ca_akl.h>
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

#define CERT_NPARA(x) ((HLD_CERT_MODULE << 24)|(x << 16))

struct cert_plat_data {
	int is_cert_disabled; /* asa and akl both been disabled*/
	int is_asa_disabled; /* asa is disabled */
	struct clk *clk; /* enable cert clk */
};

struct cert_driver {
	void *asa_drv;
	void *akl_drv;
	struct cert_plat_data *pdata;
	struct see_client *clnt;
#ifdef CONFIG_DEBUG_FS
	struct dentry *choice;
	u32 reserve;
#endif
};

#endif

