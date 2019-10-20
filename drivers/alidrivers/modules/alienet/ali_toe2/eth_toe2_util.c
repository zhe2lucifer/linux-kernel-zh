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

#include <linux/netdevice.h>
#include <linux/random.h>
#include <linux/if_vlan.h>
#include <linux/mii.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/crc32.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include <asm/dma.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <linux/workqueue.h>
//#include <asm/mach-ali/m6303.h>
//#include <asm/mach-ali/m36_gpio.h>
//#include <asm/gpio.h>

#include "eth_reg.h"
#include "eth_toe2.h"
#include "eth_toe2_util.h"
#define SWAP(value) (((value<<8)&0xFF00)|((value>>8)&0x00FF))

static u8 MacDstAddr_uni[6]={0x60,0xEB,0x69,0x92,0x8A,0xFE};
static u8 MacSrcAddr_uni[6]={0x00,0x12,0x34,0x56,0x78,0x9a};
static u8 MacDstAddr_mul[6]={0xFF,0xEB,0x69,0x92,0x8A,0xFE};
static u8 MacDstAddr_bro[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
#define TRUE 0
#define FALSE -1
/****************************************************
#define TOE2U_DRV_NAME "ALi Ethernet TOE2 Utility"
#define TOE2U_DRV_VER "Ver 0.1"
 ****************************************************/
#define  MAC_FRAME_IP_TCP   0
#define  MAC_FRAME_IP_UDP   1
#define  MAC_FRAME_IP_TCPV6 2
#define  MAC_FRAME_IP_UDPV6 3
#define  MAC_FRAME_ARP      4
#define  MAC_FRAME_IP_ICMP  5
#define  MAC_FRAME_IP_IGMP  6

#define  MAC_FRAME_SETUP    7
#define  MAC_FRAME_PAUSE    8
#define  MAC_FRAME_PPPoE_IP_ICMP  9
#define  MAC_FRAME_PPPoE_IP_IGMP  10
#define  MAC_FRAME_PPPoE_IP_UDP   11
#define  MAC_FRAME_PPPoE_IP_TCP   12
#define  MAC_FRAME_8023     13 
#define  MAC_FRAME_ETHERII  14

#define  MAC_FILTER_HASH      0
#define  MAC_FILTER_HASHONLY  1
#define  MAC_FILTER_INVERSE   2
#define  MAC_FILTER_PERFECT   3

/******temp*******/
#define MAX_ETH_PKT_LEN	    1536
#define MIN_ETH_PKT_LEN	    64
//#define ETH_HLEN		    14
//#define MAC_VLAN_SIZE		4
//#define MAC_CRC_SIZE		4
//#define ARP_HDR_LEN       28
//#define ICMP_HDR_LEN		12
//#define IGMP_HDR_LEN		8
//#define TCP_HDR_LEN       20
//#define UDP_HDR_LEN       8
#define PPPOE_HDR_LEN		8
//#define MIN_IP_HDR_LEN	20
//#define MAX_IP_HDR_LEN	64
//#define MAX_MFL_LEN	    1536
//Eth Type definitions.

//#define ETH_P_ARP		    0x0806
//#define ETH_P_IP			0x0800
//#define ETH_P_PPP_DISC	0x8863  /* PPP Over Ethernet Discovery Stage */
//#define ETH_P_PPP_SES		0x8864  /* PPP Over Ethernet Session Stage */
//#define ETH_P_8021Q		0x8100

#define ETH_TYPE_OFF        (6+6)
#define ETH_PPPOE_PRO_OFF   (6+6+2+6)
#define ETH_PPPOE_PRO_LEN   2

#define ETH_IP_OFF          (6+6+2)
#define PPPOE_IP            0x21    /* Internet Protocol */

#define IP_LEN_OFF		0
#define IP_TOTLEN_OFF	2
#define IP_PRO_OFF		8
#define IP_IPSRC_OFF	12
#define IP_IPDES_OFF	16

//#define IPPROTO_ICMP    1
//#define IPPROTO_IGMP    2
//#define IPPROTO_UDP     17
//#define IPPROTO_UDPLITE 136
//#define IPPROTO_TCP     6
/*************/

struct toe2_private *g_ptoe2 = NULL;
MAC_ADAPTER util_mac_adapter;
static int m_bRxStopThread;
int mac_inited = 0;

//utility driver init source
#define MAC_RX_DESC_NUM 64
#define MAC_TX_DESC_NUM 64
static unsigned char util_rxbuf_addr[TOE2_BUF_SZ*MAC_RX_DESC_NUM+0x20];
static unsigned char util_txbuf_addr[(MAX_TOE2_PKT_LEN + 1024)*MAC_TX_DESC_NUM+0x20];
//board config
static unsigned int ali_phy_addr = 1;
//static unsigned int ali_physet_gpio;

//rx/tx params
static unsigned char MacSrcAddr[6];
static unsigned char MacDstAddr[6];

static MAC_TX_PARA ali_tx_para;
static struct toe2_tx_desc ali_desc_hw;
static unsigned char ali_vlan_chk[TOE2_BUF_SZ];

static unsigned short ali_desc_size[MAC_TX_DESC_NUM];
static unsigned char ali_rx_buf[MAC_RX_DESC_NUM][TOE2_BUF_SZ];

//static unsigned char MacTxBuf[TOE2_BUF_SZ];      //1536
static unsigned char ToeTxBuf[MAX_TOE2_PKT_LEN]; //64*1024

//crc
static unsigned short ChsAcc, ChsOff;
static unsigned int crc32_table[256];
#define CRC32_POLY 0x04c11db7
#define ReverseBit(data) (((data<<7)&0x80)|((data<<5)&0x40)|((data<<3)&0x20)|((data<<1)&0x10)|\
		((data>>1)&0x08)|((data>>3)&0x04)|((data>>5)&0x02)|((data>>7)&0x01))
#define ReverseByte(data) ((data&0xff)<<8)|((data&0xff00)>>8)

static void init_crc32(void)
{
	int i, j;
	unsigned long c;
	unsigned char *p=(unsigned char *)&c, *p1;
	unsigned char k;

	for (i = 0; i < 256; ++i) 
	{
		k = ReverseBit((unsigned char)i);
		for (c = ((unsigned long)k) << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
		p1 = (unsigned char *)(&crc32_table[i]);

		p1[0] = ReverseBit(p[3]);
		p1[1] = ReverseBit(p[2]);
		p1[2] = ReverseBit(p[1]);
		p1[3] = ReverseBit(p[0]);
	}
}

#if 0
static void gmac_print(unsigned char *p, unsigned short len) {
	int i;
	for(i=0; i<len; i++) {
		if (i%16 == 0) {
			printk("\n0x%08x:  ", (u32)(p+i));
		}

		printk("%02x ", *(p+i));
	}
	printk("\n");
    return;
}
#endif

static unsigned int ALiCrc32(unsigned char *buf, int len)
{
	unsigned char *p;
	unsigned long  crc;

	crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */

	for (p = buf; len > 0; ++p, --len)
	{
		crc = crc32_table[ (crc ^ *p) & 0xff] ^ (crc >> 8);
	}
	return ~crc;    /* transmit complement, per CRC-32 spec */
}

static void ALiCrcChk(PMAC_ADAPTER pAdapter, unsigned char *buf, int len)
{
	u32 Crc, CrcChk, Tmp32;
	u32 k;

	u16 DescWritePtr, DescReadPtr;

	//wait untill no sending pkts. 
	do
	{
		DescReadPtr = TOE2_R16(pAdapter->BaseAddr, TxRingDesRPtr);
		DescWritePtr = pAdapter->TxDescWPtr;

	} while(DescReadPtr != DescWritePtr);

	if ((pAdapter->ToeTxEn) && (ChsOff != 0))
		*(u16 *)(buf + ChsOff) =  ChsAcc; 

	Crc = ALiCrc32(buf, len);
	CrcChk = 0;

	for (k = 0; k < 32; k++)
	{
		Tmp32 = Crc >> k;
		Tmp32 &= 0x1;
		CrcChk |= (Tmp32<<(31 - k));
	}

	CrcChk = ~CrcChk;

	TOE2_W32(pAdapter->BaseAddr, 0x2C/*CRCCHK*/, CrcChk);

	if ((pAdapter->ToeTxEn) && (ChsOff != 0))
		*(u16 *)(buf + ChsOff) =  0; 
	//TOE2_UTILITY_TRACE("chs:%08x,offset:%d, crc:%08x", ChsAcc, ChsOff, CrcChk);
}

static u16 standard_chksum(void *dataptr, u16 len)
{
	u32 acc;
	u16 src;
	u8 *octetptr;

	acc = 0;
	/* dataptr may be at odd or even addresses */
	octetptr = (u8*)dataptr;
	while (len > 1)
	{
		/* declare first octet as most significant
		   thus assume network order, ignoring host order */
		src = (*octetptr) << 8;
		octetptr++;
		/* declare second octet as least significant */
		src |= (*octetptr);
		octetptr++;
		acc += src;
		len -= 2;
	}
	if (len > 0)
	{
		/* accumulate remaining octet */
		src = (*octetptr) << 8;
		acc += src;
	}
	/* add deferred carry bits */
	acc = (acc >> 16) + (acc & 0x0000ffffUL);
	if ((acc & 0xffff0000) != 0)
	{
		acc = (acc >> 16) + (acc & 0x0000ffffUL);
	}
	/* This maybe a little confusing: reorder sum using toe_htons()
	   instead of ntohs() since it has a little less call overhead.
	   The caller must invert bits for Internet sum ! */
	return htons((u16)acc);
}

static u32 standard_chksum_2(void *dataptr, u16 len)
{
	u32 acc;
	u16 src;
	u8 *octetptr;

	acc = 0;
	/* dataptr may be at odd or even addresses */
	octetptr = (u8*)dataptr;
	while (len > 1)
	{
		/* declare first octet as most significant
		   thus assume network order, ignoring host order */
		src = (*octetptr) << 8;
		octetptr++;
		/* declare second octet as least significant */
		src |= (*octetptr);
		octetptr++;
		acc += src;
		len -= 2;
	}

	if (len > 0)
	{
		/* accumulate remaining octet */
		src = (*octetptr) << 8;
		acc += src;
	}

	while ((acc >> 16) != 0)
	{
		acc = (acc & 0xffff) + (acc >> 16);
	}

	return (acc);
}

static unsigned short inet_chksum(void *dataptr, u16 len)
{
	u32 acc;

	acc = standard_chksum(dataptr, len);
	while ((acc >> 16) != 0)
	{
		acc = (acc & 0xffff) + (acc >> 16);
	}
	return (u16)(acc & 0xffff);
}

//copy built-in memory from host memory. 
static int ALICopyFromMem(u32 to, u32 from, u32 len)
{
	int Status = TRUE;

	//dump_out_mem(from, to, len);
	memcpy((unsigned char*)to, (unsigned char *)from, len);
	return Status;
}

#if 0
static int ALICopyToMem(u32 to, unsigned char *from, u32 len)
{
	int Status = TRUE;

	//load_in_mem(to, from, len);
	memcpy((unsigned char *)to, from, len);
	return Status;
}
#endif

static int MacAllocateResource(PMAC_ADAPTER pAdapter)
{
	int i;

	u32 RxDescBuffer;
	u32 TxDescBuffer;

	TOE2_UTILITY_TRACE("UTILITY Memory Allocation:\n");			

	RxDescBuffer = (u32)(&util_rxbuf_addr[0]);
	TxDescBuffer = (u32)(&util_txbuf_addr[0]);
	TOE2_UTILITY_TRACE(" 1: rx: %x", RxDescBuffer);
	TOE2_UTILITY_TRACE(" 1: tx: %x\n", TxDescBuffer);

	RxDescBuffer = (RxDescBuffer+0x1f)&0xFFFFFFE0;
	TxDescBuffer = (TxDescBuffer+0x1f)&0xFFFFFFE0;
	TOE2_UTILITY_TRACE(" 2: rx: %x", RxDescBuffer);
	TOE2_UTILITY_TRACE(" 2: tx: %x\n", TxDescBuffer);

	for (i = 0; i < MAC_RX_DESC_NUM; i++)
	{
		pAdapter->RxDescBuf[i] = RxDescBuffer + TOE2_BUF_SZ*i;
//		pAdapter->TxDescBuf[i] = TxDescBuffer + MAX_TOE2_PKT_LEN*i;// + i;
		//TOE2_UTILITY_TRACE(" sub: rx: %x", pAdapter->RxDescBuf[i]);
	    //TOE2_UTILITY_TRACE(" sub: tx: %x", pAdapter->TxDescBuf[i]);

		pAdapter->RxDescBuf_DMA[i] = dma_map_single((struct device *)NULL,(void*)pAdapter->RxDescBuf[i], TOE2_BUF_SZ, DMA_FROM_DEVICE);
//		pAdapter->TxDescBuf_DMA[i] = dma_map_single((struct device *)NULL, pAdapter->TxDescBuf[i], MAX_TOE2_PKT_LEN, DMA_FROM_DEVICE);
		//TOE2_UTILITY_TRACE(" dma: rx: %x", pAdapter->RxDescBuf_DMA[i]);
	    //TOE2_UTILITY_TRACE(" dma: tx: %x\n", pAdapter->TxDescBuf_DMA[i]);
	}

	pAdapter->RxBuffer = (unsigned int)&ali_rx_buf[0][0];
	TOE2_UTILITY_TRACE(" RxBuffer: tx: %x\n", pAdapter->RxBuffer);
	return 0;
}

static void MacInitRing(PMAC_ADAPTER pAdapter)
{
	TOE2_UTILITY_TRACE("MacInitRing()");

	pAdapter->pSetupBuf = g_ptoe2->setup_buf;
	pAdapter->pSetupBuf_DMA= g_ptoe2->setup_buf_dma;
	pAdapter->pRxDesc = g_ptoe2->rx_desc;
	pAdapter->pRxDesc_DMA = g_ptoe2->rx_desc_dma;
	pAdapter->pTxDesc = g_ptoe2->tx_desc;
	pAdapter->pTxDesc_DMA = g_ptoe2->tx_desc_dma;

	TOE2_UTILITY_TRACE("MacInitRing pTxDesc addr:     0x%x", pAdapter->pTxDesc);
	TOE2_UTILITY_TRACE("MacInitRing pTxDesc_DMA addr: 0x%x\n", pAdapter->pTxDesc_DMA);

	pAdapter->RxDescWPtr = MAC_RX_DESC_NUM -1;
	pAdapter->RxBufRPtr= 0;
	pAdapter->TxDescWPtr= 0;	

	pAdapter->InterruptMask = TOE2_INTERRUPT_MASK; 
	pAdapter->CurrentISR = 0;
}

#define	MdioDelay(MdioAddr) TOE2_R32(pAdapter->BaseAddr, MdioAddr)

static void MacChipReset(PMAC_ADAPTER pAdapter)
{
	u8 tmp_u8;
	TOE2_UTILITY_TRACE("TOE2: %s()=>O", __FUNCTION__);
	TOE2_W8(pAdapter->BaseAddr, SCR, SCRReset|TOE2_R8(pAdapter->BaseAddr, SCR));	
	do
	{
		TOE2_UTILITY_TRACE("->");	
		tmp_u8 = TOE2_R8(pAdapter->BaseAddr, SCR);
	} while(tmp_u8 & SCRReset);
	TOE2_UTILITY_TRACE("K!\n");
}

static void MdioSync(PMAC_ADAPTER pAdapter)
{
	int i;
	for (i = 32; i >= 0; i--)
	{
		TOE2_W32(pAdapter->BaseAddr, MiiMR1, MiiMdioWrite1|MiiMdioEn);
		MdioDelay(MiiMR1);
		TOE2_W32(pAdapter->BaseAddr, MiiMR1, MiiMdioWrite1|MiiMdioClk|MiiMdioEn);
		MdioDelay(MiiMR1);
	}
	//TOE2_UTILITY_TRACE("MdioSync: end\n");
}

static u32 MdioRead(PMAC_ADAPTER pAdapter, u32 Phyaddr, u32 RegAddr)
{
	u16 tmp_16;
	u32 tmp_32;
	u32 addr;
	u32 cnt = 0;
	u32 retval = 0;

	int i,	DataVal = 0;
	u32 mii_cmd = (0xf6 << 10) | (Phyaddr << 5) | RegAddr;
	u32 Tmplong, Cmdlong;

	if (pAdapter->HwMdioEn == TRUE)
	{
		addr = ((Phyaddr << MiiPhyAddrOff) & MiiPhyAddrMask)|((RegAddr << MiiRegAddrOff) & MiiRegAddrMask);

		tmp_32 = TOE2_R32(pAdapter->BaseAddr, MiiMR2);
		tmp_32 &= ~(MiiPhyAddrMask|MiiRegAddrMask|MiiOpMask);

		TOE2_W32(pAdapter->BaseAddr, MiiMR2, (tmp_32|addr|MiiOpRead|MiiTransStart));

		do
		{
			cnt ++;
			tmp_32 = TOE2_R32(pAdapter->BaseAddr, MiiMR2);
			mdelay(1);
		} while(tmp_32&MiiTransStart);

		//TOE2_UTILITY_TRACE("MdioRead: cnt(%d)\n", cnt);
		tmp_16 = TOE2_R16(pAdapter->BaseAddr, MdioR);

		return (u32)tmp_16;
	}
	else
	{
		//TOE2_UTILITY_TRACE("MdioRead: start.\n");
		MdioSync(pAdapter);

		/* Shift the read command bits out. */
		for (i = 15; i >= 0; i--)
		{
			DataVal = (mii_cmd & (1 << i)) ? MiiMdioOut : 0;

			TOE2_W32(pAdapter->BaseAddr, MiiMR1, MiiMdioDir|DataVal|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
			TOE2_W32(pAdapter->BaseAddr, MiiMR1, MiiMdioDir|DataVal|MiiMdioClk|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
		}

		/* Read the two transition, 16 data, and wire-idle bits. */
		for (i = 19; i > 0; i--)
		{
			TOE2_W32(pAdapter->BaseAddr, MiiMR1, 0|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);

			Cmdlong = TOE2_R32(pAdapter->BaseAddr, MiiMR1);
			retval = (retval << 1) | ((Cmdlong & MiiMdioIn) ? 1 : 0);

			TOE2_W32(pAdapter->BaseAddr, MiiMR1, MiiMdioClk|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
		}
		return (retval >> 1) & 0xffff;
	}
}

static void MdioWrite(PMAC_ADAPTER pAdapter, u32 Phyaddr, u32 RegAddr, u32 Value)
{
	u32 tmp_32;
	u32 addr;
	u32 cnt = 0;
	u32 mii_cmd = (0x5002 << 16) | (Phyaddr << 23) | (RegAddr << 18) | Value;
	u32 i;
	u32 Tmplong;

	if (pAdapter->HwMdioEn == TRUE)
	{		
		TOE2_W16(pAdapter->BaseAddr, MdioW, (u16)Value);

		addr = ((Phyaddr << MiiPhyAddrOff) & MiiPhyAddrMask)|((RegAddr << MiiRegAddrOff) & MiiRegAddrMask);

		tmp_32 = TOE2_R32(pAdapter->BaseAddr, MiiMR2);
		tmp_32 &= ~(MiiPhyAddrMask|MiiRegAddrMask|MiiOpMask);

		TOE2_W32(pAdapter->BaseAddr, MiiMR2, (tmp_32|addr|MiiOpWrite|MiiTransStart));

		do
		{
			cnt ++;
			tmp_32 = TOE2_R32(pAdapter->BaseAddr, MiiMR2);
			mdelay(1);
		} while(tmp_32&MiiTransStart);

		//TOE2_UTILITY_TRACE("MdioWrite: cnt(%d)\n", cnt);
	}
	else
	{
		//TOE2_UTILITY_TRACE("MdioWrite: start.\n");
		MdioSync(pAdapter);

		/* Shift the command bits out. */
		for (i = 31; i >= 0; i--)
		{
			int dataval = (mii_cmd & (1 << i)) ? MiiMdioWrite1 : MiiMdioWrite0;
			TOE2_W32(pAdapter->BaseAddr, MiiMR1, dataval|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
			TOE2_W32(pAdapter->BaseAddr, MiiMR1, dataval|MiiMdioClk|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
		}

		/* Clear out extra bits. */
		for (i = 2; i > 0; i--)
		{
			TOE2_W32(pAdapter->BaseAddr, MiiMR1, 0|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
			TOE2_W32(pAdapter->BaseAddr, MiiMR1, MiiMdioClk|MiiMdioEn);
			Tmplong = MdioDelay(MiiMR1);
		}
	}
}

static void PhySet(PMAC_ADAPTER pAdapter)
{
	TOE2_UTILITY_TRACE("PhySet()");

	//reset phy registers in a default state.
	//MdioWrite(pAdapter, ALi_PhyAddr, PhyBasicModeCtrl, BMCRReset);

    ali_phy_addr = g_ptoe2->phy_addr;

	pAdapter->Duplex = g_ptoe2->duplex_mode;
	pAdapter->Speed = (g_ptoe2->link_speed == 100) ? TRUE:FALSE;

	pAdapter->PauseFrameRx = g_ptoe2->pause_frame_rx;
	pAdapter->PauseFrameTx = g_ptoe2->pause_frame_tx;

	TOE2_UTILITY_TRACE("[NOTE] phy addr is: %d.", ali_phy_addr);
	TOE2_UTILITY_TRACE("[NOTE] phy duplex_mode is: %d.", pAdapter->Duplex);
	TOE2_UTILITY_TRACE("[NOTE] phy link_speed is: %d.", pAdapter->Speed);
	TOE2_UTILITY_TRACE("[NOTE] phy PauseFrame is: [%d,%d].", pAdapter->PauseFrameRx, pAdapter->PauseFrameTx);
}

static void PhySet_Force(PMAC_ADAPTER pAdapter)
{
	u16 BMCR = 0, TmpShort;

	do
	{	
		TmpShort = (u16)MdioRead(pAdapter, ali_phy_addr, PhyBasicModeCtrl);
	} while(TmpShort & BMCRReset);
	TOE2_UTILITY_TRACE("SetMII(): MII reset complete.");

	if (pAdapter->Speed == TRUE)
	{
		TOE2_UTILITY_TRACE("PhySet_Force(): 100M-");
		BMCR |= BMCRSpeedSet;
		if (pAdapter->Duplex == TRUE)
		{
			TOE2_UTILITY_TRACE("FD");
			BMCR |= BMCRDuplexMode;
		}
		else
		{
			TOE2_UTILITY_TRACE("HD");
			BMCR &= ~BMCRDuplexMode;
		}
	}		
	else
	{
		TOE2_UTILITY_TRACE("PhySet_Force(): 10M-");
		BMCR &= ~BMCRSpeedSet;
		if (pAdapter->Duplex == TRUE)
		{
			TOE2_UTILITY_TRACE("FD");
			BMCR |= BMCRDuplexMode;
		}
		else
		{
			TOE2_UTILITY_TRACE("HD");
			BMCR &= ~BMCRDuplexMode;
		}
	}

	//enable Rx/Tx pause frame.
	TmpShort = (u16)MdioRead(pAdapter, ali_phy_addr, PhyBasicModeCtrl);
	TmpShort &= ~(BMCRDuplexMode|BMCRSpeedSet);
	MdioWrite(pAdapter, ali_phy_addr, PhyBasicModeCtrl, BMCR|TmpShort);
}

static int MacHardwareStart(PMAC_ADAPTER pAdapter)
{
	u32 DuplexMode = 0;
	u32 PauseFrame = 0;	
	u32 tmp_u32;
	u32 rmii_speed = 0;

	TOE2_UTILITY_TRACE("MacHardwareStart()");

	tmp_u32 = TOE2_R32(pAdapter->BaseAddr, DelayControlR);
	tmp_u32 &= ~CBR_DW_DLY;
	TOE2_W32(pAdapter->BaseAddr, DelayControlR, tmp_u32 | (1<<CBR_DW_DLY_OFF));
	tmp_u32 = TOE2_R32(pAdapter->BaseAddr, MiiMR1);

	if (pAdapter->HwMdioEn == TRUE)
		tmp_u32 &= ~MiiMdioEn;
	else
		tmp_u32 |= MiiMdioEn;
	TOE2_W32(pAdapter->BaseAddr, MiiMR1, tmp_u32);

	//check phy registers first.
	if (pAdapter->AutoNeg == TRUE)
	{
		PhySet(pAdapter);	//Auto-Neg
	}
	else
	{
		PhySet_Force(pAdapter);
	}

	//set mac address.
	TOE2_W32(pAdapter->BaseAddr, PAR, *((u32*)(pAdapter->MacAddr + 0)));
	TOE2_W32(pAdapter->BaseAddr, PAR - 4, *((u32*)(pAdapter->MacAddr + 4)));

	//TOE2_W32(pAdapter->BaseAddr, TimerR, 0x1bfffff); //28M Clk
	TOE2_W32(pAdapter->BaseAddr, TimerR, 0x31fffff); //50M Clk

	//Set RMII.
	tmp_u32 = TOE2_R32(pAdapter->BaseAddr, RmiiCR);
	tmp_u32 &= ~(RmiiCrSpeed|RmiiEn);	
	if (pAdapter->Speed == TRUE) {
		rmii_speed = (u32)RmiiCrSpeed;
	} else {
		rmii_speed = 0;
	}
	if (pAdapter->RmiiEn == TRUE) {
		TOE2_W32(pAdapter->BaseAddr, RmiiCR, (tmp_u32|rmii_speed|RmiiEn));
	} else {
		TOE2_W32(pAdapter->BaseAddr, RmiiCR, (tmp_u32|rmii_speed|RgmiiEn));
	}

	if (pAdapter->Duplex)
		DuplexMode = (u32)FullDuplexMode;
	else 
		DuplexMode = (u32)0;

	if (pAdapter->ToeRxEn)
		DuplexMode |= RxTOEWorkMode;	

	TOE2_W32(pAdapter->BaseAddr, NetworkOM, (DuplexMode|NetworkOMConfig)); 

	//Test Mux
	//TOE2_W8(pAdapter->BaseAddr, 0x58, 0x0E);  

	if (pAdapter->PauseFrameRx)	
		PauseFrame |= (u32)RxFlowControlEn;
	if (pAdapter->PauseFrameTx)	
		PauseFrame |= (u32)TxFlowControlEn;

	//Tx & Rx Config 2
	if (pAdapter->VlanSupport)
	{
		printk("vlan_support\n");
		if (pAdapter->VlanTagRemove)
			TOE2_W32(pAdapter->BaseAddr, TxRxCR2, (PauseFrame|TxRxConfig2|RxRemoveVlanTagEn|VlanEn));
		else
			TOE2_W32(pAdapter->BaseAddr, TxRxCR2, (PauseFrame|TxRxConfig2|VlanEn));
	}
	else
		TOE2_W32(pAdapter->BaseAddr, TxRxCR2, (PauseFrame|TxRxConfig2));

	TOE2_W32(pAdapter->BaseAddr, TSAD, pAdapter->pTxDesc_DMA);
	TOE2_W32(pAdapter->BaseAddr, RSAD, pAdapter->pRxDesc_DMA);

	TOE2_W16(pAdapter->BaseAddr, RxDesTotNum, MAC_RX_DESC_NUM);
	TOE2_W16(pAdapter->BaseAddr, TxDesTotNum, MAC_TX_DESC_NUM);

	TOE2_W16(pAdapter->BaseAddr, RxRingDesWPtr, MAC_RX_DESC_NUM - 1);
	TOE2_W16(pAdapter->BaseAddr, TxRingDesWPtr, 0);

	//Clr Cnts.
	TOE2_W16(pAdapter->BaseAddr, MFC, 0);
	TOE2_W16(pAdapter->BaseAddr, PPC, 0);
	TOE2_W16(pAdapter->BaseAddr, LFC, 0);
	TOE2_W16(pAdapter->BaseAddr, RPC, 0);
	TOE2_W16(pAdapter->BaseAddr, AlignErrCnt, 0);
	TOE2_W16(pAdapter->BaseAddr, CrcErrCnt, 0);
	TOE2_W16(pAdapter->BaseAddr, IPHdrChsFailPC, 0);
	TOE2_W16(pAdapter->BaseAddr, IPPayChsFailPC, 0);

	//Dis & Clr Int.
	TOE2_W32(pAdapter->BaseAddr, IMR, 0);
	TOE2_W32(pAdapter->BaseAddr, ISR, 0);

	return 0;
}

//long phy_detect(struct toe2_private *ptoe2);
static void Mac_Init(PMAC_ADAPTER pAdapter)
{
	TOE2_UTILITY_TRACE("toe2 start phy hw reset...");
//	ali_gpio_set_value(ali_physet_gpio, 0);
//	mdelay(10);
//	ali_gpio_set_value(ali_physet_gpio, 1);
#ifdef UTIL_DEBUG
    pAdapter->AutoNeg = 1;
    pAdapter->HwMdioEn = 1;
#endif

    	TOE2_UTILITY_TRACE("  BasicAddr =      %x", pAdapter->BaseAddr);
	TOE2_UTILITY_TRACE("  AutoNeg =        %d", pAdapter->AutoNeg);
	TOE2_UTILITY_TRACE("  Speed =          %d", pAdapter->Speed);
	TOE2_UTILITY_TRACE("  Duplex =         %d", pAdapter->Duplex);
	TOE2_UTILITY_TRACE("  RmiiEn =         %d", pAdapter->RmiiEn);
	TOE2_UTILITY_TRACE("  HwMdioEn =       %d", pAdapter->HwMdioEn);
	TOE2_UTILITY_TRACE("  VlanSupport =    %d", pAdapter->VlanSupport);
	TOE2_UTILITY_TRACE("  VlanTagRemove =  %d", pAdapter->VlanTagRemove);
	TOE2_UTILITY_TRACE("  ToeRxEn =        %d", pAdapter->ToeRxEn);

	MacChipReset(pAdapter);

	((u16*)pAdapter->MacAddr)[0] = 0x0012;
	((u16*)pAdapter->MacAddr)[1] = 0x3456;
	((u16*)pAdapter->MacAddr)[2] = 0x789a;
	TOE2_UTILITY_TRACE("Mac addr: 00-12-34-56-78-9a.");

	// phy_detect(g_ptoe2);
	MacAllocateResource(pAdapter);
	MacInitRing(pAdapter);
	init_crc32();
	MacHardwareStart(pAdapter);
	pAdapter->link_status = 0;
}

int util_mac_init(struct toe2_private *ptoe2, struct ali_mac_init_io *init_info) {
	g_ptoe2  = ptoe2;
	memset(&util_mac_adapter, 0, sizeof(MAC_ADAPTER));
	util_mac_adapter.BaseAddr = ptoe2->io_base; 
	util_mac_adapter.Duplex = init_info->duplex;
	util_mac_adapter.Speed = init_info->speed;
	util_mac_adapter.AutoNeg = init_info->autonego;
	util_mac_adapter.RmiiEn = init_info->rmii_rgmii;
#if 0
	util_mac_adapter.HwMdioEn = xmit_info->hw_mdio_en;
	util_mac_adapter.VlanSupport = xmit_info->vlan_support;
	util_mac_adapter.VlanTagRemove = xmit_info->vlan_tag_remove;
	util_mac_adapter.ToeRxEn = init_cxt->ToeRxEn;
#endif
#if 0
	util_mac_adapter.Duplex = 1;
	util_mac_adapter.Speed = 1;
	util_mac_adapter.AutoNeg = 1;
	util_mac_adapter.RmiiEn = 0;
#endif
	util_mac_adapter.HwMdioEn = 1;
	util_mac_adapter.VlanSupport = 1;
	util_mac_adapter.VlanTagRemove = 0;
	util_mac_adapter.ToeRxEn = 0;
	Mac_Init(&util_mac_adapter);
	return 0;
}

int util_mac_init_tx(struct toe2_private *ptoe2, struct ali_mac_xmit_io *xmit_info) {
	memset(&util_mac_adapter, 0, sizeof(MAC_ADAPTER));
//	util_mac_adapter.BaseAddr = _TOE2_BASE - ALI_REGS_PHYS_BASE + ALI_REGS_VIRT_BASE;//__REG32ALI(_TOE2_BASE);
	g_ptoe2  = ptoe2;
	util_mac_adapter.BaseAddr = ptoe2->io_base; 
	util_mac_adapter.Duplex = xmit_info->duplex;
	util_mac_adapter.Speed = xmit_info->speed;
	util_mac_adapter.AutoNeg = xmit_info->autonego;
	util_mac_adapter.RmiiEn = xmit_info->rmii_rgmii;
#if 0
	util_mac_adapter.HwMdioEn = xmit_info->hw_mdio_en;
	util_mac_adapter.VlanSupport = xmit_info->vlan_support;
	util_mac_adapter.VlanTagRemove = xmit_info->vlan_tag_remove;
	util_mac_adapter.ToeRxEn = init_cxt->ToeRxEn;
#endif
#if 0
	util_mac_adapter.Duplex = 1;
	util_mac_adapter.Speed = 1;
	util_mac_adapter.AutoNeg = 1;
	util_mac_adapter.RmiiEn = 0;
#endif
	util_mac_adapter.HwMdioEn = 1;
	util_mac_adapter.VlanSupport = 1;
	util_mac_adapter.VlanTagRemove = 0;
	util_mac_adapter.ToeRxEn = 0;
	Mac_Init(&util_mac_adapter);
	return 0;
}

static void MacWeirdInterrupt(PMAC_ADAPTER pAdapter)
{
	u16 TmpShort;

	TmpShort = TOE2_R16(pAdapter->BaseAddr, MFC);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, MFC, 0);
		pAdapter->net_stats.rx_missed_errors += TmpShort;
		pAdapter->net_stats.rx_over_errors += TmpShort;
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, PPC);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, PPC, 0);
		pAdapter->net_stats.rx_fifo_errors+= TmpShort;
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, LFC);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, LFC, 0);
		pAdapter->net_stats.rx_length_errors += TmpShort;	
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, RPC);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, RPC, 0);
		pAdapter->net_stats.rx_length_errors += TmpShort;	
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, CrcErrCnt);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, CrcErrCnt, 0);
		pAdapter->net_stats.rx_crc_errors += TmpShort;	
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, AlignErrCnt);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, AlignErrCnt, 0);
		pAdapter->net_stats.rx_frame_errors += TmpShort;	
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, IPHdrChsFailPC);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, IPHdrChsFailPC, 0);
		pAdapter->mac_stats.rx_hdr_chs_errs+= TmpShort;
	}

	TmpShort = TOE2_R16(pAdapter->BaseAddr, IPPayChsFailPC);
	if (TmpShort)
	{
		TOE2_W16(pAdapter->BaseAddr, IPPayChsFailPC, 0);
		pAdapter->mac_stats.rx_pay_chs_errs+= TmpShort;
	}

	if ((pAdapter->CurrentISR & (ISRLinkStatus|ISRTimer)) && (pAdapter->AutoNeg))
	{
		TmpShort = (u16)MdioRead(pAdapter, ali_phy_addr, PhyBasicModeStatus);
		if (TmpShort & BMSRLinkStatus)
		{
			if (!pAdapter->link_status) {
				pr_info("%s : Link Connected\n", __FUNCTION__);
				pAdapter->link_status = 1;
#if 0 /* too mess */
			TmpShort = (u16)MdioRead(pAdapter, ali_phy_addr, PhyNWayLPAR);
			if (TmpShort != pAdapter->LinkPartner)
			{
				if (TmpShort)
				{
					TOE2_UTILITY_TRACE("MacWeirdInterrupt: Link Connected. \n");
					//Mac_Init(pAdapter);
				}
			}
#endif
			}
		}
		else {
			if (pAdapter->link_status) {
				TOE2_UTILITY_TRACE("MacWeirdInterrupt: Link Disconnected. \n");
				pAdapter->link_status = 0;
			}
		}
	}
}

//analyze & recode rx status while head is okay.
static int ReceiveChksumOk(PMAC_ADAPTER pAdapter, struct packet_head *pHead)
{	
	if (pAdapter->ToeRxEn)
	{
		if (pHead->IPFrame)
		{
			if (pHead->IPFrag)
				goto Done;

			if (!pHead->IPChksum)
			{
				pAdapter->mac_stats.rx_ip_chksum_errors++;
				TOE2_UTILITY_TRACE("ReceiveChksumOk: ip checksum err");
				goto Done;
			}

			if (!pHead->TCPChksum)
			{
				pAdapter->mac_stats.rx_tcp_chksum_errors++;
				TOE2_UTILITY_TRACE("ReceiveChksumOk: tcp checksum err");
				goto Done;
			}
			return TRUE;
		}
	}

Done:
	return FALSE;
}

static int ReceiveHeadOK(PMAC_ADAPTER pAdapter, struct packet_head *pHead)
{
	int fatal_err = 0;

	if (pHead->ES)
	{
		if (pHead->WatchdogTimeout)
		{
			fatal_err ++;
			pAdapter->mac_stats.rx_wd_timeout_errors++;
		}
		if (pHead->PhysicalLayerError)
		{
			fatal_err ++;
			pAdapter->mac_stats.rx_phy_layer_errors++;
		}
		if (pHead->LateCollision)
		{
			fatal_err ++;
			pAdapter->mac_stats.rx_late_col_seen++;
		}
		if (pHead->Long)
		{
			fatal_err ++;
			pAdapter->mac_stats.rx_long_errors++;
		}
		if (pHead->Runt) 
		{
			fatal_err ++;
			pAdapter->mac_stats.rx_runt_errors++;	
		}
		if (pHead->Dribble)
		{
			fatal_err ++;
			pAdapter->mac_stats.rx_dribble_errors++;	
		}

		if ((pHead->FifoOverflow) && (0 == fatal_err))
		{
			return TRUE;
		}
		else
		{
			pAdapter->net_stats.rx_errors++;
			return FALSE;
		}
	}	
	else
	{
		if ((pHead->PacketLength > MAX_ETH_PKT_LEN) || (pHead->PacketLength < MIN_ETH_PKT_LEN))
			return FALSE;

		pAdapter->net_stats.rx_packets++;
		pAdapter->net_stats.rx_bytes += pHead->PacketLength;

		if (pHead->BF)
			pAdapter->mac_stats.rx_bc++;
		if (pHead->PF)
			pAdapter->mac_stats.rx_uc++;
		if (pHead->MF)
			pAdapter->mac_stats.rx_mc++;
		if (pHead->PPPoE)
			pAdapter->mac_stats.rx_pppoe++;
		if (pHead->VLAN)
			pAdapter->mac_stats.rx_vlan++;
		if (pHead->IPFrame)
			pAdapter->mac_stats.rx_ip++;
		if (pHead->IPFrag)
			pAdapter->mac_stats.rx_frag++;

		return TRUE;
	}	
}

static int ToeTCPCheck(PMAC_ADAPTER pAdapter, struct packet_head *pHead, u8 *pChkStart, u16 ChkLen,
		u32 ip_src, u32 ip_des, u16 ip_protocol, u16 TCPChksum)
{
	int ret = TRUE;
	u32 acc;

	acc = standard_chksum(pChkStart, ChkLen);

	while ((acc >> 16) != 0) {
		acc = (acc & 0xffff) + (acc >> 16);
	}

	acc += (ip_src & 0xFFFFUL);
	acc += ((ip_src>>16) & 0xFFFFUL);

	acc += (ip_des & 0xFFFFUL);
	acc += ((ip_des>>16) & 0xFFFFUL);

	acc += (u32)htons(ip_protocol);
	acc += (u32)htons(ChkLen);

	while ((acc >> 16) != 0) {
		acc = (acc & 0xffffUL) + (acc >> 16);
	}

	if ((u16)acc != TCPChksum)	
	{
		TOE2_UTILITY_TRACE("ToeTCPCheck: err.\n");
		ret = FALSE;
	}

	return ret;	
}

static int ToeICMPCheck(PMAC_ADAPTER pAdapter, struct packet_head *pHead, u8 *pChkStart, u16 ChkLen, u16 TCPChksum)
{
	int ret = TRUE;
	u16 tmp_u16;

	tmp_u16 = inet_chksum(pChkStart, ChkLen);

	if (tmp_u16 != TCPChksum)	
	{
		TOE2_UTILITY_TRACE("ToeICMPCheck: err.\n");
		ret = FALSE;
	}

	return ret;	
}

static int ToeIPCheck(PMAC_ADAPTER pAdapter, struct packet_head *pHead, u8 *pPkt, u16 IPChksum, u16 TCPChksum)
{
	int ret = TRUE;
	u32 ip_src, ip_des;
	u16 ip_hdrlen, ip_totlen, ip_chksum;
	u16 eth_type;
	u16 tmp_u16;
	u16 chk_len;
	u8 ip_protocol;
	u8 *chk_start = pPkt;

	tmp_u16 = *(u16 *)(pPkt + ETH_TYPE_OFF);
	eth_type = htons(tmp_u16);

	if (pHead->IPFrame)
	{
		if (pHead->PPPoE)
		{
			if (eth_type != ETH_P_PPP_SES)
			{
				TOE2_UTILITY_TRACE("ToeCheck: eth_type != ETHTYPE_PPPOE, Eth type err.\n");
				ret = FALSE;
				goto ToeCheckEnd;
			}

			chk_start += ETH_PPPOE_PRO_OFF;

			tmp_u16 = *((u16 *)chk_start);
			tmp_u16 = htons(tmp_u16);

			if (tmp_u16 != PPPOE_IP)
			{
				TOE2_UTILITY_TRACE("ToeCheck: tmp_u16 != PPPOE_IP, No need to do chksum.\n");
				ret = TRUE;
				goto ToeCheckEnd;
			}			

			chk_start += ETH_PPPOE_PRO_LEN;
		}
		else
		{
			chk_start += ETH_IP_OFF;
		}

		tmp_u16 = *(u16 *)(chk_start + IP_LEN_OFF);
		tmp_u16 = htons(tmp_u16);
		ip_hdrlen = ((tmp_u16>>8) & 0xF) * 4;

		tmp_u16 = *(u16 *)(chk_start + IP_TOTLEN_OFF);
		ip_totlen = htons(tmp_u16);		

		tmp_u16 = *(u16 *)(chk_start + IP_PRO_OFF);
		tmp_u16 = htons(tmp_u16);
		ip_protocol = (u8)(tmp_u16 & 0xFF);

		//ip_src = *(u32 *)(chk_start + IP_IPSRC_OFF);
		//ip_des = *(u32*)(chk_start + IP_IPDES_OFF);

		ip_src = (chk_start + IP_IPSRC_OFF)[0]|((chk_start + IP_IPSRC_OFF)[1]<<8)|((chk_start + IP_IPSRC_OFF)[2]<<16)|((chk_start + IP_IPSRC_OFF)[3]<<24);
		ip_des = (chk_start + IP_IPDES_OFF)[0]|((chk_start + IP_IPDES_OFF)[1]<<8)|((chk_start + IP_IPDES_OFF)[2]<<16)|((chk_start + IP_IPDES_OFF)[3]<<24);

		ip_chksum = inet_chksum(chk_start, ip_hdrlen);
		if (ip_chksum != IPChksum)
		{
			TOE2_UTILITY_TRACE("ToeCheck: (ip_chksum != IPChksum).\n");
			ret = FALSE;
			goto ToeCheckEnd;
		}			

		if ((pHead->IPChksum) && (IPChksum != 0xFFFF)) //IP chksum is ok.
		{
			TOE2_UTILITY_TRACE("ToeCheck: ((pHead->IPChksum) && (IPChksum != 0xFFFF)).\n");
			ret = FALSE;
			goto ToeCheckEnd;
		}

		//ip payloader
		chk_start += ip_hdrlen;
		chk_len = ip_totlen - ip_hdrlen;

		switch(ip_protocol)
		{
			case IPPROTO_TCP:
			case IPPROTO_UDP:	
			case IPPROTO_UDPLITE:
				ret = ToeTCPCheck(pAdapter, pHead, chk_start, chk_len, 
						ip_src, ip_des, (u16)ip_protocol, TCPChksum);
				break;

			case IPPROTO_ICMP:
			case IPPROTO_IGMP:
				ret = ToeICMPCheck(pAdapter, pHead, chk_start, chk_len, TCPChksum);
				break;

			default:
				TOE2_UTILITY_TRACE("ToeCheck: Unknow ip_protocol.\n");
				ret = ToeICMPCheck(pAdapter, pHead, chk_start, chk_len, TCPChksum);
				break;				
		}

	}
	else //(pHead->IPFrame)
	{
		if (pHead->PPPoE)
		{
			if ((eth_type != ETH_P_PPP_SES) && (eth_type != ETH_P_PPP_DISC))
			{
				TOE2_UTILITY_TRACE("ToeCheck: (eth_type != ETHTYPE_PPPOE) && (eth_type != ETHTYPE_PPPOEDISC).\n");
				ret = FALSE;
				goto ToeCheckEnd;
			}
		}
		else
		{
			;//TOE2_UTILITY_TRACE("ToeCheck: Unknow IP type.\n");
		}
	}

ToeCheckEnd:

	return ret;
}

static void MacRxStop(PMAC_ADAPTER pAdapter)
{
	u8 TmpChar;

	TmpChar = TOE2_R8(pAdapter->BaseAddr, SCR);

	if (pAdapter->ToeRxEn)
		TOE2_W8(pAdapter->BaseAddr, SCR, (TmpChar&(~(u8)SCRRxEn)&(~(u8)SCRRxCoeEn)));
	else	
		TOE2_W8(pAdapter->BaseAddr, SCR, (TmpChar&(~(u8)SCRRxEn)));
}

static unsigned short ReceiveUpdateDescPtr(PMAC_ADAPTER pAdapter)
{
	u16 BufReadPtr, DescReadPtr, DescWritePtr;
	u16 Updata = 0;
	u32 RxDescD0, RxDescD1, RxDescD2, RxDescD3;
	u32 i;

	BufReadPtr = pAdapter->RxBufRPtr;
	DescReadPtr = TOE2_R16(pAdapter->BaseAddr, RxRingDesRPtr);
	DescWritePtr = pAdapter->RxDescWPtr;

	//TOE2_UTILITY_TRACE("DescReadPtr:%d, BufReadPtr:%d, DescWritePtr:%d.\n", DescReadPtr, BufReadPtr, DescWritePtr);

	//Complicated here...
	if (DescWritePtr > DescReadPtr)
	{
		if ((BufReadPtr > DescReadPtr) && (BufReadPtr <= DescWritePtr))
			goto RX_LOST;
		else
		{
			if (BufReadPtr > DescWritePtr)
				Updata = BufReadPtr - DescWritePtr - 1;
			else
				Updata = MAC_RX_DESC_NUM + BufReadPtr - DescWritePtr -1;
		}			
	}	
	else if (DescWritePtr < DescReadPtr)
	{
		if ((BufReadPtr > DescReadPtr) ||(BufReadPtr <=DescWritePtr))
			goto RX_LOST;	
		else
			Updata = BufReadPtr - DescWritePtr -1;
	}
	else
	{
		if (DescWritePtr < BufReadPtr)
			Updata = BufReadPtr - DescWritePtr -1;
		else if (DescWritePtr > BufReadPtr)
			Updata = MAC_RX_DESC_NUM + BufReadPtr - DescWritePtr -1;
		else
			goto RX_LOST;	
	}

	if (Updata > 0)
	{
		i = DescWritePtr;
		while (Updata > 0)
		{
			RxDescD0 = pAdapter->RxDescBuf[i];
			RxDescD1 = pAdapter->RxDescBuf[i] + 4;
			RxDescD2 = pAdapter->RxDescBuf[i] + 8;
			RxDescD3 = pAdapter->RxDescBuf[i] + 12;
			/*
			   WriteMemD(RxDescD0,0);
			   WriteMemD(RxDescD1,0);
			   WriteMemD(RxDescD3, pAdapter->RxDescBuf[i]);
			   */
			*((u32*)RxDescD0) = 0;
			*((u32*)RxDescD1) = 0;
			*((u32*)RxDescD3) = pAdapter->RxDescBuf[i];

			if (i == MAC_RX_DESC_NUM - 1)
			{
				//WriteMemD(RxDescD2,RX_DESC_EOR);
				//#define RX_DESC_EOR	0x80000000
				*((u32*)RxDescD2) = 0x80000000;//RX_DESC_EOR;
				i = 0;
			}
			else
			{
				//WriteMemD(RxDescD2,0);
				*((u32*)RxDescD2) = 0;
				i ++;
			}		
			Updata --;
		}

		pAdapter->RxDescWPtr = i;
		TOE2_W16(pAdapter->BaseAddr, RxRingDesWPtr, pAdapter->RxDescWPtr);
	}

	//TOE2_UTILITY_TRACE("DescReadPtr:%d, BufReadPtr:%d, DescWritePtr:%d.\n", DescReadPtr, BufReadPtr, DescWritePtr);
	return DescReadPtr;

RX_LOST:
	//AfxMessageBox("BufReadPtr Got Lost");
	//while(1);//asm("sdbbp");
	TOE2_UTILITY_TRACE("DescReadPtr:%d, BufReadPtr:%d, DescWritePtr:%d.\n", 
			DescReadPtr, BufReadPtr, DescWritePtr);
	return DescReadPtr;
}

static int ReceivePackets(PMAC_ADAPTER pAdapter)
{
	u16 BufReadPtr, DescReadPtr;
	u16 HdrChs, PayChs, VlanTag;
	u16 Pkts;
	u32 RxDescD0, RxDescD1, RxDescD2, RxDescD3;
	u32 Sts, BufAddr; 
	u32 Crc,Crc_hw;
	struct packet_head *pHead;
	u16 PacketSize;
	u32 i;
	u16 *Temp;
	u8 *pPkt = NULL;
	int Result = TRUE;

	DescReadPtr = ReceiveUpdateDescPtr(pAdapter);
	//DescReadPtr = MAC_R16(RxRingDesRPtr);
	BufReadPtr = pAdapter->RxBufRPtr;

	if (DescReadPtr >= BufReadPtr)
		Pkts = DescReadPtr - BufReadPtr;
	else
		Pkts = MAC_RX_DESC_NUM + DescReadPtr - BufReadPtr;
	printk("%s\n pkts %d", __FUNCTION__, Pkts);
	if (Pkts == 0)
		return FALSE;
	if (DescReadPtr == pAdapter->RxDescWPtr)
		Pkts --;

	i = BufReadPtr;
	while(Pkts > 0)
	{
		RxDescD0 = pAdapter->RxDescBuf[i];
		RxDescD1 = pAdapter->RxDescBuf[i] + 4;
		RxDescD2 = pAdapter->RxDescBuf[i] + 8;
		RxDescD3 = pAdapter->RxDescBuf[i] + 12;
		/*
		   Sts = ReadMemD(RxDescD0);
		   PayChs = ReadMemW(RxDescD1);
		   HdrChs = ReadMemW(RxDescD1+2);
		   VlanTag = ReadMemW(RxDescD2);
		   BufAddr = ReadMemD(RxDescD3);
		   */
		Sts = *((u32*)RxDescD0);
		Temp = (u16*)RxDescD1;
		PayChs = *(Temp);
		Temp = (u16*)(RxDescD1+2);
		HdrChs = *(Temp);
		Temp = (u16*)RxDescD2;
		VlanTag = *(Temp);
		BufAddr = *((u32*)RxDescD3);
		BufAddr |= 0xa0000000;

		pHead = (struct packet_head *)(&Sts);
		if (ReceiveHeadOK(pAdapter, pHead))
		{
			//TOE2_UTILITY_TRACE("head ok 0x:%08x \n",Sts);
			PacketSize = pHead ->PacketLength;
			ReceiveChksumOk(pAdapter, pHead);

			if (pAdapter->CrcVerify || pAdapter->ChksumVerify)
			{
				//copy one packet with head & crc to host memory.
				ALICopyFromMem(
						pAdapter->RxBuffer + (i * TOE2_BUF_SZ),
						BufAddr,
						PacketSize//pkt & crc.
						);

				pPkt = (u8*)(pAdapter->RxBuffer + (i * TOE2_BUF_SZ));
			}

			/*************************************************
			 *	Checksum Verify
			 *************************************************/
			if (pAdapter->ChksumVerify)
			{
				if (!pAdapter->ToeRxEn)
					return FALSE;

				//ALiPrintBuf(pPkt, PacketSize);	

				Result = ToeIPCheck(pAdapter, pHead, pPkt, HdrChs, PayChs);

				if (Result == FALSE)
				{
					MacRxStop(pAdapter);
					//test use.
					//TmpCMD = MAC_R8(0x5b);  
					//MAC_W8(0x5b, TmpCMD|0x80);  
					//AfxMessageBox("ReceivePackets: Chksum error!!\n");
					while(1);// asm("sdbbp");
				}
			}

			/*************************************************
			 *	CRC Verify
			 *************************************************/
			if (pAdapter->CrcVerify)
			{
				if (pHead->VLAN && pAdapter->VlanTagRemove)
				{
					int v_off = 0;
					memcpy(ali_vlan_chk, pPkt, 12); v_off += 12;
					*(u16 *)(ali_vlan_chk + v_off) = SWAP(ETH_P_8021Q); v_off += 2;
					*(u16 *)(ali_vlan_chk + v_off) = SWAP(VlanTag); v_off += 2;

					memcpy((ali_vlan_chk + v_off), (pPkt + 12), PacketSize -12); 

					PacketSize += 4;
					pPkt = ali_vlan_chk;
				}

				Crc = ALiCrc32(pPkt, (PacketSize - 4));
				//TOE2_UTILITY_TRACE("Len(%x)->CRC(0x%08x)\n", PacketSize - i, CRC);
				//if (Crc != *(u32 *)(pPkt +  PacketSize - 4))
				Crc_hw = (pPkt+PacketSize-4)[0]|((pPkt+PacketSize-4)[1]<<8)|((pPkt+PacketSize-4)[2]<<16)|((pPkt+PacketSize-4)[3]<<24);
				if (Crc != Crc_hw)
				{
					//MacRxStop(pAdapter);
					TOE2_UTILITY_TRACE("CRC error: Crc(0x%08x)!=(0x%08x)\n", Crc, Crc_hw);
					//test use.
					//TmpCMD = MAC_R8(0x5b);  
					//MAC_W8(0x5b, TmpCMD|0x80);  
					//AfxMessageBox("ReceivePackets: CRC compare error!!\n");
					while(1);//asm("sdbbp");
				}
			}
		}
		else
		{
			//TOE2_UTILITY_TRACE("head err 0x%08x\n",Sts);
			PacketSize = pHead ->PacketLength;
			//copy one packet with head & crc to host memory.
			ALICopyFromMem(
					pAdapter->RxBuffer + (i * TOE2_BUF_SZ),
					BufAddr,
					PacketSize//pkt & crc.
					);
			pPkt = (u8*)(pAdapter->RxBuffer + (i * TOE2_BUF_SZ));

			//test use.
			//TmpCMD = MAC_R8(0x54);  
			//MAC_W8(0x54, TmpCMD|0x01);  
			//AfxMessageBox("ReceivePackets: Header error!!\n");

			if (i == MAC_RX_DESC_NUM - 1)
				i = 0;
			else
				i ++;
			break;
		}

		if (i == MAC_RX_DESC_NUM - 1)
			i = 0;
		else
			i ++;

		Pkts --;
	}

	pAdapter->RxBufRPtr = i;
	return TRUE;
}

static void MacRxInterrupt(PMAC_ADAPTER pAdapter)
{
	u8 TmpCMD;
	TOE2_UTILITY_TRACE("MacRxInterrupt. \n");
	do
	{
		// read Command Register.
		TmpCMD = TOE2_R8(pAdapter->BaseAddr, SCR);
		// if Rx ring buffer empty, break out.
		if ((TmpCMD & (u8)SCRBufEmpty))
		{
			pAdapter->mac_stats.rx_buf_empty ++;
			break;
		}

		if (ReceivePackets(pAdapter) != 0)
			break;
	} while(1);
}

void MacIsr(PMAC_ADAPTER pAdapter)
{
	u32 CurrentISR = TOE2_R32(pAdapter->BaseAddr, ISR);

	TOE2_W32(pAdapter->BaseAddr, ISR, 0);

	pr_info("%s ISR 0x%x InterruptMask 0x%x\n", __func__, (unsigned int)CurrentISR, (unsigned int)pAdapter->InterruptMask);
	if (CurrentISR & pAdapter->InterruptMask)
	{
		pAdapter->CurrentISR = (CurrentISR & pAdapter->InterruptMask);

		//Rx interrupt.
		if (pAdapter->CurrentISR & (ISRRxComplete|ISRRxFifoOverflow |ISRRxBufOverflow))
		{
			TOE2_UTILITY_TRACE("rx ISR=%x. \n", CurrentISR);
			MacRxInterrupt(pAdapter);
		}

		if (pAdapter->CurrentISR & (ISRLinkStatus |ISRTimer))
			MacWeirdInterrupt(pAdapter);

		if (pAdapter->CurrentISR & (ISRRxHdrErr|ISRTxCoeErr|ISRWatchdog))
			TOE2_UTILITY_TRACE("(ISRRxHdrErr|ISRTxCoeErr|ISRWatchdog)=>%x.", CurrentISR);
	}	
}

irqreturn_t util_toe2_isr(int Irq, void *dev_id) {
	PMAC_ADAPTER pAdapter;
	pAdapter = &util_mac_adapter;
	MacIsr(pAdapter);	
	return IRQ_HANDLED;
}

static void MacRxStart(PMAC_ADAPTER pAdapter)
{
	u8 TmpChar;
	u32 Tmp32;

	pAdapter->net_stats.rx_packets = 0;
	pAdapter->net_stats.rx_bytes = 0;
	pAdapter->net_stats.rx_errors = 0;
	pAdapter->net_stats.rx_dropped = 0;
	pAdapter->net_stats.rx_length_errors = 0;
	pAdapter->net_stats.rx_over_errors = 0;
	pAdapter->net_stats.rx_crc_errors = 0;
	pAdapter->net_stats.rx_frame_errors = 0;
	pAdapter->net_stats.rx_fifo_errors = 0;
	pAdapter->net_stats.rx_missed_errors = 0;

	pAdapter->mac_stats.rx_mc = 0;
	pAdapter->mac_stats.rx_bc = 0;
	pAdapter->mac_stats.rx_uc = 0;
	pAdapter->mac_stats.rx_vlan = 0;
	pAdapter->mac_stats.rx_pppoe = 0;	
	pAdapter->mac_stats.rx_ip = 0;
	pAdapter->mac_stats.rx_runt_errors = 0;
	pAdapter->mac_stats.rx_long_errors = 0;
	pAdapter->mac_stats.rx_dribble_errors = 0;
	pAdapter->mac_stats.rx_phy_layer_errors = 0;
	pAdapter->mac_stats.rx_wd_timeout_errors = 0;
	pAdapter->mac_stats.rx_ip_chksum_errors = 0;
	pAdapter->mac_stats.rx_tcp_chksum_errors = 0;
	pAdapter->mac_stats.rx_buf_empty = 0;
	pAdapter->mac_stats.rx_late_col_seen = 0;
	pAdapter->mac_stats.rx_lost_in_ring = 0;
	pAdapter->mac_stats.rx_hdr_chs_errs = 0;
	pAdapter->mac_stats.rx_pay_chs_errs = 0;

	memset((void*)pAdapter->RxBuffer, 0x0, (MAC_RX_DESC_NUM * TOE2_BUF_SZ));

	Tmp32 = TOE2_R32(pAdapter->BaseAddr, NetworkOM);
	Tmp32 &= ~(FilteringMask|PassMask|WorkModeMask);

	if (pAdapter->LoopBackEn)
		Tmp32 |= WorkModeLoopBack;
	else
		Tmp32 |= WorkModeNormal;

	if (pAdapter->PassMulticast)
		Tmp32 |= PassAllMulticast;
	if (pAdapter->Promiscuous)
		Tmp32 |= PassPromiscuous;
	if (pAdapter->PassBad)
		Tmp32 |= PassErr;

	Tmp32 |= PassAllMulticast;
	Tmp32 |= PassPromiscuous;
	Tmp32 |= PassErr;

	switch (pAdapter->FilteringMode)
	{
		case 0:
			Tmp32 |= HashFiltering;
			break;
		case 1:
			Tmp32 |= HashOnlyFiltering;
			break;
		case 2:
			Tmp32 |= InverseFiltering;
			break;
		case 3:
			Tmp32 |= PerfectFiltering;
			break;
		default:
			Tmp32 |= PerfectFiltering;
			break;
	}

	TOE2_W32(pAdapter->BaseAddr, NetworkOM, Tmp32);

	TmpChar = TOE2_R8(pAdapter->BaseAddr, SCR);

	if (pAdapter->ToeRxEn)
		TOE2_W8(pAdapter->BaseAddr, SCR, (TmpChar|(u8)SCRRxEn|(u8)SCRRxCoeEn));
	else
		TOE2_W8(pAdapter->BaseAddr, SCR, (TmpChar|(u8)SCRRxEn));
}

int util_mac_rx_start(PMAC_Rx_Context pRxContext)
{
	int i = 0;
	struct toe2_private *ptoe2 = g_ptoe2;
	PMAC_ADAPTER pAdapter = NULL;
	pAdapter = &util_mac_adapter;
	util_mac_adapter.LoopBackEn = pRxContext->LoopBackEn;
	util_mac_adapter.PassMulticast = pRxContext->PassMulticast;
	util_mac_adapter.Promiscuous = pRxContext->Promiscuous;
	util_mac_adapter.PassBad = pRxContext->PassBad;
	util_mac_adapter.CrcVerify= pRxContext->CrcVerify;
	util_mac_adapter.ChksumVerify = pRxContext->ChksumVerify;	
	util_mac_adapter.FilteringMode = pRxContext->FilteringMode;

#ifdef UTIL_DEBUG
	util_mac_adapter.FilteringMode = MAC_FILTER_PERFECT;
#endif

	TOE2_UTILITY_TRACE("[T] rx LoopBackEn:%d", util_mac_adapter.LoopBackEn);
	TOE2_UTILITY_TRACE("[T] rx PassMulticast:%d", util_mac_adapter.PassMulticast);
	TOE2_UTILITY_TRACE("[T] rx Promiscuous:%d", util_mac_adapter.Promiscuous);
	TOE2_UTILITY_TRACE("[T] rx PassBad:%d", util_mac_adapter.PassBad);
	TOE2_UTILITY_TRACE("[T] rx CrcVerify:%d", util_mac_adapter.CrcVerify);
	TOE2_UTILITY_TRACE("[T] rx ChksumVerify:%d", util_mac_adapter.ChksumVerify);
	TOE2_UTILITY_TRACE("[T] rx FilteringMode:%d", util_mac_adapter.FilteringMode);

	m_bRxStopThread = 0;

	TOE2_UTILITY_TRACE("util_mac_rx_start start...");
	MacRxStart(&util_mac_adapter);

	if (ptoe2->acquired_isr) {
		free_irq(ptoe2->irq_num, (void *)ptoe2->ndev);
		ptoe2->acquired_isr = false;
	}

	i = request_irq(ptoe2->irq_num,
			util_toe2_isr, 0, ptoe2->ndev->name, (void *)ptoe2->ndev);
	if (i != 0) {
		pr_err("Unable to use IRQ = %d, errno = %d.\n", ptoe2->irq_num, i);
		return -1;
	}
	ptoe2->acquired_isr = true;

	toe2_enable_irq(ptoe2);
	return 0;
}

void test_mac_rx(struct toe2_private *ptoe2, struct ali_mac_rx_io *rx_info) {
	MAC_Rx_Context u_rx_cxt;
	memset(&u_rx_cxt, 0, sizeof(u_rx_cxt));
	u_rx_cxt.LoopBackEn = rx_info->loopback_en;
	u_rx_cxt.PassMulticast = rx_info->pass_multicast;
	u_rx_cxt.PassBad = rx_info->pass_bad;
	u_rx_cxt.Promiscuous = rx_info->promiscuous;
	u_rx_cxt.CrcVerify = rx_info->crc_verify;
	u_rx_cxt.ChksumVerify = rx_info->chksum_verify;
	u_rx_cxt.FilteringMode = rx_info->filter_mode;
	util_mac_rx_start(&u_rx_cxt);
}
void util_mac_rx_stop(PMAC_ADAPTER pAdapter)
{
	MacRxStop(pAdapter);
	m_bRxStopThread = 1;
}

static void MacHeaderBuild(PMAC_ADAPTER pAdapter, u8 *pBuf, u16 len)
{
	unsigned char *pHdr = pBuf;
	u16 tmp_u16;
	u16 IpHdrLen = 5;
	u32 acc = 0;
	u32 ip_src, ip_des; 
	u16 ip_protocol;
	u16 ChkLen;
	u32 L3HeaderLen;

	u8 IpStartOff = 12; 

	ChsOff = 0;

	memcpy(pHdr, &MacDstAddr[0], 6);
	pHdr += 6; // 6 now.
	memcpy(pHdr, &MacSrcAddr[0], 6);
	pHdr += 6; // 12 now.
	IpStartOff = 12; 

	memset(&ali_desc_hw, 0, sizeof(ali_desc_hw));

	if (ali_tx_para.AddVlanTag)
	{
#if (1)		
		ali_desc_hw.tx_sts.hw.VlanEn = 1;
		ali_desc_hw.vlan_tag = ali_tx_para.TxVlanTag;
		printk("vlan_tag %d\n", ali_desc_hw.vlan_tag);
#else		
		*(u16 *)pHdr = SWAP(ETH_P_8021Q); pHdr += 2; //type: VLAN tag
		*(u16 *)pHdr = SWAP(ali_tx_para.TxVlanTag); pHdr += 2; //tag 
		IpStartOff += 4; 
#endif
	}

	switch (ali_tx_para.FrameType)
	{
		case MAC_FRAME_8023:
			IpStartOff = 0;
			break;

		case MAC_FRAME_ETHERII:
			*(u16 *)pHdr = SWAP(ETH_P_IP); // IP
			//*(u16 *)pHdr = 0x0608; // ARP
			//*(u16 *)pHdr = 0x3580; // RARP
			IpStartOff = 0;
			break;

		case MAC_FRAME_IP_ICMP:
			*(u16 *)pHdr = SWAP(ETH_P_IP); 
			pHdr += 2; // 14 now.

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength.
			*(u16 *)(pHdr+2) = SWAP((len - ETH_HLEN)); //Tot Len.
			*(u16 *)(pHdr+6) = 0x0000; //Offset
			*(u16 *)(pHdr+8) = 0x0180; //Ttl(128) & Protocol(ICMP:0x01)
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum
			IpStartOff += 2; // 14 now.

			pHdr += (IpHdrLen*4); 
			*(u16 *)(pHdr+2) = 0x0000; //Clear ICMP checksum.

			if (pAdapter->ToeTxEn)
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
			break;

		case MAC_FRAME_IP_IGMP:
			*(u16 *)pHdr = SWAP(ETH_P_IP); 
			pHdr += 2; // 14 now.

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength.
			*(u16 *)(pHdr+2) = SWAP((len - ETH_HLEN)); //Tot Len.	
			*(u16 *)(pHdr+6) = 0x0000; //Offset
			*(u16 *)(pHdr+8) = 0x0280; //Ttl(128) & Protocol(IGMP:0x02)
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum
			IpStartOff += 2;  // 14 now.

			pHdr += (IpHdrLen*4); 
			*(u16 *)(pHdr+2) = 0x0000; //Clear IGMP checksum.
			if (pAdapter->ToeTxEn)
				ali_desc_hw.tx_sts.hw.CoeEn = 1;

			break;

		case MAC_FRAME_IP_UDP:
			*(u16 *)pHdr = SWAP(ETH_P_IP); 
			pHdr += 2; // 14 now.

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength.
			*(u16 *)(pHdr+2) = SWAP((len - ETH_HLEN)); //Tot Len.
			*(u16 *)(pHdr+6) = 0x0000; //Offset.
			ip_protocol = 0x1180;
			*(u16 *)(pHdr+8) = ip_protocol; //Ttl(128) & Protocol(UDP:0x11).
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.
			IpStartOff += 2;  // 14 now.	

			acc = standard_chksum_2(pHdr, (IpHdrLen*4));
			acc = htons((u16)~acc);
			*(u16 *)(pHdr+10) = acc;
			acc = 0;

			L3HeaderLen = 20;
			*(u16 *)(pHdr+ (IpHdrLen*4) +4)=SWAP((len - ETH_HLEN - L3HeaderLen)); 
			*(u16 *)(pHdr + (IpHdrLen*4) +6) = 0x0000; //Clear UDP checksum
			if (pAdapter->ToeTxEn)
			{
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
				ali_desc_hw.tx_sts.hw.UdpPkt = 1;

				//ip_src = *(u32 *)(pHdr + IP_IPSRC_OFF);
				//ip_des = *(u32*)(pHdr + IP_IPDES_OFF);			

				ip_src = (pHdr + IP_IPSRC_OFF)[0]|((pHdr + IP_IPSRC_OFF)[1]<<8)|((pHdr + IP_IPSRC_OFF)[2]<<16)|((pHdr + IP_IPSRC_OFF)[3]<<24);
				ip_des = (pHdr + IP_IPDES_OFF)[0]|((pHdr + IP_IPDES_OFF)[1]<<8)|((pHdr + IP_IPDES_OFF)[2]<<16)|((pHdr + IP_IPDES_OFF)[3]<<24);

				pHdr += (IpHdrLen*4);
				ChkLen = len - ETH_HLEN - (IpHdrLen*4);

				acc = standard_chksum(pHdr, ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffff) + (acc >> 16);
				}

				acc += (ip_src & 0xFFFFUL);
				acc += ((ip_src>>16) & 0xFFFFUL);

				acc += (ip_des & 0xFFFFUL);
				acc += ((ip_des>>16) & 0xFFFFUL);

				acc += (ip_protocol & 0xFF00);
				acc += (u32)SWAP(ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffffUL) + (acc >> 16);
				}

				ChsAcc =(u16)~acc;
				ChsOff = ETH_HLEN + (IpHdrLen*4) + 6;
			}
			break;

		case MAC_FRAME_IP_TCP:
			*(u16 *)pHdr = SWAP(ETH_P_IP); 
			pHdr += 2; // 14 now.	

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength.
			*(u16 *)(pHdr+2) = SWAP((len - ETH_HLEN)); //Tot Len.
			*(u16 *)(pHdr+6) = 0x0000; //Offset.
			ip_protocol = 0x0680;
			*(u16 *)(pHdr+8) = ip_protocol; //Ttl(128) & Protocol(TCP:0x06).
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.
			IpStartOff += 2;  // 14 now.

			acc = standard_chksum_2(pHdr, (IpHdrLen*4));
			acc = htons((u16)~acc);
			*(u16 *)(pHdr+10) = acc;
			acc = 0;

			*(u16 *)(pHdr+(IpHdrLen*4)+12) = 0x50;//make tcp header len is 5(5*4=20)

			*(u16 *)(pHdr+(IpHdrLen*4)+16) = 0x0000; //Clear TCP checksum
			if (pAdapter->ToeTxEn)
			{
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
				ali_desc_hw.tx_sts.hw.TcpPkt = 1;

				//ip_src = *(u32 *)(pHdr + IP_IPSRC_OFF);
				//ip_des = *(u32*)(pHdr + IP_IPDES_OFF);	

				ip_src = (pHdr + IP_IPSRC_OFF)[0]|((pHdr + IP_IPSRC_OFF)[1]<<8)|((pHdr + IP_IPSRC_OFF)[2]<<16)|((pHdr + IP_IPSRC_OFF)[3]<<24);
				ip_des = (pHdr + IP_IPDES_OFF)[0]|((pHdr + IP_IPDES_OFF)[1]<<8)|((pHdr + IP_IPDES_OFF)[2]<<16)|((pHdr + IP_IPDES_OFF)[3]<<24);

				pHdr += (IpHdrLen*4);

				ChkLen = len - ETH_HLEN - (IpHdrLen*4);
				acc = standard_chksum(pHdr, ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffff) + (acc >> 16);
				}

				acc += (ip_src & 0xFFFFUL);
				acc += ((ip_src>>16) & 0xFFFFUL);

				acc += (ip_des & 0xFFFFUL);
				acc += ((ip_des>>16) & 0xFFFFUL);

				acc += (ip_protocol & 0xFF00);
				acc += SWAP(ChkLen);;

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffffUL) + (acc >> 16);
				}

				ChsAcc =(u16)~acc;
				ChsOff = ETH_HLEN + (IpHdrLen*4) + 16;
			}
			break;

		case MAC_FRAME_PPPoE_IP_ICMP:
			*(u16 *)pHdr = SWAP(ETH_P_PPP_SES); 
			pHdr += 2; // 14 now.

			*(u16 *)(pHdr+0) = 0x0011; //PPPoE version & type.
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN + 2;
			*(u16 *)(pHdr+4) = SWAP(tmp_u16); 
			*(u16 *)(pHdr+6) = SWAP(PPPOE_IP);
			pHdr += 8; // 22 now.
			IpStartOff += 10; // 22 now.

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN;
			*(u16 *)(pHdr+2) = SWAP(tmp_u16); //Tot Len.	
			*(u16 *)(pHdr+6) = 0x0000; //Offset.
			*(u16 *)(pHdr+8) = 0x0180; //Ttl(128) & Protocol(ICMP:0x01).
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.

			pHdr += (IpHdrLen*4);
			*(u16 *)(pHdr+2) = 0x0000; //Clear ICMP/IGMP checksum
			if (pAdapter->ToeTxEn)
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
			break;	

		case MAC_FRAME_PPPoE_IP_IGMP:
			*(u16 *)pHdr = SWAP(ETH_P_PPP_SES); 
			pHdr += 2; // 14 now.	

			*(u16 *)(pHdr+0) = 0x0011; //PPPoE version & type.
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN + 2;
			*(u16 *)(pHdr+4) = SWAP(tmp_u16); 		
			*(u16 *)(pHdr+6) = SWAP(PPPOE_IP);
			pHdr += 8; // 22 now.	
			IpStartOff += 10;

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN;
			*(u16 *)(pHdr+2) = SWAP(tmp_u16); //Tot Len.	
			*(u16 *)(pHdr+6) = 0x0000; //Offset.
			*(u16 *)(pHdr+8) = 0x0280; //Ttl(128) & Protocol(IGMP:0x02).
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.

			pHdr += (IpHdrLen*4);
			*(u16 *)(pHdr+2) = 0x0000; //Clear ICMP/IGMP checksum		
			if (pAdapter->ToeTxEn)
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
			break;

		case MAC_FRAME_PPPoE_IP_UDP:
			*(u16 *)pHdr = SWAP(ETH_P_PPP_SES); 
			pHdr += 2; // 14 now.

			*(u16 *)(pHdr+0) = 0x0011; //PPPoE version & type.
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN + 2;
			*(u16 *)(pHdr+4) = SWAP(tmp_u16); 
			*(u16 *)(pHdr+6) = SWAP(PPPOE_IP);
			pHdr += 8; // 22 now.	
			IpStartOff += 10;

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN;
			*(u16 *)(pHdr+2) = SWAP(tmp_u16); //Tot Len.	
			*(u16 *)(pHdr+6) = 0x0000; //Offset.
			ip_protocol = 0x1180;
			*(u16 *)(pHdr+8) = ip_protocol; //Ttl(128) & Protocol(UDP:0x11).
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.

			*(u16 *)(pHdr+(IpHdrLen*4)+6) = 0x0000; //Clear UDP checksum		 
			if (pAdapter->ToeTxEn)
			{
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
				ali_desc_hw.tx_sts.hw.UdpPkt = 1;

				//ip_src = *(u32 *)(pHdr + IP_IPSRC_OFF);
				//ip_des = *(u32*)(pHdr + IP_IPDES_OFF);	

				ip_src = (pHdr + IP_IPSRC_OFF)[0]|((pHdr + IP_IPSRC_OFF)[1]<<8)|((pHdr + IP_IPSRC_OFF)[2]<<16)|((pHdr + IP_IPSRC_OFF)[3]<<24);
				ip_des = (pHdr + IP_IPDES_OFF)[0]|((pHdr + IP_IPDES_OFF)[1]<<8)|((pHdr + IP_IPDES_OFF)[2]<<16)|((pHdr + IP_IPDES_OFF)[3]<<24);

				pHdr += (IpHdrLen*4);
				ChkLen = len - ETH_HLEN - PPPOE_HDR_LEN - (IpHdrLen*4);
				acc = standard_chksum(pHdr, ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffff) + (acc >> 16);
				}

				acc += (ip_src & 0xFFFFUL);
				acc += ((ip_src>>16) & 0xFFFFUL);

				acc += (ip_des & 0xFFFFUL);
				acc += ((ip_des>>16) & 0xFFFFUL);

				acc += (ip_protocol & 0xFF00);
				acc += SWAP(ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffffUL) + (acc >> 16);
				}

				ChsAcc =(u16)~acc;
				ChsOff = ETH_HLEN + PPPOE_HDR_LEN + (IpHdrLen*4) + 6;
			}
			break;

		case MAC_FRAME_PPPoE_IP_TCP:
			*(u16 *)pHdr = SWAP(ETH_P_PPP_SES); 
			pHdr += 2; // 14 now.

			*(u16 *)(pHdr+0) = 0x0011; //PPPoE version & type.
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN + 2;
			*(u16 *)(pHdr+4) = SWAP(tmp_u16); 
			*(u16 *)(pHdr+6) = SWAP(PPPOE_IP);
			pHdr += 8; // 22 now.	
			IpStartOff += 10;

			*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength
			tmp_u16  = len - ETH_HLEN - PPPOE_HDR_LEN;
			*(u16 *)(pHdr+2) = SWAP(tmp_u16); //Tot Len.	
			*(u16 *)(pHdr+6) = 0x0000; //Offset.
			ip_protocol = 0x0680; 
			*(u16 *)(pHdr+8) = ip_protocol; //Ttl(128) & Protocol(TCP:0x06).
			*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.

			*(u16 *)(pHdr+(IpHdrLen*4)+16) = 0x0000; //Clear TCP checksum
			if (pAdapter->ToeTxEn)
			{
				ali_desc_hw.tx_sts.hw.CoeEn = 1;
				ali_desc_hw.tx_sts.hw.TcpPkt = 1;

				//ip_src = *(u32 *)(pHdr + IP_IPSRC_OFF);
				//ip_des = *(u32*)(pHdr + IP_IPDES_OFF);	

				ip_src = (pHdr + IP_IPSRC_OFF)[0]|((pHdr + IP_IPSRC_OFF)[1]<<8)|((pHdr + IP_IPSRC_OFF)[2]<<16)|((pHdr + IP_IPSRC_OFF)[3]<<24);
				ip_des = (pHdr + IP_IPDES_OFF)[0]|((pHdr + IP_IPDES_OFF)[1]<<8)|((pHdr + IP_IPDES_OFF)[2]<<16)|((pHdr + IP_IPDES_OFF)[3]<<24);

				pHdr += (IpHdrLen*4);
				ChkLen = len - ETH_HLEN - PPPOE_HDR_LEN - (IpHdrLen*4);
				acc = standard_chksum(pHdr, ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffff) + (acc >> 16);
				}

				acc += (ip_src & 0xFFFFUL);
				acc += ((ip_src>>16) & 0xFFFFUL);

				acc += (ip_des & 0xFFFFUL);
				acc += ((ip_des>>16) & 0xFFFFUL);

				acc += (ip_protocol & 0xFF00);
				acc += SWAP(ChkLen);

				while ((acc >> 16) != 0)
				{
					acc = (acc & 0xffffUL) + (acc >> 16);
				}

				ChsAcc =(u16)~acc;
				ChsOff = ETH_HLEN + PPPOE_HDR_LEN + (IpHdrLen*4) + 16;	
			}
			break;

		default:
			TOE2_UTILITY_TRACE("FrameType(%d) Unknow.\n", ali_tx_para.FrameType);
			break;	
	}	

	if (pAdapter->ToeTxEn)
		ali_desc_hw.tx_sts.hw.IpStartOff = IpStartOff;
}

static void MacTxStatus(PMAC_ADAPTER pAdapter, struct toe2_tx_desc *desc_sw)
{
	if ((desc_sw->tx_sts.sw.FS) && !(desc_sw->tx_sts.sw.OWN))
	{
		if (!(desc_sw->tx_sts.sw.ES))
		{
			pAdapter->net_stats.tx_packets++;
		}
		else
		{ 
			pAdapter->net_stats.tx_errors++;

			if ((desc_sw->tx_sts.sw.LossOfCarrier) ||(desc_sw->tx_sts.sw.NoCarrier))
				pAdapter->net_stats.tx_carrier_errors++;
			if (desc_sw->tx_sts.sw.LateCol) 
				pAdapter->net_stats.tx_window_errors++;
			if (desc_sw->tx_sts.sw.FifoUnderrun) 
				pAdapter->net_stats.tx_fifo_errors++;
			if (desc_sw->tx_sts.sw.HF) 
				pAdapter->net_stats.tx_heartbeat_errors++;
		}

		if (desc_sw->tx_sts.sw.ExCol) 
		{
			pAdapter->mac_stats.tx_col_errors++;
		}
		else
		{
			pAdapter->mac_stats.tx_col_cnts[desc_sw->tx_sts.sw.ColCnt]++;
		}
	}
}

static void TransmitConfig(PMAC_ADAPTER pAdapter, u16 WPtr, u16 Off, u32 Size)
{
    volatile char wait_buffer[TOE2_DESC_SZ] = {0};
	volatile void * tmp;
	struct toe2_tx_desc *DescSw = &pAdapter->pTxDesc[WPtr];

	MacTxStatus(pAdapter, DescSw);

	ali_desc_hw.seg_len = Size;

	//printk("TransmitConfig: Off/Size = [%d,%d]\n", Off, Size);

	if (Off == 0)
	{
		ali_desc_hw.tx_sts.hw.OWN = 1;
		
		if (pAdapter->TsoEn || pAdapter->UfoEn)
		{
		    TOE2_UTILITY_TRACE("TransmitConfig: Tso/Ufo enabled");
			ali_desc_hw.tx_sts.hw.CoeEn = 1;
			ali_desc_hw.tx_sts.hw.Mfl = pAdapter->MflAuto;
			if ((ali_tx_para.FrameType == MAC_FRAME_IP_UDP) ||
				(ali_tx_para.FrameType == MAC_FRAME_PPPoE_IP_UDP))
			{
				ali_desc_hw.tx_sts.hw.UdpPkt = 1;
			}
			else if ((ali_tx_para.FrameType == MAC_FRAME_IP_TCP) ||
				(ali_tx_para.FrameType == MAC_FRAME_PPPoE_IP_TCP))
			{
				ali_desc_hw.tx_sts.hw.TcpPkt = 1;	
			}
			else
			{
				TOE2_UTILITY_TRACE("TransmitConfig: Tso/Ufo Pkt Type err");
				while(1);//asm("sdbbp");
			}
		}
		else if (pAdapter->ToeTxEn)
		{
		    TOE2_UTILITY_TRACE("TransmitConfig: ToeTxEn enabled");
			ali_desc_hw.tx_sts.hw.CoeEn = 1;
			ali_desc_hw.tx_sts.hw.Mfl = TOE2_MSS;//TOE2_BUF_SZ;
			if ((ali_tx_para.FrameType == MAC_FRAME_IP_UDP) ||
				(ali_tx_para.FrameType == MAC_FRAME_PPPoE_IP_UDP))
				ali_desc_hw.tx_sts.hw.UdpPkt = 1;
			else if ((ali_tx_para.FrameType == MAC_FRAME_IP_TCP) ||
				(ali_tx_para.FrameType == MAC_FRAME_PPPoE_IP_TCP))
				ali_desc_hw.tx_sts.hw.TcpPkt = 1;	
			else
			{
				TOE2_UTILITY_TRACE("TransmitConfig: Tx Coe Pkt Type err");
				while(1);//asm("sdbbp");
			}
		}
		else
		{
		    TOE2_UTILITY_TRACE("TransmitConfig: NO ToeTxEn");
			//ali_desc_hw.tx_sts.hw.Mfl = TOE2_BUF_SZ - 18;
		}
		//gmac_print(pAdapter->TxDescBuf[WPtr], 48);
		//printk("\n");
	}

	if (WPtr == MAC_TX_DESC_NUM - 1)
		ali_desc_hw.tx_sts.hw.EOR = 1;
	else
		ali_desc_hw.tx_sts.hw.EOR = 0;

	ali_desc_hw.pkt_buf_dma = dma_map_single((struct device *)NULL, (void*)(pAdapter->TxDescBuf + Off), Size, DMA_TO_DEVICE);

	memcpy(&pAdapter->pTxDesc[WPtr], (unsigned char *)(&ali_desc_hw), sizeof(ali_desc_hw));

	TOE2_UTILITY_TRACE("WPtr %d desc_hw ali_desc_hw.tx_sts.hw.VlanEn %d\n",WPtr, ali_desc_hw.tx_sts.hw.VlanEn);
	TOE2_UTILITY_TRACE("desc_hw ali_desc_hw.vlan_tag %d\n", ali_desc_hw.vlan_tag);


	memcpy((void *)wait_buffer, (void *)&pAdapter->pTxDesc[WPtr], TOE2_DESC_SZ);//HW dma wait
	tmp = wait_buffer;
}

static int TransmitPackets(PMAC_ADAPTER pAdapter, u16 DescNum)
{
	u16 DescWritePtr, DescReadPtr;
	u16 Available;
	u16 Off, Desc;
	int j = 0;

	do {
		DescReadPtr = TOE2_R16(pAdapter->BaseAddr, TxRingDesRPtr);
		DescWritePtr = pAdapter->TxDescWPtr;

		if (DescReadPtr > DescWritePtr)
			Available = DescReadPtr - DescWritePtr - 1;
		else
			Available = MAC_TX_DESC_NUM + DescReadPtr - DescWritePtr - 1;

	} while(Available <DescNum+1);

	for (Off = 0, Desc = 0; Desc < DescNum; Desc++) {

		ali_desc_hw.tx_sts.hw.OWN = 0;
		ali_desc_hw.tx_sts.hw.FS = 0;
		ali_desc_hw.tx_sts.hw.LS = 0;
		ali_desc_hw.tx_sts.hw.Mfl = 0;
		ali_desc_hw.tx_sts.hw.CoeEn = 0;
		ali_desc_hw.tx_sts.hw.UdpPkt = 0;
		ali_desc_hw.tx_sts.hw.TcpPkt = 0;

		if (Desc == 0) {
			ali_desc_hw.tx_sts.hw.FS = 1;
		}

		if (Desc == (DescNum -1)) {
			ali_desc_hw.tx_sts.hw.LS = 1;
		}

		TransmitConfig(pAdapter, pAdapter->TxDescWPtr, Off, ali_desc_size[Desc]);

		Off += ali_desc_size[Desc];

		if ((++pAdapter->TxDescWPtr) >= MAC_TX_DESC_NUM)
			pAdapter->TxDescWPtr = 0;
	}

	TOE2_W16(pAdapter->BaseAddr, TxRingDesWPtr, pAdapter->TxDescWPtr);
	j = 0;
	do {
		DescReadPtr = TOE2_R16(pAdapter->BaseAddr, TxRingDesRPtr);
		j++;
		if (j > 1000) {
			printk("Oops toe2 read to slow DescReadPtr %d pAdapter->TxDescWPtr %d\n",
				DescReadPtr, pAdapter->TxDescWPtr);

			break;
		}
	} while (DescReadPtr != pAdapter->TxDescWPtr);

	TOE2_UTILITY_TRACE("TxDescWPtr: %d", pAdapter->TxDescWPtr);
	pAdapter->TxCrcErrs = TOE2_R16(pAdapter->BaseAddr, 0x30/*CRCERR*/);
	return TRUE;
}

static void TransmitFrames(PMAC_ADAPTER pAdapter, u32 DescNum, u32 DescLen, u32 DescTimes)
{
	unsigned int bytes, rand, i;
	int times;

	ali_desc_hw.tot_len = DescLen;
	ali_desc_hw.seg_num= DescNum;

	times = DescTimes;

	while (times--)
	{
		bytes = DescLen;
		for (i = 0; i < (DescNum - 1); i++)
		{
			rand = 1 + get_random_int()%(bytes-(DescNum-i));
			ali_desc_size[i] = rand;
			bytes -= rand;
		}

		ali_desc_size[DescNum - 1] = bytes;
		TransmitPackets(pAdapter, DescNum);
	}
}


static void MacTxTest(PMAC_ADAPTER pAdapter) {
	unsigned long DescTimes, DescLen, min_desc_len, MaxDescLen,  DescFrom, DescTo, Desc;
	long j;

	TOE2_UTILITY_TRACE("tx task start:\n");
	DescTimes = ali_tx_para.DescTimes;
	min_desc_len = ali_tx_para.DescLen;
	MaxDescLen = ali_tx_para.MaxDescLen;
	DescFrom = ali_tx_para.DescFrom;
	DescTo = ali_tx_para.DescTo;

	pr_info("MacTxTest DescTimes/DescLen = [%d,%d]", (int)DescTimes, (int)min_desc_len);
	pr_info("MacTxTest DescFrom/DescTo = [%d,%d]", (int)DescFrom, (int)DescTo);
	pr_info("MacTxTest UfoEn/TsoEn = [%d,%d]", (int)pAdapter->UfoEn, (int)pAdapter->TsoEn);

	for (pAdapter->MflAuto = pAdapter->MinMfl; pAdapter->MflAuto <= pAdapter->MaxMfl; pAdapter->MflAuto++)
	{
		for (DescLen = min_desc_len; DescLen <= MaxDescLen; DescLen++) {
			for (j=0; j<MAX_TOE2_PKT_LEN; j++) {
				ToeTxBuf[j] = (unsigned char)j;
			}
			if (DescLen > MAX_TOE2_PKT_LEN) {
				DescLen = MAX_TOE2_PKT_LEN;
			}
			pAdapter->TxDescBuf = ToeTxBuf;
			pAdapter->TxLenAuto = DescLen; 	

			for (Desc = DescFrom; Desc <= DescTo; Desc++) {
				MacHeaderBuild(pAdapter, &ToeTxBuf[0], DescLen);
				if (!pAdapter->UfoEn && !pAdapter->TsoEn) {
					ALiCrcChk(pAdapter, &ToeTxBuf[0], pAdapter->TxLenAuto);
				}
				TransmitFrames(pAdapter, Desc, DescLen, DescTimes);
			}
		}
	}
return;
}

static void MacTxStart(PMAC_ADAPTER pAdapter)
{
	u8 TmpChar;
	u32 i;

	pAdapter->net_stats.tx_packets = 0;
	pAdapter->net_stats.tx_bytes = 0;
	pAdapter->net_stats.tx_errors = 0;
	pAdapter->net_stats.tx_carrier_errors = 0;
	pAdapter->net_stats.tx_fifo_errors = 0;
	pAdapter->net_stats.tx_heartbeat_errors = 0;
	pAdapter->net_stats.tx_window_errors = 0;

	pAdapter->mac_stats.tx_col_errors = 0;

	for (i = 0; i < 16; i ++)	
		pAdapter->mac_stats.tx_col_cnts[i] = 0;

	TmpChar = TOE2_R8(pAdapter->BaseAddr, SCR);
	TmpChar &= ~(SCRTxCoeEn|SCRTsoEn|SCRUfoEn);

	if (pAdapter->ToeTxEn)
		TmpChar |= SCRTxCoeEn;
	if (pAdapter->TsoEn)
		TmpChar |= (SCRTsoEn|SCRTxCoeEn);
	if (pAdapter->UfoEn)
		TmpChar |= (SCRUfoEn|SCRTxCoeEn);	

	TOE2_W8(pAdapter->BaseAddr, SCR, (TmpChar|(u8)SCRTxEn));	
}

int util_mac_tx_start(PMAC_Tx_Context pTxContext)
{
	//ali_tx_para = pTxContext->TxPara;
	memcpy(&ali_tx_para, &(pTxContext->TxPara), sizeof(ali_tx_para));

	//Tx Settings.
	util_mac_adapter.ToeTxEn = pTxContext->ToeTxEn;
	util_mac_adapter.TsoEn = pTxContext->TsoEn;
	util_mac_adapter.UfoEn = pTxContext->UfoEn;
	util_mac_adapter.TxLenAutoInc = pTxContext->TxLenAutoInc;
	util_mac_adapter.MinMfl = pTxContext->MinMfl;
	util_mac_adapter.MaxMfl = pTxContext->MaxMfl;

	memcpy(&MacDstAddr[0], &(pTxContext->MacDstAddr[0]), 6);
	memcpy(&MacSrcAddr[0], &(pTxContext->MacSrcAddr[0]), 6);

	TOE2_UTILITY_TRACE("[T] tx ali_tx_para FrameType: %d", ali_tx_para.FrameType);
	TOE2_UTILITY_TRACE("[T] tx ali_tx_para DescFrom: %d", ali_tx_para.DescFrom);
	TOE2_UTILITY_TRACE("[T] tx ali_tx_para DescTo: %d", ali_tx_para.DescTo);
	TOE2_UTILITY_TRACE("[T] tx ali_tx_para DescLen: %d", ali_tx_para.DescLen);
	TOE2_UTILITY_TRACE("[T] tx ali_tx_para DescTimes: %d", ali_tx_para.DescTimes);
	TOE2_UTILITY_TRACE("[T] tx ali_tx_para AddVlanTag: %d", ali_tx_para.AddVlanTag);
	TOE2_UTILITY_TRACE("[T] tx ali_tx_para TxVlanTag: %d", ali_tx_para.TxVlanTag);
	TOE2_UTILITY_TRACE("[T] TxContext ToeTxEn: %d", util_mac_adapter.ToeTxEn);
	TOE2_UTILITY_TRACE("[T] TxContext TsoEn: %d", util_mac_adapter.TsoEn);
	TOE2_UTILITY_TRACE("[T] TxContext UfoEn: %d", util_mac_adapter.UfoEn);
	TOE2_UTILITY_TRACE("[T] TxContext TxLenAutoInc: %d", util_mac_adapter.TxLenAutoInc);
	TOE2_UTILITY_TRACE("[T] TxContext MinMfl: %d", util_mac_adapter.MinMfl);
	TOE2_UTILITY_TRACE("[T] TxContext MaxMfl: %d", util_mac_adapter.MaxMfl);
	
	TOE2_UTILITY_TRACE("util_mac_tx_start start...");
	MacTxStart(&util_mac_adapter);
	MacTxTest(&util_mac_adapter);
	return 0;
}

int test_mac_tx(struct toe2_private *ptoe2, struct ali_mac_xmit_io *xmit_info) {
	MAC_Tx_Context tx_ctx; 
	memset(&tx_ctx, 0, sizeof(tx_ctx));
	tx_ctx.ToeTxEn = xmit_info->toe_tx; 
	if (xmit_info->tso_ufo == 1) {
		tx_ctx.TsoEn = 1;
	} else if (xmit_info->tso_ufo == 2) {
		tx_ctx.UfoEn = 1;
	}
	tx_ctx.MinMfl = xmit_info->mtu;
	tx_ctx.MaxMfl = xmit_info->max_mtu;
	tx_ctx.TxLenAutoInc = 1;
	tx_ctx.TxPara.FrameType = xmit_info->type;
	if(xmit_info->dest_type==1) {
		memcpy(tx_ctx.MacDstAddr, &MacDstAddr_mul[0], 6);
	} else if(xmit_info->dest_type==2) {
		memcpy(tx_ctx.MacDstAddr, &MacDstAddr_bro[0], 6);
	} else {
		memcpy(tx_ctx.MacDstAddr, &MacDstAddr_uni[0], 6);
	}
	memcpy(tx_ctx.MacSrcAddr, &MacSrcAddr_uni[0], 6);
	if (xmit_info->vlan != 0) {
		tx_ctx.TxPara.AddVlanTag = 1;
		tx_ctx.TxPara.TxVlanTag = xmit_info->vlan;
	}
	tx_ctx.TxPara.DescFrom = xmit_info->desc_min;
	tx_ctx.TxPara.DescTo = xmit_info->desc_max;
	tx_ctx.TxPara.DescLen = xmit_info->len;
	tx_ctx.TxPara.MaxDescLen = xmit_info->max_len;
	tx_ctx.TxPara.DescTimes = xmit_info->repeat;
	return util_mac_tx_start(&tx_ctx);
}

void util_mac_tx_stop(void)
{
}

void util_mac_status(PMAC_ADAPTER pAdapter)
{
	//cxt->net_stats = pAdapter->net_stats;
	MAC_Status_Context u_st_cxt;
	PMAC_Status_Context cxt = NULL;
	int i;
	memset(&u_st_cxt, 0, sizeof(u_st_cxt));
	cxt = &u_st_cxt;
	memcpy(&(cxt->net_stats), &(pAdapter->net_stats), sizeof(struct net_device_stats));
	//cxt->mac_stats = pAdapter->mac_stats;
	memcpy(&(cxt->mac_stats), &(pAdapter->mac_stats), sizeof(struct toe2_device_stats));
	cxt->TxLenAuto = pAdapter->TxLenAuto;
	cxt->TxCrcErrs = pAdapter->TxCrcErrs;

#ifdef UTIL_DEBUG
	pr_info("[T] Status TxLenAuto: %d", cxt->TxLenAuto);
	pr_info("[T] Status TxCrcErrs: %d\n", cxt->TxCrcErrs);

	pr_info("[T] Status net_stats rx_packets: %lu", cxt->net_stats.rx_packets);
	pr_info("[T] Status net_stats tx_packets: %lu", cxt->net_stats.tx_packets);
	pr_info("[T] Status net_stats rx_bytes: %lu", cxt->net_stats.rx_bytes);
	pr_info("[T] Status net_stats tx_bytes: %lu", cxt->net_stats.tx_bytes);
	pr_info("[T] Status net_stats rx_errors: %lu", cxt->net_stats.rx_errors);
	pr_info("[T] Status net_stats tx_errors: %lu", cxt->net_stats.tx_errors);
	pr_info("[T] Status net_stats rx_dropped: %lu", cxt->net_stats.rx_dropped);
	pr_info("[T] Status net_stats tx_dropped: %lu", cxt->net_stats.tx_dropped);
	pr_info("[T] Status net_stats multicast: %lu", cxt->net_stats.multicast);
	pr_info("[T] Status net_stats collisions: %lu", cxt->net_stats.collisions);
	pr_info("[T] Status net_stats rx_length_errors: %lu", cxt->net_stats.rx_length_errors);
	pr_info("[T] Status net_stats rx_over_errors: %lu", cxt->net_stats.rx_over_errors);
	pr_info("[T] Status net_stats rx_crc_errors: %lu", cxt->net_stats.rx_crc_errors);
	pr_info("[T] Status net_stats rx_frame_errors: %lu", cxt->net_stats.rx_frame_errors);
	pr_info("[T] Status net_stats rx_fifo_errors: %lu", cxt->net_stats.rx_fifo_errors);
	pr_info("[T] Status net_stats rx_missed_errors: %lu", cxt->net_stats.rx_missed_errors);
	pr_info("[T] Status net_stats tx_aborted_errors: %lu", cxt->net_stats.tx_aborted_errors);
	pr_info("[T] Status net_stats tx_carrier_errors: %lu", cxt->net_stats.tx_carrier_errors);
	pr_info("[T] Status net_stats tx_fifo_errors: %lu", cxt->net_stats.tx_fifo_errors);
	pr_info("[T] Status net_stats tx_heartbeat_errors: %lu", cxt->net_stats.tx_heartbeat_errors);
	pr_info("[T] Status net_stats tx_window_errors: %lu", cxt->net_stats.tx_window_errors);
	pr_info("[T] Status net_stats rx_compressed: %lu", cxt->net_stats.rx_compressed);
	pr_info("[T] Status net_stats tx_compressed: %lu\n", cxt->net_stats.tx_compressed);

	pr_info("[T] Status mac_stats rx_bc: %u", cxt->mac_stats.rx_bc);
	pr_info("[T] Status mac_stats rx_buf_empty: %u", cxt->mac_stats.rx_buf_empty);
	pr_info("[T] Status mac_stats rx_dribble_errors: %u", cxt->mac_stats.rx_dribble_errors);
	pr_info("[T] Status mac_stats rx_frag: %u", cxt->mac_stats.rx_frag);
	pr_info("[T] Status mac_stats rx_hdr_chs_errs: %u", cxt->mac_stats.rx_hdr_chs_errs);
	pr_info("[T] Status mac_stats rx_ip: %u", cxt->mac_stats.rx_ip);
	pr_info("[T] Status mac_stats rx_ip_chksum_errors: %u", cxt->mac_stats.rx_ip_chksum_errors);
	pr_info("[T] Status mac_stats rx_late_col_seen: %u", cxt->mac_stats.rx_late_col_seen);
	pr_info("[T] Status mac_stats rx_long_errors: %u", cxt->mac_stats.rx_long_errors);
	pr_info("[T] Status mac_stats rx_lost_in_ring: %u", cxt->mac_stats.rx_lost_in_ring);
	pr_info("[T] Status mac_stats rx_mc: %u", cxt->mac_stats.rx_mc);
	pr_info("[T] Status mac_stats rx_pay_chs_errs: %u", cxt->mac_stats.rx_pay_chs_errs);
	pr_info("[T] Status mac_stats rx_phy_layer_errors: %u", cxt->mac_stats.rx_phy_layer_errors);
	pr_info("[T] Status mac_stats rx_pppoe: %u", cxt->mac_stats.rx_pppoe);
	pr_info("[T] Status mac_stats rx_runt_errors: %u", cxt->mac_stats.rx_runt_errors);
	pr_info("[T] Status mac_stats rx_tcp_chksum_errors: %u", cxt->mac_stats.rx_tcp_chksum_errors);
	pr_info("[T] Status mac_stats rx_uc: %u", cxt->mac_stats.rx_uc);
	pr_info("[T] Status mac_stats rx_vlan: %u", cxt->mac_stats.rx_vlan);
	pr_info("[T] Status mac_stats rx_wd_timeout_errors: %u", cxt->mac_stats.rx_wd_timeout_errors);
	for (i=0; i<16; i++)
	{
		pr_info("[T] Status   mac_stats tx_col_cnts: %u", cxt->mac_stats.tx_col_cnts[i]);
	}
	pr_info("[T] Status mac_stats tx_col_errors: %u\n", cxt->mac_stats.tx_col_errors);
#endif
}
