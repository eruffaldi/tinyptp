#pragma once

enum Event { EVENT_NONE, EVENT_RESET, EVENT_NEWPACKET };
enum Packet { PTPM_START, PTPM_SYNC, PTPM_TIMES, PTPM_DELAY, PTPM_NEXT, PTPS_HELLO,PTPX_MAX };

struct master_data
{
    // results below
    long long largest_offset ;
    long long smallest_offset ;
    long long sum_offset ;
    long long smallest_delay ;
    long long largest_delay ;
    long long sum_delay ;

    // state
    int steps; // effective
    int bigstate; // state

    // params here
    int nsteps ; // planned
    void * clientdata; 
    int clientdatasize;

    // internal data
    int i; // counter
    int t1[2],t3[2];
    long long ms_diff,sm_diff;
    long long offset,delay;

    // networking
    int sock;
};

int master_sm(struct master_data * md, enum Event e, unsigned char * data, int n);


struct slave_data
{
    int bigstate;
    int sock;
    void * clientdata;
    int clientdatasize;
};

int slave_sm(struct slave_data * md, enum Event e, unsigned char * data, int n);

void ptp_get_time(int in[2]);

void ptp_send_packet(int sock, unsigned char * , int n, void *clientdata,int clientdatasize);

#define TO_NSEC(t) (((long long)t[0] * 1000000000L) + t[1])
