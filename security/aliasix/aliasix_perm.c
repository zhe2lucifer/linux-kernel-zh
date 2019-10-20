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

#include "aliasix_perm.h"

/* Path status of the signed files */
typedef struct _aliasix_path_status
{
    u8      b_used : 1;
    u8      u7_reserved : 7;
    char    a_pathname[PATH_MAX];
}aliasix_path_status;

/* The file signature path number */
#define ALIASIX_MAX_RO_DIRECTORIES  256
#define ALIASIX_MAX_ROOTFS_NUM      32

/* To check the ramfs file status */
typedef struct _aliasix_rootfs_file_status
{
    u8      b_data_done : 1;
    u8      b_used : 1;
    u8      b_verified : 1;
    u8      b_valid : 1;
    u8      u4_reserved : 4;
    char    a_pathname[PATH_MAX];
}aliasix_rootfs_file_status;

struct aliasix_rootfs_status
{
    u8      b_rootfs_retire;
    aliasix_rootfs_file_status rootfs_file_status[ALIASIX_MAX_ROOTFS_NUM];
};

static struct aliasix_rootfs_status rootfs_status = 
{
    .b_rootfs_retire = 0,
};

/* We reserved 4M memory for the file status */
#define ALIASIX_MAX_RO_FILE_NUMBER ((4 * M) / NAME_MAX)

/* Mutex to protect the file attribute */
static struct mutex aliasix_directory_mutex;
static struct mutex aliasix_file_status_mutex;

/* Limit of the exception file index */
#define ALIASIX_FILE_NO_DIGEST_INDEX    35//30
#define ALIASIX_PATH_NO_DIGEST_INDEX    8//7

/* We need a fix place for verify when upgrading */
#define ALIASIX_UPGRADE_VERIFY_ROOT		"/mnt/verifyFS"
#define ALIASIX_UPGRADE_VERIFY_SIMPLE	"verifyFS"

/* Macro to cast the path */
#define ALIASIX_PERM_CAST_DIRNAME(dst, src, file) \
    do { \
        if (strlen(src) == strlen(file->f_path.dentry->d_name.name) + 1) \
            strcpy(dst, ALIASIX_ROOT_DIR); \
        else \
        { \
            strcpy(dst, src); \
            dst[strlen(src) - strlen(file->f_path.dentry->d_name.name) - 1] = '\0'; \
        } \
    } while (0)

#define ALIASIX_PATH_FREE_NUMBER		32

#ifdef ALLOC_PERM_ITEMS_STATIC_ALIASIX
/* Global status to store the file sha status */
static aliasix_path_status a_aliasix_path_status[ALIASIX_MAX_RO_DIRECTORIES] = 
{
    {1, 0, "/etc"},
    {1, 0, "/etc/network"},
    {1, 0, "/stb/etc"},
    {1, 0, "/web/etc"},
    {1, 0, ALIASIX_UPGRADE_VERIFY_ROOT"/etc"},
    {1, 0, ALIASIX_UPGRADE_VERIFY_ROOT"/etc/network"},
    [4 ... ALIASIX_MAX_RO_DIRECTORIES - 1] = {
        .b_used = 0,
        .a_pathname = {
            [0 ... PATH_MAX - 1] = {0}
        },
    }
};

static aliasix_file_sha_status a_aliasix_file_sha_status[ALIASIX_MAX_RO_FILE_NUMBER] = 
{
    /* Valid Used Verified Sig_Split Reserved */
    {1, 1, 0, 1, 0, 0, "fstab", 3 * S},
    {1, 1, 0, 1, 0, 0, "mdev.conf", 3 * S},
    {1, 1, 0, 1, 0, 0, "inittab", 3 * S},
    {1, 1, 0, 1, 0, 0, "directfbrc", 3 * S},
    {1, 1, 0, 1, 0, 0, "sysctl.conf", 3 * S},
    {1, 1, 0, 1, 0, 0, "passwd", 3 * S},
    {1, 1, 0, 1, 0, 0, "shadow", 3 * S},
    {1, 1, 0, 1, 0, 0, "group", 3 * S},
    {1, 1, 0, 1, 0, 0, "hostname", 3 * S},
    {1, 1, 0, 1, 0, 1, "interfaces", 3 * S},
    {1, 1, 0, 1, 0, 2, "passwd", 3 * S},
    {1, 1, 0, 1, 0, 2, "shadow", 3 * S},
    {1, 1, 0, 1, 0, 2, "group", 3 * S},
    {1, 1, 0, 1, 0, 2, "fstab", 3 * S},
    {1, 1, 0, 1, 0, 2, "directfbrc", 3 * S},
    {1, 1, 0, 1, 0, 3, "passwd", 3 * S},
    {1, 1, 0, 1, 0, 3, "shadow", 3 * S},
    {1, 1, 0, 1, 0, 3, "group", 3 * S},
    {1, 1, 0, 1, 0, 3, "directfbrc", 3 * S},
	[19 ... ALIASIX_MAX_RO_FILE_NUMBER - 1] = {
		.b_valid = 0,
        .b_used = 0,
        .i_dir_idx = ALIASIX_INVALID_INDEX,
        .b_verified = 0,
        .u32_expire = ALIASIX_LIVE_FOREVER,
        .a_filename = {
            [0 ... NAME_MAX - 1] = {0}
        },
	}
};

static aliasix_path_status a_aliasix_path_free[ALIASIX_PATH_FREE_NUMBER] = 
{
	{1, 0, "/tmp/userfs/opt_hbbtv/opera_free"}, 
	{1, 0, "/version"},
    {1, 0, "/stb/version"},
	{3 ... ALIASIX_PATH_FREE_NUMBER - 1} = {
		.b_used = 0,
		.a_pathname = {
			[0 ... PATH_MAX - 1] = {0}
		},
	} 
};

static aliasix_path_status a_aliasix_path_wo_record[ALIASIX_PATH_FREE_NUMBER] = 
{
    {1, 0, "/usr/share/zoneinfo"}, 
	{1 ... ALIASIX_PATH_FREE_NUMBER - 1} = {
		.b_used = 0,
		.a_pathname = {
			[0 ... PATH_MAX - 1] = {0}
		},
	} 
};

#else
static aliasix_path_status a_aliasix_path_status[ALIASIX_MAX_RO_DIRECTORIES];
static aliasix_file_sha_status a_aliasix_file_sha_status[ALIASIX_MAX_RO_FILE_NUMBER];
static aliasix_path_status a_aliasix_path_free[ALIASIX_PATH_FREE_NUMBER]; 
static aliasix_path_status a_aliasix_path_wo_record[ALIASIX_PATH_FREE_NUMBER];
#endif

#define a_aliasix_path_sig_split a_aliasix_path_free

/* Macro to get the file directory */
#define ALIASIX_PERM_DIRECTORY_GET(idx, i_path_idx, p_path) \
    { \
        if (a_aliasix_path_status[idx].b_used && \
            (0 == strcmp(p_path, a_aliasix_path_status[idx].a_pathname))) \
        { \
            ALIASIX_PATH("Info, path %s %s\n", p_path,  \
                          a_aliasix_path_status[idx].a_pathname); \
            i_path_idx = idx; \
            break; \
        } \
        if (!a_aliasix_path_status[idx].b_used) \
            break; \
    }

/*
 * Item updated
 */
#define ALIASIX_FILE_STATUS_UPDATE(idx, file_sha_status) \
    do { \
        a_aliasix_file_sha_status[idx].b_valid = file_sha_status->b_valid; \
        a_aliasix_file_sha_status[idx].b_verified = file_sha_status->b_verified; \
        a_aliasix_file_sha_status[idx].u32_expire = file_sha_status->u32_expire; \
    } while (0)

/*
 * rootfs items inited
 */
#define ALIASIX_ROOTFS_FILE_DATA_DONE(idx) \
    do { \
        rootfs_status.rootfs_file_status[idx].b_data_done = 1; \
        rootfs_status.rootfs_file_status[idx].b_valid = 0; \
        rootfs_status.rootfs_file_status[idx].b_verified = 0; \
    } while (0)

/*
 * How long the rootfs will be retire in seconds
 */
#define ALIASIX_PERM_ROOTFS_RETIRE_DELAY    180

/*
 * Initialization of the perm items
 */
#ifndef ALLOC_PERM_ITEMS_STATIC_ALIASIX
void aliasix_perm_items_init(void)
{
    int i = 0;
    /* 
     * We can't use a integer to assign the value here 
     * to avoid some bugs of the endian error
     */

    /*
     * In those directories, specific files signature will be splited
     */
    memset(a_aliasix_path_status, 0x00, sizeof(a_aliasix_path_status));
    a_aliasix_path_status[0].b_used = 1;
    a_aliasix_path_status[1].b_used = 1;
    a_aliasix_path_status[2].b_used = 1;
    a_aliasix_path_status[3].b_used = 1;
    a_aliasix_path_status[4].b_used = 1;
    a_aliasix_path_status[5].b_used = 1;
    a_aliasix_path_status[6].b_used = 1;
    a_aliasix_path_status[7].b_used = 1;
    a_aliasix_path_status[8].b_used = 1;
    strcpy(a_aliasix_path_status[0].a_pathname, "/etc");
    strcpy(a_aliasix_path_status[1].a_pathname, "/etc/network");
    strcpy(a_aliasix_path_status[2].a_pathname, "/stb/etc");
    strcpy(a_aliasix_path_status[3].a_pathname, "/web/etc");
    strcpy(a_aliasix_path_status[4].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
    strcat(a_aliasix_path_status[4].a_pathname, "/etc");
    strcpy(a_aliasix_path_status[5].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
    strcat(a_aliasix_path_status[5].a_pathname, "/etc/network");
    strcpy(a_aliasix_path_status[6].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
    strcat(a_aliasix_path_status[6].a_pathname, "/usr_lib/webkit/js");
    strcpy(a_aliasix_path_status[7].a_pathname, "/ufs/usr_lib/webkit/js");
    strcpy(a_aliasix_path_status[8].a_pathname, "/ott/etc");

    /*
     * In those directories, all files signature will be splited
     */
	memset(a_aliasix_path_free, 0x00, sizeof(a_aliasix_path_free));
	a_aliasix_path_free[0].b_used = 1;
	a_aliasix_path_free[1].b_used = 1;
    a_aliasix_path_free[2].b_used = 1;
    a_aliasix_path_free[3].b_used = 1;
    a_aliasix_path_free[4].b_used = 1;
    a_aliasix_path_free[5].b_used = 1;
    a_aliasix_path_free[6].b_used = 1;
    a_aliasix_path_free[7].b_used = 1;
    a_aliasix_path_free[8].b_used = 1;
	a_aliasix_path_free[9].b_used = 1;
	a_aliasix_path_free[10].b_used = 1;
	a_aliasix_path_free[11].b_used = 1;
	a_aliasix_path_free[12].b_used = 1;
	a_aliasix_path_free[13].b_used = 1;
	a_aliasix_path_free[14].b_used = 1;
	a_aliasix_path_free[15].b_used = 1;
	a_aliasix_path_free[16].b_used = 1;
	a_aliasix_path_free[17].b_used = 1;
	a_aliasix_path_free[18].b_used = 1;
	a_aliasix_path_free[19].b_used = 1;
    a_aliasix_path_free[20].b_used = 1;
	strcpy(a_aliasix_path_free[0].a_pathname, "/ufs/opt_hbbtv/opera_free"); 
	strcpy(a_aliasix_path_free[1].a_pathname, "/version"); 
    strcpy(a_aliasix_path_free[2].a_pathname, "/stb/version"); 
    strcpy(a_aliasix_path_free[3].a_pathname, "/ufs/version"); 
    strcpy(a_aliasix_path_free[4].a_pathname, "/web/version"); 
	strcpy(a_aliasix_path_free[5].a_pathname, "/web/opt/hbbtv/opera_free"); 
	strcpy(a_aliasix_path_free[6].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT); 
	strcat(a_aliasix_path_free[6].a_pathname, "/version"); 
	strcpy(a_aliasix_path_free[7].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT); 
	strcat(a_aliasix_path_free[7].a_pathname, "/opt/hbbtv/opera_free"); 
	strcpy(a_aliasix_path_free[8].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT); 
	strcat(a_aliasix_path_free[8].a_pathname, "/opt_hbbtv/opera_free"); 
	strcpy(a_aliasix_path_free[9].a_pathname, "/ufs/evaluation-fonts");
	strcpy(a_aliasix_path_free[10].a_pathname, "/web/ufs/evaluation-fonts");
	strcpy(a_aliasix_path_free[11].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
	strcat(a_aliasix_path_free[11].a_pathname, "/evaluation-fonts");
	strcpy(a_aliasix_path_free[12].a_pathname, "/ufs/etc/fonts");
	strcpy(a_aliasix_path_free[13].a_pathname, "/ufs/etc/pango");
	strcpy(a_aliasix_path_free[14].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
	strcat(a_aliasix_path_free[14].a_pathname, "/etc/fonts");
	strcpy(a_aliasix_path_free[15].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
	strcat(a_aliasix_path_free[15].a_pathname, "/etc/pango");
	strcpy(a_aliasix_path_free[16].a_pathname, "/etc/fonts");
	strcpy(a_aliasix_path_free[17].a_pathname, "/etc/pango");
	strcpy(a_aliasix_path_free[18].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT);
	strcat(a_aliasix_path_free[18].a_pathname, "/ott-ap/drm");
	strcpy(a_aliasix_path_free[19].a_pathname, "/ufs/ott-ap/drm");
    strcpy(a_aliasix_path_free[20].a_pathname, "/ott/version");
	
    /*
     * In those directories, no files status will be recorded
     */
    memset(a_aliasix_path_wo_record, 0x00, sizeof(a_aliasix_path_wo_record));
    a_aliasix_path_wo_record[0].b_used = 1;
    a_aliasix_path_wo_record[1].b_used = 1;
    a_aliasix_path_wo_record[2].b_used = 1;
    a_aliasix_path_wo_record[3].b_used = 1;
    a_aliasix_path_wo_record[4].b_used = 1;
    a_aliasix_path_wo_record[5].b_used = 1;
    strcpy(a_aliasix_path_wo_record[0].a_pathname, "/usr/share/zoneinfo");
    strcpy(a_aliasix_path_wo_record[1].a_pathname, "/ufs/opt_hbbtv/opera_free"); 
    strcpy(a_aliasix_path_wo_record[2].a_pathname, "/web/opt/hbbtv/opera_free"); 
    strcpy(a_aliasix_path_wo_record[3].a_pathname, ALIASIX_UPGRADE_VERIFY_ROOT); 
    strcpy(a_aliasix_path_wo_record[4].a_pathname, "/ufs/share"); 
    strcpy(a_aliasix_path_wo_record[5].a_pathname, "/ufs/usr/lib/gconv"); 


    memset(a_aliasix_file_sha_status, 0x00, sizeof(a_aliasix_file_sha_status));
    for (i = 0; i < ALIASIX_FILE_NO_DIGEST_INDEX + 1; i++)
    {
        a_aliasix_file_sha_status[i].b_used = 1;
        a_aliasix_file_sha_status[i].b_valid = 1;
        a_aliasix_file_sha_status[i].b_verified = 0;
        a_aliasix_file_sha_status[i].i_dir_idx = 0;
        a_aliasix_file_sha_status[i].b_sig_split = 1;
    }
    a_aliasix_file_sha_status[9].i_dir_idx = 1;
    a_aliasix_file_sha_status[10].i_dir_idx = 2;
    a_aliasix_file_sha_status[11].i_dir_idx = 2;
    a_aliasix_file_sha_status[12].i_dir_idx = 2;
    a_aliasix_file_sha_status[13].i_dir_idx = 2;
    a_aliasix_file_sha_status[14].i_dir_idx = 2;
    a_aliasix_file_sha_status[15].i_dir_idx = 3;
    a_aliasix_file_sha_status[16].i_dir_idx = 3;
    a_aliasix_file_sha_status[17].i_dir_idx = 3;
    a_aliasix_file_sha_status[18].i_dir_idx = 3;
    a_aliasix_file_sha_status[19].i_dir_idx = 4;
    a_aliasix_file_sha_status[20].i_dir_idx = 4;
    a_aliasix_file_sha_status[21].i_dir_idx = 4;
    a_aliasix_file_sha_status[22].i_dir_idx = 4;
    a_aliasix_file_sha_status[23].i_dir_idx = 4;
    a_aliasix_file_sha_status[24].i_dir_idx = 4;
    a_aliasix_file_sha_status[25].i_dir_idx = 4;
    a_aliasix_file_sha_status[26].i_dir_idx = 4;
    a_aliasix_file_sha_status[27].i_dir_idx = 4;
	a_aliasix_file_sha_status[28].i_dir_idx = 4;
    a_aliasix_file_sha_status[29].i_dir_idx = 5;
    a_aliasix_file_sha_status[30].i_dir_idx = 6;
    a_aliasix_file_sha_status[31].i_dir_idx = 7;
    a_aliasix_file_sha_status[32].i_dir_idx = 8;
    a_aliasix_file_sha_status[33].i_dir_idx = 8;
    a_aliasix_file_sha_status[34].i_dir_idx = 8;
    a_aliasix_file_sha_status[35].i_dir_idx = 8;
    
    strcpy(a_aliasix_file_sha_status[0].a_filename, "fstab");
    strcpy(a_aliasix_file_sha_status[1].a_filename, "mdev.conf");
    strcpy(a_aliasix_file_sha_status[2].a_filename, "inittab");
    strcpy(a_aliasix_file_sha_status[3].a_filename, "directfbrc");
    strcpy(a_aliasix_file_sha_status[4].a_filename, "sysctl.conf");
    strcpy(a_aliasix_file_sha_status[5].a_filename, "passwd");
    strcpy(a_aliasix_file_sha_status[6].a_filename, "shadow");
    strcpy(a_aliasix_file_sha_status[7].a_filename, "group");
    strcpy(a_aliasix_file_sha_status[8].a_filename, "hostname");
    strcpy(a_aliasix_file_sha_status[9].a_filename, "interfaces");
    strcpy(a_aliasix_file_sha_status[10].a_filename, "passwd");
    strcpy(a_aliasix_file_sha_status[11].a_filename, "shadow");
    strcpy(a_aliasix_file_sha_status[12].a_filename, "group");
    strcpy(a_aliasix_file_sha_status[13].a_filename, "fstab");
    strcpy(a_aliasix_file_sha_status[14].a_filename, "directfbrc");
    strcpy(a_aliasix_file_sha_status[15].a_filename, "passwd");
    strcpy(a_aliasix_file_sha_status[16].a_filename, "shadow");
    strcpy(a_aliasix_file_sha_status[17].a_filename, "group");
    strcpy(a_aliasix_file_sha_status[18].a_filename, "directfbrc");
    strcpy(a_aliasix_file_sha_status[19].a_filename, "fstab");
    strcpy(a_aliasix_file_sha_status[20].a_filename, "mdev.conf");
    strcpy(a_aliasix_file_sha_status[21].a_filename, "inittab");
    strcpy(a_aliasix_file_sha_status[22].a_filename, "directfbrc");
    strcpy(a_aliasix_file_sha_status[23].a_filename, "sysctl.conf");
    strcpy(a_aliasix_file_sha_status[24].a_filename, "passwd");
    strcpy(a_aliasix_file_sha_status[25].a_filename, "shadow");
    strcpy(a_aliasix_file_sha_status[26].a_filename, "group");
    strcpy(a_aliasix_file_sha_status[27].a_filename, "hostname");
	strcpy(a_aliasix_file_sha_status[28].a_filename, "bootchartd.conf");
    strcpy(a_aliasix_file_sha_status[29].a_filename, "interfaces");
    strcpy(a_aliasix_file_sha_status[30].a_filename, "patch_PlayerPC.js");
    strcpy(a_aliasix_file_sha_status[31].a_filename, "patch_PlayerPC.js");
    strcpy(a_aliasix_file_sha_status[32].a_filename, "passwd");
    strcpy(a_aliasix_file_sha_status[33].a_filename, "shadow");
    strcpy(a_aliasix_file_sha_status[34].a_filename, "group");
    strcpy(a_aliasix_file_sha_status[35].a_filename, "directfbrc");

    for (i = ALIASIX_FILE_NO_DIGEST_INDEX + 1; i < ALIASIX_MAX_RO_FILE_NUMBER; i++)
    {
        a_aliasix_file_sha_status[i].i_dir_idx = ALIASIX_INVALID_INDEX;
        a_aliasix_file_sha_status[i].u32_expire = ALIASIX_LIVE_FOREVER;
    }
}
#endif

/*
 * Set the rootfs retire
 */
void aliasix_perm_set_rootfs_retire(void *unused)
{
    ALIASIX_DAEMON_INFO("rootfs retire flag set\n");
    rootfs_status.b_rootfs_retire = 1;
}

void __attribute__((noinstrument)) aliasix_perm_rootfs_retire_thread(void *unused)
{
    ssleep(ALIASIX_PERM_ROOTFS_RETIRE_DELAY);
    aliasix_perm_set_rootfs_retire(unused);
}

/*
 * Get the status of rootfs
 */
int aliasix_perm_is_rootfs_retire(void)
{
    return rootfs_status.b_rootfs_retire;
}

/*
 * Mark the rootfs file data done
 */
void aliasix_perm_rootfs_file_data_done(struct file *unused, const char *path)
{
    int i = 0;
    u_long u32_path_len= 0;

    if (rootfs_status.b_rootfs_retire)
        return;

    for (i = 0; i < ALIASIX_MAX_ROOTFS_NUM; i++)
    {
        if (rootfs_status.rootfs_file_status[i].b_used && \
            (0 == strcmp(rootfs_status.rootfs_file_status[i].a_pathname, 
                         path)))
        {
            ALIASIX_ROOTFS_FILE_DATA_DONE(i);
            break;
        }

        if (!rootfs_status.rootfs_file_status[i].b_used)
        {
            strcpy(rootfs_status.rootfs_file_status[i].a_pathname, path);
            rootfs_status.rootfs_file_status[i].b_used = 1;
            ALIASIX_ROOTFS_FILE_DATA_DONE(i);
            break;
        }
    }
}
EXPORT_SYMBOL(aliasix_perm_rootfs_file_data_done);

/*
 * Add the rootfs file permission items
 */
static int aliasix_perm_add_rootfs_permission(struct file *unused, 
                                                  const char *p_path,
                                                  aliasix_file_sha_status *file_sha_status)
{
    int i = 0;
    
    for (i = 0; i < ALIASIX_MAX_ROOTFS_NUM; i++)
    {
        if (rootfs_status.rootfs_file_status[i].b_used && \
            (0 == strcmp(rootfs_status.rootfs_file_status[i].a_pathname, 
                         p_path)) && \
            rootfs_status.rootfs_file_status[i].b_data_done)
        {
            rootfs_status.rootfs_file_status[i].b_verified = file_sha_status->b_verified;
            rootfs_status.rootfs_file_status[i].b_valid = file_sha_status->b_valid;
            break;
        }

        if (!rootfs_status.rootfs_file_status[i].b_used || \
            !rootfs_status.rootfs_file_status[i].b_data_done)
            /* Can't reach here */
            break;
    }
    return 0;
}

/*
 * Check the rootfs file status
 */
static int aliasix_perm_rootfs(struct file *file, const char *path)
{
    int ret = -EPERM;
    int i = 0;

    if (rootfs_status.b_rootfs_retire)
        goto out;

    for (i = 0; i < ALIASIX_MAX_ROOTFS_NUM; i++)
    {
        if (rootfs_status.rootfs_file_status[i].b_used && \
            (0 == strcmp(rootfs_status.rootfs_file_status[i].a_pathname, 
                         path)))
        {
            if (rootfs_status.rootfs_file_status[i].b_data_done)
            {
                if (!rootfs_status.rootfs_file_status[i].b_verified)
                {
                    ret = ALIASIX_SHA_NEED_VERIFY;
                    break;
                }
                else if (!rootfs_status.rootfs_file_status[i].b_valid)
                    break;
            }
            
            ret = 0;
            break;
        }

        if (!rootfs_status.rootfs_file_status[i].b_used)
        {
            ret = 0;
            break;
        }
    }

out:
    return ret;
}

/*
 * Initialization of the mutex
 */
void aliasix_perm_lock_init(void)
{
    mutex_init(&aliasix_directory_mutex);
    mutex_init(&aliasix_file_status_mutex);
}

/**
 * Check if the file from the free path
 * For the .sig file in the splitted directory
 * We can't split it again
 */
static int aliasix_perm_free_path(struct file *file, 
                                    const char *path, 
                                    const char *p_path)
{
    int ret = -EINVAL;
    int l_freepath_len = 0;
    int i = 0;

    /* Firstly, we check if the file from the free path */
	for (i = 0; i < ALIASIX_PATH_FREE_NUMBER; i++)
	{
		l_freepath_len = strlen(a_aliasix_path_free[i].a_pathname);
		if (a_aliasix_path_free[i].b_used && \
			(0 == strncmp(a_aliasix_path_free[i].a_pathname,  \
			              p_path, l_freepath_len)) && \
			(0 != strcmp(&path[strlen(path) - strlen(".sig")], ".sig")))
		{
            ALIASIX_FILEEXECEPTION("Info, file signature splitted"
                                   " for whole directory %s\n", 
                                    path);
			ret = 0;
			goto out;
		}
	}

out:
    return ret;
}

/**
 * Check if the file from path status won't record
 */
static int aliasix_perm_norecord_path(struct file *file, \
                                          const char *path, \
                                          const char *p_path)
{
    int ret = -EINVAL;
    int l_worpath_len = 0;
    int i = 0;
    
    /* Firstly, we check if the file from the free path */
	for (i = 0; i < ALIASIX_PATH_FREE_NUMBER; i++)
	{
		l_worpath_len = strlen(a_aliasix_path_wo_record[i].a_pathname);
		if (a_aliasix_path_wo_record[i].b_used && \
			(0 == strncmp(a_aliasix_path_wo_record[i].a_pathname,  \
			              p_path, l_worpath_len)))
		{
            ALIASIX_FILEEXECEPTION("Info, file signature splitted"
                                   " for whole directory %s\n", 
                                    path);
			ret = 0;
			goto out;
		}
	}

out:
    return ret;
}


/*
 * Check the exception file
 */
static int aliasix_perm_exception(struct file *file, const char *path)
{
    int ret = -EPERM;
    int i = 0;
    int i_path_idx = ALIASIX_INVALID_INDEX;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    
    ALIASIX_PERM_CAST_DIRNAME(p_path, path, file);
    ALIASIX_FILEEXECEPTION("Info, path %s %s\n", path, p_path);

	if (0 == (ret = aliasix_perm_free_path(file, path, p_path)))
        goto out;

    /* We think we don't have duplicated path, that's the truth */
    for (i = 0; i < ALIASIX_PATH_NO_DIGEST_INDEX + 1; i++)
    {
        if (a_aliasix_path_status[i].b_used && \
            (0 == strcmp(p_path, a_aliasix_path_status[i].a_pathname)))
        {
            i_path_idx = i;
            break;
        }

        if (!a_aliasix_path_status[i].b_used)
            break;
    }

    if (ALIASIX_INVALID_INDEX == i_path_idx)
        goto out;

    /* Is it needed ? */
    for (i = 0; i < ALIASIX_FILE_NO_DIGEST_INDEX + 1; i++)
    {
        if (a_aliasix_file_sha_status[i].b_used && \
            (a_aliasix_file_sha_status[i].i_dir_idx == i_path_idx) && \
            (0 == strcmp(a_aliasix_file_sha_status[i].a_filename, \
            file->f_path.dentry->d_iname)) && \
            a_aliasix_file_sha_status[i].b_sig_split)
        {
            ALIASIX_FILEEXECEPTION("Info, file signature splitted %s %s\n", 
                                   a_aliasix_file_sha_status[i].a_filename, 
                                   p_path);
            ret = 0;
            goto out;
        }

        if (!a_aliasix_file_sha_status[i].b_used)
            break;
    }

out:
    kfree(p_path);
    return ret;
}

/**
 * Check if the signature of the file is splitted
 */
int aliasix_perm_sig_split(struct file *file, char *f_dir, char *f_name)
{
    char *path = kmalloc(PATH_MAX, GFP_KERNEL);
    u_long u32_path_len = 0;
    int ret = -EINVAL;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    int i = 0;

    u32_path_len = aliasix_misc_path_walk(file, path);
    if (0 == u32_path_len) goto out;

    ALIASIX_PERM_CAST_DIRNAME(p_path, path, file);
    if (ret = aliasix_perm_exception(file, path))
        ALIASIX_FILEEXECEPTION("Info, Won't split the signature %s\n", path);

    if (NULL != f_dir && NULL != f_name)
    {
        strcpy(f_dir, p_path);
        strcpy(f_name, file->f_path.dentry->d_name.name);
    }

out:
    kfree(p_path);
    kfree(path);
    return ret;
}

/*
 * Check the exception file
 *				-- Owen Origin ---
 * Normally, we'll trust the result of the SHA device
 * But some chip may have the strange behavior of
 * the SHA device, for such case, when we have an
 * invalid file, we need to verify it again.
 *				-- Owen Modified ---
 */
static int aliasix_perm_existence(struct file *file, const char *path, \
                                  int b_fileclose)
{
    int ret = ALIASIX_SHA_NEED_VERIFY;
    int i = 0;
    int i_path_idx = ALIASIX_INVALID_INDEX;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);

    ALIASIX_PERM_CAST_DIRNAME(p_path, path, file);
    ALIASIX_PATH("Info, path %s %s\n", path, p_path);

    /* We think we don't have duplicated path, that's the truth */
    for (i = 0; i < ALIASIX_MAX_RO_DIRECTORIES; i++)
    {
        ALIASIX_PERM_DIRECTORY_GET(i, i_path_idx, p_path);
    }

    if (ALIASIX_INVALID_INDEX == i_path_idx)
        goto out;

    /* Is it needed ? */
    for (i = 0; i < ALIASIX_MAX_RO_FILE_NUMBER; i++)
    {
        if ((a_aliasix_file_sha_status[i].i_dir_idx == i_path_idx) && \
            a_aliasix_file_sha_status[i].b_used && \
            (0 == strcmp(a_aliasix_file_sha_status[i].a_filename, 
            file->f_path.dentry->d_iname)))
        {
            ALIASIX_PATH("Info, file %s %s\n", p_path, 
                         a_aliasix_file_sha_status[i].a_filename);
            if (!a_aliasix_file_sha_status[i].b_verified || \
                (ALIASIX_PERMISSION_CHECK_INTERVAL < (jiffies - \
                 a_aliasix_file_sha_status[i].u32_expire)))
                break;
            
            if (a_aliasix_file_sha_status[i].b_valid)
                ret = 0;
            else
                ret = ALIASIX_SHA_NEED_VERIFY;

            /*
             * When close the file, we'll delete the file status
             * It'll be verified again when it's opened
             */
            if (b_fileclose && (0 == ret))
            {
                a_aliasix_file_sha_status[i].b_valid = 0;
                a_aliasix_file_sha_status[i].b_verified = 0;
            }
            
            break;
        }

        if (!a_aliasix_file_sha_status[i].b_used)
            break;
    }
    
out:
    kfree(p_path);
    return ret;
}

/*
 * Check the permission stored
 */
static int __aliasix_perm_permission(struct file *file, const char *path)
{
    int ret = -EPERM;
    int i = 0;
    int i_path_idx = ALIASIX_INVALID_INDEX;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);

    ALIASIX_PERM_CAST_DIRNAME(p_path, path, file);
    ALIASIX_PATH("Info, path %s %s\n", path, p_path);

    /* We think we don't have duplicated path, that's the truth */
    for (i = 0; i < ALIASIX_MAX_RO_DIRECTORIES; i++)
    {
        ALIASIX_PERM_DIRECTORY_GET(i, i_path_idx, p_path);
    }

    if (ALIASIX_INVALID_INDEX == i_path_idx)
        goto out;

    /* Is it needed ? */
    for (i = 0; i < ALIASIX_MAX_RO_FILE_NUMBER; i++)
    {
        if ((a_aliasix_file_sha_status[i].i_dir_idx == i_path_idx) && \
            (0 == strcmp(a_aliasix_file_sha_status[i].a_filename, 
            file->f_path.dentry->d_iname)) && \
            a_aliasix_file_sha_status[i].b_verified && \
            a_aliasix_file_sha_status[i].b_used && \
            a_aliasix_file_sha_status[i].b_valid)
        {
            ALIASIX_PATH("Info, file %s %s\n", p_path, 
                         a_aliasix_file_sha_status[i].a_filename);
            
            ALIASIX_LOG("Info, jiffies 0x%lx expire 0x%lx, re-check if 0x%lx > 0x%lx\n", 
                     jiffies, a_aliasix_file_sha_status[i].u32_expire,
                     jiffies - a_aliasix_file_sha_status[i].u32_expire,
                     ALIASIX_PERMISSION_CHECK_INTERVAL);
            
            if ((ALIASIX_PERMISSION_CHECK_INTERVAL < (jiffies - \
                 a_aliasix_file_sha_status[i].u32_expire)))
                ret = ALIASIX_SHA_NEED_VERIFY;
            else
                ret = 0;
            break;
        }

        if (!a_aliasix_file_sha_status[i].b_used)
            break;
    }

out:
    kfree(p_path);
    return ret;
}

/*
 * Delete the permission status when it's closed
 */
int aliasix_perm_reset_permission(struct file *file)
{
    char *path = kmalloc(PATH_MAX, GFP_KERNEL);
    u_long u32_path_len = 0;

    u32_path_len = aliasix_misc_path_walk(file, path);
    if (0 == u32_path_len) goto out;
    
    aliasix_perm_existence(file, path, 1);

out:
    kfree(path);
    return 0;
}

/*
 * Check if the SHA status is available in the list
 * \@param dir: the dir of the file
 * \@param file: the filename
 * \@return: = 0, access granted; 
 *           > 0, first time access, need to verify;
 *           < 0, access denied
 */
int aliasix_perm_permission(struct file *file)
{
    int ret = -EPERM;
    char *path = kmalloc(PATH_MAX, GFP_KERNEL);
    u_long u32_path_len = 0;

    u32_path_len = aliasix_misc_path_walk(file, path);
    if (0 == u32_path_len) goto out;

    ALIASIX_PATH("Info, path %s\n", path);

#ifdef CONFIG_ROOTFS_VERIFICATION_ALIASIX
    if ((ret = aliasix_perm_rootfs(file, path)) >= 0)
        goto out;
#endif
#ifdef ALLOW_PERMISSION_EXCEPTION_ALIASIX
    if (0 == (ret = aliasix_perm_exception(file, path)))
        goto out;
#endif

    ret = __aliasix_perm_permission(file, path);
    if ((0 == ret) || ALIASIX_SHA_NEED_VERIFY == ret)
        goto out;

    ret = aliasix_perm_existence(file, path, 0);

out:
    kfree(path);
    return ret;
}

/*
 * Add new directory in the directory list
 */
static int aliasix_perm_add_directory(const char *p_path)
{
    int i_path_idx = ALIASIX_INVALID_INDEX;
    int i = 0;

    mutex_lock(&aliasix_directory_mutex);
    for (i = 0; i < ALIASIX_MAX_RO_DIRECTORIES; i++)
    {
        if (a_aliasix_path_status[i].b_used && \
            (0 == strcmp(p_path, a_aliasix_path_status[i].a_pathname)))
        {
            ALIASIX_PATH("Info, path got %s\n", p_path);
            i_path_idx = i;
            break;
        }

        if (!a_aliasix_path_status[i].b_used)
        {
            ALIASIX_PATH("Info, path added %s\n", p_path);
            strcpy(a_aliasix_path_status[i].a_pathname, p_path);
            a_aliasix_path_status[i].b_used = 1;
            i_path_idx = i;
            break;
        }
    }
    mutex_unlock(&aliasix_directory_mutex);

    return i_path_idx;
}

/*
 * Add new directory in the directory list
 */
static int aliasix_perm_add_file(struct file *file, int i_path_idx,
                           aliasix_file_sha_status *file_sha_status)
{
    int i_file_idx = ALIASIX_INVALID_INDEX;
    int i = 0;

    mutex_lock(&aliasix_file_status_mutex);
    /* Is it needed ? */
    for (i = 0; i < ALIASIX_MAX_RO_FILE_NUMBER; i++)
    {
        if ((a_aliasix_file_sha_status[i].i_dir_idx == i_path_idx) && \
            (0 == strcmp(a_aliasix_file_sha_status[i].a_filename, 
            file->f_path.dentry->d_iname)) && \
            a_aliasix_file_sha_status[i].b_used)
        {
            i_file_idx = i;
#ifdef ALLOW_PERMISSION_EXCEPTION_ALIASIX
            if (i <= ALIASIX_FILE_NO_DIGEST_INDEX)
                break;
#endif
            
            ALIASIX_PATH("Info, file updated %s\n", a_aliasix_file_sha_status[i].a_filename);
            ALIASIX_FILE_STATUS_UPDATE(i, file_sha_status);
            break;
        }

        if (!a_aliasix_file_sha_status[i].b_used)
        {
            a_aliasix_file_sha_status[i].b_used = 1;
            a_aliasix_file_sha_status[i].i_dir_idx = i_path_idx;
            strcpy(a_aliasix_file_sha_status[i].a_filename, 
                   file->f_path.dentry->d_iname);
            ALIASIX_FILE_STATUS_UPDATE(i, file_sha_status);
            ALIASIX_PATH("Info, file added %s\n", a_aliasix_file_sha_status[i].a_filename);
            i_file_idx = i;
            break;
        }
    }
    mutex_unlock(&aliasix_file_status_mutex);
    
    return i_file_idx;
}

/**
 * Check if the file from the free path
 */
static int aliasix_perm_wo_record_path(struct file *file, \
                                           const char *path, \
                                           const char *p_path)
{
    int ret = -EINVAL;
    int l_wopath_len = 0;
    int i = 0;

    /* Firstly, we check if the file from the free path */
	for (i = 0; i < ALIASIX_PATH_FREE_NUMBER; i++)
	{
		l_wopath_len = strlen(a_aliasix_path_wo_record[i].a_pathname);
		if (a_aliasix_path_wo_record[i].b_used && \
			(0 == strncmp(a_aliasix_path_wo_record[i].a_pathname,  \
			              p_path, l_wopath_len)))
		{
            ALIASIX_PATH("Info, file status won't record %s\n", 
                          path);
			ret = 0;
			goto out;
		}
	}

out:
    return ret;
}

/*
 * Add new item to the permission list
 */
void aliasix_perm_add_permission_item(struct file *file, 
                                      aliasix_file_sha_status *file_sha_status)
{
    char *path = kmalloc(PATH_MAX, GFP_KERNEL);
    u_long u32_path_len= 0;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    int i_path_idx = ALIASIX_INVALID_INDEX;
    int i_file_idx = ALIASIX_INVALID_INDEX;

    if (!file_sha_status->b_used) 
        goto out;

    u32_path_len = aliasix_misc_path_walk(file, path);
    if (0 == u32_path_len) goto out;

    if (!rootfs_status.b_rootfs_retire)
    {
        aliasix_perm_add_rootfs_permission(NULL, path, file_sha_status);
        goto out;
    }

    ALIASIX_PERM_CAST_DIRNAME(p_path, path, file);
    ALIASIX_PATH("Info, path %s %s\n", path, p_path);

    /* Small file which can't be recorded due to too many of them */
    if (!aliasix_perm_wo_record_path(file, path, p_path))
        goto out;

    i_path_idx = aliasix_perm_add_directory(p_path);

    if (ALIASIX_INVALID_INDEX == i_path_idx)
    {
        ALIASIX_ERR("Err, Fatal error, no more path space!!!\n");
        goto out;
    }

    i_file_idx = aliasix_perm_add_file(file, i_path_idx, file_sha_status);

    if (ALIASIX_INVALID_INDEX == i_file_idx)
    {
        ALIASIX_ERR("Err, Fatal error, no more file space!!!\n");
    }

out:
    kfree(p_path);
    kfree(path);
}

/*
 * Get the first expired file's full name
 */
int aliasix_perm_get_expire_file(char *p_path)
{
    int i = 0;
    int i_file_idx = ALIASIX_INVALID_INDEX;
    int i_path_idx = ALIASIX_INVALID_INDEX;
    
    for (i = ALIASIX_FILE_NO_DIGEST_INDEX + 1; i < ALIASIX_MAX_RO_FILE_NUMBER; i++)
    {
        if (a_aliasix_file_sha_status[i].b_used && \
            (ALIASIX_PERMISSION_CHECK_INTERVAL < (jiffies - \
            a_aliasix_file_sha_status[i].u32_expire)))
        {
            i_file_idx = i;
            break;
        }

        if (!a_aliasix_file_sha_status[i].b_used)
            goto out;
    }

    if (ALIASIX_INVALID_INDEX == i_file_idx)
        goto out;

    i_path_idx = a_aliasix_file_sha_status[i].i_dir_idx;
    if (ALIASIX_INVALID_INDEX == i_path_idx || \
        !a_aliasix_path_status[i_path_idx].b_used)
        goto out;

    strcpy(p_path, a_aliasix_path_status[i_path_idx].a_pathname);
    strcat(p_path, "/");
    strcat(p_path, a_aliasix_file_sha_status[i_file_idx].a_filename);

    return 0;
    
out:        
    return -EINVAL;
}

/*
 * Show the file status in proc entry
 */
unsigned long aliasix_perm_show_status(char *page, char **start,
                            	     off_t off, int count, 
                            	     int *eof, void *data)
{
    return 0;
}
