#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
//for printk
#include <linux/kernel.h>

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

#include "alitac.h"

extern unsigned long __G_ALI_MM_TAC_MEM_SIZE ;
extern unsigned long __G_ALI_MM_TAC_MEM_START_ADDR ;

#define ALI_TAC_CLIENT_MAX 5
#define SIZE_1K               1024
#define SIZE_16K              (SIZE_1K * 16)
#define SIZE_64K              (SIZE_1K * 64)
#define ALI_TAC_MMAP_SIZE_PER_CLIENT SIZE_64K

enum tac_dev_type{
	DEV_M2S=0,
	DEV_S2M
};

typedef struct tac_dev{
	struct cdev dev;
	void * shm_addr[ALI_TAC_CLIENT_MAX];//kernel vir_addr
	int shm_size;
	enum tac_dev_type dev_type; // 0: M2S, 1: S2M
	/*for S2M*/
	int dev_S2M_status; // if userspace ready and call 
	struct kumsgq * ali_tac_kumsgq;
	int kumsgq_fd;
	struct completion S2M_finish[ALI_TAC_CLIENT_MAX];
	struct tac_rpc_ret rpc_ret;
}S_tac_dev, *P_tac_dev;

static P_tac_dev p_tac_dev[2];
static struct class *tac_class;    /* Tie with the device model */
static dev_t tac_dev_number; 




int g_tac_dbg_flag = 1;
#if 0
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

#define TAC_DEV_NUM 2
#define DEVICE_NAME "ali_tac"
#define TAC_DEV_M2S_NAME "ali_tac_m2s"
#define TAC_DEV_S2M_NAME "ali_tac_s2m"
#define TAC_DEV_M2S_INDEX 0
#define TAC_DEV_S2M_INDEX 1


#define TAC_RPC_M2S_DATA_RET_SIZE_MAX (1024*32)

//debug
//#define ALI_TAC_KUMSGQ_DEBUG



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
	return ret;
	
TAC_OPEN_FAILED:
	return -1;

	
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
	int shm_size = 1024*64; 
	int data_ret = TAC_RPC_M2S_DATA_RET_SIZE_MAX;
	
	TAC_PRF("tac_ioctl: cmd = %d, arg = 0x%08x\n",cmd,arg);
	tac_devp = (P_tac_dev) file->private_data;
	
	switch (cmd){
		case IO_TAC_GET_SHM_SIZE:
			if (copy_to_user((unsigned int *)arg, &tac_devp->shm_size, sizeof(int))){
				TAC_PRF("Err: copy_to_user\n");
				ret = -1;
			}
			else {
				TAC_PRF("SHM_SIZE %d\n",tac_devp->shm_size);
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
								rpc_call_para.dataSize_call,tac_devp->shm_addr[rpc_call_para.clientId],&data_ret);
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
			
		case IO_TAC_GET_KUMSGQ:
			if (DEV_M2S == tac_devp->dev_type){
				ret = -1; //DEV_M2S will not be here. or error
			}
			else {
				if(copy_from_user(&flags, (int *)arg, sizeof(int)))
				{
					ret = -1;
					TAC_PRF("Err: copy_from_user\n");
					break;
				}
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
			
		default:
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
	.unlocked_ioctl = tac_ioctl,
};

static int __init tac_module_init(void)
{
	TAC_PRF("module init.\n");
	int ret = 0;
	int loop = 0;
	struct device *tac_device = NULL;
	p_tac_dev[TAC_DEV_M2S_INDEX] =  kmalloc(sizeof(struct tac_dev), GFP_KERNEL);
	p_tac_dev[TAC_DEV_S2M_INDEX] =  kmalloc(sizeof(struct tac_dev), GFP_KERNEL);
	if (NULL == p_tac_dev[TAC_DEV_M2S_INDEX] || NULL == p_tac_dev[TAC_DEV_S2M_INDEX]){
		TAC_PRF("Err: kmalloc failed.\n");
		ret = -1;
		goto TAC_MODULE_INIT_FAILED_1;
	}
	else {
		TAC_PRF("tac_module_init: p_tac_dev = %p \n", p_tac_dev[TAC_DEV_M2S_INDEX]);
		TAC_PRF("tac_module_init: p_tac_dev = %p \n", p_tac_dev[TAC_DEV_S2M_INDEX]);
	}
	tac_class = class_create(THIS_MODULE, DEVICE_NAME);
	ret = alloc_chrdev_region(&tac_dev_number,0,TAC_DEV_NUM,DEVICE_NAME);
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
	for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
		p_tac_dev[TAC_DEV_M2S_INDEX]->shm_addr[loop] = (char *)(__G_ALI_MM_TAC_MEM_START_ADDR \
								+ loop * ALI_TAC_MMAP_SIZE_PER_CLIENT);
	}
	p_tac_dev[TAC_DEV_M2S_INDEX]->shm_size = __G_ALI_MM_TAC_MEM_SIZE >> 1;
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
	for (loop=0; loop<ALI_TAC_CLIENT_MAX; loop++){
		p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[loop] = (char *)(__G_ALI_MM_TAC_MEM_START_ADDR \
									+ (__G_ALI_MM_TAC_MEM_SIZE >> 1) \
									+ loop * ALI_TAC_MMAP_SIZE_PER_CLIENT);
		init_completion (&p_tac_dev[TAC_DEV_S2M_INDEX]->S2M_finish[loop]);
	}
	p_tac_dev[TAC_DEV_S2M_INDEX]->shm_size = __G_ALI_MM_TAC_MEM_SIZE >> 1;
	p_tac_dev[TAC_DEV_S2M_INDEX]->dev_type = DEV_S2M;
	p_tac_dev[TAC_DEV_S2M_INDEX]->dev_S2M_status = 0;
	
	
	//for test only
	TAC_TRACE;
	TAC_PRF("Info: M2S:%p. S2M:%p.\n",p_tac_dev[TAC_DEV_M2S_INDEX]->shm_addr[0], p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[0]);
	strcpy(p_tac_dev[TAC_DEV_M2S_INDEX]->shm_addr[0], "hello,mmap:M2S\n");
	TAC_TRACE;
	strcpy(p_tac_dev[TAC_DEV_S2M_INDEX]->shm_addr[0], "hello,mmap:S2M\n");

	TAC_TRACE;
	
	return 0;
	
TAC_MODULE_INIT_FAILED:
TAC_MODULE_INIT_FAILED_1:
TAC_MODULE_INIT_FAILED_2:
TAC_MODULE_INIT_FAILED_3:
TAC_MODULE_INIT_FAILED_4:
TAC_MODULE_INIT_FAILED_5:
	return ret;	
	
}

static void __exit tac_module_exit(void)
{
	TAC_PRF("module exit.\n");
	unregister_chrdev_region(tac_dev_number,2);
	if (NULL != p_tac_dev[TAC_DEV_M2S_INDEX]){
		cdev_del(&p_tac_dev[TAC_DEV_M2S_INDEX]->dev);
		kfree(p_tac_dev[TAC_DEV_M2S_INDEX]);
	}
	if (NULL != p_tac_dev[TAC_DEV_S2M_INDEX]){
		cdev_del(&p_tac_dev[TAC_DEV_S2M_INDEX]->dev);
		kfree(p_tac_dev[TAC_DEV_S2M_INDEX]);
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
EXPORT_SYMBOL(tac_rpc_S2M);


module_init(tac_module_init);
module_exit(tac_module_exit);

MODULE_DESCRIPTION("Ali tac driver.");
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_LICENSE("GPL");
