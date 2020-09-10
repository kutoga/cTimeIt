#include <stdio.h>
#include "timeit.h"

int calculate(int x) {
    for (int i = 0; i < 1000; ++i) {
        x = x * x + ~x * (1 - x);
        x = x ^ 13;
        x += 97;
        x *= x | 13;
        x -= 12;
    }
    return x;
}

int main() {
    int x = 2;
    int y;
    tit_execution_stats_t stats = TIMEIT(y = calculate(x));
    printf("Average runtime in us: %f\n", stats.avg);
    return 0;
}