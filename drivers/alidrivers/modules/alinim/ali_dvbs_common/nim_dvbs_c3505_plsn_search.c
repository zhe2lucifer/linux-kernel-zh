#include "nim_dvbs_c3505_plsn_search.h"
#include "nim_dvbs_c3505_plsn_plsync.h"
#include "../dem_c3505/nim_dvbs_c3505.h"

//--------------------search plsN ----------------------------------
#define GOLD_N_NUM        262142 //2^18-1-1
#define MAX_SYMBOL_NUM    33192
#define PILOT_NUM         36
#define M_SQ_ORDER        18

INT32 *plsn_sq_rn_pilot_table = NULL;

INT32 pl_pilot_cnt;
INT32 pl_pilot_i[36];
INT32 pl_pilot_q[36];

INT32 pl_data_cnt;
INT32 pl_data_i[3072];
INT32 pl_data_q[3072];

INT32 plsn_pilot_cnt;
INT32 plsn_pilot_i[36];
INT32 plsn_pilot_q[36];

INT32 plsn_data_cnt;
INT32 plsn_data_i[3072];
INT32 plsn_data_q[3072];

INT32 plds_data_i[3072];
INT32 plds_data_q[3072];


static INT32 MapTable_QPSK [6] = {
	+29, +29, 0, 2, 3, 1  
};
static INT32 MapTable_8PSK [18] = {
	+41, 0, 1, 9999, 2, 9999, 
	+29, +29, 0, 6, 3, 5, 
	0, +41, 4, 9999, 7, 9999  
};
static INT32 MapTable_16APSK_2_3 [24] = {
	+45, +12, 4, 6, 7, 5, 
	+33, +33, 0, 2, 3, 1, 
	+10, +10, 12, 14, 15, 13, 
	+12, +45, 8, 10, 11, 9  
};
static INT32 MapTable_16APSK_3_4 [24] = {
	+45, +12, 4, 6, 7, 5, 
	+33, +33, 0, 2, 3, 1, 
	+12, +12, 12, 14, 15, 13, 
	+12, +45, 8, 10, 11, 9  
};
static INT32 MapTable_16APSK_4_5 [24] = {
	+45, +12, 4, 6, 7, 5, 
	+33, +33, 0, 2, 3, 1, 
	+12, +12, 12, 14, 15, 13, 
	+12, +45, 8, 10, 11, 9  
};
static INT32 MapTable_16APSK_5_6 [24] = {
	+45, +12, 4, 6, 7, 5, 
	+33, +33, 0, 2, 3, 1, 
	+12, +12, 12, 14, 15, 13, 
	+12, +45, 8, 10, 11, 9  
};
static INT32 MapTable_16APSK_8_9 [24] = {
	+45, +12, 4, 6, 7, 5, 
	+33, +33, 0, 2, 3, 1, 
	+13, +13, 12, 14, 15, 13, 
	+12, +45, 8, 10, 11, 9  
};
static INT32 MapTable_16APSK_9_10 [24] = {
	+45, +12, 4, 6, 7, 5, 
	+33, +33, 0, 2, 3, 1, 
	+13, +13, 12, 14, 15, 13, 
	+12, +45, 8, 10, 11, 9  
};
static INT32 MapTable_32APSK_3_4 [54] = {
	+48, +20, 8, 28, 14, 26, 
	+27, +7, 16, 20, 22, 18, 
	+52, 0, 24, 9999, 30, 9999, 
	+20, +20, 0, 4, 6, 2, 
	+7, +7, 17, 21, 23, 19, 
	+37, +37, 25, 12, 31, 10, 
	+7, +27, 1, 5, 7, 3, 
	+20, +48, 9, 29, 15, 27, 
	0, +52, 13, 9999, 11, 9999  
};
static INT32 MapTable_32APSK_4_5 [54] = {
	+48, +20, 8, 28, 14, 26, 
	+28, +8, 16, 20, 22, 18, 
	+52, 0, 24, 9999, 30, 9999, 
	+21, +21, 0, 4, 6, 2, 
	+8, +8, 17, 21, 23, 19, 
	+37, +37, 25, 12, 31, 10, 
	+8, +28, 1, 5, 7, 3, 
	+20, +48, 9, 29, 15, 27, 
	0, +52, 13, 9999, 11, 9999  
};
static INT32 MapTable_32APSK_5_6 [54] = {
	+48, +20, 8, 28, 14, 26, 
	+28, +8, 16, 20, 22, 18, 
	+52, 0, 24, 9999, 30, 9999, 
	+21, +21, 0, 4, 6, 2, 
	+8, +8, 17, 21, 23, 19, 
	+37, +37, 25, 12, 31, 10, 
	+8, +28, 1, 5, 7, 3, 
	+20, +48, 9, 29, 15, 27, 
	0, +52, 13, 9999, 11, 9999  
};
static INT32 MapTable_32APSK_8_9 [54] = {
	+48, +20, 8, 28, 14, 26, 
	+29, +8, 16, 20, 22, 18, 
	+51, 0, 24, 9999, 30, 9999, 
	+21, +21, 0, 4, 6, 2, 
	+8, +8, 17, 21, 23, 19, 
	+36, +36, 25, 12, 31, 10, 
	+8, +29, 1, 5, 7, 3, 
	+20, +48, 9, 29, 15, 27, 
	0, +51, 13, 9999, 11, 9999  
};
static INT32 MapTable_32APSK_9_10 [54] = {
	+47, +20, 8, 28, 14, 26, 
	+29, +8, 16, 20, 22, 18, 
	+51, 0, 24, 9999, 30, 9999, 
	+21, +21, 0, 4, 6, 2, 
	+8, +8, 17, 21, 23, 19, 
	+36, +36, 25, 12, 31, 10, 
	+8, +29, 1, 5, 7, 3, 
	+20, +47, 9, 29, 15, 27, 
	0, +51, 13, 9999, 11, 9999  
};
#define MAX_CMP_LENGTH   1536  //3072; //fec16200_1/4_Kbch 3072/8=384Bytes
static UINT32 gold0_pls_sq[MAX_CMP_LENGTH];
static UINT32 gold0_pls_sq_diff[MAX_CMP_LENGTH];
static UINT32 gold0_demap_hd[MAX_CMP_LENGTH];
static UINT8 hd_table[4149*2]; //MAX_SYMBOL_NUM/8=4149
static UINT8 bbs_sq[4149*2];
INT32 *demap_table = NULL;
static UINT32 sq_x_reg_init = 1;      //[17]-[0]:0x00001
//static UINT32 sq_y_reg_init = 262143; //[17]-[0]:0x3ffff
static UINT32 sq_y_0_buf[96] = {
	2617507839UL, 1583049971UL, 609380874UL, 4181449157UL, 2325726456UL, 42343922UL, 599090270UL, 3054329501UL, 
	1062398450UL, 2209890148UL, 2766437129UL, 3958619278UL, 4239003908UL, 3192417828UL,1895100324UL,2351653946UL, 
	2367711429UL, 747686081UL,  2355939225UL, 2840550865UL, 3981253367UL, 3703907610UL,	2451613878UL,3605205957UL, 
	4214152753UL, 381905148UL,  2170365045UL, 3609454036UL, 3910323461UL, 913979080UL,	3948227746UL,160080015UL,  
	4266674526UL, 3850747527UL, 106658415UL,  2461883282UL, 2774430117UL, 3846607197UL, 3354966909UL,3170921701UL, 
	2368287476UL, 824265957UL,  3263509017UL, 2440779410UL, 3356249048UL, 3396224952UL, 1506972525UL,679432679UL,  
	294114457UL,  1510811314UL, 1687389951UL, 1043292572UL, 1115034039UL, 394206407UL, 1557592255UL,3350247803UL, 
	2372566734UL, 1993577186UL, 195797588UL,  1197173389UL, 4068562171UL, 4077918259UL,	4042847305UL,725313931UL,  
	4259877112UL, 1074539021UL, 3403928433UL, 3437939278UL, 661780893UL,  1274979637UL,	2198216081UL,3312354275UL, 
	359996153UL,  2244293572UL, 2678250503UL, 3072932893UL, 1206720591UL, 1001294897UL, 4251035945UL,3689722172UL, 
	1654227403UL, 176799299UL,  3973737001UL, 1633030520UL, 3450174233UL, 2106818689UL, 3908853613UL,1214107713UL, 
	1830300790UL, 1969982662UL, 434982365UL,  2070612574UL, 2204280929UL, 1157191550UL, 3301894169UL,1384926528UL}; //3072/32=96

static UINT32 sq_y_1_buf[96] = {
	3101358760UL, 1005342478UL, 3574432040UL, 4090737386UL, 3956369241UL, 1535717258UL, 3770181074UL, 3002036755UL,
	3700790725UL, 920712188UL,	2475111134UL, 767181667UL,  948411422UL,  988268274UL,  2922869550UL, 203476164UL,
	4225059690UL, 957345034UL,  3258684911UL, 712768634UL,  1770364121UL, 2250691331UL, 1589421288UL, 4238953870UL,
	204516137UL,  491810857UL,  677972508UL,  1232680898UL, 3475869641UL, 2456964837UL, 1968122246UL, 1844478538UL,
	240842622UL,  642352348UL,  2838749179UL, 586863881UL,	3975096193UL, 2622493610UL, 1222898356UL, 210949276UL,
	2347575985UL, 1619160861UL, 1518450813UL, 1153399768UL, 2403281675UL, 1916800237UL, 2206018728UL, 3678167555UL,
	1074316692UL, 2843621474UL, 3290007605UL, 2717394689UL, 1243369329UL, 2178304808UL, 1269768172UL, 2463294175UL,
	1041514517UL, 2846813021UL, 3867724906UL, 34578720UL,   1658826093UL, 1098415441UL,	3424004996UL, 1855400310UL,
	2638676116UL, 1931598275UL, 3045367817UL, 3737831099UL, 2646224090UL, 941413935UL,  766193288UL,  1796604761UL,
	2510275337UL, 1329249781UL, 2759832635UL, 641009398UL,  1719930885UL, 315676562UL,  3802111133UL, 38859483UL,
	3210737689UL, 2445055697UL, 1781686745UL, 3978310324UL, 4146859680UL, 4261045787UL, 371854148UL,  4172775626UL,
	2786134005UL, 2125535504UL, 4012738457UL, 4292537285UL, 3233799825UL, 245561645UL,  204652324UL,  2004432929UL}; 

static INT32 FEC_K_BCH[21] = {
	16008, //0
	21408, //1/
	25728, //2/
	32208, //3/
	38688, //4/
	43040, //5
	48408, //6
	51648, //7
	53840, //8
	57472, //9
	58192, //10
	3072 , //0
	5232 , //1/
	6312 , //2/
	7032 , //3/
	9552 , //4/
	10632, //5
	11712, //6
	12432, //7
	13152, //8
	14232  //9
};

UINT8 g_table_finish_flag = 1;//used for indicate whether generate table finish, 0:not finish, 1:finished
UINT8 g_release_flag;
UINT8 *g_cap_addr = NULL;

struct SQ_XY
{
	UINT8 *p_sq_x_reg;
	UINT8 *p_sq_y_0_pilot_reg;
	UINT8 *p_sq_y_1_pilot_reg;
	UINT8 *p_sq_x_0_pilot_reg;
	UINT8 *p_sq_x_1_pilot_reg;
	UINT8 *p_sq_x_pilot_reg;
	UINT8 *p_sq_rn_pilot;
};

INT32 pls_rn_pilot_generate(INT32 gold_n, struct SQ_XY sq_addr)
{
	INT32 index_i = 0;
	INT32 index_j = 0;
	//INT32 index_k = 0;
	UINT8 sq_z = 0;
	UINT8 sq_zn = 0;
	UINT8 sq_rn_tmp;
	UINT8 fb_x = 0;

	if(gold_n == 0)
		return 0;

	//step1
	fb_x = (sq_addr.p_sq_x_pilot_reg[0] + sq_addr.p_sq_x_pilot_reg[7]) % 2;
	for(index_i = 1; index_i < M_SQ_ORDER; index_i++)
	{
		sq_addr.p_sq_x_reg[index_i-1] = sq_addr.p_sq_x_pilot_reg[index_i];
		sq_addr.p_sq_x_pilot_reg[index_i-1] = sq_addr.p_sq_x_pilot_reg[index_i];
	}
	sq_addr.p_sq_x_pilot_reg[M_SQ_ORDER-1] = fb_x;
	sq_addr.p_sq_x_reg[M_SQ_ORDER-1] = fb_x;

	//step2
	for(index_i = 0; index_i < 36; index_i++)
	{
		sq_addr.p_sq_x_0_pilot_reg[index_i] = sq_addr.p_sq_x_reg[0] % 2;
		sq_addr.p_sq_x_1_pilot_reg[index_i] = (sq_addr.p_sq_x_reg[4] + sq_addr.p_sq_x_reg[6] + sq_addr.p_sq_x_reg[15]) % 2;

		sq_z = (sq_addr.p_sq_x_0_pilot_reg[index_i] + sq_addr.p_sq_y_0_pilot_reg[index_i]) % 2;
		sq_zn = (sq_addr.p_sq_x_1_pilot_reg[index_i] + sq_addr.p_sq_y_1_pilot_reg[index_i]) % 2;
		sq_rn_tmp = 2*sq_zn + sq_z;
		sq_addr.p_sq_rn_pilot[index_i] = (UINT8)sq_rn_tmp;

		//feedback
		fb_x = (sq_addr.p_sq_x_reg[0] + sq_addr.p_sq_x_reg[7]) % 2;
		for(index_j = 1; index_j < M_SQ_ORDER; index_j++)
			sq_addr.p_sq_x_reg[index_j-1] = sq_addr.p_sq_x_reg[index_j];
		sq_addr.p_sq_x_reg[M_SQ_ORDER-1] = fb_x;
	}

	return 0;
}

/*********************************************************
* generate the table for plsn query 
* it will called by the upper level
*********************************************************/
INT32 nim_c3505_generate_table(struct nim_device *dev)
{
	INT32 gold_i = 0;
	INT32 index_i = 0;
	INT32 index_j = 0;
	INT32 diff_phase = 0;
	UINT8 sq_x_reg[M_SQ_ORDER];
	UINT8 sq_y_0_pilot_reg[PILOT_NUM] = {0 ,0 ,0 ,1 ,1 ,1 ,0 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,1 ,0 ,0 ,1 ,1 ,1 ,0 ,1 ,1 ,0 ,0 ,1 ,0 ,1 ,0 ,0 ,1 ,1 ,1 ,0 ,1 ,1};
	UINT8 sq_y_1_pilot_reg[PILOT_NUM] = {1 ,0 ,1 ,1 ,0 ,1 ,1 ,1 ,0 ,0 ,1 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,1 ,0 ,0 ,1 ,0 ,0 ,1 ,1 ,1 ,0 ,0 ,0 ,0 ,1};
	UINT8 sq_x_0_pilot_reg[PILOT_NUM] = {1 ,0 ,0 ,1 ,0 ,0 ,0 ,0 ,1 ,1 ,0 ,1 ,1 ,0 ,1 ,0 ,1 ,1 ,1 ,1 ,1 ,1 ,1 ,1 ,0 ,1 ,1 ,0 ,1 ,0 ,0 ,1 ,0 ,1 ,0 ,1};
	UINT8 sq_x_1_pilot_reg[PILOT_NUM] = {0 ,1 ,0 ,0 ,0 ,1 ,0 ,0 ,1 ,0 ,1 ,0 ,0 ,1 ,0 ,0 ,1 ,0 ,0 ,0 ,0 ,1 ,0 ,1 ,1 ,0 ,1 ,1 ,1 ,0 ,0 ,1 ,1 ,0 ,0 ,1};
	UINT8 sq_x_pilot_reg[M_SQ_ORDER] = {1 ,0 ,0 ,1 ,0 ,0 ,0 ,0 ,1 ,1 ,0 ,1 ,1 ,0 ,1 ,0 ,1 ,1 };
	UINT8 sq_rn_pilot[PILOT_NUM] = {3 ,2 ,2 ,2 ,1 ,1 ,2 ,3 ,2 ,0 ,1 ,3 ,1 ,2 ,0 ,0 ,3 ,0 ,0 ,0 ,1 ,2 ,2 ,3 ,2 ,2 ,3 ,3 ,1 ,2 ,3 ,2 ,3 ,1 ,1 ,0};

	struct SQ_XY sq_xy;

	sq_xy.p_sq_x_reg = sq_x_reg;
	sq_xy.p_sq_y_0_pilot_reg = sq_y_0_pilot_reg;
	sq_xy.p_sq_y_1_pilot_reg = sq_y_1_pilot_reg;
	sq_xy.p_sq_x_0_pilot_reg = sq_x_0_pilot_reg;
	sq_xy.p_sq_x_1_pilot_reg = sq_x_1_pilot_reg;
	sq_xy.p_sq_x_pilot_reg = sq_x_pilot_reg;
	sq_xy.p_sq_rn_pilot = sq_rn_pilot;

	if ((NULL==plsn_sq_rn_pilot_table) || (NULL==g_cap_addr))
	{
		SEARCH_PLSN_PRINTF("[%s %d]the table address is NULL\n", __FUNCTION__, __LINE__);
		g_table_finish_flag = 1;
		return ERR_NO_MEM;
	}

	SEARCH_PLSN_PRINTF("[%s %d]plsn_sq_rn_pilot_table=0x%x\n", __FUNCTION__, __LINE__, plsn_sq_rn_pilot_table);
	memset(plsn_sq_rn_pilot_table, 0, 262142*3*4);
	memset(g_cap_addr, 0, 0x400000);

	for(gold_i = 0; gold_i < GOLD_N_NUM; gold_i++)
	{
		if (g_release_flag)
		{
			g_table_finish_flag = 1;
			break;
		}
		//generate sq_rn
		pls_rn_pilot_generate(gold_i, sq_xy);
		index_j = 0;
		for(index_i = 1; index_i < 36; index_i++)
		{
			//calc diff_phase
			diff_phase = sq_rn_pilot[index_i] - sq_rn_pilot[index_i-1];
			if(diff_phase < 0)
				diff_phase = 4 + diff_phase;
			//merge
			if((((index_i-1) % 12) == 0) && ((index_i-1) > 0))
				index_j ++ ;
			*(plsn_sq_rn_pilot_table + gold_i*3 + index_j) = (diff_phase & 0x03) + (*(plsn_sq_rn_pilot_table + gold_i*3 + index_j) << 2);
		}
	}

	g_table_finish_flag = 1;
	return 0;
}

INT32 nim_c3505_start_generate(struct nim_device *dev, struct ali_plsn_address *p_addr)
{
	struct nim_c3505_private *priv = NULL;
	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv->plsn.search_plsn_stop = 1;
	SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.search_plsn_stop=%d\n", __FUNCTION__, __LINE__, priv->plsn.search_plsn_stop);
	g_table_finish_flag = 0;
	queue_delayed_work(dev->plsn_gen_table_workqueue, &dev->delay_plsn_gen_table_work,0);
	g_release_flag = 0;
	plsn_sq_rn_pilot_table = (INT32 *)(p_addr->table_address);
	g_cap_addr = (UINT8 *)(p_addr->capture_address);

	return SUCCESS;
}

INT32 nim_c3505_release_table(struct nim_device *dev)
{	
	struct nim_c3505_private *priv = NULL;
	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	g_release_flag = 1;
	SEARCH_PLSN_PRINTF("[%s %d]g_release_flag=%d\n", __FUNCTION__, __LINE__, g_release_flag);
	priv->plsn.search_plsn_force_stop = 1;
	SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.search_plsn_force_stop=%d\n", __FUNCTION__, __LINE__, priv->plsn.search_plsn_force_stop);
	SEARCH_PLSN_PRINTF("[%s %d]force_stop_plsn_search!\n", __FUNCTION__, __LINE__);
	while (1)
	{
		SEARCH_PLSN_PRINTF("[%s %d]g_table_finish_flag=%d, priv->plsn.search_plsn_stop=%d\n", __FUNCTION__, __LINE__, g_table_finish_flag, priv->plsn.search_plsn_stop);
		if (g_table_finish_flag && priv->plsn.search_plsn_stop)//wait generate table and search plsn finish.
		{
			plsn_sq_rn_pilot_table = NULL;
			g_cap_addr = NULL;			
			break;
		}
		comm_sleep(1);
	}
	return SUCCESS;
}

/*****************************************************************************
*  INT32 nim_c3505_search_plsn_exit(struct nim_device *dev)
* 
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_c3505_search_plsn_exit(struct nim_device *dev)
{
	struct nim_c3505_private *priv = NULL;

	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}
	
	SEARCH_PLSN_PRINTF("[%s %d]priv->plsn_mutex=%d\n", __FUNCTION__, __LINE__, priv->plsn_mutex);
	mutex_lock(&priv->plsn_mutex);
	priv->plsn.search_plsn_stop = 1;
	SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.search_plsn_stop=%d\n", __FUNCTION__, __LINE__, priv->plsn.search_plsn_stop);
	priv->plsn.plsn_find = 0;
	mutex_unlock(&priv->plsn_mutex);
	priv->plsn.plsn_num = 0;
	memset(priv->plsn.plsn_val,0,sizeof(priv->plsn.plsn_val));

	return SUCCESS;
}

//begin:add by robin, 20170228
plsn_state nim_c3505_try_plsn(struct nim_device *dev, UINT8 *index)
{
	struct nim_c3505_private *priv = NULL;

	if (NULL == dev)
	{
		SEARCH_PLSN_PRINTF("Exit with ERR_NO_DEV in %s \n",__FUNCTION__);
		return PLSN_UNSUPPORT;
	}
	priv = (struct nim_c3505_private *)dev->priv;
	if(NULL == priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == priv\n", __FUNCTION__, __LINE__);
		return PLSN_UNSUPPORT;
	}

	mutex_lock(&priv->plsn_mutex);
	if (priv->plsn.super_scan)//support super scan
	{
		if (priv->plsn.plsn_find)//find the right value
		{
			if (*index == priv->plsn.plsn_num)//try over
			{
				mutex_unlock(&priv->plsn_mutex);
				SEARCH_PLSN_PRINTF("[%s %d]return PLSN_OVER\n", __FUNCTION__, __LINE__);
				return PLSN_OVER;
			}

			for ( ; *index < priv->plsn.plsn_num;)
			{
				priv->plsn.plsn_try_val = priv->plsn.plsn_val[*index];
				priv->plsn.plsn_try = 0;
				*index = *index + 1;
				SEARCH_PLSN_PRINTF("[%s %d]*index=%d, priv->plsn.plsn_num=%d\n", __FUNCTION__, __LINE__, *index, priv->plsn.plsn_num);
				if (priv->plsn.plsn_now != priv->plsn.plsn_try_val)
				{
					SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.plsn_try_val=%d\n", __FUNCTION__, __LINE__, priv->plsn.plsn_try_val);
					//reset demod
					nim_c3505_set_demod_ctrl(dev, NIM_DEMOD_CTRL_0X91);										

					//set plsN
					priv->plsn.plsn_try = 1;

					nim_c3505_set_plsn(dev);					

					//start demod
					nim_c3505_set_demod_ctrl(dev, NIM_DEMOD_CTRL_0X51);

					break;
				}								
			}			
		}
		else//the plsn hasn't found
		{
			if (priv->plsn.search_plsn_stop)//exit search
			{
				SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.plsn_find=%d\n", __FUNCTION__, __LINE__, priv->plsn.plsn_find);
				mutex_unlock(&priv->plsn_mutex);
				SEARCH_PLSN_PRINTF("[%s %d]return PLSN_STOPED\n", __FUNCTION__, __LINE__);
				return PLSN_STOPED;
			}
			else//the task is still running
			{	
				mutex_unlock(&priv->plsn_mutex);
				return PLSN_RUNNING;
			}
		}
	}
	else
	{
		mutex_unlock(&priv->plsn_mutex);
		SEARCH_PLSN_PRINTF("[%s %d]return PLSN_UNSUPPORT\n", __FUNCTION__, __LINE__);
		return PLSN_UNSUPPORT;	//not support super scan
	}

	mutex_unlock(&priv->plsn_mutex);
	SEARCH_PLSN_PRINTF("[%s %d]return PLSN_FIND\n", __FUNCTION__, __LINE__);
	return PLSN_FIND;
}
//end

void nim_c3505_get_bbs_sq(INT32 plsn_search_type, INT32 bbs_sq_start, INT32 bbs_sq_end, UINT32 *para_data)
{
	INT32	i;
	//INT32	scram_byte_len;
	INT32	scram_shift_reg;
	INT32	xor_flag;
	INT32	xor_data;
	INT32   bit15, bit14;
	INT32   byte_k = 0;
	INT32   bit_m = 0;
	INT32   bbh_byte_k = 0;
	INT32   bbh_bit_m = 0;

	//initialize
	scram_shift_reg = 0x4a80;

	byte_k = 0;
	bit_m = 0;
	xor_data = 0;

	if(plsn_search_type == 4)
	{
		bbh_byte_k = 0;
		bbh_bit_m = 0;
	}

	for(i = 0; i < bbs_sq_end; i++)
	{
		bit15 = scram_shift_reg & 0x1;
		bit14 = (scram_shift_reg >> 1) & 0x1;
		xor_flag = bit15 ^ bit14;
		scram_shift_reg >>= 1;
		if(xor_flag)
			scram_shift_reg |= 0x4000;

		if(plsn_search_type == 4)
		{
			if((i > 15) && (i < 56)) //UPL+DFL+SYNC=2+2+1=5 bytes
			{
				xor_flag = xor_flag ^ ((para_data[bbh_byte_k] >> (7-bbh_bit_m)) & 0x01);
				bbh_bit_m ++;
				if(bbh_bit_m == 8)
				{
					bbh_bit_m = 0;
					bbh_byte_k ++;
				}
			}
		}

		if(i >= bbs_sq_start)
		{
			xor_data <<= 1;
			if(xor_flag)
				xor_data |=1;
			bit_m ++;
			if(bit_m == 8)
			{
				bbs_sq[byte_k] = xor_data;
				byte_k ++;
				bit_m = 0;
				xor_data = 0;
			}
		}
	}
}

void nim_c3505_pl_scrambling(INT32 *plds_i, INT32 *plds_q, INT32 plds_len, INT32 start_addr, INT32 goldn_now, INT32 plsn_search_type)
{
	int    i = 0;
	int    k = 0;
	INT32  tmp_val = 0;
	UINT32 sq_z = 0;
	UINT32 sq_zn = 0;
	UINT32 fb_x = 0;
	UINT32 sq_rn = 0;
	UINT32 sq_x_0 = 0;
	UINT32 sq_x_1 = 0;
	UINT32 sq_y_0 = 0;
	UINT32 sq_y_1 = 0;
	UINT32 sq_x_reg = 0;
	//UINT32 sq_y_reg = 0;

	UINT32 num_j = 0;
	UINT32 bit_k = 0;

	INT32 goldn_i = 0;

	sq_x_reg = 1; //default gold_n = 0
	if(goldn_now > 0)
	{
		for(goldn_i = 1; goldn_i <= goldn_now; goldn_i++)
		{
			fb_x = (sq_x_reg & 0x01) ^ ((sq_x_reg >> 7) & 0x01);
			sq_x_reg = ((sq_x_reg & 0x3ffff) >> 1) + ((fb_x & 0x01) << 17);
		}
	}

	//generator
	num_j = 0;
	bit_k = 0;
	for(i = 0; i < plds_len; i++)
	{
		//get sq_rn
		sq_x_0 = (sq_x_reg & 0x01);
		sq_y_0 = ((sq_y_0_buf[num_j] >> bit_k) & 0x01);
		sq_z = sq_x_0 ^ sq_y_0;

		sq_x_1 = ((sq_x_reg >> 4) & 0x01) ^ ((sq_x_reg >> 6) & 0x01) ^ ((sq_x_reg >> 15) & 0x01);
		sq_y_1 = ((sq_y_1_buf[num_j] >> bit_k) & 0x01);
		sq_zn = sq_x_1 ^ sq_y_1;

		sq_rn = sq_zn * 2 + sq_z;

		bit_k ++;
		if(bit_k == 32)
		{
			bit_k = 0;
			num_j ++;
		}

		//update sq_x_reg
		fb_x = (sq_x_reg & 0x01) ^ ((sq_x_reg >> 7) & 0x01);
		sq_x_reg = ((sq_x_reg & 0x3ffff) >> 1) + ((fb_x & 0x01) << 17);

		//pl_descrambling
		if(i < start_addr)
		{
			continue;
		}
		else
		{
			if(plsn_search_type == 0)
				k = i - start_addr;
			else
				k = i;

			if(sq_rn == 1)
			{
				tmp_val = plds_i[k];
				plds_i[k] = -plds_q[k];
				plds_q[k] = tmp_val;
			}
			else if(sq_rn == 2)
			{
				plds_i[k] = -plds_i[k];
				plds_q[k] = -plds_q[k];
			}
			else if(sq_rn == 3)
			{
				tmp_val = -plds_i[k];
				plds_i[k] = plds_q[k];
				plds_q[k] = tmp_val;
			}
			else
			{
				plds_i[k] = plds_i[k];
				plds_q[k] = plds_q[k];
			}
		}
	}
}

void nim_c3505_pl_descrambling(INT32 *plds_out_i, INT32 *plds_out_q, INT32 *plds_in_i, INT32 *plds_in_q, INT32 plds_len, INT32 gold_n, INT32 start_addr)
{
	int    i = 0;
	UINT32 sq_z = 0;
	UINT32 sq_zn = 0;
	UINT32 fb_x = 0;
	UINT32 sq_rn = 0;
	UINT32 sq_x_0 = 0;
	UINT32 sq_x_1 = 0;
	UINT32 sq_y_0 = 0;
	UINT32 sq_y_1 = 0;
	UINT32 sq_x_reg = 0;
	//UINT32 sq_y_reg = 0;

	UINT32 num_j = 0;
	UINT32 bit_k = 0;

	if(gold_n == 0)
	{
		sq_x_reg_init = 1;      //[17]-[0]:0x00001
		memset(gold0_pls_sq, 0, sizeof(gold0_pls_sq));
	}
	else
	{
		fb_x = (sq_x_reg_init & 0x01) ^ ((sq_x_reg_init >> 7) & 0x01);
		sq_x_reg_init = ((sq_x_reg_init & 0x3ffff) >> 1) + ((fb_x & 0x01) << 17);
	}
	sq_x_reg = sq_x_reg_init;

	//generator
	num_j = 0;
	bit_k = 0;
	for(i = 0; i < plds_len; i++)
	{
		//get sq_rn
		sq_x_0 = (sq_x_reg & 0x01);
		sq_y_0 = ((sq_y_0_buf[num_j] >> bit_k) & 0x01);
		sq_z = sq_x_0 ^ sq_y_0;

		sq_x_1 = ((sq_x_reg >> 4) & 0x01) ^ ((sq_x_reg >> 6) & 0x01) ^ ((sq_x_reg >> 15) & 0x01);
		sq_y_1 = ((sq_y_1_buf[num_j] >> bit_k) & 0x01);
		sq_zn = sq_x_1 ^ sq_y_1;

		sq_rn = sq_zn * 2 + sq_z;

		bit_k ++;
		if(bit_k == 32)
		{
			bit_k = 0;
			num_j ++;
		}

		//update sq_x_reg
		fb_x = (sq_x_reg & 0x01) ^ ((sq_x_reg >> 7) & 0x01);
		sq_x_reg = ((sq_x_reg & 0x3ffff) >> 1) + ((fb_x & 0x01) << 17);

		//pl_descrambling
		if(i < start_addr)
			continue;
		else
		{
			if(sq_rn == 1)
			{
				plds_out_i[i] = plds_in_q[i];
				plds_out_q[i] = -plds_in_i[i];
			}
			else if(sq_rn == 2)
			{
				plds_out_i[i] = -plds_in_i[i];
				plds_out_q[i] = -plds_in_q[i];
			}
			else if(sq_rn == 3)
			{
				plds_out_i[i] = -plds_in_q[i];
				plds_out_q[i] = plds_in_i[i];
			}
			else
			{
				plds_out_i[i] = plds_in_i[i];
				plds_out_q[i] = plds_in_q[i];
			}
		}

		//store
		if(gold_n == 0)
		{
			gold0_pls_sq[i] = sq_rn;
		}
		else
		{
			if(sq_rn < gold0_pls_sq[i])
			{
				gold0_pls_sq_diff[i] = 4 - (gold0_pls_sq[i] - sq_rn);
			}
			else
			{
				gold0_pls_sq_diff[i] = sq_rn - gold0_pls_sq[i];
			}
		}
	}
}

INT32 nim_c3505_get_map_table(INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *p0_num, INT32 *p1_num, INT32 *p2_num)
{
	INT32 point_num = 0;

	switch(modu_type)
	{
	case 3: //8PSK
		{	
			demap_table = MapTable_8PSK;
			point_num = 18;
			*p0_num = 1; *p1_num = 1; *p2_num = 1;
			break;
		}
	case 4: //16APSK
		{
			if(frame_mode == 1)
			{
				switch(coderate)
				{
				case 5	: demap_table = MapTable_16APSK_2_3 ; break;
				case 6	: demap_table = MapTable_16APSK_3_4 ; break;
				case 7	: demap_table = MapTable_16APSK_4_5 ; break;
				case 8	: demap_table = MapTable_16APSK_5_6 ; break;
				default	: demap_table = MapTable_16APSK_8_9 ; break;
				}
			}
			else
			{
				switch(coderate)
				{
				case 5	: demap_table = MapTable_16APSK_2_3 ; break;
				case 6	: demap_table = MapTable_16APSK_3_4 ; break;
				case 7	: demap_table = MapTable_16APSK_4_5 ; break;
				case 8	: demap_table = MapTable_16APSK_5_6 ; break;
				case 9	: demap_table = MapTable_16APSK_8_9 ; break;
				default	: demap_table = MapTable_16APSK_9_10 ; break;
				}
			}
			point_num = 24;
			*p0_num = 1; *p1_num = 2; *p2_num = 1;
			break;
		}
	case 5: //32APSK
		{
			if(frame_mode == 1)
			{
				switch(coderate)
				{
				case 6	: demap_table = MapTable_32APSK_3_4 ; break;
				case 7	: demap_table = MapTable_32APSK_4_5 ; break;
				case 8	: demap_table = MapTable_32APSK_5_6 ; break;
				default	: demap_table = MapTable_32APSK_8_9 ; break;
				}
			}
			else
			{
				switch(coderate)
				{
				case 6	: demap_table = MapTable_32APSK_3_4 ; break;
				case 7	: demap_table = MapTable_32APSK_4_5 ; break;
				case 8	: demap_table = MapTable_32APSK_5_6 ; break;
				case 9	: demap_table = MapTable_32APSK_8_9 ; break;
				default	: demap_table = MapTable_32APSK_9_10 ; break;
				}
			}
			point_num = 54;
			*p0_num = 3; *p1_num = 3; *p2_num = 3;
			break;
		}
	default : 
		{	
			demap_table = MapTable_QPSK; 
			point_num = 6;
			*p0_num = 0; *p1_num = 1; *p2_num = 0;
			break;	
		}
	}

	point_num = point_num;
	return point_num;
}

void nim_c3505_demap_point_hard_decison(INT32 plsn_search_type, INT32 gold_n, INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_data_i, INT32 *plsn_data_q, INT32 plsn_data_cnt, INT32 point_num, INT32 start_addr)
{
	INT32 index_i = 0;
	INT32 index_j = 0;
	INT32 index_k = 0;
	INT32 index_m = 0;

	INT32 error_i = 0;
	INT32 error_q = 0;
	INT32 currpower = 0;
	INT32 min_power = 0;
	INT32 min_indx = 0;
	//INT32 min_indx_tmp = 0;

	UINT8 hd_bit = 0;
	UINT8 hd_byte = 0;

	INT32 data_i = 0;
	INT32 data_q = 0;
	INT32 data_i_abs = 0;
	INT32 data_q_abs = 0;
	INT32 demap_table_i = 0;
	INT32 demap_table_q = 0;


	//entity
	min_power = 0;
	min_indx  = 0;
	index_k = 0;
	index_m = 0;
	hd_byte = 0;
	for (index_i = start_addr; index_i < plsn_data_cnt; index_i++)
	{
		data_i = plsn_data_i[index_i];
		data_q = plsn_data_q[index_i];
		if(gold_n == 0)
		{
			if(data_i < 0) data_i_abs = -data_i;
			else           data_i_abs = data_i;
			if(data_q < 0) data_q_abs = -data_q;
			else           data_q_abs = data_q;

			for (index_j = 0; index_j < point_num; index_j += 6)
			{
				error_i = data_i_abs - demap_table[index_j];
				error_q = data_q_abs - demap_table[index_j + 1];
				currpower = error_i * error_i + error_q * error_q;
				if((index_j == 0) || (min_power > currpower))
				{
					min_power = currpower;
					min_indx  = index_j;
				}
			}
			gold0_demap_hd[index_i] = min_indx;
		}
		else
		{
			min_indx = gold0_demap_hd[index_i];
			if((gold0_pls_sq_diff[index_i] & 0x01) == 0x01)
			{
				data_i_abs = demap_table[min_indx + 1];
				data_q_abs = demap_table[min_indx];
			}
			else
			{
				data_i_abs = demap_table[min_indx];
				data_q_abs = demap_table[min_indx + 1];
			}

			min_indx = 0;
			for (index_j = 0; index_j < point_num; index_j += 6)
			{
				demap_table_i = demap_table[index_j];
				demap_table_q = demap_table[index_j + 1];
				if((data_i_abs == demap_table_i) && (data_q_abs == demap_table_q))
				{
					min_indx  = index_j;
					break;
				}
			}
		}

		if(demap_table[min_indx] == 0) //I==0
			if(data_q >= 0)
				min_indx = demap_table[min_indx + 2];
			else
				min_indx = demap_table[min_indx + 4];
		else if(demap_table[min_indx + 1] == 0) //Q==0
			if(data_i >= 0)
				min_indx = demap_table[min_indx + 2];
			else
				min_indx = demap_table[min_indx + 4];
		else if((data_i > 0) && (data_q > 0))        //1 quadrant
			min_indx = demap_table[min_indx + 2];
		else if((data_i < 0) && (data_q > 0))        //2 quadrant
			min_indx = demap_table[min_indx + 3];
		else if((data_i > 0) && (data_q < 0))        //4 quadrant
			min_indx = demap_table[min_indx + 5];
		else                                         //3 quadrant
			min_indx = demap_table[min_indx + 4];

		if(modu_type <= 2) //QPSK
		{
			hd_bit = min_indx & 0x03;
			hd_byte = (hd_byte << 2) + hd_bit;
			index_k = index_k + 2;
		}
		else
		{
			if(plsn_search_type == 3) //3-using null-packet&point_area_hd;
			{
				if(modu_type == 5) //32APSK
				{
					hd_bit = (min_indx >> 2) & 0x01;
				}				
				else //8PSK 16APSK
				{
					hd_bit = (min_indx >> 1) & 0x01;
				}
			}
			else //1-null-packet&point_hd;  4-using BBHeader;
			{
				if((modu_type == 3) && (coderate == 4)) //8psk & 3/5
					hd_bit = min_indx & 0x01;
				else
					hd_bit = (min_indx >> (modu_type - 1)) & 0x01;
			}
			hd_byte = (hd_byte << 1) + hd_bit;
			index_k ++;
		}

		if(index_k == 8)
		{
			hd_table[index_m] = hd_byte;
			index_k = 0;
			index_m ++;
			hd_byte = 0;
		}
	}
}

void nim_c3505_demap_area_hard_decison(INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_in_i, INT32 *plsn_in_q, INT32 plf_len, INT32 cmp_start_addr)
{
	INT32 index_i = 0;
	//INT32 index_j = 0;
	INT32 index_k = 0;
	INT32 index_m = 0;

	INT32 data_i = 0;
	INT32 data_q = 0;

	//UINT8 calc_type = 0;
	UINT8 hd_byte = 0;

	INT32 slope_k = 0;

	memset(hd_table, 0, sizeof(hd_table));

	if(modu_type == 3) //8PSK
	{
		index_k = 0;
		index_m = 0;
		hd_byte = 0;
		slope_k = 309; //tan(67.5)*(2^7)=309
		for (index_i = cmp_start_addr; index_i < plf_len; index_i++)
		{
			data_i = plsn_in_i[index_i]; //bit[1]
			data_q = plsn_in_q[index_i];

			if((data_q*128) < ((-1)*slope_k*data_i))
				hd_byte = (hd_byte << 1) + 1;
			else
				hd_byte = (hd_byte << 1);
			index_k ++;

			if(index_k == 8)
			{
				hd_table[index_m] = hd_byte;
				index_k = 0;
				index_m ++;
				hd_byte = 0;
			}
		}
	}
	else if(modu_type == 4) //16APSK
	{		
		index_k = 0;
		index_m = 0;
		hd_byte = 0;
		for (index_i = cmp_start_addr; index_i < plf_len; index_i++)
		{
			data_i = plsn_in_i[index_i]; //bit[1]

			if(data_i < 0)
				hd_byte = (hd_byte << 1) + 1;
			else
				hd_byte = (hd_byte << 1);
			index_k ++;

			if(index_k == 8)
			{
				hd_table[index_m] = hd_byte;
				index_k = 0;
				index_m ++;
				hd_byte = 0;
			}
		}
	}
	else if(modu_type == 5) //32APSK
	{
		index_k = 0;
		index_m = 0;
		hd_byte = 0;
		slope_k = 0;
		for (index_i = cmp_start_addr; index_i < plf_len; index_i++)
		{
			data_i = plsn_in_i[index_i]; //bit[2]
			data_q = plsn_in_q[index_i]; 

			if((data_i < 0) || ((data_i == 0) && (data_q > 0)))
				//if((data_q*128) < ((-1)*slope_k*data_i))
				hd_byte = (hd_byte << 1) + 1;
			else
				hd_byte = (hd_byte << 1);
			index_k ++;

			if(index_k == 8)
			{
				hd_table[index_m] = hd_byte;
				index_k = 0;
				index_m ++;
				hd_byte = 0;
			}
		}
	}
	else //QPSK
	{
		index_k = 0;
		index_m = 0;
		hd_byte = 0;
		for (index_i = cmp_start_addr; index_i < plf_len; index_i++)
		{
			data_i = plsn_in_i[index_i];
			data_q = plsn_in_q[index_i];

			if(data_i < 0)
				hd_byte = (hd_byte << 1) + 1;
			else
				hd_byte = (hd_byte << 1);
			index_k ++;
			if(data_q < 0)
				hd_byte = (hd_byte << 1) + 1;
			else
				hd_byte = (hd_byte << 1);
			index_k ++;

			if(index_k == 8)
			{
				hd_table[index_m] = hd_byte;
				index_k = 0;
				index_m ++;
				hd_byte = 0;
			}
		}
	}
}

INT32 nim_c3505_search_plsn_bbh(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_data_i, INT32 *plsn_data_q, INT32 plsn_data_cnt, INT32 goldn_now)
{
	INT32 index_i = 0;
	INT32 index_j = 0;
	INT32 index_k = 0;
	INT32 index_m = 0;
	INT32 tmp = 0;
	
	INT32 bch_k_cnt = 0;
	INT32 bbs_sq_start = 0;
	INT32 bbs_sq_end = 0;
	INT32 bbs_sq_len = 0;
	INT32 plds_len = 0;
	INT32 symbol_start_addr = 0;
	UINT32 bbh_para[5] = {0};
	
	INT32 point_num = 0;
	INT32 p0_num = 0;
	INT32 p1_num = 0;
	INT32 p2_num = 0; 
	
	INT32 goldn_i = 0;
	
	UINT8 plsn_find = 0;
	UINT8 cmp_a_byte = 0;
	UINT8 cmp_c_byte = 0;
	INT32 bbh_diff_num = 0;
	INT32 bbh_diff_num_thr = 0;
	
	UINT32 sq_z = 0;
	UINT32 sq_zn = 0;
	UINT32 fb_x = 0;
	UINT32 sq_rn = 0;
	UINT32 sq_x_0 = 0;
	UINT32 sq_x_1 = 0;
	UINT32 sq_y_0 = 0;
	UINT32 sq_y_1 = 0;
	UINT32 sq_x_reg = 0;
//	UINT32 sq_y_reg = 0;
	UINT32 num_j = 0;
	UINT32 bit_k = 0;
	
	INT32 error_i = 0;
	INT32 error_q = 0;
	INT32 currpower = 0;
	INT32 min_power = 0;
	INT32 min_indx = 0;
	
	UINT8 hd_bit = 0;
	UINT8 hd_byte = 0;
	
	INT32 data_i = 0;
	INT32 data_q = 0;
	INT32 data_i_abs = 0;
	INT32 data_q_abs = 0;
	INT32 demap_table_i = 0;
	INT32 demap_table_q = 0;
	
	//INT32 plsn_data_i_tmp = 0;
	//INT32 plsn_data_q_tmp = 0;
	INT32 gold0_pls_sq_diff_tmp = 0;

	
	struct nim_c3505_private *priv = NULL;
	
	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}
	
	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}
	
	//step0 - generate bbs_sq
	bbs_sq_start = 16;
	bbs_sq_end = 56; //detection area:bbheader_byte[2]~[6],7*8=56bits
	bbs_sq_len = 56;
	if(modu_type > 2)
	{
		symbol_start_addr = bbs_sq_start;
		plds_len = bbs_sq_len;
	}
	else
	{
		symbol_start_addr = bbs_sq_start/2;
		plds_len = bbs_sq_len/2;
	}
	bch_k_cnt = FEC_K_BCH[frame_mode*11+coderate];
	tmp = bch_k_cnt - 80;
	bbh_para[0] = 0x05; //UPL=188*8=1504=0x05e0
	bbh_para[1] = 0xe0;
	bbh_para[2] = ((tmp>>8)&0xff); //DFL=Kbch-80
	bbh_para[3] = (tmp&0xff);
	bbh_para[4] = 0x47; //SYNC
	nim_c3505_get_bbs_sq(plsn_search_type,bbs_sq_start,bbs_sq_len,bbh_para);
	
	//step0 - get demapping table
	point_num = nim_c3505_get_map_table(frame_mode, modu_type, coderate, &p0_num, &p1_num, &p2_num);
	
	//step0 - pl scrambling
	nim_c3505_pl_scrambling(plsn_data_i, plsn_data_q, plds_len, symbol_start_addr, goldn_now, plsn_search_type);
	
	if(modu_type <= 3) //qpsk or 8psk
		bbh_diff_num_thr = 3;

	priv->plsn.plsn_num = 0;
	memset(priv->plsn.plsn_val,0,sizeof(priv->plsn.plsn_val));
	for(goldn_i = 0; goldn_i < GOLD_N_NUM; goldn_i++)
	{
		if(priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: exit&stop = 1 \n",__FUNCTION__, __LINE__);
			break;
		}

		if(priv->plsn.search_plsn_force_stop)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: force_stop = 1 \n", __FUNCTION__, __LINE__);
			break;
		}
		
		//-----------------------------------------------------------------------------------------
		//step0 - initiallize
		//-----------------------------------------------------------------------------------------
		//pl_decrambing
		if(goldn_i == 0)
		{
			sq_x_reg_init = 1;		//[17]-[0]:0x00001
			memset(gold0_pls_sq, 0, sizeof(gold0_pls_sq));
		}
		else
		{
			fb_x = (sq_x_reg_init & 0x01) ^ ((sq_x_reg_init >> 7) & 0x01);
			sq_x_reg_init = ((sq_x_reg_init & 0x3ffff) >> 1) + ((fb_x & 0x01) << 17);
		}
		sq_x_reg = sq_x_reg_init;
		num_j = 0;
		bit_k = 0;
		//demap_hard-decison
		min_power = 0;
		min_indx  = 0;
		index_k = 0;
		index_m = 0;
		hd_byte = 0;
		//compare
		bbh_diff_num = 0;
		plsn_find = 1;
	
		for(index_i = 0; index_i < plds_len; index_i++)
		{
			//-----------------------------------------------------------------------------------------
			//step1 - pl descrambling
			//-----------------------------------------------------------------------------------------
			//get sq_rn
			sq_x_0 = (sq_x_reg & 0x01);
			sq_y_0 = ((sq_y_0_buf[num_j] >> bit_k) & 0x01);
			sq_z = sq_x_0 ^ sq_y_0;
			sq_x_1 = ((sq_x_reg >> 4) & 0x01) ^ ((sq_x_reg >> 6) & 0x01) ^ ((sq_x_reg >> 15) & 0x01);
			sq_y_1 = ((sq_y_1_buf[num_j] >> bit_k) & 0x01);
			sq_zn = sq_x_1 ^ sq_y_1;
			sq_rn = sq_zn * 2 + sq_z;
			bit_k ++;
			if(bit_k == 32)
			{
				bit_k = 0;
				num_j ++;
			}
			//update sq_x_reg
			fb_x = (sq_x_reg & 0x01) ^ ((sq_x_reg >> 7) & 0x01);
			sq_x_reg = ((sq_x_reg & 0x3ffff) >> 1) + ((fb_x & 0x01) << 17);
			//descrambling
			if(index_i < symbol_start_addr)
				continue;
			else
			{
				if(sq_rn == 1)
				{
					data_i = plsn_data_q[index_i];
					data_q = -plsn_data_i[index_i];
				}
				else if(sq_rn == 2)
				{
					data_i = -plsn_data_i[index_i];
					data_q = -plsn_data_q[index_i];
				}
				else if(sq_rn == 3)
				{
					data_i = -plsn_data_q[index_i];
					data_q = plsn_data_i[index_i];
				}
				else
				{
					data_i = plsn_data_i[index_i];
					data_q = plsn_data_q[index_i];
				}
			}
			//store
			if(goldn_i == 0)
			{
				gold0_pls_sq[index_i] = sq_rn;
			}
			else
			{
				if(sq_rn < gold0_pls_sq[index_i])
				{
					gold0_pls_sq_diff_tmp = 4 - (gold0_pls_sq[index_i] - sq_rn);
				}
				else
				{
					gold0_pls_sq_diff_tmp = sq_rn - gold0_pls_sq[index_i];
				}
			}
			//-----------------------------------------------------------------------------------------
			//step2 - hard decision
			//-----------------------------------------------------------------------------------------
			if(goldn_i == 0)
			{
				if(data_i < 0) data_i_abs = -data_i;
				else		   data_i_abs = data_i;
				if(data_q < 0) data_q_abs = -data_q;
				else		   data_q_abs = data_q;
	
				for (index_j = 0; index_j < point_num; index_j += 6)
				{
					error_i = data_i_abs - demap_table[index_j];
					error_q = data_q_abs - demap_table[index_j + 1];
					currpower = error_i * error_i + error_q * error_q;
					if((index_j == 0) || (min_power > currpower))
					{
						min_power = currpower;
						min_indx  = index_j;
					}
				}
				gold0_demap_hd[index_i] = min_indx;
			}
			else
			{
				min_indx = gold0_demap_hd[index_i];
				if((gold0_pls_sq_diff_tmp & 0x01) == 0x01)
				{
					data_i_abs = demap_table[min_indx + 1];
					data_q_abs = demap_table[min_indx];
				}
				else
				{
					data_i_abs = demap_table[min_indx];
					data_q_abs = demap_table[min_indx + 1];
				}
	
				min_indx = 0;
				for (index_j = 0; index_j < point_num; index_j += 6)
				{
					demap_table_i = demap_table[index_j];
					demap_table_q = demap_table[index_j + 1];
					if((data_i_abs == demap_table_i) && (data_q_abs == demap_table_q))
					{
						min_indx  = index_j;
						break;
					}
				}
			}
	
			if(demap_table[min_indx] == 0) //I==0
				if(data_q >= 0)
					min_indx = demap_table[min_indx + 2];
				else
					min_indx = demap_table[min_indx + 4];
			else if(demap_table[min_indx + 1] == 0) //Q==0
				if(data_i >= 0)
					min_indx = demap_table[min_indx + 2];
				else
					min_indx = demap_table[min_indx + 4];
			else if((data_i > 0) && (data_q > 0))		 //1 quadrant
				min_indx = demap_table[min_indx + 2];
			else if((data_i < 0) && (data_q > 0))		 //2 quadrant
				min_indx = demap_table[min_indx + 3];
			else if((data_i > 0) && (data_q < 0))		 //4 quadrant
				min_indx = demap_table[min_indx + 5];
			else										 //3 quadrant
				min_indx = demap_table[min_indx + 4];
	
			if(modu_type <= 2) //QPSK
			{
				hd_bit = min_indx & 0x03;
				hd_byte = (hd_byte << 2) + hd_bit;
				index_k = index_k + 2;
			}
			else
			{
				if((modu_type == 3) && (coderate == 4)) //8psk & 3/5
					hd_bit = min_indx & 0x01;
				else
					hd_bit = (min_indx >> (modu_type - 1)) & 0x01;
				hd_byte = (hd_byte << 1) + hd_bit;
				index_k ++;
			}
			//-----------------------------------------------------------------------------------------
			//step3 - compare
			//-----------------------------------------------------------------------------------------
			if(index_k == 8)
			{
				cmp_a_byte = bbs_sq[index_m]&0xff;
				cmp_c_byte = cmp_a_byte^hd_byte;
				while(cmp_c_byte > 0)
				{
					cmp_c_byte = cmp_c_byte & (cmp_c_byte - 1);
					bbh_diff_num ++;
				}
				if((bbh_diff_num > bbh_diff_num_thr) && (goldn_i > 0))
				{
					plsn_find = 0;
					break;
				}
				//initiallize
				index_m ++;
				index_k = 0;
				hd_byte = 0;
			}
		}
		plsn_find = (bbh_diff_num <= bbh_diff_num_thr);
		if(plsn_find == 1)
		{
			priv->plsn.plsn_val[priv->plsn.plsn_num] = goldn_i;
			priv->plsn.plsn_num ++;
			//SEARCH_PLSN_PRINTF("find plsn = %d using BB_HEADER(bbh_diff_num = %d) ================== \n",goldn_i,bbh_diff_num);
		}
	}
	return SUCCESS;
}


INT32 nim_c3505_search_plsn_null(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_data_i, INT32 *plsn_data_q, INT32 plsn_data_cnt, INT32 goldn_now)
{
	INT32 index_j = 0;
	INT32 index_k = 0;
	INT32 tmp = 0;

	INT32 fec_frame_len = 64800;
	INT32 bch_k_cnt = 0;
	INT32 interleaver_row = 0;
	INT32 bbs_sq_start = 0;
	INT32 bbs_sq_end = 0;
	INT32 bbs_sq_len = 0;
	INT32 plds_len = 0;
	INT32 symbol_start_addr = 0;
	INT32 bitbyte_start_addr = 0;
	UINT32 para_data[5] = {0};
	INT32 bit_pos = 0;

	INT32 point_num = 0;
	INT32 p0_num = 0;
	INT32 p1_num = 0;
	INT32 p2_num = 0; 

	INT32 goldn_i = 0;

	INT32 watch_window_width = 8*14; //must>8*13(16APSK_2F3_AWGNOFF) //8*20; //null-packet=188*8=1504bit
	UINT8 plsn_find = 0;
	INT32 same_num = 0;
	INT32 diff_num = 0;
	UINT8 cmp_a_byte = 0;
	UINT8 cmp_b_byte = 0;
	UINT8 cmp_c_byte = 0;
	UINT8 cmp_d_byte = 0;
	UINT8 cmp_e_byte = 0;
	INT32 bbh_diff_num = 0;
	INT32 bbh_diff_num_thr = 0;
	INT32 diff_bit0[20] = {0};
	INT32 diff_bit1[20] = {0};
	INT32 diff_bit0_sum = 0;
	INT32 diff_bit1_sum = 0;
	INT32 diff_bit_thr = 20;
	INT32 cmp_counter = 0;
	INT32 cmp_bytes_num = 14;
	INT32 diff_bit0_min = 500;
	INT32 diff_bit1_min = 500;

	struct nim_c3505_private *priv = NULL;

	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	//step0 - generate bbs_sq
	if(frame_mode == 1)
		fec_frame_len = 16200;
	bch_k_cnt = FEC_K_BCH[frame_mode*11+coderate];
	if(plsn_search_type == 1) //1-null-packet&point_hd
	{
		if(modu_type > 2) //8psk~16apsk~32apsk
		{
			interleaver_row = fec_frame_len/modu_type;
			if(bch_k_cnt <= interleaver_row)
				bbs_sq_len = bch_k_cnt;
			else
				bbs_sq_len = interleaver_row;
			if(bbs_sq_len > MAX_CMP_LENGTH)
				bbs_sq_len = MAX_CMP_LENGTH;
			plds_len = bbs_sq_len;
			symbol_start_addr = 80;
		}
		else //qpsk
		{
			bbs_sq_len = bch_k_cnt;
			if(bbs_sq_len > MAX_CMP_LENGTH)
				bbs_sq_len = MAX_CMP_LENGTH;
			plds_len = bbs_sq_len/2;
			symbol_start_addr = 40;
		}
		nim_c3505_get_bbs_sq(plsn_search_type,0,bbs_sq_len,para_data);
	}
	else if((plsn_search_type == 2)||(plsn_search_type == 3)) //2-using null-packet&area_hd; 3-using null-packet&point_area_hd;
	{
		if(modu_type > 2) //8psk~16apsk~32apsk~64apsk
		{
			if(modu_type == 5) //32APSK
				bit_pos = 2;
			else
				bit_pos = 1;
			interleaver_row = fec_frame_len/modu_type;
			if(bch_k_cnt < (interleaver_row * (modu_type - bit_pos - 1)))
			{
				SEARCH_PLSN_PRINTF("Error: have no enough data \n");
				return ERR_PARA;
			}
			else if(bch_k_cnt < (interleaver_row * (modu_type - bit_pos)))
			{
				bbs_sq_start = interleaver_row * (modu_type - bit_pos - 1);
				bbs_sq_end = bch_k_cnt;
			}
			else
			{
				bbs_sq_start = interleaver_row * (modu_type - bit_pos - 1);
				bbs_sq_end = interleaver_row * (modu_type - bit_pos);
			}
			bbs_sq_len = bbs_sq_end - bbs_sq_start;
			if(bbs_sq_len > MAX_CMP_LENGTH)
				bbs_sq_len = MAX_CMP_LENGTH;
			bbs_sq_end = bbs_sq_start + bbs_sq_len;
			plds_len = bbs_sq_len;
			symbol_start_addr = 80;
		}
		else //qpsk
		{
			bbs_sq_len = bch_k_cnt;
			if(bbs_sq_len > MAX_CMP_LENGTH)
				bbs_sq_len = MAX_CMP_LENGTH;
			plds_len = bbs_sq_len/2;
			symbol_start_addr = 40;
			bbs_sq_start = 0;
			bbs_sq_end = bbs_sq_len;
		}
		nim_c3505_get_bbs_sq(plsn_search_type,bbs_sq_start,bbs_sq_end,para_data);
	}
	else //4-using BBHeader;
	{
		bbs_sq_start = 16;
		bbs_sq_end = 56; //detection area:bbheader_byte[2]~[6],7*8=56bits
		bbs_sq_len = 56;
		if(modu_type > 2)
		{
			symbol_start_addr = bbs_sq_start;
			plds_len = bbs_sq_len;
		}
		else
		{
			symbol_start_addr = bbs_sq_start/2;
			plds_len = bbs_sq_len/2;
		}
		tmp = bch_k_cnt - 80;
		para_data[0] = 0x05; //UPL=188*8=1504=0x05e0
		para_data[1] = 0xe0;
		para_data[2] = ((tmp>>8)&0xff); //DFL=Kbch-80
		para_data[3] = (tmp&0xff);
		para_data[4] = 0x47; //SYNC
		nim_c3505_get_bbs_sq(plsn_search_type,bbs_sq_start,bbs_sq_len,para_data);
	}
	//step0 - get demapping table
	point_num = nim_c3505_get_map_table(frame_mode, modu_type, coderate, &p0_num, &p1_num, &p2_num);

	//step0 - pl scrambling
	nim_c3505_pl_scrambling(plsn_data_i, plsn_data_q, plds_len, symbol_start_addr, goldn_now, plsn_search_type);
	
    if(modu_type <= 3) //qpsk or 8psk
	    bbh_diff_num_thr = 3;

	priv->plsn.plsn_num = 0;
	memset(priv->plsn.plsn_val,0,sizeof(priv->plsn.plsn_val));
	for(goldn_i = 0; goldn_i < GOLD_N_NUM; goldn_i++)
	{
		if(priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: exit&stop = 1 \n",__FUNCTION__, __LINE__);
			break;
		}

		if(priv->plsn.search_plsn_force_stop)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: force_stop = 1 \n", __FUNCTION__, __LINE__);
			break;
		}

		//step1 - pl descrambling
		nim_c3505_pl_descrambling(plds_data_i, plds_data_q, plsn_data_i, plsn_data_q, plds_len, goldn_i, symbol_start_addr);

		//step2 - hard decision
		if(plsn_search_type == 2)
			nim_c3505_demap_area_hard_decison(frame_mode, modu_type, coderate, plds_data_i, plds_data_q, plds_len, symbol_start_addr);
		else
			nim_c3505_demap_point_hard_decison(plsn_search_type, goldn_i, frame_mode, modu_type, coderate, plds_data_i, plds_data_q, plds_len, point_num, symbol_start_addr);

		//step3 - compare
		if(plsn_search_type == 4)
		{
			bbh_diff_num = 0;
			for(index_j = 0; index_j < 5; index_j++)
			{
				cmp_a_byte = bbs_sq[index_j]&0xff;
				cmp_b_byte = hd_table[index_j]&0xff;
				cmp_c_byte = cmp_a_byte^cmp_b_byte;
				while(cmp_c_byte > 0)
				{
					cmp_c_byte = cmp_c_byte & (cmp_c_byte - 1);
					bbh_diff_num ++;
				}
				if(bbh_diff_num > bbh_diff_num_thr)
					break;
			}
			plsn_find = (bbh_diff_num <= bbh_diff_num_thr);
			if(plsn_find == 1)
			{
				priv->plsn.plsn_val[priv->plsn.plsn_num] = goldn_i;
				priv->plsn.plsn_num ++;
				//SEARCH_PLSN_PRINTF("find plsn = %d using BB_HEADER(bbh_diff_num = %d) ================== \n",goldn_i,bbh_diff_num);
			}
		}
		else
		{
			if(modu_type > 2)
				bitbyte_start_addr = symbol_start_addr/8;
			else
				bitbyte_start_addr = symbol_start_addr/4;
			plsn_find = 0;
			if(1)
			{
				cmp_counter = 0;
				diff_bit0_min = 500;
				diff_bit1_min = 500;
				for(index_j = bitbyte_start_addr; index_j < bbs_sq_len/8; index_j++)
				{
					cmp_a_byte = bbs_sq[index_j]&0xff;
					cmp_b_byte = ~(bbs_sq[index_j]&0xff);
					cmp_c_byte = hd_table[index_j - bitbyte_start_addr]&0xff;
					cmp_d_byte = cmp_a_byte ^ cmp_c_byte;
					cmp_e_byte = cmp_b_byte ^ cmp_c_byte;
					diff_bit0[cmp_counter] = 0;
					diff_bit1[cmp_counter] = 0;
					for(index_k = 0; index_k < 8; index_k++)
					{
						diff_bit0[cmp_counter] += (cmp_d_byte >> index_k) & 0x01;
						diff_bit1[cmp_counter] += (cmp_e_byte >> index_k) & 0x01;
					}
					cmp_counter ++;
					if(cmp_counter >= cmp_bytes_num)
					{
						cmp_counter = 0;
						diff_bit0_sum = 0;
						diff_bit1_sum = 0;
						for(index_k = 0; index_k < cmp_bytes_num; index_k++)
						{
							diff_bit0_sum += diff_bit0[index_k];
							diff_bit1_sum += diff_bit1[index_k];
						}
						if(diff_bit0_min > diff_bit0_sum)
							diff_bit0_min = diff_bit0_sum;
						if(diff_bit1_min > diff_bit1_sum)
							diff_bit1_min = diff_bit1_sum;
						if((diff_bit0_sum < diff_bit_thr)||(diff_bit1_sum < diff_bit_thr))
						{
							plsn_find = 1;
							SEARCH_PLSN_PRINTF("find plsn = %d, diff_num[%d %d]================== \n",goldn_i,diff_bit0_sum,diff_bit1_sum);
							break;
						}
					}
				}
				//SEARCH_PLSN_PRINTF("%d  %d  %d \n",diff_bit0_min,diff_bit1_min,goldn_i);
			}
			else
			{
				same_num = 0;
				diff_num = 0;
				for(index_j = bitbyte_start_addr; index_j < bbs_sq_len/8; index_j++)
				{
					cmp_a_byte = bbs_sq[index_j]&0xff;
					cmp_b_byte = ~(bbs_sq[index_j]&0xff);
					cmp_c_byte = hd_table[index_j - bitbyte_start_addr]&0xff;
					cmp_d_byte = cmp_a_byte ^ cmp_c_byte;
					cmp_e_byte = cmp_b_byte ^ cmp_c_byte;
					if(cmp_d_byte == 0)
					{
						same_num ++;
						diff_num = 0;
					}
					else if(cmp_e_byte == 0)
					{
						diff_num ++;
						same_num = 0;
					}
					else
					{
						diff_num = 0;
						same_num = 0;
					}
					if((same_num >= watch_window_width/8) || (diff_num >= watch_window_width/8))
					{
						plsn_find = 1;
						SEARCH_PLSN_PRINTF("find plsn = %d, same_addr[%d , %d]bytes =================\n",goldn_i,index_j-watch_window_width/8,index_j);
						break;
					}
				}
			}
			if(plsn_find == 1)
			{
				priv->plsn.plsn_val[priv->plsn.plsn_num] = goldn_i;
				priv->plsn.plsn_num ++;
			}
		}
	}
	return SUCCESS;
}

/*****************************************************************************
*  INT32 nim_C3505_search_plsn_pilot(struct nim_device *dev)
* 
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_c3505_search_plsn_pilot(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 *plsn_pilot_i, INT32 *plsn_pilot_q, INT32 plsn_pilot_cnt, INT32 goldn_now)
{
	INT32 index_i = 0;
	INT32 index_j = 0;
	//INT32 index_k = 0;
	//INT32 index_m = 0;
	INT32 tmp = 0;

	INT32 plsn_pilot_diff_phase[35];
	INT32 plsn_pilot_diff_phase_tab[3];
	INT32 plsn_pilot_angle = 0;
	INT32 plsn_pilot_angle_last = 0;

	INT32 min_diff_num = 35;
	INT32 diff_num = 0;
	//INT32 shift_num = 0;
	//INT32 shift_value = 0;

	UINT8 plsn_find = 0;
	INT32 diff_phase_xor0 = 0;
	INT32 diff_phase_xor1 = 0;
	INT32 diff_phase_xor2 = 0;	

	UINT32 start_time = 0;

	struct nim_c3505_private *priv = NULL;

	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	if (NULL == plsn_sq_rn_pilot_table)//the upper level have not generate table
	{
		SEARCH_PLSN_PRINTF("[%s %d]the table address is NULL\n", __FUNCTION__, __LINE__);
		return ERR_NO_MEM;
	}
	else
	{
		start_time = osal_get_tick();
		while (!g_table_finish_flag)
		{
			comm_sleep(2);
			if (osal_get_tick() - start_time > 7500)//the max generate time is 7500 ms
			{
				SEARCH_PLSN_PRINTF("[%s %d]generate table timeout = %d ms\n", __FUNCTION__, __LINE__, osal_get_tick() - start_time);
				break;
			}
		}
	}

	SEARCH_PLSN_PRINTF("Enter %s \n",__FUNCTION__);

	//pl-scrambling
	if(data_src != 0)
		nim_c3505_pl_scrambling(plsn_pilot_i, plsn_pilot_q, 1476, 1440, goldn_now, plsn_search_type);

	//calc phase_diff
	memset(plsn_pilot_diff_phase,0,sizeof(plsn_pilot_diff_phase));
	for(index_i = 0; index_i < 36; index_i++)
	{
		angle_by_cordic(plsn_pilot_i[index_i], plsn_pilot_q[index_i], &plsn_pilot_angle);
		if(index_i > 0)
		{
			tmp = plsn_pilot_angle - plsn_pilot_angle_last;
			if(tmp < 0)
				tmp = 360 + tmp;

			if(tmp <= 45)
				plsn_pilot_diff_phase[index_i-1] = 0;
			else if(tmp <= 135)
				plsn_pilot_diff_phase[index_i-1] = 1;
			else if(tmp <= 225)
				plsn_pilot_diff_phase[index_i-1] = 2;
			else if(tmp <= 315)
				plsn_pilot_diff_phase[index_i-1] = 3;
			else
				plsn_pilot_diff_phase[index_i-1] = 0;
		}
		plsn_pilot_angle_last = plsn_pilot_angle;
	}
	//merge
	memset(plsn_pilot_diff_phase_tab,0,sizeof(plsn_pilot_diff_phase_tab));
	index_j = 0;
	for(index_i = 0; index_i < 35; index_i++)
	{
		if(((index_i%12) == 0)&&(index_i>0))
			index_j ++;
		plsn_pilot_diff_phase_tab[index_j] = ((plsn_pilot_diff_phase[index_i])&0x03) + (plsn_pilot_diff_phase_tab[index_j]<<2);
	}
	SEARCH_PLSN_PRINTF("%s , step2 OK, plsn_pilot_diff_phase_tab = [%d , %d , %d]\n",__FUNCTION__,
		plsn_pilot_diff_phase_tab[0],plsn_pilot_diff_phase_tab[1],plsn_pilot_diff_phase_tab[2]);

	//decision
	priv->plsn.plsn_num = 0;
	memset(priv->plsn.plsn_val,0,sizeof(priv->plsn.plsn_val));
	for(index_i = 0; index_i < GOLD_N_NUM; index_i++)
	{
	
#if 0
		plsn_find = 1;
		for(index_j = 0; index_j < 3; index_j++)
		{
			if(plsn_pilot_diff_phase_tab[index_j] != *(plsn_sq_rn_pilot_table + index_i*3 + index_j))
			{
				plsn_find = 0;
				break;
			}
		}	
#endif

#if 0
		plsn_find = 1;
		diff_phase_xor0 = plsn_pilot_diff_phase_tab[0]^(*(plsn_sq_rn_pilot_table + index_i*3));
		diff_phase_xor1 = plsn_pilot_diff_phase_tab[1]^(*(plsn_sq_rn_pilot_table + index_i*3 + 1));
		diff_phase_xor2 = plsn_pilot_diff_phase_tab[2]^(*(plsn_sq_rn_pilot_table + index_i*3 + 2));
		if(diff_phase_xor0 || diff_phase_xor1 || diff_phase_xor2)
		{
			plsn_find = 0;
		}
#endif

#if 1
		diff_phase_xor0 = plsn_pilot_diff_phase_tab[0]^(*(plsn_sq_rn_pilot_table + index_i*3));
		diff_phase_xor1 = plsn_pilot_diff_phase_tab[1]^(*(plsn_sq_rn_pilot_table + index_i*3 + 1));
		diff_phase_xor2 = plsn_pilot_diff_phase_tab[2]^(*(plsn_sq_rn_pilot_table + index_i*3 + 2));
		diff_num = 0;
		for(index_j = 0; index_j < 24; index_j += 2)
		{
			diff_num += (((diff_phase_xor0 >> index_j) & 0x03) > 0);
			diff_num += (((diff_phase_xor1 >> index_j) & 0x03) > 0);
			diff_num += (((diff_phase_xor2 >> index_j) & 0x03) > 0);
			if(diff_num > 5)
				break;
		}
		plsn_find = (diff_num <= 5);
		if(min_diff_num > diff_num)
			min_diff_num = diff_num;
#endif
		
		if(plsn_find == 1)
		{
			priv->plsn.plsn_val[priv->plsn.plsn_num] = index_i;
			priv->plsn.plsn_num ++;
			//SEARCH_PLSN_PRINTF("%s , diff_num = %d \n",__FUNCTION__,diff_num);
		}
	}
	SEARCH_PLSN_PRINTF("%s , step3 OK , gold_i = %d , plsn = %d \n",__FUNCTION__,index_i,priv->plsn.plsn_num);
	return SUCCESS;
}

/*****************************************************************************
*  INT32 nim_c3505_search_plsn(struct nim_device *dev)
* 
*
* Arguments:
*  Parameter1: struct nim_device *dev
*
* Return Value: INT32
*****************************************************************************/
INT32 nim_c3505_search_plsn(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 *valid_frame_s, INT32 *valid_frame_m)
{
	INT32 index_i = 0;
	//INT32 index_j = 0;
	//INT32 index_k = 0;

	//INT32 plf_len = 0;

	INT32 plfs_in_i = 0;
	INT32 plfs_in_q = 0;
	INT32 plfs_out_i_tmp = 0;
	INT32 plfs_out_q_tmp = 0;
	INT32 plfs_success = 0;
	INT32 frame_start = 0;
	INT32 is_header = 0;
	INT32 is_data = 0;
	INT32 is_pilot = 0;
	INT32 sym_type = 0;
	INT32 plframe_len;
	INT32 pilot_on;
	INT32 modcod;
	INT32 fecframe_mode = 0;
	INT32 modu_type = 0;
	INT32 coderate = 0;
	INT32 dummy_flag;
	INT32 plfs_success_last = 0;

	INT32 plsn_fecframe_mode_tmp = 0;
	INT32 plsn_modu_type_tmp = 0;
	INT32 plsn_coderate_tmp = 0;
	UINT8 plsn_pilot_flag_tmp = 0;

	INT32 plsn_fecframe_mode = 0;
	INT32 plsn_modu_type = 0;
	INT32 plsn_coderate = 0;
	UINT8 plsn_para_update = 0;
	INT32 plsn_pilot_flag = 0;
	INT32 plsn_dummy_flag = 0;

	INT32 plf_out_ready = 0;
	INT32 plf_out_enable = 0;
	//INT32 plf_out_index = 0;

	INT32 sim_end = 0;
	INT32 search_plsn_cnt = 0;
	INT32 pd_invalid_cnt = 0;

	UINT32 dram_len = 665640; //33282*2*100 = 6656400 ~=6.5MBytes; 33282*2*20 = 1331280 ~=1.3MBytes; 33282*2*10 = 665640 ~=0.6MBytes
	UINT8  cap_src = 0x07;
	UINT8 *dram_base_ptr;
	INT32 index_ptr = 0;
	INT32 result = SUCCESS;

	UINT8 data = 0x00;
	UINT8 cr97_data = 0x00;
	UINT8 cr98_data = 0x00;

	UINT32 start_time = 0;
	UINT32 end_time = 0;

	struct nim_c3505_private *priv = NULL;

	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	SEARCH_PLSN_PRINTF("Enter %s \n",__FUNCTION__);

	//step1
	//capture data to DRAM
	dram_base_ptr = g_cap_addr;
	SEARCH_PLSN_PRINTF("dram_base_ptr=0x%x\n", dram_base_ptr);

	if (NULL==dram_base_ptr)
	{
		SEARCH_PLSN_PRINTF("[%s %d]the capture address is NULL\n", __FUNCTION__, __LINE__);
		return ERR_NO_MEM;
	}
	if(data_src == 0)
	{
		cap_src = 0x07; //EQ_OUT
		dram_len = 1331280;
	}
	else
	{
		cap_src = 0x0b; //CR_OUT
		dram_len = 1331280;

		nim_reg_read(dev, R97_S2_FEC_THR, &cr97_data, 1);//set S2_FEC_LOCK_THR[7:0] = 1
		nim_reg_read(dev, R98_S2_FEC_FAIL_THR, &cr98_data, 1);//set S2_FEC_FAIL_THR[7:0] = ff		
		data = 0x01;	   
		nim_reg_write(dev, R97_S2_FEC_THR, &data, 1);//set S2_FEC_LOCK_THR[7:0] = 1
		data = 0xff;
		nim_reg_write(dev, R98_S2_FEC_FAIL_THR, &data, 1);//set S2_FEC_FAIL_THR[7:0] = ff
	}

	nim_reg_read(dev, R04_STATUS, &data, 1);
	SEARCH_PLSN_PRINTF("[%s %d]: before capture R04_STATUS = %x \n",__FUNCTION__, __LINE__, data);
	data = 0x00;
	nim_reg_write(dev, R02_IERR, &data, 1); //clear inuerrupt events
	SEARCH_PLSN_PRINTF("Enter capture data: %x %d!\n",cap_src,dram_len);
	start_time = osal_get_tick();
	nim_c3505_adc2mem_entity(dev,dram_base_ptr,dram_len,cap_src);
	end_time = osal_get_tick();
	SEARCH_PLSN_PRINTF("[%s %d]nim_c3505_adc2mem_entity cost %d ms time\n", __FUNCTION__, __LINE__, end_time - start_time);
	nim_reg_read(dev, R02_IERR, &data, 1); //read interrupt events
	SEARCH_PLSN_PRINTF("search_plsn capture data OK , R02_reg = %x \n",data);
	nim_reg_read(dev, R04_STATUS, &data, 1);
	SEARCH_PLSN_PRINTF("[%s %d]: after capture R04_STATUS = %x \n",__FUNCTION__, __LINE__, data);

	SEARCH_PLSN_PRINTF("dram_base_ptr-2 = 0x%x, dram_base_ptr-1 = 0x%x\n", dram_base_ptr + dram_len - 2,dram_base_ptr + dram_len - 1 );
	SEARCH_PLSN_PRINTF("%02x%02x\n",*(dram_base_ptr + dram_len - 2),*(dram_base_ptr + dram_len - 1));

	if(data_src != 0)
	{
		nim_reg_write(dev, R97_S2_FEC_THR, &cr97_data, 1);//set S2_FEC_LOCK_THR[7:0] = 1
		nim_reg_write(dev, R98_S2_FEC_FAIL_THR, &cr98_data, 1);//set S2_FEC_FAIL_THR[7:0] = ff	
	}
	pl_frame_sync_init();
	plf_out_enable = 0;
	index_ptr = 0;
	priv->plsn.plsn_num = 0;
	priv->plsn.plsn_pls_lock = 0;
	priv->plsn.plsn_pls_timeout = 0;
	start_time = osal_get_tick();
	while(1)
	{
		if(priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: exit&flag = 1 \n",__FUNCTION__,  __LINE__);
			sim_end = 1;
		}

		if(priv->plsn.search_plsn_force_stop)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: force_stop = 1 \n", __FUNCTION__, __LINE__);
			sim_end = 1;
		}

		if(index_ptr >= dram_len)
		{
			SEARCH_PLSN_PRINTF("%s : data_end \n",__FUNCTION__);
			sim_end = 1;
		}

		if(sim_end == 1)
		{
			SEARCH_PLSN_PRINTF("[%s %d]break, exit while\n", __FUNCTION__, __LINE__);
			break;
		}
		
		//step2
		//PL_Sync
		plfs_in_i = *(dram_base_ptr + index_ptr);
		plfs_in_q = *(dram_base_ptr + index_ptr + 1);		
		if (plfs_in_i & 0x80)
			plfs_in_i |= 0xffffff00;
		if (plfs_in_q & 0x80)
			plfs_in_q |= 0xffffff00;
		index_ptr = index_ptr + 2;

		pl_frame_sync(plfs_in_i,plfs_in_q,&plfs_success,&frame_start,&is_header,&is_data,&is_pilot,&sym_type,&plfs_out_i_tmp,&plfs_out_q_tmp,&plframe_len,&modu_type,&pilot_on,&modcod,&coderate,&fecframe_mode,&dummy_flag,data_src);

	    if((plfs_success_last == 1)&&(plfs_success == 0))
		{
			plfs_success_last = 0;
			SEARCH_PLSN_PRINTF("========= pl_success unlock ====== \n\n");
			start_time = osal_get_tick();
		}
		
		//step3
		//framing
		if(plfs_success == 0)
		{
			if((osal_get_tick() - start_time) > 10000) //timeout=10s
			{
				SEARCH_PLSN_PRINTF("[%s %d]pl_frame_sync fail, timeout =  %d ms\n", __FUNCTION__, __LINE__, osal_get_tick() - start_time);
				priv->plsn.plsn_pls_timeout = 1;
				sim_end = 1;
			}
			continue;
		}

		if((plfs_success_last == 0)&&(plfs_success == 1))
		{
			end_time = osal_get_tick();
			SEARCH_PLSN_PRINTF("[%s %d]pl_frame_sync cost %d ms time\n", __FUNCTION__, __LINE__, end_time - start_time);
			SEARCH_PLSN_PRINTF("======= plfs_success == 1 ===========\n\n");
			plfs_success_last = 1;
			priv->plsn.plsn_pls_lock = 1;
		}

		plf_out_ready = 0;
		if(frame_start == 1)
		{
		    if(plf_out_enable == 1)
		    {
		    	*valid_frame_m = *valid_frame_m + 1;
				if(plsn_search_type > 0) //using null-packet/bbheader
				{
					if(pl_data_cnt > 0)
					{
				    	plsn_fecframe_mode = plsn_fecframe_mode_tmp;
						plsn_modu_type = plsn_modu_type_tmp;
						plsn_coderate = plsn_coderate_tmp;
						plsn_pilot_flag = plsn_pilot_flag_tmp;
						
						plsn_data_cnt = pl_data_cnt;
						memcpy(plsn_data_i,pl_data_i,sizeof(INT32)*pl_data_cnt);
						memcpy(plsn_data_q,pl_data_q,sizeof(INT32)*pl_data_cnt);
						plf_out_ready = 1;
					}
					else
					{
						SEARCH_PLSN_PRINTF("%d : have no valid data \n",__LINE__);
					}
				}
				else //default using pilot/dummy
				{
					if(pl_pilot_cnt > 0)
					{
					    pd_invalid_cnt = 0;
						
						plsn_fecframe_mode = plsn_fecframe_mode_tmp;
						plsn_modu_type = plsn_modu_type_tmp;
						plsn_coderate = plsn_coderate_tmp;
						plsn_pilot_flag = plsn_pilot_flag_tmp;
					
						plsn_pilot_cnt = pl_pilot_cnt;
						memcpy(plsn_pilot_i,pl_pilot_i,sizeof(INT32)*pl_pilot_cnt);
						memcpy(plsn_pilot_q,pl_pilot_q,sizeof(INT32)*pl_pilot_cnt);
						plf_out_ready = 1;
					}
					else
					{
#if (NIM_OPTR_CCM == ACM_CCM_FLAG)  
						sim_end = 1; //only support a kind of MODCODs in CCM mode  //have no pilot and dummy
						SEARCH_PLSN_PRINTF("%d : have no pilot and dummy \n",__LINE__);
#else
						pd_invalid_cnt = pd_invalid_cnt + 1;
						if(pd_invalid_cnt >= 10)
						{
							sim_end = 1;
							SEARCH_PLSN_PRINTF("%d : have no pilot and dummy \n",__LINE__);
						}
#endif
					}
				}
			}
			plf_out_enable = 1;
			plsn_para_update = 0;
			pl_pilot_cnt = 0;
			pl_data_cnt = 0;
		}

		if(plf_out_enable == 0)
			continue;

		if(is_header == 0)
		{
			if(plsn_search_type > 0) //using null-packet/bbheader
			{
				if((is_pilot == 0) && (pl_data_cnt < 3072)) //store all pl_data
				{
					pl_data_i[pl_data_cnt] = plfs_out_i_tmp;
					pl_data_q[pl_data_cnt] = plfs_out_q_tmp;
					pl_data_cnt ++;
				}
			}
			else //default using pilot/dummy
			{
				if((is_pilot == 1) && (pl_pilot_cnt < 36)) //only store the first pilot block
				{
					pl_pilot_i[pl_pilot_cnt] = plfs_out_i_tmp;
					pl_pilot_q[pl_pilot_cnt] = plfs_out_q_tmp;
					pl_pilot_cnt ++;
				}
			}

			if(plsn_para_update == 0) //only update one time during a PL frame
			{
				plsn_fecframe_mode_tmp = fecframe_mode;
				plsn_modu_type_tmp = modu_type;
				plsn_coderate_tmp = coderate;
				plsn_pilot_flag_tmp = pilot_on;
				plsn_para_update = 1;
			}
		}

		if(plf_out_ready == 0)
			continue;

		SEARCH_PLSN_PRINTF("[%s %d]:data_src = %d, plsn_search_type = %d, plsn_pilot_flag = %d , plsn_dummy_flag = %d\n", 
			__FUNCTION__, __LINE__, data_src, plsn_search_type, plsn_pilot_flag, plsn_dummy_flag);


		if((plsn_search_type == 0) && (plsn_pilot_flag == 0) && (plsn_dummy_flag == 0)) //have no pilot and dummy
			continue;

		if((plsn_pilot_flag == 1)||(plsn_dummy_flag == 1))
		{
			*valid_frame_s = *valid_frame_s + 1; //exist pilot or dummy
		}

		//step4
		//find plsN
		priv->plsn.plsn_num = 0;
		if(plsn_search_type > 0) //using null-packet/bbheader
		{
		    if(plsn_search_type == 4)
		    {
			    start_time = osal_get_tick();
			    result = nim_c3505_search_plsn_bbh(dev, data_src, plsn_search_type, plsn_fecframe_mode, plsn_modu_type, plsn_coderate, plsn_data_i, plsn_data_q, plsn_data_cnt, priv->plsn.plsn_now);
			    end_time = osal_get_tick();
			    SEARCH_PLSN_PRINTF("[%s %d]nim_c3505_search_plsn_bbh cost %d ms time\n", __FUNCTION__, __LINE__, end_time - start_time);
		    }
			else
			{
			    start_time = osal_get_tick();
			    result = nim_c3505_search_plsn_null(dev, data_src, plsn_search_type, plsn_fecframe_mode, plsn_modu_type, plsn_coderate, plsn_data_i, plsn_data_q, plsn_data_cnt, priv->plsn.plsn_now);
			    end_time = osal_get_tick();
			    SEARCH_PLSN_PRINTF("[%s %d]nim_c3505_search_plsn_null cost %d ms time\n", __FUNCTION__, __LINE__, end_time - start_time);
			}
		}
		else //default using pilot/dummy
		{
			start_time = osal_get_tick();
			result = nim_c3505_search_plsn_pilot(dev, data_src, plsn_search_type, plsn_pilot_i, plsn_pilot_q, plsn_pilot_cnt, priv->plsn.plsn_now);
			end_time = osal_get_tick();
			SEARCH_PLSN_PRINTF("[%s %d]nim_c3505_search_plsn_pilot cost %d ms time\n", __FUNCTION__, __LINE__, end_time - start_time);
		}

		if (result != SUCCESS)
		{
			nim_c3505_search_plsn_exit(dev);
			SEARCH_PLSN_PRINTF("[%s %d]result=%d\n", __FUNCTION__, __LINE__, result);
			return result;
		}

		search_plsn_cnt = search_plsn_cnt + 1;

		//exit conditions
		if((priv->plsn.plsn_num > 0)&&(priv->plsn.plsn_num <= 10)) 
		{
			SEARCH_PLSN_PRINTF("search_plsn success, number = %d, timer = %d\n",priv->plsn.plsn_num,search_plsn_cnt);
			for(index_i = 0; index_i < priv->plsn.plsn_num; index_i++)
				SEARCH_PLSN_PRINTF("	the %d-th suspicious gold_n : n = %d \n",index_i,priv->plsn.plsn_val[index_i]);
			sim_end = 1;
		}
		else if(priv->plsn.plsn_num > 10)
		{
			SEARCH_PLSN_PRINTF("search_plsn fail, plsn_num: %d > 10, timer = %d\n",priv->plsn.plsn_num,search_plsn_cnt);
			priv->plsn.plsn_num = 0;
		}
		else
		{
			SEARCH_PLSN_PRINTF("search_plsn fail, timer = %d\n",search_plsn_cnt);
			if(search_plsn_cnt >= 15)
				sim_end = 1;
		}
	}

	return SUCCESS;

}

INT32 nim_c3505_search_plsn_top(struct nim_device *dev)
{
	UINT8 data = 0;
	INT32 pl_timer = 0;	
	UINT8 pl_lock = 0;
	INT32 cr_timer = 0;
	UINT8 cr_lock = 0;
	INT32 plsn_timer = 0;
	UINT8 w1_sim_end = 0;
	UINT8 w2_sim_end = 0;
	UINT8 w3_sim_end = 0;
	INT32 result = SUCCESS;
	UINT32 i;
	UINT8 data_src = 0; //0-EQ_OUT, 1-CR_OUT
	INT32 plsn_search_type = 0; //0-using pilot/dummy; 1-null-packet&point_hd; 2-using null-packet&area_hd; 3-using null-packet&point_area_hd; 4-using BBHeader; 
	INT32 valid_frame_s = 0;
	INT32 valid_frame_m = 0;
	INT32 plsn_timer_thr = 15; //default:15
	INT32 pl_timer_thr = 100;
	UINT32 start_time = 0;
	UINT32 end_time = 0;

	struct nim_c3505_private *priv = NULL;

	if(NULL == dev)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	priv = (struct nim_c3505_private *)dev->priv; 
	if(NULL == dev->priv)
	{
		SEARCH_PLSN_PRINTF("[%s %d]NULL == dev->priv\n", __FUNCTION__, __LINE__);
		return ERR_NO_DEV;
	}

	SEARCH_PLSN_PRINTF("Enter %s \n",__FUNCTION__);

	//c3505_search_plsn
	priv->plsn.plsn_search_algo = plsn_search_type;
	priv->plsn.plsn_pls_unlock_cnt = 0;
	w1_sim_end = 0;
	pl_timer = 0;
	while(1)
	{
		if(priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag) //the variable will need to be corrected
		{
			nim_c3505_search_plsn_exit(dev);
			SEARCH_PLSN_PRINTF("[%s %d]: stop_flag = 1 \n", __FUNCTION__, __LINE__);
			w1_sim_end = 1;
		}

		if(priv->plsn.search_plsn_force_stop)
		{
			nim_c3505_search_plsn_exit(dev);
			SEARCH_PLSN_PRINTF("[%s %d]: search_plsn_force_stop = 1 \n", __FUNCTION__, __LINE__);
			w1_sim_end = 1;
		}

		if(w1_sim_end == 1)
		{
			SEARCH_PLSN_PRINTF("[%s %d]: w1_sim_end = 1, exit while \n", __FUNCTION__, __LINE__);
			break;
		}

		//waiting 10ms
		comm_sleep(10);
		pl_timer ++;
#if 1
		//waiting pl_lock or cr_lock
		data = 0x00;
		nim_reg_read(dev, R04_STATUS, &data, 1);
		pl_lock = data & 0x40;
		cr_lock = data & 0x08;
		SEARCH_PLSN_PRINTF("R04_STATUS:%x\n",data);
		if(((pl_lock == 0x40)&&(data_src == 0)) ||((cr_lock == 0x08)&&(data_src == 1)))
#else
		//waiting pl_lock
		//default:data_src=0,plsn_search_type=0
		data = 0x00;
		nim_reg_read(dev, R04_STATUS, &data, 1);
		pl_lock = data & 0x40;
		SEARCH_PLSN_PRINTF("R04_STATUS:%x\n",data);
		if(pl_lock == 0x40)
#endif
		{
			SEARCH_PLSN_PRINTF("pl_lock at pl_timer = %d \n",pl_timer);
			w2_sim_end = 0;
			plsn_timer = 0;
			while(1)
			{
				if(priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag)
				{
					SEARCH_PLSN_PRINTF("[%s %d]: stop_flag = 1 \n", __FUNCTION__, __LINE__);
					w2_sim_end = 1;
				}

				if(priv->plsn.search_plsn_force_stop)
				{
					SEARCH_PLSN_PRINTF("[%s %d]: search_plsn_force_stop = 1 \n",__FUNCTION__, __LINE__);
					w2_sim_end = 1;
				}

				if(w2_sim_end == 1)
				{
					SEARCH_PLSN_PRINTF("[%s %d]: w2_sim_end = 1 \n", __FUNCTION__, __LINE__);
					break;
				}

				plsn_timer ++;
				SEARCH_PLSN_PRINTF("[%s %d]plsn_timer = %d \n", __FUNCTION__, __LINE__, plsn_timer);

				comm_sleep(2);//sleep, so the system can dispatch search task
				
				//search_plsn_entity
				start_time = osal_get_tick();
				priv->plsn.plsn_num = 0;
				valid_frame_s = 0;
				valid_frame_m = 0;
				priv->plsn.plsn_pls_lock = 0;
				priv->plsn.plsn_pls_timeout = 0;
				result = nim_c3505_search_plsn(dev,data_src,plsn_search_type,&valid_frame_s,&valid_frame_m);
				if (result != SUCCESS)
				{
					nim_c3505_search_plsn_exit(dev);
					SEARCH_PLSN_PRINTF("[%s %d]result=%d\n", __FUNCTION__, __LINE__, result);
					return result;
				}
				end_time = osal_get_tick();
				SEARCH_PLSN_PRINTF("[%s %d]nim_c3505_search_plsn cost %d ms time\n", __FUNCTION__, __LINE__, end_time - start_time);

				if(priv->plsn.plsn_pls_timeout == 1)
				{
					SEARCH_PLSN_PRINTF("[%s %d]search_plsn_fail, plsn_pls_timeout\n", __FUNCTION__, __LINE__);
					priv->plsn.plsn_find = 0;
					priv->plsn.plsn_num = 0;
					priv->plsn.search_plsn_stop = 1;
					w2_sim_end = 1;
					continue;
				}

				if(priv->plsn.plsn_pls_lock == 0)
				{
					priv->plsn.plsn_pls_unlock_cnt = priv->plsn.plsn_pls_unlock_cnt + 1;
					if(priv->plsn.plsn_pls_unlock_cnt >= 2)
					{
						SEARCH_PLSN_PRINTF("[%s %d]search_plsn_fail, plsn_pls_unlock_cnt = %d\n", __FUNCTION__, __LINE__, priv->plsn.plsn_pls_unlock_cnt);
						priv->plsn.plsn_find = 0;
						priv->plsn.plsn_num = 0;
						priv->plsn.search_plsn_stop = 1;
						w2_sim_end = 1;
						continue;
					}
				}

				SEARCH_PLSN_PRINTF("[%s %d]valid_frame_ratio = %d/%d \n",__FUNCTION__, __LINE__,valid_frame_s,valid_frame_m);
				if((5*valid_frame_s < valid_frame_m)&&(plsn_search_type == 0)) //have no pilot & dummy
				{
					plsn_timer_thr = 1;
				}

				if(priv->plsn.plsn_num > 0)
				{	
					SEARCH_PLSN_PRINTF("[%s %d]priv->plsn_mutex=%d\n", __FUNCTION__, __LINE__, priv->plsn_mutex);
					mutex_lock(&priv->plsn_mutex);
					priv->plsn.plsn_find = 1;
					priv->plsn.search_plsn_stop = 1;
					SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.search_plsn_stop=%d\n", __FUNCTION__, __LINE__, priv->plsn.search_plsn_stop);
					mutex_unlock(&priv->plsn_mutex);
					w2_sim_end = 1;

					SEARCH_PLSN_PRINTF("[%s %d]priv->plsn.plsn_num=%d\n", __FUNCTION__, __LINE__, priv->plsn.plsn_num);
					for (i=0; i<priv->plsn.plsn_num; i++)
					{
						SEARCH_PLSN_PRINTF("priv->plsn.plsn_val[%d]=%d\n", i, priv->plsn.plsn_val[i]);
					}
				}
				else if(plsn_timer >= plsn_timer_thr) //capture data 15 times for searching PLSN
				{
					SEARCH_PLSN_PRINTF("[%s %d]search_plsn_fail at max plsn_timer = %d \n", __FUNCTION__, __LINE__, plsn_timer);
					w3_sim_end = 0;
					cr_timer = 0;
					while(1)
					{
						if(priv->ul_status.c3505_chanscan_stop_flag || priv->ul_status.c3505_autoscan_stop_flag)
						{
							SEARCH_PLSN_PRINTF("[%s %d]: exit&flag = 1 \n", __FUNCTION__, __LINE__);
							w3_sim_end = 1;
						}

						if(priv->plsn.search_plsn_force_stop)
						{
							SEARCH_PLSN_PRINTF("[%s %d]: force_stop = 1 \n",__FUNCTION__, __LINE__);
							w3_sim_end = 1;
						}

						if(w3_sim_end == 1)
						{
							SEARCH_PLSN_PRINTF("[%s %d]w3_sim_end=1, exit while\n", __FUNCTION__, __LINE__);
							break;
						}
						//detect whether CR is lock or not
						comm_sleep(10); //waiting 10ms
						cr_timer ++;
						data = 0x00;
						nim_reg_read(dev, R04_STATUS, &data, 1);
						cr_lock = data & 0x08;
						SEARCH_PLSN_PRINTF("R04_STATUS:%x, cr_lock = %x \n",data,cr_lock);
						if(cr_lock == 0x08)
						{
							SEARCH_PLSN_PRINTF("cr_lock at cr_timer = %d \n",cr_timer);
							SEARCH_PLSN_PRINTF("plsn_search_type=%d\n", plsn_search_type);
							if(plsn_search_type == 0)
							{
								plsn_search_type = 4;
								plsn_timer = 0;
								data_src = 1;
								plsn_timer_thr = 5;
								priv->plsn.plsn_search_algo = plsn_search_type;
								priv->plsn.plsn_pls_unlock_cnt = 0;
								SEARCH_PLSN_PRINTF("change plsn_search_type 0 -> 4, clear plsn_timer \n");
							}
							else if((plsn_search_type == 4)&&(0))
							{
								plsn_search_type = 2;
								plsn_timer= 0;
								data_src = 1;
								plsn_timer_thr = 5;
								priv->plsn.plsn_search_algo = plsn_search_type;
								priv->plsn.plsn_pls_unlock_cnt = 0;
								SEARCH_PLSN_PRINTF("change plsn_search_type 4 -> 2, clear plsn_timer \n");
							}
							else
							{
								nim_c3505_search_plsn_exit(dev);
								SEARCH_PLSN_PRINTF("all method fail \n");
								w2_sim_end = 1;
							}
							w3_sim_end = 1;
						}
						else if(cr_timer >= 10) //don't need extra time to waiting for CR to lock 
						{
							nim_c3505_search_plsn_exit(dev);
							SEARCH_PLSN_PRINTF("cr_fail at max cr_timer = %d \n",cr_timer);
							w2_sim_end = 1;
							w3_sim_end = 1;
						}
					}
				}
			}
			w1_sim_end = 1;
		}
		else if(pl_timer >= pl_timer_thr) //10ms*100~=1s , wait for pl lock, time_out is 1s
		{
			nim_c3505_search_plsn_exit(dev);
			SEARCH_PLSN_PRINTF("pl_fail at max pl_timer = %d \n",pl_timer);
			w1_sim_end = 1;
		}
	}

	SEARCH_PLSN_PRINTF("%s leave, search_plsn_stop = %d \n",__FUNCTION__, priv->plsn.search_plsn_stop);
	return SUCCESS;

}

INT32 nim_c3505_plsn_gold_to_root(INT32 plsn_gold)
{
	INT32 plsn_root = 0;
	INT32 i    = 0;
	UINT8 bit_0 = 0;
	UINT8 bit_7 = 0;
	UINT8 bit_18 = 0;
	UINT8 pbuff_x[3] = {0, 0, 0};

	pbuff_x[2] = 64;//0000 0000 0000 0000 01xx xxxx

	while(i < plsn_gold)
	{
		bit_0 = (pbuff_x[2] >> 6) & 0x01;
		bit_7 = (pbuff_x[1] >> 5) & 0x01;
		bit_18 = 0;
		bit_18 = bit_0^bit_7;

		// update
		pbuff_x[2] = (pbuff_x[1] & 0x01)*128 + (pbuff_x[2] >> 1);
		pbuff_x[1] = (pbuff_x[0] & 0x01)*128 + (pbuff_x[1] >> 1);
		pbuff_x[0] = bit_18*128 + (pbuff_x[0] >> 1);

		i ++;
	}

	plsn_root = pbuff_x[0] * 1024 + pbuff_x[1]*4 + ((pbuff_x[2] >> 6) & 0x03);
	
	NIM_PRINTF("plsn_gold = %d , translate to plsn_root is %d\n",(int)plsn_gold,(int)plsn_root);

	return plsn_root;
}

INT32 nim_c3505_plsn_root_to_gold(INT32 plsn_root)
{
	INT32 gold_try = 0;
	INT32 root_try = 0;
	UINT8 bit_0 = 0;
	UINT8 bit_7 = 0;
	UINT8 bit_18 = 0;
	UINT8 pbuff_x[3] = {0,0,0};
	
	pbuff_x[2] = 64;//0000 0000 0000 0000 01xx xxxx

	while(gold_try < 262143) // 2^18 -1
	{
		root_try = 0;
		root_try = pbuff_x[0] * 1024 + pbuff_x[1]*4 + ((pbuff_x[2] >> 6) & 0x03);

		if (plsn_root == root_try)
		{
			NIM_PRINTF("find gold num %d \n", (int)gold_try);
			break;
		}

		bit_0 = (pbuff_x[2] >> 6) & 0x01;
		bit_7 = (pbuff_x[1] >> 5) & 0x01;
		bit_18 = 0;
		bit_18 = bit_0^bit_7;

		// update
		pbuff_x[2] = (pbuff_x[1] & 0x01)*128 + (pbuff_x[2] >> 1);
		pbuff_x[1] = (pbuff_x[0] & 0x01)*128 + (pbuff_x[1] >> 1);
		pbuff_x[0] = bit_18*128 + (pbuff_x[0] >> 1);

		gold_try ++;

	}

	if (gold_try >= 262143) 
	{
		gold_try = -1;
		NIM_PRINTF("the gold num is invalid \n");
	}
	NIM_PRINTF("plsn_root is %d , translate to plsn_gold = %d \n",(int)plsn_root,(int)gold_try);
	return gold_try;
}


