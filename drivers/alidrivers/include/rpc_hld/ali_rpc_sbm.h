/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#ifndef __DRIVERS_ALI_RPC_SBM_H
#define __DRIVERS_ALI_RPC_SBM_H

#include "ali_rpc_hld.h"
#include <ali_sbm_common.h>

enum LLD_SBM_FUNC{
    FUNC_SBM_SEE_CREATE = 0,
    FUNC_SBM_SEE_DESTROY,
    FUNC_SBM_CREATE,
    FUNC_SBM_DESTROY,
};

struct sbm_desc_rpc
{
    struct sbm_config sbm_cfg;
    UINT32 sbm_rw;
    INT32 mutex;
    UINT8 status;
};

int sbm_see_create(int sbm_idx, int sbm_mode, void *sbm_init);
int sbm_see_destroy(int sbm_idx, int sbm_mode);

#endif
