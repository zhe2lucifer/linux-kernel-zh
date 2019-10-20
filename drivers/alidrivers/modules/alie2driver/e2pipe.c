/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#define RING_SIZE 0x100000
#define my_debug(...) do{}while(0)

#define CHRDEV "e2pipe"
static struct class *pe2pipedev_class;
static struct device *pe2pipedev;
static int e2pipe_cdev_major;
static int __attribute__ ((__unused__)) nesting = 0;

struct e2pipe_rb {
	wait_queue_head_t wait;
	char *start;
	char *end;
	char *r;
	char *w;
	bool full;
	struct mutex mutex;
};

static inline void __pipe_lock(struct e2pipe_rb *pipe)
{
	mutex_lock_nested(&pipe->mutex, I_MUTEX_PARENT);
}

static inline void __pipe_unlock(struct e2pipe_rb *pipe)
{
	mutex_unlock(&pipe->mutex);
}


static void rb_wait(struct e2pipe_rb *rb)
{
	DEFINE_WAIT(wait);

	/*
	 * Pipes are system-local resources, so sleeping on them
	 * is considered a noninteractive wait:
	 */
	prepare_to_wait(&rb->wait, &wait, TASK_INTERRUPTIBLE);
	__pipe_unlock(rb);
	schedule();
	finish_wait(&rb->wait, &wait);
	__pipe_lock(rb);
}

static int e2pipe_open(struct inode *inodep, struct file *filp)
{
	struct e2pipe_rb *rb;

	rb = (struct e2pipe_rb *) kmalloc (sizeof (struct e2pipe_rb), GFP_KERNEL);
	if (!rb)
		return -ENOMEM;
	
	memset(rb, 0, sizeof (struct e2pipe_rb));

	init_waitqueue_head(&rb->wait);
	mutex_init(&rb->mutex);
	
	filp->private_data = rb;
	my_debug ("pipe open %d\n", ++nesting);

	return 0;
}

static ssize_t e2pipe_read(struct file *filp, char __user *buffer, size_t len, loff_t *offset)
{
	struct e2pipe_rb *rb = filp->private_data;
	size_t count = 0;
	size_t copied = 0;

	if (rb->start == NULL) {
		my_debug ("read case 0\n");
		return 0;
	}
	
	__pipe_lock(rb);

	if (rb->w == rb->r && !rb->full) {
		if (!(filp->f_flags & O_NONBLOCK)) {
			rb_wait(rb);
		}
	}

	if (rb->w > rb->r) {
		count = (size_t) (rb->w - rb->r);
		if (count > len) {
			count = len;
		}

		copy_to_user (buffer, rb->r, count);
		rb->r += count;
		copied += count;

		if (rb->r == rb->end) {
			rb->r = rb->start;
		}

		my_debug ("read case 1\n");
		goto copy_done;
	}

	if (rb->w < rb->r) {
		count = (size_t) (rb->end - rb->r);
		if (count > len) {
			count = len;
		}

		copy_to_user (buffer, rb->r, count);
		rb->r += count;
		copied += count;

		if (rb->r == rb->end) {
			rb->r = rb->start;
		}

		if (copied == len) {
			my_debug ("read case 2.1\n");
			goto copy_done;
		}


		count = (size_t) (rb->w - rb->r);
		if (count == 0) {
			my_debug ("read case 2.2\n");
			goto copy_done;
		}

		if (count > (len - copied)) {
			count = len - copied;
		}

		copy_to_user (buffer + copied, rb->r, count);
		rb->r += count;
		copied += count;

		my_debug ("read case 2.3\n");
		goto copy_done;
	}

	if (rb->w == rb->r && rb->full) {
		count = (size_t) (rb->end - rb->r);
		if (count > len) {
			count = len;
		}

		copy_to_user (buffer, rb->r, count);
		rb->r += count;
		copied += count;
		rb->full = false;

		if (rb->r == rb->end) {
			rb->r = rb->start;
		}

		if (copied == len) {
			my_debug ("read case 3.1\n");
			goto copy_done;
		}

		count = (size_t) (rb->w - rb->r);
		if (count == 0) {
			my_debug ("read case 3.2\n");
			goto copy_done;
		}

		if (count > (len - copied)) {
			count = len - copied;
		}

		copy_to_user (buffer + copied, rb->r, count);
		rb->r += count;
		copied += count;

		my_debug ("read case 3.3\n");
		goto copy_done;
	}

copy_done:

	__pipe_unlock(rb);

	if (copied) {
		wake_up_interruptible_sync_poll(&rb->wait, POLLOUT | POLLWRNORM);
	}

	my_debug ("read 0x%x, 0x%x\n", len, copied);

	return copied;
}

static ssize_t e2pipe_write(struct file *filp, const char __user *buffer, size_t len, loff_t *offset)
{
	struct e2pipe_rb *rb = filp->private_data;
	size_t count = 0;
	size_t copied = 0;

	if (rb->start == NULL) {
		rb->start = (char *) kmalloc(RING_SIZE, GFP_KERNEL);
		if (!rb->start)
			return -ENOMEM;

		rb->end = rb->start + RING_SIZE;
		rb->r = rb->start;
		rb->w = rb->start;
		rb->full = false;
		my_debug("malloc buffer in write\n");
	}

	__pipe_lock(rb);
	if (rb->w == rb->r && rb->full) {
		if (!(filp->f_flags & O_NONBLOCK)) {
			rb_wait(rb);
		}
	}

	if (rb->w < rb->r) {
		count = (size_t) (rb->r - rb->w);
		if (count > len) {
			count = len;
		}

		copy_from_user(rb->w, buffer, count);
		rb->w += count;
		copied += count;

		if (rb->w == rb->r) {
			my_debug ("full 1\n");
			rb->full = true;
		}

		my_debug ("write case 1\n");
		goto copy_done;
	}

	if (rb->w > rb->r) {
		count = (size_t) (rb->end - rb->w);
		if (count > len) {
			count = len;
		}

		copy_from_user(rb->w, buffer, count);
		rb->w += count;
		copied += count;

		if (rb->w == rb->end) {
			rb->w = rb->start;
		}

		if (rb->w == rb->r) {
			my_debug ("full 2\n");
			rb->full = true;
		}

		if (copied == len) {
			my_debug ("write case 2.1\n");
			goto copy_done;
		}

		count = (size_t) (rb->r - rb->w);
		if (count == 0) {
			my_debug ("write case 2.2\n");
			goto copy_done;
		}

		if (count > (len - copied)) {
			count = len - copied;
		}

		copy_from_user (rb->w, buffer + copied, count);
		rb->w += count;
		copied += count;
		if (rb->w == rb->r) {
			my_debug ("full 3\n");
			rb->full = true;
		}

		my_debug ("write case 2.3\n");
		goto copy_done;
	}

	if (rb->w == rb->r && !rb->full) {
		count = (size_t) (rb->end - rb->w);
		if (count > len) {
			count = len;
		}

		copy_from_user (rb->w, buffer, count);

		rb->w += count;
		copied += count;

		if (rb->w == rb->end) {
			rb->w = rb->start;
		}

		if (rb->w == rb->r) {
			my_debug ("full 4\n");
			rb->full = true;
		}

		if (copied == len) {
			my_debug ("write case 3.1\n");
			goto copy_done;
		}

		count = (size_t) (rb->r - rb->w);
		if (count == 0) {
			my_debug ("write case 3.2\n");
			goto copy_done;
		}

		if (count > (len - copied)) {
			count = len - copied;
		}

		copy_from_user (rb->w, buffer + copied, count);
		rb->w += count;
		copied += count;

		if (rb->w == rb->r) {
			my_debug ("full 5\n");
			rb->full = true;
		}

		my_debug ("write case 3.3\n");
		goto copy_done;
	}

copy_done:

	__pipe_unlock(rb);

	if (copied) {
		wake_up_interruptible_sync_poll(&rb->wait, POLLIN | POLLRDNORM);
	}

	my_debug ("write 0x%x, 0x%x\n", len, copied);

	return copied;
}

static int e2pipe_release(struct inode *inodep, struct file *filp)
{
	struct e2pipe_rb *rb = filp->private_data;

	if (rb->start) {
		kfree (rb->start);
	}

	kfree(rb);

	my_debug ("pipe release %d\n", --nesting);

	return 0;
}

static unsigned int e2pipe_poll (struct file * file, poll_table * wait)
{
	struct e2pipe_rb *rb = file->private_data;
	unsigned int mask = 0;

	poll_wait (file, &rb->wait, wait);

	if (rb->start == NULL) {
		rb->start = (char *) kmalloc(RING_SIZE, GFP_KERNEL);
		if (!rb->start)
			return -ENOMEM;

		rb->end = rb->start + RING_SIZE;
		rb->r = rb->start;
		rb->w = rb->start;
		rb->full = false;
		my_debug("malloc buffer in poll\n");
	}

	__pipe_lock(rb);

	if (rb->w != rb->r) {
		my_debug("poll r/w\n");
		mask |= POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM;
	}

	if (rb->w == rb->r && rb->full) {
		my_debug("poll r\n");
		mask |= POLLIN | POLLRDNORM;
	}

	if (rb->w == rb->r && !rb->full) {
		my_debug("poll w\n");
		mask |= (POLLOUT | POLLWRNORM);
	}

	__pipe_unlock(rb);

	return mask;
}

static int e2pipe_flush(struct file *file, fl_owner_t id)
{
	struct e2pipe_rb *rb = file->private_data;

	__pipe_lock(rb);

	if (rb->start != NULL) {
		printk("flush e2pipe\n");
		rb->end = rb->start + RING_SIZE;
		rb->r = rb->start;
		rb->w = rb->start;
		rb->full = false;
	}
	__pipe_unlock(rb);

	return 0;
}

static long e2pipe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations e2pipe_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.read		= e2pipe_read,
	.write		= e2pipe_write,
	.poll		= e2pipe_poll,
	.unlocked_ioctl	= e2pipe_ioctl,
	.open		= e2pipe_open,
	.release	= e2pipe_release,
	.flush		= e2pipe_flush,
};

static int __devinit alie2pipe_init(void) 
{
	my_debug("alie2pipe init\n");

	e2pipe_cdev_major = register_chrdev(0, CHRDEV, &e2pipe_fops);
	if (e2pipe_cdev_major < 0) {
		my_debug (KERN_WARNING CHRDEV ": unable to register chrdev\n");
		return -EIO;
	}
	pe2pipedev_class = class_create(THIS_MODULE, CHRDEV);
	if (IS_ERR(pe2pipedev_class)) {
		unregister_chrdev(e2pipe_cdev_major, CHRDEV);
		return PTR_ERR(pe2pipedev_class);;
	}

	pe2pipedev = device_create(pe2pipedev_class, NULL, MKDEV(e2pipe_cdev_major, 0), NULL, "e2pipe");

	if (IS_ERR(pe2pipedev)) {
		class_destroy(pe2pipedev_class);
		unregister_chrdev(e2pipe_cdev_major, CHRDEV);
		return PTR_ERR(pe2pipedev);
	}

	return 0;
}

static void __exit alie2pipe_exit(void)
{
	device_destroy(pe2pipedev_class, MKDEV(e2pipe_cdev_major, 0));
	class_unregister(pe2pipedev_class);
	class_destroy(pe2pipedev_class);
	unregister_chrdev(e2pipe_cdev_major, CHRDEV);

	return;
}

module_init(alie2pipe_init);
module_exit(alie2pipe_exit);

MODULE_AUTHOR("SAM");
MODULE_DESCRIPTION("ALi e2 pipe");
MODULE_LICENSE("GPL");
