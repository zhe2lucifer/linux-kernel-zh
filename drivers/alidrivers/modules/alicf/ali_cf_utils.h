/*
 * CF utils
 *
 * Copyright (c) 2016 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CF_UTILS_H
#define _ALI_CF_UTILS_H

/* utils */
int cf_umemcpy(void *dest, const void *src, __u32 n);
int cf_uret(int rpc_ret);

#endif

