#include <stddef.h>
#include <stdio.h>
#include "ptp_common.h"

// https://github.com/bestvibes/IEEE1588-PTP/blob/dev/slave/slave.c
static const char * names[] = {"PTPM_START", "PTPM_SYNC", "PTPM_TIMES", "PTPM_DELAY", "PTPM_NEXT", "PPTS_HELLO"};

// slave it is stateless
int slave_sm(struct slave_data * md, enum Event e, unsigned char * data, int n)
{
	if(e == EVENT_RESET)
	{
		int now[2];
		ptp_get_time(now);
		ptp_send_packet(md->sock,PTPS_HELLO,now,md->clientdata,md->clientdatasize);
		printf("slave at RESET at t=%d,%d\n",now[0],now[1]);
	}
	if(e == EVENT_NEWPACKET)
	{
		if(n == 20 && strncmp(data,"PTP",3) == 0)
		{
			int ptype = -1;
			int treceived[2];
		    int treceived2[2];
        	int now[2];
			ptp_get_time(now);
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
