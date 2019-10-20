#ifndef __ETHERNET_TOE2_UTIL__H
#define __ETHERNET_TOE2_UTIL__H

//#define TOE2_UTILITY_TRACE(msg,args...) pr_info("TOE2U: " msg "\n", ## args)
#define TOE2_UTILITY_TRACE(msg,args...)
#define UTIL_DEBUG
struct ali_mac_rx_io {
	int loopback_en;
	int pass_multicast;
	int pass_bad;
	int promiscuous;
	int filter_mode;
	int crc_verify;
	int chksum_verify;
};

struct ali_mac_xmit_io {
	unsigned char  tx_rx; 
	unsigned char  type;   //0: tcpv4, 1: udpv4, 2: tcpv6, 3: udpv6
	unsigned char  toe_tx; //0: disable, 1: enable
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
	unsigned int duplex;
	unsigned int speed;
	unsigned int autonego;
	unsigned int rmii_rgmii;
};

struct ali_mac_init_io {
	unsigned int duplex;
	unsigned int speed;
	unsigned int autonego;
	unsigned int rmii_rgmii;
};

typedef struct _cmd_param {
    u32 cmd; 
    u32 reg;
    u32 value;
    void * data;
} cmd_param_t;

#define TX_PKTS        		0x6
#define RX_PKTS        		0xd
#define MAC_INIT       		0xe
#define MAC_GET_STATUS      0xf
#define MAC_RECV_INT        0x10
#define MAC_CNT_CLEAN       0x11

typedef struct tagMAC_ADAPTER
{
	//unsigned int BaseAddr;
	void __iomem * BaseAddr;
	unsigned int Version;	
	unsigned int InterruptMask;

	unsigned char MacAddr[ETH_ALEN];

	unsigned char *pSetupBuf;
	dma_addr_t pSetupBuf_DMA;
	
	struct toe2_rx_desc *pRxDesc;
	dma_addr_t pRxDesc_DMA;
	unsigned int RxDescBuf[TOE2_RX_DESC_NUM];
	dma_addr_t RxDescBuf_DMA[TOE2_RX_DESC_NUM];
	
	struct toe2_tx_desc *pTxDesc;
	dma_addr_t pTxDesc_DMA;
	unsigned char * TxDescBuf;
	//dma_addr_t TxDescBuf_DMA[MAX_TOE2_PKT_LEN];

	//Rx Pkts buf ptr.
	unsigned int RxBuffer;

	//Counter.
	unsigned short RxDescWPtr, RxBufRPtr;
	unsigned short TxDescWPtr;
	
	//current ISR bits. 
	unsigned int CurrentISR;
	//MAC Settings.
	int Duplex, Speed, AutoNeg;
	int RmiiEn;
	int LoopBackEn;
	int HwMdioEn;
	int VlanSupport, VlanTagRemove;
	int PauseFrameRx, PauseFrameTx;    
	//Rx Settings.
	int ToeRxEn;
	int PassMulticast, PassBad, Promiscuous;
	int CrcVerify, ChksumVerify; 
	//Tx Settings.
	int ToeTxEn;
	int TsoEn, UfoEn;
	unsigned short MinMfl, MaxMfl;
	unsigned short MflAuto;
	int TxLenAutoInc;
	unsigned short TxLenAuto;
	unsigned short TxCrcErrs; //Tx Dbg Use.
	//status.
	struct net_device_stats net_stats;
	struct toe2_device_stats mac_stats;

	unsigned short LinkPartner;
	unsigned char FilteringMode;
	int link_status;
} MAC_ADAPTER, *PMAC_ADAPTER;

typedef struct tagMAC_Init_Context
{       
	//MAC Settings.
	int Duplex, Speed, AutoNeg;
	int RmiiEn;
	int HwMdioEn;
	int VlanSupport, VlanTagRemove;
	int PauseFrameRx, PauseFrameTx;
	//Rx Settings.
	int ToeRxEn;
} MAC_Init_Context, *PMAC_Init_Context;

typedef struct tagMAC_Rx_Context
{
	//MAC Settings.
	int LoopBackEn;
	//Rx Settings.
	int PassMulticast, PassBad, Promiscuous;
	int CrcVerify, ChksumVerify;
	unsigned char FilteringMode;
} MAC_Rx_Context, *PMAC_Rx_Context;

typedef struct
{
	unsigned char FrameType;
	int AddVlanTag;
	unsigned short TxVlanTag;
	unsigned int DescFrom;
	unsigned int DescTo;
	unsigned int DescLen;
	unsigned int MaxDescLen;
	unsigned int DescTimes;
} MAC_TX_PARA;

typedef struct tagMAC_Tx_Context
{
	//Tx Settings.
	int ToeTxEn;
	int TsoEn, UfoEn;
	unsigned short MinMfl, MaxMfl;
	unsigned short MflAuto;
	int TxLenAutoInc;
	unsigned short TxLenAuto;
	unsigned short TxCrcErrs; //Tx Dbg Use.

	MAC_TX_PARA TxPara;

	unsigned char MacDstAddr[6];
	unsigned char MacSrcAddr[6];
} MAC_Tx_Context, *PMAC_Tx_Context;

typedef struct tagMAC_Status_Context
{
	//status.
	struct net_device_stats net_stats;
	struct toe2_device_stats mac_stats;
	unsigned short TxLenAuto;
	unsigned short TxCrcErrs;
} MAC_Status_Context, *PMAC_Status_Context;

struct tagMAC_transfer_ifr
{
	int cmd_type;
	void *pointer;
};

int util_mac_rx_start(PMAC_Rx_Context);
void util_mac_rx_stop(PMAC_ADAPTER);
int util_mac_tx_start(PMAC_Tx_Context);
void util_mac_tx_stop(void);
int util_mac_tx_setup(void);
void util_mac_rx_thread_create(void);
void util_mac_rx_thread_destroy(void);
void util_mac_tx_thread_create(void);
void util_mac_tx_thread_destroy(void);
void util_mac_status(PMAC_ADAPTER pAdapter);
int test_mac_tx(struct toe2_private *ptoe2, struct ali_mac_xmit_io *xmit_info);
int util_mac_init(struct toe2_private *ptoe2, struct ali_mac_init_io *mac_init_info);
int util_mac_init_tx(struct toe2_private *ptoe2, struct ali_mac_xmit_io *xmit_info);
void test_mac_rx(struct toe2_private *ptoe2, struct ali_mac_rx_io *rx_info);
#endif
