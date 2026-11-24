# Makefile

CC=gcc
CFLAGS_DBG=-DDEBUG -g -ggdb -O0 -Wall
CFLAGS=-Wall -O2

# Cross toolchain compile for the utils on BeagleBoard (or whatever)
# Update XTOOL var below to your toolchain prefix
XTOOL := arm-none-linux-gnueabi-

ALL := talker_dgram listener_dgram
all: ${ALL}

talker_dgram: talker_dgram.c
	${CC} ${CFLAGS_DBG} talker_dgram.c -o talker_dgram
xtalker_dgram: talker_dgram.c
	${XTOOL}${CC} ${CFLAGS_DBG} talker_dgram.c -o xtalker_dgram

listener_dgram: listener_dgram.c
	${CC} ${CFLAGS_DBG} listener_dgram.c -o listener_dgram
xlistener_dgram: listener_dgram.c
	${XTOOL}${CC} ${CFLAGS_DBG} listener_dgram.c -o xlistener_dgram

clean:
	rm -f ${ALL}
