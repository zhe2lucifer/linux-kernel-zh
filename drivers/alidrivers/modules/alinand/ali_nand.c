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

#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <asm/io.h>
#include <linux/version.h>
#if defined(CONFIG_ALI_CHIP_M3921)
#include <mach/ali-s3921.h>
#endif
#include <ali_reg.h>
#include <ali_board_config.h>
#include "ali_nand.h"


#if defined(CONFIG_SUPPORT_NAND_NOR)
extern struct mutex ali_sto_mutex;
#endif

#if   (!defined(CONFIG_ALI_CHIP_M3921))
DEFINE_MUTEX(nor_nand_mutex);
EXPORT_SYMBOL(nor_nand_mutex);
#endif

/* wait HW DMA completion */
struct completion dma_completion;
u8 micron_l73 = 0;
u8 micron_l74 = 0;
u8 micron_l83 = 0;
u8 micron_l84 = 0;
u8 micron_48 = 0; /* for "MT29F16G08CBACA", MT29F32G08CFACA, MT29F16G08CBACB, MT29F32G08CFACB */
u8 force_24_ecc = 0;

static const char nandname[] = "alidev_nand_reg";
static u8 bbt_pattern[] = {'B', 'b', 't', '0' };
static u8 mirror_pattern[] = {'1', 't', 'b', 'B' };
static u8 scan_ff_pattern[] = { 0xff, 0xff };

static struct nand_ecclayout ali_nand_oob_32 = {
	.eccbytes = 28,
	.eccpos = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
			   19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
	.oobfree = {
		{.offset = 0,
		 .length = 4}}
};

static struct nand_bbt_descr largepage_flashbased = {
	.options = NAND_BBT_SCAN2NDPAGE,
	.offs = 0,
	.len = 2,
	.pattern = scan_ff_pattern
};

static struct nand_bbt_descr ali_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 16,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr ali_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 16,
	.pattern = mirror_pattern
};

static void nf_cmd(struct mtd_info *mtd, unsigned int command, int column, int page_addr);

#if ((defined CONFIG_MTD_CMDLINE_PARTS) || (defined CONFIG_MTD_OF_PARTS))
	extern int parse_mtd_partitions(struct mtd_info *master, const char **types, struct mtd_partition **pparts, struct mtd_part_parser_data *data);
#endif

struct mtd_partition ali_nand_partitions[] = {
	{ .name = "ALI-Private    ",    .offset = 0,    .size = 0,    .mask_flags = MTD_WRITEABLE,},/* force read-only */     
	{ .name = "Partition1     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  }, /*partition name max length: 15bytes*/
	{ .name = "Partition2     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition3     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition4     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition5     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition6     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition7     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition8     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition9     ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition10    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition11    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition12    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition13    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition14    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition15    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition16    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition17    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition18    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition19    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition20    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition21    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition22    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition23    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition24    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition25    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition26    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition27    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition28    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition29    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
	{ .name = "Partition30    ",    .offset = 0,    .size = 0,    .mask_flags = 0,  },
};

static inline struct ali_nand_host *to_ali_nand_host(struct mtd_info *mtd)
{
	return container_of((void *) mtd, struct ali_nand_host, mtd);
}

static u8 nfreg_read8(struct mtd_info *mtd, u32 reg)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	return readb(host->regs + reg);
}

static void nfreg_write8(struct mtd_info *mtd, u8 val, u32 reg)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	writeb(val, host->regs + reg);
}

static u32 nfreg_read32(struct mtd_info *mtd, u32 reg)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	return readl(host->regs + reg);
}

static void nfreg_write32(struct mtd_info *mtd, u32 val, u32 reg)
{
struct ali_nand_host *host = to_ali_nand_host(mtd);
	writel(val, host->regs + reg);
}

void ali_nand_reg_dump_all(void)
{
	int i;

	for(i = 0; i < 0x800; i+=4)
	{
		if( (i % 16) == 0 )
			printk("\nreg 0x%08x ", i);
		printk("0x%08x ", (int) ali_soc_read(i));
	}
}


void ali_nand_reg_dump(struct mtd_info *mtd)
{
	int i;

	pr_err( "SOC reg:\n");
	pr_err( " reg000 0x%08x\n", (int) ali_soc_read(0));
	pr_err( " reg054 0x%08x\n", (int) ali_soc_read(0x54));
	pr_err( " reg058 0x%08x\n", (int) ali_soc_read(0x58));
	pr_err( " reg060 0x%08x\n", (int) ali_soc_read(0x60));
	pr_err( " reg070 0x%08x\n", (int) ali_soc_read(0x70));
	pr_err( " reg074 0x%08x\n", (int) ali_soc_read(0x74));
	pr_err( " reg078 0x%08x\n", (int) ali_soc_read(0x78));
	pr_err( " reg07c 0x%08x\n", (int) ali_soc_read(0x7c));
	pr_err( " reg084 0x%08x\n", (int) ali_soc_read(0x84));
	pr_err( " reg088 0x%08x\n", (int) ali_soc_read(0x88));
	pr_err( " reg08c 0x%08x\n", (int) ali_soc_read(0x8c));
	pr_err( " reg090 0x%08x\n", (int) ali_soc_read(0x90));
	pr_err( " reg0a4 0x%08x\n", (int) ali_soc_read(0xa4));
	pr_err( " reg0d4 0x%08x\n", (int) ali_soc_read(0xd4));
	pr_err( " reg0d8 0x%08x\n", (int) ali_soc_read(0xd8));
	pr_err( " reg430 0x%08x\n",(int) ali_soc_read(0x430));
	pr_err( " reg434 0x%08x\n",(int) ali_soc_read(0x434));
	pr_err( " reg490 0x%08x\n",(int) ali_soc_read(0x490));
	pr_err( " reg494 0x%08x\n",(int) ali_soc_read(0x494));
	pr_err( " reg1030 0x%08x\n",(int) ali_soc_read(0x1030));

	pr_err( "NAND reg:");
	for (i=0; i<ALI_NAND_REG_LEN; i++)
	{
		if ((i % 16) == 0)
			printk(" \n");
		if((i>=0x48) && (i<0x4c))
			printk(" 0xXX");
		else if (i == 0x18)
			printk(" 0xXX");
		else
			printk(" 0x%02x", nfreg_read8(mtd, i));
	}
	pr_err("\n nand reg[0x18] = 0x%02x\n", nfreg_read8(mtd, 0x18));
}

static void nf_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (0 == (ctrl & NAND_NCE)) {
		nfreg_write8(mtd, NF_CEJ, NF_bCTRL);
		return;
	}

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_CLE)
			nfreg_write8(mtd, NF_CLE, NF_bCTRL);
		else if (ctrl & NAND_ALE)
			nfreg_write8(mtd, NF_ALE, NF_bCTRL);
		else
			nfreg_write8(mtd, 0, NF_bCTRL);
	}
		
	if (NAND_CMD_NONE != cmd)
		nfreg_write8(mtd,  (u8) cmd, NF_bPIODATA);/* write command to register 0x1c */
}

void ali_nand_pinmux_release(struct mtd_info *mtd, unsigned int callline)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	int reg, data, val;
	int *cur = NULL;
	
	if (host == NULL) {
		pr_err("[ERR] %s, host == NULL\n", __func__);
		return;
	}

	if (host->pinmux_release) {
		cur = host->pinmux_release;
		while(*cur != 0) {
			reg = *cur;	cur++;
			data = *cur;	cur++;
			val = *cur;	cur++;
			if (val)
				ali_soc_write(ali_soc_read(reg) | data, reg);
			else
				ali_soc_write(ali_soc_read(reg) & ~(data), reg);
		}
	}
#if defined(CONFIG_SUPPORT_NAND_NOR)
	mutex_unlock(&ali_sto_mutex);
#endif
}

void ali_nand_pinmux_set(struct mtd_info *mtd, unsigned int callline)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	int reg, data, val;
	int *cur = NULL;
	
	if (host == NULL) {
		pr_err("[ERR] %s, host == NULL\n", __func__);
		return;
	}
#if defined(CONFIG_SUPPORT_NAND_NOR)
	mutex_lock(&ali_sto_mutex);
#endif
	if (host->pinmux_set) {
		cur = host->pinmux_set;
		while(*cur != 0) {
			reg = *cur;	cur++;
			data = *cur;	cur++;
			val = *cur;	cur++;
			if (val)
				ali_soc_write(ali_soc_read(reg) | data, reg);
			else
				ali_soc_write(ali_soc_read(reg) & ~(data), reg);
		}
	}
}

static void ali_nand_select_chip(struct mtd_info *mtd, int chips)
{
	switch (chips) {
	case 0:
		ali_nand_pinmux_set(mtd, __LINE__);
		break;
	case 1:
		break;
	case -1:
	case 2:
	case 3:
	default:
		ali_nand_pinmux_release(mtd, __LINE__);
		break;
	}
}
static void nf_wait_ready(struct mtd_info *mtd)
{
	nf_cmd(mtd, NAND_CMD_STATUS, -1, -1);
	while(1) {
		udelay(1);
		if(nfreg_read8(mtd, NF_bPIODATA) & NAND_STATUS_READY)
			break;
	};
}

static void nf_cmd(struct mtd_info *mtd, unsigned int command, int column, int page_addr)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);       
	unsigned long timen;

	timen = 5000; /* timeo + DELAY_1MS; */

	/* Command latch cycle */
	nf_ctrl(mtd, command, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	if ((column != -1) || (page_addr != -1)) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;
		/* not support 512 page nand */
		if (column != -1) {
			nf_ctrl(mtd, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;
			nf_ctrl(mtd, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			if (192 == host->nf_parm.pages_perblock)
				page_addr = (page_addr / 192) * 256 + (page_addr % 192);
				
			nf_ctrl(mtd, page_addr, ctrl);
			nf_ctrl(mtd, page_addr>>8, ctrl);
			
			if (host->nf_parm.rowaddr_cycle >= 3)
				nf_ctrl(mtd, page_addr>>16, ctrl);

			if (host->nf_parm.rowaddr_cycle == 4)
				nf_ctrl(mtd, page_addr>>24, ctrl);
		}
	}
	nf_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
	/*
	* program and erase have their own busy handlers
	* status, sequential in, and deplete1 need no delay
	*/
	switch (command) {
	case NAND_CMD_STATUS:
		return;
	case NAND_CMD_RESET:
		udelay(1000);
		nf_ctrl(mtd, NAND_CMD_STATUS,
			NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		nf_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		while(timen--) { /* (timen < timeo) (time_before(jiffies, timeo)) */
			if (nfreg_read8(mtd, NF_bPIODATA) & NAND_STATUS_READY)
				break;
			udelay(1000);/* timeo = boot_gettime(); */
		}
		return;
	case NAND_CMD_READ0:
		nf_ctrl(mtd, NAND_CMD_READSTART,
			NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		nf_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		/* nf_ctrl(mtd, mtd, chip); */
		nf_wait_ready(mtd);
		nf_ctrl(mtd, NAND_CMD_READ0, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		nf_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		return;
	}
}


static void nf_host_init(struct mtd_info *mtd)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u8 tmp;

	nfreg_write8(mtd, NF_EN, NF_bMODE);
	nfreg_write8(mtd, 0x00, NF_bDMACTRL);
	nfreg_write32(mtd, 0x00, NF_dwINTFLAG);
	
	if(host->nf_parm.data_scramble) {
		tmp = nfreg_read8(mtd, NF_bEDORBCYC);
		tmp |= NF_CRYPTO_EN | NF_TRB; /*Ben Chen suggested*/
		nfreg_write8(mtd, tmp, NF_bEDORBCYC);
	} else {
		tmp = nfreg_read8(mtd, NF_bEDORBCYC);
		tmp &= (~NF_CRYPTO_EN) | NF_TRB;/*Ben Chen suggested*/
		nfreg_write8(mtd, tmp, NF_bEDORBCYC);
	}

	switch (host->nf_parm.ecctype) {
	case NF_BCH_16B_MODE: /* ECC_16: */
		nfreg_write8(mtd, NF_ECC_EN | NF_ECC_NON_STOP, NF_bECCCTRL);
		nfreg_write8(mtd, NF_FW_RED_4, NF_bDMALEN);
		break;
	case NF_BCH_24B_MODE: /* ECC_24: */
		nfreg_write8(mtd, NF_ECC_EN | NF_BCH_24B_MODE | NF_ECC_NON_STOP, NF_bECCCTRL);
		nfreg_write8(mtd, NF_FW_RED_4, NF_bDMALEN);
		break;
	case NF_BCH_40B_MODE: /* ECC_40: */
		nfreg_write8(mtd, NF_ECC_EN | NF_BCH_40B_MODE | NF_ECC_NON_STOP, NF_bECCCTRL);
		nfreg_write8(mtd, NF_FW_RED_4, NF_bDMALEN);
		break;
	case NF_BCH_48B_MODE: /* ECC_48: */
		nfreg_write8(mtd, NF_ECC_EN | NF_BCH_48B_MODE | NF_ECC_NON_STOP, NF_bECCCTRL);
		nfreg_write8(mtd, NF_FW_RED_4, NF_bDMALEN);
		break;
	case NF_BCH_60B_MODE: /* ECC_60: */
		nfreg_write8(mtd, NF_ECC_EN | NF_BCH_60B_MODE | NF_ECC_NON_STOP, NF_bECCCTRL);
		nfreg_write8(mtd, NF_FW_RED_4, NF_bDMALEN);
		break;
	}
}

static void set_dma_length(struct mtd_info *mtd, u8 sectors)
{
	u8 tmp;

	tmp = nfreg_read8(mtd, NF_bDMALEN);
	tmp &= 0xE0;
	tmp |= sectors;
	nfreg_write8(mtd, tmp, NF_bDMALEN);
}

/* set DMA sectors and dma buffer address */
static void set_dma_addr(struct mtd_info *mtd, u32 addr)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u32 tmp;

	tmp = nfreg_read32(mtd, NF_dwDMACONFIG);
	tmp |= (INIT_DMA_IMB | INIT_DMA_SRAM);
	nfreg_write32(mtd, tmp, NF_dwDMACONFIG);
	tmp &= ~(INIT_DMA_IMB | INIT_DMA_SRAM);
	nfreg_write32(mtd, tmp, NF_dwDMACONFIG);
	if(host->chip_id == C3921)
		nfreg_write32(mtd, addr & ~0x80000000, NF_dwDMAADDR);
	else
		nfreg_write32(mtd, addr & ~0xA0000000, NF_dwDMAADDR);
}

/* set DMA read start*/
static void set_dma_start(struct mtd_info *mtd, u32 to_sram, u8 is_read)
{
	nfreg_write8(mtd, 0, NF_bINTFLAG);
	if (is_read)
		nfreg_write8(mtd, NF_DMA_EN|(to_sram?0:NF_DMA_IMB_EN), NF_bDMACTRL);
	else
		nfreg_write8(mtd, NF_DMA_IMB_EN | NF_DMA_OUT | NF_DMA_EN, NF_bDMACTRL);
}

static void get_oob(struct mtd_info *mtd, u32 *redundant)
{
	redundant[0] = nfreg_read32(mtd, NF_dwREADREDU0);
	redundant[1] = nfreg_read32(mtd, NF_dwREADREDU1);
}

static void fill_oob(struct mtd_info *mtd, u32 *redundant)
{
	nfreg_write32(mtd, redundant[0], NF_dwWRITEREDU0);
	nfreg_write32(mtd, redundant[1], NF_dwWRITEREDU1);
}

/*
 * get ECC status
 */
static int check_ecc_status(struct mtd_info *mtd, int sectors)
{ 
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u32 tmp, sector_mask=0xFF, i, warning_corrected_bits;
	u8 ecc_fail = 0;

	/* cehck page is blank */
	switch(sectors) {
	case 1:
		sector_mask = 0x01;
		break;
	case 2:
		sector_mask = 0x03;
		break;
	case 4:
		sector_mask = 0x0F;
		break;
	case 8:
		sector_mask = 0xFF;
		break;
	}
	
	host->data_all_ff = 0;
	if (((C3701 == host->chip_id) && (host->chip_ver >= 0x01)) ||
		(C3821 == host->chip_id) ||
		(C3921 == host->chip_id))
	{
		/*check all page is uncorrect and blank detect*/
		tmp = nfreg_read32(mtd, NF_wECCCURSEC) & sector_mask;
		if (!tmp)
		{
			if (sector_mask == (nfreg_read32(mtd, NF_dwDETECTBLANK) & sector_mask))
			{
				host->data_all_ff = 1;
				return 0;
			}
		}
	} else {
		if (NF_ALL_FF_FLAG == (nfreg_read8(mtd, NF_bECCCTRL) & NF_ALL_FF_FLAG)) {
			host->data_all_ff = 1;
			return 0;
		}
	}
	
	/*check ecc status*/
	tmp = nfreg_read32(mtd, NF_wECCCURSEC) & sector_mask;

	for (i=0; i<sectors; i++)
	{
		if (!(tmp & 0x01)) {
			/* pr_err("<check_ecc_status> ecc error %x\n", i); */
			mtd->ecc_stats.failed++;
			ecc_fail = 1;
		}	
		tmp = tmp >> 1;
	}
	if(ecc_fail) {
		/* pr_err(KERN_ERR "Nand read ECC error\n"); */
		return -1;
	}


	/*ecc is ok, check ecc corrected count*/
	if (!(nfreg_read32(mtd, NF_wECCCURSEC) & sector_mask))
		return 0;
		
	switch(host->nf_parm.ecctype) {
	case NF_BCH_24B_MODE:
		warning_corrected_bits = 20;
		break;
	case NF_BCH_40B_MODE:
		warning_corrected_bits = 32;
		break;
	case NF_BCH_48B_MODE:
		warning_corrected_bits = 40;
		break;
	case NF_BCH_60B_MODE:
		warning_corrected_bits = 48;
		break;
	case NF_BCH_16B_MODE:
	default:
		warning_corrected_bits = 13;
		break;
	}

	if ((nfreg_read8(mtd, NF_bECCSTS) & 0x3f) > warning_corrected_bits) {
		/* mtd->ecc_stats.corrected += nfreg_read8(mtd, NF_bECCSTS) & 0x3f;	 */
		pr_warn("ECC corrected bits=%d\n", nfreg_read8(mtd, NF_bECCSTS) & 0x3f);
	}
	return 0;
}


static void ali_nand_set_chip_clk(struct mtd_info *mtd)
{
struct ali_nand_host *host = to_ali_nand_host(mtd); 
		
	switch(host->chip_id) {
	case C3921:
		ali_soc_write((ali_soc_read(C3921_SOC_SYSTEM_CLOCK) | SEL_74M), C3921_SOC_SYSTEM_CLOCK);
		break;
	case C3503:
		ali_soc_write((ali_soc_read(C3503_SOC_SYSTEM_CLOCK) & ~SEL_74M_54M), C3503_SOC_SYSTEM_CLOCK);
		break;
	case C3821:
		ali_soc_write((ali_soc_read(C3821_SOC_SYSTEM_CLOCK) & ~SEL_74M_54M), C3821_SOC_SYSTEM_CLOCK);
		break;
	case C3505:
		/* NFlash Clock Select*/
		ali_soc_write((ali_soc_read(C3505_SOC_SYSTEM_CLOCK) & C3505_NF_CLK_118M), C3505_SOC_SYSTEM_CLOCK);
		break;
	default:
		break;
	}

	if (host->nf_parm.read_timing)
		nfreg_write8(mtd, host->nf_parm.read_timing, NF_bREADCYC);
	if (host->nf_parm.write_timing)
		nfreg_write8(mtd, host->nf_parm.write_timing, NF_bWRITECYC);
}

static void ali_nand_soc_reset(struct mtd_info *mtd)
{
	ali_soc_write((ali_soc_read(C3503_SOC_RESET_REG1) | (C3503_NF_RESET)), C3503_SOC_RESET_REG1);
	udelay(100);
	ali_soc_write((ali_soc_read(C3503_SOC_RESET_REG1) & (~C3503_NF_RESET)), C3503_SOC_RESET_REG1);
	udelay(100);
	
	nf_host_init(mtd);
	ali_nand_set_chip_clk(mtd);
}

static void ali_nand_clk_disable(struct mtd_info *mtd)
{
struct ali_nand_host *host = to_ali_nand_host(mtd);
	
	switch(host->chip_id) {
	case C3701:
	case C3503:
	case C3821:
		ali_soc_write((ali_soc_read(0x60) | (1<<2)), 0x60);
		break;

	case C3921:
		ali_soc_write((ali_soc_read(0x60) | (1<<5)), 0x60);
		break;
	default:
		break;
	}
}

static void ali_nand_clk_enable(struct mtd_info *mtd)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	
	switch(host->chip_id) {
	case C3701:
	case C3503:
	case C3821:
		ali_soc_write((ali_soc_read(0x60) & ~(1<<2)), 0x60);
		break;
		
	case C3921:
		ali_soc_write((ali_soc_read(0x60) & ~(1<<5)), 0x60);
		break;
		
	case C3505:
		ali_soc_write((ali_soc_read(C3505_SOC_LOCAL_CLK_CTRL) &
			~(C3505_NF_CLK_GATING)), C3505_SOC_LOCAL_CLK_CTRL);
		break;
		
	default:
		break;
	}
}

/*
 * init DMA
 */
static void init_hw_dma(struct mtd_info *mtd, u8 is_read)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u32 tmp;

	nfreg_write8(mtd, 0, NF_bDMACTRL); 
	tmp = nfreg_read32(mtd, NF_dwDMACONFIG);
	tmp |= (INIT_DMA_IMB | INIT_DMA_SRAM);
	nfreg_write32(mtd, tmp, NF_dwDMACONFIG);
	tmp &= ~(INIT_DMA_IMB | INIT_DMA_SRAM);
	nfreg_write32(mtd, tmp, NF_dwDMACONFIG);

	nfreg_write8(mtd, 0, NF_bECCSTS);
	nfreg_write32(mtd, 0, NF_dwINTFLAG);
	nfreg_write32(mtd, 0xffff, NF_wECCCURSEC);
	nfreg_write8(mtd, NF_ECC_EN | NF_ECC_NON_STOP | (u8) host->nf_parm.ecctype, NF_bECCCTRL);
	if (is_read)
		nfreg_write32(mtd, IMB_WR_FSH_INT_EN, NF_dwINTFLAG);
	else
		nfreg_write32(mtd, NF_DMA_INT_EN, NF_dwINTFLAG);
}

/*
 * wait DMA finish
 */
static int wait_dma_finish(struct mtd_info *mtd, int mode)
{
	int ret = 0;
	unsigned long timeo = jiffies;
	
	ret = wait_for_completion_timeout(&dma_completion, msecs_to_jiffies(50));
	if (!ret)
	{
		if (mode == HW_DMA_READ) {
			pr_warn("[NAND Warning] : Fix me, timeout R %x SOC IRQ 2C=%x 3C=%x\n",
				(int) nfreg_read32(mtd, NF_dwINTFLAG), (int) ali_soc_read(SOC_INT_POLARITY), (int) ali_soc_read(SOC_INT_ENABLE));
		} else {
			pr_warn("[NAND Warning] : Fix me, timeout W %x SOC IRQ 2C=%x 3C=%x\n",
				(int) nfreg_read32(mtd, NF_dwINTFLAG), (int) ali_soc_read(SOC_INT_POLARITY), (int) ali_soc_read(SOC_INT_ENABLE));
		}
		/* ali_nand_reg_dump(mtd); */
		
		/* polling finish */
		timeo = jiffies + HZ/20;
		while (time_before(jiffies, timeo)) {
			ret = nfreg_read32(mtd, NF_dwINTFLAG);
			if (ret & IMB_WR_FSH_FLAG) {
				pr_warn("[NAND Warning] : Fix me, polloing read end\n");
				nfreg_write32(mtd, 0, NF_dwINTFLAG);
				return 0;
			} else if (ret & NF_DMA_FLAG) {
				pr_warn("[NAND Warning] : Fix me, polloing write end\n");
				nfreg_write32(mtd, 0, NF_dwINTFLAG);
				return 0;
			}
			cond_resched();
		}
		pr_warn("[NAND Warning] : Fix me, Nand HW wait dma time out %x\n", (int) nfreg_read32(mtd, NF_dwINTFLAG));
		return -1;
	}	
	return 0;
}

/*
 * 
 */
static void ali_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if ((ctrl & NAND_NCE) != NAND_NCE) {
		nfreg_write8(mtd, NF_CEJ, NF_bCTRL);
		return;
	}

	if (ctrl & NAND_CTRL_CHANGE) {
		if ((ctrl & NAND_CTRL_CLE) == NAND_CTRL_CLE)
			nfreg_write8(mtd, NF_CLE, NF_bCTRL);
		else if ((ctrl & NAND_CTRL_ALE) == NAND_CTRL_ALE)
			nfreg_write8(mtd, NF_ALE, NF_bCTRL);
		else
			nfreg_write8(mtd, 0, NF_bCTRL);
	}

	if (NAND_CMD_NONE != cmd)
		nfreg_write8(mtd, (u8) cmd, NF_bPIODATA);
}

/* for 16k page and read retry */
/* 
 *ECC will be calculated automatically, and errors will be detected in
 * waitfunc.
 */ 
int ali_nand_read_page_hwecc(struct mtd_info *mtd, 
		struct nand_chip *chip, u8 * buf, int oob_required, int page)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	int i;
	u32 dma_transfer_len, max_dma_transfer_len = 8192;
	u32 ecc_err_cnt = 0;
	u8 is_read = 1;
	u8 rd_err_retry_cnt = 0;
	
	if(mtd->writesize > max_dma_transfer_len) /*  for 16kpage */
		dma_transfer_len = max_dma_transfer_len;
	else
		dma_transfer_len = mtd->writesize;

	ecc_err_cnt = mtd->ecc_stats.failed;

	if(0) {
rd_retry:
		chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
	}
	/*  for 16kpage */
	for(i=0; i<mtd->writesize; i+= dma_transfer_len) {
		/* init DMA */
		init_completion(&dma_completion); 
		init_hw_dma(mtd, is_read);
		/* set_dma address, length, start */
		set_dma_addr(mtd, host->hw_dma_addr + i);
		set_dma_length(mtd, dma_transfer_len >> 10);
		set_dma_start(mtd, 0, is_read);
		/* wait DMA finish */	
		if (wait_dma_finish(mtd, HW_DMA_READ)) {
			pr_err("[ERR] %s read page %d error\n", __FUNCTION__, page);
			ali_nand_soc_reset(mtd);
			goto dma_retry;
		}

		/* check ecc status */
		if (check_ecc_status(mtd, dma_transfer_len >> 10))
			pr_err("ecc error page #0x%08x\n", page);
		/* copy data to buf */
		if (buf != NULL) {
			if (host->data_all_ff && host->nf_parm.data_scramble)
				memset(buf+i, 0xff, dma_transfer_len);
			else
				memcpy(buf+i, host->dma_buf, dma_transfer_len);
		}
	}
	/* chip->oob always after dma buf + page lebgth */
	get_oob(mtd, (u32 *) &chip->oob_poi[0]);

	if(mtd->ecc_stats.failed > ecc_err_cnt) {
dma_retry:
		pr_err("\n[read ERR] %s page = 0x%x, times=%d\n", __FUNCTION__, page, rd_err_retry_cnt);
		
		if (rd_err_retry_cnt++ < 2) {
			pr_info("dma_retry cnt = %d", rd_err_retry_cnt);
			nf_cmd(mtd, NAND_CMD_RESET, -1, -1);
			mdelay(1);
			mtd->ecc_stats.failed = ecc_err_cnt;
			goto rd_retry;
		}
		else
			ali_nand_reg_dump(mtd);
	}
	return 0;
}

static int ali_nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
		int page)
{
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	chip->ecc.read_page(mtd, chip, NULL, 1, page);
	return 0;
}

static int ali_nand_write_page_hwecc(struct mtd_info *mtd,
		struct nand_chip *chip, const uint8_t *buf, int oob_required)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u8 is_read = 0;

	/* init hw dma */
	init_completion(&dma_completion);
	/* fill OOB*/
	fill_oob(mtd, (u32 *) &chip->oob_poi[0]);
	
	if (buf != NULL)
		memcpy(host->dma_buf, buf, mtd->writesize);
	else
		memset(host->dma_buf, 0xFF, mtd->writesize);
	/* set_dma addr, length, start */
	init_hw_dma(mtd, is_read);
	set_dma_addr(mtd, host->hw_dma_addr);
	set_dma_length(mtd, mtd->writesize >> 10);
	set_dma_start(mtd, 0, is_read);
	/* wait DMA finish */
	wait_dma_finish(mtd, HW_DMA_WRITE);

	return 0;
}

static int ali_nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip, 
		int page)
{
	int status = 0;
	
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0, page);
	chip->ecc.write_page(mtd, chip, NULL, 1);
	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);
	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/**
 * nand_write_page - [REPLACEABLE] write one page
 * @mtd:	MTD device structure
 * @chip:	NAND chip descriptor
 * @buf:	the data to write
 * @page:	page number to write
 * @cached:	cached programming
 * @raw:	use _raw version of write_page
 */
static int ali_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			uint32_t offset, int data_len, const uint8_t *buf, int oob_required, int page,
			int cached, int raw)
{
	int status;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

	if (unlikely(raw))
		chip->ecc.write_page_raw(mtd, chip, buf, 1);
	else
		chip->ecc.write_page(mtd, chip, buf, 1);
	/*
	 * Cached programming disabled for now, Not sure if its worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s)
	 */
	cached = 0;
	/* 	if (!cached || !(chip->options & NAND_CACHEPRG)) { */

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);
	/*
	 * See if operation failed and additional status checks are
	 * available
	 */
	if ((status & NAND_STATUS_FAIL) && (chip->errstat))
		status = chip->errstat(mtd, chip, FL_WRITING, status, page);

	if (status & NAND_STATUS_FAIL) {
		pr_err("[ERR] %s fail. status 0x%x !!!\n", __FUNCTION__, status);
		ali_nand_reg_dump(mtd);
		return -EIO;
	}
	
	#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	/* Send command to read back the data */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

	#endif
	return 0;
}
/**
 * nand_default_bbt - [NAND Interface] Select a default bad block table for the device
 * @mtd:	MTD device structure
 *
 * This function selects the default bad block table
 * support for the device and calls the nand_scan_bbt function
 *
 */

static int ali_nand_default_bbt(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;

	this->bbt_td = &ali_bbt_main_descr;
	this->bbt_md = &ali_bbt_mirror_descr;
	if (!this->badblock_pattern) 
		this->badblock_pattern = &largepage_flashbased;
	return nand_scan_bbt(mtd, this->badblock_pattern);
}

/*
 *	alidev_udc_irq - interrupt handler
 */
static irqreturn_t ali_nand_irq(int dummy, void *_host)
{	
	struct ali_nand_host *host = _host;
	struct mtd_info	*mtd = &host->mtd;
	
	nfreg_write32(mtd, 0, NF_dwINTFLAG);
	complete(&dma_completion);
	return IRQ_HANDLED;
}

/*	C3701 C version IC support new blank page detecttion
	0x58 [23:16] : threshold cont
	0x58 [31:24] : detect value (default is 0)
	0x58 [7:0]	: page 0~7 status, 1: blank / 0: non blank 
	
	initial set:
	1. 0x58 [23:16] = x , threshold value , for 16 bit ECC 15, 24 bit ECC 23
*/
static int set_blank_page_detect(struct mtd_info *mtd)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);	
		
	if (((C3701 == host->chip_id) && (host->chip_ver >= 0x01)) ||
		 (C3821 == host->chip_id) ||
		 (C3503 == host->chip_id) ||
		 (C3921 == host->chip_id)) {
		if (host->nf_parm.ecctype  == NF_BCH_16B_MODE)
			nfreg_write32(mtd, BLANK_DETECT_16ECC, NF_dwDETECTBLANK);
		else if(host->nf_parm.ecctype  == NF_BCH_24B_MODE)
			nfreg_write32(mtd, BLANK_DETECT_24ECC, NF_dwDETECTBLANK);
		else if(host->nf_parm.ecctype  == NF_BCH_40B_MODE)
			nfreg_write32(mtd, BLANK_DETECT_40ECC, NF_dwDETECTBLANK);
		else if(host->nf_parm.ecctype  == NF_BCH_48B_MODE)
			nfreg_write32(mtd, BLANK_DETECT_48ECC, NF_dwDETECTBLANK);
		else if(host->nf_parm.ecctype  == NF_BCH_60B_MODE)
			nfreg_write32(mtd, BLANK_DETECT_60ECC, NF_dwDETECTBLANK);
		else {
			pr_err("host->nf_parm.ecctype  error \n");
			return -1;
		}
	}
	return 0;
}

static int ali_nand_set_nf_parm(struct mtd_info *mtd,
						struct platform_device *pdev)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	struct nand_ecc_ctrl *ecc = &host->nand.ecc;
	struct device_node *np = pdev->dev.of_node;

	u32 val;
	int ret = 0;
	int oobsize_per_1k;

	oobsize_per_1k = mtd->oobsize / (mtd->writesize / ecc->size);

	if (oobsize_per_1k >= 32 && oobsize_per_1k < 46) 
		host->nf_parm.ecctype = NF_BCH_16B_MODE;
	else if (oobsize_per_1k >= 46 && oobsize_per_1k < 74)
		host->nf_parm.ecctype = NF_BCH_24B_MODE;
	else if (oobsize_per_1k >= 74 && oobsize_per_1k < 88)
		host->nf_parm.ecctype = NF_BCH_40B_MODE;
	else if (oobsize_per_1k >= 88 && oobsize_per_1k < 110) {
		/* For the compatible reason */
		host->nf_parm.ecctype = NF_BCH_40B_MODE;
	} else if (oobsize_per_1k >= 110)
		host->nf_parm.ecctype = NF_BCH_60B_MODE;

	if (of_property_read_u32(np, "data-scramble", &val) == 0) {
		if (val > 1) {
			dev_err(host->dev, "invalid data-scramble %u\n", val);
			return -EINVAL;
		}
		host->nf_parm.data_scramble = val;
	}

	host->nf_parm.read_timing = 0x12;
	host->nf_parm.write_timing = 0x11;

	nf_host_init(mtd);

	switch (host->nf_parm.ecctype) {
	case NF_BCH_24B_MODE:
		ecc->strength = 24;
		break;
	case NF_BCH_40B_MODE:
		ecc->strength = 40;
		break;
	case NF_BCH_48B_MODE:
		ecc->strength = 48;
		break;
	case NF_BCH_60B_MODE:
		ecc->strength = 60;
		break;
	case NF_BCH_16B_MODE:
	default:
		ecc->strength = 16;
		break;
	}
	return ret;
}

static int ali_nand_parse_partition(struct mtd_info *mtd)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u8 i = 0;
	struct mtd_partition *partitions = NULL;
#ifdef CONFIG_MTD_CMDLINE_PARTS
	static const char *part_probe_types[] = { "cmdlinepart", NULL };
#elif defined CONFIG_MTD_OF_PARTS
	static const char *part_probe_types[] = { "ofpart", NULL };
    struct mtd_part_parser_data ali_nand_flash;
#endif

#ifdef CONFIG_MTD_CMDLINE_PARTS
	host->nr_parts = parse_mtd_partitions(mtd, part_probe_types, &partitions, (struct mtd_part_parser_data *)DRIVER_NAME);
	memcpy(ali_nand_partitions, partitions, host->nr_parts*sizeof(struct mtd_partition));
#elif defined CONFIG_MTD_OF_PARTS
	pr_info("ali_nand: parse DTS!\n");
	memset(&ali_nand_flash, 0 ,sizeof(struct mtd_part_parser_data));
	ali_nand_flash.of_node = of_find_node_by_path("/NAND_flash@0");
	host->nr_parts = parse_mtd_partitions(mtd, part_probe_types, &partitions, &ali_nand_flash);
	memcpy(ali_nand_partitions, partitions, host->nr_parts*sizeof(struct mtd_partition));
	pr_info("ali_nand: host->nr_parts: %d\n", host->nr_parts);

#else
	pr_err("[ERR] no partitions\n");
	return -1;
#endif

	pr_info("total partition number %d\n", host->nr_parts);
	for (i=0; i<host->nr_parts; i++) {
		pr_info("partition[%02d] ofs=0x%012llx len=0x%012llx\n",
				i, ali_nand_partitions[i].offset, ali_nand_partitions[i].size);
	}

	host->parts = ali_nand_partitions;
	/* Register the partitions */
	if (host->nr_parts > 0)
		add_mtd_partitions(mtd, host->parts, host->nr_parts);
	else {
		pr_info("Registering %s as whole device\n", mtd->name);
		add_mtd_device(mtd);
	}
	return 0;
}

/**
 * ali_nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @mtd: MTD device structure
 * @command: the command to be sent
 * @column: the column address for this command, -1 if none
 * @page_addr: the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page
 * devices. We don't have the separate regions as we have in the small page
 * devices. We must emulate NAND_CMD_READOOB to keep the code compatible.
 */
static void ali_nand_command_lp(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}

	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (chip->options & NAND_BUSWIDTH_16)
				column >>= 1;
			chip->cmd_ctrl(mtd, column, ctrl);
			if ((command != NAND_CMD_READID) &&
					(command != NAND_CMD_PARAM)) {
			    ctrl &= ~NAND_CTRL_CHANGE;
			    chip->cmd_ctrl(mtd, column >> 8, ctrl);
		    }
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(mtd, page_addr, ctrl);
			chip->cmd_ctrl(mtd, page_addr >> 8,
				       NAND_NCE | NAND_ALE);
			/* One more address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(mtd, page_addr >> 16,
					       NAND_NCE | NAND_ALE);
		}
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * Program and erase have their own busy handlers status, sequential
	 * in, and deplete1 need no delay.
	 */
	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_STATUS:
		return;

	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY))
				;
		return;

	case NAND_CMD_RNDOUT:
		/* No ready / busy check necessary */
		chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		return;

	case NAND_CMD_READ0:
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
		//wait for R/B ready
        chip->waitfunc(mtd, chip);
        chip->cmd_ctrl(mtd, NAND_CMD_READ0,
                       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
        chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay.
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}

	/*
	 * Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine.
	 */
	ndelay(100);
	nand_wait_ready(mtd);
}

int ali_nand_pinmux_init(struct mtd_info* mtd, struct platform_device *pdev)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	struct device_node *dev_node = pdev->dev.of_node, *pinmux_node = NULL;
	struct property *pinmux_property = NULL;
	char pinmux_node_full_name[36], property_name[16];
	int num_reg = 0, num_field = 0, i = 0, j = 0, k = 0, has_set_property = 0;
	u32 *cur = NULL, *tmp = NULL;

	if(!dev_node) {
		dev_err(&pdev->dev, "No device node!\n");
		return -1;
	}
	for( i = 0; i < 2; i++) {
		num_field = 0;
		num_reg = 0;
		if(i == 0)
			snprintf(pinmux_node_full_name, sizeof(pinmux_node_full_name), "%s/pinmux_set", dev_node->full_name);
		else
			snprintf(pinmux_node_full_name, sizeof(pinmux_node_full_name), "%s/pinmux_release", dev_node->full_name);
		printk("pinmux_node_full_name: %s\n", pinmux_node_full_name);
		pinmux_node = of_find_node_by_path(pinmux_node_full_name);
		if(!pinmux_node) {
			dev_err(&pdev->dev, "No pinmux node!\n");
			return -1;
		}
		/* get total field needed for pinmux setting */
		snprintf(property_name, sizeof(property_name), "reg%d", num_reg);
		while( of_property_read_bool(pinmux_node, property_name) ) {
			snprintf(property_name, sizeof(property_name), "reg%d_set", num_reg);
			if(of_property_read_bool(pinmux_node, property_name))
				num_field+=3;
			snprintf(property_name, sizeof(property_name), "reg%d_clr", num_reg);
			if(of_property_read_bool(pinmux_node, property_name))
				num_field+=3;
			num_reg++;
			snprintf(property_name, sizeof(property_name), "reg%d", num_reg);
		}
		num_field++;

		if( i == 0)
			cur = host->pinmux_set = kzalloc(sizeof(u32)*num_field, GFP_KERNEL);
		else 
			cur = host->pinmux_release = kzalloc(sizeof(u32)*num_field, GFP_KERNEL);

		for(j = 0; j < num_reg; j++) {
			snprintf(property_name, sizeof(property_name), "reg%d", j);
			of_property_read_u32(pinmux_node, property_name, cur++);
			snprintf(property_name, sizeof(property_name), "reg%d_set", j);
			if(of_property_read_bool(pinmux_node, property_name)) {
				has_set_property = 1;
				pinmux_property = of_find_property(pinmux_node, property_name, NULL);
				tmp = kzalloc(sizeof(u8)*pinmux_property->length, GFP_KERNEL);
				of_property_read_u32_array(pinmux_node, property_name, tmp, pinmux_property->length / 4);
				for(k = 0; k*4 < pinmux_property->length; k++)
					*cur |= (1 << *(tmp+k) );
				*(++cur) = 1;
				cur++;
				kfree(tmp);
			}
			snprintf(property_name, sizeof(property_name), "reg%d_clr", j);
			if(of_property_read_bool(pinmux_node, property_name)) {	
				if(has_set_property) {
					*cur = *(cur -3);
					cur++;
				}
				pinmux_property = of_find_property(pinmux_node, property_name, NULL);
				tmp = kzalloc(sizeof(u8)*pinmux_property->length, GFP_KERNEL);
				of_property_read_u32_array(pinmux_node, property_name, tmp, pinmux_property->length / 4);
				for(k = 0; k*4 < pinmux_property->length; k++)
					*cur |= (1 << *(tmp+k) );
				*(++cur) = 0;	
				cur++;
				kfree(tmp);
			}
			has_set_property = 0;
		}
		*cur = 0;
		if(i == 0) {
			printk("pinmux set numbers: \n");
			for(j = 0; j < num_field; j++) 
				printk("%x ", *(host->pinmux_set+j));
		}
		else {
			printk("pinmux release numbers: \n");
			for(j = 0; j < num_field; j++) 
				printk("%x ", *(host->pinmux_release+j));
		}
		printk("\n");
	}
	return 0;
}

static int __init ali_nand_probe(struct platform_device *pdev)
{
	struct ali_nand_host *host = NULL;
	struct nand_chip *nand = NULL;
	struct mtd_info *mtd = NULL;
	struct resource *res = NULL;
	int irq;
	int retval = 0;
	int err = 0;
	long nand_wp_gpio = 0;
	
	struct ali_nand_platform_data {
		int *nand_wp_gpio;
	} *platform_data = pdev->dev.platform_data;

	dev_info(&pdev->dev, "%s, %s\n", __FUNCTION__,ALI_NAND_DRIVER_VERSION);
	/* Allocate memory for MTD device structure and private data */
	host = kzalloc(sizeof(struct ali_nand_host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;
	
	host->chip_id = (u16) (ali_soc_read(0) >> 16);
	host->chip_pkg = (u8) (ali_soc_read(0) >> 8) & 0xFF;
	host->chip_ver = (u8) (ali_soc_read(0)) & 0xFF;
	
	dev_info(&pdev->dev, "%s ALI_SOC_BASE 0x%x\n", __FUNCTION__, (u32)ALI_SOC_BASE);
	dev_info(&pdev->dev, "%s chip id 0x%x, chip_pkg 0x%x, ver 0x%x\n", __FUNCTION__,
		host->chip_id, host->chip_pkg, host->chip_ver);
	
	host->dev = kzalloc(sizeof(struct device), GFP_KERNEL);
	memcpy(host->dev, &pdev->dev, sizeof(struct device));
	res  = pdev->resource;
	/* structures must be linked */
	nand = &host->nand;
	mtd = &host->mtd;
	mtd->priv = nand;
	mtd->owner = THIS_MODULE;
	mtd->dev.parent = &pdev->dev;
	mtd->name = "ali_nand";
	/* Nand flash reset / clk not gated */
	ali_nand_clk_enable(mtd);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		err = -ENODEV;
		goto eres;
	}

	switch(host->chip_id) {
	case C3701:
	case C3503:
	case C3821:
	case C3505:
		host->regs = ioremap(ALI_NANDREG_BASE, ALI_NANDREG_LEN);
		dev_info(&pdev->dev, "mips ali_nand_reg, viture=0x%08x\n", (int) host->regs);
		break;
	case C3921:
		host->regs = ioremap(ALI_NANDREG_BASE, ALI_NANDREG_LEN);
		dev_info(&pdev->dev, "arm ali_nand_reg, viture=0x%08x\n", (int) host->regs);
		break;
	default:
		break;
	}

	nand->IO_ADDR_R = (void __iomem *) (host->regs + NF_bPIODATA);
	nand->IO_ADDR_W = (void __iomem *) (host->regs + NF_bPIODATA);
	if (!request_mem_region(res->start, res->end - res->start + 1, 
				nandname))
		return -EBUSY;
	/* buffer1 for DMA access */
#ifdef CONFIG_USE_OF
	host->dma_buf = dma_alloc_coherent(&pdev->dev,
			0x4000, &host->hw_dma_addr, GFP_KERNEL);
#else
	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res) {
		err = -ENODEV;
		goto eres;
	}
	
	host->dma_buf = dma_alloc_coherent(&pdev->dev, 
			res->end - res->start + 1, &host->hw_dma_addr, GFP_KERNEL);
#endif

	if (!host->dma_buf) {
		err = -ENOMEM;
		goto eres;
	}
	dev_info(&pdev->dev, "vitual dma buffer addr 0x%x\n", (u32) host->dma_buf);
	dev_info(&pdev->dev, "physical dma buffer addr 0x%x\n", (u32)host->hw_dma_addr);
	
	platform_data = pdev->dev.platform_data;	/*  Get Platform Data: nand_wp_gpio */
	if (platform_data != NULL) {
		dev_info(&pdev->dev, "ali nand write protect gpio %d\n", *platform_data->nand_wp_gpio);
		nand_wp_gpio = *platform_data->nand_wp_gpio;
		if(nand_wp_gpio > 0) {
			if(gpio_request(nand_wp_gpio, "nand_wp_gpio")) {
				dev_err(&pdev->dev, "[ERR] ali nand write protect gpio!\n");
				gpio_free(nand_wp_gpio);
			}
			else {
				gpio_enable_pin(nand_wp_gpio);
				gpio_direction_output(nand_wp_gpio, 0);
			}
		}
	}

	/* Reference hardware control function */
	nand->cmd_ctrl  = ali_nand_cmd_ctrl;
	nand->write_page = ali_nand_write_page;
	nand->ecc.read_page = ali_nand_read_page_hwecc;
	nand->ecc.write_page = ali_nand_write_page_hwecc;
	nand->select_chip = ali_nand_select_chip;
	nand->ecc.read_oob = ali_nand_read_oob_std;
	nand->ecc.write_oob = ali_nand_write_oob_std;
	nand->scan_bbt = ali_nand_default_bbt;
	nand->cmdfunc = ali_nand_command_lp;
	nand->dev_ready = NULL;
	nand->chip_delay = 1;
	
	/* reauest irq */	
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed %s (%d)  \n", __FUNCTION__, __LINE__);
		goto eres;
	}
	retval = request_irq(irq, ali_nand_irq, IRQF_SHARED, mtd->name, host);	
	if (retval != 0) {
		dev_err(&pdev->dev, "failed %s (%d)  \n", __FUNCTION__, __LINE__);
		goto eres;
	}
	dev_info(&pdev->dev, "nand irq = %d\n", irq);
		
	/* use complete */
	init_completion(&dma_completion);
	
	/*chip Enable*/
	nfreg_write8(mtd, NF_EN, NF_bMODE);
	
	/* options */
	nand->options = NAND_NO_SUBPAGE_WRITE;
	nand->ecc.layout = &ali_nand_oob_32;
	nand->ecc.mode = NAND_ECC_HW;
	nand->ecc.size = 1024;
	nand->ecc.bytes = 28;
	nand->ecc.layout->oobavail = 8;
	nand->bbt_td = &ali_bbt_main_descr;
	nand->bbt_md = &ali_bbt_mirror_descr;
	nand->bbt_options |= NAND_BBT_USE_FLASH;

	// if (ali_nand_set_nf_parm(mtd, pdev)) {
	// 	err = -ENXIO;
	// 	dev_err(&pdev->dev, "[ERR]ali_set_nf_parm fail, err %d\n", err);
	// 	goto escan;
	// }

	ali_nand_set_chip_clk(mtd);

	if (ali_nand_pinmux_init(mtd, pdev)) {
		err = -ENXIO;
		dev_err(&pdev->dev, "[ERR] ali_nand_pinmux_init fail, err %d\n", err);
		goto epinmux;
	}

	if (nand_scan_ident(mtd, 1, NULL)) {
		err = -ENXIO;
		dev_err(&pdev->dev, "[ERR] nand_scan_ident fail, err %d\n", err);
		goto escan;
	}

	if (ali_nand_set_nf_parm(mtd, pdev)) {
		err = -ENXIO;
		dev_err(&pdev->dev, "[ERR]ali_set_nf_parm fail, err %d\n", err);
		goto escan;
	}

	if (nand_scan_tail(mtd)) {
		err = -ENXIO;
		dev_err(&pdev->dev, "[ERR] nand_scan_tail fail, err %d\n", err);
		goto escan;
	}

	if (-1 == set_blank_page_detect(mtd))
		goto escan;   

	ali_nand_parse_partition(mtd);
	platform_set_drvdata(pdev, host);
	return 0;
escan:	
	iounmap(host->regs);
	dma_free_coherent(&pdev->dev, res->end - res->start + 1, host->dma_buf, host->hw_dma_addr);
epinmux:
	if(host->pinmux_set)
		kfree(host->pinmux_set);
	if(host->pinmux_release)
		kfree(host->pinmux_release);
eres:
	kfree(host);
	return err;
}

#ifdef CONFIG_PM
#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ali_nand_reg_store(struct mtd_info *mtd)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	int i = 0;

	for(i=0; i<ALI_NAND_REG_LEN; i+=4) {
		if((i != NF_dwDMADATA) && (i != NF_bPIODATA))
			host->ali_nand_regs[i/4] = nfreg_read32(mtd, i);
	}
	return 0;
}

static int ali_nand_reg_load(struct mtd_info *mtd)
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	int i = 0;

	for(i=0; i<ALI_NAND_REG_LEN; i+=4) {
		if((i != NF_dwDMADATA) && (i != NF_bPIODATA))
			nfreg_write32(mtd, host->ali_nand_regs[i/4], i);
	}
	return 0;
}
#endif
static int ali_nand_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ali_nand_host *host = platform_get_drvdata(pdev);
	struct mtd_info	*mtd;
	int ret = 0;

	if(host != NULL)
		mtd = &host->mtd;

	dev_dbg(&pdev->dev, "ALI_NAND : NAND suspend\n");
	if(host == NULL)
		return -1;

#ifdef CONFIG_ALI_STANDBY_TO_RAM
	ali_nand_reg_store(mtd);
#endif	
	ret = mtd->_suspend(mtd);
	ali_nand_clk_disable(mtd);

	return ret;
}

static int ali_nand_resume(struct platform_device *pdev)
{
	struct ali_nand_host *host = platform_get_drvdata(pdev);
	struct mtd_info	*mtd;
	struct nand_chip *chip;
	int ret = 0;

	if(host == NULL)
		return -1;

	mtd = &host->mtd;
	chip = &host->nand;

	/* Enable the NFC clock */
	ali_nand_clk_enable(mtd);

	mtd->_resume(mtd);

	ali_nand_select_chip(mtd, 0);
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	ali_nand_reg_load(mtd);
#endif
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
	ali_nand_select_chip(mtd, -1);
	udelay(500);

	return ret;
}
#else
	#define ali_nand_suspend   NULL
	#define ali_nand_resume    NULL
#endif /* CONFIG_PM */

static int __devexit ali_nand_remove(struct platform_device *pdev)
{
struct ali_nand_host *host = platform_get_drvdata(pdev);
struct mtd_info	*mtd = &host->mtd;

	ali_nand_clk_disable(mtd);	
	platform_set_drvdata(pdev, NULL);
	nand_release(mtd);	
	iounmap(host->regs);
	kfree(host->pinmux_set);
	kfree(host->pinmux_release);
	kfree(host);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ali_nand_of_match[] = {
	{ .compatible = "ali_tech,nandctrl", },
	{},
};
MODULE_DEVICE_TABLE(of, ali_nand_of_match)

static struct platform_driver ali_nand_driver = {
	.probe = ali_nand_probe,
	.remove = ali_nand_remove,
	.suspend = ali_nand_suspend,
	.resume = ali_nand_resume,
	.driver = {
		.name = DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = ali_nand_of_match,
	},
};

static int __init ali_nand_driver_init(void)
{
	int ret;
	ret = platform_driver_register(&ali_nand_driver);
	return ret;
}

#else
static struct platform_driver ali_nand_driver = {
	.driver = {
		.name = DRIVER_NAME,
	},
	.remove = ali_nand_remove,
	.suspend = ali_nand_suspend,
	.resume = ali_nand_resume,
};

static int __init ali_nand_driver_init(void)
{
	return platform_driver_probe(&ali_nand_driver, ali_nand_probe);
}
#endif

static void __exit ali_nand_driver_cleanup(void)
{
	platform_driver_unregister(&ali_nand_driver);
}

EXPORT_SYMBOL(ali_nand_partitions);
module_init(ali_nand_driver_init);
module_exit(ali_nand_driver_cleanup);
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI NAND MTD driver");
MODULE_LICENSE("GPL");
