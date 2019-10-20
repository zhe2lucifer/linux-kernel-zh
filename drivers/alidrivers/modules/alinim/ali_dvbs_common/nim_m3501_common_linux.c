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

#include "../porting_linux_header.h"
#include "porting_m3501_linux.h"
#include <linux/ali_kumsgq.h>

static INT32 					fd_dmx;

RET_CODE nim_send_as_msg(struct kumsgq * nim_kumsgq, unsigned char lck, unsigned char polar, unsigned short freq, unsigned int sym, unsigned char fec, unsigned char as_stat)
{
    RET_CODE ret = RET_SUCCESS;
    unsigned char msg[13]={0};
    int msg_size = 13;

    memset((void *)msg, 0x00, msg_size);
    msg[0] = 0x01;
    msg[1] = 0;
    msg[2] = 9;
    memcpy((void *)(msg + 3), &lck, 1);
    memcpy((void *)(msg + 4), &polar, 1);
    memcpy((void *)(msg + 5), &fec, 1);
    memcpy((void *)(msg + 6), &freq, 2);
    memcpy((void *)(msg + 8), &sym, 4);
    memcpy((void *)(msg + 12), &as_stat, 1);
   	ali_kumsgq_sendmsg(nim_kumsgq, msg, msg_size);
	return ret;
}


__u32 dvbs_as_cb2_ui(void *p_priv, __u8 lck, __u8 polar, __u16 freq, __u32 sym, __u8 fec, __u8 as_stat)
{
    struct nim_s3501_private *priv = (struct nim_s3501_private *)p_priv;
    priv->as_status = 0;

    nim_send_as_msg(priv->nim_kumsgq, lck, polar, freq, sym, fec, as_stat);
    wait_event_interruptible_timeout(priv->as_sync_wait_queue, priv->as_status & 0x01, 0x7fffffff);
    priv->as_status = 0;

    return SUCCESS;
}

INT32 nim_s3501_autoscan_signal_input(struct nim_device *dev, UINT8 s_case)
{
    struct nim_s3501_private *priv = (struct nim_s3501_private *) dev->priv;
	
    fd_dmx = sys_open((const char __user *)"/dev/ali_m36_dmx_0", O_RDWR, 0);
    if (fd_dmx< 0)
    {
		nim_print(KERN_WARNING "Warning: Unable to open an dmx.\n");
		return -1;
    }	

    switch (s_case)
    {
    case NIM_SIGNAL_INPUT_OPEN:
        sys_ioctl(fd_dmx, ALI_DMX_IO_SET_BYPASS_MODE, (UINT32) (priv->ul_status.adcdata));//Set bypass mode
        break;
    case NIM_SIGNAL_INPUT_CLOSE:
        sys_ioctl(fd_dmx, ALI_DMX_IO_CLS_BYPASS_MODE, (UINT32)(NULL));//Clear bypass mode.
        break;
    }

    if (sys_close(fd_dmx))
    {
        nim_print("%s in:Error closing the dmx_hdl.\n", __FUNCTION__);
    }
    return SUCCESS;
}

INT32 nim_callback(NIM_AUTO_SCAN_T *pst_auto_scan, void *pfun, UINT8 status, UINT8 polar, UINT16 freq, UINT32 sym, UINT8 fec, UINT8 stop)
{
    return pst_auto_scan->callback(pfun, status, polar, freq, sym, fec, stop);
}



