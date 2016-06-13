#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "ptp_common.h"

// https://github.com/bestvibes/IEEE1588-PTP/blob/dev/slave/slave.c
static const char * names[] = {"PTPM_START", "PTPM_SYNC", "PTPM_TIMES", "PTPM_DELAY", "PTPM_NEXT", "PPTS_HELLO"};

static void ptp_send_packet_ins(struct slave_data * md, int what, ptp_time_t t_send)
{
    unsigned char bufout[4+8+8]; // PTP# timein timeout
    memcpy(bufout,"PTP",3);
    bufout[3] = what;
    memset(bufout+4,0,8);
    memcpy(bufout+12,&t_send,8);
    ptp_send_packet(md->sock,bufout,sizeof(bufout),md->clientdata,md->clientdatasize);
}

// slave it is stateless
int slave_sm(struct slave_data * md, enum Event e, unsigned char * data, int n)
{
	if(e == EVENT_RESET)
	{
		ptp_time_t now;
		ptp_get_time(&now);
		ptp_send_packet_ins(md,PTPS_HELLO,now);
		printf("slave at RESET at t=%lld\n",now);
	}
	if(e == EVENT_NEWPACKET)
	{
		if(n == 20 && strncmp((char *)data,"PTP",3) == 0)
		{
			int ptype = -1;
			ptp_time_t treceived,treceived2;
        	ptp_time_t now;
			ptp_get_time(&now); // TODO: replace it with the SOCKET timestamp if SO_TIMESTAMP available
        	if(md->alttime != 0)
       		{
       			long long a = TO_NSEC(md->alttime);
       			long long b = TO_NSEC(now);
       			if(a != b)
       			{
       				printf("delta %lld\n",b-a);
       				now = md->alttime;
       			}
		       			else
		       				printf("nodelta\n");
       		}
			ptype = (int)data[3];
			treceived = *(ptp_time_t*)(data+4);
            // mark time back
            *((ptp_time_t*)(data+12)) = now;
            printf("slave received %s(%d) at t=%lld => t2=%lld\n",ptype >= 0 && ptype < PTPX_MAX ? names[ptype] : "unknown",ptype,treceived,now);
            ptp_send_packet(md->sock,data,n,md->clientdata,md->clientdatasize);
		}
		else
		{
			printf("wrong packet data: length %d %s\n",n,data);
		}
	}
	return 0;
}
