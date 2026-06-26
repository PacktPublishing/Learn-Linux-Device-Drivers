# sblkdev
Simple Block Device Linux kernel module

The original README.md is shown here.

Original url: https://github.com/CodeImp/sblkdev  \[1\]

Original ref article: https://prog.world/linux-kernel-5-0-we-write-simple-block-device-under-blk-mq/
(Unfortunately, the original article is no longer available.)

Contains a minimum of code to create the most primitive block device.

From \[1\]: 
"The Linux kernel is constantly evolving. And that's fine, but it complicates
the development of out-of-tree modules. I created this out-of-tree kernel
module to make it easier for beginners (and myself) to study the block layer.

Features:

- Compatible with Linux kernel from 5.10 to 6.0.
    - 6.8.y - tested on x86_64 Ubuntu 24.04 and Fedora 38 only
    - **[UPDATE - wrt the LDDIA book] : tested and working on 6.18.33 LTS**
 - Allows to create bio-based and request-based block devices.
 - Allows to create multiple block devices.
 - The Linux kernel code style is followed (checked by checkpatch.pl).


**BUILD**

`$ ls  

Kconfig   Makefile-standalone  blkdrv_tester.sh*  device.c  loadblk.sh*  mk.sh*

Makefile  README.md            convenient.h       device.h  main.c

$ ./mk.sh
 
Usage 
Compile project: 
	./mk.sh {build | clean} 
Install module : 
	./mk.sh {install | uninstall}
$ ./mk.sh build
Making ...
make: Entering directory '/home/c2dd/disk2/linux-6.18.33'
make[1]: Entering directory '/home/c2dd/kaiwanTECH/Learn-Linux-Device-Drivers/ch11/sblkdev'
  CC [M]  main.o
main.c:24:9: note: ‘#pragma message: Request-based scheme selected.’
   24 | #pragma message("Request-based scheme selected.")
      |         ^~~~~~~
main.c:34:9: note: ‘#pragma message: Show additional details in request function selected.’
   34 | #pragma message("Show additional details in request function selected.")
      |         ^~~~~~~
main.c:38:9: note: ‘#pragma message: The struct bio have pointer to struct block_device.’
   38 | #pragma message("The struct bio have pointer to struct block_device.")
      |         ^~~~~~~
main.c:44:9: note: ‘#pragma message: The blk_mq_alloc_disk() function was found.’
   44 | #pragma message("The blk_mq_alloc_disk() function was found.")
      |         ^~~~~~~
main.c:47:9: note: ‘#pragma message: The function add_disk() has a return code.’
   47 | #pragma message("The function add_disk() has a return code.")
      |         ^~~~~~~
  CC [M]  device.o
  LD [M]  sblkdev.o
  MODPOST Module.symvers
  CC [M]  sblkdev.mod.o
  CC [M]  .module-common.o
  LD [M]  sblkdev.ko
make[1]: Leaving directory '/home/c2dd/kaiwanTECH/Learn-Linux-Device-Drivers/ch11/sblkdev'
make: Leaving directory '/home/c2dd/disk2/linux-6.18.33'
Completed.
$ modinfo ./sblkdev.ko 
filename:       /home/c2dd/lddia_src/ch11/sblkdev/./sblkdev.ko
description:    Simple request-based block driver Linux kernel module for modern >= 5.x blk-mq Linux kernels
author:         Sergei Shtepa, Kaiwan NB
license:        GPL
srcversion:     DBDC5DC12DA03575BF5D501
depends:        
name:           sblkdev
retpoline:      Y
vermagic:       6.18.33-lddia SMP preempt mod_unload modversions 
parm:           catalog:New block devices catalog in format '<name>,<capacity sectors>;...' (charp)
$ 
    `


////////////////
How to use (run as root):
* Install kernel headers and compiler
deb:
	`apt install linux-headers gcc make`
	or
	`apt install dkms`
rpm:
	yum install kernel-headers

* Compile module
	`cd ${HOME}/sblkdev; ./mk.sh build`

* Install to current system
	`cd ${HOME}/sblkdev; ./mk.sh install`

* Load module
	`modprobe sblkdev catalog="sblkdev1,2048;sblkdev2,4096"`

* Unload
	`modprobe -r sblkdev`

* Uninstall module
	`cd ${HOME}/sblkdev; ./mk.sh uninstall`

---
**Alternate: Steps to test:**

- `./mk.sh build`
- `sudo ./loadblk.sh`

*< Now partition setup and format of the (pseudo) disk follows >*

'fdisk /dev/sblkdev1' will now run..

Apply these commands in this order:

`n      : New partition`

`p      : type Primary  [Enter]`

`1      : partition # 1 [Enter]`

`1      : First sector (1-2047, default 1): [Enter]`

`4095   : Last sector, +/-sectors or +/-size{K,M,G,T,P} (1-4095, default 4095): [Enter]`

`w      : write partition table`

(*Tip:* Just pressing `[Enter]` typically has the correct defaults setup)

It might now ask:

"Found a dos partition table in /dev/sblkdev1

Proceed anyway? (y,N) "

Type `y`

- `./blkdrv_tester.sh`

(This script fires off some disk IO, sleeps for a few seconds, then issues a `sync`. 

*Tip:* keep another terminal window open where you can watch the kernel log as it unfolds; to do so, try :
`journalctl -kf`
).

---
Feedback is welcome.
