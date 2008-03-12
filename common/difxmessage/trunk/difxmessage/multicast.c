#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int openMultiCastSocket(const char *group, int port)
{
	int sock, v;
	unsigned int yes=1;
	struct sockaddr_in addr;
	struct ip_mreq mreq;
	struct timeval tv;

	/* Make UDP socket */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) 
	{
		return -1;
	}
	
	/* Set 1 second timeout */
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	/* Allow reuse of port */
	v = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if(v < 0) 
	{
		return -2;
	}
	
	/* set up destination address */
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	/* bind to receive address */
	v = bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if(v < 0) 
	{
		return -3;
	}
	
	v = inet_aton(group, &mreq.imr_multiaddr);
	if(!v) 
	{
		return -4;
	}
	
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	v = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		&mreq, sizeof(struct ip_mreq));
	if(v < 0) 
	{
		return -5;
	}
	
	return sock;
}

int closeMultiCastSocket(int sock)
{
	if(sock > 0) 
	{
		close(sock);
	}
	
	return 1;
}

int MultiCastReceive(int sock, char *message, int maxlen, char *from)
{
	struct sockaddr_in addr;
	int nbytes;
	socklen_t addrlen;

	addrlen = sizeof(struct sockaddr_in);

	nbytes = recvfrom(sock, message, maxlen, 0,
		(struct sockaddr *) &addr, &addrlen);

	if(addrlen > 0 && from != 0)
	{
		strncpy(from, inet_ntoa(addr.sin_addr), 16);
	}

	return nbytes;
}

