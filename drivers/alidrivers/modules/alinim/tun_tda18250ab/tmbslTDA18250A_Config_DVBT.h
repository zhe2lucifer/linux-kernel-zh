/* 
* NXP TDA18250AB silicon tuner driver 
* 
* Copyright (C) 2016 <Alexandre TANT> 
* 
*    This program is free software; you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation; either version 2 of the License, or 
*    (at your option) any later version. 
* 
*    This program is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
* 
*    You should have received a copy of the GNU General Public License along 
*    with this program; if not, write to the Free Software Foundation, Inc., 
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
*/ 

#ifndef _TDA18250A_CONFIG_DVBT_H
#define _TDA18250A_CONFIG_DVBT_H

#ifdef __cplusplus
extern "C"
{
#endif

    /*============================================================================*/
    /* Types and defines:                                                         */
    /*============================================================================*/

    /* Driver settings version definition */
#define TDA18250A_DVBT_SETTINGS_CUSTOMER_NUM      (0)                     /* SW Settings Customer Number */
#define TDA18250A_DVBT_SETTINGS_PROJECT_NUM       (0)                     /* SW Settings Project Number  */
#define TDA18250A_DVBT_SETTINGS_MAJOR_VER         (1)                     /* SW Settings Major Version   */
#define TDA18250A_DVBT_SETTINGS_MINOR_VER         (4)                     /* SW Settings Minor Version   */

#define TDA18250A_CONFIG_STD_DVBT_1_7MHZ \
    {																		/* DVB-T/T2 1.7MHz */	  \
    /****************************************************************/							  \
    /* IF Settings                                                  */							  \
    /****************************************************************/							  \
    850000,																/* IF */				  \
    0,																	/* CF_Offset */			  \
    \
    /****************************************************************/							  \
    /* IF SELECTIVITY Settings                                      */							  \
    /****************************************************************/							  \
    TDA18250A_LPF_1_5MHz,												/* LPF */				  \
    TDA18250A_LPFOffset_plus_4pc,										/* LPF_Offset */		  \
    TDA18250A_DC_Notch_IF_PPF_Disabled,									/* DC notch IF PPF */	  \
    TDA18250A_HPF_Disabled,												/* Hi Pass */			  \
    TDA18250A_HPFOffset_0pc,											/* HPF Offset */		  \
    TDA18250A_IF_Notch_Disabled,										/* IF notch */			  \
    TDA18250A_IF_Notch_Freq_6_25MHz,									/* IF Notch Freq */		  \
    TDA18250A_IF_Notch_Offset_0pc,										/* IF Notch Offset */     \
    TDA18250A_IFnotchToRSSI_Enabled,									/* IF notch To RSSI */    \
    \
    /****************************************************************/							  \
    /* AGC Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250AAGC1_GAIN_Free,                                            /* AGC1 GAIN */           \
    TDA18250AAGC1_GAIN_SMOOTH_ALGO_Enabled,                             /* AGC1 GAIN SMOOTH ALGO */ \
    {                                                                   /* AGC1 TOP DN/UP ES1 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES2 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES3 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_AGC1_TOP_STRATEGY_4,										/* AGC1 TOP STRATEGY 0 */ \
    TDA18250A_AGC1_DET_SPEED_125KHz,									/* AGC1 Det Speed */	  \
    TDA18250A_AGC1_SMOOTH_T_CST_1_6ms,                                  /* AGC1 Smooth T Cst */   \
    TDA18250A_AGC1_Do_Step_0_512ms,                                      /* AGC1 Do Step  */                \
    TDA18250A_LNA_ZIN_NF,												/* LNA_Zin */			  \
    True,                                                               /* AGC2 Gain Control En ES1 */ \
    False,                                                              /* AGC2 Gain Control En ES2 */ \
    True,                                                               /* AGC2 Gain Control En ES3 */ \
    {                                                                   /* AGC2  TOP ES1 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)99,                                                                                   \
            (UInt8)97                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)102,                                                                                  \
            (UInt8)100                                                                                   \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC2 TOP ES2 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)103,                                                                                   \
            (UInt8)102                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)106,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                         \
    {                                                                   /* AGC2 TOP ES3 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)97,                                                                                   \
            (UInt8)96                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)99,                                                                                  \
            (UInt8)98                                                                                   \
        }                                                                                         \
    },                                                                                         \
    TDA18250A_AGC2_DET_SPEED_125KHz,									/* AGC2 Det Speed */	  \
    False,                                                              /* AGC2 Adapt TOP23 Enable ES2 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES2 */     \
    False,                                                              /* AGC2 Adapt TOP23 Enable ES3 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES3 */     \
    True,                                                               /* AGC2 Gain Control Speed (False 1ms ; True 0.5ms)   */     \
    TDA18250A_AGC2_Do_Step_0_512ms,                                      /* AGC2 Do Step  */                \
    TDA18250A_AGC2_Up_Step_23_8,                                         /* AGC2 Up Step  */                \
    TDA18250A_AGC2_Up_Udld_Step_23_8,                                    /* AGC2 Up Udld Step */ \
    -8,                                                                 /* AGC2 TOP Up Udld */ \
    5,                                                                  /* AGC2 Fast Auto Delta */ \
    TDA18250A_DET12_CINT_50fF,											/* Det12 Cint */		  \
    {                                                                   /* AGC3 TOP */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)112,                                                                                  \
            (UInt8)107                                                                                   \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)110,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_IF_Output_Level_1Vpp_min_6_24dB,							/* IF Output Level */	  \
    {                                                                   /* S2D Gain */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            TDA18250A_S2D_Gain_9dB                                                                \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            TDA18250A_S2D_Gain_6dB                                                                \
        }                                                                                         \
    },                                                                                            \
    \
    True,                                                                   /* Fast Auto Launch */          \
    TDA18250A_AGC_Timer_Mod_Fast_Auto_01,                                   /* AGC Timer Mod Fast Auto */   \
    False,                                                                  /* AGC2 Up Udld Fast Reduce */  \
    False,                                                                  /* AGC2 Up Fast Reduce */       \
    TDA18250A_AGC_Type_Fast_Auto_10,                                        /* AGC Type Fast Auto */        \
    /****************************************************************/							  \
    /* GSK Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_AGCK_Time_Constant_32_768ms,								/* AGCK Time Constant */  \
    TDA18250A_RSSI_HP_FC_2_25M,											/* RSSI HP FC */		  \
    \
    /****************************************************************/							  \
    /* H3H5 Settings                                                */							  \
    /****************************************************************/							  \
    TDA18250A_VHF_III_Mode_Disabled,										/* VHF III Mode */		  \
    \
    /****************************************************************/							  \
    /*RSSI Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_RSSI_CAP_VAL_3pF,											/* RSSI_Ck_speed */		  \
    TDA18250A_RSSI_CK_SPEED_31_25kHz,									/* RSSI_Cap_Val */		  \
    \
    0x44,                                                               /* ES1 Power Saving Byte 1 */ \
    0x06,                                                               /* ES1 Power Saving Byte 2 */ \
    0x07,                                                               /* ES1 Power Saving Byte 3 */ \
    0x88,                                                               /* ES2 Power Saving Byte 1 */ \
    0x42,                                                               /* ES2 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES2 Power Saving Byte 3 */ \
    0x88,                                                               /* ES3 Power Saving Byte 1 */ \
    0x42,                                                               /* ES3 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES3 Power Saving Byte 3 */ \
}

#define TDA18250A_CONFIG_STD_DVBT_6MHZ \
    {																		/* QAM 6MHz */			  \
    /****************************************************************/							  \
    /* IF Settings                                                  */							  \
    /****************************************************************/							  \
    4000000,															/* IF */				  \
    0,																	/* CF_Offset */			  \
    \
    /****************************************************************/							  \
    /* IF SELECTIVITY Settings                                      */							  \
    /****************************************************************/							  \
    TDA18250A_LPF_7MHz,													/* LPF */				  \
    TDA18250A_LPFOffset_min_8pc,										/* LPF_Offset */		  \
    TDA18250A_DC_Notch_IF_PPF_Disabled,									/* DC notch IF PPF */	  \
    TDA18250A_HPF_1MHz,												/* Hi Pass */			  \
    TDA18250A_HPFOffset_0pc,											/* HPF Offset */		  \
    TDA18250A_IF_Notch_Enabled,											/* IF notch */			  \
    TDA18250A_IF_Notch_Freq_7_25MHz,									/* IF Notch Freq */		  \
    TDA18250A_IF_Notch_Offset_0pc,									/* IF Notch Offset */	  \
    TDA18250A_IFnotchToRSSI_Enabled,									/* IF notch To RSSI */	  \
    \
    /****************************************************************/							  \
    /* AGC Settings                                                 */						      \
    /****************************************************************/							  \
    TDA18250AAGC1_GAIN_Free,                                            /* AGC1 GAIN */           \
    TDA18250AAGC1_GAIN_SMOOTH_ALGO_Enabled,                             /* AGC1 GAIN SMOOTH ALGO */ \
    {                                                                   /* AGC1 TOP DN/UP ES1 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES2 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES3 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_AGC1_TOP_STRATEGY_4,										/* AGC1 TOP STRATEGY 0 */ \
    TDA18250A_AGC1_DET_SPEED_125KHz,									/* AGC1 Det Speed */	  \
    TDA18250A_AGC1_SMOOTH_T_CST_1_6ms,                                  /* AGC1 Smooth T Cst */   \
    TDA18250A_AGC1_Do_Step_0_512ms,                                      /* AGC1 Do Step  */                \
    TDA18250A_LNA_ZIN_NF,												/* LNA_Zin */			  \
    True,                                                               /* AGC2 Gain Control En ES1 */ \
    False,                                                              /* AGC2 Gain Control En ES2 */ \
    True,                                                              /* AGC2 Gain Control En ES3 */ \
    {                                                                   /* AGC2  TOP ES1 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)99,                                                                                   \
            (UInt8)97                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)102,                                                                                  \
            (UInt8)100                                                                                   \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC2 TOP ES2 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)103,                                                                                   \
            (UInt8)102                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)106,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                         \
    {                                                                   /* AGC2 TOP ES3 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)97,                                                                                   \
            (UInt8)96                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)99,                                                                                  \
            (UInt8)98                                                                                   \
        }                                                                                         \
    },                                                                                         \
    TDA18250A_AGC2_DET_SPEED_125KHz,									/* AGC2 Det Speed */	  \
    False,                                                              /* AGC2 Adapt TOP23 Enable ES2 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES2 */     \
    True,                                                              /* AGC2 Adapt TOP23 Enable ES3 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES3 */     \
    True,                                                               /* AGC2 Gain Control Speed (False 1ms ; True 0.5ms)   */     \
    TDA18250A_AGC2_Do_Step_0_512ms,                                      /* AGC2 Do Step  */                \
    TDA18250A_AGC2_Up_Step_23_8,                                         /* AGC2 Up Step  */                \
    TDA18250A_AGC2_Up_Udld_Step_23_8,                                    /* AGC2 Up Udld Step */ \
    -8,                                                                 /* AGC2 TOP Up Udld */ \
    5,                                                                  /* AGC2 Fast Auto Delta */ \
    TDA18250A_DET12_CINT_50fF,											/* Det12 Cint */		  \
    {                                                                   /* AGC3 TOP */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)112,                                                                                  \
            (UInt8)107                                                                                   \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)110,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_IF_Output_Level_1Vpp_min_6_24dB,							/* IF Output Level */     \
    {                                                                   /* S2D Gain */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            TDA18250A_S2D_Gain_9dB                                                                \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            TDA18250A_S2D_Gain_6dB                                                                \
        }                                                                                          \
    },                                                                                            \
    \
    True,                                                                   /* Fast Auto Launch */          \
    TDA18250A_AGC_Timer_Mod_Fast_Auto_01,                                   /* AGC Timer Mod Fast Auto */   \
    False,                                                                  /* AGC2 Up Udld Fast Reduce */  \
    False,                                                                  /* AGC2 Up Fast Reduce */       \
    TDA18250A_AGC_Type_Fast_Auto_10,                                        /* AGC Type Fast Auto */        \
    /****************************************************************/							  \
    /* GSK Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_AGCK_Time_Constant_32_768ms,								/* AGCK Time Constant */  \
    TDA18250A_RSSI_HP_FC_2_25M,											/* RSSI HP FC */		  \
    \
    /****************************************************************/							  \
    /* H3H5 Settings                                                */							  \
    /****************************************************************/							  \
    TDA18250A_VHF_III_Mode_Disabled,										/* VHF III Mode */		  \
    \
    /****************************************************************/							  \
    /*RSSI Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_RSSI_CAP_VAL_3pF,											/* RSSI_Ck_speed */		  \
    TDA18250A_RSSI_CK_SPEED_31_25kHz,									/* RSSI_Cap_Val */		  \
    \
    0x44,                                                               /* ES1 Power Saving Byte 1 */ \
    0x06,                                                               /* ES1 Power Saving Byte 2 */ \
    0x07,                                                               /* ES1 Power Saving Byte 3 */ \
    0x88,                                                               /* ES2 Power Saving Byte 1 */ \
    0x42,                                                               /* ES2 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES2 Power Saving Byte 3 */ \
    0x88,                                                               /* ES3 Power Saving Byte 1 */ \
    0x42,                                                               /* ES3 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES3 Power Saving Byte 3 */ \
}

#define TDA18250A_CONFIG_STD_DVBT_7MHZ \
    {																		/* DVB-T/T2 7MHz */		  \
    /****************************************************************/							  \
    /* IF Settings                                                  */							  \
    /****************************************************************/							  \
    4500000,															/* IF */				  \
    0,																	/* CF_Offset */			  \
    \
    /****************************************************************/							  \
    /* IF SELECTIVITY Settings                                      */							  \
    /****************************************************************/							  \
    TDA18250A_LPF_8MHz,													/* LPF */				  \
    TDA18250A_LPFOffset_min_8pc,										/* LPF_Offset */		  \
    TDA18250A_DC_Notch_IF_PPF_Disabled,									/* DC notch IF PPF */	  \
    TDA18250A_HPF_1MHz,												/* Hi Pass */			  \
    TDA18250A_HPFOffset_0pc,											/* HPF Offset */		  \
    TDA18250A_IF_Notch_Enabled,											/* IF notch */			  \
    TDA18250A_IF_Notch_Freq_8_25MHz,									/* IF Notch Freq */		  \
    TDA18250A_IF_Notch_Offset_0pc,										/* IF Notch Offset */	  \
    TDA18250A_IFnotchToRSSI_Enabled,									/* IF notch To RSSI */    \
    \
    /****************************************************************/							  \
    /* AGC Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250AAGC1_GAIN_Free,                                            /* AGC1 GAIN */           \
    TDA18250AAGC1_GAIN_SMOOTH_ALGO_Enabled,                             /* AGC1 GAIN SMOOTH ALGO */ \
    {                                                                   /* AGC1 TOP DN/UP ES1 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES2 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES3 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_AGC1_TOP_STRATEGY_4,										/* AGC1 TOP STRATEGY 0 */ \
    TDA18250A_AGC1_DET_SPEED_125KHz,									/* AGC1 Det Speed */	  \
    TDA18250A_AGC1_SMOOTH_T_CST_1_6ms,                                  /* AGC1 Smooth T Cst */   \
    TDA18250A_AGC1_Do_Step_0_512ms,                                      /* AGC1 Do Step  */                \
    TDA18250A_LNA_ZIN_NF,												/* LNA_Zin */			  \
    True,                                                               /* AGC2 Gain Control En ES1 */ \
    False,                                                              /* AGC2 Gain Control En ES2 */ \
    True,                                                              /* AGC2 Gain Control En ES3 */ \
    {                                                                   /* AGC2  TOP ES1 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)99,                                                                                   \
            (UInt8)97                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)102,                                                                                  \
            (UInt8)100                                                                                   \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC2 TOP ES2 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)100,                                                                                  \
            (UInt8)99                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                             \
            (UInt8)102,                                                                                   \
            (UInt8)101                                                                                    \
        }                                                                                         \
    },                                                                                         \
    {                                                                   /* AGC2 TOP ES3 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)97,                                                                                  \
            (UInt8)96                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                             \
            (UInt8)99,                                                                                   \
            (UInt8)98                                                                                    \
        }                                                                                         \
    },                                                                                         \
    TDA18250A_AGC2_DET_SPEED_125KHz,									/* AGC2 Det Speed */	  \
    False,                                                              /* AGC2 Adapt TOP23 Enable ES2 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES2 */     \
    True,                                                              /* AGC2 Adapt TOP23 Enable ES3 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES3 */     \
    True,                                                               /* AGC2 Gain Control Speed (False 1ms ; True 0.5ms)   */     \
    TDA18250A_AGC2_Do_Step_0_512ms,                                      /* AGC2 Do Step  */                \
    TDA18250A_AGC2_Up_Step_23_8,                                         /* AGC2 Up Step  */                \
    TDA18250A_AGC2_Up_Udld_Step_23_8,                                    /* AGC2 Up Udld Step */ \
    -8,                                                                 /* AGC2 TOP Up Udld */ \
    5,                                                                  /* AGC2 Fast Auto Delta */ \
    TDA18250A_DET12_CINT_50fF,											/* Det12 Cint */		  \
    {                                                                   /* AGC3 TOP */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)112,                                                                                  \
            (UInt8)107                                                                                   \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)110,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_IF_Output_Level_1Vpp_min_6_24dB,							/* IF Output Level */	  \
    {                                                                   /* S2D Gain */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            TDA18250A_S2D_Gain_9dB                                                                \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            TDA18250A_S2D_Gain_6dB                                                                \
        }                                                                                          \
    },                                                                                            \
    \
    True,                                                                   /* Fast Auto Launch */          \
    TDA18250A_AGC_Timer_Mod_Fast_Auto_01,                                   /* AGC Timer Mod Fast Auto */   \
    False,                                                                  /* AGC2 Up Udld Fast Reduce */  \
    False,                                                                  /* AGC2 Up Fast Reduce */       \
    TDA18250A_AGC_Type_Fast_Auto_10,                                        /* AGC Type Fast Auto */        \
    /****************************************************************/							  \
    /* GSK Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_AGCK_Time_Constant_32_768ms,								/* AGCK Time Constant */  \
    TDA18250A_RSSI_HP_FC_2_25M,											/* RSSI HP FC */		  \
    \
    /****************************************************************/							  \
    /* H3H5 Settings                                                */							  \
    /****************************************************************/							  \
    TDA18250A_VHF_III_Mode_Disabled,										/* VHF III Mode */		  \
    \
    /****************************************************************/							  \
    /*RSSI Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_RSSI_CAP_VAL_3pF,											/* RSSI_Ck_speed */		  \
    TDA18250A_RSSI_CK_SPEED_31_25kHz,									/* RSSI_Cap_Val */		  \
    \
    0x44,                                                               /* ES1 Power Saving Byte 1 */ \
    0x06,                                                               /* ES1 Power Saving Byte 2 */ \
    0x07,                                                               /* ES1 Power Saving Byte 3 */ \
    0x88,                                                               /* ES2 Power Saving Byte 1 */ \
    0x42,                                                               /* ES2 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES2 Power Saving Byte 3 */ \
    0x88,                                                               /* ES3 Power Saving Byte 1 */ \
    0x42,                                                               /* ES3 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES3 Power Saving Byte 3 */ \
}

#define TDA18250A_CONFIG_STD_DVBT_8MHZ \
    {																		/* DVB-T/T2 8MHz */		  \
    /****************************************************************/							  \
    /* IF Settings                                                  */							  \
    /****************************************************************/							  \
    5000000,															/* IF */				  \
    0,																	/* CF_Offset */			  \
    \
    /****************************************************************/							  \
    /* IF SELECTIVITY Settings                                      */							  \
    /****************************************************************/							  \
    TDA18250A_LPF_9MHz,													/* LPF */				  \
    TDA18250A_LPFOffset_min_8pc,										/* LPF_Offset */		  \
    TDA18250A_DC_Notch_IF_PPF_Disabled,									/* DC notch IF PPF */	  \
    TDA18250A_HPF_1MHz,                                                 /* Hi Pass */			  \
    TDA18250A_HPFOffset_0pc,											/* HPF Offset */		  \
    TDA18250A_IF_Notch_Enabled,											/* IF notch */			  \
    TDA18250A_IF_Notch_Freq_9_25MHz,									/* IF Notch Freq */		  \
    TDA18250A_IF_Notch_Offset_0pc,										/* IF Notch Offset */	  \
    TDA18250A_IFnotchToRSSI_Enabled,									/* IF notch To RSSI */	  \
    \
    /****************************************************************/							  \
    /* AGC Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250AAGC1_GAIN_Free,                                            /* AGC1 GAIN */           \
    TDA18250AAGC1_GAIN_SMOOTH_ALGO_Enabled,                             /* AGC1 GAIN SMOOTH ALGO */ \
    {                                                                   /* AGC1 TOP DN/UP ES1 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES2 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES3 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_AGC1_TOP_STRATEGY_4,										/* AGC1 TOP STRATEGY 0 */ \
    TDA18250A_AGC1_DET_SPEED_125KHz,									/* AGC1 Det Speed */	  \
    TDA18250A_AGC1_SMOOTH_T_CST_51_2ms,                                 /* AGC1 Smooth T Cst */   \
    TDA18250A_AGC1_Do_Step_0_512ms,                                      /* AGC1 Do Step  */                \
    TDA18250A_LNA_ZIN_NF,												/* LNA_Zin */			  \
    True,                                                               /* AGC2 Gain Control En ES1 */ \
    False,                                                              /* AGC2 Gain Control En ES2 */ \
    True,                                                              /* AGC2 Gain Control En ES3 */ \
    {                                                                   /* AGC2 TOP ES1 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)103,                                                                                  \
            (UInt8)102                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)-1,                                                                                   \
            (UInt8)-1,                                                                                   \
            (UInt8)-1                                                                                    \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC2 TOP ES2 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)100,                                                                                  \
            (UInt8)99                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                             \
            (UInt8)102,                                                                                   \
            (UInt8)101                                                                                    \
        }                                                                                         \
    },                                                                                             \
    {                                                                   /* AGC2 TOP ES3 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)97,                                                                                  \
            (UInt8)96                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                             \
            (UInt8)98,                                                                                   \
            (UInt8)97                                                                                    \
        }                                                                                         \
    },                                                                                         \
    TDA18250A_AGC2_DET_SPEED_125KHz,									/* AGC2 Det Speed */	  \
    False,                                                              /* AGC2 Adapt TOP23 Enable ES2 */     \
    7,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES2 */     \
    True,                                                              /* AGC2 Adapt TOP23 Enable ES3 */     \
    7,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES3 */     \
    True,                                                               /* AGC2 Gain Control Speed (False 1ms ; True 0.5ms)   */     \
    TDA18250A_AGC2_Do_Step_0_512ms,                                      /* AGC2 Do Step  */                \
    TDA18250A_AGC2_Up_Step_131ms,                                         /* AGC2 Up Step  */                \
    TDA18250A_AGC2_Up_Udld_Step_23_8,                                    /* AGC2 Up Udld Step */ \
    -8,                                                                 /* AGC2 TOP Up Udld */ \
    5,                                                                  /* AGC2 Fast Auto Delta */ \
    TDA18250A_DET12_CINT_50fF,											/* Det12 Cint */		  \
    {                                                                   /* AGC3 TOP */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)112,                                                                                  \
            (UInt8)107                                                                                   \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                                   \
            (UInt8)110,                                                                                   \
            (UInt8)105                                                                                    \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_IF_Output_Level_1Vpp_min_6_24dB,							/* IF Output Level */	  \
    {                                                                   /* S2D Gain */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            TDA18250A_S2D_Gain_9dB                                                                \
        },                                                                                        \
        {                                                                                         \
            (UInt32)-1,                                                                                   \
            TDA18250A_S2D_Gain_6dB                                                                \
        }                                                                                         \
    },                                                                                            \
    \
    True,                                                                   /* Fast Auto Launch */          \
    TDA18250A_AGC_Timer_Mod_Fast_Auto_01,                                   /* AGC Timer Mod Fast Auto */   \
    False,                                                                  /* AGC2 Up Udld Fast Reduce */  \
    False,                                                                  /* AGC2 Up Fast Reduce */       \
    TDA18250A_AGC_Type_Fast_Auto_10,                                        /* AGC Type Fast Auto */        \
    /****************************************************************/							  \
    /* GSK Settings                                                 */						      \
    /****************************************************************/							  \
    TDA18250A_AGCK_Time_Constant_32_768ms,								/* AGCK Time Constant */  \
    TDA18250A_RSSI_HP_FC_2_25M,											/* RSSI HP FC */		  \
    \
    /****************************************************************/							  \
    /* H3H5 Settings                                                */							  \
    /****************************************************************/							  \
    TDA18250A_VHF_III_Mode_Disabled,										/* VHF III Mode */		  \
    \
    /****************************************************************/							  \
    /*RSSI Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_RSSI_CAP_VAL_3pF,											/* RSSI_Ck_speed */		  \
    TDA18250A_RSSI_CK_SPEED_31_25kHz,									/* RSSI_Cap_Val */		  \
    \
    0x44,                                                               /* ES1 Power Saving Byte 1 */ \
    0x06,                                                               /* ES1 Power Saving Byte 2 */ \
    0x07,                                                               /* ES1 Power Saving Byte 3 */ \
    0x88,                                                               /* ES2 Power Saving Byte 1 */ \
    0x42,                                                               /* ES2 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES2 Power Saving Byte 3 */ \
    0x88,                                                               /* ES3 Power Saving Byte 1 */ \
    0x42,                                                               /* ES3 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES3 Power Saving Byte 3 */ \
}

#define TDA18250A_CONFIG_STD_DVBT_10MHZ \
    {																		/* DVB-T/T2 10MHz */	  \
    /****************************************************************/							  \
    /* IF Settings                                                  */							  \
    /****************************************************************/							  \
    6000000,															/* IF */                  \
    0,																	/* CF_Offset */			  \
    \
    /****************************************************************/							  \
    /* IF SELECTIVITY Settings                                      */							  \
    /****************************************************************/							  \
    TDA18250A_LPF_11_2MHz,												/* LPF */				  \
    TDA18250A_LPFOffset_0pc,										/* LPF_Offset */		  \
    TDA18250A_DC_Notch_IF_PPF_Enabled,									/* DC notch IF PPF */	  \
    TDA18250A_HPF_1MHz,												/* Hi Pass */			  \
    TDA18250A_HPFOffset_0pc,											/* HPF Offset */		  \
    TDA18250A_IF_Notch_Enabled,											/* IF notch */			  \
    TDA18250A_IF_Notch_Freq_11_45MHz,									/* IF Notch Freq */		  \
    TDA18250A_IF_Notch_Offset_0pc,										/* IF Notch Offset */	  \
    TDA18250A_IFnotchToRSSI_Enabled,									/* IF notch To RSSI */	  \
    \
    /****************************************************************/							  \
    /* AGC Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250AAGC1_GAIN_Free,                                            /* AGC1 GAIN */           \
    TDA18250AAGC1_GAIN_SMOOTH_ALGO_Enabled,                             /* AGC1 GAIN SMOOTH ALGO */ \
    {                                                                   /* AGC1 TOP DN/UP ES1 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES2 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC1 TOP DN/UP ES3 */  \
        {                                                                                         \
            (UInt32)0,                                                                            \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        },                                                                                        \
        {                                                                                         \
            (UInt32)100000000,                                                                    \
            TDA18250A_AGC1_TOP_I2C_DN_UP_d97_u91dBuV                                              \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_AGC1_TOP_STRATEGY_4,										/* AGC1 TOP STRATEGY 0 */ \
    TDA18250A_AGC1_DET_SPEED_125KHz,									/* AGC1 Det Speed */	  \
    TDA18250A_AGC1_SMOOTH_T_CST_1_6ms,                                  /* AGC1 Smooth T Cst */   \
    TDA18250A_AGC1_Do_Step_0_512ms,                                      /* AGC1 Do Step  */                \
    TDA18250A_LNA_ZIN_NF,												/* LNA_Zin */			  \
    True,                                                               /* AGC2 Gain Control En ES1 */ \
    False,                                                              /* AGC2 Gain Control En ES2 */ \
    True,                                                              /* AGC2 Gain Control En ES3 */ \
    {                                                                   /* AGC2  TOP ES1 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)99,                                                                                   \
            (UInt8)97                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)102,                                                                                  \
            (UInt8)100                                                                                   \
        }                                                                                         \
    },                                                                                            \
    {                                                                   /* AGC2 TOP ES2 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)103,                                                                                   \
            (UInt8)102                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)106,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                         \
    {                                                                   /* AGC2 TOP ES3 */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)97,                                                                                   \
            (UInt8)96                                                                                    \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)99,                                                                                  \
            (UInt8)98                                                                                  \
        }                                                                                         \
    },                                                                                         \
    TDA18250A_AGC2_DET_SPEED_125KHz,									/* AGC2 Det Speed */	  \
    False,                                                              /* AGC2 Adapt TOP23 Enable ES2 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES2 */     \
    True,                                                              /* AGC2 Adapt TOP23 Enable ES3 */     \
    0,                                                                  /* AGC2 Adapt TOP23 Delta (in dB) ES3 */     \
    True,                                                               /* AGC2 Gain Control Speed (False 1ms ; True 0.5ms)   */     \
    TDA18250A_AGC2_Do_Step_0_512ms,                                      /* AGC2 Do Step  */                \
    TDA18250A_AGC2_Up_Step_23_8,                                         /* AGC2 Up Step  */                \
    TDA18250A_AGC2_Up_Udld_Step_23_8,                                    /* AGC2 Up Udld Step */ \
    -8,                                                                 /* AGC2 TOP Up Udld */ \
    5,                                                                  /* AGC2 Fast Auto Delta */ \
    TDA18250A_DET12_CINT_50fF,											/* Det12 Cint */		  \
    {                                                                   /* AGC3 TOP */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            (UInt8)112,                                                                                  \
            (UInt8)107                                                                                   \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            (UInt8)110,                                                                                  \
            (UInt8)105                                                                                   \
        }                                                                                         \
    },                                                                                            \
    TDA18250A_IF_Output_Level_1Vpp_min_6_24dB,							/* IF Output Level */	  \
    {                                                                   /* S2D Gain */            \
        {                                                                                         \
            (UInt32)0,                                                                                    \
            TDA18250A_S2D_Gain_9dB                                                                \
        },                                                                                        \
        {                                                                                         \
            (UInt32)320000000,                                                                            \
            TDA18250A_S2D_Gain_6dB                                                                \
        }                                                                                          \
    },                                                                                            \
    \
    True,                                                                   /* Fast Auto Launch */          \
    TDA18250A_AGC_Timer_Mod_Fast_Auto_01,                                   /* AGC Timer Mod Fast Auto */   \
    False,                                                                  /* AGC2 Up Udld Fast Reduce */  \
    False,                                                                  /* AGC2 Up Fast Reduce */       \
    TDA18250A_AGC_Type_Fast_Auto_10,                                        /* AGC Type Fast Auto */        \
    /****************************************************************/							  \
    /* GSK Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_AGCK_Time_Constant_32_768ms,								/* AGCK Time Constant */  \
    TDA18250A_RSSI_HP_FC_2_25M,											/* RSSI HP FC */		  \
    \
    /****************************************************************/							  \
    /* H3H5 Settings                                                */							  \
    /****************************************************************/							  \
    TDA18250A_VHF_III_Mode_Disabled,										/* VHF III Mode */		  \
    \
    /****************************************************************/							  \
    /*RSSI Settings                                                 */							  \
    /****************************************************************/							  \
    TDA18250A_RSSI_CAP_VAL_3pF,											/* RSSI_Ck_speed */		  \
    TDA18250A_RSSI_CK_SPEED_31_25kHz,									/* RSSI_Cap_Val */		  \
    \
    0x44,                                                               /* ES1 Power Saving Byte 1 */ \
    0x06,                                                               /* ES1 Power Saving Byte 2 */ \
    0x07,                                                               /* ES1 Power Saving Byte 3 */ \
    0x88,                                                               /* ES2 Power Saving Byte 1 */ \
    0x42,                                                               /* ES2 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES2 Power Saving Byte 3 */ \
    0x88,                                                               /* ES3 Power Saving Byte 1 */ \
    0x42,                                                               /* ES3 Power Saving Byte 2 */ \
    0x0B,                                                               /* ES3 Power Saving Byte 3 */ \
}

#ifdef __cplusplus
}
#endif

#endif /* _TDA18250A_CONFIG_DVBT_H */

