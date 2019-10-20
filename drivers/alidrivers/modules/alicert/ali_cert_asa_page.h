/*
 * Some ops for ASA page
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_ASA_PAGE_H
#define _ALI_CERT_ASA_PAGE_H

void cert_asa_page_register(void *data);
void cert_asa_work_pollwr(struct work_struct *work);
void cert_asa_work_pollrd(struct work_struct *work);

#endif /* _ALI_CERT_ASA_PAGE_H */

