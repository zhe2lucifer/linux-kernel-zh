#ifndef __ADF_SYSDEF__
#define __ADF_SYSDEF__
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Hardware and software type coding, used for component ID.
 ****************************************************************************/
#define HW_TYPE_CHIP			0x00010000	/* Chip */
#define HW_TYPE_CHIP_REV		0x00020000	/* Chip Revised */
#define HW_TYPE_CPU				0x00030000	/* CPU */
#define HW_TYPE_ENDIAN			0x00040000	/* CPU endian */
#define HW_TYPE_BOARD			0x00050000	/* Main board type */

#define LLD_DEV_TYPE_DEC		0x02010000	/* Decoder */
#define LLD_DEV_TYPE_DMX		0x02020000	/* DeMUX */
#define LLD_DEV_TYPE_OSD		0x02030000	/* OSD */
#define LLD_DEV_TYPE_NET		0x02040000	/* Network */
#define LLD_DEV_TYPE_NIM		0x02050000	/* NIM */
#define LLD_DEV_TYPE_DEM		0x02050100	/* Demodulator */
#define LLD_DEV_TYPE_TUN		0x02050200	/* Tuner */
#define LLD_DEV_TYPE_LNB		0x02050300	/* LNB */
#define LLD_DEV_TYPE_IRC		0x02060000	/* IR Controller */
#define LLD_DEV_TYPE_PAN		0x02070000	/* Panel */
#define LLD_DEV_TYPE_SMC		0x02080000	/* Smart-card */
#define LLD_DEV_TYPE_SND		0x02090000	/* Sound card */
#define LLD_DEV_TYPE_DSC		0x020A0000	/* Descrambler */
#define LLD_DEV_TYPE_USB		0x020B0000	/* USB device */
#define LLD_DEV_TYPE_STO		0x020C0000	/* Character storage device */
#define LLD_DEV_TYPE_RFM		0x020D0000	/* RF modulator device */
#define LLD_DEV_TYPE_MST		0x020E0000	/* Block storage device */
#define LLD_DEV_TYPE_DIS		0x020F0000	/* Display device */
#define LLD_DEV_TYPE_CIC		0x02100000	/* CI controler device */
#define LLD_DEV_TYPE_SCART      0x02110000	/* SCART */

#define BUS_DEV_TYPE_GPIO		0x03010000	/* GPIO port */
#define BUS_DEV_TYPE_SCI		0x03020000	/* SCI interface */
#define BUS_DEV_TYPE_I2C		0x03030000	/* I2C interface */
#define BUS_DEV_TYPE_ATA		0x03040000	/* ATA interface */
#define BUS_DEV_TYPE_PCI		0x03050000	/* PCI interface */
#define BUS_DEV_TYPE_OHCI		0x03060000	/* OHCI interface */
#define BUS_DEV_TYPE_1394		0x03070000	/* 1394 interface */
#define BUS_DEV_TYPE_ISO7816	0x03080000	/* ISO7816 interface */
#define BUS_DEV_TYPE_TSI		0x03090000	/* TS input interface */

#define ACC_DEV_TYPE_IRP		0x04010000	/* IR Pad */
#define ACC_DEV_TYPE_JSP		0x04020000	/* Joy-stick Pad */

#define SYS_DEFINE_NULL			0x00000000	/* NULL define */

#define SYS_FUNC_ON				0x00000001	/* Function on */
#define SYS_FUNC_OFF			0x00000000	/* Function disable */

/****************************************************************************
 * Section for HW configuration, include bus configuration.
 ****************************************************************************/
/* CHIP */
#define ALI_M3327				(HW_TYPE_CHIP + 4)
#define ALI_M3329E				(HW_TYPE_CHIP + 6)
#define ALI_M3327C				(HW_TYPE_CHIP + 7)
#define ALI_M3202				(HW_TYPE_CHIP + 10)
#define ALI_M3101				(HW_TYPE_CHIP + 30)
#define ALI_M3501				(HW_TYPE_CHIP + 50)
#define ALI_S3601				(HW_TYPE_CHIP + 60)
#define ALI_S3602				(HW_TYPE_CHIP + 70)
#define ALI_S3602F				(HW_TYPE_CHIP + 71)
#define ALI_S3811				(HW_TYPE_CHIP + 72)
#define ALI_S3281				(HW_TYPE_CHIP + 73)
#define ALI_C3701				(HW_TYPE_CHIP + 74)
#define ALI_S3901               (HW_TYPE_CHIP + 75)
#define ALI_S3503               (HW_TYPE_CHIP + 76)
#define ALI_S3821               (HW_TYPE_CHIP + 78)

#define ALI_CAP210              (HW_TYPE_CHIP + 79)

#define ALI_C3505               (HW_TYPE_CHIP + 80)
#define ALI_C3702				(HW_TYPE_CHIP + 81)
/* for ARM based chipset */
#define ALI_ARM_CHIP			(HW_TYPE_CHIP + 0x00050000)
#define ALI_C3921				(ALI_ARM_CHIP + 1)
#define ALI_M3921				(ALI_C3921)
#define ALI_S3921               (ALI_C3921)
#define ALI_S3922                        (ALI_ARM_CHIP + 2)
#define ALI_C3922                        (ALI_ARM_CHIP + 3)

/* CHIP Revised */
#define IC_REV_0				(HW_TYPE_CHIP_REV + 1)
#define IC_REV_1				(HW_TYPE_CHIP_REV + 2)
#define IC_REV_2				(HW_TYPE_CHIP_REV + 3)
#define IC_REV_3				(HW_TYPE_CHIP_REV + 4)
#define IC_REV_4				(HW_TYPE_CHIP_REV + 5)
#define IC_REV_5				(HW_TYPE_CHIP_REV + 6)
#define IC_REV_6				(HW_TYPE_CHIP_REV + 7)
#define IC_REV_7				(HW_TYPE_CHIP_REV + 8)
#define IC_REV_8				(HW_TYPE_CHIP_REV + 9)

/* CPU */
#define CPU_M6303				(HW_TYPE_CPU + 2)
#define CPU_MIPS24KE			(HW_TYPE_CPU + 3)
#define CPU_ALI_V1   			(HW_TYPE_CPU + 4)
#define CPU_MIPS74KF            (HW_TYPE_CPU + 5)

/* Endian */
#define ENDIAN_BIG				(HW_TYPE_ENDIAN + 1)
#define ENDIAN_LITTLE			(HW_TYPE_ENDIAN + 2)

/****************************************************************************
 * Section for LLD configuration.
 ****************************************************************************/

/* Demodulator */
#define MT312					(LLD_DEV_TYPE_DEM + 1)
#define MT10312					(LLD_DEV_TYPE_DEM + 2)
#define ST0299					(LLD_DEV_TYPE_DEM + 3)
#define M3327QPSK				(LLD_DEV_TYPE_DEM + 4)
#define ST0297					(LLD_DEV_TYPE_DEM + 5)
#define MT352					(LLD_DEV_TYPE_DEM + 6)
#define ST0360					(LLD_DEV_TYPE_DEM + 7)
#define AF9003					(LLD_DEV_TYPE_DEM + 8)
#define MT353					(LLD_DEV_TYPE_DEM + 9)
#define ST0361					(LLD_DEV_TYPE_DEM + 10)
#define PN2020					(LLD_DEV_TYPE_DEM + 11)
#define COFDM_READ_EEPROM		(LLD_DEV_TYPE_DEM + 12)
#define DRX3975					(LLD_DEV_TYPE_DEM + 13)
#define COFDM_M3101 		    (LLD_DEV_TYPE_DEM + 14)
#define QAM_S3201 			    (LLD_DEV_TYPE_DEM + 15)
#define SH1409				    (LLD_DEV_TYPE_DEM + 16) //ATSC
#define NEX2004			    	(LLD_DEV_TYPE_DEM + 17) //ATSC
#define SH1411			     	(LLD_DEV_TYPE_DEM + 18) //ATSC
#define AU8524				    (LLD_DEV_TYPE_DEM + 19) //ATSC
#define MICRONAS_DRX3933J	    (LLD_DEV_TYPE_DEM + 20) //ATSC
#define MTK_MT5112EE	    	(LLD_DEV_TYPE_DEM + 21) //ATSC
#define ALI_S3501				(LLD_DEV_TYPE_DEM + 22) //dvbs2
#define ST0362					(LLD_DEV_TYPE_DEM + 23) //dvbt
#define SUNPLUS210              (LLD_DEV_TYPE_DEM + 24) //dvbt
#define CX24116			    	(LLD_DEV_TYPE_DEM + 25) //dvbs2
#define SI2165			    	(LLD_DEV_TYPE_DEM + 26) //dvbt
#define DIB7070			    	(LLD_DEV_TYPE_DEM + 27) //dvbt
#define TC90517			        (LLD_DEV_TYPE_DEM + 28) //ISDBT
#define DIB8000		         	(LLD_DEV_TYPE_DEM + 29) //ISDBT
#define MXL101			        (LLD_DEV_TYPE_DEM + 30) //DVBT
#define TC90512		 	        (LLD_DEV_TYPE_DEM + 31) //ISDBT
#define COFDM_M3100             (LLD_DEV_TYPE_DEM + 32) //DVBT
#define FUJIA21                 (LLD_DEV_TYPE_DEM + 33) //ISDBT
#define TC90527			        (LLD_DEV_TYPE_DEM + 34) //ISDBT
#define SMS2270			        (LLD_DEV_TYPE_DEM + 35) //ISDBT
#define COFDM_S3811		    	(LLD_DEV_TYPE_DEM + 36)
#define CXD2820			    	(LLD_DEV_TYPE_DEM + 37) //Sony DE202 full nim, dvb-t2
#define SHARP6158		     	(LLD_DEV_TYPE_DEM + 38) //dvb-t2 full nim: (MN88472+MXL301)
#define MN88472                 (LLD_DEV_TYPE_DEM + 39) //dvb-t2
#define CXD2834                 (LLD_DEV_TYPE_DEM + 40) //dvb-t2, CDT full nim: CXD2834+MXL603.
#define CDT_MN88472_MXL603      (LLD_DEV_TYPE_DEM + 41) //CDT full nim: MN88472 + MXL603
#define CXD2837				    (LLD_DEV_TYPE_DEM + 42)	//dvb-t2:CXD2837 + CXD2861
#define ALI_T2                  (LLD_DEV_TYPE_DEM + 43) //EMBED_T2
/* Tuner */
#define ANY_TUNER               (LLD_DEV_TYPE_TUN + 0)
#define SL1925					(LLD_DEV_TYPE_TUN + 1)
#define SL1935					(LLD_DEV_TYPE_TUN + 2)
#define MAX2118					(LLD_DEV_TYPE_TUN + 3)
#define IX2360					(LLD_DEV_TYPE_TUN + 4)
#define IX2410					(LLD_DEV_TYPE_TUN + 5)
#define TUA6120					(LLD_DEV_TYPE_TUN + 6)
#define ZL10036					(LLD_DEV_TYPE_TUN + 7)
#define SL1935D					(LLD_DEV_TYPE_TUN + 8)
#define ZL10039                 (LLD_DEV_TYPE_TUN + 9)
#define STB6000                 (LLD_DEV_TYPE_TUN + 10)
#define IX2476                  (LLD_DEV_TYPE_TUN + 11)
#define ED5056					(LLD_DEV_TYPE_TUN + 12)  // for DVBT
#define TD1300AL				(LLD_DEV_TYPE_TUN + 13)  // for DVBT
#define G151D					(LLD_DEV_TYPE_TUN + 14)  // for DVBT
#define H10D8                   (LLD_DEV_TYPE_TUN + 15)  // for DVBT
#define DTF8570					(LLD_DEV_TYPE_TUN + 16)  // for DVBT
#define C9251D				    (LLD_DEV_TYPE_TUN + 17)  // for DVBT
#define DCQ_1D					(LLD_DEV_TYPE_TUN + 18)
#define MT2050					(LLD_DEV_TYPE_TUN + 19)
#define ED6055					(LLD_DEV_TYPE_TUN + 20)
#define SAMSUNG886		  	    (LLD_DEV_TYPE_TUN + 21)
#define TD1311ALF				(LLD_DEV_TYPE_TUN + 22)
#define ED6265					(LLD_DEV_TYPE_TUN + 23)
#define ALP504A					(LLD_DEV_TYPE_TUN + 24)
#define RF4000					(LLD_DEV_TYPE_TUN + 25)
#define DTT7596					(LLD_DEV_TYPE_TUN + 26)
#define QT1010      	        (LLD_DEV_TYPE_TUN + 27) //for DVBT
#define DTT73000				(LLD_DEV_TYPE_TUN + 28)
#define DTT75300				(LLD_DEV_TYPE_TUN + 29)
#define DCT2A 					(LLD_DEV_TYPE_TUN + 30)
#define MT2060					(LLD_DEV_TYPE_TUN + 31)

#define TSA5059					(LLD_DEV_TYPE_TUN + 32)
#define SP5769					(LLD_DEV_TYPE_TUN + 33)
#define G101D						(LLD_DEV_TYPE_TUN + 34)  // for DVBT

#define TD1311ALF_G				(LLD_DEV_TYPE_TUN + 35)
#define TD1611ALF				(LLD_DEV_TYPE_TUN + 36)
#define DTT75101				(LLD_DEV_TYPE_TUN + 37)
#define ED5065					(LLD_DEV_TYPE_TUN + 38)
#define GX1001					(LLD_DEV_TYPE_TUN + 39) //for DVB-C GuoXin 1001 Tuner

#define DTT4A111                (LLD_DEV_TYPE_TUN + 40)
#define DTT73001                (LLD_DEV_TYPE_TUN + 41)
#define EDT1022B                (LLD_DEV_TYPE_TUN + 42)
#define RADIO2004               (LLD_DEV_TYPE_TUN + 43)
#define UM_ZL               	(LLD_DEV_TYPE_TUN + 44)

#define MAX3580                 (LLD_DEV_TYPE_TUN + 45)
#define QT3010                  (LLD_DEV_TYPE_TUN + 46)
#define SH201A					(LLD_DEV_TYPE_TUN + 47) //ATSC
#define TD1336					(LLD_DEV_TYPE_TUN + 48) //ATSC
#define ALPS510					(LLD_DEV_TYPE_TUN + 49) //ATSC
#define DTT76806				(LLD_DEV_TYPE_TUN + 50) //ATSC
#define DTT76801				(LLD_DEV_TYPE_TUN + 51) //ATSC
#define UBA00AP 				(LLD_DEV_TYPE_TUN + 52) //ATSC SYNYO
#define HZ6306                  (LLD_DEV_TYPE_TUN + 53)
#define MT2131                  (LLD_DEV_TYPE_TUN + 54)
#define DCT70701				(LLD_DEV_TYPE_TUN + 55)//DVBC
#define DCT7044					(LLD_DEV_TYPE_TUN + 56)//DVBC
#define ALPSTDQE				(LLD_DEV_TYPE_TUN + 57)//DVBC
#define SAMSUNG_DPH261D		    (LLD_DEV_TYPE_TUN + 58)//atsc
#define MXL5005                 (LLD_DEV_TYPE_TUN + 59) //ATSC Maxlinear
#define UBD00AL 			    (LLD_DEV_TYPE_TUN + 60) 
#define DTT76852 			    (LLD_DEV_TYPE_TUN + 61) 
#define ALPSTDAE				(LLD_DEV_TYPE_TUN + 62) //DVBC
#define DTT75411				(LLD_DEV_TYPE_TUN + 63)//DVBT
#define MT2063					(LLD_DEV_TYPE_TUN + 64)//DVBT
#define TDA18211				(LLD_DEV_TYPE_TUN + 65)//DVBT
#define TDCCG0X1F				(LLD_DEV_TYPE_TUN + 66)//DVBC
#define DNOD44QZV102A		    (LLD_DEV_TYPE_TUN + 67)//DVBT
#define DNOD404PH105A		    (LLD_DEV_TYPE_TUN + 68)//DVBT
#define DTT76809                (LLD_DEV_TYPE_TUN + 69)//ATSC
#define MXL5007                 (LLD_DEV_TYPE_TUN + 70)//DVBT
#define DBCTE702F1				(LLD_DEV_TYPE_TUN + 71)//DVBC
#define STV6110				    (LLD_DEV_TYPE_TUN + 72)//DVBS2 half-nim
#define CD1616LF				(LLD_DEV_TYPE_TUN + 73)//DVBC
#define EN4020					(LLD_DEV_TYPE_TUN + 74)//DVBT
#define TDA18218				(LLD_DEV_TYPE_TUN + 75)//DVBT
#define DIB0070					(LLD_DEV_TYPE_TUN + 76)//DVBT
#define RADIO3432               (LLD_DEV_TYPE_TUN + 77)//DVBT
#define TDA18212			    (LLD_DEV_TYPE_TUN + 78)
#define FC0012                  (LLD_DEV_TYPE_TUN + 79)
#define DIB0090                 (LLD_DEV_TYPE_TUN + 80) //ISDBT
#define BF6009                  (LLD_DEV_TYPE_TUN + 81) //ISDBT
#define ALPSTDAC                (LLD_DEV_TYPE_TUN + 82) //DVBC/DTMB
#define ZL10037                 (LLD_DEV_TYPE_TUN + 83) //DVB-S2
#define ED6092B                 (LLD_DEV_TYPE_TUN + 84)//DVBT
#define	NT220x                  (LLD_DEV_TYPE_TUN + 85) //DVB-C
#define	MXL136                  (LLD_DEV_TYPE_TUN + 86) //ISDBT
#define TDA18219                (LLD_DEV_TYPE_TUN + 87)//DVBT
#define	MXL241                  (LLD_DEV_TYPE_TUN + 88) //DVB-C SOC
#define SMS2270TUNER            (LLD_DEV_TYPE_TUN + 89) //ISDBT
#define TDA18250                (LLD_DEV_TYPE_TUN + 90) //DVB-C
#define MXL301                  (LLD_DEV_TYPE_TUN + 91) //DVB-T2
#define SHARP6401               (LLD_DEV_TYPE_TUN + 92) //ISDBT
#define NM120                   (LLD_DEV_TYPE_TUN + 93) //DVBT
#define MXL603                  (LLD_DEV_TYPE_TUN + 94) //DVB-T2, DVBT, ISDB-T, ATSC, DTMB
#define RT810                   (LLD_DEV_TYPE_TUN +95)//DVBC
#define RT820                   (LLD_DEV_TYPE_TUN +96)//DVBC
#define DB5515                  (LLD_DEV_TYPE_TUN + 97) //DVBT
#define RT820T                  (LLD_DEV_TYPE_TUN + 98) //DVBT
#define CDT_9VM80               (LLD_DEV_TYPE_TUN + 99) //DVBT
#define CDT_9VM135_40           (LLD_DEV_TYPE_TUN + 100) //DVBT
#define SP7006                  (LLD_DEV_TYPE_TUN + 101) //DVBS
#define CXD2861	                (LLD_DEV_TYPE_TUN + 102) //DVBT2
#define DCT70707                (LLD_DEV_TYPE_TUN + 103) //DVBC
#define MXL203                  (LLD_DEV_TYPE_TUN + 104)//DVBS2 poe
#define AV_2012                 (LLD_DEV_TYPE_TUN + 105)//DVBS2 poe
#define SHARP_7306              (LLD_DEV_TYPE_TUN + 106) //ISDBT
#define SHARP_VZ7306  	        (LLD_DEV_TYPE_TUN + 107)
#define TDA18250AB              (LLD_DEV_TYPE_TUN + 108) //DVB-C
#define MXL214C                 (LLD_DEV_TYPE_TUN + 109) //DVB-C
#define CXD2872                 (LLD_DEV_TYPE_TUN + 110) //DVB-T(SONY)
#define RDA5815M                (LLD_DEV_TYPE_TUN + 111)
#define M3031                	(LLD_DEV_TYPE_TUN + 112)//ALi M3031 tuner
#define SI2141                	(LLD_DEV_TYPE_TUN + 113)
#define R858                    (LLD_DEV_TYPE_TUN + 114)//DVBC
#define RT7X0                   (LLD_DEV_TYPE_TUN + 115)//DVBS, maybe RT710, RT720...
#define TUN_R836					(LLD_DEV_TYPE_TUN + 116)//DVBC 


/*LNB*/
#define LNB_A8304				(LLD_DEV_TYPE_LNB + 1) 
#define LNB_TPS65233			(LLD_DEV_TYPE_LNB + 2) 



/* RF modulator */
#define TA1243					(LLD_DEV_TYPE_RFM + 1)
#define MCBS373					(LLD_DEV_TYPE_RFM + 2)
#define	RMUP74055				(LLD_DEV_TYPE_RFM + 3)
#define SHARP5056				(LLD_DEV_TYPE_RFM + 4)
#define SM0268					(LLD_DEV_TYPE_RFM + 5)
#define MC44BS374T1             (LLD_DEV_TYPE_RFM + 6)
#define V8060                   (LLD_DEV_TYPE_RFM + 7)

/****************************************************************************
 * Section for compiler and linker configuration.
 ****************************************************************************/
/* Memory mapping option */
#define __ATTRIBUTE_RAM_		/* Code section running in RAM */
#define __ATTRIBUTE_ROM_		/* Code section running in ROM */
#define __ATTRIBUTE_REUSE_		/* Code section overlay in RAM */
#define __ATTRIBUTE_ICON16_		/* Icon section */
#define __ATTRIBUTE_FONTMAP_	/* Font section */
#ifdef __cplusplus
}
#endif
#endif
