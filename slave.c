#include <stddef.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "ptp_common.h"

// https://github.com/bestvibes/IEEE1588-PTP/blob/dev/slave/slave.c
static const char * names[] = {"PTPM_START", "PTPM_SYNC", "PTPM_TIMES", "PTPM_DELAY", "PTPM_NEXT", "PPTS_HELLO"};

static void ptp_send_packet_ins(struct slave_data * md, int what, int t_send[2])
{
    unsigned char bufout[4+8+8]; // PTP# timein timeout
    memcpy(bufout,"PTP",3);
    bufout[3] = what;
    memset(bufout+4,0,8);
    memcpy(bufout+12,t_send,8);
    ptp_send_packet(md->sock,bufout,sizeof(bufout),md->clientdata,md->clientdatasize);
}

// slave it is stateless
int slave_sm(struct slave_data * md, enum Event e, unsigned char * data, int n)
{
	if(e == EVENT_RESET)
	{
		int now[2];
		ptp_get_time(now);
		ptp_send_packet_ins(md,PTPS_HELLO,now);
		printf("slave at RESET at t=%d,%d\n",now[0],now[1]);
	}
	if(e == EVENT_NEWPACKET)
	{
		if(n == 20 && strncmp((char *)data,"PTP",3) == 0)
		{
			int ptype = -1;
			int treceived[2];
		    int treceived2[2];
        	int now[2];
			ptp_get_time(now); // TODO: replace it with the SOCKET timestamp if SO_TIMESTAMP available
        	if(md->alttime[0] != 0 || md->alttime[1] != 0)
       		{
       			long long a = TO_NSEC(md->alttime);
       			long long b = TO_NSEC(now);
       			if(a != b)
       			{
       				printf("delta %lld\n",b-a);
	       			now[0] = md->alttime[0];
	       			now[1] = md->alttime[1];       				
       			}
		       			else
		       				printf("nodelta\n");
       		}
			ptype = (int)data[3];
			treceived[0] = *(int*)data+4;
			treceived[1] = *(int*)data+8;
            // mark time back
            ((int*)data)[3] = now[0];
            ((int*)data)[4] = now[1];
			printf("slave received %s(%d) at t=%d,%d => t2=%d,%d\n",ptype >= 0 && ptype < PTPX_MAX ? names[ptype] : "unknown",ptype,treceived[0],treceived[1],now[0],now[1]);
			ptp_send_packet(md->sock,data,n,md->clientdata,md->clientdatasize);
		}
		else
		{
			printf("wrong packet data: length %d %s\n",n,data);
		}
	}
	return 0;
}
