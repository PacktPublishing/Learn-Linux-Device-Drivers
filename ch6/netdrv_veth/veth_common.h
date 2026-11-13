/*
 * veth_common.h
 * Common header for the 'Virtual Ethernet' "veth" network driver project
 */
#ifndef __VETH_COMMON_H__
#define __VETH_COMMON_H__

#define PORTNUM     54295 // the port users will be connecting to
#define INTF_NAME  "veth"

#ifdef __KERNEL__
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/debugfs.h>
#include <net/ip.h>

#include "../../convenient.h"

#define DUMP_PKTS	1
#define	Print_hex_dump_bytes(prefix_str, prefix, dataptr, len)    do { \
	if (DUMP_PKTS)  \
		print_hex_dump_bytes(prefix_str, prefix, dataptr, len); \
} while (0)

#define MAXPKTSZ	1500

/*
 * SKB_PEEK : glean info about the passed socket buffer, esp it's memory (n/w packet).
 * Ref: http://vger.kernel.org/~davem/skb_data.html
 * Notes:
 * - on 64-bit head, tail, end are offsets but not on 32-bit where they are actual
 * pointers (addr); so use the helper routines when accessing them
 * - %td gives ptr difference
 * the local var _skb aids in macro eval just once
 * Minor note: The printk o/p of print_hex_dump_bytes may not appear straight away
 * (on minicom) due to the KERN_ loglevel it's written at; use dmesg to see everything...
 */
#define SKB_PEEK(skb) do { \
	const struct sk_buff *_skb = (skb); \
	PRINT_CTX(); \
	pr_debug("////////////////////////// SKB_PEEK\n"   \
	"skb ptr: %px\n" \
	" len=%u truesize=%u users=%u\n" \
	" Offsets: mac_header:%u network_header:%u transport_header:%u\n" \
	" SKB packet pointers & offsets:\n" \
	"  headroom : head:%px - data:%px  [%4u bytes]\n"    \
	"  pkt data :                         data - tail:%3td        [%4u bytes]\n"  \
	"  tailroom :                                tail - end:%3u  [%4u bytes]\n", \
	_skb, \
	_skb->len, _skb->truesize, refcount_read(&_skb->users), \
	_skb->mac_header, _skb->network_header, _skb->transport_header, \
	_skb->head, _skb->data, \
	skb_headroom(_skb),                  /* headroom len */ \
	skb_tail_pointer(_skb) - _skb->head, /* tail offset */ \
	skb_headlen(_skb),                   /* linear pkt data len */ \
	skb_end_offset(_skb), \
	skb_tailroom(_skb)    \
	);    \
	pr_debug("////////////////////////\n");    \
	/* print_hex_dump_bytes() dumps at KERN_DEBUG if DEBUG is defined  */ \
	print_hex_dump_bytes(" ", DUMP_PREFIX_OFFSET, _skb->head, skb_end_offset(_skb)); \
} while (0)

#endif /* kernel */
#endif
