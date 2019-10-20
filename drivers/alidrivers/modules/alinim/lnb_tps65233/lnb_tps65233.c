/*****************************************************************************
*    Copyright (C)2016 Ali Corporation. All Rights Reserved.
*
*    File:    tps65233.c
*
*    Description: tps65233 lnb driver
*    History:
*                   Date                      Athor            Version                 Reason
*        ============       =========     ======     ================
*    1.     2016-03-13          	Robin gan        Ver 1.0                Create file.
*   
*****************************************************************************/

#include "../lnb_common.h"
#include "../porting_linux_header.h"
#include <linux/i2c.h>
#include "lnb_tps65233.h"

/*******************************/
/*VSEL2    VSEL1    VSEL0    LNB (V) */
/*0             0         0           13       */
/*0             0         1          13.4     */
/*0             1         0          13.8     */
/*0             1         1          14.2     */
/*1             0         0           18      */
/*1             0         1          18.6    */
/*1             1         0          19.2    */
/*1             1         1          19.8    */
/******************************/
#define LNB_H_VALUE	0x6
#define LNB_V_VALUE	0x2

/*******************************/
/*CL1    CL0    Current Limit (mA) */
/*0       0         300      */
/*0       1         500      */
/*1       0         700	     */
/*1       1         1000     */
/******************************/

#define LNB_CURRENT_LIMIT  0x00

#define LNB_I2C_ENABLE		(1<<7)		//I2C control enable
#define LNB_I2C_DISABLE		(~(1<<7))	//I2C control disable

#define LNB_TONE_GATE_ON	(1<<5)		//tone gate on
#define LNB_TONE_GATE_OFF	(~(1<<5))	//tone gate off

#define LNB_OUTPUT_ENABLE	(1<<3)		//lnb output enable
#define LNB_OUTPUT_DISABLE	(~(1<<3))	//lnb output disable

#define LNB_CURRENT_LIMIT_SELF  0xFE    //0:current limit set by register
#define LNB_CURRENT_LIMIT_EXTERNAL 0x01 //1: current limit set by external resistor


UINT32 lnb_tps65233_cnt = 0;



struct LNB_TPS65233_CONTROL lnb_tps65233_array[MAX_LNB_TPS65233] = {{0}};

static INT32 lnb_i2c_write(UINT32 lnb_id,UINT8 chip_addr,UINT8 reg_addr,UINT8 *data,UINT8 wlen)
{
	
	INT32 ret = ERR_FAILURE,result = 0;
	struct LNB_TPS65233_CONTROL * lnb_device = &lnb_tps65233_array[lnb_id];
	UINT8 buffer[8]={0};
	UINT8 i = 0;
	struct i2c_adapter *adapter;
    struct i2c_msg msgs = { .addr = chip_addr>>1, .flags = 0, .len = wlen+1,  .buf = buffer };
	if(NULL == data || NULL == lnb_device)
	{
		LNB_PRINTF("%s %d transfer point is NULL\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	
	if((0 == wlen) || (wlen > 7))
	{
		LNB_PRINTF("%s %d w/r len is unbefitting\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	buffer[0] = reg_addr;
    for (i = 0; i < wlen; i++)
    {
        buffer[i + 1] = data[i];
    }
	LNB_PRINTF("Write buffer 0 = 0x%2x, buffer 1 = 0x%2x\n",buffer[0],buffer[1]);
	mutex_lock(&lnb_device->i2c_mutex);
	adapter = i2c_get_adapter(lnb_device->i2c_type_id);
	if(adapter)
	{						
		if((result = i2c_transfer(adapter, &msgs, 1)) != 1)
		{
			LNB_PRINTF("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
			__FUNCTION__, __LINE__, (int)result, adapter->name, (int)adapter->nr, (int)adapter->nr, adapter->class);
		    ret = (u32)ERR_FAILURE;
		}    
		else
		{
			ret = (u32)SUCCESS;
		}
	}
	else
	{
		LNB_PRINTF("[ %s %d ], adapter is NULL, i2c_id = %d\n", __FUNCTION__, __LINE__, (int)lnb_device->i2c_type_id);
		ret = (u32)ERR_FAILURE;
	}	
	mutex_unlock(&lnb_device->i2c_mutex);
	
	return ret;
	
}

static INT32 lnb_i2c_read(UINT32 lnb_id,UINT8 chip_addr,UINT8 reg_addr,UINT8 *data,UINT8 rlen)
{
	
	UINT32 ret = (u32)ERR_FAILURE;
	struct LNB_TPS65233_CONTROL * lnb_device= &lnb_tps65233_array[lnb_id];
	UINT8 offset = reg_addr;
	struct i2c_adapter *adapter;
	#if 1
    struct i2c_msg msgs[] = {   { .addr = chip_addr>>1, .flags = 0, .len = 0, .buf = &offset },
                                { .addr = chip_addr>>1, .flags	= I2C_M_RD,  .len = rlen,  .buf = data    } };
	#else
	//struct i2c_msg wmsgs = { .addr = chip_addr>>1, .flags = 0, .len = 0,  .buf = &offset };
	struct i2c_msg rmsgs = { .addr = chip_addr>>1, .flags = I2C_M_RD, .len = rlen,  .buf = data };
	#endif
	int result = 0;	

	if(NULL == data || NULL == lnb_device || 0 == rlen)
	{
		LNB_PRINTF("%s %d transfer point is NULL or w/r len is unbefitting\n",__FUNCTION__,__LINE__);
		return ERR_FAILURE;
	}

	
	mutex_lock(&lnb_device->i2c_mutex);
	adapter = i2c_get_adapter(lnb_device->i2c_type_id);
	if(adapter)
	{		
		#if 1
		if((result = i2c_transfer(adapter, msgs, 2)) != 2)
		{
			LNB_PRINTF("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
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
			LNB_PRINTF("%s %d i2c read error\n",__FUNCTION__,__LINE__);
			ret = (u32)ERR_FAILURE;
		}
		else
		{
			if((result = i2c_transfer(adapter, &rmsgs, 1)) != 1)
			{
				LNB_PRINTF("[ %s %d ], i2c_transfer fail, result = %d, name = %s, id = %d, nr = %d, class = 0x%08x\n", 
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
		LNB_PRINTF("[ %s %d ], adapter is NULL, i2c_id = %d\n", __FUNCTION__, __LINE__, (int)lnb_device->i2c_type_id);
		ret = (u32)ERR_FAILURE;
	}
	LNB_PRINTF("Read offset = 0x%x,data[0] = 0x%x\n",offset,data[0]);
	mutex_unlock(&lnb_device->i2c_mutex);
	return ret;
}

INT32 lnb_tps65233_set_pol(UINT32 id, UINT8 param)
{
	INT32 result;
	UINT8 byte;

	result = lnb_i2c_read(id, lnb_tps65233_array[id].i2c_base_addr, REG_CONTROL_1, &byte, 1);
	if (result != SUCCESS)
	{
		LNB_PRINTF("[%s %d], lnb_i2c_read error, result=%d\n", __FUNCTION__, __LINE__, (int)result);
		return result;
	}

	LNB_PRINTF("[%s %d], id = %d, before set reg value=0x%x\n", __FUNCTION__, __LINE__, (int)id, byte);
	
	if(NIM_PORLAR_HORIZONTAL==param)
	{
		byte = (byte&0xF8) | LNB_H_VALUE;
	}
    else
    {
		byte = (byte&0xF8) | LNB_V_VALUE;
    }
	
	if ((result = lnb_i2c_write(id, lnb_tps65233_array[id].i2c_base_addr, REG_CONTROL_1, &byte, 1)) != SUCCESS) 
	{
		LNB_PRINTF("[%s %d], lnb_i2c_write error, result=%d\n", __FUNCTION__, __LINE__, (int)result);
		return result;
	}	

	LNB_PRINTF("[%s %d], after set reg value=0x%x\n", __FUNCTION__, __LINE__, byte);
	
	return SUCCESS;
}

static INT32 lnb_tps65233_get_ocp(UINT32 lnb_id)
{
	struct LNB_TPS65233_CONTROL * lnb_device = &lnb_tps65233_array[lnb_id];
	UINT8 data = 0x0;
	if(FALSE == lnb_device->init_flag)//whether lnb_device has been initialized
	{
		LNB_PRINTF("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,(int)lnb_id);	
		return ERR_FAILURE;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	if(lnb_i2c_read(lnb_id,lnb_device->i2c_base_addr,REG_STATUS,&data,1) != 0)
	{
		LNB_PRINTF("%s %d i2c read error\n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return ERR_FAILURE;
	}
	LNB_PRINTF("******OCP  data:0x%2x \n",data);
	if (0x04 == (data & 0x04)) //bit2 OCP:1: over current protection triggered.
	{
		LNB_PRINTF("tps65233 overcurrent happen \n");
		mutex_unlock(&lnb_device->lnb_mutex);
		return OCP_HAPPEN;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}

static INT32 lnb_tps65233_current_limit_control(UINT32 lnb_id , UINT8 on_off)
{
	struct LNB_TPS65233_CONTROL * lnb_device = &lnb_tps65233_array[lnb_id];
	UINT8 data = 0x0;
	INT32 result;
	if(FALSE == lnb_device->init_flag)
	{
		LNB_PRINTF("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,(int)lnb_id);	
		return ERR_FAILURE;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	result = lnb_i2c_read(lnb_id, lnb_device->i2c_base_addr, REG_CONTROL_2, &data, 1);
	if (result != SUCCESS)
	{
		LNB_PRINTF("[%s %d], lnb_i2c_read error, result=%d\n", __FUNCTION__, __LINE__, (int)result);
		return result;
	}
	if(CURRENT_LIMIT_ON == on_off)
	{
		data = (data & LNB_CURRENT_LIMIT_SELF)| (LNB_CURRENT_LIMIT << 1);
		LNB_PRINTF("******CURRENT_LIMIT_ON  data:0x%2x \n",data);
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,REG_CONTROL_2,&data,1) != 0)
		{
			LNB_PRINTF("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else if (CURRENT_LIMIT_OFF == on_off)
	{
		data = (data | LNB_CURRENT_LIMIT_EXTERNAL) & (~(LNB_CURRENT_LIMIT << 1));
		LNB_PRINTF("******CURRENT_LIMIT_OFF  data:0x%2x \n",data);
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,REG_CONTROL_2,&data,1) != 0)
		{
			LNB_PRINTF("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else
	{
		LNB_PRINTF("%s %d don't support the value of on_off \n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return ERR_FAILURE;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}
static INT32 lnb_tps65233_power_control(UINT32 lnb_id , UINT8 on_off)
{
	struct LNB_TPS65233_CONTROL * lnb_device = &lnb_tps65233_array[lnb_id];
	UINT8 data = 0x0;
	if(FALSE == lnb_device->init_flag)
	{
		LNB_PRINTF("%s %d lnb[%d] device has not been initialized\n",__FUNCTION__,__LINE__,(int)lnb_id);	
		return ERR_FAILURE;
	}
	mutex_lock(&lnb_device->lnb_mutex);
	if(on_off == POWER_ON) 
	{
		data = LNB_I2C_ENABLE | LNB_TONE_GATE_ON | LNB_OUTPUT_ENABLE;
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,REG_CONTROL_1,&data,1) != 0)
		{
			LNB_PRINTF("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else if(on_off == POWER_OFF) 
	{
		data = 0x00;
		if(lnb_i2c_write(lnb_id,lnb_device->i2c_base_addr,REG_CONTROL_1,&data,1) != 0)
		{
			LNB_PRINTF("%s %d i2c write error\n",__FUNCTION__,__LINE__);
			mutex_unlock(&lnb_device->lnb_mutex);
			return ERR_FAILURE;
		}
	}
	else
	{
		LNB_PRINTF("%s %d don't support the value of on_off \n",__FUNCTION__,__LINE__);
		mutex_unlock(&lnb_device->lnb_mutex);
		return ERR_FAILURE;
	}
	mutex_unlock(&lnb_device->lnb_mutex);
	return SUCCESS;
}

INT32 lnb_tps65233_init(UINT32 *lnb_id,struct EXT_LNB_CTRL_CONFIG *ext_lnb_config)
{
	struct LNB_TPS65233_CONTROL * lnb_device = NULL;
	UINT8 data = 0xff;
	
	#ifdef RD_LNB_DEBUG
	UINT8 i = 0;
	UINT32 tmp = 0;
	#endif
	if(NULL == ext_lnb_config)
	{
		LNB_PRINTF("%s %d transfer point is NULL\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	*lnb_id = lnb_tps65233_cnt;
	lnb_device = &lnb_tps65233_array[lnb_tps65233_cnt];
	lnb_tps65233_cnt++;
	
	mutex_init(&lnb_device->lnb_mutex);
	mutex_init(&lnb_device->i2c_mutex);
	lnb_device->i2c_base_addr = ext_lnb_config->i2c_base_addr;
	lnb_device->i2c_type_id   = ext_lnb_config->i2c_type_id;
	lnb_device->gpio_num	  = ext_lnb_config->int_gpio_num;
	lnb_device->gpio_polar    = ext_lnb_config->int_gpio_polar;
	lnb_device->current_polar = NIM_PORLAR_HORIZONTAL;
	lnb_device->init_flag 	  = TRUE;
	

	LNB_PRINTF(
		"lnb_id         %d\n"
		"i2c_base_addr  0x%x\n"
		"i2c_type_id    %d\n" 
		"gpio_num	    %d\n"
		"gpio_polar     %d\n"
		"current_polar  %d\n",
		(int)*lnb_id,
		(int)lnb_device->i2c_base_addr,(int)lnb_device->i2c_type_id,lnb_device->gpio_num,
		lnb_device->gpio_polar,lnb_device->current_polar);
	if(lnb_i2c_read(*lnb_id,lnb_device->i2c_base_addr,REG_CONTROL_1,&data,1) != 0)
	{
		LNB_PRINTF("%s %d i2c read error\n",__FUNCTION__,__LINE__);	
		return ERR_FAILURE;
	}
	else
	{
		data = data | LNB_I2C_ENABLE | LNB_TONE_GATE_ON | LNB_OUTPUT_ENABLE;//enable I2C control;
		if(lnb_i2c_write(*lnb_id,lnb_device->i2c_base_addr,REG_CONTROL_1,&data,1) != 0)
		{
			LNB_PRINTF("%s %d i2c write error\n",__FUNCTION__,__LINE__);	
			return ERR_FAILURE;
		}
		
	}
	lnb_tps65233_current_limit_control(*lnb_id,CURRENT_LIMIT_ON);
	#ifdef RD_LNB_DEBUG
	tmp = *lnb_id;
	
	for(i = 0;i < 10;i++)
	{
		LNB_PRINTF("*********************times :%d******************\n",i);
		//lnb_tps65233_set_pol(tmp,NIM_PORLAR_HORIZONTAL);
		//lnb_tps65233_power_control(tmp,POWER_ON);
		//lnb_tps65233_current_limit_control(tmp,CURRENT_LIMIT_ON);
		//mdelay(1000);
		//lnb_tps65233_set_pol(tmp,NIM_PORLAR_VERTICAL);
		mdelay(10000);
		lnb_tps65233_get_ocp(tmp);
		//lnb_printk("**********OCP status %d\n",lnb_a8304_get_ocp(tmp));
		//lnb_tps65233_power_control(tmp,POWER_OFF);
		//mdelay(2000);
		//lnb_tps65233_current_limit_control(tmp,CURRENT_LIMIT_OFF);
		
	}
	#endif
	return SUCCESS;
}
#if 0
INT32 lnb_tps65233_init(UINT32 id)
{
    INT32 result = SUCCESS;
	UINT8 byte;

	LNB_PRINTF("[%s %d], id = %d, Enter!\n", __FUNCTION__, __LINE__, id);
	result = lnb_i2c_read(id, lnb_tps65233_array[id].i2c_base_addr, REG_CONTROL_1, &byte, 1);
	if (result != SUCCESS)
	{
		LNB_PRINTF("[%s %d], lnb_i2c_read error, result=%d\n", __FUNCTION__, __LINE__, result);
		return result;
	}
	
	byte = byte | LNB_I2C_ENABLE | LNB_TONE_GATE_ON | LNB_OUTPUT_ENABLE;//enable I2C control	
	if ((result = lnb_i2c_write(id, lnb_tps65233_array[id].i2c_base_addr, REG_CONTROL_1, &byte, 1)) != SUCCESS) 
	{
		LNB_PRINTF("[%s %d], lnb_i2c_write error, result=%d\n", __FUNCTION__, __LINE__, result);
		return result;
	}	
	
    if(lnb_tps65233_array[id].mutex_id == OSAL_INVALID_ID)
    {
        lnb_tps65233_array[id].mutex_id = osal_mutex_create();        
    }   

	LNB_PRINTF("[%s %d], Leave!\n", __FUNCTION__, __LINE__);
    return result;
}

INT32 lnb_tps65233_power_en(UINT32 id)
{
    INT32 result = SUCCESS;
	//todo
    return result;
}


INT32 lnb_tps65233_power_off(UINT32 id)
{
    INT32 result = SUCCESS;
	UINT8 byte;
	UINT32 addr;
	
	LNB_PRINTF("[%s %d], id = %d, Enter! \n", __FUNCTION__, __LINE__, id);

	//before call this function, EN pin has been pull low by lnb_ant_power_off function in power.c
	//so we must change the I2C addr according to spec.
	//EN=0, I2C addr=0x60; EN=1, I2C addr=0x61
	addr = lnb_tps65233_array[id].i2c_base_addr - 2;
	
	byte = 0x00;	
	if ((result = lnb_i2c_write(id, addr, REG_CONTROL_1, &byte, 1)) != SUCCESS) 
	{
		LNB_PRINTF("[%s %d], lnb_i2c_write error, id = %d, result=%d\n", __FUNCTION__, __LINE__, id, result);
		return result;
	}	

	LNB_PRINTF("[%s %d], Leave!\n", __FUNCTION__, __LINE__);	
    return result;
}
#endif

INT32 lnb_tps65233_command(UINT32 *lnb_id, UINT32 cmd, UINT32 param)
{
    INT32 result = SUCCESS;
    UINT32 lnb_id_tmp = 0;
	lnb_id_tmp = *lnb_id;
    switch(cmd)
    { 
    	case LNB_CMD_ALLOC_ID:
            break;
			
        case LNB_CMD_INIT_CHIP:
            result = lnb_tps65233_init(lnb_id,(struct EXT_LNB_CTRL_CONFIG *)param);
            break;
			
        case LNB_CMD_SET_POLAR:
            result = lnb_tps65233_set_pol(lnb_id_tmp, (UINT8)param);
            break;

		case LNB_CMD_POWER_ONOFF:
			result = lnb_tps65233_power_control(lnb_id_tmp,(UINT8)param);
			break;
		case LNB_CMD_CURRENT_LIMIT_CONTROL:
			result = lnb_tps65233_current_limit_control(lnb_id_tmp,(UINT8)param);
			break;
		case LNB_CMD_GET_OCP:
			result = lnb_tps65233_get_ocp(lnb_id_tmp);
			break;
        default:
            result = !SUCCESS;
            break;
    }
    return result;
}

