#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <alidefinition/adf_vpo.h>

extern struct vpo_device *g_vpo_dev;
extern struct vpo_device *g_sd_vpo_dev;
extern unsigned long __G_ALI_MM_STILL_FRAME_SIZE;

static struct proc_dir_entry * ali_fb_proc_info = NULL;

static int ali_fb_debug_open(struct inode *inode, struct file *file);

static const struct file_operations ali_fb_debug_fops = {
	.owner   = THIS_MODULE,
	.open    = ali_fb_debug_open,
	.read    = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


static int ali_fb_debug_show(struct seq_file *sf, void *unused)
{
	int sd = g_sd_vpo_dev == 0?0:1;
	int hd = g_vpo_dev == 0?0:1;
	int support_still = 0;

	if(0 == __G_ALI_MM_STILL_FRAME_SIZE )
		support_still = 0;
	else
		support_still = 1;

	seq_printf(sf, "vpo_sd_support=%d\n", sd);
	seq_printf(sf, "vpo_hd_support=%d\n", hd);
	seq_printf(sf, "support_still=%d\n", support_still);
	return 0;
}

static int ali_fb_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, ali_fb_debug_show, PDE_DATA(inode));
}

int ali_fb_procfs_init(void)
{
	if(ali_fb_proc_info == NULL)
		ali_fb_proc_info = proc_create("sysinfo", 0, NULL, &ali_fb_debug_fops);
	return 0;
}

