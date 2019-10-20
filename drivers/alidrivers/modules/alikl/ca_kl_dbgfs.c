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

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#endif
#include "ca_kl.h"
#include "ca_kl_priv.h"
#include "ca_kl_dbgfs.h"

static int kl_choice;

static void ca_kl_show_algo(struct seq_file *f,
	struct ca_kl_session *s)
{
	char calgo[][32] = {
		[KL_ALGO_TDES] = "KL-TDES",
		[KL_ALGO_AES] = "KL-AES",
	};

	if (s->algo != KL_ALGO_TDES && s->algo != KL_ALGO_AES)
		seq_printf(f, "%12s: Invalid[%d]\n", "algo", s->algo);
	else
		seq_printf(f, "%12s: %s\n", "algo", calgo[s->algo]);
}

static void ca_kl_show_cksize(struct seq_file *f,
	struct ca_kl_session *s)
{
	char csize[][32] = {
		[KL_CK_KEY64] = "64",
		[KL_CK_KEY128] = "128",
	};

	if (s->ck_size != KL_CK_KEY64 && s->ck_size != KL_CK_KEY128)
		seq_printf(f, "%12s: Invalid[%d]\n", "ck_size", s->ck_size);
	else
		seq_printf(f, "%12s: %s\n", "ck_size", csize[s->ck_size]);
}

static void ca_kl_show_crypto(struct seq_file *f,
	struct ca_kl_session *s)
{
	char calgo[][32] = {
		[KL_ENCRYPT] = "KL_ENCRYPT",
		[KL_DECRYPT] = "KL_DECRYPT",
	};

	if (s->crypt_mode != KL_ENCRYPT &&
		s->crypt_mode != KL_DECRYPT) {
		seq_printf(f, "%12s: Invalid[%d]\n",
			"crypt_mode", s->crypt_mode);
	} else {
		seq_printf(f, "%12s: %s\n", "crypt_mode",
			calgo[s->crypt_mode]);
	}
}

static void ca_kl_show_key_cell(struct seq_file *f,
	struct ca_kl_session *s)
{
	struct kl_key_cell *cell = NULL;
	unsigned int key[4] = {0};
	int ret, idx, root, even, odd;
	unsigned char *pkey, *pk;

	pkey = (unsigned char *)key;

	cell = s->cell;
	if (!cell) {
		seq_puts(f, "\tno key cell!");
		return;
	}

	seq_printf(f, "%12s: %d\n", "valid", cell->valid);
	seq_printf(f, "%12s: 0x%04x\n", "pos", cell->pos);
	seq_printf(f, "%12s: %d\n", "num", cell->num);
	seq_printf(f, "%12s: 0x%02x\n", "kl_sel", cell->kl_sel);
	seq_printf(f, "%12s: %d\n", "run_parity", cell->ck_parity);
	seq_printf(f, "%12s: %d\n", "_count",
		atomic_read(&cell->_count));

	/*root*/
	root = (s->pdev->type == KL_TYPE_ETSI) ? 8 : 0;
	memset(key, 0, sizeof(key));
	ret = ca_kl_key_dump(s, cell->kl_sel, root, key);
	if (ret) {
		dev_dbg(s->pdev->dev, "dump root err!\n");
	} else {
		seq_printf(f, "%12s: ", "root");
		for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
			seq_printf(f, "%02x ", pkey[idx]);
		seq_puts(f, "\n");
	}

	/*pk1*/
	pk = (unsigned char *)s->gen_key.pk[0];
	seq_printf(f, "%12s: ", "pk1");
	for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
		seq_printf(f, "%02x ", pk[idx]);
	seq_puts(f, "\n");

	/*pk2*/
	pk = (unsigned char *)s->gen_key.pk[1];
	seq_printf(f, "%12s: ", "pk2");
	for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
		seq_printf(f, "%02x ", pk[idx]);
	seq_puts(f, "\n");

	/*pk_even*/
	pk = (unsigned char *)s->gen_key.key_even;
	seq_printf(f, "%12s: ", "pk_even");
	for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
		seq_printf(f, "%02x ", pk[idx]);
	seq_puts(f, "\n");

	/*pk_odd*/
	pk = (unsigned char *)s->gen_key.key_odd;
	seq_printf(f, "%12s: ", "pk_odd");
	for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
		seq_printf(f, "%02x ", pk[idx]);
	seq_puts(f, "\n");

	/*cw_even*/
	memset(key, 0, sizeof(key));
	even = cell->pos & 0xFF;
	ret = ca_kl_key_dump(s, cell->kl_sel, even, key);
	if (ret) {
		dev_dbg(s->pdev->dev, "dump err!\n");
	} else {
		seq_printf(f, "%12s: ", "cw_even");
		for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
			seq_printf(f, "%02x ", pkey[idx]);
		seq_puts(f, "\n");
	}

	/*cw_odd*/
	memset(key, 0, sizeof(key));
	odd = (cell->num == 2) ? ((cell->pos + 1) & 0xFF) : (cell->pos & 0xFF);
	ret = ca_kl_key_dump(s, cell->kl_sel, odd, key);
	if (ret) {
		dev_dbg(s->pdev->dev, "dump err!\n");
	} else {
		seq_printf(f, "%12s: ", "cw_odd");
		for (idx = 0; idx < KL_KEY_SIZE_MAX; idx++)
			seq_printf(f, "%02x ", pkey[idx]);
		seq_puts(f, "\n");
	}
}

static int ca_kl_show_status(struct seq_file *f, void *p)
{
	struct ca_kl_session *s = f->private;

	if (!s)
		return -ENODEV;

	mutex_lock(&s->mutex);

	seq_printf(f, "root_key_addr: 0x%x\n", s->pdev->root_key_addr);
	seq_printf(f, "max_level: %d\n", s->pdev->max_level);
	seq_printf(f, "type: %d\n", s->pdev->type);

	seq_puts(f, "@@--CFG INFO--@@:\n");
	ca_kl_show_algo(f, s);
	ca_kl_show_cksize(f, s);
	ca_kl_show_crypto(f, s);
	seq_printf(f, "%12s: %d\n", "level", s->level);
	seq_printf(f, "%12s: %d\n", "ck_parity", s->ck_parity);

	seq_printf(f, "%12s: %d\n", "is_cfg", s->is_cfg);

	seq_puts(f, "@@--KEY CELL INFO--@@:\n");
	ca_kl_show_key_cell(f, s);

	mutex_unlock(&s->mutex);
	return 0;
}

static int ca_kl_debugfs_open(struct inode *i, struct file *f)
{
	return single_open(f, ca_kl_show_status, i->i_private);
}

static const struct file_operations ca_kl_debugfs_ops = {
	.open = ca_kl_debugfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.owner = THIS_MODULE,
};

void ca_kl_dbgfs_create(struct ca_kl_sub_dev *pdev)
{
	char name[64] = {0};

	sprintf(name, "%s", pdev->basename);
	pdev->debugfs_dir = debugfs_create_dir(name, NULL);
	if (!pdev->debugfs_dir || IS_ERR(pdev->debugfs_dir))
		dev_dbg(pdev->dev, "debugfs create dir failed\n");
}

void ca_kl_dbgfs_remove(struct ca_kl_sub_dev *pdev)
{
	debugfs_remove(pdev->debugfs_dir);
}

int ca_kl_dbgfs_add_session(struct ca_kl_session *sess)
{
	char name[128];
	struct ca_kl_sub_dev *pdev;

	if (!sess)
		return -1;

	pdev = sess->pdev;
	if (!pdev || !pdev->debugfs_dir)
		return -1;

	sprintf(name, "session@%d", sess->id);
	sess->session_dir = debugfs_create_dir(name, pdev->debugfs_dir);
	if (!sess->session_dir || IS_ERR(sess->session_dir)) {
		dev_dbg(pdev->dev, "create session dir failed\n");
		return -1;
	}

	sprintf(name, "dbg");
	sess->debugfs = debugfs_create_file(name, S_IFREG | S_IRUGO,
		sess->session_dir, (void *)sess, &ca_kl_debugfs_ops);
	if (!sess->debugfs || IS_ERR(sess->debugfs))
		dev_dbg(pdev->dev, "debugfs create file failed\n");

	sprintf(name, "choice");
	sess->choice = debugfs_create_u32(name, S_IFREG | S_IRUGO | S_IWUGO,
		sess->session_dir, &kl_choice);
	if (!sess->choice || IS_ERR(sess->choice))
		dev_dbg(pdev->dev, "debugfs create choice failed\n");

	return 0;
}


int ca_kl_dbgfs_del_session(struct ca_kl_session *sess)
{
	if (!sess)
		return -1;

	debugfs_remove(sess->debugfs);
	debugfs_remove(sess->choice);

	debugfs_remove(sess->session_dir);
	return 0;
}

