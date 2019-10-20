/*****************************************************************************
*    Copyright (C) 2010 ALi Corp. All Rights Reserved.
*    
*    Company confidential and Properietary information.       
*    This information may not be disclosed to unauthorized  
*    individual.    
*    File: nim_rda5815m.c
*   
*    Description: 
*    
*    History: 
*    Date            Author        Version       Reason
*    ========       ========     ========       ========
*    2013/8/26       David         V1.0
*    2017/06/29      robin.gan     V1.1        1.modify max support num
*                                              2.fix init error when two tuner work
*                                              3.add get rf_level interface
*****************************************************************************/
#include "nim_rda5815m.h"

#if 0
#define RDA5815M_PRINTF(x...) printk(KERN_INFO x)
#else
#define RDA5815M_PRINTF(...)
#endif

#define MAX_TUNER_SUPPORT_NUM  2

// Tuner crystal CLK Freqency
//static UINT32 rda5815m_tuner_cnt = 0;
static struct QPSK_TUNER_CONFIG_EXT  rda5815m_dev_id[MAX_TUNER_SUPPORT_NUM];
static BOOL rda5815m_tuner_id_flag[MAX_TUNER_SUPPORT_NUM] = {0,0};
#define Xtal_27M
//#define Xtal_30M
//#define Xtal_24M
UINT32 g_cur_freq;

// I2C write function (register start address, register array pointer, register length)
static int Tuner_I2C_write(UINT32 tuner_id, unsigned char reg_start, unsigned char* buff, unsigned char length)
{
	UINT8 data[16];
	UINT32 rd = 0;
	int i2c_result;
	struct QPSK_TUNER_CONFIG_EXT * rda5815m_ptr = NULL;

	rda5815m_ptr = &rda5815m_dev_id[tuner_id];	
	data[0] = reg_start;

	while((rd+15)<length)
	{
		memcpy(&data[1], &buff[rd], 15);
		i2c_result = ali_i2c_write(rda5815m_ptr->i2c_type_id, rda5815m_ptr->c_tuner_base_addr, data, 16);
		rd+=15;
		data[0] += 15;
		if(SUCCESS != i2c_result)
		{
			RDA5815M_PRINTF("[%s %d]i2c write fail. i2c_result=%d\n", __FUNCTION__, __LINE__, i2c_result);
			return i2c_result;
		}
	}
	memcpy(&data[1], &buff[rd], length-rd);
	i2c_result = ali_i2c_write(rda5815m_ptr->i2c_type_id, rda5815m_ptr->c_tuner_base_addr, data, length-rd+1);

	return i2c_result;
}

static int Tuner_I2C_read(UINT32 tuner_id, unsigned char reg_start, unsigned char* buff, unsigned char length)
{
	UINT8 data[16];
	UINT32 rd = 0;
	int i2c_result;
	struct QPSK_TUNER_CONFIG_EXT * rda5815m_ptr = NULL;

	rda5815m_ptr = &rda5815m_dev_id[tuner_id];	
	data[0] = reg_start;

	while((rd+15)<length)
	{
		i2c_result = ali_i2c_write_read(rda5815m_ptr->i2c_type_id, rda5815m_ptr->c_tuner_base_addr, data, 1, 15);
		memcpy(&buff[rd], &data[0], 15);
		rd+=15;
		data[0] += 15;
		if(SUCCESS != i2c_result)
		{
			RDA5815M_PRINTF("[%s %d]i2c read fail. i2c_result=%d\n", __FUNCTION__, __LINE__, i2c_result);
			return i2c_result;
		}
	}
	i2c_result = ali_i2c_write_read(rda5815m_ptr->i2c_type_id, rda5815m_ptr->c_tuner_base_addr, data, 1, length-rd);
	memcpy(&buff[rd], &data[0], length-rd);
	return i2c_result;
}

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
INT32 nim_rad5815m_allocate_tuner_id(void)
{
	UINT32 i = 0;
	for (i=0; i<MAX_TUNER_SUPPORT_NUM;i++)
	{
		if(FALSE == rda5815m_tuner_id_flag[i]) //find unoccupied tuner_id
		{
			return i;//allocate  success
		}
	}
	return ERR_FAILUE;//alloctae fail
}


INT32 nim_rda5815m_init(UINT32* tuner_id, struct QPSK_TUNER_CONFIG_EXT * ptrTuner_Config)
{
	struct QPSK_TUNER_CONFIG_EXT * rda5815m_ptr = NULL;
	UINT8 i;
	UINT8 data;
	INT32 ret;
	
	static UINT8 control_data[][2] = 
	{
		{0x04,0x04},
		{0x04,0x05}, 

		// Initial configuration start

		//pll setting 

		{0x1a,0x13},
		{0x41,0x53},
		{0x38,0x9B},
		{0x39,0x15},
		{0x3A,0x00},
		{0x3B,0x00},
		{0x3C,0x0c},
		{0x0c,0xE2},
		{0x2e,0x6F},

#ifdef Xtal_27M
		{0x72,0x07},	
		{0x73,0x10},
		{0x74,0x71},
		{0x75,0x06}, 
		{0x76,0x40},
		{0x77,0x89},
		{0x79,0x04},	
		{0x7A,0x2A},
		{0x7B,0xAA},
		{0x7C,0xAB},
#endif
#ifdef Xtal_30M
		{0x72,0x06},	
		{0x73,0x60},
		{0x74,0x66},
		{0x75,0x05}, 
		{0x76,0xA0},
		{0x77,0x7B},
		{0x79,0x03},	
		{0x7A,0xC0},
		{0x7B,0x00},
		{0x7C,0x00},
#endif
#ifdef Xtal_24M
		{0x72,0x08},	
		{0x73,0x00},
		{0x74,0x80},
		{0x75,0x07}, 
		{0x76,0x10},
		{0x77,0x9A},
		{0x79,0x04},	
		{0x7A,0xB0},
		{0x7B,0x00},
		{0x7C,0x00},
#endif

		{0x2f,0x57},
		{0x0d,0x70},
		{0x18,0x4B},
		{0x30,0xFF},
		{0x5c,0xFF},
		{0x65,0x00},
		{0x70,0x3F},
		{0x71,0x3F},
		{0x53,0xA8},
		{0x46,0x21},
		{0x47,0x84},
		{0x48,0x10},
		{0x49,0x08},
		{0x60,0x80},
		{0x61,0x80},
		{0x6A,0x08},
		{0x6B,0x63},
		{0x69,0xF8},
		{0x57,0x64},
		{0x05,0xaa},
		{0x06,0xaa},
		{0x15,0xAE},
		{0x4a,0x67},
		{0x4b,0x77},

		//agc setting

		{0x4f,0x40},
		{0x5b,0x20},

		{0x16,0x0C},
		{0x18,0x0C},            
		{0x30,0x1C},            
		{0x5c,0x2C},            
		{0x6c,0x3C},            
		{0x6e,0x3C},            
		{0x1b,0x7C},            
		{0x1d,0xBD},            
		{0x1f,0xBD},            
		{0x21,0xBE},            
		{0x23,0xBE},            
		{0x25,0xFE},            
		{0x27,0xFF},            
		{0x29,0xFF},            
		{0xb3,0xFF},            
		{0xb5,0xFF},            

		{0x17,0xF0},            
		{0x19,0xF0},            
		{0x31,0xF0},            
		{0x5d,0xF0},            
		{0x6d,0xF0},            
		{0x6f,0xF1},            
		{0x1c,0xF5},            
		{0x1e,0x35},            
		{0x20,0x79},            
		{0x22,0x9D},            
		{0x24,0xBE},            
		{0x26,0xBE},            
		{0x28,0xBE},            
		{0x2a,0xCF},            
		{0xb4,0xDF},            
		{0xb6,0x0F},            

		{0xb7,0x15},	//start    
		{0xb9,0x6c},	           
		{0xbb,0x63},	           
		{0xbd,0x5a},	           
		{0xbf,0x5a},	           
		{0xc1,0x55},	           
		{0xc3,0x55},	           
		{0xc5,0x47},	           
		{0xa3,0x53},	           
		{0xa5,0x4f},	           
		{0xa7,0x4e},	           
		{0xa9,0x4e},	           
		{0xab,0x54},            
		{0xad,0x31},            
		{0xaf,0x43},            
		{0xb1,0x9f},               

		{0xb8,0x6c}, //end      
		{0xba,0x92},            
		{0xbc,0x8a},            
		{0xbe,0x8a},            
		{0xc0,0x82},            
		{0xc2,0x93},            
		{0xc4,0x85},            
		{0xc6,0x77},            
		{0xa4,0x82},            
		{0xa6,0x7e},            
		{0xa8,0x7d},            
		{0xaa,0x6f},            
		{0xac,0x65},            
		{0xae,0x43},            
		{0xb0,0x9f},             
		{0xb2,0xf0},             

		{0x81,0x92}, //rise     
		{0x82,0xb4},            
		{0x83,0xb3},            
		{0x84,0xac},            
		{0x85,0xba},            
		{0x86,0xbc},            
		{0x87,0xaf},            
		{0x88,0xa2},            
		{0x89,0xac},            
		{0x8a,0xa9},            
		{0x8b,0x9b},            
		{0x8c,0x7d},            
		{0x8d,0x74},            
		{0x8e,0x9f},           
		{0x8f,0xf0},               
		             
		{0x90,0x15}, //fall     
		{0x91,0x39},            
		{0x92,0x30},            
		{0x93,0x27},            
		{0x94,0x29},            
		{0x95,0x0d},            
		{0x96,0x10},            
		{0x97,0x1e},            
		{0x98,0x1a},            
		{0x99,0x19},            
		{0x9a,0x19},            
		{0x9b,0x32},            
		{0x9c,0x1f},            
		{0x9d,0x31},            
		{0x9e,0x43},                  
	};	

	RDA5815M_PRINTF("[%s %d] MAX_Tuner_SUPPROT_NUM = %d\n ", __FUNCTION__, __LINE__,MAX_TUNER_SUPPORT_NUM);
	if (ptrTuner_Config == NULL) 
	{
		RDA5815M_PRINTF("[%s %d]ptrTuner_Config == NULL\n", __FUNCTION__, __LINE__);
		return ERR_FAILUE;
    }

	ret = nim_rad5815m_allocate_tuner_id();
	if(ret < 0)//alloctae fail
	{
		RDA5815M_PRINTF("[%s]line=%d,get tuner_id fail,support tuner num beyond limit!\n",__FUNCTION__,__LINE__);
		return ERR_FAILUE;
	}
	else 
	{
		*tuner_id = (UINT32)ret;//allocate tuner_id successfully and pass back for demod driver
	}	
	
	rda5815m_ptr = &rda5815m_dev_id[*tuner_id];	
	memcpy(rda5815m_ptr, ptrTuner_Config, sizeof(struct QPSK_TUNER_CONFIG_EXT));
			
	RDA5815M_PRINTF("[%s %d]tuner_id=%d\n", __FUNCTION__, __LINE__, (int)(*tuner_id));
	for(i = 0 ; i < sizeof(control_data)/sizeof(control_data[0]); i++)
	{
		data = control_data[i][1];
		Tuner_I2C_write(*tuner_id, control_data[i][0], &data, 1);           
	}

	rda5815m_tuner_id_flag[*tuner_id] = TRUE;
	
	comm_sleep(10);	// Time delay 10ms	

	RDA5815M_PRINTF("[%s %d],tuner_id=%d\n", __FUNCTION__, __LINE__,(int)(*tuner_id));
	return SUCCESS;
}

INT32 nim_rda5815m_close(UINT32 tuner_id)
{
	RDA5815M_PRINTF("[%s]line=%d,enter!\n", __FUNCTION__, __LINE__);

	if (tuner_id >= MAX_TUNER_SUPPORT_NUM)
	{
		RDA5815M_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	
	if (rda5815m_tuner_id_flag[tuner_id] != FALSE)
	{
		rda5815m_tuner_id_flag[tuner_id] = FALSE;//clean tuner_id use status
	}
	else
	{
		RDA5815M_PRINTF("NOTE error this tuner_id has not been used\n"); //please don't replace this printk or make any restrictions,let it print after run else
		return ERR_FAILUE;
	}

	return RET_SUCCESS;
}
/***********************************************************************
* unsigned int nim_rda5815m_control (unsigned int tuner_id, unsigned int channel_freq, unsigned int bb_sym)
*  Arguments:
*  Parameter1: unsigned int channel_freq		: Channel frequency (MHz)
*  Parameter2: unsigned int bb_sym		    : Baseband Symbol Rate (KHz)
*  Return Value: unsigned int			: Result
							
					
***********************************************************************/
INT32 nim_rda5815m_control(UINT32 tuner_id, UINT32 channel_freq, UINT32 bb_sym)
{
    UINT8 data;    
 	UINT32 temp_value = 0;
	UINT32 bw;
	UINT8 Filter_bw_control_bit;	
    
	struct QPSK_TUNER_CONFIG_EXT * rda5815m_ptr = NULL;	

	RDA5815M_PRINTF("[%s %d]tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
	if(tuner_id >= MAX_TUNER_SUPPORT_NUM)
	{
		RDA5815M_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	rda5815m_ptr = &rda5815m_dev_id[tuner_id];
	g_cur_freq = channel_freq;	

    data=0xc1;
    Tuner_I2C_write(tuner_id,0x04,&data, 1); 
    data=0x95;
    Tuner_I2C_write(tuner_id,0x2b,&data, 1);     
	//set frequency start
	#ifdef Xtal_27M		
	temp_value = (UINT32)(channel_freq* 77672);//((2^21) / RDA5815_XTALFREQ);
	#endif
	#ifdef Xtal_30M		
	temp_value = (UINT32)(channel_freq* 69905);//((2^21) / RDA5815_XTALFREQ);
	#endif
	#ifdef Xtal_24M		
	temp_value = (UINT32)(channel_freq* 87381);//((2^21) / RDA5815_XTALFREQ);
	#endif

	data = ((UINT8)((temp_value>>24)&0xff));
	Tuner_I2C_write(tuner_id,0x07,&data, 1);
	data = ((UINT8)((temp_value>>16)&0xff));	
	Tuner_I2C_write(tuner_id,0x08,&data, 1);	
   	data = ((UINT8)((temp_value>>8)&0xff));
	Tuner_I2C_write(tuner_id,0x09,&data, 1);	
   	data = ((UINT8)( temp_value&0xff));
	Tuner_I2C_write(tuner_id,0x0a,&data, 1);
	//set frequency end
	
	// set Filter bandwidth start
	bw=bb_sym;
	if (bb_sym == 0)
    {
	      //Filter_bw_control_bit = 40;
	     Filter_bw_control_bit = 30; 
	}
	else
	{
	    Filter_bw_control_bit = (UINT8)((bw*135/200+4000)/1000);
	}

    if(Filter_bw_control_bit<4)
		Filter_bw_control_bit = 4;    // MHz
	else if(Filter_bw_control_bit>40)
		Filter_bw_control_bit = 40;   // MHz

    Filter_bw_control_bit&=0x3f;
	Filter_bw_control_bit|=0x40;		
	
	Tuner_I2C_write(tuner_id,0x0b,&Filter_bw_control_bit, 1);
	// set Filter bandwidth end
	
	data=0xc3;
	Tuner_I2C_write(tuner_id,0x04,&data, 1); 
	data=0x97;	
    Tuner_I2C_write(tuner_id,0x2b,&data, 1);
	comm_sleep(5);//Wait 5ms;
  
    return SUCCESS;
}

INT32 nim_rda5815m_status(UINT32 tuner_id, UINT8 *lock)
{
	if (tuner_id >= MAX_TUNER_SUPPORT_NUM)
	{
		*lock = 0;
		RDA5815M_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	/* Because rda5815m doesn't has this flag,return 1 directly */
	*lock = 1;
	return SUCCESS;
}

INT32 nim_rda5815m_command(UINT32 tuner_id, INT32 cmd, UINT32 param)
{
	INT32 ret = SUCCESS;

	if (tuner_id >= MAX_TUNER_SUPPORT_NUM)
	{
		RDA5815M_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	
	switch(cmd)
	{
		case NIM_TUNER_GET_RF_POWER_LEVEL:			
			ret = nim_rda5815m_rssi(tuner_id, (struct ali_nim_agc *)param);
			break;

		case NIM_TURNER_SET_STANDBY:
			break;

		case NIM_TUNER_WAKE_UP:
			break;
		
		default:
			ret = ERR_FAILUE;
			break;
	}
	return ret;
}


INT32 nim_rda5815m_rssi(UINT32 tuner_id, struct ali_nim_agc *tuner_agc)
{	
	/* Gain stage limits				           st1  pre st2	post i2v fil  */
#define RDA5815m_Gain_Stage__0 0xc00 /* '11   00	00	00	 00	 00	' */
#define RDA5815m_Gain_Stage__1 0xc00 /* '11   00	00	00	 00	 00 ' */
#define RDA5815m_Gain_Stage__2 0xc01 /* '11   00	00	00	 00	 01 ' */
#define RDA5815m_Gain_Stage__3 0xc02 /* '11   00	00	00	 00	 10 ' */
#define RDA5815m_Gain_Stage__4 0xc03 /* '11   00	00	00	 00	 11 ' */
#define RDA5815m_Gain_Stage__5 0xc13 /* '11   00	00	01	 00	 11 ' */
#define RDA5815m_Gain_Stage__6 0xd17 /* '11   01	00	01	 01	 11 ' */
#define RDA5815m_Gain_Stage__7 0xd5b /* '11   01	01	01	 10	 11 ' */
#define RDA5815m_Gain_Stage__8 0xe5b /* '11   10	01	01	 10	 11 ' */
#define RDA5815m_Gain_Stage__9 0xf9b /* '11   11	10	01	 10	 11 ' */
#define RDA5815m_Gain_Stage_10 0xfab /* '11   11	10	10	 10	 11 ' */
#define RDA5815m_Gain_Stage_11 0xfaf /* '11   11	10	10	 11	 11 ' */
#define RDA5815m_Gain_Stage_12 0xfef /* '11   11	11	10	 11	 11 ' */
#define RDA5815m_Gain_Stage_13 0xfff /* '11   11	11	11	 11	 11 ' */
#define RDA5815m_Gain_Stage_14 0xfff /* '11   11	11	11	 11	 11 ' */
#define RDA5815m_Gain_Stage_15 0xfff /* '11   11	11	11	 11	 11 ' */

	UINT8 buffer;
	UINT8 data16,data17,st1,pre,st2,post,i2v,filter,vga;
	UINT32 stage_code;
	UINT8 gain_stage;
	UINT8 band;
	UINT32 vga_gain, total_gain;

	/* gain band/gain_stage:    0       1      2     3     4      5      6     7     8     9     10    11    12    13    14   15 */
	INT8 gain[13][16] = {	
							{-7, -7, -1, 4, 10, 15, 25, 34, 40, 49, 55, 61, 65, 71, 71, 71},  /* 1  950MHz<=Freq<1000MHz */
							{-7, -7, -1, 4, 10, 16, 25, 34, 40, 49, 55, 61, 65, 71, 71, 71},  /* 2  1000MHz<=Freq<1100MHz */
							{-6, -6, -0, 5, 11, 17, 26, 34, 41, 49, 55, 61, 65, 71, 71, 71},  /* 3  1100MHz<=Freq<1200MHz */
							{-5, -5, 0,  6, 12, 17, 27, 35, 41, 49, 54, 61, 65, 71, 71, 71},  /* 4  1200MHz<=Freq<1300MHz */
							{-5, -5, 0,  6, 12, 17, 27, 35, 40, 48, 54, 60, 64, 71, 71, 71},  /* 5  1300MHz<=Freq<1400MHz */
							{-5, -5, 0,  6, 12, 18, 27, 35, 41, 48, 53, 60, 64, 70, 70, 70},  /* 6  1400MHz<=Freq<1500MHz */
							{-5, -5, 1,  7, 13, 18, 27, 36, 41, 48, 54, 60, 64, 70, 70, 70},  /* 7  1500MHz<=Freq<1600MHz */
							{-4, -4, 1,  7, 13, 19, 28, 36, 41, 47, 54, 60, 64, 70, 70, 70},  /* 8  1600MHz<=Freq<1700MHz */
							{-4, -4, 1,  7, 13, 19, 28, 36, 41, 47, 53, 59, 63, 70, 70, 70},  /* 9  1700MHz<=Freq<1800MHz */
							{-4, -4, 1,  7, 13, 19, 27, 36, 40, 46, 53, 59, 62, 69, 69, 69},  /* 10  1800MHz<=Freq<1900MHz */
							{-4, -4, 1,  7, 13, 19, 28, 35, 40, 46, 52, 58, 62, 69, 69, 69},  /* 11  1900MHz<=Freq<2000MHz */
							{-4, -4, 1,  7, 13, 19, 28, 36, 40, 46, 52, 58, 61, 68, 68, 68},  /* 12  2000MHz<=Freq<2100MHz */
							{-4, -4, 1,  7, 13, 19, 27, 35, 39, 45, 51, 57, 60, 67, 67, 67}   /* 13  2100MHz<=Freq<=2150MHz */
							};

	if (tuner_id >= MAX_TUNER_SUPPORT_NUM)
	{
		RDA5815M_PRINTF("[%s %d]error. tuner_id=%d\n", __FUNCTION__, __LINE__, (int)tuner_id);
		return ERR_FAILUE;
	}
	
	Tuner_I2C_read(tuner_id, 0x16, &data16, 1);
	i2v    = (data16&0xc0)>>6;
	filter = (data16&0x30)>>4;
	st1    = (data16&0x0c)>>2;
	st2    = (data16&0x03)>>0;

	Tuner_I2C_read(tuner_id, 0x17, &data17, 1);
	pre    = (data17&0x0c)>>2;
	post   = (data17&0x03)>>0;

	stage_code = (st1<<10) + (pre<<8) + (st2<<6) + (post<<4) + (i2v<<2) + (filter<<0);

	if (stage_code == RDA5815m_Gain_Stage_13) { gain_stage = 13; }
	else if (stage_code == RDA5815m_Gain_Stage_12) { gain_stage = 12; }
	else if (stage_code == RDA5815m_Gain_Stage_11) { gain_stage = 11; }
	else if (stage_code == RDA5815m_Gain_Stage_10) { gain_stage = 10; }
	else if (stage_code == RDA5815m_Gain_Stage__9) { gain_stage =  9; }
	else if (stage_code == RDA5815m_Gain_Stage__8) { gain_stage =  8; }
	else if (stage_code == RDA5815m_Gain_Stage__7) { gain_stage =  7; }
	else if (stage_code == RDA5815m_Gain_Stage__6) { gain_stage =  6; }
	else if (stage_code == RDA5815m_Gain_Stage__5) { gain_stage =  5; }
	else if (stage_code == RDA5815m_Gain_Stage__4) { gain_stage =  4; }
	else if (stage_code == RDA5815m_Gain_Stage__3) { gain_stage =  3; }
	else if (stage_code == RDA5815m_Gain_Stage__2) { gain_stage =  2; }
	else if (stage_code == RDA5815m_Gain_Stage__1) { gain_stage =  1; }
	else  { gain_stage =  0; }

	if (g_cur_freq < 945)       {return 1;}
	else if (g_cur_freq < 1000)	{band =0;}
	else if (g_cur_freq < 1100)	{band =1;}
	else if (g_cur_freq < 1200)	{band =2;}
	else if (g_cur_freq < 1300)	{band =3;}
	else if (g_cur_freq < 1400)	{band =4;}
	else if (g_cur_freq < 1500)	{band =5;}
	else if (g_cur_freq < 1600)	{band =6;}
	else if (g_cur_freq < 1700)	{band =7;}
	else if (g_cur_freq < 1800)	{band =8;}
	else if (g_cur_freq < 1900)	{band =9;}
	else if (g_cur_freq < 2000)	{band =10;}
	else if (g_cur_freq < 2100)	{band =11;}
	else if (g_cur_freq <= 2155){band =12;}
	else     {return 1;}

	Tuner_I2C_read(tuner_id, 0xb7, &buffer, 1);
	vga = buffer;

	vga_gain = vga*30/255;

	total_gain = gain[band][gain_stage] + vga_gain;

	if (total_gain >= 88)
	{
		tuner_agc->rf_level = -total_gain - 1;
	}
	else
	{
		tuner_agc->rf_level = -total_gain + 2;
	}
	RDA5815M_PRINTF("RDA5815m RSSI = gain[%d][%d] + vga_gain = %d + %d = %d\n", band, gain_stage, gain[band][gain_stage], vga_gain, total_gain);
	RDA5815M_PRINTF("total_gain=0x%x, total_gain=%d\n", total_gain, total_gain);
	RDA5815M_PRINTF("tuner_agc->rf_level=0x%x, tuner_agc->rf_level=%d\n", tuner_agc->rf_level, tuner_agc->rf_level);
	return  0;
}
