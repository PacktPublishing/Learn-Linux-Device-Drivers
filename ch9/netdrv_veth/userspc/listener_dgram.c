/*
** listener.c - a datagram sockets "server" demo
** From Beej's Guide to Network Programming close(sockfd);
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

#define MYPORT 54295		//56100 // the port users will be connecting to
#define MAXBUFLEN 1024

static void hexdump(unsigned char *srcbuf, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printf("%02x ", srcbuf[i]);
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in my_addr;	// my address information
	struct sockaddr_in their_addr;	// connector's address information
	int numbytes;
	socklen_t addr_len;
	unsigned char buf[MAXBUFLEN];

#if 0
	if (argc == 1) {
		fprintf(stderr, "Usage: %s interface-to-bind-to\n", argv[0]);
		exit(1);
	}
#endif

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

#if 0
	if (setsockopt
	    (sockfd, SOL_SOCKET, SO_BINDTODEVICE, argv[1],
	     strlen(argv[1]) + 1) < 0) {
		printf("\n%s:setsockopt failed...\n", argv[0]);
		close(sockfd);
		exit(1);
	}
	printf("%s: successfully bound to interface '%s'\n", argv[0], argv[1]);
#endif

	my_addr.sin_family = AF_INET;	// host byte order
	my_addr.sin_port = htons(MYPORT);	// short, network byte order
	//my_addr.sin_addr.s_addr = inet_addr("10.10.1.5"); // DEST IP address
	my_addr.sin_addr.s_addr = INADDR_ANY;	// automatically fill with my IP zero the rest of the struct
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(sockfd, (struct sockaddr *)&my_addr,
		 sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);
	printf("%s: blocking on recvfrom...\n", argv[0]);
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
				 (struct sockaddr *)&their_addr,
				 &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	printf("got packet from %s\n", inet_ntoa(their_addr.sin_addr));
	printf("packet is %d bytes long\n", numbytes);
	if (numbytes == 58) {	/* 42+16=58; we know then that for this app/drv, 
				   it's the "meta-data" functionality, where the net driver gives us the 
				   actual packet content, i.e., skb->data for pkt len bytes */
		printf("42+16=58; we know then that for this app/drv, \n\
it's the \"meta-data\" functionality, where the net driver gives us the \n\
actual packet content, i.e., skb->data for pkt len bytes.\n\
\n\
Interpret as:\n\
\n\
 Len:           14                    20                   8           x     \n\
         +------------------------------------------------------------------+\n\
         |  Eth II Hdr  |         IPv4 Hdr         |  UDP Hdr   |    Data   |\n\
         +------------------------------------------------------------------+\n\
         ^                                                                  ^\n\
         |                                                                  |\n\
  skb-> data                                                             tail\n\
\n\
");
		hexdump(buf, numbytes);
	}
	buf[numbytes] = '\0';
	printf("packet contains \"%s\"\n", buf);

	return 0;
}
