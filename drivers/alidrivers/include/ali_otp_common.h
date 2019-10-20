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
 
#ifndef _ALI_OTP_COMMON_H
#define _ALI_OTP_COMMON_H 

#define ALI_OTP_DEVNAME "ali_otp"
#define ALI_OTP_BASE_ADDR        0x18042000
#define MAX_ALI_OTP_PARAS 0x80
#define OTP_OFFSET 0x508

struct otp_read_paras {
    unsigned long offset;
    unsigned char *buf;
    int len;
};                    

struct otp_write_paras {
    unsigned char *buf;
    unsigned long offset;
    int len;
};  


#define OTP_ERR_NOSUPPORT		(-1)
#define OTP_ERR_LOCKTMO		(-2)

#define OTP_VOLTAGE_6V5 65    //OTP voltage 6.5V
#define OTP_VOLTAGE_1V8 18    //OTP voltage 1.8V

/*voltage control callback function, OTP driver will tell APP the correct voltage by 
* OTP_VOLTAGE_6V5 or OTP_VOLTAGE_6V5*/
typedef void(*ALI_SOC_OTP_VOLTAGE_CONTROL)(unsigned long voltage);

/* APP must make OTP driver to control programming voltage to guarantee program timming.
* So App can choose to register GPIO to OTP driver or register voltage control callback function to OTP driver*/
typedef struct {
	unsigned short	vpp_by_gpio: 1;		/*1: we need use one GPIO control vpp voltage.*/
								/*0: we use Jumper to switch vpp voltage.*/
	unsigned short	reserved1: 15;		/*reserved for future usage*/
	unsigned short	vpp_polar	: 1;		/* Polarity of GPIO, 0 or 1 active to set VPP to 6.5V*/
	unsigned short	vpp_io		: 1;		/* HAL_GPIO_I_DIR or HAL_GPIO_O_DIR in hal_gpio.h */
	unsigned short	vpp_position: 14;	/* GPIO Number*/
	ALI_SOC_OTP_VOLTAGE_CONTROL volctl_cb;		/*OTP program voltage control callback*/
										/*OTP_VOLTAGE_6V5 for 6.5V,OTP_VOLTAGE_1V8 for 1.8V*/
}SOC_OTP_CONFIG;


#define ALI_OTP_READ  _IOR('S', 1, struct otp_read_paras)
#define ALI_OTP_WRITE _IOW('S', 2, struct otp_write_paras)

int ali_otp_read(unsigned long offset, unsigned char *buf, int len);
int ali_otp_write(unsigned char *buf, unsigned long offset, int len);
int ali_otp_hw_init(void);


#define ENABLE   1
#define DISABLE  0

/*********************************************************************
		0xC0 - Configuration 0
*********************************************************************/
int otp_c0_as_bootrom_enabled(void);
int otp_c0_mem_split_enabled(void);
int otp_c0_io_split_enabled(void);
int otp_c0_uart_enabled(void);

int ali_sys_ic_sip_nor_enabled(void);
int ali_sys_ic_split_enabled(void);


#define OTP_CLOSE_UART 12                                                 
#define OTP_CONFIG_ADDR_03 0x18042044
/*
0: uart is not fused by OTP
1: uart is fused by OTP
*/
#define IS_UART_FUSE_ENABLE ((__REG32ALI(OTP_CONFIG_ADDR_03)>>OTP_CLOSE_UART)&0x1)

#endif 
