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
 
#ifndef __DRIVERS_ALI_RPC_H
#define __DRIVERS_ALI_RPC_H

#if 0
#define RPC_PRF printk
#else
#define RPC_PRF(...) 	do{}while(0)
#endif

//#define CONFIG_BOOT_SEE_IN_KERNEL       1

/*added by kinson for RPC handshake test*/
//#define CONFIG_RPC_HANDSHAKE_TEST     1 //defined in linux/ali_rpc.h

#ifdef CONFIG_ARM
#define RPC_BASE_ADDR 					((unsigned long)phys_to_virt(__G_ALI_MM_SHARED_MEM_START_ADDR&0x8FFFFFFF))
#else
#define RPC_BASE_ADDR 					(__G_ALI_MM_SHARED_MEM_START_ADDR|0xA0000000)
#endif

#define RPC_MEM_LEN  					(__G_ALI_MM_SHARED_MEM_SIZE)

#define MSG_BUFFER_SIZE 				(RPC_MEM_LEN>>2)

#define RPC_STATISTICS_BUF_LEN 			(sizeof(struct RpcStatstics)) 

#ifdef CONFIG_ALI_TEE_ENABLE
#define ALLOC_SM_LEN    				(MSG_BUFFER_SIZE-12-RPC_STATISTICS_BUF_LEN)
#else
#define ALLOC_SM_LEN                    ((RPC_MEM_LEN>>1)-12-RPC_STATISTICS_BUF_LEN)
#endif

#define CPU_MSG_BUFFER  				RPC_BASE_ADDR  

#define SEE_MSG_BUFFER  				(CPU_MSG_BUFFER + MSG_BUFFER_SIZE)

#ifdef CONFIG_ALI_TEE_ENABLE
#define RPC_STATISTICS_BUF_ADDR 		(SEE_MSG_BUFFER + MSG_BUFFER_SIZE*2)     
#else
#define RPC_STATISTICS_BUF_ADDR         (SEE_MSG_BUFFER + MSG_BUFFER_SIZE) 
#endif

#define ALLOC_SM_ADDR   				(RPC_STATISTICS_BUF_ADDR + RPC_STATISTICS_BUF_LEN)

#define READ_DUAL_MUTEX_ADDR 			((ALLOC_SM_ADDR + ALLOC_SM_LEN))

#define WRITE_DUAL_MUTEX_ADDR 			((ALLOC_SM_ADDR + ALLOC_SM_LEN)+4)

#define SEE_EXCEPTION_HANDLER_ADDR 	    (ALLOC_SM_ADDR + ALLOC_SM_LEN + 8)

/* interrupt vector of main mailbox */
#if defined(CONFIG_ARM)
 #define MAIL_BOX_REMOTE_CALL_INT_ID  	91 
 #define MAIL_BOX_REMOTE_RET_INT_ID   	90	
#else
 #define MAIL_BOX_REMOTE_CALL_INT_ID  	71
 #define MAIL_BOX_REMOTE_RET_INT_ID   	70
#endif

#define CHIP_ID_REG             (__REGALIRAW(0x18000000))

#define MAILIN0_ADDR      		(__REGALIRAW(0x18040200))
#define MAILIN1_ADDR      		(__REGALIRAW(0x18040204))
#define MAILOUT0_ADDR     		(__REGALIRAW(0x18000200))
#define MAILOUT1_ADDR     		(__REGALIRAW(0x18000204))
#define MAILIN0_BUF       		SEE_MSG_BUFFER
#define MAILOUT0_BUF      		CPU_MSG_BUFFER

#define DRAM_SPLIT_CTRL_BASE  	__REGALIRAW(0x18041000)
#define PVT_S_ADDR 0x10
#define PVT_E_ADDR 0x14
#define SHR_S_ADDR 0x18
#define SHR_E_ADDR 0x1c

#define GET_DWORD(addr)         *(volatile UINT32 *)(addr)
#define SET_DWORD(addr, d)      *(volatile UINT32 *)(addr) = (d)
#define SETB_DWORD(addr, bit)   *(volatile UINT32 *)(addr) |= (bit)
#define SETB_BYTE(addr, bit)    *(volatile UINT8 *)(addr) |= (bit)
#define GET_BYTE(addr)          *(volatile UINT8 *)(addr)
#define SET_BYTE(addr, bit)     *(volatile UINT8 *)(addr) = (bit)

#define BOOT_SEE_DELAY  		250
#define RPC_NUM					5
#define MAILBOX_TIME_OUT		50000

/* interrupt vector of main mailbox */
#if defined(CONFIG_ALI_CHIP_M3921)
	#define MAIN_MBX0_INT           INT_ALI_MBX0 
        #define MAIN_MBX1_INT           INT_ALI_MBX1
        #define MAIN_MBX2_INT           INT_ALI_MBX2
        #define MAIN_MBX3_INT           INT_ALI_MBX3
#else
	#define	MAIN_MBX0_INT           0x47
	#define	MAIN_MBX1_INT           0x46
	#define	MAIN_MBX2_INT           0x45
	#define	MAIN_MBX3_INT           0x44
#endif

/* mailbox reg for send */
#define REG_MAIN_MBX0_SEND      (0x18000200)
#define REG_MAIN_MBX1_SEND      (0x18000204)
#define REG_MAIN_MBX2_SEND      (0x18000208)
#define REG_MAIN_MBX3_SEND      (0x1800020C)
#define REG_STANDBY_CHECK        (0xB8055FF0)

/* mailbox reg for receive */
#define REG_MAIN_MBX0_RECV      (0x18040200)
#define REG_MAIN_MBX1_RECV      (0x18040204)
#define REG_MAIN_MBX2_RECV      (0x18040208)
#define REG_MAIN_MBX3_RECV      (0x1804020C)

/* mailbox interrupt enable register */
#define REG_MAIN_MBX_ENABLE     (0x1804003C)
#define MAIN_MBX0_ENABLE_FLG    (1<<31)
#define MAIN_MBX1_ENABLE_FLG    (1<<30)
#define MAIN_MBX2_ENABLE_FLG    (1<<29)
#define MAIN_MBX3_ENABLE_FLG    (1<<28)
#define MAIN_MBX_ENABLE_MASK    (MAIN_MBX0_ENABLE_FLG|MAIN_MBX1_ENABLE_FLG|MAIN_MBX2_ENABLE_FLG|MAIN_MBX3_ENABLE_FLG)

#define MAIN_MBX_ENABLE         \
    writel(MAIN_MBX_ENABLE_MASK, REG_MAIN_MBX_ENABLE)

/* interrupt register */
#define	REG_MAIN_MBX_INT        (0x18000034)
#define	MAIN_MBX0_SET_FLG       (1<<27)
#define	MAIN_MBX1_SET_FLG       (1<<26)
#define	MAIN_MBX2_SET_FLG       (1<<25)
#define	MAIN_MBX3_SET_FLG       (1<<24)
#define	MAIN_MBX0_CLR_FLG       (1<<31)
#define	MAIN_MBX1_CLR_FLG       (1<<30)
#define	MAIN_MBX2_CLR_FLG       (1<<29)
#define	MAIN_MBX3_CLR_FLG       (1<<28)

#define MAIN_CLR_SEE_MBX0_INT   (MAIN_MBX0_CLR_FLG, REG_MAIN_MBX_INT)
#define MAIN_CLR_SEE_MBX1_INT   (MAIN_MBX1_CLR_FLG, REG_MAIN_MBX_INT)
#define MAIN_CLR_SEE_MBX2_INT   (MAIN_MBX2_CLR_FLG, REG_MAIN_MBX_INT)
#define MAIN_CLR_SEE_MBX3_INT   (MAIN_MBX3_CLR_FLG, REG_MAIN_MBX_INT)

/* sense which mailbox is interruptting */
#define	REG_MAIN_SENSE_MBX      (0x18000034)
#define REG_SEE_SENSE_MAIN         REG_MAIN_SENSE_MBX
#define	MAIN_MBX0_SENSE_FLG     (1<<27)
#define	MAIN_MBX1_SENSE_FLG     (1<<26)
#define	MAIN_MBX2_SENSE_FLG     (1<<25)
#define	MAIN_MBX3_SENSE_FLG     (1<<24)
#define	MAIN_MBX_SENSE_MASK     (MAIN_MBX0_SENSE_FLG|MAIN_MBX1_SENSE_FLG|MAIN_MBX2_SENSE_FLG|MAIN_MBX3_SENSE_FLG)

/*----------------------------------------------------------------------------*/

/* interrupt vector of see mailbox */
#define	SEE_MBX0_INT            0x43
#define	SEE_MBX1_INT            0x42
#define	SEE_MBX2_INT            0x41
#define	SEE_MBX3_INT            0x40

/* mailbox reg for send */
#define	REG_SEE_MBX0_SEND       (0x18040200)
#define	REG_SEE_MBX1_SEND       (0x18040204)
#define	REG_SEE_MBX2_SEND       (0x18040208)
#define	REG_SEE_MBX3_SEND       (0x1804020C)

/* mailbox reg for receive */
#define	REG_SEE_MBX0_RECV       (0x18000200)
#define	REG_SEE_MBX1_RECV       (0x18000204)
#define	REG_SEE_MBX2_RECV       (0x18000208)
#define	REG_SEE_MBX3_RECV       (0x1800020C)

/* mailbox interrupt enable register */
#define	REG_SEE_MBX_ENABLE      (0x1800003C)
#define	SEE_MBX0_ENABLE_FLG		(1<<31)
#define	SEE_MBX1_ENABLE_FLG		(1<<30)
#define	SEE_MBX2_ENABLE_FLG		(1<<29)
#define	SEE_MBX3_ENABLE_FLG		(1<<28)
#define SEE_MBX_ENABLE_MASK     (SEE_MBX0_ENABLE_FLG|SEE_MBX1_ENABLE_FLG|SEE_MBX2_ENABLE_FLG|SEE_MBX3_ENABLE_FLG)

#define SEE_MBX_ENABLE          \
    writel(SEE_MBX_ENABLE_MASK, REG_SEE_MBX_ENABLE)

/* Issue interrupt register */
#define	REG_SEE_MBX_INT         __REGALIRAW(0x18040034)
#define	SEE_MBX0_SET_FLG        (1<<27)
#define	SEE_MBX1_SET_FLG        (1<<26)
#define	SEE_MBX2_SET_FLG        (1<<25)
#define	SEE_MBX3_SET_FLG        (1<<24)
#define	SEE_MBX0_CLR_FLG        (1<<31)
#define	SEE_MBX1_CLR_FLG        (1<<30)
#define	SEE_MBX2_CLR_FLG        (1<<29)
#define	SEE_MBX3_CLR_FLG        (1<<28)

#define SEE_CLR_MAIN_MBX0_INT   writel(SEE_MBX0_CLR_FLG, REG_SEE_MBX_INT)
#define SEE_CLR_MAIN_MBX1_INT   writel(SEE_MBX1_CLR_FLG, REG_SEE_MBX_INT)
#define SEE_CLR_MAIN_MBX2_INT   writel(SEE_MBX2_CLR_FLG, REG_SEE_MBX_INT)
#define SEE_CLR_MAIN_MBX3_INT   writel(SEE_MBX3_CLR_FLG, REG_SEE_MBX_INT)

/* sense which mailbox is interruptting */
#define	REG_SEE_SENSE_MBX       __REGALIRAW(0x18040034)
#define REG_MAIN_SENSE_SEE      REG_SEE_SENSE_MBX
#define	SEE_MBX0_SENSE_FLG      (1<<27)
#define	SEE_MBX1_SENSE_FLG      (1<<26)
#define	SEE_MBX2_SENSE_FLG      (1<<25)
#define	SEE_MBX3_SENSE_FLG      (1<<24)
#define	SEE_MBX_SENSE_MASK      (SEE_MBX0_SENSE_FLG|SEE_MBX1_SENSE_FLG|SEE_MBX2_SENSE_FLG|SEE_MBX3_SENSE_FLG)

/*Add for STR test.*/
#define SEE_IS_IN_SUSPEND               (0xDEADBEEF)
#define SEE_WAKEUP_FROM_STR         (0x57414B45)/*'W'----0x57; 'A'----0x41; 'K'----4B; 'E'----0x45.*/

struct RpcStatstics
{
	UINT32 M_ScmIsrCnt;
	UINT32 M_SrmIsrCnt;
	UINT32 M_McsReqCnt;
	UINT32 M_MrsReqCnt;

    UINT32 S_ScmReqCnt;
    UINT32 S_SrmReqCnt;
    UINT32 S_McsIsrCnt;
    UINT32 S_MrsIsrCnt;
};

struct remote_call_msg
{
    UINT32 size:30;
    volatile UINT32 flag:2;
    UINT8  func;
    UINT8  pri;
    UINT8  resv;
    UINT8  module;
    UINT32 caller;
    UINT32 retbuf;
    UINT32 parasize;
    UINT32 para[16];
};

struct remote_response_msg
{
    UINT32 size:30;
    UINT32 flag:2;
    UINT32 ret;
    UINT32 caller;
    UINT32 retbuf;
};


//Serialize structure 
//被串行化结构的描述
struct desc_storage_struct
{
    UINT16 size;  //size of this structure 结构大小
    UINT8  flag;  //flag of this structure 结构标志
    UINT8  ind;   //index in out para list
    UINT32 off;   //offset of message buffer for storage 结构的存储位置
    UINT32 wp;    //write back address for out parameter
};
//指向被串行化结构的指针的描述
//不支持不对齐的指针
struct desc_pointer_struct
{              
    UINT16 pflag:2; //0: pointer of this structure is a parameter  
                    //1: pointer of this structure is member of another structure
    UINT16 poff:14; //pointer offset if pflag == 1, max struct 16K
                    //index of parameter list if  pflag == 0 
    UINT8  pind;    //index of desc_struct list for pointer 
    UINT8  sind;    //index of desc_struct list for pointed
};

//Attribute of pointer
#define DESC_P_IS_PARA(x)    (((x)&0x1)  == 0)
#define DESC_P_IS_MEMBER(x)  (((x)&0x1)  == 1)
#define DESC_P_IS_RET(x)     (((x)&0x1)  == 0)

#define DESC_STRU_IS_OUTPUT(x) (((x)&0x1) == 1)
#define DESC_STRU_IS_CONSTR(x) (((x)&0x2) == 2)  
#define DESC_STRU_IS_LIST(x)   (((x)&0x4) == 4) 
#define DESC_STRU_IS_DYNAMIC(x)   ((x)&0xe) 

#define TYPE_OUTPUT            1
#define TYPE_STRING            2
#define TYPE_LIST              4
#define TYPE_SIZE_UB           0x10
#define TYPE_SIZE_UH           0x20
#define TYPE_SIZE_UW           0x00

typedef void (*NORMAL_CALLEE)(UINT8 *);

/* actively call the remote device */
#define MAILBOX_GET_REMOTE_CALL			1

/* passively reply to the remote device */
#define MAILBOX_GET_REMOTE_RET			2

//eric.cai enlarge from 8k to 36k 
#ifdef CONFIG_ALI_TAC
#if 1 //for work with logsee2main
#define CALL_MSG_BUFFER_SIZE 			(1024 * 52)//(1024 * 36)//(MSG_BUFFER_SIZE * 16) -->enlarge to 52K
#define RET_MSG_BUFFER_SIZE			(1024 * 52)//(1024 * 36)//(MSG_BUFFER_SIZE * 16) -->enlarge to 52K
#else 
#define CALL_MSG_BUFFER_SIZE 			(MSG_BUFFER_SIZE * 4 * 36)//(MSG_BUFFER_SIZE * 16)
#define RET_MSG_BUFFER_SIZE				(MSG_BUFFER_SIZE * 4 * 36)//(MSG_BUFFER_SIZE * 16)
#endif
#else 
#define CALL_MSG_BUFFER_SIZE 			(MSG_BUFFER_SIZE * 32)//(MSG_BUFFER_SIZE * 16)
#define RET_MSG_BUFFER_SIZE				(MSG_BUFFER_SIZE * 32)//(MSG_BUFFER_SIZE * 16)
#endif

#define MSG_RECEIVED 					0xffffffff
#define RPC_DISABLE                     0xdeaddead

#define REMOTE_CALL_TIME_OUT			(HZ * 100)

struct ali_rpc_private
{
	volatile unsigned long tsk_id;
	/* device name */
	char *name;

	/* semaphore for call */
	struct semaphore sem_call;

	/* semaphore for mutex operation */
	struct semaphore sem_mutex;

	/* control flag */
	volatile unsigned long flag[RPC_NUM];
	
	/* remote call wait queue */
	wait_queue_head_t wait_call[RPC_NUM];

	/* kernel thread to deal with the local call */
	struct task_struct *thread_call[RPC_NUM];

	/* out and in msg buffer */			
	void *out_msg_buf[RPC_NUM];
	int out_msg_buf_valid[RPC_NUM];
	volatile unsigned long rpc_flag[RPC_NUM];
	
	void *in_msg_buf[RPC_NUM];

	/* isr status */
	unsigned long isr_call:1;
	unsigned long isr_ret:1;
	unsigned long res:30;

};

//add by phil for boot-media
void rpc_remote_boot_media(void);
void rpc_remote_dev_init(void);
void ali_rpc_hld_base_callee(UINT8 *msg);
#ifdef CONFIG_ALI_VSC
void lld_vsc_callee(UINT8 *msg);
#endif
void show_software_tag(void);
#ifdef CONFIG_ALI_TAC
INT32 tac_rpc_S2M(UINT32 clientId, UINT32 contextId,void *data_call,
						UINT32 data_len_call, UINT32 *data_addr_ret, UINT32 *data_len_ret);
#endif 
#ifdef CONFIG_ALI_SEC
int rpc_s2m_bc_generate_cwc(unsigned char bServiceIdx, unsigned char ecm_value[255], unsigned char length);
int rpc_s2m_bc_hdmi_cmd(unsigned int cmd , int *status);
int rpc_s2m_bc_set_dsc_status(unsigned char bServiceIdx, unsigned long param);
#endif


#endif
