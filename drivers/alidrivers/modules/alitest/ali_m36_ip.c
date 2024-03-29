
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/div64.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/ali_test.h>
#include "ali_m36_ip.h"
#include <ali_soc.h>

#define ALI_SOC_REG_BASE 0x18000000

#if defined(CONFIG_ALI_CHIP_3922)
	#define DMARB_IOBase1 	0x18012000
	#define DMARB_IOBase2 	0x18013000
	#define DMCTRL_IOBase1 	0x18010000
	#define DMCTRL_IOBase2 	0x18011000
#endif

#ifdef CONFIG_ARM
#include <linux/io.h>
#endif

#if 0
#define PRINTK_BW printk
#else
#define PRINTK_BW(x...) do{}while(0)
#endif

//#define ALI_IP_TWO_SPRT_CHAN       //contain two single IP channel
//#define ALI_IP_BW_DRV_CNT              //compute bw in driver

// the wait time must less than 10s ,27s
// 2^32*2.5 ns = 10737418240 ns = 10 s
// 2^40*2.5 ns = 2748779069440 ns = 27 s

struct test_ip_tmp_info pre_bw_info[MAX_IP_IDX];	//store the Byte Gap and Jiff for monitored IP

struct test_ip_get_bw final_bw_info;	//Sotre the final BW and Jiff for User

struct ali_m36_ip_bw_dev g_ali_ip_bw_dev;
struct class *g_ali_ip_bw_class;

struct test_ip_bw_cfg_info ipbw_cfg;	//Store the IP Bw config info

static uint16_t ipbw_monitor_timegap = 20;	//Unit is ms
static uint16_t valid_ip_num = 0;

/* For Total IP BW Test */
static uint32_t mem_cycle_time = 0;	//Memory Cycle CLK in time with the unit of ns
static uint32_t dfi_cycle_time = 0;	//DFI Cycle CLK; unit is ns

static uint64_t total_pre_jiff = 0;
static uint64_t total_post_jiff = 0;

/* For Single IP BW Test */
static uint64_t single_pre_jiff = 0;
static uint64_t single_post_jiff = 0;

/********************************************************
* Function Definition
*********************************************************/

/* Find the minimum jiff in the Arrays */
uint32_t
get_min_jiff_group_idx (uint8_t row)
{
  uint32_t i, min_idx = 0;
  uint32_t tmp = 0;

  tmp = pre_bw_info[row].ip_bw_jiff[0];

  for (i = 1; i < MAX_BW_GROUP; i++)
  {
  	if (tmp > pre_bw_info[row].ip_bw_jiff[i])
	{
	  min_idx = i;

	  tmp = pre_bw_info[row].ip_bw_jiff[i];
	}
  }

  return min_idx;

}


/* Find Line Index with the maximun jiff in the Arrays */
uint32_t
get_max_jiff_group_idx (uint8_t row)
{
  uint32_t i, max_idx = 0;
  uint32_t tmp = 0;

  tmp = pre_bw_info[row].ip_bw_jiff[0];

  for (i = 1; i < MAX_BW_GROUP; i++)
  {
    if (tmp < pre_bw_info[row].ip_bw_jiff[i])
		{
	  	max_idx = i;
	  	tmp = pre_bw_info[row].ip_bw_jiff[i];
		}
  }

  return max_idx;

}

/* Rearrange the  Arrays for big jiffie value to small jiff value */
uint32_t
resort_tmp_bw_array (void)
{
  uint8_t i = 0, j = 0, k = 0;
  uint32_t tmp_total_value = 0;
  uint32_t tmp_real_value = 0;
  uint32_t tmp_bw_jiff = 0;
  uint32_t tmp_bw_time_gap = 0;

#ifdef ALI_IP_TWO_SPRT_CHAN
  uint32_t tmp_total_value_0 = 0, tmp_total_value_1 = 0;
  uint32_t tmp_real_value_0 = 0, tmp_real_value_1 = 0;
#endif

  for (i = 0; i < MAX_IP_IDX; i++)
  {
    for (j = 0; j < MAX_BW_GROUP - 1; j++)
	{
	  	for (k = j + 1; k < MAX_BW_GROUP; k++)
	  	{
	      if (pre_bw_info[i].ip_bw_jiff[j] < pre_bw_info[i].ip_bw_jiff[k])
		{
		  tmp_total_value = pre_bw_info[i].ip_total_value[k];
		  tmp_real_value = pre_bw_info[i].ip_real_value[k];

#ifdef ALI_IP_TWO_SPRT_CHAN
		  tmp_total_value_0 = pre_bw_info[i].ip_total_value_0[k];
		  tmp_total_value_1 = pre_bw_info[i].ip_total_value_1[k];
		  tmp_real_value_0 = pre_bw_info[i].ip_real_value_0[k];
		  tmp_real_value_1 = pre_bw_info[i].ip_real_value_1[k];
#endif
		  tmp_bw_jiff = pre_bw_info[i].ip_bw_jiff[k];
		  tmp_bw_time_gap = pre_bw_info[i].ip_bw_time_gap[k];


		  pre_bw_info[i].ip_total_value[k] =
		    pre_bw_info[i].ip_total_value[j];
		  pre_bw_info[i].ip_real_value[k] =
		    pre_bw_info[i].ip_real_value[j];

#ifdef ALI_IP_TWO_SPRT_CHAN
		  pre_bw_info[i].ip_total_value_0[k] =
		    pre_bw_info[i].ip_total_value_0[j];
		  pre_bw_info[i].ip_total_value_1[k] =
		    pre_bw_info[i].ip_total_value_1[j];
		  pre_bw_info[i].ip_real_value_0[k] =
		    pre_bw_info[i].ip_real_value_0[j];
		  pre_bw_info[i].ip_real_value_1[k] =
		    pre_bw_info[i].ip_real_value_1[j];
#endif
		  pre_bw_info[i].ip_bw_jiff[k] = pre_bw_info[i].ip_bw_jiff[j];
		  pre_bw_info[i].ip_bw_time_gap[k] =
		    pre_bw_info[i].ip_bw_time_gap[j];



		  pre_bw_info[i].ip_total_value[j] = tmp_total_value;
		  pre_bw_info[i].ip_real_value[j] = tmp_real_value;

#ifdef ALI_IP_TWO_SPRT_CHAN
		  pre_bw_info[i].ip_total_value_0[j] = tmp_total_value_0;
		  pre_bw_info[i].ip_total_value_1[j] = tmp_total_value_1;
		  pre_bw_info[i].ip_real_value_0[j] = tmp_real_value_0;
		  pre_bw_info[i].ip_real_value_1[j] = tmp_real_value_1;
#endif
		  pre_bw_info[i].ip_bw_jiff[j] = tmp_bw_jiff;
		  pre_bw_info[i].ip_bw_time_gap[j] = tmp_bw_time_gap;

		}
	    }
	}

    }

  return 0;

}

/*
Select the information via CFG_MEM_BIU2(1800_1030) [23:20]
0000: Page miss count
0001: Total cycle count
0010: Valid Command cycle count
0011: Valid Read and Write cycle count
0100: SEQ monitor master latency out count
0101: IMB0 monitor master latency out count
0110: IMB1 monitor master latency out count
0111: IMB2 monitor master latency out count
1000: IMB3 monitor master latency out count
1001: IMB4 monitor master latency out count
1010: IMB5 monitor master latency out count
*/
//Choose the status which we want to monitor
static uint32_t
total_ip_set_monitor_type (uint32_t idx)
{
  	uint32_t value = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;

  	if (idx > 10)
  	{
      return -1;
  	}
	#if defined(CONFIG_ALI_CHIP_3921)
  		value = ioread32(ipbw->base + 0x1030);	
  		value &= (~(0xf << 20));
  		value |= (idx << 20);
		iowrite32 (value, ipbw->base + 0x1030);
	#elif defined(CONFIG_ALI_CHIP_3922)
		// 1
		value = ioread32(ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
		value &= (~(0x07 << 12));
		value |= (idx<<12);
		iowrite32(value, ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
		
		// 2
		value = ioread32(ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
		value &= (~(0x07 << 12));
		value |= (idx<<12);
		iowrite32(value, ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
	#else
  		value = ioread32 (ipbw->base + 0x1030);
  		value &= (~(0x7<<21));
		value |= (idx<<21);
		iowrite32 (value, ipbw->base + 0x1030);
	#endif

 	return 0;
}

//Enable the status monitor counter
static void
total_ip_start_monitor (void)
{
//	uint16_t value = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;
	
	#if defined(CONFIG_ALI_CHIP_3921)
  		value = ioread8 (ipbw->base + 0x1005);
  		value |= (1 << 7);
  		iowrite8 (value, ipbw->base + 0x1005);
	#elif defined(CONFIG_ALI_CHIP_3922)
		value = ioread16(ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
		value |= (1 << 15);	
		iowrite16(value, ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
		
		value = ioread16(ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
		value |= (1 << 15);
		iowrite16(value, ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
	#endif
}

//Disable the status monitor counter
static void
total_ip_stop_monitor (void)
{
//	uint16_t value = 0;
	struct ali_m36_ip_bw_dev  *ipbw ;
	ipbw = &g_ali_ip_bw_dev;
	
	#if defined(CONFIG_ALI_CHIP_3921)
	  	value = ioread8 (ipbw->base + 0x1005);
  		value &= (~(1 << 7));
  		iowrite8 (value, ipbw->base + 0x1005);
	#elif defined(CONFIG_ALI_CHIP_3922)
		value = ioread16(ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
		value &= (~(1 << 15));
		iowrite16(value, ipbw->pMEM_DMCTRL_IOBase1 + 0x04);

		value = ioread16(ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
		value &= (~(1 << 15));
		iowrite16(value, ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
	#endif
}


/*Get Memory cycle clock in time;
* The unit is ns
*/
// MEM_CLK = DPLL_FOUT / 4
// DFI_CLK = DPLL_FOUT / 2
// tDFI_CLK = 1000 / DFI_CLK
// tMEM_CLK = 1000 / MEM_CLK

static void
total_ip_get_mem_cycle_time (void)
{
  uint32_t mem_clk;
//  uint32_t tmem_clk;
//  uint32_t dpll_clk;
  uint32_t mask;

#if defined(CONFIG_ALI_CHIP_3921)
  mask = ioread8 (g_ali_ip_bw_dev.base + 0x70);
  mask = (mask & 0xe0) >> 5;

  switch (mask)
    {
    case 0:
      dpll_clk = 264;
      break;
    case 1:
      dpll_clk = 330;		//660/688 -- 83
      break;
    case 2:
      dpll_clk = 396;		//800 -- 100
      break;
    case 3:
      dpll_clk = 33;
      break;
    case 4:
      dpll_clk = 528;		//1066 -- 132
      break;
    case 5:
      dpll_clk = 660;		//1333 -- 166
      break;
    case 6:
      dpll_clk = 792;		// 1600 -- 200
      break;
    case 7:
      dpll_clk = 924;
      break;
    default:
      PRINTK_BW ("\n Can't get the Memory CLK \n");
      break;
    }
  mem_clk = dpll_clk / 4;
  tmem_clk = 1000 / mem_clk;
  mem_cycle_time = tmem_clk;

#elif defined(CONFIG_ALI_CHIP_3922)

  mask = ioread8(g_ali_ip_bw_dev.base + 0x70);
  mask = (mask & 0xf0) >> 4;
  
  dpll_clk = 0;
  switch (mask)
    {
    case 0:
      dpll_clk = 660; // 660MHz (IMB_CLK_166MHz //DDR3-1333 Mbps)
      break;
    case 1:
      dpll_clk = 726; // 726MHz (IMB_CLK_182MHz //DDR3-1456 Mbps)
      break;
    case 2:
      dpll_clk = 792; // 792MHz (IMB_CLK_198MHz //DDR3/4-1600 Mbps)
      break;
    case 3:
      dpll_clk = 858; // 858MHz (IMB_CLK_216MHz //DDR3/4-1728 Mbps) (default)
      break;
    case 4:
      dpll_clk = 924; // 924MHz (IMB_CLK_232MHz //DDR3/4-1866 Mbps)
      break;
    case 5:
      dpll_clk = 990; // 990MHz (IMB_CLK_248MHz //DDR3/4-1984 Mbps)
      break;
    case 6:
      dpll_clk = 1054; // 1056MHz(IMB_CLK_264MHz //DDR3/4-2133 Mbps)
      break;
    case 7:
      dpll_clk = 1122; // 1122MHz(IMB_CLK_280MHz //DDR4-2240 Mbps) (not support)
      break;
    case 8:
      dpll_clk = 1188; // 1188MHz(IMB_CLK_297MHz //DDR4-2400 Mbps) (not support)
      break;
    case 9:
      dpll_clk = 1254; // 1254MHz (no guaranteed)
      break;
    case 10:
      dpll_clk = 1320; // 1320MHz (no guaranteed)
      break;
    case 15:
      dpll_clk = 33;   // 33MHz (DDR test)
      break;
    default:
      PRINTK_BW ("\n Can't get the Memory CLK \n");
      break;
    }
  if (dpll_clk)
  {
    mem_clk = dpll_clk / 4;
    tmem_clk = 1000 / mem_clk;
    mem_cycle_time = tmem_clk;
  }	
#else

  uint32_t low = 0;
  uint32_t high = 0;

  //The following calculation is only for 3701C   

  mem_clk = ioread32 (g_ali_ip_bw_dev.base + 0x70);	//__REG32ALI(ALI_SOC_REG_BASE+0x70);
  mask = 0;
  mask = ((1 << 5) | (1 << 6));
  low = ((mem_clk & mask) >> 5);

  mem_clk = ioread32 (g_ali_ip_bw_dev.base + 0x74);	//__REG32ALI(ALI_SOC_REG_BASE+0x74);
  mask = 0;
  mask = (1 << 17);
  high = ((mem_clk & mask) >> 15);	//0x74H, bit17 is the bit2 of MEM CLK

  mem_clk = high | low;

  switch (mem_clk)
  {
    case 0:
      mem_clk = 1000 / 200;	//200MHz;5ns
      break;
    case 1:
      mem_clk = 1000 / 133;	//133MHz
      break;
    case 2:
      mem_clk = 1000 / 100;	//100MHz
      break;
    case 3:
      mem_clk = 1000 / 2.06;	//2.06MHz
      break;
    case 4:
      mem_clk = 1000 / 166.6;	//166.6MHz
      break;
    case 5:
      mem_clk = 1000 / 83.3;	//83.3MHz
      break;
    default:
      //ALI_IPBW_DEBUG("\n Can't get the Memory CLK \n");
      PRINTK_BW ("\n Can't get the Memory CLK \n");
      break;
  }
  mem_cycle_time = mem_clk;
#endif

}

//Get the DFI Cycle clock in ns
static void
total_ip_get_dfi_cycle_time (void)
{
  dfi_cycle_time = (10 * mem_cycle_time) / 2;	//ns
}


/* To Calculate the Band Width;
* Param In:
*    uint32_t total_cycle_num ://total cycle number
*    uint32_t valid_cycle_num : //valid cycle number of read/writes
* RET Value:
* 	 the Unit is MBps
*/
uint32_t
total_ip_calc_bw (uint32_t total_cycle_num, uint32_t valid_cycle_num)
{

  uint32_t bw = 0;
  uint32_t MAX_32 = 0xFFFFFFFF;
  uint32_t FACTOR = 0;
  uint32_t NEW_FACT = 0;
  uint32_t i = 0;
  uint32_t times = 0;
  uint32_t time_tag;

  time_tag = total_cycle_num * dfi_cycle_time / 10000000;

  //bw = (valid_cycle_num*16*1000*10)/(total_cycle_num*dfi_cycle_time);   //MBps
  //bw = (valid_cycle_num)/(time_tag);   //MBps

  FACTOR = 16 * 1000 * 10 / dfi_cycle_time;

  times = MAX_32 / valid_cycle_num;	//avoid the data overflow

  if (times > FACTOR)
  {
      //  bw = (valid_cycle_num*16*1000)/(total_cycle_num*dfi_cycle_time);   //MBps        
      bw = (valid_cycle_num * FACTOR) / (total_cycle_num);	//MBps
  }
  else
  {
    NEW_FACT = FACTOR;
    i = 1;
    do
		{
	  	i++;
	  	NEW_FACT = FACTOR / i;

	  	if (times > NEW_FACT)
	    	break;
		}
    while (1);
    bw = i * ((valid_cycle_num * NEW_FACT) / (total_cycle_num));	//MBps
  }

  return bw;
}


/*********************************************************************
* To Get the Bandwidth of total IP
*********************************************************************/
uint32_t
get_total_ip_bw (void)
{
  uint32_t time_tag = 0;
  uint32_t t = 0;

//  uint32_t data = 0;

  uint32_t total_cycle_cnt_0;
  uint32_t valid_cycle_cnt_0;
	uint32_t valid_read_cycle_cnt_0;
	uint32_t valid_write_cycle_cnt_0;
	
  uint32_t total_cycle_cnt_1;
  uint32_t valid_cycle_cnt_1;
	uint32_t valid_read_cycle_cnt_1;
	uint32_t valid_write_cycle_cnt_1;

  struct ali_m36_ip_bw_dev *ipbw;
  ipbw = &g_ali_ip_bw_dev;

  if (mutex_lock_interruptible (&ipbw->data_wr_mutex))
  {
      PRINTK_BW ("%s, %d\n", __FUNCTION__, __LINE__);
      return (-ERESTARTSYS);
  }

  if (0 == mem_cycle_time)
  {
      total_ip_get_mem_cycle_time ();
  }

  if (0 == dfi_cycle_time)
  {
     total_ip_get_dfi_cycle_time ();
  }

	
	/***** Stop totla IP band-width Monitor to reset counter value *******/
  total_ip_stop_monitor ();

	/**************Start totla IP band-width Monitor**************/
#if defined(CONFIG_ALI_CHIP_3921)
  data = ioread8 (ipbw->base + 0x1006);
  data = data & (~(1 << 4));
  iowrite8 (data, ipbw->base + 0x1006); 
  
#elif defined(CONFIG_ALI_CHIP_3922)
  data = ioread32 (ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
  data = data & (~(1 << 16));
  iowrite32 (data, ipbw->pMEM_DMCTRL_IOBase1 + 0x04);

  data = ioread32 (ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
  data = data & (~(1 << 16));
  iowrite32 (data, ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
#endif
  total_ip_start_monitor ();

	/**************Set the start time**************/
  total_pre_jiff = jiffies;

  msleep (ipbw_monitor_timegap);

	/*****Stop totla IP band-width Monitor*******/
  total_ip_stop_monitor ();

	/*****Get the time tag to monitor IP BW	*****/
  total_post_jiff = jiffies;
#if defined(CONFIG_ALI_CHIP_3921)
  data = ioread8 (ipbw->base + 0x1006);	
  data = data | (1 << 4);
  iowrite8 (data, ipbw->base + 0x1006);	 
#elif defined(CONFIG_ALI_CHIP_3922)
  data = ioread32 (ipbw->pMEM_DMCTRL_IOBase1 + 0x04);
  data = data | (1 << 16);
  writeb (data, ipbw->pMEM_DMCTRL_IOBase1 + 0x04);

  data = ioread32 (ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
  data = data | (1 << 16);
  writeb (data, ipbw->pMEM_DMCTRL_IOBase2 + 0x04);
#endif
  if (total_pre_jiff == 0)
    {
      mutex_unlock (&ipbw->data_wr_mutex);

      total_pre_jiff = jiffies;

      return 0;
    }


  total_cycle_cnt_0 = 0;
  valid_cycle_cnt_0 = 0;
  valid_read_cycle_cnt_0 = 0;
  valid_write_cycle_cnt_0 = 0;

  total_cycle_cnt_1 = 0;
  valid_cycle_cnt_1 = 0;
  valid_read_cycle_cnt_1 = 0;
  valid_write_cycle_cnt_1 = 0;

	/*******Get the total cycle count***************/
  total_ip_set_monitor_type (1);
	
#if defined(CONFIG_ALI_CHIP_3921)
  //Get the first channel
  data = ioread32 (ipbw->base + 0x1060);	
  data = data & (~(1 << 30));
  iowrite32 (data, ipbw->base + 0x1060);	

  total_cycle_cnt_0 = ioread32 (ipbw->base + 0x1090);	

  //Get the second group 
  data = ioread32 (ipbw->base + 0x1060);	
  data = data | (1 << 30);
  iowrite32 (data, ipbw->base + 0x1060);	 

  total_cycle_cnt_1 = ioread32 (ipbw->base + 0x1090);	
  
#elif defined(CONFIG_ALI_CHIP_3922)
	total_cycle_cnt_0 = ioread32(ipbw->pMEM_DMCTRL_IOBase1 + 0x68);
	total_cycle_cnt_1 = ioread32(ipbw->pMEM_DMCTRL_IOBase2 + 0x68);
#endif

	/**************Get the valid cycle count***************/
#if defined(CONFIG_ALI_CHIP_3921)
  //Get the first channel
  total_ip_set_monitor_type (3);
  data = ioread32 (ipbw->base + 0x1060);	
  data = data & (~(1 << 30));
  iowrite32 (data, ipbw->base + 0x1060);	 

  valid_cycle_cnt_0 = ioread32 (ipbw->base + 0x1090);	//Get the first group

  //Get the second group 
  data = ioread32 (ipbw->base + 0x1060);	
  data = data | (1 << 30);
  iowrite32 (data, ipbw->base + 0x1060);	 

  valid_cycle_cnt_1 = ioread32 (ipbw->base + 0x1090);	 //Get the second group
  
#elif defined(CONFIG_ALI_CHIP_3922)
	total_ip_set_monitor_type (2);
	valid_write_cycle_cnt_0 = ioread32(ipbw->pMEM_DMCTRL_IOBase1 + 0x68);
	valid_write_cycle_cnt_1 = ioread32(ipbw->pMEM_DMCTRL_IOBase2 + 0x68);	
	total_ip_set_monitor_type (3);
	valid_read_cycle_cnt_0 = ioread32(ipbw->pMEM_DMCTRL_IOBase1 + 0x68);
	valid_read_cycle_cnt_1 = ioread32(ipbw->pMEM_DMCTRL_IOBase2 + 0x68);	
	valid_cycle_cnt_0 = valid_write_cycle_cnt_0 + valid_read_cycle_cnt_0;
	valid_cycle_cnt_1 = valid_write_cycle_cnt_1 + valid_read_cycle_cnt_1;
#endif

  time_tag = (total_post_jiff - total_pre_jiff) * (1000 / HZ);

	/****Calculate the Band Width******/

  t = (uint32_t) get_min_jiff_group_idx (0);
  {
    pre_bw_info[0].ip_total_value[t] =
      (total_cycle_cnt_0 + total_cycle_cnt_1) / 2;

    pre_bw_info[0].ip_total_value_0[t] = total_cycle_cnt_0;
    pre_bw_info[0].ip_total_value_1[t] = total_cycle_cnt_1;

    pre_bw_info[0].ip_real_value[t] = valid_cycle_cnt_0 + valid_cycle_cnt_1;

    pre_bw_info[0].ip_real_value_0[t] = valid_cycle_cnt_0;
    pre_bw_info[0].ip_real_value_1[t] = valid_cycle_cnt_1;

    pre_bw_info[0].ip_bw_time_gap[t] = time_tag;
    pre_bw_info[0].ip_bw_jiff[t] = (uint32_t) (total_post_jiff & 0xFFFFFFFF);
  }

  mutex_unlock (&ipbw->data_wr_mutex);

  return 0;
}

/**********Enable BW Detect Clock Gate **************/
static void
single_ip_set_clock (void)
{
//  	uint32_t value;
		struct ali_m36_ip_bw_dev	*ipbw ;
		ipbw = &g_ali_ip_bw_dev;
		
#if defined(CONFIG_ALI_CHIP_3921)
		value = ioread16(ipbw->base + 0x1200);
		value |=(1<<14);
		iowrite16(value, ipbw->base + 0x1200);
#elif defined(CONFIG_ALI_CHIP_3922)
		// 1
		value = ioread16(ipbw->pMEM_DMARB_IOBase1 + 0x200);	
		value |=(1<<14);
		iowrite16(value, ipbw->pMEM_DMARB_IOBase1 + 0x200);

		//PRINTK_BW ("Set DMARB_IOBase1+0x200[0x%08x] to enable \n ", value);
		
		value = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x240);
		value &= (0xFFC << 20);
		iowrite32(value, ipbw->pMEM_DMARB_IOBase1 + 0x240);
		// 2
		value = ioread16(ipbw->pMEM_DMARB_IOBase2 + 0x200);
		value |=(1<<14);
		iowrite16(value, ipbw->pMEM_DMARB_IOBase2 + 0x200);
	
		value = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x240);
		value &= (0xFFC << 20);
		iowrite32(value, ipbw->pMEM_DMARB_IOBase2 + 0x240);
	
#endif

}


/**********Start Single IP detect ******************/
static void
single_ip_start_detect (void)
{
//  uint32_t value;
	struct ali_m36_ip_bw_dev  *ipbw ;
	ipbw = &g_ali_ip_bw_dev;
	
#if defined(CONFIG_ALI_CHIP_3921)
	value = ioread16(ipbw->base + 0x1200);
	value |=(1<<13);
	value &=(~(1<<12));
	iowrite16(value, ipbw->base + 0x1200);
#elif defined(CONFIG_ALI_CHIP_3922)
	  value = ioread16(ipbw->pMEM_DMARB_IOBase1 + 0x200);
	  value |=(1<<13);
	  value &=(~(1<<12));
	  iowrite16(value, ipbw->pMEM_DMARB_IOBase1 + 0x200);
		
		//PRINTK_BW ("Set DMARB_IOBase1+0x200[0x%08x] to start detect\n ", value);

	  value = ioread16(ipbw->pMEM_DMARB_IOBase2 + 0x200);
	  value |=(1<<13);
	  value &=(~(1<<12));
	  iowrite16(value, ipbw->pMEM_DMARB_IOBase2 + 0x200);	
#endif

}


/**********End Single IP detect ******************/
static void
single_ip_stop_detect (void)
{
//  uint32_t value;
	struct ali_m36_ip_bw_dev  *ipbw ;
	ipbw = &g_ali_ip_bw_dev;
#if defined(CONFIG_ALI_CHIP_3921)
		value = ioread16(ipbw->base + 0x1200);
		value |=(1<<12);
		value &=(~(1<<13));
		iowrite16(value, ipbw->base + 0x1200);
#elif defined(CONFIG_ALI_CHIP_3922)

		value = ioread16(ipbw->pMEM_DMARB_IOBase1 + 0x200);
		value |=(1<<12);
		value &=(~(1<<13));
		iowrite16(value, ipbw->pMEM_DMARB_IOBase1 + 0x200);
		
		//PRINTK_BW ("Set DMARB_IOBase1+0x200[0x%04x] to stop detect\n ", value);
	
		value = ioread16(ipbw->pMEM_DMARB_IOBase2 + 0x200);
		value |=(1<<12);
		value &=(~(1<<13));
		iowrite16(value, ipbw->pMEM_DMARB_IOBase2 + 0x200);
#endif

}



/************************************************************************************
* 0x18001200[19:16]:
* Determine the IPs in different IP Groups to Count 
* 0000 ~ 1000 ---> 0 ~ 33
* Input Parameter,
*	uint8_t idx:  IP Index , 0 ~ 33
*************************************************************************************/
static void
single_ip_select_master (uint8_t idx)
{
//  uint32_t group_idx = 0;
//  uint32_t value;
	struct ali_m36_ip_bw_dev  *ipbw ;
	ipbw = &g_ali_ip_bw_dev;
	
#if defined(CONFIG_ALI_CHIP_3921)
		if(idx <= 33) 
		{
			group_idx = idx/4; // 0 ~ 8
			value = ioread32(g_ali_ip_bw_dev.base + 0x1200);
			value &= (~((0xF)<<16));
			iowrite32(value, g_ali_ip_bw_dev.base + 0x1200);
			
			value = ioread32(g_ali_ip_bw_dev.base + 0x1200);
			value |= (group_idx<<16);
			iowrite32(value, g_ali_ip_bw_dev.base + 0x1200);
			
		}
		else
		{
			printk("\n %s():Can't Select the IP[%d] \n", __FUNCTION__,idx);
		}
			
#elif defined(CONFIG_ALI_CHIP_3922)
		if (idx <=39)
		{
				group_idx = idx/4; // 0 ~ 8
				
				value = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x200);
				value |= (0x0F << 16);
				value &= ((~(0x0F << 16))|(group_idx<<16));
				iowrite32(value, ipbw->pMEM_DMARB_IOBase1 + 0x200);

				value = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x200);
				value |= (0x0F << 16);
				value &= ((~(0x0F << 16))|(group_idx<<16));
				iowrite32(value, ipbw->pMEM_DMARB_IOBase2 + 0x200);
		}
  	else
    {
      printk ("\n %s():Can't Select the IP[%d] \n", __FUNCTION__, idx);
    }
#endif

}


#if 0
/***************Get high part of read count ******************/
static uint32_t
single_ip_get_read_count_high (uint8_t index, uint8_t iochanel)
{
  uint32_t data = 0;
//  uint8_t group_idx = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;
		
#if defined(CONFIG_ALI_CHIP_3921)
	
	if(index > 33)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
			
	data = ioread32(ipbw->base + 0x1214);
		
	group_idx = index%4;
	switch(group_idx)
	{
		case 0:
			data	= data & 0xFF;
			break;
		case 1:
			data	= (data>>8) & 0xFF;
			break;
		case 2:
			data	= (data>>16) & 0xFF;
			break;
		case 3:
			data	= (data>>24) & 0xFF;
			break;
		default:
			printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
			break;
	}
#elif defined(CONFIG_ALI_CHIP_3922)
	if(index > 39)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
	if (iochanel==0)
		data = ioread32(ipbw->pMEM_DMARB_IOBase1+ 0x214);
	else
		data = ioread32(ipbw->pMEM_DMARB_IOBase2+ 0x214);
	
	group_idx = index%4;
	switch(group_idx)
	{
		case 0:
			data	= data & 0xFF;
			break;
		case 1:
			data	= (data>>8) & 0xFF;
			break;
		case 2:
			data	= (data>>16) & 0xFF;
			break;
		case 3:
			data	= (data>>24) & 0xFF;
			break;
		default:
			printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
			break;
	}
#endif

  return data;
}

/************Get low part of Read Count ****************/
static uint32_t
single_ip_get_read_count_low (uint8_t index, uint8_t iochanel)
{
  uint32_t data = 0;
//  uint8_t group_idx = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;
	
#if defined(CONFIG_ALI_CHIP_3921)
	if(index>33)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
	
	group_idx = index%4;
	switch(group_idx)
	{
			case 0:
				data = ioread32(ipbw->base + 0x1218);
				break;
			case 1:
				data = ioread32(ipbw->base + 0x121c);
				break;
			case 2:
				data = ioread32(ipbw->base + 0x1220);
				break;
			case 3:
				data = ioread32(ipbw->base + 0x1224);
				break;
			default:
				printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
				break;
	}
#elif defined(CONFIG_ALI_CHIP_3922)
	if(index>39)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
	
	group_idx = index%4;
	switch(group_idx)
	{
		case 0:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x218);
				PRINTK_BW("IP low read data[DMARB_IOBase1+0x218]=[%u],index=%d\n",	data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x218);
			break;
		case 1:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x21c);
				PRINTK_BW("IP low read data[DMARB_IOBase1+0x21c]=[%u],index=%d\n",	data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x21c);
			break;
		case 2:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x220);
				PRINTK_BW("IP low read data[DMARB_IOBase1+0x220]=[%u],index=%d\n",	data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x220);
			break;
		case 3:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x224);
				PRINTK_BW("IP low read data[DMARB_IOBase1+0x224]=[%u],index=%d\n",	data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x224);
			break;
		default:
			printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
			break;
	}
	
#endif

  return data;
}


/************Get High part of Write Count ****************/
static uint32_t
single_ip_get_write_count_high (uint8_t index, uint8_t iochanel)
{
  uint32_t data = 0;
//  uint8_t group_idx = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;
			
#if defined(CONFIG_ALI_CHIP_3921)
	if(index>33)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
	
	group_idx = index%4;
	data = ioread32(ipbw->base + 0x1228);
	switch(group_idx)
	{
					case 0:
							data	= data & 0xFF;
							break;
					case 1:
							data	= (data>>8) & 0xFF;
							break;
					case 2:
							data	= (data>>16) & 0xFF;
							break;
					case 3:
							data	= (data>>24) & 0xFF;
							break;
					default:
							printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
							break;
	
	}
#elif defined(CONFIG_ALI_CHIP_3922)
	if(index>39)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
	
	group_idx = index%4;
	if (iochanel==0)
		data = ioread32(ipbw->pMEM_DMARB_IOBase1+ 0x228);
	else
		data = ioread32(ipbw->pMEM_DMARB_IOBase2+ 0x228);
	
	switch(group_idx)
	{
		case 0:
			data	= data & 0xFF;
			break;
		case 1:
			data	= (data>>8) & 0xFF;
			break;
		case 2:
			data	= (data>>16) & 0xFF;
			break;
		case 3:
			data	= (data>>24) & 0xFF;
			break;
		default:
			printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
			break;
	}
#endif

  return data;
}


/************Get low part of Write Count ****************/
static uint32_t
single_ip_get_write_count_low (uint8_t index, uint8_t iochanel)
{
  uint32_t data = 0;
//  uint8_t group_idx = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;
					
#if defined(CONFIG_ALI_CHIP_3921)
	
	if(index>33)
	{
					printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
					return 0;
	}
	group_idx = index%4;
	switch(group_idx)
	{
			case 0:
				data = ioread32(ipbw->base + 0x122c);
					break;
			case 1:
					data = ioread32(ipbw->base + 0x1230);
					break;
			case 2:
					data = ioread32(ipbw->base + 0x1234);
					break;
			case 3:
					data = ioread32(ipbw->base + 0x1238);
					break;
			default:
					printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
					break;
	}
#elif defined(CONFIG_ALI_CHIP_3922)
	if(index>39)
	{
			printk("\n %s():Can't calculate the IP[%d] \n", __FUNCTION__,index);
			return 0;
	}
	
	group_idx = index%4;
	switch(group_idx)
	{
		case 0:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x22c);
				PRINTK_BW("IP low write data[DMARB_IOBase1+0x22c]=[%u],index=%d\n", data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x22c);
			break;
		case 1:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x230);
				PRINTK_BW("IP low write data[DMARB_IOBase1+0x230]=[%u],index=%d\n", data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x230);
			break;
		case 2:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x234);
				PRINTK_BW("IP low write data[DMARB_IOBase1+0x234]=[%u],index=%d\n", data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x234);
			break;
		case 3:
			if (iochanel==0)
			{
				data = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x238);
				PRINTK_BW("IP low write data[DMARB_IOBase1+0x238]=[%u],index=%d\n", data, index);
			}
			else
				data = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x238);
			break;
		default:
			printk("\n %s():IP Group_Idx[%d] \n", __FUNCTION__,group_idx);
			break;
	}
#endif

  return data;
}
#endif

/**************Get Detect Time for Single IP******************/
// only for mode 1 or mode 2
#if 0
static uint32_t
single_ip_get_detect_time (uint8_t iochanel)
{
  uint32_t time_ret = 0;
	struct ali_m36_ip_bw_dev	*ipbw ;
	ipbw = &g_ali_ip_bw_dev;

#if defined(CONFIG_ALI_CHIP_3922)
	if (iochanel==0)
		time_ret = ioread32(ipbw->pMEM_DMARB_IOBase1 + 0x210);
	else
		time_ret = ioread32(ipbw->pMEM_DMARB_IOBase2 + 0x210);
#endif
	
	return time_ret;
}
#endif

#if 0
/**************Get Read Count for Single IP******************/
static uint32_t
single_ip_get_read_count (uint8_t index, uint8_t iochanel)
{
  uint32_t high_ret = 0;
  uint32_t low_ret = 0;

  high_ret = single_ip_get_read_count_high (index,iochanel);
  if (high_ret > 0)
  {
    PRINTK_BW ("\n******  Read high count = 0x%x ******\n", high_ret);
  }

  low_ret = single_ip_get_read_count_low (index,iochanel);

  //printk("\n******  Read low count = 0x%x ******\n", low_ret);

  /* we can lessen test time and so that can just read low 32bit data */
  /* Because the high 8bit that time have no data */

  //ret = (((high_ret&0xFF)<<32)|low_ret); 
  //return (ret&0xFFFFFFFFFF);                         

  return low_ret;

}

/****************Get Write Count for Single IP******************/
static uint32_t
single_ip_get_write_count (uint8_t index,uint8_t iochanel)
{
  uint32_t high_ret = 0;
  uint32_t low_ret = 0;

  high_ret = single_ip_get_write_count_high (index, iochanel);
  if (high_ret > 0)
  {
      PRINTK_BW ("\n****** Write high count = 0x%x \n", high_ret);
  }

  low_ret = single_ip_get_write_count_low (index, iochanel);

  /* we can lessen test time and so that can just read low 32bit data */
  /* Because the high 8bit that time have no data */

  //ret = (((high_ret&0xFF)<<32) | low_ret);
  //return (ret&0xFFFFFFFFFF);

  return low_ret;

}
#endif




/*****************************************************************
* Function: Calculate Band_Width in the time
* Parameter(IN):
* 		bw   : The Byte number of Read/Write; in Byte
* 		time : monitor time tag in ms
*****************************************************************/
uint32_t
single_ip_count_bw (uint32_t bw, uint32_t time)
{
  uint32_t bw_value;

  if (time > 0)
    {
      bw_value = (bw) / time;
    }
  else
    {
      bw_value = 0;
    }

  return (bw_value);
}




extern __u32 read_CP0_config2(void);


/*****************************************************************
* Function: check L2 cache has been implemented or not
* return(BOOL):
* 		true : L2 cache has been implemented
* 		false : L2 cache has been disabled
*****************************************************************/
__u32 
check_L2_cache_enable(void)
{
#ifdef CONFIG_ARM	// only support 3921/3921B
	uint32_t data_tmp = 0;
	void __iomem	*base;
	base = ioremap(0x1bf02000, 0x200);
	data_tmp = readl(base + 0x100);		// register 0x1bf02100
	if(( (data_tmp >> 0) & 0x1 ) == 0x0)	//L2 cache not implemented on processor
		return false;	
	else
		return true;
#else
	unsigned int  rst = 0xffff;

	rst = read_CP0_config2();
	PRINTK_BW("L2 test :  rst = %x\n", (int)rst);

	if( ((rst >> 12) & 0x1)  == 0x1)	//bit12 is set, when L2 cache bypass mode is enable
		return false;
	else
		return true;
#endif
}





/************************************************************************
* Function: Used to calculate the Single IP BW
* Parameter(In):
* 		idx: The IP IDX to be monitored
************************************************************************/
int32_t
get_single_ip_bw (void)
{
  uint32_t time_tag = 0;
  uint8_t temp_idx = 0, idx;
  uint8_t i;
  uint32_t t = 0, row = 0;

//  uint32_t read_count_0 = 0;
//  uint32_t write_count_0 = 0;
  uint32_t valid_count_0 = 0;

//  uint32_t read_count_1 = 0;
//  uint32_t write_count_1 = 0;
  uint32_t valid_count_1 = 0;

  struct ali_m36_ip_bw_dev *ipbw;
  ipbw = &g_ali_ip_bw_dev;

  if (mutex_lock_interruptible (&ipbw->data_wr_mutex))
  {
      PRINTK_BW ("%s, %d\n", __FUNCTION__, __LINE__);
      return (-ERESTARTSYS);
  }

	/*******Enable Clock Gating******************/
  single_ip_set_clock ();

	/********Start count IP*********************/
  single_ip_start_detect ();
  single_pre_jiff = jiffies;

	/*******Monitor Time tag *******************/
  msleep (ipbw_monitor_timegap);
	
  /********Stop count IP*********************/
 	single_ip_stop_detect ();

  single_post_jiff = jiffies;

  if (single_pre_jiff == 0)
  {
      mutex_unlock (&ipbw->data_wr_mutex);
      single_pre_jiff = jiffies;
      return 0;
  }

    /********Calc the real time_tag in ms *********/
  time_tag = (single_post_jiff - single_pre_jiff) * (1000 / HZ);

    /*******Calculate the IP Band Width************/
  for (idx = 0; idx < MAX_SINGLE_IP_IDX /*+ 2*/; idx += 4)
    {
	/********Select IP to count******************/
      single_ip_select_master (idx);
      temp_idx = idx;
      for (i = 0; i < 4; i++)
	{
	  if ((temp_idx + i) >= MAX_SINGLE_IP_IDX)	//  cari
	    {
	      break;
	    }
	  if ((0 == ((1 << (temp_idx + i)) & (ipbw->mon_ip_idx)))
	      && ((temp_idx + i) < 32))
	    {
	      continue;
	    }
	  if ((0 == ((1 << (temp_idx + i - 32)) & (ipbw->mon_ip_idx_add)))
	      && ((temp_idx + i) > 31))
	    {
	      continue;
	    }

	  	/*Get the IP BW of the first channel */
		#if defined(CONFIG_ALI_CHIP_3921)
			data = ioread32(ipbw->base+ 0x1200);
			data = data&(~(1<<31));
			iowrite32(data, ipbw->base+ 0x1200); 
				
			read_count_0 = (single_ip_get_read_count (temp_idx + i, 0) & 0xFFFFFFFF);
			write_count_0 = (single_ip_get_write_count (temp_idx + i, 0) & 0xFFFFFFFF);
			valid_count_0 = ((read_count_0 + write_count_0) & 0xFFFFFFFF);

			data = ioread32(ipbw->base+ 0x1200);
			data = data|(1<<31);
			iowrite32(data, ipbw->base + 0x1200);

			read_count_1 = (single_ip_get_read_count (temp_idx + i, 0) & 0xFFFFFFFF);
	  	write_count_1 = (single_ip_get_write_count (temp_idx + i, 0) & 0xFFFFFFFF);
	  	valid_count_1 = ((read_count_1 + write_count_1) & 0xFFFFFFFF);
				
		#elif defined(CONFIG_ALI_CHIP_3922)

			read_count_0 = (single_ip_get_read_count (temp_idx + i, 0) & 0xFFFFFFFF);
	  	write_count_0 = (single_ip_get_write_count (temp_idx + i, 0) & 0xFFFFFFFF);
	  	valid_count_0 = ((read_count_0 + write_count_0) & 0xFFFFFFFF);

			read_count_1 = (single_ip_get_read_count (temp_idx + i, 1) & 0xFFFFFFFF);
	  	write_count_1 = (single_ip_get_write_count (temp_idx + i, 1) & 0xFFFFFFFF);
	  	valid_count_1 = ((read_count_1 + write_count_1) & 0xFFFFFFFF);
		#endif

	  	//if ((valid_count_1 == valid_count_0) && (valid_count_1 != 0))
	  	//{
	    // 	PRINTK_BW (" %s(): Cnt_1[%d], Cnt_2[%d] \n ", __FUNCTION__,
			// 	valid_count_0, valid_count_1);
	  	//}

	  row = temp_idx + i + 1;	// the first one is total ip
	  t = (uint32_t) get_min_jiff_group_idx (row);
	  {
	    pre_bw_info[row].ip_real_value[t] = valid_count_0 + valid_count_1;

	    pre_bw_info[row].ip_real_value_0[t] = valid_count_0;
	    pre_bw_info[row].ip_real_value_1[t] = valid_count_1;

	    pre_bw_info[row].ip_bw_time_gap[t] = time_tag;

	    pre_bw_info[row].ip_bw_jiff[t] =
	      (uint32_t) (single_post_jiff & 0xFFFFFFFF);
	  }
	}
    }

  mutex_unlock (&ipbw->data_wr_mutex);

  return 0;

}


#ifdef ALI_IP_TWO_SPRT_CHAN
int32_t
get_ip_bw (uint8_t ipmod, uint8_t ip_ch_mod)
#else
int32_t
get_ip_bw (uint8_t ipmod)
#endif
{

  uint32_t i = 0, j = 0;

  uint32_t total_cycle_cnt = 0, valid_cycle_cnt = 0;
  uint32_t total_cycle_cnt_0 = 0, valid_cycle_cnt_0 = 0;
  uint32_t total_cycle_cnt_1 = 0, valid_cycle_cnt_1 = 0;
  uint32_t time_gap = 0;

  //PRINTK_BW ("%s, %d.\n", __FUNCTION__, __LINE__);

  resort_tmp_bw_array ();

  if (0 == ipmod)		//if(0 == final_bw_info.ip_mode)// only for total ip
  {
    //PRINTK_BW ("%s, %d.\n", __FUNCTION__, __LINE__);

    //Set the valid group number 
    final_bw_info.ip_bw_cnt = valid_ip_num;
    //PRINTK_BW ("\nfinal_bw_info.ip_bw_cnt = %d\n", final_bw_info.ip_bw_cnt);
    if (valid_ip_num < 1)
		{
	  	memset (final_bw_info.bw_info, 0,
		  	MAX_IP_IDX * sizeof (struct test_ip_bw_info));
	  	memset (pre_bw_info, 0,
		  	MAX_IP_IDX * sizeof (struct test_ip_tmp_info));
	  	return UNSUCCESS;
		}

    //Total IP BW calculation       
    for (j = 0; j < valid_ip_num; j++)
		{
	  	//Firstly test the Two Single Channel BW, then add for the total BW  
	  	//Two Single Channel IP BW info

	  	total_cycle_cnt_0 = pre_bw_info[0].ip_total_value_0[j];
	  	valid_cycle_cnt_0 = pre_bw_info[0].ip_real_value_0[j];
	  	final_bw_info.bw_info[0].ip_bw_value_0[j] =
	    	1000 * total_ip_calc_bw (total_cycle_cnt_0, valid_cycle_cnt_0);

	  	total_cycle_cnt_1 = pre_bw_info[0].ip_total_value_1[j];
	  	valid_cycle_cnt_1 = pre_bw_info[0].ip_real_value_1[j];
	  	final_bw_info.bw_info[0].ip_bw_value_1[j] =
	    	1000 * total_ip_calc_bw (total_cycle_cnt_1, valid_cycle_cnt_1);

	  	//Add two channel info to get the value
	  	total_cycle_cnt = pre_bw_info[0].ip_total_value[j];
	  	valid_cycle_cnt = pre_bw_info[0].ip_real_value[j];
	  	final_bw_info.bw_info[0].ip_bw_value[j] =
	    	1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
			
	  	//final_bw_info.bw_info[0].ip_bw_value[j] = final_bw_info.bw_info[0].ip_bw_value_0[j]+final_bw_info.bw_info[0].ip_bw_value_1[j];
	  	final_bw_info.bw_info[0].ip_bw_jiff[j] = pre_bw_info[0].ip_bw_jiff[j];
	  	final_bw_info.bw_info[0].ip_bw_time_gap[j] = pre_bw_info[0].ip_bw_time_gap[j];

#ifdef ALI_IP_TWO_SPRT_CHAN
	  	if (1 == ip_ch_mod)
	  	{
	    	//Two Single Channel IP BW info
	    	total_cycle_cnt = pre_bw_info[0].ip_total_value_0[j];
	    	valid_cycle_cnt = pre_bw_info[0].ip_real_value_0[j];
	    	final_bw_info.bw_info[0].ip_bw_value_0[j] =
					1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);

	    	total_cycle_cnt = pre_bw_info[0].ip_total_value_1[j];
	    	valid_cycle_cnt = pre_bw_info[0].ip_real_value_1[j];
	    	final_bw_info.bw_info[0].ip_bw_value_1[j] =
					1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
	  	}
	  	else if (2 == ip_ch_mod)
	  	{
	   		//Only the first Channel IP BW        
	    	total_cycle_cnt = pre_bw_info[0].ip_total_value_0[j];
	    	valid_cycle_cnt = pre_bw_info[0].ip_real_value_0[j];
	    	final_bw_info.bw_info[0].ip_bw_value_0[j] =
					1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
	  	}
	  	else if (3 == ip_ch_mod)
	  	{
	    	//Only the second Channel IP BW 
	    	total_cycle_cnt = pre_bw_info[0].ip_total_value_1[j];
	    	valid_cycle_cnt = pre_bw_info[0].ip_real_value_1[j];
	    	final_bw_info.bw_info[0].ip_bw_value_1[j] =
					1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
	  	}
#endif
		}
  }
  else if (1 == ipmod)		//(1 == final_bw_info.ip_mode) // only for single ip
  {
    PRINTK_BW ("%s, %d.\n", __FUNCTION__, __LINE__);
    //Set the valid group number 
    final_bw_info.ip_bw_cnt = valid_ip_num;
    PRINTK_BW ("\nfinal_bw_info.ip_bw_cnt = %d\n", final_bw_info.ip_bw_cnt);
    if (valid_ip_num < 1)
		{
	  	memset (final_bw_info.bw_info, 0,
		  	MAX_IP_IDX * sizeof (struct test_ip_bw_info));
	  	memset (pre_bw_info, 0,
		  	MAX_IP_IDX * sizeof (struct test_ip_tmp_info));

	  	return UNSUCCESS;
		}

    //Single IP BW calculation
    for (i = 1; i < MAX_IP_IDX; i++)
		{
	  	if (i <= 32)
	    {
	      if (0 == ((final_bw_info.ip_idx_flag) & (1 << (i - 1))))
					continue;
	    }
	  	else
	    {
	      if (0 ==
		  		((final_bw_info.ip_idx_flag_add) & (1 << (i - 1 - 32))))
				continue;
	    }

	  	for (j = 0; j < valid_ip_num; j++)
	    {
	      time_gap = pre_bw_info[i].ip_bw_time_gap[j];

	      valid_cycle_cnt_0 = pre_bw_info[i].ip_real_value_0[j];
	      final_bw_info.bw_info[i].ip_bw_value_0[j] =
					single_ip_count_bw (valid_cycle_cnt_0, time_gap);

	      valid_cycle_cnt_1 = pre_bw_info[i].ip_real_value_1[j];
	      final_bw_info.bw_info[i].ip_bw_value_1[j] =
					single_ip_count_bw (valid_cycle_cnt_1, time_gap);

	      valid_cycle_cnt = pre_bw_info[i].ip_real_value[j];
	      final_bw_info.bw_info[i].ip_bw_value[j] =
					single_ip_count_bw (valid_cycle_cnt, time_gap);
				
	      final_bw_info.bw_info[i].ip_bw_jiff[j] =	pre_bw_info[i].ip_bw_jiff[j];
	      final_bw_info.bw_info[i].ip_bw_time_gap[j] = pre_bw_info[i].ip_bw_time_gap[j];

#ifdef ALI_IP_TWO_SPRT_CHAN
	      if (1 == ip_ch_mod)
		{
		  //Two Single Channel IP BW info
		  valid_cycle_cnt = pre_bw_info[i].ip_real_value_0[j];
		  final_bw_info.bw_info[i].ip_bw_value_0[j] =
		    single_ip_count_bw (valid_cycle_cnt, time_gap);

		  valid_cycle_cnt = pre_bw_info[i].ip_real_value_1[j];
		  final_bw_info.bw_info[i].ip_bw_value_1[j] =
		    single_ip_count_bw (valid_cycle_cnt, time_gap);
		}
	      else if (2 == ip_ch_mod)
		{
		  //Only the first Channel IP BW                                  
		  valid_cycle_cnt = pre_bw_info[i].ip_real_value_0[j];
		  final_bw_info.bw_info[i].ip_bw_value_0[j] =
		    single_ip_count_bw (valid_cycle_cnt, time_gap);
		}
	      else if (3 == ip_ch_mod)
		{
		  //Only the second Channel IP BW 
		  valid_cycle_cnt = pre_bw_info[i].ip_real_value_1[j];
		  final_bw_info.bw_info[i].ip_bw_value_1[j] =
		    single_ip_count_bw (valid_cycle_cnt, time_gap);
		}
#endif
#if 0
	      PRINTK_BW ("\n \n %s: Single: row = %d, line = %d \n",
			 __FUNCTION__, i, j);
	      PRINTK_BW (" bw = %d, pre_bw = %d \n",
			 final_bw_info.bw_info[i].ip_bw_value[j],
			 pre_bw_info[i].ip_real_value[j]);
	      PRINTK_BW (" bw_time[%d], pre_time[%d] \n",
			 final_bw_info.bw_info[i].ip_bw_time_gap[j],
			 pre_bw_info[i].ip_bw_time_gap[j]);
	      PRINTK_BW (" bw_jiff[%d], pre_jiff[%d] \n",
			 final_bw_info.bw_info[i].ip_bw_jiff[j],
			 pre_bw_info[i].ip_bw_jiff[j]);
#endif
	    }
		}
  }
  else if (2 == ipmod)		// //if(2 == final_bw_info.ip_mode) // for both single and total ip
  {
    //Set the valid group number  
    final_bw_info.ip_bw_cnt = valid_ip_num;
    if (valid_ip_num < 1)
		{
	  	memset (final_bw_info.bw_info, 0, MAX_IP_IDX * sizeof (struct test_ip_bw_info));
	  	memset (pre_bw_info, 0,	MAX_IP_IDX * sizeof (struct test_ip_tmp_info));
	  	return UNSUCCESS;
		}

    for (i = 0; i < MAX_IP_IDX; i++)
		{
	  	//Total IP BW calculation 
	  	if (0 == i)
	    {
	      for (j = 0; j < valid_ip_num; j++)
				{
		  		//Firstly test the Two Single Channel BW, then add for the total BW  
		  		//Two Single Channel IP BW info

		  		total_cycle_cnt_0 = pre_bw_info[0].ip_total_value_0[j];
		  		valid_cycle_cnt_0 = pre_bw_info[0].ip_real_value_0[j];
		  		final_bw_info.bw_info[0].ip_bw_value_0[j] =
		    		1000 * total_ip_calc_bw (total_cycle_cnt_0, valid_cycle_cnt_0);

		  		total_cycle_cnt_1 = pre_bw_info[0].ip_total_value_1[j];
		  		valid_cycle_cnt_1 = pre_bw_info[0].ip_real_value_1[j];
		  		final_bw_info.bw_info[0].ip_bw_value_1[j] =
		    		1000 * total_ip_calc_bw (total_cycle_cnt_1, valid_cycle_cnt_1);

		  		//Add two channel info to get the value
		  		total_cycle_cnt = pre_bw_info[0].ip_total_value[j];
		  		valid_cycle_cnt = pre_bw_info[0].ip_real_value[j];
		  		final_bw_info.bw_info[0].ip_bw_value[j] =
		    		1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
					
		  		//final_bw_info.bw_info[0].ip_bw_value[j] = final_bw_info.bw_info[0].ip_bw_value_0[j]+final_bw_info.bw_info[0].ip_bw_value_1[j];
		  		final_bw_info.bw_info[0].ip_bw_jiff[j] = pre_bw_info[0].ip_bw_jiff[j];
		  		final_bw_info.bw_info[0].ip_bw_time_gap[j] = pre_bw_info[0].ip_bw_time_gap[j];

#ifdef ALI_IP_TWO_SPRT_CHAN
		  		if (1 == ip_ch_mod)
		    	{
		      	//Two Single Channel IP BW info
		     	 	total_cycle_cnt = pre_bw_info[0].ip_total_value_0[j];
		      	valid_cycle_cnt = pre_bw_info[0].ip_real_value_0[j];
		      	final_bw_info.bw_info[0].ip_bw_value_0[j] =
							1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);

		      	total_cycle_cnt = pre_bw_info[0].ip_total_value_1[j];
		      	valid_cycle_cnt = pre_bw_info[0].ip_real_value_1[j];
		      	final_bw_info.bw_info[0].ip_bw_value_1[j] =
							1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
		    	}
		  		else if (2 == ip_ch_mod)
		    	{
		      	//Only the first Channel IP BW        
		      	total_cycle_cnt = pre_bw_info[0].ip_total_value_0[j];
		      	valid_cycle_cnt = pre_bw_info[0].ip_real_value_0[j];
		      	final_bw_info.bw_info[0].ip_bw_value_0[j] =
							1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
		    	}
		  		else if (3 == ip_ch_mod)
		    	{
		      	//Only the second Channel IP BW 
		      	total_cycle_cnt = pre_bw_info[0].ip_total_value_1[j];
		      	valid_cycle_cnt = pre_bw_info[0].ip_real_value_1[j];
		      	final_bw_info.bw_info[0].ip_bw_value_1[j] =
							1000 * total_ip_calc_bw (total_cycle_cnt, valid_cycle_cnt);
		    	}
#endif

#if 0
		  		PRINTK_BW ("\n \n %s: Total: row = %d, line = %d \n",	__FUNCTION__, i, j);
		  		PRINTK_BW (" bw = %d, pre_bw = %d \n",
			     final_bw_info.bw_info[i].ip_bw_value[j], pre_bw_info[i].ip_real_value[j]);
		  		PRINTK_BW (" bw_time = %d, pre_time = %d \n",
			     final_bw_info.bw_info[i].ip_bw_time_gap[j], pre_bw_info[i].ip_bw_time_gap[j]);
		  		PRINTK_BW (" bw_jiff = %d, pre_jiff = %d \n",
			     final_bw_info.bw_info[i].ip_bw_jiff[j], pre_bw_info[i].ip_bw_jiff[j]);
#endif
				}

			}
	 		else			//Single IP BW calculation
	  	{
	    	if (i <= 32)
			{
		  		if (0 == ((final_bw_info.ip_idx_flag) & (1 << (i - 1))))
		    		continue;
			}
	    	else
			{
		  		if (0 ==((final_bw_info.ip_idx_flag_add) & (1 << (i - 1 - 32))))
		    		continue;
			}
	    	
	    	for (j = 0; j < valid_ip_num; j++)
				{
		  		time_gap = pre_bw_info[i].ip_bw_time_gap[j];

		  		valid_cycle_cnt_0 = pre_bw_info[i].ip_real_value_0[j];
		  		final_bw_info.bw_info[i].ip_bw_value_0[j] =	single_ip_count_bw (valid_cycle_cnt_0, time_gap);

		  		valid_cycle_cnt_1 = pre_bw_info[i].ip_real_value_1[j];
		  		final_bw_info.bw_info[i].ip_bw_value_1[j] =	single_ip_count_bw (valid_cycle_cnt_1, time_gap);

		  		valid_cycle_cnt = pre_bw_info[i].ip_real_value[j];
		  		final_bw_info.bw_info[i].ip_bw_value[j] =	single_ip_count_bw (valid_cycle_cnt, time_gap);
					
		  		final_bw_info.bw_info[i].ip_bw_jiff[j] = pre_bw_info[i].ip_bw_jiff[j];
		  		final_bw_info.bw_info[i].ip_bw_time_gap[j] = pre_bw_info[i].ip_bw_time_gap[j];

#ifdef ALI_IP_TWO_SPRT_CHAN
		  		if (1 == ip_ch_mod)
		    	{
		      	//Two Single Channel IP BW info
		      	valid_cycle_cnt = pre_bw_info[i].ip_real_value_0[j];
		      	final_bw_info.bw_info[i].ip_bw_value_0[j] =	single_ip_count_bw (valid_cycle_cnt, time_gap);

		      	valid_cycle_cnt = pre_bw_info[i].ip_real_value_1[j];
		      	final_bw_info.bw_info[i].ip_bw_value_1[j] =	single_ip_count_bw (valid_cycle_cnt, time_gap);
		    	}
		  		else if (2 == ip_ch_mod)
		    	{
		      	//Only the first Channel IP BW                      
		      	valid_cycle_cnt = pre_bw_info[i].ip_real_value_0[j];
		      	final_bw_info.bw_info[i].ip_bw_value_0[j] =	single_ip_count_bw (valid_cycle_cnt, time_gap);
		    	}
		  		else if (3 == ip_ch_mod)
		    	{
		      	//Only the second Channel IP BW 
		      	valid_cycle_cnt = pre_bw_info[i].ip_real_value_1[j];
		      	final_bw_info.bw_info[i].ip_bw_value_1[j] =	single_ip_count_bw (valid_cycle_cnt, time_gap);
		    	}
#endif

#if 0
		  		PRINTK_BW ("\n \n %s: Single: row = %d, line = %d \n", __FUNCTION__, i, j);
		  		PRINTK_BW (" bw = %d, pre_bw = %d \n",
			     final_bw_info.bw_info[i].ip_bw_value[j], pre_bw_info[i].ip_real_value[j]);
					PRINTK_BW (" bw0 = %d, pre_bw0 = %d \n",
			     final_bw_info.bw_info[i].ip_bw_value_0[j], pre_bw_info[i].ip_real_value_0[j]);
					PRINTK_BW (" bw1 = %d, pre_bw1 = %d \n",
			     final_bw_info.bw_info[i].ip_bw_value_1[j], pre_bw_info[i].ip_real_value_1[j]);
		  		PRINTK_BW (" bw_time[%d], pre_time[%d] \n",
			     final_bw_info.bw_info[i].ip_bw_time_gap[j], pre_bw_info[i].ip_bw_time_gap[j]);
		  		PRINTK_BW (" bw_jiff[%d], pre_jiff[%d] \n",
			     final_bw_info.bw_info[i].ip_bw_jiff[j], pre_bw_info[i].ip_bw_jiff[j]);
#endif

				}
	  	}

		}
  }

  return SUCCESS;

}


void
ali_m36_ip_bw_task (struct work_struct *work)
{
  struct ali_m36_ip_bw_dev *ipbw;

  /* Init sw sturctures. */
  ipbw = &g_ali_ip_bw_dev;

  // Calculate the IP Bandwidth
  do
  {
    if (ipbw->ip_test_flag == IP_BW_MON_STOP)
		{
	  	ipbw->ip_test_flag = IP_BW_MON_IDLE;
	  	//PRINTK_BW ("\n\n%s, -%d-. Stop IP BW Test Task \n\n\n",
		  //   __FUNCTION__, __LINE__);
	  	break;
		}
    else if (ipbw->ip_test_flag == IP_BW_MON_PAUSE)
		{
	  	//PRINTK_BW ("\n\n%s, -%d-. Pouse IP BW Test Task \n\n\n",
		  //   __FUNCTION__, __LINE__);
	  	//msleep (2000);
	  	msleep(500);
	  	continue;
		}
    else if (ipbw->ip_test_flag == IP_BW_MON_RESUME)
		{
	  	//PRINTK_BW ("\n\n%s, -%d-. RESUME IP BW Test Task \n\n\n",
		  //   __FUNCTION__, __LINE__);
	  	ipbw->ip_test_flag = IP_BW_MON_START;
	  	continue;
		}
    else if (ipbw->ip_test_flag == IP_BW_MON_START)
		{
	  	//PRINTK_BW ("\n\n%s, -%d-. START IP BW Test Task \n\n\n",
		  //   __FUNCTION__, __LINE__);
	  	if (1 == ipbw->ip_bw_total_flag)
	    {
	      // Get Total IP bandwidth 
	      //PRINTK_BW ("\n\n%s, -%d-. Get Total IP BW Test Task \n\n\n",
			 	//	__FUNCTION__, __LINE__);
	      get_total_ip_bw ();
	    }

	  	if (1 == ipbw->ip_bw_single_flag)
	    {
	      // Get Single IP bandwidth
	      //PRINTK_BW ("\n\n%s, -%d-. Get Single IP BW Test Task \n\n\n",
			 	//	__FUNCTION__, __LINE__);
	      get_single_ip_bw ();
	    }

	  	if ((ipbw->ip_bw_total_flag + ipbw->ip_bw_single_flag))
	    {
	      if (valid_ip_num >= MAX_BW_GROUP)
				{
		  		valid_ip_num = MAX_BW_GROUP;
				}
	      else
				{
		  		valid_ip_num++;
					//PRINTK_BW ("\n\n%s, -%d-. valid_ip_num=%d \n\n\n",
			 		//	__FUNCTION__, __LINE__,valid_ip_num);
					ipbw->bw_status = 1;
				}
	  	}
			
	  	if ((0 == ipbw->ip_bw_total_flag) && (0 == ipbw->ip_bw_single_flag))
	  	{
		  	ipbw->bw_status = 0;
	      msleep (100);
	  	}

			msleep (100);
		}
    else
		{
	  	break;
		}
  }
  while (1);

  //PRINTK_BW ("%s, %d.\r\n", __FUNCTION__, __LINE__);
}



int32_t
ali_m36_ip_bw_tsk_start (void)
{
  struct ali_m36_ip_bw_dev *ipbw;

  //PRINTK_BW ("%s, %d.\n", __FUNCTION__, __LINE__);

  /* Init sw sturctures. */
  ipbw = &g_ali_ip_bw_dev;
  ipbw->ip_test_flag = IP_BW_MON_START;

  /* Init the task for IP Band Width */
  ipbw->xfer_workq = create_workqueue ("ali_m36_ip_bw_0");
  if (!(ipbw->xfer_workq))
  {
      PRINTK_BW ("%s,create workqueue failed %d.\n", __FUNCTION__, __LINE__);
      return -1;
  }
  INIT_WORK (&ipbw->xfer_work, ali_m36_ip_bw_task);
  queue_work (ipbw->xfer_workq, &ipbw->xfer_work);

  return 0;
}



int32_t
ali_m36_ip_bw_tsk_stop (void)
{
  struct ali_m36_ip_bw_dev *ipbw;


  /* Init sw sturctures. */
  ipbw = &g_ali_ip_bw_dev;

  ipbw->mon_ip_idx = 0;
  ipbw->mon_ip_idx_add = 0;
  ipbw->ip_bw_total_flag = 0;
  ipbw->ip_bw_single_flag = 0;
  ipbw->ip_test_flag = IP_BW_MON_STOP;
	ipbw->bw_status = 0;

  while (ipbw->ip_test_flag)
    {
      msleep (10);
      break;
    }
  destroy_workqueue (ipbw->xfer_workq);
  return 0;
}


int32_t
ali_m36_ip_bw_tsk_pause (void)
{
  struct ali_m36_ip_bw_dev *ipbw;


  /* Init sw sturctures. */
  ipbw = &g_ali_ip_bw_dev;

  ipbw->ip_test_flag = IP_BW_MON_PAUSE;

  return 0;
}

int32_t
ali_m36_ip_bw_tsk_resume (void)
{
  struct ali_m36_ip_bw_dev *ipbw;


  //Clear the buffer to store the BW info
  memset (pre_bw_info, 0, MAX_IP_IDX * sizeof (struct test_ip_tmp_info));

  valid_ip_num = 0;

  /* Init sw sturctures. */
  ipbw = &g_ali_ip_bw_dev;

  ipbw->ip_test_flag = IP_BW_MON_RESUME;
	ipbw->bw_status = 0;

  return 0;
}


int32_t
ali_m36_ip_bw_total (void)
{
  int ret = 0;
  struct ali_m36_ip_bw_dev *ipbw = NULL;

  ipbw = &g_ali_ip_bw_dev;

  if (ipbw->status != ALI_M36_IP_BW_STATUS_RUN)
    {
      PRINTK_BW ("%s, %d, Dev in Idle Statu \n", __FUNCTION__, __LINE__);
      return (-EPERM);
    }
  ipbw->ip_bw_total_flag = 1;
  ipbw->ip_bw_single_flag = 0;

  return (ret);
}


int32_t
ali_m36_ip_bw_single (unsigned long index1, unsigned long index2)
{
  int ret = 0;
  struct ali_m36_ip_bw_dev *ipbw = NULL;
  ipbw = &g_ali_ip_bw_dev;

  if (ipbw->status != ALI_M36_IP_BW_STATUS_RUN)
    {
      PRINTK_BW ("%s, %d, Dev in Idle Statu \n", __FUNCTION__, __LINE__);
      return (-EPERM);
    }

  ipbw->ip_bw_total_flag = 0;
  ipbw->ip_bw_single_flag = 1;

  ipbw->mon_ip_idx = 0;
  ipbw->mon_ip_idx_add = 0;

  //if (index2 > 0x03)
  //{
  //  return -1;
  //}
  
  /*
     if(index<32)
     {
     ipbw->mon_ip_idx |= (1<<index);
     }
     else
     {
     ipbw->mon_ip_idx = 0;
     ipbw->mon_ip_idx_add |= (1<<(index-32));
     }
   */
  //printk("index1 = 0x%x index2 = 0x%x\n",index1,index2);

  ipbw->mon_ip_idx = index1;	// for test
  ipbw->mon_ip_idx_add = index2;

  return (ret);
}


int32_t
ali_m36_ip_bw_all (unsigned long index1, unsigned long index2)
{
  int ret = 0;
  struct ali_m36_ip_bw_dev *ipbw = NULL;

  ipbw = &g_ali_ip_bw_dev;

  if (ipbw->status != ALI_M36_IP_BW_STATUS_RUN)
    {
      PRINTK_BW ("%s, %d, Dev in Idle Statu \n", __FUNCTION__, __LINE__);
      return (-EPERM);
    }
  ipbw->ip_bw_total_flag = 1;
  ipbw->ip_bw_single_flag = 1;

  ipbw->mon_ip_idx = index1;
  ipbw->mon_ip_idx_add = index2;

  return (ret);
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
long
ali_m36_ip_bw_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
#else
int32_t
ali_m36_ip_bw_ioctl (struct inode * inode, struct file * filp,
		     unsigned int cmd, unsigned long arg)
#endif
{
  //eric.cai debug
  int val_test = -1;
  int32_t ret = 0;
  struct ali_m36_ip_bw_dev *ipbw = NULL;

  
  __u32 val_ret; 

  //ipbw = filp->private_data;
  ipbw = &g_ali_ip_bw_dev;
  if (mutex_lock_interruptible (&ipbw->io_mutex))
  {
      return (-ERESTARTSYS);
  }
  switch (cmd)
  {
    case ALI_TEST_IP_TASK_START:
    {
			ret = ali_m36_ip_bw_tsk_start ();
			if (0 != ret)
	  		PRINTK_BW ("%s, %d. Fail to start IP_BW task! \n", __FUNCTION__, __LINE__);
			break;
    }
    case ALI_TEST_IP_TASK_STOP:
    {
			ali_m36_ip_bw_tsk_stop ();
			//PRINTK_BW ("%s, %d. Success to stop IP_BW task! \n", __FUNCTION__,
		  // __LINE__);
			break;
    }
    case ALI_TEST_IP_TASK_PAUSE:
    {
			ali_m36_ip_bw_tsk_pause ();
			//PRINTK_BW ("%s, %d. Success to pause IP_BW task! \n", __FUNCTION__,
		  // __LINE__);
			break;
    }
    case ALI_TEST_IP_TASK_RESUME:
    {
			ali_m36_ip_bw_tsk_resume ();
			//PRINTK_BW ("%s, %d. Success to resume IP_BW task! \n", __FUNCTION__,
		  // __LINE__);
			break;
    }
    case ALI_TEST_IP_BW_CONFIG:
    {
			memset (&ipbw_cfg, 0, sizeof (struct test_ip_bw_cfg_info));
			ret = copy_from_user(&ipbw_cfg, (struct test_ip_bw_cfg_info *)arg, sizeof(struct test_ip_bw_cfg_info));  

			if (0 == ret)
			{
	    	ipbw_monitor_timegap = ipbw_cfg.time_gap;
	    	if (0 == ipbw_cfg.ip_mode)
	    	{
					ali_m36_ip_bw_total ();
	    	}
	    	else if (1 == ipbw_cfg.ip_mode)
	    	{
					ali_m36_ip_bw_single (ipbw_cfg.ip_enable_flag, ipbw_cfg.ip_enable_flag_add);
	    	}
	    	else if (2 == ipbw_cfg.ip_mode)
	    	{
					ali_m36_ip_bw_all (ipbw_cfg.ip_enable_flag, ipbw_cfg.ip_enable_flag_add);
	    	}
			}
			else
			{
	    	PRINTK_BW("%s, %d: ALI_TEST_IP_BW_CONFIG: Can't copy data from User to kernel! \n",
	       __FUNCTION__, __LINE__);
			}
			break;
    }

    case ALI_TEST_IP_BW_GET:
    {
			ipbw->ip_test_flag = IP_BW_MON_PAUSE;
			
			memset (&final_bw_info, 0, sizeof (struct test_ip_get_bw));
			ret = copy_from_user (&final_bw_info, (void __user *)arg, sizeof (struct test_ip_get_bw));

			if (0 == ret)
	  	{
	    	//Calulate the real bw according to the Byte gap and Time gap
#ifdef ALI_IP_TWO_SPRT_CHAN
	    	get_ip_bw (final_bw_info.ip_mode, final_bw_info.ip_chan_mode);
#else
	    	get_ip_bw (final_bw_info.ip_mode);
#endif

	    	ret = copy_to_user((void __user *)arg,(void *)&final_bw_info,sizeof (struct test_ip_get_bw));
	    	if (ret)
	    	{
					PRINTK_BW("%s, %d: ALI_TEST_IP_BW_GET: Can't copy data to User! \n",__FUNCTION__, __LINE__);
	    	}
	  	}
			else
	  	{
	    	PRINTK_BW("%s, %d: ALI_TEST_IP_BW_GET: Can't copy data from User to kernel! \n",__FUNCTION__, __LINE__);
	  	}
			break;
  	}

	//eric.cai 
	case ALI_TEST_IP_INT:
		{
			//PRINTK_BW ("1111%s, %d. ALI_TEST_IP_INT(0x%08x)! \r\n", __FUNCTION__,
			//	   __LINE__, arg);					  
			ret = copy_from_user ((void *)&val_test, (void *)arg, sizeof(int));					  
			if (0 == ret){
				PRINTK_BW("ok. val_test=%d.\n",val_test);
			}
			else{
				PRINTK_BW("Err.copy_from_user failed.\n");
			}
			break;
		}
	
		case ALI_TEST_IP_BW_STATUS:
		{
			ret = copy_to_user ((void __user *) arg, (void *) &ipbw->bw_status, sizeof (ipbw->bw_status));
			break;
		}

	//tony.su : check L2 cache open or not
	case ALI_TEST_CHECK_L2_CACHE:
	{
		val_ret = (__u32)check_L2_cache_enable();

		ret = copy_to_user ((void *)arg, (void *)&val_ret, sizeof(__u32)); 
		if (0 == ret){
			PRINTK_BW("ok. check_L2_cache_enable   val_ret=%d.\n",val_ret);
		}
		else{
			PRINTK_BW("Err.copy_to_user failed.\n");
		}
		break;
	}
		
    default:
    {
			ret = -ENOTTY;
			PRINTK_BW ("%s, %d. (cmd=%d)Fail to CTRL! \n", __FUNCTION__, __LINE__,
		   cmd);
			break;
    }
  }

  mutex_unlock (&ipbw->io_mutex);

  return (ret);
}

int32_t
ali_m36_ip_bw_open (struct inode * inode, struct file * file)
{
  struct ali_m36_ip_bw_dev *ipbw;


  ipbw = container_of (inode->i_cdev, struct ali_m36_ip_bw_dev, cdev);
  if (mutex_lock_interruptible (&ipbw->io_mutex))
  {
    return (-ERESTARTSYS);
  }

  if (ipbw->status != ALI_M36_IP_BW_STATUS_IDLE)
  {
    mutex_unlock (&ipbw->io_mutex);
    return (-EMFILE);
  }

  file->private_data = ipbw;

  ipbw->status = ALI_M36_IP_BW_STATUS_RUN;
  ipbw->ip_bw_total_flag = 0;
  ipbw->ip_bw_single_flag = 0;
  ipbw->mon_ip_idx = 0;
  ipbw->mon_ip_idx_add = 0;

  total_pre_jiff = 0;
  single_pre_jiff = 0;

  //Clear the storage buffer
  memset (pre_bw_info, 0, MAX_IP_IDX * sizeof (struct test_ip_tmp_info));
  memset (&final_bw_info, 0, sizeof (struct test_ip_get_bw));

  mutex_unlock (&ipbw->io_mutex);

  return (0);
}



int32_t
ali_m36_ip_bw_close (struct ali_m36_ip_bw_dev * ipbw)
{

  if (mutex_lock_interruptible (&ipbw->io_mutex))
  {
      return (-ERESTARTSYS);
  }

  if (ipbw->status != ALI_M36_IP_BW_STATUS_RUN)
  {
      mutex_unlock (&ipbw->io_mutex);
      return (-EPERM);
  }

  if (mutex_lock_interruptible (&(ipbw->data_wr_mutex)))
  {
      return (-ERESTARTSYS);
  }

  ipbw->status = ALI_M36_IP_BW_STATUS_IDLE;

  mutex_unlock (&ipbw->data_wr_mutex);

  mutex_unlock (&ipbw->io_mutex);

  return (0);
}

int32_t
ali_m36_ip_bw_release (struct inode * inode, struct file * file)
{
  struct ali_m36_ip_bw_dev *ipbw;

  ipbw = file->private_data;

  return (ali_m36_ip_bw_close (ipbw));
}

struct file_operations g_ali_ip_bw_fops = {
  .owner = THIS_MODULE,
#if(LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
  .unlocked_ioctl = ali_m36_ip_bw_ioctl,
#else
  .ioctl = ali_m36_ip_bw_ioctl,
#endif
  .open = ali_m36_ip_bw_open,
  .release = ali_m36_ip_bw_release,
};

static int ali_ip_bw_probe(struct platform_device * pdev)
{
  int32_t result;
  struct device *clsdev;
  struct ali_m36_ip_bw_dev *ipbw;

  /* Init sw sturctures. */
  ipbw = &g_ali_ip_bw_dev;
  memset (ipbw, 0, sizeof (struct ali_m36_ip_bw_dev));
  mutex_init (&ipbw->data_wr_mutex);
  mutex_init (&ipbw->io_mutex);

  ipbw->base = ioremap (ALI_SOC_REG_BASE, 0x1300);	//

	#if defined(CONFIG_ALI_CHIP_3922)
		ipbw->pMEM_DMARB_IOBase1 = ioremap(DMARB_IOBase1,0x300); 		//0x18012000
		ipbw->pMEM_DMARB_IOBase2 = ioremap(DMARB_IOBase2,0x300); 		//0x18013000
		ipbw->pMEM_DMCTRL_IOBase1 = ioremap(DMCTRL_IOBase1, 0x100); 	//0x18010000
		ipbw->pMEM_DMCTRL_IOBase2 = ioremap(DMCTRL_IOBase2, 0x100); 	//0x18011000
	#endif

	result = of_get_major_minor(pdev->dev.of_node,&ipbw->dev_id, 
			0, 1, "ali_m36_ip_bw_0");
	if (result  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto fail;
	}

  cdev_init (&(ipbw->cdev), &g_ali_ip_bw_fops);
  ipbw->cdev.owner = THIS_MODULE;
  result = cdev_add (&ipbw->cdev, ipbw->dev_id, 1);

  /* Fail gracefully if need be. */
  if (result)
    {
      PRINTK_BW ("cdev_add() failed, result:%d\n", result);
      goto fail;
    }

  /* Init Class. */
  g_ali_ip_bw_class = class_create (THIS_MODULE, "ali_ip_bw_class");
  if (IS_ERR (g_ali_ip_bw_class))
    {
      result = PTR_ERR (g_ali_ip_bw_class);
      goto fail;
    }
  clsdev = device_create (g_ali_ip_bw_class, NULL, ipbw->dev_id,
			  ipbw, "ali_m36_ip_bw_0");
  if (IS_ERR (clsdev))
    {
      PRINTK_BW (KERN_ERR "device_create() failed!\n");
      result = PTR_ERR (clsdev);
      goto fail;
    }
  return (0);

fail:
  return (-1);
}

static int ali_ip_bw_remove(struct platform_device * pdev)
{
  ali_m36_ip_bw_close (&g_ali_ip_bw_dev);
  return 0;
}
static const struct of_device_id ali_ip_bw_match[] = {
       { .compatible = "alitech, ip_bw", },
       {},
};

MODULE_DEVICE_TABLE(of, ali_ip_bw_match);

static struct platform_driver ali_tsi_platform_driver = {
	.probe   = ali_ip_bw_probe, 
	.remove   = ali_ip_bw_remove,
	.driver   = {
			.owner  = THIS_MODULE,
			.name   = "ali_ip_bw",
			.of_match_table = ali_ip_bw_match,
	},
};
module_platform_driver(ali_tsi_platform_driver);
MODULE_AUTHOR ("ALi Corporation, Inc.");
MODULE_DESCRIPTION ("ALI BANDWIDTH TEST driver");
MODULE_LICENSE ("GPL");
