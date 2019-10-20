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

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/if_vlan.h>
#include <linux/mii.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/crc32.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/delay.h> 
#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/mach-ali/m6303.h>
#include <linux/version.h>
#include <asm-generic/dma-coherent.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include "gmac_util.h"
#include "eth_reg.h"

#ifdef TX_XMIT_DEBUG
#define ali_tx_info(msg,args...) printk(KERN_INFO msg "\n", ##args)
#else
#define ali_tx_info(msg,args...)
#endif
static u8 MacDstAddr_uni[6]={0x60,0xEB,0x69,0x92,0x8A,0xFE};
static u8 MacDstAddr_mul[6]={0xFF,0xEB,0x69,0x92,0x8A,0xFE};
static u8 MacDstAddr_bro[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
extern pgmac_private g_pgmac;
static u8* ToeTxBuf = NULL;
tx_con_desc TxContextDescriptor;
tx_data_desc TxDataDescriptor;
u32 DescSize[GMAC_TX_DESC_NUM];
u16 ChsAcc, ChsOff;
extern u32 gmac_base;
extern u8 stb_gmac_mac[ETH_ALEN];
void show_tx_desc(ptx_data_desc tx_desc) ;
#define  SWAP(value)   (((value<<8)&0xFF00)|((value>>8)&0x00FF))
void TransmitConfigContextDescriptor(pgmac_private pgmac) {
	u16 WPtr;

	WPtr = pgmac->tx_wptr;
	TxContextDescriptor.OWN = 1;
	if(pgmac->TsoUfoEn) {
		TxContextDescriptor.MSS = pgmac->tx_mtu;
	}

	memcpy(&pgmac->tx_desc[WPtr], (u8 *)(&TxContextDescriptor), sizeof(tx_con_desc));
	
	if((++ pgmac->tx_wptr) >= GMAC_TX_DESC_NUM) {
		pgmac->tx_wptr = 0;
	}
    return;
}

// updata TSAD & config TSD.
void TransmitConfigDataDescriptor(pgmac_private pgmac, u16 WPtr, u16 Off, u32 Size, ptx_data_desc pTxDataDes)
{
	u8 temp_buffer[64];
    ptx_data_desc tmp;
	pTxDataDes->seg_len = Size;
	
	if(WPtr == GMAC_TX_DESC_NUM - 1) {
		pTxDataDes->EOR = 1;
	} else {
		pTxDataDes->EOR = 0;
	}

	//pTxDataDes->PacketBuffer = (u32)(ToeTxBuf + Off);
	pTxDataDes->pkt_buf_dma = dma_map_single((struct device *)NULL, (void*)(ToeTxBuf + Off), Size, DMA_TO_DEVICE);

    pTxDataDes->pkt_buf_dma = pTxDataDes->pkt_buf_dma;

	memcpy(&pgmac->tx_desc[WPtr], (u8 *)(pTxDataDes), sizeof(tx_data_desc));
    memcpy(temp_buffer, &pgmac->tx_desc[WPtr], sizeof(tx_data_desc));
    tmp = (ptx_data_desc)temp_buffer;
#if 0
    show_tx_desc(tmp);
#endif
	if((++ pgmac->tx_wptr) >= GMAC_TX_DESC_NUM) {
		pgmac->tx_wptr = 0;
	}
    return;
}
unsigned char gmac_temp_buff[60]=
{0xff,0xff,0xff,0xff,0xff,0xff,0xd4,0xae,0x52,0x82,0xe7,0xa4,0x08,0x06,0x00,0x01,
0x08,0x00,0x06,0x04,0x00,0x01,0xd4,0xae,0x52,0x82,0xe7,0xa4,0x0a,0x29,0x96,0x02,
0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x29,0x96,0xc2,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void show_tx_desc(ptx_data_desc tx_desc) {
    ali_info("Begin show tx desc:\n");
    ali_info("SegmentNum : %d\n", tx_desc->SegmentNum); 
    ali_info("Reserved20_16: 0x%x\n", tx_desc->Reserved20_16); 
    ali_info("TOE_UFO_En: %d\n", tx_desc->TOE_UFO_En); 
    ali_info("VlanEn: %d\n", tx_desc->VlanEn); 
    ali_info("CoeEn: %d\n", tx_desc->CoeEn); 
    ali_info("LS: %d\n", tx_desc->LS); 
    ali_info("FS: %d\n", tx_desc->FS); 
    ali_info("EOR: %d\n", tx_desc->EOR); 
    ali_info("ContextIndex: %d\n", tx_desc->ContextIndex); 
    ali_info("ContextData: %d\n", tx_desc->ContextData); 
    ali_info("OWN: %d\n", tx_desc->OWN); 
    ali_info("pkt_buf_dma: 0x%x\n", tx_desc->pkt_buf_dma); 
    ali_info("seg_len: %d\n", tx_desc->seg_len); 
    ali_info("tot_len: %d\n", tx_desc->tot_len); 
    ali_info("FragmentID: %u\n", tx_desc->FragmentID); 
    return;
}

static int gmac_util_xmit(pgmac_private pgmac, u32 DescNum) {
	u16 DescWritePtr, DescReadPtr;
	u16 Available;
	u16 Off, Desc;
	tx_data_desc TxDataDes;
	u16 TxStatusDescPtr;
	u16 tmp16;
	u8 temp_buffer[64];
    int i = 0;

	if(!(pgmac->link_established)) {
		return 0;
    }

    tmp16 = GMAC_R16(TxRingDesRPtr);
	do {
		DescReadPtr = GMAC_R16(TxRingDesRPtr);
		DescWritePtr = pgmac->tx_wptr;

		if(DescReadPtr > DescWritePtr) {
			Available = DescReadPtr - DescWritePtr -1;
		} else {
			Available = GMAC_TX_DESC_NUM + DescReadPtr - DescWritePtr -1;
		}

	} while(Available < DescNum+1);

	if ((TxDataDescriptor.CoeEn) || (TxDataDescriptor.VlanEn)) {
		TransmitConfigContextDescriptor(pgmac);
	}

	TxStatusDescPtr = pgmac->tx_wptr;

	memset(&TxDataDes, 0, sizeof(tx_data_desc));
	//Load config from app
	memcpy(&TxDataDes, &TxDataDescriptor, sizeof(tx_data_desc));//TxDataDes = TxDataDescriptor;
	for(Off = 0, Desc = 0; Desc < DescNum; Desc ++) {
		TxDataDes.OWN = 1;
		if(pgmac->TsoUfoEn) {
			TxDataDes.TOE_UFO_En = 1;
		} if(Desc == 0) {			
			TxDataDes.FS = 1;			
		} else {
			TxDataDes.FS = 0;
		}	
		
		TransmitConfigDataDescriptor(pgmac, pgmac->tx_wptr, Off, DescSize[Desc], &TxDataDes);
		Off += DescSize[Desc];
	}
	memcpy(temp_buffer, &TxDataDes, sizeof(tx_data_desc));

	GMAC_W16(TxRingDesWPtr, pgmac->tx_wptr);
    i = 0;
	do {
		DescReadPtr = GMAC_R16(TxRingDesRPtr);
		DescWritePtr = pgmac->tx_wptr;
        i++;
        if (i > 100) {
            break;
        }
	} while(DescWritePtr != DescReadPtr);// increment several in TxDescWPtr?

	pgmac->TxCrcErrs += GMAC_R16(TxCrcErrCnt);
	return 0;
}

static int pkts = 0;
void MacTransFrames(pgmac_private pgmac, u32 DescNum, u32 DescLen, u32 DescTimes) {
	u32 bytes, rand;
	u32 times;
	u32 i;
	
	TxDataDescriptor.tot_len = DescLen;
	TxDataDescriptor.SegmentNum = DescNum;

	times = DescTimes;

	while(times--) {
		bytes = DescLen;
		ali_tx_info("DescSize(%u): ", DescNum);
		for(i = 0; i < (DescNum - 1); i++) {		
			rand = 1 + get_random_int() % (bytes-(DescNum - i));
			DescSize[i] = rand;
			ali_tx_info("(%u)->", rand);
			bytes -=  rand;
		}
		
		DescSize[DescNum - 1] = bytes;
		ali_tx_info("(%u)\n", bytes);

		pkts++;
		gmac_util_xmit(pgmac, DescNum); //TransmitPackets(pAdapter, DescNum);			
	}
    return;
}

/* pHdr is IP header starting address */
u32 Ipv6BuildExtHdr(struct ali_mac_xmit_io* cmd, u8* pHdr) {
	u8 exthdr_hop_len = 0, exthdr_dest_len = 0;
    u8 exthdr_route_len =0, exthdr_dest2_len=0;
	u32 exthdr_hop_len_byte = 0, exthdr_dest_len_byte = 0;
    u32 exthdr_route_len_byte =0, exthdr_dest2_len_byte=0;
	u32 byte_num;
	u8 i;
	u8 *pIP_base = pHdr;
#if 0
#ifdef simple_ext_header
	*(u16 *)(pHdr+6) = 0x8000|43;
	pHdr+= IPv6_NXT_HDR;//point to next header
 	*(u8 *)pHdr = 6;//next header is tcp
 	*(u8 *)(pHdr+1) = 2;//route len
	*(u8 *)(pHdr+2) = 0;//route type
	*(u8 *)(pHdr+3) = 0;
	*(u32 *)(pHdr+4) = 0;//reserved
	*(u8 *)(pHdr+4) = 1;//rest route
	*(u32 *)(pHdr+8) = 0x11223344;
	*(u32 *)(pHdr+12) = 0x55667788;
	*(u32 *)(pHdr+16) = 0x88776655;
	*(u32 *)(pHdr+20) = 0x44332211;

	//ext_head_totlen = 24;

	return (24);
#endif
#endif
	//judge what is next ext header...
	if(cmd->hop_hd_len) {
		*(u8 *)(pIP_base+6) = 0;
	} else if(cmd->dst_hd_len) {
		*(u8 *)(pIP_base+6) = 60;
	} else if(cmd->rt_hd_len) {
        *(u8 *)(pIP_base+6) = 43;
	} else if(cmd->dst_2_hd_len) {
        *(u8 *)(pIP_base+6) = 60;
	} else {
		if(cmd->type == MAC_FRAME_IPv6_TCP) {
            /* next header is tcp ext header */
			*(u8 *)(pIP_base+6)  = 6; 
		} else if(cmd->type == MAC_FRAME_IPv6_UDP) {
            /* next header is udp ext header */
			*(u8 *)(pIP_base+6)  = 17;
        }
	}
	
	pHdr+= IPv6_NXT_HDR;//point to next header
	if(cmd->hop_hd_len) {	
		//judge what is next ext header...
		if(cmd->dst_hd_len) {//(Adapter.exthdr_dest)
			*(u8 *)pHdr = 60; //next header is dest ext header
        } else if(cmd->rt_hd_len) {//(Adapter.exthdr_route)
			*(u8 *)pHdr = 43; //next header is route ext header
		} else if(cmd->dst_2_hd_len) { 
			*(u8 *)pHdr = 60;
		} else {
			if(cmd->type == MAC_FRAME_IPv6_TCP) {
				*(u8 *)pHdr = 6; //next header is tcp ext header
			} else if(cmd->type == MAC_FRAME_IPv6_UDP) {
				*(u8 *)pHdr = 17;//next header is udp ext header
            }
		}

		//hop ext header len
		exthdr_hop_len = cmd->hop_hd_len;//means option is 8*exthdr_hop_len bytes
		*(u8 *)(pHdr+1) = exthdr_hop_len;
		*(u16 *)(pHdr+2) = 0;
		*(u32 *)(pHdr+4) = 0;

		//option: type, len, and date...
		for(byte_num = 0; byte_num < exthdr_hop_len*8; byte_num++) {
			*(u8 *)(pHdr+8+byte_num) = (u8)byte_num;
        }
		exthdr_hop_len_byte = (1+exthdr_hop_len)*8;
	}	
	pHdr+=exthdr_hop_len_byte;
	if(cmd->dst_hd_len) {  //(Adapter.exthdr_dest)
		//judge what is next ext header...
		if(cmd->rt_hd_len) {        //(Adapter.exthdr_route)
			*(u8 *)pHdr = 43; //next header is route ext header
		} else if(cmd->dst_2_hd_len) {
			*(u8 *)pHdr = 60;
		} else {
			if(cmd->type == MAC_FRAME_IPv6_TCP) {
				*(u8 *)pHdr = 6; //next header is tcp ext header
			} else if(cmd->type == MAC_FRAME_IPv6_UDP) {
				*(u8 *)pHdr = 17;//next header is udp ext header
            }
		}	

		//dest ext header len
		exthdr_dest_len = cmd->dst_hd_len;//means option is 8*exthdr_dest_len bytes
		*(u8 *)(pHdr+1) = exthdr_dest_len;
		*(u16 *)(pHdr+2) = 0;
		*(u32 *)(pHdr+4) = 0;
		
		//option: type, len, and date...
		for(byte_num = 0; byte_num < exthdr_dest_len*8; byte_num++) {
			*(u8 *)(pHdr+8+byte_num) = (u8)byte_num;
        }
		exthdr_dest_len_byte = (1+exthdr_dest_len)*8;
	}


	pHdr+=exthdr_dest_len_byte;
	if(cmd->rt_hd_len) {    //(Adapter.exthdr_route)
		if(cmd->dst_2_hd_len) {
			*(u8 *)pHdr = 60;
		} else {
			if(cmd->type == MAC_FRAME_IPv6_TCP) {
				*(u8 *)pHdr = 6; //next header is tcp ext header
			} else if(cmd->type == MAC_FRAME_IPv6_UDP) {
				*(u8 *)pHdr = 17;//next header is udp ext header
            }
		}

		//route ext header len
		exthdr_route_len = cmd->rt_hd_len*2; //Adapter.exthdr_route_num*2;

		*(u8 *)(pHdr+1) = exthdr_route_len;
		*(u8 *)(pHdr+2) = 0;//route type
		*(u8 *)(pHdr+3) = cmd->rt_hd_len; //Adapter.exthdr_route_num;//route type? which one is type?
		*(u32 *)(pHdr+4) = 0;//reserved

		for(i=0; i<cmd->rt_hd_len; i++) {
			*(u32 *)(pHdr+8+i*16) = 0x44332211+i;
			*(u32 *)(pHdr+12+i*16) = 0x88776655+i;
			*(u32 *)(pHdr+16+i*16) = 0x55667788;
			*(u32 *)(pHdr+20+i*16) = 0x11223344;		
		}
		exthdr_route_len_byte = (1+exthdr_route_len)*8;
	}
	
	pHdr+=exthdr_route_len_byte;
	if(cmd->dst_2_hd_len) {   //(Adapter.exthdr_dest)
		//judge what is next ext header...
		if(cmd->type == MAC_FRAME_IPv6_TCP) {
			*(u8 *)pHdr = 6; //next header is tcp ext header
		} else if(cmd->type == MAC_FRAME_IPv6_UDP) {
			*(u8 *)pHdr = 17;//next header is udp ext header
        }

		//dest ext header len
		exthdr_dest2_len = cmd->dst_2_hd_len;//means option is 8*exthdr_dest2_len bytes
		*(u8 *)(pHdr+1) = exthdr_dest2_len;
		*(u16 *)(pHdr+2) = 0;
		*(u32 *)(pHdr+4) = 0;
		
		//option: type, len, and date...
		for(byte_num = 0; byte_num < exthdr_dest2_len*8; byte_num++) {
			*(u8 *)(pHdr+8+byte_num) = (u8)byte_num;
        }

		exthdr_dest2_len_byte = (1+exthdr_dest2_len)*8;
	}
	pHdr+=exthdr_dest2_len_byte;

	return (exthdr_hop_len_byte+exthdr_dest_len_byte+exthdr_route_len_byte+exthdr_dest2_len_byte);
}


u16 standard_chksum(void *dataptr, u16 len) {
    u32 acc;
    u16 src;
    u8 *octetptr;

    acc = 0;
    /* dataptr may be at odd or even addresses */
    octetptr = (u8*)dataptr;
        while (len > 1) {
        /* declare first octet as most significant thus assume network order, ignoring host order */
        src = (*octetptr) << 8;
        octetptr++;
        /* declare second octet as least significant */
        src |= (*octetptr);
        octetptr++;
        acc += src;
        len -= 2;
    }
    if (len > 0) {
        /* accumulate remaining octet */
        src = (*octetptr) << 8;
        acc += src;
    }
    /* add deferred carry bits */
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
    if ((acc & 0xffff0000) != 0) {
        acc = (acc >> 16) + (acc & 0x0000ffffUL);
    }
    /* This maybe a little confusing: reorder sum using htons()
     instead of ntohs() since it has a little less call overhead.
     The caller must invert bits for Internet sum ! */
    return htons((u16)acc);
}

u32 standard_chksum_2(void *dataptr, u16 len) {
    u32 acc;
    u16 src;
    u8 *octetptr;

    acc = 0;
    /* dataptr may be at odd or even addresses */
    octetptr = (u8*)dataptr;
    while (len > 1) {
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

    if (len > 0) {
        /* accumulate remaining octet */
        src = (*octetptr) << 8;
        acc += src;
    }

    while ((acc >> 16) != 0) {
        acc = (acc & 0xffff) + (acc >> 16);
    }
    return (acc);
}

u8* ipv6FindDestAddressOffset(struct ali_mac_xmit_io* cmd, u8* pHdr) {
	//u8* ipv6_des_off;
	u32 exthdr_hop_len_byte = 0, exthdr_dest_len_byte = 0, exthdr_route_len_byte = 0; 

	//pHdr points to ip header 
	//(Adapter.exthdr_route)//pseudo2
	if(cmd->rt_hd_len) { 
		//ipv6_des_off = (u8*)(pHdr + IPv6_IPDES_OFF);

		pHdr+=IPv6_NXT_HDR; // pHdr points to next header
		
		if(cmd->hop_hd_len) {     //(Adapter.exthdr_hop)
			exthdr_hop_len_byte = (1 + *(u8*)(pHdr+1))*8;
        }
		pHdr+=exthdr_hop_len_byte;
		
		if(cmd->dst_hd_len) {   //(Adapter.exthdr_dest)
			exthdr_dest_len_byte = (1 + *(u8*)(pHdr+1))*8;
        }
		pHdr+=exthdr_dest_len_byte;

		exthdr_route_len_byte = (1 + *(u8*)(pHdr+1))*8;
		pHdr+=exthdr_route_len_byte; //pHdr points to route ext header
		pHdr-=16; //pHdr points to last address in route, which is dest adress
	} else {
		pHdr += IPv6_IPDES_OFF;
    }
	return pHdr;
}

void MacBuildHdr(struct ali_mac_xmit_io* cmd, u8 *pBuf, u32 len) {
	u8 *pHdr = pBuf;
	u16 IpHdrLen = 5;
	u32 acc = 0;
	u32 ip_src, ip_des; 
	u16 ip_protocol;
	u16 ChkLen;
	u16 L2HeaderLen = 0;	
	u16 L3HeaderLen = 0;
	u16 L4HeaderLen = 0;
	u8 *ipv6_src_off, *ipv6_des_off;
	u16 uplayerlen;
	u32 ext_header_len = 0;
    static u16 udp_ip_id = 1;
    static u16 tcp_ip_id = 1;
    static u32 tcp_seq_num = 1;
    static u32 tcp6_seq_num = 1;
    u32 tcp_seq_intl = 65535;
    u32 tcp6_seq_intl = 65535;
		
	ChsOff = 0;

	if(cmd->dest_type==1) {
		memcpy(pHdr, &MacDstAddr_mul[0], 6);
	} else if(cmd->dest_type==2) {
		memcpy(pHdr, &MacDstAddr_bro[0], 6);
	} else {
		memcpy(pHdr, &MacDstAddr_uni[0], 6);
    }
	pHdr += 6; // 6 now.
	memcpy(pHdr, &stb_gmac_mac[0], 6);
	pHdr += 6; // 12 now.
	L2HeaderLen = 12; 

	
	memset(&TxContextDescriptor, 0, sizeof(tx_con_desc));
	TxContextDescriptor.ContextData = 1;
	memset(&TxDataDescriptor, 0, sizeof(tx_data_desc));


	switch(cmd->type) {
	case MAC_FRAME_IP_UDP:
		*(u16 *)pHdr = SWAP(ETH_IP); 
		pHdr += 2; // 14 now.

		L3HeaderLen = 20;
		L4HeaderLen = 8;
		
		*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength.
		*(u16 *)(pHdr+2) = SWAP((len - MAC_HDR_LEN)); //Tot Len.
		*(u16 *)(pHdr+4) = htons(udp_ip_id++); //ip_id.
		*(u16 *)(pHdr+6) = 0x0000; //Offset.
		ip_protocol = 0x1180;
		*(u16 *)(pHdr+8) = ip_protocol; //Ttl(128) & Protocol(UDP:0x11).
		*(u16 *)(pHdr+10) = 0x0000; //IP Checksum.
		L2HeaderLen += 2;  // 14 now.	

		acc = standard_chksum_2(pHdr, (IpHdrLen*4));
		acc =  htons((u16)~acc);
		*(u16 *)(pHdr+10) = acc;
		acc = 0;

		*(u16 *)(pHdr + (IpHdrLen*4) +4) = SWAP((len - MAC_HDR_LEN-L3HeaderLen)); //udp len
		*(u16 *)(pHdr + (IpHdrLen*4) +6) = 0x0000; //Clear UDP checksum

		if(cmd->toe_tx) {
			TxDataDescriptor.CoeEn = 1;
			TxContextDescriptor.L3Type = 0;//indicate IPv4
			TxContextDescriptor.L4Type = 1;
			
			ip_src = *(u32 *)(pHdr + IP_IPSRC_OFF);
			ip_des = *(u32*)(pHdr + IP_IPDES_OFF);

			pHdr += (IpHdrLen*4);
			ChkLen = len - MAC_HDR_LEN - (IpHdrLen*4);
			
			acc = standard_chksum(pHdr, ChkLen);//
		
			while ((acc >> 16) != 0) {
			  acc = (acc & 0xffff) + (acc >> 16);
			}
			
			acc += (ip_src & 0xFFFFUL);
			acc += ((ip_src>>16) & 0xFFFFUL);
			
			acc += (ip_des & 0xFFFFUL);
			acc += ((ip_des>>16) & 0xFFFFUL);
			
			acc += (ip_protocol & 0xFF00);
			acc += (u32)SWAP(ChkLen);//fake header used for checksum	
		
			while ((acc >> 16) != 0) {
				acc = (acc & 0xffffUL) + (acc >> 16);
			}

			ChsAcc =(u16)~acc;
			ChsOff = MAC_HDR_LEN + (IpHdrLen*4) +6;
		}
		break;

	case MAC_FRAME_IP_TCP:
		*(u16 *)pHdr = SWAP(ETH_IP); 
		pHdr += 2; // 14 now.point to the IP position	

		L3HeaderLen = 20;//IP header 20 bytes, what if use options? 
		L4HeaderLen = 20;// TCP header 20 bytes		

		*(u16 *)(pHdr+0) = (0x0040 |IpHdrLen); //Version HeaderLength.
		*(u16 *)(pHdr+2) = SWAP((len - MAC_HDR_LEN)); //Tot Len., how about CRC in the last?
		*(u16 *)(pHdr+4) = htons(tcp_ip_id); //ip_id.
        tcp_ip_id += 500;
		*(u16 *)(pHdr+6) = 0x0000; //Offset., where is pHdr+4??
		ip_protocol = 0x0680;
		*(u16 *)(pHdr+8) = ip_protocol; //Ttl(128) & Protocol(TCP:0x06).careful with the SWAP
		*(u16 *)(pHdr+10) = 0x0000; //IP Checksum. when to calculate?
		L2HeaderLen += 2;  // 14 now.

		acc = standard_chksum_2(pHdr, (IpHdrLen*4));
		acc =  htons((u16)~acc);
		*(u16 *)(pHdr+10) = acc;
		acc = 0;

		*(u16 *)(pHdr+(IpHdrLen*4)+12) = 0x0050; //set TCP header len
		*(u32 *)(pHdr+(IpHdrLen*4)+4) = htonl(tcp_seq_num); //set TCP header len
        tcp_seq_num += tcp_seq_intl;
		*(u16 *)(pHdr+(IpHdrLen*4)+16) = 0x0000; //Clear TCP checksum

		if(cmd->toe_tx) {
			TxDataDescriptor.CoeEn = 1;
			TxContextDescriptor.L3Type = 0;//indicate IPv4
			TxContextDescriptor.L4Type = 0;
			
			ip_src = *(u32 *)(pHdr + IP_IPSRC_OFF);
			ip_des = *(u32*)(pHdr + IP_IPDES_OFF);	
			
			pHdr += (IpHdrLen*4);
			
			ChkLen = len - MAC_HDR_LEN - (IpHdrLen*4);
			acc = standard_chksum(pHdr, ChkLen);
		
			while ((acc >> 16) != 0) {
			  acc = (acc & 0xffff) + (acc >> 16);
			}

			acc += (ip_src & 0xFFFFUL);
			acc += ((ip_src>>16) & 0xFFFFUL);
			acc += (ip_des & 0xFFFFUL);
			acc += ((ip_des>>16) & 0xFFFFUL);
			acc += (ip_protocol & 0xFF00);
			acc += SWAP(ChkLen);;

			while ((acc >> 16) != 0) {
				acc = (acc & 0xffffUL) + (acc >> 16);
			}
			ChsAcc =(u16)~acc;//TCP checksum
			ChsOff = MAC_HDR_LEN + (IpHdrLen*4) +16;//TCP checksum offset in ethernet.
		}
		break;

	case MAC_FRAME_IPv6_UDP:

		*(u16 *)pHdr = SWAP(ETH_IPv6); 
		pHdr += 2; // 14 now, point to the IP position	
		L2HeaderLen += 2;
		L3HeaderLen = 40;//IPv6 header 40 bytes
		L4HeaderLen = 8;// UDP header 8 bytes	

		*(u16 *)(pHdr+0) = (0x0060);	//ver. class, flow
		*(u16 *)(pHdr+2) = (0x0);	//flow
		*(u16 *)(pHdr+4) = SWAP((len - MAC_HDR_LEN - L3HeaderLen)); //payload len

		*(u16 *)(pHdr+6) = 0x8000;//hop limit
		ext_header_len = Ipv6BuildExtHdr(cmd, (u8 *)pHdr);
		//udp len
		*(u16 *)(pHdr+L3HeaderLen+ext_header_len+4)=SWAP((len - MAC_HDR_LEN - L3HeaderLen-ext_header_len)); 
		*(u16 *)(pHdr+L3HeaderLen+ext_header_len+6) = 0x0000; //Clear UDP checksum

		if(cmd->toe_tx) {
			TxDataDescriptor.CoeEn = 1;
			TxContextDescriptor.L3Type = 1;//indicate IPv6
			TxContextDescriptor.L4Type = 1;	//indicate UDP

			ipv6_src_off = (u8 *)(pHdr + IPv6_IPSRC_OFF);	//pseudo1
			ipv6_des_off= ipv6FindDestAddressOffset(cmd, (u8 *)pHdr);

			pHdr += L3HeaderLen;
			pHdr += ext_header_len;//pHdr points to uplayer header, here is udp header

			uplayerlen = (len - MAC_HDR_LEN - L3HeaderLen - ext_header_len);//udp total len

			*(u16 *)(pHdr+6) = 0;//clear udp checksum
			acc = standard_chksum_2(pHdr, uplayerlen);//calculate tcp checksum (tcp header and tcp data)

			//pseudo1, src address
			acc += standard_chksum_2(ipv6_src_off, 16);
			//pseudo2, dest address
			acc += standard_chksum_2(ipv6_des_off, 16);
			//pseudo3, uplayerlen
			acc += (uplayerlen);
			//pseudo4, udp header type 17
			acc += 17;
			
			while ((acc >> 16) != 0) {
				acc = (acc & 0xffffUL) + (acc >> 16);
			}

			ChsAcc = htons((u16)~acc);//UDP checksum with pseudo header...
			ChsOff = MAC_HDR_LEN + L3HeaderLen + ext_header_len + 6;
			ali_tx_info("MAC_FRAME_IPv6_UDP, ChsAcc = 0x%x, ChsOff=0x%x\n", ChsAcc, ChsOff);
		}
		break;
		
	case MAC_FRAME_IPv6_TCP:
		*(u16 *)pHdr = SWAP(ETH_IPv6); 
		pHdr += 2; // 14 now, point to the IP position	
		
		L2HeaderLen += 2;
		L3HeaderLen = 40;//IPv6 header 40 bytes
		L4HeaderLen = 20;// TCP header 20 bytes		

		*(u16 *)(pHdr+0) = (0x0060);	//ver. class, flow
		*(u16 *)(pHdr+2) = (0x0);	//flow
		*(u16 *)(pHdr+4) = SWAP((len - MAC_HDR_LEN - L3HeaderLen)); //payload len

		*(u16 *)(pHdr+6) = 0x8000;//hop limit
		ext_header_len = Ipv6BuildExtHdr(cmd, (u8 *)pHdr);

		*(u16 *)(pHdr+L3HeaderLen+ext_header_len+12)=0x0050;//make tcp header len is 5(5*4=20)
		*(u16 *)(pHdr+L3HeaderLen+ext_header_len+16) = 0x0000; //Clear TCP checksum
		*(u32 *)(pHdr+L3HeaderLen+ext_header_len+4) = htonl(tcp6_seq_num); //Clear TCP checksum
        tcp6_seq_num += tcp6_seq_intl;
		
		if(cmd->toe_tx) {
			TxDataDescriptor.CoeEn = 1;
			TxContextDescriptor.L3Type = 1;//indicate IPv6
			TxContextDescriptor.L4Type = 0;	//indicate TCP

			ipv6_src_off = (u8 *)(pHdr + IPv6_IPSRC_OFF);	//pseudo1
			ipv6_des_off= ipv6FindDestAddressOffset(cmd, (u8 *)pHdr);

			pHdr += L3HeaderLen;
			pHdr += ext_header_len;//pHdr points to uplayer header, here is tcp header
			
			uplayerlen = (len - MAC_HDR_LEN - L3HeaderLen - ext_header_len);//tcp total len
			
			//ali_info("MAC_FRAME_IPv6_TCP, pHdr = 0x%x, uplayerlen=%d\n", pHdr, uplayerlen);
			//ali_info("	>>	*(u32 *)pHdr = 0x%x\n", (*(u32 *)pHdr));

			*(u16 *)(pHdr+16) = 0;//clear tcp checksum
			acc = standard_chksum_2(pHdr, uplayerlen);//calculate tcp checksum (tcp header and tcp data)

			//ali_info("	>>	*(u32 *)ipv6_src_off = 0x%x\n", (*(u32 *)ipv6_src_off));
			//ali_info("	>>	*(u32 *)ipv6_des_off = 0x%x\n", (*(u32 *)ipv6_des_off));

			//pseudo1, src address
			acc += standard_chksum_2(ipv6_src_off, 16);
			//pseudo2, dest address
			acc += standard_chksum_2(ipv6_des_off, 16);
			//pseudo3, uplayerlen
			acc += (uplayerlen);
			//pseudo4, tcp header type 0x06
			acc += 6;
			
			while ((acc >> 16) != 0) {
				acc = (acc & 0xffffUL) + (acc >> 16);
			}

			ChsAcc = htons((u16)~acc);//TCP checksum with pseudo header...
			ChsOff = MAC_HDR_LEN + L3HeaderLen + ext_header_len + 16;

			ali_tx_info("MAC_FRAME_IPv6_TCP, ChsAcc = 0x%x, ChsOff=0x%x\n", ChsAcc, ChsOff);
		}
		break;
		
	default:
		ali_info("FrameType(%d) Unknow.\n", cmd->type);
		break;	
	}	

	if(TxDataDescriptor.CoeEn) {
		TxContextDescriptor.L2HeaderLen = L2HeaderLen;
		TxContextDescriptor.L3HeaderLen = L3HeaderLen + ext_header_len;//problem??
		TxContextDescriptor.L4HeaderLen = L4HeaderLen;

		ali_info("TxContextDescriptor, L2HeaderLen = %d, L3HeaderLen = %d, L4HeaderLen = %d.\n", TxContextDescriptor.L2HeaderLen,\
			TxContextDescriptor.L3HeaderLen, TxContextDescriptor.L4HeaderLen);
	}

	if(cmd->vlan) {
		TxDataDescriptor.VlanEn = 1;
		TxContextDescriptor.VlanTag = cmd->vlan;
	}
	
}

u32 crc32_table[256];
#define CRC32_POLY 0x04c11db7
#define ReverseBit(data)	( ((data<<7)&0x80) | ((data<<5)&0x40) | ((data<<3)&0x20) | ((data<<1)&0x10) |\
                	   ((data>>1)&0x08) | ((data>>3)&0x04) | ((data>>5)&0x02) | ((data>>7)&0x01)	)

void init_crc32(void)
{
    int i, j;
    u32 c;
    u8 *p=(u8 *)&c, *p1;
    u8 k;
	
    for (i = 0; i < 256; ++i) {
		k = ReverseBit((u8)i);
		for (c = ((u32)k) << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
		p1 = (u8 *)(&crc32_table[i]);

		p1[0] = ReverseBit(p[3]);
		p1[1] = ReverseBit(p[2]);
		p1[2] = ReverseBit(p[1]);
		p1[3] = ReverseBit(p[0]);
    }
}

u32 ALiCrc32(u8 *buf, u32 len) {
	u8 *p;
	u32 crc;
	crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */

	for (p = buf; len > 0; ++p, --len) {
		crc = crc32_table[ (crc ^ *p) & 0xff] ^ (crc >> 8);
	}
	return ~crc;    /* transmit complement, per CRC-32 spec */
}

void ALiCrcChk(struct ali_mac_xmit_io* cmd, u8 *buf, u32 len) {
	u32 Crc, CrcChk, Tmp32;
	int k;	

	if(cmd->toe_tx&& (ChsOff != 0)) {
		*(u16 *)(buf + ChsOff) =  ChsAcc; 
    }

	Crc = ALiCrc32(buf, len);
	CrcChk = 0;
	
	for(k = 0; k < 32; k++) {
		Tmp32 = Crc >> k;
		Tmp32 &= 0x1;
		CrcChk |= (Tmp32<<(31 - k));
	}
	
	CrcChk = ~CrcChk;

 	GMAC_W32(TxCrcValue,CrcChk);
	
	if(cmd->toe_tx && (ChsOff != 0)) {
		*(u16 *)(buf + ChsOff) =  0; 
    }
    return;
}

void start_xmit_test(pgmac_private pgmac, struct ali_mac_xmit_io* cmd)
{
	u32 DescTimes, DescLen, DescFrom, DescTo, Desc, i;
    u16 tmp16;
    ali_info ("malloc ToeTxBuf\n");
	ToeTxBuf = kzalloc(MAX_TOE_PKT_LEN, GFP_KERNEL);
    if (!ToeTxBuf) {
        ali_info("can't malloc ToeTxBuf size is %d\n", MAX_TOE_PKT_LEN);
        return;
    }
    ali_info ("init crc32\n");
    init_crc32();
	ali_info("%s %d: ali_mac_xmit_io size %d",__FUNCTION__,__LINE__, sizeof(struct ali_mac_xmit_io));
	ali_info("%s %d: tx_rx = 0x%08X\n",__FUNCTION__,__LINE__, cmd->tx_rx);
	ali_info("%s %d: type = 0x%08X\n",__FUNCTION__,__LINE__, cmd->type);
	ali_info("%s %d: toe_tx = 0x%08X\n",__FUNCTION__,__LINE__, cmd->toe_tx);
	ali_info("%s %d: tso_ufo = 0x%08X\n",__FUNCTION__,__LINE__, cmd->tso_ufo);
	ali_info("%s %d: len = 0x%08X\n",__FUNCTION__,__LINE__, (unsigned int)cmd->len);
	ali_info("%s %d: max_len = 0x%08X\n",__FUNCTION__,__LINE__, (unsigned int)cmd->max_len);
	ali_info("%s %d: mtu = 0x%08X\n",__FUNCTION__,__LINE__, cmd->mtu);
	ali_info("%s %d: max_mtu = 0x%08X\n",__FUNCTION__,__LINE__, cmd->max_mtu);
	ali_info("%s %d: desc_min = 0x%08X\n",__FUNCTION__,__LINE__, cmd->desc_min);
	ali_info("%s %d: desc_max = 0x%08X\n",__FUNCTION__,__LINE__, cmd->desc_max);
	ali_info("%s %d: vlan = 0x%08X\n",__FUNCTION__,__LINE__, cmd->vlan);
	ali_info("%s %d: repeat = 0x%08X\n",__FUNCTION__,__LINE__, (unsigned int)cmd->repeat);
	ali_info("%s %d: dest_type = 0x%08X\n",__FUNCTION__,__LINE__, cmd->dest_type);
	ali_info("%s %d: hop_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->hop_hd_len);
	ali_info("%s %d: dst_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->dst_hd_len);
	ali_info("%s %d: rt_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->rt_hd_len);
	ali_info("%s %d: dst_2_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->dst_2_hd_len);
	pkts = 0;

	//printk("0x1fc %x\n 0x1c8 %x\n 0x1cc %x\n 0x1d0 %x\n 0x1d4 %x\n 0x1d8 %x\n 0x1dc %x\n0x1e0 %x\n0x1e4 %x\n", GMAC_R32(0x1fc), GMAC_R32(0x1c8), GMAC_R32(0x1cc), GMAC_R32(0x1d0), GMAC_R32(0x1d4), GMAC_R32(0x1d8), GMAC_R32(0x1dc), GMAC_R32(0x1e0), GMAC_R32(0x1e4));
    tmp16 = GMAC_R16(TxRingDesRPtr);
    //ali_info("%s rd 0x%x wd 0x%x\n", __FUNCTION__, tmp16, pgmac->tx_wptr);
	DescTimes = cmd->repeat;	
	DescFrom = cmd->desc_min;
	DescTo = cmd->desc_max;

	//pgmac->tx_mtu = cmd->mtu;
	pgmac->TsoUfoEn = cmd->tso_ufo;
	pgmac->toe_tx = cmd->toe_tx;

	//DescLen = cmd->len;
	for(pgmac->tx_mtu=cmd->mtu; pgmac->tx_mtu <= cmd->max_mtu; pgmac->tx_mtu++) {
		ali_info("%s %d: pgmac->tx_mtu = 0x%08X\n",__FUNCTION__,__LINE__, pgmac->tx_mtu);
		for(DescLen = cmd->len; DescLen <= cmd->max_len; DescLen++) {
			ali_info("%s %d: DescLen = 0x%08X\n",__FUNCTION__,__LINE__, DescLen);
			//Fill in Frame Buffer.
			for(i=0; i<MAX_TOE_PKT_LEN; i++) {
				ToeTxBuf[i] = (u8)i;
			}
			
			if(DescLen > MAX_TOE_PKT_LEN) {
				DescLen = MAX_TOE_PKT_LEN;
			}

			for(Desc = DescFrom; Desc <= DescTo; Desc++) {
                MacBuildHdr(cmd, ToeTxBuf, DescLen);

                if(!cmd->tso_ufo) {
                    ALiCrcChk(cmd, ToeTxBuf, DescLen);
                }
				MacTransFrames(pgmac, Desc, DescLen, DescTimes);
			}
		}
	}
	ali_info("%s send %d\n", __FUNCTION__, pkts);
	//printk("0x1fc %x\n 0x1c8 %x\n 0x1cc %x\n 0x1d0 %x\n 0x1d4 %x\n 0x1d8 %x\n 0x1dc %x\n0x1e0 %x\n0x1e4 %x\n", GMAC_R32(0x1fc), GMAC_R32(0x1c8), GMAC_R32(0x1cc), GMAC_R32(0x1d0), GMAC_R32(0x1d4), GMAC_R32(0x1d8), GMAC_R32(0x1dc), GMAC_R32(0x1e0), GMAC_R32(0x1e4));
    if(ToeTxBuf) {
        kfree(ToeTxBuf);
        ToeTxBuf = NULL;
    }
	return;
}
