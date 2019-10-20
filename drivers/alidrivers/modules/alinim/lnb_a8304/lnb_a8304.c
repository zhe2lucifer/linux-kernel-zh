/*****************************************************************************
*    Copyright (C)2017 Ali Corporation. All Rights Reserved.
*
*    File:    lnb_a8304.c
*
*    Description: a8304 lnb driver
*    History:
*                   Date                      Athor            Version                 Reason
*        ============       =========     ======     ================
*    1.     2017-02-20          	  Leo.liu           Ver 1.0                Create file.
*   
*****************************************************************************/


/***************    control reg bit0 -bit3  ******************
VLES3 VLES2 VLES1  VLES0 
bit3    bit2    bit1     bit0      value        LNB output voltage(uint :V)
  0       0        1         0         0x2                13.333
  0       0        1         1         0x3                13.667
  0       1        0         1         0x5                14.333
  0       1        1         1         0x7                15.667
  1       0        1         1         0xb                18.667
  1       1        0         0         0xc                 19.000
  1       1        0         1         0xd                19.333
  1       1        1         0         0xe                19.667
*********************************************************/
#include "lnb_a8304.h"
#include "../lnb_common.h"
#include "../porting_linux_header.h"
#include <linux/i2c.h>

#define LNB_MAX_NU 2
#define LNB_OUTPUT_ENABLE   (1<<4)      //enable lnb output
#define LNB_OUTPUT_DISABLE   (~(1<<4))  //disable lnb output
#define LNB_TONE_GATE_ON    (1<<5) 		//22K square wave on
#define LNB_TONE_GATE_OFF   (~(1<<5))   //22K square wave off
 

struct LNB_A8304_CONTROL a8304_device[LNB_MAX_NU]= {{0}}; //lnb device
static UINT32 lnb_count = 0;

static INT32 lnb_i2c_write(UINT32 lnb_id,UINT8 chip_addr,UINT8 reg_addr,UINT8 *data,UINT8 wlen)
{
	
	INT32 ret = ERR_FAILURE,result = 0;
	struct LNB_A8304_CONTROL * lnb_device = &a8304_device[lnb_id];
	UINT8 buffer[8]={0};
	UINT8 i = 0;
	struct i2c_adapter *adapter;
    struct i2c_msg msgs = { .addr = chip_addr>>1, .flags = 0, .len = wlen+1,  .buf = buffer };
	if(NULL == data || NULL == lnb_device)
	{
		lnb_printk("%s %d transfer point is NULL\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	
	if((0 == wlen) || (wlen > 7))
	{
		lnb_printk("%s %d w/r len is unbefitting\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	buffer[0] = reg_addr;
    for (i = 0; i < wlen; i++)
    {
        buffer[i + 1] = data[i];
    }
	lnb_printk("Write buffer 0 = 0x%2x, buffer 1 = 0x%2x\n",buffer[0],buffer[1]);
	mutex_lock(&lnb_device->i2c_mutex);
	adapter = i2c_get_adapter(lnb_device->i2c_type_id);
	if(adapter)
	{						
		if((result = i2c_transfer(adapter, &msgs, 1)) != 1)
		{
			lnb_printk("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
			__FUNCTION__, __LINE__, result, adapter->name, adapter->nr, adapter->nr, adapter->class);
		    ret = (u32)ERR_FAILURE;
		}    
		else
		{
			ret = (u32)SUCCESS;
		}
	}
	else
	{
		lnb_printk("[ %s %d ], adapter is NULL, i2c_id = %d\n", __FUNCTION__, __LINE__, lnb_device->i2c_type_id);
		ret = (u32)ERR_FAILURE;
	}	
	mutex_unlock(&lnb_device->i2c_mutex);
	
	return ret;
	
}

static INT32 lnb_i2c_read(UINT32 lnb_id,UINT8 chip_addr,UINT8 reg_addr,UINT8 *data,UINT8 rlen)
{
	
	UINT32 ret = (u32)ERR_FAILURE;
	struct LNB_A8304_CONTROL * lnb_device = &a8304_device[lnb_id];
	UINT8 offset = reg_addr;
	struct i2c_adapter *adapter;
	#if 0
    struct i2c_msg msgs[] = {   { .addr = chip_addr>>1, .flags = 0, .len = 0, .buf = &offset },
                                { .addr = chip_addr>>1, .flags	= I2C_M_RD,  .len = rlen,  .buf = data    } };
	#else
	//struct i2c_msg wmsgs = { .addr = chip_addr>>1, .flags = 0, .len = 0,  .buf = &offset };
	struct i2c_msg rmsgs = { .addr = chip_addr>>1, .flags = I2C_M_RD, .len = rlen,  .buf = data };
	#endif
	int result = 0;	

	if(NULL == data || NULL == lnb_device || 0 == rlen)
	{
		lnb_printk("%s %d transfer point is NULL or w/r len is unbefitting\n",__FUNCTION__,__LINE__);
		return ERR_FAILURE;
	}

	
	mutex_lock(&lnb_device->i2c_mutex);
	adapter = i2c_get_adapter(lnb_device->i2c_type_id);
	if(adapter)
	{		
		#if 0
		if((result = i2c_transfer(adapter, msgs, 2)) != 2)
		{
			lnb_printk("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
			__FUNCTION__, __LINE__, result, adapter->name, adapter->nr, adapter->nr, adapter->class);
		    ret = (u32)ERR_FAILURE;
		}    
		else
		{
			ret = (u32)SUCCESS;
		}
		#else
		if(ali_i2c_write(lnb_device->i2c_type_id,chip_addr,&offset,1)< 0)
		{
			lnb_printk("%s %d i2c read error\n",__FUNCTION__,__LINE__);
			ret = (u32)ERR_FAILURE;
		}
		else
		{
			if((result = i2c_transfer(adapter, &rmsgs, 1)) != 1)
			{
				lnb_printk("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
				__FUNCTION__, __LINE__, result, adapter->name, adapter->nr, adapter->nr, adapter->class);
		    	ret = (u32)ERR_FAILURE;
			}    
			else
			{
				ret = (u32)SUCCESS;
			}
		}
		#endif
	}
	else
	{
		lnb_printk("[ %s %d ], adapter is NULL, i2c_id = %d\n", __FUNCTION__, __LINE__, lnb_device->i2c_type_id);
		ret = (u32)ERR_FAILURE;
	}
	lnb_printk("Read offset = 0x%x,data[0] = 0x%x\n",offset,data[0]);
	mutex_unlock(&lnb_device->i2c_mutex);
	return ret;
}


/*static INT32 lnb_a8304_disable_output(UINT32 lnb_id)
{
	struct LNB_A8304_CONTROL * lnb_device = &a8304_device[lnb_id];
	UINT8 = data;
	if(FALSE == lnb_device->init_flag)//whether lnb_device has been initialized
	{
		lnb_printk("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,lnb_id);	
		return FAIL;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	data = LNB_OUTPUT_DISABLE;
	if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1) == 0)
	{
		lnb_printk("%s %d i2c write error\n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return FAIL;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}*/

static INT32 lnb_a8304_get_ocp(UINT32 lnb_id)
{
	struct LNB_A8304_CONTROL * lnb_device = &a8304_device[lnb_id];
	UINT8 data = 0x0;
	if(FALSE == lnb_device->init_flag)//whether lnb_device has been initialized
	{
		lnb_printk("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,lnb_id);	
		return ERR_FAILURE;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	if(lnb_i2c_read(lnb_id,lnb_device->i2c_base_addr,STATUS_REG,&data,1) != 0)
	{
		lnb_printk("%s %d i2c read error\n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return ERR_FAILURE;
	}
	if (LNB_A8304_REG_OCP == (data & LNB_A8304_REG_OCP))
	{
		lnb_printk("A8304 overcurrent happen \n");
		mutex_unlock(&lnb_device->lnb_mutex);
		return OCP_HAPPEN;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}
static INT32 lnb_a8304_set_pol(UINT32 lnb_id,UINT8 polirity)
{
	struct LNB_A8304_CONTROL * lnb_device = &a8304_device[lnb_id];
	UINT8 data = 0x0;
	if(FALSE == lnb_device->init_flag)//whether lnb_device has been initialized
	{
		lnb_printk("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,lnb_id);	
		return ERR_FAILURE;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	if(polirity == NIM_PORLAR_HORIZONTAL) //horizontal polarity
	{
		data = LNB_OUTPUT_ENABLE | 0x02 |LNB_TONE_GATE_ON;
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1) != 0)
		{
			lnb_printk("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else if(polirity == NIM_PORLAR_VERTICAL) //vertical polarity
	{
		data = LNB_OUTPUT_ENABLE |LNB_TONE_GATE_ON |0x0d;
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1) != 0)
		{
			lnb_printk("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else
	{
		lnb_printk("%s %d don't support this polarity\n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return ERR_FAILURE;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}

static INT32 lnb_a8304_power_control(UINT32 lnb_id , UINT8 on_off)
{
	struct LNB_A8304_CONTROL * lnb_device = &a8304_device[lnb_id];
	UINT8 data = 0x0;
	if(FALSE == lnb_device->init_flag)
	{
		lnb_printk("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,lnb_id);	
		return ERR_FAILURE;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	if(on_off == POWER_ON) 
	{
		data = LNB_OUTPUT_ENABLE | LNB_TONE_GATE_ON;
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1) != 0)
		{
			lnb_printk("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else if(on_off == POWER_OFF) 
	{
		data = 0x00;
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1) != 0)
		{
			lnb_printk("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else
	{
		lnb_printk("%s %d don't support the value of on_off \n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return ERR_FAILURE;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}
static INT32 lnb_a8304_init(UINT32 *lnb_id,struct EXT_LNB_CTRL_CONFIG *ext_lnb_config)
{
	struct LNB_A8304_CONTROL * lnb_device = NULL;
	UINT8 data = 0xff;
	
	#ifdef RD_LNB_DEBUG
	UINT8 i = 0;
	UINT32 tmp = 0;
	#endif
	if(NULL == ext_lnb_config)
	{
		lnb_printk("%s %d transfer point is NULL\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	*lnb_id = lnb_count;
	lnb_device = &a8304_device[lnb_count];
	lnb_count++;
	
	mutex_init(&lnb_device->lnb_mutex);
	mutex_init(&lnb_device->i2c_mutex);
	lnb_device->i2c_base_addr = ext_lnb_config->i2c_base_addr;
	lnb_device->i2c_type_id   = ext_lnb_config->i2c_type_id;
	lnb_device->gpio_num	  = ext_lnb_config->int_gpio_num;
	lnb_device->gpio_polar    = ext_lnb_config->int_gpio_polar;
	lnb_device->current_polar = NIM_PORLAR_HORIZONTAL;
	lnb_device->init_flag 	  = TRUE;
	
	//lnb_device->power_en        = ;
	//lnb_device->gpio_en           = ext_lnb_config->int_gpio_en;

	lnb_printk(
		"lnb_id         %d\n"
		"i2c_base_addr  %x\n"
		"i2c_type_id    %d\n" 
		"gpio_num	    %d\n"
		"gpio_polar     %d\n"
		"current_polar  %d\n",
		*lnb_id,
		lnb_device->i2c_base_addr,lnb_device->i2c_type_id,lnb_device->gpio_num,
		lnb_device->gpio_polar,lnb_device->current_polar);
	if(lnb_i2c_read(*lnb_id,lnb_device->i2c_base_addr,STATUS_REG,&data,1) != 0)
	{
		lnb_printk("%s %d i2c read error\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	else
	{
		/*if((data & 0x01) == 0x01 || data == 0xff)
		{
			lnb_printk("%s %d i2c read error\n",__FUNCTION__,__LINE__);
			return ERR_FAILURE;
		}*/
		data = LNB_OUTPUT_ENABLE|LNB_TONE_GATE_ON;
		if(lnb_i2c_write(*lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1) != 0)
		{
			lnb_printk("%s %d i2c write error\n",__FUNCTION__,__LINE__);	
			return ERR_FAILURE;
		}
		
	}

	#ifdef RD_LNB_DEBUG
	tmp = *lnb_id;

	for(i = 0;i < 10;i++)
	{
		lnb_printk("*********************Start set_pol times :%d******************\n",i);
		//lnb_a8304_set_pol(tmp,NIM_PORLAR_HORIZONTAL);
		lnb_a8304_power_control(tmp,POWER_ON);
		mdelay(1000);
		//lnb_a8304_set_pol(tmp,NIM_PORLAR_VERTICAL);
		//mdelay(500);
		//lnb_printk("**********OCP status %d\n",lnb_a8304_get_ocp(tmp));
		lnb_a8304_power_control(tmp,POWER_OFF);
		mdelay(1000);
		//lnb_printk("**********LNB_TONE_GATE_ON \n");
		/*
		data = LNB_OUTPUT_DISABLE;
		lnb_i2c_write(*lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1);
		mdelay(1000);

		lnb_i2c_read(*lnb_id,lnb_device->i2c_base_addr,STATUS_REG,&data,1);
		mdelay(1000);


		data = LNB_OUTPUT_ENABLE|0x0d;
		lnb_i2c_write(*lnb_id,lnb_device->i2c_base_addr,CONTROL_REG,&data,1);
		mdelay(1000);

		lnb_i2c_read(*lnb_id,lnb_device->i2c_base_addr,STATUS_REG,&data,1);
		mdelay(1000);
		*/
	}
	#endif
	return SUCCESS;
}
static INT32 lnb_a8304_release(UINT32 lnb_id)
{
	struct LNB_A8304_CONTROL * lnb_device = NULL;
	lnb_device = &a8304_device[lnb_id];

	lnb_device->i2c_base_addr = 0;
	lnb_device->i2c_type_id   = 0;
	lnb_device->gpio_num	  = 0;
	lnb_device->gpio_polar    = 0;
	lnb_device->current_polar = 0;
	lnb_device->init_flag 	  = FALSE;
	mutex_destroy(&lnb_device->lnb_mutex);
	mutex_destroy(&lnb_device->i2c_mutex);
	if(lnb_count >0)
		lnb_count--;
	else
		lnb_count = 0;
	lnb_printk("[%s %d] lnb_id %d release!!lnb_count = %d\n",__FUNCTION__,__LINE__,lnb_id,lnb_count);
	return SUCCESS;
}
INT32 lnb_a8304_command(UINT32 *lnb_id,UINT32 cmd,UINT32 param)
{
	INT32 result = SUCCESS;
	UINT32 lnb_id_tmp = 0;
	lnb_id_tmp = *lnb_id;
	
	if(lnb_count > LNB_MAX_NU)
	{
		lnb_printk("%s %d LNB nu beyong max support nu\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	switch(cmd)
	{
		case LNB_CMD_ALLOC_ID:
			break;
		case LNB_CMD_INIT_CHIP:
			//need to back lnd_id to caller
			result = lnb_a8304_init(lnb_id,(struct EXT_LNB_CTRL_CONFIG *)param);
			break;
		case LNB_CMD_SET_POLAR:
			result = lnb_a8304_set_pol(lnb_id_tmp,(UINT8)param);
			break;
		case LNB_CMD_POWER_ONOFF:
			result = lnb_a8304_power_control(lnb_id_tmp,(UINT8)param);
			break;
		case LNB_CMD_RELEASE_CHIP:
			result = lnb_a8304_release(lnb_id_tmp);
			break;
		case LNB_CMD_GET_OCP:
			result = lnb_a8304_get_ocp(lnb_id_tmp);
			break;
		//case LNB_CMD_POWER_OFF:
		default:
			break;
	}
	return result;
}

