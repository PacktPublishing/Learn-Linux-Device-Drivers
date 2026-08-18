#!/bin/bash
# Part of the LDDIA book's source code.
# Location: ch10/platform_pushbtn_input_drv/bbb_dtoverlay
# Wrapper script to help setup the DT overlay -to- DTB for the TI BBB.
set -euo pipefail

KPFX=~/6.18.33  # location of kernel src tree; ADJUST this for your setup

echo "cp gpio_btn_bbb.dts ${KPFX}/arch/arm/boot/dts/ti/omap/"
cp gpio_btn_bbb.dts ${KPFX}/arch/arm/boot/dts/ti/omap/

echo "
Now head over to ${KPFX}, build the DTB:
 make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabihf- dtbs

Once that's done, ensure the BBB's ready and then
press [Enter] to continue here...
"
read x

ls -l ${KPFX}/arch/arm/boot/dts/ti/omap/gpio_btn_bbb.dtb
BBB_IP=192.168.0.30   # UPDATE this for your board!
DEST_BBB=/boot/dtbs/6.18.39-bone44/overlays # can't directly scp here: Permission denied (even w/ sudo)
echo "scp ${KPFX}/arch/arm/boot/dts/ti/omap/gpio_btn_bbb.dtb  debian@${BBB_IP}:~/"
scp ${KPFX}/arch/arm/boot/dts/ti/omap/gpio_btn_bbb.dtb  debian@${BBB_IP}:~/

echo "Done; now, on the BBB, do this:
 - sudo cp ~/gpio_btn_bbb.dtb ${DEST_BBB}/BBB-GPIO-BTN.dtbo ; sync
 - reboot the BBB
 and test the driver."
