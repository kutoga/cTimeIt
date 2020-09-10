# cTimeIt

cTimeIt is a header-only library which basically only exposes one macro: `TIMEIT`.
Just download and include `timit.h` to use the library.

It can be used to measure the time required to execute some code.

Example:
```c
#include <stdio.h>
#include "timeit.h"

/*
 * A "complex" function for which we want to know how long an execution takes.
 */
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

    /*
     * Measure the time and print the statistics.
     */
    TIMEIT(y = calculate(x));

    return 0;
}
```

Output:
```bash
➜  /mnt/c/git/source/c_timeit git:(master) ✗ ./main 
TIMEIT
  Executions: 394187
  Min [at 1599759670573824]:   0.000 ns
  Max [at 1599759671197718]:   58.763 ms
  Avg:             3.809 us
  Std. deviation:  +/- 212.395 us
  95%-quantile:    3.000 us
  99%-quantile:    4.000 us
  99.9%-quantile:  101.000 us
  99.99%-quantile: 2.547 ms
  Code: y = calculate(x)
➜  /mnt/c/git/source/c_timeit git:(master) ✗ 
```

The `TIMIT` macro actually returns a data structure with the printed information:
```c
typedef long tit_time_us;

typedef double tit_timespan_us;

typedef struct {
    const char *code;
    long execution_count;

    tit_timespan_us max;
    tit_time_us max_ts;

    tit_timespan_us min;
    tit_time_us min_ts;

    tit_timespan_us avg;
    tit_timespan_us standard_deviation;

    tit_timespan_us q95;
    tit_timespan_us q99;
    tit_timespan_us q99_9;
    tit_timespan_us q99_99;
} tit_execution_stats_t;
```

Usage:
```c
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
```
