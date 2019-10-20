/*
 * Copyright (C) 2011 Zhao Owen <owen.zhao@alitech.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, version 2.
 *
 * Author:
 *      Zhao Owen <owen.zhao@alitech.com>
 *
 */

#ifndef _SECURITY_ALIASIX_H_
#define _SECURITY_ALIASIX_H_

/*
 * Kernel standard header
 */
#include <linux/xattr.h>
#include <asm/page.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/mount.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/fs_struct.h>
#include <linux/stat.h>
#include <linux/kd.h>
#include <asm/ioctls.h>
#include <linux/mutex.h>
#include <linux/mtd/mtd.h>
#include <linux/vmalloc.h>
#include <linux/capability.h>
#include <linux/spinlock.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/limits.h>
#include <linux/kthread.h>
//#include <net/netlabel.h>
/*
 * Crypto graphic header
 */
#include <asm/scatterlist.h>
#include <asm/io.h>
#include <crypto/hash.h>
#include <crypto/algapi.h>
#include <crypto/rsa.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/gfp.h>
#include <linux/proc_fs.h>
#include <ali_cache.h>

#if defined(CONFIG_CRYPTO_HW_DIGEST_ALIASIX) || \
    defined(CONFIG_MEMORY_PROTECTION_ALIASIX)
#include <rpc_hld/ali_rpc_hld.h>
#include <linux/ali_dsc.h>
#endif

/*
 * ALi special headers
 */
//#include <linux/dvb/ali_nand.h>

/*
 * Get the buffer parameters
 */
//#include <ali_board_config.h>
#ifdef CONFIG_ARM
#include <../arch/arm/mach-ali3921/board_config.h>
#endif

#ifdef CONFIG_MIPS
#include <../arch/mips/ali/m36/board_config.h>
#endif

#include <alidefinition/adf_basic.h>
//#define ALIASIX_DEBUG
/* Log options */
#ifdef ALIASIX_DEBUG
#define ALIASIX_LOG(fmt, arg...) \
    do { \
        printk("ALI SECURITY: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_LOG(...) do{}while(0)
#endif

//#define ALIASIX_MNTDBG
/* Log options */
#ifdef ALIASIX_MNTDBG
#define ALIASIX_MNT(fmt, arg...) \
    do { \
        printk("ALI SECURITY MNT: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_MNT(...) do{}while(0)
#endif

////#define ALIASIX_RSADBG
/* Log options */
#ifdef ALIASIX_RSADBG
#define ALIASIX_RSA(fmt, arg...) \
    do { \
        printk("ALI SECURITY RSA: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_RSA(...) do{}while(0)
#endif

//#define ALIASIX_MEMCHKDBG
/* Log options */
#ifdef ALIASIX_MEMCHKDBG
#define ALIASIX_MEMCHK(fmt, arg...) \
    do { \
        printk("ALI SECURITY MEMCHK: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_MEMCHK(...) do{}while(0)
#endif

//#define ALIASIX_PERFDBG
/* Log options */
#ifdef ALIASIX_PERFDBG
#define ALIASIX_FILENAMEDBG
#define ALIASIX_PERF(fmt, arg...) \
    do { \
        printk("ALI SECURITY PERF: In %s %lu ms"fmt, \
				__func__, jiffies, ##arg); \
    } while (0)
#else
#define ALIASIX_PERF(...) do{}while(0)
#endif

//#define ALIASIX_FILENAMEDBG
#ifdef ALIASIX_FILENAMEDBG
#define ALIASIX_FILENAME(fmt, arg...) \
	do { \
	    printk("ALI SECURITY FILE NAME: In %s "fmt, __func__, ##arg); \
	} while (0)
#else
#define ALIASIX_FILENAME(...) do{}while(0)
#endif

//#define ALIASIX_DUMPDBG
#ifdef ALIASIX_DUMPDBG
#define ALIASIX_DUMP(info, data, len) \
    do { \
        u_long i = 0; \
        printk("ALI SECURITY: In %s "info, __func__); \
        printk("\n"); \
        for (i = 0; i < len; i++) \
            printk("0x%x ", (data)[i]); \
        printk("\n"); \
    } while (0)
#else
#define ALIASIX_DUMP(...) do{}while(0)
#endif

// #define ALIASIX_FILEVERDEBUG
#ifdef ALIASIX_FILEVERDEBUG
#define ALIASIX_FILEVER(fmt, arg...) \
	do { \
	    printk("ALI SECURITY FILE VERIFICATION: In %s "fmt, __func__, ##arg); \
	} while (0)
#else
#define ALIASIX_FILEVER(...) do{}while(0)
#endif

#define ALIASIX_ERR(fmt, arg...) \
    do { \
        printk("ALI SECURITY: In %s "fmt, __func__, ##arg); \
    } while (0)

//#define ALIASIX_PATHDBG
/* Log options */
#ifdef ALIASIX_PATHDBG
#define ALIASIX_PATH(fmt, arg...) \
    do { \
        printk("ALI SECURITY PATH: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_PATH(...) do{}while(0)
#endif

//#define ALIASIX_FILEEXECEPT
/* Log options */
#ifdef ALIASIX_FILEEXECEPT
#define ALIASIX_FILEEXECEPTION(fmt, arg...) \
    do { \
        printk("ALI SECURITY EXCEPTION FILE: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_FILEEXECEPTION(...) do{}while(0)
#endif

//#define ALIASIX_DAEMON
/* Log options */
#ifdef ALIASIX_DAEMON
#define ALIASIX_DAEMON_INFO(fmt, arg...) \
    do { \
        printk("ALI SECURITY DAEMON: In %s "fmt, __func__, ##arg); \
    } while (0)
#else
#define ALIASIX_DAEMON_INFO(...) do{}while(0)
#endif

/*
 * ALiasix fs macic number
 */
#define ALIASIX_MAGIC	0x43415d41 /* "AMAC" */

/*
 * Mount options from the kernel
 */
#define ALIASIX_VERIFICATION "verify"

/*
 * Common macros
 */
#define K 1024
#define M (1024 * K)
#define G (1024 * M)

/*
 * Length of digest out
 */
#define ALIASIX_HASH_DIGEST_LEN     32
#define ALIASIX_SOURCE_DIGEST_LEN   0x100
#define ALIASIX_SOURCE_DIGEST_LEN_ULONG (ALIASIX_SOURCE_DIGEST_LEN / sizeof(u32))

/*
 * Threshold of data buffer
 */
#define ALIASIX_BUF_MALLOC_THRESHOLD    (4 * K)
#define ALIASIX_BUF_PAGES_THRESHOLD     4

/*
 * At this stage, video buffer is free, so we can use it temporary
 * It's only usable at boot time (Rootfs mount time)
 */
#define ALIASIX_IMAGE_START     (__MM_VIDEO_BUF_START & 0xFFFFFFF)
#define ALIASIX_IMAGE_MAX_LEN   VDEC_FB_MEM_SIZE

/*
 * We have to use the MP memory for the AS signature temprory
 * To use the HW digest method, we need to reserv the memory
 * The last 4K is retained for the file signature
 */
#ifdef CONFIG_FILE_SIGNATURE_ALIASIX
#define ALIASIX_FILE_START     (__MM_FILE_SIGNATURE_TOP_ADDR & 0xFFFF000)
#define ALIASIX_FILE_MAX_LEN   (__MM_FILE_SIGNATURE_LEN - 0x1000)
#else
#define ALIASIX_FILE_START     (0xFFFFFFFF & 0xFFFF000)
#define ALIASIX_FILE_MAX_LEN   (0xFFFFFFFF)
#endif

/*
 * Max file to be digested
 */
#define ALIASIX_MAX_DIGEST_FILE     ALIASIX_FILE_MAX_LEN

/*
 * Digest struct for ALi as ix
 */
#define ALIASIX_DIGEST_IMAGE    0
#define ALIASIX_DIGEST_FILE     1

/* Check the digest */
typedef struct _aliasix_sha_digest_info
{
    struct file *file;
    u8  *p_buf;
    u8  *src;
    u8  *result;
    u_long u32_file_size;
    u8  u8_digest_type;
    int b_siginfile;
}aliasix_sha_digest_info;

/*
 * Invalid index
 */
#define ALIASIX_INVALID_INDEX   0xFFFFFFFF
#define ALIASIX_INVALID_FD      0xFFFFFFFF

/*
 * Never expire
 */
#define ALIASIX_LIVE_FOREVER    0xFFFFFFFF
#define US      1
#define MS      (1000 * US)
#define S       (1000 * MS)
#define MINU    60

/*
 * Interval to check the permission (30 mins)
 */
#define ALIASIX_PERMISSION_CHECK_INTERVAL   (90 * MS * MINU)

/*
 * Return value of need verify
 */
#define ALIASIX_SHA_NEED_VERIFY 9999

/*
 * The root directory
 */
#define ALIASIX_ROOT_DIR            "/"
#define ALIASIX_CURRENT_DIR         "."
#define ALIASIX_FATHER_DIR          ".."
#define ALIASIX_LOST_FOUND          "lost+found"

#define ALIASIX_MODE_ARX            0555

/*
 * How to initialize the items
 */
// #define ALLOC_PERM_ITEMS_STATIC_ALIASIX

/**
 * Allow permission exception
 */
// #define ALLOW_PERMISSION_EXCEPTION_ALIASIX

/**
 * Allow to use the old permission store in the array
 */
// #define REUSE_OPENED_PERMISSION_ALIASIX
#define ALLOW_OLD_PERMISSION_ALIASIX

#endif  /* _SECURITY_ALIASIX_H */
