// SPDX-License-Identifier: GPL-2.0
/*
 * Simple Block Device Linux kernel module for modern >= 5.x Linux kernels.
 * Uses the blk-mq framework and supports only the request-based scheme.
 * This ver is designed to support only modern 6.5+ kernels; this allows us to
 * eliminate a huge amount of #ifdeffery... making the code smaller and much
 * more readable, at the cost of not supporting older distros and kernels.
 *
 * Practically:
 * 1. This out-of-tree demo block driver supports 6.5+; tested on
 * x86_64 Ubuntu 22.04, Ubuntu 24.04, 25.10, Debian Trixie.
 * Contains a minimum of code to create the most primitive block device.
 * This version's code is available here:
 * https://github.com/PacktPublishing/Learn-Linux-Device-Drivers/tree/main/ch9/sblkdev
 * The book (Ch 9) shows and teaches this ver only.
 *
 * 2. For a 'compat' version (Not this one, and closer to Shtepa's original), pl
 * see here:
 * https://github.com/PacktPublishing/Learn-Linux-Device-Drivers/tree/main/ch9/sblkdev_compat
 * It does support slightly older kernels and distros (5.10+); the tradeoff
 * of course is a whole lot of #ifdef...#else...#endif jugglery.
 * Original code credit: Sergei Shtepa:
 * https://github.com/CodeImp/sblkdev
 * Ref article: https://prog.world/linux-kernel-5-0-we-write-simple-block-device-under-blk-mq/
 * (Unfortunately, the original article is no longer available.)
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include "device.h"

/*
 * A module can create more than one block device.
 * The configuration of our block devices is implemented in the simplest way:
 * by hard-coding a variable, sblkdev_catalog:
 * format: name,size_in_sectors   (a sector is 512 bytes)
 */
static int sblkdev_major;
static LIST_HEAD(sblkdev_device_list);
// the sblkdev1 disk is of size 84 MB, slbkdev2 of size 8 MB
static char *sblkdev_catalog = "sblkdev1,172032;sblkdev2,16384";

static void sblkdev_remove_all(void)
{
	struct sblkdev_device *dev, *next;

	list_for_each_entry_safe(dev, next, &sblkdev_device_list, link) {
		list_del(&dev->link);
		sblkdev_remove(dev);
	}
}

/*
 * sblkdev_init() - Entry point 'init'.
 * --- Block driver Init step 0
 *
 * Executed when the module is loaded. Parses the catalog parameter and
 * creates block devices.
 */
static int __init sblkdev_init(void)
{
	int ret = 0;
	int inx = 0;
	char *catalog;
	char *next_token;
	char *token;
	size_t length;

	sblkdev_major = register_blkdev(sblkdev_major, KBUILD_MODNAME);
	if (sblkdev_major < 0) {
		pr_info("Unable to get major number\n");
		return sblkdev_major;
	}

	length = strlen(sblkdev_catalog);
	if ((length < 1) || (length > PAGE_SIZE)) {
		pr_info("Invalid 'catalog'\n");
		ret = -EINVAL;
		goto fail_unregister;
	}

	catalog = kzalloc(length + 1, GFP_KERNEL);
	if (!catalog) {
		ret = -ENOMEM;
		goto fail_unregister;
	}
	strscpy(catalog, sblkdev_catalog, length + 1);

	/*
	 * The 'catalog' module param / variable is of the form:
	 *  "sblkdev1,4096;sblkdev2,8192"
	 * Parse it
	 */
	next_token = catalog;
	while ((token = strsep(&next_token, ";"))) {
		struct sblkdev_device *dev;
		char *name;
		char *capacity;
		sector_t capacity_value;

		name = strsep(&token, ",");
		if (!name)
			continue;
		capacity = strsep(&token, ",");
		if (!capacity)
			continue;

		ret = kstrtoull(capacity, 10, &capacity_value);
		if (ret) {
			pr_err("disk '%s', failed to fetch valid 'capacity' "
				"value (%d), rolling back\n",
			       name, ret);
			sblkdev_remove_all();
			unregister_blkdev(sblkdev_major, KBUILD_MODNAME);
			goto out_free;
		}

		// Setup the new disk
		dev = sblkdev_add(sblkdev_major, inx, name, capacity_value);
		if (IS_ERR(dev)) {
			ret = PTR_ERR(dev);
			pr_err("creating '%s' failed (%d), rolling back\n",
			       name, ret);
			sblkdev_remove_all();
			unregister_blkdev(sblkdev_major, KBUILD_MODNAME);
			goto out_free;
		}

		list_add(&dev->link, &sblkdev_device_list);
		inx++;
	}
	pr_info("registered %d disks, major# %d\n", inx, sblkdev_major);
out_free:
	kfree(catalog);
	return ret;

fail_unregister:
	unregister_blkdev(sblkdev_major, KBUILD_MODNAME);
	return ret;
}

/*
 * sblkdev_exit() - Entry point 'exit'.
 *
 * Executed when the module is unloaded. Remove all block devices and cleanup
 * all resources.
 */
static void __exit sblkdev_exit(void)
{
	sblkdev_remove_all();
	unregister_blkdev(sblkdev_major, KBUILD_MODNAME);
	pr_info("unregistered\n");
}

module_init(sblkdev_init);
module_exit(sblkdev_exit);

MODULE_AUTHOR("Sergei Shtepa, Kaiwan NB");
#define DRIVER_DESC	"Simple request-based block driver Linux kernel module for modern 6.5+ blk-mq Linux kernels"
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
