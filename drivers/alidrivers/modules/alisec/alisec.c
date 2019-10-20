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
//for complete
#include <linux/completion.h>
//for fdget...
#include <linux/file.h>
//for platform
#include <linux/platform_device.h>
//for device tree
#include <linux/of.h>

//for rpc call
#include <rpc_hld/ali_rpc_sec.h>
#include <linux/ali_sec.h>
#include <ali_hdmi_common.h>
#include <hdmi_io_common.h>
#include <ali_soc.h>

#define DEVICE_NAME "ali_sec"
#define ALI_BC_DVB_DEV_PREFIX "ali_bc_dvb"
#define ALI_BC_IPTV_DEV_PREFIX "ali_bc_iptv"
#define ALI_BC_DVR_DEV_PREFIX "ali_bc_dvr"
#define ALI_BC_OTT_DEV_PREFIX "ali_bc_ott"
#define ALI_BC_DVB_DEV_NUM 3
#define ALI_BC_IPTV_DEV_NUM 3
#define ALI_BC_DVR_DEV_NUM 3
#define ALI_BC_OTT_DEV_NUM 2
#define ALI_SEC_DEV_INDEX_START     0 
#define ALI_BC_DVB_DEV_INDEX_START  ALI_SEC_DEV_INDEX_START
#define ALI_BC_IPTV_DEV_INDEX_START (ALI_BC_DVB_DEV_INDEX_START  + ALI_BC_DVB_DEV_NUM)
#define ALI_BC_DVR_DEV_INDEX_START  (ALI_BC_IPTV_DEV_INDEX_START + ALI_BC_IPTV_DEV_NUM)
#define ALI_BC_OTT_DEV_INDEX_START  (ALI_BC_DVR_DEV_INDEX_START  + ALI_BC_DVR_DEV_NUM)

#define ALI_SEC_DEV_MAX (ALI_BC_DVB_DEV_NUM + ALI_BC_IPTV_DEV_NUM \
					   + ALI_BC_DVR_DEV_NUM + ALI_BC_OTT_DEV_NUM)


enum SEC_DEV_USERSPACE_STATUS
{
	DEV_INIT=0,
	USERSPACE_READY=1,
};

typedef struct sec_dev{
	struct cdev dev;
	struct semaphore sem;
	char name[20];
	enum SEC_DEV_USERSPACE_STATUS status;
	int open_count;
	struct kumsgq * sec_kumsgq;
	int kumsgq_fd;
	struct completion sec_completion;
      unsigned char service_idx;
	S_DSC_HDL dsc_hdl_info;
}S_sec_dev, *P_sec_dev;

static P_sec_dev p_sec_dev_priv[ALI_SEC_DEV_MAX];
static struct class *sec_class;    /* Tie with the device model */
static dev_t sec_dev_number; 
//DVB has 3 service index: 0x0, 0x1, 0x2
//IPTV has 3 service index: 0x40, 0x41, 0x42
//DVR has 3 service index: 0x80,0x81,0x82
//OTT has 2 service index 0xC0, 0xC1
unsigned char g_ali_sec_service_id[ALI_SEC_DEV_MAX] = {0x0,  0x1,  0x2, \
											  0x40, 0x41, 0x42,\
											  0x80, 0x81, 0x82,\
											  0xC0, 0xC1};

static int find_dev_by_service_id(unsigned char service_id)
{
	int index = -1;
	switch (service_id){
		case 0x0:
			index = 0;
			break;
		case 0x1:
			index = 1;
			break;
		case 0x2:
			index = 2;
			break;
		case 0x40:
			index = 3;
			break;
		case 0x41:
			index = 4;
			break;
		case 0x42:
			index = 5;
			break;
		case 0x80:
			index = 6;
			break;
		case 0x81:
			index = 7;
			break;
		case 0x82:
			index = 8;
			break;
		case 0xC0:
			index = 9;
			break;
		case 0xC1:
			index = 10;
			break;
		default:
			break;
	}
	return index;
}

int g_sec_dbg_flag = 1;
#if 1
#define SEC_PRF(arg, value...)  
#else
#define SEC_PRF(fmt, args...) \
	    do \
	    { \
		    if(g_sec_dbg_flag) \
		    { \
                printk("[ali_sec] "); \
			    printk(fmt, ##args); \
		    } \
	    }while(0)
#endif

#define SEC_ENTRY SEC_PRF("into: %s.\n",__func__)
#define SEC_EXIT  SEC_PRF("exit: %s.\n",__func__)
#define SEC_TRACE SEC_PRF("at %s[%d].\n",__func__,__LINE__)


static int sec_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	P_sec_dev sec_devp = NULL;
	SEC_PRF("into : sec_open.\n");
	sec_devp = container_of(inode->i_cdev, struct sec_dev, dev);
	SEC_PRF("sec_open: file->private_data = %p \n", sec_devp);
	file->private_data = sec_devp;
	if (sec_devp->open_count == 0){
		sec_devp->sec_kumsgq = ali_new_kumsgq();
		if (NULL == sec_devp->sec_kumsgq){
			goto SEC_OPEN_FAILED;
		}
	}
	sec_devp->open_count ++;
	return ret;
	
SEC_OPEN_FAILED:
	return -1;	
}

static int sec_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	P_sec_dev sec_devp = NULL;
	SEC_PRF("into : sec_release.\n");
	sec_devp = (P_sec_dev)file->private_data;
	if (sec_devp->open_count >= 1){
		if (-- sec_devp->open_count){
			SEC_PRF("after sec_release, opencount = %d.\n",sec_devp->open_count);
		}
		else {
			//release
			ali_destroy_kumsgq(sec_devp->sec_kumsgq);
			sec_devp->sec_kumsgq = NULL;
			sec_devp->kumsgq_fd  = -1;
			sec_devp->status = DEV_INIT;
			memset(&sec_devp->dsc_hdl_info,0,sizeof(S_DSC_HDL));
		}
	}
	else {
		ret = -1;
	}	
	return ret;
}



static ssize_t sec_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int ret = 0;
	/*should return how many bytes kernel handle*/
	return ret;
}

static long sec_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned int kumsgq_flags = 0;
	P_sec_dev sec_devp = (P_sec_dev)file->private_data;
	CA_ALGO_PARAM ca_algo_param;
	PID_INFO pid_info;
	BC_DSC_STATUS bc_dsc_status;
	S_DSC_HDL     dsc_hdl_status;
    SEC_TEST_PARAM sec_test_param;
	
	SEC_PRF("sec_ioctl: cmd = 0x%x, arg = 0x%08lx\n",cmd,arg);
	
	switch(cmd){
		case IO_SEC_SET_BC_CA_ALGO:
			if(copy_from_user(&ca_algo_param,(CA_ALGO_PARAM *)arg, sizeof(CA_ALGO_PARAM))){
				SEC_PRF("Err: copy_from_user\n");
				ret = -1;
			}
			else {
                            SEC_PRF("IO_SEC_SET_BC_CA_ALGO: bServiceIdx = 0x%x, ca_device = %d, ca_mode = %d, iv_value[0]=0x%x, iv_value[31]=0x%x\n",\
					(int)ca_algo_param.bServiceIdx,(int)ca_algo_param.ca_device,\
					(int)ca_algo_param.ca_mode,ca_algo_param.iv_value[0],ca_algo_param.iv_value[31]);
				ret = rpc_m2s_bc_set_dvb_ca_descriptor(ca_algo_param.bServiceIdx,\
													   ca_algo_param.ca_device,\
													   ca_algo_param.ca_mode,\
													   ca_algo_param.iv_value);
			}
			break;
			
		case IO_SEC_SET_BC_DVR_PID:
			if(copy_from_user(&pid_info, (PID_INFO *)arg, sizeof(PID_INFO))){
				SEC_PRF("Err: copy_from_user\n");
				ret = -1;
			}
			else {
				SEC_PRF("IO_SEC_SET_BC_DVR_PID: bServiceIdx = 0x%x, pid_number = %d, pid_table[0] = %d, pid_table[31] = %d\n",\
						(int)pid_info.bServiceIdx,(int)pid_info.pid_number,\
						(int)pid_info.pid_table[0],(int)pid_info.pid_table[31]);
				ret = rpc_m2s_bc_set_dvr_pid(pid_info.bServiceIdx, pid_info.pid_number, pid_info.pid_table);
			}
			break;
			
		case IO_SEC_GET_BC_DSC_STATUS:
			if(copy_from_user(&bc_dsc_status, (BC_DSC_STATUS*)arg, sizeof(BC_DSC_STATUS))){
				SEC_PRF("Err: copy_from_user\n");
				ret = -1;
			}
			else {
				SEC_PRF("IO_SEC_GET_BC_DSC_STATUS: bServiceIdx = 0x%x\n",(int)bc_dsc_status.bServiceIdx);
				memset(&dsc_hdl_status, 0, sizeof(S_DSC_HDL));
				ret = rpc_m2s_find_dsc_status_by_serviceIdx(bc_dsc_status.bServiceIdx,&dsc_hdl_status);
				if (0 == ret){ // rpc ok
					//need ?
					if(down_interruptible(&sec_devp->sem))
						return -EINTR;

                                  if(bc_dsc_status.bServiceIdx==sec_devp->service_idx)
                                  {
					memcpy(&sec_devp->dsc_hdl_info, &dsc_hdl_status, sizeof(S_DSC_HDL));
                                  }
                                  else
                                  {
                                     ret = -1;
                                     SEC_PRF("Err: service index is incorrect\n");
                                  }
					//need ?
					up(&sec_devp->sem);
                    SEC_PRF("IO_SEC_GET_BC_DSC_STATUS: ServiceIdx:0x%x, dsc_algo:0x%x, key_type:0x%x, stream_id:0x%lx, sub_dev_hdl:0x%p\n",\
                        bc_dsc_status.bServiceIdx, dsc_hdl_status.dsc_algo, dsc_hdl_status.key_type, dsc_hdl_status.stream_id, dsc_hdl_status.sub_dev_hdl);
					bc_dsc_status.rec_block_count = dsc_hdl_status.rec_block_count;
					if (copy_to_user((BC_DSC_STATUS*)arg, &bc_dsc_status, sizeof(BC_DSC_STATUS))){
						SEC_PRF("Err: copy_to_user\n");
						ret = -1;
					}
				}
                           else
                           {
                                SEC_PRF("Err: IO_SEC_GET_BC_DSC_STATUS\n");
                                ret=-1;
                           }
			}
			break;
			
		case IO_SEC_GET_KUMSGQ:
			if (copy_from_user(&kumsgq_flags, (unsigned int *)arg, sizeof(unsigned int))){
				SEC_PRF("Err: copy_from_user\n");
				ret = -1;
			}
			SEC_PRF("ali_kumsgq_newfd arg=0x%lx, flags=%d\n",arg,kumsgq_flags);
			if (-1 == sec_devp->kumsgq_fd){
				ret  = ali_kumsgq_newfd(sec_devp->sec_kumsgq, kumsgq_flags);
				if(ret >= 0){
					sec_devp->kumsgq_fd = ret;
					sec_devp->status = USERSPACE_READY; // 
				}
			}
			else {
				ret = sec_devp->kumsgq_fd;
			}
			SEC_PRF("ali_kumsgq_newfd %d\n",ret);
			break;

		case IO_SEC_RPC_RET:
			complete(&sec_devp->sec_completion);
			SEC_PRF("complete !!\n");
			break;
			
        case IO_SEC_TEST_DSC:
             if(copy_from_user(&sec_test_param, (SEC_TEST_PARAM *)arg, sizeof(SEC_TEST_PARAM))){
                SEC_PRF("Err: copy_from_user\n");
                ret = -1;
            }
            else {
                SEC_PRF("Info: sec_test_param: bServiceIdx=0x%x, dsc_algo=0x%x, data_type=0x%x, root_key_pos=0x%x, pid_num=0x%x, pid_list[0]=0x%x\n",\
                		(int)sec_test_param.bServiceIdx, (int)sec_test_param.dsc_algo, (int)sec_test_param.data_type, (int)sec_test_param.root_key_pos,\
                		(int)sec_test_param.pid_num, (int)sec_test_param.pid_list[0]);
                ret = rpc_m2s_sec_test(sec_test_param.bServiceIdx, &sec_test_param);
            }
            SEC_PRF("IO_SEC_TEST_DSC ret=%d\n",ret);
            break;
        
		default:
             ret=-1;
			SEC_PRF("Err: %s[%d] Unknown cmd.\n",__func__,__LINE__);
			break;
	}
	return ret ;
}

struct file_operations sec_file_ops = {
	.owner = THIS_MODULE,
	.open  = sec_open,
	.release  = sec_release,     /* Release method */
	.write = sec_write,
	.unlocked_ioctl = sec_ioctl
};

static int sec_dev_create_and_init(const char *dev_name_prefix,int dev_start_index, int dev_num)
{
	int ret = 0;
	int loop = 0;
	int index = 0;
	struct device *sec_device = NULL;
	for(loop=0; loop<dev_num; loop++){
		//create 
		index = loop + dev_start_index;
		cdev_init(&p_sec_dev_priv[index]->dev,&sec_file_ops);
		ret = cdev_add(&p_sec_dev_priv[index]->dev,sec_dev_number + index,1);
		if (0 != ret){
			SEC_PRF("Err: %s cdev_add[%d].\n",dev_name_prefix, loop);
			ret = -1; 
		}
		sec_device = device_create(sec_class, NULL, p_sec_dev_priv[index]->dev.dev,NULL,"%s%d",dev_name_prefix,loop);
		if (NULL == sec_device){
			SEC_PRF("Err: %s device_create[%d].\n",dev_name_prefix, loop);
			ret = -2;
		}
		//init
		sprintf(p_sec_dev_priv[index]->name, "%s%d",dev_name_prefix,loop);
		p_sec_dev_priv[index]->status = DEV_INIT;
		p_sec_dev_priv[index]->open_count = 0;
		p_sec_dev_priv[index]->sec_kumsgq = NULL;
		p_sec_dev_priv[index]->kumsgq_fd  = -1;
		sema_init(&p_sec_dev_priv[index]->sem, 1);
		init_completion(&p_sec_dev_priv[index]->sec_completion);
		memset(&p_sec_dev_priv[index]->dsc_hdl_info,0, sizeof(S_DSC_HDL));
             p_sec_dev_priv[index]->service_idx=g_ali_sec_service_id[index];
	}
	return ret;

}

static int ali_sec_probe(struct platform_device *pdev)
{
	int ret = 0;
	int loop = 0;

	SEC_PRF("module init.\n");
	SEC_PRF("sec_module_init: sizeof short = %d, int = %d, CA_ALGO_PARAM=%d\n",sizeof(short),sizeof(int),sizeof(CA_ALGO_PARAM));
	SEC_PRF("sec_module_init: sizeof sec_dev = %d, completion = %d, S_DSC_HDL=%d\n",\
			sizeof(struct sec_dev),sizeof(struct completion),sizeof(S_DSC_HDL));
	p_sec_dev_priv[0] = kmalloc(ALI_SEC_DEV_MAX * sizeof(struct sec_dev), GFP_KERNEL);
	if (NULL == p_sec_dev_priv[0]){
		SEC_PRF("Err: kmalloc failed.\n");
		ret = -1;
		goto SEC_MODULE_INIT_FAILED_1;
	}
	else {
		for(loop = 0; loop < ALI_SEC_DEV_MAX; loop++){
			// p_sec_dev_priv[loop] = p_sec_dev_priv[0] + sizeof(struct sec_dev)*loop;  // do NOT BE stupid like this!!
			p_sec_dev_priv[loop] = (P_sec_dev)((int)p_sec_dev_priv[0] + sizeof(struct sec_dev)*loop);
			SEC_PRF("sec_module_init: p_sec_dev_priv[%d] = %p \n", loop, p_sec_dev_priv[loop]);
		}
	}

	sec_class = class_create(THIS_MODULE, DEVICE_NAME);

	ret = of_get_major_minor(pdev->dev.of_node,&sec_dev_number, 
			0, ALI_SEC_DEV_MAX, DEVICE_NAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		return ret;
	}

	// for dvb
	SEC_TRACE;
	ret = sec_dev_create_and_init(ALI_BC_DVB_DEV_PREFIX, ALI_BC_DVB_DEV_INDEX_START, ALI_BC_DVB_DEV_NUM);
	if (0 != ret){
		SEC_PRF("Err: DVB sec_create_dev.\n");
		goto SEC_MODULE_INIT_FAILED_2;
	}

	// for iptv
	SEC_TRACE;
	ret = sec_dev_create_and_init(ALI_BC_IPTV_DEV_PREFIX, ALI_BC_IPTV_DEV_INDEX_START, ALI_BC_IPTV_DEV_NUM);
	if (0 != ret){
		SEC_PRF("Err: IPTV sec_create_dev.\n");
		goto SEC_MODULE_INIT_FAILED_2;
	}

	// for dvr
	SEC_TRACE;
	ret = sec_dev_create_and_init(ALI_BC_DVR_DEV_PREFIX, ALI_BC_DVR_DEV_INDEX_START, ALI_BC_DVR_DEV_NUM);
	if (0 != ret){
		SEC_PRF("Err: DVR sec_create_dev.\n");
		goto SEC_MODULE_INIT_FAILED_2;
	}

	// for ott
	SEC_TRACE;
	ret = sec_dev_create_and_init(ALI_BC_OTT_DEV_PREFIX, ALI_BC_OTT_DEV_INDEX_START, ALI_BC_OTT_DEV_NUM);
	if (0 != ret){
		SEC_PRF("Err: OTT sec_create_dev.\n");
		goto SEC_MODULE_INIT_FAILED_2;
	}
	
	return 0;
	
SEC_MODULE_INIT_FAILED_1:
SEC_MODULE_INIT_FAILED_2:
	return ret;	
	
}

static int ali_sec_remove(struct platform_device *pdev)
{
	SEC_PRF("module exit.\n");

	return 0;
}


//S-->M
#define BC_GENERATE_CWC_DEFAULT_TIMEOUT	(30 * HZ) // 30s
#define EMC_VALUE_LENGTH_MAX 255
typedef enum {
	BC_GENERATE_CWC_RET_OK = 0,
	BC_GENERATE_CWC_RET_NONEEXIT = -1,
	BC_GENERATE_CWC_RET_NOREADY  = -2,
	BC_GENERATE_CWC_RET_TIMEOUT  = -3,
} E_BC_GENERATE_CWC_RET;

typedef struct{
	unsigned char bServiceIdx;//[in]
	unsigned char ecm_value[EMC_VALUE_LENGTH_MAX];//[in] ------>unsigned char ecm_value[255]
	unsigned char length;//[in]
}BC_CWC_DATA;
int rpc_s2m_bc_generate_cwc(unsigned char bServiceIdx, unsigned char ecm_value[255], unsigned char length)
{
	int ret_tmp = -1;
	E_BC_GENERATE_CWC_RET ret = BC_GENERATE_CWC_RET_OK ;
	BC_CWC_DATA bc_cwc_data = {0};
	int dev_id = -1;
	
	//SEC_TRACE;
	dev_id = find_dev_by_service_id(bServiceIdx);
	if (-1 == dev_id){
		SEC_PRF("[%d]ERR: bServiceIdx[%d] is NOT exit!\n",__LINE__,bServiceIdx);
		ret = BC_GENERATE_CWC_RET_NONEEXIT;// ERR
		goto OUT;
	}
	if (NULL != p_sec_dev_priv[dev_id]  && USERSPACE_READY == p_sec_dev_priv[dev_id]->status){
		bc_cwc_data.bServiceIdx = bServiceIdx;
		bc_cwc_data.length		= length > (unsigned char)EMC_VALUE_LENGTH_MAX ? EMC_VALUE_LENGTH_MAX : length;
		memcpy(bc_cwc_data.ecm_value, ecm_value, bc_cwc_data.length);
		
		ret_tmp = ali_kumsgq_sendmsg(p_sec_dev_priv[dev_id]->sec_kumsgq, &bc_cwc_data, sizeof(BC_CWC_DATA));
		if (0 != ret_tmp ){
			SEC_PRF("ali_kumsgq_sendmsg failed .\n");
			ret = BC_GENERATE_CWC_RET_NOREADY;// ERR
			goto OUT;
		}
		SEC_PRF("INFO:%s[%d]:bServiceIdx=0x%x,length=%d,ecm_value[0]=0x%x ecm_value[%d]=0x%x",__func__,__LINE__,\
			(int)bServiceIdx,(int)length,ecm_value[0],length>1 ? length-1 : 0, ecm_value[length>1 ? length-1 : 0]);

		// wait for usersapce to handle this rpc call from see
re_wait_timeout:
		SEC_PRF("waiting for completion....\n");
		init_completion(&p_sec_dev_priv[dev_id]->sec_completion);
		ret_tmp = wait_for_completion_interruptible_timeout(&p_sec_dev_priv[dev_id]->sec_completion,\
					BC_GENERATE_CWC_DEFAULT_TIMEOUT);
		if (ret_tmp < 0) { // interrupt
			goto re_wait_timeout;
		}  
		else if (ret_tmp == 0) { //timeout
			SEC_PRF("waiting for completion timeout[%d] jiffies....\n",BC_GENERATE_CWC_DEFAULT_TIMEOUT);
			ret = BC_GENERATE_CWC_RET_TIMEOUT;
			goto OUT;
		}
		SEC_PRF("completion!!! rpc return to see.\n");
	}
	else {
		SEC_PRF(":ERR rpc_s2m_bc_generate_cwc is not ready.\n");
		ret = BC_GENERATE_CWC_RET_NOREADY;// ERR
		goto OUT;
	}
	
OUT:	
	return ret;
}

EXPORT_SYMBOL(rpc_s2m_bc_generate_cwc);

typedef enum {
	BC_GET_HDCP_STATUS = 0,
	BC_GET_HDMI_LINK_STATUS,
	BC_GET_HDMI_AUDIO_ONOFF,
	BC_SET_HDMI_AUDIO_ONOFF,
} E_BC_HDMI_CMD;

int rpc_s2m_bc_hdmi_cmd(unsigned int cmd , int *status)
{
    int ret = 0 ;
    int state=0;

    switch(cmd){
        case BC_GET_HDCP_STATUS:
            *status = (int) ali_hdmi_get_hdcp_onoff();
            SEC_PRF("%s:hdcp_status=%d\n",__func__,(int)*status);
            break;

        case BC_GET_HDMI_LINK_STATUS:
            state=api_get_hdmi_state();
            if(state & 0x20) //0x20 plugin flag
            {
                *status = 1;
            }
            else
            {
                *status = 0;
            }
            SEC_PRF("%s:link_status=%d\n",__func__,(int)*status);
            break;

        case BC_GET_HDMI_AUDIO_ONOFF:
            *status = (int)ali_hdmi_audio_get_onoff();
            SEC_PRF("%s:audio_status=%d\n",__func__,(int)*status);
            break;

        case BC_SET_HDMI_AUDIO_ONOFF:
            SEC_PRF("BC_SET_HDMI_AUDIO_ONOFF=%d\n",(int)*status);
            ali_hdmi_audio_switch_onoff(*status);
            break;

        default:
            ret=-1;
            SEC_PRF("%s:wrong cmd=%d\n",__func__,cmd);
            break;       
    }
	
	return ret;
}
EXPORT_SYMBOL(rpc_s2m_bc_hdmi_cmd);

int rpc_s2m_bc_set_dsc_status(unsigned char bServiceIdx, unsigned long param)
{
    int ret=-1;
    int dev_id = -1;
    P_sec_dev sec_devp = NULL;
    S_DSC_HDL *dsc_hdl_status = (S_DSC_HDL *)param;

    dev_id = find_dev_by_service_id(bServiceIdx);
    if(-1 == dev_id)
    {
        SEC_PRF("[%d]ERR: bServiceIdx[0x%x] is NOT exit!\n",__LINE__,bServiceIdx);
        return -1;
    }
    if(NULL != p_sec_dev_priv[dev_id])
    {
        sec_devp = p_sec_dev_priv[dev_id];
        //need ?
		if(down_interruptible(&sec_devp->sem))
			return -EINTR;
        memcpy(&sec_devp->dsc_hdl_info, dsc_hdl_status, sizeof(S_DSC_HDL));
        //need ?
        up(&sec_devp->sem);
        SEC_PRF("S2M_DSC_status: bServiceIdx:0x%x, dsc_algo:0x%x, key_type:0x%x, stream_id:0x%lx, sub_device_id:0x%lx, sub_dev_hdl:0x%p, pid_lists[0]:0x%x, pid_cnt:0x%lx, rec_block_count:0x%x\n",\
            bServiceIdx, dsc_hdl_status->dsc_algo, dsc_hdl_status->key_type, dsc_hdl_status->stream_id, dsc_hdl_status->sub_device_id,\
            dsc_hdl_status->sub_dev_hdl, dsc_hdl_status->pid_lists[0], dsc_hdl_status->pid_cnt, dsc_hdl_status->rec_block_count);

        ret=0;
    }
    else
    {
        ret=-1;
        SEC_PRF("[%d]ERR: bServiceIdx[0x%x] of p_sec_dev_priv is NOT exit!\n",__LINE__,bServiceIdx);
    }
    return ret;
}
EXPORT_SYMBOL(rpc_s2m_bc_set_dsc_status);

int sec_get_key_attr(int sec_fd, struct sec_ca_key_attr *p_ca_key_attr)
{
     __s32 ret = 0;
    struct sec_dev *s = NULL;
    struct fd k_fd;
	
    if (!p_ca_key_attr)
    {
        SEC_PRF("%s:invalid input parameter\n",__func__);
        return -EINVAL;
    }

    k_fd = fdget(sec_fd);
    if(!k_fd.file)
    {
        SEC_PRF("%s:get k_fd failed.\n",__func__);
        ret = -EBADF;
        goto ERR;
    }

    s = (struct sec_dev *)k_fd.file->private_data;
    if (!s)
    {
        SEC_PRF("%s:get private data failed.\n",__func__);
        ret = -EFAULT;
        goto ERR;
    }

	if(down_interruptible(&s->sem))
		return -EINTR;

    p_ca_key_attr->key_handle= s->dsc_hdl_info.key_handle;
    p_ca_key_attr->key_pos= s->dsc_hdl_info.key_pos;
    p_ca_key_attr->kl_sel= (s->dsc_hdl_info.key_pos>>8) & 0xFF;
    memcpy(p_ca_key_attr->iv_ctr, s->dsc_hdl_info.iv_ctr, 32);
    SEC_PRF("%s: service_idx=0x%x, key_handle=0x%x,key_pos=0x%x,kl_sel=0x%x\n",__func__,s->service_idx, p_ca_key_attr->key_handle,p_ca_key_attr->key_pos,p_ca_key_attr->kl_sel);
    SEC_PRF("iv_ctr[0]=0x%x, iv_ctr[8]=0x%x, iv_ctr[16]=0x%x, iv_ctr[24]=0x%x\n",p_ca_key_attr->iv_ctr[0], p_ca_key_attr->iv_ctr[8], p_ca_key_attr->iv_ctr[16], p_ca_key_attr->iv_ctr[24]);
	
    up(&s->sem);

ERR:
    fdput(k_fd);

    return ret;
}

//get attribute 
int sec_get_session_attr(int sec_fd, struct sec_ca_attr *p_ca_attr)
{
    __s32 ret = 0;
    struct sec_dev *s = NULL;
    struct fd k_fd;
	
    if (!p_ca_attr)
    {
        SEC_PRF("%s:invalid input parameter\n",__func__);
        return -EINVAL;
    }

    k_fd = fdget(sec_fd);
    if(!k_fd.file)
    {
        SEC_PRF("%s:get k_fd failed.\n",__func__);
        ret = -EBADF;
        goto ERR;
    }

    s = (struct sec_dev *)k_fd.file->private_data;
    if (!s)
    {
        SEC_PRF("%s:get private data failed.\n",__func__);
        ret = -EFAULT;
        goto ERR;
    }

	if(down_interruptible(&s->sem))
		return -EINTR;

    p_ca_attr->stream_id = s->dsc_hdl_info.stream_id;
    p_ca_attr->crypt_mode = s->dsc_hdl_info.dsc_algo;
    p_ca_attr->sub_dev_id = s->dsc_hdl_info.sub_device_id;
    p_ca_attr->sub_dev_see_hdl=  s->dsc_hdl_info.sub_dev_hdl;
    SEC_PRF("%s: service_idx=0x%x, stream_id=0x%x,crypt_mode=0x%x,sub_dev_see_hdl=0x%p\n",__func__,s->service_idx, p_ca_attr->stream_id,p_ca_attr->crypt_mode,p_ca_attr->sub_dev_see_hdl);
	
    up(&s->sem);

ERR:
    fdput(k_fd);

    return ret;
}

int sec_check_is_sec_fd(int sec_fd)
{
    __s32 ret = 0;
    struct fd k_fd;
    k_fd = fdget(sec_fd);
    if(!k_fd.file)
    {
        ret = -EBADF;
        goto ERR1;
    }
    if(k_fd.file->f_op == &sec_file_ops)
    {
        SEC_PRF("%s: SEC_FD\n",__func__);
        ret = 1;
    }
ERR1:
    fdput(k_fd);
    return ret;
}


EXPORT_SYMBOL(sec_check_is_sec_fd);
EXPORT_SYMBOL(sec_get_session_attr);
EXPORT_SYMBOL(sec_get_key_attr);


static const struct of_device_id ali_sec_of_match[] = {
       { .compatible = "alitech, alisec", },
       {},
};
MODULE_DEVICE_TABLE(of, ali_sec_of_match);

static struct platform_driver ali_sec_driver = {
       .driver    = {
               .name  = "ALi sec",
               .owner = THIS_MODULE,
               .of_match_table = ali_sec_of_match,
       },
       .probe     = ali_sec_probe,
       .remove    = ali_sec_remove,
};

module_platform_driver(ali_sec_driver);

MODULE_DESCRIPTION("Ali sec driver for vmx.");
MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_LICENSE("GPL");
