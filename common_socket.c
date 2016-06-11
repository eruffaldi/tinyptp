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


void ptp_get_time(int in[2])
{
    /* check for nanosecond resolution support */
    #ifndef CLOCK_REALTIME
        struct timeval tv = {0};
        gettimeofday(&tv, NULL);
        in[0] = (int) tv.tv_sec;
        in[1] = (int) tv.tv_usec * 1000;
    #else
        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        in[0] = (int) ts.tv_sec;
        in[1] = (int) ts.tv_nsec;
    #endif
}

void ptp_send_packet(int sock, int what, int t_send[2], void *clientdata,int clientdatasize)
{
	unsigned char bufout[4+8];
	memcpy(bufout,"PTP",3);
	bufout[3] = what;
	memcpy(bufout+4,t_send,8);
//    send(sock, bufout, 12, 0); //using connect , 0, (struct sockaddr *) clientdata, clientdatasize);
    sendto(sock, bufout, 12, 0, (struct sockaddr *) clientdata, clientdatasize);
}