/*
 * SBM for CERT ASA
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_ASA_SBM_H
#define _ALI_CERT_ASA_SBM_H

#include "ali_cert_asa_priv.h"

int cert_asa_sbm_add(struct cert_asa_session *sess);
void cert_asa_sbm_del(struct cert_asa_session *sess);
int cert_asa_sbm_epage(struct cert_asa_session *sess,
	struct cert_asa_page *p);
int cert_asa_sbm_qpage(struct cert_asa_session *sess,
	struct cert_asa_page *p);

#endif /* _ALI_CERT_ASA_SBM_H */

