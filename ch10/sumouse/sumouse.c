// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * sumouse.c - our simple USB HID boot proto mouse driver.
 * It began life as a copy of the original USB HID mouse driver:
 * drivers/hid/usbhid/usbmouse.c by Vojtech Pavlik. It's been tweaked
 * and slightly enhanced...
 *
 * Use our wrapper scripts - bind, restore, sumouse-bind.sh - to try out
 * this driver.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#define DRIVER_DESC	"Simple USB HID boot protocol mouse driver"

/* Mouse boot protocol 'layout' format; byte offsets in the layout 'report' */
#define SMOUSE_REPORT_MIN	3	/* buttons + X + Y: the boot minimum */
#define SMOUSE_OFF_BUTTONS	0
#define SMOUSE_OFF_X		1
#define SMOUSE_OFF_Y		2
#define SMOUSE_OFF_WHEEL	3	/* typically @ offset 3 */

/* Our driver private data or context structure; one instance is alloc'ed per
 * interface (that we bind to).
 */
struct usb_mouse {
	struct usb_device *usbdev;  /* phy device */
	struct usb_interface *intf; /* USB intf we're bound to */
	struct input_dev *input;      /* our facing toward the input core */
	struct urb *urb;            /* our interrupt-IN URB */

	u8 *databuf;    /* DMA-coherent xfer buffer; CPU view */
	dma_addr_t databuf_dma;     /* same buffer, host ctrlr view */
	u16 buflen;	/* buffer size == endpoint max pkt size */
	u8 ifnum;	/* bInterfaceNumber, for class requests */
	char name[128];
	char phys[64];
};

/*
 * The URB completion callback routine; invoked when the mouse has a report.
 * (How? the controller hw is continually polling it! So when there is a report,
 * the controller writes it ito databuf[] over DMA, then raises its hw interrupt;
 * thus it's driver - xhci/ohci/...-hcd - invokes our callback (with the status,
 * actual_length fields set)!
 */
static void usb_mouse_irq(struct urb *urb)
{
	struct usb_mouse *mouse = urb->context;
	u8 *databuf = mouse->databuf;
	struct input_dev *input = mouse->input;
	u8 buttons;
	s8 dx, dy, wheel;
	int status;

	switch (urb->status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}

	if (urb->actual_length < SMOUSE_REPORT_MIN)
		goto resubmit;
		
	/*
	 * databuf[] has the actual boot layout 'report', for actual_length
	 * bytes; interpret it as shown:
	 *          +--------+--------+--------+--------+
         *	    |  Wheel | Y moved| X moved|btn stat|
         *	    +--------+--------+--------+--------+
	 *  byte #       3        2        1        0
	 *  (our macros reflect this layout)
	 * Only bytes 0,1,2 are guaranteed with the boot protocol; but realistically,
	 * if actual_length is 4, the 4th byte is the mouse Scroll Wheel state
	 */
	buttons = databuf[SMOUSE_OFF_BUTTONS];
	dx = (s8)databuf[SMOUSE_OFF_X];
	dy = (s8)databuf[SMOUSE_OFF_Y];
	wheel = (urb->actual_length > SMOUSE_OFF_WHEEL) ?
		(s8)databuf[SMOUSE_OFF_WHEEL] : 0;

	/*
	 * Enable at runtime with dynamic debug (as root), e.g.
	 *   echo 'module sumouse +p' > /proc/dynamic_debug/control
	 * %*ph is the kernel's hex-dump printk format (up to 64 bytes).
	 * Careful: this printk fires on every report, i.e. up to 1000 times/sec,
	 * (which is why it's on only when DEBUG's defined or dyn-debug is used).
	 */
	dev_dbg(&input->dev, "report[%u] %*ph: L%d R%d M%d dx=%4d dy=%4d wheel=%2d\n",
		urb->actual_length,
		(int)min_t(u32, urb->actual_length, 8), databuf,
		!!(buttons & BIT(0)), !!(buttons & BIT(1)), !!(buttons & BIT(2)),
		dx, dy, wheel);

	input_report_key(input, BTN_LEFT,   buttons & 0x01);
	input_report_key(input, BTN_RIGHT,  buttons & 0x02);
	input_report_key(input, BTN_MIDDLE, buttons & 0x04);

	input_report_rel(input, REL_X,     dx);
	input_report_rel(input, REL_Y,     dy);
	input_report_rel(input, REL_WHEEL, wheel);

	input_sync(input);
resubmit:
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status)
		dev_err(&mouse->usbdev->dev,
			"can't resubmit intr, %s-%s/input0, status %d\n",
			mouse->usbdev->bus->bus_name,
			mouse->usbdev->devpath, status);
	/* URB re-submitted; the databuf again belongs to the controller,
	 * don't touch it...
	 */
}

static int usb_mouse_open(struct input_dev *input)
{
	struct usb_mouse *mouse = input_get_drvdata(input);

	dev_dbg(&input->dev, "in %s()\n", __func__);

	return usb_submit_urb(mouse->urb, GFP_KERNEL);
}

static void usb_mouse_close(struct input_dev *input)
{
	struct usb_mouse *mouse = input_get_drvdata(input);

	dev_dbg(&input->dev, "in %s()\n", __func__);
	usb_kill_urb(mouse->urb);
}

static void sumouse_free_urb(void *d)
{
	struct usb_mouse *m = d;

	dev_dbg(&m->input->dev, "in %s()\n", __func__);
	usb_kill_urb(m->urb); /* ensure the URB's idle */
	usb_free_urb(m->urb);
}

static void sumouse_free_buf(void *d)
{
	struct usb_mouse *m = d;

	dev_dbg(&m->input->dev, "in %s()\n", __func__);
	usb_free_coherent(m->usbdev, m->buflen, m->databuf, m->databuf_dma);
}

static int sumouse_set_protocol(struct usb_mouse *m, u16 proto)
{
	dev_dbg(&m->input->dev, "in %s()\n", __func__);
	return usb_control_msg(m->usbdev, usb_sndctrlpipe(m->usbdev, 0),
			       HID_REQ_SET_PROTOCOL,
			       USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			       proto, m->ifnum, NULL, 0, USB_CTRL_SET_TIMEOUT);
}

/*
 * Our probe method: we're here because usbcore - which encapsulates the usb
 bus type 'driver' - found an interface matching our id_table.

 */
static int sumouse_probe(struct usb_interface *intf,
			const struct usb_device_id *id)
{
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct device *dev = &intf->dev;
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	struct input_dev *input;
	int pipe, maxp;
	int ret;

	dev_dbg(dev, "in %s()\n", __func__);
	interface = intf->cur_altsetting;

	/*
	 * 1. We can't simply assume the device is 'correct'; malicious or buggy
	 * devices can claim to be a BP mouse when they aren't! So, validate!
	 * We expect 1 Interrupt IN endpoint.
	 * Wait: the original usbmouse driver seems to be too strict, checking
	 * for exactly 1 EP and that it (EP 0) is an interrupt-IN. On my native
	 * x86_64 laptop that didn't work! It showed 2 EPs... so let's relax
	 * the check just a tad...
	 */
	ret = usb_find_int_in_endpoint(interface, &endpoint);
	if (ret) {
		dev_err(dev, "no interrupt-IN endpoint\n");
		return -ENODEV;
	}

	pipe = usb_rcvintpipe(usbdev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(usbdev, pipe); // typically = 4
	if (maxp < SMOUSE_REPORT_MIN) {
		dev_err(dev, "max packets is < %d\n", SMOUSE_REPORT_MIN);
		return -ENODEV;
	}

	/*
	 * 2. We try and use devres mechanisms as far as possible...
	 * It's best practice, and allows us to skip the convoluted 'goto'
	 * ladder
	 */
	mouse = devm_kzalloc(dev, sizeof(struct usb_mouse), GFP_KERNEL);
	if (!mouse)
		return -ENOMEM;

	mouse->usbdev = usbdev;
	mouse->intf = intf;
	mouse->buflen = min(maxp, 8);
	mouse->ifnum = intf->cur_altsetting->desc.bInterfaceNumber;

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;
	mouse->input = input;

	/*
	 * 3. usb_alloc_coherent() and usb_alloc_urb() don't have devm_ versions;
	 * so we attach our own devm hooks to clean 'em up! (via the
	 * devm_add_action_or_reset()) 
	 */
	mouse->databuf = usb_alloc_coherent(usbdev, mouse->buflen, GFP_KERNEL,
					&mouse->databuf_dma);
	if (!mouse->databuf)
		return -ENOMEM;
	ret = devm_add_action_or_reset(dev, sumouse_free_buf, mouse);
	if (ret)
		return ret;
	
	mouse->urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->urb)
		return -ENOMEM;
	ret = devm_add_action_or_reset(dev, sumouse_free_urb, mouse);
	if (ret)
		return ret;

	/* 4. Describe ourself to the input core */
	if (usbdev->manufacturer)
		strscpy(mouse->name, usbdev->manufacturer, sizeof(mouse->name));

	if (usbdev->product) {
		if (usbdev->manufacturer)
			strlcat(mouse->name, " ", sizeof(mouse->name));
		strlcat(mouse->name, usbdev->product, sizeof(mouse->name));
	}

	if (!strlen(mouse->name))
		snprintf(mouse->name, sizeof(mouse->name),
			 "Simple USB HIDBP Mouse %04x:%04x",
			 le16_to_cpu(usbdev->descriptor.idVendor),
			 le16_to_cpu(usbdev->descriptor.idProduct));

	usb_make_path(usbdev, mouse->phys, sizeof(mouse->phys));
	strlcat(mouse->phys, "/input0", sizeof(mouse->phys));

	input->name = mouse->name;
	input->phys = mouse->phys;
	usb_to_input_id(usbdev, &input->id);
	input->dev.parent = &intf->dev;

	/*
	 * 5. Setup our input capabilities; these are the input events that we
	 * promise to generate. Userspace libs like libinput check these bitmaps
	 * to see 'is this a mouse?'.
	 * The input_set_capability() lets us set both the event type and event
	 * code at once.
	 */
	input_set_capability(input, EV_KEY, BTN_LEFT);
	input_set_capability(input, EV_KEY, BTN_RIGHT);
	input_set_capability(input, EV_KEY, BTN_MIDDLE);
	input_set_capability(input, EV_REL, REL_X);
	input_set_capability(input, EV_REL, REL_Y);
	input_set_capability(input, EV_REL, REL_WHEEL);

	input_set_drvdata(input, mouse);
	input->open = usb_mouse_open;
	input->close = usb_mouse_close;

	usb_fill_int_urb(mouse->urb, usbdev, pipe, mouse->databuf,
			 mouse->buflen, usb_mouse_irq, mouse,
			 endpoint->bInterval);
	mouse->urb->transfer_dma = mouse->databuf_dma;
	mouse->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;


	/*
	 * IMP: must set mouse in BP (boot protocol) mode! Else it can behave
	 * erratically...
	 */
	ret = sumouse_set_protocol(mouse, HID_BOOT_PROTOCOL);
	if (ret == -EPIPE) {
		/* STALL == unsupported error; it violates the spec; happens on
		 * VirtualBox! Will just Assume report layout is of boot proto
		 */
		dev_err(dev, "SET_PROTOCOL(boot) unsupported ('stall'): assuming boot layout\n");
	} else if (ret < 0) {
		dev_err(dev, "SET_PROTOCOL(boot) failed: %d\n", ret);
		return ret;
	}

	// Register with a kernel framework, here, input of course
	ret = input_register_device(input);
	if (ret) {
		sumouse_set_protocol(mouse, HID_REPORT_PROTOCOL);
		return ret;
	}

	usb_set_intfdata(intf, mouse);

	dev_info(dev,
	"%s: simple boot protocol mouse ready (ep 0x%02x, maxp %d, bInterval %u)\n",
	mouse->name, endpoint->bEndpointAddress, maxp, endpoint->bInterval);

	return 0;
}

static void sumouse_disconnect(struct usb_interface *intf)
{
	struct usb_mouse *mouse = usb_get_intfdata (intf);

	dev_dbg(&mouse->input->dev, "in %s()\n", __func__);
	usb_set_intfdata(intf, NULL);
	if (mouse) {
		input_unregister_device(mouse->input);
		sumouse_set_protocol(mouse, HID_REPORT_PROTOCOL); /* best effort */
	}
}

/*
 * Which interface(s) can we drive: we specify intf class as HID, subclass as
 * boot, and mouse boot protocol. This matches literally *all* USB HID
 * interfaces, the same as the original usbhid driver does! It (usbhid)
 * typically loads
 * before us and will thus drive our mouse. So, we'll have to first unbind it,
 * then bind our interface thus making this driver the boss! (Our wrapper
 * scripts - bind, restore - perform this work.)
 */
static const struct usb_device_id sumouse_id_table[] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
				USB_INTERFACE_SUBCLASS_BOOT,
				USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, sumouse_id_table);

static struct usb_driver usb_mouse_driver = {
	.name		= "sumouse",
	.probe		= sumouse_probe,
	.disconnect	= sumouse_disconnect,
	.id_table	= sumouse_id_table,
};

module_usb_driver(usb_mouse_driver);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
