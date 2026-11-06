/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LDDIA_DEVICE_H__
#define __LDDIA_DEVICE_H__

#include <linux/module.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/device.h>
#include <linux/blk-mq.h>
#include <linux/list.h>
#include "convenient.h"

struct sblkdev_device {
	struct list_head link;
	sector_t capacity;		/* Device size in sectors */
	u8 *data;			/* The 'disk' data in virtual memory */
	struct blk_mq_tag_set tag_set;
	struct gendisk *disk;
};

struct sblkdev_device *sblkdev_add(int major, int minor, const char *name,
				  sector_t capacity);
void sblkdev_remove(struct sblkdev_device *dev);
#endif   /* #ifndef __LDDIA_DEVICE_H__ */
