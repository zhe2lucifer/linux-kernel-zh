/*
 * ALi CRC32 driver
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

#include <crypto/internal/hash.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/cryptohash.h>
#include <linux/types.h>
#include <crypto/sha.h>
#include <asm/byteorder.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/file.h>

#include <ali_sbm_client.h>
#include "ali_crc.h"

struct ali_crc_dev_tag ali_crc_dev;

static int see_crc_final(struct ali_crc_desc *dctx,
	unsigned int *rc)
{
	int ret = 0;
	struct ali_crc_dev_tag *pdev;
	int session_id;

	if (!dctx || !dctx->init)
		return -EFAULT;

	pdev = dctx->pdev;
	session_id = dctx->session_id;

	if (dctx->entry.entry != NULL) {
		while (1) {
			ret = wait_event_interruptible_timeout(dctx->wq_sbm,
					!see_query_sbm_entry(&dctx->sbm_desc,
						&dctx->entry),
					usecs_to_jiffies(100 * 5));
			if (likely(ret > 0))
				break;
		}
		dctx->unchecked_pkt = 0;
	}

	ret = _crc_final(pdev, session_id, rc);
	if (ret) {
		dev_err(&pdev->clnt->dev,
			"see_crc_final error: %d\n", ret);
		ret = -EIO;
	}

	return ret;
}

static int see_crc_update(struct ali_crc_desc *dctx,
	u8 *input, unsigned int len)
{
	struct crc32_sbm_packet pkt;
	int ret = 0;

	if (!input || (!len)) {
		dev_err(&dctx->pdev->clnt->dev,
			" ##!! %p,%d\n", input, len);
		return -EINVAL;
	}

	pkt.sess_id = dctx->session_id;
	pkt.input = input;
	pkt.len = len;

	while (1) {
		ret = wait_event_interruptible_timeout(dctx->wq_sbm,
				!see_enqueue_sbm_entry(&dctx->sbm_desc, &pkt,
				sizeof(pkt), &dctx->entry),
				usecs_to_jiffies(100));
		if (likely(ret > 0))
			break;
	}
	dctx->unchecked_pkt++;

	if ((dctx->unchecked_pkt >= CRC32_SBM_NR_NODES)
		|| (dctx->unchecked_pkt >= dctx->nents)) {
		if (dctx->entry.entry != NULL) {
			while (1) {
				ret = wait_event_interruptible_timeout
					(dctx->wq_sbm, !see_query_sbm_entry
					(&dctx->sbm_desc, &dctx->entry),
					usecs_to_jiffies(100 * 5));
				if (likely(ret > 0))
					break;
			}
			dctx->unchecked_pkt = 0;
		}
	}

	return ret;
}

static int ali_crc_init(struct ahash_request *req)
{
	int rc, ret;
	struct scr_sess_create param;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_crc_desc *dctx = crypto_ahash_ctx(tfm);

	if (!dctx)
		return -EFAULT;

	if (dctx->init)
		return 0;

	dctx->pdev = &ali_crc_dev;

	dev_dbg(&dctx->pdev->clnt->dev, "%s enter\n", __func__);

	mutex_lock(&dctx->pdev->mutex);

	memset(&param, 0, sizeof(param));
	param.algo = ALGO_CRC32;

	rc = _crc_session_create(dctx->pdev, &dctx->session_id, &param);
	if (rc) {
		dev_err(&dctx->pdev->clnt->dev,
			"crc create session error, ret:%d\n", rc);
		mutex_unlock(&dctx->pdev->mutex);
		return rc;
	}

	/*open sbm session*/
	memset(&dctx->sbm_desc, 0, sizeof(dctx->sbm_desc));
	dctx->sbm_desc.id = -1;
	dctx->sbm_desc.buf_size = CRC32_SBM_NR_NODES *
		sizeof(struct crc32_sbm_packet);
	dctx->sbm_desc.buf_start = devm_kzalloc(
		&dctx->pdev->clnt->dev,
		dctx->sbm_desc.buf_size,
		GFP_KERNEL);

	ret = see_sbm_create(&dctx->sbm_desc);
	if (ret < 0) {
		dev_err(&dctx->pdev->clnt->dev,
			"create sbm failed: %p, %d, %d\n",
			dctx->sbm_desc.buf_start,
			dctx->sbm_desc.buf_size,
			ret);
		ret = -EIO;
		goto err_out;
	}

	init_waitqueue_head(&dctx->wq_sbm);
	ret = _crc_create_sbm_task(dctx->pdev,
		dctx->sbm_desc.id);
	if (ret < 0) {
		dev_err(&dctx->pdev->clnt->dev,
			"_crc_create_sbm_task, ret:%d\n", ret);
		ret = -EIO;
		goto err_out;
	}

	mutex_unlock(&dctx->pdev->mutex);
	dctx->rc = 0;
	dctx->init = 1;

	return 0;

err_out:
	mutex_unlock(&dctx->pdev->mutex);

	if (dctx->sbm_desc.id >= 0)
		see_sbm_destroy(&dctx->sbm_desc);

	if (dctx->sbm_desc.buf_start)
		devm_kfree(&dctx->pdev->clnt->dev,
			dctx->sbm_desc.buf_start);

	_crc_session_delete(dctx->pdev, dctx->session_id);

	return ret;
}

static int ali_crc_update(struct ahash_request *req)
{
	int i, ret;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_crc_desc *dctx = crypto_ahash_ctx(tfm);

	if (!dctx->init)
		return -EFAULT;

	if (!dctx->pdev)
		return -ENODEV;

	if (!req->src) {
		dev_dbg(&dctx->pdev->clnt->dev, "%s, req->src NULL\n",
			__func__);
		return -EFAULT;
	}

	dev_dbg(&dctx->pdev->clnt->dev, "%s: length[%d], iv[%x]\n",
		__func__, req->nbytes, dctx->rc);

	dctx->nents = sg_nents(req->src);
	ret = dma_map_sg(NULL, req->src, dctx->nents, DMA_TO_DEVICE);
	if (ret != dctx->nents) {
		dev_err(&dctx->pdev->clnt->dev,
			"dma_map_sg fail: ret[%d]|nents[%d]", ret, dctx->nents);
		return -ENOMEM;
	}

	for (i = 0; i < dctx->nents; i++) {
		ret = see_crc_update(dctx,
			(u8 *)req->src[i].dma_address,
			req->src[i].length);
		if (ret < 0)
			dev_err(&dctx->pdev->clnt->dev,
				"%s, crc32 cal error!%x\n",
				__func__, ret);
	}

	return 0;
}

static int ali_crc_final(struct ahash_request *req)
{
	int ret;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_crc_desc *dctx = crypto_ahash_ctx(tfm);

	if (!dctx->init)
		return -EFAULT;

	if (!req->result) {
		dev_dbg(&dctx->pdev->clnt->dev, "%s, req->result NULL\n",
			__func__);
		return -EFAULT;
	}

	mutex_lock(&dctx->pdev->mutex);

	ret = see_crc_final(dctx, &dctx->rc);
	if (ret)
		dev_err(&dctx->pdev->clnt->dev,
			"%s, crc32 final error!%x\n",
			__func__, ret);

	*((unsigned int *)req->result) = dctx->rc;

	/*release the resource*/
	_crc_delete_sbm_task(dctx->pdev, dctx->sbm_desc.id);
	if (dctx->sbm_desc.id >= 0)
		see_sbm_destroy(&dctx->sbm_desc);
	if (dctx->sbm_desc.buf_start)
		devm_kfree(&dctx->pdev->clnt->dev, dctx->sbm_desc.buf_start);
	_crc_session_delete(dctx->pdev, dctx->session_id);
	mutex_unlock(&dctx->pdev->mutex);

	memset(dctx, 0, sizeof(*dctx));
	return 0;
}

static int ali_crc_finup(struct ahash_request *req)
{
	int ret;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_crc_desc *dctx = crypto_ahash_ctx(tfm);

	dev_dbg(&dctx->pdev->clnt->dev, "ali_sha_finup start\n");

	if (!req->src || !req->result) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s, req->src or req->result NULL\n",
			__func__);

		ret = -EFAULT;
		goto out;
	}

	ret = ali_crc_update(req);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto out;
	}

	ret = ali_crc_final(req);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto out;
	}

out:
	return ret;
}

static int ali_crc_digest(struct ahash_request *req)
{
	int ret;

	ret = ali_crc_init(req);
	if (ret)
		return ret;

	return ali_crc_finup(req);
}

static struct ahash_alg ali_crc32 = {
	.init		= ali_crc_init,
	.update		= ali_crc_update,
	.final		= ali_crc_final,
	.finup		= ali_crc_finup,
	.digest		= ali_crc_digest,
	.halg.digestsize	= CRC32_CHKSUM_DIGEST_SIZE,
	.halg.base	= {
		.cra_name		= "ali-crc32",
		.cra_driver_name	= "ali-crc32",
		.cra_priority		= 100,
		.cra_flags		= CRYPTO_ALG_TYPE_AHASH |
						CRYPTO_ALG_ASYNC,
		.cra_blocksize		= CRC32_CHKSUM_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct ali_crc_desc),
		.cra_alignmask		= 3,
		.cra_module		= THIS_MODULE,
	}
};

static int ali_crc32_probe(struct see_client *clnt)
{
	int rc;
	struct ali_crc_dev_tag *pcrc = &ali_crc_dev;

	memset(pcrc, 0, sizeof(struct ali_crc_dev_tag));
	mutex_init(&pcrc->mutex);
	pcrc->clnt = clnt;

	rc = _crc_api_attach(pcrc);
	if (rc) {
		dev_dbg(&clnt->dev, "crc attach error!\n");
		return rc;
	}

	rc = crypto_register_ahash(&ali_crc32);
	if (rc)
		goto out;

	dev_set_drvdata(&clnt->dev, pcrc);
	dev_info(&clnt->dev, "probe.\n");
	return 0;

out:
	dev_dbg(&clnt->dev, "Ali CRC32 initialization failed.\n");
	return rc;
}

static int ali_crc32_remove(struct see_client *clnt)
{
	struct ali_crc_dev_tag *pcrc = dev_get_drvdata(&clnt->dev);
	if (!pcrc)
		return -ENODEV;

	dev_info(&clnt->dev, "remove.\n");
	crypto_unregister_ahash(&ali_crc32);

	return 0;
}

static const struct of_device_id see_crc32_matchtbl[] = {
	{ .compatible = "alitech,crc32" },
	{ }
};

static struct see_client_driver crc32_drv = {
	.probe	= ali_crc32_probe,
	.remove	= ali_crc32_remove,
	.driver	= {
		.name		= "CRC32",
		.of_match_table	= see_crc32_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

module_see_client_driver(crc32_drv);


MODULE_DESCRIPTION("Ali CRC algorithms support.");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ali");
MODULE_VERSION("1.1.0");
MODULE_ALIAS("Ali-Crc32");

