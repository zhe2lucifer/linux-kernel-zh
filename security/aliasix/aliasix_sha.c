/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced verification implementations.
 *
 *  Author:
 *	Zhao Owen <owen.zhao@alitech.com>
 *
 *  Copyright (C) 2011 Zhao Owen <owen.zhao@alitech.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2,
 *      as published by the Free Software Foundation.
 */

#include "aliasix_sha.h"
// just let compile go through. 
extern  __u32 ali_dsc_get_free_sub_device_id(__u8 sub_mode);
extern int ali_dsc_set_sub_device_id_idle(__u8 sub_mode,__u32 device_id);
extern int ali_trig_ram_mon(__u32 start_addr,__u32 end_addr, __u32 interval, \
								__u32 sha_mode, int DisableOrEnable);
extern int ali_sha_digest(SHA_DEV *pShaDev, __u8 *input, __u8 *output,__u32 data_length);
extern int ali_sha_digest_ex(SHA_DEV *pShaDev, __u8 *input, 
								__u8 *output,__u32 data_length);
extern int ali_sha_ioctl(SHA_DEV *pShaDev, __u32 cmd , __u32 param);

/*
 * Macros of the digest algorithm
 */
#define ALIASIX_HASH_DRIVER         "sha256-generic"
#define ALIASIX_HASH_TYPE           CRYPTO_ALG_TYPE_SHASH
#define ALIASIX_HASH_MASK           0x00000000
#define ALIASIX_INVALID_SHA_DEV     0xff

/* Signature file's extension */
#define ALIASIX_SIG_EXT		".sig"

/* Get the digest */
#define SRC_DIGEST(addr, offset) (*(u_long *)(addr + 0xe0 + offset))
#define DST_DIGEST(addr, offset) (*(u_long *)(addr + offset))

/* Init the digest parameters */
#define ALIASIX_SHA_INIT_DIGEST(in, out, len, sha_len) \
    do { \
        u8 *p_sha_len = kzalloc(sizeof(u_long), GFP_KERNEL); \
        memcpy(p_sha_len, in + len - 2 * ALIASIX_SOURCE_DIGEST_LEN, sizeof(u_long)); \
        sha_len = p_sha_len[3] | (p_sha_len[2] << 8) | \
                  (p_sha_len[1] << 16) | p_sha_len[0] << 24; \        
        memset(out, 0x00, ALIASIX_HASH_DIGEST_LEN); \
        ALIASIX_LOG("Info, len of SHA data %p 0x%x 0x%x 0x%x %x %x %x\n", \
                in, sha_len, len, \
                p_sha_len[0], p_sha_len[1], p_sha_len[2], p_sha_len[3]); \
        kfree(p_sha_len); \
    } while (0)

/* Macro to cast the path */
#define ALIASIX_SHA_CAST_DIRNAME(dst, src, file) \
    do { \
        if (strlen(src) == strlen(file->f_path.dentry->d_name.name) + 1) \
            strcpy(dst, ALIASIX_ROOT_DIR); \
        else \
        { \
            strcpy(dst, src); \
            dst[strlen(src) - strlen(file->f_path.dentry->d_name.name) - 1] = '\0'; \
        } \
    } while (0)

/* Protect the SHA device */
static struct semaphore aliasix_sha_device_sem;

/* 
 * It's not a good way to request hash device every time 
 * We maintain a global one
 */
#if ((defined CONFIG_CRYPTO_SHA1) || \
     (defined CONFIG_CRYPTO_SHA256) || \
     (defined CONFIG_CRYPTO_SHA512))
static struct crypto_shash *p_sw_shash_dev = NULL;
#endif

/*
 * To protect the read buffer
 */
static struct mutex aliasix_read_mutex;
static struct mutex aliasix_buf_mutex;

/*
 * To protect the open + verify routine
 */
static struct mutex aliasix_sig_mutex;

/**
 * Initialization of the mutex
 */
void aliasix_sha_mutex_init(void)
{
    mutex_init(&aliasix_read_mutex);
    mutex_init(&aliasix_sig_mutex);
    mutex_init(&aliasix_buf_mutex);
}

/*
 * Initialization of the SEM
 */
void aliasix_sha_device_sem_init(void)
{
    sema_init(&aliasix_sha_device_sem, 4);
}

/*  
 * Do digest for the given data
 */
#ifdef CONFIG_CRYPTO_HW_DIGEST_ALIASIX   
static u_long aliasix_sha_request_hw_dev(void)
{
    return ali_dsc_get_free_sub_device_id(SHA); 
}

static void aliasix_sha_release_hw_dev(u_long u_sha_dev)
{
    ali_dsc_set_sub_device_id_idle(SHA, u_sha_dev);
}

#define ALIASIX_SHA_STATIC_DEV
#ifdef ALIASIX_SHA_STATIC_DEV
static u_long aliasix_sha_id = ALIASIX_INVALID_SHA_DEV;
static pSHA_DEV aliasix_sha_dev = NULL;
#endif

static pSHA_DEV aliasix_sha_request_dev(u_long *u_sha_id)
{
	pSHA_DEV p_sha_dev = NULL;
#ifdef ALIASIX_SHA_STATIC_DEV
	if (ALIASIX_INVALID_SHA_DEV == aliasix_sha_id)
#endif
	{
		*u_sha_id = aliasix_sha_request_hw_dev();

		if (ALIASIX_INVALID_SHA_DEV == *u_sha_id)
		{
			ALIASIX_ERR("Err, no more available device.\n");
			return NULL;
		}
#ifdef ALIASIX_SHA_STATIC_DEV
		aliasix_sha_id = *u_sha_id;
#endif
	}

#ifdef ALIASIX_SHA_STATIC_DEV
	*u_sha_id = aliasix_sha_id;
#endif

#ifdef ALIASIX_SHA_STATIC_DEV
	if (NULL == aliasix_sha_dev)
#endif
	{
		p_sha_dev = hld_dev_get_by_id(HLD_DEV_TYPE_SHA, *u_sha_id);
		if (NULL == p_sha_dev)
		{        
			ALIASIX_ERR("Err, get sha device failed 0x%x.\n", *u_sha_id);
			aliasix_sha_release_hw_dev(*u_sha_id);
			*u_sha_id = ALIASIX_INVALID_SHA_DEV;
			return NULL;
		}
#ifdef ALIASIX_SHA_STATIC_DEV
		aliasix_sha_dev = p_sha_dev;
#endif
	}

#ifdef ALIASIX_SHA_STATIC_DEV
	return aliasix_sha_dev;
#else
	return p_sha_dev;
#endif
}

static pSHA_DEV aliasix_sha_free_dev(u_long u_sha_id)
{
#ifndef ALIASIX_SHA_STATIC_DEV
    aliasix_sha_release_hw_dev(u_sha_id);
#endif
}

static int __aliasix_sha_hw_digest(const u8 *in, u8 *out, const u_long len)
{
    pSHA_DEV p_sha_dev;
    SHA_INIT_PARAM sha_param;
    u_long u_sha_id = 0;
    int ret = 0;

    p_sha_dev = aliasix_sha_request_dev(&u_sha_id);
    if (NULL == p_sha_dev)
    {        
        ALIASIX_ERR("Err, get sha device failed 0x%x.\n", u_sha_id);
        goto err;
    }

    memset(&sha_param, 0, sizeof(SHA_INIT_PARAM));
	sha_param.sha_data_source = SHA_DATA_SOURCE_FROM_DRAM ; /* from flah or dram */
	sha_param.sha_work_mode = SHA_SHA_256; /* SHA1 224, 256, 384, 512 */
    ali_sha_ioctl(p_sha_dev, (IO_INIT_CMD & 0xff), (__u32)(&sha_param));
    ALIASIX_LOG("Info, hardware sha digest data addr 0x%x 0x%x 0x%x %lu\n", 
                p_sha_dev, in, out, len);
    __CACHE_FLUSH_ALI((u_long)in, len);
    //if (0 != ali_sha_digest_ex(p_sha_dev, in ,out ,len))
	if (0 != ali_sha_digest(p_sha_dev, in ,out ,len)) //just let compile go through.
    {
        ALIASIX_ERR("Err, sha digest failed %p 0x%x.\n", \
					p_sha_dev, u_sha_id);
        goto err;
    }

    goto out1;
err:
    ret = -EACCES;
out1:
	aliasix_sha_free_dev(u_sha_id);
out0:
    return ret;
}
#endif

#if ((defined CONFIG_CRYPTO_SHA1) || \
     (defined CONFIG_CRYPTO_SHA256) || \
     (defined CONFIG_CRYPTO_SHA512))
/* SW SHA routines */
static struct crypto_shash *aliasix_sha_request_sw_dev(void)
{
    ALIASIX_LOG("Info, driver %s type %d mask 0x%x\n", 
                ALIASIX_HASH_DRIVER,
                ALIASIX_HASH_TYPE,
                ALIASIX_HASH_MASK);
    
    if (NULL != p_sw_shash_dev) return p_sw_shash_dev;
    
    p_sw_shash_dev = crypto_alloc_shash(ALIASIX_HASH_DRIVER,
                                        ALIASIX_HASH_TYPE,
				                        ALIASIX_HASH_MASK);

    return p_sw_shash_dev;
}

static void aliasix_sha_release_sw_dev(struct crypto_shash *p_shash)
{
    crypto_free_shash(p_shash);
}

static int __aliasix_sha_sw_digest(const u8 *in, u8 *out, const u_long len)
{
    struct shash_alg *p_shash_alg = NULL;
    struct shash_desc *p_desc = kmalloc(sizeof(struct shash_desc), GFP_KERNEL);

    aliasix_sha_request_sw_dev();
    if (IS_ERR(p_sw_shash_dev) || (NULL == p_sw_shash_dev))
        return -ENOMEM;
    
    p_shash_alg = crypto_shash_alg(p_sw_shash_dev);
    if (IS_ERR(p_shash_alg) || (NULL == p_shash_alg))
	{
		ALIASIX_ERR("Err, Failed to load shash alg %ld\n", PTR_ERR(p_shash_alg));
		return -ENOMEM;
	}

    /*
     * Do the digest, this may also call the digest function
     * with a scatterlist memory usage
     */
    p_shash_alg->init(p_desc);
    p_shash_alg->update(p_desc, in, len);
    p_shash_alg->final(p_desc, out);

    /*
     * We need to free the request device every time normally
     * But we have some crash, this need to be traced
     * And, it's not efficient, so we maintain a global one since we'll use it at all time
     */
    
    return 0;
}
#endif

/*
 *  For the image digest, we use the HW method firstly
 */
static int aliasix_sha_digest(const u8 *in, u8 *out, \
                               const u_long len, int b_siginfile)
{
    int ret = 0;
    u_long u32_sha_len = 0;

    if (b_siginfile)
    {
        ALIASIX_SHA_INIT_DIGEST(in, out, len, u32_sha_len);
        if (u32_sha_len > len || 0 == u32_sha_len) 
		{
			ALIASIX_ERR("Err, Failed to get the file length %ld %ld\n", \
						u32_sha_len, len);
            return -EINVAL;
		}
    }
    else
    {
        u32_sha_len = len;
        memset(out, 0x00, ALIASIX_HASH_DIGEST_LEN);
    }

#ifdef CONFIG_CRYPTO_HW_DIGEST_ALIASIX   
    ret = __aliasix_sha_hw_digest(in, out, u32_sha_len);
#else
    ret = __aliasix_sha_sw_digest(in, out, u32_sha_len);
#endif

    ALIASIX_DUMP("Digest result: ", out, ALIASIX_HASH_DIGEST_LEN);

	return ret;
}

/*
 * Verify the given data
 */
static int __aliasix_sha_verify(const u8 *src, const u8 *result)
{
    int i = 0;

#ifdef ALIASIX_SHA_OUTPUT_DBG
    ALIASIX_LOG("Start verify: source digest\n");
    for (i = 0; i < ALIASIX_HASH_DIGEST_LEN; i += 4)
	{
		printk("0x%x ", SRC_DIGEST(src, i));
	}

    printk("\n");
    ALIASIX_LOG("Start verify: result digest\n");
    for (i = 0; i < ALIASIX_HASH_DIGEST_LEN; i += 4)
	{
		printk("0x%x ", DST_DIGEST(result, i));
	}
    printk("\n");
#endif

    for (i = 0; i < ALIASIX_HASH_DIGEST_LEN; i += 4)
	{
		if (SRC_DIGEST(src, i) != DST_DIGEST(result, i))
		{
            return -EACCES;
		}
	}
    
    return 0;
}

/*  
 * Convert to the ALi internal format
 * src: to be converted
 * buf: converted
 */
static void aliasix_sha_convert(u8 *buf, u8 *src, u_long len)
{
    u_long i = 0;

    for (i = 0; i < len; i++)
    {
        buf[len - i - 1] = src[i];
    }
}

static int aliasix_sha_get_source_digest(const u8 *in, u8 *src, 
                                            const u_long len, int b_siginfile)
{
    u8 *p = NULL;
    /* We need to assign a buffer which has size equal to rsa_m->data */
    /* Otherwise we'll have a buffer overflow error       -- Owen --  */
    u8 *buf = kmalloc(ALIASIX_SOURCE_DIGEST_LEN, GFP_KERNEL);
    int ret = -EACCES;

    if (b_siginfile)
        p = in + len - ALIASIX_SOURCE_DIGEST_LEN;
    else
    {
        p = kmalloc(ALIASIX_SOURCE_DIGEST_LEN, GFP_KERNEL);
        memcpy(p, src, ALIASIX_SOURCE_DIGEST_LEN);
    }

    /* Thanks to the stupid implementation of the RSA, we need to pass 
     * pointer alway but the address of the local varibles
     */
    rsa_op *rsa_res = NULL;
    rsa_op *rsa_m = NULL;
    rsa_op *rsa_e = NULL;
    rsa_op *rsa_n = NULL;

    ALIASIX_DUMP("Digest Encrypted Source: ", (u8 *)p, ALIASIX_SOURCE_DIGEST_LEN);

    /* rsa_op_alloc will manage the data buf internally */
    ret = rsa_op_alloc(&rsa_res, ALIASIX_SOURCE_DIGEST_LEN_ULONG);
    if (ret < 0) goto out;
    ret = rsa_op_alloc(&rsa_m, ALIASIX_SOURCE_DIGEST_LEN_ULONG);
    if (ret < 0) goto err0;
    if (NULL != rsa_m->data) rsa_decode(rsa_m, rsa_m->size, 
                                        p, ALIASIX_SOURCE_DIGEST_LEN);
    ALIASIX_DUMP("Digest Converted Source: ", (u8 *)(rsa_m->data), ALIASIX_SOURCE_DIGEST_LEN);
    ret = rsa_op_alloc(&rsa_e, ALIASIX_MAX_RSA_MODULUS_LEN_ULONG);
    if (ret < 0) goto err1;
    /* TO DO: copy exponent to e */
    if (NULL != rsa_e->data) rsa_decode(rsa_e, rsa_e->size,
                                        aliasix_rsa_public_exponent,
                                        ALIASIX_MAX_RSA_MODULUS_LEN);
    ALIASIX_DUMP("Digest Source Exponent: ", (u8 *)aliasix_rsa_public_exponent, ALIASIX_MAX_RSA_MODULUS_LEN);
    ALIASIX_DUMP("Digest Converted Exponent: ", (u8 *)(rsa_e->data), ALIASIX_MAX_RSA_MODULUS_LEN);
    ret = rsa_op_alloc(&rsa_n, ALIASIX_MAX_RSA_MODULUS_LEN_ULONG);
    if (ret < 0) goto err2;
    /* TO DO: copy modulus to n */
    if (NULL != rsa_n->data) rsa_decode(rsa_n, rsa_n->size,
                                        aliasix_rsa_public_modulus,
                                        ALIASIX_MAX_RSA_MODULUS_LEN);
    ALIASIX_DUMP("Digest Source Modulus: ", (u8 *)aliasix_rsa_public_modulus, ALIASIX_MAX_RSA_MODULUS_LEN);
    ALIASIX_DUMP("Digest Converted Modulus: ", (u8 *)(rsa_n->data), ALIASIX_MAX_RSA_MODULUS_LEN);

    ALIASIX_DUMP("Digest Crypted Source: ", (u8 *)(rsa_m->data), ALIASIX_SOURCE_DIGEST_LEN);
    if ((ret = rsa_cipher(&rsa_res, rsa_m, rsa_e, rsa_n)) < 0)
    {
        ALIASIX_ERR("Err, RSA cipher failed.\n");
        goto err3;
    }
    ALIASIX_DUMP("Digest Decrypted Source: ", (u8 *)(rsa_res->data), ALIASIX_SOURCE_DIGEST_LEN);

    /* We need to convert the kernel algorithm to the ALi kernel one */    
    memcpy(buf, (u8 *)(rsa_res->data), ALIASIX_SOURCE_DIGEST_LEN);
    aliasix_sha_convert(src, buf, ALIASIX_SOURCE_DIGEST_LEN);
    ALIASIX_DUMP("Digest Converted Source: ", (u8 *)src, ALIASIX_SOURCE_DIGEST_LEN);
    ret = 0;

err3:
    rsa_op_free(rsa_n);
err2:
    rsa_op_free(rsa_e);
err1:
    rsa_op_free(rsa_m);
err0:
    rsa_op_free(rsa_res);
out:
    if (!b_siginfile) kfree(p);
    kfree(buf);
    return ret;
}

int aliasix_sha_file_source(const u8 *in, u8 *src, \
				const u_long len, int b_siginfile)
{
	aliasix_sha_get_source_digest(in, src, len, b_siginfile);
}
EXPORT_SYMBOL(aliasix_sha_file_source);

/*
 * Verify the file or image, if needed, add the permission item
 */
int aliasix_sha_verify(struct file *file, aliasix_sha_digest_info *sha_digest_info)
{
    int ret = -EPERM;
    aliasix_file_sha_status file_sha_status;
	int i_sha_retry = 0;

#define ALIASIX_SHA_MAX_RETRY	5

    memset(&file_sha_status, 0x00, sizeof(aliasix_file_sha_status));
    file_sha_status.i_dir_idx = ALIASIX_INVALID_INDEX;
    file_sha_status.b_used = 1;
    file_sha_status.b_valid = 1;
    file_sha_status.b_verified = 1;

    ALIASIX_PERF("Performance, start RSA\n");
    if ((ret = aliasix_sha_get_source_digest(sha_digest_info->p_buf, 
                                             sha_digest_info->src, 
                                             sha_digest_info->u32_file_size, \
                                             sha_digest_info->b_siginfile)) < 0)
    {
        ALIASIX_ERR("Err: get source digest failed\n");
        file_sha_status.b_valid = 0;
		goto finish;
    }
    ALIASIX_PERF("Performance, end RSA\n");

retry:
    ALIASIX_PERF("Performance, start digest\n");
    if ((ret = aliasix_sha_digest(sha_digest_info->p_buf, 
                                  sha_digest_info->result, 
                                  sha_digest_info->u32_file_size, \
                                  sha_digest_info->b_siginfile)) < 0)
    {
        ALIASIX_ERR("Err: digest failed\n");
        file_sha_status.b_valid = 0;
		goto finish;
    }
    ALIASIX_PERF("Performance, end digest\n");
    if ((ret = __aliasix_sha_verify(sha_digest_info->src, 
                                    sha_digest_info->result)) < 0)
    {
		i_sha_retry += 1;
		if (i_sha_retry < ALIASIX_SHA_MAX_RETRY)
		{
			ALIASIX_ERR("Warning: retry for the %s\n", file->f_dentry->d_iname);
			msleep(20);
			goto retry;
		}
        ALIASIX_ERR("Err: verify failed\n");
        file_sha_status.b_valid = 0;
		goto finish;
    }

finish:
    ALIASIX_PATH("Info, file %p type %d used %d valid %d\n", 
                 file, sha_digest_info->u8_digest_type,
                 file_sha_status.b_used,
                 file_sha_status.b_valid);
    file_sha_status.u32_expire = jiffies;
    if ((ALIASIX_DIGEST_FILE == sha_digest_info->u8_digest_type) && \
        (NULL != file))
    {
        aliasix_perm_add_permission_item(file, &file_sha_status);
    }

    return ret;
}

/**
 * Lock the mutex
 */
void aliasix_sha_read_lock(void)
{
    mutex_lock(&aliasix_read_mutex);    
}

void aliasix_sha_read_unlock(void)
{
    mutex_unlock(&aliasix_read_mutex);
}


/**
 * Check if the file is a signature file 
 */
static int aliasix_sha_is_sigfile(struct file *file, 
                                         char *f_path, char *f_name)
{
    char *path = kmalloc(PATH_MAX, GFP_KERNEL);
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    u_long u32_path_len = 0;
    int ret = 0;
    int sig_ext_len = strlen(ALIASIX_SIG_EXT);

    aliasix_misc_path_walk(file, path);

    /* only cast the directory */
    ALIASIX_SHA_CAST_DIRNAME(p_path,path,file);
    
    strcpy(f_path, p_path);
    strcpy(f_name, file->f_path.dentry->d_name.name);
    
    if ((0 == strncmp(".", f_name, 1)) && 
            (strlen(f_name) > sig_ext_len) &&
            (0 == strncmp(ALIASIX_SIG_EXT, 
                f_name + strlen(f_name) - sig_ext_len, 
                sig_ext_len)))
    {
        ret = 1;
    }

out:
    kfree(path);
    kfree(p_path);
    return ret;
}


/**
 * Prepare the signature file name for the splitted file
 */
static void inline aliasix_sha_prepare_sigfile(struct file *file, 
                                         char *path, char *name, 
                                         char *path_sig, int b_looped)
{
    struct fs_struct *fs = current->fs;
    char *p_fsroot = kmalloc(PATH_MAX, GFP_KERNEL);
    struct file file_t;
    u_long u32_path_len = 0;

    int i = 0;

    file_t.f_path = fs->root;
    /*
     * We may change to another root
     * In that case, we have to ignore
     * the prefix of the currently root
     */
    u32_path_len = aliasix_misc_path_walk(&file_t, p_fsroot);

    if (0 == strncmp(p_fsroot, path, strlen(p_fsroot) - 1))
        strcpy(path_sig, &path[strlen(p_fsroot) - 1]);
    else
        ALIASIX_ERR("Warning: Which root are you from %s %s ?\n",\
                    p_fsroot, path);
    strcat(path_sig, "/.");
    strcat(path_sig, name);
    strcat(path_sig, ".sig");

out:
    kfree(p_fsroot);
}

/**
 * Verify the file or image, if needed, add the permission item
 * Extension of the verification function to process
 * the file larger than limit size, and the files which
 * can't be modified
 */
int aliasix_sha_verify_ex(struct file *file, u8 *p_file_buf, 
                           u32 u32_file_size)
{
    int ret = -EPERM;
    loff_t pos = 0;
    u32 u32_buf_size = u32_file_size;
    u8 u8_sig_split = 1;
    u8 u8_looped = 0;
    u32 u32_perver_size = u32_file_size;
    aliasix_sha_digest_info sha_digest_info;
    u8 *result = kmalloc(ALIASIX_HASH_DIGEST_LEN, GFP_KERNEL);
    u8 *src = kmalloc(ALIASIX_SOURCE_DIGEST_LEN, GFP_KERNEL); 
    u8 counter = 0;
    int u_sig_fd = -1;
    char *path = kmalloc(PATH_MAX, GFP_KERNEL);
    char *name = kmalloc(NAME_MAX, GFP_KERNEL);
    char *path_sig = NULL;
    mm_segment_t u32_cur_ds = KERNEL_DS;
    
    if (u32_file_size > ALIASIX_FILE_MAX_LEN)
    {
        u8_looped = 1;
        u32_perver_size = ALIASIX_FILE_MAX_LEN;
    }

    /* Due to the signature appending in the tail of file 
     * may damage the original file, currently the signature
     * file is separated from the original file.
     *
     * Only the signature file can have its signature appending on the tail.
    */
    if(aliasix_sha_is_sigfile(file, path, name))
        u8_sig_split = 0;
    /*
    if (0 == aliasix_perm_sig_split(file, path, name))
        u8_sig_split = 1;
    */
    
    u32_cur_ds = get_fs();
    set_fs(KERNEL_DS);

    if (u8_sig_split)
    {
        path_sig = kmalloc(PATH_MAX, GFP_KERNEL);
        aliasix_sha_prepare_sigfile(file, path, name, path_sig, u8_looped);
        ALIASIX_FILEVER("Info, Splitted signature file %s\n", path_sig);
        mutex_lock(&aliasix_sig_mutex);
        u_sig_fd = sys_open(path_sig, O_RDONLY, ALIASIX_MODE_ARX);
        mutex_unlock(&aliasix_sig_mutex);
        kfree(path_sig);
        if (u_sig_fd < 0) 
        {
            ALIASIX_FILEVER("Info, fail opening File %s/%s\n", path, name);
            goto out;
        }
        ALIASIX_FILEVER("Info, File %s/%s splited\n", path, name);
    }

    memset(&sha_digest_info,0x00,sizeof(sha_digest_info));
    sha_digest_info.b_siginfile = 1;
    mutex_lock(&aliasix_buf_mutex);
	/* Assure we are at the beginning of the file */
    vfs_llseek(file, 0, SEEK_SET);
    do {        
        ALIASIX_PERF("Performance, start read\n");
        vfs_llseek(file, counter * ALIASIX_FILE_MAX_LEN, SEEK_SET);
        vfs_read(file, p_file_buf, u32_perver_size, &pos);
        ALIASIX_PERF("Performance, end read\n");

        /* 3  Verify the signature */
        sha_digest_info.file = file;
        sha_digest_info.p_buf = p_file_buf;
        sha_digest_info.src = src;
        sha_digest_info.result = result;
        sha_digest_info.u32_file_size = u32_perver_size;
        sha_digest_info.u8_digest_type = ALIASIX_DIGEST_FILE;

        if (u8_sig_split && (u_sig_fd >= 0))
        {
            sys_lseek(u_sig_fd, (counter + 2) * 0x100, SEEK_SET);
            sys_read(u_sig_fd ,src, ALIASIX_SOURCE_DIGEST_LEN);
            sha_digest_info.b_siginfile = 0;
        }

        ALIASIX_FILEVER("Info, Verify file %s/%s start\n", path, name);
        if ((ret = aliasix_sha_verify(file, &sha_digest_info)) < 0)
        {
            ALIASIX_ERR("Info, Verify file %s/%s error\n", path, name);
            ALIASIX_ERR("Info, Verify buf 0x%x size 0%d\n", \
						p_file_buf, u32_perver_size);
			/* asm(".word 0x7000003f"); */
            goto out1;
        }
        ALIASIX_FILEVER("Info, Verify file %s/%s end\n", path, name);

        u32_buf_size -= u32_perver_size;
        if (u32_buf_size > ALIASIX_FILE_MAX_LEN)
            u32_perver_size = ALIASIX_FILE_MAX_LEN;
        else
            u32_perver_size = u32_buf_size;
        
        counter++;        
    } while (u32_buf_size > 0 && \
             u8_sig_split && u8_looped);
    
	vfs_llseek(file, 0, SEEK_SET);

out1:
    mutex_unlock(&aliasix_buf_mutex);
    if (u_sig_fd >= 0) sys_close(u_sig_fd);
out:
    set_fs(u32_cur_ds);
    kfree(name);
    kfree(path);
    kfree(result);
    kfree(src);
    
    return ret;
}
