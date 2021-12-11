#include <stdint.h>
#include <stdio.h>

#define BLOCK_BITS 8 
uint16_t CRC16_Table[1 << BLOCK_BITS];

/*
1000 1000 xxxx xxxx xxxx xxxx  
gggg gggg gggg gggg g000 0000
    |
    v
  G_pre8
*/
void create_ltb16() {
    //uint16_t G = 0x8005;//omit the msb 1 1000 0000 0000 0101

    uint32_t poly = 0x18005 << (BLOCK_BITS - 1);
    uint8_t G_pre8 = poly >> 16;
    printf("poly=%#x, G_pre8=%#x\n",poly, G_pre8);
    #define mask 0xffff

    for (int X = 0; X < (1 << BLOCK_BITS); X++) {
        uint8_t S = X;
        uint16_t b = 0;
        for (int i = 0; i < BLOCK_BITS; i++) {
            if (S & 0x80) {
                b ^= ((poly >> i) & mask);
                S = (S ^ G_pre8) << 1; 
            }
            else {
                b ^= (0 & mask); //can be omitted
                S = (S ^ 0) << 1;
            }
        }
        CRC16_Table[X] = b;
    }    
}

/*
#define BLOCK_BITS_ 2
uint8_t CRC4_Table[1 << BLOCK_BITS_];

void create_ltb4() {
    //uint8_t G = 0x03;//omit the msb; 1 0011

    uint32_t poly = 0x13 << (BLOCK_BITS_ - 1);
    uint8_t G_pre2 = poly >> 4;
    printf("poly=%#x, G_pre2=%#x\n",poly, G_pre2);
    #define mask4 0xf

    for (int X = 0; X < (1 << BLOCK_BITS_); X++) {
        uint8_t S = X;
        uint16_t b = 0;
        for (int i = 0; i < BLOCK_BITS_; i++) {
            if (S & 0x2) {
                b ^= ((poly >> i) & mask4);
                S = (S ^ G_pre2 ) << 1; 
            }
            else {
                b ^= (0 & mask4); //can be omitted
                S = (S ^ 0 ) << 1;
            }
        }
        CRC4_Table[X] = b;
        printf("%0#.4x, ", CRC4_Table[X]);
    }    
}
*/

int main() {
#define NLINE 8
    int enums = sizeof(CRC16_Table) / sizeof(CRC16_Table[0]);
    int lines = (enums - 1) / NLINE + 1;
    create_ltb16();
    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < 8 && (i * NLINE + j < enums); j++) {
            printf("%0#.4x, ", CRC16_Table[i * NLINE + j]);
        }
        printf("\n");
    }    
    //create_ltb4();
}