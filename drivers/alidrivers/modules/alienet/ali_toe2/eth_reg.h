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

#ifndef _ALI_ETH_REG_
#define _ALI_ETH_REG_

enum ALI_MAC_REGISTERS {
	/* Command Register */
	SCR = 0,
	/* Phydical Address Register */
	PAR = 0x8,
	/* Tx Start Address of Descriptors */
	TSAD = 0x0C,
	/* Rx Start Address of Descriptors */
	RSAD = 0x10,
	/* by SW */
	RxRingDesWPtr = 0x14,
	/* by HW */
	RxRingDesRPtr = 0x16,
	/* by SW */
	TxRingDesWPtr = 0x18,
	/* by HW */
	TxRingDesRPtr = 0x1a,
	RmiiMode = 0x1c,
	VerID = 0x20,
	/* Tx Descriptor Total Number */
	TxDesTotNum = 0x24,
	/* Rx Descriptor Total Number */
	RxDesTotNum = 0x26,
	BackPressure =  0x28,
	/* Rx Status Register */
	RSR = 0x34,
	/* Interrupt Status Register */
	ISR = 0x38,
	/* Interrupt Mask Register */
	IMR = 0x3C,
	/* Network Operation Mode */
	NetworkOM = 0x40,
	/* Transmit and Receive Configuration Register 1 */
	TxRxCR1 = 0x44,
	/* Mii Management Register 1 */
	MiiMR1 = 0x48,
	/* Delay Control Register */
	DelayControlR = 0x4C,
	/* Late Collision Ajustment Register */
	LateColAjustR = 0x4E,

	/* Transmit and Receive Configuration Register 2 */
	TxRxCR2 = 0x54,
	/* Monitor Register */
	MonitorR = 0x58,
	/* General Purpose Timer Register */
	TimerR = 0x5C,
	/* Missed Frame Counter. due to rx ring buffer overflow.
	 * 15 bits valid, 1 bit overflow. write clears */
	MFC = 0x60,
	/* Purged Packet Counter. due to rx fifo overflow.
	 * 15 bits valid, 1 bit overflow. write clears */
	PPC = 0x62,
	/* Long Frame Counter. due to long packet received */
	LFC = 0x64,
	/* Runt Packet Counter. due to fragment packet received */
	RPC = 0x66,
	/* Alignment Error Counter */
	AlignErrCnt = 0x68,
	/* CRC error Packet Counter. due to CRC error packet received */
	CrcErrCnt = 0x6A,
	/* Clock Delay Chain Setting Register */
	ClkDelayChainSR = 0x74,
	/* RMII Control Registe */
	RmiiCR = 0x78,
	/* Mii Management Register 2 */
	MiiMR2 = 0x7C,
	/* Mdio Write Data */
	MdioW =	0x80,
	/* Mdio Read Data */
	MdioR = 0x82,

	/* MAC IMB Latency */
	IMBLatency = 0x84,
	/* Rx CheckSum Start Offset */
	RxChkSumStartOff = 0x8C,
	/* IP Header Checksum Fail Packet Counter. TOE II */
	IPHdrChsFailPC = 0x90,
	/* IP Payloader Checksum Fail Packet Counter. TOE II*/
	IPPayChsFailPC = 0x92,
};

enum SCRBits {
	SCRUfoEn = (1<<7),
	SCRTsoEn = (1<<6),
	SCRRxCoeEn = (1<<5),
	SCRTxCoeEn = (1<<4),
	SCRReset = (1<<3),
	SCRRxEn = (1<<2),
	SCRTxEn = (1<<1),
	SCRBufEmpty = 1,
};

enum RmiiModeBits {
	RmiiDuplex = (1<<4),
	RmiiMdSpeed = (1<<3),
	RmiiLinkStatus = (1<<2),
	RmiiTxCrsEn = (1<<1),
	RmiiAutoNeg = 1,
};

enum BackPressureBits {
	BP_En = (1<<15),
	BP_SendCntMask =  (0x7ff<<4),
	BP_SendPatMask = (0xf),
};

#define BP_SendCntOff    4
#define BP_SendPatOff    0

enum ISRBits {
	/* Rx Header Error */
	ISRRxHdrErr = (1<<14),
	/* Tx COE interpolator Error */
	ISRTxCoeErr = (1<<13),
	/* Normal Interrupt Summary */
	ISRNormal = (1<<12),
	/* Abnormal Interrupt Summary */
	ISRAbnormal = (1<<11),
	/* Buffer Discard */
	ISRRxBufDiscard = (1<<10),
	/* Link Status Change */
	ISRLinkStatus = (1<<9),
	/* HW Mdio Transaction Completion */
	ISRMdioComplete = (1<<8),
	/* Early Transmit */
	ISRTxEarly = (1<<7),
	/* Receive Watchdog Time-Out */
	ISRWatchdog = (1<<6),
	/* Receive Complete */
	ISRRxComplete = (1<<5),
	/* Transmit Fifo Underrun */
	ISRTxUnderrun = (1<<4),
	/* General Purpose Timer Expired */
	ISRTimer = (1<<3),
	/* Transmit Complete Mask */
	ISRTxComplete = (1<<2),
	/* Receive Fifo Overflow Mask */
	ISRRxFifoOverflow = (1<<1),
	/* Receive Buffer Overflow Mask */
	ISRRxBufOverflow = 1,
};

#define TOE2_INTERRUPT_MASK (ISRTxCoeErr|ISRLinkStatus|ISRWatchdog\
		|ISRRxComplete|ISRTxUnderrun|ISRTimer|ISRRxFifoOverflow\
		|ISRRxBufOverflow|ISRTxComplete)

#define TOE2_INTERRUPT_MASK2 (ISRTxCoeErr|ISRLinkStatus|ISRWatchdog\
		|ISRTxUnderrun|ISRTimer|ISRRxFifoOverflow|ISRRxBufOverflow\
		|ISRRxBufDiscard)

enum NetworkOMBits {
	RxTOEWorkMode = (1<<14),
	ForceTxFifoUnderrun = (1<<13),
	ForceLateColMode = (1<<12),
	PassExPadding = (1<<11),
	HeartBeatEn = (1<<10),
	ForceColMode = (1<<9),

	WorkModeNormal = 0,
	WorkModeWakeUp = (1<<7),
	WorkModeLoopBack = (2<<7),
	WorkModeMask = (0x3<<7),

	FullDuplexMode = (1<<6),

	PassAllMulticast = (1<<5),
	PassPromiscuous = (1<<4),
	PassErr = (1<<3),
	PassMask = (0x7<<3),

	DribbleHandling = (1<<2),

	PerfectFiltering = (0<<0),
	HashFiltering = (1<<0),
	InverseFiltering = (2<<0),
	HashOnlyFiltering = (3<<0),
	FilteringMask = (0x3<<0),
};

#define NetworkOMConfig	(PassExPadding|WorkModeNormal|PassPromiscuous\
		|PerfectFiltering|DribbleHandling)

enum TxRxCR1Bits {
	ForceReqDis = (1<<23),
	ForceSendPause = (1<<22),
	CrtlFrameTranEn = (1<<21),
};

enum MiiMR1Bits {
	MiiMdioEn = (1<<10),
	/* Used to read data from external PHY */
	MiiMdioIn = (1<<9),
	MiiMdioDir = (1<<8),
	/* Used to write data to external PHY */
	MiiMdioOut = (1<<7),
	MiiMdioClk = (1<<6),

	MiiMdioWrite0 = (MiiMdioDir),
	MiiMdioWrite1 = (MiiMdioDir|MiiMdioOut),
};

enum DelayControlRBits {
	LATE_BND = (0x7F<<16),
	CHKSUM_CNT_DLY = (0x3F<<10),
	CBR_DW_DLY = (0x1F<<5),
	CBR_CNT_DLY = (0x1F),
};

#define LATE_BND_OFF		16
#define CHKSUM_CNT_DLY_OFF	10
#define CBR_DW_DLY_OFF		5
#define CBR_CNT_DLY_OFF		0

enum TxRxCR2Bits {
	PadDis = (1<<29),
	ChkLinkMode = (1<<25),
	ChkMagicMode = (1<<24),
	RxMaxLenEn = (1<<23),

	RxFifoTh16 = (0x0<<20),
	RxFifoTh32 = (0x1<<20),
	RxFifoTh64 = (0x2<<20),
	RxFifoTh128 = (0x3<<20),
	RxFifoTh256 = (0x4<<20),
	RxFifoTh512 = (0x5<<20),
	RxFifoTh1024 = (0x6<<20),
	RxFifoThPkt = (0x7<<20),
	RxFifoThMask = (0x7<<20),

	/* 8 bytes in unit */
	TxFifoThMask = (0x3f<<9),

	CrcDis = (1<<8),
	RxRemoveVlanTagEn = (1<<7),

	TxFlowControlEn = (1<<6),
	RxFlowControlEn = (1<<5),
	TxPauseFlag = (1<<4),
	RxPauseFlag = (1<<3),
	VlanEn = (1<<2),
	RxWatchdogRelease = (1<<1),
	RxWatchdogDis = 1,
};

#define RxFifoThOff	20
#define TxFifoThOff	9
#define TxRxConfig2	(0x3f<<TxFifoThOff)

enum RmiiCRBits {
	RgmiiEn = (1<<6),
	RmiiEn = (1<<5),
	RmiiCrSpeed = (1<<4),
	RmiiPhyDlyMask = (0xf),
};

enum MiiMR2Bits {
	MiiTransStart = (1<<31),
	MiiOpRead = (2<<26),
	MiiOpWrite = (1<<26),
	MiiOpMask = (0x3<<26),
	MiiPhyAddrMask = (0x1F<<21),
	MiiRegAddrMask = (0x1F<<16),
};

#define MiiPhyAddrOff	21
#define MiiRegAddrOff	16

enum IMBLatencyBits {
	MAC_IMB_LAT = (0xFF<<8),
	MAC_HI_PRTY_CNT = (0xFF),
};

#define MAC_IMB_LAT_OFF		8
#define MAC_HI_PRTY_CNT_OFF	0

enum TxDesIPStartOBits {
	RxMaxPktLenMask = (0xfff),
};

enum ALI_PHY_REGISTERS {
	PhyBasicModeCtrl = 0,
	PhyBasicModeStatus = 1,
	PhyNWayAdvert = 4,
	PhyNWayLPAR = 5,
	PhyNWayExpansion = 6,
};

enum BMCRBits {
	BMCRReset = (1<<15),
	BMCRLoopback = (1<<14),
	BMCRSpeedSet = (1<<13),
	BMCRSpeedSet13 = (1<<13),
	BMCRANEnable = (1<<12),
	BMCRPowerDown = (1<<11),
	BMCRRestartAN = (1<<9),
	BMCRDuplexMode = (1<<8),
	BMCRSpeedSet6 = (1<<6),
};

enum BMSRBits {
	BMSR100BT4 = (1<<15),
	BMSR100BTXFULL = (1<<14),
	BMSR100BTXHALF = (1<<13),
	BMSR10BTFULL = (1<<12),
	BMSR10BTHALF = (1<<11),
	BMSRANComplete = (1<<5),
	BMSRRemoteFault = (1<<4),
	BMSRAN = (1<<3),
	BMSRLinkStatus = (1<<2),
	BMSRJabberDectect = (1<<1),
	BMSREC = 1,
};

enum ANARBits {
	ANARNP = (1<<15),
	ANARACK = (1<<14),
	ANARRF = (1<<13),
	ANARASMDIR = (1<<11),
	ANARPause = (1<<10),
	ANART4 = (1<<9),
	ANARTXFD = (1<<8),
	ANARTX = (1<<7),
	ANAR10FD = (1<<6),
	ANAR10 = (1<<5),
};

#define ANAR_MEDIA	(ANAR10|ANAR10FD|ANARTX|ANARTXFD)

#endif /* _ALI_ETH_REG_ */

