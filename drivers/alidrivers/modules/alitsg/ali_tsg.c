/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

/*****************************************************************************/
/**
 * MODIFICATION HISTORY:
 *
 * Ver   Who          Date     Changes
 * ----- ------------ -------- -----------------------------------------------
 * 1.00a Jingang Chu  06/11/14 First release
 *
 *****************************************************************************/

 /***************************** Include Files *********************************/
 
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> 

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
#include <linux/sched.h>
#include <linux/sched/rt.h>
#else
#include <linux/sched.h>
#endif

#include <linux/kthread.h>
#include <linux/completion.h>
#include <ali_interrupt.h>
#include <asm/uaccess.h>
#include <ali_tsg_common.h>
#include <ali_reg.h>
#include <asm/io.h>
#include "ali_tsg.h"

#include <linux/of.h>
#include <linux/of_platform.h>
#include <ali_soc.h>

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

/************************** Variable Definitions *****************************/

/* Predefined ALI TSG HW buffer address.
*/
extern __u32 __G_ALI_MM_TSG_BUF_START_ADDR;
extern __u32 __G_ALI_MM_TSG_BUF_SIZE;


#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
struct file_operations ali_tsg_fops = 
{
    .owner =    THIS_MODULE,
    .write =    ali_tsg_usr_wr,
    .unlocked_ioctl = ali_tsg_ioctl,
    .open =     ali_tsg_open,
    .release =  ali_tsg_close,
};
#else
struct file_operations ali_tsg_fops = 
{
    .owner =    THIS_MODULE,
    .write =    ali_tsg_usr_wr,
    .ioctl =    ali_tsg_ioctl,
    .open =     ali_tsg_open,
    .release =  ali_tsg_close,
};
#endif

/* Internal data structure.
*/
static struct ali_tsg_dev  ali_tsg_dev[1];
static struct class       *ali_tsg_class;
static struct device      *ali_tsg_classdev;
static __s32               ali_tsg_cdev_add_ret;
static __s32               ali_tsg_alloc_chrdev_ret;
static struct task_struct *ali_tsg_xfer_task_ret; 
static __s32               ali_tsg_request_irq_ret;

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
__s32 ali_tsg_close
(
    struct inode *inode,
    struct file  *file
)
{
    struct ali_tsg_dev *tsg;

    tsg = file->private_data;
    
    if (mutex_lock_interruptible(&tsg->io_mutex))
    {
        return(-ERESTARTSYS);
    }

	if (tsg->used == 0)
	{
		mutex_unlock(&tsg->io_mutex);
		return(-EPERM);        
	}

	tsg->used--;

	if(tsg->used == 0)
	{
	    tsg->buf_rd = 0;
	    tsg->buf_wr = 0;

	    tsg->status = ALI_TSG_STATUS_IDLE;
	}

    mutex_unlock(&tsg->io_mutex);

    return(0);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
__s32 ali_tsg_open
(
    struct inode *inode,
    struct file  *file
)
{
    struct ali_tsg_dev *tsg;
    
    tsg = container_of(inode->i_cdev, struct ali_tsg_dev, cdev);
    
    if (mutex_lock_interruptible(&tsg->io_mutex))
    {
        return(-ERESTARTSYS);
    }
    
    file->private_data = tsg;

	if(tsg->used == 0)
		tsg->status = ALI_TSG_STATUS_RUN;

	tsg->used++;

    mutex_unlock(&tsg->io_mutex);
    
    return(0);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
__s32 ali_tsg_hw_chg_setting
(
    struct ali_tsg_dev *tsg
)
{
    __u8 clk;

    clk = ioread8(tsg->base + ALI_TSG_REG_GLB_CTRL);
    if (clk != tsg->hw_clk)
    {
        iowrite8(tsg->hw_clk, tsg->base + ALI_TSG_REG_GLB_CTRL); 
    }

    return(0);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
__s32 ali_tsg_hw_set_output_clk
(
    struct ali_tsg_dev *tsg,
    unsigned int        cmd,
    unsigned long       arg
)
{
    int  ret;
    __u8 clk;
    
    if (tsg->status != ALI_TSG_STATUS_RUN)
    {
        return(-EPERM);        
    }

    ret = copy_from_user(&clk, (void __user *)arg, _IOC_SIZE(cmd));
    if (0 != ret)
    {
        ali_tsg_dbg_show("%s,%d,copy_from_user() failed!\n", __FUNCTION__, __LINE__);

        return(-EFAULT);
    }

    tsg->hw_clk = clk;
    
    return(ret);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
__s32 ali_tsg_xfer_task
(
    void *param
)
{
    __u32               rd_len;
    __u32               rd_unit_cnt;    
    struct ali_tsg_dev *tsg;
    __u8               *rd_addr;
    __u32               rd_phy_addr;
	__s32               ret;
	
    tsg = (struct ali_tsg_dev *)param;
    
    for (;;)
    {
        if (kthread_should_stop())
        {
            ali_tsg_dbg_show("%s,%d,now exit.\n", __FUNCTION__, __LINE__);

            return(-__LINE__);
        }
        
        msleep(5);
        
        if (mutex_lock_interruptible(&tsg->io_mutex))
        {
            return(-__LINE__);
        }   

        if (tsg->status != ALI_TSG_STATUS_RUN)
        {
            mutex_unlock(&tsg->io_mutex);
    
            continue;        
        }
        
        /* Dynamicly change TSG HW setting during each start of a transfer.
        */
        ali_tsg_hw_chg_setting(tsg);

        if (tsg->buf_wr >= tsg->buf_rd)
        {
            rd_len = tsg->buf_wr - tsg->buf_rd;
        }
        else
        {
            rd_len = tsg->buf_len - tsg->buf_rd;
        }

        /* C3921 TSG has a HW restriction:
         * 1, data must be xfered in unit of 16 bytes(128 bits for C3921);
         * 2, data better be xfered in unit of 188 bytes(TS packet).
         * Hence, the xfer unit must be rounded in unit if 752 byte
         * (Least Common Multiple of 128 and 16).
         */
        rd_len = ((rd_len / ALI_TSG_XFER_UNIT_LEN) * ALI_TSG_XFER_UNIT_LEN);

        /* ALI TSG HW accept xfer length as 16 byte per unit(for C3921) .
        */
        rd_unit_cnt = (rd_len >> 4);

        if (rd_unit_cnt > 0)
        {
            rd_addr = tsg->buf + tsg->buf_rd;

            ali_tsg_dbg_chk_conti_out(rd_addr, rd_len, "Ali TSG OUT");
            
            /* Step 1: Set xfer length to HW.
             */            
            iowrite32(rd_unit_cnt, tsg->base + ALI_TSG_REG_HW_BUF_LEN); 
            
            /* Step 2: 
             * Set xfer data address(physical) to hw.
             * Also set xfer data ready bit to hw.
             */   
            #ifdef CONFIG_ARM
            rd_phy_addr = (((__u32)(rd_addr - 0xC0000000)) & 0x0FFFFFF0);
            #elif defined CONFIG_MIPS
            rd_phy_addr = ((__u32)rd_addr & 0x0FFFFFF0);           
            #else
            #error "Unsupported CPU, suppoted main CPU archs are ARM and MIPS."
            #endif

            iowrite32(rd_phy_addr | 0x1, tsg->base + ALI_TSG_REG_HW_BUF_ADDR);     
            
            /* Step 3: Set xfer star bit to hw, Start TSG xfer. 
            */
            iowrite8(1, tsg->base + ALI_TSG_REG_DMA_CTRL); 
    
            /* Step 4: Wait until TSG HW finished this xfer.
            */
            ali_tsg_dbg_xfer_start_cnt_inc();

	        ret = wait_for_completion_interruptible_timeout(
				&tsg->xfer_finish, 10000);

			if (ret > 0)
			{
                ali_tsg_dbg_xfer_ok_cnt_inc();
			}
		    /* Timeout?
			*/			
			else if(0 == ret)
			{
                ali_tsg_dbg_xfer_timeout_cnt_inc();				
			}
			else
			{
                ali_tsg_dbg_xfer_interrupted_cnt_inc();
			}

            /* Step 6: Update buffer read pointer. 
            */
            tsg->buf_rd += rd_len;
    
            if (tsg->buf_rd >= tsg->buf_len)
            {
                tsg->buf_rd = 0;
            }
        }

        mutex_unlock(&tsg->io_mutex);
    }
    
    return(-__LINE__);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
ssize_t ali_tsg_usr_wr
(
    struct file       *file, 
    const char __user *buf,
    size_t             count,
    loff_t            *ppos
)
{
    struct ali_tsg_dev *tsg;
    __u32               copied_len;
    __u32               free_len;
    __u32               wr_len;

    ali_tsg_dbg_show("%s,%d,count:%d\n", __FUNCTION__, __LINE__, count);
    
    tsg = file->private_data;

    ali_tsg_dbg_chk_conti_in(buf, count, "Ali TSG IN");  
        
    copied_len = 0;

	ali_tsg_dbg_usr_wr_cnt_inc();
    
    while (copied_len < count)
    {  
        /* Protect against concurrent data access.
        */
        if (mutex_lock_interruptible(&(tsg->io_mutex)))
        {
            printk("%s,%d\n", __FUNCTION__, __LINE__);
            return(-ERESTARTSYS);
        }

        if (tsg->status != ALI_TSG_STATUS_RUN)
        {
            mutex_unlock(&tsg->io_mutex);

	        printk("%s,%d,status:%d\n", __FUNCTION__, __LINE__, tsg->status);

            return(-EPERM);   
        }

        if (tsg->buf_rd > tsg->buf_wr)
        {
            free_len = tsg->buf_rd - tsg->buf_wr - 1;
        }
		/* (tsg->buf_rd <= tsg->buf_wr) && (0 == tsg->buf_rd)
		*/
        else if (0 == tsg->buf_rd)
        {
            free_len = tsg->buf_len - tsg->buf_wr - 1;        
        }
		/* (tsg->buf_rd <= tsg->buf_wr) && (0 != tsg->buf_rd)
		*/		
		else
		{
            free_len = tsg->buf_len - tsg->buf_wr;
		}

		/* Buffer full, wait data to be consumed by ali_tsg_xfer_task().
		*/
		if (free_len <= 0)
		{
		    ali_tsg_dbg_usr_wr_wait_cnt_inc();

            mutex_unlock(&(tsg->io_mutex));	
		
            msleep(5);

			continue;
		}

		/* Buffer not full, copy data into it.
		*/  
		wr_len = count - copied_len;

		if (wr_len >= free_len)
		{
            wr_len = free_len;
		}

        if (copy_from_user(tsg->buf + tsg->buf_wr, buf + copied_len, wr_len))
        {    
            mutex_unlock(&(tsg->io_mutex));
            
            return(-EFAULT);
        }
        
        tsg->buf_wr += wr_len;
        
        if (tsg->buf_wr >= tsg->buf_len)
        {        
            tsg->buf_wr = 0;
        }
        
        copied_len += wr_len;

        mutex_unlock(&(tsg->io_mutex));		
    }	
    
    return(copied_len);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
irqreturn_t ali_tsg_isr
(
    __s32  irq,
    void  *dev
)
{
    __s8                event;
    struct ali_tsg_dev *tsg;
    
    tsg = dev;
    
    event = ioread8(tsg->base + ALI_TSG_REG_INT_STAT);
        
    /* Sync byte error interrupt.
    */
    if (event & 0x10)
    {
        //ali_tsg_dbg_show("%s, %d\n", __FUNCTION__, __LINE__);    
        ali_tsg_dbg_syncerr_irq_inc();
    }

    /* Buf & FIFO transfer complete interrupt.
    */
    if (event & 0x80)
    {
        //ali_tsg_dbg_show("%s, %d\n", __FUNCTION__, __LINE__);
        complete(&tsg->xfer_finish);
        ali_tsg_dbg_xfer_irq_cnt_inc();
    }

    /* Must clear int status register.
    */
    iowrite8(event, tsg->base + ALI_TSG_REG_INT_STAT);
    
    return(IRQ_HANDLED);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
int ali_tsg_get_cur_data_len
(
    struct ali_tsg_dev *tsg,
    unsigned int        cmd,
    unsigned long       arg
)
{
    int   ret;
    __u32 data_len;
 
    ret = 0;
    
    ALI_TSG_DEBUG("%s, %d\n", __FUNCTION__, __LINE__);
    
    if (tsg->status != ALI_TSG_STATUS_RUN)
    {
        return(-EPERM);        
    }

    if (tsg->buf_wr >= tsg->buf_rd)
    {
        data_len = tsg->buf_wr - tsg->buf_rd;
    }
    else
    {
        data_len = tsg->buf_len - tsg->buf_rd + tsg->buf_wr;
    }    

    ret = copy_to_user((void __user *)arg, &data_len, _IOC_SIZE(cmd));

    if (0 != ret)
    {
        ret = -ENOTTY;
    }  

    ALI_TSG_DEBUG("%s,%d,%d\n", __FUNCTION__, __LINE__, data_len);

    return(ret);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
long ali_tsg_ioctl
(
    struct file  *filp,
    unsigned int cmd,
    unsigned long arg
)
#else
long ali_tsg_ioctl
(
    struct inode *inode,
    struct file  *filp,
    unsigned int cmd,
    unsigned long arg
)
#endif
{
    int                 ret;
    struct ali_tsg_dev *tsg;
    
    ret = 0;
    tsg = filp->private_data;
    
    if (mutex_lock_interruptible(&tsg->io_mutex))
    {
        return(-ERESTARTSYS);
    }

    switch(cmd)
    {
        case ALI_TSG_OUTPUT_CLK_SET:
        {
            ali_tsg_hw_set_output_clk(tsg, cmd, arg);
        }
        break;

        case ALI_TSG_GET_CUR_DATA_LEN:
        {
            ali_tsg_get_cur_data_len(tsg, cmd, arg);
        }
        break;
        
        default:
        {
            ret = -ENOTTY;
        }
        break;
    }

    mutex_unlock(&tsg->io_mutex);
    
    return(ret); 
}


__s32 ali_tsg_buf_wr_set
(
    __u32 wr
)
{
    ali_tsg_dev[0].buf_wr = wr;

	return(0);
}


__s32 ali_tsg_buf_wr_get
(
    void
)
{
    return(ali_tsg_dev[0].buf_wr);
}


__s32 ali_tsg_buf_rd_set
(
    __u32 rd
)
{
    ali_tsg_dev[0].buf_rd = rd;

	return(0);
}


__s32 ali_tsg_buf_rd_get
(
    void
)
{
    return(ali_tsg_dev[0].buf_rd);
}


__s32 ali_tsg_buf_len_set
(
    __u32 len
)
{
    ali_tsg_dev[0].buf_len = len;

	return(0);
}


__s32 ali_tsg_buf_len_get
(
    void
)
{
    return(ali_tsg_dev[0].buf_len);
}

/*****************************************************************************/
/**
* TODO: Description of this function
* - TODO: Description 1
* - TODO: Description 2
* - TODO: Description 3
* - TODO: Description 4
*
*
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
* @param :TODO: Description of this param.
*        
* @return
* - TODO: Description of this return value.
* - TODO: Description of this return value.
*
******************************************************************************/
static void ali_tsg_exit
(
    void
)
{
    __s32               ret;   
    struct ali_tsg_dev *tsg;  
    
    tsg = &(ali_tsg_dev[0]);
    
    ali_tsg_dbg_show("%s,%d,Go.\n", __FUNCTION__, __LINE__);

    if (!IS_ERR(ali_tsg_xfer_task_ret))
    { 
        ret = kthread_stop(ali_tsg_xfer_task_ret); 
        
        ali_tsg_dbg_show("%s,%d,kthread_stop() failed,ret:%d.\n", __FUNCTION__, __LINE__, ret);
    } 

    if (0 == ali_tsg_request_irq_ret)
    {
        free_irq(tsg->irq_num, (void *)tsg);
    }

    /* Destroy device node under directory "/dev".
     */
    if (!IS_ERR(ali_tsg_classdev))
    {
        device_destroy(ali_tsg_class, tsg->dev_id);
    }

    /* Destroy class node under directory "/sys/class".
    */
    if (!IS_ERR(ali_tsg_class))
    {
        class_destroy(ali_tsg_class);

        ali_tsg_class = NULL;
    }

    if (0 == ali_tsg_cdev_add_ret)
    {
        cdev_del(&tsg->cdev);
    }

    if (ali_tsg_alloc_chrdev_ret >= 0) 
    {
        unregister_chrdev_region(tsg->dev_id, 1);
    }   
    
    ali_tsg_dbg_show("%s,%d,Succeed.\n", __FUNCTION__, __LINE__);

    ali_tsg_dbg_exit();
	
    return;
}

static __s32 ali_tsg_hw_init(void)
{
	struct ali_tsg_dev *tsg; 
	
	tsg = &(ali_tsg_dev[0]);

    /* Init regs. 
    */
    /* Sync byte: 0x47.
    */
    iowrite8(0x47, tsg->base + ALI_TSG_REG_SYNC_BYTE);   
    
    /* Set TSG output clock to default to ALI_TSG_DEFAULT_CLK_SETTING.
    */
    iowrite8(tsg->hw_clk, tsg->base + ALI_TSG_REG_GLB_CTRL);

    /* Enable sync byte checking & valid continue while clk discontinue.
    */
    /* Pause mode.
    */
    iowrite8(0x80, tsg->base + ALI_TSG_REG_GLB_CTRL + 1); 

    /* Enable tsg finish interrupt & sync byte error interrupt.
    */
    iowrite8(0x90, tsg->base + ALI_TSG_REG_INT_CTRL);

    /* Clear all interrupt status & enable sync byte error interrupt. 
    */
    iowrite8(0xF0, tsg->base + ALI_TSG_REG_INT_STAT);

    /* 1, Set dummy buffer header;
     * 2, Disable dummy buffer insertion. 
     */
    iowrite32(0x10FF1F00, tsg->base + ALI_TSG_REG_DUMMY_INSERT_CFG);

	return 0;
}


/*****************************************************************************/
/**
* Initialize a specific ali TSG instance/driver. The initialization entails:
* - Initialize fields of the TSG instance structure
* - Reset HW and apply default options
* - Configure the packet FIFOs if present
* - Configure the DMA channels if present
*
*        
* @return
* - TODO:implement
* - TODO:implement
*
******************************************************************************/
static __s32 ali_tsg_init(struct platform_device *pdev)
{
    __u32	result;
    struct ali_tsg_dev *tsg;
    struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};
    
    ali_tsg_dbg_show("%s, %d.\n", __FUNCTION__, __LINE__);
    
    /* Init sw sturctures. 
    */
    tsg = &(ali_tsg_dev[0]);
    
    memset(tsg, 0, sizeof(struct ali_tsg_dev));
    mutex_init(&tsg->io_mutex);
    tsg->base = (void __iomem *)(ALI_PHYS2VIRT(ALI_TSG_REG_BASE));

    /* Clk default to 0x18.
    */
    tsg->hw_clk = ALI_TSG_DEFAULT_CLK_SETTING;

	ali_tsg_hw_init();

    init_completion(&tsg->xfer_finish); 

	result = of_get_major_minor(pdev->dev.of_node,&tsg->dev_id, 
			0, 1, "ali_m36_tsg_0");
	if (result  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto fail;
	}

    cdev_init(&(tsg->cdev), &ali_tsg_fops);
    tsg->cdev.owner = THIS_MODULE;
    
    ali_tsg_cdev_add_ret = cdev_add(&tsg->cdev, tsg->dev_id, 1);
    
    /* Fail gracefully if need be.
    */
    if (ali_tsg_cdev_add_ret)
    {
        ali_tsg_dbg_show("%s,cdev_add() failed, result:%d\n",
                         __FUNCTION__, ali_tsg_cdev_add_ret);
        goto fail;
    }

    /* Create device node under directory "/dev".
    */
    ali_tsg_class = class_create(THIS_MODULE, "ali_tsg_class");
    if (IS_ERR(ali_tsg_class))
    {
        result = PTR_ERR(ali_tsg_class);
        ali_tsg_dbg_show("%s,class_create() failed, result:%d\n",
                         __FUNCTION__, result);      
        goto fail;
    }

    ali_tsg_classdev = device_create(ali_tsg_class, NULL, tsg->dev_id, 
                                     tsg, "ali_m36_tsg_0");
    if (IS_ERR(ali_tsg_classdev))
    {
        result = PTR_ERR(ali_tsg_classdev);
        ali_tsg_dbg_show("%s,class_create() failed, result:%d\n",
                         __FUNCTION__, result);      
        goto fail;
    }

    /* Register ISR.
    */
   
    tsg->irq_num = INT_ALI_TSG;
    
    
    ali_tsg_request_irq_ret = request_irq(tsg->irq_num, ali_tsg_isr, 0,
                                          "ali_tsg_0", (void *)tsg);

    if (ali_tsg_request_irq_ret)
    {
        ali_tsg_dbg_show("%s,%d,ret:%d.\n", __FUNCTION__, __LINE__,
                         ali_tsg_request_irq_ret);
        goto fail;
    }

    ali_tsg_xfer_task_ret = kthread_create(ali_tsg_xfer_task, (void *)tsg,
                                           "ali_tsg_xfer_task");

    if (IS_ERR(ali_tsg_xfer_task_ret))
    {
        return(PTR_ERR(ali_tsg_xfer_task_ret)); 
    }

    sched_setscheduler(ali_tsg_xfer_task_ret, SCHED_RR, &param);

    wake_up_process(ali_tsg_xfer_task_ret);

    /* HW buffer address, also could be accessed by main CPU.
    */
    tsg->buf = (__u8 *)(__G_ALI_MM_TSG_BUF_START_ADDR);
    
    /* Round to a multiple of 47K to best fit ALI_PVR VOBU size.
    */
    tsg->buf_len = (__G_ALI_MM_TSG_BUF_SIZE / ALI_TSG_HW_BUF_UNIT_LEN);
    
    tsg->buf_len *= ALI_TSG_HW_BUF_UNIT_LEN; 
    
    ali_tsg_dbg_init();

    ali_tsg_dbg_show("%s,%d,succeed.\n", __FUNCTION__, __LINE__);
    
    return(0);

fail:
    ali_tsg_exit();
    ali_tsg_dbg_show("%s,%d,failed\n", __FUNCTION__, __LINE__);
    return(-1);
}


/*
 *  Initialisation
 */
static int alitsg_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk("%s, %d\n", __FUNCTION__, __LINE__);

	ali_tsg_init(pdev);
	
	return ret;
}


/*
 *  Removement
 */
static int alitsg_remove(struct platform_device *pdev)
{
	printk("%s, %d\n", __FUNCTION__, __LINE__);

	ali_tsg_exit();

	return 0;
}


static int alitsg_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}


static int alitsg_resume(struct platform_device *pdev)
{
	struct ali_tsg_dev *tsg; 
	
	tsg = &(ali_tsg_dev[0]);
	
	ali_tsg_hw_init();
	
	return 0;
}


static const struct of_device_id ali_tsg_dt_ids[] = {
	{ .compatible = "alitech,ali-tsg" },
	{ }
};
MODULE_DEVICE_TABLE(of, ali_tsg_dt_ids);


static struct platform_driver alitsg_driver = {
	.probe	 = alitsg_probe,
	.remove  = alitsg_remove,
	.suspend = alitsg_suspend,
	.resume  = alitsg_resume,	
	.driver  =
	{
		.name	= "ali-tsg",
		.owner	= THIS_MODULE,	
		.of_match_table = ali_tsg_dt_ids,
	},
};

module_platform_driver(alitsg_driver);

MODULE_AUTHOR("ALi Corporation, Inc.");
MODULE_DESCRIPTION("ALI tsg driver");
MODULE_LICENSE("GPL");

