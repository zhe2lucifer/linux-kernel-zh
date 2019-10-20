/*
 *      Alitech CI Driver
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */
#ifndef __ALI_CIC_H__
#define __ALI_CIC_H__

#include <linux/pci.h>
#include <asm/mach-ali/typedef.h>

/* For FPGA PCI testing board */
/*#define M3602CIC_VID			0x8866*/
/*#define M3602CIC_DID			0x0101*/

#define RB_TSCR_TSEN			0x80
#define RB_TSCR_ORDER			0x08
#define RB_TSCR_SWAP			0x01
#define RB_TSCR_CIBYPASS		0x80

#define RB_TSCR_QPSK			0
#define RB_TSCR_SPI				1
#define RB_TSCR_SSI1			2
#define RB_TSCR_SSI2			3

/* cic register define */
enum CIC_REG_ADDRESS {
	R_TSCR0 = 0x0000,	/* TS Control Register 0 */
	R_TSCR1 = 0x0008,	/* TS Control Register 1 */
	R_CISEL = 0x000c,	/*for CI sel, byte opration mode.*/
	R_CRVSR = 0x0100,	/* Chip Revision Register */
	R_IFXSR	= 0x0101,	/* Interface Status Register */
	R_PWRCR = 0x0102,	/* Power Control Register */
	R_IGCR = 0x0103,/* Interrupt and General Control Register */
	R_CSCR = 0x0104,	/* Card Status Change Register */
	R_MICR = 0x0105,	/* Management Interrupt Config Register */
	R_MER = 0x0106,	/* Mapping Enable Register */
	R_IOWR = 0x0107,	/* IO Windows Control Register */
	R_IOMSAR0 = 0x0108,	/* IO Map 0 Start Address Register */
	R_IOMEAR0 = 0x010a,	/* IO Map 0 End Address Register */
	R_IOMSAR1 = 0x010c,	/* IO Map 1 Start Address Register */
	R_IOMEAR1 = 0x010e,	/* IO Map 1 End Address Register */
	R_MMSAR0 = 0x0110,	/* Memory Map 0 Start Address Register */
	R_MMEAR0 = 0x0112,	/* Memory Map 0 End Address Register */
	R_MMOAR0 = 0x0114,	/* Memory Map 0 Offset Address Register */
	R_MISCCR = 0x0116,	/* Misc Control Register */
	R_MMSAR1 = 0x0118,	/* Memory Map 1 Start Address Register */
	R_MMEAR1 = 0x011a,	/* Memory Map 1 End Address Register */
	R_MMOAR1 = 0x011c,	/* Memory Map 1 Offset Address Register */
	R_MMSAR2 = 0x0120,	/* Memory Map 2 Start Address Register */
	R_MMEAR2 = 0x0122,	/* Memory Map 2 End Address Register */
	R_MMOAR2 = 0x0124,	/* Memory Map 2 Offset Address Register */
	R_MMSAR3 = 0x0128,	/* Memory Map 3 Start Address Register */
	R_MMEAR3 = 0x012a,	/* Memory Map 3 End Address Register */
	R_MMOAR3 = 0x012c,	/* Memory Map 3 Offset Address Register */
	R_EIR = 0x012e,		/* Extend Index Register */
	R_EDR = 0x012f,		/* Extend Data Register */
	R_MMSAR4 = 0x0130,	/* Memory Map 4 Start Address Register */
	R_MMEAR4 = 0x0132,	/* Memory Map 4 End Address Register */
	R_MMOAR4 = 0x0134,	/* Memory Map 4 Offset Address Register */
	R_IOMOAR0 = 0x0136,	/* IO Map 0 Offset Address Register */
	R_IOMOAR1 = 0x0138,	/* IO Map 1 Offset Address Register */
	R_STM0 = 0x013a,	/* Setup Timing 0 */
	R_CTM0 = 0x013b,	/* Command Timing 0 */
	R_RTM0 = 0x013c,	/* Recovery Timing 0 */
	R_STM1 = 0x013d,	/* Setup Timing 1 */
	R_CTM1 = 0x013e,	/* Command Timing 1 */
	R_RTM1 = 0x013f,	/* Recovery Timing 1 */

	R_IOBASE = 0x0200,	/* Card IO Space Base Address */
	R_MBASE = 0x1000,	/* Card Memory Space Base Address */

	RE_EXTCR1 = 0x03,	/* Extension control */
	RE_SYSMMR = 0x05,	/* System memory map upper address base */
	RE_CVSR = 0x0a		/* Card voltage sense */
};

enum CIC_MEM_CONFIG {
	ACCESS_ATTRIBUTE_MEM,
	ACCESS_COMMON_MEM
};

/* Struncture for M3602 CIC private structure */
struct ali_cic_private {
	void __iomem *base_addr;

	/* CAM command interface registers */
	void __iomem *reg_data;	/* CI data register */
	void __iomem *reg_cs;	/* CI control/status register */
	void __iomem *reg_szl;	/* CI size low byte */
	void __iomem *reg_szh;	/* CI size high byte */
	/*void (*callback)(UINT32);*/
	int slot_irq_num;
	int status;
};

struct ali_cic_device {
	dev_t id;
	unsigned long port;
	char name[8];
	struct cdev cdev;
	struct device *dev;
	struct ali_cic_private priv;
	struct mutex lock;
	int in_use;
};

static inline void OUTPUT_UINT32(void __iomem *addr, unsigned long val)
{
	iowrite32(val, addr);
}

static inline void OUTPUT_UINT16(void __iomem *addr, unsigned long val)
{
	iowrite16(val, addr);
}

static inline void OUTPUT_UINT8(void __iomem *addr, unsigned long val)
{
	iowrite8(val, addr);
}

static inline unsigned long INPUT_UINT32(void __iomem *addr)
{
	return ioread32(addr);
}

static inline unsigned short INPUT_UINT16(void __iomem *addr)
{
	return ioread16(addr);
}

static inline unsigned char INPUT_UINT8(void __iomem *addr)
{
	return ioread8(addr);
}

#endif /*__ALI_CIC_H__*/
