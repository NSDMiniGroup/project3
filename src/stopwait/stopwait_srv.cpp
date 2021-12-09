#include <stopwait/stopwait.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// common
static SWFrame sndbuf, rcvbuf;
static int check() {
    //TODO
    return 0;
}

// receiver
static int srvsock, peersock; 
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

    assert ( (peersock = accept(srvsock, NULL, NULL)) != -1 );

    return 0;
}

void sw_closesrv() { close(srvsock); }

static inline void send_ack() {
     //fill with data 
    sndbuf = { .flag = F_ACK, .seq = 0, .dlen = 0, };
    //check
    sndbuf.check = check();

    size_t len = SWHEADSIZE;
    //send data
    ssize_t sndn = 0;
    while (len > 0) {
        assert( (sndn = send(peersock, (char *)(&sndbuf) + sndn, len, 0)) != -1 );
        //应该把所有字段通过memcpy的方式放在一个buf里面？编译器可能会对结构体有不同的对齐方式（或者说额外的字节填充）。
        len -= sndn;
    }
}

static inline void recv_data0(int *state, void *buf, size_t *len) {
    for( ; ; ) {
        ssize_t rcvn = recv(peersock, &rcvbuf, sizeof(rcvbuf), 0);
        if (rcvn == 0) { *len = 0; return;}
        perror("0");
        //handle rcvn != sizeof(rcvbuf)
        if (rcvn != sizeof(rcvbuf)) { }
        //handle check
        if (rcvbuf.flag == F_SEND) {
            int event = rcvbuf.seq == FRAME0 ? EVENT_FRAME0 : EVENT_FRAME1;
            switch (event) {
                case EVENT_FRAME0:
                    *len = *len < rcvbuf.dlen ? *len : rcvbuf.dlen;
                    memcpy(buf, rcvbuf.data, *len);
                    send_ack();
                    *state = STATE_RECV1;
                    return;
                case EVENT_FRAME1:
                    send_ack(); break;
                default: assert(0);
            }
        }
    }  
}

static inline void recv_data1(int *state, void *buf, size_t *len) {
    for( ; ; ) {
        ssize_t rcvn = recv(peersock, &rcvbuf, sizeof(rcvbuf), 0);
        perror("1");
        if (rcvn == 0) { *len = 0; return; }
        //handle rcvn != sizeof(rcvbuf)
        if (rcvn != sizeof(rcvbuf)) { }
        //handle check
        if (rcvbuf.flag == F_SEND) {
            int event = rcvbuf.seq == FRAME0 ? EVENT_FRAME0 : EVENT_FRAME1;
            switch (event) {
                case EVENT_FRAME1:
                    *len = *len < rcvbuf.dlen ? *len : rcvbuf.dlen;
                    memcpy(buf, rcvbuf.data, *len);
                    send_ack();
                    *state = STATE_RECV0;
                    return;
                case EVENT_FRAME0:
                    send_ack(); break;
                default: assert(0);
            }
        }
    }  
}

ssize_t sw_recv(void* buf, size_t len){
/*如果len < rcvbuf.dlen, 则(len, rcvbuf.dlen]的区间的数据会被丢弃，
sw_recv不负责,invoker应该自己保证buf足够大*/
    static int state = STATE_RECV0;
printf("%s\n", state ? "STATE_RECV1" : "STATE_RECV0");
    switch (state) {     
        case STATE_RECV0: 
            recv_data0(&state, buf, &len); break;
        case STATE_RECV1:
            recv_data1(&state, buf, &len); break;
        default: assert(0);break;
    }
    return len;
}
