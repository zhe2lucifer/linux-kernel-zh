/*
 *      Alitech CI Driver
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */
//#define DEBUG

#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/ali_transport.h>
#include <ali_cic.h>
#include "ali_m36_cic.h"
#include <linux/ali_kumsgq.h>
#include <ali_soc.h>

#define DRV_VERSION     "1.0.0"
#define ALI_CIC_DEVICE_NAME	"ali_cic"
/*#define CIC_BASE_ADDRESS	(0x1801a000)*/
/*#define CIC_ADDRESS_RANGE	(0x4900)*/

//cara.shi    2016/8/1
struct ali_cic_com_device_data
{
	//struct	cdev cdev;
	struct kumsgq *cic_kumsgq;
	struct mutex cic_mutex;
	//void    *priv;
};

struct ali_cic_com_device_data ali_cic_dev;

static struct class *m_cic_class;

static void cic_mem_mapping_dump(struct ali_cic_device *cic_dev)
{
	unsigned char  status;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	status = INPUT_UINT8(priv->base_addr + R_MER);
	dev_dbg(dev, "Mapping Enable[0x%08X : 0x%02X]\n",
		(unsigned int)priv->base_addr + R_MER, status);

	if (status & 0x01) {
		dev_dbg(dev, "Memory Map 0 Start Address Low[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_MMSAR0,
				INPUT_UINT8(priv->base_addr + R_MMSAR0));
		dev_dbg(dev, "Memory Map 0 Start Address High[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_MMSAR0 + 1,
				INPUT_UINT8(priv->base_addr + R_MMSAR0 + 1));
		dev_dbg(dev, "Memory Map 0 End Address Low[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_MMEAR0,
				INPUT_UINT8(priv->base_addr + R_MMEAR0));
		dev_dbg(dev, "Memory Map 0 End Address High[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_MMEAR0 + 1,
				INPUT_UINT8(priv->base_addr + R_MMEAR0 + 1));
		dev_dbg(dev, "Memory Map 0 Offset Address Low[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_MMOAR0,
				INPUT_UINT8(priv->base_addr + R_MMOAR0));
		dev_dbg(dev, "Memory Map 0 Offset Address High[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_MMOAR0 + 1,
				INPUT_UINT8(priv->base_addr + R_MMOAR0 + 1));
	}

	if (status & 0x40) {
		dev_dbg(dev, "I/O Window Control[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOWR,
				INPUT_UINT8(priv->base_addr + R_IOWR));
		dev_dbg(dev, "I/O Map 0 Start Address Low[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOMSAR0,
				INPUT_UINT8(priv->base_addr + R_IOMSAR0));
		dev_dbg(dev, "I/O Map 0 Start Address High[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOMSAR0 + 1,
				INPUT_UINT8(priv->base_addr + R_IOMSAR0 + 1));
		dev_dbg(dev, "I/O Map 0 End Address Low[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOMEAR0,
				INPUT_UINT8(priv->base_addr + R_IOMEAR0));
		dev_dbg(dev, "I/O Map 0 End Address High[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOMEAR0 + 1,
				INPUT_UINT8(priv->base_addr + R_IOMEAR0 + 1));
		dev_dbg(dev, "I/O Map 0 Offset Address Low[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOMOAR0,
				INPUT_UINT8(priv->base_addr + R_IOMOAR0));
		dev_dbg(dev, "I/O Map 0 Offset Address High[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_IOMOAR0 + 1,
				INPUT_UINT8(priv->base_addr + R_IOMOAR0 + 1));
	}
}

#if 0
static void cic_memory_config(struct ali_cic_device *cic_dev,
		enum CIC_MEM_CONFIG config)
{
	struct device *dev = cic_dev->dev;
	struct ali_cic_private *priv = &cic_dev->priv;

	/* Access attribute memory space for CIS */
	switch (config) {
	case ACCESS_ATTRIBUTE_MEM:
		OUTPUT_UINT8(priv->base_addr + R_MMOAR0 + 1,
		(INPUT_UINT8(priv->base_addr + R_MMOAR0 + 1) & 0xbf) | 0x40);
		dev_dbg(dev, "attribute memory access configuration done !\n");
		break;
	case ACCESS_COMMON_MEM:
		OUTPUT_UINT8(priv->base_addr + R_MMOAR0 + 1,
		(INPUT_UINT8(priv->base_addr + R_MMOAR0 + 1) & 0xbf));
		dev_dbg(dev, "common memory access configuration done !\n");
		break;
	default:
		dev_warn(dev, "unknown memory configuration !\n");
		break;
	}
	dev_dbg(dev, "memory Map 0 Offset Address High[0x%08X : 0x%02X]\n",
		(unsigned int)priv->base_addr + R_MMOAR0 + 1,
			INPUT_UINT8(priv->base_addr + R_MMOAR0 + 1));
}
#endif

static void cic_slot_status_notifier(struct ali_cic_device* cic_dev, int slot)
{
	struct cic_cb_para info;
	struct device *dev = NULL;
	struct ali_cic_private *priv = NULL;
	int i;
	
	/*mutex_lock(&cic_dev->lock);*/
	dev = cic_dev->dev;
	priv = &cic_dev->priv;


	info.slot = slot;
	info.flags = AUI_CIC_CB_CAM_DETECT;
	info.status = priv->status;
	
	dev_dbg(dev,"\nfunc:%s,line:%d,cic_dev->port:%lu\n",__func__,__LINE__,cic_dev->port);
	dev_dbg(dev,"send message len:%d,info:",sizeof(struct cic_cb_para));
	for(i = 0;i<sizeof(info);i++)
		dev_dbg(dev,"%02x",(*((char*)(&info) + i))	 & 0xFF);
	dev_dbg(dev,"\n");
	
	ali_kumsgq_sendmsg(ali_cic_dev.cic_kumsgq, &info, sizeof(struct cic_slot_info));
	/*mutex_unlock(&cic_dev->lock);*/
}

static irqreturn_t cic_interrupt_handler(int irq, void *param)
{
	unsigned char status;
	int i = 0, retval = IRQ_NONE;
	struct ali_cic_device *cic_dev = (struct ali_cic_device *)param;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	for (i = 0; i < CIC_MAX_SLOT_NUM; i++) {
		dev_dbg(dev, "CardA Interface Status[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + (i * 0x40) + R_IFXSR,
			INPUT_UINT8(priv->base_addr + (i * 0x40) + R_IFXSR));

		dev_dbg(dev, "CardA Status Change[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + (i * 0x40) + R_CSCR,
				INPUT_UINT8(priv->base_addr + (i * 0x40)
					+ R_CSCR));

		dev_dbg(dev, "CardA Management Interrupt Config[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + (i * 0x40) + R_MICR,
			INPUT_UINT8(priv->base_addr + (i * 0x40) + R_MICR));

		status = (INPUT_UINT8(priv->base_addr + (i * 0x40) + R_CSCR)
			& 0x0c);
		status &= INPUT_UINT8(priv->base_addr + (i * 0x40) + R_MICR);
		if (0 == status) {
			dev_warn(dev, "Card%d's status is invalid !\n", i);
			continue;
		}

		retval = IRQ_HANDLED;

		/* Card detect changed interrupt */
		if ((status & 0x08) != 0) {
			switch (INPUT_UINT8(priv->base_addr + (i * 0x40)
				+ R_IFXSR)
				& 0x0C) {
			case 0x0c:	/* Card insetted */
				dev_dbg(dev, "Card%d is inserted.\n.", i);
				/*OUTPUT_UINT8(priv->base_addr + R_MER, 0x41);*/
				OUTPUT_UINT8(priv->base_addr + R_MER,
					(INPUT_UINT8(priv->base_addr + R_MER)
						& 0xbe) | 0x41);
				dev_dbg(dev, "Card%d map window done !\n", i);
				cic_mem_mapping_dump(cic_dev);
				/*cic_memory_config(cic_dev,
						ACCESS_ATTRIBUTE_MEM);*/
				priv->status = CIC_CAM_INSERTED;
				break;
			case 0x00:	/* Card removed */
				dev_dbg(dev, "Card%d is removd.\n", i);
				/*if (((INPUT_UINT8(ioaddr + + R_IFXSR)
					& 0x0C) == 0) &&
					((INPUT_UINT8(
					ioaddr + 0x40 + R_IFXSR) & 0x0C)
					== 0))*/
				/*OUTPUT_UINT8(priv->base_addr + R_MER,
					0x00);*/
				OUTPUT_UINT8(priv->base_addr + R_MER,
					INPUT_UINT8(priv->base_addr + R_MER)
						& 0xbe);
				dev_dbg(dev, "Card%d clear window done !\n", i);
				priv->status = CIC_CAM_REMOVED;
				break;
			default:
				/* Only one of CD1 and CD2 changed */
				dev_warn(dev, "Card%d's unknown action !\n", i);
				break;
			}

			dev_dbg(dev, "Mapping Enable[0x%08X : 0x%02X]\n",
				(unsigned int)priv->base_addr + R_MER,
					INPUT_UINT8(priv->base_addr + R_MER));
	
			cic_slot_status_notifier(cic_dev, i);
			/*if (priv->callback != NULL) {*/
				/* Callback CI stack to tell
				the slot actived. Current slot 0 */
				/*osal_interrupt_register_hsr(
					(((struct ali_cic_private *)
					param)->callback), (UINT32)i);
				*/
			/*}*/
		}

		/* Card IREQ interrupt */
		if ((status & 0x04) != 0) {
			/*PRINTF("IREQ interrupt\n"); */
			/* Do nothing */
		}
	}

	return retval;
}

static int cic_read(struct ali_cic_device *dev,
	unsigned short size, unsigned char *buffer)
{
	int i = 0;
	struct ali_cic_private *priv = &dev->priv;

	BUG_ON(NULL == buffer);

	mutex_lock(&dev->lock);
	for (i = 0; i < size; i++)
		buffer[i] = INPUT_UINT8(priv->reg_data);
	mutex_unlock(&dev->lock);

	return 0;
}

static int cic_write(struct ali_cic_device *dev,
		unsigned short size, unsigned char *buffer)
{
	int i = 0;
	struct ali_cic_private *priv = &dev->priv;

	BUG_ON(NULL == buffer);

	mutex_lock(&dev->lock);
	for (i = 0; i < size; i++)
		OUTPUT_UINT8(priv->reg_data, buffer[i]);
	mutex_unlock(&dev->lock);

	return 0;
}

static int cic_readmem(struct ali_cic_device *cic_dev,
	struct cic_msg *param)
{
	int i = 0;
	UINT16 addr;
	void __iomem *memaddr = NULL;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	BUG_ON(NULL == param);

	addr = param->type & 0xffff;
	memaddr = priv->base_addr + R_MBASE + (addr << 1);

	dev_dbg(dev, "read %d data in 0x%04X (beginning 0x%08X) ......\n",
		param->length, addr, (unsigned int)memaddr);

	if (param->length > CIC_MSG_MAX_LEN || addr + param->length >= 0x2000) {
		dev_err(dev, "address or length is overflow !\n");
		return -EFAULT;
	}

	mutex_lock(&cic_dev->lock);
	for (i = 0; i < param->length; i++) {
		param->msg[i] = INPUT_UINT8(memaddr + (i << 1));
		dev_dbg(dev, "\tread 0x%02X from 0x%08X.\n",
			param->msg[i], (unsigned int)memaddr + (i << 1));
	}
	mutex_unlock(&cic_dev->lock);

	return 0;
}

static int cic_writemem(struct ali_cic_device *cic_dev,
	struct cic_msg *param)
{
	int i = 0;
	UINT16 addr;
	void __iomem *memaddr = NULL;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	BUG_ON(NULL == param);

	addr = param->type & 0xffff;
	memaddr = priv->base_addr + R_MBASE + (addr << 1);

	dev_dbg(dev, "write %d data in 0x%04X (beginning 0x%08X) ......\n",
		param->length, addr, (unsigned int)memaddr);

	if (param->length > CIC_MSG_MAX_LEN || addr + param->length >= 0x2000) {
		dev_err(dev, "address or length is overflow !\n");
		return -EFAULT;
	}

	mutex_lock(&cic_dev->lock);
	for (i = 0; i < param->length; i++) {
		OUTPUT_UINT8(memaddr + (i << 1), param->msg[i]);
		dev_dbg(dev, "\twrite 0x%02X in 0x%08X.\n",
			param->msg[i], (unsigned int)memaddr + (i << 1));
	}
	mutex_unlock(&cic_dev->lock);

	return 0;
}

static int cic_readio(struct ali_cic_device *cic_dev,
	struct cic_msg *param)
{
	UINT16 reg;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	BUG_ON(NULL == param);

	reg = param->type >> 16;

	mutex_lock(&cic_dev->lock);
	switch (reg) {
	case CIC_DATA:
		param->msg[0] = INPUT_UINT8(priv->reg_data);
		dev_dbg(dev, "to read 0x%02X from IO data 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_data);
		break;
	case CIC_CSR:
		param->msg[0] = INPUT_UINT8(priv->reg_cs);
		dev_dbg(dev, "to read 0x%02X from IO CS 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_cs);
		break;
	case CIC_SIZELS:
		param->msg[0] = INPUT_UINT8(priv->reg_szl);
		dev_dbg(dev, "to read 0x%02X from IO Size Low 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_szl);
		break;
	case CIC_SIZEMS:
		param->msg[0] = INPUT_UINT8(priv->reg_szh);
		dev_dbg(dev, "to read 0x%02X from IO Size High 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_szh);
		break;
	default:
		dev_warn(dev, "to read data from unknown io register.\n");
		break;
	}
	mutex_unlock(&cic_dev->lock);

	return 0;
}

static int cic_writeio(struct ali_cic_device *cic_dev,
	struct cic_msg *param)
{
	UINT16 reg;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	BUG_ON(NULL == param);

	reg = param->type>>16;

	mutex_lock(&cic_dev->lock);
	switch (reg) {
	case CIC_DATA:
		dev_dbg(dev, "to write 0x%02X in IO data 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_data);
		OUTPUT_UINT8(priv->reg_data, param->msg[0]);
		break;
	case CIC_CSR:
		dev_dbg(dev, "to write 0x%02X in IO CS 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_cs);
		OUTPUT_UINT8(priv->reg_cs, param->msg[0]);
		break;
	case CIC_SIZELS:
		dev_dbg(dev, "to write 0x%02X in IO Size Low 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_szl);
		OUTPUT_UINT8(priv->reg_szl, param->msg[0]);
		break;
	case CIC_SIZEMS:
		dev_dbg(dev, "to write 0x%02X in IO Size High 0x%08X.\n",
			param->msg[0], (unsigned int)priv->reg_szh);
		OUTPUT_UINT8(priv->reg_szh, param->msg[0]);
		break;
	default:
		dev_warn(dev, "to write data in unknown io register.\n");
		break;
	}
	mutex_unlock(&cic_dev->lock);

	return 0;
}

static int cic_tsignal(struct ali_cic_device *cic_dev,
		struct cic_slot_info *param)
{
	int retval = 0;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	BUG_ON(NULL == param);

	if (param->num < 0 || param->num > CIC_MAX_SLOT_NUM - 1) {
		dev_err(dev, "slot num is invalid !\n");
		return -EINVAL;
	}

	dev_dbg(dev, "slot num is %d and action type is %d.\n",
		param->num, param->type);

	mutex_lock(&cic_dev->lock);
	switch (param->type) {
	case CIC_CARD_DETECT:
		param->flags = ((INPUT_UINT8(priv->base_addr +
			(param->num * 0x40) + R_IFXSR) & 0x0C) >> 2);
		dev_dbg(dev, "CardA Status Change[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + (param->num * 0x40)
				+ R_CSCR, INPUT_UINT8(priv->base_addr +
					(param->num * 0x40) + R_CSCR));
		dev_dbg(dev, "CardA Interface Status[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + (param->num * 0x40)
				+ R_IFXSR, INPUT_UINT8(priv->base_addr +
					(param->num * 0x40) + R_IFXSR));
		dev_dbg(dev, "Card%d insert detect done (status : 0x%02x).\n",
			param->num, param->flags);
		break;
	case CIC_CARD_READY:
		param->flags = ((INPUT_UINT8(priv->base_addr +
			(param->num * 0x40) + R_IFXSR) & 0x6C) == 0x6C);
		dev_dbg(dev, "CardA Interface Status[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + (param->num * 0x40)
				+ R_IFXSR, INPUT_UINT8(priv->base_addr +
					(param->num * 0x40) + R_IFXSR));
		dev_dbg(dev, "Card%d power and ready detect done (status : 0x%02x).\n",
			param->num, param->flags);
		break;
	default:
		retval = -EINVAL;
		dev_warn(dev, "but do nothing here !\n");
		break;
	}
	mutex_unlock(&cic_dev->lock);

	return retval;
}

static int cic_ssignal(struct ali_cic_device *cic_dev,
		struct cic_slot_info *param)
{
	int retval = 0;
	struct ali_cic_private *priv = &cic_dev->priv;
	struct device *dev = cic_dev->dev;

	BUG_ON(NULL == param);

	if (param->num < 0 || param->num > CIC_MAX_SLOT_NUM - 1) {
		dev_err(dev, "slot num is invalid !\n");
		return -EINVAL;
	}

	dev_dbg(dev, "slot num is %d and action type is %d.\n",
		param->num, param->type);

	mutex_lock(&cic_dev->lock);
	switch (param->type) {
	case CIC_ENSTREAM:	/* Enable stream (1) or not (0) */
		if (param->flags) {
			/* Set TS pass CAM card */
			if (param->num == 0) {
				OUTPUT_UINT8(priv->base_addr + R_TSCR1,
				INPUT_UINT8(priv->base_addr + R_TSCR1)
					& ~RB_TSCR_CIBYPASS);
				OUTPUT_UINT8(priv->base_addr + R_CISEL + 2,
				INPUT_UINT8(priv->base_addr + R_CISEL + 2)
						& (~0x80));
				dev_dbg(dev, "TS Control 1[0x%08X : 0x%02X]\n",
					(unsigned int)priv->base_addr + R_TSCR1,
						INPUT_UINT8(priv->base_addr +
							R_TSCR1));
				dev_dbg(dev, "CI Select[0x%08X : 0x%02X]\n",
					(unsigned int)priv->base_addr + R_CISEL
						+ 2, INPUT_UINT8(priv->base_addr
						+ R_CISEL + 2));
				dev_dbg(dev, "CardA stream pass enabled !\n");
			} else if (param->num == 1) {
				OUTPUT_UINT8(priv->base_addr + R_CISEL,
				INPUT_UINT8(priv->base_addr + R_CISEL)
						& ~RB_TSCR_CIBYPASS);
				OUTPUT_UINT8(priv->base_addr + R_CISEL
					+ 2, INPUT_UINT8(priv->base_addr
						+ R_CISEL + 2) & (~0x40));
				dev_dbg(dev, "CI Select[0x%08X : 0x%02X]\n",
					(unsigned int)priv->base_addr + R_CISEL,
						INPUT_UINT8(priv->base_addr
							+ R_CISEL));
				dev_dbg(dev, "CI Select[0x%08X : 0x%02X]\n",
					(unsigned int)priv->base_addr + R_CISEL
						+ 2, INPUT_UINT8(priv->base_addr
							+ R_CISEL + 2));
				dev_dbg(dev, "CardB stream pass enabled !\n");
			}
		} else {
			/* Set TS bypass CAM card */
			if (param->num == 0) {
				OUTPUT_UINT8(priv->base_addr + R_TSCR1,
					INPUT_UINT8(priv->base_addr + R_TSCR1)
						| RB_TSCR_CIBYPASS);
				dev_dbg(dev, "TS Control 1[0x%08X : 0x%02X]\n",
					(unsigned int)priv->base_addr + R_TSCR1,
						INPUT_UINT8(priv->base_addr
							+ R_TSCR1));
				dev_dbg(dev, "CardA stream pass disabled !\n");
			} else if (param->num == 1) {
				OUTPUT_UINT8(priv->base_addr + R_CISEL,
				INPUT_UINT8(priv->base_addr + R_CISEL)
						| RB_TSCR_CIBYPASS);
				dev_dbg(dev, "CI Select[0x%08X : 0x%02X]\n",
					(unsigned int)priv->base_addr + R_CISEL,
						INPUT_UINT8(priv->base_addr
							+ R_CISEL));
				dev_dbg(dev, "CardB stream pass disabled !\n");
			}
		}
		break;
	case CIC_ENSLOT:	/* enable slot */
		if (param->flags) {
			/* just provide Vcc, not enable the output here. */
			/*OUTPUT_UINT8(priv->base_addr + (param->num * 0x40)
				+ R_PWRCR, 0x90);*/
			OUTPUT_UINT8(priv->base_addr +
				(param->num * 0x40) + R_PWRCR,
					(INPUT_UINT8(priv->base_addr
						+ (param->num * 0x40)
							+ R_PWRCR) & 0x6F)
								|0x90);
			dev_dbg(dev, "CardA power control[0x%08X : 0x%02X]\n",
				(unsigned int)priv->base_addr +
					(param->num * 0x40) + R_PWRCR,
						INPUT_UINT8(priv->base_addr +
							(param->num * 0x40)
								+ R_PWRCR));

			if (param->num == 0) {
				/* slot 0. */
				/* Make sure auto clear power supply */
				OUTPUT_UINT8(priv->base_addr
						+ R_EIR, RE_EXTCR1);
				/*OUTPUT_UINT8(priv->base_addr + R_EDR,
					((INPUT_UINT8(priv->base_addr + R_EDR)
						& 0xfc) | 0x02));*/
			} else if (param->num == 1) {
				/* slot 1. */
				/* Make sure auto clear power supply */
				OUTPUT_UINT8(priv->base_addr + R_EIR,
						RE_EXTCR1);
				/*OUTPUT_UINT8(priv->base_addr + R_EDR,
					((INPUT_UINT8(
					priv->base_addr + R_EDR) & 0xf3)
						| 0x08));*/
			}
			dev_dbg(dev, "Extend Index[0x%08X : 0x%02X]\n",
				(unsigned int)priv->base_addr + R_EIR,
					INPUT_UINT8(priv->base_addr + R_EIR));
			dev_dbg(dev, "Extend Data[0x%08X : 0x%02X]\n",
				(unsigned int)priv->base_addr + R_EDR,
					INPUT_UINT8(priv->base_addr + R_EDR));
			dev_dbg(dev, "Card%d enabled !\n", param->num);
		} else {
			/* Disable power supply */
			/*OUTPUT_UINT8(priv->base_addr + (param->num * 0x40)
				+ R_PWRCR, 0x00);*/
			OUTPUT_UINT8(priv->base_addr +
				(param->num * 0x40) + R_PWRCR,
					INPUT_UINT8(priv->base_addr
						+ (param->num * 0x40) + R_PWRCR)
							& 0x6F);
			dev_dbg(dev, "Card%d Power Control[0x%08X : 0x%02X]\n",
				param->num, (unsigned int)priv->base_addr
					+ (param->num * 0x40) + R_PWRCR,
						INPUT_UINT8(priv->base_addr +
							(param->num * 0x40)
								+ R_PWRCR));
			dev_dbg(dev, "Card%d disabled !\n", param->num);
		}
		break;
	case CIC_RSTSLOT:	/* reset slot (0) or not (1) */
		/*if (param->flags)
			OUTPUT_UINT8(priv->base_addr +
				(param->num * 0x40) + R_IGCR,
					(INPUT_UINT8(priv->base_addr
						+ (param->num * 0x40) + R_IGCR)
							&0x7F) | 0x80);
		else
			OUTPUT_UINT8(priv->base_addr +
				(param->num * 0x40) + R_IGCR,
					INPUT_UINT8(priv->base_addr
						+ (param->num * 0x40) + R_IGCR)
							&0x7F);*/
		OUTPUT_UINT8(priv->base_addr +
				(param->num * 0x40) + R_IGCR,
					(!param->flags & 1) << 7);
		dev_dbg(dev, "Card%d Interrupt and General Control[0x%08X : 0x%02X]\n",
			param->num, (unsigned int)priv->base_addr
				+ (param->num * 0x40) + R_IGCR,
					INPUT_UINT8(priv->base_addr +
						(param->num * 0x40) + R_IGCR));
		dev_dbg(dev, "Card%d Reset !\n", param->num);
		/*msleep(250);*/
		break;
	case CIC_IOMEM:		/* switch io (1) or mem space (0) */
		/* Do nothing for IO and mem space
		all accessable at sametime */
		break;
	case CIC_SLOTSEL:	/* select slot 0 (0) or slot 1 (1) */
		OUTPUT_UINT8(priv->base_addr + R_CISEL,
			((INPUT_UINT8(priv->base_addr + R_CISEL) & 0xfd)
			| ((param->num == 0) ? 0x00 : 0x02)));
		dev_dbg(dev, "CI Select[0x%08X : 0x%02X]\n",
			(unsigned int)priv->base_addr + R_CISEL,
				INPUT_UINT8(priv->base_addr + R_CISEL));
		dev_dbg(dev, "Card%d Selected !\n", param->num);
		break;
	default:
		retval = -EINVAL;
		dev_dbg(dev, "do nothing here !\n");
		break;
	}
	mutex_unlock(&cic_dev->lock);

	return retval;
}

static int ali_cic_open(struct inode *inode, struct file *file);
static int ali_cic_release(struct inode *inode, struct file *file);
static long ali_cic_ioctl(
	struct file *file, unsigned int cmd, unsigned long parg);

/***************************************************************************
 * driver registration
 ***************************************************************************/

static const struct file_operations ali_cic_fops = {
	.owner		= THIS_MODULE,
	/* .write		= ali_m36_cic_write, */
	.unlocked_ioctl = ali_cic_ioctl,
	.open		= ali_cic_open,
	.release	=  ali_cic_release,
	/* .poll		= dvb_cic_poll, */
};

static long ali_cic_ioctl(
	struct file *file, unsigned int cmd, unsigned long parg)
{
	struct ali_cic_device *cic_dev = file->private_data;
	unsigned int flag;
	struct cic_msg ali_ci_msg;
	struct cic_slot_info info;
	struct device *dev = NULL;
	int ret = -EINVAL;

	BUG_ON(NULL == cic_dev);
	dev =  cic_dev->dev;

	switch (cmd) {
	/*case CIC_GET_CAP:
		dev_dbg(dev, "io command CA_GET_CAP ==> ");
		break;*/
	case CIC_GET_GET_KUMSGQ:
	{
		int flags = -1;
		mutex_lock(&ali_cic_dev.cic_mutex);
		if(copy_from_user(&flags, (int *)parg, sizeof(int)))
		{
			printk("Err: copy_from_user\n");
			mutex_unlock(&ali_cic_dev.cic_mutex);
			return -EFAULT;
		}
		ret  = ali_kumsgq_newfd(ali_cic_dev.cic_kumsgq, flags);
		if(ret> 0)
		{
			mutex_unlock(&ali_cic_dev.cic_mutex);
			return ret;	
		}	
	}
	case CIC_GET_SLOT_INFO:
	{
		dev_dbg(dev, "io command CA_GET_SLOT_INFO ==> ");
		if (0 != copy_from_user(&info, (struct cic_slot_info __user *)
				parg, sizeof(struct cic_slot_info))) {
			dev_err(dev, "data from user copy error !\n");
			return -EPERM;
		}

		ret = cic_tsignal(cic_dev, &info);

		if (0 != copy_to_user((struct cic_slot_info __user *)parg,
				&info, sizeof(struct cic_slot_info))) {
			dev_err(dev, "data to user copy error !\n");
			return -EPERM;
		}
		break;
	}
	case CIC_SET_SLOT_INFO:
	{
		dev_dbg(dev, "io command CA_SET_SLOT_INFO ==> ");
		if (0 != copy_from_user(&info, (struct cic_slot_info __user *)
				parg, sizeof(struct cic_slot_info))) {
			dev_err(dev, "data from user copy error !\n");
			return -EPERM;
		}

		ret =  cic_ssignal(cic_dev, &info);
		break;
	}
	case CIC_GET_MSG:
		dev_dbg(dev, "io command CIC_GET_MSG ==> ");
		if (0 != copy_from_user(&ali_ci_msg, (struct cic_msg __user *)
				parg, sizeof(struct cic_msg))) {
			dev_err(dev, "data from user copy error !\n");
			return -EPERM;
		}
		flag = ali_ci_msg.type>>16;
		switch (flag) {
		case CIC_DATA:		/* CI data register */
		case CIC_CSR:		/* CI command/stauts register */
		case CIC_SIZELS:	/* CI size register low bytes */
		case CIC_SIZEMS:
			/* CI size register high bytes */
			ret = cic_readio(cic_dev, &ali_ci_msg);
			break;
		case CIC_MEMORY:		/* CI memory space*/
			ret = cic_readmem(cic_dev, &ali_ci_msg);
			break;
		case CIC_BLOCK:
			ret = cic_read(cic_dev, ali_ci_msg.length, ali_ci_msg.msg);
			break;
		default:
			dev_warn(dev, "do nothing here !\n");
			break;
		}
		if (0 != copy_to_user((struct cic_msg __user *)parg,
				&ali_ci_msg, sizeof(struct cic_msg))) {
			dev_err(dev, "data to user copy error !\n");
			return -EPERM;
		}
		break;
	case CIC_SEND_MSG:
		dev_dbg(dev, "io command CA_SEND_MSG ==> ");
		if (0 != copy_from_user(&ali_ci_msg, (struct cic_msg __user *)
				parg, sizeof(struct cic_msg))) {
			dev_err(dev, "data from user copy error !\n");
			return -EPERM;
		}
		flag = ali_ci_msg.type>>16;
		switch (flag) {
		case CIC_DATA:			/* CI data register */
		case CIC_CSR:
			/* CI command/stauts register */
		case CIC_SIZELS:
			/* CI size register low bytes */
		case CIC_SIZEMS:
			/* CI size register high bytes */
			ret = cic_writeio(cic_dev, &ali_ci_msg);
			break;
		case CIC_MEMORY:		/* CI memory space*/
			ret = cic_writemem(cic_dev, &ali_ci_msg);
			break;
		case CIC_BLOCK:
			ret = cic_write(cic_dev, ali_ci_msg.length, ali_ci_msg.msg);
			break;
		default:
			dev_warn(dev, "do nothing here !\n");
			break;
		}
		break;
	default:
		dev_warn(dev, "unknown io command !\n");
		break;
	}

	return ret;
}

static int ali_cic_open(struct inode *inode, struct file *file)
{
	struct ali_cic_device *cic_dev = NULL;
	struct ali_cic_private *priv = NULL;
	struct device *dev = NULL;
	cic_dev = container_of(inode->i_cdev, struct ali_cic_device, cdev);
	if (cic_dev == NULL) {
		dev_err(dev, "cic device opens failure !\n");
		return -EINVAL;
	}

	ali_cic_dev.cic_kumsgq = ali_new_kumsgq();
	if (!ali_cic_dev.cic_kumsgq)
	{
		goto out0;
	}

	mutex_lock(&cic_dev->lock);
	dev = cic_dev->dev;

	if (cic_dev->in_use) {
		dev_err(dev, "cic device opens already !\n");
		mutex_unlock(&cic_dev->lock);
		return -EBUSY;
	}

	priv = &cic_dev->priv;

	/* Setup CI command interface registers */
	priv->reg_data = priv->base_addr + R_IOBASE;
	priv->reg_cs = priv->base_addr + R_IOBASE + 1;
	priv->reg_szl = priv->base_addr + R_IOBASE + 2;
	priv->reg_szh = priv->base_addr + R_IOBASE + 3;
	dev_dbg(dev, "IO Data : 0x%08x  IO CS : 0x%08x.\n",
		(unsigned int)priv->reg_data, (unsigned int)priv->reg_cs);
	dev_dbg(dev, "IO Size Low : 0x%08x  IO Size High : 0x%08x.\n",
		(unsigned int)priv->reg_szl, (unsigned int)priv->reg_szh);

	OUTPUT_UINT8(priv->base_addr + R_STM0, 0x03);
	dev_dbg(dev, "Setup Timing 0[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_STM0,
			INPUT_UINT8(priv->base_addr + R_STM0));
	OUTPUT_UINT8(priv->base_addr + R_CTM0, 0x06);
	dev_dbg(dev, "Command Timing 0[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_CTM0,
			INPUT_UINT8(priv->base_addr + R_CTM0));
	OUTPUT_UINT8(priv->base_addr + R_RTM0, 0x03);
	dev_dbg(dev, "Recovery Timing 0[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_RTM0,
			INPUT_UINT8(priv->base_addr + R_RTM0));
	OUTPUT_UINT8(priv->base_addr + R_STM1, 0x03);
	dev_dbg(dev, "Setup Timing 1[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_STM1,
			INPUT_UINT8(priv->base_addr + R_STM1));
	OUTPUT_UINT8(priv->base_addr + R_CTM1, 0x06);
	dev_dbg(dev, "Command Timing 1[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_CTM1,
			INPUT_UINT8(priv->base_addr + R_CTM1));
	OUTPUT_UINT8(priv->base_addr + R_RTM1, 0x03);
	dev_dbg(dev, "Recovery Timing 1[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_RTM1,
			INPUT_UINT8(priv->base_addr + R_RTM1));
	
	OUTPUT_UINT8(priv->base_addr + R_MER,
		(INPUT_UINT8(priv->base_addr + R_MER)
			& 0xbe) | 0x41);
	
	OUTPUT_UINT8(priv->base_addr + R_MMOAR0 + 1,
	(INPUT_UINT8(priv->base_addr + R_MMOAR0 + 1) & 0xbf) | 0x40);

	/*OUTPUT_UINT8(priv->base_addr + R_MICR, 0x8c);*/
	OUTPUT_UINT8(priv->base_addr + R_MICR,
		(INPUT_UINT8(priv->base_addr + R_MICR)&0x77)|0x88);
	/*OUTPUT_UINT8(priv->base_addr + 0x40 + R_MICR, 0x88);*/
    dev_dbg(dev, "CardA Management Interrupt Configuration[0x%08X : 0x%02X]\n",
		(unsigned int)(priv->base_addr) + R_MICR,
			INPUT_UINT8(priv->base_addr + R_MICR));

	file->private_data = (void *)cic_dev;
	cic_dev->in_use = 1;
	mutex_unlock(&cic_dev->lock);

	return 0;
out0:
	WARN(1,"False to new cic kumsgq!!!!!!");
	return -EFAULT;
}

static int ali_cic_release(struct inode *inode, struct file *file)
{
	struct ali_cic_device *dev = file->private_data;
	struct ali_cic_private *priv = NULL;


	BUG_ON(NULL == dev);

	mutex_lock(&dev->lock);
	priv = &dev->priv;
	/*free_irq(priv->slot_irq_num, priv);*/

	OUTPUT_UINT8(priv->base_addr + R_TSCR1,
	INPUT_UINT8(priv->base_addr + R_TSCR1) | RB_TSCR_CIBYPASS);

	/* Disable power supply */
	/*OUTPUT_UINT8(tp->base_addr + R_PWRCR, 0x00);*/

	dev->in_use = 0;
	ali_destroy_kumsgq(ali_cic_dev.cic_kumsgq);
	ali_cic_dev.cic_kumsgq = NULL;
	mutex_unlock(&dev->lock);

	return 0;
}

static int __init ali_cic_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct resource *res = NULL;
	struct device *dev = &pdev->dev;
	struct ali_cic_private *priv = NULL;
	struct ali_cic_device *cic_dev = NULL;

	
	
	mutex_init(&ali_cic_dev.cic_mutex);
	
	cic_dev = devm_kzalloc(dev, sizeof(struct ali_cic_device), GFP_KERNEL);
	if (IS_ERR(cic_dev)) {
		retval = PTR_ERR(cic_dev);
		dev_err(dev, "cic device memory allocates failure (err : %d) !\n",
			retval);
		return retval;
	}

	memset(cic_dev, 0x00, sizeof(struct ali_cic_device));

	/* define device name */
	strncpy(cic_dev->name, ALI_CIC_DEVICE_NAME, sizeof(cic_dev->name));

	m_cic_class = class_create(THIS_MODULE, cic_dev->name);
	if (IS_ERR(m_cic_class)) {
		retval = PTR_ERR(m_cic_class);
		dev_err(dev, "cic device class creates failure (err : %d) !\n",
			retval);
		goto mem_free;
	}

	retval = of_get_major_minor(pdev->dev.of_node,&cic_dev->id, 
			0, 1, cic_dev->name);
	if (retval  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto class_del;
	}

	cdev_init(&cic_dev->cdev, &ali_cic_fops);
	cic_dev->cdev.owner = ali_cic_fops.owner;
	/*kobject_set_name(&(cic_dev->cdev.kobj), "%s", ALI_CIC_DEVICE_NAME);*/
	retval  = cdev_add(&cic_dev->cdev, cic_dev->id, 1);
	if (retval) {
		dev_err(dev, "cic device adds failure (err : %d) !\n",
			retval);
		goto cdevno_free;
	}

	cic_dev->dev = device_create(m_cic_class, NULL, cic_dev->id,
					cic_dev, cic_dev->name);
	if (IS_ERR(cic_dev->dev)) {
		retval = PTR_ERR(cic_dev->dev);
		dev_err(dev, "cic device creates failure (err : %d) !\n",
			retval);
		goto cdev_del;
	}

	platform_set_drvdata(pdev, cic_dev);

	priv = &(cic_dev->priv);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base_addr = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base_addr)) {
		retval = PTR_ERR(priv->base_addr);
		dev_err(dev, "cic device base address requests failure  (err : %d) !\n",
				retval);
		goto dev_del;
	}
	dev_dbg(dev, "CI controller base address is 0x%08x.\n",
		(unsigned int)priv->base_addr);

	retval = platform_get_irq(pdev, 0);
	if (retval < 0) {
		dev_err(dev, "cic device irq number request failure (err : %d) !\n",
				retval);
		goto  dev_del;
	}
	priv->slot_irq_num = retval;
	dev_dbg(dev, "CI controller interrupt number is %d.\n", retval);

	retval = devm_request_irq(dev, priv->slot_irq_num,
		(irq_handler_t)cic_interrupt_handler,
		 0, cic_dev->name, cic_dev);
	if (retval) {
		dev_err(dev, "cic device irq handler register failure (err : %d) !\n",
			retval);
		goto dev_del;
	}

	/* mutex resource request */
	mutex_init(&cic_dev->lock);

	return 0;

dev_del:
	device_del(cic_dev->dev);
	cic_dev->dev = NULL;
cdev_del:
	cdev_del(&cic_dev->cdev);
cdevno_free:
	unregister_chrdev_region(cic_dev->id, 1);
class_del:
	class_destroy(m_cic_class);
	m_cic_class = NULL;
mem_free:
	devm_kfree(dev, cic_dev);
	cic_dev = NULL;
	return retval;
}

static int ali_cic_remove(struct platform_device *pdev)
{
	struct ali_cic_device *dev = platform_get_drvdata(pdev);
	struct ali_cic_private *priv = NULL;

	if (IS_ERR(dev))
		return PTR_ERR(dev);

	priv = &dev->priv;
	devm_free_irq(&pdev->dev, priv->slot_irq_num, dev);
	mutex_destroy(&dev->lock);

	if (dev->dev != NULL) {
		device_del(dev->dev);
		dev->dev = NULL;
	}
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->id, 1);

	if (m_cic_class != NULL) {
		class_destroy(m_cic_class);
		m_cic_class = NULL;
	}

	devm_kfree(&pdev->dev, dev);
	dev = NULL;

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id ali_cic_of_match[] = {
	{.compatible= "alitech, cic", },
	{},
};
MODULE_DEVICE_TABLE(of, ali_cic_of_match);

static struct platform_driver ali_ci_driver = {
	.driver = {
		.name = ALI_CIC_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ali_cic_of_match,
	},
	.probe = ali_cic_probe,
	.remove = __devexit_p(ali_cic_remove),
};

module_platform_driver(ali_ci_driver);
MODULE_DESCRIPTION("Alitech CI Controller Driver");
MODULE_AUTHOR("Rivaille Zhu");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
