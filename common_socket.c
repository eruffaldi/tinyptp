

#ifdef WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

void init_socket()
{
  #ifdef WIN32
    WSADATA wsa_data;
    long result = ::WSAStartup(MAKEWORD(major, minor), &wsa_data);
  #endif
}

void ptp_get_time(int in[2])
{
#ifdef WIN32
    // no need for absolute time 
  __int64 wintime; 
   GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -= 116444736000000000;  //1jan1601 to 1jan1970
   wintime *= 100; // nanoseconds from 100-nanosecond
   *(__int64*)in = wintime;
#else
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