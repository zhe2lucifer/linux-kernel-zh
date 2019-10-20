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

/************************************************************************************************************
*
* File: hdmi_cec.c
*
* Description�GThis is a simple sample file to illustrate format of file
*             prologue.
*
* History:
*     Date         By          Reason           Ver.
*   ==========  =========    ================= ======
************************************************************************************************************/
/*******************
* INCLUDE FILES    *
********************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/wait.h>

#include "hdmi_cec.h"
#include "hdmi_proc.h"
#include "hdmi_register.h"

extern HDMI_PRIVATE_DATA* hdmi_drv;

/************************************************************************************************************
* NAME: hdmi_cec_polling_message
* Returns : bool
* Additional information:
* For Address Allocation
************************************************************************************************************/
bool hdmi_cec_polling_message(unsigned char dest_addr)
{
	unsigned char buf;

	buf = (hdmi_drv->cec.logical_address <<4) | dest_addr;
	CEC_DEBUG("<Polling> 0x%.2x \n", buf);
	return hdmi_cec_transmit(&buf, 1);
}

/************************************************************************************************************
* NAME: hdmi_cec_polling_message
* Returns : void
* Additional information:
* For Address Allocation
************************************************************************************************************/
void hdmi_cec_report_physicalAddr(void)
{
	unsigned char buf[5];

	buf[0] = (hdmi_drv->cec.logical_address<<4) | LOGADDR_UNREG_BRDCST;
	buf[1] = 0x84;
	buf[2] = (unsigned char) (hdmi_drv->edid.physical_address >>8);
	buf[3] = (unsigned char) (hdmi_drv->edid.physical_address & 0xFF);
	buf[4] = 3; // "TV" =0, "Recording Device"=1, "STB"=3, "DVD"=4, "Audio"System"=5.

	CEC_DEBUG("<Report Physical Address> 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
	hdmi_cec_transmit(buf, 5);
}

/************************************************************************************************************
* NAME: hdmi_cec_init
* Returns : void
* Additional information:
* Add this function to hdmi_porc.c: hdmi_init_hw_config funciton to initial CEC module.
*
************************************************************************************************************/
void hdmi_cec_init()
{

	// Set CEC RESET to TRUE
	CEC_DEBUG("%s: Set CEC RESET to TRUE\n", __FUNCTION__);
	HDMI_REG_OPT2 = HDMI_REG_OPT2 | B_CEC_RST;

	// debug mode
	//HDMI_REG_OPT1 = (HDMI_REG_OPT1 & 0x7F) | 0x40;

	hdmi_set_biu_frequency();

	// Clear Interrupt (CEC_STA) and unmask CEC interrupt.
	CEC_DEBUG("%s: Clear Interrupt (CEC_STA) and unmask CEC interrupt\n", __FUNCTION__);
	hdmi_clear_cec_status();
	HDMI_REG_OPT1 = HDMI_REG_OPT1 & (~B_CEC_MSK);	//unmask CEC interrupt

    // Set CEC RESET to FALSE
	CEC_DEBUG("%s: Set CEC RESET to FALSE\n", __FUNCTION__);
	HDMI_REG_OPT2 = HDMI_REG_OPT2 & (~B_CEC_RST);

	return;
}

/************************************************************************************************************
* NAME: hdmi_cec_transmit(unsigned char* Data, unsigned char Data_length)
*
* Returns : SUCCESS (ACK) /ERR_FAILUE (NACK)
*
* Transmit cec message data, with max length 16 blocks.
************************************************************************************************************/
bool hdmi_cec_transmit(unsigned char* Data, unsigned char Data_length)
{
	unsigned char i=0;
	
	CEC_DEBUG("%s len 0x%x\n", __FUNCTION__, Data_length);

	// Set Broadcast bit
	HDMI_REG_CEC_DIV_NUM = ((Data[0] & LOGADDR_UNREG_BRDCST) == LOGADDR_UNREG_BRDCST) ?
							(HDMI_REG_CEC_DIV_NUM|B_BRDCST) : (HDMI_REG_CEC_DIV_NUM & (~B_BRDCST));

	// Write Transmit Data to Buffer
	for( i=0; i< Data_length; i++)
	{
        HDMI_REG_CEC_TX_ADDR = (i & B_CEC_TX_REG_ADDR) | B_CEC_TX_REG_RW;		// Fill Address
        HDMI_REG_CEC_TX_DATA = Data[i];

        if(i == (Data_length-1))
        {
            HDMI_REG_CEC_CTRL = HDMI_REG_CEC_CTRL | B_CEC_TX_EOM;
            HDMI_REG_CEC_TX_ADDR = HDMI_REG_CEC_TX_ADDR & ~(B_CEC_TX_REG_RW);
            HDMI_REG_CEC_CTRL = HDMI_REG_CEC_CTRL | B_CEC_TX_EN;
        }
        else
        {
            HDMI_REG_CEC_CTRL = HDMI_REG_CEC_CTRL  & ~(B_CEC_TX_EOM);
        }
	}

	// Wait transmit done interrupt. (4.5 (start bit) + 2.4 (ms/bit) * 10(10bits/block) * 16(max blocks)) * 5 (retry) = 1942.5 ms
    wait_event_interruptible_timeout(hdmi_drv->control.wait_queue, hdmi_drv->control.cec_int_status & B_TX_DONE, 2000);
    if((hdmi_drv->control.cec_int_status & B_TX_DONE) == B_TX_DONE )
    {
		hdmi_drv->control.cec_int_status &= ~B_TX_DONE;

		if((Data[0] & LOGADDR_UNREG_BRDCST) == LOGADDR_UNREG_BRDCST)
			HDMI_REG_CEC_DIV_NUM &= ~(B_BRDCST);			// Clear Broadcast bit

		// Check ACK
		if( (HDMI_REG_CEC_CTRL & B_CEC_TX_TRY_DONE) == B_CEC_TX_TRY_DONE)
		{
            CEC_DEBUG("hdmi_cec_transmit: Already retry 4 times for unsuccessful transmission, CEC Transmit Fail\n");
			HDMI_REG_CEC_CTRL |= B_CEC_TX_TRY_DONE;
			return false;	
		}
		else
		{
            CEC_DEBUG("hdmi_cec_transmit: Got HDMI_FLG_CEC_TX_DONE, CEC Transmit Success\n");
			return true;
		}
    }
	else
	{
    	wait_event_interruptible_timeout(hdmi_drv->control.wait_queue, hdmi_drv->control.cec_int_status & B_RCV_UNKNOW, 1);
    	if((hdmi_drv->control.cec_int_status & B_RCV_UNKNOW) == B_RCV_UNKNOW )
    	{
 			CEC_DEBUG("hdmi_cec_transmit: Got HDMI_FLG_CEC_RCV_UNKNOWN, CEC Transmit Fail\n");
 			hdmi_drv->control.cec_int_status &= ~B_RCV_UNKNOW;
			if((Data[0] & LOGADDR_UNREG_BRDCST) == LOGADDR_UNREG_BRDCST)
				HDMI_REG_CEC_DIV_NUM &= ~(B_BRDCST);			// Clear Broadcast bit
    	}
		else
		{
			if(HDMI_REG_CEC_CTRL & B_CEC_TX_TRY_DONE)
			{
				CEC_DEBUG("hdmi_cec_transmit: CEC_TX_TRY_DONE\n");
				HDMI_REG_CEC_CTRL |= B_CEC_TX_TRY_DONE;
			}

		}
		return false;
	}
}

/************************************************************************************************************
* NAME: hdmi_cec_receive(unsigned char* buf)
*
* Returns : unsigned char recieve message block length
*
* Receive cec message data, with max length 16 blocks.
************************************************************************************************************/
unsigned char hdmi_cec_receive(unsigned char* buf)
{
	unsigned char message_length, i;

	// Read Number of message size
	message_length = HDMI_REG_CEC_RX_ADDR+1;

	// Read Block from i = 0 to message_length-1
	for(i = 0; i<message_length; i++)
	{
		HDMI_REG_CEC_RX_ADDR = i & B_CEC_RX_BLOCK_CNT;
		buf[i] = HDMI_REG_CEC_RX_DATA;
	}

	hdmi_clear_cec_datastatus();
	
	return message_length;
}

/************************************************************************************************************
* NAME: hdmi_set_biu_frequency( void)
*
* Returns : void
*
* Set BIU Frequency according HDMI PHY clock. this register should be set per each video resolution change.
************************************************************************************************************/
inline void hdmi_set_biu_frequency( void)
{
    int rate = 0;

    // LOGIC Address, Need to perform Logic address allocation after read EDID physical address.
	CEC_DEBUG("%s: Set LOGIC Address to LOGADDR_UNREG_BRDCST\n", __FUNCTION__);
	hdmi_set_cec_logic_address(LOGADDR_UNREG_BRDCST);

	// Set BIU Frequency according HDMI PHY clock. this register should be set per each video resolution change.
	switch(hdmi_drv->tmds_clock)
	{
		case HDMI_TMDS_CLK_27000:
		case HDMI_TMDS_CLK_74250:
		case HDMI_TMDS_CLK_148500:
		case HDMI_TMDS_CLK_297000:
			rate = 100;
			break;
		case HDMI_TMDS_CLK_27000_125:
		case HDMI_TMDS_CLK_74250_125:
		case HDMI_TMDS_CLK_148500_125:
			rate = 125;
			break;
		case HDMI_TMDS_CLK_27000_15:
		case HDMI_TMDS_CLK_74250_15:
		case HDMI_TMDS_CLK_148500_15:
			rate = 150;
			break;
		case HDMI_TMDS_CLK_27000_2:
		case HDMI_TMDS_CLK_74250_2:
		case HDMI_TMDS_CLK_148500_2:
			rate = 200;
			break;
		default:
			rate = 100;
			break;
	
	}
	if(hdmi_drv->chip_info.chip_id == 0x3503)
	{
	switch((M36_SYS_REG_SYS_CTRL & 0x00000070) >> 4) // read phy clock from system register
	{
			case 0x00: //27 MHz
				  rate = (rate*(27-1))/100;
			      break;
			case 0x01: //54M
				  rate = (rate*(54-1))/100;
			      break;
			case 0x02: //74M
				   rate = (rate*74)/100;
			      break;
			case 0x03: //72M
				  rate = (rate*(72-1))/100;
			      break;
			case 0x04: //48M
				  rate = (rate*(48-1))/100;
			      break;
			case 0x05: //148M
				  rate = (rate*(148-1))/100;
			      break;
		}
	}
	else if((hdmi_drv->chip_info.chip_id == 0x3921) ||(hdmi_drv->chip_info.chip_id == 0x3821))
	{
		switch((M36_SYS_REG_SYS_CTRL & 0x00000070) >> 4) // read phy clock from system register
		{
			case 0x00: //27 MHz
				  rate = (rate*(27-1))/100;
			      break;
			case 0x01: //54M
			 	  rate = (rate*(54-1))/100;
			      break;
			case 0x02: //108M
				  rate = (rate*108)/100;
			      break;
			case 0x03: //74M
				  rate = (rate*74)/100;
			      break;
			case 0x04: //148M  will big than 255 but not good idea.
				  rate = (rate*148)/100;
				  if(rate > 255)
				  {
				  	rate = 255;
				  }
			      break;
			case 0x05: //297M //not support
				  rate = 297;
			      break;
			default :
				  rate = 297;
				break;
		}
	}
	else if(hdmi_drv->chip_info.chip_id == 0x3505)
	{
		switch((M36_SYS_REG_SYS_CTRL & 0x00000007)) // read phy clock from system register
		{
			case 0x00: //27 MHz
				  rate = (rate*(27-1))/100;
			      break;
			case 0x01: //54M
			 	  rate = (rate*(54-1))/100;
			      break;
			case 0x02: //108M
				  rate = (rate*108)/100;
			      break;
			case 0x03: //74M
				  rate = (rate*74)/100;
			      break;
			case 0x04: //148M  will big than 255 but not good idea.
				  rate = (rate*148)/100;
				  if(rate > 255)
				  {
				  	rate = 26;
				  }
			      break;
			case 0x05: //297M //not support
				  rate = 297;
			      break;
			default :
				  rate = 297;
				break;

		}
	}
	
	HDMI_REG_CEC_BIU_FREQ = rate;
	CEC_DEBUG("%s:Set BIU Frequency to %d\n", __FUNCTION__,rate);
	
	// DIVISER Number: 0x31 = 49. suggested value from Jia Lin Sheu.
	if(255 == rate)
	{
		HDMI_REG_CEC_DIV_NUM = (HDMI_REG_CEC_DIV_NUM & (~B_DIV_NUM)) | 57;
		CEC_DEBUG("%s:Set DIV_NUM to %d\n", __FUNCTION__,57);
	}
	else
	{
		HDMI_REG_CEC_DIV_NUM = (HDMI_REG_CEC_DIV_NUM & (~B_DIV_NUM)) | (0x31 & B_DIV_NUM);
	}
    // Low_CNT, Sampling timing count value tried by Victor.
	CEC_DEBUG("%s: Set Low_CNT to 0x06\n", __FUNCTION__);
	HDMI_REG_CEC_LOW_CNT = (HDMI_REG_CEC_LOW_CNT & (~B_LOW_CNT)) | (0x06 & B_LOW_CNT);

}

/************************************************************************************************************
*    CEC Module Register set access functions.
************************************************************************************************************/
// offset 0x6E[3:0]
inline void hdmi_set_cec_logic_address( unsigned char cec_address)
{
	HDMI_REG_OPT1 = (HDMI_REG_OPT1 & (~B_CEC_ADDR)) | (cec_address & B_CEC_ADDR);
}

inline unsigned char hdmi_get_cec_logic_address(void)
{
	return (HDMI_REG_OPT1 & B_CEC_ADDR);
}

// offset 0x6E[5]
void hdmi_set_cec_intr_mask(bool bMask)
{
    if(bMask == true)
    	HDMI_REG_OPT1 = (HDMI_REG_OPT1 | B_CEC_MSK);	//mask CEC interrupt
    else
   	    HDMI_REG_OPT1 = (HDMI_REG_OPT1 & (~B_CEC_MSK));	//unmask CEC interrupt
}

// offset 0x6F[2]
inline void hdmi_set_cec_rst( bool cec_reset)
{
	HDMI_REG_OPT2 = (cec_reset) ? (HDMI_REG_OPT2 | B_CEC_RST) : (HDMI_REG_OPT2 & (~B_CEC_RST));
}

//offset 0x71
inline unsigned char hdmi_get_cec_status(void)
{
	return HDMI_REG_CEC_STATUS;
}

// offse 0x 71
inline void hdmi_clear_cec_status(void)
{
	HDMI_REG_CEC_STATUS = HDMI_REG_CEC_STATUS;
}

//offset 0x76
inline unsigned char hdmi_get_cec_datastatus(void)
{
	return HDMI_REG_CEC_CTRL;
}

//offset 0x76[4]
inline void hdmi_clear_cec_datastatus(void)
{
	HDMI_REG_CEC_CTRL |= B_CEC_RX_DATA_CLR;
}

