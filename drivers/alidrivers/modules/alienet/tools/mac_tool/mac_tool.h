#ifndef _MAC_TOOL_H
#define _MAC_TOOL_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>
#include <sys/wait.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define READ_MAC_REG     0x1
#define READ_SOC_REG     0x2
#define WRITE_MAC_REG    0x3
#define WRITE_SOC_REG    0x4
#define DEBUG_DRIVER_LEVEL 0x5 
#define TX_PKTS            0x6
#define WRITE_BIU_1BYTE   0x7
#define WRITE_BIU_2BYTE   0x8
#define WRITE_BIU_4BYTE   0x9
#define READ_BIU_1BYTE   0xa
#define READ_BIU_2BYTE   0xb
#define READ_BIU_4BYTE   0xc

#define REG_MAX_VALUE    0x94

#define MAC_TOOL_IOCTL_CMD  (SIOCDEVPRIVATE+11) 

typedef struct _cmd_param {
    u32 cmd; 
    u32 reg;
    u32 value;
    void *data;
} cmd_param_t;

/* used for tx cmd */
struct ali_mac_xmit_io {
	unsigned char  tx_rx;  
	unsigned char  type;   
	unsigned char  toe_tx; 
	unsigned char  tso_ufo;

	unsigned long  len;    

	unsigned long max_len;

	unsigned short  mtu;    
	unsigned short  max_mtu;
	
	unsigned short  desc_min; 
	unsigned short  desc_max; 
	
	unsigned short vlan;   
	unsigned short  reserve_1;
	
	unsigned long  repeat;  

	unsigned char  dest_type;
	unsigned char  hop_hd_len;
	unsigned char  dst_hd_len; 
	unsigned char  rt_hd_len;   

	unsigned char  dst_2_hd_len;
	unsigned char  reserve_2;
	unsigned short reserve_3;
};

#endif
