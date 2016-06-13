
#ifdef WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ptp_common.h"

void init_socket();
extern long long test_extra_offset_ns;

// only used if SO_TIMESTAMP
extern int recv_with_timestamp(int sock, char * bufin, int bufin_size, int flags, struct sockaddr_in * from_addr, int *from_addr_size, ptp_time_t *aa);


int main(int argc, char const *argv[])
{
	struct slave_data md;

	init_socket();
	int myport = 1320;
	int outport = 1319;
	const char * myaddress = "0.0.0.0"; // NOTE that it can be multicast
	if(argc > 1)
		test_extra_offset_ns = strtol(argv[1],0,10);
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == 0)
	{
		perror("cannot socket");
		return -1;
	}

	struct sockaddr_in my_addr,out_addr;
	memset(&my_addr,0, sizeof(my_addr));
	memset(&out_addr,0, sizeof(out_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(myaddress);  /* send to server address */
    my_addr.sin_port = htons(myport);


	md.sock = sock;
	md.clientdata = &out_addr;
	md.clientdatasize = sizeof(out_addr);
	int enable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
	{
		perror("cannot reuseaddr opt\n");
		return -1;
	}
    if(bind(sock,(struct sockadrr*)&my_addr,sizeof(my_addr)))
    {
    	perror("cannot bind master\n");
    	return -1;
    }

    int hastimestamp = 0;
#ifdef SO_TIMESTAMP
	{ 
		if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, &hastimestamp, sizeof(hastimestamp)) < 0)
		{
			perror("setsockopt SO_TIMESTAMP");
		}
		else 
			hastimestamp = 1;
	}
#endif

	printf("Listening to %d%s\n",myport,hastimestamp ? "with timestamp" : "");
	slave_sm(&md,EVENT_RESET,0,0); // initial step
	while(1)
	{
		unsigned char bufin[128];
		// automatic reply to the receiver
		int rf = sizeof(out_addr);
		int n;
#ifdef SO_TIMESTAMP		
		if(hastimestamp)
			n = recv_with_timestamp(md.sock,bufin,sizeof(bufin),0, &out_addr,&rf,&md.alttime);
		else
#endif
			 n = recvfrom(md.sock,bufin,sizeof(bufin),0,&out_addr,&rf);
		slave_sm(&md,EVENT_NEWPACKET,bufin,n);
	}
	
	return 0;
}