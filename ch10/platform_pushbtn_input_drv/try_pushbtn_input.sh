#!/bin/bash
# Trying out the very simple pushbutton platform device via it's input driver

load_kdrv()
{
  local KDRV=pushbtn_input_drv
  [[ ! -f ${KDRV}.ko ]] && {
    make || exit 1
  }
  sudo insmod ./${KDRV}.ko || exit 1
  echo "[.] Our ${KDRV}.ko module's loaded up"
}
test_it()
{
  echo "[.] Running evtest; select the \"LLDIA: My Pushbutton\" entry"
  echo "    Then, press the push button a few times"
  echo "    When satisified, press ^C to exit..."
  echo
  echo "    Then lookup the kernel log with 'sudo dmesg'; done."
  echo
  sudo evtest
}

#--- 'main'

# build and load up the DT overlay (DTBO) file
echo "[.] DT overlay for the pushbutton platform device
(If you see this failure message (or similar):
 * Failed to apply overlay '1_gpio_btn_rpi4b' (kernel)
it typically implies that the DT overlay's already loaded)"
echo
[[ ! -f rpi4b_dtoverlay/build_cp_dtb ]] && {
  echo "${name}: rpi4b_dtoverlay/build_cp_dtb script missing? Aborting..."
  exit 1
}
(cd rpi4b_dtoverlay ; ./build_cp_dtb ) || exit 1
echo "[.] Our rpi4b_dtoverlay/gpio_btn_rpi4b.dtbo DT overlay's loaded up"
load_kdrv
test_it
exit 0
