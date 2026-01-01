
FNAME := bbb_gpiokeys_pushbtn
all: dt
dt:
ifeq (,$(shell which dtc))
        $(error ERROR: install the DTC compiler first)
endif
	@echo "--- compiles the Device Tree Blob (DTB) from the DTS (ARM/PPC/Risc-V/etc) ---"
	dtc -@ -I dts -O dtb -o $(FNAME).dtbo $(FNAME).dts
	# DTBO - Device Tree Blob Overlay
install:
	@echo "--- install: copying .dtbo to /boot/dtbs/$(shell uname -r)/overlays/"
	sudo cp $(FNAME).dtbo /boot/dtbs/$(shell uname -r)/overlays/
	@echo "NOTE: you must update /boot/uEnv.txt to load this DTBO"
clean:
	rm -f $(FNAME).dtbo
