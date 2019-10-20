/*
 * ALi True Random Number Generator driver
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

#ifndef _ALI_TRNG_PRIV_H_
#define _ALI_TRNG_PRIV_H_

#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <linux/hw_random.h>
#include "see_bus.h"

#define CA_TRNG_MAX_GROUP (16)
	/*!< Max group number to get the TRNG data each time*/
#define CA_TRNG_64BITS (8)
	/*!< Group unit in byte.*/

enum TRNG_SW_FUNC
{
    FUNC_TRNG_GENERATE_BYTE = 0,
    FUNC_TRNG_GENERATE_64BITS,
    FUNC_TRNG_SEE_GET64BIT,
};

struct ca_trng {
	struct hwrng rng;
	struct see_client *clnt;
};

int ca_trng_get_64bits(struct ca_trng *trng, __u8 *data, __u32 n);

#endif


