
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
	printf("Listening to %d\n",myport);
	slave_sm(&md,EVENT_RESET,0,0); // initial step
	while(1)
	{
		unsigned char bufin[128];
		// automatic reply to the receiver
		int rf = sizeof(out_addr);
		int n = recvfrom(md.sock,bufin,sizeof(bufin),0,&out_addr,&rf);
		slave_sm(&md,EVENT_NEWPACKET,bufin,n);
	}
	
	return 0;
}