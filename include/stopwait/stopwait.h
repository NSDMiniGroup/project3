#ifndef __STOPWAIT_H__
#define __STOPWAIT_H__

#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

// common
#define TIMEOUT 5

#define MAX_FRAME_DATA 128
typedef struct sw_frame{
	uint8_t   flag   : 2;
	uint8_t   seq    : 2;
	uint16_t  dlen   :12;
	uint16_t  check  :16;
	char data[MAX_FRAME_DATA];
}SWFrame;

enum { F_SEND, F_ACK, };
enum { FRAME0, FRAME1, };

#define SWHEADSIZE 4 

// sender
enum { EVENT_TIMEOUT,  EVENT_ACK, };
enum { STATE_SEND0, STATE_SEND1, STATE_WAIT0, STATE_WAIT1, };

// receiver
enum { EVENT_FRAME0, EVENT_FRAME1, };
enum { STATE_RECV0, STATE_RECV1, };
#define DELAY 5

ssize_t sw_send(const void* data, size_t len);
ssize_t sw_recv(void* buf, size_t len);
int sw_connect(uint32_t ipaddr, uint16_t port);
int sw_listen(uint32_t ipaddr, uint16_t port);
void sw_closeclt();
void sw_closesrv();

#endif
