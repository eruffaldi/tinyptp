#include <stddef.h>
#include "ptp_common.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>


	/*
	SLAVE
		loop
			if receive PTPM_START
				send PTPM_START
				receive PTP_TIMES => times
				clocksync(times):
					sync:
						receive * (should be PTPM_SYNC else abort)
						now=t2
						send (PTPM_SYNC,t2)
					delay:
						receive * (should be PPTM_DELAY else abort)
						now=t3
						send (PPTM_DELAY,t3)
					receive_packet * (should be PPTM_NEXT else abort)

			else:
				send PPTS_HELLO

	 */
		

	/*
	MASTER
	 send PTPM_START
	 wait receive PTPM_START
	 send PTPM_TIMES # 
	 wait receive PTPM_TIMES
	 clock:
		 loop times #
		 	sync
		 		send PTPM_SYNC 
		 		now=t1
			 	receive_packet (PTPM_SYNC,othertime) 
		 		now=t2
			 	return t2-t1
			delay_packet   
				send PTPM_DELAY
				receive PTPM_DELAY othertime=t3
				now=t4
				return t4-t3
			send PTPM_NEXT
	 */

static void ptp_send_packet_in(struct master_data * md, int what, int t_send[2])
{
	unsigned char bufout[4+8+8]; // PTP# timein timeout
	memcpy(bufout,"PTP",3);
	bufout[3] = what;
	memcpy(bufout+4,t_send,8);
	memset(bufout+12,0,8);
	ptp_send_packet(md->sock,bufout,sizeof(bufout),md->clientdata,md->clientdatasize);
}

enum MasterState { MASTER_START, MASTER_WAIT0,MASTER_TIMES, MASTER_SYNC, MASTER_SYNCANS, MASTER_DELAY, MASTER_DELAYANS,MASTER_LOOPEND, MASTER_DONE};

static const char * names[] = {"PTPM_START", "PTPM_SYNC", "PTPM_TIMES", "PTPM_DELAY", "PTPM_NEXT", "PPTS_HELLO"};

int master_sm(struct master_data * md, enum Event e, unsigned char * data, int n)
{
	int ptype = -1;
	int treceived[2];
	int treceived2[2];

	// multiple steps
	if(e == EVENT_RESET)
	{
		md->bigstate = MASTER_START;
	}
	else if(e == EVENT_NEWPACKET)
	{
		if(n == 20 && strncmp((char*)data,"PTP",3) == 0)
		{			
			// extract for the state machine and dump
			ptype = (int)data[3];
			treceived[0] = *(int*)(data+4);
			treceived[1] = *(int*)(data+8);
			treceived2[0] = *(int*)(data+12);
			treceived2[1] = *(int*)(data+16);
			printf("master received %s(%d) with t=%d,%d t2=%d,%d\n",ptype >= 0 && ptype < PTPX_MAX ? names[ptype] : "unknown",ptype,treceived[0],treceived[1],treceived2[0],treceived2[1]);
		}
		else
		{
			printf("wrong packet data: length %d %s\n",n,data);
		}
	}

	while(1)
	switch(md->bigstate)
	{
		case MASTER_START:
			md-> largest_offset = LONG_MIN;
		    md-> smallest_offset = LONG_MAX;
		    md-> sum_offset = 0;
		    md-> smallest_delay = LONG_MAX;
		    md-> largest_delay = LONG_MIN;
		    md-> sum_delay = 0;
		    md-> i = 0;
		    md-> bigstate = MASTER_WAIT0;
		    int dummy[2] = {0,0};
		    ptp_send_packet_in(md,PTPM_START,dummy);
		    return 0; // wait
		case MASTER_WAIT0:
			if(e == EVENT_NEWPACKET)
			{
				if(ptype != PTPM_START)
				{
					printf("expected START got %d\n",ptype);
					md->bigstate = MASTER_START;
					break; // retry
				}
				else
				{
					// check PTPM_START otherwise go MASTER_START
					int d[2] = {md->nsteps,0};
				    ptp_send_packet_in(md,PTPM_TIMES,d);
				    md->bigstate = MASTER_TIMES;
					return 0; // wait
				}
			}
		    return 0; // ignore
 		case MASTER_TIMES:
			if(e == EVENT_NEWPACKET)
			{
				if(ptype != PTPM_TIMES)
				{
					printf("expected TIMES\n");
					md->bigstate = MASTER_START;
					break; // retry
				}
				else
				{				
					md->bigstate = MASTER_SYNC;
					break; // next state
				}
			}
		    return 0; // ignore
		case MASTER_SYNC:
			{	
				// IMPORTANT: in real PTP we use the clock at the moment of the NIC exit				
				// NOTE: under Linux with SO_TIMESTAMPING it is possible to know when the packet left the NIC so we'll use SYNCDETAIL
				int t1[2];
				// sync is: get time, send, get answer, measure offset as difference between answer and original
				ptp_get_time(t1);
		    	ptp_send_packet_in(md,PTPM_SYNC,t1);
		    	md->bigstate = MASTER_SYNCANS;
			}
		    return 0; // wait
		case MASTER_SYNCANS:
			if(e == EVENT_NEWPACKET)
			{
				if(ptype != PTPM_SYNC)
				{
					printf("expected SYNC\n");
					md->bigstate = MASTER_START;
					break;  // restart
				}
				md->ms_diff = (TO_NSEC(treceived2) - TO_NSEC(treceived)); // t2-t1
				md->bigstate = MASTER_DELAY;
				break; // next state
			}
			else
				return 0; // ignore
		case MASTER_DELAY:
			{	
				// this is a REQUEST for DELAY we don't care about master time here
				int tmp[2];
				ptp_get_time(tmp);
		    	ptp_send_packet_in(md,PTPM_DELAY,tmp); // NOT used in protocol
		    	md->bigstate = MASTER_DELAYANS;
			}
		    return 0; // wait
		case MASTER_DELAYANS:
			if(e == EVENT_NEWPACKET)
			{
				if(ptype != PTPM_DELAY)
				{
					printf("expected delay!");
					md->bigstate = MASTER_START;
					break; // restart
				}
				else
				{
					// TODO: replace it with the SOCKET timestamp if SO_TIMESTAMP available
					int t4[2];
					ptp_get_time(t4); // == t4
				    md->sm_diff =  (TO_NSEC(t4) - TO_NSEC(treceived2)); // t4-t3
				    md->bigstate = MASTER_LOOPEND;				   
				 	break; // next state
				}
			}
			return 0; // ignore
		case MASTER_LOOPEND:
			{
			long long offset = (md->ms_diff - md->sm_diff)/2;
	        long long delay = (md->ms_diff + md->sm_diff)/2;
	         /* calculate averages, min, max */
	        md->sum_offset += offset;
	        if (md->largest_offset < offset) {
	            md->largest_offset = offset;
	        }
	        if (md->smallest_offset > offset) {
	            md->smallest_offset = offset;
	        }
	        
	        md->sum_delay += delay;
	        if (md->largest_delay < delay) {
	            md->largest_delay = delay;
	        }
	        if (md->smallest_delay > delay) {
	            md->smallest_delay = delay;
	        }
        	if(++md->i >= md->nsteps)
        	{
        		md->bigstate = MASTER_DONE;        		
        		md->steps = md->i;
        		return 1; // completion
        	}
        	else
        	{
				int dummy[2] = {0,0};
		        ptp_send_packet_in(md, PTPM_NEXT, dummy);
        		md->bigstate = MASTER_SYNC;
		        return 0; // wait
        	}
        		}
    	default:
        	return 0; // ignore
		}
}

