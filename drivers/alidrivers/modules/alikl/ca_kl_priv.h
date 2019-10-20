/*
 * Key Ladder Core driver
 * Copyright(C) 2014 ALi Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __CA_KL_PRIV_H__
#define __CA_KL_PRIV_H__

#include <linux/idr.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include "ca_kl.h"

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <alidefinition/adf_ce.h>
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"
#include "see_bus.h"

#define KL_INVALID_KEY_POS 0xff
#define KL_AES_BLOCK_LEN 16
#define KL_DES_BLOCK_LEN 8
#define KL_LOW_ADDR		0       /* low addr for even*/
#define KL_HIGH_ADDR	1       /* high addr for odd*/

#define KL_MAX_LEVEL_0 (0)
#define KL_MAX_LEVEL_1 (1)
#define KL_MAX_LEVEL_2 (2)
#define KL_MAX_LEVEL_3 (3)
#define KL_MAX_LEVEL_5 (5)

/*KL0 ~ 4*/
#define KL_INDEX_0 (0)
#define KL_INDEX_1 (1)
#define KL_INDEX_2 (2)
#define KL_INDEX_3 (3)
/*KL4, Available only when one-kl-one-engine enables*/
#define KL_INDEX_4 (4)

/*ETSI0 ~ 4*/
#define ETSI_INDEX_PREFIX (0x10)
#define ETSI_INDEX_0 (ETSI_INDEX_PREFIX + 0)
#define ETSI_INDEX_1 (ETSI_INDEX_PREFIX + 1)
#define ETSI_INDEX_2 (ETSI_INDEX_PREFIX + 2)
#define ETSI_INDEX_3 (ETSI_INDEX_PREFIX + 3)
/*ETSI4, Available only when one-kl-one-engine enables*/
#define ETSI_INDEX_4 (ETSI_INDEX_PREFIX + 4)


#define KL_MAX_ENGINE (4)

#define KL_TYPE_ALI  (0)
#define KL_TYPE_ETSI (1)
#define KL_TYPE_AKL	(2)
#define KL_TYPE_VSC	(3)

enum HLD_CE_FUNC
{
    FUNC_CE_ATTACH = 0,
	FUNC_CE_SUSPEND,
	FUNC_CE_RESUME,
    FUNC_CE_SET_AKSV,
    FUNC_PATCH_HDMI,
    FUNC_CE_DETACH,
    FUNC_CE_GENERATE,
    FUNC_CE_LOAD,
    FUNC_CE_IOCTL,
    FUNC_CE_GENERATE_CW_KEY,
    FUNC_CE_GENERATE_SINGLE_LEVEL_KEY,

    FUNC_CE_GENERATE_HDCP_KEY,
};

/*
 * Device structure
 */
struct ca_kl_dev {
	struct see_client *clnt;
	struct mutex mutex;
	void *see_ce_id;

	/* global OTP info */
	int fixed_addr_mode;
		/*KL 1-4 -- 0: disable; 1: enable*/
	int fixed_eng_mode;
		/* 0: disable; 1: enable*/
	int one_kl_one_engine_mode;
		/* 0: disable; 1: enable*/
	int disable_kdf;
		/* 0: enable kdf; 1: disable kdf*/

	int legacy_hw;

	struct list_head sub_dev_list;
};

struct ca_kl_sub_dev {
	struct ca_kl_dev *parent;
	dev_t  devt;
	struct class *class;
	struct device *dev;
	struct cdev cdev;

	struct ida s_ida;

	/*KL_TYPE_ALI or KL_TYPE_ETSI*/
	int type;

	int dev_index;
	char basename[16];
	int root_key_addr;
	int fixed_engine_algos[KL_MAX_ENGINE];
	int fixed_engine_algos_num;
	int one_kl_one_engine_algo;

	/* individule OTP info */
	int level_5th_en;
		/*0: disable 5 level; 1: enable 5 level*/
	int level_1st2nd3rd_sel;
		/*0x: 1st level; 01: 2nd level; 11: 3rd level*/

	int sess_num;
		/* number of opening sessions */
	int support_level;
		/* KL supported gen key level, decided by OTP*/
	int max_level;
		/* legacy IC: max 3; cap210: max 5*/
	int kl_index;
		/*KL_INDEX_0 ~ 3 or ETSI_INDEX_0~3*/

	int is_hdcp;
		/*loads root key from 0x59 && loads CW to internal sram*/

	struct kl_fd_operations fd_ops;
	struct list_head sub_dev;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
#endif

	int debug_mode;
};

/*
 * Session structure
 */
struct ca_kl_session {
	struct ca_kl_sub_dev *pdev;
	struct mutex mutex;

	int id;

	/*private data*/
	int level;
	int ck_size;
	int ck_parity;
	int crypt_mode;
	int algo;

	int is_cfg; /*1: this session is configured*/

	struct kl_key_cell *cell;
	struct kl_gen_key gen_key;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
	struct dentry *session_dir;
	struct dentry *choice;
#endif
};

static inline struct ca_kl_session *file2session(struct file *f)
{
	return (struct ca_kl_session *)f->private_data;
}

int ca_kl_key_dump(struct ca_kl_session *s, int kl_sel,
	int dump_pos, unsigned int *pkey);

#endif /*__CA_KL_PRIV_H__*/

