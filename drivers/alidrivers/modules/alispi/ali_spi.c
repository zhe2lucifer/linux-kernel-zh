/*
 * Driver for ALi SPI Controllers
 *
 * Copyright (C) 2015 ALi Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <mtd/mtd-abi.h>
#include <linux/device.h>
#include <linux/of.h>
#include <ali_reg.h>
#include "ali_spi.h"
/*
 * The core SPI transfer engine just talks to a register bank to set up
 * DMA transfers; transfer queue progress is driven by IRQs.  The clock
 * framework provides the base clock, subdivided for each spi_device.
 *
 * Newer controllers, marked with "new_1" flag, have:
 *  - CR.LASTXFER
 *  - SPI_MR.DIV32 may become FDIV or must-be-zero (here: always zero)
 *  - SPI_SR.TXEMPTY, SPI_SR.NSSR (and corresponding irqs)
 *  - SPI_CSRx.CSAAT
 *  - SPI_CSRx.SBCR allows faster clocking
 */

static u8 sfreg_read8(struct ali_spi *as, u32 reg)
{
	return	ioread8(as->ali_sflash_base_addr  + reg);
}

static void sfreg_write8(struct ali_spi *as, u8 val, u32 reg)
{
	iowrite8(val, as->ali_sflash_base_addr + reg);
}

static u32 sfreg_read32(struct ali_spi *as, u32 reg)
{
	return ioread32(as->ali_sflash_base_addr + reg);
}

static void sfreg_write32(struct ali_spi *as, u32 val, u32 reg)
{
	iowrite32(val, as->ali_sflash_base_addr + reg);
}

static u32 soc_read32(struct ali_spi *as, u32 reg)
{
	return ioread32(as->ali_soc_base_addr + reg);
}

static void soc_write32(struct ali_spi *as, u32 reg, u32 val)
{
	iowrite32(val, as->ali_soc_base_addr + reg);
}

static void spi_set_flash_ctrl_clk(struct ali_spi *as, u32 rate)
{
	/*
	 * SCLK = MEM_CLK / [2 * (CLK_SEL + 1)]
	 * default  clock 54MHz = 0xC0
	 */
	u32 clk_sel = MEM_CLK / 2 / rate - 1;

	sfreg_write8(as, SPI_CFG_DEFAULT | clk_sel, ALI_SPI_CFG);
}

static inline void spi_set_clock_mode(struct ali_spi *as, u32 mode)
{
	sfreg_write32(as, (mode & (SPI_CPHA | SPI_CPOL)) << 1, ALI_SPI_CPU_CTRL_DMA);
}

const int C3505_SPI_PINMUX_CLR[22] = {0x490, 8, 0, 0x430, 20, 1, 0x058, 20, 1, \
	0x54, 20, 0, 0x430, 16, 1, 0x058, 16, 1, 0x54, 16, 1, 0};
const int C3505_QFP_SPI_PINMUX_CLR[4] = {0x438, 7, 1, 0};
const int C3921_SPI_PINMUX_CLR[7] = {0x88, 19, 0, 0x1030, 28, 0, 0};

void spi_pinmux_release(struct ali_spi *as)
{
	int *pinmux_setting = NULL;
	int reg, bit, val;

	if (as == NULL) {
		dev_err(&as->pdev->dev, "[ERR] %s, host == NULL\n", __func__);
		return;
	}
	
	switch (as->chip_id) {
	case C3921:
		pinmux_setting =  (int *) &C3921_SPI_PINMUX_CLR;
		break;

	case C3503:

		break;

	case C3821:

		break;

	case C3505:
		if (C3505_QFP_PACKAGE == as->chip_package)
			pinmux_setting =  (int *) &C3505_QFP_SPI_PINMUX_CLR;
		else
			pinmux_setting =  (int *) &C3505_SPI_PINMUX_CLR;
		break;

	default:
		dev_err(&as->pdev->dev, "[ERR] chip id N/A");
		break;
	}

	if (pinmux_setting != NULL)
	{
		while(*pinmux_setting != 0)
		{
			reg = *pinmux_setting;    pinmux_setting++;
			bit = *pinmux_setting;    pinmux_setting++;             
			val = *pinmux_setting;    pinmux_setting++; 

			if (val)
			 	soc_write32(as, reg, soc_read32(as, reg) | (1<<bit));
			else
				 soc_write32(as, reg, soc_read32(as, reg) & ~(1<<bit));
		}   
	}
}

const int C3505_SPI_PINMUX[40] = {0x490, 10, 1, 0x490, 9, 0, 0x490, 8, 1, 0x490, 3, 1, \
    0x490, 2, 1, 0x490, 1, 1, 0x490, 0, 1, 0x430, 20, 0, 0x58, 20, 0, 0x54, 20, 0, \
    0x430, 16, 0, 0x58, 16, 0, 0x54, 16, 0, 0};
const int C3505_QFP_SPI_PINMUX[13] = {0x438, 7, 0, 0x74, 30, 1, 0x74, 18, 0, 0x74, 17, 0, 0};
const int C3921_SPI_PINMUX[13] = {0x88, 3, 0, 0x88, 19, 1, 0x8c, 31, 0, 0x1030, 28, 1, 0};

void spi_pinmux_set(struct ali_spi *as)
{	
	int *pinmux_setting = NULL;
	int reg, bit, val;

	if (as == NULL) {
		dev_err(&as->pdev->dev, "[ERR] %s, host == NULL\n", __func__);
		return;
	}

       switch(as->chip_id)
       {
	case C3921:
		pinmux_setting =  (int *) &C3921_SPI_PINMUX;
		break;

	case C3503:
 
		break;

	case C3821:

		break;

	case C3505:
		if (C3505_QFP_PACKAGE == as->chip_package)
			pinmux_setting =  (int *) &C3505_QFP_SPI_PINMUX;
		else
			pinmux_setting =  (int *) &C3505_SPI_PINMUX;
		break;

	default:
		dev_err(&as->pdev->dev, "[ERR] chip id N/A");
		break;
	}

	if (pinmux_setting != NULL)
	{
		while(*pinmux_setting != 0)
		{
			reg = *pinmux_setting;    pinmux_setting++;
			bit = *pinmux_setting;    pinmux_setting++;             
			val = *pinmux_setting;    pinmux_setting++; 

			if (val)
			 	soc_write32(as, reg, soc_read32(as, reg) | (1<<bit));
			else
				 soc_write32(as, reg, soc_read32(as, reg) & ~(1<<bit));
		}   
	}
}

static int ali_spi_setup(struct spi_device *spi)
{
	struct ali_spi  *as;
	
	dev_dbg(&spi->dev, "%s.\n", __func__);
	as = spi_master_get_devdata(spi->master);
	sfreg_write8(as, SPI_CFG_DEFAULT, ALI_SPI_CFG);
	sfreg_write8(as, SPI_MODE_DEFAULT, ALI_SPI_MODE);
	sfreg_write8(as, SPI_FMT_DEFAULT, ALI_SPI_FMT);
	sfreg_write8(as, SPI_INS_DEFAULT, ALI_SPI_INS);
	
	/* get chip id number */
	as->chip_id =  (u16) (soc_read32(as, 0x0) >> 16);
	as->chip_package =  (u8) ((soc_read32(as, 0x0) >> 8) & 0x0F);

	spi_set_clock_mode(as, spi->mode);
	
	return 0;
}

static int
spi_dma_map(struct ali_spi *as, void *buf, u32 tmp_len, int xfter)
{
	struct device	*dev = &as->pdev->dev;

	as->tx_dma = as->rx_dma = INVALID_DMA_ADDRESS;
	if (buf) {
		if (xfter == SPI_DMA_TX) {
			as->tx_dma = dma_map_single(dev,
					buf, tmp_len,
					DMA_TO_DEVICE);
			if (dma_mapping_error(dev, as->tx_dma))
				return -ENOMEM;
		} else if (xfter == SPI_DMA_RX) {
			as->rx_dma = dma_map_single(dev,
					buf, tmp_len,
					DMA_FROM_DEVICE);
			if (dma_mapping_error(dev, as->rx_dma))
				return -ENOMEM;
		}
	}
	return 0;
}

static void
spi_dma_unmap(struct ali_spi *as, dma_addr_t buf, u32 tmp_len, int xfter)
{
	struct device	*dev = &as->pdev->dev;

	if (buf != INVALID_DMA_ADDRESS) {
		if (xfter == SPI_DMA_TX)
			dma_unmap_single(dev, as->tx_dma,
						tmp_len, DMA_TO_DEVICE);
		else if (xfter == SPI_DMA_RX)
			dma_unmap_single(dev, as->rx_dma,
						tmp_len, DMA_FROM_DEVICE);
	}
}

static void
spi_dma_transfer(void *buf, u32 tmp_len, struct ali_spi *as , int xfter)
{
	u32 value;
	int ret;

	as->dma_align_flag = 1;
	as->spi_dma_start = 1;

	/* 3921 set nand IMB share nor 1030 28bit 1 */
	if (as->chip_id == C3921)
		soc_write32(as, ALI_MEM_BIU2_REG,
			soc_read32(as, ALI_MEM_BIU2_REG) | (IMB_SHARE_ENABLE));

	/*set reg BC*/
	sfreg_write32(as, tmp_len, ALI_SPI_FLASH_COUNTER);

	/* set reg 5C*/
	sfreg_write32(as, 0, ALI_SPI_DMA_FLASH_ADDR);

	/* set reg 60*/
	sfreg_write32(as, tmp_len, ALI_SPI_DMA_LEN);

	if (xfter == SPI_DMA_TX) {

		as->dma_xfer = xfter;
		as->tx_dma_len = tmp_len;

		/* set the DMA buffer map */
		if (as->is_dma_mapped)
			as->tx_dma = as->current_transfer->tx_dma;
		 else {
		 	/* There are some conditions to set dma map 
			     1. buffer must align 0x1f
			     2. not vmalloc on virtual addresses
			*/
		 	if ((!((u32) buf & ALIGN_LEN)) && (!is_vmalloc_addr(buf))) {
			ret = spi_dma_map(as, buf, tmp_len, SPI_DMA_TX);
			if (ret < 0) {
				dev_err(&as->pdev->dev, "DMA map Tx fail\n");
				return;
			}
		 	} else {
		 		/* use copy to alloc DMA buffer */
				as->dma_align_flag = 0;
				as->tx_dma = as->buf_dma;
			}
		}

		if (!as->dma_align_flag)
			memcpy(as->buf, buf, tmp_len);

		/* set reg 58*/
		sfreg_write32(as, ((as->tx_dma) & 0x3fffffff),
					ALI_SPI_DMA_MEM_ADDR);

		/*set reg 98*/
		value  = sfreg_read32(as, ALI_SPI_INS);
		value |= (SPI_CONTINUE_COUNT | SPI_CONTINUE_WRITE_CTRL);
		sfreg_write32(as, value, ALI_SPI_INS);

		 /*set reg 64*/
		sfreg_write32(as, (SPI_DMA_TX_CTRL | as->stay->chip_select),
					ALI_SPI_DMA_CTRL);

	} else {
		/* SPI_DMA_RX  */

		as->dma_xfer = xfter;
		as->rx_dma_len = tmp_len;

		/* set the DMA buffer map */
		if (as->is_dma_mapped)
			as->rx_dma = as->current_transfer->rx_dma;
		else {
			/* There are some conditions to set dma map 
			     1. buffer must align 0x1f
			     2. not vmalloc on virtual addresses
			*/
			if ((!((u32) buf & ALIGN_LEN)) && (!is_vmalloc_addr(buf))) {
			ret = spi_dma_map(as, buf, tmp_len, SPI_DMA_RX);
			if (ret < 0) {
				dev_err(&as->pdev->dev, "DMA map Rx fail\n");
				return;
			}
			} else {
				/* use copy to alloc DMA buffer */
				as->dma_align_flag = 0;
				as->rx_dma = as->buf_dma;
			}
		}

		/* set reg 58*/
		sfreg_write32(as, ((as->rx_dma) & 0x3fffffff),
					ALI_SPI_DMA_MEM_ADDR);

		/*set reg 98*/
		value = sfreg_read32(as, ALI_SPI_INS);
		value |= (SPI_CONTINUE_COUNT | SPI_CONTINUE_READ_CTRL);
		sfreg_write32(as, value, ALI_SPI_INS);

		 /*set reg 64*/
		sfreg_write32(as, (SPI_DMA_RX_CTRL | as->stay->chip_select),
						ALI_SPI_DMA_CTRL);

	}
}

static void set_multi_io_reg(struct ali_spi *as, int set_mode)
{
	u32 value;

	if (set_mode == MULTI_IO_0) {
		value = sfreg_read8(as, ALI_SPI_MODE);
		value &= ~(0x7);
		sfreg_write8(as, value, ALI_SPI_MODE);
	} else if (set_mode == MULTI_IO_1) {
		value = sfreg_read8(as, ALI_SPI_MODE);
		value |= SPI_IO_MODE_1;
		sfreg_write8(as, value, ALI_SPI_MODE);
	} else if (set_mode == MULTI_IO_5) {
		value = sfreg_read8(as, ALI_SPI_MODE);
		value |= SPI_IO_MODE_5;
		sfreg_write8(as, value, ALI_SPI_MODE);
	} else {
		dev_err(&as->pdev->dev, "not set multi io reg\n");
	}
}

static void set_multi_io_mode(struct ali_spi *as, u8 cmd)
{
	if (as->spi_cmd_flag && (cmd == ALI_SPI_RX_DUAL
		|| cmd == ALI_SPI_NOR_RX_DUAL
		|| cmd == ALI_SPI_RX_QUAD
		|| cmd == ALI_SPI_NOR_RX_QUAD
		|| cmd == ALI_SPI_TX_QUAD)) {
		switch (cmd) {
		case ALI_SPI_RX_DUAL:
		case ALI_SPI_NOR_RX_DUAL:
				as->spi_rx_dual = 1;
				break;

		case ALI_SPI_RX_QUAD:
		case ALI_SPI_NOR_RX_QUAD:
				as->spi_rx_quad = 1;
				break;

		case ALI_SPI_TX_QUAD:
				as->spi_tx_quad = 1;
				break;

		default:
				dev_err(&as->pdev->dev, "not set io mode\n");
				break;
		}
	}
}

static void set_cs_reg(struct ali_spi *as, int set_mode)
{
	u32 value;

	if (set_mode == CS_ENABLE) {
		value = sfreg_read32(as, ALI_SPI_CS);
		value |= (SPI_CS_CTRL_ENABLE | SPI_CS_ENABLE);
		sfreg_write32(as, value,  ALI_SPI_CS);
	} else if (set_mode == CS_DISABLE) {
		value = sfreg_read32(as, ALI_SPI_CS);
		value &= ~(SPI_CS_CTRL_ENABLE);
		sfreg_write32(as, value,  ALI_SPI_CS);
	} else {
		dev_err(&as->pdev->dev, "not set chip select reg\n");
	}
}

static void transfer_data_tx(void *tx_buf, u32 len, struct ali_spi *as)
{
	u32 tmp_len;
	u32 ali_flash_cs_addr;
	u32 addr, i;

	ali_flash_cs_addr =
			(u32)as->ali_sflash_cs_addr[as->stay->chip_select];

	if (as->chip_id == C3505)
		ali_flash_cs_addr |= SOC_DRAM_SET;
	tmp_len = len;
	sfreg_write8(as, SPI_HIT_DATA, ALI_SPI_FMT);
	set_multi_io_mode(as, *(u8 *)tx_buf);
	set_multi_io_reg(as, MULTI_IO_0);

	/* if cmd 6b to set quad so len == 1, but addr is 6b than len > 1 */
	if (as->spi_tx_quad && len >= DMA_MIN_LEN) {
		set_multi_io_reg(as, MULTI_IO_5);
		as->spi_tx_quad = 0;
	}

	/* patch to read cc error 	
	CPU access FLASH IO register and FLASH memory could be out of order.
	To make sure IO register configuration is successful before access
	FLASH memory, please read back IO register value confirm the
	configuration before access FLASH memory */
	if (as->spi_cmd_flag)
		sfreg_read32(as, 0);

	if (as->spi_dma_flag && (tmp_len >= DMA_MIN_LEN) &&
						(!(tmp_len & ALIGN_LEN))) {
		spi_dma_transfer(tx_buf, tmp_len, as, SPI_DMA_TX);
	} else {
		i = 0;
		while (tmp_len) {
			addr = *(u8 *)((u32) tx_buf + i);
			*(u8 *) (ali_flash_cs_addr + addr) = addr;
			tmp_len--;
			i++;
		}
	}
}

static void transfer_data_rx(void *rx_buf, u32 len, struct ali_spi *as)
{
	u32 tmp_len;
	u32 ali_flash_cs_addr;
	int i;

	sfreg_write8(as, SPI_HIT_DATA, ALI_SPI_FMT);
	set_multi_io_reg(as, MULTI_IO_0);

	if (as->spi_rx_dual) {
		set_multi_io_reg(as, MULTI_IO_1);
		as->spi_rx_dual = 0;
	}
	if (as->spi_rx_quad) {
		set_multi_io_reg(as, MULTI_IO_5);
		as->spi_rx_quad = 0;
	}

	ali_flash_cs_addr =
			(u32)as->ali_sflash_cs_addr[as->stay->chip_select];

	if (as->chip_id == C3505)
		ali_flash_cs_addr |= SOC_DRAM_SET;
	tmp_len = len;

	/* patch to read cc error 	
	CPU access FLASH IO register and FLASH memory could be out of order.
	To make sure IO register configuration is successful before access
	FLASH memory, please read back IO register value confirm the
	configuration before access FLASH memory */
	sfreg_read32(as, 0);

	if (as->spi_dma_flag && (tmp_len >= DMA_MIN_LEN) &&
						(!(tmp_len & ALIGN_LEN))) {
		spi_dma_transfer(rx_buf, tmp_len, as, SPI_DMA_RX);
	} else {
		i = 0;

		while (tmp_len) {
			/* align */
			if (((((u32) rx_buf + i) & 3) == 0) && (tmp_len >= 4)) {
				*(u32 *)((u32) rx_buf + i) =
					*(u32 *)(ali_flash_cs_addr);
				tmp_len -= 4;
				i += 4;
			} else {
				while ((((((u32)rx_buf + i) & 3) != 0) ||
						(tmp_len < 4)) && tmp_len) {
					*(u8 *)((u32)rx_buf + i) =
						*(u8 *)(ali_flash_cs_addr);
					i++;
					tmp_len--;
				}
			}
		}
	}
}


static int ali_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct ali_spi  *as;
	struct spi_transfer *parse_xfer;
	struct spi_transfer *xfer;
	int  status = 0;
	bool cs_flag = 0;

	as = spi_master_get_devdata(spi->master);
	as->stay = spi;
	as->ali_msg = msg;
	msg->actual_length = 0;

	/* dma_start not set==> cmd or pio  */
	as->spi_dma_start = 0;

	/* cmd to set multi-io mode  */
	as->spi_cmd_flag = 1;

	/* set the dma mode by device tree */
	if (as->dma_mode)
		as->spi_dma_flag = as->dma_mode;
	else
		as->spi_dma_flag = 0;

	/* set pinmux to spi */
	spi_pinmux_set(as);

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {

		/* check device clock rate */
		if ((xfer->speed_hz / MHZ) < as->flash_ctrl_clk_select)
			spi_set_flash_ctrl_clk(as, xfer->speed_hz / MHZ);
		else
			spi_set_flash_ctrl_clk(as, as->flash_ctrl_clk_select);

		/* check device clock mode */
		spi_set_clock_mode(as, spi->mode);

		/* set chip select reg C8 */
		set_cs_reg(as, CS_ENABLE);
		parse_xfer = xfer;
		as->current_transfer = xfer;
		as->is_dma_mapped = msg->is_dma_mapped;

		if (parse_xfer->tx_buf != NULL && parse_xfer->len != 0) {
				cs_flag = 1;
				msg->actual_length += parse_xfer->len;
				transfer_data_tx((void *)parse_xfer->tx_buf,
						(u32) parse_xfer->len, as);
				as->spi_cmd_flag = 0;
		}
		if (parse_xfer->rx_buf != NULL && parse_xfer->len != 0) {
				msg->actual_length += parse_xfer->len;
				transfer_data_rx(parse_xfer->rx_buf,
						(u32) parse_xfer->len, as);
				cs_flag = 0;
		}
		if (!cs_flag && !as->spi_dma_start) {
			/* disable chip select reg C8 */
			set_cs_reg(as, CS_DISABLE);
		}
	}
	if (!as->spi_dma_start) {
		/* disable chip select reg C8 */
		set_cs_reg(as, CS_DISABLE);
		/* disable pinmux to spi */
		spi_pinmux_release(as);
		msg->status = status;
		msg->complete(msg->context);
	}

	return 0;

}

static void ali_spi_cleanup(struct spi_device *spi)
{
	dev_dbg(&spi->dev, "%s.\n", __func__);
}

/*-------------------------------------------------------------------------*/

/*
 *  alidev_udc_irq - interrupt handler
 */

static irqreturn_t ali_spi_irq(int dummy, void *as)
{
	struct ali_spi *irq_as = (struct ali_spi *)as;
	u32 value;

	sfreg_write32(irq_as,
				SPI_DMA_INT_CLEAN, ALI_SPI_DMA_INT_STATUS);
	sfreg_write32(irq_as, 0x0, ALI_SPI_DMA_CTRL);

	if (irq_as->dma_xfer == SPI_DMA_TX) {
		value = sfreg_read32(irq_as, ALI_SPI_INS);
		value &= ~(SPI_CONTINUE_COUNT | SPI_CONTINUE_WRITE_CTRL);
		sfreg_write32(irq_as, value, ALI_SPI_INS);

		if (!irq_as->is_dma_mapped && irq_as->tx_dma)
			spi_dma_unmap(irq_as, irq_as->tx_dma,
					irq_as->tx_dma_len, SPI_DMA_TX);
	} else {
		value = sfreg_read32(irq_as, ALI_SPI_INS);
		value &= ~(SPI_CONTINUE_COUNT | SPI_CONTINUE_READ_CTRL);
		sfreg_write32(irq_as, value, ALI_SPI_INS);

		if (!irq_as->is_dma_mapped && irq_as->rx_dma)
			spi_dma_unmap(irq_as, irq_as->rx_dma,
					irq_as->rx_dma_len, SPI_DMA_RX);

		if (!irq_as->dma_align_flag)
			memcpy(irq_as->current_transfer->rx_buf,
					irq_as->buf, irq_as->rx_dma_len);
	}

	/* 3921 disable nand IMB share nor 1030 28bit 1 */
	if (irq_as->chip_id == C3921)
		soc_write32(irq_as, ALI_MEM_BIU2_REG,
			soc_read32(irq_as, ALI_MEM_BIU2_REG) & ~(IMB_SHARE_ENABLE));

	/* set pinmux to default reg C8*/
	set_cs_reg(irq_as, CS_DISABLE);
	/* disable pinmux to spi */
	spi_pinmux_release(as);

	if (irq_as->ali_msg) {
		irq_as->ali_msg->status = 0;
		irq_as->ali_msg->complete(irq_as->ali_msg->context);
	}

	return IRQ_HANDLED;
}

static int ali_spi_probe(struct platform_device *pdev)
{
	int     ret;
	struct spi_master *master;
	struct ali_spi  *as;
	int i;

	/*add for device tree*/
	struct device   *dev = &pdev->dev;
	struct resource *res;
	void __iomem    *base;

	dev_dbg(&pdev->dev,"SPI_CONTROLLER_VERSION: %s\n", SPI_CONTROLLER_VERSION);

	/* setup spi core then ali-specific driver state */
	ret = -ENOMEM;
	master = spi_alloc_master(&pdev->dev, sizeof(*as));
	if (!master)
		goto out_free;

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH
						| SPI_TX_DUAL | SPI_TX_QUAD
						| SPI_RX_DUAL | SPI_RX_QUAD;
	master->bus_num = 1;      /* pdev->id  not set*/
	master->num_chipselect = CHIP_SELECT_MAX_NUM;
	master->setup = ali_spi_setup;
	master->dev.of_node = pdev->dev.of_node;
	master->bus_num = pdev->id;
	master->transfer = ali_spi_transfer;
	master->cleanup = ali_spi_cleanup;
	platform_set_drvdata(pdev, master);

	as = spi_master_get_devdata(master);

	/* get sflash reg */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed %s (%d)\n", __func__, __LINE__);
		ret = -ENXIO;
		goto out_free;
	}
	as->ali_sflash_base_addr = ioremap(res->start,
						res->end - res->start + 1);
	if (!as->ali_sflash_base_addr) {
		dev_err(&pdev->dev, "could not ioremap I/O port range\n");
		ret = -EFAULT;
		goto out_free;
	}

	/* add for device tree */
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	/* get chip select reg */
	for (i = 0; i < CHIP_SELECT_MAX_NUM; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i + 1);
		if (!res) {
			dev_err(&pdev->dev, "failed %s (%d)\n",
			__func__, __LINE__);
			ret = -ENXIO;
			goto out_free;
		}
		as->ali_sflash_cs_addr[i] = ioremap(res->start,
						res->end - res->start + 1);
		if (!as->ali_sflash_cs_addr[i]) {
			dev_err(&pdev->dev, "could not ioremap I/O port range\n");
			ret = -EFAULT;
			goto out_free;
		}
	}

	/* get irq */
	as->irq = platform_get_irq(pdev, 0);
	if (as->irq < 0) {
			dev_err(&pdev->dev, "failed %s (%d)\n",
			__func__, __LINE__);
			goto out_free;
	}
	ret = request_irq(as->irq, ali_spi_irq, 0, dev_name(&pdev->dev), as);
	if (ret != 0) {
			dev_err(&pdev->dev, "failed %s (%d)\n",
			__func__, __LINE__);
			goto out_free;
	}
	
	ret = of_property_read_u32(pdev->dev.of_node, "ctrl_clk_select",
						&as->flash_ctrl_clk_select);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed %s (%d)\n", __func__, __LINE__);
		ret = -ENXIO;
		goto out_free_irq;
	}
	
	ret = of_property_read_u32(pdev->dev.of_node, "dma_mode",
							&as->dma_mode);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed %s (%d)\n", __func__, __LINE__);
		ret = -ENXIO;
		goto out_free_irq;
	}

	/*
	* Set default dma-mask to 32 bit. Drivers are expected to setup
	* the correct supported dma_mask.
	*/
	dev->coherent_dma_mask = DMA_BIT_MASK(32);

	/*
	* Set it to coherent_dma_mask by default if the architecture
	* code has not set it.
	*/
	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;

	as->buf = dma_alloc_coherent(&pdev->dev, DMA_BUF_SIZE,
					&as->buf_dma, GFP_KERNEL);
	if (!as->buf) {
			dev_err(&pdev->dev, "failed %s (%d)\n",
			__func__, __LINE__);
			ret = -ENOMEM;
			goto out_free_irq;
	}
	
	/* set soc base address ioremap */
#if defined(CONFIG_ARM)
	as->ali_soc_base_addr = ioremap(ALI_SOC_BASE_ADDR, ALI_SOC_BASE_ADDR_SIZE);
#elif defined(CONFIG_MIPS)
	as->ali_soc_base_addr = (void __iomem  *)ALI_SOC_BASE_ADDR;
#endif

	spin_lock_init(&as->lock);
	INIT_LIST_HEAD(&as->queue);
	as->pdev = pdev;
	ret = spi_register_master(master);
	if (ret) {
		dev_err(&pdev->dev, "failed %s (%d)\n",
		__func__, __LINE__);
		goto out_free_dma;
	}
	
	return 0;

out_free_irq:
	free_irq(as->irq, &pdev->dev);
out_free_dma:
	dma_free_coherent(&pdev->dev, DMA_BUF_SIZE, as->buf, as->buf_dma);
out_free:
	spi_master_put(master);
	return ret;
}

static int __exit ali_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct ali_spi  *as = spi_master_get_devdata(master);
	struct spi_message  *msg;

	/* reset the hardware and block queue progress */
	as->stopping = 1;

	/* Terminate remaining queued transfers */
	list_for_each_entry(msg, &as->queue, queue) {
		/* REVISIT unmapping the dma is a NOP on ARM and AVR32
		 * but we shouldn't depend on that...
		 */
		msg->status = -ESHUTDOWN;
		msg->complete(msg->context);
	}
	free_irq( as->irq, &pdev->dev);
	dma_free_coherent(&pdev->dev, DMA_BUF_SIZE, as->buf, as->buf_dma);
	spi_unregister_master(master);
	platform_device_unregister(pdev);

	return 0;
}

static int ali_spi_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int ali_spi_resume(struct platform_device *pdev)
{
	unsigned int chip_id = __REG32ALI(0xb8000000) & 0xFFFF0000;

	__REG32ALI(0x1802E098) = 0xC1000D03;
	if(0x35050000 == chip_id)
	{
		__REG32ALI(0xB8070000) |= (1<<0);
	}
	else
	{
		__REG32ALI(0xB8000110) |= (1<<0);
	}

	return 0;
}

/*add for device tree.*/
static const struct of_device_id ali_spi_of_match[] = {
{ .compatible = "alitech,spictrl", },
	 {},
};
MODULE_DEVICE_TABLE(of, ali_spi_of_match)

static struct platform_driver ali_spi_driver = {
	.probe = ali_spi_probe,
	.remove = __exit_p(ali_spi_remove),
	.suspend = ali_spi_suspend,
	.resume = ali_spi_resume,
	.driver = {
		.name = "ali_spi_bus",
		.owner  = THIS_MODULE,
		.of_match_table = ali_spi_of_match,
	},
};
static int __init ali_spi_init(void)
{
	int ret;
	ret = platform_driver_register(&ali_spi_driver);
	return ret;
}

static void __exit ali_spi_exit(void)
{
	platform_driver_unregister(&ali_spi_driver);
}


module_init(ali_spi_init);
module_exit(ali_spi_exit);

MODULE_DESCRIPTION("Ali SPI Controller driver");
MODULE_AUTHOR("Barry Chang");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ali_spi_bus");
MODULE_VERSION(SPI_CONTROLLER_VERSION);

