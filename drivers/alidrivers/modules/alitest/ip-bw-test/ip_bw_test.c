/*****************************************************************************
*	Copyrights(C) 2012 Acer Laboratries inc. All Rights Reserved.
*
*	FILE NAME:		adr_dbg_bw.c
*
*	DESCRIPTION:	Band width debug module.
*
*	HISTORY:
*						Date 	 Author      Version 	  Notes
*					=========	=========	=========	===========
*					2012-12-3	 Leo.Ma      Ver 1.0	Create file.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>


#include "ali_test.h"

typedef unsigned char   	uint8;
typedef unsigned short  	uint16;
typedef unsigned int     	uint32;
typedef unsigned char  	UINT8;
typedef unsigned short 	UINT16;
typedef unsigned int   		UINT32;
typedef int 				INT32;

#define RET_SUCCESS		((INT32)0)
#define	RET_FAILURE		((INT32)1)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MS_100           (1000*100)

typedef unsigned char	ARG_CHK;

#define DBG_ARG_INVALID	(0)
#define DBG_ARG_VALID	(1)


#define BW_DBG_PRINT(fmt, args...)  \
			do { \
					printf(fmt, ##args); \
			} while(0)


#define BW_FD_PATH	"/dev/ali_m36_ip_bw_0"
#define BW_DBG_POLL_INTRV	20
#define BW_DBG_IP_FLAG	0xffffffff

#if defined(CONFIG_ALI_CHIP_3921)
	#define BW_DBG_IP_FLAG_ADD	0x0000003
#else
	#define BW_DBG_IP_FLAG_ADD	0x00000ff
#endif

#define BW_DBG_CHAN_MODE	3
#define avr_inc(x, y, z)	(((x) >= (y)) ? (((x) - (y)) / (++(z))) : (-(((y) - (x)) / (++(z)))))

typedef enum bw_dbg_ip_mode {
	BW_DBG_TOTAL_IP,
	BW_DBG_SINGLE_IP = 1,
	BW_DBG_BOTH_TOT_SIG_IP,
} BW_DBG_IP_MODE_E;

typedef enum bw_dbg_chan_mode {
	BW_DBG_SINGLE_CHAN,
	BW_DBG_DOUBLE_CHAN = 1,
	BW_DBG_FIRST_CHAN,
	BW_DBG_SECOND_CHAN,
} BW_DBG_CHAN_MODE_E;

typedef struct bw_dbg_info {
	int fd;
	UINT32 BwDbgInit;
	INT32 TestCnt;
	BW_DBG_CHAN_MODE_E chan_mode;
	struct test_ip_bw_cfg_info bw_config;
}__attribute__( ( packed, aligned( 1 ) ) ) BW_DBG_INFO_S;



static BW_DBG_INFO_S BwDbgInfo;


//eric.cai log to local file will slow down the system, and may cause some other issue. you'd better DO NOT enable it.
// #define LOG_TO_LOCAL_FILE 
#ifdef LOG_TO_LOCAL_FILE
#define LOCAL_LOG_FILE_PATH "/data/ip_bw_test.csv" // for excel
int g_local_log_file_fd = -1;
#endif

static INT32 BwDbgFdOpen()
{
	INT32 Ret;

    BwDbgInfo.fd = open(BW_FD_PATH, O_RDONLY | O_CLOEXEC);

	if (BwDbgInfo.fd < 0)
    {		
        return RET_FAILURE;
    }	

	return RET_SUCCESS;
}

static void BwDbgFdClose()
{
	if (BwDbgInfo.fd > 0)
  {
		close(BwDbgInfo.fd);
  }

	memset(&BwDbgInfo, 0, sizeof(BwDbgInfo));

	return;
}

static void bw_dbg_init(void)
{
	int i = 0;
	if (!BwDbgInfo.BwDbgInit)
	{
		memset(&BwDbgInfo, 0, sizeof(BwDbgInfo));
		BwDbgInfo.BwDbgInit = TRUE;
		BwDbgInfo.bw_config.ip_enable_flag = BW_DBG_IP_FLAG;
		BwDbgInfo.bw_config.ip_enable_flag_add= BW_DBG_IP_FLAG_ADD;

		//BwDbgInfo.bw_config.ip_mode = BW_DBG_BOTH_TOT_SIG_IP;
		//BwDbgInfo.bw_config.time_gap = BW_DBG_POLL_INTRV;
		//BwDbgInfo.chan_mode = BW_DBG_DOUBLE_CHAN;
		//BwDbgInfo.TestCnt = 1;
		
		/*eric.cai modify for M3823: single ip bw + single channel + once */
		BwDbgInfo.bw_config.ip_mode = BW_DBG_BOTH_TOT_SIG_IP;
		BwDbgInfo.bw_config.time_gap = BW_DBG_POLL_INTRV;
		BwDbgInfo.chan_mode = BW_DBG_SINGLE_CHAN;//
		BwDbgInfo.TestCnt = 1;

		if (BwDbgFdOpen() != RET_SUCCESS)
		{
			BW_DBG_PRINT("Band width debug open fail!\n");
			return;
		}
	}
	
#ifdef LOG_TO_LOCAL_FILE
		g_local_log_file_fd = open(LOCAL_LOG_FILE_PATH,O_RDWR);
		if (g_local_log_file_fd <0)
		{
			printf("Error:open file %s failed.\n",LOCAL_LOG_FILE_PATH);
			return ;
		}
		for(i = 0; i < MAX_IP_IDX; i++){
			write (g_local_log_file_fd,IP_NAME[i],strlen(IP_NAME[i]));
			if (i +1 < MAX_IP_IDX){
				write (g_local_log_file_fd,",",1);
			}
			else {
				write (g_local_log_file_fd,"\r\n",2);
			}

		}
#endif
	
	return;
}

static void bw_dbg_exit(int argc, char **argv)
{
	BwDbgFdClose();
#ifdef LOG_TO_LOCAL_FILE
		if (g_local_log_file_fd >=0)
		{
			close(g_local_log_file_fd);
			g_local_log_file_fd = -1;
		}
#endif

	return;
}

static void bw_dbg_start(int argc, char **argv)
{
	INT32 Ret;

	Ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_START, 0);

  if (Ret < 0)
  {
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", Ret, __FUNCTION__, __LINE__, errno);
  }
	
	return;
}

static void bw_dbg_pause(int argc, char **argv)
{
	INT32 Ret;
	
	Ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_STOP, 0);

    if (Ret < 0)
    {
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", Ret, __FUNCTION__, __LINE__, errno);
    }
	
	return;
}

static void bw_dbg_show_info(int argc, char **argv)
{
	INT32					ret, k;
	struct test_ip_get_bw	bw_stat;
	UINT32					j;
	INT32 					i;
  	UINT32					bw_avr[BW_DBG_CHAN_MODE][MAX_IP_IDX];
	UINT32					bw_cnt[BW_DBG_CHAN_MODE][MAX_IP_IDX];
  	UINT32					bw_avr2[BW_DBG_CHAN_MODE][MAX_IP_IDX];
	UINT32					bw_cnt2[BW_DBG_CHAN_MODE][MAX_IP_IDX];
	char tmp_buf[20] = {0};


	memset(bw_avr2, 0, sizeof(bw_avr2));
	memset(bw_cnt2, 0, sizeof(bw_cnt2));
	memset(&bw_stat, 0, sizeof(bw_stat));
	bw_stat.ip_mode = BwDbgInfo.bw_config.ip_mode;
	bw_stat.ip_idx_flag = BwDbgInfo.bw_config.ip_enable_flag;
	bw_stat.ip_idx_flag_add= BwDbgInfo.bw_config.ip_enable_flag_add;
	
	bw_stat.ip_chan_mode = BwDbgInfo.chan_mode;

	//tony.su 
	usleep(MS_100);
	UINT32 nL2Cachestatus = 0;
	ret = ioctl(BwDbgInfo.fd, ALI_TEST_CHECK_L2_CACHE, &nL2Cachestatus);
	if (ret < 0)
	{
	    BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}
	else{
	    BW_DBG_PRINT("ALI_TEST_CHECK_L2_CACHE    nL2Cachestatus=%x %s,%d\n", \
	    	nL2Cachestatus, __FUNCTION__, __LINE__);
	}
	
		
	return;
}



static void bw_dbg_ip_mode_set(int argc, char **argv)
{
	INT32 number, ret;

	ret = sscanf(*argv, "%d", &number);
	if (0 == ret || number < 0 || number > BW_DBG_BOTH_TOT_SIG_IP)
	{
		BW_DBG_PRINT("Argument \"%s\" not a number or out of range 0-2!\n", *argv);
		return;
	}

	if (!BwDbgInfo.BwDbgInit)
		bw_dbg_init();
	
	BwDbgInfo.bw_config.ip_mode = number;

	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_PAUSE, 0);
	if (ret < 0)
	{
		BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}

	BW_DBG_PRINT("%s,%d\n", __FUNCTION__, __LINE__);
	
	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_BW_CONFIG, &BwDbgInfo.bw_config);
    if (ret < 0)
    {
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
    }

	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_RESUME, 0);
	if (ret < 0)
	{
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}

	return;
}

static ARG_CHK bw_dbg_ip_mode_preview(int argc, char **argv, char *option)
{
	int number, ret;

	if (0 == argc)
	{
		BW_DBG_PRINT("Option \"%s\": Lack of valid argument(ex: number)!\n", option);
		return DBG_ARG_INVALID;
	}

	if (argc != 1)
	{
		BW_DBG_PRINT("Option \"%s\": Only one argument admitted!\n", option);
		return DBG_ARG_INVALID;
	}
	
	ret = sscanf(*argv, "%d", &number);
	if (0 == ret || number < 0 || number > BW_DBG_BOTH_TOT_SIG_IP)
	{
		BW_DBG_PRINT("Option \"%s\": Argument \"%s\" not a number or out of range 0-2!\n", option, *argv);
		return DBG_ARG_INVALID;
	}

	return DBG_ARG_VALID;
}

static void bw_dbg_set_intv(int argc, char **argv)
{
	INT32 number, ret;

	ret = sscanf(*argv, "%d", &number);
	if (0 == ret || number < 0)
	{
		BW_DBG_PRINT("Argument \"%s\" not a number or less than zero!\n", *argv);
		return;
	}

	if (!BwDbgInfo.BwDbgInit)
		bw_dbg_init();
	
	BwDbgInfo.bw_config.time_gap = number;

	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_PAUSE, 0);
	if (ret < 0)
	{
		BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}

	BW_DBG_PRINT("%s,%d\n", __FUNCTION__, __LINE__);
	
	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_BW_CONFIG, &BwDbgInfo.bw_config);
    if (ret < 0)
    {
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
    }

	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_RESUME, 0);
	if (ret < 0)
	{
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}

	return;
}

static void bw_dbg_set_tms(int argc, char **argv)
{
	INT32 number, ret;

	ret = sscanf(*argv, "%d", &number);
	if (0 == ret || number < 0)
	{
		BW_DBG_PRINT("Argument \"%s\" not a number or less than zero!\n", *argv);
		return;
	}

	if (!BwDbgInfo.BwDbgInit)
		bw_dbg_init();
	
	BwDbgInfo.TestCnt = number;

	return;
}

static ARG_CHK bw_dbg_num_preview(int argc, char **argv, char *option)
{
	int number, ret;

	if (0 == argc)
	{
		BW_DBG_PRINT("Option \"%s\": Lack of valid argument(ex: number)!\n", option);
		return DBG_ARG_INVALID;
	}

	if (argc != 1)
	{
		BW_DBG_PRINT("Option \"%s\": Only one argument admitted!\n", option);
		return DBG_ARG_INVALID;
	}
	
	ret = sscanf(*argv, "%d", &number);
	if (0 == ret || number < 0)
	{
		BW_DBG_PRINT("Option \"%s\": Argument \"%s\" not a number or less than zero!\n", option, *argv);
		return DBG_ARG_INVALID;
	}
	
	return DBG_ARG_VALID;
}

static void bw_dbg_ip_flag_set(int argc, char **argv)
{
	INT32 number, ret;

	ret = sscanf(*argv, "%x", &number);
	if (0 == ret)
	{
		BW_DBG_PRINT("Argument \"%s\" not a hexadecimal number!\n", *argv);
		return;
	}

	if (!BwDbgInfo.BwDbgInit)
		bw_dbg_init();
	
	BwDbgInfo.bw_config.ip_enable_flag = number;   //

	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_PAUSE, 0);
	if (ret < 0)
	{
		BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}

	BW_DBG_PRINT("%s,%d\n", __FUNCTION__, __LINE__);
	
	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_BW_CONFIG, &BwDbgInfo.bw_config);
    if (ret < 0)
    {
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
    }

	ret = ioctl(BwDbgInfo.fd, ALI_TEST_IP_TASK_RESUME, 0);
	if (ret < 0)
	{
        BW_DBG_PRINT("RET=%d %s,%d,%m\n", ret, __FUNCTION__, __LINE__, errno);
		return;
	}

	return;
}

static ARG_CHK bw_dbg_flag_preview(int argc, char **argv, char *option)
{
	int number, ret;

	if (0 == argc)
	{
		BW_DBG_PRINT("Option \"%s\": Lack of valid argument(ex: number)!\n", option);
		return DBG_ARG_INVALID;
	}

	if (argc != 1)
	{
		BW_DBG_PRINT("Option \"%s\": Only one argument admitted!\n", option);
		return DBG_ARG_INVALID;
	}
	
	if (!('0' == *argv[0] && 'x' == (*argv[1] | 0x20) && isxdigit(*argv[2])))
	{
		BW_DBG_PRINT("Option \"%s\": Argument should be hexadecimal number(ex: 0x1)!\n", option);
		return DBG_ARG_INVALID;
	}

	ret = sscanf(*argv, "%x", &number);
	if (0 == ret)
	{
		BW_DBG_PRINT("Option \"%s\": Argument \"%s\" not a hexadecimal number!\n", option, *argv);
		return DBG_ARG_INVALID;
	}

	return DBG_ARG_VALID;
}

 
int main(int argc, char*argv[])
{
	int test_counter = 0;
	int testtimes = 10;
	if (argc == 2){
		//printf("usage: ip_bw_test testtimes");
		testtimes = strtol(argv[1],NULL,10);
		//return -1;
	}
	
	printf("start to test ip bw.timetimes = %d \n",testtimes);
	
	bw_dbg_init();
	bw_dbg_start(0,NULL);

	for (test_counter= 0; test_counter < testtimes; test_counter++){
		bw_dbg_show_info(0,NULL);
	}
	
	bw_dbg_pause(0,NULL);
	bw_dbg_exit(0,NULL);
	printf("\ntest ip bw end.\n");
	return 0;

}
