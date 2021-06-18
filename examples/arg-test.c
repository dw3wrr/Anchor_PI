#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char* a = argv[1];
    a = "lol";
    printf("%s\n",a);
    for(int i = 0; i < argc; i ++){
        printf("%s\n",argv[i]);
    }
}