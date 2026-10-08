/*
 * dtdemo_platdrv.c
 *
 * Here, we have a platform device driver for a pseudo non-existant platform
 * device! It's just to demonstrate that:
 * a) we can rig up a device via the DT, whether or not it's 'real', and
 * b) whatever OF properties we defined in it's DT node can be retrieved and
 *    displayed in here, the platform driver!
 *
 * Kaiwan N Billimoria, kaiwanTECH
 * License: DUal MIT/GPL
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/of.h>		// of_* APIs (OF = Open Firmware)
#include <linux/of_device.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
	("Demo: setting up a DT node, so that this demo platform driver can get bound");

static const struct of_device_id my_of_ids[];

static int dtdemo_platdev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *match;	// security: explicit match validation
	const char *prop = NULL;
	s32 myval;
	int len = 0;

	dev_dbg(dev, "platform driver probe enter\n");

	match = of_match_device(my_of_ids, dev);
	if (!match)
		return dev_err_probe(dev, -ENODEV, "error: device not matched!\n");

	/*  So, let's retrieve the property of the node by name, 'aproperty' */
	if (pdev->dev.of_node) {
		prop = of_get_property(pdev->dev.of_node, "aproperty", &len);
		if (!prop)
			dev_warn(dev, "getting DT property 'aproperty' failed\n");
		else
			dev_info(dev, "DT property 'aproperty' = \"%s\" (len=%d)\n", prop,
				 len);

		// Note that the ret value isn't the property value..(the 3rd param is)
		len = of_property_read_s32(pdev->dev.of_node, "my_value", &myval);
		if (len < 0)
			dev_warn(dev, "getting DT property 'my_value' failed\n");
		else
			dev_info(dev, "DT property 'my_value' = %d\n", myval);
	} else
		return dev_err_probe(dev, -ENODEV, "error fetching OF node\n");

	/* Initialize the device, mapping I/O memory, registering the interrupt handlers. The
	 * bus infrastructure provides methods to get the addresses, interrupt numbers and
	 * other device-specific information...
	 */

	// Register the device to the proper kernel framework
	// eg. register_netdev(...);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int dtdemo_platdev_remove(struct platform_device *pdev)
#else
static void dtdemo_platdev_remove(struct platform_device *pdev)
#endif
{
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "platform driver remove\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
	return 0;
#endif
}

#ifdef CONFIG_OF
static const struct of_device_id my_of_ids[] = {
	/*
	 * DT compatible property syntax: <manufacturer,model> ...
	 * Can have multiple pairs of <oem,model>, from most specific to most general.
	 * This is especially important: it MUST EXACTLY match the 'compatible'
	 * property in the DT; *even a mismatched space will cause the match to
	 * fail* !
	 * Well, there's more to this; in reality, the kernel's platform_match()
	 * tries in this order: first driver_override, then OF, then ACPI, then
	 * id_table, and only then name matching as the last fallback. With our
	 * my_platform_driver of_match_table member being set, the
	 * compatible string is what takes effect.
	 */
	{.compatible = "lddia,dtdemo_platdev"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);
#endif

static struct platform_driver my_platform_driver = {
	.probe = dtdemo_platdev_probe,
	.remove = dtdemo_platdev_remove,
	.driver = {
		   .name = "dtdemo_platdev",
		   /* platform driver name; can be overriden (by .driver_override),
		    * OF, ACPI, id_table, and then this .name is checked.
		    */
#ifdef CONFIG_OF
		   .of_match_table = my_of_ids,
#endif
		   .owner = THIS_MODULE,
	}
};

#if 1
module_platform_driver(my_platform_driver);
#else
/*
 * Have deliberately kept the (commented) code below to show that, by using the
 * module_platform_driver() macro, we can save ourselves writing all
 * this code!
 */
static int dtdemo_platdrv_init(void)
{
	int ret_val;

	pr_info("inserted\n");

	// Register ourself to the platform bus, as we're a platform device
	ret_val = platform_driver_register(&my_platform_driver);
	if (ret_val != 0) {
		pr_err("platform value returned %d\n", ret_val);
		return ret_val;
	}
	return 0;		/* success */
}

static void dtdemo_platdrv_cleanup(void)
{
	platform_driver_unregister(&my_platform_driver);
	pr_info("removed\n");
}

module_init(dtdemo_platdrv_init);
module_exit(dtdemo_platdrv_cleanup);
#endif
