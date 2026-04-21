// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/hdreg.h> /* for HDIO_GETGEO */
#include <linux/cdrom.h> /* for CDROM_GET_CAPABILITY */
#include <linux/version.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include "device.h"

#ifdef CONFIG_SBLKDEV_REQUESTS_BASED

static inline int process_request(struct request *rq, unsigned int *nr_bytes)
{
	int ret = BLK_STS_OK; // 0
	struct bio_vec bvec;
	struct req_iterator iter;
	struct sblkdev_device *dev = rq->q->queuedata;
	loff_t pos = blk_rq_pos(rq) << SECTOR_SHIFT;
	loff_t dev_size = (dev->capacity << SECTOR_SHIFT);

	/*
	 * The request contains a list of memory pages (bio_vec).
	 * In a 'real' driver you must map these to your device's DMA.
	 * Once ready, submit it to Hardware by writing the command to the
	 * device's "Doorbell" register or ring buffer.
	 * Here, we do iterate over all the bio's - and each bio_vec within
	 * them - calculating the src or dest address and the length; we then
	 * perform a simple memcpy() (in lieu of DMA) to perform the actual IO,
	 * the read or write.
	 */
	PRINT_CTX();
	rq_for_each_segment(bvec, rq, iter) {
		unsigned long len = bvec.bv_len;
		void *buf = page_address(bvec.bv_page) + bvec.bv_offset;

		if ((pos + len) > dev_size)
			len = (unsigned long)(dev_size - pos);

		if (rq_data_dir(rq))
			memcpy(dev->data + pos, buf, len); /* WRITE */
		else
			memcpy(buf, dev->data + pos, len); /* READ */

		pos += len;
		*nr_bytes += len;
	}

	return ret;
}

/*
 * IMPORTANT:
 * This is where any new request from block IO layer is handled; this is the
 * request queuing logic!
 * Akin to the "main() function of your driver's data path". It is called when
 * the block layer has a request ready for hardware.
 */
static blk_status_t sblkdev_queue_rq(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data *bd)
{
	unsigned int nr_bytes = 0;
	blk_status_t status = BLK_STS_OK;
	struct request *rq = bd->rq;

	pr_debug("new request from block IO layer queued\n");
	PRINT_CTX();
	//might_sleep();
	cant_sleep(); /* cannot use any locks that make the thread sleep */

	blk_mq_start_request(rq);

	if (process_request(rq, &nr_bytes))
		status = BLK_STS_IOERR;

	pr_debug("request %llu:%d (pos:#bytes) processed\n", blk_rq_pos(rq), nr_bytes);

	blk_mq_end_request(rq, status);

	return status;
}

static struct blk_mq_ops mq_ops = {
	.queue_rq = sblkdev_queue_rq,
};

#else  /* CONFIG_SBLKDEV_REQUESTS_BASED */

static inline void process_bio(struct sblkdev_device *dev, struct bio *bio)
{
	struct bio_vec bvec;
	struct bvec_iter iter;
	loff_t pos = bio->bi_iter.bi_sector << SECTOR_SHIFT;
	loff_t dev_size = (dev->capacity << SECTOR_SHIFT);
	unsigned long start_time;

	PRINT_CTX();
	start_time = bio_start_io_acct(bio);
	bio_for_each_segment(bvec, bio, iter) {
		unsigned int len = bvec.bv_len;
		void *buf = page_address(bvec.bv_page) + bvec.bv_offset;

		if ((pos + len) > dev_size) {
			/* len = (unsigned long)(dev_size - pos);*/
			bio->bi_status = BLK_STS_IOERR;
			break;
		}

		if (bio_data_dir(bio))
			memcpy(dev->data + pos, buf, len); /* WRITE */
		else
			memcpy(buf, dev->data + pos, len); /* READ */

		pos += len;
	}
	bio_end_io_acct(bio, start_time);
	bio_endio(bio);
}

#ifdef HAVE_QC_SUBMIT_BIO
blk_qc_t sblkdev_submit_bio(struct bio *bio)
{
	blk_qc_t ret = BLK_QC_T_NONE;
#else
void sblkdev_submit_bio(struct bio *bio)
{
#endif
#ifdef HAVE_BI_BDEV
	struct sblkdev_device *dev = bio->bi_bdev->bd_disk->private_data;
#endif
#ifdef HAVE_BI_BDISK
	struct sblkdev_device *dev = bio->bi_disk->private_data;
#endif

	PRINT_CTX();
	might_sleep();
	//cant_sleep(); /* cannot use any locks that make the thread sleep */

	process_bio(dev, bio);

#ifdef HAVE_QC_SUBMIT_BIO
	return ret;
}
#else
}
#endif

#endif /* CONFIG_SBLKDEV_REQUESTS_BASED */

#if KERNEL_VERSION(6, 5, 0) >= LINUX_VERSION_CODE
static int sblkdev_open(struct gendisk *disk, fmode_t mode)
#else
static int sblkdev_open(struct block_device *bdev, fmode_t mode)
#endif
{
#if KERNEL_VERSION(6, 5, 0) >= LINUX_VERSION_CODE
	struct sblkdev_device *dev = disk->private_data;
#else
	struct sblkdev_device *dev = bdev->bd_disk->private_data;
#endif

	if (!dev) {
		pr_err("Invalid disk private_data\n");
		return -ENXIO;
	}

	pr_debug("Device was opened\n");
	PRINT_CTX();

	return 0;
}

#if KERNEL_VERSION(6, 5, 0) >= LINUX_VERSION_CODE
static void sblkdev_release(struct gendisk *disk)
#else
static void sblkdev_release(struct gendisk *disk, fmode_t mode)
#endif
{
	struct sblkdev_device *dev = disk->private_data;

	if (!dev) {
		pr_err("Invalid disk private_data\n");
		return;
	}

	pr_debug("Device was closed\n");
	PRINT_CTX();
}

// get disk geometry
static inline int ioctl_hdio_getgeo(struct sblkdev_device *dev, unsigned long arg)
{
	struct hd_geometry geo = {0};

	PRINT_CTX();
	geo.start = 0;
	if (dev->capacity > 63) {
		sector_t quotient;

		geo.sectors = 63;
		quotient = (dev->capacity + (63 - 1)) / 63;

		if (quotient > 255) {
			geo.heads = 255;
			geo.cylinders = (unsigned short)
				((quotient + (255 - 1)) / 255);
		} else {
			geo.heads = (unsigned char)quotient;
			geo.cylinders = 1;
		}
	} else {
		geo.sectors = (unsigned char)dev->capacity;
		geo.cylinders = 1;
		geo.heads = 1;
	}

	if (copy_to_user((void *)arg, &geo, sizeof(geo)))
		return -EINVAL;

	return 0;
}

static int sblkdev_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct sblkdev_device *dev = bdev->bd_disk->private_data;

	pr_debug("contol command [0x%x] received\n", cmd);

	switch (cmd) {
	case HDIO_GETGEO:
		return ioctl_hdio_getgeo(dev, arg);
	case CDROM_GET_CAPABILITY:
		return -EINVAL;
	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
static int sblkdev_compat_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	// CONFIG_COMPAT is to allow running 32-bit userspace code on a 64-bit kernel
	return -ENOTTY; // not supported
}
#endif

static const struct block_device_operations fops = {
	.owner = THIS_MODULE,
	.open = sblkdev_open,
	.release = sblkdev_release,
	.ioctl = sblkdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sblkdev_compat_ioctl,
#endif
#ifndef CONFIG_SBLKDEV_REQUESTS_BASED
	.submit_bio = sblkdev_submit_bio,
#endif
};

/*
 * sblkdev_remove() - Remove simple block device
 */
void sblkdev_remove(struct sblkdev_device *dev)
{
	del_gendisk(dev->disk);

#ifdef HAVE_BLK_MQ_ALLOC_DISK
#ifdef HAVE_BLK_CLEANUP_DISK
	blk_cleanup_disk(dev->disk);
#else
	put_disk(dev->disk);
#endif
#else
	blk_cleanup_queue(dev->disk->queue);
	put_disk(dev->disk);
#endif

#ifdef CONFIG_SBLKDEV_REQUESTS_BASED
	blk_mq_free_tag_set(&dev->tag_set);
#endif
	kvfree(dev->data);
	kfree(dev);
	pr_info("simple block device was removed\n");
}

#ifdef CONFIG_SBLKDEV_REQUESTS_BASED
static inline int init_tag_set(struct blk_mq_tag_set *set, void *data)
{
	set->ops = &mq_ops;	// block driver behavior
	set->nr_hw_queues = 1;
	set->nr_maps = 1;
	set->queue_depth = 128;
	set->numa_node = NUMA_NO_NODE;
	set->flags = BLK_MQ_F_STACKING;
	//set->flags = BLK_MQ_F_SHOULD_MERGE | BLK_MQ_F_STACKING; // not on 6.14?

	set->cmd_size = 0;	// additional bytes to alloc per request
	set->driver_data = data;

	// 'Alloc a tag set to be associated with one or more request queues.'
	return blk_mq_alloc_tag_set(set);
}
#endif

#ifndef HAVE_BLK_MQ_ALLOC_DISK
static inline struct gendisk *my_blk_mq_alloc_disk(struct blk_mq_tag_set *set, struct queue_limits *lim,
						void *queuedata)
{
	struct gendisk *disk;
	struct request_queue *queue;

	queue = blk_mq_init_queue(set);
	if (IS_ERR(queue)) {
		pr_err("Failed to allocate queue\n");
		return ERR_PTR(PTR_ERR(queue));
	}

	queue->queuedata = queuedata;
	blk_queue_bounce_limit(queue, BLK_BOUNCE_HIGH);

	disk = alloc_disk(1);
	if (!disk)
		pr_err("Failed to allocate disk\n");
	else
		disk->queue = queue;

	return disk;
}
#endif

#ifndef HAVE_BLK_ALLOC_DISK
static inline struct gendisk *blk_alloc_disk(int node)
{
	struct request_queue *q;
	struct gendisk *disk;

	q = blk_alloc_queue(node);
	if (!q)
		return NULL;

	disk = __alloc_disk_node(0, node);
	if (!disk) {
		blk_cleanup_queue(q);
		return NULL;
	}
	disk->queue = q;

	return disk;
}
#endif

/*
 * sblkdev_add() - Add simple block device
 */
struct sblkdev_device *sblkdev_add(int major, int minor, char *name,
				  sector_t capacity)
{
	struct sblkdev_device *dev = NULL;
	int ret = 0;
	struct gendisk *disk;

	pr_info("add device '%s' capacity %llu sectors\n", name, capacity);

	dev = kzalloc(sizeof(struct sblkdev_device), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto fail;
	}

	INIT_LIST_HEAD(&dev->link);
	dev->capacity = capacity;
	dev->data = kvzalloc(capacity << SECTOR_SHIFT, GFP_KERNEL);
	if (!dev->data) {
		ret = -ENOMEM;
		goto fail_kfree;
	}

#ifdef CONFIG_SBLKDEV_REQUESTS_BASED
	// >= 6.8: this seems to be the default approach
	pr_info("Going via explicit (longer) request-based approach\n");
	ret = init_tag_set(&dev->tag_set, dev);
	if (ret) {
		pr_err("Failed to allocate tag set\n");
		goto fail_kvfree;
	}

	/* >=5.14: blk_mq_alloc_disk() is a kernel macro, a wrapper over
	 * blk_mq_alloc_queue() and __alloc_disk_node().
	 * If < 5.14 we have our own implementation of this func...
	 */
	disk = blk_mq_alloc_disk(&dev->tag_set, NULL, dev);
	if (unlikely(!disk)) {
		ret = -ENOMEM;
		pr_err("Failed to allocate disk (1)\n");
		goto fail_free_tag_set;
	}
	if (IS_ERR(disk)) {
		ret = PTR_ERR(disk);
		pr_err("Failed to allocate disk (2)\n");
		goto fail_free_tag_set;
	}

#else
	pr_info("Going via simpler blk_alloc_queue() and __alloc_disk_node() method\n");
	/* blk_alloc_disk() is actually a simpler way - wrapper - to get
	 * the same behavior as above via init_tag_set(), blk_mq_init_queue() &
	 * alloc_disk() (via our blk_mq_alloc_disk() function)
	 */
	disk = blk_alloc_disk(NUMA_NO_NODE);
	if (!disk) {
		pr_err("Failed to allocate disk\n");
		ret = -ENOMEM;
		goto fail_kvfree;
	}
#endif
	/* Ok, we have the 'disk'; init it ... */
	dev->disk = disk;

	/* only one partition */
#ifdef GENHD_FL_NO_PART_SCAN
	disk->flags |= GENHD_FL_NO_PART_SCAN;
#else
	disk->flags |= GENHD_FL_NO_PART;
#endif

	/* removable device */
	/* disk->flags |= GENHD_FL_REMOVABLE; */

	disk->major = major;
	disk->first_minor = minor;
	disk->minors = 1;

	disk->fops = &fops;	// blk device ops
	disk->private_data = dev;

	sprintf(disk->disk_name, name);
	set_capacity(disk, dev->capacity);

#ifdef CONFIG_SBLKDEV_BLOCK_SIZE
	// true: set to 512
	blk_queue_physical_block_size(disk->queue, CONFIG_SBLKDEV_BLOCK_SIZE);
	blk_queue_logical_block_size(disk->queue, CONFIG_SBLKDEV_BLOCK_SIZE);
	blk_queue_io_min(disk->queue, CONFIG_SBLKDEV_BLOCK_SIZE);
	blk_queue_io_opt(disk->queue, CONFIG_SBLKDEV_BLOCK_SIZE);
#else
	queue_physical_block_size(disk->queue);
	//blk_queue_physical_block_size(disk->queue, SECTOR_SIZE);  // not on 6.14?
	queue_logical_block_size(disk->queue);
	//blk_queue_logical_block_size(disk->queue, SECTOR_SIZE);   // not on 6.14?
#endif
	queue_max_hw_sectors(disk->queue);
	//blk_queue_max_hw_sectors(disk->queue, BLK_SAFE_MAX_SECTORS);   // not on 6.14?
	blk_queue_flag_set(QUEUE_FLAG_NOMERGES, disk->queue);


	// Add the disk; makes it 'live'
#ifdef HAVE_ADD_DISK_RESULT
	ret = add_disk(disk);
	if (ret) {
		pr_err("Failed to add disk '%s'\n", disk->disk_name);
		goto fail_put_disk;
	}
#else
	add_disk(disk);
#endif

	pr_info("Simple block device [%d:%d] was added\n", major, minor);

	return dev;

#ifdef HAVE_ADD_DISK_RESULT
fail_put_disk:
#ifdef HAVE_BLK_MQ_ALLOC_DISK
#ifdef HAVE_BLK_CLEANUP_DISK
	blk_cleanup_disk(dev->disk);
#else
	put_disk(dev->disk);
#endif
#else
	blk_cleanup_queue(dev->queue);
	put_disk(dev->disk);
#endif
#endif /* HAVE_ADD_DISK_RESULT */

#ifdef CONFIG_SBLKDEV_REQUESTS_BASED
fail_free_tag_set:
	blk_mq_free_tag_set(&dev->tag_set);
#endif
fail_kvfree:
	kvfree(dev->data);
fail_kfree:
	kfree(dev);
fail:
	pr_err("Failed to add block device\n");

	return ERR_PTR(ret);
}
