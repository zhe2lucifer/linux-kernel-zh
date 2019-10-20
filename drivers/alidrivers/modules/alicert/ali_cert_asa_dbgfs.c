/*
 * CERT debugfs for ASA debuging
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

#include "ali_cert_asa_dbgfs.h"

static int cert_asa_dbg_sess_info_get(struct seq_file *f, void *pointer)												 
{
	struct cert_asa_session *sess = f->private;
	struct cert_asa_key *key = NULL;
	struct cert_asa_page *p = NULL;

	seq_printf(f,
		"see-sess: 0x%x, ts-format: %d\n",
		sess->see_sess, sess->ts_fmt);

	seq_printf(f,
		"nr_pages: 0x%x\n", atomic_read(&sess->page_c));

	list_for_each_entry(key, &sess->keys, list) {
		seq_printf(f,
			"sess->keys->handle: 0x%x\n",
			key->handle);
	}

	down(&sess->sem);
	seq_printf(f,
		"nr bytes: 0x%x\n", atomic_read(&sess->nr_bytes));

	seq_printf(f,
		"bytes-combined: 0x%x\n", sess->nr_combined);

	list_for_each_entry(p, &sess->pages, list) {
		seq_printf(f,
		"p-page:%p-%p, addr:0x%x, rh&rt:0x%x-%x, offset&len:0x%x-%x, flag:%d\n",
		p, p->page, p->addr, p->rh, p->rt,
		p->offset_in, p->len, p->flag);
	}

	list_for_each_entry(p, &sess->rpages, list) {
		seq_printf(f,
		"rp-page:%p-%p, addr:0x%x, rh&rt:0x%x-%x, offset&len:0x%x-%x, flag:%d\n",
		p, p->page, p->addr, p->rh, p->rt,
		p->offset, p->len, p->flag);
	}

	up(&sess->sem);

	return 0;
}

static int cert_debugfs_open(struct inode *i, struct file *f)
{
	return single_open(f, cert_asa_dbg_sess_info_get, i->i_private);
}

static const struct file_operations cert_asa_dbg_sess_info_fops = {
    .open = cert_debugfs_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .owner = THIS_MODULE,
};

void cert_asa_dbgfs_create(struct cert_asa_drv *drv)
{

	drv->debugfs_dir = debugfs_create_dir(CERT_ASA_DEV, NULL);

	if (!drv->debugfs_dir || IS_ERR_OR_NULL(drv->debugfs_dir))
		dev_err(drv->dev, "debugfs create dentry failed\n");
}

void cert_asa_dbgfs_remove(struct cert_asa_drv *drv)
{
	if (drv && drv->debugfs_dir)
		debugfs_remove_recursive(drv->debugfs_dir);
}

int cert_asa_dbgfs_add(struct cert_asa_session *sess)
{
	char name[128];

	if (unlikely(!sess || !sess->drv ||
		!sess->drv->debugfs_dir))
		return -EBADF;

	memset(name, 0, sizeof(name));
	sprintf(name, "cert_asa_session@%p", sess);
	sess->debugfs = debugfs_create_file(name, S_IFREG | S_IRUGO,
					    sess->drv->debugfs_dir,
					    (void *)sess,
					    &cert_asa_dbg_sess_info_fops);

	if (!sess->debugfs || IS_ERR_OR_NULL(sess->debugfs))
		dev_err(sess->drv->dev, "debugfs create file failed\n");

	return 0;
}

void cert_asa_dbgfs_del(struct cert_asa_session *sess)
{
	if (unlikely(!sess || !sess->debugfs))
		return;

	debugfs_remove(sess->debugfs);
}

