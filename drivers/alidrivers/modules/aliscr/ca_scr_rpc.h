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

#ifndef _CA_SCR_RPC_H_
#define _CA_SCR_RPC_H_

#include <alidefinition/adf_scr.h>

int ali_scr_umemcpy(void *dest, const void *src, __u32 n);
void _scr_api_attach(struct ca_scr_dev *scr);
void _scr_api_detach(struct ca_scr_dev *scr);
int _scr_create_sbm_task(struct ca_scr_dev *scr,
	UINT32 sbm_id);
int _scr_delete_sbm_task(struct ca_scr_dev *scr,
	UINT32 sbm_id);
RET_CODE _scr_api_ioctl(struct ca_scr_dev *scr,
	struct scr_dev *p_scr_dev, UINT32 cmd, UINT32 param);
RET_CODE _scr_session_create(struct ca_scr_dev *scr,
	unsigned int *sess_id, struct scr_sess_create *param);
RET_CODE _scr_session_delete(struct ca_scr_dev *scr,
	unsigned int sess_id);
RET_CODE _scr_session_add_key(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_key *user_key);
RET_CODE _scr_session_del_key(struct ca_scr_dev *scr,
	unsigned int sess_id, UINT32 key_handle);
RET_CODE _scr_session_update_key(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_update_info *user_up);
RET_CODE _scr_session_add_pid(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_pid *pPid);
RET_CODE _scr_session_del_pid(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_pid *pPid);
RET_CODE _scr_session_contns_renew(struct ca_scr_dev *scr,
	unsigned int sess_id);
RET_CODE _scr_session_crypto(struct ca_scr_dev *scr,
	unsigned int sess_id, struct scr_sess_dio_crypto *crypto);

#endif

