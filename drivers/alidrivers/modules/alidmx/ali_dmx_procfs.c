#include <linux/module.h>

#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include "dmx_see_interface.h"

#define DMX_DEBUG_PROC_DIR         "alidmx"
#define DMX_DEBUG_PROC_INFO     "debuginfo"
#define DMX_DATA_CONTROL_OPTION    "datacontrol"
#define DBUG_MAX_BUF_LEN 2048 //do not bigger than one page size 1024 bytes
#define MAX_BUF_LEN 1000 //do not bigger than one page size 1024 bytes


static struct proc_dir_entry *dmx_proc_dir = NULL;
static struct proc_dir_entry *dmx_proc_dbginfo_file = NULL;
static struct proc_dir_entry *dmx_data_control_file = NULL;

static int dmx_datalow_control_level =0;

static int dmx_read_debug_info(char * buffer)
{
    int len = 0;
    unsigned long len_max = DBUG_MAX_BUF_LEN - 100;
    sed_dmx_dbg_info_t dmx_dbg_info;

    len += sprintf(&buffer[len],"****See Dmx Debug Info****\n");
    if (len_max <= len)
            goto out;
    memset(&dmx_dbg_info, 0, sizeof(sed_dmx_dbg_info_t));
    sed_dmx_get_dbg_info(0, &dmx_dbg_info);
    len += sprintf(&buffer[len],"aud_pid : %d\n", (dmx_dbg_info.aud_pid));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"vde_pid : %d\n", (dmx_dbg_info.vde_pid));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"aud_pkt_num : %d\n", (int)(dmx_dbg_info.aud_pkt_num));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"vde_pkt_num : %d\n", (int)(dmx_dbg_info.vde_pkt_num));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"aud_pkt_discontinue : %d\n", (int)(dmx_dbg_info.aud_pkt_discontinue));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"vde_pkt_discontinue : %d\n", (int)(dmx_dbg_info.vde_pkt_discontinue));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"total_pkt_discontinue : %d\n", (int)(dmx_dbg_info.total_pkt_discontinue));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"aud_pes_lost_num : %d\n", (int)(dmx_dbg_info.aud_pes_lost_num));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"vde_pes_lost_num : %d\n", (int)(dmx_dbg_info.vde_pes_lost_num));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"parse_vde_pts_instream: 0x%08x\n", (int)(dmx_dbg_info.parse_vde_pts_instream));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"parse_aud_pts_instream: 0x%08x\n", (int)(dmx_dbg_info.parse_aud_pts_instream));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"parse_stream_busy_cnt: %d\n", (int)(dmx_dbg_info.parse_stream_busy_cnt));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"req_vde_busy_cnt: %d\n", (int)(dmx_dbg_info.sed_vde_req_busy_cnt));
    if (len_max <= len)
            goto out;

    len += sprintf(&buffer[len],"req_vde_fail_cnt: %d\n", (int)(dmx_dbg_info.sed_vde_req_fail_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_vde_ok_cnt: %d\n", (int)(dmx_dbg_info.sed_vde_req_ok_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_aud_busy_cnt: %d\n", (int)(dmx_dbg_info.sed_aud_req_busy_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_aud_fail_cnt: %d\n", (int)(dmx_dbg_info.sed_aud_req_fail_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_aud_ok_cnt: %d\n", (int)(dmx_dbg_info.sed_aud_req_ok_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_other_busy_cnt: %d\n", (int)(dmx_dbg_info.sed_other_req_busy_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_other_fail_cnt: %d\n", (int)(dmx_dbg_info.sed_other_req_fail_cnt));
    if (len_max <= len)
            goto out;
    len += sprintf(&buffer[len],"req_other_ok_cnt: %d\n", (int)(dmx_dbg_info.sed_other_req_ok_cnt));
    if (len_max <= len)
            goto out;

out:

    return len;
}

/*Process debug info*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static ssize_t dmx_dbginfo_procfile_read(struct file *file, char __user *ubuf, size_t size, loff_t *ppos)
{
	int len = 0;
	char *buffer = kmalloc(DBUG_MAX_BUF_LEN, GFP_KERNEL);
	ssize_t ret_len = 0;

	if(!buffer)
		return ret_len;

	memset(buffer, 0, DBUG_MAX_BUF_LEN);

	len = dmx_read_debug_info(buffer);

	ret_len = simple_read_from_buffer(ubuf, size, ppos, buffer, len);

	kfree(buffer);

	return ret_len;
}

static ssize_t dmx_dbginfo_procfile_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
	return count;
}

static ssize_t dmx_dataflow_control_procfile_read(struct file *file, char __user *ubuf, size_t size, loff_t *ppos)
{
	char buffer[MAX_BUF_LEN] = {0,};
	int len = 0;
	len += sprintf(&buffer[len],"controllevel=0x%08x\n",dmx_datalow_control_level);

	return simple_read_from_buffer(ubuf, size, ppos, buffer, len);
}

static ssize_t dmx_dataflow_control_procfile_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
	char buf[MAX_BUF_LEN] = {0,};
	if(count<=0 || count > MAX_BUF_LEN)
	  return 0;
	if (copy_from_user(buf, buffer, count))
	  return -EFAULT;
	if (sscanf(buf, "controllevel=0x%08x",&dmx_datalow_control_level) != 1)
	{
		return 0;
	}
    sed_dmx_dataflow_control_set(0, dmx_datalow_control_level);

	return count;

}
#else
static int dmx_dbginfo_procfile_read(char*buffer, char**buffer_localation, off_t offset,int buffer_length,int* eof, void *data )
{
	int len = 0;
	len =  dmx_read_debug_info(buffer);
	*eof = 1;
        return len;
}

static int dmx_dbginfo_procfile_write(struct file *filp, const char *buffer,unsigned long count,void *data)
{
        return count;
}

static int dmx_dataflow_control_procfile_read(char*buffer, char**buffer_localation, off_t offset,int buffer_length,int* eof, void *data )
{
	int len = 0;
	len += sprintf(&buffer[len],"controllevel=0x%08x\n",dmx_datalow_control_level);

	*eof  = 1;
	return len;
}

static int dmx_dataflow_control_procfile_write(struct file *filp, const char *buffer,unsigned long count,void *data)
{
	char buf[MAX_BUF_LEN] = {0,};
	if(count<=0 || count > MAX_BUF_LEN)
	  return 0;

	if (copy_from_user(buf, buffer, count))
	  return -EFAULT;
	if (sscanf(buf, "controllevel=0x%08x",&dmx_datalow_control_level) != 1)
	{
		return 0;
	}
    sed_dmx_dataflow_control_set(0, dmx_datalow_control_level);
	return count;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static const struct file_operations alidmx_debuginfo_fops = {
	.read = dmx_dbginfo_procfile_read,
	.write = dmx_dbginfo_procfile_write,
	.llseek = default_llseek,
};

static const struct file_operations alidmx_dataflow_control_fops = {
	.read = dmx_dataflow_control_procfile_read,
	.write = dmx_dataflow_control_procfile_write,
	.llseek = default_llseek,
};
#endif

int  dmx_debug_procfs_init(void)
{
    dmx_proc_dir = proc_mkdir(DMX_DEBUG_PROC_DIR, NULL);

    if (dmx_proc_dir == NULL) {
        pr_err("Error: dmx_debug_procfs_init create dir alidmx failed!!\n");
        return -1;
    }

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 10, 0))
    /*For Debug info*/
    dmx_proc_dbginfo_file = proc_create(DMX_DEBUG_PROC_INFO,0644,dmx_proc_dir, &alidmx_debuginfo_fops);
#else
    /*For Debug info*/
    dmx_proc_dbginfo_file = create_proc_entry(DMX_DEBUG_PROC_INFO,0644,dmx_proc_dir);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
        dmx_data_control_file = proc_create(DMX_DATA_CONTROL_OPTION,0644,dmx_proc_dir, &alidmx_dataflow_control_fops);
#else
        dmx_data_control_file = create_proc_entry(DMX_DATA_CONTROL_OPTION,0644,dmx_proc_dir);
#endif

    if(dmx_data_control_file == NULL)
    {
        remove_proc_entry(DMX_DEBUG_PROC_INFO, NULL);
        pr_err("Error:could not initialize /proc/%s/%s\n",DMX_DEBUG_PROC_INFO,DMX_DATA_CONTROL_OPTION);
        return -1;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
    dmx_data_control_file->read_proc = dmx_dataflow_control_procfile_read;
    dmx_data_control_file->write_proc = dmx_dataflow_control_procfile_write;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
    dmx_proc_dbginfo_file->read_proc = dmx_dbginfo_procfile_read;
    dmx_proc_dbginfo_file->write_proc = dmx_dbginfo_procfile_write;
#endif
    return 0;
}


void  dmx_debug_procfs_exit(void)
{
    remove_proc_entry(DMX_DEBUG_PROC_INFO, dmx_proc_dir);
    remove_proc_entry(DMX_DATA_CONTROL_OPTION, dmx_proc_dir);
}

