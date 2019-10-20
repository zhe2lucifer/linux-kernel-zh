/*
 * CERT utils for ASA and AKL
 *
 * Copyright (c) 2015 Youri Zhang
 *
 * This file is released under the GPLv2
 */

#ifndef _CASI_UTILS_H
#define _CASI_UTILS_H
#include "as_attr_core.h"

/* utils */
int as_attr_umemcpy(void *dest, const void *src, __u32 n);
int as_attr_get_kl_res(int kl_fd, struct as_attr_device *as_attr_dev);
int as_attr_get_dsc_res(int fd, struct as_attr_device *as_attr_dev);
__u32 fetch_dsc_real_dev(__u32 dev_hdl);

#endif