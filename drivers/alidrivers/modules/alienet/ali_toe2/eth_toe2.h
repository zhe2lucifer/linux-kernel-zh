/*
 * Ethernet driver for ALi SoCs
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Shih <david.shih@alitech.com>,
 *          Lucas.Lai  <lucas.lai@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __ETHERNET_TOE2__H
#define __ETHERNET_TOE2__H

#include <linux/phy.h>
#include <linux/interrupt.h>
#include <ali_reg.h>
#define TOE2_DEF_MSG_ENABLE	0x7fff

#define TOE2_TX_TIMEOUT	    (2*HZ)

/* Link mode bits. */
#define TOE2_10HD		(0x01UL)
#define TOE2_10FD		(0x02UL)
#define TOE2_100HD		(0x04UL)
#define TOE2_100FD		(0x08UL)
#define TOE2_RX_PAUSE	(0x10UL)
#define TOE2_TX_PAUSE	(0x20UL)
#define TOE2_AUTONEG	(0x40UL)

/* Rx Mode */
#define TOE2_RX_MODE_ISR		(0UL)
#define TOE2_RX_MODE_NAPI		(2UL)

/* Rx Filtering mode */
#define TOE2_RX_FILTER_PERFECT	(0UL)
#define TOE2_RX_FILTER_HASH		(1UL)
#define TOE2_RX_FILTER_INVERSE	(2UL)
#define TOE2_RX_FILTER_HASHONLY	(3UL)

#define RING_CRC_SZ				4
#define MAX_TOE2_PKT_LEN		(64*1024)

#ifdef CONFIG_ALI_TOE2_128M
#define TOE2_RX_DESC_NUM		64
#define TOE2_TX_DESC_NUM		64
#define TOE2_BUF_SZ				1536
#define TOE2_DESC_SZ			16
#else
#define TOE2_RX_DESC_NUM		512
#define TOE2_TX_DESC_NUM		128
#define TOE2_BUF_SZ				1536
#define TOE2_DESC_SZ			16
#endif

#define TOE2_MSS				1514

/* Setup frame size. */
#define SETUP_FRAME_SZ			192
#define IMPERFECT_PA_OFF		156
/* Hash */
#define TOE2_HASH_TABLE_LEN	    512
#define TOE2_HASH_BITS			0x01ff

#define TOE2_REGS_RANGE			0x98
#define TOE2_REGS_VER			1

#define HIBYTE(word)  ((u8)(((u16)(word))>>8))
#define LOBYTE(word)  ((u8)(((u16)(word))&0x00FFU))
#define HIWORD(dword) ((u16)(((u32)(dword))>>16))
#define LOWORD(dword) ((u16)(((u32)(dword))&0x0000FFFFUL))

struct toe2_tx_desc {
	union {
		struct {
			/* bit 1~0.Reserved. */
			u32 Reserved1_0 : 2;
			/* bit 2. Heartbeat Fail Error. */
			u32 HF : 1;
			/* bit 3. Fifo Underrun Error. */
			u32 FifoUnderrun : 1;
			/* bit 7~4. Collision Count. */
			u32 ColCnt : 4;
			/* bit 8. Excessive Collsion. */
			u32 ExCol : 1;
			/* bit 9. Late Collsion. */
			u32 LateCol : 1;
			/* bit 10. No Carrier. */
			u32 NoCarrier : 1;
			/* bit 11. Loss Of Carrier.. */
			u32 LossOfCarrier : 1;
			/* bit 12. Error Sumary. */
			u32 ES : 1;
			/* bit 27~13. Reserved. */
			u32 Reserved27_13 : 15;
			/* bit 28. indicate a Last Segment Descriptor. */
			u32 LS : 1;
			/* bit 29. indicate a First Segment Descriptor. */
			u32 FS : 1;
			/* bit 30. indicate End Of descriptor Ring. */
			u32 Reserved30 : 1;
			/* bit 31. 0->Owned by SW, 1->Owned by HW. */
			u32 OWN : 1;
		} sw;

		struct {
			/* bit 12~0.Maximum Frame Length(Without CRC) */
			u32 Mfl : 13;
			/* bit 13. */
			u32 VlanEn : 1;
			/* bit 21~14.IP header offset. */
			u32 IpStartOff : 8;
			/* bit 22. indicate UDP packet type. */
			u32 UdpPkt : 1;
			/* bit 23. indicate TCP packet type. */
			u32 TcpPkt : 1;
			/* bit 24. enable Tx COE. */
			u32 CoeEn : 1;
			/* bit 25. indicate Setup packet type. */
			u32 SetupPkt : 1;
			/* bit 27~26. filtering mode for a setup packet. */
			u32 FilteringMode : 2;
			/* bit 28. indicate a Last Segment Descriptor. */
			u32 LS : 1;
			/* bit 29. indicate a First Segment Descriptor. */
			u32 FS : 1;
			/* bit 30. indicate End Of descriptor Ring. */
			u32 EOR : 1;
			/* bit 31. 0->Owned by SW, 1->Owned by HW. */
			u32 OWN : 1;
		} hw;
	} tx_sts;

	dma_addr_t pkt_buf_dma;
	u16 seg_len;
	u16 tot_len;
	u16 vlan_tag;
	u16 seg_num;
} __packed;

struct packet_head {
	u16	PacketLength:12;

	/* TCP Checksum is Okay. */
	u16	TCPChksum:1;
	/* IP Checksum is Okay. */
	u16	IPChksum:1;
	/* PPPoE packet. */
	u16	PPPoE:1;
	/* VLAN Fram. */
	u16	VLAN:1;

	/* Receive Watchdog Time-out. */
	u16	WatchdogTimeout:1;
	/* Frame Type. */
	u16	FrameType:1;
	/* Physical Layer Error. */
	u16	PhysicalLayerError:1;
	/* FIFO Overflow Error. */
	u16	FifoOverflow:1;
	/* Alignment Error. */
	u16	FAE:1;
	/* CRC error. */
	u16	CRC:1;
	/* Late Collision Seen. */
	u16	LateCollision:1;
	/* Excessive Frame Length. */
	u16	Long:1;
	/* Runt Frame. */
	u16	Runt:1;
	/* Error Summary. */
	u16	ES:1;
	/* Broadcast Frame. */
	u16	BF:1;
	/* Physical Frame. */
	u16	PF:1;
	/* Multicast Frame. */
	u16	MF:1;
	/* Dribble Error. */
	u16	Dribble:1;
	/* IP Fragment. */
	u16	IPFrag:1;
	/* IP Frame. */
	u16	IPFrame:1;
} __packed;

struct toe2_rx_desc {
	struct packet_head pkt_hdr;
	u16 l3_chs;
	u16 l4_chs;
	u16 vlan_tag;
	u16 reserved:15;
	u16 EOR:1;
	dma_addr_t pkt_buf_dma;
} __packed;

struct toe2_device_stats {
	/* muticast packets received. */
	u32 rx_mc;
	/* broadcast packets received. */
	u32 rx_bc;
	/* unicast packets received. */
	u32 rx_uc;
	u32 rx_vlan;
	/* pppoe packets received. */
	u32 rx_pppoe;
	u32 rx_ip;
	u32 rx_frag;

	u32 rx_runt_errors;
	u32 rx_long_errors;
	u32 rx_dribble_errors;
	u32 rx_phy_layer_errors;
	u32 rx_wd_timeout_errors;
	u32 rx_ip_chksum_errors;
	u32 rx_tcp_chksum_errors;

	/* there is no packet stored in rx ring buffer. */
	u32 rx_buf_empty;
	u32 rx_late_col_seen;
	u32 rx_lost_in_ring;

	u32 rx_hdr_chs_errs;
	u32 rx_pay_chs_errs;

	/* 0 to 15 collisions. */
	u32 tx_col_cnts[16];
	/* excessive collision. */
	u32 tx_col_errors;
};

#define TOE2_NUM_STATS 36

struct toe2_skb_info {
	u16 first;
	u16 cnt;
	struct sk_buff *skb;
};

struct toe2_private {
	struct net_device *ndev;
	void __iomem *io_base;
	void __iomem *soc_base;
	void __iomem *io_reset;
	u32 chip_id;
	struct device *dev;

	bool acquired_isr;
	u32 irq_num;
	u32 cur_isr;
	u32 isr_mask;

	spinlock_t lock;
	u32 msg_enable;

    u32 phy_addr;
    struct  mii_bus *mdio_bus;
    struct  phy_device *phy_dev;
	struct device_node *phy_node;
    phy_interface_t phy_interface;
    u32 dev_id;
	int link;
	int duplex_mode;
	int link_speed;

	struct napi_struct napi;
	struct toe2_device_stats mac_stats;
	/* struct toe2_rx_stats rx_stats; */

	u8 *setup_buf;
	dma_addr_t setup_buf_dma;

	struct toe2_rx_desc *rx_desc;
	dma_addr_t rx_desc_dma;
	struct sk_buff *rx_skb[TOE2_RX_DESC_NUM];

	struct toe2_tx_desc *tx_desc;
	dma_addr_t tx_desc_dma;
	struct sk_buff *tx_skb[TOE2_TX_DESC_NUM];

	u16 rx_wptr, rx_bptr;
	u16 tx_wptr;

	struct tasklet_struct toe2_tasklet;

	/* link status change. */
	bool auto_n_completed;
	bool transmit_okay;

	bool pause_frame_rx;
	bool pause_frame_tx;

	u16 link_partner;

	bool toe2_dev_is_open;
	u32 toe2_rx_csum;
	u32 toe2_tx_csum;
	bool toe2_sg;
	bool toe2_tso;
	bool toe2_ufo;
	u32 toe2_reversemii;
	int toe2_debug;
	u32 toe2_link_mode;
	u32 toe2_mac_hi16;
	u32 toe2_mac_lo32;

	struct vlan_group *vlgrp;
	bool vlan_tag_remove;

	/* time spot */
	u32 timer_freq;
	bool cur_dny_imk;

	u32 num_rx_complete;
	u32 num_timer;

	u16 tx_skb_wr;
	u16 tx_skb_rd;
	struct toe2_skb_info as_tx_skb[TOE2_TX_DESC_NUM];
	/* patch for 38/39 */
	/* for PLL bug */
	u32 mac_5sec;
	u32 mac_pll_time;

	u32 times_of_link_change;
	u32 unlink_error_state;
	u32 para_detect_times;
	u32 unlink_with_signal_times;
/* begin gmac utility */
	u8 toe_tx;
	u8 TsoUfoEn;
	u16 tx_mtu;
	u32 TxCrcErrs;
/* end gmac utility */

	bool blink_light;
	bool in_blink;
    struct gpio_desc *phy_reset_gpio;
    struct gpio_desc *speed_led_gpio;
    struct gpio_desc *link_led_gpio;
};

typedef struct toe2_private *pgmac_private;
struct ali_mac_priv_io {
	unsigned short  reg;
	unsigned long  value;
};

#define SOC_BASE_ADDR       0x18000000
#define OLD_SOC_R32(i)          (__REG32ALI(SOC_BASE_ADDR+(i)))
#define OLD_SOC_W32(i, val)     ((__REG32ALI(SOC_BASE_ADDR+(i))) = (u32)(val))

static inline u8 TOE2_R8(void __iomem *base, unsigned long reg)
{
	return (u8)ioread8((void __iomem *)(base + reg));
}

static inline u16 TOE2_R16(void __iomem *base, unsigned long reg)
{
	return (u16)ioread16((void __iomem *)(base + reg));
}

static inline u32 TOE2_R32(void __iomem *base, unsigned long reg)
{
	return (u32)ioread32((void __iomem *)(base + reg));
}

static inline void TOE2_W8(void __iomem *base, unsigned long reg, u8 val)
{
	iowrite8(val, (void __iomem *)(base + reg));
}

static inline void TOE2_W16(void __iomem *base, unsigned long reg, u16 val)
{
	iowrite16(val, (void __iomem *)(base + reg));
}

static inline void TOE2_W32(void __iomem *base, unsigned long reg, u32 val)
{
	iowrite32(val, (void __iomem *)(base + reg));
}

static inline void SOC_W32(void __iomem *soc_base, unsigned long reg, u32 val)
{
    if (soc_base != 0) {
        iowrite32(val, (void __iomem *)(soc_base + reg));
    } else {
        OLD_SOC_W32(reg, val);
    }
}

static inline u32 SOC_R32(void __iomem *soc_base, unsigned long reg)
{
    if (soc_base != 0) {
        return (u32)ioread32((void __iomem *)(soc_base + reg));
    } else {
        return OLD_SOC_R32(reg);
    }
}

void toe2_enable_irq(struct toe2_private *ptoe2);
#endif /* __ETHERNET_TOE2__H */

