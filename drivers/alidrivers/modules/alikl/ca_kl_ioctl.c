/*
 * Key Ladder Core driver
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
#include "ca_kl_ioctl.h"
#include "ca_kl_ioctl_legacy.h"
#include "ca_kl_priv.h"
#include "ca_kl_rpc.h"


static int kl_free_key_pos(struct ca_kl_sub_dev *son, int key_pos)
{
	int ret;

	ret = ali_ce_ioctl(son->parent->see_ce_id,
		CE_IO_CMD(IO_CRYPT_POS_SET_IDLE), (__u32)key_pos);
	if (0 != ret) {
		dev_dbg(son->dev, "IO_CRYPT_POS_SET_IDLE, ret:%x\n", ret);
		return -EIO;
	}

	return 0;
}

static int kl_put_key_cell(struct kl_key_cell *cell)
{
	int ret;
	struct ca_kl_sub_dev *son = NULL;

	if (!cell)
		return -EINVAL;

	if (!cell->valid)
		return -EINVAL;

	BUG_ON(atomic_read(&cell->_count) <= 0);
	atomic_dec(&cell->_count);

	if (atomic_read(&cell->_count) == 0) {

		son = (struct ca_kl_sub_dev *)cell->private_data;
		if (!son)
			return -ENODEV;

		ret = kl_free_key_pos(son, cell->pos);
		if (ret)
			return ret;

		if (cell->num == 2) {
			ret = kl_free_key_pos(son, cell->pos + 1);
			if (ret)
				return ret;
		}

		memset(cell, 0, sizeof(struct kl_key_cell));
		kfree(cell);
	}

	return 0;
}


static int kl_alloc_key_cell(struct ca_kl_session *s)
{
	unsigned char key_num;
	int ret = RET_FAILURE;
	CE_FOUND_FREE_POS_PARAM key_pos_param;
	struct kl_key_cell *cell = NULL;

	cell = kzalloc(sizeof(struct kl_key_cell), GFP_KERNEL);
	if (!cell)
		return -ENOMEM;

	key_num = (s->ck_size == KL_CK_KEY128) ? 2 : 1;

	memset(&key_pos_param, 0, sizeof(CE_FOUND_FREE_POS_PARAM));
	key_pos_param.ce_key_level = s->level;
	key_pos_param.pos = KL_INVALID_KEY_POS;
	key_pos_param.root = s->pdev->kl_index;
	key_pos_param.number = key_num;
	ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
		(CE_IO_CMD(IO_CRYPT_FOUND_FREE_POS)), (__u32)&key_pos_param);
	if (0 != ret) {
		dev_dbg(s->pdev->dev, "IO_CRYPT_FOUND_FREE_POS, ret:%x\n", ret);
		ret = -EIO;
		goto errout;
	}

	if (key_pos_param.pos == KL_INVALID_KEY_POS) {
		dev_dbg(s->pdev->dev, "find free key pos fail!\n");
		ret = -EIO;
		goto errout;
	}

	cell->valid = 1;
	
	/**
	 * In legacy IC such as C3921B, the pos return from the IO_CRYPT_FOUND_FREE_POS 
	 * does not have key_sel value.
	 */
	cell->kl_sel = s->pdev->kl_index;
	cell->pos = key_pos_param.pos;
	cell->num = key_num;
	cell->dev = s->pdev->dev;
	cell->ck_parity = s->ck_parity;
	cell->ck_size = s->ck_size;
	cell->private_data = (void *)s->pdev;
	cell->put_cell = kl_put_key_cell;
	atomic_inc(&cell->_count);

	s->cell = cell;
	dev_dbg(s->pdev->dev, "find pos[%x]\n", s->cell->pos);

	return 0;

errout:
	kfree(cell);
	return ret;
}


/*
	sanity check && config key
*/
int ca_kl_key_config(struct ca_kl_session *s,
	struct kl_config_key *cfg_key)
{
	int ret;

	if (NULL == cfg_key) {
		dev_dbg(s->pdev->dev, "argument NULL\n");
		return -EINVAL;
	}

	if (s->is_cfg) {
		dev_dbg(s->pdev->dev, "session configured already!\n");
		return -EPERM;
	}

	if (cfg_key->crypt_mode != KL_ENCRYPT &&
		cfg_key->crypt_mode != KL_DECRYPT) {
		dev_dbg(s->pdev->dev, "invalid crypt_mode[%d]\n",
			cfg_key->crypt_mode);
		return -EINVAL;
	}

	if (cfg_key->algo != KL_ALGO_TDES &&
		cfg_key->algo != KL_ALGO_AES) {
		dev_dbg(s->pdev->dev, "invalid algo[%d]\n",
			cfg_key->algo);
		return -EINVAL;
	}

	if (cfg_key->ck_size != KL_CK_KEY64 &&
		cfg_key->ck_size != KL_CK_KEY128) {
		dev_dbg(s->pdev->dev, "invalid ck_size[%d]\n",
			cfg_key->ck_size);
		return -EINVAL;
	}

	if (cfg_key->ck_parity < 0 ||
		cfg_key->ck_parity > 2) {
		dev_dbg(s->pdev->dev, "invalid ck_parity[%d]\n",
			cfg_key->ck_parity);
		return -EINVAL;
	}

	if (cfg_key->level > s->pdev->support_level) {
		dev_dbg(s->pdev->dev,
			"invalid level[%d], support_level:%d\n",
			cfg_key->level, s->pdev->support_level);

		return -EINVAL;
	}

	s->algo = cfg_key->algo;
	s->crypt_mode = cfg_key->crypt_mode;
	s->ck_size = cfg_key->ck_size;
	s->level = cfg_key->level;
	s->ck_parity = cfg_key->ck_parity;

	/*hdcp keyladder(kl5) don't need alloc key cell*/
	if (!s->pdev->is_hdcp) {
		/*alloc key cell && get free target pos*/
		ret = kl_alloc_key_cell(s);
		if (ret != 0) {
			dev_err(s->pdev->dev, "alloc_key_cell() error!\n");
			return ret;
		}
	}
	else
	{
		/*hdcp key can be only configed to aes decrypt*/
		s->algo = KL_ALGO_AES;
		s->crypt_mode = KL_DECRYPT;
		s->level = 1;
	}

	

	s->is_cfg = 1;
	return 0;
}

static int ca_kl_key_gen(struct ca_kl_session *s,
	struct kl_gen_key *gen_key)
{
	int ret;
	int i;
	CE_NLEVEL_PARAM ce_nlevel_param;

	if (NULL == gen_key)
		return -EINVAL;

	if (s->pdev->is_hdcp) {
		dev_err(s->pdev->dev, "hdcp keyladder(kl5) can't generate normal key!\n");
		return -EPERM;
	}

	if (!s->is_cfg) {
		dev_dbg(s->pdev->dev, "please config session first!\n");
		return -EPERM;
	}

	s->cell->ck_parity = gen_key->run_parity;

	memcpy(&s->gen_key, gen_key, sizeof(struct kl_gen_key));

	memset(&ce_nlevel_param, 0, sizeof(ce_nlevel_param));
	ce_nlevel_param.algo = s->algo;
	ce_nlevel_param.crypto_mode = s->crypt_mode;
	ce_nlevel_param.kl_index = s->cell->kl_sel;
	ce_nlevel_param.otp_addr = s->pdev->root_key_addr;
	
	if (s->level > 1)
	{
		ce_nlevel_param.protecting_key_num = s->level - 1;
		for (i=0; i<ce_nlevel_param.protecting_key_num; i++)
		{
			memcpy(&(ce_nlevel_param.protecting_key[i*KL_KEY_SIZE_MAX]), 
				gen_key->pk[i], KL_KEY_SIZE_MAX);
		}
	}
	else
	{
		ce_nlevel_param.protecting_key_num = 0;
	}

	switch(gen_key->run_parity)
	{
		case KL_CK_PARITY_EVEN:
			{
				ce_nlevel_param.pos = s->cell->pos;

				if ((KL_ALGO_TDES == ce_nlevel_param.algo) && (KL_CK_KEY64 == s->ck_size))
				{
					//gen 64bit even key...
					ce_nlevel_param.parity = CE_PARITY_EVEN;
					memcpy(&(ce_nlevel_param.content_key[8]), gen_key->key_even, 8);
				}
				else
				{
					//gen 128bit even key...
					ce_nlevel_param.parity = CE_PARITY_EVEN_ODD;
					memcpy(ce_nlevel_param.content_key, gen_key->key_even, KL_KEY_SIZE_MAX);
				}

				ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
					CE_IO_CMD(IO_CRYPT_GEN_NLEVEL_KEY), (__u32)(&ce_nlevel_param));
				if (0 != ret) {
					dev_dbg(s->pdev->dev, "IO_CRYPT_GEN_NLEVEL_KEY, ret:%x\n", ret);
					return -EIO;
				}
			}
			break;
		case KL_CK_PARITY_ODD:
			{
				ce_nlevel_param.pos = (s->ck_size == KL_CK_KEY128) ? 
						(s->cell->pos+1) : (s->cell->pos);
				
				if ((KL_ALGO_TDES == ce_nlevel_param.algo) && (KL_CK_KEY64 == s->ck_size))
				{
					//gen 64bit odd key
					ce_nlevel_param.parity = CE_PARITY_ODD;
					memcpy(ce_nlevel_param.content_key, gen_key->key_odd, 8);
				}
				else
				{
					//gen 128bit odd key...
					ce_nlevel_param.parity = CE_PARITY_EVEN_ODD;
					memcpy(ce_nlevel_param.content_key, gen_key->key_odd, KL_KEY_SIZE_MAX);
				}

				ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
					CE_IO_CMD(IO_CRYPT_GEN_NLEVEL_KEY), (__u32)(&ce_nlevel_param));
				if (0 != ret) {
					dev_dbg(s->pdev->dev, "IO_CRYPT_GEN_NLEVEL_KEY, ret:%x\n", ret);
					return -EIO;
				}
			}
			break;
		case KL_CK_PARITY_ODD_EVEN:
		default:
			{
				ce_nlevel_param.parity = CE_PARITY_EVEN_ODD;

				if ((KL_ALGO_TDES == ce_nlevel_param.algo) && (KL_CK_KEY64 == s->ck_size))
				{
					//gen 64bit even odd key...
					ce_nlevel_param.pos = s->cell->pos;
					memcpy(&(ce_nlevel_param.content_key[8]), gen_key->key_even, 8);
					memcpy(ce_nlevel_param.content_key, gen_key->key_odd, 8);

					ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
						CE_IO_CMD(IO_CRYPT_GEN_NLEVEL_KEY), (__u32)(&ce_nlevel_param));
					if (0 != ret) {
						dev_dbg(s->pdev->dev, "IO_CRYPT_GEN_NLEVEL_KEY, ret:%x\n", ret);
						return -EIO;
					}
				}
				else
				{
					//gen 128bit even key...
					ce_nlevel_param.pos = s->cell->pos;
					memcpy(ce_nlevel_param.content_key, gen_key->key_even, KL_KEY_SIZE_MAX);

					ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
						CE_IO_CMD(IO_CRYPT_GEN_NLEVEL_KEY), (__u32)(&ce_nlevel_param));
					if (0 != ret) {
						dev_dbg(s->pdev->dev, "IO_CRYPT_GEN_NLEVEL_KEY, ret:%x\n", ret);
						return -EIO;
					}

					//gen 128bit odd key...
					ce_nlevel_param.pos = s->cell->pos+1;
					memcpy(ce_nlevel_param.content_key, gen_key->key_odd, KL_KEY_SIZE_MAX);

					ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
						CE_IO_CMD(IO_CRYPT_GEN_NLEVEL_KEY), (__u32)(&ce_nlevel_param));
					if (0 != ret) {
						dev_dbg(s->pdev->dev, "IO_CRYPT_GEN_NLEVEL_KEY, ret:%x\n", ret);
						return -EIO;
					}
				}
			}
			break;
	}

	return 0;
}

static int ca_kl_hdcp_key_gen(struct ca_kl_session *s,
	struct kl_gen_hdcp_key *gen_key)
{
	int ret;

	if (NULL == gen_key)
		return -EINVAL;

	if (!s->pdev->is_hdcp) {
		dev_dbg(s->pdev->dev, "only hdcp keyladder(kl5) can generate hdcp key!\n");
		return -EPERM;
	}

	if (!s->is_cfg) {
		dev_dbg(s->pdev->dev, "please config session first!\n");
		return -EPERM;
	}

	ret = ali_ce_generate_hdcp_key((pCE_DEVICE)s->pdev->parent->see_ce_id, (__u8 *)gen_key->hdcp_pk, 288);
	if (ret != 0)
		dev_dbg(s->pdev->dev, "ca_kl_hdcp_key_gen() error!\n");

	return ret;
}

static int ca_kl_etsi_challenge(struct ca_kl_session *s,
	struct kl_etsi_challenge *c)
{
	int ret = -EPERM;
	struct ce_etsi_challenge rpc_etsi_challenge;

	if (!s)
		return -EFAULT;

	if (NULL == c)
		return -EFAULT;

	if ((s->pdev->kl_index < ETSI_INDEX_0) ||
		(s->pdev->kl_index > ETSI_INDEX_4)) {
		dev_dbg(s->pdev->dev, "only support ETSI KL!\n");
		return -EPERM;
	}

	if ((KL_ALGO_AES != c->algo) &&
		(KL_ALGO_TDES != c->algo)) {
		dev_dbg(s->pdev->dev, "algo shall be AES or TDES\n");
		return -EINVAL;
	}

	memset(&rpc_etsi_challenge, 0, sizeof(rpc_etsi_challenge));
	rpc_etsi_challenge.kl_index = s->pdev->kl_index;
	rpc_etsi_challenge.otp_addr = s->pdev->root_key_addr;
	rpc_etsi_challenge.algo = (KL_ALGO_AES == c->algo) ?
				CE_SELECT_AES : CE_SELECT_DES;
	memcpy(rpc_etsi_challenge.ek2, c->ek2, sizeof(c->ek2));
	memcpy(rpc_etsi_challenge.nonce, c->nonce, sizeof(c->nonce));
	ret = ali_ce_ioctl(s->pdev->parent->see_ce_id,
		(CE_IO_CMD(IO_CRYPT_ETSI_CHALLENGE)),
		(__u32)&rpc_etsi_challenge);
	if (0 != ret) {
		dev_dbg(s->pdev->dev, "ETSI_CHALLENGE, ret:%x\n", ret);
		ret = -EIO;
	}

	memcpy(c->da_nonce, rpc_etsi_challenge.da_nonce,
		sizeof(c->da_nonce));
	return ret;
}

static int ca_kl_cw_derive(struct ca_kl_session *s,
	struct kl_cw_derivation *d)
{
	int ret = -EPERM;
	struct ce_cw_derivation rpc_derivation;
	int fput_needed = 0;
	struct file *target_file = NULL;
	struct kl_key_cell *key_cell = NULL;
	struct kl_key_cell *data_cell = NULL;
	struct kl_key_cell *target_cell = NULL;

	if (!s)
		return -EFAULT;

	if (NULL == d)
		return -EFAULT;

	if ((s->pdev->kl_index != KL_INDEX_0) &&
		(s->pdev->kl_index != KL_INDEX_1) &&
		(s->pdev->kl_index != ETSI_INDEX_0) &&
		(s->pdev->kl_index != ETSI_INDEX_1)) {
		dev_dbg(s->pdev->dev, "only kl0/kl1 etsi0/etsi1 support cw->cw!\n");
		return -EPERM;
	}

	if (!s->is_cfg) {
		dev_dbg(s->pdev->dev, "please config session first!\n");
		return -EPERM;
	}

	if ((KL_KEY_HW != d->key_src) &&
		(KL_DATA_HW != d->data_src)) {
		dev_dbg(s->pdev->dev, "at least one of key and data should from HW\n");
		return -EINVAL;
	}

	if ((KL_ALGO_AES != s->algo) &&
		(KL_ALGO_XOR == s->algo)) {
		dev_dbg(s->pdev->dev, "Current CW derivation only supports AES or XOR\n");
		return -EINVAL;
	}

	target_file = fget_light(d->target_fd, &fput_needed);
	if (!target_file)
		return -ENXIO;
	if (file2session(target_file) != s) {
		dev_dbg(s->pdev->dev, "only target fd can do this action\n");
		fput_light(target_file, fput_needed);
		return -EPERM;
	}
	fput_light(target_file, fput_needed);

	s->cell->ck_parity = d->target_parity;

	memset(&rpc_derivation, 0, sizeof(rpc_derivation));
	rpc_derivation.algo = (KL_ALGO_AES == s->algo) ?
				(CE_SELECT_AES) : (CE_SELECT_XOR);
	rpc_derivation.crypto_mode = (KL_ENCRYPT == s->crypt_mode) ?
				(CE_IS_ENCRYPT) : (CE_IS_DECRYPT);
	rpc_derivation.key_src = (KL_KEY_HW == d->key_src) ?
				(CE_KEY_FROM_SRAM) : (CE_KEY_FROM_CPU);
	if (KL_KEY_HW == d->key_src) {
		ret = fetch_key_cell_by_fd(d->key.fd, &key_cell);
		if (unlikely(!key_cell)) {
			dev_dbg(s->pdev->dev, "err fetch key_cell\n");
			return ret;
		}
		rpc_derivation.key.pos = key_cell->pos;
		if ((KL_CK_KEY128 == key_cell->ck_size) &&
			(KL_CK_PARITY_ODD == d->key.parity))
			rpc_derivation.key.pos++;
	} else {
		memcpy(&rpc_derivation.key,
			&d->key, sizeof(d->key));
	}

	rpc_derivation.data_src = (KL_DATA_HW == d->data_src) ?
		(CE_DATA_IN_FROM_SRAM) : (CE_DATA_IN_FROM_CPU);
	if (KL_DATA_HW == d->data_src) {
		ret = fetch_key_cell_by_fd(d->data.fd, &data_cell);
		if (unlikely(!data_cell)) {
			dev_err(s->pdev->dev, "err fetch data_cell\n");
			return ret;
		}
		rpc_derivation.data.pos = data_cell->pos;
		if ((KL_CK_KEY128 == data_cell->ck_size) &&
			(KL_CK_PARITY_ODD == d->data.parity))
			rpc_derivation.data.pos++;
	} else {
		memcpy(&rpc_derivation.data,
			&d->data, sizeof(d->data));
	}

	ret = fetch_key_cell_by_fd(d->target_fd, &target_cell);
	if (unlikely(!target_cell)) {
		dev_dbg(s->pdev->dev, "err fetch target_cell\n");
		return ret;
	}
	rpc_derivation.target_pos = target_cell->pos;
	if ((KL_CK_KEY128 == s->cell->ck_size) &&
		(KL_CK_PARITY_ODD == d->target_parity))
		rpc_derivation.target_pos++;

	ret = ali_ce_ioctl((CE_DEVICE *)s->pdev->parent->see_ce_id,
			CE_IO_CMD(IO_CRYPT_CW_DERIVE_CW),
			(__u32)&rpc_derivation);
	if (0 != ret) {
		dev_dbg(s->pdev->dev, "CW_DERIVE_CW, ret:%x\n", ret);
		ret = -EIO;
	}

	return ret;
}

long ca_kl_ioctl(struct file *filp, __u32 cmd, unsigned long arg)
{
	int ret = RET_SUCCESS;

	struct ca_kl_sub_dev *son = NULL;
	struct ca_kl_session *s = NULL;

	s = (struct ca_kl_session *)file2session(filp);
	if (!s)
		return -EBADF;

	son = s->pdev;
	if (!son)
		return -EBADF;

	mutex_lock(&son->parent->mutex);

	switch (CE_IO_CMD(cmd)) {
	case CE_IO_CMD(KL_CONFIG_KEY):
		{
			struct kl_config_key cfg_key;
			ret = ali_ce_umemcpy((void *)&cfg_key,
					(void __user *)arg,
					sizeof(struct kl_config_key));
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}

			ret = ca_kl_key_config(s, &cfg_key);
			if (ret < 0) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}
		}
		break;

	case CE_IO_CMD(KL_GEN_KEY):
		{
			struct kl_gen_key gen_key;
			ret = ali_ce_umemcpy((void *)&gen_key,
				(void __user *)arg, sizeof(struct kl_gen_key));
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}

			ret = ca_kl_key_gen(s, &gen_key);
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}
		}
		break;

	case CE_IO_CMD(KL_GEN_HDCP_KEY):
		{
			struct kl_gen_hdcp_key gen_hdcp_key;
			ret = ali_ce_umemcpy((void *)&gen_hdcp_key,
					(void __user *)arg,
					sizeof(struct kl_gen_hdcp_key));
			if (0 != ret) {
				dev_err(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}

			ret = ca_kl_hdcp_key_gen(s, &gen_hdcp_key);
			if (0 != ret) {
				dev_err(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}
		}
		break;

	case CE_IO_CMD(KL_DERIVE_CW):
		{
			struct kl_cw_derivation cw_derivation;
			ret = ali_ce_umemcpy((void *)&cw_derivation,
				(void __user *)arg,
				sizeof(struct kl_cw_derivation));
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}

			ret = ca_kl_cw_derive(s, &cw_derivation);
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}
		}
		break;

	case CE_IO_CMD(KL_ETSI_CHALLENGE):
		{
			struct kl_etsi_challenge etsi_challenge;
			ret = ali_ce_umemcpy((void *)&etsi_challenge,
				(void __user *)arg,
				sizeof(struct kl_etsi_challenge));
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}

			ret = ca_kl_etsi_challenge(s, &etsi_challenge);
			if (0 != ret) {
				dev_dbg(son->dev, "failed, ret:%d\n", ret);
				goto DONE;
			}
			if (0 == ret) {
				ret = ali_ce_umemcpy((void __user *)arg,
					(void *)&etsi_challenge,
					sizeof(struct kl_etsi_challenge));
				if (0 != ret)
					goto DONE;
			}
		}
		break;

	default:
		ret = ca_kl_ioctl_legacy(filp, cmd, arg);
		if (ret)
			dev_dbg(son->dev, "invalid ioctl\n");
	}

DONE:
	mutex_unlock(&son->parent->mutex);

	return ret;
}

