/*
 * CERT utils for ASA and AKL
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_UTILS_H
#define _ALI_CERT_UTILS_H

/* utils */
int cert_umemcpy(void *dest, const void *src, __u32 n);
int cert_uret(int rpc_ret);

#endif

