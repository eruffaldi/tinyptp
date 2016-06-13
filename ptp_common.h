#pragma once

typedef long long ptp_time_t; // ns since epoch

// external events to the state machines: zero both and send EVENT_RESET to both
enum Event { EVENT_NONE, EVENT_RESET, EVENT_NEWPACKET };

//packet exchanged
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
    long long ms_diff,sm_diff;
    long long offset,delay;

    ptp_time_t alttime;

    // networking
    int sock;
};

struct slave_data
{
    int bigstate;
    int sock;
    void * clientdata;
    int clientdatasize;
    ptp_time_t alttime;
};

// master state machine
int master_sm(struct master_data * md, enum Event e, unsigned char * data, int n);

// state machine of the slave
int slave_sm(struct slave_data * md, enum Event e, unsigned char * data, int n);

/// returns time in nanoseconds with in[0] containing the high-part (seconds) and in[1] the nanoseconds
void ptp_get_time(ptp_time_t *in);

/// sends a packet via sock with given size and client data
void ptp_send_packet(int sock, unsigned char * , int n, void *clientdata,int clientdatasize);

//#define TO_NSEC(t) t (((long long)t[0] * 1000000000L) + t[1])
#define TO_NSEC(t) t
