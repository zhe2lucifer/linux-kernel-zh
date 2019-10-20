/*
 * One Time Programming Core driver
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

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <ali_soc.h>

#include "otp_priv.h"

#define IS_BELONG_64BIT_REGION(addr) (((addr>=0x4d*4) && (addr<0x5e*4) && !((addr-(0x4d*4))%8)) \
			|| ((addr>=0x60*4) && (addr<0x6f*4) && !((addr-(0x60*4))%8)))

static int otp_read_busy(struct otp_dev *otp)
{
	unsigned int live = 1000;

	while ((ioread32(otp->base + OTP_STATUS) &
			OTP_BIT_READ_BUSY) &&
			(live--))
		msleep_interruptible(1);

	return !live;
}

static int otp_write_busy(struct otp_dev *otp)
{
	unsigned int live = 1000;

	while ((ioread32(otp->base + OTP_STATUS) &
			OTP_BIT_PROG_BUSY) &&
			(live--))
		msleep_interruptible(1);

	return !live;
}

static unsigned int otp_read_ddw(struct otp_dev *otp,
	unsigned short addr, unsigned int *buf)
{
	if (!otp || !buf)
		return -EINVAL;
	
	if (otp_read_busy(otp) || otp_write_busy(otp))
		return -EBUSY;
		
	iowrite32(addr & (otp->memsize - 1),
			otp->base + OTP_ADDR);
	iowrite32(ioread32(otp->base + OTP_CTRL) | OTP_BIT_READ_TRIG,
			otp->base + OTP_CTRL);
	if (otp_read_busy(otp))
		return -EBUSY;
	
	iowrite32(ioread32(otp->base + OTP_CTRL) & (~(0x3 << 24)), 
			otp->base + OTP_CTRL);
	iowrite32(ioread32(otp->base + OTP_CTRL) | (0x1 << 24), 
			otp->base + OTP_CTRL);
			
	iowrite32(ioread32(otp->base + OTP_CTRL) | OTP_BIT_READ_TRIG,
			otp->base + OTP_CTRL);
	if (otp_read_busy(otp))
		return -EBUSY;
	
	buf[0] = ioread32(otp->base + OTP_64BIT_R_DATA0);
	buf[1] = ioread32(otp->base + OTP_64BIT_R_DATA1);
	
	iowrite32(ioread32(otp->base + OTP_CTRL) & (~(0x3 << 24)), 
			otp->base + OTP_CTRL);
	
	return 0;
}

static unsigned int otp_write_ddw(struct otp_dev *otp,
	unsigned short addr, unsigned int *buf)
{
	if (!otp || !buf)
		return -EINVAL;
	
	if (otp_read_busy(otp) || otp_write_busy(otp))
		return -EBUSY;
		
	iowrite32(addr & (otp->memsize - 1),
			otp->base + OTP_ADDR);
	
	iowrite32(ioread32(otp->base + OTP_CTRL) & (~(0x3 << 24)), 
			otp->base + OTP_CTRL);
	iowrite32(ioread32(otp->base + OTP_CTRL) | (0x1 << 24), 
			otp->base + OTP_CTRL);

	iowrite32(buf[0], otp->base + OTP_64BIT_W_DATA0);
	iowrite32(buf[1], otp->base + OTP_64BIT_W_DATA1);
			
	iowrite32(ioread32(otp->base + OTP_CTRL) | OTP_BIT_PROG_TRIG,
			otp->base + OTP_CTRL);
	if (otp_write_busy(otp))
		return -EBUSY;
	
	iowrite32(ioread32(otp->base + OTP_CTRL) & (~(0x3 << 24)), 
			otp->base + OTP_CTRL);
	
	return 0;
}


static unsigned int otp_read_dword(struct otp_dev *otp,
	unsigned short addr, unsigned int *data)
{
	if (!otp || !data)
		return -EINVAL;

	if (otp_read_busy(otp) || otp_write_busy(otp))
		return -EBUSY;

	iowrite32(addr & (otp->memsize - 1),
			otp->base + OTP_ADDR);
	iowrite32(ioread32(otp->base + OTP_CTRL) | OTP_BIT_READ_TRIG,
			otp->base + OTP_CTRL);
	if (otp_read_busy(otp))
		return -EBUSY;

	*data = ioread32(otp->base + OTP_RDATA);
	return 0;
}

static int otp_write_byte(struct otp_dev *otp,
	unsigned short addr, unsigned char data)
{
	if (!otp)
		return -EINVAL;

	if (otp_read_busy(otp) || otp_write_busy(otp))
		return -EBUSY;

	iowrite32(addr & (otp->memsize - 1),
			otp->base + OTP_ADDR);
	iowrite8(data, otp->base + OTP_WDATA);

	iowrite32(ioread32(otp->base + OTP_CTRL) | OTP_BIT_PROG_TRIG,
			otp->base + OTP_CTRL);

	if (otp_write_busy(otp))
		return -EBUSY;

	if (ioread32(otp->base + OTP_INT) & OTP_BIT_INT_ENABLE)
		if (ioread32(otp->base + OTP_INT) & OTP_BIT_PROG_FAIL)
			return -EIO;

	return 0;
}

int otp_hw_read(struct otp_dev *otp,
	unsigned long offset, unsigned char *buf, int len)
{
	unsigned int i = 0;
	int ret = -EIO;
	int rdata = 0;
	int need_64bit = 0;
	unsigned long chip_id = ali_sys_ic_get_chip_id();
	unsigned long chip_rev = ali_sys_ic_get_rev_id();
	unsigned int tmp_offset = offset & (~0x3);
	unsigned int tmp_len = 0;

	if (((ALI_S3921 == chip_id) && (chip_rev >= IC_REV_2)) || \
		(ALI_CAP210 == chip_id) || (ALI_C3505 == chip_id) || \
		(ALI_C3922 == chip_id) || (ALI_S3922 == chip_id))
	{
		need_64bit = 1;
	}

	if (0 == (len % 4))
		tmp_len = (len & (~0x3))/4;
	else
		tmp_len = (len & (~0x3))/4 + 1;

	if (!otp || (NULL == buf) || (0 == len) ||
		((offset + len) > otp->memsize)) {
		dev_err(otp->dev, "invalid param\n");
		return -EINVAL;
	}

	for (i = 0; i < tmp_len;) {
		if(need_64bit && IS_BELONG_64BIT_REGION(tmp_offset + i*4))
		{
			otp_read_ddw(otp, (tmp_offset + i*4), (unsigned int *)&otp->obuf[i*4]);
			i += 2;
		}
		else
		{
			ret = otp_read_dword(otp,
					tmp_offset + i*4,
					&rdata);
			if (ret) {
				dev_err(otp->dev,
					"read failed, ret[%d], ofset[0x%x]\n",
					ret, tmp_offset + i);
				return ret;
			} else {
				*((unsigned int *)otp->obuf + i) = rdata;
				i++;
			}
		}
	}

	i = offset - tmp_offset;
	memcpy(buf, &otp->obuf[i], len);

	return len;
}

int otp_hw_write(struct otp_dev *otp,
	unsigned long offset, unsigned char *buf, int len)
{
	int i = 0;
	int ret = -EIO;
	int need_64bit = 0;
	unsigned long chip_id = ali_sys_ic_get_chip_id();
	unsigned long chip_rev = ali_sys_ic_get_rev_id();
	unsigned int tmp_offset = offset & (~0x3);

	if (!otp || (NULL == buf) || (0 == len) ||
		((offset + len) > otp->memsize)) {
		dev_err(otp->dev, "invalid param\n");
		return -EINVAL;
	}

	if (((ALI_S3921 == chip_id) && (chip_rev >= IC_REV_2)) || \
		(ALI_CAP210 == chip_id) || (ALI_C3505 == chip_id) || \
		(ALI_C3922 == chip_id) || (ALI_S3922 == chip_id))
	{
		need_64bit = 1;
	}

	if(IS_BELONG_64BIT_REGION(offset))
	{
		if(offset != tmp_offset)
		{
			dev_err(otp->dev, "invalid param,it's 64bit region\n");
			return -EINVAL;
		}
	}

	dev_dbg(otp->dev, "buf[0x%p], offset[0x%lx], len[0x%x]\n",
		buf, offset, len);

	
	while (i < len) {
		if(need_64bit && IS_BELONG_64BIT_REGION(offset))
		{
			ret = otp_write_ddw(otp, offset, (unsigned int *)buf);
			
			if (ret) {
				dev_err(otp->dev, "otp program err, addr[0x%lx]\n",
						offset);
				iowrite32(ioread32(otp->base + OTP_INT) &
						~OTP_BIT_PROG_FAIL,
						otp->base + OTP_INT);
				break;
			} else {
				offset += 8;
				buf += 8;
				i += 8;
			}
		}
		else
		{
			ret = otp_write_byte(otp, offset++, *buf++);
			if (ret) {
				dev_err(otp->dev, "otp program err, addr[0x%lx]\n",
						offset);
				iowrite32(ioread32(otp->base + OTP_INT) &
						~OTP_BIT_PROG_FAIL,
						otp->base + OTP_INT);
				break;
			} else {
				i++;
			}
		}
	}

	if (i != len)
		return ret;

	return len;
}


void otp_enable_io_private(struct otp_dev *otp)
{
	if (!otp)
		return;
	iowrite32(ioread32(otp->base + OTP_SCTRL) |
					OTP_BIT_IOPRIVATE_ENABLE,
					otp->base + OTP_SCTRL);
}

/**********************************************************************
	legacy OTP interface ....
*/
static struct otp_dev *g_otp;

void otp_legacy_interface_support(struct otp_dev *otp, int support)
{
	if (!support) {
		g_otp = NULL;
		return;
	}

	g_otp = otp;
}

int ali_otp_hw_init(void)
{
	return (g_otp == NULL) ? -1 : 0;
}
EXPORT_SYMBOL(ali_otp_hw_init);

int ali_otp_read(unsigned long offset, unsigned char *buf, int len)
{
	if (!g_otp)
		return -1;

	return otp_hw_read(g_otp, offset, buf, len);
}
EXPORT_SYMBOL(ali_otp_read);

int ali_otp_write(unsigned char *buf, unsigned long offset, int len)
{
	if (!g_otp)
		return -1;

	return otp_hw_write(g_otp, offset, buf, len);
}
EXPORT_SYMBOL(ali_otp_write);

unsigned int otp_get_value(unsigned int addr)
{
	unsigned int data = 0;

	if (!g_otp)
		return -1;

	otp_read_dword(g_otp, addr, &data);

	return data;
}

int otp_c0_mem_split_enabled(void)
{
	if (!g_otp)
		return -1;

	return (ioread32(g_otp->base + 0x44) >> 6) & 0x01;
}

int otp_c0_sip_nor_enabled(void)
{
	if (!g_otp)
		return -1;
	return (ioread32(g_otp->base + 0x44) >> 29) & 0x01;
}


int otp_c0_as_bootrom_enabled(void)
{
	if (!g_otp)
		return -1;
	//0x03[1]
	return (ioread32(g_otp->base + 0x44) >> 1) & 0x01;	
}

int ali_sys_ic_split_enabled(void)
{
   int spl_enabled = 0;

   spl_enabled = otp_c0_mem_split_enabled();
   
   return spl_enabled;
}
int ali_sys_ic_sip_nor_enabled(void)
{
   int sip_enabled = 0;

   sip_enabled = otp_c0_sip_nor_enabled();
   
   return sip_enabled;
}


