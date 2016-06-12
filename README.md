
Minimal PTP
===========

Simple implementation of master-slave PTP with a loop made of sync & delay exchanges.

The files are master.c and slave.c with slave being very minimal. For embedding it is needed to implement ptp_get_time and ptp_send_packet in both

The provided example uses UDP sockets for computing, and allows for an extra argument in slave_socket application for introducing a time offset for local machine testing: test_extra_offset_ns.

For FULL ptp look at ptpd that works also on Windows

Limitations
===========
- not the real protocol
- sync is measuring time at packet preparation
- sync should be sent regularly multiple times
- delay should be initiated by slave (here by master)
- reliability of UDP is not addressed
- OSX uses gettime but we could use another timer
- Windows tests needed

Ideas and Improvements
======================
1) In our target embedding (STM32F4) we are not using a very good timer (nanoseconds) so we could take into account the computation the granularity of the clock (in milliseconds) to have an error model of the result

2) We could use the Kernel timestamping in Linux for computing the precise timestamps in WRITING and RECEIVING:
	- SO_TIMESTAMP reports usec in msg structure of recvmsg
	- SO_TIMESTAMPNS same as nsec
	- SO_TIMESTAMPING returns the time of writing message (sendmsg)

	This means we need SYNCREP to which slave should answer with exact time of SYNC and the SYNCREP

	For Windows you need to use libpcap that it is supported by the ptpd daemon

	See: https://www.kernel.org/doc/Documentation/networking/timestamping.txt
	See for OSX: http://opensource.apple.com//source/network_cmds/network_cmds-329.2/ping.tproj/ping.c
	See ptpd: https://github.com/ptpd/ptpd/blob/5c7c22b4194d5f9acf1b60ac598d05796a031979/src/dep/net.c 
	
Thanks
======
The packet exchange and number crunching started from https://github.com/bestvibes/IEEE1588-PTP . I have made it state machine and created custom packets to reduce the state needed.

