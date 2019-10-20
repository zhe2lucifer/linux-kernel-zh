/*
 * mmc-ali-3823.c -MMC/SD/SDIO driver for ALi SoCs
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
#include <linux/mutex.h>

#include "mmc-ali-soc.h"

enum ali_3823_mmc_pin_group {
	PIN_SD = 0,	/* SD 4bit */
	PIN_SDIO,	/* SDIO 8bit */
	PIN_EMMC,	/* eMMC */
};

#define SOC_PINMUX_REG		0x18000490
#define SOC_GPIO_ENABLE_REG	0x18000430	/* GPIO[31:0] */
#define SOC_GPIOA_ENABLE_REG	0x18000434	/* GPIO[63:32] */
#define SOC_GPIOB_ENABLE_REG	0x18000438	/* GPIO[95:64] */
#define SOC_GPIOC_ENABLE_REG	0x1800043C	/* GPIO[127:96] */

#if defined(CONFIG_SUPPORT_NAND_NOR)
extern struct mutex ali_sto_mutex;
#endif

void ali_mmc_3823_pinmux_set(unsigned int mmc_group)
{
	void __iomem *pin_mux_reg = ioremap(SOC_PINMUX_REG, 0x4);
	void __iomem *gpioa_reg = ioremap(SOC_GPIOA_ENABLE_REG, 0x4);
	u32 PinMuxCtrl = ioread32(pin_mux_reg);
	u32 gpioa = ioread32(gpioa_reg);

#if defined(CONFIG_SUPPORT_NAND_NOR)
	mutex_lock(&ali_sto_mutex);
#endif

	/* Disable GPIO[44:35] Function */
	gpioa &= ~(BIT(35-32) | BIT(36-32) | BIT(37-32) | BIT(38-32)
		 | BIT(39-32) | BIT(40-32) | BIT(41-32) | BIT(42-32)
		 | BIT(43-32) | BIT(44-32));


	/* Disable Share Function (NAND) & Enable SD Interface, SD 8bit */
	PinMuxCtrl &= ~BIT(3);
	switch (mmc_group) {
	case PIN_EMMC:
		PinMuxCtrl |= (BIT(26) | BIT(16) | BIT(17) | BIT(18));
		break;
	case PIN_SDIO:
		PinMuxCtrl |= (BIT(26) | BIT(16) | BIT(17));
		break;
	case PIN_SD:
		PinMuxCtrl |= (BIT(26) | BIT(16));
		break;
	}

	iowrite32(PinMuxCtrl, pin_mux_reg);
	iowrite32(gpioa, gpioa_reg);
	iounmap(gpioa_reg);
	iounmap(pin_mux_reg);
}

void ali_mmc_3823_pinmux_restore(unsigned int mmc_group)
{
	void __iomem *pin_mux_reg = ioremap(SOC_PINMUX_REG, 0x4);

	iowrite32(ioread32(pin_mux_reg) & ~(BIT(16) | BIT(17) | BIT(18)),
		pin_mux_reg);
	iounmap(pin_mux_reg);

#if defined(CONFIG_SUPPORT_NAND_NOR)
	mutex_unlock(&ali_sto_mutex);
#endif
}

const struct ali_mmc_soc_data ali_mmc_3823_data = {
	.reset_reg	= 0x18000080,
	.reset_bit	= 30,
	.clock_gate_reg	= 0x18000060,
	.clock_gate_bit	= 1,
	.pinmux_set	= ali_mmc_3823_pinmux_set,
	.pinmux_restore	= ali_mmc_3823_pinmux_restore,
};
