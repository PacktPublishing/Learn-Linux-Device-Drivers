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
 * Input (platform) driver for a simple GPIO pushbutton.
 * We wire a GPIO line from the embedded target board - here it's a TI BeagleBone
 * Black - across a 1k resistor and a simple pushbutton switch (to which 3.3V
 * power is applied, and thus the circuit gets made when the button’s pressed
 * down) on a breadboard.
 * This causes an interrupt (IRQ) to be raised! How? As we map the GPIO line to
 * an IRQ line; please carefully refer the Device Tree Overlay source file
 * (bbb_dtoverlay/gpio_btn_bbb.dts, and this driver code) to see how exactly
 * this is achieved.
 * Wiring:
 * - The board P9 (left) header’s VDD 3.3 V - physical pin 4 – for power (red
 *   color wire), and
 * - The board P9 header’s GPIO_49 – physical pin 23 – as the input GPIO to the
 *   pushbutton (orange+yellow color wire).
 * (Refer the schematic and photo in the book - Figures 10.5 and 10.6.)
 *
 * _Secure probe_: care is taken to validate DT compatible string(s), properties,
 * check and report function errors, etc.
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
#include <linux/of.h>		// of_* APIs (OF = Open Firmware)
#include <linux/refcount.h>
#include <linux/of_device.h>
#include <linux/version.h>

int input_pushbtn_platdev_probe(struct platform_device *pdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
int input_pushbtn_platdev_remove(struct platform_device *pdev);
#else
void input_pushbtn_platdev_remove(struct platform_device *pdev);
#endif

struct pushbtn_device {
	struct gpio_desc *gpio;
	struct input_dev *input;
	int irq;
	refcount_t irqcount;
};
static const struct of_device_id my_of_ids[];
/*
 * Which key or button to emit on our pushbutton press & release.
 * You can change this to any key or button you like!
 * (We don't do this via a module parameter as that will require
 * a wrapper script to parse the key or btn macro as an integer
 * and then pass it (we do this kind of thing in a later USB input
 * driver; keep an eye out!)
 */
static int key_or_btn = KEY_ENTER;

static irqreturn_t key_irq_handler(int irq, void *dev_id)
{
	struct pushbtn_device *pushb = dev_id;
	struct device *dev = &pushb->input->dev;
	int state;

	/* Read current GPIO state */
	state = gpiod_get_value(pushb->gpio);
	/*
	 * Alternately, we can also do so in a bloking manner with
	 *  state = gpiod_get_value_cansleep(pushb->gpio);
	 */
	dev_dbg(dev, "irq:count=%u:btn-state=%d\n",
		refcount_read(&pushb->irqcount), state);

	/* Report key event (KEY_xxx from linux/input-event-codes.h) */
	input_report_key(pushb->input, key_or_btn, state);
	if (state == 0)		// sync only on key/btn on release
		input_sync(pushb->input);

	refcount_inc(&pushb->irqcount);

	return IRQ_HANDLED;
}

int input_pushbtn_platdev_probe(struct platform_device *pdev)
{
	struct pushbtn_device *pushb;
	struct device *dev = &pdev->dev;
	const struct of_device_id *match; // security: explicit match validation
	const char *prop = NULL;
	int len = 0, ret;

	dev_dbg(dev, "platform input driver probe enter\n");

	match =	of_match_device(my_of_ids, dev);
	if (!match)
		dev_err_probe(dev, -ENODEV, "error matching compatible string\n");

	pushb = devm_kzalloc(&pdev->dev, sizeof(*pushb), GFP_KERNEL);
	if (!pushb)
		return -ENOMEM;

	/* Initialize the device, mapping I/O memory, registering the interrupt
	 * handlers. The bus infrastructure provides methods to get the
	 * addresses, interrupt numbers and other device-specific information
	 */

	/* Get GPIO descriptor from device tree
	 *  property name before -gpio is what you use in devm_gpiod_get()
	 *  DT:
	 *  ...
	 *	pushbtn-gpios = <&gpio1 17 GPIO_ACTIVE_LOW>;
	 * ref: https://elixir.bootlin.com/linux/v6.12.17/source/Documentation/devicetree/bindings/gpio/gpio.txt
	 */
	pushb->gpio = devm_gpiod_get(&pdev->dev, "pushbtn", GPIOD_IN);
	if (IS_ERR(pushb->gpio))
		dev_err_probe(dev, PTR_ERR(pushb->gpio), "Failed at devm_gpiod_get()\n");

	/* Map to IRQ line */
	pushb->irq = gpiod_to_irq(pushb->gpio);
	if (pushb->irq < 0)
		dev_err_probe(dev, pushb->irq, "failed at gpiod_to_irq()\n");
	dev_info(dev, "GPIO line mapped to IRQ line %d\n", pushb->irq);

	/* Register the IRQ via a threaded handler */
	ret = devm_request_threaded_irq(&pdev->dev, pushb->irq,
					NULL, key_irq_handler,
					IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
					IRQF_ONESHOT, "pushbtn-simple", pushb);
	if (ret)
		dev_err_probe(dev, ret, "failed at devm_request_threaded_irq()\n");

	/* Just fyi, let's retrieve the 'purpose' property by name */
	if (pdev->dev.of_node) {
		prop = of_get_property(pdev->dev.of_node, "purpose", &len);
		if (!prop)
			dev_warn(dev, "getting DT property 'purpose' failed\n");
		else
			dev_info(dev, "DT property 'purpose' = \"%s\" (len=%d)\n",
				prop, len);
	} else
		dev_warn(dev, "couldn't access DT 'purpose' node\n");

	/* Setup as an input device */
	pushb->input = devm_input_allocate_device(&pdev->dev);
	if (!pushb->input)
		dev_err_probe(dev, -ENOMEM, "failed at devm_input_allocate_device()\n");

	pushb->input->name = "LDDIA: GPIO PushButton";
	pushb->input->phys = "pushbtn_simple/input0";
	/* Which events this device supports */
	input_set_capability(pushb->input, EV_KEY, key_or_btn);
	/*
	 * Can also achieve setting the input dev capabilities (the above) via:
	 *	pushb->input->evbit[0] = BIT_MASK(EV_KEY);
	 *	pushb->input->keybit[BIT_WORD(key_or_btn)] = BIT_MASK(key_or_btn);
	 * or like this:
	 * 	set_bit(EV_KEY, pushb->input->evbit);
	 * 	set_bit(key_or_btn, pushb->input->keybit);
	 */

	/* Register input device */
	ret = input_register_device(pushb->input);
	if (ret)
		dev_err_probe(dev, ret, "failed at input_register_device()\n");
	platform_set_drvdata(pdev, pushb);
	refcount_set(&pushb->irqcount, 1);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
int input_pushbtn_platdev_remove(struct platform_device *pdev)
#else
void input_pushbtn_platdev_remove(struct platform_device *pdev)
#endif
{
	struct device *dev = &pdev->dev;
	struct pushbtn_device *pushb = platform_get_drvdata(pdev);

	dev_dbg(dev, "in platform driver remove method:\n"
		"# irq or input events = %u\n", refcount_read(&pushb->irqcount));

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
	{ .compatible = "lddia,pushbtn_simple" },
	{},
};
MODULE_DEVICE_TABLE(of, my_of_ids);
#endif

static struct platform_driver pushbtn_platform_input_driver = {
	.probe = input_pushbtn_platdev_probe,
	.remove = input_pushbtn_platdev_remove,
	.driver = {
		   .name = "pushbtn_simple",
		   /* platform driver name must
		    * EXACTLY match the DT 'compatible' property 'model' name
		    * - described in the DT [overlay] for the board - for
		    * binding to occur. Then, if installed, this module is
		    * loaded up and it's probe method invoked!
		    */
#ifdef CONFIG_OF
		   .of_match_table = of_match_ptr(my_of_ids),
#endif
		   .owner = THIS_MODULE,
	},
};
module_platform_driver(pushbtn_platform_input_driver);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("Input (platform) driver for a simple GPIO pushbutton");
