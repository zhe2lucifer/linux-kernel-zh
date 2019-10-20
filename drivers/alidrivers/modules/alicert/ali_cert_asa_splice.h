/*
 * Some ops for ASA splice
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_ASA_SPLICE_H
#define _ALI_CERT_ASA_SPLICE_H

#include <linux/splice.h>

struct cert_asa_splice_ops {
	ssize_t (*to_pipe)(struct pipe_inode_info *pipe,
		       struct splice_pipe_desc *spd);
	ssize_t (*from_pipe)(struct pipe_inode_info *, struct file *,
			loff_t *, size_t, unsigned int,
			splice_actor *);
	void (*spd_release)(struct splice_pipe_desc *spd,
			unsigned int i);
	ssize_t (*pipe_size)(struct pipe_inode_info *pipe);
	int (*grow_spd)(const struct pipe_inode_info *pipe,
			struct splice_pipe_desc *spd);
	void (*shrink_spd)(struct splice_pipe_desc *spd);
	const struct pipe_buf_operations *buf_ops;
};

void cert_asa_splice_register(void *data);

#endif /* _ALI_CERT_ASA_SPLICE_H */

