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
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>

struct e2proc_dev {
	wait_queue_head_t wait;
	bool updated;
	char string[1024];
	umode_t mode;
	const char *name;
	const char *parent;
};

static struct e2proc_dev procdevs[] = {
	{
		.name = "stb/audio/aac",
		.mode = 0644,
		.updated = false,
		.string = "downmix\n",
	},
	{
		.name = "stb/audio/aac_choices",
		.mode = 0644,
		.updated = false,
		.string = "downmix passthrough\n",
	},
	{
		.name = "stb/audio/ac3",
		.mode = 0644,
		.updated = false,
		.string = "downmix\n"
	},
	{
		.name = "stb/audio/ac3_choices",
		.mode = 0644,
		.updated = false,
		.string = "downmix passthrough\n",
	},
	{
		.name = "stb/audio/ac3plus",
		.mode = 0644,
		.updated = false,
		.string = "force_ac3\n",
	},
	{
		.name = "stb/audio/ac3plus_choices",
		.mode = 0644,
		.updated = false,
		.string = "use_hdmi_caps force_ac3\n",
	},
	{
		.name = "stb/audio/delay_bitstream",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/audio/audio_delay_bitstream",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/audio/delay_pcm",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/audio/audio_delay_pcm",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/audio/j1_mute",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/avs/0/input",
		.mode = 0644,
		.updated = false,
		.string = "encoder\n",
	},
	{
		.name = "stb/avs/0/input_choices",
		.mode = 0644,
		.updated = false,
		.string = "encoder\n",
	},
	{
		.name = "stb/denc/0/wss",
		.mode = 0644,
		.updated = false,
		.string = "off\n",
	},
	{
		.name = "stb/fb/3dmode",
		.mode = 0644,
		.updated = false,
		.string = "off\n",
	},
	{
		.name = "stb/fb/3dmode_choices",
		.mode = 0644,
		.updated = false,
		.string = "off sidebyside topandbottom\n",
	},
	{
		.name = "stb/fb/dst_height",
		.mode = 0644,
		.updated = false,
		.string = "240\n",
	},
	{
		.name = "stb/fb/dst_left",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/fb/dst_top",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/fb/dst_width",
		.mode = 0644,
		.updated = false,
		.string = "2d0\n",
	},
	{
		.name = "stb/fb/znorm",
		.mode = 0644,
		.updated = false,
		.string = "50\n",
	},
	{
		.name = "stb/fp/rtc",
		.mode = 0644,
		.updated = false,
		.string = "1543652733\n",
	},
	{
		.name = "stb/fp/temp_sensor_avs",
		.mode = 0644,
		.updated = false,
		.string = "61\n",
	},
	{
		.name = "stb/fp/wakeup_time",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/fp/was_timer_wakeup",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/hdmi/preemphasis",
		.mode = 0644,
		.updated = false,
		.string = "off\n",
	},
	{
		.name = "stb/hdmi/output",
		.mode = 0644,
		.updated = false,
		.string = "on\n",
	},
	{
		.name = "stb/info/board_revision",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/info/boxtype",
		.mode = 0644,
		.updated = false,
		.string = "osninoali\n",
	},
	{
		.name = "stb/info/chipset",
		.mode = 0644,
		.updated = false,
		.string = "ALI3505\n",
	},
	{
		.name = "stb/info/model",
		.mode = 0644,
		.updated = false,
		.string = "M3528\n",
	},
	{
		.name = "stb/info/subtype",
		.mode = 0644,
		.updated = false,
		.string = "1\n",
	},
	{
		.name = "stb/info/version",
		.mode = 0644,
		.updated = false,
		.string = "20181201\n",
	},
	{
		.name = "stb/info/vumodel",
		.mode = 0644,
		.updated = false,
		.string = "ultimo\n",
	},
	{
		.name = "stb/power/standbyled",
		.mode = 0644,
		.updated = false,
		.string = "on\n",
	},
	{
		.name = "stb/video/alpha",
		.mode = 0644,
		.updated = false,
		.string = "255\n",
	},
	{
		.name = "stb/video/aspect",
		.mode = 0644,
		.updated = false,
		.string = "16:9\n",
	},
	{
		.name = "stb/video/policy",
		.mode = 0644,
		.updated = false,
		.string = "letterbox\n",
	},
	{
		.name = "stb/video/policy2",
		.mode = 0644,
		.updated = false,
		.string = "letterbox\n",
	},
	{
		.name = "stb/video/policy2_choices",
		.mode = 0644,
		.updated = false,
		.string = "letterbox panscan bestfit nonlinear\n",
	},
	{
		.name = "stb/video/policy_choices",
		.mode = 0644,
		.updated = false,
		.string = "letterbox panscan bestfit nonlinear\n",
	},
	{
		.name = "stb/video/videomode",
		.mode = 0644,
		.updated = false,
		.string = "720p50\n",
	},
	{
		.name = "stb/video/videomode_50hz",
		.mode = 0644,
		.updated = false,
		.string = "720p50\n",
	},
	{
		.name = "stb/video/videomode_60hz",
		.mode = 0644,
		.updated = false,
		.string = "720p50\n",
	},
	{
		.name = "stb/video/videomode_choices",
		.mode = 0644,
		.updated = false,
		.string = "pal ntsc 576i 480i 576p 480p 720p50 720p 1080i50 1080i 1080p50 1080p\n",
	},
	{
		.name = "stb/vmpeg/deinterlace",
		.mode = 0644,
		.updated = false,
		.string = "auto\n",
	},
	{
		.name = "stb/vmpeg/0/dst_height",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/vmpeg/0/dst_left",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/vmpeg/0/dst_top",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/vmpeg/0/dst_width",
		.mode = 0644,
		.updated = false,
		.string = "0\n",
	},
	{
		.name = "stb/vmpeg/0/framerate",
		.mode = 0644,
		.updated = false,
		.string = "25000\n",
	},
	{
		.name = "stb/vmpeg/0/progressive",
		.mode = 0644,
		.updated = false,
		.string = "25000\n",
	},
	{
		.name = "stb/vmpeg/0/xres",
		.mode = 0644,
		.updated = false,
		.string = "500\n",
	},
	{
		.name = "stb/vmpeg/0/yres",
		.mode = 0644,
		.updated = false,
		.string = "2d0\n",
	},
	{
		.name = "bus/nim_sockets",
		.mode = 0644,
		.updated = false,
		.string =
			"NIM Socket 0:\n"
			"    Type: DVB-S2\n"
			"    Name: DVB-S2 (Internal)\n"
			"    I2C_Device: 255\n"
			"    Has_Outputs: no\n"
			"    Frontend_Device: 0\n",
	},
};

static int e2proc_show(struct seq_file *m, void *v)
{
	struct e2proc_dev *dev = m->private;

	//printk("show %s : %s\n", dev->name, dev->string);

	seq_printf(m, "%s", dev->string);

	return 0;
}

static int e2proc_open(struct inode *inode, struct file *file)
{
	struct e2proc_dev *dev = (struct e2proc_dev *)PDE_DATA(inode);

	//printk("open %s\n", dev->name);

	return single_open(file, e2proc_show, dev);
}

extern void * memcpy(void *, const void *, __kernel_size_t);
extern void * memset(void *, int , __kernel_size_t);
static ssize_t e2proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct seq_file *seq = file->private_data;
	struct e2proc_dev *dev = seq->private;
	unsigned long n;
	char tmp[1024];

	n = count < 1024 ? count : 1024;

	//printk("write %s\n", dev->name);

	memcpy(tmp, dev->string, sizeof(dev->string));
	memset(dev->string, 0, sizeof(dev->string));
	if (copy_from_user(dev->string, buffer, n)) {
		memcpy(dev->string, tmp, sizeof(dev->string));
		return -EFAULT;
	} else {
		dev->updated = true;
		wake_up_interruptible_sync_poll(&dev->wait, POLLIN | POLLRDNORM);
	}

	//printk("%s --> %s\n", dev->name, dev->string);

	return n;
}

static unsigned int e2proc_poll(struct file * file, poll_table * wait)
{
	struct seq_file *seq = file->private_data;
	struct e2proc_dev *dev = seq->private;
	unsigned int mask = 0;

	//printk("poll %s\n", dev->name);

	poll_wait(file, &dev->wait, wait);

	mask |= (POLLOUT | POLLWRNORM);

	if (dev->updated) {
		mask |= POLLIN | POLLRDNORM;
		dev->updated = false;
	}

	return mask;
}

static const struct file_operations e2proc_fops = {
	.owner		= THIS_MODULE,
	.open		= e2proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= e2proc_write,
	.poll		= e2proc_poll,
};

static int __devinit alie2driver_init(void) 
{
	struct e2proc_dev *dev;
	int i;

	//printk("alie2driver init\n");

	proc_mkdir("stb", NULL);
	proc_mkdir("stb/audio", NULL);
	proc_mkdir("stb/avs/0", NULL);
	proc_mkdir("stb/denc", NULL);
	proc_mkdir("stb/power", NULL);
	proc_mkdir("stb/denc/0", NULL);
	proc_mkdir("stb/denc/0/wss", NULL);
	proc_mkdir("stb/fb", NULL);
	proc_mkdir("stb/fp", NULL);
	proc_mkdir("stb/hdmi", NULL);
	proc_mkdir("stb/info", NULL);
	proc_mkdir("stb/video", NULL);
	proc_mkdir("stb/vmpeg", NULL);
	proc_mkdir("stb/vmpeg/deinterlace", NULL);
	proc_mkdir("stb/vmpeg/0", NULL);

	for (i = 0; i < ARRAY_SIZE(procdevs); i++) {
		dev = &procdevs[i];
		init_waitqueue_head(&dev->wait);
		proc_create_data(dev->name, dev->mode, NULL, &e2proc_fops, dev);
	}

	return 0;
}

static void __exit alie2driver_exit(void)
{
}

module_init(alie2driver_init);
module_exit(alie2driver_exit);

MODULE_AUTHOR("Sam");
MODULE_DESCRIPTION("ALi e2 Driver");
MODULE_LICENSE("GPL");
