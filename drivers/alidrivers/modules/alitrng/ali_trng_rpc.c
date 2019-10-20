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
#include <linux/types.h>

#include "ali_trng_priv.h"

#define TRNG_NPARA(x) ((HLD_DSC_MODULE<<24)|(x<<16))

static int ca_trng_get_64bits_ex(__u8 *data, __u32 n)
{
	__u32 desc[] =
	{
	    1, DESC_OUTPUT_STRU(0, (CA_TRNG_64BITS*n)), 
		1, DESC_P_PARA(0, 0, 0), 
		0, 
		0, 
	};
	jump_to_func(NULL, ali_rpc_call, data, TRNG_NPARA(2) | FUNC_TRNG_SEE_GET64BIT, desc);
}

int ca_trng_get_64bits(struct ca_trng *trng,
	__u8 *data, __u32 n)
{
	int ret = RET_FAILURE;
	__u32 trng_c = 0;
	__u32 deal_c = 0;

	while (deal_c < n) {
		trng_c = (n-deal_c) > CA_TRNG_MAX_GROUP ?
			CA_TRNG_MAX_GROUP : (n-deal_c);

		ret = ca_trng_get_64bits_ex(data + CA_TRNG_64BITS * deal_c,
			(__u32)trng_c);
		if (0 != ret) {
			dev_err(&trng->clnt->dev,
				"get 64bits error! ret:%d\n", ret);
			return ret;
		}

		deal_c += trng_c;
	}

	return ret;
}



