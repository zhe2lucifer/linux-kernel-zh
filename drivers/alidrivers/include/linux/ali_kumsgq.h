/*
 * ALi Kernel-Userspace message queue implementation
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com

 * Authors: Christian Ruppert <christian.ruppert@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef ALIKUMSGQ_H
#define ALIKUMSGQ_H


struct kumsgq;

/**
 * ali_new_kumsgq - Create a new kernel-userspace message queue
 *
 * In the case of success, returns a pointer to a freshly initialised kumsgq
 * structure. Returns NULL in the case of failure.
 */
struct kumsgq *ali_new_kumsgq(void);

/**
 * ali_destroy_kumsgq - Destroy kumsgq structure after usage
 *
 * Marks @msgq for destruction as soon as the last message file descriptor is
 * closed.
 */
int ali_destroy_kumsgq(struct kumsgq *msgq);

/**
 * ali_kumsgq_newfd - Create a new message file descriptor for @msgq
 *
 * Returns a new file descriptor from which @current can read messages or
 * poll for them. @flags can be any combination of O_NONBLOCK and O_CLOEXEC.
 *
 * Returns a negative error code in the case of failure.
 */
int ali_kumsgq_newfd(struct kumsgq *msgq, int flags);

/**
 * ali_kumsgq_sendmsg - Send a message to a kernel-userspace message queue
 *
 * Sends a @msg_size byte message @msg_buf to the message queue. The message
 * is broadcast to all file descriptors listening on the message queue at the
 * time this function is called.
 *
 * Returns 0 on success and a negative error code in the case of failure.
 */
int ali_kumsgq_sendmsg(struct kumsgq *msgq, void *msg_buf,
			unsigned long msg_size);

#endif /* ALIKUMSGQ_H */