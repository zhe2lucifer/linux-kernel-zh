/*****************************************************************************
*    Tuner sample code
*
*    History:
*          Date           Athor            Version         Reason
*     ========  ========  ========  ==========================
*  1.22/01/2017     robin.gan       ver 1.2         porting to linux, x mean 1,2,...,
										it can support RT710 or RT720
*****************************************************************************/
#include "tun_rt7x0.h"

#if 0
#define RT7X0_PRINTF   printk
#else
#define RT7X0_PRINTF(...)
#endif

#define MAX_TUNER_SUPPORT_NUM 2

struct QPSK_TUNER_CONFIG_EXT * RT7x0_dev_id[MAX_TUNER_SUPPORT_NUM] = {NULL};
static UINT32 RT7x0_tuner_cnt = 0;
static UINT32 g_cur_freq;

UINT8 rt7x0_convert(UINT8 InvertNum)
{
	UINT8 ReturnNum = 0;
	UINT8 AddNum    = 0x80;
	UINT8 BitNum    = 0x01;
	UINT8 CuntNum   = 0;
	for (CuntNum = 0;CuntNum < 8;CuntNum ++)
	{
		if(BitNum & InvertNum)
		{
			ReturnNum += AddNum;
		}
		AddNum = AddNum >> 1;
		BitNum = BitNum << 1;
	}
	return ReturnNum;
}
RT710_Err_Type rt7x0_i2c_write(UINT32 tuner_id, I2C_TYPE *I2C_Info)
{
	I2C_LEN_TYPE i2c_info_len;
	i2c_info_len.Data[0] = I2C_Info->Data;
	i2c_info_len.RegAddr = I2C_Info->RegAddr;
	i2c_info_len.Len = 1;

	return rt7x0_i2c_write_len(tuner_id, &i2c_info_len);
}

RT710_Err_Type rt7x0_i2c_write_len(UINT32 tuner_id, I2C_LEN_TYPE *I2C_Info)
{
	UINT8 data[56];	
	UINT8 length;
	int i2c_result;	
	UINT32 rd = 0;
	struct QPSK_TUNER_CONFIG_EXT * RT7x0_ptr = NULL;

	RT7x0_ptr = RT7x0_dev_id[tuner_id];	
	
	length = I2C_Info->Len;			//the length need to write
	data[0] = I2C_Info->RegAddr;	//the reg addr

	RT7X0_PRINTF("[%s %d]RT7x0_ptr->i2c_type_id=0x%x, RT7x0_ptr->c_tuner_base_addr=0x%x\n", __FUNCTION__, __LINE__, RT7x0_ptr->i2c_type_id, RT7x0_ptr->c_tuner_base_addr);
	while((rd+15)<length)
	{
		comm_memcpy(&data[1], &I2C_Info->Data[rd], 15);
		i2c_result = nim_i2c_write(RT7x0_ptr->i2c_type_id, RT7x0_ptr->c_tuner_base_addr, data, 16);
		rd+=15;
		data[0] += 15;
		if(SUCCESS != i2c_result)
		{
			RT7X0_PRINTF("[%s %d]error, i2c_result=%d\n", __FUNCTION__, __LINE__, i2c_result);
			return RT_Fail;
		}
	}
	comm_memcpy(&data[1], &I2C_Info->Data[rd], length-rd);
	i2c_result = nim_i2c_write(RT7x0_ptr->i2c_type_id, RT7x0_ptr->c_tuner_base_addr, data, length-rd+1);
	return RT_Success;
}

RT710_Err_Type rt7x0_i2c_read_len(UINT32 tuner_id, I2C_LEN_TYPE *I2C_Info)
{
	UINT8 data[56];
	UINT8 length;
	int i2c_result;
	struct QPSK_TUNER_CONFIG_EXT * RT7x0_ptr = NULL;
	RT7x0_ptr = RT7x0_dev_id[tuner_id];
	UINT8 i;
	
	data[0] = 0x00;			//the Reg addr,  it must read from 0x00 for r858
	length = I2C_Info->Len;	//the length need to write	
	
	i2c_result = nim_i2c_write_read(RT7x0_ptr->i2c_type_id, RT7x0_ptr->c_tuner_base_addr, data, 1, length);
	if(SUCCESS != i2c_result)
	{
		RT7X0_PRINTF("[%s %d]I2c_write_read error, i2c_result=%d!\n", __FUNCTION__, __LINE__, i2c_result);
		return RT_Fail;
	}
	for (i=0; i<length; i++)
	{
		I2C_Info->Data[i] = rt7x0_convert(data[i]);
	}
	return RT_Success;
}

INT32 tun_rt7x0_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config)
{
	RT710_INFO_Type RT710_Set_Info;
	struct QPSK_TUNER_CONFIG_EXT * RT7x0_ptr = NULL;
	
	RT7X0_PRINTF("Run into %s, i2c addr = 0x%x\n", __FUNCTION__, ptrTuner_Config->c_tuner_base_addr);
	
	if (ptrTuner_Config == NULL||RT7x0_tuner_cnt>=MAX_TUNER_SUPPORT_NUM)
	{
		RT7X0_PRINTF("[%s %d]ptrTuner_Config == NULL||RT7x0_tuner_cnt>=MAX_TUNER_SUPPORT_NUM\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
	
	RT7x0_ptr = (struct QPSK_TUNER_CONFIG_EXT *)comm_malloc(sizeof(struct QPSK_TUNER_CONFIG_EXT));
	if(!RT7x0_ptr)
	{
		RT7X0_PRINTF("[%s %d]RT7x0_ptr is NULL!\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
	comm_memcpy(RT7x0_ptr, ptrTuner_Config, sizeof(struct QPSK_TUNER_CONFIG_EXT));
	RT7x0_dev_id[RT7x0_tuner_cnt] = RT7x0_ptr;
	*tuner_id = RT7x0_tuner_cnt;

	//Get User Parameter from board_config, 1--> Single end, 0--> Differential
    if(0 == RT7x0_ptr->c_tuner_out_S_d_sel)
        RT710_Set_Info.RT710_OutputSignal_Mode = DifferentialOut;
    else
        RT710_Set_Info.RT710_OutputSignal_Mode = SingleOut;

    //Set default config
	RT710_Set_Info.RT710_LoopThrough_Mode = SIGLE_IN;
	RT710_Set_Info.RT710_ClockOut_Mode = ClockOutOff;
     
	RT710_Set_Info.RT710_AGC_Mode = AGC_Negative;
	
	// Default set mVp-p = 720(single mode)
	RT710_Set_Info.RT710_AttenVga_Mode = ATTENVGAOFF;	
	RT710_Set_Info.R710_FineGain = FINEGAIN_2DB;
	
	RT710_init(*tuner_id, RT710_Set_Info);
	
	RT7x0_tuner_cnt++;
	
	return SUCCESS;
}

INT32 tun_rt7x0_release(void)
{
	RT7X0_PRINTF("[%s %d]Enter!\n",__FUNCTION__,__LINE__);
    RT7x0_tuner_cnt = 0;
    return SUCCESS;
}

/*****************************************************************************
* INT32 nim_rt7x0_control(UINT32 freq, UINT8 sym, UINT8 cp)
*
* Tuner write operation
*
* Arguments:
*  Parameter1: UINT32 freq		: Synthesiser programmable divider
*  Parameter2: UINT8 sym		: symbol rate
*
* Return Value: INT32			: Result
*****************************************************************************/
INT32 tun_rt7x0_control(UINT32 tuner_id, UINT32 freq, UINT32 sym)
{
	RT710_INFO_Type RT710_Set_Info;
	struct QPSK_TUNER_CONFIG_EXT * RT7x0_ptr = NULL;
	
	RT7X0_PRINTF("Run into %s, tuner_id = %d, freq = %d, sym = %d\n", __FUNCTION__, tuner_id, freq, sym);
	
	if(tuner_id>=RT7x0_tuner_cnt||tuner_id>=MAX_TUNER_SUPPORT_NUM)
	{
		RT7X0_PRINTF("[%s %d]ptrTuner_Config == NULL||RT7x0_tuner_cnt>=MAX_TUNER_SUPPORT_NUM\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
	
	RT7x0_ptr = RT7x0_dev_id[tuner_id];

	g_cur_freq = freq;

	// User parameter
	RT710_Set_Info.RF_KHz = freq*1000;
	RT710_Set_Info.SymbolRate_Kbps = sym;
	
	// For autoscan sym = 0 means autoscan
	if(0 == sym)
	{   
		RT710_Set_Info.RT710_Scan_Mode = AUTO_SCAN;
		sym = 35000; // Can not set 45m, because it may be cause get energy too low, paladin.ye
	}
	else
	{   
		RT710_Set_Info.RT710_Scan_Mode = MANUAL_SCAN;
	}		
	
	// Low symbol offset 3M
	if (sym < 6500)
	{
		RT710_Set_Info.SymbolRate_Kbps += 6000;
	}

	// Add 2M for LNB frequency shifting
	RT710_Set_Info.SymbolRate_Kbps += 2000;
	
	// Set RollOff default 0.35
	RT710_Set_Info.RT710_RollOff_Mode = ROLL_OFF_0_35;	

	RT710_Setting(tuner_id, RT710_Set_Info);
	return SUCCESS;
}

/*****************************************************************************
* INT32 nim_rt7x0_status(UINT8 *lock)
*
* Tuner read operation
*
* Arguments:
*  Parameter1: UINT8 *lock		: Phase lock status
*
* Return Value: INT32			: Result
*****************************************************************************/
INT32 tun_rt7x0_status(UINT32 tuner_id,UINT8 *lock)
{	
	struct QPSK_TUNER_CONFIG_EXT * tuner_dev_ptr = NULL;
	
	RT7X0_PRINTF("Run into %s, tuner_id = %d\n", __FUNCTION__, tuner_id);
	
	if(tuner_id>=RT7x0_tuner_cnt||tuner_id>=MAX_TUNER_SUPPORT_NUM)
	{
		*lock = 0;
		RT7X0_PRINTF("[%s %d]tuner_id>=RT7x0_tuner_cnt||tuner_id>=MAX_TUNER_SUPPORT_NUM\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
	tuner_dev_ptr = RT7x0_dev_id[tuner_id];
	
	*lock = 1;
	return SUCCESS;
}

INT32 tun_rt7x0_command(UINT32 tuner_id, INT32 cmd, INT32 *param)
{
	INT32 ret = SUCCESS;

	switch(cmd)
	{
		case NIM_TUNER_GET_RF_POWER_LEVEL:			
			ret = tun_rt7x0_get_rf_rssi(tuner_id, (struct ali_nim_agc *)param);
			break;
			
		default:
			ret = ERR_FAILUE;
			break;
	}
	return ret;
}

INT32 tun_rt7x0_Standby(UINT32 tuner_id, RT710_LoopThrough_Type RT710_LTSel, RT710_ClockOut_Type RT710_CLKSel)
{	
	struct QPSK_TUNER_CONFIG_EXT * tuner_dev_ptr = NULL;
	
	RT7X0_PRINTF("Run into %s, tuner_id = %d, RT710_LTSel = %d, RT710_CLKSel = %d\n", __FUNCTION__, RT710_LTSel, RT710_CLKSel);
	
	if(tuner_id>=RT7x0_tuner_cnt||tuner_id>=MAX_TUNER_SUPPORT_NUM)
	{
		RT7X0_PRINTF("[%s %d]tuner_id>=RT7x0_tuner_cnt||tuner_id>=MAX_TUNER_SUPPORT_NUM\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
	}
	tuner_dev_ptr = RT7x0_dev_id[tuner_id];
	
	RT710_Standby(tuner_id, RT710_LTSel, RT710_CLKSel);
	
	return SUCCESS;
}

INT32 tun_rt7x0_get_rf_rssi(UINT32 tuner_id, struct ali_nim_agc *tuner_agc)
{ 
	INT32    rf_level;
	UINT8	rf_gain;

	RT710_GetRfRssi(tuner_id, g_cur_freq, &rf_level, &rf_gain);
	if (0==rf_gain)//if the rf_gain is 0 and 1, used the demod agc to map the rf_level
	{
		if (tuner_agc->demod_agc < 128)
		{
			tuner_agc->rf_level= tuner_agc->demod_agc/2 - 6;
		}
		else
		{
			tuner_agc->rf_level= (tuner_agc->demod_agc-256)/2 - 6;
		}
	}
	else if (1==rf_gain)
	{
		tuner_agc->rf_level= (tuner_agc->demod_agc-256)/3 - 58;
	}
	else////if the rf_gain is 2, used the tuner agc
	{
		rf_level /= 1000;//change the unit to dbm
		
		if (tuner_agc->demod_agc > 240)
		{
			tuner_agc->rf_level = rf_level - 4;
		}
		else
		{
			tuner_agc->rf_level = rf_level - 6;
		}				
	}		
    return SUCCESS;
}

