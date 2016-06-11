

enum Event { EVENT_NONE, EVENT_RESET, EVENT_NEWPACKET };
enum Packet { PTPM_START, PTPM_SYNC, PTPM_TIMES, PTPM_DELAY, PTPM_NEXT, PTPS_HELLO,PTPX_MAX };

struct common_data
{
    long long largest_offset ;
    long long smallest_offset ;
    long long sum_offset ;
    long long smallest_delay ;
    long long largest_delay ;
    long long sum_delay ;
    int i;
    int nsteps ;
    int bigstate;
    int t1[2],t3[2];
    long ms_diff,sm_diff;
    long offset,delay;
    int steps;

    int sock;
    void * clientdata;
    int clientdatasize;
};

int master_sm(struct common_data * md, enum Event e, unsigned char * data, int n);


struct slave_data
{
    int bigstate;
    int sock;
    void * clientdata;
    int clientdatasize;
};

int slave_sm(struct common_data * md, enum Event e, unsigned char * data, int n);

void ptp_get_time(int in[2]);

void ptp_send_packet(int sock, int what, int t_send[2], void *clientdata,int clientdatasize);

#define TO_NSEC(t) (((long)t[0] * 1000000000L) + t[1])
