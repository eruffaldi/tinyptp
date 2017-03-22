
#ifdef WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ptp_common.h"

void init_socket();


// only used if SO_TIMESTAMP
extern int recv_with_timestamp(int sock, char * bufin, int bufin_size,int flags, struct sockaddr_in * from_addr, int*from_addr_size, ptp_time_t* alttime);

int main(int argc, char const *argv[])
{
	struct master_data md;

	if(argc ==2 && strcmp(argv[1],"-h") == 0)
	{	
		printf("tinyptp master socket by ER@SSSA 2016-2017\nSyntax: [outaddress=0.0.0.0 [repeats=10]]\n");
		return 0;
	}

	init_socket(); // for windows

	int myport = 1319;
	int outport = 1320;
	const char * myaddress = "0.0.0.0"; // NOTE that it can be multicast
	const char * outaddress = argc == 1 ? "0.0.0.0" : argv[1];
	int repeats = argc > 2 ? strtol(argv[2],0,10): 10;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == 0)
	{
		perror("cannot create socket");
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

    memset(&md,0,sizeof(md));
	md.sock = sock;
	md.nsteps = repeats;
	md.clientdata = &out_addr;
	md.clientdatasize = sizeof(out_addr);
#ifdef WIN32
	int enable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int)))
#else
	int enable = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
#endif
	{
		perror("cannot reuseaddr opt\n");
		return -1;		
	}
    if(bind(sock,(const struct sockaddr*)&my_addr,sizeof(my_addr)))
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
	printf("Listening to %s:%d and sending to %s:%d repeats:%d%s\n",myaddress,myport,outaddress,outport,repeats,hastimestamp? "has timestamp" : "");
	master_sm(&md,EVENT_RESET,0,0); // initial step


	while(1)
	{
		char bufin[128];
		int n;
		int rf = sizeof(out_addr);
#ifdef SO_TIMESTAMP		
		if(hastimestamp)
			n = recv_with_timestamp(md.sock,bufin,sizeof(bufin),0,&out_addr,&rf,&md.alttime);
		else
#endif
			n = recv(md.sock,bufin,sizeof(bufin),0);
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