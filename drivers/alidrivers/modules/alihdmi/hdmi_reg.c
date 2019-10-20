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

/***************************************************************************************************
*
*    File:    
*		hdmi_reg.c
*
*    Description:    
*		ALi HDMI Driver
*
*    History:
*	 	Date           Author        	Version     				Reason
*	 	============	=============	=========	=================
*
*************************************************************************************************/
#include <linux/kernel.h>
#include "hdmi_register.h"

#include "hdmi_proc.h"
#include "hdmi_infoframe.h"
#include "hdmi_edid.h"

//offset 0x0B
void hdmi_set_txp_ctl0(unsigned char cur)
{
	HDMI_REG_CFG2 = (HDMI_REG_CFG2 & (~B_TXP_CTL0_2_0)) | ((cur << 0) & B_TXP_CTL0_2_0);
}

//offset 0x0B
void hdmi_set_txn_ctl0(unsigned char cur)
{
	HDMI_REG_CFG2 = (HDMI_REG_CFG2 & (~B_TXN_CTL0_2_0)) | ((cur << 3) & B_TXN_CTL0_2_0);
}

//offset 0x0B,0x0C
void hdmi_set_txp_ctl1(unsigned char cur)
{
	HDMI_REG_CFG2 = (HDMI_REG_CFG2 & (~B_TXP_CTL1_1_0)) | ((cur << 6) & B_TXP_CTL1_1_0);
	HDMI_REG_CFG3 = (HDMI_REG_CFG3 & (~B_TXP_CTL1_2)) | ((cur >> 2) & B_TXP_CTL1_2);
}

//offset 0x0C
void hdmi_set_txn_ctl1(unsigned char cur)
{
	HDMI_REG_CFG3 = (HDMI_REG_CFG3 & (~B_TXN_CTL1_2_0)) | ((cur << 1) & B_TXN_CTL1_2_0);
}

//offset 0x0C,0x0D
void hdmi_set_txn_ctl2(unsigned char cur)
{
	HDMI_REG_CFG3 = (HDMI_REG_CFG3 & (~B_TXN_CTL2_0)) | ((cur << 7) & B_TXN_CTL2_0);
	HDMI_REG_CFG4 = (HDMI_REG_CFG4 & (~B_TXN_CTL2_2_1)) | ((cur >> 1) & B_TXN_CTL2_2_1);
}

//offset 0x0C
void hdmi_set_txp_ctl2(unsigned char cur)
{
	HDMI_REG_CFG3 = (HDMI_REG_CFG3 & (~B_TXP_CTL2_2_0)) | ((cur << 4) & B_TXP_CTL2_2_0);
}

//offset 0x0D
void hdmi_set_txp_ctl3(unsigned char cur)
{
	HDMI_REG_CFG4 = (HDMI_REG_CFG4 & (~B_TXP_CTL3_2_0)) | ((cur << 2) & B_TXP_CTL3_2_0);
}

//offset 0x0D
void hdmi_set_txn_ctl3(unsigned char cur)
{
	HDMI_REG_CFG4 = (HDMI_REG_CFG4 & (~B_TXN_CTL3_2_0)) | ((cur << 5) & B_TXN_CTL3_2_0);
}

//offset 0x0E[1:0],0x0F[4]
void hdmi_set_pll_sel(unsigned char pll_sel)
{
	unsigned char buf = 0x00;
	buf = HDMI_REG_CFG5;
	HDMI_REG_CFG5 = (buf & ~(B_PLL_SEL_1_0)) | (pll_sel & B_PLL_SEL_1_0);

	buf = HDMI_REG_CFG6;
	HDMI_REG_CFG6 = (buf & ~B_PLL_SEL_2) | ((pll_sel<<2) & B_PLL_SEL_2);
}

//offset 0x0E[7]
void hdmi_set_T_sel_type(bool bSelect)
{
	unsigned char buf = 0x00;
	buf = HDMI_REG_CFG5;
	HDMI_REG_CFG5 = (bSelect == true) ? (buf | B_T_SEL) : (buf & (~B_T_SEL));
}

//offset 0x0A,0x0E
void hdmi_set_ldo_sel(unsigned char ldo)
{
	unsigned char buf = 0x00;
	buf = HDMI_REG_CFG1;
	HDMI_REG_CFG1 = ( buf & (~B_LDO_SEL0) ) | ((ldo << 7) & B_LDO_SEL0);
	buf = HDMI_REG_CFG5;
	HDMI_REG_CFG5 = ( buf & (~B_LDO_SEL1) ) | ((ldo << 6) & B_LDO_SEL1);
}

// offset 0x0F[4:1]
void hdmi_set_emp_en(unsigned char emp_en)
{
	HDMI_REG_CFG6 = (HDMI_REG_CFG6 & (~B_EMP_EN)) | ((emp_en << 1) & B_EMP_EN);
}

INT32 hdmi_get_hdmi_interrupt2_source(void)
{
	// [0] : PP INT
	UINT8 buf = (HDMI_REG_PP_REG2& B_PP_VALUE_INT);
	return buf;
}

/*******************************************
* 
*       3821 PHY design
*
********************************************/
// offset 0x0A
void hdmi_set_cmu_voldet_pd(bool b_enable)
{
	HDMI_REG_CFG1 = (b_enable==TRUE) ? (HDMI_REG_CFG1 | B_CMU_VOLDET_PD) : (HDMI_REG_CFG1 & (~B_CMU_VOLDET_PD));
}
//offset 0x0B
void hdmi_set_ida_ccntl(unsigned char cur)
{
	HDMI_REG_CFG2= (HDMI_REG_CFG2 & (~B_IDA_CCNTL)) |((cur << 0) & B_IDA_CCNTL);
}
//offset 0x0B
void hdmi_set_ick_ccntl(unsigned char cur)
{
	HDMI_REG_CFG2= (HDMI_REG_CFG2 & (~B_ICK_CCNTL)) |((cur << 4) & B_ICK_CCNTL);
}
//offset 0x0C
void hdmi_set_ida_fcntl(unsigned char cur)
{
	HDMI_REG_CFG3= (HDMI_REG_CFG3 & (~B_IDA_FCNTL)) |((cur << 0) & B_IDA_FCNTL);
}
//offset 0x0C
void hdmi_set_ick_fcntl(unsigned char cur)
{
	HDMI_REG_CFG3= (HDMI_REG_CFG3 & (~B_ICK_FCNTL)) |((cur << 2) & B_ICK_FCNTL);
}
//offset 0x6B
void hdmi_set_drvbst_cntl(unsigned char cur)
{
	HDMI_REG_PHY_REG1= (HDMI_REG_PHY_REG1 & (~B_DRVBST_CNTL)) |((cur << 4) & B_DRVBST_CNTL);
}
//offset 0x0C
void hdmi_set_dterm(unsigned char cur)
{
	HDMI_REG_CFG3= (HDMI_REG_CFG3 & (~B_DTERM)) |((cur << 4) & B_DTERM);
}

INT32 hdmi_get_phy_interrupt_source(void)
{
	// [0] : PCG INT, [1] : CMU INT
	UINT8 buf = ((HDMI_REG_CFG3 & B_PCG_RDY) >> 7) | ((HDMI_REG_CFG4 & B_CMU_RDY) >> 6);
	return buf;
}

// offset 0x0C
void hdmi_set_pcg_rdy(bool b_enable)
{
	HDMI_REG_CFG3 = (b_enable) ? (HDMI_REG_CFG3|(B_PCG_RDY)) : (HDMI_REG_CFG3&(~(B_PCG_RDY)));
}

//offset 0x0C
INT32 hdmi_get_pcg_rdy(void)
{
	return (HDMI_REG_CFG3 & B_PCG_RDY);
}

// offset 0x0D
void hdmi_set_cmu_sel(unsigned char cur)
{
	HDMI_REG_CFG4 = (HDMI_REG_CFG4 & (~B_CMU_SEL) ) | ((cur << 0) & B_CMU_SEL);
}

//offset 0x0D
void hdmi_set_cterm(unsigned char cur)
{
	HDMI_REG_CFG4= (HDMI_REG_CFG4 & (~B_CTERM)) |((cur << 4) & B_CTERM);
}

// offset 0x0D
void hdmi_set_cmu_rdy(bool b_enable)
{
	HDMI_REG_CFG4 = (b_enable) ? (HDMI_REG_CFG4|(B_CMU_RDY)) : (HDMI_REG_CFG4&(~(B_CMU_RDY)));
}

//offset 0x0D
INT32 hdmi_get_cmu_rdy(void)
{
	return (HDMI_REG_CFG4 & B_CMU_RDY);
}

//offset 0x0E
void hdmi_set_cmu_pd(bool b_enable)
{
	HDMI_REG_CFG5 = (b_enable) ? (HDMI_REG_CFG5|(B_CMU_PD)) : (HDMI_REG_CFG5&(~(B_CMU_PD)));
}
//offset 0x0E
void hdmi_set_cmu_vcosel(unsigned char cur)
{
	HDMI_REG_CFG5= (HDMI_REG_CFG5 & (~B_CMU_VCOSEL)) |((cur << 0) & B_CMU_VCOSEL);
}

//offset 0x6A[2]
void hdmi_set_phy_sel0(unsigned char cur)
{
	HDMI_REG_PHY_REG0 = (HDMI_REG_PHY_REG0 & (~B_SEL0)) |((cur << 2) & B_SEL0);
}

//offset 0x6A[4:3]
void hdmi_set_phy_sel1_2(unsigned char cur)
{
	HDMI_REG_PHY_REG0 = (HDMI_REG_PHY_REG0 & (~B_SEL1_2)) |((cur << 3) & B_SEL1_2);
}

//offset 0x6A[5]
void hdmi_set_phy_sel3(unsigned char cur)
{
	HDMI_REG_PHY_REG0 = (HDMI_REG_PHY_REG0 & (~B_SEL3)) |((cur << 5) & B_SEL3);
}

// offser 0x97[3:2]
void hdmi_set_phy_async_ini(unsigned char cur)
{
	HDMI_REG_DP_REG1= (HDMI_REG_DP_REG1 & (~B_PHY_ASYNC_INI)) | ((cur << 2) & B_PHY_ASYNC_INI);
}

//offset 0x0F
void hdmi_set_phy_async_set(BOOL b_enable)
{
	HDMI_REG_CFG6 = (b_enable) ? (HDMI_REG_CFG6|(B_PHY_ASYNC_SET)) : (HDMI_REG_CFG6&(~(B_PHY_ASYNC_SET)));
}

// System Register, 0xB8000630
void hdmi_set_phy_pcg_powerdown(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = (b_enable) ? (M36F_SYS_REG_HDMI_PHY0_CTRL|(0x01)) : (M36F_SYS_REG_HDMI_PHY0_CTRL&(~(0x01)));
}
// System Register, 0xB8000630
void hdmi_set_phy_drv_trim(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = (M36F_SYS_REG_HDMI_PHY0_CTRL & (~0x1C)) | ((cur << 2) & 0x1C); 
}
// System Register, 0xB8000631
void hdmi_set_phy_reference_clk(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY1_CTRL = (b_enable) ? (M36F_SYS_REG_HDMI_PHY1_CTRL|(0x02)) : (M36F_SYS_REG_HDMI_PHY1_CTRL&(~(0x02)));
}
// System Register, 0xB8000631
void hdmi_set_phy_rst(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY1_CTRL = (b_enable) ? (M36F_SYS_REG_HDMI_PHY1_CTRL|(0x01)) : (M36F_SYS_REG_HDMI_PHY1_CTRL&(~(0x01)));
}
// System Register, 0xB8000632
void hdmi_set_phy_deepcolor_sel(unsigned char mode)
{
	M36F_SYS_REG_HDMI_PHY2_CTRL = (M36F_SYS_REG_HDMI_PHY2_CTRL & (~0x03)) | (mode & 0x03); 
}
// System Register, 0xB8000633
void hdmi_set_phy_pcg_sel(unsigned char select)
{
	M36F_SYS_REG_HDMI_PHY3_CTRL = (M36F_SYS_REG_HDMI_PHY3_CTRL & (~0x0F)) | (select & 0x0F); 
}

/*******************************************
* 
*       3921 PHY design
*
********************************************/
// offser 0x96[2:0]
void hdmi_set_yuv_ch_sel(unsigned char cur)
{
#ifdef CONFIG_ALI_CHIP_M3921
	HDMI_REG_DP_REG0= (HDMI_REG_DP_REG0 & (~B_YUV_CH_SEL)) | ((cur << 0) & B_YUV_CH_SEL);
#else
	HDMI_REG_DP_REG2= (HDMI_REG_DP_REG2 & (~B_YUV_CH_SEL)) | ((cur << 4) & B_YUV_CH_SEL);
#endif
}
// offser 0x96[5:4]
void hdmi_set_de_data_width(unsigned char cur)
{
	HDMI_REG_DP_REG0= (HDMI_REG_DP_REG0 & (~B_DE_DATA_WIDTH)) | ((cur << 4) & B_DE_DATA_WIDTH);
}
// offset 0x97[2:1]
void hdmi_set_vou_buf_addr(unsigned char cur)
{
	HDMI_REG_DP_REG1= (HDMI_REG_DP_REG1 & (~B_VOU_BUF_ADDR_INI)) | ((cur << 1) & B_VOU_BUF_ADDR_INI);
}
// offser 0x97[5:4]
void hdmi_set_dp_mode(unsigned char cur)
{
	HDMI_REG_DP_REG1= (HDMI_REG_DP_REG1 & (~B_DP_MODE)) | ((cur << 4) & B_DP_MODE);
}
// offser 0x97[7:6]
void hdmi_set_data_buf_grp_sel(unsigned char cur)
{
	HDMI_REG_DP_REG1= (HDMI_REG_DP_REG1 & (~B_DATA_BUF_GRP_SEL)) | ((cur << 6) & B_DATA_BUF_GRP_SEL);
}
//offset 0x9B [6:4]
unsigned char hdmi_get_pp_value(void)
{
	return ((HDMI_REG_PP_REG1 & B_PP_VALUE) >> 4);
}

#ifdef CONFIG_ALI_CHIP_M3921
// System Register, 0x1800006E
void hdmi_set_3d_phy_power(bool bPower)
{
	// Power up
	if(bPower)
	{
		M39_SYS_REG_HDMI_3DPHY = M39_SYS_REG_HDMI_3DPHY | 0x04;
		M39_SYS_REG_HDMI_3DPHY = M39_SYS_REG_HDMI_3DPHY & (~0x01);
	}
	// Power down
	else
	{
		M39_SYS_REG_HDMI_3DPHY = M39_SYS_REG_HDMI_3DPHY | 0x01;
		M39_SYS_REG_HDMI_3DPHY = M39_SYS_REG_HDMI_3DPHY & (~0x04);
	}
}
#endif
/*******************************************
* 
*       3505 PHY design
*
********************************************/
void hdmi_set_drvbw_cntl(unsigned char cur)
{
	HDMI_REG_PHY_REG0 = (HDMI_REG_PHY_REG0 & (~0x03)) | (cur & 0x03); 
}

void hdmi_set_predrv_cntl(unsigned char cur)
{
	HDMI_REG_PP_REG2 = (HDMI_REG_PP_REG2 & (~0xF0)) | ((cur << 4) & 0xF0); 
}

void hdmi_set_sys_pll_ldo_pd(bool b_enable)
{
	M36F_SYS_REG_PLL_VCXO_0 = (b_enable) ? (M36F_SYS_REG_PLL_VCXO_0|(0x01)) : (M36F_SYS_REG_PLL_VCXO_0&(~(0x01)));
}

void hdmi_set_sys_pll_ldo_sel(unsigned char cur)
{
	M36F_SYS_REG_PLL_VCXO_1 = (M36F_SYS_REG_PLL_VCXO_1 & (~0x03)) | (cur & 0x03); 
}

void hdmi_set_sys_spll_foutsel(unsigned char cur)
{
	M36F_SYS_REG_PLL_VCXO_2 = (M36F_SYS_REG_PLL_VCXO_2 & (~0xE0)) | ((cur << 5) & 0xE0); 
}

void hdmi_set_sys_spll_hdmi_sel(unsigned char cur)
{
	M36F_SYS_REG_PLL_VCXO_2 = (M36F_SYS_REG_PLL_VCXO_2 & (~0x10)) | ((cur << 4) & 0x10); 
}

 void hdmi_set_sys_cmu_pd(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = 
	(b_enable) ? (M36F_SYS_REG_HDMI_PHY0_CTRL|(0x80)) : (M36F_SYS_REG_HDMI_PHY0_CTRL&(~(0x80)));
}

void hdmi_set_sys_drv_trim(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = (M36F_SYS_REG_HDMI_PHY0_CTRL & (~0x70)) | ((cur << 4) & 0x70); 
}

void hdmi_set_sys_ldorck_pd(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = 
	(b_enable) ? (M36F_SYS_REG_HDMI_PHY0_CTRL|(0x08)) : (M36F_SYS_REG_HDMI_PHY0_CTRL&(~(0x08)));
}

void hdmi_set_sys_pd_hdmi(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = 
	(b_enable) ? (M36F_SYS_REG_HDMI_PHY0_CTRL|(0x04)) : (M36F_SYS_REG_HDMI_PHY0_CTRL&(~(0x04)));
}

void hdmi_set_sys_iref_sel(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY0_CTRL = 
	(b_enable) ? (M36F_SYS_REG_HDMI_PHY0_CTRL|(0x02)) : (M36F_SYS_REG_HDMI_PHY0_CTRL&(~(0x02)));
}

void hdmi_set_sys_vref_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY1_CTRL = (M36F_SYS_REG_HDMI_PHY1_CTRL & (~0xF0)) | ((cur << 4) & 0xF0); 
}

void hdmi_set_sys_r_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY1_CTRL = (M36F_SYS_REG_HDMI_PHY1_CTRL & (~0x0E)) | ((cur << 1) & 0x0E); 
}

void hdmi_set_sys_ldorck_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY2_CTRL = (M36F_SYS_REG_HDMI_PHY2_CTRL & (~0xC0)) | ((cur << 6) & 0xC0); 
}

void hdmi_set_sys_ldo_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY2_CTRL = (M36F_SYS_REG_HDMI_PHY2_CTRL & (~0x30)) | ((cur << 4) & 0x30); 
}

void hdmi_set_sys_rcksel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY2_CTRL = (M36F_SYS_REG_HDMI_PHY2_CTRL & (~0x0C)) | ((cur << 2) & 0x0C); 
}

void hdmi_set_sys_pre_div_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY3_CTRL = (M36F_SYS_REG_HDMI_PHY3_CTRL & (~0xC0)) | ((cur << 6) & 0xC0); 
}

void hdmi_set_sys_post_div_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY3_CTRL = (M36F_SYS_REG_HDMI_PHY3_CTRL & (~0x30)) | ((cur << 4) & 0x30); 
}

void hdmi_set_sys_ddp_spdif_sel(bool b_enable)
{
	M36F_SYS_REG_HDMI_PHY3_CTRL = 
	(b_enable) ? (M36F_SYS_REG_HDMI_PHY3_CTRL|(0x08)) : (M36F_SYS_REG_HDMI_PHY3_CTRL&(~(0x08)));
}

void hdmi_set_sys_pcg_sel(unsigned char cur)
{
	M36F_SYS_REG_HDMI_PHY3_CTRL = (M36F_SYS_REG_HDMI_PHY3_CTRL & (~0x07)) | (cur & 0x07); 
}

