/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced security hook function implementations.
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

#include "aliasix.h"
#include "aliasix_sha.h"
#include "aliasix_daemon.h"
#include "aliasix_perm.h"
#include "aliasix_memchk.h"

#define CONAX_CERTI_INIT

/*
 * I hope these are the hokeyist lines of code in the module. Casey.
 */
#define ALIASIX_DEVPTS_SUPER_MAGIC	0x1cd1
#define ALIASIX_SOCKFS_MAGIC		0x534F434B
#define ALIASIX_TMPFS_MAGIC		    0x01021994

/*
 * Signature method & object
 */
#define ALIASIX_TYPE_ROOTFS  IMG_ROOTFS

/*
 * Memory type
 */
#define ALIASIX_MEM_TYPE_IMAGE  0
#define ALIASIX_MEM_TYPE_FILE   1

/*
 * Debug option to show the file status in proc
 */
static struct proc_dir_entry *aliasix_dir = NULL;
static struct proc_dir_entry *aliasix_file = NULL;

/*
 * Useful MACROs to request / release memory
 * For memory request within 16K, we goto get free page
 * For memory request out of 16K, we get from the signature pool
 * We don't use kmalloc any more due to page align request
 */
#define ALIASIX_REQUEST_MEM(buf, len, contignous, max, type, err_tag) \
    do { \
        if (0 == len) goto err_tag; \
        ALIASIX_LOG("Info, buffer request len %ld %ld\n", len, get_order(len)); \
        if (0 /* len < ALIASIX_BUF_MALLOC_THRESHOLD */) \
            buf = kmalloc(len, GFP_KERNEL); \
        else if (!contignous && get_order(len) <= ALIASIX_BUF_PAGES_THRESHOLD) \
            buf = __get_free_pages(GFP_KERNEL, get_order(len)); \
        else if (len <= max) \
        { \
            if (contignous) buf = ALI_VIRT(type ? ALIASIX_FILE_START : ALIASIX_IMAGE_START); \
            else buf = vmalloc(len); \
        } \
        else buf = NULL; \
        ALIASIX_LOG("Info, buffer request address %p\n", buf); \
        if (NULL == buf) goto err_tag; \
    } while (0)

#define ALIASIX_RELEASE_MEM(buf, len, max, contignous) \
    do { \
        if (0 != len && NULL != buf) \
        { \
            ALIASIX_LOG("Info, buffer free address %p %ld\n", buf, len); \
            if (0 /* len <= ALIASIX_BUF_MALLOC_THRESHOLD */) kfree(buf); \
            else if (!contignous && get_order(len) <= ALIASIX_BUF_PAGES_THRESHOLD) \
                free_pages(buf, get_order(len)); \
            else if (len <= max) \
            { \
                if (contignous) buf = NULL; \
                else vfree(buf);\
            } \
        } \
    } while (0)

/*
 * Don't check the ramfs permission
 */
#define ALIASIX_RAMFS		"ramfs"
#define ALIASIX_ROOTFS		"rootfs"
#define ALIASIX_TMPFS		"tmpfs"
#define ALIASIX_SYSFS		"sysfs"
#define ALIASIX_PROCFS		"proc"
#define ALIASIX_DEVTMPFS	"devtmpfs"

/*
 * Check the authenrization / permission
 */
#if (defined CONFIG_FS_SIGNATURE_ALIASIX)
#define IS_RUNABLE(inode)   (inode->i_sb->s_need_verify) 
#elif (defined CONFIG_MTD_SIGNATURE_ALIASIX) 
#define IS_RUNABLE(inode) \
    ((0 == strcmp(ALIASIX_ROOTFS, inode->i_sb->s_type->name)) ? \
    inode->i_sb->s_need_verify : \
    (inode->i_sb->s_mtd ? (inode->i_sb->s_mtd->mtd_need_verify) : 0)) 
#else
#define IS_RUNABLE(inode)   (inode->i_sb->s_need_verify)
#endif

#define FILE_IN_RDONLY_PART(file) \
	(IS_RDONLY(file->f_path.dentry->d_inode) && \
	__mnt_is_readonly(file->f_path.mnt))
/* Rule 1: No application runable in writable partition */
#define EXECABLE_IN_WRITABLE_PART(file) \
    (!IS_RDONLY(file->f_path.dentry->d_inode) && \
	 !__mnt_is_readonly(file->f_path.mnt) && \
    (0 == inode_permission(file->f_path.dentry->d_inode, MAY_EXEC)))
/* Rule 2: All file should be not writable in RDONLY partition */
#define WRITABLE_IN_RDONLY_PART(file) \
    (IS_RDONLY(file->f_path.dentry->d_inode) && \
	 __mnt_is_readonly(file->f_path.mnt) && \
    (0 == inode_permission(file->f_path.dentry->d_inode, MAY_WRITE)))  
/* 
 * Rule 3: Writable file in writable partition will not be digested 
 *         So, Rule 1 should be judged before Rule 3
 */
#define WRITABLE_FILE_NO_DIGEST(file) \
    (!IS_RDONLY(file->f_path.dentry->d_inode) && \
	 !__mnt_is_readonly(file->f_path.mnt) && \
    (0 != inode_permission(file->f_path.dentry->d_inode, MAY_EXEC)))
/* Rule 4: Don't check permission the special file */
#define SPECIAL_FILE_NO_DIGEST(file) \
    (special_file(file->f_path.dentry->d_inode->i_mode) || \
    S_ISDIR(file->f_path.dentry->d_inode->i_mode) || \
    S_ISLNK(file->f_path.dentry->d_inode->i_mode))
/*
 * Rule 5: Only application on verified partition could be run
 *         Some people may mount a u-disk / etc to ro then run an application
 */
#define EXECABLE_IN_UNVERIFIED_PART(file) \
    (!IS_RUNABLE(file->f_path.dentry->d_inode) && \
    (0 == inode_permission(file->f_path.dentry->d_inode, MAY_EXEC)))    

/* Rule 6: File in ramfs is not digested */
#define UNEXECABLE_FILE_FROM_RAMFS(file) \
    (((0 == strcmp(ALIASIX_RAMFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_TMPFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_ROOTFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_SYSFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_PROCFS, file->f_path.dentry->d_inode->i_sb->s_type->name))) && \
     (0 != inode_permission(file->f_path.dentry->d_inode, MAY_EXEC)))
/* Rule 7: File in ramfs can't be run */
#define EXECABLE_FILE_FROM_RAMFS(file) \
    (((0 == strcmp(ALIASIX_RAMFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_TMPFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_ROOTFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_SYSFS, file->f_path.dentry->d_inode->i_sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_PROCFS, file->f_path.dentry->d_inode->i_sb->s_type->name))) && \
     (0 == inode_permission(file->f_path.dentry->d_inode, MAY_EXEC)))

/* 
 * Rule 8: We always check files' signature from rootfs, but the image's 
 * verification will be done with the kernel in U-Boot
 */
#define IS_FILES_FROM_ROOTFS(file) \
     (0 == strcmp(ALIASIX_ROOTFS, file->f_path.dentry->d_inode->i_sb->s_type->name))
#define IS_SB_FROM_ROOTFS(sb) \
     (0 == strcmp(ALIASIX_ROOTFS, sb->s_type->name))

/*
 * Rule 9: Create in write part is always allowed
 */
#define NEW_FILES_IN_WRITABLE_PART(file) \
    (!IS_RDONLY(file->f_path.dentry->d_inode) && \
	 !__mnt_is_readonly(file->f_path.mnt) && \
    (0 == file->f_path.dentry->d_inode->i_size))
    
/* Rule 10: ramfs don't need to check mount option */
#define IS_SB_FROM_RAMFS(sb) \
    (((0 == strcmp(ALIASIX_RAMFS, sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_TMPFS, sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_ROOTFS, sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_SYSFS, sb->s_type->name)) || \
     (0 == strcmp(ALIASIX_PROCFS, sb->s_type->name))))

/*
 * Don't check the permission of "/"
 */
#define ROOT_DENTRY_IGNORE_PERM(file) \
    (file->f_path.dentry->d_parent == file->f_path.dentry)

// #define ALIASIX_FILEATTRDBG_ENABLE

/* 
 * Print the file attribute
 */
#ifdef ALIASIX_FILEATTRDBG_ENABLE
#define ALIASIX_FILEATTRDBG(file, filename) \
	do { \
		if (NULL == filename || 0 == strcmp(filename, file->f_dentry->d_iname)) \
		{ \
			printk("ALIASIX FILE ATTRIBUTE: START ... %s \n", file->f_dentry->d_iname); \
			printk("Part RDONLY ...............%s\n", __mnt_is_readonly(file->f_path.mnt) ? "yes" : "no"); \
			printk("File RDONLY ...............%s\n", IS_RDONLY(file->f_path.dentry->d_inode) ? "yes" : "no"); \
			printk("File Writtable ............%s\n", inode_permission(file->f_path.dentry->d_inode, MAY_WRITE) ? "no" : "yes"); \
			printk("File Readable .............%s\n", inode_permission(file->f_path.dentry->d_inode, MAY_READ) ? "no" : "yes"); \
			printk("File Executable ...........%s\n", inode_permission(file->f_path.dentry->d_inode, MAY_EXEC) ? "no" : "yes"); \
			printk("END\n"); \
		} \
	} while (0)
#else
#define ALIASIX_FILEATTRDBG(...) do{}while(0)
#endif

/**
 * LSM hooks.
 * We here, that is fun!
 * We fun for what ? It's a miracle
 */

/* 
 * For the following implementations, normally we'll use the default 
 * check in the LSM, but we also recreate the ALiasix function for extension
 *                                       ---- Owen ----
 */

/**
 * aliasix_sb_kern_mount - ALiasix specific mount processing
 * @sb: the file system superblock
 * @flags: the mount flags
 * @data: the aliasix mount options
 * @notes: We use video memory here, so only the rootfs can
 *         mount with verify option
 *
 * Returns 0 on success, an error code on failure
 */
static int aliasix_sb_kern_mount(struct super_block *sb, int flags, void *data)
{
    size_t len = 0;
    int ret = -EACCES;
    u8 *image = NULL;
    u_long ret_len = 0;
    u8 contignous = 0;
    u8 *result = kmalloc(ALIASIX_HASH_DIGEST_LEN, GFP_KERNEL);
    u8 *src = kmalloc(ALIASIX_SOURCE_DIGEST_LEN, GFP_KERNEL);
    aliasix_sha_digest_info sha_digest_info;
	static int b_launch_daemon = 0;
    
    BUG_ON(NULL == sb);

    ALIASIX_LOG("Info, FS verify %s %x %x %x\n", 
        sb->s_type->name,
        IS_SB_FROM_RAMFS(sb),
        sb->s_flags, 
        (sb->s_flags & MS_RDONLY));
    if (
        /* 
         * We'll check the file's signature of the rootfs 
         * But never check the image, the signature of the rootfs image
         * will be verified by the kernel image signature. We don't need to 
         * set the need verify flag here due to it's set when get the super block
         */
        IS_SB_FROM_RAMFS(sb))
    {
        goto pass1;
    }

    if ((NULL != data && \
        NULL == strstr(data, ALIASIX_VERIFICATION)) || \
        !(sb->s_flags & MS_RDONLY))
    {
        sb->s_flags |= MS_NOEXEC;
        goto pass1;
    }

#ifdef CONFIG_CRYPTO_HW_DIGEST_ALIASIX
    contignous = 1;
#endif
    ALIASIX_PERF("Performance, mount start\n");
   
#if (defined CONFIG_FS_SIGNATURE_ALIASIX)
    ALIASIX_LOG("Info, FS verify %d\n", sb->s_need_verify);
    if (!sb->s_need_verify)
    {
        goto normal;
    }
    if (sb->s_op && sb->s_op->get_image_len && sb->s_op->get_image_data)
    {
        len = sb->s_op->get_image_len(sb);
        ALIASIX_REQUEST_MEM(image, len, contignous, 
                              ALIASIX_IMAGE_MAX_LEN,
                              ALIASIX_MEM_TYPE_IMAGE, out);
        if ((ret_len = sb->s_op->get_image_data(sb, image, len)) != len)
        {
            ALIASIX_ERR("Err, FS request len %d, read len %d\n", len, ret_len);
            goto out;
        }
    }
    else
        goto pass;
#elif (defined CONFIG_MTD_SIGNATURE_ALIASIX)
    ALIASIX_LOG("Info, MTD verify %p\n", sb->s_mtd);
    if ((NULL == sb->s_mtd) || !sb->s_mtd->mtd_need_verify)
    {
        goto normal;
    }
    if (sb->s_mtd && sb->s_mtd->get_image_size && sb->s_mtd->get_image_bin)
    {
        int type = ALIASIX_TYPE_ROOTFS;
        size_t addr = 0;

    	ALIASIX_PERF("Performance, start read\n");
        sb->s_mtd->get_image_size(sb->s_mtd, type, &addr, &len);
        ALIASIX_REQUEST_MEM(image, len, contignous,   
                            ALIASIX_IMAGE_MAX_LEN,
                            ALIASIX_MEM_TYPE_IMAGE, out);
        dma_cache_wback((u_long)image, len);
        if (0 != sb->s_mtd->get_image_bin(sb->s_mtd, addr, len, image))
        {
            ALIASIX_ERR("Err, MTD request len %d\n", len);
            goto out;
        }
    	ALIASIX_PERF("Performance, end read\n");
        ALIASIX_DUMP("Info, image read: ", 
                     image + len - ALIASIX_SOURCE_DIGEST_LEN, 
                     ALIASIX_SOURCE_DIGEST_LEN);
    }
    else
        goto pass;
#else
    /* For this case, we just set the fs signature psuedo */
    sb->s_need_verify = 1;
    goto normal;
#endif

    BUG_ON(0 == len || NULL == image);

    sha_digest_info.file = NULL;
    sha_digest_info.p_buf = image;
    sha_digest_info.src = src;
    sha_digest_info.result = result;
    sha_digest_info.u32_file_size = len;
    sha_digest_info.u8_digest_type = ALIASIX_DIGEST_IMAGE;

    ALIASIX_PERF("Performance, start verify\n");
    if ((ret = aliasix_sha_verify(NULL, &sha_digest_info)) < 0)
    {
        ALIASIX_ERR("Err: image verify failed\n");
        goto out;
    }
    ALIASIX_PERF("Performance, end verify\n");
    ALIASIX_PERF("Performance, mount end\n");

normal:

	if (b_launch_daemon) goto pass1;
	b_launch_daemon = 1;

#ifdef CONFIG_ROOTFS_RETIRE_THTREAD
    /* 
     * The user init should be OK, but we give them 1 more min
     * to finish the job
     */
    kthread_run(aliasix_perm_rootfs_retire_thread, NULL, "rootfs_retire");
#endif
#ifdef CONFIG_DAEMON_SIGNATURE_ALIASIX
    /* 
     * This should be a proper time to run the daemon thread 
     * This can't be done when security init, due to the thread enviroment is not ready
     */
    kthread_run(aliasix_daemon_thread, NULL, "aliasix_daemon");
#endif

pass1:
    ret = 0;
out:   
    ALIASIX_RELEASE_MEM(image, len, ALIASIX_IMAGE_MAX_LEN, contignous);
pass:
    kfree(result);
    kfree(src);
    return ret;
}

/**
 * aliasix_sb_mount - ALiasix check for mounting
 * @dev_name: unused
 * @path: mount point
 * @type: unused
 * @flags: unused
 * @data: unused
 *
 * Returns 0 if current can write the floor of the filesystem
 * being mounted on, an error code otherwise.
 */
static int aliasix_sb_mount(char *dev_name, struct path *path,
	                    char *type, unsigned long flags, void *data)
{
    /* Check the reserved path which do not allow mount here */
    ALIASIX_LOG("Info, FS name %s, type %s, flags 0x%x, options %s\n", 
                 dev_name, type, flags, data);
    return 0;
}

/**
 * aliasix_sb_umount - ALiasix check for unmounting
 * @mnt: file system to unmount
 * @flags: unused
 *
 * Returns 0 if current can write the floor of the filesystem
 * being unmounted, an error code otherwise.
 */
static int aliasix_sb_umount(struct vfsmount *mnt, int flags)
{
    /* Check the reserved path which do not allow umount here */
    return 0;
}

/*
 * aliasix_dentry_open - check the opened file.
 *
 * return 0 if everything is OK
 */
static int aliasix_dentry_open(struct file *file, const struct cred *cred)
{
    int ret = -EPERM;
#ifdef CONFIG_FILE_SIGNATURE_ALIASIX
    size_t u32_file_size = 0;
    size_t u32_buf_size = 0;
    u8 *p_virt_file = NULL;
    u8 contignous = 0;
    mm_segment_t u32_cur_ds = KERNEL_DS;
    int i_file_perm = 0;
    struct kstat f_stat;

    /* 
     * Store the memory check strategy 
     * Some function need a user space memory, but actually, it's
     * hard to simulate a user space address in kernel, so we need
     * to modify the memory check strategy temporary
     */
    u32_cur_ds = get_fs();

#ifdef CONFIG_CRYPTO_HW_DIGEST_ALIASIX
    contignous = 1;
#endif
    /**********************************************************
     * BE CAREFUL: The sequence of the check can't be changed *
     **********************************************************/
    if (NEW_FILES_IN_WRITABLE_PART(file) && \
        !SPECIAL_FILE_NO_DIGEST(file))
    {
        file->f_path.dentry->d_inode->i_mode &= ~S_IXUGO;
        goto pass;
    }

    if (ROOT_DENTRY_IGNORE_PERM(file) || \
        /* The /dev directory will be excluded here */
        SPECIAL_FILE_NO_DIGEST(file))
    {
        goto pass;
    }
    
    /* 
     * rootfs type is always writable, so it's judged here 
     * You can't create any file in rootfs
     * You can create files in tmpfs, but it can't be executable
     */
    if (!aliasix_perm_is_rootfs_retire() && \
        IS_FILES_FROM_ROOTFS(file))
    {
#ifdef CONFIG_ROOTFS_VERIFICATION_ALIASIX
        goto verify;
#else
        goto pass;
#endif
    }

    /* Is the file from ramfs ?? Is the file named "/" ?? */ 
    if (UNEXECABLE_FILE_FROM_RAMFS(file))
    {
        goto pass;
    }
#if 1
    if (EXECABLE_IN_WRITABLE_PART(file) || \
        EXECABLE_IN_UNVERIFIED_PART(file))
    {
	    file->f_path.dentry->d_inode->i_mode &= ~S_IXUGO;
	    file->f_path.mnt->mnt_flags |= MNT_NOEXEC;
        goto pass;
    }
    
    /* We don't check the signature for the writtable file */
    if (WRITABLE_IN_RDONLY_PART(file) || \
        EXECABLE_FILE_FROM_RAMFS(file))
    {
        ALIASIX_ERR("Err: access permission denied %s %s\n", \
		    file->f_dentry->d_iname, \
		    file->f_path.dentry->d_inode->i_sb->s_type->name);
        goto out;
    }
#endif    
    if (WRITABLE_FILE_NO_DIGEST(file))
    {
pass:
        ret = 0;
        goto out;
    }
    /**********************************************************
     * BE CAREFUL: The sequence of the check can't be changed *
     **********************************************************/

    /*
     * Here, we have the first regular file from verified partition
     * So, we set the rootfs retire
     */
#ifdef CONFIG_ROOTFS_RETIRE_THTREAD
    if (!aliasix_perm_is_rootfs_retire())
    {
        aliasix_perm_set_rootfs_retire(NULL);
    }
#endif

verify:   
    /* 
     * Some special txt file need to be executable, we ignore them 
     * We need to verify the file only when i_file_perm > 0
     * It means no available permission rules
     */
#if (defined REUSE_OPENED_PERMISSION_ALIASIX || \
     defined ALLOW_OLD_PERMISSION_ALIASIX)
    i_file_perm = aliasix_perm_permission(file);
    if (0 == i_file_perm) 
    {
        ALIASIX_LOG("Info, file verified %s %s\n", \
                    file->f_dentry->d_iname, \
                    file->f_path.dentry->d_inode->i_sb->s_type->name);
        ret = 0;
        goto out_release;
    }
    else if (i_file_perm < 0) 
    {
        ALIASIX_ERR("Err: sha permission denied %s\n", file->f_dentry->d_iname);
        goto out_release;
    }
#endif

    ALIASIX_FILENAME("Info, File to be digested %s\n", file->f_dentry->d_iname);

#define USE_INODE_SIZE
#ifdef USE_INODE_SIZE
    u32_file_size = i_size_read(file->f_path.dentry->d_inode);
#else
    /* For nand file, we can read size here. For rootfs file, we can't */
    vfs_getattr(file->f_path.mnt, file->f_path.dentry, &f_stat);
    u32_file_size = f_stat.size;
#endif
    if ((0 == u32_file_size) 
#ifdef CONFIG_IGNORE_LARGEFILE_ALIASIX
        || (u32_file_size > ALIASIX_MAX_DIGEST_FILE) 
#endif
       )
    {
#ifdef CONFIG_IGNORE_LARGEFILE_ALIASIX
		if (u32_file_size > ALIASIX_MAX_DIGEST_FILE)
		{
			ALIASIX_ERR("Debuging: file size is too large %ld, ignore\n", \
						 u32_file_size);
			ret = 0;
			goto out_release;
		}
#endif
        ALIASIX_ERR("Err: file size read failed %s %ld %ld\n", 
                    file->f_dentry->d_iname,
                    u32_file_size, ALIASIX_MAX_DIGEST_FILE);
        ret = -EINVAL;
        goto out_release;
    }

    set_fs(KERNEL_DS);
#define VMALLOC_DIGEST_FILE_ALIASIX
    /* 2  Get the signature of the file */
#if (defined MMAP_DIGEST_FILE_ALIASIX)
    ALIASIX_LOG("Info: mmap file %s %lu mm %p\n", 
                file->f_dentry->d_iname, u32_file_size, current->mm);
    /* 
     * Actually, we don't have any mm struct available at this moment
     * for a executable file to be executed, but I leave this option
     * to see if we can have another way to do the mmap
     */
    p_virt_file = do_mmap(file, NULL, u32_file_size, PROT_READ, 
                          MAP_ANONYMOUS | MAP_PRIVATE, 0);
#elif (defined VMALLOC_DIGEST_FILE_ALIASIX)
    ALIASIX_LOG("Info: request mem for file %s %s %lu\n", 
                file->f_dentry->d_iname, file->f_dentry->d_name.name, u32_file_size);
    u32_buf_size = u32_file_size;
    if (u32_file_size > ALIASIX_MAX_DIGEST_FILE && \
        contignous)
        u32_buf_size = ALIASIX_MAX_DIGEST_FILE;
    ALIASIX_REQUEST_MEM(p_virt_file, u32_buf_size, contignous, 
                        ALIASIX_MAX_DIGEST_FILE,
                        ALIASIX_MEM_TYPE_FILE, out_release);
#elif (defined USER_PGD_DIGEST_FILE_ALIASIX)
#endif
    if (ret = IS_ERR(p_virt_file))
        goto out_release; 

    /* 
     * Really, it's not a good choice to read all of the data then free them 
     * Can we just use the mapping memory which the file will be put ?
     */
    ALIASIX_PERF("Performance, start verify\n");
    if ((ret = aliasix_sha_verify_ex(file, p_virt_file, u32_file_size)) < 0)
    {
        ALIASIX_ERR("Err: file verification failed %s\n", file->f_dentry->d_iname);
        goto err;
    }
    ALIASIX_PERF("Performance, end verify\n");

    ret = 0;
err:
#if (defined MMAP_DIGEST_FILE_ALIASIX)
    do_munmap(current->mm, p_virt_file, u32_file_size);
#elif (defined VMALLOC_DIGEST_FILE_ALIASIX)
    ALIASIX_RELEASE_MEM(p_virt_file, u32_buf_size, ALIASIX_MAX_DIGEST_FILE, contignous);
#elif (defined USER_PGD_DIGEST_FILE_ALIASIX)
#endif
#undef VMALLOC_DIGEST_FILE_ALIASIX
out_release:  
out:
    set_fs(u32_cur_ds);
#else
    ret = 0;
#endif 
    return ret;
}


/*
 * aliasix_sb_copy_data - copy the ALiasix options.
 * @key_ref: unused
 * @context: unused
 * @perm: unused
 *
 * return 0 if everythins OK
 */
static int aliasix_sb_copy_data(char *orig, char *copy)
{
    int i_slen = strlen(orig);
    char *p = NULL;

    if ((NULL == orig ) || (0 == i_slen) || (NULL == copy)) return 0;
    ALIASIX_LOG("Info, Copy mount data orig %p %s\n", orig, orig);
    if (NULL != (p = strstr(orig, ALIASIX_VERIFICATION)))
        strcpy(copy, ALIASIX_VERIFICATION); 
    ALIASIX_LOG("Info, Copy mount data dest %p %s\n", copy, copy);
    return 0;
}

/*
 * aliasix_mem_protect - set mem protect to kernel
 *
 * return
 */
static void aliasix_mem_protect(void)
{
#ifdef CONFIG_MEMORY_PROTECTION_ALIASIX
    aliasix_memchk_set_protect();
#endif
}

/*
 * aliasix_file_free - check the opened file.
 *
 * return 0 if everything is OK
 */
static int aliasix_file_free(struct file *file)
{
#ifndef ALLOW_OLD_PERMISSION_ALIASIX
    aliasix_perm_reset_permission(file);
#endif
    return 0;
}
/*
security.h
aliasix directory
fs.h
+crypto/rsa.h
ali_dsc.h
kernel config
*/
struct security_operations aliasix_ops = {
	.name =				    "aliasix",

	.sb_kern_mount = 		aliasix_sb_kern_mount,
	.sb_mount = 			aliasix_sb_mount,
	.sb_umount = 			aliasix_sb_umount,
	.sb_copy_data = 		aliasix_sb_copy_data,

#ifdef CONFIG_FILE_SIGNATURE_ALIASIX
	// .dentry_open =          aliasix_dentry_open,
	.file_open =          aliasix_dentry_open,
	.file_free_security =   aliasix_file_free,
#endif
	
	.ali_mem_protect =      aliasix_mem_protect,
};

static const struct file_operations aliasix_proc_fops = {
	.read = aliasix_perm_show_status,
	.llseek = default_llseek,
};

/**
 * aliasix_init - initialize the ALiasix system
 *
 * Returns 0
 */ 
static __init int aliasix_init(void)
{
	printk("aliasix_init\n");
	
	//if (!security_module_enable(&aliasix_ops))
	//	return 0;
	security_module_enable(&aliasix_ops);
	
	printk("ALI SECURITY: Info, Initializing\n");

	/*
	 * Register with LSM
	 */
	if (register_security(&aliasix_ops))
		panic("ALI SECURITY: Unable to register with kernel.\n");

#ifdef CONFIG_FILE_SIGNATURE_ALIASIX
    printk("ALI SECURITY: file signature enabled\n");
#endif
#ifdef CONFIG_ROOTFS_VERIFICATION_ALIASIX
    printk("ALI SECURITY: rootfs signature enabled\n");
#endif
#ifdef CONFIG_MTD_SIGNATURE_ALIASIX
    printk("ALI SECURITY: mtd signature enabled\n");
#endif
#ifdef CONFIG_FS_SIGNATURE_ALIASIX
    printk("ALI SECURITY: fs signature enabled\n");
#endif
#ifdef CONFIG_MEMORY_PROTECTION_ALIASIX
    printk("ALI SECURITY: memory protection enabled\n");
#endif

    aliasix_perm_lock_init();
    aliasix_sha_device_sem_init();
#ifndef ALLOC_PERM_ITEMS_STATIC_ALIASIX
    aliasix_perm_items_init();
#endif
    aliasix_sha_mutex_init();

	if (NULL == (aliasix_dir = proc_mkdir("aliasix", NULL)))
	{
		ALIASIX_ERR("Err, can't create proc dir.\n");
	}

	if (NULL != aliasix_dir && \
	    NULL == (aliasix_file = \
		proc_create("perm_status", 0644, aliasix_dir, &aliasix_proc_fops)))
	{
		ALIASIX_ERR("Err, can't create proc file.\n");
		remove_proc_entry("aliasix", NULL);
	}

//	if (NULL != aliasix_file)
//		aliasix_file->read_proc = aliasix_perm_show_status;

#ifdef CONAX_CERTI_INIT
	aliasix_mem_protect();
#endif 

	return 0;
}

/*
 * Smack requires early initialization in order to label
 * all processes and objects when they are created.
 */ 
#ifdef CONAX_CERTI_INIT  /* just fit for conax certification */
late_initcall(aliasix_init);
#else
security_initcall(aliasix_init);
#endif

