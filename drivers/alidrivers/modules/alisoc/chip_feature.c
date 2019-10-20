/*****************************************************************************
*    Copyright (C) 2010 ALi Corp. All Rights Reserved.
*    
*    Company confidential and Properietary information.       
*    This information may not be disclosed to unauthorized  
*    individual.    
*    File: chip_feature.c
*   
*    Description: 
*    
*    History: 
*    Date           Athor        Version        Reason
*    ========       ========     ========       ========
*    2010/8/30      tt         
*        
*****************************************************************************/

//#include <linux/dvb/sys_config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/init.h>
#include <ali_soc_common.h>
#include <ali_soc.h>
#include <linux/kernel.h> 
#include <ali_reg.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <ali_otp_common.h>

#ifndef FALSE
#define	FALSE			(0)
#endif
#ifndef	TRUE
#define	TRUE			(!FALSE)
#endif

#if defined(CONFIG_MIPS)
extern int sys_ic_get_hd_enabled_ex(void);
extern UINT32 sys_ic_get_rev_id(void);
extern UINT32 sys_ic_get_chip_id(void);
extern int sys_ic_get_product_id_ex(void);
#endif

#ifndef __instrument
#define __instrument
#define __noinstrument __attribute__ ((no_instrument_function))
#endif

/* Need to sync with ZHA, __noinstrument is masked out to pass compilation for
 * GCC ver 3.04.
 * Date: 2012.03.19 by Joy.
 */
#if ((__GNUC__ == 3) && (__GNUC_MINOR__ == 4))
extern UINT32 sys_ic_get_cpu_clock(void);

#else
extern UINT32 __noinstrument sys_ic_get_cpu_clock(void);
#endif

#if defined(CONFIG_MIPS)
extern UINT32 sys_ic_get_dram_clock(void);
extern int sys_reboot_get_timer(UINT32 *time_exp, UINT32 *time_cur);
extern void sys_ic_enter_standby(unsigned int time_exp, unsigned int time_cur);
extern UINT32 product_feature[];
#endif

// functions for drivers
//int sys_ic_get_usb_num(void);    // return USB host number.  0 -- No usb, 1 -- 1 USB host, 2 -- 2 USB host
//int sys_ic_get_ci_num(void);     // return CI slot number.   0 -- No CI.
//int sys_ic_get_mac_num(void);    // return MAC number.       0 -- No MAC.
//int sys_ic_hd_is_enabled(void);	 // 0: enable SD only, 1: enable SD/HD

typedef struct
{
	UINT8 usb_num;
	UINT8 ci_slot_num;
	UINT8 mac_num;
	UINT8 hd_enabled;
	UINT8 m3501_support;				// 3501 support
	UINT8 nim_support;					// other DVB-T/C Demo suppport
	UINT8 sata_enable;
	UINT8 tuner_num;
} PRODUCT_BND;

struct OTP_CONFIG_VPFF //C3503&C3701c chip feature OTP
{
    UINT32 Security_Num:6;    
    UINT32 Certification_Num:7;
    UINT32 Package_Num:3;    
    UINT32 Version_Num:4;
    UINT32 Product_Num:5;    
    UINT32 reserved:6;
};

struct OTP_CONFIG_C3921_VPFF
{
    UINT32 Security_Num:6;      //[5:0]   
    UINT32 Certification_Num:7; //[12:6]
    UINT32 Package_Num1:3;      //[15:13]
    UINT32 Version_Num:4;       //[19:16]
    UINT32 Product_Num:5;       //[24:20]
    UINT32 Package_Num2:2;      //[26:25]
    UINT32 MECO_Num:3;          //[29:27]
    UINT32 Full_Mask_Num:2;     //[31:30]
};

static const PRODUCT_BND m_product_bnd[] = 
{
	{/*"M3601E",*/ 1, 0, 0, 1, 0, 1, 0, 1},    // 0
	{/*"M3381E",*/ 1, 0, 0, 0, 1, 1, 0, 2},    // 1
	{/*"M3606", */ 2, 0, 1, 1, 1, 1, 0, 2},    // 2
	{/*"M3606B",*/ 1, 0, 1, 1, 1, 1, 1, 2},    // 3
	{/*"M3606C",*/ 2, 1, 1, 1, 1, 1, 0, 2},    // 4
	{/*"M3606D",*/ 1, 1, 1, 1, 1, 1, 1, 2},    // 5
	{/*"M3386", */ 2, 0, 1, 0, 1, 1, 0, 2},    // 6
	{/*"M3386B",*/ 1, 0, 1, 0, 1, 1, 1, 2},    // 7
	{/*"M3386C",*/ 2, 1, 1, 0, 1, 1, 0, 2},    // 8
	{/*"M3386D",*/ 1, 1, 1, 0, 1, 1, 1, 2},    // 9
	{/*"M3701E",*/ 2, 0, 1, 1, 1, 1, 0, 2},    // 10
	{/*"M3701D",*/ 1, 0, 1, 1, 1, 1, 1, 2},    // 11
	{/*"M3701C",*/ 2, 0, 1, 1, 1, 1, 0, 2},    // 12
	{/*"M3701F",*/ 1, 0, 1, 1, 1, 1, 1, 2},    // 13
	{/*"M3603", */ 2, 2, 1, 1, 1, 1, 1, 2},    // 14
	{/*"M3383", */ 2, 2, 1, 0, 1, 1, 1, 2},    // 15
	{/*"M3601S",*/ 1, 0, 0, 1, 1, 1, 0, 1},    // 16
};


static UINT32 reg_88, reg_ac, reg_2e000, reg_70;


#define PRODUCT_NUM_C3603   (sizeof(m_product_bnd)/sizeof(m_product_bnd[0]))
#define AS_BOOT_EN  1

UINT32 ali_sys_ic_get_chip_id(void);

/*
 *
 *The API below specified for KERNEL call!
 *
*/

void ali_sys_ic_enter_standby(UINT32 time_exp, UINT32 time_cur)
{
	#if defined(CONFIG_MIPS)
    	sys_ic_enter_standby(time_exp, time_cur);
    #else
    	return;
    #endif
}

extern int ali_pmu_get_time(unsigned long *time_exp);
int ali_sys_reboot_get_timer(unsigned long *time_exp, unsigned long *time_cur)
{
	return 0;
}

UINT32 ali_sys_ic_get_boot_type(void)
{
    UINT8 ret = 0;

    #if defined(CONFIG_MIPS)
    	//ret = board_is_nand_boot();
    #endif
    
    return ret;
}


/*get bonding with c3603*/
void ali_sys_ic_get_bonding(void)
{
}

/*return product id*/
UINT32 ali_sys_ic_get_product_id(void)
{
	return 0;
}

/*return c3603 product id*/
UINT32 ali_sys_ic_get_c3603_product(void)
{
	return 0;
}

/*return chip id*/
UINT32 ali_sys_ic_get_chip_id(void)
{
	UINT32 chip_id,chip_id_reg;

	chip_id_reg = __REG32ALI(0x18000000);
	chip_id_reg = chip_id_reg>>16;
	
	if(chip_id_reg ==0x3327){
		chip_id = ALI_M3327;
	}else if(chip_id_reg ==0x3101){
		chip_id = ALI_M3327;
	}else if(chip_id_reg ==0x3202){
		chip_id = ALI_M3327;
	}else if(chip_id_reg ==0x3329){
		chip_id = ALI_M3329E;
	}else if(chip_id_reg ==0x3602){
		chip_id = ALI_S3602;
	}else if(chip_id_reg ==0x3603){
		chip_id = ALI_S3602F;
	}else if(chip_id_reg ==0x3901){
		chip_id = ALI_S3901;
	}else if(chip_id_reg ==0x3701){
		chip_id = ALI_C3701;
	}else if(chip_id_reg ==0x3921){
		chip_id = ALI_C3921;
	}else if(chip_id_reg ==0x3503){
		chip_id = ALI_S3503;	
	}else if(chip_id_reg ==0x3821){
		chip_id = ALI_S3821;
	}else if(chip_id_reg ==0x3505){
		chip_id = ALI_C3505;		
	}else{
		chip_id = SYS_DEFINE_NULL;
	}

	return chip_id;
}
EXPORT_SYMBOL(ali_sys_ic_get_chip_id);



/*return revised id*/
UINT32 ali_sys_ic_get_rev_id(void)
{
	UINT32 rev_id,chip_id_reg;

	chip_id_reg = __REG8ALI(0x18000000);

	if(chip_id_reg ==0){
		rev_id = IC_REV_0;
	}else if(chip_id_reg ==1){
		rev_id = IC_REV_1;
	}else if(chip_id_reg ==2){
		rev_id = IC_REV_2;
	}else if(chip_id_reg ==3){
		rev_id = IC_REV_3;
	}else if(chip_id_reg ==4){
		rev_id = IC_REV_4;
	}else{
		rev_id = IC_REV_4;
	}

	return rev_id;
}

/*return cpu clock*/

static UINT32 cpu_freq_setting_3821[9] = {8,675,594,450,396,337,297,225,801};
static UINT32 cpu_freq_setting_3503[5] = {4,594,450,396,513};
static UINT32 cpu_freq_setting_3505[9] = {8,800,900,1000,1100,1200,1300,1400,1500};
static UINT32 *dfs_freq_setting = NULL;

UINT32 ali_sys_ic_get_cpu_clock(void)
{
	unsigned long strap_pin_reg = 0, pll_reg = 0, M = 0, N = 0, L = 0, cpu_clock = 0;
	
    UINT32 chip_id = ali_sys_ic_get_chip_id();

	if (chip_id >= ALI_C3921)
	{
		strap_pin_reg = __REG32ALI(0x18000070);
		strap_pin_reg = (strap_pin_reg>>8)&0x07;
		if(strap_pin_reg == 0)
		{
			cpu_clock = 800;	// 800MHz
		}
		else if(strap_pin_reg == 1)
		{
			cpu_clock = 900;	// 900MHz
		}
		else if(strap_pin_reg == 2)
		{
			cpu_clock = 1000;	// 1000MHz
		}
		else if(strap_pin_reg == 3)
		{
			cpu_clock = 1100;	// 1100MHz
		}
		else if(strap_pin_reg == 4)
		{
			cpu_clock = 1200;	// 1200MHz
		}
		else if(strap_pin_reg == 5)
		{
			cpu_clock = 1300;	// 1300MHz
		}
		else if(strap_pin_reg == 6)
		{
			cpu_clock = 1400;	// 1400MHz
		}
		else if(strap_pin_reg == 7)
		{
			cpu_clock = 1500;	// 1500MHz
		} 
		
	}
	else if (chip_id == ALI_S3821)
	{
		strap_pin_reg = __REG32ALI(0x18000070);
		strap_pin_reg = (strap_pin_reg>>8)&0x07;
		dfs_freq_setting = cpu_freq_setting_3821;
		cpu_clock = dfs_freq_setting[strap_pin_reg+1];
	}
	else if (chip_id == ALI_S3503)
	{
		strap_pin_reg = __REG32ALI(0x18000070);
		strap_pin_reg = (strap_pin_reg>>8)&0x07;
		dfs_freq_setting = cpu_freq_setting_3503;
		cpu_clock = dfs_freq_setting[strap_pin_reg+1];
	}
	else if (chip_id == ALI_C3505)
	{
		strap_pin_reg = __REG32ALI(0x18000070);
		strap_pin_reg = (strap_pin_reg>>8)&0x07;
		dfs_freq_setting = cpu_freq_setting_3505;
		cpu_clock = dfs_freq_setting[strap_pin_reg+1];
	}	
    else{ // 3701c
		//cpu_clock = sys_ic_get_cpu_clock();

		strap_pin_reg = __REG32ALI(0x18000070);
		strap_pin_reg = (strap_pin_reg>>7)&0x07;
		if(strap_pin_reg == 0)
		{
			cpu_clock = 600;	//600MHz
		}
		else if(strap_pin_reg == 1)
		{
			cpu_clock = 450;	//450MHz
		}
		else if(strap_pin_reg == 2)
		{
			cpu_clock = 396;	//396MHz
		}
		else if(strap_pin_reg == 3)
		{
			cpu_clock = 297;	//297MHz
		}
		else if(strap_pin_reg == 4)
		{	// cpu use pll clk
			//5:0	Digital PLL function L control Register
			//13:8	Digital PLL function N Control Register
			//25:16	Digital PLL function M Control Register
			//CPU_CLK = 27*(MCTRL+1)/( NCTRL+1)/( LCTRL+1)
			pll_reg = __REG32ALI(0x180004d0);
			M= (pll_reg>>16)&0x3FF;
			N= (pll_reg>>8)&0x3F;
			L= (pll_reg>>0)&0x3F;
			cpu_clock=27*(M+1)/(N+1)/(L+1);
		 }
    	}
	return cpu_clock;
}


/*
	3701c STRAP_INFO Strap pin information. Reflect the strap pin status that latched into C3701C when power up or warm reset.
Bit 3-0 :	WORK_MODE[3:0]
Bit 4:		PLL_BYPASS
Bit 6-5:	MCLK_SEL[1:0]
Bit 7: 	CPUCLK_SEL
Bit8:     reserved
Bit 9:		reserved 
Bit11-10:	reserved
Bit12:	CRYSTAL_54M_SEL
Bit 13:	UART_UPGRADE_EN
Bit 14:	SINGLE_CPU_SEL
Bit 15:	EN_EXT_SFLASH
Bit 16:	FUNCTION_MODE
Bit 17:	MCLK_SEL[2]
Bit 18:	NF_SF_SEL (1: NAND boot , default , 0: NOR boot)
Bit 19:    CPU_Probe Enable

6:5	MEM_CLK_SEL[2:0]
. (MEM_CLK_SEL[2] is BIT[17] of 74H register)
000: 166M
001: 133M ;
010: 100M
011: 1.68M
100: Reserved
101:83.3M
110:reserved

#define MEM_CLK_166M	0x00	// DDR3 1333Mbps DDR2:NULL, BGA256 only, QFP not support
#define MEM_CLK_133M	0x01	//DDR3/2 1066Mbps  internal bus:133MHz external clk:533 MHz 
#define MEM_CLK_100M	0x02	//DDR3/2 800Mbps 
#define MEM_CLK_2d06M	0x03	//16.5Mbps
#define MEM_CLK_200M	0x04	//not support
#define MEM_CLK_83M		0x05	// DD2 667Mbps 
#define MEM_CLK_16d5M	0x03	// 2.06*8
#define MEM_CLK_27M	0x06	//0xb8000074 bit[4] 0:enable pll 1: Pll by pass, can not trigger by sw, just can be set in strap pin

*/

/*return dram clock*/
UINT32 ali_sys_ic_get_dram_clock()
{
	unsigned long strap_pin_reg,dram_clock;
    UINT32 chip_id = ali_sys_ic_get_chip_id();

	strap_pin_reg = __REG32ALI(0x18000070);

    if (chip_id >= ALI_C3921)
    {
		strap_pin_reg = (strap_pin_reg>>5)&0x07;
		if(strap_pin_reg == 0)
		{
			dram_clock = 33*2;	// bypass
		}
		else if(strap_pin_reg == 1)
		{
			dram_clock = 264*2;	// 528Mbps
		}
		else if(strap_pin_reg == 2)
		{
			dram_clock = 330*2;	// 688Mbps
		}
		else if(strap_pin_reg == 3)
		{
			dram_clock = 396*2;	// 800Mbps
		}
		else if(strap_pin_reg == 4)
		{
			dram_clock = 528*2;	// 1066Mbps
		}
		else if(strap_pin_reg == 5)
		{
			dram_clock = 660*2;	// 1333Mbps
		}
		else
		{
			dram_clock = 792*2;	// 1600Mbps
		}
    	
    }
	else if(chip_id == ALI_C3505)
	{
		strap_pin_reg = (strap_pin_reg>>4)&0x0F;
		if(strap_pin_reg == 0)
		{
			dram_clock = 264;
		}
		else if(strap_pin_reg == 1)
		{
			dram_clock = 297;
		}
		else if(strap_pin_reg == 2)
		{
			dram_clock = 330;
		}
		else if(strap_pin_reg == 3)
		{
			dram_clock = 363;
		}
		else if(strap_pin_reg == 4)
		{
			dram_clock = 396;
		}
		else if(strap_pin_reg == 5)
		{
			dram_clock = 429;
		}
		else if(strap_pin_reg == 6)
		{
			dram_clock = 462;
		}	
		else if(strap_pin_reg == 7)
		{
			dram_clock = 33;
		}
		else if(strap_pin_reg == 8)
		{
			dram_clock = 528;
		}
		else if(strap_pin_reg == 9)
		{
			dram_clock = 594;
		}
		else if(strap_pin_reg == 10)
		{
			dram_clock = 660;
		}		
		else if(strap_pin_reg == 11)
		{
			dram_clock = 726;
		}
		else if(strap_pin_reg == 12)
		{
			dram_clock = 792;
		}
		else if(strap_pin_reg == 13)
		{
			dram_clock = 858;
		}
		else if(strap_pin_reg == 14)
		{
			dram_clock = 924;
		}		
	}
	else{ // 3701c
		strap_pin_reg = (strap_pin_reg>>5)&0x03;
		if(strap_pin_reg == 0)
		{
			dram_clock = 166*8;	//1333Mbps 
		}
		else if(strap_pin_reg == 1)
		{
			dram_clock = 133*8;	//1066Mbps
		}
		else if(strap_pin_reg == 2)
		{
			dram_clock = 100*8;	//800Mbps
		}
		else
		{
			dram_clock = 83*8;	//667Mbps
		}
	}
	
	return dram_clock;
}


/*return dram size*/
//0x18001000 bit[2:0] Organization
// 000:  16 MB@16bits    32 MB@32bits
// 001:  32 MB@16bits    64 MB@32bits	
// 010:  64 MB@16bits   128 MB@32bits
// 011: 128 MB@16bits  256 MB@32bits
// 100: 256 MB@16bits  512 MB@32bits	
// 101: 512 MB@16bits  1GB@32bits
// 110: 128MB (DDR2: 8bits x 2, Just 16 bits Interleave Mapping)	
// 111: 1GB@16bits 2GB@32bits		
unsigned long ali_sys_ic_get_dram_size(void) 
{
	unsigned long data = 0,dram_size = 0;
    UINT32 chip_id = ali_sys_ic_get_chip_id();

	
    if ((chip_id >= ALI_C3921) || (chip_id == ALI_C3505))
    {
		data = __REG32ALI(0x18001000) & 0x07;				
		if(data == 0)
		{
			dram_size = 32;		// 32M
		}
		else if(data == 1)
		{
			dram_size = 64;		// 64M
		}
		else if(data == 2)
		{
			dram_size = 128;	// 128M
		}
		else if(data == 3)
		{
			dram_size = 256;	// 256M
		}
		else if(data == 4)
		{
			dram_size = 512;	// 512M
		}
		else if(data == 5)
		{
			dram_size = 1024;	// 1024M
		}
		else if(data == 6)
		{
			dram_size = 128;	// 128M
		}
		else
		{
			dram_size = 2048;	// 2048M
		}    	
    }
    
	
	return dram_size;
}


/*return usb number supported by chip*/
int ali_sys_ic_get_usb_num(void)
{
    UINT32 chip_id = ali_sys_ic_get_chip_id();
    UINT32 product_id;

    // C3603
    if (chip_id == ALI_S3602F && ali_sys_ic_get_rev_id() >= IC_REV_1)
    {
        product_id = ali_sys_ic_get_c3603_product();
        if (product_id < PRODUCT_NUM_C3603)
        {
            return m_product_bnd[product_id].usb_num;
        }
        return 1;
    }
    if(chip_id == ALI_S3503)
        return 2;  
    
    return 1;
}

/*return NIM number supported by chip*/
int ali_sys_ic_get_ci_num(void)
{
    UINT32 chip_id = ali_sys_ic_get_chip_id();
    UINT32 product_id;

    if (chip_id == ALI_S3602F)
    {
        // S3602F
        if (ali_sys_ic_get_rev_id() == IC_REV_0)
            return 2;

        // C3603
        product_id = ali_sys_ic_get_c3603_product();
        if (product_id < PRODUCT_NUM_C3603)
        {
            return m_product_bnd[product_id].ci_slot_num;
        }
        return 0;
    }

    if (chip_id == ALI_S3602)
        return 2;
    if (chip_id == ALI_S3503)
        return 1; 

    return 2;   // S3329E
}


/*return TUNER number supported by chip*/
int ali_sys_ic_get_tuner_num(void)
{
	UINT32 chip_id = ali_sys_ic_get_chip_id();
    UINT32 product_id;

    if (chip_id == ALI_S3602F)
    {
        // S3602F
        if (ali_sys_ic_get_rev_id() == IC_REV_0)
            return 2;

        // C3603
        product_id = ali_sys_ic_get_c3603_product();
        if (product_id < PRODUCT_NUM_C3603)
        {
            return m_product_bnd[product_id].tuner_num;
        }
    }
	return 2;	
}

/*return MAC number supported by chip*/
int ali_sys_ic_get_mac_num(void)
{
    return 0;
}

int ali_sys_ic_nim_support(void) //BOOL
{
	return TRUE;
}


int ali_sys_ic_nim_m3501_support(void) //BOOL
{
	return TRUE;
}

int sys_ic_get_mac_num(void)
{
	return TRUE;
}
int ali_sys_ic_change_boot_type(unsigned int type)
{
    UINT32 chip_id, chip_package;
    UINT32 tmp32;

    chip_id = ali_sys_ic_get_chip_id();
    chip_package = (UINT8)(readl(ALI_SOC_BASE) >> 8) & 0x0F;	

    switch(type)
    {
        case ALI_SOC_BOOT_TYPE_NOR:
            if ((ALI_C3701 == chip_id) || ((ALI_S3901 == chip_id) && (0x2 == chip_package)) || (ALI_C3505 == chip_id))
            {// chip package 0x2 is M3701G
                reg_70 = readl(ALI_SOC_BASE + 0x70);
            	tmp32 = reg_70;
                tmp32 &= ~(1<<18);
            	tmp32 |= (1<<30);
            	writel(tmp32, ALI_SOC_BASE + 0x74);
            }
            else if(ALI_S3503 == chip_id)
            {   
                reg_70 = readl(ALI_SOC_BASE + 0x70);
                tmp32 = reg_70;
                tmp32 &= ~(1<<18);
                tmp32 |= ((1<<30)|(1<<29)|(1<<24)); // trigger bit
                writel(tmp32, ALI_SOC_BASE + 0x74);
            }

            break;
            
        case ALI_SOC_BOOT_TYPE_NAND:
            if ((ALI_C3701 == chip_id) || ((ALI_S3901 == chip_id) && (0x2 == chip_package)) || (ALI_C3505 == chip_id))
            {// chip package 0x2 is M3701G
                reg_70 = readl(ALI_SOC_BASE + 0x70);
            	tmp32 = reg_70;
            	tmp32 |= ((1<<18)|(1<<30));
            	writel(tmp32, ALI_SOC_BASE + 0x74);
            }
            else if((ALI_S3602F == chip_id )&&((0x0b == chip_package)||(0x7 == chip_package)))
            {// chip package 0x0b is M3606, chip package 0x7 is M3701E, 
            	reg_88 = readl(ALI_SOC_BASE+0x88);
            	reg_ac = readl(ALI_SOC_BASE+0xAC);
            	reg_2e000 = readl(ALI_SOC_BASE+0x2e000);

            	tmp32 = readl(ALI_SOC_BASE+0x88);
            	tmp32 &= ~((1<<0)|(1<<1)|(1<<20));	//CI UART2
            	tmp32 &= ~((3<<20)|(3<<16));	//URAT2 _216_SEL, UART2_BGA_SEL
            	writel(tmp32,ALI_SOC_BASE+0x88);
            	
            	tmp32 = readl(ALI_SOC_BASE+0xAC);
            	tmp32 |= (1<<22) | (1<<24);	
            	tmp32 &= ~1;		// SFLASH CS1 SEL(216PIN) 0: GPIO[79] 1: SFLASH CS[1]
            	writel(tmp32,ALI_SOC_BASE+0xAC);
            	//flash reg CPU_CTRL_DMA
            	//bit8: PIO_arbit_fuc_en 1 sflash/pflash/ci can share the bus with flash arbiter
            	//bit9:cpu_set_arbit_en 
            	//bit12:10 cpu_set_arbit_en 001 sflash is enable 010 pflash is enable 100 CI is enable
            	tmp32 = readl(ALI_SOC_BASE+0x2E000);
            	tmp32 &= ~(0x00001F00);	
            	tmp32 |= 0x00001200 ;	
            	writel(tmp32, ALI_SOC_BASE+0x2E000);
            }
            else if(ALI_S3503 == chip_id)
            {   
                reg_70 = readl(ALI_SOC_BASE + 0x70);
                tmp32 = reg_70;
                tmp32 |= (1<<18);
                tmp32 &= ~((1<<17)|(1<<15));
                tmp32 |= ((1<<30)|(1<<29)|(1<<24)); // trigger bit
                writel(tmp32, ALI_SOC_BASE + 0x74);
            }
            break;
        default:
            return 1;
    }

	return 0;
}
int ali_sys_ic_revert_boot_type(void)
{
	UINT32 chip_id,chip_package;
	UINT32 tmp32;

    chip_id = ali_sys_ic_get_chip_id();
    chip_package = (UINT8)(readl(ALI_SOC_BASE) >> 8) & 0x0F;	

    if ((ALI_C3701 == chip_id) || ((ALI_S3901 == chip_id) && (0x2 == chip_package)) || (ALI_C3505 == chip_id))
    {// chip package 0x2 is M3701G
        tmp32 = reg_70;
		tmp32 |= 1<<30;
		writel(tmp32, ALI_SOC_BASE + 0x74); 
    }
    else if((ALI_S3602F == chip_id )&&((0x0b == chip_package)||(0x7 == chip_package)))
    {// chip package 0x0b is M3606, chip package 0x7 is M3701E, 
		writel(reg_88, ALI_SOC_BASE+0x88);
		writel(reg_ac, ALI_SOC_BASE+0xac);
		writel(reg_2e000, ALI_SOC_BASE+0x2E000);
    }
    else if(ALI_S3503 == chip_id)
    {   
        tmp32 = reg_70;
        tmp32 |= ((1<<30)|(1<<29)|(1<<24));
        writel(tmp32, ALI_SOC_BASE + 0x74);
    }
	return 0;
}

/* Time to the reboot occur, Unit is Second */
void hw_watchdog_reboot_time(int time)
{
	//ali_sys_ic_change_boot_type(ali_sys_ic_get_boot_type());
	UINT32 total_cnt = 0;
	UINT32 unit_1s = 105469;
	
	if(!time)
	{//fixed to 39ms
		__REG32ALI(ALI_SOC_BASE+0x18500) = 0xfffff000;
	}
	else
	{
		total_cnt = ((time > 40000)?40000:time) * unit_1s;
		__REG32ALI(ALI_SOC_BASE+0x18500) = (0xffffffff - total_cnt);
	}

	/*fixed to 256 clkdiv */
	__REG32ALI(ALI_SOC_BASE+0x18504) = 0x67;
}

void hw_watchdog_reboot(void)
{
	hw_watchdog_reboot_time(0);
	while(1);
}

/**
 * of_get_major_minor() - get major and minor base number for a device
 * @np: pointer to node to create device for
 * @dev: output parameter for first assigned number
 * @baseminor: first of the requested range of minor numbers
 * @count: the number of minor numbers required
 * @name: the name of the associated device or driver
 */
int of_get_major_minor(struct device_node *node, dev_t *dev,
                       unsigned baseminor, unsigned count,const char *name)
{
       int ret = 0;
       int major_minor[2] = {0,0};

       /* Check if property major-minor exist */
       ret = of_property_read_u32_array(node, "major-minor",
               major_minor, 2);

       if((0 == ret) && (major_minor[0] > 0)){
               *dev = MKDEV(major_minor[0], major_minor[1]);
               if(register_chrdev_region(*dev, count, name))
               {
                       *dev = 0;
                       pr_err("register ali dev_t fail\n");
                       return -EBUSY;
               }
       }
       else {
               if(alloc_chrdev_region(dev, baseminor, count, name))
               {
                       *dev = 0;
                       pr_err("alloc ali dev_t fail\n");
                       return -EBUSY;
               }
       }

       return 0;
}
