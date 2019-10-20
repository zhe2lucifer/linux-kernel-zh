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

#ifndef _ALI_SBM_CLIENT_H_
#define _ALI_SBM_CLIENT_H_

#include <ali_sbm_types.h>

/*!
 *  @brief Create one SBM channel
 *  @param[in/out] sbm Pointer to an SBM buffer description
 *  @return int
 *  @retval 0          Create SBM successfully
 *  @retval <0         Failed to create SBM, returns negative errno of error
 */
int see_sbm_create(struct see_sbm *sbm);

/*!
 *  @brief Destroy specified SBM channel
 *  @param[in] sbm Pointer to an SBM buffer description
 *  @return int
 *  @retval 0      Destroy SBM successfully
 *  @retval <0     Failed to Destroy SBM, returns negative errno of error
 */
int see_sbm_destroy(struct see_sbm *sbm);

/*!
 *  @brief Add one SBM entry: copy entry data to SBM and increase write pointer
 *  @param[in]  sbm   Pointer to an SBM buffer description
 *  @param[in]  data  Pointer to the data copied into the SBM entry
 *  @param[in]  len   Length of the SBM entry/data in bytes
 *  @param[out] entry Pointer to struct sbm_entry which is initialised with a
 *            valid SBM entry descriptor in the case of success. this
 *            structure is not modified in the case of an error
 *  @return int
 *  @retval 0         Add SBM entry successfully
 *  @retval <0        Failed to add SBM entry, returns negative errno of error
 */
int see_enqueue_sbm_entry(struct see_sbm *sbm, void *data, size_t len,
			struct see_sbm_entry *entry);

/*!
 *  @brief Query if processing of an SBM entry is finished
 *  @param[in] sbm   Pointer to an SBM buffer description
 *  @param[in] entry Pointer to an SBM entry description
 *  @return int
 *  @retval 0        The SBM entry is finished
 *  @retval <0       Failed to query SBM entry, returns negative errno of error
 */
int see_query_sbm_entry(struct see_sbm *sbm, struct see_sbm_entry *entry);

#endif /* _ALI_SBM_CLIENT_H_ */
