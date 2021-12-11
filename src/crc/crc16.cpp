#include <crc/crc.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <assert.h>

//正向(大端字节序)
uint16_t crc16(uint8_t *frame, size_t len, uint16_t tail) {
    uint16_t r = 0; //初始化为零，相当于给data前面补零，但并不会影响最终结果
    
    for (size_t i = 0; i < len; i++) {
        r = ((r << 8) | frame[i]) ^ CRC16_Table[(r >> 8) & 0xff];
    }
    
    uint8_t *f = (uint8_t *)&tail;
    for (size_t i = 0; i < sizeof(tail) / sizeof(uint8_t); i++) {
        r = ((r << 8) | f[i])^ CRC16_Table[(r >> 8) & 0xff];
    }
    return r;
}//用于网络传输时，返回值需要作网络序转换

/*tester
uint8_t data_frame[1024];
int main() {
    srandom(time(0));
    int cnt = 10000;
    while (cnt--) {
        for (int i = 0; i < 1024; i++) data_frame[i] = (uint8_t)random();
        uint16_t crc = crc16(data_frame, 1024, 0);
        assert(crc16(data_frame, 1024, htons(crc)) == 0);

           int idex = (uint16_t)random() % 1024;
           int val = (uint8_t)random();
           if (data_frame[idex] != val) {
                data_frame[idex] = val;
                assert(crc16(data_frame, 1024, htons(crc)) != 0);
           }
    }
    
//    printf("%0#.4x\n", crc);
//    printf("%0#.4x\n", crc16(data_frame, 1024, htons(crc)));
// 
//    data_frame[(uint16_t)random() % 1024] = (uint8_t)random();
//    printf("%0#.4x\n", crc16(data_frame, 1024, htons(crc)));
    
}
*/