#include <stdio.h>
#include <stopwait/stopwait.h>

#define IPADDR INADDR_LOOPBACK
#define PORT 0x1234 

#define MAXN 1024 
char data_buf[MAXN];

int main() {
    for (int i = 0; i < MAXN; i++) data_buf[i] = i;
    sw_connect(IPADDR, PORT);
    size_t offset = 0, len = MAXN;

    while (len > 0) {
        ssize_t sndn = sw_send(&data_buf[offset], len);
        offset += sndn; 
        len -= sndn;
    }
    sw_closeclt();

    return 0;

}
