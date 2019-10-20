
#include <linux/version.h>

#if LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 35)
#include <linux/slab.h> 
#include <asm/io.h>
#include <linux/dma-mapping.h>
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/reboot.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <ali_reg.h>
#include <ali_board_config.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>

struct task_struct *g_ali_logsm_see2main_thread_id;

static unsigned char logsm_see2main_debug = 0;
#define LOGSM_SEE2MAIN_PRINTK(fmt, args...)				\
{										\
	if (0 !=  logsm_see2main_debug)					\
	{									\
		printk(fmt, ##args);					\
	}									\
}

#define MAILIN3_ADDR      0x1804020c
#define logsm_header_read()		   __REG32ALI(MAILIN3_ADDR)
#define logsm_header_write(val)    __REG32ALI(MAILIN3_ADDR) = val

struct logsm_buffer_st
{
    char data[LOGSM_SEE2MAIN_BUF_SIZE];
};

#define LOGSM_SEE2MAIN_BUF_RECIVED    0XFFFFFFFF

#define GET_LOGSM_HEADER_FLAG(A)                (A&0xFF)
#define GET_LOGSM_HEADER_LEN(A)                 ((A>>8)&0xFF)
#define GET_LOGSM_HEADER_CURRENT_INDEX(A)       ((A>>16)&0xFF)
#define GET_LOGSM_HEADER_TOTALE_INDEX(A)        ((A>>24)&0xFF)

static struct logsm_buffer_st* g_logsm_buf = NULL;
static unsigned int g_logsm_buf_len;

#define  MAX_LINE_LEN          512
#define  MAX_RELAX_THRESHOLD   10

/*
**this thread is used for see to output log to main cpu by share memory
*/
static int ali_logsm_see2main_thread(void *p)
{
    static unsigned char compare_current_index = 0;
    static unsigned char compare_total_index = 0;
	static unsigned int current_data_pos = 0;
    static unsigned int msg_find = 0;
	char printk_buf[MAX_LINE_LEN] = {0};
	unsigned char pkt_size = 0;
	unsigned char current_index = 0;
	unsigned char total_index = 0;
	static int force_relax_cpu_threshold = 0;

    LOGSM_SEE2MAIN_PRINTK("%s,%d enter\n", __FUNCTION__, __LINE__);
    
    while (!kthread_should_stop())
	{
       	   if(g_logsm_buf)
		   {
		       if(logsm_header_read() != LOGSM_SEE2MAIN_BUF_RECIVED)
		       {
					current_index = GET_LOGSM_HEADER_CURRENT_INDEX(logsm_header_read());
					total_index = GET_LOGSM_HEADER_TOTALE_INDEX(logsm_header_read());
					LOGSM_SEE2MAIN_PRINTK("current_index:%d, total_index:%d\n",current_index, total_index);
					if(msg_find == 0)
					{
					  if((total_index > 0)&&(current_index == 0))
					  {   
						 compare_total_index = total_index;
						 compare_current_index = 0;
						 msg_find = 1;
						 memset(printk_buf, 0, MAX_LINE_LEN);
						 current_data_pos = 0;	  
					  }
					}
					
					if(msg_find == 1)
					{
					   if((current_index == compare_current_index)&&(total_index == compare_total_index))
					   {	      
						  LOGSM_SEE2MAIN_PRINTK("current_index:%d, compare_total_index:%d\n",current_index, compare_total_index); //internal debug

						  if(current_index < compare_total_index)
						  {
							  compare_current_index++;		  
							  pkt_size=GET_LOGSM_HEADER_LEN(logsm_header_read());
							  if (current_data_pos + pkt_size >= MAX_LINE_LEN - 1)
								pkt_size = MAX_LINE_LEN - current_data_pos;
							  if(pkt_size > 0)
								memcpy(printk_buf + current_data_pos, g_logsm_buf->data, pkt_size);
							  /*notify see cpu*/
							  logsm_header_write(LOGSM_SEE2MAIN_BUF_RECIVED);
							  current_data_pos += pkt_size;
						  }
				  
						  if(compare_current_index == compare_total_index)
						  {  	      
							  LOGSM_SEE2MAIN_PRINTK("current_index:%d, compare_total_index:%d\n",current_index, compare_total_index); //internal debug
                              /*really output by main cpu*/
							  printk("[soc]%s",printk_buf); 	
							  msg_find = 0;
							  force_relax_cpu_threshold++;
						  }
					   	}
			         }

					if(force_relax_cpu_threshold > MAX_RELAX_THRESHOLD)
					{
					   force_relax_cpu_threshold = 0;
					   /*can not be removed*/
					   msleep(1);
					}
	            }
			    else
		   		{
		   		   force_relax_cpu_threshold = 0;
		   		   /*can not be removed*/
	               msleep(1);
		   	    }
	       }
   } //end while
   return 0;
}

static int ali_logsm_start_kthread(void)
{	
	g_ali_logsm_see2main_thread_id = kthread_create(ali_logsm_see2main_thread, NULL, "ali_logsm");
	if(IS_ERR(g_ali_logsm_see2main_thread_id)){
		printk("g_ali_logsm_see2main_thread_id create fail\n");
		g_ali_logsm_see2main_thread_id = NULL;
		return -1;
	}
	wake_up_process(g_ali_logsm_see2main_thread_id);
	
	return 0;
}


static int __init ali_logsm_init(void)
{
	g_logsm_buf = (struct logsm_buffer_st*)__G_ALI_MM_LOGSM_SEE2MAIN_START_ADDR;
	g_logsm_buf_len = (unsigned int)__G_ALI_MM_LOGSM_SEE2MAIN_SIZE;
	if(!g_logsm_buf)
	{
	    printk("g_logsm_see2main_addr is NULL!!\n");
		goto fail;
	}
	
    LOGSM_SEE2MAIN_PRINTK("g_logsm_see2main_addr:0x%p, g_logsm_see2main_len:0x%x\n", g_logsm_buf, g_logsm_buf_len);
	
	memset(g_logsm_buf, 0, g_logsm_buf_len);
	logsm_header_write(LOGSM_SEE2MAIN_BUF_RECIVED);
		
	ali_logsm_start_kthread();

	LOGSM_SEE2MAIN_PRINTK("%s, %d. OK\n", __FUNCTION__, __LINE__);

    return(0);

fail:
    return(-1);
}

static void __exit ali_logsm_exit(void)
{
	LOGSM_SEE2MAIN_PRINTK("[ %s, %d ]\n", __FUNCTION__, __LINE__);
}

module_init(ali_logsm_init);
module_exit(ali_logsm_exit);
