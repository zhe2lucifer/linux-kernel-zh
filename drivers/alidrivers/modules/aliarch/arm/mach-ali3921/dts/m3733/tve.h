
#ifndef __DTS_TVE_H
#define __DTS_TVE_H

#define	 TVE_COMPOSITE_Y_DELAY                  0x00																																							
#define	 TVE_COMPOSITE_C_DELAY                  0x01																																							
#define	 TVE_COMPOSITE_LUMA_LEVEL 							0x02																																								
#define	 TVE_COMPOSITE_CHRMA_LEVEL							0x03
#define	 TVE_COMPOSITE_SYNC_DELAY               0x04
#define	 TVE_COMPOSITE_SYNC_LEVEL               0x05
#define	 TVE_COMPOSITE_FILTER_C_ENALBE          0x06        
#define	 TVE_COMPOSITE_FILTER_Y_ENALBE          0x07  
#define	 TVE_COMPOSITE_PEDESTAL_LEVEL           0x08  
#define  TVE_COMPONENT_IS_PAL                   0x09        
#define  TVE_COMPONENT_PAL_MODE                 0x0A      
#define	 TVE_COMPONENT_ALL_SMOOTH_ENABLE        0x0B
#define	 TVE_COMPONENT_BTB_ENALBE               0x0C  
#define	 TVE_COMPONENT_INSERT0_ONOFF            0x0D  
#define	 TVE_COMPONENT_DAC_UPSAMPLEN            0x0E  
#define  TVE_COMPONENT_Y_DELAY                  0x0F  																																							
#define	 TVE_COMPONENT_CB_DELAY   							0x10																																
#define	 TVE_COMPONENT_CR_DELAY   							0x11																																
#define	 TVE_COMPONENT_LUM_LEVEL              	0x12																																							          
#define	 TVE_COMPONENT_CHRMA_LEVEL           		0x13																																						          																																				          																													
#define	 TVE_COMPONENT_PEDESTAL_LEVEL       		0x14	
#define	 TVE_COMPONENT_UV_SYNC_ONOFF            0x15
#define	 TVE_COMPONENT_SYNC_DELAY               0x16
#define	 TVE_COMPONENT_SYNC_LEVEL               0x17
#define	 TVE_COMPONENT_R_SYNC_ONOFF             0x18
#define	 TVE_COMPONENT_G_SYNC_ONOFF             0x19
#define	 TVE_COMPONENT_B_SYNC_ONOFF             0x1A
#define	 TVE_COMPONENT_RGB_R_LEVEL              0x1B 
#define	 TVE_COMPONENT_RGB_G_LEVEL              0x1C
#define	 TVE_COMPONENT_RGB_B_LEVEL              0x1D
#define	 TVE_COMPONENT_FILTER_Y_ENALBE          0x1E
#define	 TVE_COMPONENT_FILTER_C_ENALBE          0x1F
#define	 TVE_COMPONENT_PEDESTAL_ONOFF           0x20
#define	 TVE_COMPONENT_PED_RGB_YPBPR_ENABLE     0x21
#define	 TVE_COMPONENT_PED_ADJUST               0x22 
#define	 TVE_COMPONENT_G2Y                      0x23
#define	 TVE_COMPONENT_G2U                      0x24
#define	 TVE_COMPONENT_G2V                      0x25
#define	 TVE_COMPONENT_B2U                      0x26
#define	 TVE_COMPONENT_R2V                      0x27

#define  TVE_BURST_POS_ENABLE                   0x28
#define  TVE_BURST_LEVEL_ENABLE   						  0x29																																	
#define	 TVE_BURST_CB_LEVEL       						  0x2A																																	
#define	 TVE_BURST_CR_LEVEL       							0x2B																																
#define	 TVE_BURST_START_POS                    0x2C
#define	 TVE_BURST_END_POS                      0x2D
#define	 TVE_BURST_SET_FREQ_MODE                0x2E
#define	 TVE_BURST_FREQ_SIGN            				0x2F																															
#define	 TVE_BURST_PHASE_COMPENSATION   				0x30																																		
#define  TVE_BURST_FREQ_RESPONSE  							0x31																																
#define  TVE_ASYNC_FIFO                         0x32
#define  TVE_CAV_SYNC_HIGH                      0x33
#define  TVE_SYNC_HIGH_WIDTH                    0x34
#define  TVE_SYNC_LOW_WIDTH                     0x35
#define  TVE_VIDEO_DAC_FS												0x36				
#define	 TVE_SECAM_PRE_COEFFA3A2 								0x37																																
#define	 TVE_SECAM_PRE_COEFFB1A4  							0x38																																
#define	 TVE_SECAM_PRE_COEFFB3B2  							0x39																																
#define	 TVE_SECAM_F0CB_CENTER    							0x3A																																
#define	 TVE_SECAM_F0CR_CENTER    							0x3B																															
#define	 TVE_SECAM_FM_KCBCR_AJUST 							0x3C																																																					
#define	 TVE_SECAM_CONTROL        							0x3D																																																					
#define	 TVE_SECAM_NOTCH_COEFB1   							0x3E																																																					
#define	 TVE_SECAM_NOTCH_COEFB2B3 							0x3F																																																					
#define	 TVE_SECAM_NOTCH_COEFA2A3 							0x40																																																				
#define  TVE_COMPONENT_PLUG_OUT_EN              0x41
#define  TVE_COMPONENT_PLUG_DETECT_LINE_CNT_HD  0x42
#define  TVE_CB_CR_INSERT_SW                    0x43
#define  TVE_VBI_LINE21_EN                      0x44
#define  TVE_COMPONENT_PLUG_DETECT_AMP_ADJUST_HD 0x45
#define  TVE_SCART_PLUG_DETECT_LINE_CNT_HD      0x46
#define  TVE_SCART_PLUG_DETECT_AMP_ADJUST_HD    0x47
#define  TVE_COMPONENT_PLUG_DETECT_FRAME_COUNT  0x48
#define  TVE_COMPONENT_PLUG_DETECT_VCOUNT       0x49
#define  TVE_COMPONENT_VDAC_ENBUF               0x4A
#define	 TVE_ADJ_FIELD_NUM                      0x4B

//tvencoder adjustable register define
#define TVE_ADJ_COMPOSITE_Y_DELAY       0
#define TVE_ADJ_COMPOSITE_C_DELAY       1
#define TVE_ADJ_COMPONENT_Y_DELAY       2
#define TVE_ADJ_COMPONENT_CB_DELAY      3
#define TVE_ADJ_COMPONENT_CR_DELAY      4
#define TVE_ADJ_BURST_LEVEL_ENABLE      5
#define TVE_ADJ_BURST_CB_LEVEL          6
#define TVE_ADJ_BURST_CR_LEVEL          7
#define TVE_ADJ_COMPOSITE_LUMA_LEVEL    8
#define TVE_ADJ_COMPOSITE_CHRMA_LEVEL   9
#define TVE_ADJ_PHASE_COMPENSATION      10
#define TVE_ADJ_VIDEO_FREQ_RESPONSE     11
//secam adjust value
#define TVE_ADJ_SECAM_PRE_COEFFA3A2    12
#define TVE_ADJ_SECAM_PRE_COEFFB1A4    13
#define TVE_ADJ_SECAM_PRE_COEFFB3B2    14
#define TVE_ADJ_SECAM_F0CB_CENTER      15
#define TVE_ADJ_SECAM_F0CR_CENTER      16
#define TVE_ADJ_SECAM_FM_KCBCR_AJUST   17
#define TVE_ADJ_SECAM_CONTROL          18
#define TVE_ADJ_SECAM_NOTCH_COEFB1     19
#define TVE_ADJ_SECAM_NOTCH_COEFB2B3   20
#define TVE_ADJ_SECAM_NOTCH_COEFA2A3   21
#define TVE_ADJ_VIDEO_DAC_FS			     22
#define TVE_ADJ_C_ROUND_PAR				     23

//advance tvencoder adjustable register define
#define TVE_ADJ_ADV_PEDESTAL_ONOFF              0
#define TVE_ADJ_ADV_COMPONENT_LUM_LEVEL         1
#define TVE_ADJ_ADV_COMPONENT_CHRMA_LEVEL       2
#define TVE_ADJ_ADV_COMPONENT_PEDESTAL_LEVEL    3
#define TVE_ADJ_ADV_COMPONENT_SYNC_LEVEL        4
#define TVE_ADJ_ADV_RGB_R_LEVEL                 5
#define TVE_ADJ_ADV_RGB_G_LEVEL                 6
#define TVE_ADJ_ADV_RGB_B_LEVEL                 7
#define TVE_ADJ_ADV_COMPOSITE_PEDESTAL_LEVEL    8
#define TVE_ADJ_ADV_COMPOSITE_SYNC_LEVEL        9
#define TVE_ADJ_ADV_PLUG_OUT_EN                 10
#define TVE_ADJ_ADV_PLUG_DETECT_LINE_CNT_SD     11
#define TVE_ADJ_ADV_PLUG_DETECT_AMP_ADJUST_SD   12


#define	SYS_525_LINE	                          0
#define	SYS_625_LINE	                          1
#define	SYS_720_LINE	                          2

#define CB_LEVEL_PAL_SD				0x85
#define CB_LEVEL_NTSC_SD      0x85
#define CR_LEVEL_PAL_SD				0x55
#define CR_LEVEL_NTSC_SD			0x55
#define LUMA_LEVEL_PAL_SD			0x52 //0x53
#define LUMA_LEVEL_NTSC_SD		0x4d //0x4f
#define CHRMA_LEVEL_PAL_SD		0x6
#define CHRMA_LEVEL_NTSC_SD		0x6
#define FREQ_RESPONSE_PAL_SD  0x102
#define FREQ_RESPONSE_NTSC_SD	0x102

#endif