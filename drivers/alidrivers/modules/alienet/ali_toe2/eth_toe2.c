/*
 * Ethernet driver for ALi SoCs
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com
 *
 * Authors: David.Shih <david.shih@alitech.com>,
 *		  Lucas.Lai  <lucas.lai@alitech.com>
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
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/dma.h>
#include <linux/uaccess.h>
#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/if_vlan.h>

#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/phy.h>

#include "eth_reg.h"
#include "eth_toe2.h"
#include "eth_toe2_util.h"

#define PHY_BLINK_USING_GPIO
#ifdef PHY_BLINK_USING_GPIO
#include <ali_board_config.h>
/*
 * static INT32 g_enet_link_gpio = -1;
 * static INT32 g_enet_speed_gpio = -1;
 * static INT32 g_enet_gpio_light;
 */
#endif

#define TOE2_DRV_NAME "ALi Ethernet TOE2"
#define TOE2_DRV_VER  "Ver 1.6"

/* chip id */
#define C3701	0x3701
#define C3821	0x3821
#define C3921	0x3921
#define C3505	0x3505

/* soc reg address */
#define SOC_REG0    0x0
#define SOC_REG640  0x640 
#define SOC_REG84   0x84

/* patch for 38/39 */
#define MAX_LINK_CHANGE_IN_5SEC 3
/*static u32 mac_clk = 50 * 1024 * 1024; 50M
static u32 mac_5sec;
static u32 mac_pll_time;
*/
#define MAC_CLK (50 * 1024 * 1024) /* 50M */

/* almost 200 timer interrupt per minute */
#define TIMER_200_FREQ	0xc7ffff
/* almost 2000 timer interrupt per minute */
#define TIMER_2000_FREQ   0x18FFFE

#define ETH_ALEN     6
u8 stb_mac_addr[ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 dft_mac_addr[ETH_ALEN] = {0x00, 0x90, 0xe6, 0x00, 0x00, 0x0a};

/* numbers of pkts in per HZ/2 */
#define TOO_MANY_RX_INT		  500
/* times of zero pkt ingress in timer interrupt */
#define TOO_MANY_REDUN_TIMER_INT 500

#define HIGH_INGRESS_IMK 1
#define LOW_INGRESS_IMK  0

#define JUDGE_TOO_MANY_RX_INT_PERIOD (HZ/2)

#define LOOP_TIMEOUT_TIMES  30
#define MDELAY_M_SECONDS	1

#define READ_MAC_REG   0x1
#define READ_SOC_REG   0x2
#define WRITE_MAC_REG  0x3
#define WRITE_SOC_REG  0x4
#define TX_PKTS		0x6
#define RX_PKTS		0xd
#define MAC_INIT	   0xe
#define MAC_GET_STATUS 0xf
#define MAC_RECV_INT		0x10
#define MAC_CNT_CLEAN	   0x11
static struct net_device_stats *toe2_get_stats(struct net_device *dev);

static void mac_tx_sts(struct toe2_private *ptoe2,
		struct toe2_tx_desc *desc_sw);

static void toe2_set_rx_mode(struct net_device *dev);

static int toe2_poweron_sequence(struct toe2_private *ptoe2);

static void phy_link_changed(struct toe2_private *ptoe2);
static void phy_link_restart(struct toe2_private *ptoe2);
static void phy_link_on(struct toe2_private *ptoe2);
static void phy_link_off(struct toe2_private *ptoe2);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 20, 0))
#define skb_vlan_tag_present(x) vlan_tx_tag_present(x)
#endif

/******************************************************************************/

void toe2_enable_irq(struct toe2_private *ptoe2)
{
	TOE2_W32(ptoe2->io_base, IMR, ptoe2->isr_mask);
	/* DYNAMIC_CHANGING_IMK, the actually changing happens here!! */
	TOE2_W32(ptoe2->io_base, TimerR, ptoe2->timer_freq);
}

static void toe2_disable_irq(struct toe2_private *ptoe2)
{
	TOE2_W32(ptoe2->io_base, IMR, 0);
}

static void dny_imk2low(struct toe2_private *ptoe2)
{
	ptoe2->isr_mask = TOE2_INTERRUPT_MASK;
	ptoe2->cur_dny_imk = LOW_INGRESS_IMK;
	ptoe2->timer_freq = TIMER_200_FREQ;
}

static int mac_mdio_read(struct net_device *dev, int phy_addr, int reg_addr)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	u16 tmp_16;
	 u32 tmp_32;
	 u32 addr;
	 u32 loop_times = 0;

	addr = ((phy_addr << MiiPhyAddrOff) & MiiPhyAddrMask)
		| ((reg_addr << MiiRegAddrOff) & MiiRegAddrMask);

	tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
	tmp_32 &= ~(MiiPhyAddrMask|MiiRegAddrMask|MiiOpMask);

	TOE2_W32(ptoe2->io_base,
			MiiMR2, (tmp_32|addr|MiiOpRead|MiiTransStart));

	do {
		mdelay(MDELAY_M_SECONDS);
		tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
		loop_times++;
		if (loop_times > LOOP_TIMEOUT_TIMES) {
			pr_err("TOE2 %s loop timeout\n", __func__);
			break;
		}
	} while (tmp_32&MiiTransStart);

	tmp_16 = TOE2_R16(ptoe2->io_base, MdioR);

	return (int)tmp_16;
}

static void mac_mdio_write(struct net_device *dev,
		int phy_addr, int reg_addr, int value)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	 u32 tmp_32;
	 u32 addr;
	 u32 loop_times = 0;

	TOE2_W16(ptoe2->io_base, MdioW, (u16)value);

	addr = ((phy_addr << MiiPhyAddrOff) & MiiPhyAddrMask)
		| ((reg_addr << MiiRegAddrOff) & MiiRegAddrMask);

	tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
	tmp_32 &= ~(MiiPhyAddrMask|MiiRegAddrMask|MiiOpMask);

	TOE2_W32(ptoe2->io_base,
			MiiMR2, (tmp_32|addr|MiiOpWrite|MiiTransStart));

	do {
		mdelay(MDELAY_M_SECONDS);
		tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
		loop_times++;
		if (loop_times > LOOP_TIMEOUT_TIMES) {
			pr_err("TOE2 %s loop timeout\n", __func__);
			break;
		}
	} while (tmp_32&MiiTransStart);
}

int toe2_mdio_read(struct mii_bus *bus, int phy_addr, int reg_addr)
{
	struct toe2_private *ptoe2 = NULL;
	struct net_device *ndev = NULL;
	u16 tmp_16 = 0;
	u32 tmp_32 = 0;
	u32 addr = 0;
	u32 loop_times = 0;

	ptoe2 = bus->priv;
	ndev = ptoe2->ndev;
	addr = ((phy_addr << MiiPhyAddrOff) & MiiPhyAddrMask)
		| ((reg_addr << MiiRegAddrOff) & MiiRegAddrMask);

	tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
	tmp_32 &= ~(MiiPhyAddrMask|MiiRegAddrMask|MiiOpMask);

	TOE2_W32(ptoe2->io_base,
			MiiMR2, (tmp_32|addr|MiiOpRead|MiiTransStart));

	do {
		msleep(MDELAY_M_SECONDS);
		tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
		loop_times++;
		if (loop_times > LOOP_TIMEOUT_TIMES) {
			netdev_err(ndev, "TOE2 %s loop timeout\n", __func__);
			return -ETIMEDOUT;
		}
	} while (tmp_32&MiiTransStart);

	tmp_16 = TOE2_R16(ptoe2->io_base, MdioR);
	return (int)tmp_16;
}

int toe2_mdio_write(struct mii_bus *bus, int phy_addr,
							int reg_addr, u16 value)
{
	struct toe2_private *ptoe2 = NULL;
	struct net_device *ndev = NULL;
	u32 tmp_32 = 0;
	u32 addr = 0;
	u32 loop_times = 0;

	ptoe2 = bus->priv;
	ndev = ptoe2->ndev;
	TOE2_W16(ptoe2->io_base, MdioW, (u16)value);

	addr = ((phy_addr << MiiPhyAddrOff) & MiiPhyAddrMask)
		| ((reg_addr << MiiRegAddrOff) & MiiRegAddrMask);

	tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
	tmp_32 &= ~(MiiPhyAddrMask|MiiRegAddrMask|MiiOpMask);

	TOE2_W32(ptoe2->io_base,
			MiiMR2, (tmp_32|addr|MiiOpWrite|MiiTransStart));

	do {
		msleep(MDELAY_M_SECONDS);
		tmp_32 = TOE2_R32(ptoe2->io_base, MiiMR2);
		loop_times++;
		if (loop_times > LOOP_TIMEOUT_TIMES) {
			netdev_err(ndev, "TOE2 %s loop timeout\n", __func__);
			return -ETIMEDOUT;
			pr_err("TOE2 %s loop timeout\n", __func__);
			break;
		}
	} while (tmp_32 & MiiTransStart);
	return 0;
}

static void do_dll_pd_reset(struct toe2_private *ptoe2)
{
	u32 tmp_val = 0;
	pr_err("WARNING!!! TOE2 DLL PD need reset.\n");
	tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
	tmp_val |= (1<<1);
	SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);

	mdelay(250);

	tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
	tmp_val &= ~(1<<1);
	SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
	pr_err("WARNING!!! TOE2 DLL PD reset completed.\n");
}

static void handle_unlink_error(struct toe2_private *ptoe2)
{
	u16 ctrl = 0;
	u16 status = 0;
	u16 reg10 = 0;
	u16 tmp = 0;

	switch (ptoe2->unlink_error_state) {
	case 0: /* state A */
		reg10 = (u16)mac_mdio_read(ptoe2->ndev,
				ptoe2->phy_addr, 0x10);
		ctrl = (u16)mac_mdio_read(ptoe2->ndev,
				ptoe2->phy_addr, PhyBasicModeCtrl);
		tmp = (1 << 7);
		/* check whether there is signal on line */
		if (!(reg10 & tmp)) {
			/* no signal on line */
			netdev_dbg(ptoe2->ndev,
					"[UNLINK] no signal on line\n");
			return;
		}
		/* check support auto nego and unlink */
		tmp = reg10;
		tmp = (tmp >> 12);
		if (!(ctrl & BMCRANEnable) || (tmp == 6)) {
			/* doesn't support auto nego or link */
			netdev_dbg(ptoe2->ndev,
					"[UNLINK] non auto nego or linked\n");
			return;
		}
		/* change state to parallel detection */
		ptoe2->unlink_error_state = 1;
		ptoe2->para_detect_times = 0;
		/* through */
		netdev_dbg(ptoe2->ndev,
				"[UNLINK] From A to Para_Detect\n");
	case 1:
		/* only when para_detect_times == 0 do parallel
		 * detect 41ms * 12 = 500ms */
		if (ptoe2->para_detect_times >= 12) {
			ptoe2->unlink_error_state = 0;
			netdev_dbg(ptoe2->ndev,
					"[UNLINK] Para_Detect over 500ms => A\n");
			return;
		}
		reg10 = (u16)mac_mdio_read(ptoe2->ndev,
				ptoe2->phy_addr, 0x10);
		/* check link partner support auto nego */
		tmp = reg10 & 0x0F00;
		tmp = (tmp >> 8);
		if (tmp == 3 || tmp == 5) {
			ptoe2->unlink_error_state = 0; /* change state to A */
			netdev_dbg(ptoe2->ndev,
					"[UNLINK] partner auto nego => A\n");
			return;
		}
		ptoe2->para_detect_times++;
		if (tmp != 4) {
			netdev_dbg(ptoe2->ndev,
					"[UNLINK] do Para Detect again\n");
			return;
		}
		/* change state to unlink and signal state */
		ptoe2->unlink_error_state = 2;
		ptoe2->unlink_with_signal_times = 0;
		netdev_dbg(ptoe2->ndev,
				"[UNLINK] Para Detect ok => no signale state\n");
		/* through */
	case 2:
		if (ptoe2->unlink_with_signal_times >= 48) {
			/* > 2s */
			do_dll_pd_reset(ptoe2);
			ptoe2->unlink_error_state = 0;
			return;
		}
		/* check whether there is signal on line */
		reg10 = (u16)mac_mdio_read(ptoe2->ndev,
				ptoe2->phy_addr, 0x10);
		status = (u16)mac_mdio_read(ptoe2->ndev,
				ptoe2->phy_addr, PhyBasicModeStatus);
		tmp = (1 << 7);
		if (!(reg10 & tmp) || (status & BMSRLinkStatus)) {
			/* no signal on line */
			netdev_dbg(ptoe2->ndev,
					"[UNLINK] no signal on line => A\n");
			ptoe2->unlink_error_state = 0;
			return;
		}
		ptoe2->unlink_with_signal_times++;
		break;
	default:
		netdev_dbg(ptoe2->ndev,
				"[UNLINK] something wrong error state is %d\n",
				ptoe2->unlink_error_state);
		ptoe2->unlink_error_state = 0;
		break;
	}
	return;
}

void toe2_adjust_link(struct net_device *ndev)
{
	struct toe2_private *ptoe2 = NULL;
	struct phy_device *phy_dev = NULL;
	int new_state = 0;
	ptoe2 = netdev_priv(ndev);
	phy_dev = ptoe2->phy_dev;

	if (!phy_dev) {
		return;
	}
	if (phy_dev->state == PHY_HALTED) {
		return;
	}

	if (!netif_running(ndev) || !netif_device_present(ndev)) {
		ptoe2->link = 0;
		return;
	}
	if (phy_dev->link) {
		/* linked */
		if (phy_dev->duplex != ptoe2->duplex_mode) {
			new_state = 1;
			ptoe2->duplex_mode = phy_dev->duplex;
		}
		if (phy_dev->speed != ptoe2->link_speed) {
			new_state = 1;
			ptoe2->link_speed =  phy_dev->speed;
		}
		if (phy_dev->pause != ptoe2->pause_frame_rx) {
			ptoe2->pause_frame_rx = phy_dev->pause;
			ptoe2->pause_frame_tx = phy_dev->pause;
		}
		if (!ptoe2->link) {
			ptoe2->link = phy_dev->link;
			phy_link_on(ptoe2);
			ptoe2->times_of_link_change++;
		} else if (new_state) {
			phy_link_restart(ptoe2);
		}
	} else if (ptoe2->link) {
		/* disconnected */
		new_state = 1;
		phy_link_off(ptoe2);
		ptoe2->times_of_link_change++;
	} else {
		if ((C3821 == ptoe2->chip_id)) {
			handle_unlink_error(ptoe2);
		}
	}
	if (new_state && netif_msg_link(ptoe2)) {
		phy_print_status(phy_dev);
	}

	return;
}

#ifdef PHY_BLINK_USING_GPIO
static void link_light_blink(struct toe2_private *ptoe2)
{
	if (!ptoe2->link)
		return;
	if (ptoe2->blink_light) {
		ptoe2->blink_light = 0;
		ptoe2->in_blink = 1;
		gpio_direction_output(g_enet_link_gpio, !g_enet_gpio_light);
		return;
	}
	if (ptoe2->in_blink) {
		ptoe2->in_blink = 0;
		gpio_direction_output(g_enet_link_gpio, g_enet_gpio_light);
	}

	if (((u32)100 == ptoe2->link_speed) && (g_enet_speed_gpio >= 0))
		gpio_direction_output(g_enet_speed_gpio, g_enet_gpio_light);

	return;
}
#endif

static void mac_chip_rst(struct toe2_private *ptoe2)
{
	u8 tmp_u8;
	u32 loop_times = 0;

	TOE2_W8(ptoe2->io_base, SCR, SCRReset|TOE2_R8(ptoe2->io_base, SCR));
	do {
		mdelay(MDELAY_M_SECONDS);
		tmp_u8 = TOE2_R8(ptoe2->io_base, SCR);
		loop_times++;
		if (loop_times > LOOP_TIMEOUT_TIMES) {
			pr_err("TOE2 %s loop timeout\n", __func__);
			break;
		}
	} while (tmp_u8 & SCRReset);
}

static void mac_cnt_init(struct toe2_private *ptoe2)
{
	ptoe2->cur_isr = 0;

	dny_imk2low(ptoe2);
	spin_lock_init(&ptoe2->lock);

	ptoe2->rx_wptr = TOE2_RX_DESC_NUM - 1;
	ptoe2->rx_bptr = 0;
	ptoe2->tx_wptr = 0;

	ptoe2->auto_n_completed = false;
	ptoe2->link = 0;
	ptoe2->transmit_okay = false;

	ptoe2->pause_frame_rx = false;
	ptoe2->pause_frame_tx = false;

	ptoe2->vlan_tag_remove = false;

	ptoe2->blink_light = 0;
	ptoe2->in_blink = 0;

	ptoe2->cur_dny_imk = LOW_INGRESS_IMK;

	ptoe2->num_rx_complete = 0;
	ptoe2->num_timer = 0;
}

static int mac_rx_refill(struct toe2_private *ptoe2)
{
	int i;
	struct sk_buff *skb = NULL;
	struct toe2_rx_desc *rx_desc = NULL;
	struct net_device *dev = ptoe2->ndev;

	for (i = 0; i < TOE2_RX_DESC_NUM; i++) {
		rx_desc = &ptoe2->rx_desc[i];
		if (NULL == ptoe2->rx_skb[i]) {
			skb = netdev_alloc_skb(dev, TOE2_BUF_SZ);
			if (!skb)
				goto refill_err_out;

			/* skb_reserve(skb, NET_IP_ALIGN); */
			rx_desc->pkt_buf_dma =
				dma_map_single(ptoe2->dev, skb->data,
						TOE2_BUF_SZ, DMA_FROM_DEVICE);
			ptoe2->rx_skb[i] = skb;
		}
	}
	return 0;

refill_err_out:
	netdev_err(ptoe2->ndev, "%s()=>fatal error, alloc skb failed.\n",
			__func__);
	return -ENOMEM;
}

static int mac_alloc_rings(struct toe2_private *ptoe2)
{
	void *rx_desc = NULL;
	void *tx_desc = NULL;
	void *setup_buf = NULL;

	rx_desc = dma_alloc_coherent(ptoe2->dev,
			(TOE2_DESC_SZ*TOE2_RX_DESC_NUM),
			&ptoe2->rx_desc_dma, GFP_KERNEL);
	if (!rx_desc)
		goto err_out_fail;
	memset(rx_desc, 0, (TOE2_DESC_SZ * TOE2_RX_DESC_NUM));
	ptoe2->rx_desc = (struct toe2_rx_desc *)rx_desc;

	tx_desc = dma_alloc_coherent(ptoe2->dev,
			(TOE2_DESC_SZ*TOE2_TX_DESC_NUM),
			&ptoe2->tx_desc_dma, GFP_KERNEL);
	if (!tx_desc)
		goto err_out_fail;
	memset(tx_desc, 0, (TOE2_DESC_SZ * TOE2_TX_DESC_NUM));
	ptoe2->tx_desc = (struct toe2_tx_desc *)tx_desc;

	setup_buf = dma_alloc_coherent(ptoe2->dev,
			SETUP_FRAME_SZ, &ptoe2->setup_buf_dma, GFP_KERNEL);
	if (!setup_buf)
		goto err_out_fail;

	memset(setup_buf, 0, SETUP_FRAME_SZ);
	ptoe2->setup_buf = (u8 *)setup_buf;

	return 0;

err_out_fail:
	return -ENOMEM;
}

static void mac_desc_clean(struct toe2_private *ptoe2)
{
	unsigned i;
	struct toe2_rx_desc *rx_desc;
	struct toe2_tx_desc *tx_desc;

	for (i = 0; i < TOE2_RX_DESC_NUM; i++) {
		if (ptoe2->rx_skb[i]) {
			rx_desc = &ptoe2->rx_desc[i];
			dma_unmap_single(ptoe2->dev,
					rx_desc->pkt_buf_dma,
					TOE2_BUF_SZ, DMA_FROM_DEVICE);
			dev_kfree_skb_any(ptoe2->rx_skb[i]);
			ptoe2->rx_skb[i] = NULL;
		}
	}

	while (ptoe2->tx_skb_rd != ptoe2->tx_skb_wr) {
		if (ptoe2->as_tx_skb[ptoe2->tx_skb_rd].skb) {
			dev_kfree_skb_any(
					ptoe2->as_tx_skb[ptoe2->tx_skb_rd].skb);
			ptoe2->as_tx_skb[ptoe2->tx_skb_rd].first = 0;
			ptoe2->as_tx_skb[ptoe2->tx_skb_rd].cnt = 0;
			ptoe2->as_tx_skb[ptoe2->tx_skb_rd].skb = NULL;
		}
		if (ptoe2->tx_skb_rd+1 == TOE2_TX_DESC_NUM)
			ptoe2->tx_skb_rd = 0;
		else
			ptoe2->tx_skb_rd++;
	}
	ptoe2->tx_skb_rd = 0;
	ptoe2->tx_skb_wr = 0;

	for (i = 0; i < TOE2_TX_DESC_NUM; i++) {
		tx_desc = &ptoe2->tx_desc[i];
		if (tx_desc->seg_len) {
			dma_unmap_single(ptoe2->dev,
					tx_desc->pkt_buf_dma,
					tx_desc->seg_len, DMA_TO_DEVICE);
			tx_desc->seg_len = 0;
		}
	}
}

static void mac_hw_start(struct toe2_private *ptoe2)
{
	u8 tmp_8;
	 u32 tmp_u32;
	 u32 duplex_mode = 0;
	 u32 pause_frame = 0;
	 u32 rmii_speed = 0;
	int re = 0;

	netdev_dbg(ptoe2->ndev, "%s %d enter.\n", __func__, __LINE__);

	if (netif_msg_ifup(ptoe2))
		netdev_dbg(ptoe2->ndev, "mac_hardware_start()...\n");

	TOE2_W8(ptoe2->io_base, ClkDelayChainSR, 0XE);
	/* TOE2_W32(ptoe2->io_base, BackPressure, 0X784c5); */

	/* set mac address. */
	TOE2_W32(ptoe2->io_base, PAR, ptoe2->toe2_mac_lo32);
	TOE2_W32(ptoe2->io_base, PAR - 4, ptoe2->toe2_mac_hi16);

	if (ptoe2->link_speed == (u32)100)
		rmii_speed = (u32)RmiiCrSpeed;  /* 100Mbps */
	else
		rmii_speed = (u32)0;  /* 10Mbps */

	/* Set RMII. */
	tmp_u32 = TOE2_R32(ptoe2->io_base, RmiiCR);
	tmp_u32 &= ~(RmiiCrSpeed);
	if (ptoe2->phy_interface == PHY_INTERFACE_MODE_RMII)
		TOE2_W32(ptoe2->io_base, RmiiCR, (tmp_u32|rmii_speed|RmiiEn));
	else
		TOE2_W32(ptoe2->io_base, RmiiCR, (tmp_u32|rmii_speed|RgmiiEn));
	tmp_u32 = TOE2_R32(ptoe2->io_base, RmiiCR);

	if (ptoe2->duplex_mode)
		duplex_mode = (u32)FullDuplexMode;
	else
		duplex_mode = (u32)0;

	if (true == ptoe2->toe2_rx_csum)
		duplex_mode |= RxTOEWorkMode;

	/* config network operation mode. */
	TOE2_W32(ptoe2->io_base, NetworkOM, (duplex_mode|NetworkOMConfig));

	/* test mux */
	TOE2_W8(ptoe2->io_base, 0x58, 0x0F);

	if (ptoe2->pause_frame_rx)
		pause_frame |= (u32)RxFlowControlEn;
	if (ptoe2->pause_frame_tx)
		pause_frame |= (u32)TxFlowControlEn;

	/* 1. Increase IPG time to ensure the gap
	 * between 2 packets > mini IPG time
	 * 2. decrease the first portion of the interframe gap time
	 */
	TOE2_W8(ptoe2->io_base, TxRxCR1, 0xdf);
	TOE2_W8(ptoe2->io_base, TxRxCR1+1, 0x00);

	/* VLAN Patch 1/3 For receiving packet length 1517 & 1518,
	 * we need to add a patch here.
	 * tmp_u32 = TOE2_R32(ptoe2->io_base, RxChkSumStartOff);
	 * tmp_u32 &= ~(0xfff);
	 * tmp_u32 |= 0x5f7;
	 * TOE2_W32(ptoe2->io_base, RxChkSumStartOff, tmp_u32);
	 */

	tmp_u32 = TOE2_R32(ptoe2->io_base, TxRxCR2);

	/* VLAN Patch 2/3 For receiving packet length 1517 & 1518,
	 * we need to add a patch here.
	 * tmp_u32 |= RxMaxLenEn;
	 */

	tmp_u32 &= 0xf7ffffff;
	tmp_u32 |= 0x04000000;
	tmp_u32 &= ~TxFifoThMask;
	tmp_u32 |= (0x2<<TxFifoThOff);
	tmp_u32 &= ~RxFlowControlEn;
	tmp_u32 &= ~TxFlowControlEn;

	if (ptoe2->vlan_tag_remove)
		TOE2_W32(ptoe2->io_base, TxRxCR2, (pause_frame|tmp_u32
					|TxRxConfig2|RxRemoveVlanTagEn|VlanEn));
	else
		TOE2_W32(ptoe2->io_base, TxRxCR2,
				(pause_frame|tmp_u32|TxRxConfig2|VlanEn));

	TOE2_W32(ptoe2->io_base, TSAD, ptoe2->tx_desc_dma);
	TOE2_W32(ptoe2->io_base, RSAD, ptoe2->rx_desc_dma);

	TOE2_W16(ptoe2->io_base, RxDesTotNum, TOE2_RX_DESC_NUM);
	TOE2_W16(ptoe2->io_base, TxDesTotNum, TOE2_TX_DESC_NUM);

	TOE2_W16(ptoe2->io_base, RxRingDesWPtr, TOE2_RX_DESC_NUM - 1);
	TOE2_W16(ptoe2->io_base, TxRingDesWPtr, 0);

	if (ptoe2->timer_freq != TIMER_200_FREQ)
		netdev_dbg(ptoe2->ndev, "weird timer!\n");
	TOE2_W32(ptoe2->io_base, TimerR, ptoe2->timer_freq);

	toe2_set_rx_mode(ptoe2->ndev);
	re = mac_rx_refill(ptoe2);
	if (re) {
		/*asm("sdbbp");*/
		netdev_err(ptoe2->ndev, "sdbbp\n");
	}

	tmp_8 = (SCRRxEn|SCRTxEn);
	if (ptoe2->toe2_tso)
		tmp_8 |= (SCRTxCoeEn|SCRTsoEn);
	if (ptoe2->toe2_ufo)
		tmp_8 |= (SCRTxCoeEn|SCRUfoEn);
	if (ptoe2->toe2_tx_csum)
		tmp_8 |= SCRTxCoeEn;

	if (ptoe2->toe2_rx_csum)
		tmp_8 |= SCRRxCoeEn;
	TOE2_W8(ptoe2->io_base, SCR, tmp_8);

	/* Enable all surported interrupts. */
	TOE2_W32(ptoe2->io_base, IMR, ptoe2->isr_mask);
}

static void mac_free_rings(struct toe2_private *ptoe2)
{
	mac_desc_clean(ptoe2);

	if (ptoe2 != NULL) {
		if (ptoe2->rx_desc != NULL) {
			dma_free_coherent(ptoe2->dev,
					(TOE2_DESC_SZ*TOE2_RX_DESC_NUM),
					ptoe2->rx_desc, ptoe2->rx_desc_dma);
			ptoe2->rx_desc = NULL;
		}

		if (ptoe2->tx_desc != NULL) {
			dma_free_coherent(ptoe2->dev,
					(TOE2_DESC_SZ*TOE2_TX_DESC_NUM),
					ptoe2->tx_desc, ptoe2->tx_desc_dma);
			ptoe2->tx_desc = NULL;
		}

		if (ptoe2->setup_buf != NULL) {
			dma_free_coherent(ptoe2->dev, SETUP_FRAME_SZ,
					ptoe2->setup_buf,
					ptoe2->setup_buf_dma);
			ptoe2->setup_buf = NULL;
		}
	}
}


static void phy_link_restart(struct toe2_private *ptoe2)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&ptoe2->lock, flags);
	TOE2_W16(ptoe2->io_base, IMR, 0);
	phy_link_changed(ptoe2);
	spin_unlock_irqrestore(&ptoe2->lock, flags);
}

static void phy_link_changed(struct toe2_private *ptoe2)
{
	if (!ptoe2->link) {
		netdev_dbg(ptoe2->ndev, "%s shouln't be false!\n",
				__func__);
		return;
	}

	//if (C3821 == ptoe2->chip_id) {
#ifdef PHY_BLINK_USING_GPIO
	if ((u32)100 == ptoe2->link_speed) {
		/* TOE2_TRACE("100M, light speed led\n"); */
		if (g_enet_speed_gpio >= 0)
			gpio_direction_output(g_enet_speed_gpio, g_enet_gpio_light);
	} else {
		/* TOE2_TRACE("non 100M, disable speed led\n"); */
		if (g_enet_speed_gpio >= 0)
			gpio_direction_output(g_enet_speed_gpio, !g_enet_gpio_light);
	}
#endif
//	}

	mac_hw_start(ptoe2);
	ptoe2->transmit_okay = true;
}

static void mac_init_for_link_established(struct toe2_private *ptoe2)
{
	mac_desc_clean(ptoe2);
	mac_chip_rst(ptoe2);

	dny_imk2low(ptoe2);

	ptoe2->cur_isr = 0;

	ptoe2->rx_wptr = TOE2_RX_DESC_NUM - 1;
	ptoe2->rx_bptr = 0;
	ptoe2->tx_wptr = 0;

	ptoe2->pause_frame_rx = false;
	ptoe2->pause_frame_tx = false;

	ptoe2->vlan_tag_remove = false;
}

static void phy_link_on(struct toe2_private *ptoe2)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&ptoe2->lock, flags);
	TOE2_W16(ptoe2->io_base, IMR, 0);
	mac_init_for_link_established(ptoe2);
	TOE2_W16(ptoe2->io_base, IMR, (ISRTimer|ISRLinkStatus));
	phy_link_changed(ptoe2);
	ptoe2->unlink_error_state = 0;
	spin_unlock_irqrestore(&ptoe2->lock, flags);

//	if (C3821 == ptoe2->chip_id) {
#ifdef PHY_BLINK_USING_GPIO
	if (g_enet_link_gpio >= 0) {
		gpio_direction_output(g_enet_link_gpio, g_enet_gpio_light);
	}
#endif
//	}
}

static void phy_link_off(struct toe2_private *ptoe2)
{
	unsigned long flags = 0;
	spin_lock_irqsave(&ptoe2->lock, flags);
	TOE2_W16(ptoe2->io_base, IMR, 0);
	netif_carrier_off(ptoe2->ndev);
	ptoe2->link = 0;
	ptoe2->duplex_mode = 0;
	ptoe2->link_speed = -1;
	ptoe2->link_partner = 0;
	ptoe2->link = 0;
	ptoe2->transmit_okay = false;
	spin_unlock_irqrestore(&ptoe2->lock, flags);

	//if (C3821 == ptoe2->chip_id) {
#ifdef PHY_BLINK_USING_GPIO
		ptoe2->blink_light = 0;
		ptoe2->in_blink = 0;
		if (g_enet_link_gpio >= 0)
			gpio_direction_output(g_enet_link_gpio, !g_enet_gpio_light);
		if (g_enet_speed_gpio >= 0)
			gpio_direction_output(g_enet_speed_gpio, !g_enet_gpio_light);
#endif
	//}
}

#if 0
int mac_build_setup(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	struct dev_mc_list *mc_list = dev->mc_list;
	u8 *pa, *buf_base, *addr;
	u32 crc;
	u16  bit, byte, hashcode;
	int i, j, rv = 0;

	memset(ptoe2->setup_buf, 0x00, SETUP_FRAME_SZ);

	if (toe2_rx_filter == TOE2_RX_FILTER_HASH) {
		pa = ptoe2->setup_buf + IMPERFECT_PA_OFF;
		/* fill our mac addr in setup frame buffer(offset from 156 to 167) */
		for (i = 0; i < ETH_ALEN; i++) {
			pa[i&0x01] = dev->dev_addr[i]; /* host mac addr. */
			if (i&0x01)
				pa += 4;
		}
		/* offset is 168 now. */

		pa[(TOE2_HASH_TABLE_LEN >> 3) - 3] = 0x80;

		buf_base = ptoe2->setup_buf;
		for (i = 0, mc_list = dev->mc_list;
				    mc_list && i < dev->mc_count; i++) {
			addr = &mc_list->da_addr[0];
			crc = ether_crc(ETH_ALEN, addr);

			hashcode = (u16)crc & TOE2_HASH_BITS;

			byte = hashcode >> 3;
			bit = 1 << (hashcode & 0x07);

			byte <<= 1;
			if (byte & 0x02)
				byte -= 1;

			buf_base[byte] |= bit;
			mc_list = mc_list->next;
		}
	} else if (toe2_rx_filter == TOE2_RX_FILTER_PERFECT) {
		pa = ptoe2->setup_buf + 0;
		for (i = 0; i < ETH_ALEN; i++) {
			pa[i&0x01] = dev->dev_addr[i];  /* host mac addr. */
			if (i&0x01)
				pa += 4;
		}

		for (i = 0; i < ETH_ALEN; i++) {
			pa[i&0x01] = 0xFF;				/* bc addr. */
			if (i & 0x01)
				pa += 4;
		}
		/* offset is 24 now. know why 14 now? */

		for (i = 0, mc_list = dev->mc_list;
					mc_list && i < dev->mc_count; i++) {
			for (j = 0; j < ETH_ALEN; j++) {
				pa[j&0x01] = mc_list->da_addr[j];
				if (j&0x01)
					pa += 4;
			}
			mc_list = mc_list->next;
		}
	} else {
		TOE2_WARNING("%s()=>toe2_rx_filter(%ld) not supported yet.",
				__func__, toe2_rx_filter);
		rv = -1;
	}
	return rv;
}

int mac_set_mc_filter(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	u32 ba = ptoe2->io_base;
	u8 cmd_u8;
	u32 cmd_u32;

	cmd_u8 = TOE2_R8(SCR);
	TOE2_W8(SCR, cmd_u8 & ~(SCRRxEn|SCRTxEn));

	cmd_u32 = TOE2_R32(NetworkOM);
	cmd_u32 &= ~PassMask;
	TOE2_W32(NetworkOM, cmd_u32);

	return mac_build_setup(dev);
}

#endif

static u16 mac_rx_update_wptr(struct toe2_private *ptoe2)
{
	struct net_device *dev = ptoe2->ndev;
	u16 rx_bptr, rx_rptr, rx_wptr;
	u16 updata = 0;
	struct toe2_rx_desc *rx_desc;
	struct sk_buff *new_skb;
	int i;

	rx_wptr = ptoe2->rx_wptr;
	rx_bptr = ptoe2->rx_bptr;
	rx_rptr = TOE2_R16(ptoe2->io_base, RxRingDesRPtr);

	if (rx_wptr > rx_rptr) {
		if ((rx_bptr > rx_rptr) && (rx_bptr <= rx_wptr))
			goto rx_lost;
		else {
			if (rx_bptr > rx_wptr)
				updata = rx_bptr - rx_wptr - 1;
			else
				updata = TOE2_RX_DESC_NUM + rx_bptr -
					rx_wptr - 1;
		}
	} else if (rx_wptr < rx_rptr) {
		if ((rx_bptr > rx_rptr) || (rx_bptr <= rx_wptr))
			goto rx_lost;
		else
			updata = rx_bptr - rx_wptr - 1;
	} else {
		if (rx_bptr > rx_wptr)
			updata = rx_bptr - rx_wptr - 1;
		else if (rx_bptr < rx_wptr)
			updata = TOE2_RX_DESC_NUM + rx_bptr - rx_wptr - 1;
		else
			goto rx_lost;
	}

	if (updata > 0) {
		i = rx_wptr;
		while (updata > 0) {
			if (ptoe2->rx_skb[i])
				new_skb = ptoe2->rx_skb[i];
			else
				new_skb = netdev_alloc_skb(dev, TOE2_BUF_SZ);
			if (!new_skb) {
				dev->stats.rx_dropped++;
				break;
			}

			ptoe2->rx_skb[i] = new_skb;

			rx_desc = &ptoe2->rx_desc[i];
			dma_unmap_single(ptoe2->dev,
					rx_desc->pkt_buf_dma,
					TOE2_BUF_SZ, DMA_FROM_DEVICE);
			rx_desc->pkt_buf_dma =
				dma_map_single(ptoe2->dev,
					new_skb->data, TOE2_BUF_SZ,
					DMA_FROM_DEVICE);

			if (i == TOE2_RX_DESC_NUM - 1) {
				rx_desc->EOR = 1;
				i = 0;
			} else {
				rx_desc->EOR = 0;
				i++;
			}
			updata--;
		}

		ptoe2->rx_wptr = i;
		TOE2_W16(ptoe2->io_base, RxRingDesWPtr, ptoe2->rx_wptr);
	}

	return rx_rptr;

rx_lost:
	netdev_dbg(ptoe2->ndev, "%s()=>rx_bptr got lost.\n", __func__);
	netdev_dbg(ptoe2->ndev, "rx_wptr:%d, rx_bptr:%d, rx_rptr:%d.\n",
			rx_wptr, rx_bptr, rx_rptr);
	return rx_rptr;
}

static bool mac_rx_hdr_chk(struct net_device *dev, struct packet_head *pHead)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	int fatal_err = 0;
	u32 temp_u32 = 0;

	if (pHead->ES) {
		if (pHead->WatchdogTimeout) {
			fatal_err++;
			ptoe2->mac_stats.rx_wd_timeout_errors++;
		}
		if (pHead->PhysicalLayerError) {
			fatal_err++;
			ptoe2->mac_stats.rx_phy_layer_errors++;
		}
		if (pHead->LateCollision) {
			fatal_err++;
			ptoe2->mac_stats.rx_late_col_seen++;
		}
		if (pHead->Long) {
			fatal_err++;
			ptoe2->mac_stats.rx_long_errors++;
		}
		if (pHead->Runt) {
			fatal_err++;
			ptoe2->mac_stats.rx_runt_errors++;
		}
		if (pHead->Dribble) {
			fatal_err++;
			ptoe2->mac_stats.rx_dribble_errors++;
		}

		if ((pHead->FifoOverflow) && (0 == fatal_err))
			return true;
		dev->stats.rx_errors++;
		return false;

	} else {
		if (pHead->PacketLength > 1536)
			return false;

		if ((pHead->VLAN) && (ptoe2->vlan_tag_remove))
			temp_u32 = 60;
		else
			temp_u32 = 64;

		if (pHead->PacketLength < temp_u32)
			return false;

		/* VLAN Patch 3/3 For receiving packet length 1517 & 1518,
		 * we need to add a patch here. */
		if ((pHead->VLAN) && (!ptoe2->vlan_tag_remove))
			temp_u32 = 1522;
		else
			temp_u32 = 1518;
		if (pHead->PacketLength > temp_u32) {
			dev->stats.rx_length_errors++;
			ptoe2->mac_stats.rx_long_errors++;
			return false;
		}

		dev->stats.rx_packets++;
		dev->stats.rx_bytes += pHead->PacketLength;

		if (pHead->BF)
			ptoe2->mac_stats.rx_bc++;
		if (pHead->PF)
			ptoe2->mac_stats.rx_uc++;
		if (pHead->MF)
			ptoe2->mac_stats.rx_mc++;
		if (pHead->PPPoE)
			ptoe2->mac_stats.rx_pppoe++;
		if (pHead->VLAN)
			ptoe2->mac_stats.rx_vlan++;
		if (pHead->IPFrame)
			ptoe2->mac_stats.rx_ip++;
		if (pHead->IPFrag)
			ptoe2->mac_stats.rx_frag++;
		return true;
	}
}

/* analyze & recode rx status while head is okay. */
static bool mac_rx_chs_ok(struct toe2_private *ptoe2, struct packet_head *pHead)
{
	if ((true == ptoe2->toe2_rx_csum) && (pHead->IPFrame)) {
		if (pHead->IPFrag)
			goto Done;

		if (!pHead->IPChksum) {
			ptoe2->mac_stats.rx_ip_chksum_errors++;
			netdev_dbg(ptoe2->ndev,
					"mac_rx_chksum_ok=>ip checksum err\n");
			goto Done;
		}

		if (!pHead->TCPChksum) {
			ptoe2->mac_stats.rx_tcp_chksum_errors++;
			netdev_dbg(ptoe2->ndev,
					"mac_rx_chksum_ok=>tcp checksum err\n");
			goto Done;
		}

		return true;
	}

Done:
	return false;
}

static int mac_rx_pkts(struct toe2_private *ptoe2, int budget)
{
	struct toe2_rx_desc *rx_desc;
	u16 rx_bptr, rx_rptr;
	struct packet_head *pHead;
	u16 pkt_sz, pkts, rx, i;
	struct sk_buff *skb;

	if (!netif_running(ptoe2->ndev)) {
		return 0;
	}

	rx_rptr = mac_rx_update_wptr(ptoe2);
	rx_bptr = ptoe2->rx_bptr;

	if (rx_rptr >= rx_bptr)
		pkts = rx_rptr - rx_bptr;
	else
		pkts = TOE2_RX_DESC_NUM + rx_rptr - rx_bptr;

	if ((pkts > 0) && (rx_rptr == ptoe2->rx_wptr))
		pkts -= 1;

	if ((budget != 0) && (pkts > budget))
		pkts = budget;

	i = rx_bptr;
	rx = 0;
	while (pkts > 0) {
		rx_desc = &ptoe2->rx_desc[i];
		pHead = &(rx_desc->pkt_hdr);

		if (mac_rx_hdr_chk(ptoe2->ndev, pHead)) {
			pkt_sz = pHead->PacketLength - RING_CRC_SZ;
			skb = ptoe2->rx_skb[i];
			skb_put(skb, pkt_sz);

#ifdef SW_CRCCHK
			u32 crc_sw, crc_hw;

			crc_sw = *(u32 *)(skb->data + skb->len);
			crc_sw = ~crc_sw;
			crc_hw = ether_crc_le(skb->len, skb->data);
			if (crc_sw != crc_hw) {
				netdev_dbg(ptoe2->ndev,
						"CRC ERR:sw = 0x%08x, hw = 0x%08x.\n",
						crc_sw, crc_hw);
				/* TOE2_W8(0x5b, (TOE2_R8(0x5b)|0x80)); */
			}
#endif

			if (mac_rx_chs_ok(ptoe2, pHead)) {
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			} else {
				skb->ip_summed = CHECKSUM_NONE;
			}

			skb->protocol = eth_type_trans(skb, ptoe2->ndev);
			skb->dev = ptoe2->ndev;
			netif_receive_skb(skb);

			//if (C3821 == ptoe2->chip_id) {
#ifdef PHY_BLINK_USING_GPIO
				if (!ptoe2->blink_light && !ptoe2->in_blink) {
					ptoe2->blink_light = 1;
				}
#endif
			//}
		} else {
			netdev_dbg(ptoe2->ndev,
					"rx_pkts(head error): Head(%08x).\n",
					*(u32 *)pHead);
			dev_kfree_skb_any(ptoe2->rx_skb[i]);
			netdev_dbg(ptoe2->ndev,
					"rx head err to free rx skb[%d]\n", i);
		}

		/* free rx skb. */
		ptoe2->rx_skb[i] = NULL;

		if (i == TOE2_RX_DESC_NUM - 1)
			i = 0;
		else
			i++;

		pkts--;
		rx++;
	}

	ptoe2->rx_bptr = i;

	return rx;
}

static int mac_rx_poll(struct napi_struct *napi, int budget)
{
	struct toe2_private *ptoe2 = container_of(napi,
			struct toe2_private, napi);
	int re;

	re = mac_rx_pkts(ptoe2, budget);

	if (re < budget) {
		napi_complete(napi);
		toe2_enable_irq(ptoe2);
		netdev_dbg(ptoe2->ndev,
				"%s: re<budget, re=%d, budget=%d\n", __func__,
				re, budget);
	}
	return re;
}

static void free_tx_skb(struct toe2_private *ptoe2, bool ulock)
{
	u16 first, desc_num;
	struct toe2_tx_desc *desc;
	u16 tx_rptr;
	unsigned long flags = 0;

	if (ulock)
		spin_lock_irqsave(&ptoe2->lock, flags);

	tx_rptr = TOE2_R16(ptoe2->io_base, TxRingDesRPtr);
	while (ptoe2->tx_skb_wr != ptoe2->tx_skb_rd) {
		first = ptoe2->as_tx_skb[ptoe2->tx_skb_rd].first;
		desc_num = ptoe2->as_tx_skb[ptoe2->tx_skb_rd].cnt;

		if ((first < tx_rptr && tx_rptr-first >= desc_num) ||
			(first > tx_rptr && ptoe2->tx_wptr >= tx_rptr &&
			 first > ptoe2->tx_wptr &&
			 TOE2_TX_DESC_NUM-(first-tx_rptr) >= desc_num)) {
			;
		} else {
			if (ulock)
				spin_unlock_irqrestore(&ptoe2->lock, flags);
			return;
		}

		desc = &ptoe2->tx_desc[first];
		mac_tx_sts(ptoe2, desc);

		if (ptoe2->as_tx_skb[ptoe2->tx_skb_rd].skb) {
			dev_kfree_skb_any(
					ptoe2->as_tx_skb[ptoe2->tx_skb_rd].skb);
			ptoe2->as_tx_skb[ptoe2->tx_skb_rd].skb = NULL;
			ptoe2->as_tx_skb[ptoe2->tx_skb_rd].first = 0;
			ptoe2->as_tx_skb[ptoe2->tx_skb_rd].cnt = 0;

			do {
				if (desc->seg_len) {
					dma_unmap_single(ptoe2->dev,
							desc->pkt_buf_dma,
							desc->seg_len,
							DMA_TO_DEVICE);
					desc->seg_len = 0;
				} else {
					/* asm("sdbbp"); */
					netdev_err(ptoe2->ndev, "sdbbp\n");
				}

				if ((++first) >= TOE2_TX_DESC_NUM)
					first = 0;
				desc = &ptoe2->tx_desc[first];
				desc_num--;

			} while (desc_num > 0);
		} else {
			/* asm("sdbbp"); */
			netdev_err(ptoe2->ndev, "sdbbp\n");
		}

		if (ptoe2->tx_skb_rd+1 == TOE2_TX_DESC_NUM)
			ptoe2->tx_skb_rd = 0;
		else
			ptoe2->tx_skb_rd++;
	}
	if (ulock)
		spin_unlock_irqrestore(&ptoe2->lock, flags);
}

static void mac_tx_sts(struct toe2_private *ptoe2, struct toe2_tx_desc *desc_sw)
{
	if ((desc_sw->tx_sts.sw.FS) && !(desc_sw->tx_sts.sw.OWN)) {
		if (!(desc_sw->tx_sts.sw.ES))
			ptoe2->ndev->stats.tx_packets++;
		else {
			ptoe2->ndev->stats.tx_errors++;
			if ((desc_sw->tx_sts.sw.LossOfCarrier)
					|| (desc_sw->tx_sts.sw.NoCarrier)) {
				ptoe2->ndev->stats.tx_carrier_errors++;
			}
			if (desc_sw->tx_sts.sw.LateCol)
				ptoe2->ndev->stats.tx_window_errors++;
			if (desc_sw->tx_sts.sw.FifoUnderrun)
				ptoe2->ndev->stats.tx_fifo_errors++;
			if (desc_sw->tx_sts.sw.HF)
				ptoe2->ndev->stats.tx_heartbeat_errors++;
		}

		if (desc_sw->tx_sts.sw.ExCol)
			ptoe2->mac_stats.tx_col_errors++;
		else
			ptoe2->mac_stats.tx_col_cnts[
				desc_sw->tx_sts.sw.ColCnt]++;
	}
}

static void mac_tx_cfg(struct toe2_private *ptoe2, u16 off,
		struct sk_buff *skb, void *seg_addr, u16 seg_len)
{
	struct toe2_tx_desc *desc = &ptoe2->tx_desc[off];

	memset(desc, 0, TOE2_DESC_SZ);
	desc->pkt_buf_dma = dma_map_single(
				(struct device *)ptoe2->dev,
				seg_addr, seg_len, DMA_TO_DEVICE);

	if (off == (TOE2_TX_DESC_NUM - 1))
		desc->tx_sts.hw.EOR = 1;

	desc->seg_len = seg_len;

	if (skb) {
		if (skb->ip_summed == CHECKSUM_PARTIAL) {
			if (ptoe2->toe2_tx_csum) {
				const struct iphdr *ip = ip_hdr(skb);

				desc->tx_sts.hw.IpStartOff =
					(u8)((u8 *)ip - skb->data);

				desc->tx_sts.hw.CoeEn = 1;
				if (desc->tx_sts.hw.IpStartOff > 0) {
					if (ip->protocol == IPPROTO_TCP)
						desc->tx_sts.hw.TcpPkt = 1;
					else if (ip->protocol == IPPROTO_UDP)
						desc->tx_sts.hw.UdpPkt = 1;
					else
						netdev_dbg(ptoe2->ndev,
							"PktTypeErr\n");
				} else
					netdev_dbg(ptoe2->ndev,
							"IpStartOff<=0\n");
			} else
				netdev_dbg(ptoe2->ndev, "TxCks not enabled\n");
		}

		if (ptoe2->vlgrp && skb_vlan_tag_present(skb)) {
			desc->vlan_tag = swab16(skb_vlan_tag_present(skb));
			desc->tx_sts.hw.VlanEn = 1;
		} else {
			desc->tx_sts.hw.VlanEn = 0;
		}
	}
}

static void c3505_phy_set(struct toe2_private *ptoe2)
{
	struct net_device *dev = ptoe2->ndev;
	u16 tmp_u16 = 0;
	int phy_reg = 0;

	pr_info("%s %d:\n", __func__, __LINE__);

	/*Step1: set FSM to disable state, this process will shut down
	 * all related PHY's DSP part.
	 */
	/*Reset main FSM to IDLE State*/
	phy_reg = 0x10;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<0);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step2: configure the autoneg ability, that we can't support next page
	 * exchange.
	 *For Mass product, do not modify this register*/
	phy_reg = 0x04;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<15);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step3: configure the AGC threshold.*/
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x07);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<1);
	tmp_u16 &= ~(1<<2);
	tmp_u16 &= ~(1<<7);
	tmp_u16 &= ~(1<<8);
	tmp_u16 |= (1<<9);
	tmp_u16 &= ~(1<<10);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
	phy_reg = 0x1E;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<8);
	tmp_u16 &= ~(1<<9);
	tmp_u16 |= (1<<10);
	tmp_u16 |= (1<<11);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step4: configure the DAC clock phase.*/
	phy_reg = 0x1C;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<4);
	tmp_u16 &= ~(1<<5);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step5: enlarge the AGC kill det windows length to 8us.*/
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x0A);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<0);
	tmp_u16 |= (1<<1);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step6: enlarge the lower threshold of slicer.*/
#if 0
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x03);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<8);
	tmp_u16 |= (1<<9);
	tmp_u16 &= ~(1<<10);
	tmp_u16 |= (1<<11);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
#endif

	/*Step7: decrease the BLW gain when in tracking killer pattern.*/
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x0B);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<5);
	tmp_u16 &= ~(1<<6);
	tmp_u16 |= (1<<7);
	tmp_u16 &= ~(1<<8);
	tmp_u16 &= ~(1<<9);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step8: enlarge the TX template swing.*/
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1C, 0xC500);

	/*Step9: decrease the PLL setting*/
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1d, 0x8);
	phy_reg = 0x1E;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<5);
	tmp_u16 &= ~(1<<6);
	tmp_u16 |= (1<<4);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
	phy_reg = 0x1f;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<7);
	tmp_u16 &= ~(1<<6);
	tmp_u16 |= (1<<5);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*Step9: set FSM to start.*/
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1d, 0);
	phy_reg = 0x10;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<0);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
	return;
}

static void c3821_phy_set(struct toe2_private *ptoe2)
{
	struct net_device *dev = ptoe2->ndev;
	u16 tmp_u16 = 0;
	int phy_reg = 0;

	/* Step1: set FSM to disable state,
	 * this process will shut down all related PHY's DSP part. */
	phy_reg = 0x10;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	/* Reset main FSM to IDLE State */
	tmp_u16 |= (1<<0);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/* Step2: configure the autoneg ability,
	 * that we can't support next page exchange. */
#ifndef PHY_IOL_TEST
	/* For Mass product, do not modify this register */
	phy_reg = 0x04;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<15);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
#else
	/* For IOL test, the following register should be added */
	phy_reg = 0x04;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<15);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	phy_reg = 0x11;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<10);
	tmp_u16 &= ~(1<<11);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/* enlarge the TX template swing */
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x06);
	phy_reg = 0x1E;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<15);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x0A);
	phy_reg = 0x1E;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<12);
	tmp_u16 |= (1<<13);
	tmp_u16 |= (1<<14);
	tmp_u16 |= (1<<15);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x02);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<4);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
#endif

	/* Step3: configure the AGC threshold. */
	/* select the AGC debug register */
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x07);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	/*tmp_u16 &= ~(1<<1);
	 * tmp_u16 |= (1<<2);
	 */
	tmp_u16 &= ~(1<<7);
	tmp_u16 &= ~(1<<8);
	tmp_u16 |= (1<<9);
	tmp_u16 &= ~(1<<10);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
	phy_reg = 0x1E;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<8);
	tmp_u16 &= ~(1<<9);
	tmp_u16 |= (1<<10);
	tmp_u16 |= (1<<11);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/* Step4: configure the DAC clock phase. */
	phy_reg = 0x1C;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<4);
	tmp_u16 &= ~(1<<5);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/* Step5: enlarge the AGC kill det windows length to 8us. */
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x0A);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<0);
	tmp_u16 |= (1<<1);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/*
	Step6: enlarge the lower threshold of slicer.(remove)
	mac_mdio_write(dev, toe2_phy_addr, 0x1D, 0x03);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, toe2_phy_addr, phy_reg);
	tmp_u16 |= (1<<8);
	tmp_u16 |= (1<<9);
	tmp_u16 &= ~(1<<10);
	tmp_u16 |= (1<<11);
	mac_mdio_write(dev, toe2_phy_addr, phy_reg, tmp_u16);
	 */

	/* Step7: decrease the BLW gain when in tracking killer pattern. */
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0x0B);
	phy_reg = 0x1F;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 |= (1<<5);
	tmp_u16 &= ~(1<<6);
	tmp_u16 |= (1<<7);
	tmp_u16 &= ~(1<<8);
	tmp_u16 &= ~(1<<9);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);

	/* Step8: enlarge the TX template swing. */
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1C, 0xC500);

	/* Step9: set FSM to start. */
	mac_mdio_write(dev, ptoe2->phy_addr, 0x1D, 0);
	phy_reg = 0x10;
	tmp_u16 = (u16)mac_mdio_read(dev, ptoe2->phy_addr, phy_reg);
	tmp_u16 &= ~(1<<0);
	mac_mdio_write(dev, ptoe2->phy_addr, phy_reg, tmp_u16);
	return;
}

static void phy_set(struct toe2_private *ptoe2)
{
	if (C3821 == ptoe2->chip_id) {
		c3821_phy_set(ptoe2);
	} else if (C3505 == ptoe2->chip_id) {
		c3505_phy_set(ptoe2);
	} else {
		;
	}
}

static void toe2_get_drvinfo(struct net_device *dev,
		struct ethtool_drvinfo *info)
{
	strcpy(info->driver, TOE2_DRV_NAME);
	strcpy(info->version, TOE2_DRV_VER);
	strcpy(info->bus_info, "Local Bus");
}

static int toe2_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return TOE2_NUM_STATS;
	default:
		return -EOPNOTSUPP;
	}
}

static int toe2_get_settings(struct net_device *dev,
							 struct ethtool_cmd *cmd)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	struct phy_device *phy_dev = ptoe2->phy_dev;
	if (!phy_dev) {
		return -ENODEV;
	}

	return phy_ethtool_gset(phy_dev, cmd);
}

static int toe2_set_settings(struct net_device *dev,
							 struct ethtool_cmd *cmd)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	struct phy_device *phy_dev = ptoe2->phy_dev;
	if (!phy_dev) {
		return -ENODEV;
	}

	return phy_ethtool_sset(phy_dev, cmd);
}

static int toe2_nway_reset(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	struct phy_device *phy_dev = ptoe2->phy_dev;
	if (!phy_dev) {
		return -ENODEV;
	}

	return genphy_restart_aneg(phy_dev);
}

static u32 toe2_get_msglevel(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);

	return ptoe2->msg_enable;
}

static void toe2_set_msglevel(struct net_device *dev, u32 value)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);

	ptoe2->msg_enable = value;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 0, 0))

#else
static u32 toe2_get_rx_csum(struct net_device *dev)
{
	return (u32)(ptoe2->toe2_rx_csum);
}

static int toe2_set_rx_csum(struct net_device *dev, u32 data)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	u32 ba = ptoe2->io_base;
	bool chg = false;
	u8 tmp_8;

	if (ptoe2->toe2_rx_csum) {
		if (!data) {
			chg = true;
			ptoe2->toe2_rx_csum = false;
		}
	} else {
		if (data) {
			chg = true;
			ptoe2->toe2_rx_csum = true;
		}
	}

	if (chg) {
		unsigned long flags = 0;
		spin_lock_irqsave(&ptoe2->lock, flags);

		tmp_8 = TOE2_R8(SCR);
		if (true == ptoe2->toe2_rx_csum)
			TOE2_W8(SCR, (tmp_8 | SCRRxCoeEn));
		else
			TOE2_W8(SCR, (tmp_8 & (~SCRRxCoeEn)));

		spin_unlock_irqrestore(&ptoe2->lock, flags);
	}

	return 0;
}
#endif

static void toe2_get_regs(struct net_device *dev,
		struct ethtool_regs *regs, void *p)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	unsigned long flags = 0;

	if (regs->len < TOE2_REGS_RANGE)
		return;

	regs->version = TOE2_REGS_VER;

	spin_lock_irqsave(&ptoe2->lock, flags);
	memcpy_fromio(p, (void *)ptoe2->io_base, TOE2_REGS_RANGE);
	spin_unlock_irqrestore(&ptoe2->lock, flags);
}

static const char toe2_ethtool_gstrings_stats[][ETH_GSTRING_LEN] = {
	"rx_mc",			  /* muticast packets received */
	"rx_bc",			  /* broadcast packets received */
	"rx_uc",			  /* unicast packets received */
	"rx_vlan",
	"rx_pppoe",			  /*pppoe packets received */
	"rx_ip",
	"rx_frag",
	"rx_runt_errors",
	"rx_long_errors",
	"rx_dribble_errors",
	"rx_phy_layer_errors",
	"rx_wd_timeout_errors",
	"rx_ip_chksum_errors",
	"rx_tcp_chksum_errors",
	"rx_buf_empty",	  /* there's no packet stored in rx ring buffer */
	"rx_late_col_seen",
	"rx_lost_in_ring",
	"rx_hdr_chs_errs",
	"rx_pay_chs_errs",
	"tx_col_cnts[0]",	/* 0 to 15 collisions */
	"tx_col_cnts[1]",
	"tx_col_cnts[2]",
	"tx_col_cnts[3]",
	"tx_col_cnts[4]",
	"tx_col_cnts[5]",
	"tx_col_cnts[6]",
	"tx_col_cnts[7]",
	"tx_col_cnts[8]",
	"tx_col_cnts[9]",
	"tx_col_cnts[10]",
	"tx_col_cnts[11]",
	"tx_col_cnts[12]",
	"tx_col_cnts[13]",
	"tx_col_cnts[14]",
	"tx_col_cnts[15]",
	"tx_col_errors",	 /* excessive collision */
	"rx_packets",		 /* total packets received	*/
	"tx_packets",		 /* total packets transmitted */
	"rx_bytes",		  /* total bytes received */
	"tx_bytes",		  /* total bytes transmitted */
	"rx_errors",		 /* bad packets received */
	"tx_errors",		 /* packet transmit problems */
	"rx_dropped",		 /* no space in linux buffers */
	"tx_dropped",		 /* no space available in linux */
	"multicast",		 /* multicast packets received */
	"collisions",
	"rx_length_errors",  /* detailed rx_errors: */
	"rx_over_errors",	/* receiver ring buff overflow */
	"rx_crc_errors",	 /* recved pkt with crc error */
	"rx_frame_errors",   /* recv'd frame alignment error */
	"rx_fifo_errors",	/* recv'r fifo overrun */
	"rx_missed_errors",  /* receiver missed packet */
	"tx_aborted_errors", /* detailed tx_errors */
	"tx_carrier_errors",
	"tx_fifo_errors",
	"tx_heartbeat_errors",
	"tx_window_errors",
	"rx_compressed",	 /* for cslip etc */
	"tx_compressed",
};

#define TOE2_STATS_LEN	ARRAY_SIZE(toe2_ethtool_gstrings_stats)

static int toe2_ethtool_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return TOE2_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void toe2_ethtool_get_strings(struct net_device *dev,
		u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_STATS:
		memcpy(data, *toe2_ethtool_gstrings_stats,
				sizeof(toe2_ethtool_gstrings_stats));
		break;
	}
}

static void toe2_get_ethtool_stats(struct net_device *dev,
		struct ethtool_stats *estats, u64 *data)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	struct toe2_device_stats mac_stats;
	struct net_device_stats net_stats;
	long i;
	long count1;
	long count2;

	toe2_get_stats(dev);
	net_stats = dev->stats;
	mac_stats = ptoe2->mac_stats;

	count1 = sizeof(mac_stats) / 4;
	count2 = sizeof(struct net_device_stats) / 4;

	if ((count1 + count2) != TOE2_STATS_LEN) {
		netdev_dbg(dev, "toe2_get_ethtool_stats: error!\n");
		return;
	}

	for (i = 0; i < TOE2_STATS_LEN; i++) {
		if (i < count1)
			data[i] = ((unsigned long *)&mac_stats)[i];
		else
			data[i] = ((unsigned long *)&net_stats)[i-count1];
	}
}

static const struct ethtool_ops toe2_ethtool_ops = {
	.get_drvinfo	   = toe2_get_drvinfo,
	.get_sset_count	= toe2_get_sset_count,
	.get_settings	   = toe2_get_settings,
	.set_settings	   = toe2_set_settings,
	.nway_reset		= toe2_nway_reset,
	.get_link	   = ethtool_op_get_link,
	.get_msglevel	   = toe2_get_msglevel,
	.set_msglevel	   = toe2_set_msglevel,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 0, 0))

#else
	.get_rx_csum		= toe2_get_rx_csum,
	.set_rx_csum		= toe2_set_rx_csum,
	.set_tx_csum		= ethtool_op_set_tx_csum,
	.set_sg			 = ethtool_op_set_sg,
	.set_tso			= ethtool_op_set_tso,
#endif
	.get_regs	   = toe2_get_regs,
	.get_strings	   = toe2_ethtool_get_strings,
	.get_sset_count	   = toe2_ethtool_get_sset_count,
	.get_ethtool_stats = toe2_get_ethtool_stats,
};

irqreturn_t toe2_isr(int Irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct toe2_private *ptoe2;
	 u32 cur_isr;

	if (unlikely(dev == NULL))
		return IRQ_NONE;

	ptoe2 = netdev_priv(dev);

	if (unlikely(!netif_running(dev))) {
		TOE2_W16(ptoe2->io_base, IMR, 0);
		return IRQ_HANDLED;
	}

	cur_isr = TOE2_R32(ptoe2->io_base, ISR);
	TOE2_W32(ptoe2->io_base, ISR, 0);

	if (cur_isr & ISRRxComplete)
		ptoe2->num_rx_complete++;
	if (cur_isr & ISRTimer)
		ptoe2->num_timer++;

	if (cur_isr & ptoe2->isr_mask) {
		if (cur_isr & ISRRxBufDiscard)
			netdev_dbg(ptoe2->ndev, "%s()=>isr(0x%08x).\n",
					__func__, cur_isr);
		ptoe2->cur_isr = (cur_isr & ptoe2->isr_mask);

		if (ptoe2->cur_isr & (ISRTxComplete|ISRTimer))
			free_tx_skb(ptoe2, true);
#ifdef PHY_BLINK_USING_GPIO
			if (g_enet_link_gpio >= 0)
				link_light_blink(ptoe2);
#endif
		if ((C3821 == ptoe2->chip_id)) {

			if (ptoe2->cur_isr & ISRTimer) {
				ptoe2->mac_pll_time++;
				if (ptoe2->mac_pll_time > ptoe2->mac_5sec) {
					if (ptoe2->times_of_link_change >=
							MAX_LINK_CHANGE_IN_5SEC)
						do_dll_pd_reset(ptoe2);

					ptoe2->times_of_link_change = 0;
					ptoe2->mac_pll_time = 0;
				}
			}
		}

		if (ptoe2->cur_isr &
				(ISRRxComplete | ISRRxFifoOverflow |
				 ISRRxBufOverflow | ISRRxBufDiscard |
				 ISRTimer)) {
			if (likely(napi_schedule_prep(&ptoe2->napi))) {
				toe2_disable_irq(ptoe2);
				__napi_schedule(&ptoe2->napi);
			} else {
				netdev_dbg(ptoe2->ndev, "%s:NapiScheduleFail\n", __func__);
			}
		}
	} else if (cur_isr & 0x1FFF) {
		if (netif_msg_ifup(ptoe2)) {
			netdev_dbg(ptoe2->ndev, "ISR(0x%08x) not supported\n", cur_isr);
		}
	}

	return IRQ_HANDLED;
}

void set_for_chip(struct toe2_private *ptoe2)
{
	u32 tmp32;
	u32 chip_id = ptoe2->chip_id;

	if (chip_id == C3505 || chip_id == C3821) {
		toe2_poweron_sequence(ptoe2);
	}
	if (C3505 == chip_id) {
		tmp32 = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp32 &= ~(1<<8);
		tmp32 &= ~(1<<1);
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp32);
	}
}

static int toe2_open(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	u32 tmp_u32 = 0;

	set_for_chip(ptoe2);
	if (netif_msg_ifup(ptoe2))
		netdev_dbg(ptoe2->ndev, "%s: enabling interface.\n", dev->name);

	mac_chip_rst(ptoe2);
	tmp_u32 = TOE2_R32(ptoe2->io_base, MiiMR1);
	tmp_u32 &= ~MiiMdioEn;
	TOE2_W32(ptoe2->io_base, MiiMR1, tmp_u32);
	ptoe2->rx_desc = NULL;
	ptoe2->tx_desc = NULL;
	ptoe2->setup_buf = NULL;
	if (mac_alloc_rings(ptoe2)) {
		netdev_err(dev, "alloc ring buffer fail\n");
		goto open_err_out;
	}
	mac_cnt_init(ptoe2);

	/* for PLL reset patch for ALi Phy (eg:38/39) */
	/* when link change, it shouldn't set to 0 */
	ptoe2->times_of_link_change = 0;
	ptoe2->unlink_error_state = 0;
	ptoe2->para_detect_times = 0;
	ptoe2->unlink_with_signal_times = 0;

	netif_carrier_off(dev);

	phy_set(ptoe2);

	TOE2_W32(ptoe2->io_base, TimerR, ptoe2->timer_freq);

	/* right now only passes TIMER/LINK interrupt */
	TOE2_W16(ptoe2->io_base, IMR, (ISRTimer | ISRLinkStatus));

	//if (C3821 == ptoe2->chip_id) {
#ifdef PHY_BLINK_USING_GPIO
		gpio_direction_output(g_enet_speed_gpio, !g_enet_gpio_light);
		gpio_direction_output(g_enet_link_gpio, !g_enet_gpio_light);
#endif
	//}

	if (ptoe2->phy_node) {
		napi_enable(&ptoe2->napi);
		phy_start(ptoe2->phy_dev);
		netif_start_queue(ptoe2->ndev);
	}
	return 0;

open_err_out:
	if (test_bit(NAPI_STATE_SCHED, &((ptoe2->napi).state)) == 0)
		napi_disable(&ptoe2->napi);
	mac_free_rings(ptoe2);
	return -EFAULT;
}

static int toe2_close(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);

	TOE2_W16(ptoe2->io_base, IMR, 0);
	TOE2_W8(ptoe2->io_base, SCR, 0);

	if (ptoe2->phy_dev) {
		phy_stop(ptoe2->phy_dev);
	}
	if (netif_device_present(dev)) {
		napi_disable(&ptoe2->napi);
		netif_tx_disable(dev);
		netif_carrier_off(dev);
	}

	ptoe2->link = 0;
	ptoe2->duplex_mode = 0;
	ptoe2->link_speed = -1;

	ptoe2->link_partner = 0;
	ptoe2->link = 0;
	ptoe2->transmit_okay = false;

	if (netif_msg_ifdown(ptoe2))
		netdev_dbg(ptoe2->ndev, "%s: disabling interface.\n", dev->name);

	mac_free_rings(ptoe2);
	return 0;
}

static void toe2_set_rx_mode(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	 u32 cmd_u32 = 0;
	 unsigned long flags = 0;

	spin_lock_irqsave(&ptoe2->lock, flags);

	cmd_u32 = TOE2_R32(ptoe2->io_base, NetworkOM);
	cmd_u32 &= ~(PassMask);

	if (dev->flags & IFF_PROMISC) {
		TOE2_W32(ptoe2->io_base, NetworkOM, (cmd_u32|PassPromiscuous));
		netdev_dbg(ptoe2->ndev,
				"%s()=>(dev->flags & IFF_PROMISC).\n",
				__func__);
	}

	spin_unlock_irqrestore(&ptoe2->lock, flags);
}

static struct net_device_stats *toe2_get_stats(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	unsigned long flags = 0;
	u16 tmp_u16;

	spin_lock_irqsave(&ptoe2->lock, flags);

	if (netif_running(dev) && netif_device_present(dev)) {
		tmp_u16 = TOE2_R16(ptoe2->io_base, MFC);
		netdev_dbg(dev, "Missed error = %d\n", tmp_u16);
		TOE2_W16(ptoe2->io_base, MFC, 0);
		dev->stats.rx_missed_errors += tmp_u16;
		dev->stats.rx_over_errors += tmp_u16;

		tmp_u16 = TOE2_R16(ptoe2->io_base, PPC);
		netdev_dbg(dev, "FIFO error = %d\n", tmp_u16);
		TOE2_W16(ptoe2->io_base, PPC, 0);
		dev->stats.rx_fifo_errors += tmp_u16;

		tmp_u16 = TOE2_R16(ptoe2->io_base, LFC);
		netdev_dbg(dev, "Long error = %d\n", tmp_u16);
		TOE2_W16(ptoe2->io_base, LFC, 0);
		dev->stats.rx_length_errors += tmp_u16;
		ptoe2->mac_stats.rx_long_errors += tmp_u16;

		tmp_u16 = TOE2_R16(ptoe2->io_base, RPC);
		netdev_dbg(dev, "Runt error = %d\n", tmp_u16);
		TOE2_W16(ptoe2->io_base, RPC, 0);
		dev->stats.rx_length_errors += tmp_u16;
		ptoe2->mac_stats.rx_runt_errors += tmp_u16;

		tmp_u16 = TOE2_R16(ptoe2->io_base, CrcErrCnt);
		netdev_dbg(dev, "CRC error = %d\n", tmp_u16);
		TOE2_W16(ptoe2->io_base, CrcErrCnt, 0);
		dev->stats.rx_crc_errors += tmp_u16;

		tmp_u16 = TOE2_R16(ptoe2->io_base, AlignErrCnt);
		netdev_dbg(dev, "Align error = %d\n", tmp_u16);
		TOE2_W16(ptoe2->io_base, AlignErrCnt, 0);
		dev->stats.rx_frame_errors += tmp_u16;
	}

	spin_unlock_irqrestore(&ptoe2->lock, flags);

	return &dev->stats;
}

int process_mac_tool_ioctl(struct net_device *dev, struct toe2_private *ptoe2,
						   cmd_param_t *cmd_param)
{
	struct ali_mac_xmit_io util_xmit_cmd;
	int rc = 0;

	if (cmd_param->cmd == READ_MAC_REG) {
		cmd_param->value = TOE2_R32(ptoe2->io_base, cmd_param->reg);
		pr_info("Mac Reg 0x%02x is 0x%08x\n", cmd_param->reg,
											cmd_param->value);
		return rc;
	}
	if (cmd_param->cmd == READ_SOC_REG) {
		cmd_param->value = SOC_R32(ptoe2->soc_base, cmd_param->reg);
		pr_info("Soc Reg 0x%02x is 0x%08x\n", cmd_param->reg,
											cmd_param->value);
		return rc;
	}

	if (cmd_param->cmd == WRITE_MAC_REG) {
		TOE2_W32(ptoe2->io_base, cmd_param->reg, cmd_param->value);
		pr_info("Mac Reg 0x%02x is 0x%08x\n", cmd_param->reg,
				TOE2_R32(ptoe2->io_base, cmd_param->reg));
		return rc;
	}

	if (cmd_param->cmd == WRITE_SOC_REG) {
		SOC_W32(ptoe2->soc_base, cmd_param->reg, cmd_param->value);
		pr_info("Soc Reg 0x%02x is 0x%08x\n", cmd_param->reg,
				(u32)SOC_R32(ptoe2->soc_base, cmd_param->reg));
		return rc;
	}

	if (cmd_param->cmd == MAC_RECV_INT) {
		pr_info("[*] num_rx_complete = %d\n", ptoe2->num_rx_complete);
		pr_info("[*] ptoe2->num_timer = %d\n", ptoe2->num_timer++);
		return rc;
	}

	if (cmd_param->cmd == MAC_CNT_CLEAN) {
		pr_info("clean mac counts!!!\n");
		memset(&(ptoe2->mac_stats), 0,
				sizeof(struct toe2_device_stats));
		memset(&(dev->stats), 0,
				sizeof(struct net_device_stats));
		ptoe2->num_rx_complete = 0;
		ptoe2->num_timer = 0;
		return rc;
	}

	if (cmd_param->cmd == TX_PKTS) {
		pr_info("%s %d: tx_pkt test start...\n", __func__, __LINE__);
		if (copy_from_user(&util_xmit_cmd, cmd_param->data,
						   sizeof(struct ali_mac_xmit_io))) {
			pr_err("copy_from_user error\n");
			rc = -EINVAL;
		} else {
			pr_info("!!!Utility MAC Init!!!\n");
			pr_info("cmd_param_.data 0x%x sizeof(ali_mac_xmit_io) %d\n",
					(unsigned int)cmd_param->data,
					sizeof(struct ali_mac_xmit_io));

			pr_info("ioctl tx_rx %d\n", util_xmit_cmd.tx_rx);
			pr_info("ioctl type %d\n", util_xmit_cmd.type);
			pr_info("ioctl toe_tx %d\n", util_xmit_cmd.toe_tx);
			pr_info("ioctl tso_ufo %d\n", util_xmit_cmd.tso_ufo);
			pr_info("ioctl len %lu\n", util_xmit_cmd.len);
			pr_info("ioctl max_len %lu\n", util_xmit_cmd.max_len);
			pr_info("ioctl desc_min %d\n", util_xmit_cmd.desc_min);
			pr_info("ioctl desc_max %d\n", util_xmit_cmd.desc_max);
			pr_info("ioctl duplex%d\n", util_xmit_cmd.duplex);
			pr_info("ioctl speed%d\n", util_xmit_cmd.speed);
			pr_info("ioctl rmii_rgmii%d\n", util_xmit_cmd.rmii_rgmii);
			pr_info("ioctl autonego %d\n", util_xmit_cmd.autonego);
			pr_info("sizeof  %d\n", sizeof(struct ali_mac_xmit_io));
			util_mac_init_tx(ptoe2, &util_xmit_cmd);
			test_mac_tx(ptoe2, &util_xmit_cmd);
			pr_info("%s %d: tx_pkt test done!\n", __func__, __LINE__);
		}
		return rc;
	}
	return -EOPNOTSUPP;
}

static int toe2_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	int rc = 0;
	cmd_param_t *cmd_param = NULL;
	struct toe2_private *ptoe2 = netdev_priv(dev);
	struct  phy_device *phy_dev = ptoe2->phy_dev;

	if (!netif_running(dev)) {
		return -EINVAL;
	}
	if (!phy_dev) {
		return -ENODEV;
	}
	if ((cmd >= SIOCDEVPRIVATE) && (cmd <= SIOCDEVPRIVATE + 15)) {
		switch (cmd) {
			case SIOCDEVPRIVATE + 11: {
				pr_info("++++++Test TOE2 utility++++\n");
				cmd_param = (cmd_param_t *) &(rq->ifr_data);
				rc = process_mac_tool_ioctl(dev, ptoe2, cmd_param);
				break;
			}
			default: {
				pr_info("Doesn't support your tools,Please use unify tools\n");
				rc = -EOPNOTSUPP;
				break;
			}
		}
		return rc;
	}
	return phy_mii_ioctl(phy_dev, rq, cmd);
}

static int toe2_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	unsigned long flags = 0;
	struct toe2_tx_desc *desc;
	u16 tx_wptr, tx_rptr;
	u16 desc_num;
	u16 off, first, last, gso_type = 0;
	u8 hdr_len = 0;
	int err, ret = NETDEV_TX_OK;
	struct skb_shared_info *skb_info = skb_shinfo(skb);
	static u8 temp_buff[TOE2_DESC_SZ];

	spin_lock_irqsave(&ptoe2->lock, flags);

	if (!ptoe2->link) {
		dev_kfree_skb_any(skb);
		goto xmit_done;
	}

	free_tx_skb(ptoe2, false);

	desc_num = skb_info->nr_frags + 1;
	tx_wptr = ptoe2->tx_wptr;

	if (ptoe2->tx_skb_rd != ptoe2->tx_skb_wr) {
		tx_rptr = ptoe2->as_tx_skb[ptoe2->tx_skb_rd].first;
		/* full */
		if ((tx_wptr > tx_rptr && TOE2_TX_DESC_NUM-(tx_wptr-tx_rptr) <
					desc_num+1) ||
				(tx_wptr < tx_rptr &&
				 tx_rptr-tx_wptr < desc_num+1)) {
			netdev_dbg(ptoe2->ndev, "%s %d: tx ring desc full.\n",
					__func__, __LINE__);
			ret = NETDEV_TX_BUSY;
			goto xmit_done;
		}
	}

	if (skb_is_gso(skb)) {
		if (skb_header_cloned(skb)) {
			err = pskb_expand_head(skb, 0, 0, GFP_ATOMIC);
			if (err) {
				spin_unlock_irqrestore(&ptoe2->lock, flags);
				return err;
			}
		}
		gso_type = skb_info->gso_type;
	}

	first = tx_wptr;
	last = first + skb_info->nr_frags;

	off = tx_wptr;
	if (skb_shinfo(skb)->nr_frags == 0) {
		mac_tx_cfg(ptoe2, off, skb, skb->data, skb->len);
		desc = &ptoe2->tx_desc[off];
		desc->tx_sts.hw.OWN = 1;
		desc->tx_sts.hw.FS = 1;
		desc->tx_sts.hw.LS = 1;
		desc->tot_len = skb->len;
		desc->seg_num = 1;

		if (gso_type & SKB_GSO_TCPV4) {
			hdr_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
			desc->tx_sts.hw.Mfl = skb_info->gso_size + hdr_len;
			netdev_dbg(ptoe2->ndev, "TSO:\n");
		} else if (gso_type & SKB_GSO_UDP) {
			desc->tx_sts.hw.Mfl  = dev->mtu + 14;
			/* desc->tx_sts.hw.Mfl = TOE2_MSS; */
			netdev_dbg(ptoe2->ndev, "UFO:\n");
		} else
			desc->tx_sts.hw.Mfl = TOE2_MSS;

		netdev_dbg(ptoe2->ndev, "len(%d), nr_frags(%d), Mfl(%d)\n",
				skb->len, desc_num, desc->tx_sts.hw.Mfl);

		if ((++off) >= TOE2_TX_DESC_NUM)
			off = 0;
	} else {  /* skb_shinfo(skb)->nr_frags >= 1 */
		u32 first_len, frag_len;
		int frag;
		void *seg_addr;

		first_len = skb_headlen(skb);
		mac_tx_cfg(ptoe2, off, skb, skb->data, first_len);
		desc = &ptoe2->tx_desc[off];
		desc->tx_sts.hw.OWN = 1;
		desc->tx_sts.hw.FS = 1;
		desc->tot_len = skb->len;
		desc->seg_num = desc_num;

		if (gso_type & SKB_GSO_TCPV4) {
			hdr_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
			desc->tx_sts.hw.Mfl = skb_info->gso_size + hdr_len;
			netdev_dbg(ptoe2->ndev, "TSO:\n");
		} else if (gso_type & SKB_GSO_UDP) {
			desc->tx_sts.hw.Mfl  = dev->mtu + 14;
			netdev_dbg(ptoe2->ndev, "UFO:\n");
		} else
			desc->tx_sts.hw.Mfl = TOE2_MSS;

		netdev_dbg(ptoe2->ndev, "len(%d), nr_frags(%d), Mfl(%d)\n",
				skb->len, desc_num, desc->tx_sts.hw.Mfl);

		if ((++off) >= TOE2_TX_DESC_NUM)
			off = 0;

		for (frag = 0; frag < skb_shinfo(skb)->nr_frags; frag++) {
			skb_frag_t *this_frag = &skb_shinfo(skb)->frags[frag];

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0))
			seg_addr = (void *)page_address(this_frag->page) +
				this_frag->page_offset;
#else
			seg_addr =  (void *)skb_frag_address(this_frag);
#endif
			frag_len = this_frag->size;
			mac_tx_cfg(ptoe2, off, NULL, seg_addr, frag_len);

			if (frag == skb_shinfo(skb)->nr_frags - 1) {
				desc = &ptoe2->tx_desc[off];
				desc->tx_sts.hw.LS = 1;
			}

			if ((++off) >= TOE2_TX_DESC_NUM)
				off = 0;
		}
	}
	memcpy((void *)temp_buff, (void *)desc, TOE2_DESC_SZ);

	TOE2_W16(ptoe2->io_base, TxRingDesWPtr, off);
	ptoe2->tx_wptr = off;

	ptoe2->as_tx_skb[ptoe2->tx_skb_wr].first = first;
	ptoe2->as_tx_skb[ptoe2->tx_skb_wr].cnt = desc_num;
	ptoe2->as_tx_skb[ptoe2->tx_skb_wr].skb = skb;
	if (ptoe2->tx_skb_wr+1 == TOE2_TX_DESC_NUM)
		ptoe2->tx_skb_wr = 0;
	else
		ptoe2->tx_skb_wr++;

	//if (C3821 == ptoe2->chip_id) {
#ifdef PHY_BLINK_USING_GPIO
		if (!ptoe2->blink_light && !ptoe2->in_blink)
			ptoe2->blink_light = 1;
#endif
	//}
xmit_done:
	spin_unlock_irqrestore(&ptoe2->lock, flags);

	return ret;
}

static void toe2_tx_timeout(struct net_device *dev)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	unsigned long flags = 0;

	netdev_dbg(dev, "%s() happened...\n", __func__);

	spin_lock_irqsave(&ptoe2->lock, flags);

	mac_desc_clean(ptoe2);
	mac_chip_rst(ptoe2);
	mac_cnt_init(ptoe2);
	mac_hw_start(ptoe2);

	netif_wake_queue(dev);

	spin_unlock_irqrestore(&ptoe2->lock, flags);

	return;
}

static int toe2_set_mac_address(struct net_device *dev, void *p)
{
	struct toe2_private *ptoe2 = netdev_priv(dev);
	u8 *da = NULL;
	struct sockaddr *addr = p;

	if (netif_running(dev))
		return -EBUSY;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	da = (u8 *)dev->dev_addr;

	TOE2_W32(ptoe2->io_base, PAR - 4, da[0] << 8 | da[1]);
	TOE2_W32(ptoe2->io_base, PAR,
			((da[2] << 24) | (da[3] << 16) | (da[4] << 8) | da[5]));

	return 0;
}

static const struct net_device_ops toe2_netdev_ops = {
	.ndo_open			= toe2_open,
	.ndo_stop			= toe2_close,
	.ndo_validate_addr	= eth_validate_addr,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 0, 0))
	.ndo_set_rx_mode	=  toe2_set_rx_mode,
#else
	.ndo_set_multicast_list = toe2_set_rx_mode,
#endif
	.ndo_get_stats		= toe2_get_stats,
	.ndo_do_ioctl		= toe2_ioctl,
	.ndo_start_xmit		= toe2_start_xmit,
	.ndo_tx_timeout		= toe2_tx_timeout,
	.ndo_set_mac_address	= toe2_set_mac_address,
};

static void print_baseinfo(struct toe2_private *ptoe2)
{
	netdev_dbg(ptoe2->ndev,
			"toe2_base	   = 0x%08X\n", (u32)ptoe2->io_base);
	netdev_dbg(ptoe2->ndev,
			"toe2_irq		= %d\n", ptoe2->irq_num);
	netdev_dbg(ptoe2->ndev,
			"toe2_mac_hi16   = 0x%08X\n", ptoe2->toe2_mac_hi16);
	netdev_dbg(ptoe2->ndev,
			"toe2_mac_lo32   = 0x%08X\n", ptoe2->toe2_mac_lo32);
	netdev_dbg(ptoe2->ndev,
			"toe2_phy_addr   = %d\n", ptoe2->phy_addr);

	if (ptoe2->toe2_link_mode > 0x7FUL) {
		netdev_dbg(ptoe2->ndev, "toe2_link_mode = %d, Unknown\n",
				ptoe2->toe2_link_mode);
		ptoe2->toe2_link_mode = 0x7FUL;
		netdev_dbg(ptoe2->ndev, "reset link_mode to %d\n",
				ptoe2->toe2_link_mode);
	} else if (0UL == ptoe2->toe2_link_mode) {
		netdev_dbg(ptoe2->ndev, "link_mode = %d, Link Off\n",
				ptoe2->toe2_link_mode);
	} else {
		netdev_dbg(ptoe2->ndev, "link_mode=0x%X, %s,%s,%s,%s,%s,%s,%s\n",
				ptoe2->toe2_link_mode,
				(ptoe2->toe2_link_mode & TOE2_10HD) ?
				"10HD" : "",
				(ptoe2->toe2_link_mode & TOE2_10FD) ?
				"10FD" : "",
				(ptoe2->toe2_link_mode & TOE2_100HD) ?
				"100HD" : "",
				(ptoe2->toe2_link_mode & TOE2_100FD) ?
				"100FD" : "",
				(ptoe2->toe2_link_mode & TOE2_RX_PAUSE) ?
				"PAUSE RX" : "",
				(ptoe2->toe2_link_mode & TOE2_TX_PAUSE) ?
				"PAUSE_TX" : "",
				(ptoe2->toe2_link_mode & TOE2_AUTONEG) ?
				"AUTONEG" : "");
	}


	netdev_dbg(ptoe2->ndev, "phy_interface %d\n", ptoe2->phy_interface);
	netdev_dbg(ptoe2->ndev, "toe2_sg = %s\n", (true == ptoe2->toe2_sg) ?
			"true" : "false");
	netdev_dbg(ptoe2->ndev, "tx_csum = %s\n", (true == ptoe2->toe2_tx_csum) ?
			"true" : "false");
	netdev_dbg(ptoe2->ndev, "rx_csum = %s\n", (true == ptoe2->toe2_rx_csum) ?
			"true" : "false");
	netdev_dbg(ptoe2->ndev, "toe2_tso = %s\n", (true == ptoe2->toe2_tso) ?
			"true" : "false");
	netdev_dbg(ptoe2->ndev, "toe2_ufo = %s\n", (true == ptoe2->toe2_ufo) ?
			"true" : "false");
	netdev_dbg(ptoe2->ndev, "reversemii = %s\n",
			(true == ptoe2->toe2_reversemii) ? "true" : "false");
}

static int toe2_poweron_sequence(struct toe2_private *ptoe2)
{
	/* "Async reset ETH_TOP" */
	u32 chip_id = ptoe2->chip_id;
	u32 tmp_val;
	if (chip_id == C3821) {
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG84);
		tmp_val |= 1<<17;
		SOC_W32(ptoe2->soc_base, SOC_REG84, tmp_val);

		/* "Enable ENET_PLL_PD & ENET_DLL_PD" */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp_val |= 1<<0;
		tmp_val |= 1<<1;
		tmp_val |= 1<<8;
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
		udelay(10);

		/* "Disable ENET_SLEEP" */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp_val &= ~(1<<8);
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
		udelay(250);

		/* "Disable ENET_PLL_PD" */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp_val &= ~(1<<0);
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
		udelay(250);

		/* "Disable ENET_DLL_PD" */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp_val &= ~(1<<1);
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
		udelay(250);

		/* "Async reset ETH_TOP" */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG84);
		tmp_val &= ~(1<<17);
		SOC_W32(ptoe2->soc_base, SOC_REG84, tmp_val);
		udelay(250);

		/* Power On Patch
		 * "PLL_PD=1"
		 */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp_val |= 1<<0;
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
		udelay(10);

		/* "PLL_PD=0" */
		tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG640);
		tmp_val &= ~(1<<0);
		SOC_W32(ptoe2->soc_base, SOC_REG640, tmp_val);
		udelay(250);
	}
	/* "IP_RST=1" */
	tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG84);
	tmp_val |= 1<<17;
	SOC_W32(ptoe2->soc_base, SOC_REG84, tmp_val);
	udelay(10);

	/* "IP_RST=0" */
	tmp_val = SOC_R32(ptoe2->soc_base, SOC_REG84);
	tmp_val &= ~(1<<17);
	SOC_W32(ptoe2->soc_base, SOC_REG84, tmp_val);
	udelay(10);
	return 0;
}

void do_phy_reset(struct toe2_private *ptoe2)
{
	enum of_gpio_flags flags;
	int err, phy_reset;
	int msec = 10;
	struct device_node *np = ptoe2->dev->of_node;

	if (!np)
		return;
	phy_reset = of_get_named_gpio_flags(np, "phy-reset-gpios", 0, &flags);
	if (!gpio_is_valid(phy_reset)) {
		dev_err(ptoe2->dev, "no phy_reset gpio\n");
		return;
	}

	err = devm_gpio_request_one(ptoe2->dev, phy_reset,
			flags ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
			"phy-reset");
	if (err) {
		dev_err(ptoe2->dev, "failed to get phy-reset-gpios: %d\n", err);
		return;
	}
	msleep(msec);
	gpio_set_value(phy_reset, !flags ? 1 : 0);
}


static int ali_mac_probe(struct platform_device *pdev)
{
	struct net_device *ndev = NULL;
	struct toe2_private *ptoe2 = NULL;
	struct resource *res = NULL;
	const unsigned char *of_mac_addr = NULL;
	int err = 0;
	u32 tmp_u32 = 0;

	struct phy_device *phy_dev = NULL;
	struct device_node *phy_node = NULL;
	struct device_node *node = NULL;
	int ret = 0;
	struct clk *clk = NULL;
	const char *clk_name = NULL;
	u8 mac_addr[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

	ndev = alloc_etherdev(sizeof(struct toe2_private));
	if (!ndev) {
		dev_err(&pdev->dev, "alloc_etherdev fail!\n");
		err = -ENOMEM;
		goto err_out;
	}

	platform_set_drvdata(pdev, ndev);
	ptoe2 = netdev_priv(ndev);
	ptoe2->tx_skb_rd = 0;
	ptoe2->ndev = ndev;
	ptoe2->dev = &pdev->dev;
	ptoe2->tx_skb_wr = 0;
	memset(ptoe2->as_tx_skb,
			0, sizeof(struct toe2_skb_info) * TOE2_TX_DESC_NUM);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ptoe2->io_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ptoe2->io_base)) {
		dev_err(&pdev->dev, "io_base fail!\n");
		err = PTR_ERR(ptoe2->io_base);
		goto err_out_free_netdev;
	}

	dev_dbg(&pdev->dev, "OF: of_iomap: get base = 0x%08x\n",
		(u32)ptoe2->io_base);

	ptoe2->soc_base = of_iomap(pdev->dev.of_node, 1);
	if (!ptoe2->soc_base) {
		dev_dbg(&pdev->dev, "dts hasn't soc base address, use old soc_r32/w32\n");
	}
	ptoe2->chip_id = (SOC_R32(ptoe2->soc_base, 0) >> 16);
	if ((C3821 == ptoe2->chip_id)) {
		ptoe2->mac_5sec = (u32)(6 * MAC_CLK / TIMER_200_FREQ);
	}

	ptoe2->irq_num = platform_get_irq(pdev, 0);
	if (ptoe2->irq_num <= 0) {
		dev_err(&pdev->dev, "no irq resource specified\n");
		err = -ENOENT;
		goto err_out_free_netdev;
	}

	of_mac_addr = of_get_mac_address (pdev->dev.of_node);
	if (of_mac_addr) {
		memcpy(mac_addr, of_mac_addr, ETH_ALEN);
		netdev_dbg(ndev, "\t%02x %02x %02x %02x %02x %02x\n",
				mac_addr[0], mac_addr[1], mac_addr[2],
				mac_addr[3], mac_addr[4], mac_addr[5]);
	}

	if (of_property_read_u32(pdev->dev.of_node, "rx-csum-enable",
		&ptoe2->toe2_rx_csum)) {
		dev_err(&pdev->dev, "read rx-csum-enable failed\n");
		err = -EINVAL;
		goto err_out_free_netdev;
	}

	if (of_property_read_u32(pdev->dev.of_node, "tx-csum-enable",
		&ptoe2->toe2_tx_csum)) {
		dev_err(&pdev->dev, "read tx-csum-enable failed\n");
		err = -EINVAL;
		goto err_out_free_netdev;
	}
	if (of_property_read_u32(pdev->dev.of_node, "phy-reversemii",
		&ptoe2->toe2_reversemii)) {
		dev_err(&pdev->dev, "read rx-csum-enable failed\n");
		err = -EINVAL;
		goto err_out_free_netdev;
	}

	of_property_read_string(pdev->dev.of_node, "clock-names", &clk_name);
	clk = devm_clk_get(&pdev->dev, clk_name);
	if (IS_ERR(clk)) {
		err = PTR_ERR(clk);
		dev_dbg(&pdev->dev, "get clk %d failed\n", err);
	} else {
		clk_prepare_enable(clk);
	}

	ptoe2->toe2_sg		 = true;
	ptoe2->toe2_tso		= true;
	ptoe2->toe2_ufo		= true;
	ptoe2->toe2_debug	  = -1;

	/* All: 10HD,10FD,100HD,100FD,ASYMP,SYMP,ANEG */
	ptoe2->toe2_link_mode  = 0x7FUL;

	if (!is_valid_ether_addr(mac_addr)) {
		if (is_valid_ether_addr(stb_mac_addr)) {
			dev_dbg(&pdev->dev, "use mac addr from cmdline.\n");
			memcpy(mac_addr, stb_mac_addr, ETH_ALEN);
		} else {
			dev_dbg(&pdev->dev, "use default mac addr.\n");
			memcpy(mac_addr, dft_mac_addr, ETH_ALEN);
		}
	} else {
		dev_dbg(&pdev->dev, "use OF mac addr.\n");
	}
	ptoe2->toe2_mac_lo32 = (mac_addr[3]<<24)|
		(mac_addr[2]<<16)|(mac_addr[1]<<8)|
		(mac_addr[0]);
	ptoe2->toe2_mac_lo32 &= 0xFFFFFFFE;
	ptoe2->toe2_mac_hi16 = (mac_addr[5]<<8)|(mac_addr[4]);

	do_phy_reset(ptoe2);

	mac_chip_rst(ptoe2);
	tmp_u32 = TOE2_R32(ptoe2->io_base, MiiMR1);
	tmp_u32 &= ~MiiMdioEn;
	TOE2_W32(ptoe2->io_base, MiiMR1, tmp_u32);

	ptoe2->mdio_bus = mdiobus_alloc();
	if (ptoe2->mdio_bus == NULL) {
		dev_err(&pdev->dev, "mdiobus_alloc failed\n");
		err = -ENOMEM;
		goto err_out_free_netdev;
	}

	ptoe2->mdio_bus->name = "toe2_mdio_bus";
	ptoe2->mdio_bus->read = toe2_mdio_read;
	ptoe2->mdio_bus->write = toe2_mdio_write;

	snprintf(ptoe2->mdio_bus->id, MII_BUS_ID_SIZE, "%s-%x",
			 pdev->name, ptoe2->dev_id + 1);

	ptoe2->mdio_bus->priv = ptoe2;
	ptoe2->mdio_bus->parent = &pdev->dev;

	node = of_get_child_by_name(pdev->dev.of_node, "mdio");
	if (node) {
		err = of_mdiobus_register(ptoe2->mdio_bus, node);
		of_node_put(node);
	} else {
		err = mdiobus_register(ptoe2->mdio_bus);
	}

	if (err) {
		dev_err(&pdev->dev, "failed to register MDIO bus\n");
		goto err_out_free_mdiobus;
	}

	phy_node = of_parse_phandle(pdev->dev.of_node, "phy-handle", 0);
	if (!phy_node) {
		err = -ENOENT;
		dev_err(&pdev->dev, "phy_node failed!\n");
		goto err_out_unregister_mdiobus;
	}

	ret = of_get_phy_mode(pdev->dev.of_node);
	if (ret < 0) {
		ptoe2->phy_interface = PHY_INTERFACE_MODE_RMII;
	} else {
		ptoe2->phy_interface = ret;
	}

	ptoe2->phy_node = phy_node;

	phy_dev = of_phy_connect(ptoe2->ndev, ptoe2->phy_node,
							 &toe2_adjust_link, 0,
							 ptoe2->phy_interface);
	if (!phy_dev) {
		dev_err(&pdev->dev, "of_phy_connect failed\n");
		err = -ENODEV;
		goto err_out_unregister_mdiobus;
	}
	ptoe2->phy_dev = phy_dev;
	ptoe2->phy_addr = phy_dev->addr;
	phy_dev->supported &= (SUPPORTED_10baseT_Half |
						  SUPPORTED_10baseT_Full |
						  SUPPORTED_100baseT_Half |
						  SUPPORTED_100baseT_Full |
						  SUPPORTED_Autoneg);
	phy_dev->advertising &= (ADVERTISED_10baseT_Half |
						  ADVERTISED_10baseT_Full |
						  ADVERTISED_100baseT_Half |
						  ADVERTISED_100baseT_Full |
						  ADVERTISED_Autoneg);
	phy_dev->autoneg = AUTONEG_ENABLE;

	ptoe2->msg_enable =
		(ptoe2->toe2_debug < 0 ?
		 TOE2_DEF_MSG_ENABLE : ptoe2->toe2_debug);
	spin_lock_init(&ptoe2->lock);

	ndev->dev_addr[0] = LOBYTE(LOWORD(ptoe2->toe2_mac_lo32));
	ndev->dev_addr[1] = HIBYTE(LOWORD(ptoe2->toe2_mac_lo32));
	ndev->dev_addr[2] = LOBYTE(HIWORD(ptoe2->toe2_mac_lo32));
	ndev->dev_addr[3] = HIBYTE(HIWORD(ptoe2->toe2_mac_lo32));
	ndev->dev_addr[4] = LOBYTE(LOWORD(ptoe2->toe2_mac_hi16));
	ndev->dev_addr[5] = HIBYTE(LOWORD(ptoe2->toe2_mac_hi16));
	ndev->irq = ptoe2->irq_num;

	print_baseinfo(ptoe2);

	ndev->netdev_ops = &toe2_netdev_ops;

	/********************************************************************
	VALUE_ADJUST:
	netif_napi_add(dev, &ptoe2->napi, mac_rx_poll, 256);
	256 -- how many packets linux network stack process at a time.
	it should not be too big, because it will cause longer latency,
	also because mac_rx_poll is in softirq or ksoftirq,
	it might affect other threads.
	 ********************************************************************/
	netif_napi_add(ndev, &ptoe2->napi, mac_rx_poll, 256);

	ndev->ethtool_ops = &toe2_ethtool_ops;
	ndev->watchdog_timeo = TOE2_TX_TIMEOUT;

	ndev->features |= NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX;

	if (ptoe2->toe2_sg)
		ndev->features |= NETIF_F_SG | NETIF_F_FRAGLIST;
	if (ptoe2->toe2_tso)
		ndev->features |= NETIF_F_TSO;
	if (ptoe2->toe2_ufo)
		ndev->features |= NETIF_F_UFO | NETIF_F_HW_CSUM;
	if (ptoe2->toe2_tx_csum)
		/* Support TCP/UDP checksum over IPv4 */
		ndev->features |= NETIF_F_IP_CSUM;

	ndev->flags |= IFF_PROMISC | IFF_MULTICAST;

	err = register_netdev(ndev);
	if (err) {
		dev_err(&pdev->dev, "register_netdev failed %d\n", err);
		goto err_out_unregister_mdiobus;
	}

	toe2_disable_irq(ptoe2);
	err = devm_request_irq(&pdev->dev, ptoe2->irq_num, toe2_isr,
				0, ndev->name, (void *)ndev);
	if (err) {
		netdev_err(ndev, "Could not allocate IRQ\n");
		goto err_out_unregister_mdiobus;
	}
#ifdef PHY_BLINK_USING_GPIO
	gpio_request(g_enet_link_gpio, "gpio_link");
	gpio_enable_pin(g_enet_link_gpio);
	gpio_request(g_enet_speed_gpio, "gpio_speed");
	gpio_enable_pin(g_enet_speed_gpio);
#endif	
	return 0;
err_out_unregister_mdiobus:
	mdiobus_unregister(ptoe2->mdio_bus);
err_out_free_mdiobus:
	mdiobus_free(ptoe2->mdio_bus);
err_out_free_netdev:
	free_netdev(ndev);
err_out:
	return err;
}

static int ali_mac_suspend(struct platform_device *pdev , pm_message_t state)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct toe2_private *ptoe2 = netdev_priv(dev);

	toe2_close(ptoe2->ndev);
	return 0;
}

static int ali_mac_resume(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct toe2_private *ptoe2 = netdev_priv(dev);

	toe2_open(ptoe2->ndev);
	return 0;
}

static int ali_mac_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	unregister_netdev(dev);
	free_netdev(dev);
	return 0;
}

static const struct of_device_id ali_mac_of_match[] = {
	{ .compatible = "alitech,cap210-toe2", },
	{},
};
MODULE_DEVICE_TABLE(of, ali_mac_of_match);

static struct platform_driver ali_mac_driver = {
	.driver	= {
		.name  = "ALi TOE2",
		.owner = THIS_MODULE,
		.of_match_table = ali_mac_of_match,
	},
	.probe	 = ali_mac_probe,
	.remove	= ali_mac_remove,
	.suspend   = ali_mac_suspend,
	.resume	= ali_mac_resume,
};

static void __init parse_mac_addr(char *macstr) {
	int i, j;
	unsigned char result, value;
	u8 tmp_mac_addr[ETH_ALEN];

	if(strlen(macstr) < (12+5)) {
		return;
	}
	memset(tmp_mac_addr, 0, (sizeof(u8)*ETH_ALEN));	
	for (i = 0; i < 6; i++) {
		if (i != 5 && *(macstr + 2) != ':') {
			return;
		}
		result = 0;	
		for (j = 0; j < 2; j++) {
			if (isxdigit(*macstr) &&
			(value = isdigit(*macstr) ? *macstr - '0' : toupper(*macstr) - 'A' + 10) < 16) {
				result = result * 16 + value;
				macstr++;
			} else {
				return;
			}
		}
		macstr++;
		tmp_mac_addr[i] = result;
	}
	if (is_valid_ether_addr(tmp_mac_addr)) {
		memcpy(stb_mac_addr, tmp_mac_addr, sizeof(u8)*ETH_ALEN); 
	}
	return;
}

static int __init program_setup_kmac(char *s) {
	if(s) {
		parse_mac_addr(s);
	}
	return 0;
}
__setup("mac=", program_setup_kmac);

module_platform_driver(ali_mac_driver);

MODULE_AUTHOR("ALi STU SHA <corey.chi@alitech.com>");
MODULE_DESCRIPTION("ALi 10/100Mbps ethernet mac driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
