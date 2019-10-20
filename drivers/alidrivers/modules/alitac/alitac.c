#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
//for printk
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>

//for cdev
#include <linux/cdev.h>
//for file_operation
#include <linux/fs.h>
//for copy_from_user
#include <asm/uaccess.h>
//for kmalloc
#include <linux/slab.h> 
//for alloc_page
#include <linux/gfp.h>
//for vm_operations_struct
#include <linux/mm.h>
//for class_op
#include <linux/device.h>
//for kumsgq
#include <linux/ali_kumsgq.h>
// for kthread etc
#include <linux/sched.h>
#include <linux/kthread.h>
//for msleep
#include <linux/delay.h>
//for rpc call
#include <rpc_hld/ali_rpc_tac.h>
//for clean cache
#include <ali_cache.h>

#include "alitac.h"

//#define OTA_MAP_BUF

#ifdef OTA_MAP_BUF
extern unsigned long __G_ALI_MM_OTA_MEM_START_ADDR;
extern unsigned long __G_ALI_MM_OTA_MEM_SIZE;
#endif
extern unsigned long __G_ALI_MM_TAC_MEM_SIZE ;
extern unsigned long __G_ALI_MM_TAC_MEM_START_ADDR ;

#define ALI_TAC_CLIENT_MAX 5
#define SIZE_1K               1024
#define SIZE_16K              (SIZE_1K * 16)
#define SIZE_64K              (SIZE_1K * 64)
#define SIZE_MK               (SIZE_1K * SIZE_1K)
#define ALI_TAC_MMAP_SIZE_PER_CLIENT SIZE_64K
#define VMXTAC_DEV_NAME "vmxtac"

#ifdef ALI_TAC_CLIENT_MAX
#define ALI_TAC_MMAP_SIZE_CLIENT_0 SIZE_MK
#define ALI_TAC_MMAP_SIZE_CLIENT_1 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_2 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_3 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_4 SIZE_64K
#else
#define ALI_TAC_MMAP_SIZE_CLIENT_0 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_1 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_2 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_3 SIZE_64K
#define ALI_TAC_MMAP_SIZE_CLIENT_4 SIZE_64K
#endif

#ifdef OTA_MAP_BUF
#define TAC_DEV_NUM 3
#define TAC_DEV_OTA_BUF_NAME "ali_tac_ota_buf"
#define TAC_DEV_OTA_BUF_INDEX 2
#else
#define TAC_DEV_NUM 2
#endif
#define DEVICE_NAME "ali_tac"
#define TAC_DEV_M2S_NAME "ali_tac_m2s"
#define TAC_DEV_S2M_NAME "ali_tac_s2m"
#define TAC_DEV_M2S_INDEX 0
#define TAC_DEV_S2M_INDEX 1


#define TAC_RPC_M2S_DATA_RET_SIZE_MAX (1024*32)
#define ALI_TAC_VERTUON_01 1
#define ALI_TAC_VERTUON_02 2

static struct proc_dir_entry *proc_tac;

unsigned int g_print_status = 1;
unsigned int rpc_init_data[ALI_TAC_CLIENT_MAX*4+3];
enum tac_dev_type{
	DEV_M2S=0,
	DEV_S2M,
#ifdef OTA_MAP_BUF
       DEV_OTA_BUF,
#endif
};

typedef struct tac_dev{
	struct cdev dev;
	void * shm_addr[ALI_TAC_CLIENT_MAX];//kernel vir_addr
	int shm_size[ALI_TAC_CLIENT_MAX];
	void * tmp_addr[ALI_TAC_CLIENT_MAX];//kernel vir_addr
	int tmp_size[ALI_TAC_CLIENT_MAX];
	unsigned long total_shm_size;
	enum tac_dev_type dev_type; // 0: M2S, 1: S2M
	/*for S2M*/
	int dev_S2M_status; // if userspace ready and call 
	struct kumsgq * ali_tac_kumsgq;
	int kumsgq_fd;
	struct completion S2M_finish[ALI_TAC_CLIENT_MAX];
	struct tac_rpc_ret rpc_ret;
}S_tac_dev, *P_tac_dev;

typedef struct mem_info {
	unsigned int addr;
	unsigned int len;
}mem_info_t;

typedef struct SHM_info {
	mem_info_t data_buf;
	mem_info_t tmp_buf;
}SHM_info_t;

typedef struct MMP_info {
	unsigned int g_print_status_flag;
	SHM_info_t M2S_shm_info[ALI_TAC_CLIENT_MAX];
	SHM_info_t S2M_shm_info[ALI_TAC_CLIENT_MAX];
}MMP_info_t;

MMP_info_t mmp_info;

#ifdef OTA_MAP_BUF
static P_tac_dev p_tac_dev[TAC_DEV_NUM];
#else
static P_tac_dev p_tac_dev[2];
#endif
static struct class *tac_class;    /* Tie with the device model */
static dev_t tac_dev_number; 

int TAC_VERTION_FLAG = ALI_TAC_VERTUON_02;

int g_tac_dbg_flag = 1;
#if 1
#define TAC_PRF(arg, value...)  
#else
#define TAC_PRF(fmt, args...) \
	    do \
	    { \
		    if(g_tac_dbg_flag) \
		    { \
                printk("[ali_tac] "); \
			    printk(fmt, ##args); \
		    } \
	    }while(0)
#endif

#define TAC_ENTRY TAC_PRF("into: %s.\n",__func__)
#define TAC_EXIT  TAC_PRF("exit: %s.\n",__func__)
#define TAC_TRACE TAC_PRF("at %s[%d].\n",__func__,__LINE__)


//for mips
#define TAC_VIRT_TO_SEE_ADDR_NOCACHE(x)  ((x)|0xA0000000)

//debug
//#define ALI_TAC_KUMSGQ_DEBUG
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
#include <linux/seq_file.h>
unsigned char ptr[2];
static int tac_proc_read(struct seq_file *m, void *v)
{
	seq_printf(m, "\n");
	seq_printf(m, "address 0x%x status=%d\n", (unsigned int)&mmp_info.g_print_status_flag,mmp_info.g_print_status_flag);
	return 0;
}

static int tac_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, tac_proc_read,NULL);
}

static ssize_t tac_proc_write(struct file *file, const char *buffer, size_t len,
  loff_t *off)
{
  int user_len = 0;
  user_len = len;
  if(user_len == 2)
  {
		if(copy_from_user(ptr, buffer, user_len))
		{
			printk( "copy data error\n");
			return -EFAULT;
		}
		if(ptr[0] == 0x31)
		{
			mmp_info.g_print_status_flag= 1;
		}
		else if(ptr[0] == 0x30)
		{
			mmp_info.g_print_status_flag= 0;
		}
		__CACHE_FLUSH_ALI(&mmp_info.g_print_status_flag,sizeof(mmp_info.g_print_status_flag));	  
		return user_len;
  }
  else
  {
		printk( "write len error\n");
		return -EFAULT;
  }
}

static const struct file_operations proc_panel_fops = {
	.open		= tac_proc_open,
	.read		= seq_read,
	.write 		= tac_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#else
static int tac_proc_read(char *buffer, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	printk( "not implement in low kernel version.\n");
	return -EFAULT;
}
#endif



//for vma
void tac_vma_open(struct vm_area_struct *vma)
{
	TAC_ENTRY;
	TAC_EXIT;
}

void tac_vma_close(struct vm_area_struct *vma)
{
	TAC_ENTRY;
	TAC_EXIT;
}


static struct vm_operations_struct tac_vma_ops = {
	.open = tac_vma_open,
	.close = tac_vma_close, 
};

static int tac_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	P_tac_dev tac_devp = NULL;
	TAC_PRF("into : tac_open.\n");
    /* Get the per-device structure that contains this cdev */
    tac_devp = container_of(inode->i_cdev, struct tac_dev, dev);
	TAC_PRF("tac_open: file->private_data = %p \n", tac_devp);
    /* Easy access to tac_devp from rest of the entry points */
    file->private_data = tac_devp;

#ifdef OTA_MAP_BUF
    if (DEV_S2M == tac_devp->dev_type)
    {
        tac_devp->ali_tac_kumsgq = ali_new_kumsgq();
	 if (NULL == tac_devp->ali_tac_kumsgq)
	 {
		goto TAC_OPEN_FAILED;
	 }
    }
#else
	if (DEV_M2S == tac_devp->dev_type){
		; // DEV_M2S will not new kumsgq
	}
	else {
		tac_devp->ali_tac_kumsgq = ali_new_kumsgq();
		if (NULL == tac_devp->ali_tac_kumsgq)
		{
			goto TAC_OPEN_FAILED;
	    }
	}
#endif
	return ret;
	
TAC_OPEN_FAILED:
	return -1;

	
}

static int tac_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	P_tac_dev tac_devp = NULL;
	TAC_PRF("into : tac_release.\n");
	tac_devp = (P_tac_dev)file->private_data;
	//release
	ali_destroy_kumsgq(tac_devp->ali_tac_kumsgq);
	tac_devp->ali_tac_kumsgq = NULL;
	tac_devp->kumsgq_fd  = -1;
	tac_devp->dev_S2M_status = 0;
	return ret;
}


static ssize_t tac_write(struct file *file, const char __user * buf, size_t size, loff_t *off)
{
	int ret = 0;
	/*should return how many bytes kernel handle*/
	return ret;
}

static int tac_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = 0;
	P_tac_dev tac_devp = NULL;
	TAC_ENTRY;
	tac_devp = (P_tac_dev) file->private_data;
	TAC_PRF("@@@DTS MMAP noncached: V=%p, P=%p.\n",tac_devp->shm_addr[0],virt_to_phys((void *)tac_devp->shm_addr[0]));
	if (remap_pfn_range(vma,vma->vm_start,\
						virt_to_phys((void *)tac_devp->shm_addr[0]) >> PAGE_SHIFT,\
						vma->vm_end-vma->vm_start,pgprot_noncached(vma->vm_page_prot)))
		return -EAGAIN;
	vma->vm_ops = & tac_vma_ops;
	tac_vma_open(vma);
	
	return ret;
}

#ifdef ALI_TAC_KUMSGQ_DEBUG
static int kumsgq_test_thread(void *arg)
{
	struct kumsgq *ali_tac_kumsgq = (struct kumsgq *)arg;
	//struct kumsgq *ali_tac_kumsgq = p_tac_dev->ali_tac_kumsgq;
	struct tac_see_rpc_call see_rcp_call = {1,1};
	int ret = 0;
	TAC_ENTRY;
	while (1){
		msleep(5000);
		TAC_PRF("@@@kumsgq_test_thread round : %d.\n",see_rcp_call.clientId);
		ret = ali_kumsgq_sendmsg(ali_tac_kumsgq,&see_rcp_call,sizeof(struct tac_see_rpc_call));
		if (0 != ret ){
			TAC_PRF("kumsgq_test_thread failed .\n");
		}
		see_rcp_call.clientId ++;
		see_rcp_call.dataSize_call ++;
	}
	return 0;
}

#endif 


static long tac_ioctl(struct file * file , unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int flags = -1;
	P_tac_dev tac_devp = NULL;
	struct tac_rpc_call rpc_call_para;
	unsigned int* shm_size;
	int data_ret = TAC_RPC_M2S_DATA_RET_SIZE_MAX;
	int loop = 0;
	unsigned long shm_ptr;
	int mem_offset = 0;
	unsigned int client_id;
	int  mm_size;
       
	TAC_PRF("tac_ioctl: cmd = %d, arg = 0x%08x\n",cmd,arg);
	tac_devp = (P_tac_dev) file->private_data;
	TAC_PRF("===>  %d %d\n",IO_TAC_RPC_RET,IO_TAC_RPC_INIT);
	switch (cmd){
		case IO_TAC_GET_SHM_SIZE:
			TAC_VERTION_FLAG = ALI_TAC_VERTUON_01;
			shm_size = (unsigned int *)arg;
			mm_size = tac_devp->total_shm_size;
			if (copy_to_user((unsigned int *)arg, &mm_size, sizeof(int))){
				TAC_PRF("Err: copy_to_user\n");
				ret = -1;
			}
			else {
				TAC_PRF("SHM_SIZE %d\n",tac_devp->shm_size);
				if(__G_ALI_MM_TAC_MEM_SIZE == SIZE_MK) {
					shm_ptr =  __G_ALI_MM_TAC_MEM_START_ADDR;
					mem_offset = 0;
					for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
						p_tac_dev[TAC_DEV_M2S_INDEX]->shm_addr[loop] = (char *)(shm_ptr + mem_offset);
						p_tac_dev[TAC_DEV_M2S_INDEX]->shm_size[loop] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
						mem_offset += ALI_TAC_MMAP_SIZE_PER_CLIENT;
						p_tac_dev[TAC_DEV_M2S_INDEX]->tmp_addr[loop] = (char *)(shm_ptr + mem_offset);
						p_tac_dev[TAC_DEV_M2S_INDEX]->tmp_size[loop] = 0;
					}
					p_tac_dev[TAC_DEV_M2S_INDEX]->total_shm_size = mem_offset;
					shm_ptr =  __G_ALI_MM_TAC_MEM_START_ADDR + (__G_ALI_MM_TAC_MEM_SIZE >> 1);
					mem_offset = 0;
					for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
						p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[loop] = (char *)(shm_ptr + mem_offset);
						p_tac_dev[TAC_DEV_S2M_INDEX]->shm_size[loop] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
						mem_offset += ALI_TAC_MMAP_SIZE_PER_CLIENT;
						p_tac_dev[TAC_DEV_S2M_INDEX]->tmp_addr[loop] = (char *)(shm_ptr + mem_offset);
						p_tac_dev[TAC_DEV_S2M_INDEX]->tmp_size[loop] = 0;
					}
					p_tac_dev[TAC_DEV_S2M_INDEX]->total_shm_size = mem_offset;
				}
				for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
					tac_devp->shm_addr[loop] = (char *)(tac_devp->shm_addr[0] + loop * ALI_TAC_MMAP_SIZE_PER_CLIENT);
					tac_devp->shm_size[loop] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
				}
			}
			break;

		case IO_TAC_GET_ALL_SHM_SIZE:
			TAC_VERTION_FLAG = ALI_TAC_VERTUON_02;
			shm_size = (unsigned int *)arg;
			mm_size = tac_devp->total_shm_size;
			if (copy_to_user( &shm_size[0], &mm_size, sizeof(int))){
				TAC_PRF("Err: copy_to_user\n");
				ret = -1;
			}
			if (copy_to_user( &shm_size[1], &tac_devp->shm_size[0], sizeof(unsigned int )*ALI_TAC_CLIENT_MAX)){
				TAC_PRF("Err: copy_to_user\n");
				ret = -1;
			} else if (copy_to_user( &shm_size[ALI_TAC_CLIENT_MAX+1], &tac_devp->tmp_size[0], sizeof(int )*ALI_TAC_CLIENT_MAX)){
				TAC_PRF("Err: copy_to_user\n");
				ret = -1;
			}

			break;

		case IO_TAC_RPC_CALL:
			if (DEV_M2S == tac_devp->dev_type){
				copy_from_user(&rpc_call_para,(struct tac_rpc_call *)arg, sizeof(struct tac_rpc_call));
				if (rpc_call_para.clientId < ALI_TAC_CLIENT_MAX){
					TAC_PRF("INFO: rpc_call_para: clientId=%d,dataSize_call=%d,dataSize_return=%d\n",\
						rpc_call_para.clientId, rpc_call_para.dataSize_call,rpc_call_para.dataSize_return);
					TAC_PRF("INFO: memory: vaddr=%p,paddr=%p\n",\
						tac_devp->shm_addr[rpc_call_para.clientId], \
						virt_to_phys(tac_devp->shm_addr[rpc_call_para.clientId]));
					tac_rpc_M2S(rpc_call_para.clientId,rpc_call_para.contextId,\
								TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)(tac_devp->shm_addr[rpc_call_para.clientId])),\
								rpc_call_para.dataSize_call,tac_devp->shm_addr[rpc_call_para.clientId],(UINT32 *)&data_ret);
					TAC_PRF("data_ret = %d, tac_devp->shm_addr[0] = %c, [1] = %c\n",data_ret,\
							*(char *)tac_devp->shm_addr[rpc_call_para.clientId],\
							*((char *)tac_devp->shm_addr[rpc_call_para.clientId] +1));
					rpc_call_para.dataSize_return = data_ret;
					copy_to_user((struct tac_rpc_call *)arg, &rpc_call_para, sizeof(struct tac_rpc_call));
				}
				else {
					TAC_PRF("[%d]ERR: clientId[%d] is bigger than ALI_TAC_CLIENT_MAX[%d]\n",__LINE__,rpc_call_para.clientId,ALI_TAC_CLIENT_MAX);
				}
			}
			else {
				ret = -1; //DEV_S2M will not be here. or error
			}

			break;

		case IO_TAC_RPC_DMDA_CALL:
			if (DEV_M2S == tac_devp->dev_type){
				copy_from_user(&rpc_call_para,(struct tac_rpc_call *)arg, sizeof(struct tac_rpc_call));
				if (rpc_call_para.clientId < ALI_TAC_CLIENT_MAX){
					TAC_PRF("INFO: rpc_call_para: clientId=%d,dataSize_call=%d,dataSize_return=%d\n",\
						rpc_call_para.clientId, rpc_call_para.dataSize_call,rpc_call_para.dataSize_return);
					TAC_PRF("INFO: memory: vaddr=%p,paddr=%p\n",\
						tac_devp->shm_addr[rpc_call_para.clientId], \
						virt_to_phys(tac_devp->shm_addr[rpc_call_para.clientId]));
					tac_rpc_DMDA_M2S(rpc_call_para.clientId,rpc_call_para.contextId,\
								TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)(tac_devp->shm_addr[rpc_call_para.clientId])),\
								rpc_call_para.dataSize_call,tac_devp->shm_addr[rpc_call_para.clientId],(UINT32 *)&data_ret);
					TAC_PRF("data_ret = %d, tac_devp->shm_addr[0] = %c, [1] = %c\n",data_ret,\
							*(char *)tac_devp->shm_addr[rpc_call_para.clientId],\
							*((char *)tac_devp->shm_addr[rpc_call_para.clientId] +1));
					rpc_call_para.dataSize_return = data_ret;
					copy_to_user((struct tac_rpc_call *)arg, &rpc_call_para, sizeof(struct tac_rpc_call));
				}
				else {
					TAC_PRF("[%d]ERR: clientId[%d] is bigger than ALI_TAC_CLIENT_MAX[%d]\n",__LINE__,rpc_call_para.clientId,ALI_TAC_CLIENT_MAX);
				}
			}
			else {
				ret = -1; //DEV_S2M will not be here. or error
			}

			break;

		case IO_TAC_RPC_RET:
			if (DEV_M2S == tac_devp->dev_type){
				ret = -1; //DEV_M2S will not be here. or error
			}
			else {
				if (copy_from_user(&tac_devp->rpc_ret, (struct tac_rpc_ret *)arg, sizeof(struct tac_rpc_ret))){
					TAC_PRF("Err: copy_from_user\n");
					ret = -1;
				}
				else {
					TAC_PRF("dataSize_ret %d\n",tac_devp->rpc_ret.dataSize_ret);
				}
				//waitup tac_rpc_S2M
				if (tac_devp->rpc_ret.clientId < ALI_TAC_CLIENT_MAX){
					complete(&tac_devp->S2M_finish[tac_devp->rpc_ret.clientId]);
				}
				else {
					TAC_PRF("[%d]ERR: clientId[%d] is bigger than ALI_TAC_CLIENT_MAX[%d]\n",__LINE__,tac_devp->rpc_ret.clientId,ALI_TAC_CLIENT_MAX);
				}
			}
			
			break;
			
		case IO_TAC_RPC_INIT:
			TAC_PRF("tac_rpc_Init ALI_TAC_VERTUON_01\n");
			tac_rpc_Init((unsigned int)&mmp_info);
			
			break;

		case IO_TAC_RPC_DMDA_INIT:
			TAC_PRF("tac_rpc_DMDA_Init ALI_TAC_VERTUON_02\n");
			tac_rpc_DMDA_Init((unsigned int)&mmp_info);
			
			break;

             case IO_TAC_START_TEE_CLIENT:
                     if (copy_from_user(&client_id, (unsigned int *)arg, sizeof(unsigned int)))
                     {
			    TAC_PRF("Err: copy_from_user\n");
			    ret = -1;
			}
			else 
                    {
			   TAC_PRF("start tee client id(%d)\n",client_id);
                        tac_rpc_start_tee_client(client_id);
			}
                     break;
                     
		case IO_TAC_GET_KUMSGQ:
			if (DEV_M2S == tac_devp->dev_type){
				ret = -1; //DEV_M2S will not be here. or error
			}
			else {
				copy_from_user(&flags, (int *)arg, sizeof(int));
				TAC_PRF("ali_kumsgq_newfd arg=0x%x, flags=%d\n",arg,flags);
				ret  = ali_kumsgq_newfd(tac_devp->ali_tac_kumsgq, flags);
				if(ret >= 0)
				{
					tac_devp->kumsgq_fd = ret;
					tac_devp->dev_S2M_status = 1; // 
				}
			}
			TAC_PRF("ali_kumsgq_newfd %d\n",ret);
			break;
             case IO_TAC_GET_OTA_MEM_INFO:
             {
                    struct tac_ota_mem_info ota_mem_info;
                    ota_mem_info.size=(unsigned long)tac_devp->shm_size;
                    ota_mem_info.addr=(unsigned long)tac_devp->shm_addr[0];
                    if (copy_to_user((struct tac_ota_mem_info *)arg, &ota_mem_info, sizeof(struct tac_ota_mem_info)))
                    {
				TAC_PRF("Err: copy_to_user\n");
				ret = -1;
			}
			else {
				TAC_PRF("OTA_SIZE %d\n",tac_devp->shm_size);
			}
                
             }
                     break;
			
		default:
                    ret=-1;
                    TAC_PRF("Err: wrong command\n");
			break;
	}

	TAC_EXIT;
	return ret;
}



struct file_operations tac_file_ops = {
	.owner = THIS_MODULE,
	.open  = tac_open,
	.write = tac_write,
	.mmap  = tac_mmap,
	.release  = tac_release,     /* Release method */
	.unlocked_ioctl = tac_ioctl,
};

static int __init tac_module_init(void)
{
	int ret = 0;
	int loop = 0;
	int mem_offset = 0;
	unsigned long shm_ptr;
	int client_buf_size[5];
	struct device *tac_device = NULL;

	TAC_PRF("module init.\n");
	p_tac_dev[TAC_DEV_M2S_INDEX] =  kmalloc(sizeof(struct tac_dev), GFP_KERNEL);
	p_tac_dev[TAC_DEV_S2M_INDEX] =  kmalloc(sizeof(struct tac_dev), GFP_KERNEL);
#ifdef OTA_MAP_BUF
       p_tac_dev[TAC_DEV_OTA_BUF_INDEX] =  kmalloc(sizeof(struct tac_dev), GFP_KERNEL);
       if (NULL == p_tac_dev[TAC_DEV_M2S_INDEX] || NULL == p_tac_dev[TAC_DEV_S2M_INDEX]
        || NULL == p_tac_dev[TAC_DEV_OTA_BUF_INDEX])
#else
	if (NULL == p_tac_dev[TAC_DEV_M2S_INDEX] || NULL == p_tac_dev[TAC_DEV_S2M_INDEX])
#endif
       {
		TAC_PRF("Err: kmalloc failed.\n");
		ret = -1;
		goto TAC_MODULE_INIT_FAILED_1;
	}
	else {
		TAC_PRF("tac_module_init: p_tac_dev = %p \n", p_tac_dev[TAC_DEV_M2S_INDEX]);
		TAC_PRF("tac_module_init: p_tac_dev = %p \n", p_tac_dev[TAC_DEV_S2M_INDEX]);
        #ifdef OTA_MAP_BUF
              TAC_PRF("tac_module_init: p_tac_dev = %p \n", p_tac_dev[TAC_DEV_OTA_BUF_INDEX]);
        #endif
	}
	tac_class = class_create(THIS_MODULE, DEVICE_NAME);
	ret = alloc_chrdev_region(&tac_dev_number,0,TAC_DEV_NUM,DEVICE_NAME);

	if(__G_ALI_MM_TAC_MEM_SIZE >= SIZE_MK*4) {
		client_buf_size[0] = ALI_TAC_MMAP_SIZE_CLIENT_0;
		client_buf_size[1] = ALI_TAC_MMAP_SIZE_CLIENT_1;
		client_buf_size[2] = ALI_TAC_MMAP_SIZE_CLIENT_2;
		client_buf_size[3] = ALI_TAC_MMAP_SIZE_CLIENT_3;
		client_buf_size[4] = ALI_TAC_MMAP_SIZE_CLIENT_4;
	}
	else if(__G_ALI_MM_TAC_MEM_SIZE >= SIZE_64K*15){
		client_buf_size[0] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[1] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[2] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[3] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[4] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
	}
	else {
		TAC_PRF("Err: memory size is too small MEM_SIZE = 0x%x\n", __G_ALI_MM_TAC_MEM_SIZE);
		client_buf_size[0] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[1] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[2] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[3] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
		client_buf_size[4] = ALI_TAC_MMAP_SIZE_PER_CLIENT;
	}
	
	// for M2S
	TAC_TRACE;
	cdev_init(&p_tac_dev[TAC_DEV_M2S_INDEX]->dev,&tac_file_ops);
	ret = cdev_add(&p_tac_dev[TAC_DEV_M2S_INDEX]->dev,tac_dev_number,1);
	if (0 != ret){
		TAC_PRF("Err: cdev_add.\n");
		goto TAC_MODULE_INIT_FAILED_3;
	}
	tac_device = device_create(tac_class, NULL, p_tac_dev[TAC_DEV_M2S_INDEX]->dev.dev,NULL,TAC_DEV_M2S_NAME);
	if (NULL == tac_device){
		TAC_PRF("Err: device_create.\n");
		goto TAC_MODULE_INIT_FAILED_4;
	}

	shm_ptr =  __G_ALI_MM_TAC_MEM_START_ADDR;
	mem_offset = 0;
	for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
		p_tac_dev[TAC_DEV_M2S_INDEX]->shm_addr[loop] = (char *)(shm_ptr + mem_offset);
		p_tac_dev[TAC_DEV_M2S_INDEX]->shm_size[loop] = client_buf_size[loop];
		mem_offset += client_buf_size[loop];
		p_tac_dev[TAC_DEV_M2S_INDEX]->tmp_addr[loop] = (char *)(shm_ptr + mem_offset);
		p_tac_dev[TAC_DEV_M2S_INDEX]->tmp_size[loop] = client_buf_size[loop];
		mem_offset += client_buf_size[loop];
	}
	p_tac_dev[TAC_DEV_M2S_INDEX]->total_shm_size = mem_offset;
	p_tac_dev[TAC_DEV_M2S_INDEX]->dev_type = DEV_M2S;

	// for S2M
	TAC_TRACE;
	cdev_init(&p_tac_dev[TAC_DEV_S2M_INDEX]->dev,&tac_file_ops);
	ret = cdev_add(&p_tac_dev[TAC_DEV_S2M_INDEX]->dev,  tac_dev_number+1, 1);
	if (0 != ret){
		TAC_PRF("Err: cdev_add.\n");
		goto TAC_MODULE_INIT_FAILED_3;
	}
	tac_device = device_create(tac_class, NULL, p_tac_dev[TAC_DEV_S2M_INDEX]->dev.dev,NULL,TAC_DEV_S2M_NAME);
	if (NULL == tac_device){
		TAC_PRF("Err: device_create.\n");
		goto TAC_MODULE_INIT_FAILED_4;
	}

	shm_ptr = __G_ALI_MM_TAC_MEM_START_ADDR + p_tac_dev[TAC_DEV_M2S_INDEX]->total_shm_size;
	mem_offset = 0;
	for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
		p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[loop] = (char *)(shm_ptr + mem_offset);
		p_tac_dev[TAC_DEV_S2M_INDEX]->shm_size[loop] = client_buf_size[loop];
		mem_offset += client_buf_size[loop];
		p_tac_dev[TAC_DEV_S2M_INDEX]->tmp_addr[loop] = (char *)(shm_ptr + mem_offset);
		p_tac_dev[TAC_DEV_S2M_INDEX]->tmp_size[loop] = 0;
		init_completion (&p_tac_dev[TAC_DEV_S2M_INDEX]->S2M_finish[loop]);
	}
	p_tac_dev[TAC_DEV_S2M_INDEX]->total_shm_size = mem_offset;
	p_tac_dev[TAC_DEV_S2M_INDEX]->dev_type = DEV_S2M;
	p_tac_dev[TAC_DEV_S2M_INDEX]->dev_S2M_status = 0;
	
#ifdef OTA_MAP_BUF //for OTA BUF
       TAC_TRACE;
	cdev_init(&p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->dev,&tac_file_ops);
	ret = cdev_add(&p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->dev,  tac_dev_number+2, 1);
	if (0 != ret){
		TAC_PRF("Err: cdev_add.\n");
		goto TAC_MODULE_INIT_FAILED_3;
	}
	tac_device = device_create(tac_class, NULL, p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->dev.dev,NULL,TAC_DEV_OTA_BUF_NAME);
	if (NULL == tac_device){
		TAC_PRF("Err: device_create.\n");
		goto TAC_MODULE_INIT_FAILED_4;
	}
	p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->shm_addr[0] = (char *)(__G_ALI_MM_OTA_MEM_START_ADDR);

	p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->shm_size[0] = __G_ALI_MM_OTA_MEM_SIZE;
	p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->dev_type = DEV_OTA_BUF;
	p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->dev_S2M_status = 0;
       TAC_PRF("shm_addr[%p]\n",p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->shm_addr[0]);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	proc_create("bcprint", 0, NULL, &proc_panel_fops);
#else
	if (NULL != (proc_tac = create_proc_entry( "bcprint", 0, NULL )))
	proc_tac->read_proc = tac_proc_read;
#endif
	for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
		mmp_info.M2S_shm_info[loop].data_buf.addr = \
			TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)((unsigned int)p_tac_dev[TAC_DEV_M2S_INDEX]->shm_addr[loop]));
		mmp_info.M2S_shm_info[loop].data_buf.len = p_tac_dev[TAC_DEV_M2S_INDEX]->shm_size[loop];
		mmp_info.M2S_shm_info[loop].tmp_buf.addr = \
			TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)((unsigned int)p_tac_dev[TAC_DEV_M2S_INDEX]->tmp_addr[loop]));
		mmp_info.M2S_shm_info[loop].tmp_buf.len = p_tac_dev[TAC_DEV_M2S_INDEX]->tmp_size[loop];	

		mmp_info.S2M_shm_info[loop].data_buf.addr = \
			TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)((unsigned int)p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[loop]));
		mmp_info.S2M_shm_info[loop].data_buf.len = p_tac_dev[TAC_DEV_S2M_INDEX]->shm_size[loop];
		mmp_info.S2M_shm_info[loop].tmp_buf.addr = \
			TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)((unsigned int)p_tac_dev[TAC_DEV_S2M_INDEX]->tmp_addr[loop]));
		mmp_info.S2M_shm_info[loop].tmp_buf.len = p_tac_dev[TAC_DEV_S2M_INDEX]->tmp_size[loop];	
	}
	mmp_info.g_print_status_flag = g_print_status;
	__CACHE_FLUSH_ALI(&mmp_info,sizeof(mmp_info));	
	
	return 0;
	
//TAC_MODULE_INIT_FAILED:
TAC_MODULE_INIT_FAILED_1:
//TAC_MODULE_INIT_FAILED_2:
TAC_MODULE_INIT_FAILED_3:
TAC_MODULE_INIT_FAILED_4:
//TAC_MODULE_INIT_FAILED_5:
	return ret;	
	
}

static void __exit tac_module_exit(void)
{
	TAC_PRF("module exit.\n");
#ifdef OTA_MAP_BUF
       unregister_chrdev_region(tac_dev_number,TAC_DEV_NUM);
#else
	unregister_chrdev_region(tac_dev_number,2);
#endif
	if (NULL != p_tac_dev[TAC_DEV_M2S_INDEX]){
		cdev_del(&p_tac_dev[TAC_DEV_M2S_INDEX]->dev);
		kfree(p_tac_dev[TAC_DEV_M2S_INDEX]);
	}
	if (NULL != p_tac_dev[TAC_DEV_S2M_INDEX]){
		cdev_del(&p_tac_dev[TAC_DEV_S2M_INDEX]->dev);
		kfree(p_tac_dev[TAC_DEV_S2M_INDEX]);
	}
#ifdef OTA_MAP_BUF
       if (NULL != p_tac_dev[TAC_DEV_OTA_BUF_INDEX]){
		cdev_del(&p_tac_dev[TAC_DEV_OTA_BUF_INDEX]->dev);
		kfree(p_tac_dev[TAC_DEV_OTA_BUF_INDEX]);
	}    
#endif
	if (proc_tac)
	{
		remove_proc_entry("bcprint", NULL);
	}

	class_destroy(tac_class);
}


//S-->M
//eric.cai add
INT32 tac_rpc_S2M(UINT32 clientId, UINT32 contextId,void *data_call,
						UINT32 data_len_call, UINT32 *data_addr_ret, UINT32 *data_len_ret)
{
	INT32 ret = 0 ;
	struct tac_see_rpc_call see_rpc_call = {1,1,1};
	if (clientId >= ALI_TAC_CLIENT_MAX){
		TAC_PRF("[%d]ERR: clientId[%d] is bigger than ALI_TAC_CLIENT_MAX[%d]\n",__LINE__,clientId,ALI_TAC_CLIENT_MAX);
		ret = -1;// ERR
	}
	see_rpc_call.clientId = clientId;
	see_rpc_call.contextId = contextId;
	see_rpc_call.dataSize_call = data_len_call;
	if (NULL != p_tac_dev[TAC_DEV_S2M_INDEX]  && 1 == p_tac_dev[TAC_DEV_S2M_INDEX]->dev_S2M_status){
		if(TAC_VERTION_FLAG == ALI_TAC_VERTUON_01)
			memcpy(p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[see_rpc_call.clientId], data_call,data_len_call);
		ret = ali_kumsgq_sendmsg(p_tac_dev[TAC_DEV_S2M_INDEX]->ali_tac_kumsgq,&see_rpc_call,sizeof(struct tac_see_rpc_call));
		if (0 != ret ){
			TAC_PRF("ali_kumsgq_sendmsg failed .\n");
		}
		TAC_PRF("clientId=%d,contextId=%d,data_len_call=%d,fisrt=%c\n",
			clientId,contextId,data_len_call,*(char*)data_call);

		// wait for usersapce to handle this rpc call from see
		TAC_PRF("waiting for completion....\n");
		wait_for_completion_interruptible(&p_tac_dev[TAC_DEV_S2M_INDEX]->S2M_finish[see_rpc_call.clientId]);
		TAC_PRF("completion!!! rpc return to see.\n");
		//return data
		*data_addr_ret = TAC_VIRT_TO_SEE_ADDR_NOCACHE((UINT32)(p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[see_rpc_call.clientId]));
		*data_len_ret = p_tac_dev[TAC_DEV_S2M_INDEX]->rpc_ret.dataSize_ret;
	}
	else {
		TAC_PRF(":ERR tac_rpc_S2M is not ready.");
		ret = -1; // if MAIN is not ready, return immediately by -1
	}
	return ret;
}

INT32 tac_rpc_DMDA_S2M(UINT32 clientId, UINT32 contextId,void *data_call,
						UINT32 data_len_call, UINT32 *data_addr_ret, UINT32 *data_len_ret)
{
	return tac_rpc_S2M(clientId, contextId, data_call, data_len_call, data_addr_ret, data_len_ret);
}

EXPORT_SYMBOL(tac_rpc_S2M);
EXPORT_SYMBOL(tac_rpc_DMDA_S2M);

module_init(tac_module_init);
module_exit(tac_module_exit);

MODULE_DESCRIPTION("Ali tac driver.");
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_LICENSE("GPL");
