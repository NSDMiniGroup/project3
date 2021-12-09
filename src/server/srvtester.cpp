#include <stdio.h>
#include <stopwait/stopwait.h> 

#define IPADDR INADDR_ANY
#define PORT 0x1234
#define MAXN 1024 

char data_buf[MAXN];

int main(){
    sw_listen(IPADDR, PORT);
    while (1) {
        ssize_t len = sw_recv(data_buf, MAXN);
        if (len == 0) { 
            printf("exit...\n");
            sw_closesrv();
            return 0;
        }
        printf("receive %ld bytes\n", len);
        /*
        for (int i = 0; i < len; i++)
            printf("%d\n", (int)(data_buf[i]));
        */
    }
}