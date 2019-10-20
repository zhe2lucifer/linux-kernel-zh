/*
 * mmc-ali-soc.h - ALi SDIO/SD/MMC driver header file
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Shih  <david.shih@alitech.com>
 *	    Lucas.Lai	<lucas.lai@alitech.com>
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
#ifndef _MMC_ALI_SOC_H
#define _MMC_ALI_SOC_H

struct ali_mmc_soc_data {
	unsigned long		reset_reg;
	unsigned long		reset_bit;
	unsigned long		clock_gate_reg;
	unsigned long		clock_gate_bit;

	void (*pinmux_set)(unsigned int mmc_group);
	void (*pinmux_restore)(unsigned int mmc_group);
};

extern const struct ali_mmc_soc_data ali_mmc_3733_data;
extern const struct ali_mmc_soc_data ali_mmc_3823_data;

struct ali_mmc_soc_data *ali_mmc_soc_data_init(void);
void ali_mmc_soc_reset(struct ali_mmc_soc_data *soc);
void ali_mmc_soc_clock_gate(struct ali_mmc_soc_data *soc, bool clock_gated);
void ali_mmc_soc_pinmux_set(struct ali_mmc_soc_data *soc,
	unsigned int mmc_group);
void ali_mmc_soc_pinmux_restore(struct ali_mmc_soc_data *soc,
	unsigned int mmc_group);

#endif /* _MMC_ALI_SOC_H */
