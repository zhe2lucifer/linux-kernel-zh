/*
 * DeScrambler Core driver
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/splice.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/ali_dsc.h>

#include <ca_dsc.h>
#include "ca_dsc_priv.h"
#include "ca_dsc_rpc.h"

static int ca_dio_fake_rw_check(struct ca_dsc_session *s,
	struct ca_dio_write_read *pdio)
{
	if (!s)
		return -EBADF;

	if (!s->fmt_flag)
		return -EPERM;

	if (!pdio || !pdio->input || !pdio->output || !pdio->length)
		return -EINVAL;

	if (pdio->crypt_mode != CA_ENCRYPT &&
		pdio->crypt_mode != CA_DECRYPT) {
		dev_dbg(s->dsc->dev, "invalid crypt_mode[%d]\n",
			pdio->crypt_mode);
		return -EINVAL;
	}

	if (s->crypt_mode != pdio->crypt_mode) {
		/*change the session crypto_mode here ?*/
		s->crypt_mode = pdio->crypt_mode;
	}

	return 0;
}

static int ca_dio_fake_rw(struct file *file,
	struct ca_dio_write_read *pdio)
{
	int ret;
	struct ca_dsc_session *s = file->private_data;
	int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
	int wr_pos = 0;
	int rd_pos = 0;
	int m = 0;

	ret = ca_dio_fake_rw_check(s, pdio);
	if (ret)
		return ret;

	if (blocking)
		file->f_flags |= O_NONBLOCK;

	dev_dbg(s->dsc->dev, "input[%p],output[%p],len[%d]\n",
		pdio->input, pdio->output, pdio->length);

	/*cork*/
	if (s->ts_chaining != CHAINING_NONE) {
		ret = file->f_op->unlocked_ioctl(file,
			CA_SET_OPT, CA_SET_CORK);
		if (ret) {
			dev_dbg(s->dsc->dev, "set cork err %d\n", ret);
			return ret;
		}
	}

	while (wr_pos < pdio->length) {
		m = file->f_op->write(file, &pdio->input[wr_pos],
			pdio->length - wr_pos, NULL);
		if (m < 0) {
			if (m == -EAGAIN)
				goto read;

			dev_dbg(s->dsc->dev, "%s:%d: %d\n", __func__,
				__LINE__, m);

			return m;
		}
		wr_pos += m;

		/*uncork if all buffer written*/
		if (s->ts_chaining != CHAINING_NONE &&
			wr_pos >= pdio->length) {
			ret = file->f_op->unlocked_ioctl(file,
				CA_SET_OPT, CA_SET_UNCORK);
			if (ret) {
				dev_dbg(s->dsc->dev, "set uncork err %d\n", ret);
				return ret;
			}
		}
	read:
		while (rd_pos < wr_pos) {
			int len = wr_pos - rd_pos;
			int k;

			k = file->f_op->read(file, &pdio->output[rd_pos],
					len, NULL);
			if (k < 0) {
				if (k == -EAGAIN) {
					if (wr_pos < pdio->length)
						break;
					else
						continue;
				}

				dev_dbg(s->dsc->dev, "%s:%d: %d\n", __func__,
					__LINE__, k);

				return k;
			}

			rd_pos += k;

			dev_dbg(s->dsc->dev, "rd_pos:%d,wr_pos:%d\n",
				rd_pos, wr_pos);
		}
	}

	if (blocking)
		file->f_flags &= ~O_NONBLOCK;

	return 0;
}

long ca_dsc_ioctl_legacy(struct file *file, unsigned int cmd,
			 unsigned long args)
{
	int ret = 0;
	struct ca_dsc_session *s = file->private_data;

	if (!s)
		return -EBADF;

	switch (cmd) {
	case CA_DIO_WRITE_READ:
	{
		struct ca_dio_write_read dio;
		memset(&dio, 0, sizeof(struct ca_dio_write_read));
		ret = ali_dsc_umemcpy(&dio, (void __user *)args,
			sizeof(struct ca_dio_write_read));
		if (0 != ret) {
			dev_dbg(s->dsc->dev, "%s\n", __func__);
			goto exit;
		}

		ret = ca_dio_fake_rw(file, &dio);
		if (ret < 0)
			goto exit;

		break;
	}

	case CA_CALC_CMAC:
	{
		struct ca_dsc_dev *dsc = s->dsc;
		struct ca_calc_cmac calc_cmac_info;
		memset(&calc_cmac_info, 0, sizeof(struct ca_calc_cmac));
		ret = ali_dsc_umemcpy(&calc_cmac_info, (void __user *)args,
		sizeof(struct ca_calc_cmac));
		if (0 != ret)
		{
			dev_err(s->dsc->dev, "%s\n", __func__);
			goto exit;
		}

		ret = ali_dsc_ioctl(s->dsc, (DSC_DEV *)dsc->see_dsc_id,DSC_IO_CMD(IO_DSC_CALC_CMAC), (__u32)&calc_cmac_info);
		if (ret < 0)
		{
			goto exit;
		}

		break;
	}
#ifdef CONFIG_USER_SPACE_MEMORY_INTEGRITY_CHECKING
	case CA_DSC_IO_TRIG_RAM_MON:
	{
		struct ca_dsc_ram_mon ram_mon;                                                                                                                                            
        ret = ali_dsc_umemcpy((void *)&ram_mon, (void __user *)args, \
                                sizeof(struct ca_dsc_ram_mon));
        if (0 != ret) 
        {    
            dev_err(s->dsc->dev, "Err:ali_dsc_umemcpy() failed, ret:%d\n", ret);
            goto exit;
        }    
        ret = ali_trig_ram_mon((__u32)ram_mon.start_addr, \
                            	(__u32)ram_mon.end_addr, \
                            	0, 0, 0);
		break;
	}
#endif
	default:
		ret = -ENOIOCTLCMD;
	}

exit:
	return ret;
}





//---------------------------------------------------------------------------
//The interface which export for the other kernel modules.
//---------------------------------------------------------------------------

__s32 ca_dsc_get_session_attr(__s32 ca_fd, struct ca_session_attr* p_ca_attr)
{
	__s32 ret = 0;
	struct ca_dsc_session *s = NULL;
	struct fd k_fd;

	if (!p_ca_attr)
		return -EINVAL;

	k_fd = fdget(ca_fd);
	if(!k_fd.file)
	{
		ret = -EBADF;
		goto ERR2;
	}

	s = k_fd.file->private_data;
	if (!s)
	{
		ret = -EFAULT;
		goto ERR2;
	}

	if (ALI_INVALID_DSC_SUB_DEV_ID == s->sub_dev_id) {
		dev_err(s->dsc->dev, "this session haven't init yet!\n");
		ret = -EFAULT;
		goto ERR2;
	}

	if(!s->dsc)
	{
		ret = -EFAULT;
		goto ERR2;
	}
	mutex_lock(&s->dsc->mutex);


	s = k_fd.file->private_data;
	if (!s)
	{
		p_ca_attr->sub_dev_see_hdl = NULL;
		
		ret = -EFAULT;
		goto ERR1;
	}

	switch (s->algo) {
	case CA_ALGO_AES:
		p_ca_attr->crypt_mode = AES;
		break;
	case CA_ALGO_DES:
		p_ca_attr->crypt_mode = DES;
		break;
	case CA_ALGO_TDES:
		p_ca_attr->crypt_mode = TDES;
		break;
	case CA_ALGO_CSA1:
	case CA_ALGO_CSA2:
	case CA_ALGO_CSA3:
		p_ca_attr->crypt_mode = CSA;
		break;
	default:
		ret = -EINVAL;
		goto ERR1;
	}
	
	p_ca_attr->sub_dev_see_hdl = (void*)(s->sub_dev_see_hdl);
	p_ca_attr->stream_id = s->stream_id;
    p_ca_attr->sub_dev_id = s->sub_dev_id;

ERR1:
	mutex_unlock(&s->dsc->mutex);
ERR2:
	fdput(k_fd);

	return ret;
}
EXPORT_SYMBOL(ca_dsc_get_session_attr);


__s32 ca_dsc_get_key_attr(__s32 ca_fd, __s32 key_id, struct ca_key_attr* p_key_attr)
{
	__s32 ret = 0;
	struct fd k_fd;
	struct ca_dsc_session *s = NULL;
	struct ali_inst_key *key = NULL;

	if(!p_key_attr)
		return -EINVAL;

	k_fd = fdget(ca_fd);
	if(!k_fd.file)
	{
		ret = -EBADF;
		goto ERR2;
	}

	s = k_fd.file->private_data;
	if (!s)
	{
		ret = -EFAULT;
		goto ERR2;
	}

	if(!s->dsc)
	{
		ret = -EFAULT;
		goto ERR2;
	}
	mutex_lock(&s->dsc->mutex);

	s = k_fd.file->private_data;
	if (!s)
	{
		ret = -EFAULT;
		goto ERR1;
	}
	
	/*find the key*/
	key = inst_key_find_by_handle(s,(key_id & DSC_INST_KEY_ID_MASK));
	if (!key) {
		dev_err(s->dsc->dev, "key_handle[0x%x] is not found!!\n",key_id);
		ret = -EFAULT;
		goto ERR1;
	}

	p_key_attr->key_handle = key->key_handle;
	if(key->cell)
	{
		p_key_attr->kl_sel = key->cell->kl_sel;
		p_key_attr->key_pos = key->cell->pos;
	}

ERR1:
	mutex_unlock(&s->dsc->mutex);
ERR2:
	fdput(k_fd);

	return ret;
}
EXPORT_SYMBOL(ca_dsc_get_key_attr);
