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

/* support read retry for MICRON
 * Extended commands for ONFI 
 */
#define NAND_CMD_MICRON_GET_FEATURES	0xEE
#define NAND_CMD_MICRON_SET_FEATURES	0xEF
/* Feature Address Definitions */
#define FADDR_TIMING_MODE		0x01
#define FADDR_READ_RETRY		0x89

/* for HYNIX
 * ????????????	ID AD DE 94 97 44 45 1xnm 2nd 64GbMLC D-die Not in AVL
 * H27UCG8T2ETR ID AD DE 94 A7 42 48 1xnm 2nd 64GbMLC E-die
 * H27UCG8T2ATR ID AD DE 94 DA 74 C4 F20 64Gb MLC 20nm A-die
 * H27UCG8T2BTR ID AD DE 94 EB 74 44 F20 64Gb MLC 20nm B-die
 * H27UBG8T2CTR ID AD D7 94 91 60 44 F20 32Gb MLC 20nm C-die
 *
 * ???????????? RRReg 38 39 3A 3B
 * H27UCG8T2ETR RRReg 38 39 3A 3B
 * H27UCG8T2ATR RRReg CC / BF / AA / AB / CD / AD / AE / AF
 * H27UCG8T2BTR RRReg B0 / B1 / B2 / B3 / B4 / B5 / B6 / B7
 * H27UBG8T2CTR RRReg B0 / B1 / B2 / B3 / B4 / B5 / B6 / B7
 *
 * ???????????? RRSeq FFh ¡V36h ¡V 38h ¡V 52h ¡V 16h ¡V 17h ¡V 04h ¡V 19h ¡V 00h ¡K 30h ¡V Data Out - FFh - 36h ¡V 38h ¡V 00h ¡V 16h ¡K
 * H27UCG8T2ETR RRSeq FFh ¡V36h ¡V 38h ¡V 52h ¡V 16h ¡V 17h ¡V 04h ¡V 19h ¡V 00h ¡K 30h ¡V Data Out - FFh - 36h ¡V 38h ¡V 00h ¡V 16h ¡K
 * H27UCG8T2ATR RRSeq FFh ¡V36h ¡VFFh¡V40h¡VCCh-4Dh -16h ¡V17h ¡V04h ¡V19h ¡V00h ¡K
 * H27UCG8T2BTR RRSeq FFh ¡V36h ¡VAEh¡V00h¡VB0h-4Dh -16h ¡V17h ¡V04h ¡V19h ¡V00h ¡K
 * H27UBG8T2CTR RRSeq FFh ¡V36h ¡VAEh¡V00h¡VB0h-4Dh -16h ¡V17h ¡V04h ¡V19h ¡V00h ¡K
 */

enum NF_CHIP_TYPE {
	RR_MODE_NONE = 0,
	RR_MODE_MICRON = 1,
	HYNIX_20NM_A_DIE = 2,
	HYNIX_20NM_B_DIE = 3,
	HYNIX_20NM_C_DIE = 4,
	RR_MODE_HYNIX_1XNM_32 = 5,
	RR_MODE_HYNIX_1XNM_64 = 6,
};


/* hynix read retry register*/
struct hynix_rr_param_reg_20nm
{
	u8 rr_reg[8];
};

/* hynix read retry set*/
struct hynix_rr_set_20nm
{
	struct hynix_rr_param_reg_20nm rr_set[8];
	struct hynix_rr_param_reg_20nm inv_rr_set[8];
};

/* hynix read retry table*/
struct hynix_rr_table_20nm
{
	u8 total_count;
	u8 reg_count;
	struct hynix_rr_set_20nm set[8];
  u8 rr_table[8][8];
};

struct hynix_rr_table_20nm hynix_20nm_table;
int g_retry_options_nb = 0;
int hynix_20nm_retry_count = 0;


void vfy_20nm_rr_tble(void)
{
	u8 set, copy, i;
  u8 inv_reg, reg;
	
	pr_info("total_count %d\n", hynix_20nm_table.total_count);
	pr_info("regl_count %d\n", hynix_20nm_table.reg_count);

	for (copy=0; copy<8; copy++)
	{
  	for (set=0; set<8; set++)
   	{
    	for (i=0; i<8; i++)
	    {                    
      	reg = hynix_20nm_table.set[set].rr_set[copy].rr_reg[i];
        inv_reg = hynix_20nm_table.set[set].inv_rr_set[copy].rr_reg[i];
        hynix_20nm_table.rr_table[copy][i] = reg;
        if ((reg ^ inv_reg) != 0xFF)	
        	break;                    
      }                
      if (i==8)
      	break;
    } 
	}
}

/* RR_REG */
/* A_DIE_RR 0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF */
/*BC_DIE_RR 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8 */
u8 A_DIE_RR[8] = {0xCC, 0xBF, 0xAA, 0xAB, 0xCD, 0xAD, 0xAE, 0xAF};
u8 BC_DIE_RR[8] = {0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8};
void hynix_set_param_rr_20nm(struct mtd_info *mtd, u8 *params)
{
	struct nand_chip *chip = NULL;
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	
	u8 *rr_addr;
	u8 *rr;
	int i;
	
	if(mtd->priv)
		chip = mtd->priv;
	else
		return;
	
	rr = params;
	if (host->nf_parm.rr_mode == HYNIX_20NM_A_DIE)
	{	
		rr_addr = (u8 *) &A_DIE_RR;
	}	
	else if ((host->nf_parm.rr_mode == HYNIX_20NM_B_DIE) ||
			(host->nf_parm.rr_mode == HYNIX_20NM_C_DIE))
	{		
		rr_addr = (u8 *) &BC_DIE_RR;
	}	
	else
	{
			pr_err("[ERR] no vlaid rr addr\n");
			return;
	}		
	
	chip->cmd_ctrl(mtd, 0x36, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(20);
	for (i=0; i<8; i++)
	{		
		chip->cmd_ctrl(mtd, rr_addr[i], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);
		ndelay(200);
		chip->cmd_ctrl(mtd, rr[i] , NAND_NCE | NAND_CTRL_CHANGE);
		ndelay(20);
	}	
	chip->cmd_ctrl(mtd, 0x16, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(20);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
}

int load_hynix_retry_table(struct mtd_info *mtd, u8 set)
{
	u8 *rr_reg = NULL;
	
	rr_reg = (u8 *) &hynix_20nm_table.rr_table[hynix_20nm_retry_count%set][0];	
	hynix_set_param_rr_20nm(mtd, rr_reg);	
  hynix_20nm_retry_count++; 
  return 0;     
}


/* do at initial */
/* svae table to hynix_20nm_table */
/* RR table OTP start*/
/* A-die FFh ¡V 36h ¡V FFh ¡V 40h ¡V CCh - 4Dh - 16h ¡V 17h ¡V 04h ¡V 19h ¡V 00h */
/* B-die FFh ¡V 36h ¡V AEh ¡V 00h ¡V B0h - 4Dh - 16h ¡V 17h ¡V 04h ¡V 19h ¡V 00h */
/* C-die FFh ¡V 36h ¡V AEh ¡V 00h ¡V B0h - 4Dh - 16h ¡V 17h ¡V 04h ¡V 19h ¡V 00h */
u8 A_DIE_CMD[16] =   {0x36, 0xFF, 0x40, 0xCC, 0x4D, 0x16, 0x17, 0x04, 0x19, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x30};
u8 BC_DIE_CMD[16] = {0x36, 0xAE, 0x00, 0xB0, 0x4D, 0x16, 0x17, 0x04, 0x19, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x30};
int hynix_get_rr_table_20nm(struct mtd_info *mtd)
{
struct ali_nand_host *host = to_ali_nand_host(mtd);
struct nand_chip *chip = NULL;

int i;
u8 *p;
u8 *cmd;
	
	if(mtd->priv)
	{
		chip = mtd->priv;
	}
	else
	{
		return -1;
	}

	
	if (host->nf_parm.rr_mode == HYNIX_20NM_A_DIE)
	{
		cmd = (u8 *)&A_DIE_CMD;
	}
	else if ((host->nf_parm.rr_mode == HYNIX_20NM_B_DIE) ||
			(host->nf_parm.rr_mode == HYNIX_20NM_C_DIE))
	{
		cmd = (u8 *)&BC_DIE_CMD;
	}		
	else
	{
			pr_err("[ERR] no vlaid rr cmd\n");
			return -1;
	}	

	pr_info("rr_mode=%d\n", host->nf_parm.rr_mode);
	
	//hynix_20nm_table.pass_rr_set = 0;
	//hynix_20nm_table.pass_rr_copy = 0;
	
	// Read Retry Table
	nf_ctrl(mtd, 0xFF, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	udelay(20); // T-RST
		
	nf_ctrl(mtd, cmd[0], NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[1], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[2], NAND_NCE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[3], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[4], NAND_NCE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[5], NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[6], NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[7], NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[8], NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[9], NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, cmd[10], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[11], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[12], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[13], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[14], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);
	nf_ctrl(mtd, cmd[15], NAND_NCE | NAND_ALE | NAND_CTRL_CHANGE);		
	ndelay(10);	
	nf_ctrl(mtd, 0x30, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	ndelay(10);
	nf_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
	udelay(100); //T-R
	
	p = (u8*) &hynix_20nm_table;
	for(i=0; i<1026; i++)
	{		
		p[i] = nfreg_read8(mtd, NF_bPIODATA); 		
		ndelay(10);
	}	
		
	nf_ctrl(mtd, 0xFF, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	udelay(20); // T-RST
	nf_ctrl(mtd, 0x38, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	udelay(20); // T-RST
	nf_ctrl(mtd, 0x00, NAND_NCE | NAND_CTRL_CHANGE);
	
	//test code
	vfy_20nm_rr_tble();
	
	//set = hynix_20nm_table.pass_rr_set;	
	//copy = hynix_20nm_table.pass_rr_copy;
 	//rr_reg = get_hynix_20nm_rr(copy, set);
  //return load_hynix_retry_table(mtd, 0); 
  return 0;
}

//for MICRON
/* 
 * Set feature command for MICRON NAND Flash 
 * 
 */
static int nand_micron_set_feature(struct mtd_info *mtd,  uint8_t feature_addr, uint8_t *option)
{
	struct nand_chip *chip = NULL;

	//pr_info("%s FA 0x%x option 0x%x\n", __FUNCTION__, feature_addr, option[0]);
	if(mtd->priv)
	{
		chip = mtd->priv;
	}
	else
	{
		return -1;
	}
	/* Set Feature command for MICRON */
	//chip->cmdfunc(mtd, NAND_CMD_MICRON_SET_FEATURES, feature_addr, -1);
	nf_cmd(mtd, NAND_CMD_MICRON_SET_FEATURES, feature_addr, -1);
	nfreg_write8(mtd, option[0], NF_bPIODATA); 		
	nfreg_write8(mtd, option[1], NF_bPIODATA); 		
	nfreg_write8(mtd, option[2], NF_bPIODATA); 		
	nfreg_write8(mtd, option[3], NF_bPIODATA); 			
	return 0;
}

/* 
 * Set read retry option for MICRON NAND Flash
 * 
 */
static int nand_micron_set_feature_rr(struct mtd_info *mtd, uint8_t option)
{
	uint8_t p[4] = {0};

	
	p[0] = option;	
	//pr_info("%s 0x%x\n", __FUNCTION__, option);
	nand_micron_set_feature(mtd, FADDR_READ_RETRY, p);

	return 0;
}
/* 
 * Set feature command read retry for NAND Flash supported Read Retry
 * 
 */
static int nand_set_feature_rr(struct mtd_info *mtd, uint8_t option)
{
struct ali_nand_host *host = to_ali_nand_host(mtd);
int ret;
	
	pr_info("%s 0x%x\n", __FUNCTION__, option);
	if(host->nf_parm.rr_mode == RR_MODE_NONE)
	{
		return 0;
	}
	else if(host->nf_parm.rr_mode == RR_MODE_MICRON)
	{
		nand_micron_set_feature_rr(mtd, option);
	}	
	else if((host->nf_parm.rr_mode == HYNIX_20NM_A_DIE) 
			|| (host->nf_parm.rr_mode == HYNIX_20NM_B_DIE) 
			|| (host->nf_parm.rr_mode == HYNIX_20NM_C_DIE))
	{
		//inc_hynix_20nm_rr_set();
		//set = hynix_20nm_table.pass_rr_set;	
		//copy = hynix_20nm_table.pass_rr_copy;
 		//rr_reg = get_hynix_20nm_rr(copy, set);
  	ret = load_hynix_retry_table(mtd, 8); 	  	  	
		return ret;
	}
	return 0;
}


static void get_mlc_nand_id(struct mtd_info *mtd, struct nand_chip *chip)                                        
{
	struct ali_nand_host *host = to_ali_nand_host(mtd);
	u8 nand_id[10], i;

	/* Select the device */      
	chip->select_chip(mtd, 0);                                                                                       
	nf_cmd(mtd, NAND_CMD_RESET, -1, -1);
	//chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);                                                                      
	udelay(100);
	/* Send the command for reading device ID */                                                                     
	//chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);                                                                   
	nf_cmd(mtd, NAND_CMD_READID, 0x00, -1);
	pr_info("nand id => ");
	for (i=0; i<10; i++)
	{
		nand_id[i] = nfreg_read8(mtd, NF_bPIODATA);
		pr_info(" %02x", nand_id[i]);
		ndelay(20);
	}
 	pr_info("\n");
	
	/* for UBI patch, need know nand chip id */
	if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x44))
		micron_l83 = 1;
	else if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x64))
		micron_l84 = 1;
	else if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x84))
		micron_l84 = 1;
	else if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x88))
		micron_l74 = 1;
	else if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x68))
		micron_l73 = 1;
	else if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x48))
		micron_48 = 1;

	if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x44) &&                                                              
		(nand_id[2] == 0x44) && (nand_id[3] == 0x4B) && (nand_id[4] == 0xA9))//2C44444BA9                              
	{                                                                                                                
		//pr_info("RR patch for MT29F32G08CBADA\n");                                                                    
		g_retry_options_nb = 8;                                                                                        
		host->nf_parm.rr_mode = RR_MODE_MICRON;                                                                                      
	}                                                                                                                
	else if ((nand_id[0] == 0x2C) && (nand_id[1] == 0x64) &&                                                         
			(nand_id[2] == 0x44) && (nand_id[3] == 0x4B) && (nand_id[4] == 0xA9))//2C64444BA9                            
	{                                                                                                                
		//pr_info("RR patch for MICRON 29F64G08CBABA\n");                                                               
		g_retry_options_nb = 8;                                                                                        
		host->nf_parm.rr_mode = RR_MODE_MICRON;                                                                                      
	}                                                                                                                
	else if ((nand_id[0] == 0xAD) && (nand_id[1]== 0xDE) &&                                                          
			(nand_id[2] == 0x94) && (nand_id[3] == 0xDA) && (nand_id[4] == 0x74) && (nand_id[5] == 0xC4))                
	{                                                                                                                
		//pr_info("RR patch for HYNIX H27UCG8T2ATR \n"); // HYNIX H27UCG8T2ATR-BC(ADDE94DA74C4) F20 64Gb MLC A-die      
		g_retry_options_nb = 8;
		host->nf_parm.rr_mode = HYNIX_20NM_A_DIE;
		if(hynix_get_rr_table_20nm(mtd))
		{
			pr_err("[ERR] HYNIX HYNIX  H27UCG8T2ATR get_rr_table fail!\n");
			g_retry_options_nb = 0;
			host->nf_parm.rr_mode = RR_MODE_NONE;
		}
	}                                                                                                                
	else if ((nand_id[0] == 0xAD) && (nand_id[1] == 0xDE) &&                                                         
			(nand_id[2] == 0x94) && (nand_id[3] == 0xEB) && (nand_id[4] == 0x74) && (nand_id[5] == 0x44))                
	{                                                                                                                
		//pr_info("RR patch for HYNIX H27UBG8T2BTR \n"); // HYNIX H27UBG8T2BTR-BC(ADDE94EB7444) F20 64Gb MLC B-die      
		g_retry_options_nb = 8;
		host->nf_parm.rr_mode = HYNIX_20NM_B_DIE;
		if (hynix_get_rr_table_20nm(mtd))
		{
			pr_err("[ERR] HYNIX HYNIX  H27UCG8T2BTR get_rr_table fail!\n");
			g_retry_options_nb = 0;
			host->nf_parm.rr_mode = RR_MODE_NONE;
		}
	}                                                                                                                
	else if ((nand_id[0] == 0xAD) && (nand_id[1] == 0xD7) &&                                                         
			(nand_id[2] == 0x94) && (nand_id[3] == 0x91) && (nand_id[4] == 0x60) && (nand_id[5] == 0x44))                
	{                                                                                                                
		//pr_info("RR patch for HYNIX H27UBG8T2CTR-BC \n");	// HYNIX H27UBG8T2CTR-BC(ADD794916044) F20 32Gb MLC        
		g_retry_options_nb = 8;                                                                                        
		host->nf_parm.rr_mode = HYNIX_20NM_C_DIE;
		if (hynix_get_rr_table_20nm(mtd))
		{
			pr_err("[ERR] HYNIX HYNIX  H27UBG8T2CTR-BC get_rr_table fail!\n");
			g_retry_options_nb = 0;
			host->nf_parm.rr_mode = RR_MODE_NONE;
		}
	}
	// if ((nand_id[0] == 0x98) && (nand_id[1] == 0xDA) &&
	// 	(nand_id[2] == 0x90) && (nand_id[3] == 0x15) && (nand_id[4] == 0x76) ) 
	// 	force_24_ecc = 1;
	// if ((nand_id[0] == 0x01) && (nand_id[1] == 0xDA) &&
	// 	(nand_id[2] == 0x90) && (nand_id[3] == 0x95) && (nand_id[4] == 0x46) ) 
	// 	force_24_ecc = 1;
	// if ((nand_id[0] == 0x01) && (nand_id[1] == 0xDC) &&
	// 	(nand_id[2] == 0x90) && (nand_id[3] == 0x95) && (nand_id[4] == 0x56) ) 
	// 	force_24_ecc = 1;
	chip->select_chip(mtd, -1);                                                                                      
}                                                                                                                  