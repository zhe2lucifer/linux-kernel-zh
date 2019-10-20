/*
 * VSC dedicated Key Ladder Core driver
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

#include <linux/io.h>
#include <linux/time.h>
#include <linux/uaccess.h>

#include <ca_kl.h>
#include <ca_kl_vsc.h>
#include "ca_kl_vsc_ioctl.h"
#include "ca_kl_rpc.h"

int ca_kl_key_config(struct ca_kl_session *s,
	struct kl_config_key *cfg_key);

static int ca_ce_generate_vsc_rec_key(struct ca_kl_session *s,
	struct kl_vsc_rec_en_key *pRecEn_key)
{
	int ret;
	CE_REC_KEY_PARAM key_param;
	struct ca_kl_sub_dev *son = NULL;
	if (NULL == pRecEn_key)
		return -EINVAL;

	if (NULL == s)
		return -EBADF;

	if (NULL == s->cell)
		return -EBADF;

	son = s->pdev;
	if (!son)
		return -EBADF;

	memset(&key_param, 0, sizeof(CE_REC_KEY_PARAM));
	memcpy(key_param.en_key, pRecEn_key->en_key, KL_VSC_REC_KEY_SIZE);

	if (pRecEn_key->parity == CE_PARITY_EVEN ||
		pRecEn_key->parity == CE_PARITY_ODD) {
		key_param.pos = (pRecEn_key->parity == CE_PARITY_EVEN) ?
				s->cell->pos : s->cell->pos + 1;

		ret = ali_ce_ioctl(son->parent->see_ce_id,
			(CE_IO_CMD(IO_CRYPT_VSC_REC_EN_KEY)),
			(__u32)&key_param);
		if (0 != ret) {
			dev_err(son->dev, "%s,%d: failed, ret:%d\n",
					__func__, __LINE__, ret);
			return -EIO;
		}
	} else {
		key_param.pos = s->cell->pos;

		ret = ali_ce_ioctl(son->parent->see_ce_id,
			(CE_IO_CMD(IO_CRYPT_VSC_REC_EN_KEY)),
			(__u32)&key_param);
		if (0 != ret) {
			dev_err(son->dev, "%s,%d: failed, ret:%d\n",
					__func__, __LINE__, ret);
			return -EIO;
		}

		key_param.pos = s->cell->pos + 1;

		ret = ali_ce_ioctl(son->parent->see_ce_id,
			(CE_IO_CMD(IO_CRYPT_VSC_REC_EN_KEY)),
			(__u32)&key_param);
		if (0 != ret) {
			dev_err(son->dev, "%s,%d: failed, ret:%d\n",
					__func__, __LINE__, ret);
			return -EIO;
		}
	}

	return 0;
}

static int ca_vsc_decrypt_uk(struct ca_kl_session *s,
	struct kl_vsc_uk_en_key *pUKEn_key)
{
	int ret;
	CE_UK_KEY_PARAM uk_param;
	struct ca_kl_sub_dev *son = NULL;
	if (NULL == pUKEn_key)
		return -EINVAL;

	if (NULL == s)
		return -EBADF;

	if (NULL == s->cell)
		return -EBADF;

	son = s->pdev;
	if (!son)
		return -EBADF;

	memset(&uk_param, 0, sizeof(CE_UK_KEY_PARAM));
	memcpy(uk_param.en_key, pUKEn_key->en_key, KL_VSC_UK_KEY_SIZE);

	uk_param.pos = s->cell->pos;

	ret = ali_ce_ioctl(son->parent->see_ce_id,
		(CE_IO_CMD(IO_CRYPT_VSC_UK_EN_KEY)),
		(__u32)&uk_param);
	if (0 != ret) {
		dev_err(son->dev, "failed, ret:%d\n", ret);
		return -EIO;
	}
	return 0;
}

static int ca_kl_vsc_cw_derive(struct ca_kl_session *s,
	struct kl_cw_derivation *d)
{
	int ret = -EPERM;
	struct ce_cw_derivation rpc_derivation;
	struct kl_key_cell *key_cell = NULL;
	struct kl_key_cell *data_cell = NULL;
	struct kl_key_cell *target_cell = NULL;

	if (!s)
		return -EFAULT;

	if (NULL == d)
		return -EFAULT;

	if (((s->pdev->kl_index&0x0f) != KL_INDEX_0) &&
		((s->pdev->kl_index&0x0f) != KL_INDEX_1)) {
		dev_err(s->pdev->dev, "only vsc0/vsc1 support cw->cw!\n");
		return -EPERM;
	}

	if (!s->is_cfg) {
		dev_err(s->pdev->dev, "please config session first!\n");
		return -EPERM;
	}

	memset(&rpc_derivation, 0, sizeof(rpc_derivation));
	ret = fetch_key_cell_by_fd(d->key.fd, &key_cell);
	if (unlikely(!key_cell)) {
		dev_err(s->pdev->dev, "err fetch key_cell\n");
		return ret;
	}
	rpc_derivation.key.pos = key_cell->pos;

	ret = fetch_key_cell_by_fd(d->data.fd, &data_cell);
	if (unlikely(!data_cell)) {
		dev_err(s->pdev->dev, "err fetch data_cell\n");
		return ret;
	}
	rpc_derivation.data.pos = data_cell->pos;

	ret = fetch_key_cell_by_fd(d->target_fd, &target_cell);
	if (unlikely(!target_cell)) {
		dev_err(s->pdev->dev, "err fetch target_cell\n");
		return ret;
	}
	rpc_derivation.target_pos = target_cell->pos;

	ret = ali_ce_ioctl((CE_DEVICE *)s->pdev->parent->see_ce_id,
			CE_IO_CMD(IO_CRYPT_VSC_CW_DERIVE_CW),
			(__u32)&rpc_derivation);
	if (0 != ret) {
		dev_err(s->pdev->dev, "VSC_CW_DERIVE_CW, ret:%x\n", ret);
		ret = -EIO;
	}

	return ret;
}

long ca_kl_vsc_ioctl(struct file *file,
	unsigned int cmd, unsigned long args)
{
	int ret = 0;
	struct kl_vsc_rec_en_key rec_key;
	struct kl_vsc_uk_en_key uk_key;
	int ck_size=0;
	struct kl_config_key cfg_key;
	struct ca_kl_session *s = NULL;

	s = file2session(file);
	if (!s)
		return -EBADF;

	switch (cmd) {

	case KL_VSC_REC_EN_KEY:
		{
			ret = ali_ce_umemcpy((void *)&rec_key,
				(void __user *)args, sizeof(struct kl_vsc_rec_en_key));
			if (0 != ret) {
				dev_err(s->pdev->dev, "failed, ret:%d\n", ret);
				return -EIO;
			}

			ret = ca_ce_generate_vsc_rec_key(s, &rec_key);
		}
		break;

	case KL_VSC_UK_EN_KEY:
		{
			ret = ali_ce_umemcpy((void *)&uk_key,
				(void __user *)args, sizeof(struct kl_vsc_uk_en_key));
			if (0 != ret) {
				dev_err(s->pdev->dev, "failed, ret:%d\n", ret);
				return -EIO;
			}	

			ret = ca_vsc_decrypt_uk(s, &uk_key);
		}
		break;

	case KL_CONFIG_KEY:
		{			
			ret = ali_ce_umemcpy((void *)&cfg_key,
					(void __user *)args,
					sizeof(struct kl_config_key));
			if (0 != ret) {
				dev_err(s->pdev->dev, "failed, ret:%d\n", ret);
				return -EIO;
			}

			if (s->is_cfg) {
				dev_dbg(s->pdev->dev, "already config key\n");
				return 0;
			}
			//only can set ck_size to get free key pos
			ck_size = cfg_key.ck_size;
			memset(&cfg_key,0x0,sizeof(struct kl_config_key));
			cfg_key.ck_size = ck_size;
			
			ret = ca_kl_key_config(s, &cfg_key);
		}
		break;

	case KL_DERIVE_CW:
		{
			struct kl_cw_derivation cw_derivation;
			ret = ali_ce_umemcpy((void *)&cw_derivation,
				(void __user *)args,
				sizeof(struct kl_cw_derivation));
			if (0 != ret) {
				dev_err(s->pdev->dev, "failed, ret:%d\n", ret);
				return -EIO;
			}

			ret = ca_kl_vsc_cw_derive(s, &cw_derivation);
		}
		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
