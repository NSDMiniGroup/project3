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

    return 0;
}

void sw_closeclt() { close(cltsock); }

static inline void fill_data(int *state, const void* data, size_t len) {
    //fill with data 
    sndbuf.flag = F_SEND; sndbuf.dlen = (uint16_t)len;

    memcpy(sndbuf.data, data, len);
    if (*state == STATE_SEND0) {
        sndbuf.seq = FRAME0; 
        *state = STATE_WAIT0; 
    }else {
        sndbuf.seq = FRAME1; 
        *state = STATE_WAIT1;
    }
    //check
    sndbuf.check = check();
}

static inline void send_data(size_t len) {
    len += SWHEADSIZE;
    //send data
printf("send %lu bytes\n", len);
    ssize_t sndn = 0;
    while (len > 0) {
        printf("snd_buf_size:%lu\n", sizeof(sndbuf));
        assert( (sndn = send(cltsock, (char *)(&sndbuf) + sndn, len, 0)) != -1 );
        len -= sndn;
    }
}

static inline void recv_ack(int *state, size_t len) {
    timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    fd_set readfds;
    
    int event;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(cltsock, &readfds);
        int nfds = select(1024, &readfds, NULL, NULL, &timeout);
        if (nfds < 0) perror("Error");
        if (nfds == 0) { //TIMEMOUT
            event = EVENT_TIMEOUT;
        }else {
            if (FD_ISSET(cltsock, &readfds)) {
                ssize_t rcvn = recv(cltsock, &rcvbuf, sizeof(rcvbuf), 0);
                if (rcvn != sizeof(rcvbuf)) { 
                    //handle the case of rcvn == sizeof(rcvbuf)
                }
                //handle check
                event = rcvbuf.flag == F_ACK ? EVENT_ACK : EVENT_TIMEOUT;
            }
        }

        switch (event) {
            case EVENT_TIMEOUT: 
                send_data(len); 
                printf("timeout...");
                timeout.tv_sec = TIMEOUT;
                timeout.tv_usec = 0;
                break;
            case EVENT_ACK: 
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

    for (; ;) {
        switch (state) {
            case STATE_SEND0: case STATE_SEND1:
                fill_data(&state, data, len); send_data(len); break;
            case STATE_WAIT0: case STATE_WAIT1:
                recv_ack(&state, len); return len;
        }
    }
}