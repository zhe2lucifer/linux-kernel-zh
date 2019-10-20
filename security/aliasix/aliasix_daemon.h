/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced verification daemon.
 *
 *  Author:
 *	Zhao Owen <owen.zhao@alitech.com>
 *
 *  Copyright (C) 2011 Zhao Owen <owen.zhao@alitech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2,
 *      as published by the Free Software Foundation.
 */

#ifndef _SECURITY_ALIASIX_DAEMON_H_
#define _SECURITY_ALIASIX_DAEMON_H_

/*
 * ALiasix header
 */
#include "aliasix.h"
#include "aliasix_perm.h"

int aliasix_daemon_thread(void *unused);

#endif
