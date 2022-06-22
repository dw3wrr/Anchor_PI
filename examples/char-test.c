#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

int main() {
    // char *test = "/";
    // printf("String: %s\n", test);
    // printf("Num: %d\n", test[3]);
    // printf("%x\n", -1);
    // printf("%x\n",1);
    // printf("%x\n",2);
    // printf("%x\n",-2);
    // printf("%x\n",2147483647);
    // printf("%.8x\n",0x12345678); 
    // printf("%.8x\n",0x12345678 >> 8);
    // printf("%.8x\n",(0x12345678 >> 8) & 0xff);
    // printf("%.8x\n",0x12345678 & 0xff);
    // printf("%.8x\n",1 & 0xff);
    // int a = 0x12345678;
    // a = 3840;
    // uint8_t b = a & 0xff;
    // uint8_t c = (a >> 8) & 0xff;
    // printf("Size: %d, Value %x\n", sizeof(a), a);
    // printf("Size: %d, Value %x\n", sizeof(b), b);
    // printf("Size: %d, Value %x\n", sizeof(c), c);
    // printf("%x\n",-2147483648);
    // uint8_t a = 0;


    // uint8_t x[5] = {0};

    // for(int i = 0; i < 5; i++) {
    //     printf(BYTE_TO_BINARY_PATTERN,BYTE_TO_BINARY(x[i]));
    // }
    // int test = 1;

    // int index_num = 4 - ((test-1) / 8);
    // printf("\nIndex to insert: %d\n",index_num);
    
    // int insert_bit = (test - 1) % 8;
    // printf("Bit to insert: %d\n", insert_bit);

    // printf(BYTE_TO_BINARY_PATTERN"\n",BYTE_TO_BINARY(x[index_num]));
    // x[index_num] = (int)(pow(2,insert_bit) + 1e-9);
    // printf(BYTE_TO_BINARY_PATTERN"\n",BYTE_TO_BINARY(x[index_num]));

    // for(int i = 0; i < 5; i++) {
    //     printf(BYTE_TO_BINARY_PATTERN,BYTE_TO_BINARY(x[i]));
    // }
    // printf("\n'%s'\n", x);

    // uint8_t test1[10] = {0,0,0,0,0,1,2,3,4,5};
    // uint8_t test2[10] = {0};

    // for(int i = 0; i < 10; i++) {
    //     printf("%d",test1[i]);
    // }
    // printf("\n");
    // for(int i = 0; i < 10; i++) {
    //     printf("%d",test2[i]);
    // }
    // printf("\n");

    // //when using memcpy if using uint8_t array, must have & in front and use
    // memcpy(test2, &test1[5], 4);

    // for(int i = 0; i < 10; i++) {
    //     printf("%d",test1[i]);
    // }
    // printf("\n");
    // for(int i = 0; i < 10; i++) {
    //     printf("%d",test2[i]);
    // }
    // printf("\n");

    // uint8_t a = 3;
    // uint8_t b = 2;

    // printf("%d\n",a | b);

    // uint8_t *test_string = "test\0";
    // printf("%d\n", strlen(test_string));
    // printf("done\n");

    int a = 0;
    int *b;

    *b = 10;
    a = *b;

    printf("testing\n");
    printf("%d\n", a);
    printf("%d\n", *b);

    return 0;
}