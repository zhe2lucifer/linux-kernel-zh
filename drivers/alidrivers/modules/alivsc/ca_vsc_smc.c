/*
 * VSC Driver for Conax Virtual Smart Card
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/pagemap.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/reset.h>

#include <alidefinition/adf_vsc.h>
#include <alidefinition/adf_ce.h>
#include <ali_soc.h>

#include "ca_vsc_priv.h"
#include "ca_vsc.h"
#include "ca_vsc_rpc.h"


#define VSC_RPC_DATA_SIZE_MAX (128)
#define VSC_DATA_TIMEOUT_MS (1000) /* in milliseconds */
#define VSC_DATA_TIMEOUT (VSC_DATA_TIMEOUT_MS * HZ / 1000) /* jiffies */


static struct ca_vsc_smc *g_sdev;

/* SEE callback function called within smc_cmd_transfer context */
void ca_vsc_smc_cmd_response(const VSC_PKT *pkt, const u16 *sw)
{
	struct ca_vsc_smc *sdev = g_sdev;
	u8 *ptr = NULL;
	int len;

	if (pkt->total_data_length > VSC_DATA_SIZE_MAX) {
		dev_err(sdev->dev, "%s: error in total len %d\n",
				__func__, pkt->total_data_length);
		goto exit;
	}

	len = pkt->valid_data_length + sdev->response_len;
	if (len > VSC_DATA_SIZE_MAX) {
		dev_err(sdev->dev, "%s: error in data len %d\n",
				__func__, len);
		goto exit;
	}

	if (pkt->pkt_index > 1 && *sw != sdev->sw) {
		dev_info(sdev->dev, "%s: %x-%x!\n",
				__func__, sdev->sw, *sw);
	}

	sdev->sw = *sw;
	ptr = &sdev->response[sdev->response_len];
	memcpy(ptr, pkt->data, pkt->valid_data_length);
	sdev->response_len += pkt->valid_data_length;

	if (sdev->response_len < pkt->total_data_length)
		goto exit;

	dev_dbg(sdev->dev, "%s: xfer complete len=%d!\n",
			__func__, pkt->total_data_length);

	sdev->state |= VSC_STATE_SMC_DATA;
	wake_up_interruptible(&sdev->wq);

exit:
	return;
}

static int ca_vsc_smc_cmd_transfer(struct ca_vsc_smc *sdev,
									struct vsc_cmd_transfer *tr)
{
	int ret = 0;
	int i = 0;
	VSC_PKT pkt;
	u16 sw = 0, resp_len = 0;
	int pos = 0, len;
	int cnt, mod;

	if (tr->num_to_write < 1) {
		dev_dbg(sdev->dev, "%s: error in num_to_write %d!\n",
				__func__, tr->num_to_write);
		return -EINVAL;
	}

	if (tr->num_to_write > VSC_DATA_SIZE_MAX) {
		dev_dbg(sdev->dev, "%s: error in num_to_write %d!\n",
				__func__, tr->num_to_write);
		return -EINVAL;
	}

	sdev->response_len = 0;
	sdev->state &= ~VSC_STATE_SMC_DATA;

	cnt = tr->num_to_write / VSC_RPC_DATA_SIZE_MAX;
	mod = tr->num_to_write % VSC_RPC_DATA_SIZE_MAX;

	while (pos < tr->num_to_write) {
		len = (i < cnt) ? VSC_RPC_DATA_SIZE_MAX : mod;

		pkt.session_id = tr->session_id;
		memcpy((void *)pkt.data, &tr->command[pos], len);

		pkt.pkt_index = i + 1; /* index starts from 1 */
		pkt.valid_data_length = len;
		pkt.total_data_length = tr->num_to_write;

		ret = vsc_dispatch_cmd_transfer(&pkt, &resp_len, &sw);
		if (ret) {
			dev_info(sdev->dev, "%s: vsc_dispatch_cmd [%d,%d] failed ret=%d!\n",
							__func__, i, cnt, ret);
			ret = -EIO;
			goto exit;
		}

		pos += len;
		i++;
	}

	tr->response_len = 0;
	tr->sw1 = (sw >> 0) & 0xff;
	tr->sw2 = (sw >> 8) & 0xff;

	/* wait for response from SEE */
	if (resp_len) {
		while (!(sdev->state & VSC_STATE_SMC_DATA)) {

			ret = wait_event_interruptible_timeout(
					sdev->wq,
					(sdev->state & VSC_STATE_SMC_DATA),
					VSC_DATA_TIMEOUT);

			if (ret == 0) {
				ret = -ETIME;
				goto exit;
			}

			if (ret < 0) {
				ret = -ERESTARTSYS;
				goto exit;
			}
		}

		memcpy(tr->response, sdev->response, sdev->response_len);
		tr->response_len = sdev->response_len;
		tr->sw1 = (sdev->sw >> 0) & 0xff;
		tr->sw2 = (sdev->sw >> 8) & 0xff;
	}

	return 0;

exit:
	return ret;
}

static int ca_vsc_smc_decw_key(
	struct ca_vsc_smc *sdev, struct vsc_decw_key *key)
{
	int ret = 0;
	struct kl_key_cell *cell = NULL;
	CE_DECW_KEY_PARAM decw_param;

	memset(&decw_param, 0, sizeof(CE_DECW_KEY_PARAM));
	memcpy(decw_param.en_key, key->decw_key, VSC_KEY_DECW_SIZE);

	/* kl key cell already exists? */
	ret = fetch_key_cell_by_fd(key->kl_fd, &cell);
	if (ret || !cell) {
		dev_info(sdev->dev,
			"%s: fetch_key_cell_by_fd by fd %d failed %d\n",
			__func__, key->kl_fd, ret);
		return -EINVAL;
	}

	decw_param.ce_data_info.key_info.cw_pos = cell->pos;
	if (VSC_CK_PARITY_EVEN == key->ck_parity)
		decw_param.ce_data_info.des_aes_info.des_low_or_high = KL_LOW_ADDR;
	else
		decw_param.ce_data_info.des_aes_info.des_low_or_high = KL_HIGH_ADDR;

	/* load protecting keys for decw decryption */
	ret = vsc_lib_get_key(&key->key_id);
	if (ret) {
		ret = -EIO;
		dev_info(sdev->dev, "%s: vsc_lib_get_key failed!\n",
				__func__);
		goto exit;
	}

	/* decrypt and load key from decw */
	ret = vsc_crypt_decw_key(sdev->drv->see_ce_id, (__u32)&decw_param);
	if (0 != ret) {
		dev_info(sdev->dev, "%s: failed, ret:%d\n", __func__, ret);
		ret = -EIO;
		goto exit;
	}

exit:
	return ret;
}

long ca_vsc_smc_ioctl(struct file *file, __u32 cmd, unsigned long arg)
{
	int ret = -EIO;
	struct ca_vsc_smc *sdev  = NULL;

	if (unlikely(!file || !file->private_data))
		return -EBADF;

	sdev = (struct ca_vsc_smc *)file->private_data;
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	switch (cmd) {
	case VSC_CMD_DISPATCH:
	{
		struct vsc_cmd_transfer *tr = &sdev->tr;

		ret = copy_from_user(tr, (void __user *) arg,
				sizeof(struct vsc_cmd_transfer));
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = ca_vsc_smc_cmd_transfer(sdev, tr);
		if (ret)
			break;

		ret = copy_to_user((void __user *) arg, tr,
				sizeof(struct vsc_cmd_transfer));
		if (ret) {
			ret = -EFAULT;
			break;
		}

		break;
	}

	case VSC_DECW_KEY:
	{
		struct vsc_decw_key key;

		ret = copy_from_user(&key, (void __user *) arg,
				sizeof(struct vsc_decw_key));
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = ca_vsc_smc_decw_key(sdev, &key);
		break;
	}

	default:
	{
		dev_dbg(sdev->dev, "unsupported cmd=0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}
	} /* switch */

	mutex_unlock(&sdev->drv->mutex);
	return ret;
}


int ca_vsc_smc_open(struct inode *inode, struct file  *file)
{
	struct ca_vsc_smc *sdev = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	sdev = container_of(inode->i_cdev, struct ca_vsc_smc, cdev);
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	sdev->cnt += 1;
	file->private_data = sdev;

	mutex_unlock(&sdev->drv->mutex);
	return 0;
}


int ca_vsc_smc_close(struct inode *inode, struct file  *file)
{
	struct ca_vsc_smc *sdev = NULL;

	if (unlikely(!inode || !file || !file->private_data))
		return -EBADF;

	sdev = container_of(inode->i_cdev, struct ca_vsc_smc, cdev);
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	if (!sdev->cnt) {
		mutex_unlock(&sdev->drv->mutex);
		return -EBADF;
	}

	sdev->cnt -= 1;
	file->private_data = NULL;

	mutex_unlock(&sdev->drv->mutex);
	return 0;
}

static const struct file_operations vsc_smc_fops = {
	.owner = THIS_MODULE,
	.open = ca_vsc_smc_open,
	.release = ca_vsc_smc_close,
	.unlocked_ioctl	= ca_vsc_smc_ioctl,
	.compat_ioctl	= ca_vsc_smc_ioctl,
};

int ca_vsc_smc_probe(struct ca_vsc_drv *drv)
{
	int ret = -1;
	struct device_node *child = NULL;
	struct ca_vsc_smc *sdev = NULL;

	sdev = kzalloc(sizeof(struct ca_vsc_smc),
			GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;

	child = of_get_child_by_name(drv->clnt->dev.of_node, VSC_DEVNAME);
	if (!child) {
		pr_err("Don't find child <functions> of DTS node<%s>!\n",VSC_DEVNAME);
		ret = alloc_chrdev_region(&sdev->devt, 0, 1, VSC_DEVNAME);
		if (ret < 0)
		{
			sdev->devt =0;
			pr_err("alloc ali dev_t fail\n");
			return -1;
		}
	}
	else{
		ret = of_get_major_minor(child,&sdev->devt, 0, 1, VSC_DEVNAME);
		if (ret  < 0) {
			pr_err("unable to get major and minor for char devive\n");
			return ret;
		}
	}
	cdev_init(&sdev->cdev, &vsc_smc_fops);

	sdev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&sdev->cdev, sdev->devt, 1);
	if (ret < 0)
		goto cleanup;

	sdev->dev_class = class_create(THIS_MODULE, VSC_DEVNAME);
	if (IS_ERR_OR_NULL(sdev->dev_class)) {
		ret = PTR_ERR(sdev->dev_class);
		goto cleanup;
	}

	sdev->dev = device_create(sdev->dev_class, NULL,
				sdev->devt, &sdev, VSC_DEVNAME);
	if (IS_ERR_OR_NULL(sdev->dev)) {
		ret = PTR_ERR(sdev->dev);
		goto cleanup;
	}

	/* TODO: debugfs */

	drv->smc_dev = sdev;
	sdev->see_clnt = drv->clnt;
	sdev->state = 0;
	sdev->drv = drv;
	init_waitqueue_head(&sdev->wq);
	g_sdev = sdev;
	dev_dbg(&drv->clnt->dev, "VSC-SMC driver probed\n");

	return 0;

cleanup:
	if (sdev->dev_class) {
		device_destroy(sdev->dev_class, sdev->devt);
		cdev_del(&sdev->cdev);
		class_destroy(sdev->dev_class);
		unregister_chrdev_region(sdev->devt, 1);
	}
	kfree(sdev);

	return ret;
}

int ca_vsc_smc_remove(struct ca_vsc_drv *drv)
{
	struct ca_vsc_smc *sdev = drv->smc_dev;

	/* TODO: debugfs */

	device_destroy(sdev->dev_class, sdev->devt);
	cdev_del(&sdev->cdev);
	unregister_chrdev_region(sdev->devt, 1);
	class_destroy(sdev->dev_class);
	kfree(sdev);
	drv->smc_dev = NULL;
	g_sdev = NULL;
	dev_dbg(&drv->clnt->dev, "VSC-SMC driver removed\n");

	return 0;
}
