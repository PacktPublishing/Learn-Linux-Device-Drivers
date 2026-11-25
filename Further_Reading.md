# Further Reading
**For the book "Linux Device Drivers in Action", Kaiwan N Billimoria, Packt (2026).**

**Book GitHub repo: https://github.com/PacktPublishing/Learn-Linux-Device-Drivers**

Here, we present, in a chapter-wise format, additional online as well as book references. We feel that these will help you, the reader, in digging deeper into topics, or gaining additional views/information, or both. We definitely recommend you browse through the provided references. Also, you will find that some resources are repeated across chapters; we leave it this way so that you can gain all required resources in one place (and chapter-wise).
(Obviously, these reference articles/blogs/etc are provided "as-is"; we cannot vouch for their correctness or validity. If you find something amiss, like a broken link, please report it via the GitHub site's 'Issues' and I'll try and fix it, thank you.)

Before anything else, please note:

**Companion Volume (and effectively, Prerequisite reading material!):**

- **Linux Kernel Programming, Part 2 (LKP-P2) - Writing character device drivers: Learn to work with user-kernel interfaces, handle peripheral I/O & hardware interrupts, Kaiwan N Billimoria, Packt**
    - [Good news! Download LKP-P2 as a PDF for free!]( https://github.com/PacktPublishing/Linux-Kernel-Programming/tree/master/Linux-Kernel-Programming-(Part-2))
    - [LKP - Part 2 GitHub repo](https://github.com/PacktPublishing/Linux-Kernel-Programming-Part-2)


For your convenience, below are chapter-wise links to the *Further Reading* notes:

- [*Generic online and book resources: miscellaneous, very useful*](https://github.com/PacktPublishing/Linux-Kernel-Programming_2E/blob/main/Further_Reading.md#generic-online-and-book-resources--miscellaneous-very-useful)

*The Basics - setting up the workspace*

**Qemu and the VMM GUI setup**

 - [Install KVM & Virtual Machine Manager on Ubuntu (Complete Tutorial) – Beginner-Friendly Tutorial, YouTube, Nov 2025](https://www.youtube.com/watch?v=9ea_pKCOQ9Q)

Bonus (though older) links:

 - [Chapter 1, Kernel workspace setup](https://github.com/PacktPublishing/Linux-Kernel-Programming_2E/blob/main/Further_Reading.md#chapter-1-kernel-development-workspace-setup---further-reading)


## Chapter 2, The modern LDM and Writing platform drivers - Further Reading

- LDM
    - [Linux Device Model (LDM)](https://linux-kernel-dev.blogspot.com/2017/02/linux-device-model-ldm.html)
    - [The Driver Model Core, Part I, Greg K Hartman, June 2003! (old but gold)](https://www.linuxjournal.com/article/6717)
    - [Understanding the Structure of a Linux Kernel Device Driver, Sergio Prado, ELC 2021](https://www.youtube.com/watch?v=XoYkHUnmpQo&t=2561s)
    - [Driver Binding](https://www.kernel.org/doc/html/latest/driver-api/driver-model/binding.html)
    - [Device drivers infrastructure](https://www.kernel.org/doc/html/v6.18/driver-api/infrastructure.html)
    - [A tour of the mfd, regmap and syscon APIs, Bootlin](https://bootlin.com/pub/conferences/2015/elce/belloni-mfd-regmap-syscon/belloni-mfd-regmap-syscon.pdf)


## Chapter 3, The Device Tree - Further Reading

- Device Tree
    - [Linux and the Devicetree, Grant Likely, official kernel doc](https://docs.kernel.org/devicetree/usage-model.html)
    - [About the Device Tree, ofitself (PDF)](http://www.ofitselfso.com/BeagleNotes/AboutTheDeviceTree.pdf)
    - [Device Tree Mysteries, eLinux](https://elinux.org/Device_Tree_Mysteries#)
    - [Device Tree Technical Overview, Toradex](https://developer.toradex.com/software/linux-resources/device-tree/device-tree-overview/)
    - [Devicetree Specifications](https://www.devicetree.org/specifications/)
        - [Devicetree Specification, latest as of this writing: Release v0.4  - Released Wednesday, June 28, 2023](https://github.com/devicetree-org/devicetree-specification/releases/download/v0.4/devicetree-specification-v0.4.pdf)
    - [Device Trees for Dummies, Thomas Pettazoni, Bootlin](https://bootlin.com/pub/conferences/2013/elce/petazzoni-device-tree-dummies/petazzoni-device-tree-dummies.pdf)
    - [How (the) linux device tree operate(s), G Chakraborty, LinkedIn](https://www.linkedin.com/feed/update/urn:li:activity:7448733399097294848/)

- [Matching the device to the driver, a LinkedIn post by D Zhu](https://www.linkedin.com/posts/david-zhu-3a68a855_embeddedlinux-kerneldevelopment-devicedrivers-activity-7359187989844869121-l0U7?utm_source=social_share_send&utm_medium=member_desktop_web&rcm=ACoAAAA6kn8BD03SDQPxl_gJbaNGlauCaJ-lxzA)


- ACPI
    - [ACPI vs. Device Tree: The Linux Approach to Hardware Description, Tushar V](https://www.linkedin.com/pulse/acpi-vs-device-tree-linux-approach-hardware-tushar-vyavahare-cbxac/)
    - [Linux Device Configuration Mechanisms - ACPI vs Device Tree | Linux kernel and Linux device driver, YouTube](https://www.youtube.com/watch?v=UIXtM1IlnG4)

- DT Overlays
    - [Using Device Tree Overlays, example on BeagleBone boards, Michael Opdenacker, Feb 2022](https://bootlin.com/blog/using-device-tree-overlays-example-on-beaglebone-boards/)
    - [How to Write Device Tree Overlays, Toradex](https://developer.toradex.com/software/linux-resources/device-tree/how-to-write-device-tree-overlays/)
    - [Official kernel doc - Devicetree Overlay Notes](https://elixir.bootlin.com/linux/v6.18.33/source/Documentation/devicetree/overlay-notes.rst)
- PinMux
    - [Pinmuxing Guide, Toradex](https://developer.toradex.com/software/linux-resources/device-tree/pinmuxing-guide/)
    - [Pinmuxing AM6X Based Modules, Toradex](https://developer.toradex.com/software/linux-resources/device-tree/pinmuxing-guide/pinmuxing-with-am6x-based-modules/)
    - SoC packaging: BGA and TQFP
        - [What Is a BGA Chip? A Complete Guide to Ball Grid Array Packaging](https://jlcpcb.com/blog/bga-chip-explained)
        - [10 Reasons Why I Love the BGA](https://www.sigcon.com/Pubs/straight/bga.htm)
        - [TQFP Package Explained: Dimensions, Pin Pitch, PCB Design & Comparisons](https://jlcpcb.com/blog/tqfp-package-explained)
- []()

- TI SysConfig tool
    - [Download](https://www.ti.com/tool/SYSCONFIG#downloads)
    - [Getting started with Sysconfig Tool (HTML)](https://www.ti.com/document-viewer/lit/html/sdaa194)
    - [TI SysConfig Deep Dive: Generating Pin and Peripheral Configurations](https://hubble.com/community/guides/ti-sysconfig-deep-dive-generating-pin-and-peripheral-configurations/)



## Chapter 5, Writing a PCIe skeleton driver - Further Reading

- [PCI SIG - Special Interest Group](https://pcisig.com/specifications) - PCI standards documents, etc
- Official kernel doc: [How To Write Linux PCI Drivers](https://docs.kernel.org/PCI/pci.html)
- Tutorial: [Writing a PCI device driver for Linux, Oleg Kutkov, January 2021](https://olegkutkov.me/2021/01/07/writing-a-pci-device-driver-for-linux/)
- [Wrt Linux, what exactly is SRIOV? [Credit: ChatGPT-4]](https://chatgpt.com/share/6840214e-99f0-8011-8fce-06889b7e9d3d)


[\[Top\]](https://github.com/PacktPublishing/Linux-Kernel-Programming_2E/blob/main/Further_Reading.md#further-reading)


## Chapter 6, Writing a NIC driver - Further Reading

- Required reading: [Internet protocol suite, Wikipedia article](https://en.wikipedia.org/wiki/Internet_protocol_suite#Comparison_of_TCP/IP_and_OSI_layering)

- Book: [Understanding Linux Network Internals, C Benvuneti, O'Reilly Media, Feb 2006](https://a.co/d/dlx5GjY)
- [Linux ip Command with Examples, M Aleksic, Sept 2024](https://phoenixnap.com/kb/linux-ip-command-examples)
- Notifier Chain mechanism
    - [Mastering Notifier Chains and Learn how Reboot Notifiers in the Linux Kernel is implemented, Faayiz M, Mar 2024](https://www.linkedin.com/pulse/mastering-notifier-chains-learn-how-reboot-notifiers-linux-faayiz-m-gc5nc/)
    - [Notifier Chains, Clemson University](https://people.computing.clemson.edu/~westall/853/notes/notifier.pdf)

- [Linux kernel design patterns - part 1, Neil Brown, June 2009, LWN (this is the first of a 3-part series)](https://lwn.net/Articles/336224/)

- [ChatGPT 4: Qs> When i send or receive network packets (or just ping) how can i see which network interface is actually used to perform the tx or rx?](https://chatgpt.com/share/684bc0fd-0a94-8011-8149-e1153a5af398)
-  [Netconsole: official kernel doc, v6.12](https://www.kernel.org/doc/html/v6.12/networking/netconsole.html#netconsole)
- [Monitoring and Tuning the Linux Networking Stack: Sending Data, Joe Damato, packagecloud, Feb 2017](https://blog.packagecloud.io/monitoring-tuning-linux-networking-stack-sending-data/)
- [Illustrated Guide to Monitoring and Tuning the Linux Networking Stack: Receiving Data, Joe Damato, packagecloud, Mar 2022](https://blog.packagecloud.io/illustrated-guide-monitoring-tuning-linux-networking-stack-receiving-data/)
 
- Linux Socket Buffer (SKB)
    - Though old, still excellent: [How SKB's Work (1), Dave Miller](http://oldvger.kernel.org/~davem/skb_data.html)
    - [How SKB's Work (2), Dave Miller](http://oldvger.kernel.org/~davem/skb.html)
    - [struct sk_buff, official Linux kernel doc](https://docs.kernel.org/networking/skbuff.html)
    - [Printing sk_buff data, Oleg Kutkov, Oct 2019](https://olegkutkov.me/2019/10/17/printing-sk_buff-data/)

- Realtek RTL8169 NIC Datasheets
    - [Realtek RTL8169S-32/RTL8169S-64 Hardware datasheet PDF](http://realtek.info/pdf/rtl8169s.pdf)
    - [Software programmer’s datasheet for the Realtek RTL8169 PDF](https://www.manualslib.com/manual/1612965/Realtek-Rtl8169.html)

- [DMA: LWN kernel index of articles](https://lwn.net/Kernel/Index/#Direct_memory_access)

- Memory Barriers
    - PDF: ['Memory Barriers: a Hardware View for Software Hackers', Paul E. McKenney, IBM LTC, June 2010](http://www.rdrop.com/~paulmck/scalability/paper/whymb.2010.06.07c.pdf)
    - [Memory Barriers Are Like Source Control Operations, Preshing on Programming blog, July 2012](https://preshing.com/20120710/memory-barriers-are-like-source-control-operations/)

- New API (NAPI)
    - Official kernel doc: [NAPI](https://docs.kernel.org/networking/napi.html)
    - [Linux Foundation wiki site: napi](https://wiki.linuxfoundation.org/networking/napi)

- Netfilter
    - [Packet Flow, Netfilter and Iptables, Dipak KR Das, Medium](https://medium.com/@dipakkrdas/netfilter-and-iptables-f8a946bb83af)
    - [Netfilter’s flowtable infrastructure, official kernel doc](https://docs.kernel.org/networking/nf_flowtable.html)
    - [Linux NetFilter, IP Tables and Conntrack Diagrams, nerdalert, GitHub Gists](https://gist.github.com/nerdalert/a1687ae4da1cc44a437d)
    - [Linux network programming | Netfilter Framework- Part -1 | Easy Explanation | Youtube](https://www.youtube.com/watch?v=vKbjhU--Lok)

- NIC Offloading
    - [TCP offload engine](https://en.wikipedia.org/wiki/TCP_offload_engine)
    - [JLS2009: Generic receive offload, Jon Corbet, LWN, Oct 2009](https://lwn.net/Articles/358910/)
- Ethtool
    - [Using ethtool in Linux, Chin Ming JunMar 2024](https://www.baeldung.com/linux/using-ethtool)
    - [ethtool(8) — Linux manual page](https://man7.org/linux/man-pages/man8/ethtool.8.html)
- Network visualization tools
    - [Traceroute Online - Trace and Map the Packets Path](https://traceroute-online.com/)
    - [16 network visualization tools that you should know!, Dr. Veronica Espinoza, Medium, Nov 2023](https://medium.com/@vespinozag/16-network-visualization-tools-that-you-should-know-2c26957b707e)
    - [What are the Network Visualization Tools you guys use for the audit purpose?, Reddit](https://www.reddit.com/r/sysadmin/comments/1bpmisl/what_are_the_network_visualization_tools_you_guys/)


## Chapter 7, Writing an Input device driver - Further Reading

- [Wayland (protocol)](https://en.wikipedia.org/wiki/Wayland_(protocol))
- [What is libinput?](https://wayland.freedesktop.org/libinput/doc/latest/what-is-libinput.html)
- [Mouse Events and Input Event Interface in Linux, baeldung, Mar 2024](https://www.baeldung.com/linux/mouse-events-input-event-interface)
- [Creating an input device driver - official kernel doc](https://docs.kernel.org/input/input-programming.html)
- [Using a push button with Raspberry Pi GPIO, Soren, Feb 2018](https://raspberrypihq.com/use-a-push-button-with-raspberry-pi-gpio/)
- [Definitive GPIO guide (biased to i.MX6)](https://kosagi.com/w/index.php?title=Definitive_GPIO_guide)
- [Goodix GT911 touchscreen controller Datasheet PDF](https://www.crystalfontz.com/controllers/datasheet-viewer.php?id=464)
- [Approaches to Keylogging perhaps? IMP NOTE: to Only be used ethically!]
    - [See a conversation with claude.ai on this topic](https://claude.ai/share/e8d09c7e-2e38-4297-b39d-5f12117bd1fe)
    - [kernel doc: get called back on keyboard events! Keylogger like?](https://docs.kernel.org/input/notifier.html)



## Chapter 8, Writing a modern Block Driver, Part 1 – the Linux Storage Stack - Further Reading
- [Cylinder-head-sector, Wikipedia](https://en.wikipedia.org/wiki/Cylinder-head-sector)
- [Logical block addressing, Wikipedia](https://en.wikipedia.org/wiki/Logical_block_addressing)
- [Managed Flash Background Operations Series; Part 5: Understanding Logical Block Address to Physical Block Address Translation in NAND Flash Memory](https://americas.kioxia.com/content/dam/kioxia/en-us/business/memory/mlc-nand/asset/KIOXIA_Managed_Flash_BOS_P5_Understanding_L2P_Block_Address_Translation_Tech_Brief.pdf)

- [LINUX SYSTEM CALLS UNDER THE HOOD, CROSSING THE BORDER BETWEEN USERLAND AND THE KERNEL, J Sobcazk, Aug 2021](https://juliensobczak.com/inspect/2021/08/10/linux-system-calls-under-the-hood/)
- [Block Device Drivers](https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html)
- [Direct Access for files (DAX), official kernel doc](https://docs.kernel.org/filesystems/dax.html)

- [How to Use fio for I/O Performance Benchmarking on Ubuntu, @nawazdhandala, March 2026](https://oneuptime.com/blog/post/2026-03-02-use-fio-io-performance-benchmarking-ubuntu/view)
- [The official NVMe website](https://nvmexpress.org/)
- [Device Mapper (dm), official Linux kernel doc (links to dm-foo)](https://docs.kernel.org/admin-guide/device-mapper/index.html)
- RAID data storage tech
    - [RAID on Wikipedia](https://en.wikipedia.org/wiki/RAID)
    - [dm-raid, Linux kernel official doc](https://docs.kernel.org/admin-guide/device-mapper/dm-raid.html)
    - [RAID arrays, Linux kernel official doc](https://docs.kernel.org/admin-guide/md.html)
- DM Multipath
    - [Linux DM Multipath on Wikipedia](https://en.wikipedia.org/wiki/Linux_DM_Multipath)
    - [Introduction to device mapper multipathing (“multipath”), Ubuntu Server docs](https://ubuntu.com/server/docs/explanation/intro-to/multipath/)
    - [Device Mapper Multipath, an Introduction, Oracle blog, Aug 2024](https://blogs.oracle.com/linux/device-mapper-multipath-an-introduction)
    - [Configuring and managing Device Mapper Multipath, RedHat RHEL7](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html-single/dm_multipath/index)


## Chapter 9, Writing a modern Block Driver, Part 2 – the implementation - Further Reading
- [Block Device Drivers; excellent](https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html#block-device-drivers)
- [Block - official kernel docs](https://www.kernel.org/doc/html/v6.18/block/index.html)
    - [Multi-Queue Block IO Queueing Mechanism (blk-mq) for 6.18](https://www.kernel.org/doc/html/v6.18/block/blk-mq.html)
    - Includes links to docs describing the I/O schedulers

- [Linux Multi-Queue Block IO Queueing Mechanism (blk-mq) Details, Werner Fischer, Thomas Krenn, 2015](https://www.thomas-krenn.com/en/wiki/Linux_Multi-Queue_Block_IO_Queueing_Mechanism_(blk-mq)_Details)

- *NVMe protocol:*
    - [Overview of NVMe Architecture, Oracle blog, Nov 2025](https://blogs.oracle.com/linux/overview-of-nvme-architecture)
    - [SGL in NVM Express, mingw, Feb 2025](https://medium.com/@wmwby/sgl-in-nvm-express-7e9ac9c1c652)
    - [NVMe: What - (this article nicely describes PRP and SGL with diagrams; this page may or may not work)](https://vatiminxuyu.gitbooks.io/xuyu/content/blog/nvme-what.html)
    - [NVMe - (useful for driver authors)](https://wiki.osdev.org/NVMe)
    - [The NVMe Specification, 1.1a, Sept 2013](https://www.nvmexpress.org/wp-content/uploads/NVM-Express-1_1a.pdf)
    - [SGL in NVM Express, mingw, Medium, Feb 2025](https://medium.com/@wmwby/sgl-in-nvm-express-7e9ac9c1c652)

- [Did you know that userspace block drivers are possible too!? 'Userspace block device driver (ublk driver)
' - official kernel doc](https://www.kernel.org/doc/html/latest/block/ublk.html)


## Chapter 10, Writing a USB Device Driver
- USB: Protocol, architecture, spec, working
    - [How USB Works: Introduction (Part 1), J Shah, Apr 2024](https://www.circuitbread.com/tutorials/how-usb-works-introduction-part-1)
    - [How USB Works: Introduction (Part 2), J Shah, Apr 2024](https://www.circuitbread.com/tutorials/how-usb-works-communication-protocol-part-2)
    - [USB Background](https://www.totalphase.com/support/articles/200349256-usb-background/)
    - [USB Protocol : Its Architecture, Working, Synchronisation, DataFormat & Its Applications, haya, July 2022](https://www.mathaelectronics.com/digital-car-turning-and-braking-indicator/)
    - [Technical Deep Dive: What is USB Root Hub Architecture?, PURPLELEC, Feb 2026](https://www.purplelec.com/info-detail/usb-root-hub-architecture-technical-analysis)
    - [USB Specification Rev 2.0 PDF (includes the famous Chapter 9, USB Device Framework)](https://eater.net/downloads/usb_20.pdf)
    - [Defined Class Codes (USB CLasses)](https://www.usb.org/defined-class-codes)

- USB drivers
    - [Writing USB Device Drivers - the official kernel doc for a USB 'skeleton' driver](https://docs.kernel.org/driver-api/usb/writing_usb_driver.html)

- Misc:
    - [What actually happens when you plug in a USB device?, technovelty, July 2007](https://technovelty.org/linux/what-actually-happens-when-you-plug-in-a-usb-device.html)
    - []()
    - [USB gadget mode in Raspberry Pi OS: SSH over USB, P Oberosier, Jan 2026](https://www.raspberrypi.com/news/usb-gadget-mode-in-raspberry-pi-os-ssh-over-usb/)
    - TinyUSB
        - [TinyUSB - Getting Started](https://docs.tinyusb.org/en/latest/getting_started.html)
        - [GitHub repo](https://github.com/hathach/tinyusb)
- []()
- []()

[End Doc]
