/*
 * Scramber Core driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
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

#ifndef __CA_SCR_PRIV_H__
#define __CA_SCR_PRIV_H__

#include <linux/idr.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif


#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <ali_otp_common.h>
#include <alidefinition/adf_scr.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#include "ca_dsc.h"
#include "see_bus.h"
#include "ali_sbm_client.h"
#include "ca_scr_session_engine.h"

#define TS_PACKET_SIZE 188

#define SCR_INVALID_SESSION_FORMAT (0xFF)
#define SCR_INVALID_PID (0xFFFF)
#define SCR_INVALID_SESSION_ID (0xFFFFFFFF)
#define SCR_INVALID_KEY_HDL (0xFFFFFFFF)

#define SCR_INST_CLEAR_KEY (0x80)
#define SCR_INST_KL_KEY (0x81)
#define SCR_INST_OTP_KEY (0x82)
#define SCR_INST_MIX_KEY (0x83)
#define SCR_INST_KEY_ID_MASK (0x0000FFFF)

#define SCR_INVALID_TSC_FLAG			(0xA5)
#define SCR_INVALID_CRYPTO_MODE_FLAG	(0xFF)
#define SCR_INVALID_PARITY_FLAG			(0xFF)
#define SCR_INVALID_ALGO_FLAG			(0xFF)

#define SCR_SCHE_DELAY usecs_to_jiffies(100)

struct scr_pids {
	unsigned short pid;
	unsigned char ltsid;
	unsigned char tsc;

	struct list_head pid_node;
};

struct scr_inst_key {
	int key_id;
	unsigned int key_handle;

	/*SCR_INST_CLEAR_KEY, SCR_INST_KL_KEY*/
	int key_type;
	unsigned char iv_odd[CA_IV_SIZE_MAX];
	unsigned char iv_even[CA_IV_SIZE_MAX];
	unsigned char key_odd[CA_KEY_SIZE_MAX];
	unsigned char key_even[CA_KEY_SIZE_MAX];
	int key_size;
	int kl_fd;
	struct kl_key_cell *cell;

	struct list_head pid_list;
	int pid_size;

	int no_even;
	int no_odd;
	int even_locate; /*0: host, 1: KL*/
	int odd_locate; /*0: host, 1: KL*/

	int tsc_flag;
	int crypt_mode;
	int parity;
	int residue;
	int chaining;

	struct list_head key_node;
	struct ca_scr_session *s;
};

/*
 * Device structure
 */
struct ca_scr_dev {
	dev_t  devt;
	struct class *class;
	struct device *dev;
	struct cdev cdev;
	void *base;

	/* info from OTP */
	int fixed_addr_mode;
	int fixed_eng_mode;

	/* number of instance */
	int num_inst;
	__u32 mode;
	struct mutex mutex;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
	/*1: do not process by see hw*/
	int not_gothrough_hw;
#endif

	void *see_scr_id;

	struct see_client *clnt;
	struct ida sess_ida;
	int debug_mode;
};

/*
 * Session structure
 */
struct ca_scr_session {
	struct ca_scr_dev *scr;
	struct mutex rd_mutex;
	struct mutex wr_mutex;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
	struct dentry *choice;
	struct dentry *buffer_id;
#endif

	int id;
	struct ca_scr_se engine;

	/* Externel Parameters */
	struct list_head key_list;

	int format;
	int opt; /*cork / uncork*/
	int algo;
	int crypt_mode;
	int chaining_mode;
	int residue_mode;
	int parity;
	unsigned char ts_chaining;
	unsigned char sc_mode;
	unsigned char tsc_flag;
	unsigned char fmt_flag;
	/*1: raw data; 188: TS188; 200: TS200*/
	int pkt_size;

	/*internal Resources*/
	struct ida key_ida;
	__u32 see_sess_id;

	struct see_sbm sbm;

#ifdef CONFIG_DEBUG_FS
	struct dentry *session_dir;
#endif
};

void scr_key_delete(struct scr_inst_key *inst_key);
void low_scr_delete_session(struct ca_scr_session *s);

#endif /* __CA_SCR_PRIV_H__ */


