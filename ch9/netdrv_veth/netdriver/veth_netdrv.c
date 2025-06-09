/*
 * Virtual Ethernet - veth - NIC driver.
 *
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * This version has a few deliberate bugs.
 * Run, test, catch and fix 'em!
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * This network driver creates a "virtual interface" named 'veth' on the system.
 *
 * When our user-land datagram socket app (talker_dgram) sends network packet(s)
 * to the interface, the "packet sniffing" in the transmit routine confirms this.
 * It checks for a UDP packet with 'our' port number, and on finding these
 * criteria match, it 'accepts' the packet to be 'transmitted'.
 * Of course, as there's no real h.w so this is just a learning exercise with a
 * very simple 'virtual ethernet' NIC...
 *
 * The 'tx' routine then peeks into the SKB, displaying it's vitals (we even show
 * the packet content, including the eth, IP, UDP headers and the data payload!).
 *
 * To try it out:
 * 1. cd <netdrv_veth>
 * 2. cd netdriver/
 * 3. ./run
 * 4. cd ../userspc
 * 5. ./runapp
 * ...
 * It should work.. watch the kernel log with 'journalctl -f -k'

 *---------------------------------------------------------------------------------
 * Sample output when a UDP packet with the right port# is detected in the Tx path:
[ ... ]
(added the emphasis)                 vvv                vvvvvvvvvv
buggy_veth_netdrv:vnet_start_xmit(): UDP pkt::src=21124 dest=54295 len=6400
                                     ^^^                ^^^^^^^^^^
buggy_veth_netdrv:vnet_start_xmit(): ////////////////////////// SKB_PEEK
                                skb ptr: ffff8e5e031b3b00
                                 len=59 truesize=768 users=1
                                 Offsets: mac_header:2 network_header:16 transport_header:36
                                 SKB packet pointers & offsets:
                                  headroom : head:ffff8e5e113c5a00 - data:ffff8e5e113c5a02 [   2 bytes]
                                  pkt data :                         data - tail: 61       [  59 bytes]
                                  tailroom :                                tail - end:192 [ 131 bytes]
buggy_veth_netdrv:vnet_start_xmit(): ////////////////////////
00000000: 00 00 48 0f 0e 0d 0a 02 48 0f 0e 0d 0a 02 08 00  ..H.....H.......
00000010: 45 00 00 2d 4a 4e 40 00 40 11 21 f2 0a 00 02 0f  E..-JN@.@.!.....
00000020: c0 a8 01 c9 84 52 17 d4 00 19 60 0c 68 65 79 2c  .....R....`.hey,
00000030: 20 76 65 74 68 2c 20 77 61 73 73 75 70 00 00 00   veth, wassup...
00000040: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00000050: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00000060: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00000070: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00000080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
00000090: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
000000a0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
000000b0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
[ ... ]
buggy_veth_netdrv:vnet_start_xmit(): UDP pkt::src=17408 dest=17152 len=9473
[ ... ]
 *---------------------------------------------------------------------------------
Analyzing the sample o/p :
   The to-be-Tx packet:
Len:       2       14                    20                   8           x
          +----------------------------------------------------------------------+
          |  |  Eth II Hdr  |         IPv4 Hdr         |  UDP Hdr   |  Data      |
          +----------------------------------------------------------------------+

                <--------------- Eth II header --------->
          <pad> <---MAC Addr --->
00000000: 00 00 48 0f 0e 0d 0a 02 48 0f 0e 0d 0a 02 08 00  ..H.....H.......
          <----------------- IP header -----------------|
00000010: 45 00 00 2d 4a 4e 40 00 40 11 21 f2 0a 00 02 0f  E..-JN@.@.!.....
          |--IP    -> <--- UDP hdr --- -----> <--- Data |
00000020: c0 a8 01 c9 84 52 17 d4 00 19 60 0c 68 65 79 2c  .....R....`.hey,
          | ------ data payload  -------------->
00000030: 20 76 65 74 68 2c 20 77 61 73 73 75 70 00 00 00   veth, wassup...
00000040: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
 [ ... ]
 *---------------------------------------------------------------------------------
 * Kaiwan N Billimoria
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__
#include "../veth_common.h"

struct stVnetIntfCtx {
	struct net_device *netdev;
	int txpktnum, rxpktnum;
	int tx_bytes, rx_bytes;
	unsigned int data_xform;
	spinlock_t lock;
};
static struct stVnetIntfCtx *gpstCtx;

/*
 * The Tx entry point.
 * Runs in process context.
 * To get to the Tx path, try:
 * - running our "custom" simple datagram app "talker_dgram"
 *   ./talker_dgram <IP addr> "<some msg>"
 * - 'ping -c1 10.10.1.x , with 'x' != IP addr (5, in our current setup;
 *   also but realize that ping won't actually work on a local interface).
 * Use a network analyzer (eg Wireshark) to see packets flowing across the interface..
*/
static int tx_pkt_count;	//, rx_pkt_count;
static int vnet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct iphdr *ip;
	struct udphdr *udph;
	struct stVnetIntfCtx *pstCtx = netdev_priv(ndev);

//#define SHOW_TIME_SPINLOCK_HELD
#undef SHOW_TIME_SPINLOCK_HELD
#ifdef SHOW_TIME_SPINLOCK_HELD
	u64 ts1, ts2;
#endif

	if (!skb) {		// paranoia!
		pr_alert("skb NULL!\n");
		return -EAGAIN;
	}
	//SKB_PEEK(skb);

/*
   The to-be-Tx packet:
Len:      2       14                    20                   8           x
         +----------------------------------------------------------------------+
         |  |  Eth II Hdr  |         IPv4 Hdr         |  UDP Hdr   |  Data      |
         +----------------------------------------------------------------------+
         ^  ^                                                            ^      ^
         |  |                                                            |      |
skb-> head data                                                        tail   end
*/

	/*---------Packet Filtering :) --------------*/
	/* If the outgoing packet is not of the UDP protocol, discard it */
	ip = ip_hdr(skb);
	if (ip->protocol != IPPROTO_UDP) {	// not UDP proto?
		pr_cont("x");
		//pr_debug("not UDP,disregarding pkt..\n");
		goto out_tx;
	}
	//SKB_PEEK(skb);

	/* If the outgoing packet (of protocol UDP) does not have destination port=54295,
	 * then it's not sent to our n/w interface via our talker_dgram app, so simply ignore it.
	 */
	udph = udp_hdr(skb);
	pr_debug("UDP pkt::src=%d dest=%d len=%u\n", ntohs(udph->source), ntohs(udph->dest),
		 udph->len);
	if (udph->dest != ntohs(PORTNUM))	// port # 54295
		goto out_tx;
	//------------------------------

	pr_info("ah, a UDP packet Tx via our app (dest port %d)\n", PORTNUM);
	SKB_PEEK(skb);

#if 0				// already seen with the SKB_PEEK()
	pr_debug("data payload:\n");	// it's after the Eth + IP + UDP headers
	print_hex_dump_bytes(" ", DUMP_PREFIX_OFFSET, skb->head + 16 + 20 + 8, skb->len);
#endif

	/* Update stat counters; ugly with the silly time delta calculation... */
	spin_lock(&pstCtx->lock);
#ifdef SHOW_TIME_SPINLOCK_HELD
	ts1 = ktime_get_real_ns();
#endif

	pstCtx->txpktnum = ++tx_pkt_count;
	pstCtx->tx_bytes += skb->len;

#ifdef SHOW_TIME_SPINLOCK_HELD
	ts2 = ktime_get_real_ns();
#endif
	spin_unlock(&pstCtx->lock);

#ifdef SHOW_TIME_SPINLOCK_HELD
	SHOW_DELTA(ts2, ts1);
#endif

#if 0
	pr_debug("Emulating Rx by artificially invoking vnet_rx() now...\n");
	vnet_rx((unsigned long)ndev, skb);
#endif
 out_tx:
	dev_kfree_skb(skb);
	return 0;
}

static int vnet_open(struct net_device *ndev)
{
	QP;

	netif_carrier_on(ndev);
	netif_start_queue(ndev);
	return 0;
}

static int vnet_stop(struct net_device *ndev)
{
	QP;
	netif_stop_queue(ndev);
	netif_carrier_off(ndev);
	return 0;
}

// Do an 'ifconfig veth' to see the effect of this 'getstats' routine..
static struct net_device_stats ndstats;
static struct net_device_stats *vnet_get_stats(struct net_device *ndev)
{
	struct stVnetIntfCtx *pstCtx = netdev_priv(ndev);

	spin_lock(&pstCtx->lock);
	ndstats.rx_packets = pstCtx->rxpktnum;
	ndstats.rx_bytes = pstCtx->rx_bytes;
	ndstats.tx_packets = pstCtx->txpktnum;
	ndstats.tx_bytes = pstCtx->tx_bytes;
	spin_unlock(&pstCtx->lock);

	return &ndstats;
}

static void vnet_tx_timeout(struct net_device *ndev, unsigned int txq)
{
	pr_info("!! Tx timed out !!\n");
}

static const struct net_device_ops vnet_netdev_ops = {
	.ndo_open = vnet_open,
	.ndo_stop = vnet_stop,
	.ndo_get_stats = vnet_get_stats,
//      .ndo_do_ioctl           = vnet_ioctl,
	.ndo_start_xmit = vnet_start_xmit,
	.ndo_tx_timeout = vnet_tx_timeout,
};

/*
FIXME: [  579.273860] Device 'vnet.0' does not have a release() function, it is broken and must be fixed.
*/
// ETH_ALEN is 6
static const u8 veth_MAC_addr[ETH_ALEN] = { 0x48, 0x0F, 0x0E, 0x0D, 0x0A, 0x02 };

static int vnet_probe(struct platform_device *pdev)
{
	struct net_device *ndev = NULL;
	int res = 0;

	QP;
	ndev = alloc_etherdev(sizeof(struct stVnetIntfCtx));
	if (!ndev) {
		pr_alert("alloc_etherdev failed!\n");
		return -ENOMEM;
	}
#if 0
	SET_NETDEV_DEV(ndev, &pdev->dev);
	/* we can't do this here, as the 'pdev->dev'
	   is a pointer to the 'device' structure, not the net_device ...
	   So, to make this work, (being able to lookup the net_device struct at will),
	   we *need* one global pointer to our "driver context"; this is 'gpstCtx' & is
	   initialized below.. */
#endif

	ether_setup(ndev);
	strscpy(ndev->name, INTF_NAME, strlen(INTF_NAME) + 1);
	dev_addr_set(ndev, veth_MAC_addr);
//	memcpy(ndev->dev_addr, (const void *)veth_MAC_addr, ETH_ALEN);
	//memcpy(ndev->dev_addr, (const void *)veth_MAC_addr, sizeof(veth_MAC_addr));

#if 0
	MAC addr..ndo methods:open xmit stop get_stats do_ioctl ? iomem addr irq
#endif
	if (!is_valid_ether_addr(ndev->dev_addr)) {
		pr_warn("%s: Invalid ethernet MAC address. Please set using ifconfig\n",
			 ndev->name);
	}

	/* keep the default flags, just add NOARP */
	ndev->flags |= IFF_NOARP;

	ndev->watchdog_timeo = 8 * HZ;
	spin_lock_init(&gpstCtx->lock);

	/* Initializing the netdev ops struct is essential; else, we Oops.. */
	ndev->netdev_ops = &vnet_netdev_ops;

	res = register_netdev(ndev);
	if (res) {
		pr_alert("failed to register net device!\n");
		goto out_regnetdev_fail;
	}
	gpstCtx->netdev = ndev;
	return 0;

 out_regnetdev_fail:
	free_netdev(ndev);
	return res;
}

static void vnet_remove(struct platform_device *pdev)
{
	struct net_device *ndev = gpstCtx->netdev;

	QP;
	unregister_netdev(ndev);
	free_netdev(ndev);
	//return 0;
}

/*
 * Setup a bare-bones platform device & associated driver.
 * Platform devices get bound to their driver simply on the basis of the 'name' field;
 * if they match, the driver core "binds" them, invoking the 'probe' routine. Conversely, the
 * 'remove' method is invoked at unload/shutdown.
 * Done here mainly so that we have a 'probe' method that will get invoked on it's registration.
 */
static struct platform_device veth0 = {
	.name = DRVNAME,
	.id = 0,
};

static struct platform_device *veth_platform_devices[] __initdata = {
	&veth0,
};

static struct platform_driver virtnet = {
	.probe = vnet_probe,
	.remove = vnet_remove,
	.driver = {
		   .name = DRVNAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init vnet_init(void)
{
	int res = 0;

	pr_debug("Initializing network driver...\n");
	gpstCtx = kzalloc(sizeof(struct stVnetIntfCtx), GFP_KERNEL);
	if (!gpstCtx)
		return -ENOMEM;

	res = platform_add_devices(veth_platform_devices, ARRAY_SIZE(veth_platform_devices));
	if (res) {
		pr_alert("platform_add_devices failed!\n");
		goto out_fail_pad;
	}

	res = platform_driver_register(&virtnet);
	if (res) {
		pr_alert("platform_driver_register failed!\n");
		goto out_fail_pdr;
	}
	// successful platform_driver_register() will cause the registered 'probe'
	// method to be invoked now..

	pr_info("loaded.\n");
	return res;

 out_fail_pdr:
	platform_driver_unregister(&virtnet);
	platform_device_unregister(&veth0);
 out_fail_pad:
	kfree(gpstCtx);
	return res;
}

static void __exit vnet_exit(void)
{
#if 0
	struct net_device *ndev = gpstCtx->netdev;

//      unregister_netdev (ndev);
//      free_netdev (ndev);
#endif
	platform_driver_unregister(&virtnet);
	platform_device_unregister(&veth0);
	kfree(gpstCtx);
	pr_info("unloaded.\n");
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_DESCRIPTION("Simple demo virtual ethernet (NIC) driver; allows a user \
app to transmit a UDP packet via this network driver");
MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_LICENSE("Dual MIT/GPL");
