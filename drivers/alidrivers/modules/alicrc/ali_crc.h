/*
 * ALi CRC32 driver
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

#ifndef __ALI_CRC_H__
#define __ALI_CRC_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <alidefinition/adf_scr.h>
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <ali_sbm_client.h>
#include "see_bus.h"

#define ALGO_CRC32 (3)
#define ALI_CRC_MAX_BUF (1000*1000)

#define CRC32_CHKSUM_BLOCK_SIZE	(1)
#define CRC32_CHKSUM_DIGEST_SIZE (4)

struct ali_crc_dev_tag {
	struct mutex mutex;
	struct see_client *clnt;
};

struct ali_crc_desc {
	int init;
	int session_id;
	unsigned int rc;
	struct ali_crc_dev_tag *pdev;
	struct see_sbm sbm_desc;
	wait_queue_head_t wq_sbm;
	struct see_sbm_entry entry;
	unsigned int unchecked_pkt;
	int nents;
};

int _crc_api_attach(struct ali_crc_dev_tag *crc);
void _crc_api_detach(struct ali_crc_dev_tag *crc);
int _crc_session_create(struct ali_crc_dev_tag *crc,
	unsigned int *sess_id, struct scr_sess_create *param);
int _crc_session_delete(struct ali_crc_dev_tag *crc,
	unsigned int sess_id);
int _crc_update(struct ali_crc_dev_tag *crc,
	unsigned int sess_id, struct scr_sess_dio_crc32 *crc32);
int _crc_final(struct ali_crc_dev_tag *crc,
	unsigned int sess_id, unsigned int *rc);
int _crc_create_sbm_task(struct ali_crc_dev_tag *crc,
	UINT32 sbm_id);
int _crc_delete_sbm_task(struct ali_crc_dev_tag *crc,
	UINT32 sbm_id);


#endif /*__ALI_CRC_H__*/


