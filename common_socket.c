

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
    sendto(sock, data, size, 0, (struct sockaddr *) clientdata, clientdatasize);
}

#ifdef SO_TIMESTAMP
int recv_with_timestamp(int sock, char * bufin, int bufin_size, int flags, struct sockaddr_in * from_addr, int* from_addr_size, int alttime[2])
{
      // FROM: https://www.kernel.org/doc/Documentation/networking/timestamping/timestamping.c
      int n;
      struct timeval tv;
      struct msghdr msg;
      struct iovec iov;
      struct cmsghdr *cmsg;
      struct {
        struct cmsghdr cm;
        char control[512]; // maybe too much
      } control;
      memset(&msg, 0, sizeof(msg));
      iov.iov_base = bufin;
      iov.iov_len = bufin_size;
      msg.msg_iov = &iov;
      msg.msg_iovlen = 1;
      msg.msg_name = (caddr_t)from_addr;
      msg.msg_namelen = *from_addr_size;
      msg.msg_control = &control;
      msg.msg_controllen = sizeof(control);
      // mark default
      alttime[0] = 0;
      alttime[1] = 0;
      n = recvmsg(sock,&msg,flags); //|MSG_DONTWAIT|MSG_ERRQUEUE);
#if 1
  // generic loopy from linux
      for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        switch (cmsg->cmsg_type) {
        case SOL_SOCKET:      
          switch (cmsg->cmsg_type) {
            case SO_TIMESTAMP: 
              {
                struct timeval *stamp = (struct timeval *)CMSG_DATA(cmsg);
                alttime[0] = stamp->tv_sec;
                alttime[1] = (int)(stamp->tv_usec*1000 + test_extra_offset_ns);
                return n; // no need to wait for the rest
              }
              break;
          }
        }
      }
#else
    // compact as in OSX ping (when we are sure of what we setted)
      cmsg = (struct cmsghdr *)&control;
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SCM_TIMESTAMP &&
          cmsg->cmsg_len == CMSG_LEN(sizeof tv)) {
        /* Copy to avoid alignment problems: */
        memcpy(&tv, CMSG_DATA(cmsg), sizeof(tv));
        alttime[0] = tv.tv_sec;
        alttime[1] = (int)(tv.tv_usec*1000 + test_extra_offset_ns);
      }
#endif
      return n;
}
#endif