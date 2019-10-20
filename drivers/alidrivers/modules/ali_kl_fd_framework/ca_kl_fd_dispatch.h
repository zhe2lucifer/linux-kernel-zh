/*
 * Key Ladder/DSC file descriptor dispatcher interface
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

#ifndef CA_KL_FD_DISPATCH_H
#define CA_KL_FD_DISPATCH_H

#include <linux/file.h>

struct kl_key_cell {
	int valid;
	int pos;
	int num;
	int kl_sel;
	int ck_parity;
	int ck_size;
	struct device *dev;
	void *private_data;
	atomic_t _count;

	int (*put_cell)(struct kl_key_cell *cell);
};

struct kl_fd_operations {
	int (*fetch_key_cell)(struct file *f, struct kl_key_cell **cell);
};

static inline void get_key_cell(struct kl_key_cell *cell)
{
	if (!cell)
		return;

	if (!cell->valid)
		return;

	BUG_ON(atomic_read(&cell->_count) <= 0);
	atomic_inc(&cell->_count);
}

int put_key_cell(struct kl_key_cell *cell);

/**
 * fetch_key_cell_by_fd - Obtain key cell memory from key ladder file descriptor
 * @fd (IN): Key ladder device file descriptor
 * @cell (INOUT): pointer of the key cell pointer
 *
 * The return value is 0 in the case of success or a negative error code.
 */
int fetch_key_cell_by_fd(int fd, struct kl_key_cell **cell);

/**
 * register_kl_callbacks - Register key ladder callback table
 * @file_ops: Pointer to struct file_operations used by the key ladder driver
 * @kl_ops: Pointer to struct kl_ops associated to all device files for which
 *          file_ops are installed.
 *
 * Install the key memory lookup functions defined in @kl_ops for device files
 * which use the file operations from @file_ops. Intended to be called at
 * device initialisation time from XXX_probe type of functions.
 * Returns 0 on success and a negative error code in the case of error.
 */
int register_kl_callbacks(const struct file_operations *file_ops,
				const struct kl_fd_operations *kl_ops);

/**
 * unregister_kl_callbacks - Undo a key ladder callback table registration
 * @file_ops: Pointer to struct file_operations to which the table is
 *            associated
 *
 * Uninstall the key memory lookup functions for device files which use the
 * file operations from @file_ops. Intended to be called at device removal time
 * from XXX_remove type of functions.
 * Returns 0 on success and a negative error code in the case of error.
 */
int unregister_kl_callbacks(const struct file_operations *file_ops);

#endif /* CA_KL_FD_DISPATCH_H */
