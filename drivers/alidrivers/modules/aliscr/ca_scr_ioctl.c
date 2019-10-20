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


#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/splice.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/delay.h>

#include <ca_dsc.h>
#include "ca_scr_ioctl.h"
#include "ca_scr_ioctl_legacy.h"
#include "ca_scr_priv.h"
#include "ca_scr_rpc.h"
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

#define SCR_INVALID_FORMAT(format) ( \
	(format) != CA_FORMAT_RAW && \
	(format) != CA_FORMAT_TS188 && \
	(format) != CA_FORMAT_TS188_LTSID && \
	(format) != CA_FORMAT_TS200)

#define SCR_INVALID_ALGO(algo) ((algo) != CA_ALGO_AES && \
	(algo) != CA_ALGO_DES && \
	(algo) != CA_ALGO_TDES)

#define SCR_INVALID_RESIDUE(mode) ( \
	(mode) != CA_RESIDUE_CLEAR && \
	(mode) != CA_RESIDUE_AS_ATSC && \
	(mode) != CA_RESIDUE_HW_CTS)

#define SCR_INVALID_CHAINING(mode) ( \
	(mode) != CA_MODE_ECB && \
	(mode) != CA_MODE_CBC && \
	(mode) != CA_MODE_CFB && \
	(mode) != CA_MODE_CTR && \
	(mode) != CA_MODE_OFB)

#define SCR_NEED_IV(mode) ( \
	(mode) == CA_MODE_CBC || \
	(mode) == CA_MODE_CFB || \
	(mode) == CA_MODE_OFB)

#define SCR_INVALID_PARITY(parity) ( \
	((parity) != CA_PARITY_ODD) && \
	((parity) != CA_PARITY_EVEN) && \
	((parity) != CA_PARITY_AUTO))

#define SCR_INVALID_TSC(tsc) ( \
	((tsc) != CA_TSC_FLAG_CLEAR) && \
	((tsc) != CA_TSC_FLAG_EVEN) && \
	((tsc) != CA_TSC_FLAG_ODD) && \
	((tsc) != CA_TSC_FLAG_AUTO))

#define SCR_INVALID_CRYPTO(crypt) ( \
	(crypt) != CA_ENCRYPT && (crypt) != CA_DECRYPT)

#define SCR_INVALID_AES_KEY_SIZE(size) ( \
	(size) != CA_KEY_SIZE_BYTE_16 && \
	(size) != CA_KEY_SIZE_BYTE_24 && \
	(size) != CA_KEY_SIZE_BYTE_32)
#define SCR_INVALID_DES_KEY_SIZE(size) ( \
	(size) != CA_KEY_SIZE_BYTE_8)
#define SCR_INVALID_TDES_KEY_SIZE(size) ( \
	(size) != CA_KEY_SIZE_BYTE_16 && \
	(size) != CA_KEY_SIZE_BYTE_24)

static int low_scr_create_session(struct ca_scr_session *s)
{
	struct scr_sess_create sess;
	unsigned int sess_id = SCR_INVALID_SESSION_ID;
	int ret;

	memset(&sess, 0, sizeof(sess));

	sess.format = s->format;
	sess.algo = s->algo;
	sess.parity = s->parity;
	sess.residue = s->residue_mode;
	sess.chaining = s->chaining_mode;
	sess.crypto = (s->crypt_mode == CA_ENCRYPT) ?
		SCR_ENCRYPT : SCR_DECRYPT;
	sess.continuous = s->ts_chaining;
	sess.sc_mode = s->sc_mode;

	ret = _scr_session_create(s->scr, &sess_id, &sess);
	if (ret != 0 || sess_id == SCR_INVALID_SESSION_ID) {
		dev_dbg(s->scr->dev, "create see sess err! sid[%x][%x]\n",
			sess_id, ret);
		return -EIO;
	}

	s->see_sess_id = sess_id;
	return 0;
}

void low_scr_delete_session(struct ca_scr_session *s)
{
	int ret;

	if (s->see_sess_id == SCR_INVALID_SESSION_ID)
		return;

	ret = _scr_session_delete(s->scr, s->see_sess_id);
	if (ret != 0)
		dev_err(s->scr->dev, "del see sess err![%x]\n", ret);

	s->see_sess_id = SCR_INVALID_SESSION_ID;
}

static int low_scr_add_key(struct ca_scr_session *s,
	struct scr_inst_key *key)
{
	struct scr_key user_key;
	int ret;

	if (s->see_sess_id == SCR_INVALID_SESSION_ID) {
		dev_dbg(s->scr->dev, "pls create session first!\n");
		return -EPERM;
	}

	memset(&user_key, 0, sizeof(struct scr_key));

	if (key->key_type == SCR_INST_CLEAR_KEY) {
		user_key.key_from = SCR_KEY_FROM_SRAM;
	} else if (key->key_type == SCR_INST_KL_KEY) {
		user_key.key_from = SCR_KEY_FROM_KL;

		/*check key cell available*/
		if (!key->cell) {
			dev_dbg(s->scr->dev, "ERR: pls fetch key cell first\n");
			return ret;
		}

		user_key.key_pos = key->cell->pos;
	}

	user_key.key_handle = SCR_INVALID_KEY_HANDLE;
	user_key.key_size = (key->key_size / 8 - 1);
	memcpy(user_key.iv_even, key->iv_even, SCR_IV_SIZE_MAX);
	memcpy(user_key.iv_odd, key->iv_odd, SCR_IV_SIZE_MAX);
	memcpy(user_key.key_even, key->key_even, SCR_KEY_SIZE_MAX);
	memcpy(user_key.key_odd, key->key_odd, SCR_KEY_SIZE_MAX);

	ret = _scr_session_add_key(s->scr, s->see_sess_id, &user_key);
	if (user_key.key_handle == SCR_INVALID_KEY_HANDLE ||
		ret != 0) {
		dev_dbg(s->scr->dev, "add sess key err[%x]\n", ret);
		return -EIO;
	}

	key->key_handle = user_key.key_handle;

	return 0;
}

static int low_scr_del_key(struct ca_scr_session *s,
	struct scr_inst_key *key)
{
	int ret;

	if (s->see_sess_id == SCR_INVALID_SESSION_ID)
		return -EPERM;

	if (key->key_handle == SCR_INVALID_KEY_HDL)
		return -EPERM;

	ret = _scr_session_del_key(s->scr, s->see_sess_id,
		key->key_handle);
	if (ret != 0) {
		dev_dbg(s->scr->dev, "del sess key err[%x]\n", ret);
		return -EIO;
	}

	return 0;
}

static int low_scr_add_pid(struct ca_scr_session *s,
	struct scr_inst_key *key, unsigned short pid,
	unsigned char ltsid, unsigned char tsc_flag)
{
	struct scr_pid pid_param;
	int ret;

	if (s->see_sess_id == SCR_INVALID_SESSION_ID)
		return -EPERM;

	if (key->key_handle == SCR_INVALID_KEY_HDL)
		return -EPERM;

	memset(&pid_param, 0, sizeof(struct scr_pid));

	pid_param.key_handle = key->key_handle;
	pid_param.pid = pid;
	pid_param.ltsid = ltsid;
	pid_param.tsc_flag = tsc_flag;

	ret = _scr_session_add_pid(s->scr, s->see_sess_id,
		&pid_param);
	if (ret != 0) {
		dev_dbg(s->scr->dev, "add pid error![%x]\n", ret);
		return -EIO;
	}

	return 0;
}

static int low_scr_del_pid(struct ca_scr_session *s,
	struct scr_inst_key *key, unsigned short pid,
	unsigned char ltsid, unsigned char tsc_flag)
{
	struct scr_pid pid_param;
	int ret;

	if (s->see_sess_id == SCR_INVALID_SESSION_ID)
		return -EPERM;

	if (key->key_handle == SCR_INVALID_KEY_HDL)
		return -EPERM;

	memset(&pid_param, 0, sizeof(struct scr_pid));

	pid_param.key_handle = key->key_handle;
	pid_param.pid = pid;
	pid_param.ltsid = ltsid;
	pid_param.tsc_flag = tsc_flag;

	ret = _scr_session_del_pid(s->scr, s->see_sess_id,
		&pid_param);
	if (ret != 0) {
		dev_dbg(s->scr->dev, "del pid error![%x]\n", ret);
		return -EIO;
	}

	return 0;
}

static struct scr_inst_key *scr_key_new(void *key,
	int key_type, struct ca_scr_session *s)
{
	int ret;
	struct scr_inst_key *p_inst_key = NULL;
	struct ca_create_clear_key *clear_key;
	struct ca_create_kl_key *kl_key;

	/*new s key*/
	p_inst_key = kmalloc(sizeof(struct scr_inst_key),
		GFP_KERNEL);
	if (!p_inst_key)
		return NULL;

	memset(p_inst_key, 0, sizeof(struct scr_inst_key));
	p_inst_key->no_even = 1;
	p_inst_key->no_odd = 0;

	if (SCR_INST_CLEAR_KEY == key_type) {
		clear_key = (struct ca_create_clear_key *)key;

		p_inst_key->even_locate = 0;
		p_inst_key->odd_locate = 0;
		p_inst_key->key_size = clear_key->key_size;
		p_inst_key->key_type = SCR_INST_CLEAR_KEY;
		p_inst_key->tsc_flag = clear_key->tsc_flag;
		p_inst_key->crypt_mode = clear_key->crypt_mode;
		p_inst_key->parity = clear_key->parity;
		p_inst_key->residue = clear_key->residue_mode;
		p_inst_key->chaining = clear_key->chaining_mode;
		if (clear_key->valid_mask & CA_VALID_KEY_EVEN) {
			memcpy(p_inst_key->key_even,
				clear_key->key_even,
				CA_KEY_SIZE_MAX);
			p_inst_key->no_even = 0;
		}
		if (clear_key->valid_mask & CA_VALID_KEY_ODD) {
			memcpy(p_inst_key->key_odd,
				clear_key->key_odd,
				CA_KEY_SIZE_MAX);
			p_inst_key->no_odd = 0;
		}

		if (clear_key->valid_mask & CA_VALID_IV_EVEN) {
			memcpy(p_inst_key->iv_even,
				clear_key->iv_even,
				CA_IV_SIZE_MAX);
		}

		if (clear_key->valid_mask & CA_VALID_IV_ODD) {
			memcpy(p_inst_key->iv_odd,
				clear_key->iv_odd,
				CA_IV_SIZE_MAX);
		}
	} else if (SCR_INST_KL_KEY == key_type) {
		kl_key = (struct ca_create_kl_key *)key;

		p_inst_key->even_locate = 1;
		p_inst_key->odd_locate = 1;
		p_inst_key->key_type = key_type;
		p_inst_key->kl_fd = kl_key->kl_fd;
		p_inst_key->tsc_flag = kl_key->tsc_flag;
		p_inst_key->crypt_mode = kl_key->crypt_mode;
		p_inst_key->parity = kl_key->parity;
		p_inst_key->residue = kl_key->residue_mode;
		p_inst_key->chaining = kl_key->chaining_mode;
		ret = fetch_key_cell_by_fd(kl_key->kl_fd, &p_inst_key->cell);
		if (ret || !p_inst_key->cell) {
			dev_dbg(s->scr->dev,
				"fetch key cell by fd error:%d\n", ret);
			kfree(p_inst_key);
			return NULL;
		} else {
			get_key_cell(p_inst_key->cell);
		}

		if (kl_key->algo == CA_ALGO_AES ||
			kl_key->algo == CA_ALGO_TDES ||
			kl_key->algo == CA_ALGO_CSA3) {
			p_inst_key->key_size = CA_KEY_SIZE_BYTE_16;
		} else if (kl_key->algo == CA_ALGO_DES ||
			kl_key->algo == CA_ALGO_CSA1 ||
			kl_key->algo == CA_ALGO_CSA2) {
			p_inst_key->key_size = CA_KEY_SIZE_BYTE_8;
		}

		if (kl_key->valid_mask & CA_VALID_IV_EVEN) {
			memcpy(p_inst_key->iv_even,
				kl_key->iv_even,
				CA_IV_SIZE_MAX);
		}

		if (kl_key->valid_mask & CA_VALID_IV_ODD) {
			memcpy(p_inst_key->iv_odd,
				kl_key->iv_odd,
				CA_IV_SIZE_MAX);
		}
	}

	p_inst_key->key_id = ida_simple_get(&s->key_ida, 1, 0, GFP_KERNEL);
	p_inst_key->key_handle = SCR_INVALID_KEY_HDL;

	INIT_LIST_HEAD(&p_inst_key->pid_list);
	p_inst_key->pid_size = 0;

	p_inst_key->s = s;
	return p_inst_key;
}

void scr_key_delete(struct scr_inst_key *inst_key)
{
	struct scr_pids *ppid, *_ppid;
	struct ca_scr_session *s;

	if (inst_key == NULL)
		return;

	s = inst_key->s;

	/*clean the pid_list*/
	list_for_each_entry_safe(ppid, _ppid,
		&inst_key->pid_list, pid_node) {
		kfree(ppid);
	}

	if (inst_key->cell)
		put_key_cell(inst_key->cell);

	ida_simple_remove(&s->key_ida, inst_key->key_id);

	kfree(inst_key);
}

static int scr_key_is_pid_exist(struct scr_inst_key *inst_key,
	unsigned short pid, unsigned char ltsid)
{
	struct scr_pids *ppid;

	list_for_each_entry(ppid, &inst_key->pid_list, pid_node) {

		/*this pid already exist*/
		if (inst_key->s->format == CA_FORMAT_TS188)
			if (ppid->pid == pid)
				return 1;

		/*this pid+ltsid already exist*/
		if (inst_key->s->format == CA_FORMAT_TS200 ||
			inst_key->s->format == CA_FORMAT_TS188_LTSID)
			if (ppid->pid == pid &&
				ppid->ltsid == ltsid)
				return 1;
	}

	return 0;
}

static int scr_key_add_pid(struct scr_inst_key *inst_key,
	unsigned short pid, unsigned char ltsid, unsigned char tsc)
{
	struct scr_pids *ppid = NULL;

	if (inst_key->pid_size == CA_PID_MAX)
		return -ENOBUFS; /*Full ...*/

	if (scr_key_is_pid_exist(inst_key, pid, ltsid))
		return -EPERM; /*already exist*/

	/*not exist, add it.*/
	ppid = kmalloc(sizeof(struct scr_pids),
		GFP_KERNEL);
	if (!ppid)
		return -ENOMEM;

	ppid->pid = pid;
	ppid->ltsid = ltsid;
	ppid->tsc = tsc;

	list_add_tail(&ppid->pid_node, &inst_key->pid_list);
	inst_key->pid_size++;

	return 0;
}

static void scr_key_del_pid(struct scr_inst_key *inst_key,
	unsigned short pid, unsigned char ltsid)
{
	struct scr_pids *ppid;

	list_for_each_entry(ppid, &inst_key->pid_list, pid_node) {

		if (ppid->pid == pid) {
			if (ppid->ltsid == ltsid ||
				inst_key->s->format == CA_FORMAT_TS188) {

				list_del(&ppid->pid_node);
				inst_key->pid_size--;
				kfree(ppid);
				break;
			}
		}
	}
}

static struct scr_inst_key *scr_key_find_by_handle(
	struct ca_scr_session *s, int key_id)
{
	struct scr_inst_key *key;
	int found = 0;

	list_for_each_entry(key, &s->key_list, key_node) {
		if (key->key_id == key_id) {
			found = 1;
			break;
		}
	}

	if (!found) {
		dev_dbg(s->scr->dev, "key[%d] not found\n", key_id);
		return NULL;
	}

	return key;
}

static int scr_keep_consistent(struct ca_scr_session *s)
{
	int ret;

	ret = s->engine.ops->is_busy(&s->engine);
	while (ret == -EBUSY) {
		if (schedule_timeout_interruptible(SCR_SCHE_DELAY)) {
			ret = -ERESTARTSYS;
			return ret;
		}

		ret = s->engine.ops->is_busy(&s->engine);
	}

	return 0;
}

static int scr_format_set(struct ca_scr_session *s,
	int ca_format)
{
	int format;
	unsigned char ts_chaining;
	unsigned char sc_mode;

	if (!s)
		return -EINVAL;

	if (s->fmt_flag) {
		dev_dbg(s->scr->dev, "cannot set twice!\n");
		return -EPERM;
	}

	format = ca_format & CA_FORMAT_MASK;
	ts_chaining = (ca_format & CA_FORMAT_TS_CHAINING) ? 1 : 0;
	sc_mode = (ca_format & CA_FORMAT_CLEAR_UNTOUCHED) ? 1 : 0;

	if (SCR_INVALID_FORMAT(format)) {
		dev_dbg(s->scr->dev, "Invalid ca_format!\n");
		return -EINVAL;
	}

	if ((format == CA_FORMAT_RAW) && ts_chaining) {
		dev_dbg(s->scr->dev,
			"Raw format not support chaining!\n");
		return -EINVAL;
	}

	if (format == CA_FORMAT_RAW)
		s->pkt_size = PAGE_SIZE;
	else if (format == CA_FORMAT_TS200)
		s->pkt_size = 200;
	else
		s->pkt_size = 188;

	s->format = format;
	s->ts_chaining = ts_chaining;
	s->sc_mode = sc_mode;
	s->fmt_flag = 1;

	if (s->ts_chaining)
		s->opt = CA_SET_CORK;
	else
		s->opt = CA_SET_UNCORK;

	return 0;
}

static int scr_opt_set(struct ca_scr_session *s,
	int ca_opt)
{
	int ret = 0;

	if (!s)
		return -EINVAL;

	if (!s->ts_chaining) {
		dev_dbg(s->scr->dev,
			"session not in chaining mode\n");
		return -EPERM;
	}

	if (ca_opt != CA_SET_CORK && ca_opt != CA_SET_UNCORK) {
		dev_dbg(s->scr->dev, "Invalid ca_opt!\n");
		return -EINVAL;
	}

	if (ca_opt == CA_SET_UNCORK)
		ca_scr_se_queue_last(&s->engine);

	ret = scr_keep_consistent(s);
	if (ret) {
		dev_dbg(s->scr->dev,
			"cannot set opt now!\n");
		return ret;
	}

	if (ca_opt == CA_SET_UNCORK &&
		s->see_sess_id != SCR_INVALID_SESSION_ID) {
		ret = _scr_session_contns_renew(s->scr,
			s->see_sess_id);
		if (ret != 0) {
			dev_dbg(s->scr->dev, "renew contns err![%x]\n", ret);
			ret = -EIO;
		}
	}

	s->opt = ca_opt;

	return ret;
}

static int scr_parity_sc(struct ca_scr_session *s,
	int crypt_mode, int parity, int checksingle)
{
	if (SCR_INVALID_PARITY(parity))
		return -EINVAL;

	/*In descrambling, HW only support AUTO parity, that
	means HW always uses even keys to descramble
	even packets; uses odd keys to descramble odd packets.
	*/
	if (crypt_mode == CA_DECRYPT &&
		parity != CA_PARITY_AUTO)
		return -EPERM;

	/*In scrambling mode 1, HW only support EVEN/ODD parity,
	that	means HW uses even keys to scramble
	all packets when parity is EVEN;
	or
	uses odd keys to scramble all packets when parity is ODD.
	*/
	if (crypt_mode == CA_ENCRYPT &&
		s->sc_mode == 0 &&
		parity != CA_PARITY_EVEN &&
		parity != CA_PARITY_ODD)
		return -EPERM;

	/*In scrambling mode 2, HW only support AUTO parity, that
	means HW always uses even keys to scramble
	even packets; uses odd keys to scramble odd packets.
	*/
	if (crypt_mode == CA_ENCRYPT &&
		s->sc_mode == 1 &&
		parity != CA_PARITY_AUTO)
		return -EPERM;

	/*single parity per session*/
	if (s->parity != SCR_INVALID_PARITY_FLAG &&
		s->parity != parity &&
		checksingle) {
		dev_dbg(s->scr->dev,
			"single parity allowed in one session!!\n");
		return -EPERM;
	}

	return 0;
}

static int scr_key_tsc_sc(struct ca_scr_session *s,
	int crypt_mode, int tsc_flag)
{
	if (SCR_INVALID_TSC(tsc_flag))
		return -EINVAL;

	/*In descrambling, tsc flag support CLEAR/EVEN/ODD*/
	if (crypt_mode == CA_DECRYPT &&
		tsc_flag == CA_TSC_FLAG_AUTO)
		return -EPERM;

	/*In scrambling mode 1, tsc flag support CLEAR/EVEN/ODD*/
	if (crypt_mode == CA_ENCRYPT &&
		s->sc_mode == 0 &&
		tsc_flag == CA_TSC_FLAG_AUTO)
		return -EPERM;

	/*In scrambling mode 2, tsc flag only support AUTO*/
	if (crypt_mode == CA_ENCRYPT &&
		s->sc_mode == 1 &&
		tsc_flag != CA_TSC_FLAG_AUTO)
		return -EPERM;

	return 0;
}

static int scr_pid_tsc_sc(struct ca_scr_session *s,
	struct scr_inst_key *key, int tsc_flag)
{
	if (SCR_INVALID_TSC(tsc_flag))
		return -EINVAL;

	/*pid tsc fall back to key tsc_flag when pid tsc AUTO*/
	if (tsc_flag == CA_TSC_FLAG_AUTO)
		tsc_flag = key->tsc_flag;

	/* check if incoming tsc_flag conflict with session->tsc_flag */
	if (s->tsc_flag != SCR_INVALID_TSC_FLAG &&
		s->tsc_flag != tsc_flag) {
		dev_dbg(s->scr->dev, "session tsc_flag[%d] exist!\n",
			s->tsc_flag);
		return -EEXIST;
	} else if (s->sc_mode == 1) {
		/*session in clear_untouch mode,
		pid tsc has to be AUTO in this case.
		*/
		if (tsc_flag != CA_TSC_FLAG_AUTO)
			return -EINVAL;
	}

	s->tsc_flag = tsc_flag;

	return 0;
}


static int scr_clear_key_sc(struct ca_scr_session *s,
	struct ca_create_clear_key *clear_key)
{
	int ret = -EINVAL;

	if (!clear_key || !s) {
		dev_dbg(s->scr->dev, "argument NULL\n");
		return -EINVAL;
	}

	/*before this, ca format should be set first*/
	if (!s->fmt_flag) {
		dev_dbg(s->scr->dev, "CA format not set!\n");
		return -EPERM;
	}

	if (!(clear_key->valid_mask & CA_VALID_PARITY) ||
		!(clear_key->valid_mask & CA_VALID_TSC_FLAG) ||
		!(clear_key->valid_mask & CA_VALID_CRYPT_MODE) ||
		!(clear_key->valid_mask & CA_VALID_CHAINING_MODE) ||
		!(clear_key->valid_mask & CA_VALID_RESIDUE_MODE)) {
		dev_dbg(s->scr->dev, "Invalid valid_mask\n");
		return -EINVAL;
	}

	if (SCR_INVALID_ALGO(clear_key->algo)) {
		dev_dbg(s->scr->dev, "Invalid algo\n");
		return -EINVAL;
	}

	if (SCR_INVALID_CRYPTO(clear_key->crypt_mode)) {
		dev_dbg(s->scr->dev, "Invalid crypt_mode\n");
		return -EINVAL;
	}

	if (SCR_INVALID_CHAINING(clear_key->chaining_mode)) {
		dev_dbg(s->scr->dev, "Invalid chaining_mode\n");
		return -EINVAL;
	}

	if (SCR_INVALID_RESIDUE(clear_key->residue_mode)) {
		dev_dbg(s->scr->dev, "Invalid residue, %d\n",
			clear_key->residue_mode);
		return -EINVAL;
	}

	if (clear_key->algo == CA_ALGO_AES) {
		if (SCR_INVALID_AES_KEY_SIZE(clear_key->key_size)) {
			dev_dbg(s->scr->dev, "Invalid key sz for AES[%d]\n",
				clear_key->key_size);
			return ret;
		}
	}

	if (clear_key->algo == CA_ALGO_DES) {
		if (SCR_INVALID_DES_KEY_SIZE(clear_key->key_size))	{
			dev_dbg(s->scr->dev, "Invalid key sz for DES[%d]\n",
				clear_key->key_size);
			return ret;
		}
	}

	if (clear_key->algo == CA_ALGO_TDES) {
		if (SCR_INVALID_TDES_KEY_SIZE(clear_key->key_size)) {
			dev_dbg(s->scr->dev, "Invalid key sz for TDES[%d]\n",
				clear_key->key_size);
			return ret;
		}
	}

	/*more than one key*/
	if (!list_empty(&s->key_list)) {

		/*only one key allowed in pure data mode*/
		if (s->format == CA_FORMAT_RAW) {
			dev_dbg(s->scr->dev, "one key allow in RAW session!!\n");
			return -EPERM;
		}
	}

	/*single crypto_mode per session*/
	if (s->crypt_mode != SCR_INVALID_CRYPTO_MODE_FLAG &&
		s->crypt_mode != clear_key->crypt_mode) {
		dev_dbg(s->scr->dev,
			"single crypto mode allowed in one session!!\n");
		return -EPERM;
	}

	/*single algo per session*/
	if (s->algo != SCR_INVALID_ALGO_FLAG &&
		s->algo != clear_key->algo) {
		dev_dbg(s->scr->dev,
			"single algo allowed in one session!!\n");
		return -EPERM;
	}

	ret = scr_parity_sc(s, clear_key->crypt_mode,
		clear_key->parity, 1);
	if (ret) {
		dev_dbg(s->scr->dev, "Invalid parity\n");
		return ret;
	}

	ret = scr_key_tsc_sc(s, clear_key->crypt_mode, clear_key->tsc_flag);
	if (ret) {
		dev_dbg(s->scr->dev, "Invalid tsc\n");
		return ret;
	}

	s->algo = clear_key->algo;
	s->crypt_mode = clear_key->crypt_mode;
	s->chaining_mode = clear_key->chaining_mode;
	s->residue_mode = clear_key->residue_mode;
	s->parity = clear_key->parity;

	if (s->chaining_mode == CA_MODE_CTR)
		s->residue_mode = CA_RESIDUE_CTR_HDL;

	return 0;
}

static int scr_clear_key_set(struct ca_scr_session *s,
	struct ca_create_clear_key *clear_key)
{
	int ret;
	struct scr_inst_key *p_inst_key = NULL;

	ret = scr_clear_key_sc(s, clear_key);
	if (ret < 0)
		return ret;

	/*create lower session*/
	if (s->see_sess_id == SCR_INVALID_SESSION_ID) {
		ret = low_scr_create_session(s);
		if (ret != 0)
			return ret;
	}

	/*create a new key*/
	p_inst_key = scr_key_new((void *)clear_key,
		SCR_INST_CLEAR_KEY, s);
	if (!p_inst_key) {
		dev_dbg(s->scr->dev, "kmalloc error!\n");
		return -ENOMEM;
	}

	/* add lower session key*/
	ret = low_scr_add_key(s, p_inst_key);
	if (ret != 0) {
		scr_key_delete(p_inst_key);
		return ret;
	}

	/*add pid for pure data.*/
	if (s->format == CA_FORMAT_RAW)
		low_scr_add_pid(s, p_inst_key, 0x1234, 0, 0);

	/*Insert into the key_list*/
	list_add(&p_inst_key->key_node, &s->key_list);

	return p_inst_key->key_id;
}

static int scr_kl_key_sc(struct ca_scr_session *s,
	struct ca_create_kl_key *kl_key)
{
	int ret;

	if (NULL == kl_key || NULL == s) {
		dev_dbg(s->scr->dev, "argument NULL\n");
		return -EINVAL;
	}

	/*before this, ca format should be set first*/
	if (!s->fmt_flag) {
		dev_dbg(s->scr->dev, "CA format not set!\n");
		return -EPERM;
	}

	if (!(kl_key->valid_mask & CA_VALID_PARITY) ||
		!(kl_key->valid_mask & CA_VALID_TSC_FLAG) ||
		!(kl_key->valid_mask & CA_VALID_CRYPT_MODE) ||
		!(kl_key->valid_mask & CA_VALID_CHAINING_MODE) ||
		!(kl_key->valid_mask & CA_VALID_RESIDUE_MODE)) {
		dev_dbg(s->scr->dev, "Invalid valid_mask\n");
		return -EINVAL;
	}

	/*more than one key*/
	if (!list_empty(&s->key_list)) {

		/*only one key allowed in pure data mode*/
		if (s->format == CA_FORMAT_RAW) {
			dev_dbg(s->scr->dev, "one key allow in RAW session!!\n");
			return -EPERM;
		}
	}

	if (SCR_INVALID_ALGO(kl_key->algo)) {
		dev_dbg(s->scr->dev, "Invalid algo\n");
		return -EINVAL;
	}

	if (SCR_INVALID_CRYPTO(kl_key->crypt_mode)) {
		dev_dbg(s->scr->dev, "Invalid crypt_mode\n");
		return -EINVAL;
	}

	if (SCR_INVALID_CHAINING(kl_key->chaining_mode)) {
		dev_dbg(s->scr->dev, "Invalid chaining_mode\n");
		return -EINVAL;
	}

	if (SCR_INVALID_RESIDUE(kl_key->residue_mode)) {
		dev_dbg(s->scr->dev, "Invalid residue, %d\n",
			kl_key->residue_mode);
		return -EINVAL;
	}

	/*single crypto_mode per session*/
	if (s->crypt_mode != SCR_INVALID_CRYPTO_MODE_FLAG &&
		s->crypt_mode != kl_key->crypt_mode) {
		dev_dbg(s->scr->dev,
			"single crypto mode allowed in one session!!\n");
		return -EPERM;
	}

	/*single algo per session*/
	if (s->algo != SCR_INVALID_ALGO_FLAG &&
		s->algo != kl_key->algo) {
		dev_dbg(s->scr->dev,
			"single algo allowed in one session!!\n");
		return -EPERM;
	}

	ret = scr_parity_sc(s, kl_key->crypt_mode,
		kl_key->parity, 1);
	if (ret) {
		dev_dbg(s->scr->dev, "Invalid parity\n");
		return ret;
	}

	ret = scr_key_tsc_sc(s, kl_key->crypt_mode, kl_key->tsc_flag);
	if (ret) {
		dev_dbg(s->scr->dev, "Invalid tsc\n");
		return ret;
	}

	s->algo = kl_key->algo;
	s->crypt_mode = kl_key->crypt_mode;
	s->chaining_mode = kl_key->chaining_mode;
	s->residue_mode = kl_key->residue_mode;
	s->parity = kl_key->parity;

	if (s->chaining_mode == CA_MODE_CTR)
		s->residue_mode = CA_RESIDUE_CTR_HDL;

	return 0;
}

static int scr_kl_key_set(struct ca_scr_session *s,
	struct ca_create_kl_key *kl_key)
{
	int ret;
	struct scr_inst_key *p_inst_key = NULL;

	ret = scr_kl_key_sc(s, kl_key);
	if (ret < 0)
		return ret;

	/*create lower session*/
	if (s->see_sess_id == SCR_INVALID_SESSION_ID) {
		ret = low_scr_create_session(s);
		if (ret != 0)
			return ret;
	}

	/*Malloc a new session key*/
	p_inst_key = scr_key_new((void *)kl_key,
		SCR_INST_KL_KEY, s);
	if (!p_inst_key) {
		dev_dbg(s->scr->dev, "kmalloc error!\n");
		return -ENOMEM;
	}

	/* add lower session key*/
	ret = low_scr_add_key(s, p_inst_key);
	if (ret != 0) {
		scr_key_delete(p_inst_key);
		return ret;
	}

	/*add pid for pure data.*/
	if (s->format == CA_FORMAT_RAW)
		low_scr_add_pid(s, p_inst_key, 0x1234, 0, 0);

	/*Insert into the key_list*/
	list_add(&p_inst_key->key_node, &s->key_list);

	return p_inst_key->key_id;
}

static int scr_pid_sc(struct ca_scr_session *s,
	struct ca_pid *pid_info,
	struct scr_inst_key **target_key, int sc_type)
{
	int ret;
	struct scr_inst_key *key, *tmpkey;

	if (!s || !pid_info || !target_key) {
		dev_dbg(s->scr->dev, "argument NULL\n");
		return -EINVAL;
	}

	if (s->format == CA_FORMAT_RAW) {
		dev_dbg(s->scr->dev, "Raw, could not add/del PID!\n");
		return -EACCES;
	}

	key = scr_key_find_by_handle(s, pid_info->key_handle);
	if (!key)
		return -EINVAL;

	if (SCR_INVALID_TSC(pid_info->tsc_flag)) {
		dev_dbg(s->scr->dev, "tsc_flag invalid\n");
		return -EINVAL;
	}

	if (1 == sc_type) {
		ret = scr_pid_tsc_sc(s, key, pid_info->tsc_flag);
		if (ret)
			return ret;

		/*In case PID full when setting PID*/
		if ((key->pid_size + 1) > CA_PID_MAX) {
			dev_dbg(s->scr->dev, "key_hdl[0x%x] PID full!!\n",
			    pid_info->key_handle);
			return -EINVAL;
		}

		/*In case same PID already associated with keyID, e.g.
		keyID1 ~ PID 1/2/3
		KeyID2 ~ PID 2/4/5
		   --> Pid 2 is not allowed, as the same streamID and
		   same PID in different keypos.
		   The HW doesnot know how to fetch.
		*/
		list_for_each_entry(tmpkey, &s->key_list, key_node) {

			if (scr_key_is_pid_exist(tmpkey, pid_info->pid,
					pid_info->ltsid)) {
				dev_dbg(s->scr->dev, "pid[%d] exists in key[0x%x]!\n",
					pid_info->pid, tmpkey->key_id);
				return -EINVAL;
			}
		}
	}

	*target_key = key;
	return 0;
}

static int scr_pid_add(struct ca_scr_session *s,
	struct ca_pid *pid_info)
{
	int ret = 0;
	struct scr_inst_key *key = NULL;

	ret = scr_pid_sc(s, pid_info, &key, 1);
	if (ret < 0)
		return ret;

	ret = low_scr_add_pid(s, key, pid_info->pid,
		pid_info->ltsid, s->tsc_flag & 0x3);
	if (ret)
		return ret;

	/*add it if lower add successfully*/
	ret = scr_key_add_pid(key, pid_info->pid,
			pid_info->ltsid, pid_info->tsc_flag);
	if (ret) {
		dev_dbg(s->scr->dev, "Add pid failed!\n");
		return ret;
	}

	return 0;
}

static int scr_pid_del(struct ca_scr_session *s,
	struct ca_pid *pid_info)
{
	int ret, no_pid = 1;
	struct scr_inst_key *key;

	ret = scr_pid_sc(s, pid_info, &key, 0);
	if (ret < 0)
		return ret;

	if (!scr_key_is_pid_exist(key,
			pid_info->pid, pid_info->ltsid)) {
		dev_dbg(s->scr->dev, "pid is not exist!");
		return -EINVAL; /*this pid is not exist*/
	}

	/*del pid*/
	scr_key_del_pid(key, pid_info->pid, pid_info->ltsid);

	ret = low_scr_del_pid(s, key, pid_info->pid,
		pid_info->ltsid, pid_info->tsc_flag);

	/* invalid tsc_flag if no more pid in the session */
	list_for_each_entry(key, &s->key_list, key_node)
		if (!list_empty(&key->pid_list))
			no_pid = 0;
	if (no_pid)
		s->tsc_flag = SCR_INVALID_TSC_FLAG;

	return ret;
}

static int scr_clear_key_update_sc(struct ca_scr_session *s,
	struct ca_update_clear_key *up)
{
	if (!up || !s) {
		dev_dbg(s->scr->dev, "argument NULL\n");
		return -EINVAL;
	}

	return 0;
}

static int scr_clear_key_update_single(
	struct ca_scr_session *s,
	struct ca_update_clear_key *up,
	struct scr_inst_key *key)
{
	int ret;
	struct scr_update_info user_up;

	if (CA_VALID_KEY_ODD & up->valid_mask) {
		memcpy(key->key_odd,
			up->key_odd,
			CA_KEY_SIZE_MAX);
		key->no_odd = 0; /*update the odd key*/
		key->odd_locate = 0; /*odd key is host key*/
	}

	if (CA_VALID_KEY_EVEN & up->valid_mask) {
		memcpy(key->key_even,
			up->key_even,
			CA_KEY_SIZE_MAX);
		key->no_even = 0; /*update the even key*/
		key->even_locate = 0; /*even key is host key*/
	}

	/*update Key params to SEE*/
	memset(&user_up, 0, sizeof(struct scr_update_info));
	if (CA_VALID_KEY_ODD & up->valid_mask) {
		memcpy(user_up.key_odd, up->key_odd,
			CA_KEY_SIZE_MAX);

		user_up.up_mask |= SCR_UP_KEY_ODD;
	}

	if (CA_VALID_KEY_EVEN & up->valid_mask) {
		memcpy(user_up.key_even, up->key_even,
			CA_KEY_SIZE_MAX);

		user_up.up_mask |= SCR_UP_KEY_EVEN;
	}

	user_up.key_handle = key->key_handle;
	user_up.no_even = key->no_even;
	user_up.no_odd = key->no_odd;
	user_up.even_locate = key->even_locate;
	user_up.odd_locate = key->odd_locate;

	ret = _scr_session_update_key(s->scr, s->see_sess_id,
		&user_up);
	if (ret != 0) {
		dev_dbg(s->scr->dev,
			"low sess up key error[%x]\n", ret);
		return -EIO;
	}

	return 0;
}


static int scr_clear_key_update(struct ca_scr_session *s,
	struct ca_update_clear_key *up)
{
	int ret;
	struct scr_inst_key *pkey = NULL;

	ret = scr_clear_key_update_sc(s, up);
	if (ret < 0)
		return ret;

	if (up->key_handle != CA_ALL_KEYS) {
		pkey = scr_key_find_by_handle(s,
			up->key_handle);
		if (!pkey)
			return -EINVAL;

		return scr_clear_key_update_single(s, up, pkey);
	} else {
		list_for_each_entry(pkey, &s->key_list, key_node) {
			ret = scr_clear_key_update_single(s, up, pkey);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int scr_params_update_sc(struct ca_scr_session *s,
	struct ca_update_params *up, struct scr_inst_key *pkey)
{
	int ret;
	int can_update = 1;
	struct scr_inst_key *pItKey = NULL;

	if (!up || !s) {
		dev_dbg(s->scr->dev, "argument NULL\n");
		return -EINVAL;
	}

	if (SCR_INVALID_RESIDUE(up->residue_mode) &&
		(up->valid_mask & CA_VALID_RESIDUE_MODE)) {
		dev_dbg(s->scr->dev, "Invalid residue, %d\n",
			up->residue_mode);
		return -EINVAL;
	}

	if (SCR_INVALID_CHAINING(up->chaining_mode) &&
		(up->valid_mask & CA_VALID_CHAINING_MODE)) {
		dev_dbg(s->scr->dev, "Invalid chaining_mode\n");
		return -EINVAL;
	}

	if (SCR_INVALID_CRYPTO(up->crypt_mode) &&
		(up->valid_mask & CA_VALID_CRYPT_MODE))
		return -EINVAL;

	if (pkey) {
		/*check if crypto_mode/parity/tsc can be updated for this key.
		We should update them if they do not conflict with other keys.
		*/
		if ((up->valid_mask & CA_VALID_CRYPT_MODE) &&
			s->crypt_mode != SCR_INVALID_CRYPTO_MODE_FLAG &&
			s->crypt_mode != up->crypt_mode) {

			can_update = 1;
			list_for_each_entry(pItKey, &s->key_list, key_node) {
				if (pItKey->crypt_mode == s->crypt_mode &&
					pItKey != pkey) {
					can_update = 0;
					break;
				}
			}

			if (!can_update)
				return -EEXIST;

			s->crypt_mode = up->crypt_mode;
		}

		if ((up->valid_mask & CA_VALID_PARITY) &&
			s->parity != SCR_INVALID_PARITY_FLAG &&
			s->parity != up->parity) {

			can_update = 1;
			list_for_each_entry(pItKey, &s->key_list, key_node) {
				if (pItKey->parity == s->parity &&
					pItKey != pkey) {
					can_update = 0;
					break;
				}
			}

			if (!can_update)
				return -EEXIST;

			ret = scr_parity_sc(s, s->crypt_mode, up->parity, 0);
			if (ret) {
				dev_dbg(s->scr->dev, "Invalid parity\n");
				return ret;
			}

			s->parity = up->parity;
		}

		if ((up->valid_mask & CA_VALID_TSC_FLAG) &&
			s->tsc_flag != SCR_INVALID_TSC_FLAG &&
			s->tsc_flag != up->tsc_flag){

			can_update = 1;
			list_for_each_entry(pItKey, &s->key_list, key_node) {
				if (pItKey->tsc_flag == s->tsc_flag &&
					pItKey != pkey) {
					can_update = 0;
					break;
				}
			}

			if (!can_update)
				return -EEXIST;

			ret = scr_key_tsc_sc(s, s->crypt_mode, up->tsc_flag);
			if (ret) {
				dev_dbg(s->scr->dev, "Invalid tsc\n");
				return ret;
			}

			s->tsc_flag = up->tsc_flag;
		}
	} else {
		/*update all keys' crypto_mode/parity/tsc*/
		if (up->valid_mask & CA_VALID_CRYPT_MODE)
			s->crypt_mode = up->crypt_mode;

		if (up->valid_mask & CA_VALID_PARITY) {
			ret = scr_parity_sc(s, s->crypt_mode, up->parity, 0);
			if (ret) {
				dev_dbg(s->scr->dev, "Invalid parity\n");
				return ret;
			}
		}

		if (up->valid_mask & CA_VALID_TSC_FLAG) {
			ret = scr_key_tsc_sc(s, s->crypt_mode, up->tsc_flag);
			if (ret) {
				dev_dbg(s->scr->dev, "Invalid tsc\n");
				return ret;
			}
		}
	}

	return 0;
}

static int scr_params_update_single(
	struct ca_scr_session *s,
	struct ca_update_params *up,
	struct scr_inst_key *key)
{
	int ret;
	struct scr_update_info user_up;

	memset(&user_up, 0, sizeof(struct scr_update_info));

	/* update parameters */
	if (up->valid_mask & CA_VALID_RESIDUE_MODE) {
		s->residue_mode = up->residue_mode;
		key->residue = up->residue_mode;

		user_up.residue = key->residue;
		user_up.up_mask |= SCR_UP_RESIDUE;
	}

	if (up->valid_mask & CA_VALID_CHAINING_MODE) {
		s->chaining_mode = up->chaining_mode;
		key->chaining = up->chaining_mode;

		user_up.chaining = key->chaining;
		user_up.up_mask |= SCR_UP_CHAINING;
	}

	if (up->valid_mask & CA_VALID_IV_ODD) {
		memcpy(key->iv_odd, up->iv_odd,
			CA_IV_SIZE_MAX);

		memcpy(user_up.iv_odd, up->iv_odd,
			CA_IV_SIZE_MAX);

		user_up.up_mask |= SCR_UP_IV_ODD;
	}

	if (up->valid_mask & CA_VALID_IV_EVEN) {
		memcpy(key->iv_even, up->iv_even,
			CA_IV_SIZE_MAX);

		memcpy(user_up.iv_even, up->iv_even,
			CA_IV_SIZE_MAX);

		user_up.up_mask |= SCR_UP_IV_EVEN;
	}

	if (up->valid_mask & CA_VALID_CRYPT_MODE) {
		s->crypt_mode = up->crypt_mode;
		key->crypt_mode = up->crypt_mode;
	}

	if (up->valid_mask & CA_VALID_PARITY) {
		s->parity = up->parity;
		key->parity = up->parity;

		user_up.parity = s->parity;
		user_up.up_mask |= SCR_UP_PARITY;
	}

	if (up->valid_mask & CA_VALID_TSC_FLAG) {
		s->tsc_flag = up->tsc_flag;
		key->tsc_flag = up->tsc_flag;

		user_up.tsc_flag = s->tsc_flag;
		user_up.up_mask |= SCR_UP_TSC_FLAG;
	}

	if (key->cell)
		user_up.key_pos = key->cell->pos;
	user_up.no_even = key->no_even;
	user_up.no_odd = key->no_odd;
	user_up.even_locate = key->even_locate;
	user_up.odd_locate = key->odd_locate;
	user_up.key_handle = key->key_handle;

	ret = _scr_session_update_key(s->scr, s->see_sess_id,
		&user_up);
	if (ret != 0) {
		dev_dbg(s->scr->dev, "low sess up param error!\n");
		return -EIO;
	}

	return 0;
}


static int scr_params_update(struct ca_scr_session *s,
	struct ca_update_params *up)
{
	int ret;
	struct scr_inst_key *pkey;

	/*iterate the key list*/
	if (up->key_handle != CA_ALL_KEYS) {
		pkey = scr_key_find_by_handle(s,
			up->key_handle);
		if (!pkey)
			return -EINVAL;

		ret = scr_params_update_sc(s, up, pkey);
		if (ret < 0)
			return ret;

		return scr_params_update_single(s, up, pkey);
	} else {
		ret = scr_params_update_sc(s, up, NULL);
		if (ret < 0)
			return ret;

		list_for_each_entry(pkey, &s->key_list, key_node) {
			ret = scr_params_update_single(s, up, pkey);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int scr_unset_key_single(struct ca_scr_session *s,
	struct scr_inst_key *key)
{
	int ret;

	/*unset this key, and the associated pids*/
	ret = low_scr_del_key(s, key);
	if (ret != 0) {
		dev_dbg(s->scr->dev, "delete key[%x] error!\n", key->key_id);
		return ret;
	}

	/*Delete this key from the key_list*/
	list_del(&key->key_node);
	scr_key_delete(key);

	return 0;
}

static int scr_unset_key(struct ca_scr_session *s,
	int key_id)
{
	struct scr_inst_key *pkey = NULL, *_pkey = NULL;
	int ret;
	int no_more = 1;

	if (key_id != CA_ALL_KEYS) {
		pkey = scr_key_find_by_handle(s, key_id);
		if (!pkey)
			return -EINVAL;

		ret = scr_unset_key_single(s, pkey);
		if (ret)
			return ret;

	} else {
		list_for_each_entry_safe(pkey, _pkey, &s->key_list, key_node) {

			ret = scr_unset_key_single(s, pkey);
			if (ret)
				return ret;
		}
	}

	/* delete session if no more key in the session,
	as well as allow crypto_mode && parity && algo change to a
	different value than before.
	*/
	if (list_empty(&s->key_list)) {
		low_scr_delete_session(s);

		s->algo = SCR_INVALID_ALGO_FLAG;
		s->crypt_mode = SCR_INVALID_CRYPTO_MODE_FLAG;
		s->parity = SCR_INVALID_PARITY_FLAG;
	}

	/* check if there is other key's crypto_mode
	equals to session's crypto_mode,
	clean the session's crypto_mode if there is none.
	*/
	no_more = 1;
	list_for_each_entry(pkey, &s->key_list, key_node) {
		if (pkey->crypt_mode == s->crypt_mode) {
			no_more = 0;
			break;
		}
	}

	if (no_more)
		s->crypt_mode = SCR_INVALID_CRYPTO_MODE_FLAG;

	/* check if there is other key's tsc_flag equals to session's tsc_flag,
	clean the session's tsc_flag if there is none.
	*/
	no_more = 1;
	list_for_each_entry(pkey, &s->key_list, key_node) {
		if (pkey->tsc_flag == s->tsc_flag) {
			no_more = 0;
			break;
		}
	}

	if (no_more)
		s->tsc_flag = SCR_INVALID_TSC_FLAG;

	/* check if there is other key's parity equals to session's parity,
	clean the session's parity if there is none.
	*/
	no_more = 1;
	list_for_each_entry(pkey, &s->key_list, key_node) {
		if (pkey->parity == s->parity) {
			no_more = 0;
			break;
		}
	}

	if (no_more)
		s->parity = SCR_INVALID_PARITY_FLAG;

	return 0;
}

long ca_scr_ioctl(struct file *file, unsigned int cmd,
	unsigned long args)
{
	int ret = RET_SUCCESS;
	struct ca_scr_session *s = NULL;

	s = (struct ca_scr_session *)file->private_data;
	if (!s)
		return -EBADF;

	mutex_lock(&s->wr_mutex);

	switch (cmd) {
	case CA_SET_FORMAT:
	{
		int ca_format;

		ret = ali_scr_umemcpy(&ca_format,
			(void __user *)args, sizeof(int));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_format_set(s, ca_format);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_CREATE_CLEAR_KEY:
	{
		struct ca_create_clear_key clear_key_info;

		memset(&clear_key_info, 0, sizeof(struct ca_create_clear_key));
		ret = ali_scr_umemcpy(&clear_key_info, (void __user *)args,
			sizeof(struct ca_create_clear_key));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_clear_key_set(s, &clear_key_info);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_CREATE_KL_KEY:
	{
		struct ca_create_kl_key kl_key_info;

		memset(&kl_key_info, 0, sizeof(struct ca_create_kl_key));
		ret = ali_scr_umemcpy(&kl_key_info, (void __user *)args,
			sizeof(struct ca_create_kl_key));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_kl_key_set(s, &kl_key_info);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_CREATE_OTP_KEY:
	{
		dev_dbg(s->scr->dev, "SCR does not support OTP key!\n");
		ret = -EPERM;
		break;
	}

	case CA_ADD_PID:
	{
		struct ca_pid pid_info;

		memset(&pid_info, 0, sizeof(struct ca_pid));
		ret = ali_scr_umemcpy(&pid_info, (void __user *)args,
			sizeof(struct ca_pid));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_keep_consistent(s);
		if (ret) {
			dev_dbg(s->scr->dev,
				"cannot add pid now!\n");
			goto DONE;
		}

		ret = scr_pid_add(s, &pid_info);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_DEL_PID:
	{
		struct ca_pid pid_info;

		memset(&pid_info, 0, sizeof(struct ca_pid));
		ret = ali_scr_umemcpy(&pid_info, (void __user *)args,
			sizeof(struct ca_pid));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_keep_consistent(s);
		if (ret) {
			dev_dbg(s->scr->dev,
				"cannot del pid now!\n");
			goto DONE;
		}

		ret = scr_pid_del(s, &pid_info);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_UPDATE_CLEAR_KEY:
	{
		struct ca_update_clear_key update_clear_key;

		memset(&update_clear_key, 0,
			sizeof(struct ca_update_clear_key));
		ret = ali_scr_umemcpy(&update_clear_key, (void __user *)args,
			sizeof(struct ca_update_clear_key));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_keep_consistent(s);
		if (ret) {
			dev_dbg(s->scr->dev,
				"cannot update key now!\n");
			goto DONE;
		}

		ret = scr_clear_key_update(s, &update_clear_key);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_UPDATE_PARAMS:
	{
		struct ca_update_params update_params;

		memset(&update_params, 0, sizeof(struct ca_update_params));
		ret = ali_scr_umemcpy(&update_params, (void __user *)args,
			sizeof(struct ca_update_params));
		if (0 != ret) {
			dev_dbg(s->scr->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = scr_keep_consistent(s);
		if (ret) {
			dev_dbg(s->scr->dev,
				"cannot update parameters now!\n");
			goto DONE;
		}

		ret = scr_params_update(s, &update_params);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_DELETE_KEY:
	{
		int key_id;

		key_id = (int)args;

		ret = scr_keep_consistent(s);
		if (ret) {
			dev_dbg(s->scr->dev,
				"cannot del key now!\n");
			goto DONE;
		}

		ret = scr_unset_key(s, key_id);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_SET_OPT:
	{
		int ca_opt;

		ca_opt = (int)args;
		ret = scr_opt_set(s, ca_opt);
		if (ret < 0)
			goto DONE;
		break;
	}

	default:
	{
		mutex_unlock(&s->wr_mutex);

		ret = ca_scr_ioctl_legacy(file, cmd, args);
		if (ret)
			dev_dbg(s->scr->dev, "invalid ioctl\n");

		return ret;
	}
	}

DONE:
	mutex_unlock(&s->wr_mutex);
	return ret;
}

