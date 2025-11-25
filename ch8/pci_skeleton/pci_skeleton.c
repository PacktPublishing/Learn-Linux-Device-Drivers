/*
 * ch9/pci-skel/Makefile
 * ***************************************************************
 * This program is part of the source code released for the book
 *  "Learn Linux Device Drivers"
 *  (c) Author: Kaiwan N Billimoria
 *  Publisher:  Packt
 *  GitHub repository:
 *  https://github.com/PacktPublishing/Learn-Linux-Device-Drivers
 *
 * From: Ch 9 : Writing a skeleton PCIe Driver
 ***************************************************************
 * Brief Description:
 * A very simple 'template' of sorts for a 'skeleton' PCIe device driver.
 * Do take the time and trouble to study it via the book's Ch 9; don't ignore
 * it's 'better' Makefile !
 * We attempt to also show how to use the PCI MSI-X interrupts model.
 * This module has been designed to run on the 6.12.17 Linux kernel.
 *
 * For details, please refer the book, Ch 9.
 */
#define pr_fmt(fmt) "%s:%s(): " fmt, KBUILD_MODNAME, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

MODULE_AUTHOR("Kaiwan N Billimoria");
MODULE_DESCRIPTION("a simple template for a PCIe device driver on Linux 6.12");
MODULE_LICENSE("Dual MIT/GPL");	// or whatever
MODULE_VERSION("0.2");

// Replace with your device's actual IDs
#define MY_VENDOR_ID  0x1234
#define MY_DEVICE_ID  0x5678
#define MEMORY_BAR	   0
#define NUM_MSIX_VECTORS   1	// request just one MSI-X vector for now

struct pci_skeleton_dev {
	void __iomem *bar_addr;
	resource_size_t bar_len;
	struct pci_dev *pdev;
	struct msix_entry msix_entries[NUM_MSIX_VECTORS];
};

// Interrupt handler
static irqreturn_t pci_skeleton_irq_handler(int irq, void *dev_id)
{
	const struct pci_skeleton_dev *dev = dev_id;
	/* We know that this line (above) results in a compiler warning:
	 * 'warning: unused variable ‘dev’ ...'
	 * We still keep the line for illustrative purposes..
	 */

	pr_info("IRQ %d triggered\n", irq);

	// ACK or clear interrupt on the device if necessary here

	return IRQ_HANDLED;
}

/*
 * The probe method! This being invoked - by the PCI bus driver - implies a new
 * PCI device has been inserted/discovered.
 *
 * @pdev: ptr to the PCI[e] device structure
 * @ent : ptr to the pci_device_id structure, in effect, the entry in the PCI
 *        device ID table that matched!
 */
static int pci_skeleton_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err;
	struct pci_skeleton_dev *dev;

	pr_info("probe() called for device %04x:%04x\n", pdev->vendor, pdev->device);

	err = pci_enable_device(pdev);
	if (err)
		/* Tip: in probe() error code paths, pl use the dev_err_probe();
		 * it has benefits, as explained in the text
		 */
		return dev_err_probe(&pdev->dev, err, "failed to enable the pci device\n");

	err = pcim_request_region(pdev, MEMORY_BAR, KBUILD_MODNAME);
	if (err) {
		dev_err_probe(&pdev->dev, err, "request PCI region BAR%d failed\n", MEMORY_BAR);
		goto disable_device;
	}

	pci_set_master(pdev);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		err = -ENOMEM;
		goto release_regions;
	}

	dev->pdev = pdev;
	pci_set_drvdata(pdev, dev);

	dev->bar_len = pci_resource_len(pdev, MEMORY_BAR);
	dev->bar_addr = pci_ioremap_bar(pdev, MEMORY_BAR);
	if (!dev->bar_addr) {
		err = PTR_ERR(dev->bar_addr);
		dev_err_probe(&pdev->dev, err, "remap BAR%d failed\n", MEMORY_BAR);
		goto free_dev;
	}

	// Interrupts: setup MSI-X
	dev->msix_entries[0].entry = 0;
	err = pci_enable_msix_range(pdev, dev->msix_entries, NUM_MSIX_VECTORS, NUM_MSIX_VECTORS);
	if (err < 0) {
		dev_err_probe(&pdev->dev, err, "enable MSI-X failed\n");
		goto unmap_bar;
	}

	err = request_irq(dev->msix_entries[0].vector, pci_skeleton_irq_handler,
			  0, KBUILD_MODNAME, dev);
	if (err) {
		dev_err_probe(&pdev->dev, err, "request IRQ# %d failed\n", dev->msix_entries[0].vector);
		goto disable_msix;
	}

	pr_info("PCI[e] device initialized with MSI-X vector %d\n", dev->msix_entries[0].vector);
	return 0;

 disable_msix:
	pci_disable_msix(pdev);
 unmap_bar:
	iounmap(dev->bar_addr);
 free_dev:
	kfree(dev);
 release_regions:
	pci_release_region(pdev, MEMORY_BAR);
 disable_device:
	pci_disable_device(pdev);
	return err;
}

static void pci_skeleton_remove(struct pci_dev *pdev)
{
	struct pci_skeleton_dev *dev = pci_get_drvdata(pdev);

	pr_info("\n");
	if (dev) {
		free_irq(dev->msix_entries[0].vector, dev);
		pci_disable_msix(pdev);

		if (dev->bar_addr)
			iounmap(dev->bar_addr);
		kfree(dev);
	}
	/*
	 * pci_release_region(pdev, MEMORY_BAR);
	 * This is commented out precisely as we used the *resource managed*
	 * ver of the API - pcim_request_region(); the kernel will thus take
	 * care of releasing it on driver removal or device detach.
	 */
	pci_disable_device(pdev);
}

/* TODO - add comments on usage of DT here */
static const struct pci_device_id pci_skeleton_ids[] = {
	{PCI_DEVICE(MY_VENDOR_ID, MY_DEVICE_ID)},
	// Add additional VID,PID pairs of devices that this driver can drive over here ...
	{0,}
};
MODULE_DEVICE_TABLE(pci, pci_skeleton_ids);

static struct pci_driver pci_skeleton_driver = {
	.name = KBUILD_MODNAME,
	.id_table = pci_skeleton_ids,
	.probe = pci_skeleton_probe,
	.remove = pci_skeleton_remove,
};

/*----------- Older style, now replaced by module_pci_driver() ------*/
static int __init pci_skeleton_init(void)
{
	pr_info("init\n");
	return pci_register_driver(&pci_skeleton_driver);
}

static void __exit pci_skeleton_exit(void)
{
	pr_info("exit\n");
	pci_unregister_driver(&pci_skeleton_driver);
}

module_init(pci_skeleton_init);
module_exit(pci_skeleton_exit);
/*--------------------------------------------------------------------*/
/*
 * Guess what: we can replace all these lines of code – the
 * module_{init|exit}() and the pci_skeleton_{init|exit}()
 * functions – with just one line of code:
 *	module_pci_driver(pci_skeleton_driver);
 *----------------------------------------------------------------------
 */
