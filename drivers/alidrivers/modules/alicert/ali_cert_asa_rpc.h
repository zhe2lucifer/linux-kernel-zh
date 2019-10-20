/*
 * RPC interface for CERT ASA
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_ASA_RPC_H
#define _ALI_CERT_ASA_RPC_H

struct cert_asa_see_ops {
	int (*open)(int sbm_id);
	int (*close)(int sess);
	int (*setfmt)(int sess, int fmt);
	int (*addpid)(int sess, int pos, __u8 ltsid, __u16 pid);
	int (*delpid)(int sess, __u8 ltsid, __u16 pid);
	int (*decrypt)(int sess, __u8 *input, __u8 *output, __u32 length);
};

int see_cert_asa_register(void *data);
int see_cert_asa_unregister(void *data);

#endif

