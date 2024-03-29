/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced verification implementations.
 *
 *  Author:
 *	Zhao Owen <owen.zhao@alitech.com>
 *
 *  Copyright (C) 2011 Zhao Owen <owen.zhao@alitech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2,
 *      as published by the Free Software Foundation.
 */

#ifndef _SECURITY_ALIASIX_SHA_H_
#define _SECURITY_ALIASIX_SHA_H_

/*
 * ALiasix header
 */
#include "aliasix.h"
#include "aliasix_rsa.h"
#include "aliasix_perm.h"

/*  
 * Do digest for the given data
 */
extern int aliasix_sha_verify_ex(struct file *file, u8 *p_file_buf, \
                                  u32 u32_file_size);
extern int aliasix_sha_verify(struct file *file, \
                              aliasix_sha_digest_info *sha_digest_info);
extern void aliasix_sha_device_sem_init(void);
extern void aliasix_sha_mutex_init(void);
extern void aliasix_sha_read_lock(void);
extern void aliasix_sha_read_unlock(void);

#endif
