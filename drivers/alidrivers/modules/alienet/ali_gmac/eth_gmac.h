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

/*
All related Ethernet GMAC II Definitions
*/

#ifndef __ETHERNET_GMAC__H
#define __ETHERNET_GMAC__H
#include <ali_interrupt.h>
#ifdef CONFIG_ARM
#include <mach/ali-s3921.h>
#endif
#include <alidefinition/adf_basic.h>


#define HW_MDIO
//#define PHY_SMSC

#define GMAC_STATUS
#define SW_CRCCHK

#define SOC_BASE_ADDR       0x18000000
#define SOC_R32(i)          __REG32ALI(SOC_BASE_ADDR+(i))
#define SOC_W32(i, val)     __REG32ALI(SOC_BASE_ADDR+(i)) = (u32)(val)
#define SOC_REG32(i)          __REG32ALI(SOC_BASE_ADDR+(i))
//gmac Base address.
#ifdef CONFIG_ARM
#define _GMAC_BASE		ALI_SW_BASE_ADDR
#endif
#ifdef CONFIG_MIPS
#define _GMAC_BASE		0xb802c000
#endif
#define _GMAC_IRQ	    INT_ALI_SWITCH_INT

//VLAN tagging feature enable/disable
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
//#define GMAC_VLAN_TAG_USED 1
#define GMAC_VLAN_TAG_USED 0
#else
#define GMAC_VLAN_TAG_USED 0
#endif

#define GMAC_DEF_MSG_ENABLE \
			0x7fff//(NETIF_MSG_DRV |NETIF_MSG_PROBE |NETIF_MSG_LINK)


#define GMAC_TX_TIMEOUT	(2*HZ)


#define GMAC_DEBUG_TRACE
#define GMAC_DEBUG_WARNING

#ifdef GMAC_DEBUG_TRACE
#	define GMAC_TRACE(msg,args...) \
		printk("GMAC: " msg "\n", ## args)
#else
#	define GMAC_TRACE(msg,args...)
#endif

#ifdef GMAC_DEBUG_WARNING
#	define GMAC_WARNING(msg,args...) \
		printk("GMAC: " msg "\n", ## args)
#else
#	define GMAC_WARNING(msg,args...)
#endif


//Link mode bits.
#define GMAC_10HD		(0x01UL)
#define GMAC_10FD		(0x02UL)
#define GMAC_100HD		(0x04UL)
#define GMAC_100FD		(0x08UL)
#define GMAC_RX_PAUSE	(0x10UL)
#define GMAC_TX_PAUSE	(0x20UL)
#define GMAC_AUTONEG	(0x40UL)

//Rx Mode
#define GMAC_RX_MODE_ISR		(0UL)
#define GMAC_RX_MODE_TASKLET	(1UL)
#define GMAC_RX_MODE_NAPI		(2UL)

//Rx Filtering mode
#define GMAC_RX_FILTER_PERFECT	(0UL)
#define GMAC_RX_FILTER_HASH		(1UL)
#define GMAC_RX_FILTER_INVERSE		(2UL)
#define GMAC_RX_FILTER_HASHONLY	(3UL)

#define RING_CRC_SZ				4
#define MAX_GMAC_PKT_LEN		(64*1024)
//#define TEST_RX_DESC_BUG 
#ifdef TEST_RX_DESC_BUG 
#define GMAC_RX_DESC_NUM	     256	
#define GMAC_TX_DESC_NUM	     256	
#else
#define GMAC_RX_DESC_NUM		(64*4)
#define GMAC_TX_DESC_NUM		(64*4)
#endif
//#define GMAC_BUF_SZ				1536
#define GMAC_BUF_SZ			    2048
#define GMAC_JUMBO_SKB_LINEAR_SIZE   (256-16)
#define GMAC_JUMBO_FRAG_SZ	    2048	
#define GMAC_DESC_SZ			16

#define GMAC_MSS				1514

//Setup frame size.
#define SETUP_FRAME_SZ			192
#define IMPERFECT_PA_OFF		156
//Hash
#define GMAC_HASH_TABLE_LEN	512		//64 bytes
#define GMAC_HASH_BITS			0x01ff	//=512-1


#define GMAC_REGS_RANGE			0xFFFF
#define GMAC_REGS_VER			1

#define HIBYTE(word)  ((u8)(((u16)(word))>>8))
#define LOBYTE(word)  ((u8)(((u16)(word))&0x00FFU))
#define HIWORD(dword) ((u16)(((u32)(dword))>>16))
#define LOWORD(dword) ((u16)(((u32)(dword))&0x0000FFFFUL))

enum LINK_SPEED_CHECK {
	LINK_SPEED_10M = 0,
	LINK_SPEED_100M = 1,
	LINK_SPEED_1000M = 2,
	LINK_SPEED_RESERVED = 3,
};

typedef struct gmac_tx_context_desc{
	u32 VlanTag: 16; 					//bit 0~15.Vlan tag
	u32 L3Type: 1; 					//bit 16.  indicate L3 header type.
	u32 L4Type: 2;						//bit 17~18. indicate L4 header type.
	u32 L4HeaderLen: 8;				//bit 19~26. indicate L4 header length.
	u32 ContextIndex: 3;				//bit 27~29. indicate Index of Context Descriptor.
	u32 ContextData: 1;					//bit 30. indicate Descriptor type.
	u32 OWN: 1;						//bit 31. 0->Owned by SW, 1->Owned by HW.		

	u32 L3HeaderLen: 9;				//bit 0~8. indicate L4 header length.		
	u32 L2HeaderLen: 7;				//bit 9~15. indicate L2 header length.		
	u32 MSS: 16;						//bit 16~31. Maximum Segment Size

	u32 Reserved1;
	u32 Reserved2;

}__attribute__((packed)) *ptx_con_desc, tx_con_desc;


typedef struct gmac_tx_data_desc{
	u32 SegmentNum: 16; 			//bit 15~0.Maximum Frame Length(Without CRC)
	u32 Reserved20_16: 5;			//bit 16~20. Reserved.
	u32 TOE_UFO_En: 1;			//bit 21.IP header offset.
	u32 VlanEn: 1;					//bit 22.
	u32 CoeEn: 1;					//bit 23. enable Tx COE.
	u32 LS: 1;						//bit 24. indicate a Last Segment Descriptor.
	u32 FS: 1;						//bit 25. indicate a First Segment Descriptor.
	u32 EOR: 1;					//bit 26. indicate End Of descriptor Ring.
	u32 ContextIndex: 3;			//bit 27~29. indicate Index of Context Descriptor.
	u32 ContextData: 1;				//bit 30. indicate Descriptor type.
	u32 OWN: 1;					//bit 31. 0->Owned by SW, 1->Owned by HW.		

	dma_addr_t pkt_buf_dma;
	
	u16 seg_len;
	u16 tot_len;
	
	u32 FragmentID;

}__attribute__((packed)) *ptx_data_desc, tx_data_desc;

typedef struct gmac_tx_status_desc{
	u32 Reserved1_0: 2;		//bit 1~0.Reserved.
	u32 HF: 1;					//bit 2. Heartbeat Fail Error.
	u32 FifoUnderrun: 1; 		//bit 3. Fifo Underrun Error.
	u32 ColCnt: 4;				//bit 7~4. Collision Count.
	u32 ExCol: 1;				//bit 8. Excessive Collsion.
	u32 LateCol: 1;				//bit 9. Late Collsion.
	u32 NoCarrier: 1;			//bit 10. No Carrier.
	u32 LossOfCarrier: 1;		//bit 11. Loss Of Carrier..
	u32 ES: 1;					//bit 12. Error Sumary.
	u32 Reserved23_13: 11;		//bit 23~13. Reserved.
	u32 LS: 1;					//bit 28. indicate a Last Segment Descriptor.
	u32 FS: 1;					//bit 29. indicate a First Segment Descriptor.
	u32 Reserved30_26: 5;		//bit 26~30. Reserved.
	u32 OWN: 1;				//bit 31. 0->Owned by SW, 1->Owned by HW.

	u32 Reserved1;
	u32 Reserved2;
	u32 Reserved3;
}__attribute__((packed)) *ptx_status_desc, tx_status_desc;


typedef union gmac_tx_desc{

	tx_con_desc					ContextDescriptor;
	tx_data_desc					DataDescriptor;
	tx_status_desc				SatusDescriptor;
	
} *ptx_desc;

//head structure
typedef struct packet_head{
	u16	SegLength: 9;
	u16	TCPChksum: 1;			//TCP Checksum is Okay.
	u16	IPChksum: 1;				//IP Checksum is Okay.	
	u16	PPPoE: 1;				//PPPoE packet.
	u16	VLAN: 1;					//VLAN Fram.
	
	u16	WatchdogTimeout: 1;		//Receive Watchdog Time-out.
	u16	FrameType : 1;			//Frame Type.
	u16	PhysicalLayerError: 1;		//Physical Layer Error.
	u16	InvalidLength: 1;			//Invalid length Error. 
	u16	FAE : 1;					//Alignment Error.
	u16	CRC: 1;					//CRC error.
	u16	LateCollision: 1;			//Late Collision Seen.
	u16	Long : 1;					//Excessive Frame Length.
	u16	Runt: 1;					//Runt Frame.
	u16	ES: 1;					//Error Summary.
	u16	BF : 1;					//Broadcast Frame.
	u16	PF : 1; 					//Physical Frame.
	u16	MF : 1;					//Multicast Frame.
	u16	Dribble: 1;				//Dribble Error.
	u16	LastSegment: 1;			//Last Segment.
	u16	FirstSegment: 1;			//First Segment.
	u16	IPFrag: 1;				//IP Fragment.
	u16	IP6Frame: 1;				//IP Frame.
	u16	IPFrame: 1;			//IPv6 Frame.
}__attribute__((packed)) *ppacket_head;

typedef struct gmac_rx_desc{
	struct packet_head pkt_hdr;
	u16 l4_chs;
	u16 l3_chs;	
	u16 vlan_tag;	
	u16 PacketLength;
	dma_addr_t pkt_buf_dma;
}__attribute__((packed)) *prx_desc;

typedef struct gmac_device_stats {
	u32	rx_mc;				//muticast packets received.	//ok
	u32	rx_bc;				//broadcast packets received.	//ok
	u32	rx_uc;				//unicast packets received.		//ok
	u32	rx_vlan;										//ok
	u32	rx_pppoe;			//pppoe packets received.		//ok
	u32	rx_ip;										//ok
	u32    rx_ipv6;
	u32	rx_frag;										//ok
	
	u32	rx_runt_errors;							//ok
	u32	rx_long_errors;							//ok
	u32	rx_dribble_errors;						//ok
	u32	rx_phy_layer_errors;						//ok
	u32	rx_wd_timeout_errors;					//ok
	u32	rx_ip_chksum_errors;						//ok
	u32	rx_tcp_chksum_errors;					//ok
	
	u32	rx_buf_empty;		//there is no packet stored in rx ring buffer.	
	u32	rx_late_col_seen;	
	u32 	rx_lost_in_ring;
	
	u32 	rx_hdr_chs_errs;							//ok
	u32 	rx_pay_chs_errs;							//ok

	u32	tx_col_cnts[16];		//0 to 15 collisions.		//ok
	u32	tx_col_errors;		//excessive collision.		//ok

	u32	tx_no_carr_errors;
	u32	tx_loss_carr_errors;
}*pgmac_device_stats;


#define GMAC_NUM_STATS 36

typedef struct _skb_info_t {
	u16 start;  /* first descript */
	u16 first;  /* first data descript */
	u16 cnt;
	struct sk_buff *skb;
} skb_info_t;

typedef struct gmac_private{
	struct net_device	*dev;
	u32 io_base;

	bool acquired_isr;	
	unsigned int irq_num;
	u32 cur_isr;
	u32 isr_mask;

	spinlock_t lock;
	u32 msg_enable;

	struct napi_struct napi;

	struct mii_if_info mii_if;			//mii interface

	struct gmac_device_stats mac_stats;


	u8 *setup_buf;				//filter
	dma_addr_t setup_buf_dma;

	prx_desc rx_desc;				
	dma_addr_t rx_desc_dma;		//Rx Start Address of Descriptors.
								//dma_map_single(DMA_FROM_DEVICE);
								
	struct sk_buff	 *rx_skb[GMAC_RX_DESC_NUM];
    struct page * pages[GMAC_RX_DESC_NUM];
    struct sk_buff * rx_top_skb;

	ptx_desc tx_desc;
	dma_addr_t tx_desc_dma;		//Tx Start Address of Descriptors.
								//dma_map_single(DMA_TO_DEVICE);
								
	struct sk_buff	 *tx_skb[GMAC_TX_DESC_NUM];

	u16 rx_wptr, rx_bptr;
	u16 tx_wptr;

	//link status change. 
	bool phy_reset;
	bool auto_n_completed;
	bool link_established;
	bool transmit_okay;
	
	bool pause_frame_rx;
	bool pause_frame_tx;
	bool duplex_mode;
    bool blink_light; 
    bool in_blink;
	
	u32 link_speed;
	u16 link_partner;	

#if GMAC_VLAN_TAG_USED
	struct vlan_group	*vlgrp;
	bool vlan_tag_remove;
#endif

/* begin async dma */
	volatile u16 tx_skb_wr;
	volatile u16 tx_skb_rd;
	volatile short avail_desc_num;
	skb_info_t gmac_tx_skb[GMAC_TX_DESC_NUM];
    int max_frame_size;
/* end async dma */

/*begin gmac utility */
	u8 toe_tx;
	u8 TsoUfoEn;
	u16 tx_mtu;
	u32 TxCrcErrs;
/*end gmac utility */
    u32 num_rx_complete;
    u32 num_timer;

/* for PLL bug */
    u32 times_of_link_change; 
    u32 unlink_error_state;
    u32 para_detect_times;
    u32 unlink_with_signal_times;
} *pgmac_private;


typedef enum
{
	ETH_PHY_MODE_RMII,
	ETH_PHY_MODE_MII,
	ETH_PHY_MODE_RGMII,	
	ETH_PHY_MODE_GMII,
}eth_phy_mode;

typedef struct _ali_mac_data {
    u16 reg;
    u32 value; 
} ali_mac_data_t;

#define GMAC_R8(reg)		(*(volatile u8 *)(gmac_base + (reg)))
#define GMAC_R16(reg)		(*(volatile u16 *)(gmac_base + (reg)))
#define GMAC_R32(reg)		(*(volatile u32 *)(gmac_base + (reg)))

#define GMAC_W8(reg, val)		(*(volatile u8 *)(gmac_base + (reg))) = (u8)(val)	
#define GMAC_W16(reg, val)	(*(volatile u16 *)(gmac_base + (reg))) = (u16)(val)	
#define GMAC_W32(reg, val)	(*(volatile u32 *)(gmac_base + (reg))) = (u32)(val)


#define ALI_SW_TRACE
#ifdef ALI_SW_TRACE
#define ali_trace(level, msg, args...) \
   {if (test_bit(level, &dbg_runtime_val)) {printk(KERN_INFO msg "\n", ##args);}}
#else
#define ali_trace(level, msg, args...)
#endif 

#define ali_warn(msg,args...) printk(KERN_WARNING msg "\n", ##args)
#define ali_error(msg,args...) printk(KERN_ERR msg "\n", ##args)
#define ali_info(msg,args...) printk(KERN_INFO msg "\n", ##args)

void 
mac_rx_tasklet(unsigned long para);

void phy_set(pgmac_private pgmac);

void 
phy_link_chk(pgmac_private pgmac);

void 
gmac_set_rx_mode(struct net_device *dev);

void show_tx_desc(ptx_data_desc tx_desc);
#endif //__ETHERNET_GMAC__H

