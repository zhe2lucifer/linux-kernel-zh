/*
 * mmc-ali-soc.c -MMC/SD/SDIO driver for ALi SoCs
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Shih <david.shih@alitech.com>,
 *          Lucas.Lai  <lucas.lai@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/types.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "mmc-ali-soc.h"

void ali_mmc_soc_reset(struct ali_mmc_soc_data *soc)
{
	void __iomem *soc_reset_reg;

	if (!soc)
		return;

	soc_reset_reg = ioremap(soc->reset_reg, 0x4);

	iowrite32(ioread32(soc_reset_reg) | BIT(soc->reset_bit),
		soc_reset_reg);
	udelay(5);
	iowrite32(ioread32(soc_reset_reg) & ~BIT(soc->reset_bit),
		soc_reset_reg);

	iounmap(soc_reset_reg);
}

void ali_mmc_soc_clock_gate(struct ali_mmc_soc_data *soc, bool clock_gated)
{
	void __iomem *soc_gate_reg;

	if (!soc)
		return;

	soc_gate_reg = ioremap(soc->clock_gate_reg, 0x4);

	if (clock_gated)
		iowrite32(ioread32(soc_gate_reg) | BIT(soc->clock_gate_bit),
			soc_gate_reg);
	else
		iowrite32(ioread32(soc_gate_reg) & ~BIT(soc->clock_gate_bit),
			soc_gate_reg);

	iounmap(soc_gate_reg);
}

void ali_mmc_soc_pinmux_set(struct ali_mmc_soc_data *soc,
	unsigned int mmc_group)
{
	if (soc && soc->pinmux_set)
		soc->pinmux_set(mmc_group);
}

void ali_mmc_soc_pinmux_restore(struct ali_mmc_soc_data *soc,
	unsigned int mmc_group)
{
	if (soc && soc->pinmux_restore)
		soc->pinmux_restore(mmc_group);
}

static void get_chip_info(u16 *id, u8 *package, u8 *version)
{
	void __iomem *soc_chip_ver_reg = ioremap(0x18000000, 0x4);
	u32 chip_info = ioread32(soc_chip_ver_reg);

	*id      = (u16)(chip_info >> 16) & 0x0000FFFF;
	*package = (u8) (chip_info >> 8)  & 0x000000FF;
	*version = (u8) (chip_info)	  & 0x000000FF;

	iounmap(soc_chip_ver_reg);
}

struct ali_mmc_soc_data *ali_mmc_soc_data_init(void)
{
	u16 id;
	u8 package, version;

	get_chip_info(&id, &package, &version);

	switch (id) {
	case 0x3921:
		return (struct ali_mmc_soc_data *) &ali_mmc_3733_data;
	case 0x3821:
		return (struct ali_mmc_soc_data *) &ali_mmc_3823_data;
	default:
		return NULL;
	}
}
