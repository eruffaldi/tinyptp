#include <stddef.h>
#include "ptp_common.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>

#define NUM_OF_TIMES 10

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


enum MasterState { MASTER_START, MASTER_WAIT0,MASTER_TIMES, MASTER_LOOP0R, MASTER_LOOP0, MASTER_LOOP1, MASTER_LOOP1R,MASTER_LOOPEND, MASTER_DONE};

static const char * names[] = {"PTPM_START", "PTPM_SYNC", "PTPM_TIMES", "PTPM_DELAY", "PTPM_NEXT", "PPTS_HELLO"};

int master_sm(struct common_data * md, enum Event e, unsigned char * data, int n)
{
	int ptype = -1;
	int treceived[2];

	// multiple steps
	if(e == EVENT_RESET)
	{
		md->bigstate = MASTER_START;
	}
	else if(e == EVENT_NEWPACKET)
	{
		if(n == 12 && strncmp((char*)data,"PTP",3) == 0)
		{			
			ptype = (int)data[3];
			treceived[0] = *(int*)data+4;
			treceived[1] = *(int*)data+8;
			printf("master received %s(%d) with t=%d,%d\n",ptype >= 0 && ptype < PTPX_MAX ? names[ptype] : "unknown",ptype,treceived[0],treceived[1]);
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
			md->largest_offset = LONG_MIN;
		    md-> smallest_offset = LONG_MAX;
		    md-> sum_offset = 0;
		    md-> smallest_delay = LONG_MAX;
		    md-> largest_delay = LONG_MIN;
		    md-> sum_delay = 0;
		    md->nsteps = NUM_OF_TIMES;
		    md->i = 0;
		    md->bigstate = MASTER_WAIT0;
		    int dummy[2] = {0,0};
		    ptp_send_packet(md->sock,PTPM_START,dummy,md->clientdata,md->clientdatasize);
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
				    ptp_send_packet(md->sock,PTPM_TIMES,d,md->clientdata,md->clientdatasize);
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
					md->bigstate = MASTER_LOOP0;
					break; // next state
				}
			}
		    return 0; // ignore
		case MASTER_LOOP0:
			{	
				ptp_get_time(md->t1); 
		    	ptp_send_packet(md->sock,PTPM_SYNC,md->t1,md->clientdata,md->clientdatasize); // no need to send t1 actually, alternatively send index i
				md->bigstate = MASTER_LOOP0R;
			}
		    return 0; // wait
		case MASTER_LOOP0R:
			if(e == EVENT_NEWPACKET)
			{
				if(ptype != PTPM_SYNC)
				{
					printf("expected SYNC\n");
					md->bigstate = MASTER_START;
					break;  // restart
				}
				md->ms_diff = (TO_NSEC(treceived) - TO_NSEC(md->t1));
				md->bigstate = MASTER_LOOP1;
				break; // next state
			}
			else
				return 0; // ignore
		case MASTER_LOOP1:
			{	
				ptp_get_time(md->t3);
		    	ptp_send_packet(md->sock,PTPM_DELAY,md->t3,md->clientdata,md->clientdatasize); // t3 no need to send, alternatively send index i
				md->bigstate = MASTER_LOOP1R;
			}
		    return 0; // wait
		case MASTER_LOOP1R:
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
					// TODO: store received data inside t3, measure t4 now
				    //ptp_receive_packet(md->sock, md->t3, sizeof(md->t3), md->t4, NULL);
				    md->sm_diff =  (TO_NSEC(treceived) - TO_NSEC(md->t3));
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
        	md->i++;
        	if(++md->i >= md->nsteps)
        	{
        		md->bigstate = MASTER_DONE;        		
        		md->steps = md->i;
        		return 1; // completion
        	}
        	else
        	{
				int dummy[2] = {0,0};
        		md->bigstate = MASTER_LOOP0;
		        ptp_send_packet(md->sock, PTPM_NEXT, dummy, md->clientdata,md->clientdatasize);
		        return 0; // wait
        	}
        		}
    	default:
        	return 0; // ignore
		}
}

#if 0

/* calculate master-slave difference */
static long sync_packet(int *sock, struct sockaddr_in *slave) {
    int t1[2], t2[2];
    send_packet(sock, "sync_packet", 11, t1, slave);
    receive_packet(sock, t2, sizeof(t2), NULL, NULL);
    return (TO_NSEC(t2) - TO_NSEC(t1));
}

/* calculate slave-master difference */
static long delay_packet(int *sock, struct sockaddr_in *slave) {
    int t3[2], t4[2];
    send_packet(sock, "delay_packet", 12, NULL, slave);
    receive_packet(sock, t3, sizeof(t3), t4, NULL);
    return (TO_NSEC(t4) - TO_NSEC(t3));
}

/* IEEE 1588 PTP master implementation */
static void sync_clock(int *sock, struct sockaddr_in *slave) {
    long largest_offset = LONG_MIN;
    long smallest_offset = LONG_MAX;
    long sum_offset = 0;
    long smallest_delay = LONG_MAX;
    long largest_delay = LONG_MIN;
    long sum_delay = 0;
    int i; /* to prevent C99 error in for loop */
    char useless_buffer[FIXED_BUFFER];
    
    printf("Running IEEE1588 PTP %d times...\n", NUM_OF_TIMES);
    receive_packet(sock, useless_buffer, FIXED_BUFFER, NULL, NULL);

    /* run protocol however many number of times */
    for(i = 0; i < NUM_OF_TIMES; i++) {
        long ms_diff = sync_packet(sock, slave);
        long sm_diff = delay_packet(sock, slave);

        /* http://www.nist.gov/el/isd/ieee/upload/tutorial-basic.pdf  <- page 20 to derive formulas */
        long offset = (ms_diff - sm_diff)/2;
        long delay = (ms_diff + sm_diff)/2;

        /* calculate averages, min, max */
        sum_offset += offset;
        if (largest_offset < offset) {
            largest_offset = offset;
        }
        if (smallest_offset > offset) {
            smallest_offset = offset;
        }
        
        sum_delay += delay;
        if (largest_delay < delay) {
            largest_delay = delay;
        }
        if (smallest_delay > delay) {
            smallest_delay = delay;
        }

        send_packet(sock, "next", 4, NULL, slave);
    }

    /* print results */
    printf("Average Offset = %ldns\n", sum_offset/(NUM_OF_TIMES));
    printf("Average Delay = %ldns\n", sum_delay/(NUM_OF_TIMES));
    
    printf("Smallest Offset = %ldns\n", smallest_offset);
    printf("Smallest Delay = %ldns\n", smallest_delay);
    
    printf("Largest Offset = %ldns\n", largest_offset);
    printf("Largest Delay = %ldns\n", largest_delay);


    printf("Done!\n");
}
#endif
