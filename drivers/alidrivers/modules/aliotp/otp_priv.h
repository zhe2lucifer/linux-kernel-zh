/*
 * One Time Programming Core driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
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

#ifndef __OTP_PRIV_H__
#define __OTP_PRIV_H__

#include <linux/cdev.h>
#include <linux/platform_device.h>

#define OTP_MAX_MEM_SIZE 0x400

#define OTP_ADDR (0x04)
#define OTP_WDATA (0x08)
#define OTP_CTRL (0x0c)
#define OTP_STATUS (0x10)
#define OTP_INT (0x10)
#define OTP_RDATA (0x18)
#define OTP_SCTRL (0x74)
#define OTP_64BIT_W_DATA0	(0x100)
#define OTP_64BIT_W_DATA1	(0x104)
#define OTP_64BIT_R_DATA0	(0x110)
#define OTP_64BIT_R_DATA1	(0x114)
#define OTP_FP_BASE (0x438)
#define OTP_MRKCN_BASE (0x500)

#define OTP_VENDOR_ID_HIGH	(0x11c)
#define OTP_VENDOR_ID_LOW	(0x118)

#define OTP_BIT_READ_BUSY	(1<<8)
#define OTP_BIT_READ_TRIG	(1<<8)
#define OTP_BIT_PROG_BUSY	(1<<0)
#define OTP_BIT_PROG_TRIG	(1<<0)
#define OTP_BIT_PROG_FAIL	(1<<8)
#define OTP_BIT_INT_ENABLE	(1<<0)
#define OTP_BIT_IOPRIVATE_ENABLE	(1<<16)

#if 0
struct otp_registers {
	unsigned int ver_id;
	unsigned int addr;
	unsigned int wdata;
	unsigned int ctrl;
	unsigned int status;
	unsigned int interrupt;
	unsigned int rdata;
	unsigned int rtim;
	unsigned int ptim0;
	unsigned int ptim1;
	unsigned int ptim2;
	unsigned int rsvd;
	unsigned int timing;
	unsigned int sdata;
	unsigned int strig;
	unsigned int ssts;
	unsigned int s_02;	/*0x40*/
	unsigned int s_03;
	unsigned int s_5f;
	unsigned int s_80;
	unsigned int s_81;
	unsigned int ptimes;
	unsigned int reserved[4];
	unsigned int s_de;
	unsigned int s_82;
	unsigned int s_83;
	unsigned int s_ctrl;/*0x74*/
	unsigned int s_dc;
	unsigned int s_dd;
	unsigned int s_e4;
	unsigned int s_e5;
	unsigned int s_e6;
	unsigned int s_e7;
	unsigned int s_8c;	/*0x90*/
	unsigned int s_8d;
	unsigned int s_85;
	unsigned int reserved2[21];
	unsigned int wdatalow;/*0x100, 64bit-mode*/
	unsigned int wdatahigh;
	unsigned int status2;/*64bit mode status*/
	unsigned int rsvd2;
	unsigned int rdatalow;/*0x110, 64bit-mode*/
	unsigned int rdatahigh;
	unsigned int vidlow;
	unsigned int vidhigh;
	unsigned int kdf_ctrl;/*0x120*/
	unsigned int kdf_sts;
	unsigned int reserved3[246];
	unsigned int cn0;/*0x500*/
	unsigned int cn1;
};
#endif

struct otp_vendor_id {
	unsigned int HighVID;
	unsigned int LowVID;
	unsigned char isValid;
};

/*
 * Device structure
 */
struct otp_dev {
	dev_t  devt;
	struct class *class;
	struct device *dev;
	struct cdev cdev;
	void *base;
	unsigned char obuf[OTP_MAX_MEM_SIZE];

	struct mutex mutex;
	int memsize;
	int num_inst;
};

int otp_hw_read(struct otp_dev *otp,
	unsigned long offset, unsigned char *buf, int len);
int otp_hw_write(struct otp_dev *otp,
	unsigned long offset, unsigned char *buf, int len);

void otp_enable_io_private(struct otp_dev *otp);
void otp_legacy_interface_support(struct otp_dev *otp, int support);
int otp_std_kdf_trigger(struct otp_dev *otp, int root, int text_idx);
int otp_etsi_kdf_trigger(struct otp_dev *otp, struct otp_vendor_id *pvid);

#endif /*__OTP_PRIV_H__*/

