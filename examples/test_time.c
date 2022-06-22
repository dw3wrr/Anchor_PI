#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

void timestamp() {
    struct timeval tv;
    struct timezone tz;
    struct tm *today;
    int zone;

    gettimeofday(&tv,&tz);
    //mingw_gettimeofday(&tv, &tz);
    time_t timer = tv.tv_sec;
    today = localtime(&timer);
    printf("TIME: %d:%0d:%0d.%d\n", today->tm_hour, today->tm_min, today->tm_sec, tv.tv_usec);
}

int main() {
    timestamp();
    return 0;
}