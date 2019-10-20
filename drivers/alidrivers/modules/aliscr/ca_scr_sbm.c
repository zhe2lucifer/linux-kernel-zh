/*
 * DeScrambler Core driver
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
#include <linux/slab.h>

#include "ca_scr_priv.h"
#include "ca_scr_rpc.h"
#include "ca_scr_sbm.h"

int scr_open_sbm(struct ca_scr_session *s)
{
	int rc;

	s->sbm.buf_size = SCR_SBM_NR_NODES * 2 * 4;
	s->sbm.buf_start = kmalloc(s->sbm.buf_size, GFP_KERNEL);
	if (!s->sbm.buf_start)
		return -ENOMEM;

	/*s->sbm.buf_start = (UINT32)(s->sbm.buf_start)&0x1FFFFFFF;*/
	memset(s->sbm.buf_start, 0, s->sbm.buf_size);
	s->sbm.priv_data = (void *)s;
	rc = see_sbm_create(&s->sbm);
	if (rc) {
		dev_dbg(s->scr->dev, "create sbm error!\n");
		return rc;
	}

	dev_dbg(s->scr->dev, "create_sbm:id[%d],addr[%p],size[%x]\n",
		s->sbm.id, s->sbm.buf_start, s->sbm.buf_size);

	return _scr_create_sbm_task(s->scr, s->sbm.id);
}

int scr_close_sbm(struct ca_scr_session *s)
{
	int ret;
	int sbm_id = s->sbm.id;

	ret = see_sbm_destroy(&s->sbm);
	if (ret) {
		dev_dbg(s->scr->dev, "delete sbm error!\n");
		return ret;
	}

	kfree(s->sbm.buf_start);
	return _scr_delete_sbm_task(s->scr, sbm_id);
}

int scr_write_sbm(struct ca_scr_session *s, char *buf, size_t count,
	struct see_sbm_entry *sbm_entry)
{
	int rc;

	rc = see_enqueue_sbm_entry(&s->sbm, buf, count, sbm_entry);
	if (rc) {
		dev_dbg(s->scr->dev, "add sbm entry error!\n");
		return rc;
	}

	return 0;
}

/*
	0: finish
	1: query failed or not finish
*/
int scr_query_sbm_entry(struct ca_scr_session *s,
	struct see_sbm_entry *sbm_entry)
{
	/* Not enqueue SBM yet*/
	if (!sbm_entry->entry)
		return 1;

	return see_query_sbm_entry(&s->sbm, sbm_entry);
}

