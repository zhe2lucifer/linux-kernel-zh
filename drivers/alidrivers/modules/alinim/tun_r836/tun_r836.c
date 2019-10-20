/*****************************************************************************
*    Copyright (C)2008 Ali Corporation. All Rights Reserved.
*
*    File:    tun_r836.c
*
*    Description:    This file contains alpstdae basic function in LLD. 
*    History:
*           Date               Athor                 Version          Reason
*	    ======	    =========	  =========	=================
*	1.  20080520		Trueve Hu		Ver 0.1		Create file.
*	2.  20170908		Orange Cheng	Ver 0.2	     Modify file:Because of R836 source code's limit, This function only support 1 tuner device
														  and support dvbc/dvbt-t2 or isdbt mode.
*****************************************************************************/

#include "tun_r836.h"
#include "R836.h"


#ifdef R840_PRINTF
#define R840_PRINTF  printk
#else
#define R840_PRINTF(...)	do{}while(0)
#endif

#define MAX_TUNER_SUPPORT_NUM 1
static BOOL r836_tuner_id_flag[MAX_TUNER_SUPPORT_NUM] = {0};

static R836_CONFIG R836_Config[MAX_TUNER_SUPPORT_NUM] = {{0}};
static UINT8 r836_reg_ini[2]={0,0};
static UINT32 tmp_rf_freq[2] = {0,0};

static UINT8 r836_work_mode[2]={0,0}; //0: DVBT2(DVB-T), 1: ISDB-T,2DVBC
static R840_Standard_Type r836_cur_std[2]={0,0}; // used for rssi cal.

static UINT8 r836_rssi_rdy[2]={0,0}; //0: ready, 1: not ready..


/*****************************************************************************
* INT32 allocate_tuner_id(void)
*
* find and allocate unoccupied tuner_id
*
* Arguments: tuner_id
*
*
* Return Value:
	 SUCCESS     :allocate  tuner_id successfully
	 ERR_FAILUE  :allocate  tuner_id fail
*****************************************************************************/
INT32 nim_r836_allocate_tuner_id(void)
{
	UINT32 i = 0;
	for (i=0; i<MAX_TUNER_SUPPORT_NUM;i++)
	{
		if(FALSE == r836_tuner_id_flag[i]) //find unoccupied tuner_id
		{
			return i;//allocate  success
		}
	}
	return ERR_FAILUE;//alloctae fail
}



INT32 tun_r836_dvbc_init(UINT32 *tuner_id, struct QAM_TUNER_CONFIG_EXT * ptrTuner_Config)
{
	INT32 ret;

	/* check Tuner Configuration structure is available or not */
	if (ptrTuner_Config == NULL)
	{
		R840_PRINTF("[%s %d]ptrTuner_Config == NULL\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
		
	ret = nim_r836_allocate_tuner_id();
	if(ret < 0)//alloctae fail
	{
		R840_PRINTF("[%s]line=%d,get tuner_id fail,support tuner num beyond limit!\n",__FUNCTION__,__LINE__);
		return ERR_FAILUE;
	}
	else 
	{
		*tuner_id = (UINT32)ret;//allocate tuner_id successfully and pass back for demod driver
	}
	
	R836_Config[*tuner_id].c_tuner_base_addr = (UINT8)ptrTuner_Config->c_tuner_base_addr;
	R836_Config[*tuner_id].c_tuner_crystal = (UINT16)ptrTuner_Config->c_tuner_crystal;
	R836_Config[*tuner_id].i2c_type_id = (UINT32)ptrTuner_Config->i2c_type_id;
	R836_Config[*tuner_id].w_tuner_if_freq = (UINT16)ptrTuner_Config->w_tuner_if_freq;

	r836_reg_ini[*tuner_id ] = 0;		// status bit for initialized r836 register
	tmp_rf_freq[*tuner_id ] = 0;
	r836_rssi_rdy[*tuner_id ] = 0;	
	r836_work_mode[*tuner_id ] = TUN_R836_DVBC; // initial as "DVB-C" mode.
	r836_tuner_id_flag[*tuner_id] = TRUE;
	
	return SUCCESS;
}


INT32 tun_r836_dvbt2_t_init(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)
{
	INT32 ret;
	/* check Tuner Configuration structure is available or not */
	if (ptrTuner_Config == NULL)
	{
		R840_PRINTF("[%s %d]ptrTuner_Config == NULL\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
	
	ret = nim_r836_allocate_tuner_id();
	
	if(ret < 0)//alloctae fail
	{
		R840_PRINTF("[%s]line=%d,get tuner_id fail,support tuner num beyond limit!\n",__FUNCTION__,__LINE__);
		return ERR_FAILUE;
	}
	else 
	{
		*tuner_id = (UINT32)ret;//allocate tuner_id successfully and pass back for demod driver
	}
	R836_Config[*tuner_id].c_tuner_base_addr = (UINT8)ptrTuner_Config->c_tuner_base_addr;
	R836_Config[*tuner_id].c_tuner_crystal = (UINT16)ptrTuner_Config->c_tuner_crystal;
	R836_Config[*tuner_id].i2c_type_id = (UINT32)ptrTuner_Config->i2c_type_id;
	R836_Config[*tuner_id].w_tuner_if_freq = (UINT16)ptrTuner_Config->w_tuner_if_freq;

	r836_reg_ini[*tuner_id]=0;		// status bit for initialized r836 register
	r836_work_mode[*tuner_id] = TUN_R836_DVBT_T2; // initial as "DVB-T2/T combo" mode.
	tmp_rf_freq[*tuner_id ] = 0;
	r836_rssi_rdy[*tuner_id ] = 0;
	r836_tuner_id_flag[*tuner_id] = TRUE;

	return SUCCESS;
}

INT32 tun_r836_isdbt_init(UINT32* tuner_id, struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)
	{
		INT32 ret;
		/* check Tuner Configuration structure is available or not */
		if (ptrTuner_Config == NULL)
		{
			R840_PRINTF("[%s %d]ptrTuner_Config == NULL\n", __FUNCTION__, __LINE__);
			return ERR_FAILUE;
		}
		
		ret = nim_r836_allocate_tuner_id();
		
		if(ret < 0)//alloctae fail
		{
			R840_PRINTF("[%s]line=%d,get tuner_id fail,support tuner num beyond limit!\n",__FUNCTION__,__LINE__);
			return ERR_FAILUE;
		}
		else 
		{
			*tuner_id = (UINT32)ret;//allocate tuner_id successfully and pass back for demod driver
		}
		
		R836_Config[*tuner_id].c_tuner_base_addr = (UINT8)ptrTuner_Config->c_tuner_base_addr;
		R836_Config[*tuner_id].c_tuner_crystal = (UINT16)ptrTuner_Config->c_tuner_crystal;
		R836_Config[*tuner_id].i2c_type_id = (UINT32)ptrTuner_Config->i2c_type_id;
		R836_Config[*tuner_id].w_tuner_if_freq = (UINT16)ptrTuner_Config->w_tuner_if_freq;

	
		r836_reg_ini[*tuner_id]=0;		// status bit for initialized r836 register
		r836_work_mode[*tuner_id] = TUN_R836_ISDBT; // initial as "ISDBT combo" mode.
		tmp_rf_freq[*tuner_id ] = 0;
		r836_rssi_rdy[*tuner_id ] = 0;
		r836_tuner_id_flag[*tuner_id] = TRUE;
	
		return SUCCESS;
	}


INT32 tun_r836_status(UINT32 tuner_id, UINT8 *lock)
{

	if ( tuner_id >= MAX_TUNER_SUPPORT_NUM )
	{
		*lock = 0;
		R840_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	*lock = 1;

	return SUCCESS;
}

INT32 tun_r836_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd)	
{	
	R840_Set_Info tmp_r840_info;

	if ( tuner_id >= MAX_TUNER_SUPPORT_NUM )
	{
		R840_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}

	if(0==r836_reg_ini[tuner_id])
	{
		R840_Init(); // direct setting. should be same with the tuner driver.
		r836_reg_ini[tuner_id]=1;
	}

	if (TUN_R836_DVBT_T2 == r836_work_mode[tuner_id]) // DVBT2/T mode.
	{
		switch (bandwidth)
		{
			case 6:
				tmp_r840_info.R840_Standard =R840_DVB_T2_6M;
				break;

			case 7:
				tmp_r840_info.R840_Standard =R840_DVB_T2_7M;
				break;

			case 8:
				tmp_r840_info.R840_Standard =R840_DVB_T2_8M;
				break;

			default:
				tmp_r840_info.R840_Standard =R840_DVB_T2_8M;
				R840_PRINTF("Error! bandwidth=%d\n",bandwidth);   
				break;
		}
	}
	if (TUN_R836_ISDBT == r836_work_mode[tuner_id]) // ISDB-T mode.
	{
		switch (bandwidth)
		{
			case 6:
				tmp_r840_info.R840_Standard =R840_ISDB_T_IF_5M;
				break;

			case 7:
				tmp_r840_info.R840_Standard =R840_DVB_T_7M_IF_5M;
				break;

			case 8:
				tmp_r840_info.R840_Standard =R840_DVB_T_8M_IF_5M;
				break;

			default:
				tmp_r840_info.R840_Standard =R840_DVB_T_8M_IF_5M;
				R840_PRINTF("Error! bandwidth=%d\n",bandwidth);   
				break;
		}
	}
	else if (TUN_R836_DVBC == r836_work_mode[tuner_id]) // DVBC mode.
	{
		switch (bandwidth)
		{
			case 6:
				tmp_r840_info.R840_Standard =R840_DVB_C_6M;
				break;
#if 0
			case 7:
				tmp_r840_info.R840_Standard =R840_DVB_T2_7M;
				break;
#endif
			case 8:
				tmp_r840_info.R840_Standard =R840_DVB_C_8M;
				break;

			default:
				tmp_r840_info.R840_Standard =R840_DVB_T2_8M;
				R840_PRINTF("Error! bandwidth=%d\n",bandwidth);   
				break;
		}
	}
	r836_cur_std[tuner_id] = tmp_r840_info.R840_Standard;
	
	tmp_r840_info.RF_KHz = freq;
	tmp_r840_info.R840_LT = LT_OFF;
	tmp_r840_info.R840_ClkOutMode = CLK_OUT_OFF;
	tmp_r840_info.R840_IfAgc_Select = IF_AGC1;
		
	R840_SetPllData(tmp_r840_info);

	tmp_rf_freq[tuner_id] = freq;

	if (0 == r836_rssi_rdy[tuner_id])
	{
		r836_rssi_rdy[tuner_id] = 1;
	}

	return SUCCESS;

}

INT32 tun_r836_get_rf_level(UINT32 tuner_id, INT32 *rf_level)// return level in dbuV.
{

	INT32 RSSI;
	if (1 == r836_rssi_rdy[tuner_id])
	{
		R840_GetTotalRssi(tmp_rf_freq[tuner_id], r836_cur_std[tuner_id], &RSSI);
		RSSI = RSSI + 107;
	}
	else
	{
		RSSI = 50;
	}

	if (RSSI > 255)
	{
		*rf_level = 255;
	}
	else if (RSSI < 0)
	{
		*rf_level = 0;
	}
	else
	{
		*rf_level = RSSI;
	}	
	return SUCCESS;
}

INT32 tun_r836_command(UINT32 tuner_id, INT32 cmd, UINT32 param)
{
    INT32 ret = SUCCESS;
    switch(cmd)
    {
        case NIM_TUNER_GET_RF_POWER_LEVEL:
			
			if (1 == r836_rssi_rdy[tuner_id])
				R840_GetTotalRssi(tmp_rf_freq[tuner_id], r836_cur_std[tuner_id], (INT32 *)param);
			else
				ret = ERR_FAILUE;
			
			//change rf_level unit from dBm to -0.1dBm
			if(*(INT32 *)param < 0)
				*(INT32 *)param = 0 - *(INT32 *)param;
			
			*(INT32 *)param *=10;
            break;

        default:
            ret = ERR_FAILUE;
            break;
    }
    return ret;
}



void R840_Delay_MS(int ms)
{
	int i = 0;
	for (i=0; i<=ms; i++)
	{
		comm_delay(1000);		
	}
}

//*--------------------------------------------------------------------------------------
//* Function Name       : UserWrittenI2CWrite
//* Object              : 
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 AddrSize,
//* 						UInt8* pAddr,
//* 						UInt32 WriteLen,
//* 						UInt8* pData
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
#define BURST_SZ 13

typedef struct _I2C_OP_TYPE
{
	UINT8 Data[51]; // 1 more than 50.
	UINT8 Len;
}I2C_OP_TYPE;

R840_ErrCode write_i2c(I2C_OP_TYPE *I2C_Info)
{
	INT32 result = 0;
	UINT8 data[BURST_SZ+1]; // every time, write 14 byte..

	INT32 RemainLen, BurstNum;
	INT32 i,j;
	UINT8 u8_add,len;
	UINT8 *buff = NULL;

	u8_add = R836_Config[0].c_tuner_base_addr;	
	len = (I2C_Info->Len)-1;
	buff = &(I2C_Info->Data[1]);

	RemainLen = len % BURST_SZ; 
	if (RemainLen)
	{
		BurstNum = len / BURST_SZ; 
	}
	else
	{
		BurstNum = len / BURST_SZ - 1;
		RemainLen = BURST_SZ;
	}
	
	for ( i = 0 ; i < BurstNum; i ++ )
	{
		for ( j = 0 ; j < BURST_SZ ; j++  )
		{
			data[j+1]   = buff[i * BURST_SZ + j ];
		}
		data[0] = I2C_Info->Data[0] + BURST_SZ*i;
		result |= nim_i2c_write(R836_Config[0].i2c_type_id, u8_add, data, BURST_SZ+1);
	}

		
	for ( i = 0 ; i < RemainLen ; i++ )
	{
		data[i+1]   = buff[BurstNum * BURST_SZ + i ];
	}

	data[0] = I2C_Info->Data[0] + BURST_SZ*BurstNum;
	
	result |= nim_i2c_write(R836_Config[0].i2c_type_id, u8_add, data, RemainLen+1);

	if (result == SUCCESS)
	{
		return RT_Success;
	}
	else
	{
		return RT_Fail;
	}
}

R840_ErrCode read_i2c(I2C_LEN_TYPE *I2C_Info)
{
	INT32 result = 0;
	UINT8 u8_add,len,*data;
	
	u8_add = R836_Config[0].c_tuner_base_addr | 0x01;

	len = I2C_Info->Len;
	data = I2C_Info->Data;

	if (len > 8)
	{
		return RT_Fail;
	}

	result |= nim_i2c_read(R836_Config[0].i2c_type_id, u8_add, data, len);


	if (result == SUCCESS)
	{
		return RT_Success;
	}
	else
	{
		return RT_Fail;
	}
}


R840_ErrCode I2C_Write_Len(I2C_LEN_TYPE *I2C_Info)
{
	I2C_OP_TYPE tmp_info;
	UINT8 i;

	for (i=0;i< I2C_Info->Len;i++)
	{
		tmp_info.Data[i+1] = I2C_Info->Data[i];
	}
	tmp_info.Data[0] = I2C_Info->RegAddr;
	tmp_info.Len = I2C_Info->Len+1;
	return write_i2c(&tmp_info);
}

UINT8 R840_Convert(UINT8 InvertNum)
{
	UINT8 ReturnNum = 0;
	UINT8 AddNum    = 0x80;
	UINT8 BitNum    = 0x01;
	UINT8 CuntNum   = 0;

	for(CuntNum = 0;CuntNum < 8;CuntNum ++)
	{
		if(BitNum & InvertNum)
			ReturnNum += AddNum;

		AddNum = AddNum >> 1;
		BitNum = BitNum << 1;
	}
	return ReturnNum;
}

R840_ErrCode I2C_Read_Len(I2C_LEN_TYPE *I2C_Info)
{
	INT32 result;
	UINT8 tmp_cnt;
	I2C_Info->RegAddr  = 0x00;
	result = read_i2c(I2C_Info);

	if(RT_Success != result)
		return RT_Fail;

	for(tmp_cnt = 0;tmp_cnt < I2C_Info->Len;tmp_cnt ++)
	{
		 I2C_Info->Data[tmp_cnt] = R840_Convert(I2C_Info->Data[tmp_cnt]);
	}
	
	return RT_Success;
}

R840_ErrCode I2C_Write(I2C_TYPE *I2C_Info)
{
	I2C_OP_TYPE tmp_info;
	tmp_info.Len       = 2;
	tmp_info.Data[0]      = I2C_Info->RegAddr;
	tmp_info.Data[1]      = I2C_Info->Data;
	return write_i2c(&tmp_info);
}


INT32 tun_r836_release(UINT32 tuner_id)
{
	if (tuner_id >= MAX_TUNER_SUPPORT_NUM)
	{
		R840_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	
	if (r836_tuner_id_flag[tuner_id] != FALSE)
	{
		r836_tuner_id_flag[tuner_id] = FALSE;//clean tuner_id use status
	}
	else
	{
		R840_PRINTF("NOTE error this tuner_id has not been used\n"); //please don't replace this printk or make any restrictions,let it print after run else
		return ERR_FAILUE;
	}

    return SUCCESS;
}


