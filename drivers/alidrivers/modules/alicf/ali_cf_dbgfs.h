/*
 * Debugfs for CF
 *
 * Copyright (c) 2016 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CF_DBGFS_H
#define _ALI_CF_DBGFS_H

#include "ali_cf_priv.h"

void cf_dbgfs_create(struct cf_drv *drv);
void cf_dbgfs_remove(struct cf_drv *drv);

#endif /* _ALI_CF_DBGFS_H */

