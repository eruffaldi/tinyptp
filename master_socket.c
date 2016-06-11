#include<stdio.h>
/* socket stuff */
#include<sys/socket.h>
/* socket structs */
#include<netdb.h>
/* strtol, other string conversion stuff */
#include<stdlib.h>
/* string stuff(memset, strcmp, strlen, etc) */
#include<string.h>
/* signal stuff */
#include<signal.h>
#include <arpa/inet.h>
#include "ptp_common.h"

int main(int argc, char const *argv[])
{
	struct common_data md;

	int myport = 1319;
	int outport = 1320;
	const char * myaddress = "0.0.0.0"; // NOTE that it can be multicast
	const char * outaddress = argc == 1 ? "0.0.0.0" : argv[1];

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

    out_addr.sin_family = AF_INET;
    out_addr.sin_addr.s_addr = inet_addr(outaddress);  /* send to server address */
    out_addr.sin_port = htons(outport);

	md.sock = sock;
	md.clientdata = &out_addr;
	md.clientdatasize = sizeof(out_addr);
	int enable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
	{
		perror("cannot reuseaddr opt\n");
		return -1;		
	}
    if(bind(sock,(struct sockaddr*)&my_addr,sizeof(my_addr)))
    {
    	perror("cannot bind master\n");
    	return -1;
    }
    /*if(connect(sock,(struct sockadrr*)&out_addr,sizeof(out_addr)))
    {
    	perror("cannot conenct to target UDP\n");
    	return -1;    	
    }
    */
	master_sm(&md,EVENT_RESET,0,0); // initial step

	while(1)
	{
		unsigned char bufin[128];
		int n = recv(md.sock,bufin,sizeof(bufin),0);
		if(master_sm(&md,EVENT_NEWPACKET,bufin,n))
		{
		    printf("Average Offset = %lldns\n", md.sum_offset/md.steps);
		    printf("Average Delay = %lldns\n", md.sum_delay/md.steps);
		    
		    printf("Smallest Offset = %lldns\n", md.smallest_offset);
		    printf("Smallest Delay = %lldns\n", md.smallest_delay);
		    
		    printf("Largest Offset = %lldns\n", md.largest_offset);
		    printf("Largest Delay = %lldns\n", md.largest_delay);			
		    break;
		}
	}
	
	return 0;
}