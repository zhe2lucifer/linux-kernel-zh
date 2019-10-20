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
 *  File: ali_rpc.c
 *  (I)
 *  Description: ali Remote Process Call driver. communicate info during dual CPUs
 *			  it support paralle operation(remote call and reply together)
 *  (S)
 *  History:(M)
 ****************************************************************************/
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/ali_rpc.h>
#include <linux/version.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/ali_reg.h>
#include <ali_board_config.h>
#include <ali_soc.h>
#include "ali_rpc.h"
//===========================================================================================================//

#define SEE_RUN_ADDR                                        (__G_ALI_MM_PRIVATE_AREA_START_ADDR + 0x200)
#define OTP_SEEROM_ENABLED                            (1)
#define DUAL_MUTEX_NUM                                   (32)
#define MUTEX_UNUSED                                       (0)
#define MUTEX_USED                                           (1)
#define GET_BIT(data, n)                                     ((1<<n)&data)
#define SET_BIT(data, n, v)                                  data = ((data & (~(1<<n))) | (v<<n))

#define SYS_SEE_NB_BASE_ADDR                         (__REGALIRAW(0x18040000))
#define SEEROM_SEE_REAL_RUN_ADDR                 (SYS_SEE_NB_BASE_ADDR+0x2b0)
#define SEEROM_SEE_CODE_LOCATION_ADDR       (SYS_SEE_NB_BASE_ADDR+0x2b4)
#define SEEROM_SEE_CODE_LEN_ADDR                 (SYS_SEE_NB_BASE_ADDR+0x2b8)
#define SEEROM_SEE_SIGN_LOCATION_ADDR        (SYS_SEE_NB_BASE_ADDR+0x2bC)
#define SEEROM_DEBUG_KEY_ADDR                      (SYS_SEE_NB_BASE_ADDR+0x2C0)
#define SEEROM_SEE_RUN_STATUS_ADDR              (SYS_SEE_NB_BASE_ADDR+0x2C4)
#define SEEROM_MAIN_RUN_STATUS_ADDR            (SYS_SEE_NB_BASE_ADDR+0x2C8)
#define SEE_ROM_RUN_BIT                                   (0xBEA<<20)
#define CPU_TRIG_SW_RUN                                  ((~SEE_ROM_RUN_BIT)&0xfff00000)
#define SEE_SW_RUN_FLAG                                  (~CPU_TRIG_SW_RUN )
#define MAX_SEE_LEN                                          (1000000)/*10M.*/
#define HEAD_SIZE                                              (4)/*Byte Header: bit0-15 is length of current free block, bit16-31 is offset of next free block.*/

#define single_thread_test                                   1
#define multi_thread_test                                    2
#define see_call_main_Test                                 3

#if 1
#define ALI_RPC_CALL_LOG(...)
#else
#define ALI_RPC_CALL_LOG printk
#endif

/*switch for ali_rpc_call_function use ASM code or C code.*/
//#define CALL_LOCAL_FUNC_IN_ASM_CODE
#ifndef CALL_LOCAL_FUNC_IN_ASM_CODE
#define RPC_MAX_PARA_SIZE  8
typedef int (*PRpcFunc)(UINT32,UINT32,UINT32,UINT32,UINT32,UINT32,UINT32,UINT32);
#endif

#if defined(CONFIG_ALI_CHIP_M3921)
#define call_to_func(func, msg)  do{}while(0)
#else
#define call_to_func(func, msg) \
                                    asm volatile (".set noreorder;  \
                                    		 subu $29, 28; \
                                    		 sw $16, 0($29); \
                                    		 sw $23, 4($29); \
                                    		 sw $31, 8($29); \
                                    		 sw $4, 12($29);\
                                    		 sw $5, 16($29);\
                                    		 sw $6, 20($29);\
                                    		 sw $7, 24($29);\
                                                  or  $23, $0, $29; \
                                                  lw  $10, 0x10(%1); \
                                                  sll $10, 2;    \
                                                  subu $29,$10; \
                                                  li   $9, 0x14;\
                                                  addu $9, %1;\
                                                  or  $11, $0, $29; \
                                                  1: beqz  $10, 2f;  \
                                                  nop;             \
                                                  lw   $8, ($9);    \
                                                  sw   $8, ($11); \
                                                  addiu $11, 4;   \
                                                  addiu $9, 4;    \
                                                  b     1b;       \
                                                  addiu $10,-4;   \
                                                  2:             \
                                                  li   $9, 0x14;  \
                                                  or  $16, $0, %1; \
                                                  addu $9, %1;  \
                                                  lw    $4, ($9);  \
                                                  lw    $5, 4($9);  \
                                                  lw    $6, 8($9);  \
                                                  lw    $7, 12($9);  \
                                                  lbu   $8, 0x4($16); \
                                                  sll   $8, 2;  \
                                                  addu  %0, %0, $8; \
                                                  lw    %0, (%0);  \
                                                  jalr  %0;        \
                                                  nop;            \
                                                  or    $29, $0, $23; \
                                                  sw    $2 , 4($16); \
                                                  lw $16, 0($29); \
                                    		 lw $23, 4($29); \
                                    		 lw $31, 8($29); \
                                    		 lw $4, 12($29);\
                                    		 lw $5, 16($29);\
                                    		 lw $6, 20($29);\
                                    		 lw $7, 24($29);\
                                                  addiu $29, 28; \
                                                  nop; \
                                                  nop;\
                                                 "::"r"(func),"r"(msg):\
                                                 "$2","$4","$5","$6","$7",\
                                                 "$8","$9","$10","$11",\
                                                 "$16","$23","$29","$31")
#endif

#define ENTRY                            RPC_PRF("%s : start\n", __FUNCTION__)
#define LEAVE                            RPC_PRF("%s : exit\n", __FUNCTION__)
//===========================================================================================================//

static void g_see_boot(void);
static void print_memory(void);
extern unsigned int see_standby(unsigned int status);
extern struct SEE_BL_PARAM
{
	unsigned int see_bl_location_address;
	unsigned int see_bl_len_address;
	unsigned int see_bl_real_run_address;
	unsigned int see_signatrue_location_address;
	unsigned int see_software_enter_address;
	unsigned int see_enter_address;
}see_bl_param;

#ifdef CONFIG_ALI_STANDBY_TO_RAM
extern unsigned int ali_get_otp_value(unsigned int otp_addr);
#endif
extern void standby_dump_reg(unsigned long addr, unsigned long len);
extern INT32 ali_rpc_ci_test(UINT32 para1,UINT32 para2,UINT32 para3,UINT32 para4,UINT32 para5,UINT32 para6,UINT32 para7,UINT32 para8);
//===========================================================================================================//

static struct semaphore sem_shm;
static struct semaphore sem_write;
static unsigned long flags;
volatile struct RpcStatstics *RpcStatstics = NULL;
static unsigned int m_rpc_int_status;
static unsigned int m_rpc_int_clear;
static unsigned int m_rpc_int_set;
static unsigned int m_rpc_mailin0_mask;
static unsigned int m_rpc_mailin1_mask;
static unsigned int m_rpc_mailin0_clear;
static unsigned int m_rpc_mailin1_clear;
#define IS_MAILIN0() (GET_BYTE(m_rpc_int_status) & m_rpc_mailin0_mask)
/*end of the RPC hw inf definition.*/

static volatile UINT32 *g_dual_mutex_read;
static volatile UINT32 *g_dual_mutex_write;
static UINT8 g_dual_mutex_cnt = 0;
static UINT8 g_dual_mutex_status[DUAL_MUTEX_NUM];
static struct semaphore g_dual_mutex_local[DUAL_MUTEX_NUM];

#define IS_MUTEX_LOCKED(n) GET_BIT((*g_dual_mutex_read), n)
#define IS_MUTEX_WRITTEN(n) GET_BIT((*g_dual_mutex_write), n)

#if 0
#define LOCK_MUTEX(n) SET_BIT((*g_dual_mutex_write), n, 1)
#define UNLOCK_MUTEX(n) SET_BIT((*g_dual_mutex_write), n, 0)
#else
#define LOCK_MUTEX(n)  {\
                                       UINT32 data;\
                                       SET_BIT((*g_dual_mutex_write), n, 1);\
                                       data = GET_BIT((*g_dual_mutex_write), n);}

#define UNLOCK_MUTEX(n) {\
                                         UINT32 data;\
                                         SET_BIT((*g_dual_mutex_write), n, 0);\
                                         data = GET_BIT((*g_dual_mutex_write), n);}
#endif

void hld_dev_init(void);

static UINT32 g_remote_callee[] = {
(UINT32)ali_rpc_hld_base_callee,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
(UINT32)0,
#ifdef CONFIG_ALI_VSC
(UINT32)lld_vsc_callee,
#endif
};

static struct ali_rpc_private *m_rpc_priv = NULL;
static char driver_name[] = "alirpc";

static UINT32 *m_rpc_sm_head;

UINT32 M_ScmIsrCnt = 0;
UINT32 M_SrmIsrCnt = 0;

UINT32 M_McsReqCnt = 0;
UINT32 M_MrsReqCnt = 0;

UINT32 M_McsSetSizeCnt = 0;
UINT32 M_MrsSetSizeCnt = 0;

UINT32 M_McsSetSizeZeroCnt = 0;
UINT32 M_MrsSetSizeZeroCnt = 0;
//===========================================================================================================//

#ifdef CONFIG_ALI_STANDBY_TO_RAM
unsigned int seerom_is_enabled(void)
{
	u32 ret = 0, otp_value = 0;

	otp_value = ali_get_otp_value(0x3);
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	RPC_PRF(KERN_ERR "\nOTP03 value: 0x%08X", otp_value);
#endif

	if(0 != (otp_value & (1<<8)))
	{
		ret = OTP_SEEROM_ENABLED;
	}

	return ret;
}
#endif

#ifdef CONFIG_PM_SLEEP
int early_mcomm_resume(void)
{
	u32 resume_flag = 0, i = 0;

	if(GET_DWORD(DRAM_SPLIT_CTRL_BASE + PVT_S_ADDR) == 0)
	{
		SET_DWORD(DRAM_SPLIT_CTRL_BASE + PVT_S_ADDR, __G_ALI_MM_PRIVATE_AREA_START_ADDR&0x1fffffff);
		SET_DWORD(DRAM_SPLIT_CTRL_BASE + PVT_E_ADDR, ((__G_ALI_MM_PRIVATE_AREA_START_ADDR&0x1fffffff) \
			+ __G_ALI_MM_PRIVATE_AREA_SIZE));
		SET_DWORD(DRAM_SPLIT_CTRL_BASE + SHR_S_ADDR, __G_ALI_MM_SHARED_MEM_START_ADDR&0x1fffffff);
		SET_DWORD(DRAM_SPLIT_CTRL_BASE + SHR_E_ADDR, ((__G_ALI_MM_SHARED_MEM_START_ADDR&0x1fffffff) \
			+ __G_ALI_MM_SHARED_MEM_SIZE));
	}

	/*Set see resuming flag and reboot see.*/
	__REG32ALI(REG_STANDBY_CHECK) = SEE_IS_IN_SUSPEND;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	RPC_PRF(KERN_EMERG "\nSTR start to boot see......");
#endif
	g_see_boot();
	print_memory();

	/*Wait see resuming from STR.*/
	do{
		resume_flag = __REG32ALI(REG_STANDBY_CHECK);
	}while(resume_flag != SEE_WAKEUP_FROM_STR);

	for(i=0; i<0x900000; i++);/*Delay some time to wait for SEE boot up.*/
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	RPC_PRF(KERN_EMERG "\nSee boot done!\n");
#endif

	return 0;
}
#endif

void dump_data_dbg(char *area_name, unsigned char *bufAddr, int bufLen)
{
#if 1
	return;
#else

	int m = 0;
	int n = 0;
	unsigned char *start_addr = NULL;
	int data_len_byte = 0;

	start_addr = (unsigned char *)bufAddr;
	data_len_byte = bufLen;
	RPC_PRF("----------Dump Data Begin: area_name:%s start_addr:0x%08x, data_len_byte:0x%08x----------\n", area_name, (unsigned int)start_addr, data_len_byte);
	for(m=0; m<data_len_byte; m+=16)
	{
		RPC_PRF("[0x%08x]: ",m);
		for(n=0; n<16; n++)
		{
			if((m+n) >= data_len_byte)
			{
				break;
			}
			RPC_PRF("%02x ",*(start_addr+m+n));
		}
		RPC_PRF("\n");
	}
	RPC_PRF("----------------------------------------------Dump Data End---------------------------------------------\n");
#endif

}

#ifdef ENABLE_RPC_STATICS
INT32 RPC_ShowCnt(char *FunName, UINT32 FunLine)
{
	RPC_PRF("%s,%ld,jifi:%lx,M_ScmIsrCnt:%ld,M_SrmIsrCnt:%ld,M_McsReqCnt:%ld,M_MrsReqCnt:%ld,S_ScmReqCnt:%ld,S_SrmReqCnt:%ld,S_McsIsrCnt:%ld,S_MrsIsrCnt:%ld\n",
		FunName, FunLine, jiffies, RpcStatstics->M_ScmIsrCnt,RpcStatstics->M_SrmIsrCnt,RpcStatstics->M_McsReqCnt,RpcStatstics->M_MrsReqCnt,
		RpcStatstics->S_ScmReqCnt,RpcStatstics->S_SrmReqCnt,RpcStatstics->S_McsIsrCnt,RpcStatstics->S_MrsIsrCnt);
	return(0);
}
#endif

void ali_umemcpy(void * dest, const void *src, size_t n)
{
	int sflag = access_ok(VERIFY_READ, (void __user *)src, n);
	int dflag = access_ok(VERIFY_WRITE, (void __user *)dest, n);

	if(segment_eq(get_fs(), USER_DS))
	{
		if(sflag && !dflag)
		{
			if (copy_from_user(dest, (void __user *)src,n)){
				return;
			}
		}
		else
		{
			if(dflag && !sflag)
			{
				if (copy_to_user(dest, src, n)){
					return;
				}
			}
			else if(!sflag && !dflag)
			{
				memcpy(dest,src,n);
			}
			else
			{
				return;
			}
		}
	}
	else
	{
		memcpy(dest,src,n);
	}
}

static void dynamic_size(struct desc_storage_struct *desc, UINT32 struc)
{
	UINT32 off = 0;

	if(DESC_STRU_IS_CONSTR(desc->flag))
	{
		desc->size = strlen((char *)struc);
	}
	else if(DESC_STRU_IS_LIST(desc->flag))
	{
		off = desc->off;
		if((desc->flag & 0x30) == TYPE_SIZE_UB)
		{
			desc->size = (UINT16)(*(UINT8 *)(struc + off));
		}
		else if((desc->flag & 0x30) == TYPE_SIZE_UH)
		{
			desc->size = (UINT16)(*(UINT16 *)(struc + off));
		}
		else
		{
			desc->size = (UINT16)(*(UINT32 *)(struc + off));
		}
	}
}

static int call_ret_serialize(UINT8 *msg)
{
	struct remote_call_msg *call_msg = (struct remote_call_msg *)msg;
	struct desc_storage_struct *struct_desc;
	struct desc_pointer_struct *pointer_desc;
	struct desc_storage_struct *ret_struct_desc;
	struct desc_pointer_struct *ret_pointer_desc;
	UINT32 *para;
	UINT8 *desc;
	UINT32 desc_struct_size = 0, desc_pointer_size = 0, pointer,size = 0, off = 0, addr = 0;
	UINT32 ret_desc_struct_size = 0, ret_desc_pointer_size = 0;
	UINT32 i = 0, j = 0;

	if(call_msg->size == ((call_msg->parasize<<2) + 20))
	{
		call_msg->size = 16;
		return 0;
	}

	para = &(call_msg->para[0]);
	desc = (UINT8 *)&para[call_msg->parasize];

	//Get desc info msg buffer
	desc_struct_size = *(UINT32 *)desc;desc += 4;
	struct_desc = (struct desc_storage_struct *)desc;
	desc += desc_struct_size*sizeof(struct desc_storage_struct);
	desc_pointer_size = *(UINT32 *)desc;desc += 4;
	pointer_desc = (struct desc_pointer_struct *)desc;
	desc += desc_pointer_size*sizeof(struct desc_pointer_struct);
	ret_desc_struct_size = *(UINT32 *)desc;desc += 4;
	ret_struct_desc = (struct desc_storage_struct *)desc;
	desc += ret_desc_struct_size*sizeof(struct desc_storage_struct);
	ret_desc_pointer_size = *(UINT32 *)desc;desc += 4;
	ret_pointer_desc = (struct desc_pointer_struct *)desc;
	desc += ret_desc_pointer_size*sizeof(struct desc_pointer_struct);
	addr = (UINT32)para;

	//Process output parameter(SO->SR->PR)
	//Resume all pointers in out structure
	for(i=0; i<desc_pointer_size; i++)
	{
		if(DESC_P_IS_MEMBER(pointer_desc[i].pflag) &&
		DESC_STRU_IS_OUTPUT(struct_desc[pointer_desc[i].pind].flag))
		{
			off = struct_desc[pointer_desc[i].pind].off;
			pointer = (UINT32)(msg + off + pointer_desc[i].poff);
			*(UINT32 *)pointer = struct_desc[pointer_desc[i].sind].wp;
		}
	}

	/*Copy back desc struct for out parameter.*/
	j = 0;
	for(i = 0; i < desc_struct_size; i++){
		if(DESC_STRU_IS_OUTPUT(struct_desc[i].flag))
		{
			memcpy((void *)addr, &struct_desc[i], sizeof(struct desc_storage_struct));
			addr += sizeof(struct desc_storage_struct);
			j++;
		}
	}

	call_msg->parasize = j;
	struct_desc = (struct desc_storage_struct *)para;
	if(j == 0 && ret_desc_struct_size == 0)
	{
		call_msg->size = 16;
		RPC_PRF("%s : no ret ouput struct info \n", __FUNCTION__);
		return 0;
	}

	/*Copy back ret desc.*/
	size = 8 + ret_desc_struct_size*sizeof(struct desc_storage_struct) +
		ret_desc_pointer_size*sizeof(struct desc_pointer_struct);
	memcpy((void *)addr, (((UINT8 *)ret_struct_desc) - 4), size);
	addr += size;

	for(i=0; i<j; i++)
	{
		memcpy((void *)addr, (msg + struct_desc[i].off), struct_desc[i].size);
		struct_desc[i].off = addr - (UINT32)msg;
		addr += struct_desc[i].size;
	}

	for(i=0; i<ret_desc_pointer_size; i++)
	{
		/*Process structure pointed by ret pointer.*/
		if(DESC_P_IS_RET(ret_pointer_desc[i].pflag))
		{
			dynamic_size(&ret_struct_desc[ret_pointer_desc[i].sind], (UINT32)(msg + 4));
			if(((struct remote_response_msg *)msg)->ret == 0)
			{
				ret_struct_desc[ret_pointer_desc[i].sind].size = 0;
			}
			size = ret_struct_desc[ret_pointer_desc[i].sind].size;
			memcpy((void *)addr, (void *)(((struct remote_response_msg *)msg)->ret), size);
			((struct remote_response_msg *)msg)->ret = (addr - (UINT32)msg);
			addr += size;
			if(addr & 0x3)
			{
				addr = (addr + 0x3) & (~0x3);
			}
		}

		/*Process structure pointed by other pointers.*/
		if(DESC_P_IS_MEMBER(ret_pointer_desc[i].pflag))
		{
			off = ret_struct_desc[ret_pointer_desc[i].pind].off;
			dynamic_size(&ret_struct_desc[ret_pointer_desc[i].sind], (UINT32)(msg + off));
			pointer = (UINT32)(msg + off + ret_pointer_desc[i].poff);
			if(pointer == 0)
			{
				ret_struct_desc[ret_pointer_desc[i].sind].size = 0;
			}
			size = ret_struct_desc[ret_pointer_desc[i].sind].size;
			memcpy((void *)addr, (void *)(*(UINT32 *)pointer), size);
			ret_struct_desc[ret_pointer_desc[i].sind].off = (addr - (UINT32)msg);
			*(UINT32 *)pointer = (addr - (UINT32)msg);
			addr += size;
			if(addr & 0x3)
			{
				addr = (addr + 0x3) & (~0x3);
			}
		}
	}

	call_msg->size = addr - (UINT32)msg;
	if(call_msg->size > CALL_MSG_BUFFER_SIZE)
	{
		return (-1);
	}

	return 0;
}

static void call_ret_unserialize(UINT8 *msg)
{
	struct remote_response_msg *response_msg = (struct remote_response_msg *)msg;
	struct desc_storage_struct *out_struct_desc;
	struct desc_storage_struct *ret_struct_desc;
	struct desc_pointer_struct *ret_pointer_desc;
	UINT8 *desc;
	UINT8 *addr;
	UINT32 pointer = 0, size = 0, off = 0;
	UINT32 ret_desc_struct_size = 0, ret_desc_pointer_size = 0;
	UINT32 i = 0;

	if(response_msg->size <= 16)
	{
		return;
	}

	addr = (msg + sizeof(struct remote_response_msg));
	size = *(UINT32 *)addr;
	addr += 4;
	desc = addr;
	out_struct_desc = (struct desc_storage_struct *)desc;

	/*Write back out parameters.*/
	for(i=0; i<size; i++)
	{
		ali_umemcpy((void *)out_struct_desc[i].wp, (msg + out_struct_desc[i].off), out_struct_desc[i].size);
	}
	desc += size*sizeof(struct desc_storage_struct);

	/*Get desc size and desc about ret pointers.*/
	ret_desc_struct_size = *(UINT32 *)desc;
	desc += 4;
	ret_struct_desc = (struct desc_storage_struct *)desc;
	desc += ret_desc_struct_size*sizeof(struct desc_storage_struct);
	ret_desc_pointer_size = *(UINT32 *)desc;
	desc += 4;
	ret_pointer_desc = (struct desc_pointer_struct *)desc;
	desc += ret_desc_pointer_size*sizeof(struct desc_pointer_struct);
	addr = desc;

	/*Process all pointers.*/
	for(i=0; i<ret_desc_pointer_size; i++)
	{
		if(DESC_P_IS_RET(ret_pointer_desc[i].pflag))
		{
			((struct remote_response_msg *)msg)->ret += (UINT32)msg;
		}
		else
		{
			off = ret_struct_desc[ret_pointer_desc[i].pind].off;
			pointer = (UINT32)(msg + off + ret_pointer_desc[i].poff);
			*(UINT32 *)pointer += (UINT32)msg;
		}
	}
}

static int call_para_serialize(UINT8 *msg, UINT8 *desc, UINT32 *fp, UINT32 funcdesc, int flag_idx)
{
	struct remote_call_msg *call_msg;
	struct desc_storage_struct *struct_desc;
	struct desc_pointer_struct *pointer_desc;
	struct desc_storage_struct *ret_struct_desc;
	struct desc_pointer_struct *ret_pointer_desc;
	UINT32 *para;
	UINT32 addr = 0;
	UINT32 desc_struct_size = 0, desc_pointer_size = 0, pointer = 0, size = 0, off = 0;
	UINT32 ret_desc_struct_size = 0, ret_desc_pointer_size = 0;
	UINT32 parasize = 0;
	UINT32 i = 0;

	call_msg = (struct remote_call_msg *)msg;
	*(UINT32 *)(msg + 4) = funcdesc;

	/*Set caller tcb and retbuf.*/
	call_msg->caller = (UINT32)flag_idx;
	call_msg->retbuf = (UINT32)msg;
	call_msg->parasize = parasize = (funcdesc>>16)&0xff;

#ifndef CALL_LOCAL_FUNC_IN_ASM_CODE
	if(call_msg->parasize > RPC_MAX_PARA_SIZE)
	{
		RPC_PRF("error: call_para_serialize, para size:%d can not be more than %d!!!\n", call_msg->size, RPC_MAX_PARA_SIZE);
		return (-1);
	}
#endif

	/*Serialize parameters from frame stack.*/
	para = &call_msg->para[0];
	para[0] = fp[1];
	para[1] = fp[2];
	para[2] = fp[3];
	para[3] = fp[4];

#if defined(CONFIG_ARM)
	for(i=4; i<parasize; i++)
	{
		para[i] = ((UINT32 *)(fp[0]))[i-4];
	}
#else
	for(i=4; i<parasize; i++)
	{
		para[i] = ((UINT32 *)fp[0])[i];
	}
#endif

	if(desc == NULL)
	{
		call_msg->size = (parasize<<2) + 20;
		RPC_PRF("call_msg->size:%d, no desc info\n",call_msg->size);
		return 0;
	}

	/*Copy desc info msg buffer.*/
	addr = (UINT32)&para[parasize];off = 0;
	desc_struct_size = *(UINT32 *)desc;off += 4;
	struct_desc = (struct desc_storage_struct *)(addr + off);
	off += desc_struct_size*sizeof(struct desc_storage_struct);
	desc_pointer_size = *(UINT32 *)(desc + off);off += 4;
	pointer_desc = (struct desc_pointer_struct *)(addr + off);
	off += desc_pointer_size*sizeof(struct desc_pointer_struct);
	ret_desc_struct_size = *(UINT32 *)(desc + off);off += 4;
	ret_struct_desc = (struct desc_storage_struct *)(addr + off);
	off += ret_desc_struct_size*sizeof(struct desc_storage_struct);
	ret_desc_pointer_size = *(UINT32 *)(desc + off);off += 4;
	ret_pointer_desc = (struct desc_pointer_struct *)(addr + off);
	off += ret_desc_pointer_size*sizeof(struct desc_pointer_struct);

	memcpy((void *)addr, desc, off);
	addr += off;

	/*Serialize pointers.*/
	for(i=0; i<desc_pointer_size; i++)
	{
		if(DESC_P_IS_PARA(pointer_desc[i].pflag))
		{
			/*Process structure pointed by parameter pointers.*/
			dynamic_size(&struct_desc[pointer_desc[i].sind], (UINT32)para);
			if(para[pointer_desc[i].pind] == 0)
			{
				struct_desc[pointer_desc[i].sind].size = 0;
			}
			size = struct_desc[pointer_desc[i].sind].size;
			ali_umemcpy((void *)addr, (void *)(para[pointer_desc[i].pind]), size);
			struct_desc[pointer_desc[i].sind].wp = para[pointer_desc[i].pind];
			struct_desc[pointer_desc[i].sind].off = (addr - (UINT32)msg);
			para[pointer_desc[i].pind] = (addr - (UINT32)msg);
			addr += size;
			if(addr & 0x3)
			{
				addr = (addr + 0x3) & (~0x3);
			}
		}
		else
		{
			/*Process structure pointed by other pointers.*/
			off = struct_desc[pointer_desc[i].pind].off;
			dynamic_size(&struct_desc[pointer_desc[i].sind], (UINT32)(msg + off));
			pointer = *(UINT32 *)(msg + off + pointer_desc[i].poff);
			if(pointer == 0)
			{
				struct_desc[pointer_desc[i].sind].size = 0;
			}
			size = struct_desc[pointer_desc[i].sind].size;
			ali_umemcpy((void *)addr, (void *)pointer, size);
			*(UINT32 *)(msg + off + pointer_desc[i].poff) = addr - (UINT32)msg;
			struct_desc[pointer_desc[i].sind].wp = pointer;
			struct_desc[pointer_desc[i].sind].off = (addr - (UINT32)msg);
			addr += size;
			if(addr & 0x3)
			{
				addr = (addr + 0x3) & (~0x3);
			}
		}
	}

	call_msg->size = addr - (UINT32)msg;
	if(call_msg->size > CALL_MSG_BUFFER_SIZE)
	{
		return (-1);
	}

	return 0;
}

static void call_para_unserialize(UINT8 *msg)
{
	struct remote_call_msg *call_msg = (struct remote_call_msg *)msg;
	struct desc_storage_struct *struct_desc;
	struct desc_pointer_struct *pointer_desc;
	UINT32 *para;
	UINT8 *addr;
	UINT32 desc_struct_size = 0, desc_pointer_size = 0, pointer = 0, off = 0;
	UINT32 i = 0;
	UINT32 parasize = 0;

	parasize = call_msg->parasize;
	para = &(call_msg->para[0]);
	if(call_msg->size == ((parasize<<2) + 20))
	{
		return;
	}

	addr = (UINT8 *)&para[call_msg->parasize];
	/*Get desc size and desc about pointers.*/
	desc_struct_size = *(UINT32 *)addr;addr += 4;
	struct_desc = (struct desc_storage_struct *)addr;
	addr += desc_struct_size*sizeof(struct desc_storage_struct);

	desc_pointer_size = *(UINT32 *)addr;addr += 4;
	pointer_desc = (struct desc_pointer_struct *)addr;
	addr += desc_pointer_size*sizeof(struct desc_pointer_struct);

	/*Process all pointers.*/
	for(i=0; i<desc_pointer_size; i++)
	{
		if(pointer_desc[i].pflag == 0)
		{
			para[pointer_desc[i].pind] += (UINT32)msg;
		}
		else
		{
			off = struct_desc[pointer_desc[i].pind].off;
			pointer = (UINT32)(msg + off + pointer_desc[i].poff);
			*(UINT32 *)pointer += (UINT32)msg;
		}
	}
}

static void read_back_check(UINT8 *addr, UINT8 v)
{
	//read back to make sure that the value is written into memory,
	//otherwise the other CPU could read the old value
	//--Michael Xie 2010/3/9
	if(*(volatile UINT8 *)(addr) != v)
	{
		RPC_PRF("rpc read back check error\n");
	}
}

static int get_free_out_buf(void)
{
	int i = 0;

	for(i=0; i<RPC_NUM; i++)
	{
		if(m_rpc_priv->out_msg_buf_valid[i] == 1)
		{
			break;
		}
	}

	if(i >= RPC_NUM)
	{
		return (-1);
	}

	m_rpc_priv->out_msg_buf_valid[i] = 0;
	return i;
}

#define MAILBOX_TIME_OUT_10S  msecs_to_jiffies(10000)
static inline int wait_read_ready(unsigned long addr)
{
	volatile unsigned long j = MAILBOX_TIME_OUT_10S + jiffies;

	while(GET_DWORD(addr) == MSG_RECEIVED){
		if(time_after(jiffies,j))
		{
			pr_crit( "-->%s[%d] tmo:%lu\n",\
				__func__,__LINE__,MAILBOX_TIME_OUT_10S);
			return 0;
		}
		RPC_PRF("wait read\n");
	}

	return 1;
}

static inline int wait_write_finish(unsigned long addr)
{
	volatile unsigned long j = MAILBOX_TIME_OUT_10S + jiffies;

	while(GET_DWORD(addr) != MSG_RECEIVED)
	{
		if(time_after(jiffies,j))
		{
			pr_crit("-->%s[%d] tmo:%lu\n",\
				__func__,__LINE__,MAILBOX_TIME_OUT_10S);
			return 0;
		}
	}

	return 1;
}

static int remote_read(int path, unsigned long buf, unsigned long *buf_ret)
{
	unsigned long addr = (path == 0) ? MAILIN0_ADDR : MAILIN1_ADDR;
	unsigned long size = 0;
	unsigned long i = 0;
	int ret = 0;

	ENTRY;

	if(path == 1)
	{
		buf = (unsigned long)GET_DWORD(MAILIN0_BUF + 12);
		*buf_ret = buf;
	}

	if(GET_DWORD(addr) == MSG_RECEIVED)
	{
		return (-1);
	}

	do{
		if(!wait_read_ready(addr))
		{
			ret = (-1);
			goto EXIT;
		}

		size = GET_DWORD(addr);
		RPC_PRF("read size %d\n", (int)size);
		if(size)
		{
			memcpy((void *)(buf + i), (void *)MAILIN0_BUF, size);
		}

		SET_DWORD(addr, MSG_RECEIVED);
		if(size < MSG_BUFFER_SIZE)
		{
			i += size;
			break;
		}
		i += size;
	}while(1);

EXIT:
	LEAVE;

	return ret;
}

static int remote_write(int path, unsigned long buf)
{
	struct remote_call_msg *msg = (struct remote_call_msg *)buf;
	unsigned long addr = path == 0 ? MAILOUT0_ADDR : MAILOUT1_ADDR;
	unsigned long bit = path == 0 ? m_rpc_mailin0_mask : m_rpc_mailin1_mask;
	unsigned long len = msg->size, size = 0;
	unsigned long i = 0;
	int ret = 0;

	if(down_interruptible(&sem_write))
	{
		RPC_PRF("rpc_call down sem fail\n");
		return (-1);
	}
	ENTRY;

	do{
		size = (len < MSG_BUFFER_SIZE) ? len : MSG_BUFFER_SIZE;
		if (MSG_RECEIVED == size)
		{
			RPC_PRF("%s,%d,j:%lx,path:%d,M_ScmIsrCnt:%ld,M_SrmIsrCnt:%ld,M_McsReqCnt:%ld,M_MrsReqCnt:%ld,M_McsSetSizeCnt:%ld,M_MrsSetSizeCnt:%ld\n",
				 __FUNCTION__, __LINE__, jiffies, path, M_ScmIsrCnt, M_SrmIsrCnt, M_McsReqCnt, M_MrsReqCnt, M_McsSetSizeCnt, M_MrsSetSizeCnt);
		}

		memcpy((void *)MAILOUT0_BUF, (void *)(buf + i), size);
		read_back_check((UINT8 *)(MAILOUT0_BUF+size-1), *((UINT8 *)buf+i+size-1));

#ifdef ENABLE_RPC_STATICS
		if (0 == path)
		{
			//printk("M_McsReqCnt:%x,j:%x\n", ++M_McsReqCnt, jiffies);
			M_McsSetSizeCnt++;
		}
		else
		{
			M_MrsSetSizeCnt++;
			//printk("M_MrsReqCnt:%x,j:%x\n", ++M_MrsReqCnt, jiffies);
		}
#endif

		SET_DWORD(addr, size);
		RPC_PRF("write size %d add %x back %d\n", (int)size, (int)addr, *(int *)addr);
		dump_data_dbg("main_send_data", (unsigned char*)MAILOUT0_BUF, size);

		if(i == 0)
		{
#ifdef ENABLE_RPC_STATICS
			if (0 == path)
			{
				//printk("M_McsReqCnt:%d,j:%x\n", M_McsReqCnt, jiffies);
				M_McsReqCnt++;
				RpcStatstics->M_McsReqCnt++;
			}
			else
			{
				//printk("M_MrsReqCnt:%d,j:%x\n", M_MrsReqCnt, jiffies);
				M_MrsReqCnt++;
				RpcStatstics->M_MrsReqCnt++;
			}
#endif
			SETB_BYTE(m_rpc_int_set, bit);
		}

		/*wait write finish.*/
		if(!wait_write_finish(addr))
		{
			ret = (-1);
			RPC_PRF("%s,%d,j:%lx,path:%d,M_ScmIsrCnt:%ld,M_SrmIsrCnt:%ld,M_McsReqCnt:%ld,M_MrsReqCnt:%ld,M_McsSetSizeCnt:%ld,M_MrsSetSizeCnt:%ld\n",
				__FUNCTION__, __LINE__, jiffies, path, M_ScmIsrCnt, M_SrmIsrCnt, M_McsReqCnt, M_MrsReqCnt, M_McsSetSizeCnt, M_MrsSetSizeCnt);
			goto EXIT;
		}

		i += size;
		len -= size;
	}while(len);

	if(size == MSG_BUFFER_SIZE)
	{
		/*Never happen.*/
		if (0 == path)
		{
			//printk("M_McsReqCnt:%x,j:%x\n", ++M_McsReqCnt, jiffies);
			M_McsSetSizeZeroCnt++;

		}
		else
		{
			M_MrsSetSizeZeroCnt++;
			//printk("M_MrsReqCnt:%x,j:%x\n", ++M_MrsReqCnt, jiffies);
		}

		/*transfer the msg with 0 size to finish this call.*/
		SET_DWORD(addr, 0);

		/*waitting call request cosumed.*/
		if(!wait_write_finish(addr))
		{
			ret = -1;
			goto EXIT;
		}
	}

EXIT:
	LEAVE;
	up(&sem_write);

	return ret;
}

void ali_rpc_disable(void)
{
	if(down_interruptible(&sem_write))
	{
		RPC_PRF("%s : down sem fail\n", __FUNCTION__);
		return;
	}

	while(GET_DWORD(MAILIN0_ADDR) != RPC_DISABLE)
	{
		msleep_interruptible(1);
	}

	SET_DWORD(MAILIN0_ADDR, MSG_RECEIVED);
}

void ali_rpc_enable(void)
{
	up(&sem_write);
}

static inline void dump_rpc_call_header(UINT8 *msg)
{
	pr_crit("rpc header: 0x%08x 0x%08x 0x%08x 0x%08x\n",\
		(int)(*(UINT32*)(msg)),\
		(int)(*(UINT32*)(msg+4)),\
		(int)(*(UINT32*)(msg+8)),\
		(int)(*(UINT32*)(msg+12))
		);
}


#define REMOTE_CALL_TIME_OUT_20S			msecs_to_jiffies(20000)
static int remote_call(void *buf, int flag_idx)
{
	int ret = 0;

	ENTRY;
	m_rpc_priv->rpc_flag[flag_idx] &= ~MAILBOX_GET_REMOTE_RET;

	/*call the remote device.*/
	ret = remote_write(0, (unsigned long)buf);
	if(ret < 0)
	{
		RPC_PRF("%s : remote write fail %d\n", __FUNCTION__, ret);
		pr_crit("%s:%d - 0x%X\n", __func__, __LINE__, ret);
		dump_rpc_call_header(buf);
		goto EXIT;
	}

WAIT_RPC_RET:
	/*wait ret from the remote device.*/
	RPC_PRF("wait remote ret isr... \n");
	ret = wait_event_interruptible_timeout(m_rpc_priv->wait_call[flag_idx],\
		(m_rpc_priv->rpc_flag[flag_idx] & MAILBOX_GET_REMOTE_RET), REMOTE_CALL_TIME_OUT_20S);

	/*it may be interrupted by signal.*/
	ret = m_rpc_priv->rpc_flag[flag_idx] & MAILBOX_GET_REMOTE_RET ? 1 : (-1);
	if(ret < 0)
	{
		pr_crit("wait event fail %d, tmo:%lu\n", ret,REMOTE_CALL_TIME_OUT_20S);
		dump_rpc_call_header(buf);
		goto WAIT_RPC_RET;
	}

	RPC_PRF("remote ret isr is coming\n");
	

	EXIT:
	LEAVE;
	return ret;
}

static int remote_ret(void *buf)
{
	struct remote_response_msg *msg = (struct remote_response_msg *)buf;
	int ret = 0;

	msg->flag = RPC_MSG_RETURN;
	ret = remote_write(1, (unsigned long)buf);
	if(ret < 0)
	{
		RPC_PRF("%s : remote write fail %d \n", __FUNCTION__, ret);
		goto EXIT;
	}

EXIT:
	return ret;
}

static int local_call(void *buf)
{
	NORMAL_CALLEE callee;
	UINT32 data = 0;
	int ret = 0;

	RPC_PRF("\n");
	ENTRY;

	data = *(UINT32 *)(buf + 4);
	RPC_PRF("func des %x\n", (int)data);
	callee = (NORMAL_CALLEE)g_remote_callee[data>>24];
	if(callee != NULL)
	{
		callee(buf);
	}
	RPC_PRF("call done\n");

	LEAVE;
	RPC_PRF("\n");

	return ret;
}

int get_free_rpc_thread(void)
{
	int index = 0;
	int free_index = (-1);
	int retry = 5;

	while(retry>0)
	{
		for(index=0;index<RPC_NUM; index++)
		{
			if(m_rpc_priv->flag[index] == ~MAILBOX_GET_REMOTE_CALL)
			{
				free_index = index;
				m_rpc_priv->flag[index] = MAILBOX_GET_REMOTE_CALL;
				break;
			}
		}
		
		if(free_index != -1)
		{
			break;
		}
		retry --;
		printk("all rpc threads are busy, try again!!!\n");
	}

	return free_index;	
}

static void rpc_tasklet_routine(unsigned long par)
{
	int ret = 0;
	int free_thd = -1;
	unsigned long  buf_ret = 0;
	
	if(m_rpc_priv->isr_call)
	{
		m_rpc_priv->isr_call = 0;
		RPC_PRF("wake up the call thread\n");

		free_thd = get_free_rpc_thread();
		RPC_PRF("get free rpc thread index = %x\n", free_thd);
		if(free_thd != -1)
		{				
			ret = remote_read(0, (unsigned long)m_rpc_priv->in_msg_buf[free_thd], NULL);
			if(ret < 0)
			{
				pr_crit("remote_read call fail\n");
				return;
			}
			wake_up(&m_rpc_priv->wait_call[free_thd]);
		}
		else
		{
			pr_crit("all rpc threads are busy!!!\n");
		}
	}

	if(m_rpc_priv->isr_ret)
	{
		m_rpc_priv->isr_ret = 0;
		ret = remote_read(1, 0, &buf_ret);
		if(ret < 0)
		{
			pr_crit("remote_read ret fail %d\n", ret);
		}
		else
		{
			struct remote_response_msg *remote_ret_msg = (struct remote_response_msg *)buf_ret;
			m_rpc_priv->rpc_flag[remote_ret_msg->caller] |= MAILBOX_GET_REMOTE_RET;
			wake_up_interruptible(&m_rpc_priv->wait_call[remote_ret_msg->caller]);
		}
	}
}

static struct tasklet_struct rpc_tasklet = {
	NULL,
	0,
	ATOMIC_INIT(0),
	rpc_tasklet_routine,
	0
};

void rpc_show_reg(void)
{

#ifndef CONFIG_ALI_CHIP_M3921
	unsigned int c0_status = 0;
	unsigned int c0_cause = 0;
	unsigned int reg_38 = 0;
	unsigned int reg_3c = 0;
	unsigned int reg_ec = 0;

	c0_status = read_c0_status();
	c0_cause = read_c0_cause();
	reg_38 = *((unsigned long *)(__REGALIRAW(0x18000038)));
	reg_3c = *((unsigned long *)(__REGALIRAW(0x1800003C)));
	reg_ec = *((unsigned long *)(__REGALIRAW(0x180000EC)));

	RPC_PRF("M,c0_status:%x,c0_cause:%x,reg_38:%x,reg_3c:%x,reg_ec:%x,*sys_rpc_addr:%x\n\n",
		c0_status, c0_cause, reg_38, reg_3c, reg_ec, /**(volatile unsigned char *)(sys_rpc_addr)*/0);
#endif
	return ;
}

static irqreturn_t rpc_call_isr(int irq, void *dev_id)
{
	UINT32 bit = 0;

	if(IS_MAILIN0())
	{
		RPC_PRF("rpc call isr ok\n");
		bit = m_rpc_mailin0_clear;
	}
	else
	{
		RPC_PRF("%s,%d,j:%lx,M_ScmIsrCnt:%ld,M_SrmIsrCnt:%ld,M_McsReqCnt:%ld,M_MrsReqCnt:%ld,M_McsSetSizeCnt:%ld,M_MrsSetSizeCnt:%ld\n",
			 __FUNCTION__, __LINE__, jiffies, M_ScmIsrCnt, M_SrmIsrCnt, M_McsReqCnt, M_MrsReqCnt, M_McsSetSizeCnt, M_MrsSetSizeCnt);
		return IRQ_NONE;
	}

	//printk("M_ScmIsrCnt:%d,j:%x\n", M_ScmIsrCnt, jiffies);
#ifdef ENABLE_RPC_STATICS
	M_ScmIsrCnt++;

	if ((RpcStatstics->M_ScmIsrCnt + 1) != RpcStatstics->S_ScmReqCnt)
	{
		RPC_ShowCnt((char *)__FUNCTION__, __LINE__);
		RPC_PRF("G_M_ScmIsrCnt:%ld\n", M_ScmIsrCnt);
		rpc_show_reg();
		SET_BYTE(m_rpc_int_clear, bit);
		return IRQ_HANDLED;
	}
#endif

	SET_BYTE(m_rpc_int_clear, bit);
	m_rpc_priv->isr_call = 1;
#ifdef ENABLE_RPC_STATICS
	RpcStatstics->M_ScmIsrCnt++;
#endif
	tasklet_schedule(&rpc_tasklet);
	return IRQ_HANDLED;
}

static irqreturn_t rpc_ret_isr(int irq, void *dev_id)
{
	UINT32 bit = 0;

#ifdef ENABLE_RPC_STATICS
	UINT8 value_0xb8040037 = 0;
	UINT8 value_0xb8000037_1 = 0;
	UINT8 value_0xb8000037_2 = 0;
#endif
	if(!IS_MAILIN0())
	{
		RPC_PRF("rpc ret isr ok\n");
		bit = m_rpc_mailin1_clear;
	}
	else
	{
		RPC_PRF("%s,%d,j:%lx,,M_ScmIsrCnt:%ld,M_SrmIsrCnt:%ld,M_McsReqCnt:%ld,M_MrsReqCnt:%ld,M_McsSetSizeCnt:%ld,M_MrsSetSizeCnt:%ld\n",
			__FUNCTION__, __LINE__, jiffies, M_ScmIsrCnt, M_SrmIsrCnt, M_McsReqCnt, M_MrsReqCnt, M_McsSetSizeCnt, M_MrsSetSizeCnt);

		return IRQ_NONE;
	}
#ifdef ENABLE_RPC_STATICS
	//printk("M_SrmIsrCnt:%d,j:%x\n", M_SrmIsrCnt, jiffies);
	M_SrmIsrCnt++;

	if ((RpcStatstics->M_SrmIsrCnt + 1) != RpcStatstics->S_SrmReqCnt)
	{
		RPC_ShowCnt((char *)__FUNCTION__, __LINE__);
		RPC_PRF("G_M_SrmIsrCnt:%ld\n", M_SrmIsrCnt);
		rpc_show_reg();
		SET_BYTE(m_rpc_int_clear, bit);
		return IRQ_HANDLED;
	}
#endif

#ifndef ENABLE_RPC_STATICS
	SET_BYTE(m_rpc_int_clear, bit);
#else
	value_0xb8000037_1 = GET_BYTE(__REGALIRAW(0x18000037));
	SET_BYTE(m_rpc_int_clear, bit);
	value_0xb8000037_2 = GET_BYTE(__REGALIRAW(0x18000037));
	value_0xb8040037 = GET_BYTE(__REGALIRAW(0x18040037));
	if ((value_0xb8040037 & 0x4) != 0)
	{
		RPC_PRF("%s,%d,j:%lx,value_0xb8000037_1:%x,value_0xb8000037_2:%x,value_0xb8040037:%x\n",
			__FUNCTION__, __LINE__, jiffies, value_0xb8000037_1, value_0xb8000037_2, value_0xb8040037);
	}
#endif
	
	m_rpc_priv->isr_ret = 1;
#ifdef ENABLE_RPC_STATICS
	RpcStatstics->M_SrmIsrCnt++;
#endif

	tasklet_schedule(&rpc_tasklet);
	return IRQ_HANDLED;
}

static void rpc_mutex_init(void)
{
	int i = 0;

	sema_init(&m_rpc_priv->sem_mutex, 1);

	g_dual_mutex_read = (volatile UINT32 *)(READ_DUAL_MUTEX_ADDR);
	g_dual_mutex_write = (volatile UINT32 *)(WRITE_DUAL_MUTEX_ADDR);
	*g_dual_mutex_read = 0;
	*g_dual_mutex_write = 0;
	g_dual_mutex_cnt = 0;
	memset(g_dual_mutex_status, MUTEX_UNUSED, DUAL_MUTEX_NUM);

	for(i=0; i<DUAL_MUTEX_NUM; i++)
	{
		sema_init(&g_dual_mutex_local[i], 1);
	}
}

void noinline ali_rpc_call_function(unsigned long entry_func, void *msg)
{
#ifdef CALL_LOCAL_FUNC_IN_ASM_CODE
	register unsigned long par1 asm ("$14");
	register unsigned long par2 asm ("$15");
	par1 = entry_func;
	par2 = (unsigned long)msg;
	call_to_func(par1, par2);
#else
	struct remote_call_msg *call_msg = (struct remote_call_msg *)msg;
	UINT32 tmp_func_para[RPC_MAX_PARA_SIZE];
	UINT32 total_para_size = call_msg->parasize;
	UINT32 *start_para_ptr = (UINT32*)(&call_msg->para[0]);
	UINT32 i = 0;
	INT32 ret = 0;
	PRpcFunc local_func;

	if(total_para_size > RPC_MAX_PARA_SIZE)
	{
		RPC_PRF("Fail: ali_rpc_call_function, para size:%d can not be more than %d!!!\n", total_para_size, RPC_MAX_PARA_SIZE);
		return;
	}

	for(i = 0; i < total_para_size; i++)
	{
		tmp_func_para[i] = *(start_para_ptr + i);
	}

	local_func = (PRpcFunc)(*(UINT32*)(entry_func + call_msg->func*sizeof(UINT32)));
	ret = local_func(tmp_func_para[0], tmp_func_para[1], tmp_func_para[2], tmp_func_para[3],\
		tmp_func_para[4], tmp_func_para[5], tmp_func_para[6], tmp_func_para[7]);
	((struct remote_response_msg *)msg)->ret = ret;
#endif
}

void ali_rpc_spinlock(int mutex_id)
{
	unsigned long val = 0;

	preempt_disable();
	local_irq_save(flags);
	while(*(volatile UINT32 *)(__REGALIRAW(0x18041000)) == 1)
	val = *(volatile UINT32 *)(__REGALIRAW(0x18000000));
}
EXPORT_SYMBOL(ali_rpc_spinlock);

void ali_rpc_spinunlock(int mutex_id)
{
	unsigned long val = 0;

	*(volatile UINT32 *)(__REGALIRAW(0x18041000)) = 0;
	val = *(volatile UINT32 *)(__REGALIRAW(0x18000000));
	local_irq_restore(flags);
	preempt_enable();
}
EXPORT_SYMBOL(ali_rpc_spinunlock);

/* Name :
			ali_rpc_mutex_create
    Description :
			create the rpc mutex for the communication during the dual CPUs
    Arguments :
    			void
    Return value:
    			<= 0 	---> fail
    			others 	---> mutex id
*/
int ali_rpc_mutex_create(void)
{
	int id = (-1);
	int i = 0;

	if(down_interruptible(&m_rpc_priv->sem_mutex))
	{
		RPC_PRF("%s : down sem fail\n", __FUNCTION__);
		return (-1);
	}

	if(g_dual_mutex_cnt >= DUAL_MUTEX_NUM)
	{
		goto EXIT;
	}

	for(i=0; i<DUAL_MUTEX_NUM; i++)
	{
		if(g_dual_mutex_status[i] == MUTEX_UNUSED)
		{
			g_dual_mutex_cnt++;
			g_dual_mutex_status[i] = MUTEX_USED;
			id = i + 1;
			break;
		}
	}

	if(id > 0)
	{
		ali_rpc_spinlock(0);
		SET_BIT(*g_dual_mutex_read, i, 0);
		SET_BIT(*g_dual_mutex_write, i, 0);
		ali_rpc_spinunlock(0);
	}

EXIT:
	up(&m_rpc_priv->sem_mutex);
	return id;
}
EXPORT_SYMBOL(ali_rpc_mutex_create);

/* Name :
			ali_rpc_mutex_delete
    Description :
			delete the rpc mutex
    Arguments :
    			void
    Return value:
    			<= 0 	---> fail
    			others 	---> ok
*/
int ali_rpc_mutex_delete(int mutex_id)
{
	int id = mutex_id - 1;

	if(g_dual_mutex_status[id] != MUTEX_USED)
	{
		return (-1);
	}

	if(down_interruptible(&m_rpc_priv->sem_mutex))
	{
		RPC_PRF("%s : down sem fail\n", __FUNCTION__);
		return (-1);
	}

	g_dual_mutex_status[id] = MUTEX_UNUSED;
	g_dual_mutex_cnt--;

	up(&m_rpc_priv->sem_mutex);
	return 0;
}
EXPORT_SYMBOL(ali_rpc_mutex_delete);

/*
Another solution to do dual cpu procetion without any hardware help
cpu 1         cpu 2
w11+r1+r2   w21+r2+r1
if(cpu 1 get mutex)
  r1 = 1 & r2 = 0
*/
/*
Local exclusive access by local mutex
To do:
wake up remote processor by interrupt?
*/
int ali_rpc_mutex_lock(int mutex_id, int timeout)
{
	int id = mutex_id - 1;

	if(down_interruptible(&g_dual_mutex_local[id]))
	{
		RPC_PRF("%s : down sem fail\n", __FUNCTION__);
		return (-1);
	}

	while(1)
	{
		ali_rpc_spinlock(0);
		if(IS_MUTEX_WRITTEN(id))
		{
			ali_rpc_spinunlock(0);
			msleep_interruptible(1);
			continue;
		}
		LOCK_MUTEX(id);
		ali_rpc_spinunlock(0);
		break;
	}

	return 0;
}

EXPORT_SYMBOL(ali_rpc_mutex_lock);

/* Name :
			ali_rpc_mutex_unlock
    Description :
			unlock the rpc mutex
    Arguments :
    			int mutex_id ---> id of the rpc mutex
    Return value:
    			<= 0 	---> fail
    			others 	---> ok
*/
int ali_rpc_mutex_unlock(int mutex_id)
{
	int id = mutex_id - 1;

	ali_rpc_spinlock(0);
	UNLOCK_MUTEX(id);
	ali_rpc_spinunlock(0);
	up(&g_dual_mutex_local[id]);
	return 0;
}
EXPORT_SYMBOL(ali_rpc_mutex_unlock);

/* Name :
			ali_rpc_malloc_shared_mm
    Description :
			malloc the shared memory for the comminuction during the dual CPUs
    Arguments :
    			unsigned long size ---> size of the needed buffer
    Return value:
    			NULL 	---> fail
    			others 	---> the shared memory buffer start address
*/
void *ali_rpc_malloc_shared_mm(unsigned long size)
{
	UINT32 flen = 0, off = 0;
	UINT32 *ptr, *prev, *rptr;
	UINT32 *fptr, *fprev;
	UINT32 head = 0;
	
	if(!size)
	{
		return NULL;
	}

	down(&sem_shm);

	head = m_rpc_sm_head[0];
	/*Alignment by DWORD.*/
	size = (size + 3) & (~0x3);

	/*Allocate shared memory in free list, fit block first, head block last.*/
	fptr = fprev = NULL;
	prev = m_rpc_sm_head;
	ptr = (UINT32 *)((head>>16) + ALLOC_SM_ADDR);
	while(1)
	{
		head = ptr[0];
		flen = head&0xffff;
		if(flen == size)
		{
			fptr = ptr;
			fprev = prev;

			break;
		}

		if(flen >= size + HEAD_SIZE)
		{
			fptr = ptr;
			fprev = prev;

			break;
		}

		if(ptr == m_rpc_sm_head)
		{
			if(fptr == NULL)
			{
				ptr = NULL;
				goto out;
			}

			break;
		}
		prev = ptr;
		ptr = (UINT32 *)((head>>16) + ALLOC_SM_ADDR);
	}

	if(flen == size)
	{
		ptr = fptr;
		prev = fprev;
		if(ptr == prev)
		{
			ptr = NULL;
			goto out;
		}
		else if(ptr == m_rpc_sm_head)
		{
			prev[0] = (prev[0]&0xffff)|(ptr[0]&0xffff0000);
			m_rpc_sm_head = (UINT32 *)((ptr[0]>>16) + ALLOC_SM_ADDR);
			
			goto out;
		}
		else
		{
			prev[0] = (prev[0]&0xffff)|(ptr[0]&0xffff0000);
		}
	}
	else
	{
		ptr = fptr; prev = fprev;
		rptr = (UINT32 *)((UINT32)ptr + size);
		off = (UINT32)rptr - ALLOC_SM_ADDR; 
		if(ptr == prev)
		{
			rptr[0] = (flen - size)|(off<<16);
		}
		else
		{
			rptr[0] = (flen - size)|(ptr[0]&0xffff0000);
			prev[0] = (prev[0]&0xffff)|(off<<16);
		}
	}

	if(ptr == m_rpc_sm_head)
	{
		m_rpc_sm_head = (UINT32 *)((UINT32)ptr + size);
	}

out:
	up(&sem_shm);
	return ptr;
}
EXPORT_SYMBOL(ali_rpc_malloc_shared_mm);

/* Name :
			ali_rpc_free_shared_mm
    Description :
			free the malloced shared memory
    Arguments :
    			void *buf ---> the shared memory buffer start address
    			unsigned logn size ---> size of the buffer
    Return value:
    			<= 0 	---> fail
    			others 	---> ok
*/
void ali_rpc_free_shared_mm(void *buf, unsigned long size)
{
	UINT32 head = 0;
	UINT32 *fptr = buf;
	UINT32 *tprev, *tfptr;
	UINT32 *ptr, *prev;

	if(!size)
	{
		return;
	}

	down(&sem_shm);
	
	/*Alignment by DWORD.*/
	size = (size + 3) & (~0x3);
	prev = ptr = m_rpc_sm_head;

	/*Find out previus and next blocks.*/
	while(1)
	{
		head = ptr[0];
		ptr = (UINT32 *)((head>>16) + ALLOC_SM_ADDR);
		if(((prev < fptr) || (prev == m_rpc_sm_head)) && (fptr < ptr))
		{
			break;
		}

		if(ptr == m_rpc_sm_head)
		{
			goto out;
		}
		prev = ptr; 		 
	}

	tprev = (UINT32 *)((UINT32)prev + (prev[0]&0xffff));
	tfptr = (UINT32 *)((UINT32)fptr + size);
	if(fptr == tprev)
	{
		/*Merge with last free block.*/
		size += (prev[0]&0xffff);
		prev[0] = (prev[0]&0xffff0000)|size;
		fptr = prev;
	}
	else
	{
		prev[0] = (prev[0]&0xffff)|(((UINT32)fptr - ALLOC_SM_ADDR)<<16);	
	}

	if(tfptr == ptr)
	{
		/*Merge with next free block.*/
		if(prev == ptr)
		{
			fptr[0] = (((UINT32)fptr- ALLOC_SM_ADDR)<<16)|(size + (ptr[0]&0xffff));
		}
		else
		{
			fptr[0] = (ptr[0]&0xffff0000)|(size + (ptr[0]&0xffff));
		}

		if(tfptr == m_rpc_sm_head)
		{
			m_rpc_sm_head = fptr;
		}
	}
	else
	{
		fptr[0] = size | (((UINT32)ptr - ALLOC_SM_ADDR)<<16);
	}

out:
	up(&sem_shm);
}
EXPORT_SYMBOL(ali_rpc_free_shared_mm);

/* Name :
			ali_rpc_call
    Description :
			rpc call entry
    Arguments :
    			void *msg ---> massage buffer start address
    			void *func_p ---> function routine pointer
    			unsinged long func_desc ---> function routine description
    			void *arg_desc ---> argument structure description
    Return value:
    			the return value of the func_p routine
*/
unsigned long noinline ali_rpc_call(void *msg, void *func_p, unsigned long func_desc, void *arg_desc)
{
	int ret = 0;
	unsigned long rret = 0;
	int free_idx = (-1);

	RPC_PRF("\n");
	ENTRY;
	ALI_RPC_CALL_LOG("%s,func_desc:0x%08x\n", __FUNCTION__, (int)func_desc);
	
	if(down_interruptible(&m_rpc_priv->sem_call))
	{
		pr_crit("rpc_call down sem fail\n");
		return ret;
	}

	//RPC_PRF("rpc: %d\n",__LINE__);
	//if(msg == NULL)
	{
		free_idx = get_free_out_buf();
		if(free_idx < 0)
		{
			up(&m_rpc_priv->sem_call);
			pr_crit("get free out buf fail\n");
			return ret;
		}
		msg = m_rpc_priv->out_msg_buf[free_idx];
	}

	up(&m_rpc_priv->sem_call);
	
	if((ret = call_para_serialize(msg, arg_desc, func_p, func_desc, free_idx)) < 0)
	{
		ALI_RPC_CALL_LOG("serialize call para fail %d\n", (int)ret);
		goto EXIT;
	}


	if((ret = remote_call(msg, free_idx)) < 0)
	{
		ALI_RPC_CALL_LOG("remote call fail %d \n", (int)ret);
		goto EXIT;
	}

	call_ret_unserialize(msg);
	rret = (((struct remote_response_msg *)msg)->ret);


EXIT:
	LEAVE;
	RPC_PRF("\n");

	if(down_interruptible(&m_rpc_priv->sem_call))
	{
		pr_crit("rpc_call down sem fail\n");
		return ret;
	}

	if(free_idx >= 0)
	{
		m_rpc_priv->out_msg_buf_valid[free_idx] = 1;
	}

	up(&m_rpc_priv->sem_call);

	if(ret < 0)
	{
		pr_crit("remote_call failed, Ret:%x\n", 0);
		return 0;
	}

	return rret;
}

EXPORT_SYMBOL(ali_rpc_call);

/* Name :
			ali_rpc_ret
    Description :
			rpc ret entry
    Arguments :
    			void *msg ---> massage buffer start address
			unsigned long entry_func ---> function entry
    Return value:
    			none
*/
void ali_rpc_ret(unsigned long entry_func, void *msg)
{
#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID
	extern void *real_dev_get_by_vir_id(UINT32 virtual_dev_id);
	struct remote_call_msg *unserialized_msg = NULL;
	int i = 0;
#endif

	ENTRY;
	call_para_unserialize(msg);

#ifdef CONFIG_RPC_USE_VIRTUAL_DEVICE_ID
	unserialized_msg = (struct remote_call_msg *)msg;
	for(i=0; i<unserialized_msg->parasize; i++)
	{
		if((unserialized_msg->para[i] & HLD_VIRTUAL_DEVICE_MAGIC_MSK) == HLD_VIRTUAL_DEVICE_MAGIC)
		{
			unserialized_msg->para[i] = (UINT32)real_dev_get_by_vir_id((UINT32)unserialized_msg->para[i]);

			if(unserialized_msg->para[i] == 0)
			{
				RPC_PRF("os_hld_callee critical error, found a NULL para[]\n");
				return;
			}
		}
	}
#endif

#ifdef CONFIG_KFT
	call_to_func(entry_func, msg);
#else
	ali_rpc_call_function(entry_func, msg);
#endif
	call_ret_serialize(msg);
	remote_ret(msg);
	LEAVE;
}
EXPORT_SYMBOL(ali_rpc_ret);

static inline UINT32 is_hw_ack_flag_true(UINT32 type)
{
	return (*(volatile UINT32 *)(SEEROM_SEE_RUN_STATUS_ADDR)) & (type);
}

static inline UINT32 is_hw_see_run_flag(void)
{
	return (*(volatile UINT32 *)(SEEROM_MAIN_RUN_STATUS_ADDR)) == SEE_SW_RUN_FLAG;
}

static int rpc_thread(void *par)
{
	struct ali_rpc_private *priv = (struct ali_rpc_private *)par;
	int ret = 0;
	unsigned long thd_index = 0;

	thd_index = priv->tsk_id;
	priv->tsk_id = -1;//indecates thread waked up already
	RPC_PRF("enter the rpc thread index=%x\n",thd_index);

	do{
		RPC_PRF("wait for new remote call comming...\n");
		wait_event(priv->wait_call[thd_index], (priv->flag[thd_index] & MAILBOX_GET_REMOTE_CALL));
		RPC_PRF("rpc_thread wait event success! thread index=%x\n",thd_index);

		/* it may be interrupted by signal */
		ret = priv->flag[thd_index] & MAILBOX_GET_REMOTE_CALL ? 1 : -1;
		if(ret < 0){
			RPC_PRF("%s : wait event fail %d\n", __FUNCTION__, ret);
			continue;
		}
		
		RPC_PRF("get new remote call from SEE \n");

		ret = local_call(priv->in_msg_buf[thd_index]);
		if(ret < 0){
			RPC_PRF("local fail %d\n", ret);
		}
		priv->flag[thd_index] = ~MAILBOX_GET_REMOTE_CALL;
		RPC_PRF("rpc_thread to be freed! thread index=%x\n",thd_index);
	}while(1);

	return ret;
}

struct ali_rpc_dev
{
	struct mutex ioctl_mutex;
	dev_t dev_id;
	struct cdev cdev;
};
struct ali_rpc_dev g_ali_rpc_device;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
INT32 ali_rpc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#else
INT32 ali_rpc_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long param)
#endif
{
	INT32 ret = 0;
	UINT32 val1 = 0x11111111;
	UINT32 val2 = 0x22222222;
	UINT32 val3 = 0x33333333;
	UINT32 val4 = 0xaaaaaaaa;
	UINT32 val5 = 0xbbbbbbbb;
	UINT32 val6 = 0xcccccccc;
	UINT32 val7 = 0xdddddddd;
	UINT32 val8 = 0xeeeeeeee;

	switch(cmd)
	{
		case single_thread_test:
			ret = ali_rpc_ci_test(val1, val2, val3, val4, val5, val6, val7, val8);
			break;

		case multi_thread_test:
			ret = ali_rpc_ci_test(val1, val2, val3, val4, val5, val6, val7, val8);
			break;

		case see_call_main_Test:
			ret = ali_rpc_ci_test(val1, val2, val3, val4, val5, val6, val7, val8);
			break;

		default:
			ret = EINVAL;//invaid parameters
			break;
	}

	return ret;
}

int ali_rpc_open(struct inode *inode, struct file *file)
{
	RPC_PRF("ali_rpc_open\n");
	return 0;
}

int ali_rpc_release(struct inode *inode, struct file *file)
{
	RPC_PRF("ali_rpc_releaseil\n");
	return 0;
}

struct class *g_ali_rpc_class;
static const struct file_operations ali_rpc_fops = {
	.owner = THIS_MODULE,
	.write = NULL,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
	.unlocked_ioctl = ali_rpc_ioctl,
#else
	.ioctl = ali_rpc_ioctl,
#endif
	.open = ali_rpc_open,
	.release = ali_rpc_release,
};

static void g_see_boot(void)
{
	u32 i = 0;
	u32 addr = 0;
#ifdef CONFIG_ALI_STANDBY_TO_RAM
	if(OTP_SEEROM_ENABLED == seerom_is_enabled())
	{
	#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		RPC_PRF(KERN_EMERG "\nFunction:%s, Line:%d, seerom bit: enabled!", __FUNCTION__, __LINE__);
	#endif
		__REG32ALI(0xb80402b0) = see_bl_param.see_bl_real_run_address;
		__REG32ALI(0xb80402b8) = see_bl_param.see_bl_len_address;
		__REG32ALI(0xb80402b4) = see_bl_param.see_bl_location_address;
		__REG32ALI(0xb80402bc) = see_bl_param.see_signatrue_location_address;
		__REG32ALI(0xb80402c0) = see_bl_param.see_software_enter_address;
		__REG32ALI(0xb8000200) = see_bl_param.see_enter_address;
		__REG32ALI(0xb8000220) |= 0x2;
		for(i=0; i<0x900000; i++);/*Delay some time to wait for SEE boot up.*/
	}
	else
#endif
	{
	#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
		RPC_PRF(KERN_EMERG "\nFunction:%s, Line:%d, seerom bit: disabled!", __FUNCTION__, __LINE__);
	#endif
	#if defined(CONFIG_CAS9_VSC_ENABLE)
		addr = SEE_RUN_ADDR + 0x30000;
	#else
		addr = SEE_RUN_ADDR;
	#endif

		addr = (addr & 0x0FFFFFFF) | 0xA0000000;
		__REG32ALI(0x18000220) &= ~0x2;
		__REG32ALI(0x18000200) = addr;/*Set see boot addr.*/
		for(i=0; i<0x100; i++);
		__REG32ALI(0x18000220) |= 0x2;/*Release SEE to boot up.*/
		for(i=0; i<0xA00000; i++);/*Delay some time to wait for SEE boot up.*/
	}
}

static void get_address_from_dram_split_reg(void)
{
	__G_ALI_MM_PRIVATE_AREA_START_ADDR = GET_DWORD(DRAM_SPLIT_CTRL_BASE+PVT_S_ADDR)&(~0x3);
	__G_ALI_MM_PRIVATE_AREA_SIZE = (GET_DWORD(DRAM_SPLIT_CTRL_BASE+PVT_E_ADDR)&(~0x3)) - __G_ALI_MM_PRIVATE_AREA_START_ADDR;
	__G_ALI_MM_PRIVATE_AREA_START_ADDR |= 0xA0000000;

	__G_ALI_MM_SHARED_MEM_START_ADDR = GET_DWORD(DRAM_SPLIT_CTRL_BASE+SHR_S_ADDR)&(~0x3);
	__G_ALI_MM_SHARED_MEM_SIZE= (GET_DWORD(DRAM_SPLIT_CTRL_BASE+SHR_E_ADDR)&(~0x3)) - __G_ALI_MM_SHARED_MEM_START_ADDR;
	__G_ALI_MM_SHARED_MEM_START_ADDR |= 0xA0000000;

#ifdef CONFIG_LOGSM_SEE2MAIN
	__G_ALI_MM_LOGSM_SEE2MAIN_START_ADDR = __G_ALI_MM_SHARED_MEM_START_ADDR;
	__G_ALI_MM_LOGSM_SEE2MAIN_SIZE = LOGSM_SEE2MAIN_BUF_SIZE;

	__G_ALI_MM_SHARED_MEM_START_ADDR += LOGSM_SEE2MAIN_BUF_SIZE;
	__G_ALI_MM_SHARED_MEM_SIZE -= LOGSM_SEE2MAIN_BUF_SIZE;
#endif	
}

static void print_memory(void)
{
	u32 priv_mem_base_addr = 0;
	u32 priv_mem_len = 0;
	u32 share_mem_base_addr = 0;
	u32 share_mem_len = 0;

	priv_mem_base_addr = __G_ALI_MM_PRIVATE_AREA_START_ADDR;
	priv_mem_len = __G_ALI_MM_PRIVATE_AREA_SIZE;
	share_mem_base_addr = __G_ALI_MM_SHARED_MEM_START_ADDR;
	share_mem_len = __G_ALI_MM_SHARED_MEM_SIZE;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "\nRPC addr as follow:\n");
	printk(KERN_EMERG "[priv_mem_base_addr:0x%08X]\n", priv_mem_base_addr);
	printk(KERN_EMERG "[priv_mem_end_addr:0x%08X]\n", priv_mem_base_addr + priv_mem_len);
	printk(KERN_EMERG "[share_mem_base_addr:0x%08X]\n", share_mem_base_addr);
	printk(KERN_EMERG "[share_mem_end_addr:0x%08X]\n", share_mem_base_addr + share_mem_len);
#ifdef CONFIG_LOGSM_SEE2MAIN
	printk(KERN_EMERG "[__G_ALI_MM_LOGSM_SEE2MAIN_START_ADDR:0x%x]\n", __G_ALI_MM_LOGSM_SEE2MAIN_START_ADDR);
	printk(KERN_EMERG "[__G_ALI_MM_LOGSM_SEE2MAIN_SIZE:0x%x]\n", __G_ALI_MM_LOGSM_SEE2MAIN_START_ADDR + __G_ALI_MM_LOGSM_SEE2MAIN_SIZE);
#endif
#endif
}

static void install_memory(void)
{
	/*get private start/end, share start/end address from dram split register.*/
	get_address_from_dram_split_reg();
	print_memory();
}

#ifdef CONFIG_RPC_HANDSHAKE_TEST
/*test rpc handshake is ok or not*/
INT32 rpc_remote_handshake_test(UINT32 flag1, UINT32 flag2, UINT32 flag3, UINT32 flag4, UINT32 flag5, UINT32 flag6, UINT32 flag7, UINT32 flag8)
{
	jump_to_func(NULL, ali_rpc_call, flag1, (HLD_BASE_MODULE<<24) | (8<<16) | 0, NULL);
}
#endif

#ifdef CONFIG_ALI_SEE_BUS_ENABLE
int ali_rpc_init(struct platform_device *pdev)
#else
static int __init ali_rpc_init(void)
#endif
{
	int result = 0;
	int ret = 0;
	struct ali_rpc_dev *rpc_dev;
	struct device *clsdev;
	int i = 0;
	u32 chip_id;
#ifdef CONFIG_RPC_HANDSHAKE_TEST
	UINT32 flag1 = 0x11111111;
	UINT32 flag2 = 0x22222222;
	UINT32 flag3 = 0x33333333;
	UINT32 flag4 = 0x44444444;

	UINT32 flag5 = 0x55555555;
	UINT32 flag6 = 0x66666666;
	UINT32 flag7 = 0x77777777;
	UINT32 flag8 = 0x5a5a55aa;
#endif

#ifdef CONFIG_BOOT_SEE_IN_KERNEL
	RPC_PRF("ali_rpc_init,install_memory...\n");
	install_memory();
	RPC_PRF("ali_rpc_init,boot see cpu...\n");
	g_see_boot();
	mdelay(BOOT_SEE_DELAY);
	RPC_PRF("ali_rpc_init,boot see cpu completed!\n");
#else
	install_memory();
	RPC_PRF("ali_rpc_init,need not boot see!\n");
#endif

	rpc_dev = &g_ali_rpc_device;

	ret = of_get_major_minor(pdev->dev.of_node,&rpc_dev->dev_id, 
			0, 1, "ali_rpc");
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto EXIT;
	}

	RPC_PRF("%s, dev_id:%d.\n", __FUNCTION__, rpc_dev->dev_id);
	cdev_init(&(rpc_dev->cdev), &ali_rpc_fops);
	rpc_dev->cdev.owner = THIS_MODULE;
	result = cdev_add(&rpc_dev->cdev, rpc_dev->dev_id, 1);

	g_ali_rpc_class = class_create(THIS_MODULE, "ali_rpc_class");
	if (IS_ERR(g_ali_rpc_class))
	{
		result = PTR_ERR(g_ali_rpc_class);
		goto EXIT;
	}
	clsdev = device_create(g_ali_rpc_class, NULL, rpc_dev->dev_id,rpc_dev, "ali_rpc");
	if (IS_ERR(clsdev))
	{
		RPC_PRF(KERN_ERR "device_create() failed!%s, %d\n", __FUNCTION__, __LINE__);
		result = PTR_ERR(clsdev);
		goto EXIT;
	}

#ifdef ENABLE_RPC_STATICS
	RpcStatstics = (volatile struct RpcStatstics *)RPC_STATISTICS_BUF_ADDR;
	memset((void *)RpcStatstics, 0, sizeof(struct RpcStatstics));
#endif

	m_rpc_priv = kmalloc(sizeof(*m_rpc_priv), GFP_KERNEL);
	if(m_rpc_priv == NULL) {
		RPC_PRF("malloc rpc priv fail\n");
		ret = -EINVAL;
		goto EXIT;
	}
	memset((void *)m_rpc_priv, 0, sizeof(*m_rpc_priv));

	m_rpc_priv->name = driver_name;
	sema_init(&m_rpc_priv->sem_call, 1);
	sema_init(&sem_write, 1);
	sema_init(&sem_shm, 1);
	
	for(i = 0;i<RPC_NUM;i++)
	{
		init_waitqueue_head(&m_rpc_priv->wait_call[i]);
	}
	
	m_rpc_sm_head = (UINT32 *)ALLOC_SM_ADDR;
	m_rpc_sm_head[0] = ALLOC_SM_LEN;
	rpc_mutex_init();
	chip_id = GET_DWORD(CHIP_ID_REG)>>16;
	RPC_PRF("%s, chip_id:0x%x.\n", __FUNCTION__, chip_id);

	if((chip_id == 0x3901) || (chip_id == 0x3701) || \
		(chip_id == 0x3503) || (chip_id == 0x3821) || \
		(chip_id == 0x3505) || (chip_id == 0x3921))
	{
		m_rpc_int_status = __REGALIRAW(0x18040037);
		m_rpc_int_clear = __REGALIRAW(0x18000037);
		m_rpc_int_set = __REGALIRAW(0x18000037);
		m_rpc_mailin0_mask = 0x08;
		m_rpc_mailin1_mask = 0x04;
		m_rpc_mailin0_clear = 0x80;
		m_rpc_mailin1_clear = 0x40;
	}
	else
	{
		m_rpc_int_status = __REGALIRAW(0x18040036);
		m_rpc_int_clear = __REGALIRAW(0x18000037);
		m_rpc_int_set = __REGALIRAW(0x18000036);
		m_rpc_mailin0_mask = 0x80;
		m_rpc_mailin1_mask = 0x40;
		m_rpc_mailin0_clear = 0x08;
		m_rpc_mailin1_clear = 0x04;
	}

#ifdef CONFIG_ALI_TAC // message buffer enlarge from 8k to 36k for tac
	for(i = 0;i < RPC_NUM;i++)
	{
		m_rpc_priv->out_msg_buf[i] = kmalloc(CALL_MSG_BUFFER_SIZE, GFP_KERNEL);
		m_rpc_priv->out_msg_buf_valid[i] = 1;
	}

	for(i = 0;i < RPC_NUM;i++)
	{
		m_rpc_priv->in_msg_buf[i] = kmalloc(RET_MSG_BUFFER_SIZE, GFP_KERNEL);
	}
#else
	m_rpc_priv->out_msg_buf[0] = kmalloc(CALL_MSG_BUFFER_SIZE * RPC_NUM + RET_MSG_BUFFER_SIZE * RPC_NUM, GFP_KERNEL);
	if(m_rpc_priv->out_msg_buf[0] == NULL)
	{
		RPC_PRF("malloc rpc msg buf fail\n");
		ret = - EINVAL;
		goto EXIT;
	}

	m_rpc_priv->out_msg_buf_valid[0] = 1;
	for(i = 1;i < RPC_NUM;i++)
	{
		m_rpc_priv->out_msg_buf[i] = m_rpc_priv->out_msg_buf[i - 1] + CALL_MSG_BUFFER_SIZE;
		m_rpc_priv->out_msg_buf_valid[i] = 1;
	}

	for(i = 0;i < RPC_NUM;i++)
	{
		m_rpc_priv->in_msg_buf[i] = m_rpc_priv->out_msg_buf[RPC_NUM-1] + CALL_MSG_BUFFER_SIZE + RET_MSG_BUFFER_SIZE*i;
	}
#endif
	/* register the rpc mail box cal and ret isr */
	ret = request_irq(MAIL_BOX_REMOTE_CALL_INT_ID, rpc_call_isr, IRQF_DISABLED, m_rpc_priv->name,
		(void *)m_rpc_priv);
	if (ret) {
		RPC_PRF("rpc register ret irq fail\n");
		ret = -EBUSY;
		goto EXIT;
	}

	ret = request_irq(MAIL_BOX_REMOTE_RET_INT_ID, rpc_ret_isr, IRQF_DISABLED, m_rpc_priv->name,
		(void *)m_rpc_priv);
	if (ret) {
		RPC_PRF("rpc register ret irq fail\n");
		ret = -EBUSY;
		goto EXIT;
	}


	for(i = 0;i<RPC_NUM;i++)
	{
		m_rpc_priv->tsk_id = i;

		m_rpc_priv->thread_call[i] = kthread_create(rpc_thread, (void *)m_rpc_priv, "ali_rpc");
		if(IS_ERR(m_rpc_priv->thread_call)){
			RPC_PRF("rpc kthread create fail\n");
			ret = -EBUSY;
			goto EXIT;
		}
		m_rpc_priv->flag[i] = ~MAILBOX_GET_REMOTE_CALL;
		wake_up_process(m_rpc_priv->thread_call[i]);
		while(m_rpc_priv->tsk_id != -1) mdelay(1);/*waiting for thread wake up.*/
	}

#ifdef CONFIG_RPC_HANDSHAKE_TEST
	printk(">>>rpc handshake with see cpu, test start...\n");
	ret = rpc_remote_handshake_test(flag1,flag2,flag3,flag4,flag5,flag6,flag7,flag8);
	if(ret)
	{
		printk(">>>rpc handshake with see cpu, test fail!\n");
	}
	else
	{
		printk(">>>rpc handshake with see cpu, test ok!\n");
	}
#endif

	RPC_PRF("init remote devices start...\n");
	rpc_remote_dev_init();
	RPC_PRF("init remote devices done\n");
	show_software_tag();
EXIT:
	if(ret != 0 && m_rpc_priv != NULL){
		free_irq(MAIL_BOX_REMOTE_CALL_INT_ID, (void *)m_rpc_priv);
		free_irq(MAIL_BOX_REMOTE_RET_INT_ID, (void *)m_rpc_priv);

		for(i = 0;i<RPC_NUM;i++)
		{
			if(m_rpc_priv->thread_call[i] != NULL)
				kthread_stop(m_rpc_priv->thread_call[i]);
		}
		
		if(m_rpc_priv->out_msg_buf[0] != NULL)
			kfree(m_rpc_priv->out_msg_buf[0]);

		kfree(m_rpc_priv);
	}

	RPC_PRF("ali rpc init %s\n", ret == 0 ? "ok" : "fail");

	return ret;
}

#ifdef ENABLE_RPC_STATICS
void ali_rpc_statis_clr(void)
{
	RpcStatstics = (volatile struct RpcStatstics *)RPC_STATISTICS_BUF_ADDR;

	memset((void *)RpcStatstics, 0, sizeof(struct RpcStatstics));

	M_ScmIsrCnt = 0;
	M_SrmIsrCnt = 0;

	M_McsReqCnt = 0;
	M_MrsReqCnt = 0;

	M_McsSetSizeCnt = 0;
	M_MrsSetSizeCnt = 0;

	M_McsSetSizeZeroCnt = 0;
	M_MrsSetSizeZeroCnt = 0;
}
#endif
static void __exit ali_rpc_exit(void)
{
	int i = 0;
	if(m_rpc_priv != NULL){
		free_irq(MAIL_BOX_REMOTE_CALL_INT_ID, (void *)m_rpc_priv);
		free_irq(MAIL_BOX_REMOTE_RET_INT_ID, (void *)m_rpc_priv);
		for(i = 0;i < RPC_NUM;i++)
		{
			kthread_stop(m_rpc_priv->thread_call[i]);
			kfree(m_rpc_priv->out_msg_buf[i]);
		}
		kfree(m_rpc_priv);
	}
}

#ifndef CONFIG_ALI_SEE_BUS_ENABLE
rootfs_initcall(ali_rpc_init);
#endif

module_exit(ali_rpc_exit);
MODULE_AUTHOR("ALi (Shanghai) Corporation");
MODULE_DESCRIPTION("ali Remote Process Call driver for dual CPU platform");
MODULE_LICENSE("GPL");
