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
    tit_execution_stat_t stat = TIMEIT({
        for (int i = 0; i < 3; ++i) {
            calculate(x);
        }
    });
    printf("Code:\n%s\n", stat.code);
    return 0;
}