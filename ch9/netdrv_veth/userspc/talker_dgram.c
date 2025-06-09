/*
 * talker.c -- a datagram "client" demo
 * Adapted from Beej's N/w Prg Guide.
 *
 * Part of the 'virtual ethernet' / veth NIC driver demo.
 * To try it out:
 * 1. cd <netdrv_veth>
 * 2. cd netdriver/
 * 3. ./run
 * 4. cd ../userspc
 * 5. ./runapp
 * ...
 * It should work.. watch the kernel log with 'journalctl -f -k'
 * Kaiwan N Billimoria
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../veth_common.h"

//#define PORTNUM 54295 // the port users will be connecting to
//#define INTF   "veth"

static int send_pkt(int counter, int sd, char *dest_ip, char *msg)
{
	struct sockaddr_in dest_addr;
	int numbytes;

	dest_addr.sin_family = AF_INET;	// host byte order
	dest_addr.sin_port = htons(PORTNUM);	// short, network byte order
	dest_addr.sin_addr.s_addr = inet_addr(dest_ip);
	memset(&(dest_addr.sin_zero), '\0', 8);

//printf("port=%d\n",dest_addr.sin_port);

	printf("talker: sending packet %3d over UDP/IP port %d now...\n",
	       counter, PORTNUM);
	if ((numbytes = sendto(sd, msg, strlen(msg), 0,
			       (struct sockaddr *)&dest_addr,
			       sizeof(dest_addr))) == -1) {
		perror("talker: sendto");
		return -1;
	}
	//printf("talker: sent %d bytes.\n", numbytes);
	return numbytes;
}

#if 0
#define MAXBUFLEN 1000
static int recv_pkt(int sd)
{
	struct sockaddr_in src_addr;	// connector's (peers) address information
	int numbytes;
	socklen_t addr_len;
	char buf[MAXBUFLEN];

	addr_len = sizeof(struct sockaddr);
	if ((numbytes = recvfrom(sd, buf, MAXBUFLEN - 1, 0,
				 (struct sockaddr *)&src_addr,
				 &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	printf("got packet from %s\n", inet_ntoa(src_addr.sin_addr));
	printf("packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("packet contains \"%s\"\n", buf);

	return numbytes;
}
#endif

int main(int argc, char *argv[])
{
	int sockfd, i;

	if (geteuid()) {
		fprintf(stderr, "%s: need to run as root.\n", argv[0]);
		exit(1);
	}
	if (argc != 4) {
		fprintf(stderr, "Usage: %s DEST-IP-address message number-of-packets-to-transmit\n", argv[0]);
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
#if 1
	// Should be running as root!
	if (setsockopt
	    (sockfd, SOL_SOCKET, SO_BINDTODEVICE, INTF_NAME,
	     strlen(INTF_NAME)) < 0) {
		printf("\n%s:setsockopt failed...\n", argv[0]);
		close(sockfd);
		exit(1);
	}
	printf("%s: successfully bound to interface '%s'\n", argv[0],
	       INTF_NAME);
#endif
	for (i = 0; i < atoi(argv[3]); i++) {
	//while (1) {
		if (send_pkt(i, sockfd, argv[1], argv[2]) == -1) {
			fprintf(stderr, "%s: send_pkt failed, aborting.\n",
				argv[0]);
			close(sockfd);
			exit(1);
		}
		sleep(1);
	}
	//recv_pkt(sockfd);
	close(sockfd);

	return 0;
}
