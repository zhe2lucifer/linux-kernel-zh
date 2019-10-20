/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
#ifndef _CA_CASI_COMMON_H
#define _CA_CASI_COMMON_H

#define ALI_CASI_DEV_NAME "casi"

/*!@addtogroup CASI
 *  @{
    @~
 */
 
#include <linux/ioctl.h>
#include <alidefinition/adf_ce.h>
#include "ca_kl.h"

#define CA_CASI_BASE 0xca
/*!< DSC ioctl cmd base. */

#define CASI_TA_DATA_SIZE (416)
/*!< TA transform data size*/

#define CASI_KEY_POS_EVEN (0)
/*!<key pos in even position*/

#define CASI_KEY_POS_ODD (1)
/*!<key pos in even position*/

/*! @struct casi_ta_data_prm
 *   @brief define for CASI TA algo parameter.
 */
struct casi_ta_data_prm {
	char ta_data[CASI_TA_DATA_SIZE];
		/*!< Array containing TA algo data.*/
	int algo;
		/*!< Algorithm only support #CA_ALGO_AES, #CA_ALGO_TDES.*/
	int fd;
		/*!< DSC fd from after open /dev/dsc.*/
};

/*! @struct casi_kl_prm
 *   @brief define for generate TA nlevel key parameter.
 */
struct casi_kl_prm {
	struct kl_gen_key ce_gen_prm;
		/*!<specify struct kl_gen_key parameter.*/
	int fd;
		/*!< DSC fd from after open /dev/kl.*/
};

/* Used for CASI TA data ioctl
*/
#define CASI_TA_DATA_SET     _IOW(CA_CASI_BASE, 1, struct casi_ta_data_prm)
/*!<
    This command would send the TA encrypted data to see and use DSC to decrypt.

    int ioctl(dev, #CASI_TA_DATA_SET, struct *casi_ta_data_prm);

    These resources are released when closing the device.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define CASI_GEN_NLEVEL_TA_KEY _IOW(CA_CASI_BASE, 2, struct ce_generate_nlevel)
/*!<
    This command would generate Key Ladder using TA algo.

    int ioctl(dev, #CASI_GEN_NLEVEL_TA_KEY, struct *ce_generate_nlevel);

    These resources are released when closing the device.

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

/*!
@}
*/

#endif //_CA_CASI_COMMON_H
