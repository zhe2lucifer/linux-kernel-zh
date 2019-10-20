/*
 * Debugfs for CERT ASA
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_ASA_DBGFS_H
#define _ALI_CERT_ASA_DBGFS_H

#include "ali_cert_asa_priv.h"

#ifdef CONFIG_DEBUG_FS
void cert_asa_dbgfs_create(struct cert_asa_drv *drv);
void cert_asa_dbgfs_remove(struct cert_asa_drv *drv);
int cert_asa_dbgfs_add(struct cert_asa_session *sess);
void cert_asa_dbgfs_del(struct cert_asa_session *sess);
#else
#define cert_asa_dbgfs_create(...)
#define cert_asa_dbgfs_remove(...)
#define cert_asa_dbgfs_add(...)
#define cert_asa_dbgfs_del(...)
#endif

#endif /* _ALI_CERT_ASA_DBGFS_H */

