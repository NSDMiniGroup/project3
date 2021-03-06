#include <stopwait/stopwait.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <crc/crc.h>
#include <time.h>
#include <unistd.h> //to use sleep

// common
#define MAX_FRAME_DATA 128
#define BUFSIZE  sizeof(SWHead) + MAX_FRAME_DATA + CRC_SIZE

static uint8_t sndbuf[BUFSIZE];
static uint8_t rcvbuf[BUFSIZE];
static uint8_t tmp_sndbuf[BUFSIZE];

#define GEN 1
#define CHECK 0

static int sock; //used by sw_send and sw_recv

static int error_sim(uint8_t *frame, size_t len) {
    memcpy(tmp_sndbuf, sndbuf, len);
    srandom(time(0)); 
    sleep(1);
    uint8_t pro = random() % 101;
    size_t idx =(size_t)(random() % len);
    uint8_t val;
printf("    pro = %d, ", pro);
    if (pro <= ERROR_PRO) {
        while ( ((val = (uint8_t)random()) == frame[idx]) );
printf("frame[%lu]=%d->", idx, frame[idx]);
         frame[idx] = val;
printf("%d\n", val);
        return 1;
    }
    return 0;
}

static inline void recover(uint8_t *frame, size_t len) {
    memcpy(sndbuf, tmp_sndbuf, len);
}

// sender 
static int cltsock;
int sw_connect(uint32_t ipaddr, uint16_t port) {
    cltsock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in srvaddr = {
        .sin_family = AF_INET, 
        .sin_port = htons(port), 
    };
    srvaddr.sin_addr.s_addr = htonl(ipaddr),

    assert( connect(cltsock, (sockaddr *)&srvaddr, sizeof(srvaddr)) != -1 );
    printf("connect %s : %d successfully!\n", inet_ntoa(srvaddr.sin_addr), port);

    sock = cltsock;

    return 0;
}

void sw_closeclt() { close(cltsock); }

static inline void fill_data(int *state, const void* data, size_t len) {
    //fill with data 
    ((SWHead *)sndbuf)->head.flag = F_SEND; 
    ((SWHead *)sndbuf)->head.dlen = (uint8_t)len;

    if (*state == STATE_SEND0) {
        ((SWHead *)sndbuf)->head.seq = FRAME0; 
        *state = STATE_WAIT0; 
    }else {
        ((SWHead *)sndbuf)->head.seq = FRAME1; 
        *state = STATE_WAIT1;
    }

    memcpy(sndbuf + sizeof(SWHead), data, len);

    //add the out-of-band data
    size_t framelen = sizeof(SWHead) + len;
    memset(sndbuf + framelen, 0, BUFSIZE - CRC_SIZE - framelen);
    //check
    uint16_t crc = htons(crc16(sndbuf, BUFSIZE - CRC_SIZE, GEN)); 
//printf("-->CRC=%d\n", crc);
    memcpy(sndbuf + BUFSIZE - CRC_SIZE, &crc, CRC_SIZE);
}

static inline void send_data(size_t len) {
    len = BUFSIZE;
    size_t tmplen = len;

    //error simulation
    int e = error_sim(sndbuf, len);
    //send data
printf("    send %lu bytes, %s\n", len, e == 0 ? "No error": "Has error");
    ssize_t sndn = 0;
    while (len > 0) {
        assert( (sndn = send(sock, sndbuf + sndn, len, 0)) != -1 );
        len -= sndn;
    }
    recover(sndbuf, tmplen);
}

static inline void recv_ack(int *state, size_t len) {
    timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    fd_set readfds;
    
    int event;
    int counter = 0; //record the dulplicate
    uint8_t expected_seq = *state == STATE_WAIT0 ? ACK0 : ACK1; 

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        int nfds = select(1024, &readfds, NULL, NULL, &timeout);
        if (nfds < 0) perror("Error");
        if (nfds == 0) { //TIMEMOUT
            event = EVENT_TIMEOUT;
        } else {
            if (FD_ISSET(sock, &readfds)) {
                ssize_t rcvn __attribute__((unused)) = recv(sock, &rcvbuf, BUFSIZE, 0);
                //handle the case of rcvn unexpected. 

                //handle check
                uint16_t r = crc16(rcvbuf, BUFSIZE, CHECK);
                if (r == 0 && ((SWHead *)rcvbuf)->head.flag == F_ACK ) {
                    event = ((SWHead *)rcvbuf)->head.seq == expected_seq ?
                            EVENT_EXPACK : EVENT_UNEXPACK;
printf("    recv an ack, r = %d\n", r);
                } else {
printf("    recv error, r = %d\n", r);
                    event = EVENT_TIMEOUT;
                    //if r != 0, there is a wrong in the frame, serve it as timeout
                }
            }
        }

        switch (event) {
            case EVENT_TIMEOUT: 
                printf("    timeout...\n");
                send_data(len); 
                timeout.tv_sec = TIMEOUT;
                timeout.tv_usec = 0;
                break;
            case EVENT_UNEXPACK:
                if (counter == MAXCNT) {
                    printf("    fast re-transition...\n");
                    send_data(len); 
                    timeout.tv_sec = TIMEOUT;
                    timeout.tv_usec = 0;
                    counter = 0;
                } else {
                    counter++;
printf("couter:%d\n", counter);
                }
                break;
            case EVENT_EXPACK: 
//printf("receive ack %d\n", expected_seq);
                *state = *state == STATE_WAIT0 ? 
                        STATE_SEND1 : STATE_SEND0; 
                return;
            default: assert(0);
        }
    }
}

ssize_t sw_send(const void* data, size_t len) {
    static int state = STATE_SEND0;
    len = len > MAX_FRAME_DATA ? MAX_FRAME_DATA : len;
printf("\nSend %d\n", state);
    for (; ;) {
        switch (state) {
            case STATE_SEND0: case STATE_SEND1:
                fill_data(&state, data, len); send_data(len); break;
            case STATE_WAIT0: case STATE_WAIT1:
                recv_ack(&state, len); return len;
        }
    }
}

// receiver
static int srvsock; 
int sw_listen(uint32_t ipaddr, uint16_t port) {
    srvsock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in srvaddr = {
        .sin_family = AF_INET, 
        .sin_port = htons(port), 
    };
    srvaddr.sin_addr.s_addr = htonl(ipaddr),

    assert( bind(srvsock, (sockaddr *)&srvaddr, sizeof(srvaddr)) != -1 );
    assert ( listen(srvsock, 5) != -1 );
    printf("listening at %s : %d\n", inet_ntoa(srvaddr.sin_addr), port);

    assert ( (sock = accept(srvsock, NULL, NULL)) != -1 );

    return 0;
}

void sw_closesrv() { close(srvsock); }

static void delay() {
    srandom(time(0));
    unsigned int t = random() % (DELAY + 1);
    sleep(t);
}

static inline void send_ack(char seq) {
     //fill with data 
    ((SWHead *)sndbuf)->head.flag = F_ACK; 
    ((SWHead *)sndbuf)->head.seq = seq; 
    ((SWHead *)sndbuf)->head.dlen = 0;

    //fill the out-of-band data
    memset(sndbuf + sizeof(SWHead), 0, BUFSIZE - sizeof(SWHead) - CRC_SIZE);
    //check
    uint16_t crc = htons(crc16(sndbuf, BUFSIZE - CRC_SIZE, GEN));
//printf("-->CRC=%d\n", crc);
    memcpy(sndbuf + BUFSIZE - CRC_SIZE, &crc, CRC_SIZE);

    size_t len = BUFSIZE;
    size_t tmplen = len;

    //error simulation
    int e = error_sim(sndbuf, len);
printf("    send ack, %s\n", e == 0? "No error":"Has error");
    //send data
    ssize_t sndn = 0;
    while (len > 0) {
        assert( (sndn = send(sock, sndbuf + sndn, len, 0)) != -1 );
        len -= sndn;
    }
    recover(sndbuf, tmplen);
}

static inline void recv_data0(int *state, void *buf, size_t *len) {
    for( ; ; ) {
        ssize_t rcvn = recv(sock, &rcvbuf, BUFSIZE, 0);
        if (rcvn == 0) {
            *len = 0; 
            close(sock);
            return; 
        }
        delay(); //delay reception
printf("    recv %ld\n", rcvn);
//       perror("0");

        //handle check
        uint16_t r = crc16(rcvbuf, BUFSIZE, CHECK);
        if (r != 0) {
printf("    recv an error frame, r = %d\n", r);
            continue; //there is a wrong in the frame, do nothing and wait untill the re-transition
        }

        if (((SWHead *)rcvbuf)->head.flag == F_SEND) {
            int event = ((SWHead *)rcvbuf)->head.seq == FRAME0 ? 
                        EVENT_FRAME0 : EVENT_FRAME1;
            switch (event) {
                case EVENT_FRAME0:
                    *len = *len < ((SWHead *)rcvbuf)->head.dlen ? 
                            *len : ((SWHead *)rcvbuf)->head.dlen;
                    memcpy(buf, rcvbuf + sizeof(SWHead), *len);
                    send_ack(0);
                    *state = STATE_RECV1;
                    return;
                case EVENT_FRAME1:
                    send_ack(1); break;
                default: assert(0);
            }
        }
    }  
}

static inline void recv_data1(int *state, void *buf, size_t *len) {
    for( ; ; ) {
        ssize_t rcvn = recv(sock, &rcvbuf, BUFSIZE, 0);
//       perror("1");
        if (rcvn == 0) { 
            *len = 0; 
            close(sock);
            return; 
        }

        delay(); //delay reception
printf("    recv %ld\n", rcvn);
        //handle check
        uint16_t r = crc16(rcvbuf, BUFSIZE, CHECK);
        if (r != 0) {
printf("    recv an error frame, r = %d\n", r);
            continue; //there is a wrong in the frame, do nothing and wait untill the re-transition
        }

        if (((SWHead *)rcvbuf)->head.flag == F_SEND) {
            int event = ((SWHead *)rcvbuf)->head.seq == FRAME0 ? 
                        EVENT_FRAME0 : EVENT_FRAME1;
            switch (event) {
                case EVENT_FRAME1:
                    *len = *len < ((SWHead *)rcvbuf)->head.dlen ? 
                            *len : ((SWHead *)rcvbuf)->head.dlen;
                    memcpy(buf, rcvbuf + sizeof(SWHead), *len);
                    send_ack(1);
                    *state = STATE_RECV0;
                    return;
                case EVENT_FRAME0:
                    send_ack(0); break;
                default: assert(0);
            }
        }
    }  
}

ssize_t sw_recv(void* buf, size_t len){
/*??????len < rcvbuf.dlen, ???(len, rcvbuf.dlen]?????????????????????????????????
sw_recv?????????,invoker??????????????????buf?????????*/
    static int state = STATE_RECV0;
printf("\nRecv %s\n", state ? "STATE_RECV1" : "STATE_RECV0");
    switch (state) {     
        case STATE_RECV0: 
            recv_data0(&state, buf, &len); break;
        case STATE_RECV1:
            recv_data1(&state, buf, &len); break;
        default: assert(0);break;
    }
    return len;
}