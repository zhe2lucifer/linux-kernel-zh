/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced verification file status.
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

#ifndef _SECURITY_ALIASIX_STATUS_H_
#define _SECURITY_ALIASIX_STATUS_H_

/*
 * ALiasix header
 */
#include "aliasix.h"
#include "aliasix_misc.h"

/* Permission items */
typedef struct _aliasix_file_sha_status
{
    u8      b_valid : 1;
    u8      b_used : 1;
    u8      b_verified : 1;
    u8      b_sig_split : 1;
    u8      u5_reserved : 4;
    int     i_dir_idx;
    char    a_filename[NAME_MAX];
    u_long  u32_expire;
}aliasix_file_sha_status;

/*
 * Check if we have stored status of the given file
 */
extern int aliasix_perm_permission(struct file *file);
extern int aliasix_perm_reset_permission(struct file *file);
extern void aliasix_perm_add_permission_item(struct file *file, 
                         aliasix_file_sha_status *file_sha_status);
extern void aliasix_perm_lock_init(void);
extern int aliasix_perm_get_expire_file(char *p_path);
#ifndef ALLOC_PERM_ITEMS_STATIC_ALIASIX
extern void aliasix_perm_items_init(void);
#endif
extern void aliasix_perm_set_rootfs_retire(void *unused);
extern int aliasix_perm_is_rootfs_retire(void);
extern void aliasix_perm_rootfs_retire_thread(void *unused);
extern unsigned long  aliasix_perm_show_status(char *page, char **start,
                            		       off_t off, int count, 
                            		       int *eof, void *data);
extern int aliasix_perm_sig_split(struct file *file, char *f_dir, char *f_name);

#endif
