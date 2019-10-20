/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
 
/****************************************************************************(I)(S)
 *  File: init.c
 *  (I)
 *  Description:
 *  (S)
 *  History:(M)
 *      	       Date        			Author         	Comment
 *      	       ====        			======		=======
 * 0.		2010.06.03			Sam			Create
 ****************************************************************************/
 
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/bootmem.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/mach-ali/prom.h>
#include "board_config.h"
#include <ali_board_config.h>
#include <linux/memblock.h>
#include <ali_soc.h>
#include <ali_reg.h>
//=======================================================================================================//

#define KB                                                  (1024)
#define MB                                                 (1024 * KB)
#define ALI_OVG_MEM                                 "ovg_mem="
#define ALI_OTP_BASE                                (0x18042000)
#define OTP_ADDR_REG                               (0x4)
#define OTP_READ_TRIG_REG                       (0xc)
#define OTP_READ_STATUS_REG                   (0x10)
#define OTP_VALUE_REG                              (0x18)
#define GET_OTP_READ_STATUS                   __REG32ALI(ALI_OTP_BASE+OTP_READ_STATUS_REG)
#define OTP_READ_BUSY                             (0x100)
#define NAND_BOOT_IS_SELECTED               (1 << 18)
#define OTP_PRODUCT_ID_ADDR                  (0x84)
#define RESVRED_PRODUCT_ID                     (0xff)
#define ALI_SOC_BASE_ADDR                      (0x18000000)
#define M3505_CHIP_ID                              0x35050000
#define GET_CHIP_ID                                  (__REG32ALI(ALI_SOC_BASE_ADDR) & 0xFFFF0000)
#define M3505_MAIN_CLK_DIV_REG             0x180000C0
#define M3505_MAIN_CLK_READ_REG           0x18000070
#define M3505_MAIN_CLK_SET_REG             0x18000074
//=======================================================================================================//

int prom_argc;
char **prom_argv;
char **prom_envp;

#ifdef CONFIG_OF
extern struct boot_param_header __dtb_start;
#endif

#ifdef CONFIG_ALI_STANDBY_TO_RAM
extern void clear_C0_EBase(void);
#endif
extern void kernel_fromboot(int boot);
extern bool is_nand_boot;

typedef enum
{
	CPU_800MHZ = 0,
	CPU_900MHZ,
	CPU_1000MHZ,
	CPU_1100MHZ,
	CPU_1200MHZ,
}M3505_MAIN_CLK;

typedef enum
{
	MAIN_NO_DIV = 0,/*No freq div.*/
	MAIN_1_DIV_2,/*1/2 freq div.*/
	MAIN_1_DIV_4,/*1/4 freq div.*/
}M3505_MAIN_CLK_DIV;

typedef struct
{                           
	UINT8 pid;
	UINT8 main_clk;
	UINT8 main_clk_div;
}CPU_CLK_BND_M3505;

static const CPU_CLK_BND_M3505 m_cpu_clk_m3505[] = 
{                                                                
	{0, CPU_1000MHZ, MAIN_NO_DIV},/*0:M3527-AF.*/
	{1, CPU_1000MHZ, MAIN_NO_DIV},/*1:M3527P.*/
	{2, CPU_1000MHZ, MAIN_NO_DIV},/*2:M3527-AL.*/
	{3, CPU_1000MHZ, MAIN_NO_DIV},/*3:M3527L.*/
	{4, CPU_1000MHZ, MAIN_NO_DIV},/*4:M3521-AL.*/
	{5, CPU_1000MHZ, MAIN_NO_DIV},/*5:M3526-AL.*/
	{6, CPU_1000MHZ, MAIN_NO_DIV},/*6:M3627-AF.*/
	{7, CPU_1000MHZ, MAIN_NO_DIV},/*7:M3529C.*/
	{8, CPU_1000MHZ, MAIN_NO_DIV},/*8:M3627-AL.*/
	{9, CPU_1000MHZ, MAIN_NO_DIV},/*9:M3627L.*/
	{10, CPU_1000MHZ, MAIN_NO_DIV},/*10:M3727-AF.*/
	{11, CPU_1000MHZ, MAIN_NO_DIV},/*11:M3727P.*/
	{12, CPU_1000MHZ, MAIN_NO_DIV},/*12:M3727-AL.*/
	{13, CPU_1000MHZ, MAIN_1_DIV_2},/*13:M3317-AL.*/
	{14, CPU_1000MHZ, MAIN_1_DIV_2},/*14:M3317L.*/
	{15, CPU_1000MHZ, MAIN_NO_DIV},/*15:M3517-AL/M3717 AF ====> At present, M3517 AL is replaced by M3717AF(PL's infomation).*/
	{16, CPU_1000MHZ, MAIN_NO_DIV},/*16:M3727L.*/
	{17, CPU_1000MHZ, MAIN_NO_DIV},/*17:M3626-AL.*/
	{18, CPU_1000MHZ, MAIN_NO_DIV},/*18:M3529-AF.*/
	{19, CPU_1000MHZ, MAIN_NO_DIV},/*19.M3529P.*/
	{20, CPU_1000MHZ, MAIN_NO_DIV},/*20.M3527C.*/
	{21, CPU_1000MHZ, MAIN_1_DIV_2},/*21:M3712AL.*/
	{22, CPU_1000MHZ, MAIN_1_DIV_2},/*22:M3712L.*/
	{23, CPU_1000MHZ, MAIN_NO_DIV},/*23.M3716L.*/
	{24, CPU_1000MHZ, MAIN_1_DIV_2},/*24:A3510L.*/
	{25, CPU_1000MHZ, MAIN_NO_DIV},/*25.M3517-AF.*/
	{26, CPU_1000MHZ, MAIN_1_DIV_2},/*26:M3216-AL.*/
	{27, CPU_1000MHZ, MAIN_NO_DIV},/*27.M3317AF.*/
	{28, CPU_1000MHZ, MAIN_NO_DIV},/*28.M3517P.*/
	{29, CPU_1000MHZ, MAIN_NO_DIV},/*29.M3727C.*/
	{30, CPU_1000MHZ, MAIN_1_DIV_2},/*30.M3216L.*/
	{31, CPU_1000MHZ, MAIN_1_DIV_2},/*30.M3216V.*/
};
#define PRODUCT_NUM_C3505   (sizeof(m_cpu_clk_m3505)/sizeof(m_cpu_clk_m3505[0]))
//=======================================================================================================//

const char *get_system_type(void)
{
	return "ALi M36";
}

char *prom_getenv(char *envname)
{
	return NULL;
}

static unsigned int ali_get_product_id(void)
{
	unsigned int ali_product_id = 0;

	__REG32ALI(ALI_OTP_BASE+OTP_ADDR_REG) = (OTP_PRODUCT_ID_ADDR*4);
	__REG32ALI(ALI_OTP_BASE+OTP_READ_TRIG_REG) |= (1<<8);
	while(OTP_READ_BUSY == (GET_OTP_READ_STATUS & (1<<8)));
	ali_product_id = ((__REG32ALI(ALI_OTP_BASE+OTP_VALUE_REG) >> 20) & 0x1F);/*Product id is OTP84[24:20].*/;

	return ali_product_id;
}

void set_cpu_clk_by_product_id(void)
{
	unsigned int product_id = ali_get_product_id();
	unsigned int temp_data = 0, main_clk_setting = 0, main_clk_div = 0;

	if(M3505_CHIP_ID != GET_CHIP_ID)/*At present, only M3505 support set cpu clk by otp product id.*/
	{
		return;
	}

	//printk(KERN_EMERG "\nFunction:%s, Line:%d, product_id: 0x%02X", __FUNCTION__, __LINE__, product_id);
	if((product_id < PRODUCT_NUM_C3505) && (product_id == m_cpu_clk_m3505[product_id].pid))
	{
		/*Step1: If current main cpu freq is correct, skip the following freq setting steps.*/
		main_clk_setting = __REG32ALI(M3505_MAIN_CLK_READ_REG) & ((1<<10) | (1<<9) | (1<<8));
		main_clk_div = __REG32ALI(M3505_MAIN_CLK_DIV_REG) & ((1<<9) | (1<<8));
		if((((m_cpu_clk_m3505[product_id].main_clk) << 8) == main_clk_setting) \
			&& (((m_cpu_clk_m3505[product_id].main_clk_div) << 8) == main_clk_div))
		{
			printk(KERN_EMERG "\nFunction:%s, Line:%d, cpu clk is correct now and it is no need to set its clk again!\n", __FUNCTION__, __LINE__);
			return;
		}

		//printk(KERN_EMERG "\nFunction:%s, Line:%d, start to set cpu clk......\n", __FUNCTION__, __LINE__);
		//printk(KERN_EMERG "main_clk: 0x%08X\nmain_clk_div: 0x%08X\n", m_cpu_clk_m3505[product_id].main_clk, m_cpu_clk_m3505[product_id].main_clk_div);
		/*Step2: Set CPLL div.*/
		temp_data = __REG32ALI(M3505_MAIN_CLK_DIV_REG);
		temp_data &= ~((1<<9) | (1<<8));
		temp_data |= ((m_cpu_clk_m3505[product_id].main_clk_div) << 8);
		__REG32ALI(M3505_MAIN_CLK_DIV_REG) = temp_data;

		/*Step3: Set main cpu clk.*/
		temp_data = __REG32ALI(M3505_MAIN_CLK_SET_REG);
		temp_data &= ~((1<<10) | (1<<9) | (1<<8));
		temp_data |= ((m_cpu_clk_m3505[product_id].main_clk) << 8);
		__REG32ALI(M3505_MAIN_CLK_SET_REG) = temp_data;
		mdelay(1);/*Delay some time.*/
		__REG32ALI(M3505_MAIN_CLK_SET_REG) |= (1<<22);
		mdelay(20);/*Delay some time.*/
	}
}

void __init prom_init(void)
{
	unsigned char *memsize_str = NULL;
	unsigned long chip_id = (*(volatile unsigned long *)(0xb8000000)) >> 16;
	unsigned long boot_type = 0;

#ifdef CONFIG_ALI_STANDBY_TO_RAM
	clear_C0_EBase();
#endif

	set_cpu_clk_by_product_id();

	if((0x3503 == chip_id) || (0x3821 == chip_id) || (0x3505 == chip_id))
	{
		boot_type = (*(volatile unsigned long *)(0xb8000070)) & (1<<18);
		printk("\n====>boot_type=0x%lx====>\n", boot_type);
		if(NAND_BOOT_IS_SELECTED == boot_type)
		{
			is_nand_boot = true;
		}
		else
		{
			is_nand_boot = false;
		}
	}
	   
#if (defined(CONFIG_ALI_M3701G) || defined(CONFIG_ALI_M3701C))
	*((volatile unsigned long *)(0xB8000110)) |= 0x01;
#endif
	*((volatile unsigned long *)(0xB8000220)) |= 0x01000000;

	if(0x3505 == chip_id)
	{
		*((volatile unsigned long *)(0xB8070000)) |= (1<<0);
	}
	else
	{
		*((volatile unsigned long *)(0xB8000110)) |= (1<<0);
	}

	/*Ehanced by tony on 2014-08-25.*/
	//if (fw_arg0 >= CKSEG0 || fw_arg1 < CKSEG0 || fw_arg2 != 0 ||fw_arg3 < CKSEG0) {
	if (fw_arg0 >= CKSEG0 || fw_arg1 < CKSEG0 || fw_arg3!= 0 ||fw_arg2 < CKSEG0) {
		/* fw_arg0 is not a valid number, or fw_arg1 is not a valid pointer */
		prom_argc = 0;
		prom_argv = prom_envp = NULL;
		printk("CKSEG0 0x%08x fw_arg0 0x%lx fw_arg1 0x%lx fw_arg2 0x%lx fw_arg3 0x%lx\n",CKSEG0,fw_arg0,fw_arg1,fw_arg2,fw_arg3);
	}else {
		prom_argc = (int) fw_arg0;
		prom_argv = (char **) fw_arg1;
		prom_envp = (char **) fw_arg2;
	}

	mips_machtype = MACH_ALI_M36;

#if 1
	prom_init_cmdline();
	memsize_str = prom_getenv("memsize");
#endif

#ifdef CONFIG_OF

#ifdef CONFIG_ALI_BUILTIN_DTB
	__dt_setup_arch(&__dtb_start);
#endif

#ifdef CONFIG_ALI_GET_DTB
	__dt_setup_arch((void *)fw_arg1);
#endif

#endif

	printk("%s,%d fw_arg1 =%x\n", __FUNCTION__, __LINE__, (unsigned int)fw_arg1);
	customize_board_setting();
	//ali_reserve_mem();	

	/*UART FIFO work mode.*/	
	*((volatile unsigned char *)(0xB8018303)) = 0x83;
	*((volatile unsigned char *)(0xB8018300)) = 0x01;	  	
	*((volatile unsigned char *)(0xB8018301)) = 0x00;	  		
	*((volatile unsigned char *)(0xB8018303)) &= 0x7F;	
	*((volatile unsigned char *)(0xB8018305)) = 0x00;		//Reset Line Status
	*((volatile unsigned char *)(0xB8018302)) = 0x47;	
	*((volatile unsigned char *)(0xB8018304)) = 0x03;	
}
