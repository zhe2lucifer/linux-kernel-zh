/*----------------------------------------------------------------------------
   Solution:  demodulater CXD2856. + tuner MXL603
------------------------------------------------------------------------------
    History:       
	Date                  athor           version                reason 
------------------------------------------------------------------------------
    2017/10/16          leo.liu           v0.1              create the debug file for 3281 driver     
----------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>


#include "nim_cxd2856_common.h"
static char device_name[2][20] 	={"nim_cxd2856_0","nim_cxd2856_1"};//proc node name
static char nim_name[2][50] 	={"ali_cxd2856_nim0","ali_cxd2856_nim1"};//dev node name

/*
	NOTE:  Because moniting functions of sony demod,such as get_cn,get_sqi,get_ssi etc,
	must be called after locked,or these api will return error.So ,we write a special api:
		nim_cxd2856_special_monitor_per();
	When the per is detected,first judge whether demod is lock,if it is unlcok, 
	the it will show print :unlock. not to pay attention to the value of per.
*/
static void nim_cxd2856_special_monitor_per(struct nim_device *dev)
{
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	struct nim_debug  	* debug_data =  NULL;
	unsigned long		parg = 0;
	static bool 		per_flag = TRUE;
	int lock= 0,cn 	= 0,rf 	= 0,per = 0;
	
	priv		= dev->priv;
	user 		= priv->user;
	debug_data	= &user->debug_data;
	per = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_RSUB,parg);
	if(0 != per)
	{	
		lock= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_LOCK_STATUS,parg);
		if(lock)
		{
			if( !per_flag)
			{
				per_flag = TRUE;
				pr_warning("demod_%ld ++++++ has re locked\n",user->demod_id);
			}
			lock= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_LOCK_STATUS,parg);
			cn 	= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_CN_VALUE,parg);
			rf 	= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_RF_LEVEL,parg);
			pr_warning("demod_%ld ++++++per = %d lock = %d cn = %d rf = %d\n",
				user->demod_id,per,lock,cn,rf);
		}
		else
		{
			if(INVALID_VALUE == per)
			{
				if( per_flag)
				{
					per_flag = FALSE;
					pr_warning("demod_%ld ++++++ unlcok,per get error\n",user->demod_id);
				}
			}
			else
			{
				lock= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_LOCK_STATUS,parg);
				cn 	= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_CN_VALUE,parg);
				rf 	= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_RF_LEVEL,parg);
				pr_warning("demod_%ld ++++++per = %d lock = %d cn = %d rf = %d\n",
					user->demod_id,per,lock,cn,rf);
			}
		}
	}
	else
	{
		if( !per_flag)
		{
			per_flag = TRUE;
			pr_warning("demod_%ld ++++++ has re locked\n",user->demod_id);
		}
	}
		
}
static void nim_cxd2856_proc_monitor_task(struct work_struct *nim_work)
{
	struct nim_device 	* dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	struct nim_debug  	* debug_data =  NULL;
	int lock= 0,ssi = 0,sqi = 0,cn 	= 0,rf 	= 0,per = 0,ber = 0;
	unsigned long parg;
	//static bool per_flag = TRUE;
	if(nim_work == NULL)
	{
		return;
	}
    dev = container_of((void *)nim_work,struct nim_device, debug_work);
	if(dev == NULL)
	{
		return;
	}
	priv		= dev->priv;
	user 		= priv->user;
	debug_data	= &user->debug_data;
	pr_warning("	start proc monitor cxd2856_%ld task\n",user->demod_id);
	while(TASK_RUN == debug_data->monitor_status)
	{
		msleep_interruptible(100);//must set this code
		if(!user->nim_init)//start monitor after hardware init
		{
			continue;
		}
		if(0xff == debug_data->monitor_object)//0xff needs to be judged separately
		{
			
			lock= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_LOCK_STATUS,parg);
			ssi = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_AGC,parg);
		 	sqi = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_SNR,parg);
			cn 	= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_CN_VALUE,parg);
			rf 	= ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_RF_LEVEL,parg);
			per = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_RSUB,parg);
			ber = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_QPSK_BER,parg);
			pr_warning("\ndemod_%ld:lock = %d cn = %d rf = %d ssi = %d sqi = %d per = %d,ber = %d\n",
				user->demod_id,lock,cn,rf,ssi,sqi,per,ber);
		}
		else
		{
			if(0x02 == debug_data->monitor_object)//0x02 needs to be judged separately
			{
				nim_cxd2856_special_monitor_per(dev);
			}
			else
			{
				if(0x01 == (debug_data->monitor_object & 0x01))
				{
					per = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_RSUB,parg);
					pr_warning("per = %d\t",per);
				}
				if(0x02 == (debug_data->monitor_object & 0x02))
				{
					per = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_RSUB,parg);
					if(per != 0)
						pr_warning("++++++per = %d\n",per);
				}
				if(0x04 == (debug_data->monitor_object & 0x04))
				{
					lock = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_LOCK_STATUS,parg);
					pr_warning(" lock = %d\t",lock);
				}
				if(0x08 == (debug_data->monitor_object & 0x08))
				{
					rf = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_RF_LEVEL,parg);
					pr_warning(" rf = %d\t",rf);
				}
				if(0x10 == (debug_data->monitor_object & 0x10))
				{
					cn = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_GET_CN_VALUE,parg);
					pr_warning(" cn = %d\t",cn);
				}
				if(0x20 == (debug_data->monitor_object & 0x20))
				{
					ssi = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_AGC,parg);
					pr_warning(" ssi = %d\t",ssi);
				}
				if(0x40 == (debug_data->monitor_object & 0x40))
				{
					sqi = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_SNR,parg);
					pr_warning(" sqi = %d",sqi);
				}
				if(0x80 == (debug_data->monitor_object & 0x80))
				{
					ber = ali_cxd2856_nim_ioctl_mutex(dev,ALI_NIM_READ_QPSK_BER,parg);
					pr_warning(" ber = %d",ber);
				}
				pr_warning("\n");
			}
			
		}
		
	}
	debug_data->monitor_status = TASK_EXIT;//task has exited
	debug_data->monitor_object = 0x0;// clear monitor  object
	pr_warning("exit demod_%ld proc monitor task\n",user->demod_id);
}

ssize_t nim_cxd2856_proc_i2c_read(struct file * file, char __user *buffer, size_t count, loff_t *fops)
{
	
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	sony_demod_t 		* pdemod = NULL;
	struct nim_debug  	* debug_data =  NULL;
	struct COFDM_TUNER_CONFIG_API	*tuner_control;
	UINT32 tuner_chipid = 0;
	//dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = (struct nim_device *)proc_get_parent_data(file->f_path.dentry->d_inode);	
	if(nim_dev == NULL)
		goto ERROR;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR;
	user 	= priv->user;
	pdemod 	= priv->pDemod;
	if(user == NULL)
		goto ERROR;
	debug_data		= &user->debug_data;
	tuner_control	= &user->tuner_control;
	
	if(user->nim_init)//read chipid after hardware init
	{
		/**** read tuner chipid ****/
		if(NULL == tuner_control->nim_tuner_command)
		{
			goto ERROR;
		}
		else
		{
			tuner_control->nim_tuner_command(user->tuner_id,NIM_TUNER_GET_CHIPID,(__u32)&tuner_chipid);
		}
		/**** read demod chipid ****/
		pdemod->chipId = 0;
		if(sony_demod_ChipID (pdemod, &(pdemod->chipId)) !=SONY_RESULT_OK )
		{
		 	pr_warning("\nread chipid error\n");
		}
	}
	else
	{
		pr_warning("\n  !!!can't read chipid,because hw don't INIT\n\n");
	}
	
	pr_warning("debug qam_%ld; tuner i2c mode is %s\n"
		   "tuner chipid = 0x%lx demod chipid = 0x%x\n"
		   "demod i2c id = 0x%lx demod i2c addr = 0x%x \n"
		   "tuner i2c id = 0x%x tuner i2c addr = 0x%x \n",
		   user->demod_id,
		   ((user->tuner_i2c_communication_mode == SONY_DEMOD_TUNER_I2C_CONFIG_GATEWAY) ? "gatway" : "repeater"),
		   tuner_chipid,pdemod->chipId,
		   user->demod_i2c_type,user->demod_addr,
		   tuner_control->tuner_config.i2c_type_id,tuner_control->tuner_config.c_tuner_base_addr);
	return 0;
ERROR:
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}
ssize_t nim_cxd2856_proc_i2c_write(struct file * file, const char __user *buffer, size_t count, loff_t *fops)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	sony_demod_t 		* pdemod = NULL;
	struct nim_debug  	* debug_data =  NULL;
	struct COFDM_TUNER_CONFIG_API	*tuner_control;
	struct debug_i2c     debug_i2c;
	int reg_addr = 0,reg_data = 0;
	char   buf[10] = {0};
	char * tmp = kzalloc((count+1), GFP_KERNEL);
	if (!tmp)
	{
		return -ENOMEM;
	}
	//dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = proc_get_parent_data(file->f_path.dentry->d_inode);
	
	if(nim_dev == NULL)
		goto ERROR1;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR1;
	user 	= priv->user;
	pdemod 	= priv->pDemod;
	if(user == NULL)
		goto ERROR1;
	debug_data		= &user->debug_data;
	tuner_control	= &user->tuner_control;
	if(!user->nim_init)//start monitor after hardware init
	{
		pr_warning("Nim is not initialized yet,It should be must init first\n");
		return count;
	}		
	if(NULL == tuner_control->nim_tuner_command)
		goto ERROR1;
	
	if (copy_from_user(tmp, buffer, count)) 
	{
		kfree(tmp);
		return -EFAULT;
	}
	pr_warning("Data entered:%s\n",tmp);
	switch(tmp[0])
	{
		case 'r':
			if(sscanf(tmp,"r %s 0x%x",buf,&reg_addr) != 2)
			{
				goto ERROR0;
			}
			debug_i2c.rw_flag = 0;
			debug_i2c.reg_addr = reg_addr;
			break;
		case 'w':
			if(sscanf(tmp,"w %s 0x%x 0x%x",buf,&reg_addr,&reg_data) != 3)
			{
				goto ERROR0;
			}
			debug_i2c.rw_flag = 1;
			debug_i2c.reg_addr = reg_addr;
			debug_i2c.reg_data = reg_data;
			break;
		default:
			goto ERROR0;
	}
	if(!strcmp(buf,"tuner"))
	{
		if(SUCCESS == tuner_control->nim_tuner_command(user->tuner_id,NIM_TUNER_RW_REG,(__u32)&debug_i2c))
		{
			if(0 == debug_i2c.rw_flag)
			{
				pr_warning("read ok:0x%x\n",debug_i2c.reg_data);
			}
			else if(1 == debug_i2c.rw_flag)
			{
				pr_warning("write ok\n");
			}
		}
		else
		{
			if(0 == debug_i2c.rw_flag)
			{
				pr_warning("read error\n");
			}
			else if(1 == debug_i2c.rw_flag)
			{
				pr_warning("write error\n");
			}
		}
	}
	else if(!strcmp(buf,"demod"))
	{
		if(0 == debug_i2c.rw_flag)
		{
			if (pdemod->pI2c->ReadRegister (pdemod->pI2c, pdemod->i2cAddressSLVX, reg_addr, (UINT8 *)&reg_data, 1) != SONY_RESULT_OK) 
			{
				pr_warning("read error\n");
        	}
			else
			{
				pr_warning("read ok:0x%x\n",reg_data);
			}
		}
		else if(1 == debug_i2c.rw_flag)
		{
			if (pdemod->pI2c->WriteOneRegister (pdemod->pI2c, pdemod->i2cAddressSLVX, reg_addr, reg_data) != SONY_RESULT_OK)
			{
            	pr_warning("write error\n");
        	}
			else
			{
				pr_warning("write ok\n");
			}
		}
		kfree(tmp);
		return count;
	}
	else
	{
		goto ERROR0;
	}
	kfree(tmp);
	return count;
ERROR0:
	pr_warning(" >>>>>>>>>>>>>>>>>>   USAGE   <<<<<<<<<<<<<<<<<<\n"
		"|read	tuner:echo r tuner 0xx[reg_addr] > i2c_debug\n"
		"|write	tuner:echo w tuner 0xx[reg_addr] 0xx[reg_data] > i2c_debug\n"
		"|read	demod:echo r demod 0xx[reg_addr] > i2c_debug\n"
		"|write	demod:echo w demod 0xx[reg_addr] 0xx[reg_data] > i2c_debug\n"
		" sample:\n"
		" read tuner reg:\n"
		" 	echo r tuner 0x18 > i2c_debug\n"
		" write tuner reg:\n"
		" 	echo w tuner 0x18 0x01 > i2c_debug\n"
		" read demod reg:\n"
		" 	echo r demod 0x10 > i2c_debug\n"
		" write demod reg:\n"
		" 	echo w demod 0x10 0x01 > i2c_debug\n\n"
		);
	kfree(tmp);
	return count;
ERROR1:
	kfree(tmp);
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}

ssize_t nim_cxd2856_proc_monitor_control_read(struct file * file, char __user *buffer, size_t count, loff_t *fops)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	struct nim_debug  	* debug_data =  NULL;
	//nim_dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = proc_get_parent_data(file->f_path.dentry->d_inode);

	if(nim_dev == NULL)
		goto ERROR;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR;
	user = priv->user;
	if(user == NULL)
		goto ERROR;
	debug_data	= &user->debug_data;

	pr_warning("\n  qam_%ld:monitor task %s;  monitor object = 0x%x;",
		user->demod_id,
		(debug_data->monitor_status == TASK_DEFAULT) ? "has not started":"is runing",debug_data->monitor_object);

	if((debug_data->monitor_object & 0x03) == 0x0)
	{
		pr_warning("\tper print has closed\n\n");
	}
	else
	{
		pr_warning("\tper print has started\n\n");
	}
	return 0;
ERROR:
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}

ssize_t nim_cxd2856_proc_monitor_control_write(struct file * file, const char __user *buffer, size_t count, loff_t *fops)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	struct nim_debug  	* debug_data =  NULL;
	int    monitor_object_tmp = 0;
	char * tmp = kzalloc((count+1), GFP_KERNEL);
	if (!tmp)
	{
		return -ENOMEM;
	}
	memset(tmp,0,count+1);
	//nim_dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = proc_get_parent_data(file->f_path.dentry->d_inode);
	
	if(nim_dev == NULL)
		goto ERROR;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR;
	user = priv->user;
	if(user == NULL)
		goto ERROR;
	debug_data	= &user->debug_data;
	
	if (copy_from_user(tmp, buffer, count-1)) 
	{
		kfree(tmp);
		return -EFAULT;
	}
	if(!user->nim_init)//start monitor after hardware init
	{
		pr_warning("Nim is not initialized yet,It should be must init first\n");
		goto EXIT;
	}
	
	if (sscanf(tmp, "0x%x",&monitor_object_tmp) != 1)
	{
		pr_warning("\n!!!must input: echo 0x** > monitor_debug\n\n"
		" >>>>>>>>>>>>       USAGE     <<<<<<<<<<<<\n"
		"| bit[0] 1: Always print per 0: close print\n"  
		"| bit[1] 1: print per when not 0 0: close print\n"
		"| !!! bit[0] and bit[1] can't be selected at the same time\n"
		"| bit[2] 1: print lock  0: close print\n" 
		"| bit[3] 1: print rf    0: close print\n"  
		"| bit[4] 1: print cn    0: close print\n" 
		"| bit[5] 1: print ssi   0: close print\n"  
		"| bit[6] 1: print sqi   0: close print\n"
		"| bit[7] 1: print ber   0: close print\n"
		"  sample:\n" 
		"  print lock:\n"
		"		echo 0x04 > monitor_debug\n"
		"  print lock && rf:\n"
		" 		echo 0x0c > monitor_debug\n\n"
		"  when \"echo 0x0 > monitor_debug\"  close print all object\n"
		"  when \"echo 0xff > monitor_debug\" print all object\n\n");
		return count;
	}
	
	pr_warning("\n monitor_object = 0x%x\n",monitor_object_tmp);
	if(0xff == monitor_object_tmp)//0xff is a special vallue,needs to be judged separately,keep the value
	{
		debug_data->monitor_object = monitor_object_tmp;
	}
	else if(0x03 == (monitor_object_tmp & 0x03))//Intercept this value:bit0 and bit1 don't be selected at the same time
	{
		pr_warning("| !!! bit[0] and bit[1] can't be selected at the same time\n");
		goto EXIT;
	}
	debug_data->monitor_object = monitor_object_tmp;

	if(0 == debug_data->monitor_object)//disable monitor
	{
		if(TASK_DEFAULT == debug_data->monitor_status)//determine whether task has quit
		{
			pr_warning("\nThere are no object to monitor\n"
				   "	cxd2856 monitor is disable\n");
			goto EXIT;
		}
		debug_data->monitor_status = TASK_DEFAULT;//stop task
		while(debug_data->monitor_status != TASK_EXIT)//wait task exit
		{
			msleep_interruptible(500);
		}
		cancel_work_sync(&nim_dev->debug_work);//remove task from work queue
		debug_data->monitor_status = TASK_DEFAULT;
		pr_warning("\ndisable cxd2856 monitor\n");
	}
	else // enable monitor
	{
		if(TASK_RUN == debug_data->monitor_status)//determine whether task is runing 
		{
			pr_warning("\n monitor is runing\n");
			goto EXIT;
		}
		debug_data->monitor_status = TASK_RUN;//enable monitor
		schedule_work(&nim_dev->debug_work);//Start scheduling task
	}
	
	kfree(tmp);
	return count;
EXIT:
	kfree(tmp);
	return count;
ERROR:
	kfree(tmp);
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}

ssize_t nim_cxd2856_proc_monitor_info_read(struct file * file, char __user *buffer, size_t count, loff_t *fops)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	struct nim_debug  	* debug_data =  NULL;
	int lock = 0,ssi = 0,sqi = 0,cn = 0,rf = 0,per = 0,ber = 0;
	unsigned long parg;
	
	//nim_dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = proc_get_parent_data(file->f_path.dentry->d_inode);
	if(nim_dev == NULL)
		goto ERROR;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR;
	user = priv->user;
	if(user == NULL)
		goto ERROR;
	debug_data	= &user->debug_data;
	if(!user->nim_init)//start monitor after hardware init
	{
		pr_err("!!!Nim is not initialized yet,It should be must init first\n");
		goto EXIT;
	}

	lock= ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_GET_LOCK_STATUS,parg);
	ssi	= ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_AGC,parg);
	sqi = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_SNR,parg);
	cn 	= ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_GET_CN_VALUE,parg);
	rf 	= ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_GET_RF_LEVEL,parg);	
	per = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_RSUB,parg);
	ber = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_QPSK_BER,parg);
	pr_warning("\n=>demod_%ld: lock = %d cn = %d rf = %d ssi = %d sqi = %d per = %d ber = %d\n",user->demod_id,lock,cn,rf,ssi,sqi,per,ber);
EXIT:
	return 0;
ERROR:
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}

ssize_t nim_cxd2856_proc_monitor_info_write(struct file * file, const char __user *buffer, size_t count, loff_t *fops)
{
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	struct nim_debug  	* debug_data =  NULL;
	int lock = 0,ssi = 0,sqi = 0,cn = 0,rf = 0,per = 0,ber = 0;
	unsigned long parg;
	
	char * tmp = kzalloc((count+1), GFP_KERNEL);
	if (!tmp)
	{
		return -ENOMEM;
	}
	memset(tmp,0,count);
	//nim_dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = proc_get_parent_data(file->f_path.dentry->d_inode);
	if(nim_dev == NULL)
		goto ERROR;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR;
	user = priv->user;
	if(user == NULL)
		goto ERROR;
	debug_data	= &user->debug_data;
	
	if (copy_from_user(tmp, buffer, count-1)) 
	{
		kfree(tmp);
		return -EFAULT;
	}
	if(!user->nim_init)//start monitor after hardware init
	{
		pr_warning("Nim is not initialized yet,It should be must init first\n");
		goto EXIT;
	}

	if(!strcmp("lock",tmp))
	{
		lock = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_GET_LOCK_STATUS,parg);
		pr_warning("lock = %d; demod %s\n",lock,(lock == 1) ? "locking":"unlocking");
	}
	else if(!strcmp("rf",tmp))
	{
		rf = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_GET_RF_LEVEL,parg);	
		pr_warning("rf = %d\n",rf);
	}
	else if(!strcmp("cn",tmp))
	{
		cn = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_GET_CN_VALUE,parg);
		pr_warning("cn = %d\n",cn);
	}
	else if(!strcmp("ssi",tmp))
	{
		ssi = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_AGC,parg);
		pr_warning("ssi = %d\n",ssi);
	}
	else if(!strcmp("sqi",tmp))
	{
		sqi = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_SNR,parg);
		pr_warning("sqi = %d\n",sqi);
	}
	else if(!strcmp("per",tmp))
	{
		per = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_RSUB,parg);
		pr_warning("per = %d\n",per);
	}
	else if(!strcmp("ber",tmp))
	{
		ber = ali_cxd2856_nim_ioctl_mutex(nim_dev,ALI_NIM_READ_QPSK_BER,parg);
		pr_warning("ber = %d\n",ber);
	}
	else
	{
		pr_warning("\n=>please input valid cmd: echo cmd > info_debug\n"
			   " >>>>>>>>>>>>       USAGE     <<<<<<<<<<<<\n"
			   "  echo cmd > info_debug\n"
			   "  cmd: \"lock\",\"cn\",\"rf\",\"ssi\",\"sqi\",\"per\",\"ber\"\n"
			   "  sample:\n"
			   "  check lock:\n"
			   "	echo lock > info_debug\n"
			   "  check rf:\n"
			   "	echo rf > info_debug\n\n");
	}
	kfree(tmp);
	return count;
EXIT:
	kfree(tmp);
	return count;
ERROR:
	kfree(tmp);
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}
ssize_t nim_cxd2856_proc_dev_info_read(struct file * file, char __user *buffer, size_t count, loff_t *fops)
{
	
	struct nim_device 	* nim_dev = NULL;
	sony_integ_t 		* priv = NULL;
    ALI_CXD2856_DATA_T 	* user = NULL;
	
	char 	buff_tmp[50] = {0};
	loff_t 	p = *fops;
	int 	len = 0;
	//dev = (struct nim_device *)PDE_DATA(file->f_path.dentry->d_inode);
	nim_dev = (struct nim_device *)proc_get_parent_data(file->f_path.dentry->d_inode);	
	if(nim_dev == NULL)
		goto ERROR;
	priv = nim_dev->priv;
	if(priv == NULL)
		goto ERROR;
	user = priv->user;
	
	if(p >= 50)
		return 0;
	if(count > (50-p))
	{
		count = 50 -p; 
	}
	
	len += sprintf(buff_tmp,"nim_name=%s,nim_id=%d,nim_type=T\n",nim_name[user->demod_id],user->nim_id);
	if (count == copy_to_user(buffer, buff_tmp + p, count))
	{
		return -EFAULT;
	}
	else
	{
		*fops += count;
	}
	return count;
ERROR:
	pr_err("!!!get data of opint fail");
	return -EFAULT;

}


static const struct file_operations proc_fops_monitor = {
//#warning Need to set some I/O handlers here
	.write		= nim_cxd2856_proc_monitor_control_write,
	.read		= nim_cxd2856_proc_monitor_control_read,
};
static const struct file_operations proc_fops_i2c = {
//#warning Need to set some I/O handlers here
	.write		= nim_cxd2856_proc_i2c_write,
	.read		= nim_cxd2856_proc_i2c_read,
};
static const struct file_operations proc_fops_info = {
//#warning Need to set some I/O handlers here
	.write		= nim_cxd2856_proc_monitor_info_write,
	.read		= nim_cxd2856_proc_monitor_info_read,
};
static const struct file_operations proc_fops_dev = {
//#warning Need to set some I/O handlers here
	.read		= nim_cxd2856_proc_dev_info_read,
};

int  nim_cxd2856_proc_init(struct platform_device * pdev)
{
	struct nim_device 	* nim_dev = platform_get_drvdata(pdev);
	sony_integ_t 		* priv = nim_dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	struct nim_debug  	* debug_data = &user->debug_data;
	if(user->demod_id > 1)
	{
		pr_err("beyond max dev id\n");
		return 0;
	}
	/* create a directory */
	debug_data->nim_dir = proc_mkdir_data(device_name[user->demod_id],0,NULL,nim_dev);
	//cxd2856_dir = proc_mkdir(device_name[dev_nu], NULL);
	if(NULL == debug_data->nim_dir)
	{
		pr_err("Error create /proc/%s/\n",device_name[user->demod_id]);
		return -1;
	}
	debug_data->monitor_control_file	= proc_create("monitor_debug", 0644, debug_data->nim_dir, &proc_fops_monitor);
	debug_data->monitor_info_file 		= proc_create("info_debug", 0644, debug_data->nim_dir, &proc_fops_info);
	debug_data->i2c_file 				= proc_create("i2c_debug", 0644, debug_data->nim_dir, &proc_fops_i2c);
	debug_data->dev_info_file			= proc_create("dev_info", 0644, debug_data->nim_dir, &proc_fops_dev);
	/*monitor_file = proc_create_data("debug_monitor", 0644, cxd2856_dir, &proc_fops_monitor,nim_dev);*/
	if(debug_data->monitor_control_file	== NULL ||
	   debug_data->monitor_info_file	== NULL || 
	   debug_data->i2c_file				== NULL ||
	   debug_data->dev_info_file 		== NULL) 
	{
		proc_remove(debug_data->nim_dir);
		pr_err("Error create /proc/cxd2856_%ld fail\n",user->demod_id);
		return -1;
	}
	INIT_WORK(&nim_dev->debug_work,nim_cxd2856_proc_monitor_task);
	debug_data->monitor_status = TASK_DEFAULT;
	return 0;
}
int  nim_cxd2856_proc_exit(struct platform_device * pdev)
{
	struct nim_device 	* nim_dev = platform_get_drvdata(pdev);
	sony_integ_t 		* priv = nim_dev->priv;
	ALI_CXD2856_DATA_T 	* user = priv->user;
	struct nim_debug  	* debug_data = &user->debug_data;
	
	proc_remove(debug_data->nim_dir);
	proc_remove(debug_data->i2c_file);
	proc_remove(debug_data->monitor_info_file);
	proc_remove(debug_data->monitor_control_file);
	proc_remove(debug_data->dev_info_file);
	if(TASK_RUN == debug_data->monitor_status)//determine whether task has quit)
	{
		debug_data->monitor_status = TASK_DEFAULT;//stop task
		while(debug_data->monitor_status != TASK_EXIT)//wait task exit
		{
			msleep(100);
		}
		cancel_work_sync(&nim_dev->debug_work);//remove task from work queue
	}
	return 0;
}

