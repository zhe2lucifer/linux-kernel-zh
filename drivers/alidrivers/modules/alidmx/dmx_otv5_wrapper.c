/**@file
 *  (c) Copyright 2013-2999  ALi Corp. ZHA Linux SDK Team (alitech.com)
 *  All rights reserved 
 *
 *  @file               dmx_otv5_wrapper.c
 *  @brief              A dmx wrapper layyer to support OTV5
 *
 *  @version            1.0
 *  @date               09/23/2014 10:56:33 AM
 *  @revision           none
 *
 *  @author             Stephen Xiao <stephen.xiao@alitech.com>
 */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/personality.h> /* for STICKY_TIMEOUTS */
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/rcupdate.h>
#include <linux/hrtimer.h>
#include <linux/sched/rt.h>
#include <linux/freezer.h>
#include <net/busy_poll.h>

#include <asm/uaccess.h>

//#include "dmx_stack.h"
#include "dmx_otv5_wrapper.h"

extern struct dmx_linux_interface_module_legacy ali_dmx_linux_interface_module_legacy;
extern struct dmx_channel_module_legacy ali_dmx_channel_module_legacy;

/* ========================================================================== */
/* Use for OTV5                                                               */
/* ========================================================================== */
int dmx_channel_open_wrapper(int index)
{
    struct inode node;
    struct file file_t;
    struct dmx_channel       *ch;
    int i = 0;
    int fd;
    int ret = 0;

    node.i_cdev = &ali_dmx_linux_interface_module_legacy.output_device[index].cdev;
    ret = dmx_channel_open(&node, &file_t);
    
    if(ret == 0)
    {
        for (; i < DMX_CHANNEL_CNT; i++)
        {
            ch = &(ali_dmx_channel_module_legacy.channel[i]);

            if (file_t.private_data == ch)
            {
                DMX_LINUX_API_DEBUG("Got channel index %d\n", i);
                break;
            }
        }
    }
    
    if(i >= DMX_CHANNEL_CNT) 
    {
        fd = -1;     
    }
    else
    {
       fd = (index << 16)|(i & 0xFF);
    }

    return fd;
}
EXPORT_SYMBOL(dmx_channel_open_wrapper);

int dmx_channel_close_wrapper(int fd)
{
    struct inode node;
    struct file file_t;
    int ret = 0;
    int i = fd & 0xFF;
    int index = (fd >> 16) & 0xFF;

    memset(&file_t, 0, sizeof(file_t));

    file_t.private_data = &(ali_dmx_channel_module_legacy.channel[i]);
    node.i_cdev = &ali_dmx_linux_interface_module_legacy.output_device[index].cdev;

    ret = dmx_channel_close(&node, &file_t);

    return ret;
       
}
EXPORT_SYMBOL(dmx_channel_close_wrapper);

int dmx_channel_read_wrapper(int fd, char *buf, size_t count)
{
    struct file file_t;
    int ret = 0;
    int i = fd & 0xFF;
    loff_t t = 0;

    memset(&file_t, 0, sizeof(file_t));

    file_t.f_flags |= O_RDWR;
    file_t.private_data = &(ali_dmx_channel_module_legacy.channel[i]);

    ret = dmx_channel_read(&file_t, buf, count, &t);

    return ret;
}
EXPORT_SYMBOL(dmx_channel_read_wrapper);

int dmx_channel_ioctl_wrapper(int fd, unsigned int cmd, unsigned long arg)
{
    struct file file_t;
    int ret = 0;
    int i = fd & 0xFF;
    memset(&file_t, 0, sizeof(file_t));

    file_t.f_flags |= O_RDWR;
    file_t.private_data = &(ali_dmx_channel_module_legacy.channel[i]);

    ret = dmx_channel_ioctl(&file_t, cmd, arg);

    return ret;
}
EXPORT_SYMBOL(dmx_channel_ioctl_wrapper);


/*
 * This poll mechanism is porting from linux kernel
 */
struct poll_list {
	struct poll_list *next;
	int len;
	struct pollfd entries[0];
};

#define POLLFD_PER_PAGE  ((PAGE_SIZE-sizeof(struct poll_list)) / sizeof(struct pollfd))

/*
 * Fish for pollable events on the pollfd->fd file descriptor. We're only
 * interested in events matching the pollfd->events mask, and the result
 * matching that mask is both recorded in pollfd->revents and returned. The
 * pwait poll_table will be used by the fd-provided poll handler for waiting,
 * if pwait->_qproc is non-NULL.
 */
static inline unsigned int do_pollfd(struct pollfd *pollfd, poll_table *pwait,
				     bool *can_busy_poll,
				     unsigned int busy_flag)
{
	unsigned int mask;
	int fd;
    int i;
    int index;
    struct file file_t;

	mask = 0;
	fd = pollfd->fd;
    if (fd >= 0) {
        i = fd & 0xFF;
        index = (fd >> 16) & 0xFF;
        file_t.f_flags |= O_RDWR;
        file_t.private_data = &(ali_dmx_channel_module_legacy.channel[i]);
        mask = DEFAULT_POLLMASK;
        pwait->_key = pollfd->events|POLLERR|POLLHUP;
        pwait->_key |= busy_flag;
        mask = dmx_channel_poll(&file_t, pwait);
        if (mask & busy_flag)
            *can_busy_poll = true;

        /* Mask out unneeded events. */
        mask &= pollfd->events | POLLERR | POLLHUP;
    }
    pollfd->revents = mask;

    return mask;
}

static int do_poll(unsigned int nfds,  struct poll_list *list,
		   struct poll_wqueues *wait, struct timespec *end_time)
{
	poll_table* pt = &wait->pt;
	ktime_t expire, *to = NULL;
	int timed_out = 0, count = 0;
	unsigned long slack = 0;
	unsigned int busy_flag = net_busy_loop_on() ? POLL_BUSY_LOOP : 0;
	unsigned long busy_end = 0;

	/* Optimise the no-wait case */
	if (end_time && !end_time->tv_sec && !end_time->tv_nsec) {
		pt->_qproc = NULL;
		timed_out = 1;
	}

	if (end_time && !timed_out)
		slack = select_estimate_accuracy(end_time);

	for (;;) {
		struct poll_list *walk;
		bool can_busy_loop = false;

		for (walk = list; walk != NULL; walk = walk->next) {
			struct pollfd * pfd, * pfd_end;

			pfd = walk->entries;
			pfd_end = pfd + walk->len;
			for (; pfd != pfd_end; pfd++) {
				/*
				 * Fish for events. If we found one, record it
				 * and kill poll_table->_qproc, so we don't
				 * needlessly register any other waiters after
				 * this. They'll get immediately deregistered
				 * when we break out and return.
				 */
				if (do_pollfd(pfd, pt, &can_busy_loop,
					      busy_flag)) {
					count++;
					pt->_qproc = NULL;
					/* found something, stop busy polling */
					busy_flag = 0;
					can_busy_loop = false;
				}
			}
		}
		/*
		 * All waiters have already been registered, so don't provide
		 * a poll_table->_qproc to them on the next loop iteration.
		 */
		pt->_qproc = NULL;
		if (!count) {
			count = wait->error;
			if (signal_pending(current))
				count = -EINTR;
		}
		if (count || timed_out)
			break;

		/* only if found POLL_BUSY_LOOP sockets && not out of time */
		if (can_busy_loop && !need_resched()) {
			if (!busy_end) {
				busy_end = busy_loop_end_time();
				continue;
			}
			if (!busy_loop_timeout(busy_end))
				continue;
		}
		busy_flag = 0;

		/*
		 * If this is the first loop and we have a timeout
		 * given, then we convert to ktime_t and set the to
		 * pointer to the expiry value.
		 */
		if (end_time && !to) {
			expire = timespec_to_ktime(*end_time);
			to = &expire;
		}

		if (!poll_schedule_timeout(wait, TASK_INTERRUPTIBLE, to, slack))
			timed_out = 1;
	}
	return count;
}

#define N_STACK_PPS ((sizeof(stack_pps) - sizeof(struct poll_list))  / \
			sizeof(struct pollfd))

static int ali_do_sys_poll(struct pollfd *ufds, unsigned int nfds,
		struct timespec *end_time)
{
	struct poll_wqueues table;
 	int err = -EFAULT, fdcount, len, size;
	/* Allocate small arguments on the stack to save memory and be
	   faster - use long to make sure the buffer is aligned properly
	   on 64 bit archs to avoid unaligned access */
	long stack_pps[POLL_STACK_ALLOC/sizeof(long)];
	struct poll_list *const head = (struct poll_list *)stack_pps;
 	struct poll_list *walk = head;
 	unsigned long todo = nfds;

	if (nfds > rlimit(RLIMIT_NOFILE))
		return -EINVAL;

	len = min_t(unsigned int, nfds, N_STACK_PPS);
	for (;;) {
		walk->next = NULL;
		walk->len = len;
		if (!len)
			break;

		if (copy_from_user(walk->entries, ufds + nfds-todo,
					sizeof(struct pollfd) * walk->len))
			goto out_fds;

		todo -= walk->len;
		if (!todo)
			break;

		len = min(todo, POLLFD_PER_PAGE);
		size = sizeof(struct poll_list) + sizeof(struct pollfd) * len;
		walk = walk->next = kmalloc(size, GFP_KERNEL);
		if (!walk) {
			err = -ENOMEM;
			goto out_fds;
		}
	}
    
	poll_initwait(&table);
	fdcount = do_poll(nfds, head, &table, end_time);
	poll_freewait(&table);

	for (walk = head; walk; walk = walk->next) {
		struct pollfd *fds = walk->entries;
		int j;

		for (j = 0; j < walk->len; j++, ufds++)
			if (__put_user(fds[j].revents, &ufds->revents))
				goto out_fds;
  	}

	err = fdcount;
out_fds:
	walk = head->next;
	while (walk) {
		struct poll_list *pos = walk;
		walk = walk->next;
		kfree(pos);
	}

	return err;
}

static long do_restart_poll(struct restart_block *restart_block)
{
	struct pollfd __user *ufds = restart_block->poll.ufds;
	int nfds = restart_block->poll.nfds;
	struct timespec *to = NULL, end_time;
	int ret;

	if (restart_block->poll.has_timeout) {
		end_time.tv_sec = restart_block->poll.tv_sec;
		end_time.tv_nsec = restart_block->poll.tv_nsec;
		to = &end_time;
	}

	ret = do_sys_poll(ufds, nfds, to);

	if (ret == -EINTR) {
		restart_block->fn = do_restart_poll;
		ret = -ERESTART_RESTARTBLOCK;
	}
	return ret;
}

int dmx_channel_poll_wrapper(struct pollfd *ufds, unsigned int nfds,
		long timeout_msecs)
{
	struct timespec end_time, *to = NULL;
	int ret;

	if (timeout_msecs >= 0) {
		to = &end_time;
		poll_select_set_timeout(to, timeout_msecs / MSEC_PER_SEC,
			NSEC_PER_MSEC * (timeout_msecs % MSEC_PER_SEC));
	}

	ret = ali_do_sys_poll(ufds, nfds, to);

	if (ret == -EINTR) {
		struct restart_block *restart_block;

		restart_block = &current_thread_info()->restart_block;
		restart_block->fn = do_restart_poll;
		restart_block->poll.ufds = ufds;
		restart_block->poll.nfds = nfds;

		if (timeout_msecs >= 0) {
			restart_block->poll.tv_sec = end_time.tv_sec;
			restart_block->poll.tv_nsec = end_time.tv_nsec;
			restart_block->poll.has_timeout = 1;
		} else
			restart_block->poll.has_timeout = 0;

		ret = -ERESTART_RESTARTBLOCK;
	}
	return ret;
}
EXPORT_SYMBOL(dmx_channel_poll_wrapper);
