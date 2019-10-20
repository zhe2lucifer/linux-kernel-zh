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



#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif

#include "ca_scr_priv.h"
#include "ca_scr_dbgfs.h"
#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

#define SCR_DBG_PRINT_BAISC_STATUS			(0)

static int choice = SCR_DBG_PRINT_BAISC_STATUS;
static int buffer_id;
static void ca_scr_show_format(struct seq_file *f,
	struct ca_scr_session *s)
{
	char cfmt[][32] = {
		[CA_FORMAT_RAW] = "raw",
		[CA_FORMAT_TS188] = "ts188",
		[CA_FORMAT_TS188_LTSID] = "ts188-ltsid",
		[CA_FORMAT_TS200] = "ts200",
	};

	if (s->format < CA_FORMAT_RAW || s->format > CA_FORMAT_TS200)
		seq_printf(f, "%12s: Invalid[%d]\n", "format", s->format);
	else
		seq_printf(f, "%12s: %s\n", "format", cfmt[s->format]);
}

static void ca_scr_show_algo(struct seq_file *f,
	struct ca_scr_session *s)
{
	char calgo[][32] = {
		[CA_ALGO_AES] = "AES",
		[CA_ALGO_DES] = "DES",
		[CA_ALGO_TDES] = "TDES",
		[CA_ALGO_CSA1] = "CSA1",
		[CA_ALGO_CSA2] = "CSA2",
		[CA_ALGO_CSA3] = "CSA3",
	};

	if (s->algo < CA_ALGO_AES || s->algo > CA_ALGO_CSA3)
		seq_printf(f, "%12s: Invalid[%d]\n", "algo", s->algo);
	else
		seq_printf(f, "%12s: %s\n", "algo", calgo[s->algo]);
}

static void ca_scr_show_crypto(struct seq_file *f,
	struct ca_scr_session *s)
{
	if (s->crypt_mode == CA_ENCRYPT)
		seq_printf(f, "%12s: ENCRYPT\n", "crypto");
	else if (s->crypt_mode == CA_DECRYPT)
		seq_printf(f, "%12s: DECRYPT\n", "crypto");
	else
		seq_printf(f, "%12s: Invalid[%d]\n", "crypto", s->crypt_mode);
}

static void ca_scr_show_chaining(struct seq_file *f,
	struct ca_scr_session *s)
{
	char cmode[][32] = {
		[CA_MODE_ECB] = "ECB",
		[CA_MODE_CBC] = "CBC",
		[CA_MODE_OFB] = "OFB",
		[CA_MODE_CFB] = "CFB",
		[CA_MODE_CTR] = "CTR",
		[CA_MODE_CTR8] = "CTR8",
	};

	if (s->chaining_mode < CA_MODE_ECB ||
		s->chaining_mode > CA_MODE_CTR8)
		seq_printf(f, "%12s: Invalid[%d]\n", "mode", s->chaining_mode);
	else
		seq_printf(f, "%12s: %s\n", "mode", cmode[s->chaining_mode]);
}

static void ca_scr_show_residue(struct seq_file *f,
	struct ca_scr_session *s)
{
	char cresidue[][32] = {
		[CA_RESIDUE_CLEAR] = "Clear",
		[CA_RESIDUE_AS_ATSC] = "AS-ATSC",
		[CA_RESIDUE_HW_CTS] = "HW-CTS",
		[CA_RESIDUE_CTR_HDL] = "CTR-HDL",
	};

	if (s->residue_mode < CA_RESIDUE_CLEAR ||
		s->residue_mode > CA_RESIDUE_CTR_HDL)
		seq_printf(f, "%12s: Invalid[%d]\n",
			"residue", s->residue_mode);
	else
		seq_printf(f, "%12s: %s\n", "residue",
			cresidue[s->residue_mode]);
}

static void ca_scr_show_parity(struct seq_file *f,
	struct ca_scr_session *s)
{
	char cparity[][32] = {
		[CA_PARITY_AUTO] = "Auto",
		[CA_PARITY_ODD] = "Odd",
		[CA_PARITY_EVEN] = "Even",
	};

	if (s->parity < CA_PARITY_AUTO || s->parity > CA_PARITY_EVEN)
		seq_printf(f, "%12s: Invalid[%d]\n", "parity", s->parity);
	else
		seq_printf(f, "%12s: %s\n", "parity", cparity[s->parity]);
}

static void ca_scr_show_cork(struct seq_file *f,
	struct ca_scr_session *s)
{
	char ccork[][32] = {
		[CA_SET_UNCORK] = "uncork",
		[CA_SET_CORK] = "cork",
	};

	if (s->opt != CA_SET_CORK && s->opt != CA_SET_UNCORK)
		seq_printf(f, "%12s: Invalid[%d]\n", "opt", s->opt);
	else
		seq_printf(f, "%12s: %s\n", "opt", ccork[s->opt]);
}

static void ca_scr_show_key(struct seq_file *f,
	struct ca_scr_session *s)
{
	struct scr_inst_key *key;
	struct scr_pids *ppid;
	int idx;

	char ckeytype[][32] = {
		[SCR_INST_CLEAR_KEY - SCR_INST_CLEAR_KEY] = "CLEAR",
		[SCR_INST_KL_KEY - SCR_INST_CLEAR_KEY] = "KL",
		[SCR_INST_OTP_KEY - SCR_INST_CLEAR_KEY] = "OTP",
	};

	if (list_empty(&s->key_list)) {
		seq_puts(f, "\tnone\n");
		return;
	}

	list_for_each_entry(key, &s->key_list, key_node) {
		seq_printf(f, "%5s@0x%x:\n", "KEY", key->key_id);
		seq_printf(f, "%20s: %x\n", "handle", key->key_handle);
		seq_printf(f, "%20s: %s\n", "type",
			ckeytype[key->key_type - SCR_INST_CLEAR_KEY]);

		if (key->cell)
			seq_printf(f, "%20s: fd[%d]:kl_sel[%d],pos[0x%x],num[%d]\n",
				"kl_fd", key->kl_fd,
				key->cell->kl_sel, key->cell->pos, key->cell->num);
		else
			seq_printf(f, "%20s: None\n", "kl_fd");

		seq_printf(f, "%20s: %d\n", "no_even", key->no_even);
		seq_printf(f, "%20s: %d\n", "no_odd", key->no_odd);
		seq_printf(f, "%20s: %d\n", "even_locate", key->even_locate);
		seq_printf(f, "%20s: %d\n", "odd_locate", key->odd_locate);

		/*pid*/
		seq_printf(f, "%20s: ", "[ltsid]pid|tsc");
		list_for_each_entry(ppid, &key->pid_list, pid_node)
			seq_printf(f, "[%02x]%04x|%02x ", ppid->ltsid,
				ppid->pid, ppid->tsc);
		seq_puts(f, "\n");

		/*key*/
		seq_printf(f, "%20s: %d\n", "size", key->key_size);
		seq_printf(f, "%20s: ", "even_key");
		for (idx = 0; idx < key->key_size; idx++)
			seq_printf(f, "%02x ", key->key_even[idx]);
		seq_puts(f, "\n");

		seq_printf(f, "%20s: ", "odd_key");
		for (idx = 0; idx < key->key_size; idx++)
			seq_printf(f, "%02x ", key->key_odd[idx]);
		seq_puts(f, "\n");

		/*iv*/
		seq_printf(f, "%20s: ", "even_iv");
		for (idx = 0; idx < CA_IV_SIZE_MAX; idx++)
			seq_printf(f, "%02x ", key->iv_even[idx]);
		seq_puts(f, "\n");

		seq_printf(f, "%20s: ", "odd_iv");
		for (idx = 0; idx < CA_IV_SIZE_MAX; idx++)
			seq_printf(f, "%02x ", key->iv_odd[idx]);
		seq_puts(f, "\n");
	}
}

static void ca_scr_show_buffer(struct seq_file *f,
	struct ca_scr_session *s)
{
	struct ca_scr_se *engine = &s->engine;
	struct ca_scr_se_buffer *sbuf;

	mutex_lock(&engine->queue_lock);

	seq_printf(f, "\tBuffer Totally Enqueued: %d, Bytes: %lld\n",
		engine->total_enqueued_buffers,
		engine->total_enqueued_bytes);
	seq_printf(f, "\tBuffer Totally Dequeued: %d, Bytes: %lld\n",
		engine->total_dequeued_buffers,
		engine->total_dequeued_bytes);
	seq_printf(f, "\tBuffer Remain In Queue: %d, bytes: %lld\n",
		engine->queued_buffers,
		engine->queued_bytes);

	seq_printf(f, "\tBuffer Totally Read Bytes: %lld\n",
		engine->read_bytes);
	seq_printf(f, "\tBuffer Totally Write Bytes: %lld\n",
		engine->write_bytes);
	seq_printf(f, "\tBuffer Totally MMap Bytes: %lld\n",
		engine->mmap_bytes);
	seq_printf(f, "\tBuffer Totally SpliceRead Bytes: %lld\n",
		engine->splice_read_bytes);
	seq_printf(f, "\tBuffer Totally SpliceWrite Bytes: %lld\n",
		engine->splice_write_bytes);

	if (!s->scr->debug_mode)
		goto out;

	if (list_empty(&engine->buf_queue)) {
		seq_puts(f, "\tno buffer in the queue\n");
		goto out;
	}
	seq_puts(f, "\t------------------------------------------------\n");
	seq_puts(f, "\tID\tlen\tInPage[dma_addr]"
		"\tOutPage[dma_addr]\tDONE\tSBMQ\tQueued\tRQ\n");
	list_for_each_entry(sbuf, &engine->buf_queue, node) {
		seq_printf(f, "\t%d\t%d\t%p[%08x]\t%p[%08x]\t%d\t%d\t%d\t%d\n",
			sbuf->id, sbuf->len, sbuf->page, sbuf->dma_hdl,
			sbuf->opage, sbuf->odma_hdl,
			sbuf->done,
			!scr_query_sbm_entry(engine->session, &sbuf->entry),
			sbuf->queued, sbuf->rq);
	}

out:
	mutex_unlock(&engine->queue_lock);
}

static int ca_scr_show_basic(struct seq_file *f,
	struct ca_scr_session *s)
{
	seq_puts(f, "@@--BASIC INFO--@@:\n");
	ca_scr_show_format(f, s);
	ca_scr_show_algo(f, s);
	ca_scr_show_crypto(f, s);
	ca_scr_show_chaining(f, s);
	ca_scr_show_residue(f, s);
	ca_scr_show_parity(f, s);
	ca_scr_show_cork(f, s);
	seq_printf(f, "%12s: %d\n", "tsc_flag", s->tsc_flag);
	seq_printf(f, "%12s: %d\n", "ts_chaining", s->ts_chaining);
	seq_printf(f, "%12s: %d\n", "sc_mode", s->sc_mode);
	seq_printf(f, "%12s: %d\n", "SeeSID", s->see_sess_id);

	seq_puts(f, "@@--KEY INFO--@@:\n");
	ca_scr_show_key(f, s);

	seq_puts(f, "@@--QUEUE BUFFER --@@:\n");
	ca_scr_show_buffer(f, s);

	return 0;
}

static int ca_scr_show_status(struct seq_file *f, void *p)
{
	struct ca_scr_session *s = f->private;

	if (!s)
		return -ENODEV;

	switch (choice) {
	case SCR_DBG_PRINT_BAISC_STATUS:
		ca_scr_show_basic(f, s);
	break;

	default:
		break;
	}

	return 0;
}


static int ca_scr_debugfs_open(struct inode *i, struct file *f)
{
	return single_open(f, ca_scr_show_status, i->i_private);
}

static const struct file_operations ca_scr_status_ops = {
	.open = ca_scr_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.owner = THIS_MODULE,
};

void ca_scr_dbgfs_create(struct ca_scr_dev *scr)
{
	scr->debugfs_dir = debugfs_create_dir("ca_scr", NULL);
	if (!scr->debugfs_dir || IS_ERR(scr->debugfs_dir))
		dev_err(scr->dev, "debugfs create dir failed\n");
}

void ca_scr_dbgfs_remove(struct ca_scr_dev *scr)
{
	debugfs_remove(scr->debugfs_dir);
}

int ca_scr_dbgfs_add_session(struct ca_scr_session *sess)
{
	char name[128];
	struct ca_scr_dev *scr;

	if (!sess)
		return -1;
	scr = sess->scr;
	if (!scr || !scr->debugfs_dir)
		return -1;

	sprintf(name, "session@%d", sess->id);
	sess->session_dir = debugfs_create_dir(name, scr->debugfs_dir);
	if (!sess->session_dir || IS_ERR(sess->session_dir)) {
		dev_err(scr->dev, "create session dir failed\n");
		return -1;
	}

	sprintf(name, "dbg");
	sess->debugfs = debugfs_create_file(name, S_IFREG | S_IRUGO,
		sess->session_dir, (void *)sess, &ca_scr_status_ops);
	if (!sess->debugfs || IS_ERR(sess->debugfs))
		dev_err(scr->dev, "debugfs create file failed\n");

	sprintf(name, "choice");
	sess->choice = debugfs_create_u32(name, S_IFREG | S_IRUGO | S_IWUGO,
		sess->session_dir, &choice);
	if (!sess->choice || IS_ERR(sess->choice))
		dev_err(scr->dev, "debugfs create choice failed\n");

	sprintf(name, "buffer_id");
	sess->buffer_id = debugfs_create_u32(name, S_IFREG | S_IRUGO | S_IWUGO,
		sess->session_dir, &buffer_id);
	if (!sess->buffer_id || IS_ERR(sess->buffer_id))
		dev_err(scr->dev, "debugfs create choice failed\n");

	return 0;
}

int ca_scr_dbgfs_del_session(struct ca_scr_session *sess)
{
	if (!sess)
		return -1;

	debugfs_remove(sess->debugfs);
	debugfs_remove(sess->choice);
	debugfs_remove(sess->buffer_id);

	debugfs_remove(sess->session_dir);

	return 0;
}

