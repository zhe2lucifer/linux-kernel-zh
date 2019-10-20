/*
 * Share Buffer Manager driver
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

#ifndef _ALI_SBM_TYPES_H_
#define _ALI_SBM_TYPES_H_

/*! @struct see_sbm
 *  @brief structure describing an SBM buffer
 */
struct see_sbm {
	int id;                     /* id of SBM */
	void    *buf_start;         /* start address of SBM buffer */
	size_t  buf_size;           /* size of SBM buffer */
	void    **read_pointer;     /* read pointer to SBM buffer */
	void    **write_pointer;    /* write pointer to SBM buffer */
	void    *priv_data;         /* private data to SBM channel */
};

/*! @struct see_sbm_entry
 *  @brief structure describing one entry in an SBM buffer
 */
struct see_sbm_entry {
	void    *entry;         /* pointer to SBM entry */
	size_t  size;           /* SBM entry size */
};

#endif /* _ALI_SBM_TYPES_H_ */
