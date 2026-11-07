// SPDX-License-Identifier: GPL-2.0
/*
 * One sblkdev 'disk': this source includes the disk setup/teardown, the
 * blk-mq data path (queue_rq) and the block_device_operations (control path).
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/hdreg.h>	/* struct hd_geometry */
#include <linux/cdrom.h>	/* CDROM_GET_CAPABILITY */
#include <linux/slab.h>
#include <linux/mm.h>		/* kvzalloc(), kvfree() */
#include <linux/string.h>
#include <linux/limits.h>
#include <linux/sizes.h>	/* SZ_1G */
#include <linux/version.h>
#include "device.h"
#include "convenient.h"		/* PRINT_CTX() */

#define SBLKDEV_SHOW_ADDN_DTL
/*
 * Let's set the upper bound for a disk (in 512-byte sectors) to the value
 * 1 GiB. This keeps the size below kvmalloc()'s limit of INT_MAX bytes, and
 * because we compare in sectors before converting to bytes, the shift below
 * can't overflow.
 */
#define SBLKDEV_MAX_SECTORS	((sector_t)SZ_1G >> SECTOR_SHIFT)

/*
 * Logical == physical block size. In-tree builds get it from Kconfig;
 * our out-of-tree driver may redefine the block size here.
 * Must be a power of 2 in the range [512, PAGE_SIZE] (checked at build time)
 */
#define SBLKDEV_BLOCK_SIZE	SECTOR_SIZE

/*
 * The actual I/O is performed here... cannot do anything that's blocking!
 */
static inline blk_status_t process_request(struct request *rq,
					unsigned int *nr_bytes)
{
	struct bio_vec bvec;
	struct req_iterator iter;
	struct sblkdev_device *sbdev = rq->q->queuedata; /* our driver context */
	loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT;
	loff_t dev_size = (sbdev->capacity << SECTOR_SHIFT);

	/*
	 * The request is nothing but a list of bios; a bio is a list of
	 * (page, offset, len) segments (struct bio_vec 's).
	 * In a 'real' driver you must map these for DMA and ring the
	 * hardware's doorbell.
	 * Here, we simply iterate over all the BIO's for this in-flight IO
	 * request - and each bio_vec within them - calculating the src or
	 * dest address and the length; we then perform a simple memcpy() (in
	 * lieu of DMA) to perform the actual IO, the read or write.
	 */

	/* We only support read/write */
	int rqst = req_op(rq);
	if (rqst != REQ_OP_READ && rqst != REQ_OP_WRITE)
		return BLK_STS_NOTSUPP;

	/* Iterate over the BIOs (and the bio_vec's of each BIO) */
	rq_for_each_segment(bvec, rq, iter) {
		unsigned int len = bvec.bv_len;
		void *buf = page_address(bvec.bv_page) + bvec.bv_offset;

		if (len > (dev_size - pos))
			return BLK_STS_IOERR;

		switch (rqst) {
		case REQ_OP_READ:
			memcpy(buf, sbdev->data + pos, len); /* READ */
			break;
		case REQ_OP_WRITE:
			memcpy(sbdev->data + pos, buf, len); /* WRITE */
			break;
		default:
			return BLK_STS_NOTSUPP;
		}
		pos += len;
		*nr_bytes += len;
	}

	return BLK_STS_OK;
}

/*
 * IMPORTANT:
 * This is where any new request from the block IO layer - in effect, from
 * blk-mq - is handled; this is the key driver 'request function'!
 * Akin to the "main() function of your driver's data path". It is called when
 * the block layer (blk-mq) has a request ready for hardware.
 *
 * It runs in the context that the I/O request was issued in: typically process
 * context - that of the userspace process/thread that invoked the I/O.
 * (Can run in softirq context as well. Our PRINT_CTX() macro reveals the
 * precise context.) Regardless, NO sleeping/blocking's allowed here...
 * (Tip: enable CONFIG_DEBUG_ATOMIC_SLEEP to catch such bugs.)
 */
static blk_status_t sblkdev_queue_rq(struct blk_mq_hw_ctx *hctx,
				const struct blk_mq_queue_data *bd)
{
	unsigned int nr_bytes = 0;
	blk_status_t status = BLK_STS_OK;
	struct request *rq = bd->rq; // the in-flight I/O request
	struct device *dev = disk_to_dev(rq->q->disk);
	int tag = rq->tag;
	sector_t sec = blk_rq_pos(rq);

	dev_dbg(dev, "<--- new IO req --->\n");
	PRINT_CTX();

	/* Saying it's a 'read' if not 'write' is a bit simplistic; we 
	 * nevertheless do so as this driver will only receive R/W requests
	 */
	dev_dbg(dev, "[tag %d] %s request from blk-mq\n",
		 tag, rq_data_dir(rq)?"write":"read");
#ifdef SBLKDEV_SHOW_ADDN_DTL
	dev_dbg(dev, "ADDN_DTL:\n"
		"# SW Qs = %d\n"
		"HCTX: cpumask = %*pbl ;"
		" queue depth (nr_tags) = %u\n"
		"last request? %s\n",
		hctx->nr_ctx,
		cpumask_pr_args(hctx->cpumask),
		hctx->tags->nr_tags,
		bd->last?"yes":"no");
	dev_dbg(dev, "hctx type: ");
	if (hctx->type == HCTX_TYPE_DEFAULT)
		dev_dbg(dev, " DEFAULT\n");
	else if (hctx->type == HCTX_TYPE_READ)
		dev_dbg(dev, " READ\n");
	else if (hctx->type == HCTX_TYPE_POLL)
		dev_dbg(dev, " POLL\n");
#endif

	blk_mq_start_request(rq);
	status = process_request(rq, &nr_bytes); // returns blk_status_t
	blk_mq_end_request(rq, status);
	/* Now the rq belongs to blk-mq again, so don't touch */

	dev_dbg(dev, "[tag %d]  processed: at %llu:%d (sector:#bytes) (stat=%d)\n",
		tag, sec, nr_bytes, status);
	if (status)
		dev_warn_ratelimited(dev, "request failed! (status=%d, errno=%d)\n",
			status, blk_status_to_errno(status));

	return BLK_STS_OK;
}

static const struct blk_mq_ops mq_ops = {
	.queue_rq = sblkdev_queue_rq, // our 'request function'!
};

static int sblkdev_open(struct gendisk *disk, blk_mode_t mode)
{
	struct sblkdev_device *sbdev = disk->private_data;
	struct device *dev = disk_to_dev(disk);

	if (!sbdev) {
		pr_err("Invalid disk private_data\n");
		return -ENXIO;
	}

	dev_dbg(dev, "Device was opened\n");
	PRINT_CTX();

	return 0;
}

static void sblkdev_release(struct gendisk *disk)
{
	struct sblkdev_device *sbdev = disk->private_data;
	struct device *dev = disk_to_dev(disk);

	if (!sbdev) {
		pr_err("Invalid disk private_data\n");
		return;
	}

	dev_dbg(dev, "Device was closed\n");
	PRINT_CTX();
}

/*
 * We don't really need to implement the ioctl here... this is as, from 6.6+,
 * the block core will itself handle it. In particular, it will invoke the
 * HDIO_GETGEO cmd ioctl (and compat_ioctl) before invoking the driver hook;
 * our 'getgeo' hook follows. So, this ioctl's a dummy, to just show who, if
 * anyone, invokes it (libblkid might).
 */
static int sblkdev_ioctl(struct block_device *bdev, blk_mode_t mode,
			unsigned int cmd, unsigned long arg)
{
	struct device *dev = disk_to_dev(bdev->bd_disk);

	dev_dbg(dev, "%s(): control command [0x%x] received\n", __func__, cmd);
	PRINT_CTX();
	return -ENOTTY;
}

/*
 * The disk geometry 'get' routine... invoked by fdisk and cousins (cfdisk,
 * sfdisk). Actually this routine is not required, just a 'dummy', as CHS
 * disk geometry is outdated anyway (we use LBA). We show the caller (via our
 * PRINT_CTX() macro) and populate the hd_geometry structure with typical
 * defaults, that's it.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 18, 0)
static int sblkdev_getgeo(struct gendisk *disk, struct hd_geometry *geo)
{
	struct sblkdev_device *sbdev = disk->private_data;
#else
static int sblkdev_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct sblkdev_device *sbdev = bdev->bd_disk->private_data;
#endif
	u64 cylinders = DIV_ROUND_UP_ULL(sbdev->capacity, 255 * 63);

	PRINT_CTX();
	geo->heads = 255;
	geo->sectors = 63;
	geo->cylinders = (unsigned short)min_t(u64, cylinders, USHRT_MAX);

	return 0;
}

/* Device management (or control) plane ops */
static const struct block_device_operations sbd_fops = {
	.owner        = THIS_MODULE,
	.open         = sblkdev_open,
	.release      = sblkdev_release,
	.getgeo	      = sblkdev_getgeo,
	.ioctl        = sblkdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = blkdev_compat_ptr_ioctl,
#endif
};

/*
 * sblkdev_remove() - Remove simple block device
 */
void sblkdev_remove(struct sblkdev_device *dev)
{
	del_gendisk(dev->disk);
	put_disk(dev->disk); // fyi, 5.14-5.19 used a blk_cleanup_disk() wrapper
	blk_mq_free_tag_set(&dev->tag_set);
	kvfree(dev->data);
	kfree(dev);
	pr_info("simple block device '%s' [%d:%d] was removed\n",
		dev->disk->disk_name, dev->disk->major, dev->disk->first_minor);
}

static inline int init_tag_set(struct blk_mq_tag_set *set, void *data)
{
	set->ops = &mq_ops;	/* block driver 'request function' setup */
	set->nr_hw_queues = 1;
	set->nr_maps = 1;
	set->queue_depth = 128;
	set->numa_node = NUMA_NO_NODE;
	/*
	 * No flags. In particular no BLK_MQ_F_STACKING (it's for drivers
	 * stacked on another blk-mq device, like dm-rq), and no
	 * BLK_MQ_F_BLOCKING (as our queue_rq never sleeps).
	 */
	set->flags = 0;
	set->cmd_size = 0;	/* additional bytes to alloc per request */
	set->driver_data = data;

	/* 'Alloc a tag set to be associated with one or more request queues */
	return blk_mq_alloc_tag_set(set);
}

/*
 * sblkdev_add() - Add simple block device
 * This function poses as an innocent but is really pretty large and important!
 */
struct sblkdev_device *sblkdev_add(int major, int minor, const char *name,
				  sector_t capacity)
{
	struct sblkdev_device *sbdev = NULL;
	int ret = 0;
	struct gendisk *disk;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	struct queue_limits lim = {
		.logical_block_size  = SECTOR_SIZE, // 512 bytes
		.physical_block_size = SBLKDEV_BLOCK_SIZE,
		.io_min              = SBLKDEV_BLOCK_SIZE,
	};
#endif

	//--- Block driver init step 0: validity checks
	BUILD_BUG_ON(SBLKDEV_BLOCK_SIZE < SECTOR_SIZE ||
		     SBLKDEV_BLOCK_SIZE > PAGE_SIZE);
	BUILD_BUG_ON(SBLKDEV_BLOCK_SIZE & (SBLKDEV_BLOCK_SIZE - 1));

	if (!capacity || capacity > SBLKDEV_MAX_SECTORS) {
		pr_err("disk '%s': invalid capacity value (%llu sectors); "
			"(valid: 1..%llu)\n",
			name, capacity, SBLKDEV_MAX_SECTORS);
		return ERR_PTR(-EINVAL);
	}

	//--- Block driver init step 1
	pr_info("add device '%s' capacity %llu sectors\n", name, capacity);

	sbdev = kzalloc(sizeof(struct sblkdev_device), GFP_KERNEL);
	if (!sbdev) {
		ret = -ENOMEM;
		goto fail;
	}

	INIT_LIST_HEAD(&sbdev->link);
	sbdev->capacity = capacity;
	sbdev->data = kvzalloc((size_t)capacity << SECTOR_SHIFT, GFP_KERNEL);
	if (!sbdev->data) {
		ret = -ENOMEM;
		goto fail_kfree;
	}

	/*--- Block driver init step 2 - tag set init; a critical part of block
	 * driver initialization when using the request-based approach.
	 */
	pr_info("Going via explicit request-based approach\n");
	ret = init_tag_set(&sbdev->tag_set, sbdev);
	if (ret) {
		pr_err("Failed to allocate tag set\n");
		goto fail_kvfree;
	}

	/*--- Block driver init step 3 - allocate the disk
	 * >=5.14: blk_mq_alloc_disk() is a kernel macro, a tiny wrapper over
	 * __blk_mq_alloc_disk().
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0)
	disk = blk_mq_alloc_disk(&sbdev->tag_set, sbdev);
#else
	disk = blk_mq_alloc_disk(&sbdev->tag_set, &lim, sbdev);
#endif
	if (IS_ERR(disk)) {
		ret = PTR_ERR(disk);
		pr_err("Failed to allocate disk (2)\n");
		goto fail_free_tag_set;
	}

	/*--- Block driver init step 4 - initialize the disk and make it live ---*/
	// Ok, we now have the 'disk'...
	sbdev->disk = disk;
	struct device *dev = disk_to_dev(disk);

	/* No partitions, just setup the disk as a single raw storage chunk */
	disk->flags |= GENHD_FL_NO_PART;

	/* If removable device:
	 * disk->flags |= GENHD_FL_REMOVABLE;
	 */
	disk->major = major;
	disk->first_minor = minor;
	disk->minors = 1;

	/* device management (or control) plane ops for the disk */
	disk->fops = &sbd_fops;
	disk->private_data = sbdev;

	snprintf(disk->disk_name, DISK_NAME_LEN, "%s", name);
	set_capacity(disk, sbdev->capacity);
	blk_queue_flag_set(QUEUE_FLAG_NOMERGES, disk->queue);

	ret = add_disk(disk);
	if (ret) {
		dev_err(dev, "Failed to add disk '%s'\n", disk->disk_name);
		goto fail_put_disk;
	}
	/*
	 * The disk's now live! It can be seen and used by userspace, via 'lsblk'
	 * or 'cat /proc/partitions' and can be used for IO.
	 */
	dev_info(dev, "Simple block device '%s' [%d:%d] was added\n",
			disk->disk_name, major, minor);

	return sbdev;

fail_put_disk:
	put_disk(sbdev->disk);
fail_free_tag_set:
	blk_mq_free_tag_set(&sbdev->tag_set);
fail_kvfree:
	kvfree(sbdev->data);
fail_kfree:
	kfree(sbdev);
fail:
	pr_err("Failed to add block device\n");

	return ERR_PTR(ret);
}
