/*
 * Virtual Ethernet - virtnic - NIC driver.
 *
 * This NIC driver is for a pseudo network device (one that doesn't even exist!).
 * We emulate the NIC via a recent and interesting kernel framework - the faux
 * bus! (As you'll realize, the word 'faux' means fake or false.)
 * The faux bus was merged in the 6.14 kernel. It's for cases like this - where
 * we don't (ab)use the platform device when there really isn't any platform
 * device. The faux bus provides the probe and remove hooks; we use them (just
 * as we would with an actual platform or other device).
 * The driver creates a virtual n/w interface named 'vnic0' on the system.
 *
 *-----------------------------------------------------------------------------
 * Transmit Path (Tx)
 *-----------------------------------------------------------------------------
 * When our user-land datagram socket app (talker_dgram) sends network packet(s)
 * to the interface, this driver's transmit method is invoked by the kernel.
 * The "packet sniffing" in the transmit routine checks for an IPv4 UDP packet
 * with 'our' port number, and on finding these criteria match, it 'accepts'
 * the packet to be 'transmitted'.
 * Of course, as there's no real hw so this is just a learning exercise with a
 * very simple 'virtual ethernet' NIC...
 * The 'tx' routine then peeks into the SKB, displaying it's vitals (we even show
 * the packet content, including the eth, IP, UDP headers and the data payload!).
 *
 * Pl see the book, Ch 6, for further details.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include "../virtnic_common.h"

/* Our driver private or context structure */
struct virtnic_pvt_data {
	struct net_device *netdev;
	spinlock_t lock;
	int txpktnum, rxpktnum;
	int tx_bytes, rx_bytes;
	struct napi_struct napi;
	struct hrtimer rx_timer;
};

//--------------------- Tx path -----------------------------------------------
/*
 * The Tx entry point.
 * Runs in atomic context, BH disabled
 * To get to the Tx path, try:
 * - running our custom simple datagram "talker_dgram" userspace app
 *   ./talker_dgram <IP addr> "<some msg>"
 * - 'ping -c1 10.10.1.x , with 'x' != IP addr (5, in our current setup;
 *   also but realize that ping won't actually work on a local interface).
 * Use a network analyzer (eg tcpdump/Wireshark) to see packets flowing across
 * the interface!
 * Best: just run our ../userspc/tx_packets_to_virtnic and
 * ../userspc/run_and_capture_pkt wrapper scripts to test.
 */
static netdev_tx_t virtnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	const struct iphdr *ip;
	const struct udphdr *udph;
	struct virtnic_pvt_data *priv = netdev_priv(dev);

	if (!skb) {		// paranoia!
		pr_alert("skb NULL!\n");
		return NETDEV_TX_BUSY;
	}

/*
   The to-be-Tx (UDP protocol) packet:
Len:      2       14                    20                   8           x
	+----------------------------------------------------------------------+
	|  |  Eth II Hdr  |         IPv4 Hdr         |  UDP Hdr   |  Data      |
	+----------------------------------------------------------------------+
	^  ^                                                            ^      ^
	|  |                                                            |      |
skb-> head data                                                        tail   end
 */

	/*---------Packet Filtering :) --------------*/
	/* Accept only IPv4 packets */
	if (skb->protocol != htons(ETH_P_IP))
		goto out_tx;

	/* If the outgoing packet is not IPv4 and UDP protocol, discard it */
	ip = ip_hdr(skb);
	if (ip->protocol != IPPROTO_UDP || ip_is_fragment(ip)) {
		pr_cont("x");
		goto out_tx;
	}
	if (!skb_transport_header_was_set(skb))
		goto out_tx;

	/*
	 * If the outgoing (UDP protocol) packet does NOT have destination port=54295,
	 * then it's not sent to our n/w interface via our talker_dgram app,
	 * so simply ignore it.
	 */
	udph = udp_hdr(skb);
	pr_debug("UDP pkt::src=%d dest=%d len=%u\n",
		 ntohs(udph->source), ntohs(udph->dest), ntohs(udph->len));
	if (udph->dest != htons(PORTNUM))	// port # 54295
		goto out_tx;
	//------------------------------

	pr_info("ah, a UDP packet Tx via our app (dest port %d)\n", PORTNUM);
	SKB_PEEK(skb);

#if 0				// already seen with the SKB_PEEK()
	pr_debug("data payload:\n");	// it's after the Eth + IP + UDP headers
	print_hex_dump_bytes(" ", DUMP_PREFIX_OFFSET, skb->head + 16 + 20 + 8,
				skb->len);
#endif

	/* Update stat counters
	 * Spin lock: don't use the _bh ver here as netconsole can invoke Tx
	 * with hw interrupts disabled
	 */
	spin_lock(&priv->lock);
	priv->txpktnum ++;
	priv->tx_bytes += skb->len;
	spin_unlock(&priv->lock);

#if 0
	pr_debug("Emulating Rx by artificially invoking virtnet_rx() now...\n");
	virtnet_rx((unsigned long)dev, skb);
#endif
 out_tx:
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

//--------------------- Rx path -----------------------------------------------
#define ONE_MS	1000000

/* This function - the hrtimer timeout - emulates the Rx 'hardware interrupt'! */
static enum hrtimer_restart pseudo_rx_timer_func(struct hrtimer *t)
{
	struct virtnic_pvt_data *priv = container_of(t,
				struct virtnic_pvt_data, rx_timer);

	napi_schedule(&priv->napi);

	hrtimer_forward_now(t, ns_to_ktime(ONE_MS * 100));	// 100 ms
	return HRTIMER_RESTART;
}

/* We ignore the @budget here */
static int pseudo_napi_poll(struct napi_struct *napi, int budget)
{
	struct virtnic_pvt_data *priv = container_of(napi,
				struct virtnic_pvt_data, napi);
	struct sk_buff *skb;
	int pkt_len = 64;

	// Simulate receiving one network packet
	skb = netdev_alloc_skb(priv->netdev, pkt_len);
	if (!skb)
		return 0;

	skb_put(skb, pkt_len);
	memset(skb->data, 0xAB, pkt_len);	// dummy data
	skb->protocol = eth_type_trans(skb, priv->netdev);
	/* Using the napi_gro_receive() is the common modern way; we just stick
	 * to the simpler netif_receive_skb() for this demo..
	 * napi_gro_receive(napi, skb);
	 */
	netif_receive_skb(skb);
	napi_complete_done(napi, 1);

	return 1;
}

static int virtnet_open(struct net_device *dev)
{
	struct virtnic_pvt_data *priv = netdev_priv(dev);

	QP;
	napi_enable(&priv->napi);
	hrtimer_start(&priv->rx_timer, ns_to_ktime((ONE_MS) * 100),
			HRTIMER_MODE_REL);	// 100 ms

	netif_carrier_on(dev);
	netif_start_queue(dev);

	return 0;
}

static int virtnet_stop(struct net_device *dev)
{
	struct virtnic_pvt_data *priv = netdev_priv(dev);

	QP;
	netif_stop_queue(dev);
	netif_carrier_off(dev);
	hrtimer_cancel(&priv->rx_timer);
	napi_disable(&priv->napi);

	return 0;
}

// Do an 'ifconfig vnic0' to see the effect of this 'getstats' routine..
static struct net_device_stats ndstats;
static struct net_device_stats *virtnet_get_stats(struct net_device *dev)
{
	struct virtnic_pvt_data *priv = netdev_priv(dev);

	/* Take the BH variant as it disables bottom halves too.. this is as a
	 * Tx softirq (BH) can occur in the Tx method
	 */
	spin_lock_bh(&priv->lock);
	ndstats.rx_packets = priv->rxpktnum;
	ndstats.rx_bytes = priv->rx_bytes;
	ndstats.tx_packets = priv->txpktnum;
	ndstats.tx_bytes = priv->tx_bytes;
	spin_unlock_bh(&priv->lock);

	return &ndstats;
}

static void virtnet_tx_timeout(struct net_device *dev, unsigned int txq)
{
	pr_info("!! Tx timed out !!\n");
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void virtnet_poll_controller(struct net_device *dev)
{
#if 0
	struct virtnic_pvt_data *priv = netdev_priv(dev);

	my_interrupt(priv->irq, priv);
#endif
	QP;
}
#endif

static const struct net_device_ops virtnet_netdev_ops = {
	.ndo_open = virtnet_open,
	.ndo_stop = virtnet_stop,
	.ndo_get_stats = virtnet_get_stats,
	.ndo_start_xmit = virtnet_start_xmit,
	.ndo_tx_timeout = virtnet_tx_timeout,
	.ndo_validate_addr = eth_validate_addr,
#if 0
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = virtnet_poll_controller,
	/* to support netconsole & netpoll;
	 * can manually invoke the interrupt hdlr! */
#endif
#endif
};

static int virtnet_probe(struct faux_device *fdev)
{
	struct net_device *netdev = NULL;
	struct virtnic_pvt_data *priv;
	int res = 0;

	QP;
	/* Use best practice devres routine; also, devm_alloc_etherdev() will
	 * call ether_setup() internally
	 */
	netdev = devm_alloc_etherdev(&fdev->dev, sizeof(*priv));
	if (!netdev)
		return -ENOMEM;

	SET_NETDEV_DEV(netdev, &fdev->dev);
	priv = netdev_priv(netdev);
	priv->netdev = netdev;
	strscpy(netdev->name, INTF_NAME, sizeof(netdev->name));
	eth_hw_addr_random(netdev);

	/* keep the default flags, just add NOARP */
	netdev->flags |= IFF_NOARP;
	netdev->features |= NETIF_F_HW_CSUM;

	netdev->watchdog_timeo = 8 * HZ;
	spin_lock_init(&priv->lock);
	/* Initializing the netdev ops struct is essential; else, we Oops.. */
	netdev->netdev_ops = &virtnet_netdev_ops;
	faux_device_set_drvdata(fdev, priv);

	netif_napi_add(netdev, &priv->napi, pseudo_napi_poll);

	hrtimer_setup(&priv->rx_timer, pseudo_rx_timer_func, CLOCK_MONOTONIC,
		      HRTIMER_MODE_REL);

	res = register_netdev(netdev);
	if (res) {
		pr_alert("failed to register net device!\n");
		netif_napi_del(&priv->napi);
		return res;
	}
	pr_info("pseudo (virtnic) NIC registered, network interface name %s\n",
		INTF_NAME);

	return 0;
}

static void virtnet_remove(struct faux_device *fdev)
{
	struct virtnic_pvt_data *priv = faux_device_get_drvdata(fdev);
	struct net_device *netdev = priv->netdev;

	QP;
	unregister_netdev(netdev);
	netif_napi_del(&priv->napi);
	/* We don't need to do the typical
	 * free_netdev(netdev);
	 * as we used the managed alloc (devm_alloc_etherdev()) !
	 */
}

static const struct faux_device_ops virtnet_faux_ops = {
	.probe = virtnet_probe,
	.remove = virtnet_remove,
};

static struct faux_device *fdev;

static int __init virtnet_init(void)
{
	int res = 0;

	/*
	 * Here, we're simply 'manually' adding a simple virtual device via
	 * the 6.14+ faux bus!
	 */
	fdev = faux_device_create(KBUILD_MODNAME, NULL, &virtnet_faux_ops);
	if (!fdev)
		return -ENODEV;

	pr_info("%s loaded\n", KBUILD_MODNAME);
	return res;
}

static void __exit virtnet_exit(void)
{
	faux_device_destroy(fdev);
	pr_info("unloaded.\n");
}

module_init(virtnet_init);
module_exit(virtnet_exit);

MODULE_DESCRIPTION("Simple demo virtual ethernet (NIC) driver; allows a user \
app to 'transmit' a UDP packet (to local loopback only) and 'receive' a simple \
packet via hrtimer timeout");
MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_LICENSE("Dual MIT/GPL");
