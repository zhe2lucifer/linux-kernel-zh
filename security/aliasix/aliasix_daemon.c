/*
 *  ALi advanced security module
 *
 *  This file contains the ALi advanced verification daemon.
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

#include "aliasix_daemon.h"

/*
 * Directory parameters
 */
#define ALIASIX_DIR_BUF_LEN     (4 * PATH_MAX)

/*
 * Dirent structure: Linux don't have a standard one ?
 */
struct linux_dirent {
    long           d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};

/*
 * Minutes to launch the daemon
 */
#define ALIASIX_MINU_LAUNCH_DAEMON  10
#define ALIASIX_MS_DAEMON_RELAX     100
#define ALIASIX_MS_LOOKUP_RELAX     50

/*
 * Path which support lookup finish
 */
typedef struct _aliasix_daemon_path
{
    char *p_path;
    int  b_travel_back;
}aliasix_daemon_path;

/*
 * Wait queue to let work done
 */
static wait_queue_head_t aliasix_daemon_wait_queue;
#define ALIASIX_DAEMON_LOOKUP_TIMEOUT   (1000 * 60 * 1000)

/*
 * Mutex lock to avoid interrupt of file process
 */
static struct mutex aliasix_daemon_file_mutex;

/*
 * Time to relax
 */
#define ALIASIX_DAEMON_THREAD_RELAX     2000

/*
 * Macros to print the path
 */
// #define ALIASIX_DENTRIES_DBG
#ifdef ALIASIX_DENTRIES_DBG
#define ALIASIX_SHOW_DENTRIES(p_path, d_buf, r_len) \
    do { \
        char d_type; \
        int d_pos = 0; \
        struct linux_dirent *d = NULL; \
        char *a_path = kmalloc(PATH_MAX, GFP_KERNEL); \
        int i_path_len = 0; \
        strcpy(a_path, p_path); \
        if (0 != strcmp(p_path, ALIASIX_ROOT_DIR)) \
            strcat(a_path, "/"); \
        i_path_len = strlen(a_path); \
        printk("--------------- nread=%d ---------------\n", r_len); \
        printk("i-node#  file type  d_reclen  d_off   d_name\n"); \
        for (d_pos = 0; d_pos < r_len; )  \
        { \
            d = (struct linux_dirent *) (d_buf + d_pos); \
            if ((0 == strcmp(ALIASIX_CURRENT_DIR, d->d_name)) || \
                (0 == strcmp(ALIASIX_FATHER_DIR, d->d_name)) || \
                (0 == strcmp(ALIASIX_LOST_FOUND, d->d_name))) \
            { \
                d_pos += d->d_reclen; \
                continue; \
            } \
            printk("%8ld  ", d->d_ino); \
            d_type = *(d_buf + d_pos + d->d_reclen - 1); \
            printk("%-10s ", (d_type == DT_REG) ?  "regular" : \
                             (d_type == DT_DIR) ?  "directory" : \
                             (d_type == DT_FIFO) ? "FIFO" : \
                             (d_type == DT_SOCK) ? "socket" : \
                             (d_type == DT_LNK) ?  "symlink" : \
                             (d_type == DT_BLK) ?  "block dev" : \
                             (d_type == DT_CHR) ?  "char dev" : "???"); \
            strcat(a_path, (char *)d->d_name); \
            printk("%4d %10lld  %s\n", d->d_reclen, \
                   (long long) d->d_off, a_path); \
            a_path[i_path_len] = '\0'; \
            d_pos += d->d_reclen; \
        } \
        kfree(a_path); \
    } while (0)
#else
#define ALIASIX_SHOW_DENTRIES(...) do{}while(0)
#endif

/*
 * This files are from /proc, /sys, etc
 */
#define SPECIAL_PATH_IGNORE_CHECK(p_path) \
    ((0 == memcmp(p_path, "/proc", 5)) || \
     (0 == memcmp(p_path, "/sys", 4)) || \
     (0 == memcmp(p_path, "/dev", 4)) || \
     (0 == memcmp(p_path, "/web/proc", 9)) || \
     (0 == memcmp(p_path, "/web/sys", 8)) || \
     (0 == memcmp(p_path, "/web/dev", 8)) || \
     (0 == memcmp(p_path, "/stb/proc", 9)) || \
     (0 == memcmp(p_path, "/stb/sys", 8)) || \
     (0 == memcmp(p_path, "/stb/dev", 8)))

/*
 * Macro to schedule work
 */
#define ALIASIX_DAEMON_SCHEDULE_TMO(thread, p_path, name, err_tag, ret) \
    do { \
        u_long ret_wq = 0; \
        p_path.b_travel_back = 0; \
        kthread_run(thread, &p_path, name); \
        ret_wq = wait_event_timeout(aliasix_daemon_wait_queue,  \
                                    p_path.b_travel_back,  \
                                    ALIASIX_DAEMON_LOOKUP_TIMEOUT); \
    	if (0 == ret_wq)  \
    	{  \
            ret = -ETIMEDOUT;  \
            goto err_tag; \
    	} \
    } while (0)

/*
 * Daemon verfication check: if no verired, we'll verify in this function
 */
static void aliasix_daemon_verify(const char *name)
{
    int fd = ALIASIX_INVALID_FD;

    mutex_lock(&aliasix_daemon_file_mutex);

    /* 
     * Just verify the given path, the caller should  
     * assure that the open of the given path won't 
     * cause error. Internally, the path will be checked
     * when do the lookup, so the calling is reliable
     */

    /* When we do sys_open, we'll verify the file */
    fd = sys_open(name, O_RDONLY, ALIASIX_MODE_ARX);
    if (fd < 0) goto done;
    sys_close(fd);
done:
    mutex_unlock(&aliasix_daemon_file_mutex);
}

/*
 * The daemon thread to verify the file
 */
static void aliasix_daemon_verify_thread(aliasix_daemon_path *p_path)
{
    if (NULL != p_path && NULL != p_path->p_path)
    {
        p_path->b_travel_back = 0;
        aliasix_daemon_verify(p_path->p_path);
    }

    p_path->b_travel_back = 1;
    if (waitqueue_active(&aliasix_daemon_wait_queue))
        wake_up(&aliasix_daemon_wait_queue);
}

/*
 * Add the directories which includes the digested file
 * First time, we'll pass "/", this should be successful
 * Then, we ignore the special path
 */
static void aliasix_daemon_lookup(aliasix_daemon_path *p_path)
{
    char *d_buf = kmalloc(ALIASIX_DIR_BUF_LEN, GFP_KERNEL);
    int fd = ALIASIX_INVALID_FD;
    u_long r_len = 0;
    char *a_path = kmalloc(PATH_MAX, GFP_KERNEL);
    int i_path_len = 0;
    int d_pos = 0;
    char d_type; 
    struct linux_dirent *d = NULL;
    aliasix_daemon_path v_path;
    int ret = 0;

    if (NULL == p_path || NULL == p_path->p_path)
        goto err;

    if (SPECIAL_PATH_IGNORE_CHECK(p_path->p_path))
        goto err;

    fd = sys_open(p_path->p_path, O_RDONLY, ALIASIX_MODE_ARX);
    if (fd < 0) goto err;

    for ( ; ; )
    {        
        r_len = sys_getdents(fd, d_buf, ALIASIX_DIR_BUF_LEN);
        if (-1 == r_len) goto err;
        if (0 == r_len) break;

        ALIASIX_SHOW_DENTRIES(p_path->p_path, d_buf, r_len);

        strcpy(a_path, p_path->p_path); 
        if (0 != strcmp(p_path->p_path, ALIASIX_ROOT_DIR)) 
            strcat(a_path, "/"); 
        i_path_len = strlen(a_path); 
        for (d_pos = 0; d_pos < r_len; )  
        { 
            d = (struct linux_dirent *) (d_buf + d_pos); 
            if ((0 == strcmp(ALIASIX_CURRENT_DIR, d->d_name)) || \ 
                (0 == strcmp(ALIASIX_FATHER_DIR, d->d_name)) || \
                (0 == strcmp(ALIASIX_LOST_FOUND, d->d_name))) 
                goto continue_dentry; 
            d_type = *(d_buf + d_pos + d->d_reclen - 1);            
            strcat(a_path, (char *)d->d_name); 
            /* 
             * For the directory, we need to walk again 
             * For the file, we need to open, then close to verify the file
             */
            ALIASIX_DAEMON_INFO("Info, path information %s\n", a_path);
            v_path.p_path = a_path;
            if (DT_DIR == d_type)
                aliasix_daemon_lookup(&v_path);
            else
                ALIASIX_DAEMON_SCHEDULE_TMO(aliasix_daemon_verify_thread, 
                                            v_path, "asix_daemon_v",
                                            err, ret);
            a_path[i_path_len] = '\0'; 
continue_dentry:
            d_pos += d->d_reclen; 
            msleep(ALIASIX_DAEMON_THREAD_RELAX);
        }
            
		msleep(ALIASIX_DAEMON_THREAD_RELAX);
    }

err:
    kfree(d_buf);
    kfree(a_path);
}

/*
 * The lookup process will take so long time
 * So we run in a work queue to let other guys run
 */
static void aliasix_daemon_lookup_thread(aliasix_daemon_path *p_path)
{
    if (NULL != p_path)
    {
        p_path->b_travel_back = 0;
        aliasix_daemon_lookup(p_path);
    }

    p_path->b_travel_back = 1;
    if (waitqueue_active(&aliasix_daemon_wait_queue))
        wake_up(&aliasix_daemon_wait_queue);
}


/*
 * Get the first expired file full path
 */
static int aliasix_daemon_expire_file(char *p_path)
{
    return aliasix_perm_get_expire_file(p_path);
}

/*
 * Daemon thread to check the 
 */
int __attribute__((noinstrument)) aliasix_daemon_thread(void *unused)
{
    mm_segment_t u32_cur_ds = KERNEL_DS;
    int ret = -EPERM;
    char *p_path = kmalloc(PATH_MAX, GFP_KERNEL);
    aliasix_daemon_path path;
    
    u32_cur_ds = get_fs();
    set_fs(KERNEL_DS);

    /* Init wait queue */
    init_waitqueue_head(&aliasix_daemon_wait_queue);
    mutex_init(&aliasix_daemon_file_mutex);

    ALIASIX_DAEMON_INFO("Info: daemon thread launched.\n");
    msleep(((ALIASIX_MINU_LAUNCH_DAEMON * S) / (MS)) * MINU);
    ALIASIX_DAEMON_INFO("Info: daemon thread running.\n");
    /* 
     * 1  Get the whole directory content. For a RDONLY partition,
     *    we just check the content at the boot time (first enter of the thread)
     *    then add them to the directory list
     */
    path.p_path = ALIASIX_ROOT_DIR;
    ALIASIX_DAEMON_SCHEDULE_TMO(aliasix_daemon_lookup_thread, 
                                path, "asix_daemon_l",
                                err, ret);

    /* 2  For now, we just need to traverse the list */
    for ( ; ; )
    {
        /* 
         * 3  Check the expire time
         *    We check the time in permission function internally
         *    But we still check it here to light the burden of the thread
         */
        while (0 == (ret = aliasix_daemon_expire_file(p_path)))
        {   
            aliasix_daemon_path v_path;

            v_path.p_path = p_path;
            ALIASIX_DAEMON_INFO("Info: verify file %s\n", p_path);
            ALIASIX_DAEMON_SCHEDULE_TMO(aliasix_daemon_verify_thread, 
                                        v_path, "asix_daemon_v",
                                        next, ret);
next:
            msleep(ALIASIX_DAEMON_THREAD_RELAX);
            continue;
        }
        /* 4  Sleep to wait for next time check */
        msleep(ALIASIX_PERMISSION_CHECK_INTERVAL);
    }
out:
    ret = 0;
err:
    kfree(p_path);
    set_fs(u32_cur_ds);
    return 0;  
}
