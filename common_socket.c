

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
    long result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  #endif
}

long long test_extra_offset_ns;

void ptp_get_time(int in[2])
{
#ifdef WIN32
    // no need for absolute time 
  __int64 wintime; 
   GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -= 116444736000000000;  //1jan1601 to 1jan1970
   wintime *= 100; // nanoseconds from 100-nanosecond
   wintime += test_extra_offset_ns;
   *(__int64*)in = wintime;
#else
    /* check for nanosecond resolution support */
    #ifndef CLOCK_REALTIME
        // TODO: OSX alternative clock
        struct timeval tv = {0};
        gettimeofday(&tv, NULL);
        in[0] = (int) tv.tv_sec;
        in[1] = (int) (tv.tv_usec * 1000 + test_extra_offset_ns);
    #else
        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        in[0] = (int) ts.tv_sec;
        in[1] = (int) (ts.tv_nsec + test_extra_offset_ns);
    #endif
#endif
}

void ptp_send_packet(int sock, const char * data, int size, void *clientdata,int clientdatasize)
{
//    send(sock, bufout, 12, 0); //using connect , 0, (struct sockaddr *) clientdata, clientdatasize);
    sendto(sock, data, size, 0, (struct sockaddr *) clientdata, clientdatasize);
}