/*
 * MTD SPI driver for ST M25Pxx (and similar) serial flash chips
 *
 * Author: Mike Lavender, mike@steroidmicros.com
 *
 * Copyright (c) 2015 ALi Corporation.
 *
 * Some parts are based on lart.c by Abraham Van Der Merwe
 *
 * Cleaned up and generalized based on mtd_dataflash.c
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mod_devicetable.h>

#include <linux/mtd/cfi.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/of_platform.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>

#define SPI_NOR_VERSION "1.0.0"
/* Flash opcodes. */
#define OPCODE_WREN             0x06    /* Write enable */
#define OPCODE_RDSR             0x05    /* Read status register */
#define OPCODE_WRSR             0x01    /* Write status register 1 byte */
#define OPCODE_NORM_READ        0x03    /* Read data bytes (low frequency) */
#define OPCODE_FAST_READ        0x0b    /* Read data bytes (high frequency) */
#define OPCODE_DUAL_READ        0x3b    /* Read data bytes (Dual SPI) */
#define OPCODE_QUAD_READ        0x6b    /* Read data bytes (Quad SPI) */
#define OPCODE_PP               0x02    /* Page program (up to 256 bytes) */
#define OPCODE_BE_4K            0x20    /* Erase 4KiB block */
#define OPCODE_BE_4K_PMC        0xd7    /* Erase 4KiB block on PMC chips */
#define OPCODE_BE_32K           0x52    /* Erase 32KiB block */
#define OPCODE_CHIP_ERASE       0xc7    /* Erase whole flash chip */
#define OPCODE_SE               0xd8    /* Sector erase (usually 64KiB) */
#define OPCODE_RDID             0x9f    /* Read JEDEC ID */
#define OPCODE_RDCR             0x35    /* Read configuration register */

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define OPCODE_NORM_READ_4B     0x13    /* Read data bytes (low frequency) */
#define OPCODE_FAST_READ_4B     0x0c    /* Read data bytes (high frequency) */
#define OPCODE_DUAL_READ_4B     0x3c    /* Read data bytes (Dual SPI) */
#define OPCODE_QUAD_READ_4B     0x6c    /* Read data bytes (Quad SPI) */
#define OPCODE_PP_4B            0x12    /* Page program (up to 256 bytes) */
#define OPCODE_SE_4B            0xdc    /* Sector erase (usually 64KiB) */

/* Used for SST flashes only. */
#define OPCODE_BP               0x02    /* Byte program */
#define OPCODE_WRDI             0x04    /* Write disable */
#define OPCODE_AAI_WP           0xad  /* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define OPCODE_EN4B             0xb7    /* Enter 4-byte mode */
#define OPCODE_EX4B             0xe9    /* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define OPCODE_BRWR             0x17    /* Bank register write */

/* Status Register bits. */
#define SR_WIP                  1       /* Write in progress */
#define SR_WEL                  2       /* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0                  4       /* Block protect 0 */
#define SR_BP1                  8       /* Block protect 1 */
#define SR_BP2                  0x10    /* Block protect 2 */
#define SR_SRWD                 0x80    /* SR write protect */

#define SR_QUAD_EN_MX           0x40    /* Macronix Quad I/O */

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN         0x2     /* Spansion Quad I/O */

/* Define max times to check status register before we give up. */
/* M25P16 specs 40s max chip erase */
#define MAX_READY_WAIT_JIFFIES  (40 * HZ)
#define MAX_CMD_SIZE            6

#define JEDEC_MFR(_jedec_id)    ((_jedec_id) >> 16)

typedef unsigned char  	UINT8;
typedef unsigned short 	UINT16;
typedef unsigned int    UINT32;
typedef int             INT32;

#define MFR_SPANSION 0x0001
#define MFR_MXIC 0x00c2
#define MFR_GIGADEVICE 0x00c8
#define MFR_WINDBOND 0x00ef
/****************************************************************************/

enum read_type {
	M25P80_NORMAL = 0,
	M25P80_FAST,
	M25P80_DUAL,
	M25P80_QUAD,
};

struct m25p {
	struct spi_device       *spi;
	struct mutex            lock;
	struct mtd_info         mtd;
	u16                     page_size;
	u16                     addr_width;
	u8                      erase_opcode;
	u8                      read_opcode;
	u8                      program_opcode;
	u8                      *command;
	enum read_type          flash_read;
};

static inline struct m25p *mtd_to_m25p(struct mtd_info *mtd)
{
	return container_of(mtd, struct m25p, mtd);
}

/****************************************************************************/

/*
 * Internal helper functions
 */

/*
 * Read the status register, returning its value in the location
 * Return the status register value.
 * Returns negative if error occurred.
 */
static int read_sr(struct m25p *flash)
{
	ssize_t retval;
	u8 code = OPCODE_RDSR;
	u8 val;

	retval = spi_write_then_read(flash->spi, &code, 1, &val, 1);

	if (retval < 0) {
		dev_err(&flash->spi->dev, "error %d reading SR\n",
				(int) retval);
		return retval;
	}

	return val;
}

/*
 * Read configuration register, returning its value in the
 * location. Return the configuration register value.
 * Returns negative if error occured.
 */
static int read_cr(struct m25p *flash)
{
	u8 code = OPCODE_RDCR;
	int ret;
	u8 val;

	ret = spi_write_then_read(flash->spi, &code, 1, &val, 1);
	if (ret < 0) {
		dev_err(&flash->spi->dev, "error %d reading CR\n", ret);
		return ret;
	}

	return val;
}

/*
 * Write status register 1 byte
 * Returns negative if error occurred.
 */
static int write_sr(struct m25p *flash, u8 val)
{
	flash->command[0] = OPCODE_WRSR;
	flash->command[1] = val;

	return spi_write(flash->spi, flash->command, 2);
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int write_enable(struct m25p *flash)
{
	u8      code = OPCODE_WREN;

	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}

/*
 * Send write disble instruction to the chip.
 */
static inline int write_disable(struct m25p *flash)
{
	u8      code = OPCODE_WRDI;

	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}

/*
 * Enable/disable 4-byte addressing mode.
 */
static inline int set_4byte(struct m25p *flash, u32 jedec_id, int enable)
{
	int status;
	bool need_wren = false;

	switch (JEDEC_MFR(jedec_id)) {
	case CFI_MFR_ST: /* Micron, actually */
		/* Some Micron need WREN command; all will accept it */
		need_wren = true;
	case CFI_MFR_MACRONIX:
	case 0xEF /* winbond */:
		if (need_wren)
			write_enable(flash);

		flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
		status = spi_write(flash->spi, flash->command, 1);

		if (need_wren)
			write_disable(flash);

		return status;
	default:
		/* Spansion style */
		flash->command[0] = OPCODE_BRWR;
		flash->command[1] = enable << 7;
		return spi_write(flash->spi, flash->command, 2);
	}
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct m25p *flash)
{
	unsigned long deadline;
	int sr;

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	do {
		sr = read_sr(flash);
		if (sr < 0)
			break;
		else if (!(sr & SR_WIP))
			return 0;

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	return 1;
}

/*
 * Write status Register and configuration register with 2 bytes
 * The first byte will be written to the status register, while the
 * second byte will be written to the configuration register.
 * Return negative if error occured.
 */
static int write_sr_cr(struct m25p *flash, u16 val)
{
	flash->command[0] = OPCODE_WRSR;
	flash->command[1] = val & 0xff;
	flash->command[2] = (val >> 8);

	return spi_write(flash->spi, flash->command, 3);
}

static int macronix_quad_enable(struct m25p *flash)
{
	int ret, val;
	u8 cmd[2];
	cmd[0] = OPCODE_WRSR;

	val = read_sr(flash);
	cmd[1] = val | SR_QUAD_EN_MX;
	write_enable(flash);

	spi_write(flash->spi, &cmd, 2);

	if (wait_till_ready(flash))
		return 1;

	ret = read_sr(flash);
	if (!(ret > 0 && (ret & SR_QUAD_EN_MX))) {
		dev_err(&flash->spi->dev, "Macronix Quad bit not set\n");
		return -EINVAL;
	}

	return 0;
}

static int spansion_quad_enable(struct m25p *flash)
{
	int ret;
	int quad_en = CR_QUAD_EN_SPAN << 8;

	write_enable(flash);

	ret = write_sr_cr(flash, quad_en);
	if (ret < 0) {
		dev_err(&flash->spi->dev,
			"error while writing configuration register\n");
		return -EINVAL;
	}

	/* read back and check it */
	ret = read_cr(flash);
	if (!(ret > 0 && (ret & CR_QUAD_EN_SPAN))) {
		dev_err(&flash->spi->dev, "Spansion Quad bit not set\n");
		return -EINVAL;
	}

	return 0;
}

static int set_quad_mode(struct m25p *flash, u32 jedec_id)
{
	int status;

	switch (JEDEC_MFR(jedec_id)) {
	case CFI_MFR_MACRONIX:
		status = macronix_quad_enable(flash);
		if (status) {
			dev_err(&flash->spi->dev,
				"Macronix quad-read not enabled\n");
			return -EINVAL;
		}
		return status;
	default:
		status = spansion_quad_enable(flash);
		if (status) {
			dev_err(&flash->spi->dev,
				"Spansion quad-read not enabled\n");
			return -EINVAL;
		}
		return status;
	}
}

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_chip(struct m25p *flash)
{
	pr_debug("%s: %s %lldKiB\n", dev_name(&flash->spi->dev), __func__,
			(long long)(flash->mtd.size >> 10));

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	write_enable(flash);

	/* Set up command buffer. */
	flash->command[0] = OPCODE_CHIP_ERASE;

	spi_write(flash->spi, flash->command, 1);

	return 0;
}

static void m25p_addr2cmd(struct m25p *flash, unsigned int addr, u8 *cmd)
{
	/* opcode is in cmd[0] */
	cmd[1] = addr >> (flash->addr_width * 8 -  8);
	cmd[2] = addr >> (flash->addr_width * 8 - 16);
	cmd[3] = addr >> (flash->addr_width * 8 - 24);
	cmd[4] = addr >> (flash->addr_width * 8 - 32);
}

static int m25p_cmdsz(struct m25p *flash)
{
	return 1 + flash->addr_width;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct m25p *flash, u32 offset)
{
	pr_debug("%s: %s %dKiB at 0x%08x\n", dev_name(&flash->spi->dev),
			__func__, flash->mtd.erasesize / 1024, offset);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	write_enable(flash);

	/* Set up command buffer. */
	flash->command[0] = flash->erase_opcode;
	m25p_addr2cmd(flash, offset, flash->command);

	spi_write(flash->spi, flash->command, m25p_cmdsz(flash));

	return 0;
}

/****************************************************************************/

/*
 * MTD implementation
 */

/*
 * Erase an address range on the flash chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int m25p80_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	u32 addr, len;
	uint32_t rem;

	mutex_lock(&flash->lock);

	pr_debug("%s: %s at 0x%llx, len %lld\n", dev_name(&flash->spi->dev),
			__func__, (long long)instr->addr,
			(long long)instr->len);

	div_u64_rem(instr->len, mtd->erasesize, &rem);
	if (rem) {
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}

	addr = instr->addr;
	len = instr->len;

	/* whole-chip erase? */
	if (len == flash->mtd.size) {
		if (erase_chip(flash)) {
			instr->state = MTD_ERASE_FAILED;
			mutex_unlock(&flash->lock);
			return -EIO;
		}

	/* REVISIT in some cases we could speed up erasing large regions
	 * by using OPCODE_SE instead of OPCODE_BE_4K.  We may have set up
	 * to use "small sector erase", but that's not always optimal.
	 */

	/* "sector"-at-a-time erase */
	} else {
		while (len) {
			if (erase_sector(flash, addr)) {
				instr->state = MTD_ERASE_FAILED;
				mutex_unlock(&flash->lock);
				return -EIO;
			}

			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}
	}

	mutex_unlock(&flash->lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

/*
 * Dummy Cycle calculation for different type of read.
 * It can be used to support more commands with
 * different dummy cycle requirements.
 */
static inline int m25p80_dummy_cycles_read(struct m25p *flash)
{
	switch (flash->flash_read) {
	case M25P80_FAST:
	case M25P80_DUAL:
	case M25P80_QUAD:
		return 1;
	case M25P80_NORMAL:
		return 0;
	default:
		dev_err(&flash->spi->dev, "No valid read type supported\n");
		return -1;
	}
}

static inline unsigned int m25p80_rx_nbits(const struct m25p *flash)
{
	switch (flash->flash_read) {
	case M25P80_DUAL:
		return 2;
	case M25P80_QUAD:
		return 4;
	default:
		return 0;
	}
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
 /* change the read function to read page by page */
static int m25p80_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	struct spi_transfer t[2];
	struct spi_message m;
	uint8_t opcode;
	int dummy;
	u32 i, page_offset, page_size = 0;
	u32 page_limit = 0x800;

	mutex_lock(&flash->lock);

	if (retlen == NULL) {
		dev_err(&flash->spi->dev, "The retlen is NULL\n");
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}

	if (flash->page_size > 0x800)
		page_limit = flash->page_size;

	/* sanity checks */
	if (!len) {
		mutex_unlock(&flash->lock);
		return 0;
	}

	if (from + len > flash->mtd.size) {
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}

	pr_debug("%s: %s from 0x%08x, len %zd\n", dev_name(&flash->spi->dev),
			__func__, (u32)from, len);

	spi_message_init(&m);
	memset(t, 0, sizeof(t));

	dummy =  m25p80_dummy_cycles_read(flash);
	if (dummy < 0) {
		dev_err(&flash->spi->dev, "No valid read command supported\n");
		mutex_unlock(&flash->lock);
		return -EINVAL;
	}

	/* Set up the write data buffer. */
	opcode = flash->read_opcode;
	flash->command[0] = opcode;
	m25p_addr2cmd(flash, from, flash->command);

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash) + dummy;
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = buf;
	t[1].rx_nbits = m25p80_rx_nbits(flash);
	spi_message_add_tail(&t[1], &m);

	/* Byte count starts at zero. */
	if (retlen)
		*retlen = 0;

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		/* REVISIT status return?? */
		mutex_unlock(&flash->lock);
		return 1;
	}

	page_offset = from & (page_limit - 1);

	/* do all the bytes fit onto one page? */
	if (page_offset + len <= page_limit) {
		t[1].len = len;
		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash)
			- dummy;
	} else {

		/* the size of data remaining on the first page */
		page_size = page_limit - page_offset;

		t[1].len = page_size;
		spi_sync(flash->spi, &m);
		*retlen = m.actual_length - m25p_cmdsz(flash)
			- dummy;

		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > page_limit)
				page_size = page_limit;
			m25p_addr2cmd(flash, from + i, flash->command);
			t[1].len = page_size;
			t[1].rx_buf = buf + i;

			spi_sync(flash->spi, &m);

			*retlen += m.actual_length - m25p_cmdsz(flash)
			- dummy;
		}
	}
	mutex_unlock(&flash->lock);

	return 0;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int m25p80_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	u32 page_offset, page_size;
	struct spi_transfer t[2];
	struct spi_message m;

	mutex_lock(&flash->lock);

	pr_debug("%s: %s to 0x%08x, len %zd\n", dev_name(&flash->spi->dev),
			__func__, (u32)to, len);

	spi_message_init(&m);
	memset(t, 0, sizeof(t));

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	spi_message_add_tail(&t[1], &m);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash)) {
		mutex_unlock(&flash->lock);
		return 1;
	}

	write_enable(flash);

	/* Set up the opcode in the write buffer. */
	flash->command[0] = flash->program_opcode;
	m25p_addr2cmd(flash, to, flash->command);

	page_offset = to & (flash->page_size - 1);

	/* do all the bytes fit onto one page? */
	if (page_offset + len <= flash->page_size) {
		t[1].len = len;

		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash);
	} else {
		u32 i;

		/* the size of data remaining on the first page */
		page_size = flash->page_size - page_offset;

		t[1].len = page_size;
		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash);

		/* write everything in flash->page_size chunks */
		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > flash->page_size)
				page_size = flash->page_size;

			/* write the next page to flash */
			m25p_addr2cmd(flash, to + i, flash->command);

			t[1].tx_buf = buf + i;
			t[1].len = page_size;

			wait_till_ready(flash);

			write_enable(flash);

			spi_sync(flash->spi, &m);

			*retlen += m.actual_length - m25p_cmdsz(flash);
		}
	}

	mutex_unlock(&flash->lock);

	return 0;
}

static int sst_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	struct spi_transfer t[2];
	struct spi_message m;
	size_t actual;
	int cmd_sz, ret;

	pr_debug("%s: %s to 0x%08x, len %zd\n", dev_name(&flash->spi->dev),
			__func__, (u32)to, len);

	spi_message_init(&m);
	memset(t, 0, sizeof(t));

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	spi_message_add_tail(&t[1], &m);

	mutex_lock(&flash->lock);

	/* Wait until finished previous write command. */
	ret = wait_till_ready(flash);
	if (ret)
		goto time_out;

	write_enable(flash);

	actual = to % 2;
	/* Start write from odd address. */
	if (actual) {
		flash->command[0] = OPCODE_BP;
		m25p_addr2cmd(flash, to, flash->command);

		/* write one byte. */
		t[1].len = 1;
		spi_sync(flash->spi, &m);
		ret = wait_till_ready(flash);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - m25p_cmdsz(flash);
	}
	to += actual;

	flash->command[0] = OPCODE_AAI_WP;
	m25p_addr2cmd(flash, to, flash->command);

	/* Write out most of the data here. */
	cmd_sz = m25p_cmdsz(flash);
	for (; actual < len - 1; actual += 2) {
		t[0].len = cmd_sz;
		/* write two bytes. */
		t[1].len = 2;
		t[1].tx_buf = buf + actual;

		spi_sync(flash->spi, &m);
		ret = wait_till_ready(flash);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - cmd_sz;
		cmd_sz = 1;
		to += 2;
	}
	write_disable(flash);
	ret = wait_till_ready(flash);
	if (ret)
		goto time_out;

	/* Write out trailing byte if it exists. */
	if (actual != len) {
		write_enable(flash);
		flash->command[0] = OPCODE_BP;
		m25p_addr2cmd(flash, to, flash->command);
		t[0].len = m25p_cmdsz(flash);
		t[1].len = 1;
		t[1].tx_buf = buf + actual;

		spi_sync(flash->spi, &m);
		ret = wait_till_ready(flash);
		if (ret)
			goto time_out;
		*retlen += m.actual_length - m25p_cmdsz(flash);
		write_disable(flash);
	}

time_out:
	mutex_unlock(&flash->lock);
	return ret;
}

static int m25p80_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	uint32_t offset = ofs;
	uint8_t status_old, status_new;
	int res = 0;

	mutex_lock(&flash->lock);
	/* Wait until finished previous command */
	if (wait_till_ready(flash)) {
		res = 1;
		goto err;
	}

	status_old = read_sr(flash);

	if (offset < flash->mtd.size-(flash->mtd.size/2))
		status_new = status_old | SR_BP2 | SR_BP1 | SR_BP0;
	else if (offset < flash->mtd.size-(flash->mtd.size/4))
		status_new = (status_old & ~SR_BP0) | SR_BP2 | SR_BP1;
	else if (offset < flash->mtd.size-(flash->mtd.size/8))
		status_new = (status_old & ~SR_BP1) | SR_BP2 | SR_BP0;
	else if (offset < flash->mtd.size-(flash->mtd.size/16))
		status_new = (status_old & ~(SR_BP0|SR_BP1)) | SR_BP2;
	else if (offset < flash->mtd.size-(flash->mtd.size/32))
		status_new = (status_old & ~SR_BP2) | SR_BP1 | SR_BP0;
	else if (offset < flash->mtd.size-(flash->mtd.size/64))
		status_new = (status_old & ~(SR_BP2|SR_BP0)) | SR_BP1;
	else
		status_new = (status_old & ~(SR_BP2|SR_BP1)) | SR_BP0;

	/* Only modify protection if it will not unlock other areas */
	if ((status_new&(SR_BP2|SR_BP1|SR_BP0)) >
					(status_old&(SR_BP2|SR_BP1|SR_BP0))) {
		write_enable(flash);
		if (write_sr(flash, status_new) < 0) {
			res = 1;
			goto err;
		}
	}

err:    mutex_unlock(&flash->lock);
	return res;
}

static int m25p80_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	uint32_t offset = ofs;
	uint8_t status_old, status_new;
	int res = 0;

	mutex_lock(&flash->lock);
	/* Wait until finished previous command */
	if (wait_till_ready(flash)) {
		res = 1;
		goto err;
	}

	status_old = read_sr(flash);

	if (offset+len > flash->mtd.size-(flash->mtd.size/64))
		status_new = status_old & ~(SR_BP2|SR_BP1|SR_BP0);
	else if (offset+len > flash->mtd.size-(flash->mtd.size/32))
		status_new = (status_old & ~(SR_BP2|SR_BP1)) | SR_BP0;
	else if (offset+len > flash->mtd.size-(flash->mtd.size/16))
		status_new = (status_old & ~(SR_BP2|SR_BP0)) | SR_BP1;
	else if (offset+len > flash->mtd.size-(flash->mtd.size/8))
		status_new = (status_old & ~SR_BP2) | SR_BP1 | SR_BP0;
	else if (offset+len > flash->mtd.size-(flash->mtd.size/4))
		status_new = (status_old & ~(SR_BP0|SR_BP1)) | SR_BP2;
	else if (offset+len > flash->mtd.size-(flash->mtd.size/2))
		status_new = (status_old & ~SR_BP1) | SR_BP2 | SR_BP0;
	else
		status_new = (status_old & ~SR_BP0) | SR_BP2 | SR_BP1;

	/* Only modify protection if it will not lock other areas */
	if ((status_new&(SR_BP2|SR_BP1|SR_BP0)) <
					(status_old&(SR_BP2|SR_BP1|SR_BP0))) {
		write_enable(flash);
		if (write_sr(flash, status_new) < 0) {
			res = 1;
			goto err;
		}
	}

err:    mutex_unlock(&flash->lock);
	return res;
}

static void sflash_dead_lock(void){
    printk("[%s] going to dead lock...\n", __func__);    
    while(1) {}
}

/*
 * Compatible with ST Micro and similar flash.
 * Supports only the block protection bits BP{0,1,2} in the status register
 * (SR). Does not support these features found in newer SR bitfields:
 *   - TB: top/bottom protect - only handle TB=0 (top protect)
 *   - SEC: sector/block protect - only handle SEC=0 (block protect)
 *   - CMP: complement protect - only support CMP=0 (range is not complemented)
 *
 * Sample table portion for 8MB flash (Winbond w25q64fw):
 *
 *   SEC |  TB | BP2 | BP1 | BP0 | Prot Length | Protected Portion 
 *  ---------------------------------------------------------------
 *    X  |  X  |  0  |  0  |  0  | NONE        | NONE              
 *    0  |  1  |  0  |  0  |  1  | 128 KB      | Lower 1/64        
 *    0  |  1  |  0  |  1  |  0  | 256 KB      | Lower 1/32        
 *    0  |  1  |  0  |  1  |  1  | 512 KB      | Lower 1/16        
 *    0  |  1  |  1  |  0  |  0  | 1 MB        | Lower 1/8         
 *    0  |  1  |  1  |  0  |  1  | 2 MB        | Lower 1/4         
 *    0  |  1  |  1  |  1  |  0  | 4 MB        | Lower 1/2         
 *    X  |  X  |  1  |  1  |  1  | 8 MB        | ALL               
 *
 * Status Register-1 for 8MB flash (Winbond w25q64fw):
 *
 * |  S7  |  S6  |  S5  |  S4  |  S3  |  S2  |  S1  |  S0  | 
 * | SRP0 |  SEC |  TB  |  BP2 |  BP1 |  BP0 |  WEL | BUSY |
 *
 */

#define SR_BP3 1<<5
#define SR_TB  1<<5
#define CON_TB 1<<3

#define MB_FIRST_BP_BIT 2
#define MB_PROTECT_LEVEL 7

#define MX_FIRST_BP_BIT 2
#define MX_PROTECT_LEVEL 16
#define MX_BLOCK_SIZE 0x10000 //64kB

static unsigned int spow2 (unsigned int val) {
    unsigned int ret = 1;
    if (val == 0) return 1;

    return ret << (val);
}

static unsigned int slog2 (unsigned int val) {    
    unsigned int ret = 0;    
    unsigned int comp_val = 1;    
    
    if (val == 0) 
        return UINT_MAX;    
    if (val == 1)
        return 0;    

    while(1){        
        ret++;
        if(comp_val >= val)            
            break;
        comp_val <<= 1;
    }

    return ret;
}

/*
 *  According to the spec, Status Register-1 could be count by function as following:
 *
 *
 *  (protected_size/total_size) <= 1/(2^n)
 *
 *  n = log2(total_size) - log2(protected_size)
 *
 *  SR = 0b00111100 - (n << 2)
 *
 */
static UINT8 wb_size_2_sr(UINT32 total_size, UINT32 protected_size)
{
    UINT8 mask = SR_TB | SR_BP2 | SR_BP1 | SR_BP0;
    UINT8 val;
    UINT32 pow;

    if(0 == total_size || protected_size < 0)
        return -1;
    if(0 == protected_size)
        return 0x00;

    pow = slog2(total_size) - slog2(protected_size);
    
    if(pow > MB_PROTECT_LEVEL - 1)
        pow = MB_PROTECT_LEVEL - 1;

    val = mask - (pow << MB_FIRST_BP_BIT);
    
	if (val & ~mask)
		return -1;
    
    if (!(val & mask))
		return -1;

    return val;
}

static UINT8 wb_sr_2_last_sr(UINT8 protected_type)
{
    UINT8 val;
    UINT8 mask = SR_TB | SR_BP2 | SR_BP1 | SR_BP0;

    if(0 == protected_type)
        return 0;

    val = ((mask - protected_type) >> MB_FIRST_BP_BIT) + 1;
    if(val >= MB_PROTECT_LEVEL){
        val = 0;
    }else{
        val = mask - (val << MB_FIRST_BP_BIT);
        if (val & ~mask)
    		return -1;
        if (!(val & mask))
	    	return -1;
    }

    return val;
}

/*
 *  According to the spec, Protected size could be count by function as following:
 *
 *
 *  SR = 0b00111100 - (n << 2)
 *
 *  (protected_size/total_size) <= 1/(2^n)
 *
 *  protected_size = total_size/(2^n)
 *
 */
static INT32 wb_sr_2_size(INT32 total_size, UINT8 protected_type)
{
    UINT32 size;
    UINT8 val;
    UINT8 mask = SR_TB | SR_BP2 | SR_BP1 | SR_BP0;

    if(0 == protected_type)
        return 0;

    protected_type &= mask;

    val = ((mask - protected_type) >> MB_FIRST_BP_BIT) & 0x07;
    size = total_size / spow2(val);

    return size;
}
static int wb_is_locked(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
    INT32 size = ofs + len;
    UINT8 tmp_sr;
    INT32 lock_size;
    int ret;
    struct m25p *flash = mtd_to_m25p(mtd);

    if(size < 0 || mtd->size <= 0)
        return -1;

    if (wait_till_ready(flash)) {
		return -1;
	}
	
    tmp_sr = read_sr(flash);
    lock_size = wb_sr_2_size(mtd->size, tmp_sr);

    if(lock_size >= size)
        ret = 1;
    else
        ret = 0;
    
    return ret;
}

static int wb_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
    UINT32 size = ofs + len;
    UINT8 sr = 0;
    UINT32 total_size = mtd->size;
	struct m25p *flash = mtd_to_m25p(mtd);

    if(size < 0 || total_size <= 0)
        return -1;

    if (wait_till_ready(flash)) {
		return -1;
	}

    if(size > total_size) 
        size = total_size;

    sr = wb_size_2_sr(total_size, size);
    if(sr < 0){
        printk("[%s] wb_get_protect_type_by_size failed\n", __func__);
        return -1;
    }

    /* We assume user should know the area actually.
       If area is wrong, stay dead-lock. */
    if(ofs != 0 || size != wb_sr_2_size(total_size, sr))
        sflash_dead_lock();

	write_enable(flash);
	if (write_sr(flash, sr) < 0) {
        printk("[%s] write_sr failed...\n", __func__);
		sflash_dead_lock();
	}

    return 0;
}
static int wb_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
    UINT32 size = ofs + len;
    UINT8 sr = 0;
    struct m25p *flash = mtd_to_m25p(mtd);

    if(size < 0 || mtd->size <= 0)
        return -1;

    if (wait_till_ready(flash)) {
		return -1;
	}

    sr = wb_size_2_sr(mtd->size, ofs);
    if(sr < 0)
        return -1;

    sr = wb_sr_2_last_sr(sr);

	write_enable(flash);
	if (write_sr(flash, sr) < 0) {
        printk("[%s] write_sr failed...\n", __func__);
		sflash_dead_lock();
	}

    return 0;
}

/*
 * Sample protect area table for 128MB flash (MXIC mx25l128):
 * All the combination with TB=1, one blocks size is 64k Bytes
 *
 *   BP3 | BP2 | BP1 | BP0 | Prot Length
 *  -------------------------------------
 *    0  |  0  |  0  |  0  | NONE
 *    0  |  0  |  0  |  1  | 0th block
 *    0  |  0  |  1  |  0  | 0-1st block
 *    0  |  0  |  1  |  1  | 0-3rd block
 *    0  |  1  |  0  |  0  | 0-7th block
 *    0  |  1  |  0  |  1  | 0-15th block
 *    0  |  1  |  1  |  0  | 0-31st block
 *    0  |  1  |  1  |  1  | 0-63rd block
 *    1  |  0  |  0  |  0  | 0-127th block
 *    1  |  0  |  0  |  1  | 0-256th block (all)
 *    1  |  0  |  1  |  0  | 0-256th block (all)
 *    1  |  0  |  1  |  1  | 0-256th block (all)
 *    1  |  1  |  0  |  0  | 0-256th block (all)
 *    1  |  1  |  0  |  1  | 0-256th block (all)
 *    1  |  1  |  1  |  0  | 0-256th block (all)
 *    1  |  1  |  1  |  1  | 0-256th block (all)
 *
 * Status Register:
 * |  S7  |  S6  |  S5  |  S4  |  S3  |  S2  |  S1  |  S0  |
 * | SRWD |  QE  |  BP3 |  BP2 |  BP1 |  BP0 |  WEL |  WIP |
 *
 * Configuration Register:
 * |  C7  |  C6  |  C5  |  C4  |  C3  |  C2  |  C1  |  C0  |
 * |  DC1 |  DC0 |  R   |  R   |  TB  | ODS2 | ODS1 | ODS0 |
 */

/*
 *  According to the spec, Status Register could be count by function as following:
 *
 *  2^(n-1) x MX_BLOCK_SIZE = protected_size
 *
 *  n = log2(protected_size) - log2(MX_BLOCK_SIZE) + 1
 *
 *  SR = (n << 2)
 *
 */
static INT32 mx_size_2_sr(INT32 total_size, INT32 protected_size)
{
    int mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;
    INT32 val;
    int pow;

    if(0 == total_size || protected_size < 0)
        return -1;
    if(0 == protected_size)
        return 0x00;

    pow = slog2(protected_size) - slog2(MX_BLOCK_SIZE) + 1;

    if(pow > MX_PROTECT_LEVEL - 1)
        pow = MX_PROTECT_LEVEL - 1;

    val = pow << MX_FIRST_BP_BIT;

    if (val & ~mask)
        return -1;

    return val;
}

/*
 *  According to the spec, Protected size could be count by function as following:
 *
 *  2^(n-1) x MX_BLOCK_SIZE = protected_size
 *
 */
static INT32 mx_sr_2_size(INT32 total_size, INT32 protected_type)
{
    INT32 size = -1;
    INT32 val;
    int mask = SR_BP3 | SR_BP2 | SR_BP1 | SR_BP0;

    if (protected_type & ~mask)
        return -1;

    if(0 == protected_type)
        return 0;

    val = (mask & protected_type) >> MX_FIRST_BP_BIT;

    size = spow2(val-1)*MX_BLOCK_SIZE;

    return size;
}

static INT32 mx_sr_2_last_sr(INT32 protected_type)
{
    INT32 val;

    if(0 == protected_type)
        return 0;

    val = protected_type - (1 << MX_FIRST_BP_BIT);

    return val;
}

static int mx_is_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
    INT32 size = ofs + len;
    UINT8 tmp_sr;
    INT32 lock_size;
    int ret;
    struct m25p *flash = mtd_to_m25p(mtd);

    if(size < 0 || mtd->size <= 0)
        return -1;

    if (wait_till_ready(flash)) {
		return -1;
	}

    tmp_sr = read_sr(flash);
    lock_size = mx_sr_2_size(mtd->size, tmp_sr);

    if(lock_size >= size)
        ret = 1;
    else
        ret = 0;

    return ret;
}

static int mx_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
    UINT32 size = ofs + len;
    UINT8 sr = 0;
    UINT32 total_size = mtd->size;
	struct m25p *flash = mtd_to_m25p(mtd);

    if(size < 0 || total_size <= 0)
        return -1;

    if (wait_till_ready(flash)) {
		return -1;
	}

    if(size > total_size) 
        size = total_size;

    sr = mx_size_2_sr(total_size, size);
    if(sr < 0) {
        printk("wb_get_protect_type_by_size failed\n");
        return -1;
    }

    /* We assume user should know the area actually.
       If area is wrong, stay dead-lock. */
    if(ofs != 0 || size != mx_sr_2_size(total_size, sr))
        sflash_dead_lock();

    sr |= (CON_TB << 8);

	write_enable(flash);
	if (write_sr(flash, sr) < 0) {
        printk("[%s] write_sr failed...\n", __func__);
		sflash_dead_lock();
	}

    return 0;
}

static int mx_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len)
{
    UINT32 size = ofs + len;
    UINT8 sr = 0;
    struct m25p *flash = mtd_to_m25p(mtd);

    if(size < 0 || mtd->size <= 0)
        return -1;

    if (wait_till_ready(flash)) {
		return -1;
	}

    sr = mx_size_2_sr(mtd->size, ofs);
    if(sr < 0)
        return -1;

    sr = mx_sr_2_last_sr(sr);

    sr |= (CON_TB << 8);

	write_enable(flash);
	if (write_sr(flash, sr) < 0) {
        printk("[%s] write_sr failed...\n", __func__);
		sflash_dead_lock();
	}

    return 0;
}
/****************************************************************************/

/*
 * SPI device driver setup and teardown
 */

struct flash_info {
	/* JEDEC id zero means "no ID" (most older chips); otherwise it has
	 * a high byte of zero plus three data bytes: the manufacturer id,
	 * then a two byte device id.
	 */
	u32             jedec_id;
	u16             ext_id;

	/* The size listed here is what works with OPCODE_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned        sector_size;
	u16             n_sectors;

	u16             page_size;
	u16             addr_width;

	u16             flags;
#define SECT_4K         0x01            /* OPCODE_BE_4K works uniformly */
#define M25P_NO_ERASE   0x02            /* No erase command needed */
#define SST_WRITE       0x04            /* use SST byte programming */
#define M25P_NO_FR      0x08            /* Can't do fastread */
#define SECT_4K_PMC     0x10            /* OPCODE_BE_4K_PMC works uniformly */
#define M25P80_DUAL_READ        0x20    /* Flash supports Dual Read */
#define M25P80_QUAD_READ        0x40    /* Flash supports Quad Read */
};

#define INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)      \
	((kernel_ulong_t)&(struct flash_info) {                         \
		.jedec_id = (_jedec_id),                                \
		.ext_id = (_ext_id),                                    \
		.sector_size = (_sector_size),                          \
		.n_sectors = (_n_sectors),                              \
		.page_size = 256,                                       \
		.flags = (_flags),                                      \
	})

#define CAT25_INFO(_sector_size, _n_sectors, _page_size, _addr_width, _flags) \
	((kernel_ulong_t)&(struct flash_info) {                         \
		.sector_size = (_sector_size),                          \
		.n_sectors = (_n_sectors),                              \
		.page_size = (_page_size),                              \
		.addr_width = (_addr_width),                            \
		.flags = (_flags),                                      \
	})

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static const struct spi_device_id m25p_ids[] = {
	/* Atmel -- some are (confusingly) marketed as "DataFlash" */
	{ "at25fs010",  INFO(0x1f6601, 0, 32 * 1024,   4, SECT_4K) },
	{ "at25fs040",  INFO(0x1f6604, 0, 64 * 1024,   8, SECT_4K) },

	{ "at25df041a", INFO(0x1f4401, 0, 64 * 1024,   8, SECT_4K) },
	{ "at25df321a", INFO(0x1f4701, 0, 64 * 1024,  64, SECT_4K) },
	{ "at25df641",  INFO(0x1f4800, 0, 64 * 1024, 128, SECT_4K) },

	{ "at26f004",   INFO(0x1f0400, 0, 64 * 1024,  8, SECT_4K) },
	{ "at26df081a", INFO(0x1f4501, 0, 64 * 1024, 16, SECT_4K) },
	{ "at26df161a", INFO(0x1f4601, 0, 64 * 1024, 32, SECT_4K) },
	{ "at26df321",  INFO(0x1f4700, 0, 64 * 1024, 64, SECT_4K) },

	{ "at45db081d", INFO(0x1f2500, 0, 64 * 1024, 16, SECT_4K) },

	/* EON -- en25xxx */
	{ "en25f32",    INFO(0x1c3116, 0, 64 * 1024,   64, SECT_4K) },
	{ "en25p32",    INFO(0x1c2016, 0, 64 * 1024,   64, 0) },
	{ "en25q32b",   INFO(0x1c3016, 0, 64 * 1024,   64, 0) },
	{ "en25p64",    INFO(0x1c2017, 0, 64 * 1024,  128, 0) },
	{ "en25q64",    INFO(0x1c3017, 0, 64 * 1024,  128, SECT_4K) },
	{ "en25qh256",  INFO(0x1c7019, 0, 64 * 1024,  512, 0) },
	{ "en25qa32b",  INFO(0x1c6016, 0, 64 * 1024,   64, 0) },

	/* ESMT */
	{ "f25l32pa", INFO(0x8c2016, 0, 64 * 1024, 64, SECT_4K) },

	/* Everspin */
	{ "mr25h256", CAT25_INFO(32 * 1024, 1, 256, 2,
						M25P_NO_ERASE | M25P_NO_FR) },
	{ "mr25h10",  CAT25_INFO(128 * 1024, 1, 256, 3,
						M25P_NO_ERASE | M25P_NO_FR) },

	/* GigaDevice */
	{ "gd25q32", INFO(0xc84016, 0, 64 * 1024,  64, SECT_4K) },
	{ "gd25q64", INFO(0xc84017, 0, 64 * 1024, 128, SECT_4K) },
	{ "gd25q128", INFO(0xc84018, 0, 64 * 1024, 256, SECT_4K) },

	/* Intel/Numonyx -- xxxs33b */
	{ "160s33b",  INFO(0x898911, 0, 64 * 1024,  32, 0) },
	{ "320s33b",  INFO(0x898912, 0, 64 * 1024,  64, 0) },
	{ "640s33b",  INFO(0x898913, 0, 64 * 1024, 128, 0) },

	/* Macronix */
	{ "mx25l2005a",  INFO(0xc22012, 0, 64 * 1024,   4, SECT_4K) },
	{ "mx25l4005a",  INFO(0xc22013, 0, 64 * 1024,   8, SECT_4K) },
	{ "mx25l8005",   INFO(0xc22014, 0, 64 * 1024,  16, 0) },
	{ "mx25l1606e",  INFO(0xc22015, 0, 64 * 1024,  32, SECT_4K) },
	{ "mx25l3205d",  INFO(0xc22016, 0, 64 * 1024,  64, 0) },
	{ "mx25l3255e",  INFO(0xc29e16, 0, 64 * 1024,  64, SECT_4K) },
	{ "mx25l6405d",  INFO(0xc22017, 0, 64 * 1024, 128, 0) },
	{ "mx25l12805d", INFO(0xc22018, 0, 64 * 1024, 256, 0) },
	{ "mx25l12855e", INFO(0xc22618, 0, 64 * 1024, 256, 0) },
	{ "mx25l6455e",  INFO(0xc22617, 0, 64 * 1024, 128, 0) },
	{ "mx25l25635e", INFO(0xc22019, 0, 64 * 1024, 512, 0) },
	{ "mx25l25655e", INFO(0xc22619, 0, 64 * 1024, 512, 0) },
	{ "mx66l51235l", INFO(0xc2201a, 0, 64 * 1024, 1024,
							M25P80_QUAD_READ) },
	{ "mx66l1g55g",  INFO(0xc2261b, 0, 64 * 1024, 2048,
							M25P80_QUAD_READ) },

	/* Micron */
	{ "n25q064",     INFO(0x20ba17, 0, 64 * 1024,  128, 0) },
	{ "n25q128a11",  INFO(0x20bb18, 0, 64 * 1024,  256, 0) },
	{ "n25q128a13",  INFO(0x20ba18, 0, 64 * 1024,  256, 0) },
	{ "n25q256a",    INFO(0x20ba19, 0, 64 * 1024,  512, SECT_4K) },
	{ "n25q512a",    INFO(0x20bb20, 0, 64 * 1024, 1024, SECT_4K) },

	/* PMC */
	{ "pm25lv512",   INFO(0,        0, 32 * 1024,    2, SECT_4K_PMC) },
	{ "pm25lv010",   INFO(0,        0, 32 * 1024,    4, SECT_4K_PMC) },
	{ "pm25lq032",   INFO(0x7f9d46, 0, 64 * 1024,   64, SECT_4K) },

	/* Spansion -- single (large) sector size only, at least
	 * for the chips listed here (without boot sectors).
	 */
	{ "s25sl032p",  INFO(0x010215, 0x4d00,  64 * 1024,  64, 0) },
	{ "s25sl064p",  INFO(0x010216, 0x4d00,  64 * 1024, 128, 0) },
	{ "s25fl256s0", INFO(0x010219, 0x4d00, 256 * 1024, 128,
					M25P80_DUAL_READ | M25P80_QUAD_READ) },
	{ "s25fl256s1", INFO(0x010219, 0x4d01,  64 * 1024, 512,
					M25P80_DUAL_READ | M25P80_QUAD_READ) },
	{ "s25fl512s",  INFO(0x010220, 0x4d00, 256 * 1024, 256,
					M25P80_DUAL_READ | M25P80_QUAD_READ) },
	{ "s70fl01gs",  INFO(0x010221, 0x4d00, 256 * 1024, 256, 0) },
	{ "s25sl12800", INFO(0x012018, 0x0300, 256 * 1024,  64, 0) },
	{ "s25sl12801", INFO(0x012018, 0x0301,  64 * 1024, 256, 0) },
	{ "s25fl129p0", INFO(0x012018, 0x4d00, 256 * 1024,  64, 0) },
	{ "s25fl129p1", INFO(0x012018, 0x4d01,  64 * 1024, 256, 0) },
	{ "s25sl004a",  INFO(0x010212,      0,  64 * 1024,   8, 0) },
	{ "s25sl008a",  INFO(0x010213,      0,  64 * 1024,  16, 0) },
	{ "s25sl016a",  INFO(0x010214,      0,  64 * 1024,  32, 0) },
	{ "s25sl032a",  INFO(0x010215,      0,  64 * 1024,  64, 0) },
	{ "s25sl064a",  INFO(0x010216,      0,  64 * 1024, 128, 0) },
	{ "s25fl132k",  INFO(0x014016,      0,  64 * 1024,  64, 0) },
	{ "s25fl164k",  INFO(0x014017, 0x0140,  64 * 1024, 128, 0) },
	{ "s25fl008k",  INFO(0xef4014,      0,  64 * 1024,  16, SECT_4K) },
	{ "s25fl016k",  INFO(0xef4015,      0,  64 * 1024,  32, SECT_4K) },
	{ "s25fl064k",  INFO(0xef4017,      0,  64 * 1024, 128, SECT_4K) },
	{ "s25fl128l",  INFO(0x016018,      0,  64 * 1024, 256, 0) },
	{ "s25fl256l",  INFO(0x016019,      0,  64 * 1024, 512, 0) },

	/* SST -- large erase sizes are "overlays", "sectors" are 4K */
	{ "sst25vf040b", INFO(0xbf258d, 0, 64 * 1024,  8,
							SECT_4K | SST_WRITE) },
	{ "sst25vf080b", INFO(0xbf258e, 0, 64 * 1024, 16,
							SECT_4K | SST_WRITE) },
	{ "sst25vf016b", INFO(0xbf2541, 0, 64 * 1024, 32,
							SECT_4K | SST_WRITE) },
	{ "sst25vf032b", INFO(0xbf254a, 0, 64 * 1024, 64,
							SECT_4K | SST_WRITE) },
	{ "sst25vf064c", INFO(0xbf254b, 0, 64 * 1024, 128, SECT_4K) },
	{ "sst25wf512",  INFO(0xbf2501, 0, 64 * 1024,  1,
							SECT_4K | SST_WRITE) },
	{ "sst25wf010",  INFO(0xbf2502, 0, 64 * 1024,  2,
							SECT_4K | SST_WRITE) },
	{ "sst25wf020",  INFO(0xbf2503, 0, 64 * 1024,  4,
							SECT_4K | SST_WRITE) },
	{ "sst25wf040",  INFO(0xbf2504, 0, 64 * 1024,  8,
							SECT_4K | SST_WRITE) },

	/* ST Microelectronics -- newer production may have feature updates */
	{ "m25p05",  INFO(0x202010,  0,  32 * 1024,   2, 0) },
	{ "m25p10",  INFO(0x202011,  0,  32 * 1024,   4, 0) },
	{ "m25p20",  INFO(0x202012,  0,  64 * 1024,   4, 0) },
	{ "m25p40",  INFO(0x202013,  0,  64 * 1024,   8, 0) },
	{ "m25p80",  INFO(0x202014,  0,  64 * 1024,  16, 0) },
	{ "m25p16",  INFO(0x202015,  0,  64 * 1024,  32, 0) },
	{ "m25p32",  INFO(0x202016,  0,  64 * 1024,  64, 0) },
	{ "m25p64",  INFO(0x202017,  0,  64 * 1024, 128, 0) },
	{ "m25p128", INFO(0x202018,  0, 256 * 1024,  64, 0) },
	 { "n25q032", INFO(0x20ba16,  0,  64 * 1024,  64, 0) },

	 { "m25p05-nonjedec",  INFO(0, 0,  32 * 1024,   2, 0) },
	 { "m25p10-nonjedec",  INFO(0, 0,  32 * 1024,   4, 0) },
	 { "m25p20-nonjedec",  INFO(0, 0,  64 * 1024,   4, 0) },
	 { "m25p40-nonjedec",  INFO(0, 0,  64 * 1024,   8, 0) },
	 { "m25p80-nonjedec",  INFO(0, 0,  64 * 1024,  16, 0) },
	 { "m25p16-nonjedec",  INFO(0, 0,  64 * 1024,  32, 0) },
	 { "m25p32-nonjedec",  INFO(0, 0,  64 * 1024,  64, 0) },
	 { "m25p64-nonjedec",  INFO(0, 0,  64 * 1024, 128, 0) },
	 { "m25p128-nonjedec", INFO(0, 0, 256 * 1024,  64, 0) },

	 { "m45pe10", INFO(0x204011,  0, 64 * 1024,    2, 0) },
	 { "m45pe80", INFO(0x204014,  0, 64 * 1024,   16, 0) },
	 { "m45pe16", INFO(0x204015,  0, 64 * 1024,   32, 0) },

	 { "m25pe20", INFO(0x208012,  0, 64 * 1024,  4,       0) },
	 { "m25pe80", INFO(0x208014,  0, 64 * 1024, 16,       0) },
	 { "m25pe16", INFO(0x208015,  0, 64 * 1024, 32, SECT_4K) },

	 { "m25px16",    INFO(0x207115,  0, 64 * 1024, 32, SECT_4K) },
	 { "m25px32",    INFO(0x207116,  0, 64 * 1024, 64, SECT_4K) },
	 { "m25px32-s0", INFO(0x207316,  0, 64 * 1024, 64, SECT_4K) },
	 { "m25px32-s1", INFO(0x206316,  0, 64 * 1024, 64, SECT_4K) },
	 { "m25px64",    INFO(0x207117,  0, 64 * 1024, 128, 0) },

	 /* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
	 { "w25x10", INFO(0xef3011, 0, 64 * 1024,  2,  SECT_4K) },
	 { "w25x20", INFO(0xef3012, 0, 64 * 1024,  4,  SECT_4K) },
	 { "w25x40", INFO(0xef3013, 0, 64 * 1024,  8,  SECT_4K) },
	 { "w25x80", INFO(0xef3014, 0, 64 * 1024,  16, SECT_4K) },
	 { "w25x16", INFO(0xef3015, 0, 64 * 1024,  32, SECT_4K) },
	 { "w25x32", INFO(0xef3016, 0, 64 * 1024,  64, SECT_4K) },
	 { "w25q32", INFO(0xef4016, 0, 64 * 1024,  64, SECT_4K) },
	 { "w25q32dw", INFO(0xef6016, 0, 64 * 1024,  64, SECT_4K) },
	 { "w25x64", INFO(0xef3017, 0, 64 * 1024, 128, SECT_4K) },
	 { "w25q64", INFO(0xef4017, 0, 64 * 1024, 128, SECT_4K) },
	 { "w25q128", INFO(0xef4018, 0, 64 * 1024, 256, SECT_4K) },
	 { "w25q80", INFO(0xef5014, 0, 64 * 1024,  16, SECT_4K) },
	 { "w25q80bl", INFO(0xef4014, 0, 64 * 1024,  16, SECT_4K) },
	 { "w25q128", INFO(0xef4018, 0, 64 * 1024, 256, SECT_4K) },
	 { "w25q256", INFO(0xef4019, 0, 64 * 1024, 512, SECT_4K) },

	 /* Catalyst / On Semiconductor -- non-JEDEC */
	 { "cat25c11", CAT25_INFO(16, 8, 16, 1, M25P_NO_ERASE | M25P_NO_FR) },
	 { "cat25c03", CAT25_INFO(32, 8, 16, 2, M25P_NO_ERASE | M25P_NO_FR) },
	 { "cat25c09", CAT25_INFO(128, 8, 32, 2, M25P_NO_ERASE | M25P_NO_FR) },
	 { "cat25c17", CAT25_INFO(256, 8, 32, 2, M25P_NO_ERASE | M25P_NO_FR) },
	 { "cat25128", CAT25_INFO(2048, 8, 64, 2, M25P_NO_ERASE | M25P_NO_FR) },
	 
	 /* BYT -- by25 "blocks" are 64K, "sectors" are 4KiB */
	 { "by25q128as", INFO(0x684018, 0, 64 * 1024,  256,  0) },
	 { },
};
MODULE_DEVICE_TABLE(spi, m25p_ids);

static const struct spi_device_id *jedec_probe(struct spi_device *spi)
{
	int                     tmp;
	u8                      code = OPCODE_RDID;
	u8                      id[5];
	u32                     jedec;
	u16                     ext_jedec;
	struct flash_info       *info;

	/* JEDEC also defines an optional "extended device information"
	* string for after vendor-specific data, after the three bytes
	* we use here.  Supporting some chips might require using it.
	*/
	tmp = spi_write_then_read(spi, &code, 1, id, 5);
	if (tmp < 0) {
		pr_debug("%s: error %d reading JEDEC ID\n",
		dev_name(&spi->dev), tmp);
		return ERR_PTR(tmp);
	}
	jedec = id[0];
	jedec = jedec << 8;
	jedec |= id[1];
	jedec = jedec << 8;
	jedec |= id[2];

	ext_jedec = id[3] << 8 | id[4];

	dev_err(&spi->dev, "JEDEC id %06x\n", jedec);
	for (tmp = 0; tmp < ARRAY_SIZE(m25p_ids) - 1; tmp++) {
		info = (void *)m25p_ids[tmp].driver_data;
		if (info->jedec_id == jedec) {
			if (info->ext_id == 0 || info->ext_id == ext_jedec)
				return &m25p_ids[tmp];
		}
	}
	dev_err(&spi->dev, "unrecognized JEDEC id %06x\n", jedec);
	return ERR_PTR(-ENODEV);
}


 /*
  * board specific setup should have ensured the SPI clock used here
  * matches what the READ command supports, at least until this driver
  * understands FAST_READ (for clocks over 25 MHz).
  */
static int m25p_probe(struct spi_device *spi)
{
	const struct spi_device_id      *id = spi_get_device_id(spi);
	struct flash_platform_data      *data;
	struct m25p                     *flash;
	struct flash_info               *info;
	unsigned                        i;
	struct mtd_part_parser_data     ppdata;
	struct device_node *np = spi->dev.of_node;
	int ret;
#ifdef CONFIG_MTD_OF_PARTS
extern int add_mtd_partitions(struct mtd_info *, const struct mtd_partition *,int);
extern int parse_mtd_partitions(struct mtd_info *master, const char **types,\
				struct mtd_partition **pparts,struct mtd_part_parser_data *data);

		struct mtd_partition *parts = NULL;
		int	nr_parts = 0;
		static const char *part_probes[] = { "ofpart", NULL };
		struct mtd_part_parser_data ali_nor_flash;
#endif
	/* Platform data helps sort out which chip type we have, as
	* well as how this board partitions it.  If we don't have
	* a chip ID, try the JEDEC id commands; they'll work for most
	* newer chips, even if we don't recognize the particular chip.
	*/
	dev_dbg(&spi->dev,"SPI_NOR_VERSION: %s\n", SPI_NOR_VERSION);

	if (id == NULL) {
		dev_err(&flash->spi->dev, "The spi_get_device_id is NULL\n");
		return -EINVAL;
	}

	data = dev_get_platdata(&spi->dev);
	if (data && data->type) {
		const struct spi_device_id *plat_id;

		for (i = 0; i < ARRAY_SIZE(m25p_ids) - 1; i++) {
			plat_id = &m25p_ids[i];
			if (strcmp(data->type, plat_id->name))
				continue;
			break;
		}

		if (i < ARRAY_SIZE(m25p_ids) - 1)
			id = plat_id;
		else
			dev_warn(&spi->dev, "unrecognized id %s\n", data->type);
	}

	info = (void *)id->driver_data;

	if (info->jedec_id) {
		const struct spi_device_id *jid;

		jid = jedec_probe(spi);
		if (IS_ERR(jid)) {
			return PTR_ERR(jid);
		} else if (jid != id) {
			/*
			* JEDEC knows better, so overwrite platform ID. We
			* can't trust partitions any longer, but we'll let
			* mtd apply them anyway, since some partitions may be
			* marked read-only, and we don't want to lose that
			* information, even if it's not 100% accurate.
			*/
			dev_warn(&spi->dev, "found %s, expected %s\n",
							jid->name, id->name);
			id = jid;
			info = (void *)jid->driver_data;
		}
	}

	flash = devm_kzalloc(&spi->dev, sizeof(*flash), GFP_KERNEL);
	if (!flash)
		return -ENOMEM;

	flash->command = devm_kzalloc(&spi->dev, MAX_CMD_SIZE, GFP_KERNEL);
	if (!flash->command)
		return -ENOMEM;

	flash->spi = spi;

	mutex_init(&flash->lock);
	spi_set_drvdata(spi, flash);

	/*
	* Atmel, SST and Intel/Numonyx serial flash tend to power
	* up with the software protection bits set
	*/

	if (JEDEC_MFR(info->jedec_id) == CFI_MFR_ATMEL ||
		JEDEC_MFR(info->jedec_id) == CFI_MFR_INTEL ||
		JEDEC_MFR(info->jedec_id) == CFI_MFR_SST) {
		write_enable(flash);
		write_sr(flash, 0);
	}

	if (data && data->name)
		flash->mtd.name = data->name;
	else
		flash->mtd.name = dev_name(&spi->dev);

	flash->mtd.type = MTD_NORFLASH;
	flash->mtd.writesize = 1;
	flash->mtd.flags = MTD_CAP_NORFLASH;
	flash->mtd.size = info->sector_size * info->n_sectors;
	flash->mtd._erase = m25p80_erase;
	flash->mtd._read = m25p80_read;

	/* flash protection support for STmicro chips */
	if (JEDEC_MFR(info->jedec_id) == CFI_MFR_ST) {
		flash->mtd._lock = m25p80_lock;
		flash->mtd._unlock = m25p80_unlock;
	}else if(JEDEC_MFR(info->jedec_id) == MFR_WINDBOND ||
		JEDEC_MFR(info->jedec_id) == MFR_GIGADEVICE ||
		JEDEC_MFR(info->jedec_id) == MFR_SPANSION) {
	    flash->mtd._lock = wb_lock;
        flash->mtd._unlock = wb_unlock;
        flash->mtd._is_locked = wb_is_locked;
    }else if(JEDEC_MFR(info->jedec_id) == MFR_MXIC){
	    flash->mtd._lock = mx_lock;
        flash->mtd._unlock = mx_unlock;
        flash->mtd._is_locked = mx_is_lock;
    }
    
	/* sst flash chips use AAI word program */
	if (info->flags & SST_WRITE)
		flash->mtd._write = sst_write;
	else
		flash->mtd._write = m25p80_write;

	/* prefer "small sector" erase if possible */
	if (info->flags & SECT_4K) {
		flash->erase_opcode = OPCODE_BE_4K;
		flash->mtd.erasesize = 4096;
	} else if (info->flags & SECT_4K_PMC) {
		flash->erase_opcode = OPCODE_BE_4K_PMC;
		flash->mtd.erasesize = 4096;
	} else {
		flash->erase_opcode = OPCODE_SE;
		flash->mtd.erasesize = info->sector_size;
	}

	if (info->flags & M25P_NO_ERASE)
		flash->mtd.flags |= MTD_NO_ERASE;

	ppdata.of_node = spi->dev.of_node;
	flash->mtd.dev.parent = &spi->dev;
	flash->page_size = info->page_size;
	flash->mtd.writebufsize = flash->page_size;

	if (np) {
		/* If we were instantiated by DT, use it */
		if (of_property_read_bool(np, "m25p,fast-read"))
			flash->flash_read = M25P80_FAST;
		else
			flash->flash_read = M25P80_NORMAL;
		} else {
			/* If we weren't instantiated by DT,
			default to fast-read */
			flash->flash_read = M25P80_FAST;
	}

	/* Some devices cannot do fast-read, no matter what DT tells us */
	if (info->flags & M25P_NO_FR)
		flash->flash_read = M25P80_NORMAL;

	/* Quad/Dual-read mode takes precedence over fast/normal */
	if (spi->mode & SPI_RX_QUAD && info->flags & M25P80_QUAD_READ) {
		ret = set_quad_mode(flash, info->jedec_id);
		if (ret) {
			dev_err(&flash->spi->dev, "quad mode not supported\n");
			return ret;
		}
		flash->flash_read = M25P80_QUAD;
	} else if (spi->mode & SPI_RX_DUAL && info->flags & M25P80_DUAL_READ) {
		flash->flash_read = M25P80_DUAL;
	}

	/* Default commands */
	switch (flash->flash_read) {
	case M25P80_QUAD:
		flash->read_opcode = OPCODE_QUAD_READ;
		break;
	case M25P80_DUAL:
		flash->read_opcode = OPCODE_DUAL_READ;
		break;
	case M25P80_FAST:
		flash->read_opcode = OPCODE_FAST_READ;
		break;
	case M25P80_NORMAL:
		flash->read_opcode = OPCODE_NORM_READ;
		break;
	default:
		dev_err(&flash->spi->dev, "No Read opcode defined\n");
		return -EINVAL;
	}

	flash->program_opcode = OPCODE_PP;

	if (info->addr_width)
		flash->addr_width = info->addr_width;
	else if (flash->mtd.size > 0x1000000) {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		flash->addr_width = 4;
		if (JEDEC_MFR(info->jedec_id) == CFI_MFR_AMD) {
			/* Dedicated 4-byte command set */
			switch (flash->flash_read) {
			case M25P80_QUAD:
				flash->read_opcode = OPCODE_QUAD_READ_4B;
				break;
			case M25P80_DUAL:
				flash->read_opcode = OPCODE_DUAL_READ_4B;
				break;
			case M25P80_FAST:
				flash->read_opcode = OPCODE_FAST_READ_4B;
				break;
			case M25P80_NORMAL:
				flash->read_opcode = OPCODE_NORM_READ_4B;
				break;
			}
			flash->program_opcode = OPCODE_PP_4B;
			/* No small sector erase for 4-byte command set */
			flash->erase_opcode = OPCODE_SE_4B;
			flash->mtd.erasesize = info->sector_size;
		} else
			set_4byte(flash, info->jedec_id, 1);
	} else {
		flash->addr_width = 3;
	}

	dev_info(&spi->dev, "%s (%lld Kbytes)\n", id->name,
	(long long)flash->mtd.size >> 10);

	pr_debug("mtd .name = %s, .size = 0x%llx (%lldMiB) "
			".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
	flash->mtd.name,
	(long long)flash->mtd.size, (long long)(flash->mtd.size >> 20),
	flash->mtd.erasesize, flash->mtd.erasesize / 1024,
	flash->mtd.numeraseregions);

	if (flash->mtd.numeraseregions)
		for (i = 0; i < flash->mtd.numeraseregions; i++)
			pr_debug("mtd.eraseregions[%d] = { .offset = 0x%llx, "
			".erasesize = 0x%.8x (%uKiB), "
			".numblocks = %d }\n",
			i, (long long)flash->mtd.eraseregions[i].offset,
			flash->mtd.eraseregions[i].erasesize,
			flash->mtd.eraseregions[i].erasesize / 1024,
			flash->mtd.eraseregions[i].numblocks);


	/* partitions should match sector boundaries; and it may be good to
	* use readonly partitions for writeprotected sectors (BP2..BP0).
	*/

#ifdef CONFIG_MTD_OF_PARTS
		pr_debug("========== ali_nor: parse DTS! ==========\n");
		memset(&ali_nor_flash, 0 ,sizeof(struct mtd_part_parser_data));
		ali_nor_flash.of_node = of_find_node_by_path("/NOR_flash@0");
		nr_parts = parse_mtd_partitions(&flash->mtd, part_probes, &parts, &ali_nor_flash);
		pr_debug("========== ali_nor: host->nr_parts: %d ==========\n", nr_parts);

		if (nr_parts > 0) 
			return add_mtd_partitions(&flash->mtd, parts, nr_parts);
		else
			return mtd_device_parse_register(&flash->mtd, NULL, &ppdata,
			data ? data->parts : NULL,
			data ? data->nr_parts : 0);
#else
	return mtd_device_parse_register(&flash->mtd, NULL, &ppdata,
	data ? data->parts : NULL,
	data ? data->nr_parts : 0);
#endif
}


static int m25p_remove(struct spi_device *spi)
{
	struct m25p     *flash = spi_get_drvdata(spi);

	/* Clean up MTD stuff. */
	return mtd_device_unregister(&flash->mtd);
}

/* add for device tree */
static const struct of_device_id ali_spi_nor_of_match[] = {
{ .compatible = "m25p80", },
	 {},
};

MODULE_DEVICE_TABLE(of, ali_spi_nor_of_match);

static struct spi_driver m25p80_driver = {
	.driver = {
	.name   = "m25p80",
	.owner  = THIS_MODULE,
	.of_match_table = of_match_ptr(ali_spi_nor_of_match),
	},
	.id_table       = m25p_ids,
	.probe  = m25p_probe,
	.remove = m25p_remove,

	/* REVISIT: many of these chips have deep power-down modes, which
	* should clearly be entered on suspend() to minimize power use.
	* And also when they're otherwise idle...
	*/
};

module_spi_driver(m25p80_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Barry Chang");
MODULE_DESCRIPTION("MTD SPI driver for ST M25Pxx flash chips");
MODULE_VERSION(SPI_NOR_VERSION);

