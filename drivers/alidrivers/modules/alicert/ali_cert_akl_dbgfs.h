/*
 * Debugfs for CERT AKL
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_AKL_DBGFS_H
#define _ALI_CERT_AKL_DBGFS_H

#include "ali_cert_akl_priv.h"

#ifdef CONFIG_DEBUG_FS
void cert_akl_dbgfs_create(struct cert_akl_drv *drv);
void cert_akl_dbgfs_remove(struct cert_akl_drv *drv);
int cert_akl_dbgfs_add(struct cert_akl_key *key);
void cert_akl_dbgfs_del(struct cert_akl_key *key);
#else
#define cert_akl_dbgfs_create(...)
#define cert_akl_dbgfs_remove(...)
#define cert_akl_dbgfs_add(...)
#define cert_akl_dbgfs_del(...)
#endif

#endif /* _ALI_CERT_AKL_DBGFS_H */
