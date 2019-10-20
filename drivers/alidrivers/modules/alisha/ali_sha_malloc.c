/*
 * Security Hashing Algorithm driver
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
#include "ali_sha.h"
#include "ali_sha_cdev.h"

#define SHA_POOL_SIZE (1<<20)

static u8 *sha_pool;
static size_t sha_pool_pos;
struct ali_sha_dev_tag ali_sha_dev;

static int ali_sha_init(struct ali_sha_desc *dctx)
{
	int ret = 0;
	unsigned int id = ALI_INVALID_DSC_SUB_DEV_ID;
	SHA_INIT_PARAM sha_param;

	dctx->pdev = &ali_sha_dev;

	dev_dbg(&dctx->pdev->clnt->dev, "%s start\n", __func__);

	mutex_lock(&dctx->pdev->mutex);

	id = ali_sha_get_free_sub_device_id();
	if (id == ALI_INVALID_DSC_SUB_DEV_ID ||
		id >= VIRTUAL_DEV_NUM) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"fail to get SHA sub devId\n");
		ret = -EBUSY;
		goto unlock;
	}

	dctx->id = id;

	memset(&sha_param, 0, sizeof(sha_param));
	sha_param.sha_work_mode = dctx->type;
	sha_param.sha_data_source = SHA_DATA_SOURCE_FROM_DRAM;

	ret = ali_sha_ioctl(dctx->pdev->see_sha_id[dctx->id],
		DSC_IO_CMD(IO_INIT_CMD), (__u32)(&sha_param));
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"ali_sha_ioctl error: %d\n", ret);
		ret = -EIO;
		goto free_id;
	}

	sha_pool_pos = 0;
	return 0;

free_id:
	ali_sha_set_sub_device_id_idle(dctx->id);
unlock:
	mutex_unlock(&dctx->pdev->mutex);

	return ret;
}

static int ali_sha1_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	memset(dctx, 0, sizeof(*dctx));
	dctx->type = SHA_SHA_1;

	return ali_sha_init(dctx);
}

static int ali_sha224_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	memset(dctx, 0, sizeof(*dctx));
	dctx->type = SHA_SHA_224;

	return ali_sha_init(dctx);
}

static int ali_sha256_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	memset(dctx, 0, sizeof(*dctx));
	dctx->type = SHA_SHA_256;

	return ali_sha_init(dctx);
}

static int ali_sha384_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	memset(dctx, 0, sizeof(*dctx));
	dctx->type = SHA_SHA_384;

	return ali_sha_init(dctx);
}

static int ali_sha512_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	memset(dctx, 0, sizeof(*dctx));
	dctx->type = SHA_SHA_512;

	return ali_sha_init(dctx);
}

static int ali_sha_update(struct ahash_request *req)
{
	int i;
	int ret = 0;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	if (!dctx->pdev) {
		ret = -ENODEV;
		goto unlock;
	}

	if (!req->src) {
		dev_dbg(&dctx->pdev->clnt->dev, "%s, req->src NULL\n",
			__func__);
		ret = -EFAULT;
		goto unlock;
	}

	dctx->page_num = sg_nents(req->src);

	for (i = 0; i < dctx->page_num; i++)
		ret += sg_dma_len(&req->src[i]);
	if ((sha_pool_pos + ret) > SHA_POOL_SIZE) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s: out of memory, max size is %dMB\n",
			__func__, SHA_POOL_SIZE>>20);
		ret = -EFBIG;
		goto unlock;
	}

	ret = sg_copy_to_buffer(req->src, dctx->page_num,
		&(sha_pool[sha_pool_pos]), SHA_POOL_SIZE - sha_pool_pos);
	sha_pool_pos += ret;

	return 0;

unlock:
	mutex_unlock(&dctx->pdev->mutex);
	return ret;
}

static int ali_sha_final(struct ahash_request *req)
{
	int ret;
	u8 out[ALI_SHA_MAX_DIGEST_SIZE];
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	dev_dbg(&dctx->pdev->clnt->dev, "%s start\n", __func__);

	if (!req->result) {
		dev_dbg(&dctx->pdev->clnt->dev, "%s, req->result NULL\n",
			__func__);
		ret = -EFAULT;
		goto unlock;
	}

	memset(out, 0, ALI_SHA_MAX_DIGEST_SIZE);

	dma_sync_single_for_device(NULL,
		virt_to_phys(sha_pool),
		sha_pool_pos, DMA_TO_DEVICE);

	ret = ali_sha_digest(dctx->pdev->see_sha_id[dctx->id],
		(u8 *)(virt_to_phys(sha_pool)), out, sha_pool_pos);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto unlock;
	}

	ali_sha_set_sub_device_id_idle(dctx->id);
	memcpy(req->result, out, crypto_ahash_digestsize(tfm));

unlock:
	mutex_unlock(&dctx->pdev->mutex);

	return ret;
}

static int ali_sha_finup(struct ahash_request *req)
{
	int ret;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	if (!req->src || !req->result) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s, req->src or req->result NULL\n",
			__func__);

		ret = -EFAULT;
		goto out;
	}

	dev_dbg(&dctx->pdev->clnt->dev, "%s, %p, %d\n",
		__func__, req->src, req->nbytes);

	ret = ali_sha_update(req);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto out;
	}

	ret = ali_sha_final(req);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto out;
	}

out:
	return ret;
}

static int ali_sha_all(struct ahash_request *req)
{
	int ret;
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ali_sha_desc *dctx = crypto_ahash_ctx(tfm);

	ret = tfm->init(req);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto out;
	}

	dev_dbg(&dctx->pdev->clnt->dev, "%s start\n", __func__);

	ret = ali_sha_finup(req);
	if (ret) {
		dev_dbg(&dctx->pdev->clnt->dev,
			"%s error: %d\n", __func__, ret);
		goto out;
	}

out:
	return ret;
}

static struct ahash_alg ali_sha[] = {
	{
		.init		= ali_sha1_init,
		.update		= ali_sha_update,
		.final		= ali_sha_final,
		.finup		= ali_sha_finup,
		.digest		= ali_sha_all,
		.halg.digestsize	= SHA1_DIGEST_SIZE,
		.halg.statesize = SHA1_DIGEST_SIZE,
		.halg.base	= {
			.cra_name		= "ali-sha1",
			.cra_driver_name	= "ali-sha1",
			.cra_priority		= 100,
			.cra_flags		= CRYPTO_ALG_TYPE_AHASH |
							CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA1_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ali_sha_desc),
			.cra_alignmask		= 3,
			.cra_module		= THIS_MODULE,
		}
	},

	{
		.init		= ali_sha224_init,
		.update		= ali_sha_update,
		.final		= ali_sha_final,
		.finup		= ali_sha_finup,
		.digest 	= ali_sha_all,
		.halg.digestsize	= SHA224_DIGEST_SIZE,
		.halg.statesize = SHA224_DIGEST_SIZE,
		.halg.base	= {
			.cra_name		= "ali-sha224",
			.cra_driver_name	= "ali-sha224",
			.cra_priority		= 100,
			.cra_flags		= CRYPTO_ALG_TYPE_AHASH |
							CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA224_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ali_sha_desc),
			.cra_alignmask		= 3,
			.cra_module		= THIS_MODULE,
		}
	},

	{
		.init		= ali_sha256_init,
		.update		= ali_sha_update,
		.final		= ali_sha_final,
		.finup		= ali_sha_finup,
		.digest 	= ali_sha_all,
		.halg.digestsize	= SHA256_DIGEST_SIZE,
		.halg.statesize = SHA256_DIGEST_SIZE,
		.halg.base	= {
			.cra_name		= "ali-sha256",
			.cra_driver_name	= "ali-sha256",
			.cra_priority		= 100,
			.cra_flags		= CRYPTO_ALG_TYPE_AHASH |
							CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA256_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ali_sha_desc),
			.cra_alignmask		= 3,
			.cra_module		= THIS_MODULE,
		}
	},

	{
		.init		= ali_sha384_init,
		.update		= ali_sha_update,
		.final		= ali_sha_final,
		.finup		= ali_sha_finup,
		.digest 	= ali_sha_all,
		.halg.digestsize	= SHA384_DIGEST_SIZE,
		.halg.statesize = SHA384_DIGEST_SIZE,
		.halg.base	= {
			.cra_name		= "ali-sha384",
			.cra_driver_name	= "ali-sha384",
			.cra_priority		= 100,
			.cra_flags		= CRYPTO_ALG_TYPE_AHASH |
							CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA384_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ali_sha_desc),
			.cra_alignmask		= 3,
			.cra_module		= THIS_MODULE,
		}
	},

	{
		.init		= ali_sha512_init,
		.update		= ali_sha_update,
		.final		= ali_sha_final,
		.finup		= ali_sha_finup,
		.digest 	= ali_sha_all,
		.halg.digestsize	= SHA512_DIGEST_SIZE,
		.halg.statesize = SHA512_DIGEST_SIZE,
		.halg.base	= {
			.cra_name		= "ali-sha512",
			.cra_driver_name	= "ali-sha512",
			.cra_priority		= 100,
			.cra_flags		= CRYPTO_ALG_TYPE_AHASH |
							CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA512_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ali_sha_desc),
			.cra_alignmask		= 3,
			.cra_module		= THIS_MODULE,
		}
	},
};

static int ali_sha_probe(struct see_client *clnt)
{
	int i;
	int rc = -ENODEV;
	struct ali_sha_dev_tag *psha = &ali_sha_dev;

	memset(psha, 0, sizeof(struct ali_sha_dev_tag));
	mutex_init(&psha->mutex);
	psha->clnt = clnt;
	ali_m36_sha_see_init();

	for (i = 0; i < VIRTUAL_DEV_NUM; i++) {
		psha->see_sha_id[i] = hld_dev_get_by_id(HLD_DEV_TYPE_SHA, i);
		if (NULL == psha->see_sha_id[i]) {
			dev_info(&clnt->dev, "SHA get see id error\n");
			goto out;
		}
	}

	dev_set_drvdata(&clnt->dev, psha);

	for (i = 0; i < ARRAY_SIZE(ali_sha); i++) {
		rc = crypto_register_ahash(&ali_sha[i]);
		if (rc)
			goto out;
	}

	sha_pool = devm_kzalloc(&clnt->dev, SHA_POOL_SIZE, GFP_KERNEL);

	rc = ali_sha_cdev_probe(clnt);
	if (rc)
		goto out;

	dev_info(&clnt->dev, "probed\n");
	return 0;

out:
	dev_info(&clnt->dev, "Ali SHA initialization failed.\n");
	return rc;
}

static int ali_sha_remove(struct see_client *clnt)
{
	int i;
	struct ali_sha_dev_tag *psha = dev_get_drvdata(&clnt->dev);

	if (!psha)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(ali_sha); i++)
		crypto_unregister_ahash(&ali_sha[i]);

	devm_kfree(&clnt->dev, sha_pool);
	ali_sha_cdev_remove(clnt);

	dev_info(&clnt->dev, "removed\n");
	return 0;
}

static const struct of_device_id see_sha_matchtbl[] = {
	{ .compatible = "alitech,sha" },
	{ }
};

static struct see_client_driver sha_drv = {
	.probe	= ali_sha_probe,
	.remove	= ali_sha_remove,
	.driver	= {
		.name		= "SHA",
		.of_match_table	= see_sha_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

module_see_client_driver(sha_drv);

MODULE_DESCRIPTION("Ali SHA algorithms support.");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ali");
MODULE_VERSION("1.1.0");
MODULE_ALIAS("Ali-sha");
