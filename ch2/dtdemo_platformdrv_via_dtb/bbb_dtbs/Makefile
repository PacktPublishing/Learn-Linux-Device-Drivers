# Makefile
# PARTICULAR to the BBB board running the $KVER kernel
# Update.edit to suit your env
KVER := 6.18.39-bone44

all: dtbs
dtbs: dtdemo_bbb.dtso
	dtc -@ -I dts ./dtdemo_bbb.dtso -O dtb -o BBB-DTDEMO.dtbo
install:
	sudo cp BBB-DTDEMO.dtbo /boot/dtbs/${KVER}/overlays/
	@echo "BBB-DTDEMO.dtbo copied into /boot/dtbs/${KVER}/overlays/"
	@echo "You must now edit /boot/uEnv.txt to add the overlay (if not already done)"
clean:
	rm -f BBB-DTDEMO.dtbo
