
//***********************************************
//RT710 software update: 2015/8/13
//file: header file
//version: v3.6
//***********************************************
#ifndef  _RT710_H_ 
#define _RT710_H_

#include "../basic_types.h"
#include "../porting_linux_header.h"

//#define UINT8 unsigned char
//#define UINT16 unsigned int
//#define UINT32 unsigned long


#define VERSION   "RT710(720)_GUI_v3.6"
#define RT710_DEVICE_ADDRESS	0xF4
#define RT710_Reg_Num	16

//Xtal Frequency Support 16MHz(16000), 27MHz(27000), 28.8MHz(28800), 24MHz(24000) 
#define RT710_Xtal	27000

#define RT710_0DBM_SETTING 	FALSE
//#define RT710_0DBM_SETTING 	TRUE  

typedef enum _RT710_Err_Type
{
	RT_Success = TRUE,
	RT_Fail    = FALSE
}RT710_Err_Type;

typedef enum _RT710_LoopThrough_Type
{
	LOOP_THROUGH = TRUE,
	SIGLE_IN     = FALSE
}RT710_LoopThrough_Type;

typedef enum _RT710_ClockOut_Type
{
	ClockOutOn = TRUE,
	ClockOutOff= FALSE
}RT710_ClockOut_Type;

typedef enum _RT710_OutputSignal_Type
{
	DifferentialOut = TRUE,
	SingleOut     = FALSE
}RT710_OutputSignal_Type;

typedef enum _RT710_AGC_Type
{
	AGC_Negative = TRUE,
	AGC_Positive = FALSE
}RT710_AGC_Type;

typedef enum _RT710_AttenVga_Type
{
	ATTENVGAON = TRUE,
	ATTENVGAOFF= FALSE
}RT710_AttenVga_Type;

typedef enum _R710_FineGain_Type
{
	FINEGAIN_3DB = 0,
	FINEGAIN_2DB,
	FINEGAIN_1DB,
	FINEGAIN_0DB
}R710_FineGain_Type;

typedef enum _RT710_RollOff_Type
{
	ROLL_OFF_0_15 = 0,	//roll-off = 0.15
	ROLL_OFF_0_20,		//roll-off = 0.2
	ROLL_OFF_0_25,		//roll-off = 0.25
	ROLL_OFF_0_30,		//roll-off = 0.3
	ROLL_OFF_0_35,		//roll-off = 0.35
	ROLL_OFF_0_40,		//roll-off = 0.4
}RT710_RollOff_Type;


typedef enum _RT710_Scan_Type
{
	AUTO_SCAN = TRUE,	//Blind Scan 
	MANUAL_SCAN= FALSE	//Normal(Default)
}RT710_Scan_Type;

typedef struct _RT710_RF_Gain_Info
{
	UINT8   RF_gain;
}RT710_RF_Gain_Info;

typedef struct _RT710_INFO_Type
{
	UINT32 RF_KHz;
	UINT32 SymbolRate_Kbps;
	RT710_RollOff_Type RT710_RollOff_Mode;
	RT710_LoopThrough_Type RT710_LoopThrough_Mode;
	RT710_ClockOut_Type RT710_ClockOut_Mode;
	RT710_OutputSignal_Type RT710_OutputSignal_Mode;
	RT710_AGC_Type RT710_AGC_Mode;
	RT710_AttenVga_Type RT710_AttenVga_Mode;
	R710_FineGain_Type R710_FineGain;
	RT710_Scan_Type RT710_Scan_Mode;	//only support RT720
}RT710_INFO_Type;

typedef enum _TUNER_NUM
{
	RT710_TUNER_1 = 0,
	RT710_TUNER_2,
	RT710_TUNER_3,
	RT710_TUNER_4,
	MAX_TUNER_NUM
}RT710_TUNER_NUM_TYPE;

typedef struct _I2C_LEN_TYPE
{
	UINT8 Data[56];
	UINT8 RegAddr;
	UINT8 Len;
}I2C_LEN_TYPE;

typedef struct _I2C_TYPE
{
	UINT8 RegAddr;
	UINT8 Data;
}I2C_TYPE;


//----------------------------------------------------------//
//                   RT710 Function                         //
//----------------------------------------------------------//
RT710_Err_Type RT710_Setting(UINT32 tuner_id, RT710_INFO_Type RT710_Set_Info);
RT710_Err_Type RT710_Standby(UINT32 tuner_id, RT710_LoopThrough_Type RT710_LTSel, RT710_ClockOut_Type RT710_CLKSel);
RT710_Err_Type RT710_GetRfGain(UINT32 tuner_id,RT710_RF_Gain_Info *RT710_rf_gain);
RT710_Err_Type RT710_GetRfRssi(UINT32 tuner_id, UINT32 RF_Freq_Khz, INT32 *RfLevelDbm, UINT8 *fgRfGainflag);
UINT8 RT710_PLL_Lock(UINT32 tuner_id);
RT710_Err_Type RT710_PLL(UINT32 tuner_id, UINT32 PLL_Freq);
RT710_Err_Type RT710_init(UINT32 tuner_id, RT710_INFO_Type RT710_Set_Info);


extern UINT8 R710_Initial_done_flag;
extern UINT8 Chip_type_flag;
extern UINT8 RT710_Reg_Arry_Init[RT710_Reg_Num] ;
extern UINT8 RT710_ADDRESS;
extern UINT8 R0TOR3_Write_Arry[4];
#endif


