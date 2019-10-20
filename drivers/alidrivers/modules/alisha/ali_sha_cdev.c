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

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <ali_board_config.h>
#include <ali_soc.h>
#include "ca_sha.h"
#include "ali_sha.h"
#include "ali_sha_cdev.h"

#define NO_CHRDEVS (1)
#define FIRST_MIN (0)
#define SHA_INVALID_TYPE(type) ( \
	(type) != CA_SHA_TYPE_1 && \
	(type) != CA_SHA_TYPE_224 && \
	(type) != CA_SHA_TYPE_256 && \
	(type) != CA_SHA_TYPE_384 && \
	(type) != CA_SHA_TYPE_512)

static int ca_sha_open(struct inode *inode, struct file *file)
{
	struct ali_sha_dev_tag *psha = container_of(inode->i_cdev,
			struct ali_sha_dev_tag, cdev);
	struct ali_sha_desc *s;
	int ret;
	unsigned int id = ALI_INVALID_DSC_SUB_DEV_ID;

	mutex_lock(&psha->mutex);

	id = ali_sha_get_free_sub_device_id();
	if (id == ALI_INVALID_DSC_SUB_DEV_ID ||
		id >= VIRTUAL_DEV_NUM) {
		dev_info(psha->dev,
			"fail to get SHA sub devId\n");
		ret = -EBUSY;
		goto GET_SUB_DEV_FAIL;
	}

	s = devm_kzalloc(psha->dev,
			sizeof(struct ali_sha_desc), GFP_KERNEL);
	if (!s) {
		ret = -ENOMEM;
		goto ZALLOC_FAIL;
	}

	/*internal resource init*/
	memset(s, 0, sizeof(struct ali_sha_desc));
	s->pdev = psha;
	file->private_data = (void *)s;
	s->id = id;

	mutex_unlock(&psha->mutex);

	return 0;

ZALLOC_FAIL:
	ali_sha_set_sub_device_id_idle(id);
GET_SUB_DEV_FAIL:
	mutex_unlock(&psha->mutex);

	return ret;
}

static void ca_sha_munmap(struct vm_area_struct *vma)
{
	struct ali_sha_desc *s = vma->vm_private_data;
	struct ali_sha_dev_tag *psha = NULL;

	if (!s)
		return;

	psha = s->pdev;
	if (!psha)
		return;

	if (psha->vm_area_node.vma != vma)
		return;

	mutex_lock(&psha->mutex);
	memset(&psha->vm_area_node, 0, sizeof(psha->vm_area_node));
	mutex_unlock(&psha->mutex);
}

static const struct vm_operations_struct ca_sha_vmops = {
	.close = ca_sha_munmap,
};

static int ca_sha_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = -1;
	struct ali_sha_desc *s = file->private_data;
	struct ali_sha_dev_tag *psha = NULL;
	size_t size = vma->vm_end - vma->vm_start;
	struct ali_sha_vm_node *p_vm_node = NULL;

	if (!s)
		return -EBADF;

	psha = s->pdev;
	if (!psha)
		return -EBADF;

	if (psha->vm_area_node.vma) {
		dev_info(s->pdev->dev, "device is already mmaped\n");
		return -EBUSY;
	}

	if (!size || (size > __G_ALI_MM_VIDEO_SIZE)) {
		dev_info(psha->dev, "size not support, size=0x%x, kbuf max:%ldMB\n",
			size, __G_ALI_MM_VIDEO_SIZE>>20);
		return -ENOMEM;
	}

	size = (size >= PAGE_SIZE) ? size : PAGE_SIZE;

	mutex_lock(&psha->mutex);

	/* map vma->vm_start to kaddr('s page frame num) in size area */
	ret = remap_pfn_range(vma, vma->vm_start,
			virt_to_phys((void *)__G_ALI_MM_VIDEO_START_ADDR) >> PAGE_SHIFT,
			size,  pgprot_noncached(PAGE_SHARED));
	if (ret != 0) {
		dev_info(psha->dev, "Kernel error - remap_pfn_range failed\n");
		return -EAGAIN;
	}

	p_vm_node = &psha->vm_area_node;

	p_vm_node->vm_kaddr = (void *)__G_ALI_MM_VIDEO_START_ADDR;
	p_vm_node->vm_start = vma->vm_start;
	p_vm_node->vm_end = vma->vm_end;
	p_vm_node->vm_size = size;
	p_vm_node->vm_owner = current->pid;
	p_vm_node->vma = vma;

	mutex_unlock(&psha->mutex);

	vma->vm_private_data = s;
	vma->vm_ops = &ca_sha_vmops;

	return 0;
}

static int ca_sha_release(struct inode *inode, struct file *file)
{
	struct ali_sha_desc *s = file->private_data;
	struct ali_sha_dev_tag *psha;

	if (!s)
		return -EBADF;

	psha = s->pdev;

	if ((psha->vm_area_node.vma) && (psha->vm_area_node.vm_owner == current->pid))
		ca_sha_munmap(psha->vm_area_node.vma);

	mutex_lock(&psha->mutex);
	ali_sha_set_sub_device_id_idle(s->id);
	file->private_data = NULL;
	devm_kfree(psha->dev, s);
	mutex_unlock(&psha->mutex);

	return 0;
}

static int ca_sha_init(struct ali_sha_desc *s, int type)
{
	int ret = 0;
	SHA_INIT_PARAM sha_param;
	const int work_mode[] = {
		[CA_SHA_TYPE_1] = SHA_SHA_1,
		[CA_SHA_TYPE_224] = SHA_SHA_224,
		[CA_SHA_TYPE_256] = SHA_SHA_256,
		[CA_SHA_TYPE_384] = SHA_SHA_384,
		[CA_SHA_TYPE_512] = SHA_SHA_512,
	};

	const int dgt_size[] = {
		[CA_SHA_TYPE_1] = SHA1_DIGEST_SIZE,
		[CA_SHA_TYPE_224] = SHA224_DIGEST_SIZE,
		[CA_SHA_TYPE_256] = SHA256_DIGEST_SIZE,
		[CA_SHA_TYPE_384] = SHA384_DIGEST_SIZE,
		[CA_SHA_TYPE_512] = SHA512_DIGEST_SIZE,
	};

	if (s->init) {
		dev_dbg(s->pdev->dev, "cannot init twice!\n");
		return -EPERM;
	}

	if (SHA_INVALID_TYPE(type)) {
		dev_dbg(s->pdev->dev, "Invalid type!\n");
		return -EINVAL;
	}

	memset(&sha_param, 0, sizeof(sha_param));
	sha_param.sha_work_mode = work_mode[type];
	sha_param.sha_data_source = SHA_DATA_SOURCE_FROM_DRAM;

	ret = ali_sha_ioctl(s->pdev->see_sha_id[s->id],
		DSC_IO_CMD(IO_INIT_CMD), (__u32)(&sha_param));
	if (ret) {
		dev_dbg(s->pdev->dev,
			"ali_sha_ioctl error: %d\n", ret);
		ret = -EIO;
	}

	s->init = 1;
	s->type = work_mode[type];
	s->digest_size = dgt_size[type];

	return ret;
}

static int ca_sha_digest(struct ali_sha_desc *s, struct digest *pdgt)
{
	int ret;
	unsigned char *input = NULL;
	unsigned char output[ALI_SHA_MAX_DIGEST_SIZE];
	struct ali_sha_vm_node *p_cur_node = &s->pdev->vm_area_node;

	if (!s->init) {
		dev_dbg(s->pdev->dev, "pls init first!\n");
		return -EPERM;
	}

	if (!pdgt->input || !pdgt->output || !pdgt->data_len)
		return -EINVAL;

	if ((!p_cur_node->vma) || (p_cur_node->vm_owner != current->pid)) {
		dev_dbg(s->pdev->dev, "pls mmap first!\n");
		return -ENOMEM;
	}

	if (((unsigned long)pdgt->input >= p_cur_node->vm_start) &&
		(((unsigned long)pdgt->input + (unsigned long)pdgt->data_len)
		<= p_cur_node->vm_end)) {

		input = (u8 *)(p_cur_node->vm_kaddr +
			(unsigned long)pdgt->input - p_cur_node->vm_start);
	} else {
		dev_dbg(s->pdev->dev, "out of memory mapped\n");
		return -EACCES;
	}

	dma_sync_single_for_device(NULL,
			virt_to_phys(input),
			pdgt->data_len, DMA_TO_DEVICE);

	ret = ali_sha_digest(s->pdev->see_sha_id[s->id],
		(u8 *)virt_to_phys(input), output, pdgt->data_len);
	if (ret) {
		dev_dbg(s->pdev->dev,
			"ali_sha_digest error: %d\n", ret);
		ret = -EIO;
		goto leave;
	}

	ret = copy_to_user(pdgt->output, output, s->digest_size);
	if (ret) {
		goto leave;
	}

leave:
	return ret;
}

static long ca_sha_ioctl(struct file *file, unsigned int cmd,
	unsigned long args)
{
	int ret = RET_SUCCESS;
	struct ali_sha_desc *s = file->private_data;

	if (!s)
		return -EBADF;

	mutex_lock(&s->pdev->mutex);

	switch (cmd) {
	case CA_SHA_SET_TYPE:
	{
		int type;

		ret = copy_from_user(&type,
			(void __user *)args, sizeof(int));
		if (ret) {
			dev_dbg(s->pdev->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = ca_sha_init(s, type);
		if (ret < 0)
			goto DONE;

		break;
	}

	case CA_SHA_DIGEST:
	{
		struct digest dgt;

		memset(&dgt, 0, sizeof(dgt));

		ret = copy_from_user(&dgt,
			(void __user *)args, sizeof(struct digest));
		if (ret) {
			dev_dbg(s->pdev->dev, "%s\n", __func__);
			goto DONE;
		}

		ret = ca_sha_digest(s, &dgt);
		if (ret < 0)
			goto DONE;

		break;
	}

	default:
	{
		ret = -ENOIOCTLCMD;
		break;
	}
	}

DONE:
	mutex_unlock(&s->pdev->mutex);
	return ret;
}

static const struct file_operations ca_sha_fops = {
	.owner		= THIS_MODULE,
	.open		= ca_sha_open,
	.mmap		= ca_sha_mmap,
	.release		= ca_sha_release,
	.unlocked_ioctl	= ca_sha_ioctl,
};

int ali_sha_cdev_probe(struct see_client *clnt)
{
	int ret;
	struct ali_sha_dev_tag *psha = dev_get_drvdata(&clnt->dev);

	/*
	* Character device initialisation
	*/
	ret = of_get_major_minor(clnt->dev.of_node,&psha->dev_num, 
			FIRST_MIN, NO_CHRDEVS, CA_SHA_BASENAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto chrdev_alloc_fail;
	}
	cdev_init(&psha->cdev, &ca_sha_fops);
	ret = cdev_add(&psha->cdev, psha->dev_num, 1);
	if (ret < 0)
		goto cdev_add_fail;

	psha->class = class_create(THIS_MODULE, CA_SHA_DRVNAME);
	if (IS_ERR(psha->class)) {
		ret = PTR_ERR(psha->dev);
		goto class_create_fail;
	}

	psha->dev = device_create(psha->class, &clnt->dev, psha->dev_num,
		psha, CA_SHA_BASENAME);
	if (IS_ERR(psha->dev)) {
		ret = PTR_ERR(psha->dev);
		goto device_create_fail;
	}

	memset(&psha->vm_area_node, 0, sizeof(psha->vm_area_node));

	dev_set_drvdata(psha->dev, psha);

	dev_info(&clnt->dev, "cdev probe.\n");
	return 0;

device_create_fail:
	class_destroy(psha->class);
class_create_fail:
	cdev_del(&psha->cdev);
cdev_add_fail:
	unregister_chrdev_region(psha->dev_num, NO_CHRDEVS);
chrdev_alloc_fail:
	devm_kfree(&clnt->dev, psha);

	return ret;
}

int ali_sha_cdev_remove(struct see_client *clnt)
{
	struct ali_sha_dev_tag *psha = dev_get_drvdata(&clnt->dev);

	if (!psha)
		return -ENODEV;

	dev_info(&clnt->dev, "removing driver @%d\n",
		clnt->service_id);

	dev_set_drvdata(psha->dev, NULL);

	memset(&psha->vm_area_node, 0, sizeof(psha->vm_area_node));
	mutex_destroy(&psha->mutex);
	device_destroy(psha->class, psha->dev_num);
	class_destroy(psha->class);
	cdev_del(&psha->cdev);
	unregister_chrdev_region(psha->dev_num, NO_CHRDEVS);

	dev_info(&clnt->dev, "cdev removed\n");

	return 0;
}
