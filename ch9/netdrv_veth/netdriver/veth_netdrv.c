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

struct veth_pvt_data {
	struct net_device *netdev;
	int txpktnum, rxpktnum;
	int tx_bytes, rx_bytes;
	unsigned int data_xform;
	spinlock_t lock;
};

/*
 * The Tx entry point.
 * Runs in process context.
 * To get to the Tx path, try:
 * - running our custom simple datagram "talker_dgram" userspace app
 *   ./talker_dgram <IP addr> "<some msg>"
 * - 'ping -c1 10.10.1.x , with 'x' != IP addr (5, in our current setup;
 *   also but realize that ping won't actually work on a local interface).
 * Use a network analyzer (eg tcpdump/Wireshark) to see packets flowing across
 * the interface!
 */
static int tx_pkt_count;
static int vnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	const struct iphdr *ip;
	const struct udphdr *udph;
	struct veth_pvt_data *vp = netdev_priv(dev);

	if (!skb) {		// paranoia!
		pr_alert("skb NULL!\n");
		return -EAGAIN;
	}
	//SKB_PEEK(skb);

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
	/* If the outgoing packet is not of the UDP protocol, discard it */
	ip = ip_hdr(skb);
	if (ip->protocol != IPPROTO_UDP) {
		pr_cont("x");
		//pr_debug("not UDP,disregarding pkt..\n");
		goto out_tx;
	}
	//SKB_PEEK(skb);

	/*
	 * If the outgoing (UDP protocol) packet does NOT have destination port=54295,
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

	/* Update stat counters */
	spin_lock(&vp->lock);
	vp->txpktnum = ++tx_pkt_count;
	vp->tx_bytes += skb->len;
	spin_unlock(&vp->lock);

#if 0
	pr_debug("Emulating Rx by artificially invoking vnet_rx() now...\n");
	vnet_rx((unsigned long)dev, skb);
#endif
 out_tx:
	dev_kfree_skb(skb);
	return 0;
}

static int vnet_open(struct net_device *dev)
{
	QP;

	netif_carrier_on(dev);
	netif_start_queue(dev);
	return 0;
}

static int vnet_stop(struct net_device *dev)
{
	QP;
	netif_stop_queue(dev);
	netif_carrier_off(dev);

	return 0;
}

// Do an 'ifconfig veth' to see the effect of this 'getstats' routine..
static struct net_device_stats ndstats;
static struct net_device_stats *vnet_get_stats(struct net_device *dev)
{
	struct veth_pvt_data *vp = netdev_priv(dev);

	spin_lock(&vp->lock);
	ndstats.rx_packets = vp->rxpktnum;
	ndstats.rx_bytes = vp->rx_bytes;
	ndstats.tx_packets = vp->txpktnum;
	ndstats.tx_bytes = vp->tx_bytes;
	spin_unlock(&vp->lock);

	return &ndstats;
}

static void vnet_tx_timeout(struct net_device *dev, unsigned int txq)
{
	pr_info("!! Tx timed out !!\n");
}

static const struct net_device_ops vnet_netdev_ops = {
	.ndo_open = vnet_open,
	.ndo_stop = vnet_stop,
	.ndo_get_stats = vnet_get_stats,
	.ndo_start_xmit = vnet_start_xmit,
	.ndo_tx_timeout = vnet_tx_timeout,
	.ndo_validate_addr = eth_validate_addr,
// TODO
//#ifdef CONFIG_NET_POLL_CONTROLLER
//	.ndo_poll_controller = vnet_poll_controller, 
           /* to support netconsole & netpoll; 
	    * can manually invoke the interrupt hdlr! */
//#endif
};

// ETH_ALEN is 6
static u8 veth_MAC_addr[ETH_ALEN] = { 0x48, 0x0F, 0x0E, 0x0D, 0x0A, 0x02 };

static int vnet_probe(struct platform_device *pdev)
{
	struct net_device *netdev = NULL;
	struct veth_pvt_data *vp;
	int res = 0;

	QP;
	netdev = devm_alloc_etherdev(&pdev->dev, sizeof (*vp));
	if (!netdev)
		return -ENOMEM;

	SET_NETDEV_DEV(netdev, &pdev->dev);
	vp = netdev_priv(netdev);
	ether_setup(netdev);
	strscpy(netdev->name, INTF_NAME, strlen(INTF_NAME) + 1);
	dev_addr_set(netdev, veth_MAC_addr);

#if 0
	MAC addr..ndo methods:open xmit stop get_stats do_ioctl ? iomem addr irq
#endif
	if (!is_valid_ether_addr(netdev->dev_addr)) {
		pr_debug("%s: Invalid ethernet MAC address. Please set using ifconfig\n",
			 netdev->name);
	}

	/* keep the default flags, just add NOARP */
	netdev->flags |= IFF_NOARP;

	netdev->watchdog_timeo = 8 * HZ;
	spin_lock_init(&vp->lock);
	/* Initializing the netdev ops struct is essential; else, we Oops.. */
	netdev->netdev_ops = &vnet_netdev_ops;
	platform_set_drvdata(pdev, vp);

	res = register_netdev(netdev);
	if (res) {
		pr_alert("failed to register net device!\n");
		return res;
	}
	vp->netdev = netdev;

	return 0;
}

static void vnet_remove(struct platform_device *pdev)
{
	struct veth_pvt_data *vp = platform_get_drvdata(pdev);
	struct net_device *netdev = vp->netdev;

	QP;
	unregister_netdev(netdev);
}

/*
 * FIXME: [  579.273860] Device 'veth_netdrv.0' does not have a release() function,
 * it is broken and must be fixed. See Documentation/core-api/kobject.rst.
...
When a reference is released, the call to kobject_put() will decrement the
201 reference count and, possibly, free the object. Note that kobject_init()
202 sets the reference count to one, so the code which sets up the kobject will
203 need to do a kobject_put() eventually to release that reference.
...
 * Specifying an explicit 'release' method as part of the platform device solves this.
 * We don't actually have to do anything here, it's automatic! The comments for
 * platform_device_put() explain this:
 * '... Free all memory associated with a platform device.  This function must
 * _only_ be externally called in error cases.  All other usage is a bug.'
 * So this will be done as part of the regular cleanup but does require us to
 * register the 'release' method!
 */
void release_myplatdev(struct device *dev)
{
	QP;
}

/*
 * Setup a bare-bones platform device & associated driver.
 * Platform devices get bound to their driver simply on the basis of the 'name' field;
 * if they match, the driver core "binds" them, invoking the 'probe' routine. Conversely, the
 * 'remove' method is invoked at unload/shutdown.
 * Done here mainly so that we have a 'probe' method that will get invoked on it's registration.
 */
static struct platform_device veth0 = {
	.name = KBUILD_MODNAME,
	.id = 0,
	.dev = {
		.release = release_myplatdev,
	},
};

static struct platform_device *veth_platform_devices[] __initdata = {
	&veth0,
};

static struct platform_driver virtnet = {
	.probe = vnet_probe,
	.remove = vnet_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
	},
};

static int __init vnet_init(void)
{
	int res = 0;

	pr_info("Initializing our simple network driver\n");

	/*
	 * Here, we're simply 'manually' adding a platform device (old-style)
	 * via the platform_add_devices() API, which is a wrapper over
	 * platform_device_register().
	 *
	 * This approach's considered a 'legacy'
	 * one and should not be generally used; in general, the modern model
	 * is to have discovery/enumeration of the device performed *outside*
	 * the driver. This is what's always done in the normal case - via the
	 * client driver registering with the appropriate bus driver, and so on...
	 * In the modern model, the driver specifies which devices it supports
	 * - it can drive - via the Device Tree (DT) on platforms that support it
	 * (or via ACPI)! In this (modern) approach,
	 * the DT devices are auto-generated as platform devices by the kernel
	 * at early boot.. 
	 *
	 * But here, in order to have a 'device' as required by the LDM,
	 * we merely and 'manually' instantiate a 'dummy' device - the platform device!
	 * We pretend it's our NIC and have fun 'driving' it.
	 */
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
	/* Successful platform_driver_register() will cause the registered 'probe'
	 * method to be invoked now..
	 */

	pr_info("loaded.\n");
	return res;

 out_fail_pdr:
	platform_driver_unregister(&virtnet);
	platform_device_unregister(&veth0);
 out_fail_pad:
	return res;
}

static void __exit vnet_exit(void)
{
	platform_driver_unregister(&virtnet);
	platform_device_unregister(&veth0);
	pr_info("unloaded.\n");
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_DESCRIPTION("Simple demo virtual ethernet (NIC) driver; allows a user \
app to 'transmit' (to local loopback only) a UDP packet via this network driver");
MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_LICENSE("Dual MIT/GPL");
