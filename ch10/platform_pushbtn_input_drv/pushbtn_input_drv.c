/*
 * pushbtn_input_drv.c
 ***************************************************************
 * This program is part of the source code released for the book
 *  "Linux Device Drivers In Action"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Learn-Linux-Device-Drivers
 *
 * From: Ch 10 : Writing an Input Device Driver
 ****************************************************************
 * Brief Description:
 * Input (platform) driver for the PS2 Joystick Module Breakout device.
 * A very simple 'template' of sorts for an input driver for a very simple
 * platform device (it's merely a pushbutton (witha resisitor) attached to an
 * embedded board - like a Raspberry Pi or a TI Beagle Bone Black.
 *
 * For details, please refer the book, Ch 10.
 * (c) Kaiwan N Billimoria, kaiwanTECH
 * License: Dual MIT/GPL
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/of.h>		// of_* APIs (OF = Open Firmware!)
#include <linux/refcount.h>
#include <linux/version.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION
("Input (platform) driver for the PS2 Joystick Module Breakout device");

int dtdemo_platdev_probe(struct platform_device *pdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
int dtdemo_platdev_remove(struct platform_device *pdev);
#else
void dtdemo_platdev_remove(struct platform_device *pdev);
#endif

struct pushbtn_device {
	struct gpio_desc *gpio;
	struct input_dev *input;
	int irq;
	refcount_t irqcount;
};

static irqreturn_t key_irq_handler(int irq, void *dev_id)
{
	struct pushbtn_device *pushb = dev_id;
	int state;

	/* Read current GPIO state */
	state = gpiod_get_value_cansleep(pushb->gpio);
	pr_debug("irq:count=%u:btn-state=%d\n", refcount_read(&pushb->irqcount), state);

	/* Report key event (KEY_xxx from linux/input-event-codes.h) */
	input_report_key(pushb->input, KEY_ENTER, state);
	if (state == 0)		// on release
		input_sync(pushb->input);

	refcount_inc(&pushb->irqcount);

	return IRQ_HANDLED;
}

int dtdemo_platdev_probe(struct platform_device *pdev)
{
	struct pushbtn_device *pushb;
	struct device *dev = &pdev->dev;
	const char *prop = NULL;
	int len = 0, ret;

	dev_dbg(dev, "platform driver probe enter\n");

	pushb = devm_kzalloc(&pdev->dev, sizeof(*pushb), GFP_KERNEL);
	if (!pushb)
		return -ENOMEM;
	refcount_set(&pushb->irqcount, 1);

	/* Initialize the device, mapping I/O memory, registering the interrupt handlers. The
	 * bus infrastructure provides methods to get the addresses, interrupt numbers and
	 * other device-specific information
	 */

	/* Get GPIO descriptor from device tree 
	 *  property name before -gpio is what you use in devm_gpiod_get()
	 *  DT:
	 *  ...
	 *   pushb-gpio = <&gpio 21 0>;
	 */
	pushb->gpio = devm_gpiod_get(&pdev->dev, "pushb", GPIOD_IN);
	if (IS_ERR(pushb->gpio))
		return PTR_ERR(pushb->gpio);
	pushb->irq = gpiod_to_irq(pushb->gpio);
	pr_info("GPIO line mapped to IRQ line %d\n", pushb->irq);

	/* Just fyi, let's retrieve a property of the node by name, 'purpose' */
	if (pdev->dev.of_node) {
		prop = of_get_property(pdev->dev.of_node, "purpose", &len);
		if (!prop) {
			dev_warn(dev, "getting DT property 'purpose' failed\n");
			return -1;
		}
		dev_info(dev, "DT property 'purpose' = \"%s\" (len=%d)\n", prop, len);
	} else
		dev_warn(dev, "couldn't access DT node(s)\n");

	/* Setup as an input device */
	pushb->input = devm_input_allocate_device(&pdev->dev);
	if (!pushb->input)
		return -ENOMEM;

	pushb->input->name = "LDDIA: My PushButton";
	pushb->input->phys = "gpio-keys/input0";
	/* which events this device supports */
	input_set_capability(pushb->input, EV_KEY, KEY_ENTER);

	ret = devm_request_threaded_irq(&pdev->dev, pushb->irq,
					NULL, key_irq_handler,
					IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
					IRQF_ONESHOT, "pushbtn-simple", pushb);
	if (ret)
		return ret;

	/* Register input device */
	ret = input_register_device(pushb->input);
	if (ret)
		return ret;
	platform_set_drvdata(pdev, pushb);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
int dtdemo_platdev_remove(struct platform_device *pdev)
#else
void dtdemo_platdev_remove(struct platform_device *pdev)
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
	 */
	{.compatible = "input_demo,pushbtn_simple"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);
#endif

static struct platform_driver pushbtn_platform_input_driver = {
	.probe = dtdemo_platdev_probe,
	.remove = dtdemo_platdev_remove,
	.driver = {
		   .name = "pushbtn_simple",
		   /* platform driver name must
		    * EXACTLY match the DT 'compatible' property 'model' name
		    * - described in the DT [overlay] for the board - for
		    * binding to occur. Then, if installed, this module is
		    * loaded up and it's probe method invoked!
		    */
#ifdef CONFIG_OF
		   .of_match_table = my_of_ids,
#endif
		   .owner = THIS_MODULE,
	}
};
module_platform_driver(pushbtn_platform_input_driver);
