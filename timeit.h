#ifndef __TBS_TIMEIT__
#define __TBS_TIMEIT__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#ifndef TIT_MIN_TOTAL_RUNTIME_MS
#define TIT_MIN_TOTAL_RUNTIME_MS                                        1500
#endif

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

static inline void _tit_fatal(const char *message) {
    fprintf(stderr, "TIMEIT Error: %s\n", message);
    exit(1);
}

static inline tit_time_us _tit_now() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        _tit_fatal("Cannot get current time!");
    }
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

typedef struct {
    tit_time_us ts;
    tit_timespan_us runtime;
} _tit_single_run_t;

typedef struct {
    _tit_single_run_t *runs;
    int runs_count;
    int runs_arr_capacity;
    tit_timespan_us total_runtime;
} _tit_stats_collector_t;

static inline _tit_stats_collector_t _tit_stats_collector_create() {
    const int default_runs_arr_capacity = 1024 * 1024;
    _tit_stats_collector_t collector = {
        .runs               = malloc(sizeof(*collector.runs) * default_runs_arr_capacity),
        .runs_arr_capacity  = default_runs_arr_capacity,
        .runs_count         = 0,
        .total_runtime      = 0
    };
    if (collector.runs == NULL) {
        _tit_fatal("Cannot allocate memory for collecting statistics!");
    }
    return collector;
}

static inline void _tit_stats_collector_cleanup(_tit_stats_collector_t *collector) {
    free(collector->runs);
}

static inline void _tit_stats_collector_ensure_capacity(_tit_stats_collector_t *collector, int required_capacity) {
    if (required_capacity <= collector->runs_arr_capacity) {
        return;
    }

    int new_capacity = collector->runs_arr_capacity;
    while (new_capacity < required_capacity) {
        new_capacity *= 2;
    }

    collector->runs = realloc(collector->runs, sizeof(*collector->runs) * new_capacity);
    if (collector->runs == NULL) {
        _tit_fatal("Cannot re-allocate memory for collecting statistics!");
    }
    collector->runs_arr_capacity = new_capacity;
}

static inline void _tit_stats_collector_append(_tit_stats_collector_t *collector, const _tit_single_run_t *run) {
    _tit_stats_collector_ensure_capacity(collector, collector->runs_count + 1);
    collector->runs[collector->runs_count++] = *run;
    collector->total_runtime += run->runtime;
}

static inline int _tit_sinle_run_cmp(const void *lhs, const void *rhs) {
    return ((_tit_single_run_t *)lhs)->runtime - ((_tit_single_run_t *)rhs)->runtime;
}

static inline void _tit_stats_collector_sort_runs(_tit_stats_collector_t *collector) {
    qsort(collector->runs, collector->runs_count, sizeof(*collector->runs), _tit_sinle_run_cmp);
}

#define _TIT_SINGLE_RUN(code)                                                   \
({                                                                              \
    const tit_timespan_us _tit_start = _tit_now();                              \
    code;                                                                       \
    const tit_timespan_us _tit_runtime = _tit_now() - _tit_start;               \
    (_tit_single_run_t) {                                                       \
        .ts         = _tit_start,                                               \
        .runtime    = _tit_runtime                                              \
    };                                                                          \
})

#define _TIT_COLLECT_STATS(code)                                                \
({                                                                              \
    _tit_stats_collector_t _tit_collector = _tit_stats_collector_create();      \
    do {                                                                        \
        const _tit_single_run_t _tit_single_run = _TIT_SINGLE_RUN(code);        \
        _tit_stats_collector_append(&_tit_collector, &_tit_single_run);         \
        if (_tit_single_run.runtime == 0) {                                     \
            _tit_collector.total_runtime += 1;                                  \
        }                                                                       \
    } while (_tit_collector.total_runtime < 1000 * TIT_MIN_TOTAL_RUNTIME_MS);   \
    _tit_stats_collector_sort_runs(&_tit_collector);                            \
    _tit_collector;                                                             \
})

static inline double _tit_abs(double x) {
    return x >= 0 ? x : -x;
}

/*
 * To avoid a dependency to libm, a simple sqrt-function is used. Otherwise
 * the user of the library would be forced to link against libm.
 */
static inline double _tit_sqrt(double x) {
    const double max_diff = 1e-12;
    double x_sqrt = 1.0;
    while (_tit_abs(x_sqrt * x_sqrt - x) >= max_diff) {
        x_sqrt = (x / x_sqrt + x_sqrt) / 2;
    }
    return x_sqrt;
}

static inline tit_execution_stats_t _tit_executions_stats_compute(const _tit_stats_collector_t *collector) {
    if (collector->runs_count < 1) {
        _tit_fatal("Need at least one run to compute some statistics!");
    }
    tit_execution_stats_t stats = {
        .execution_count    = collector->runs_count,
    };
    const _tit_single_run_t *min = &collector->runs[0];
    const _tit_single_run_t *max = &collector->runs[collector->runs_count - 1];
    stats.min    = min->runtime;
    stats.min_ts = min->ts;
    stats.max    = max->runtime;
    stats.max_ts = max->ts;
    stats.avg    = collector->total_runtime / collector->runs_count;
    stats.q95    = collector->runs[(int)(collector->runs_count * 0.95)].runtime;
    stats.q99    = collector->runs[(int)(collector->runs_count * 0.99)].runtime;
    stats.q99_9  = collector->runs[(int)(collector->runs_count * 0.999)].runtime;
    stats.q99_99 = collector->runs[(int)(collector->runs_count * 0.9999)].runtime;

    if (collector->runs_count > 1) {
        double tmp_sum = 0;
        for (int i = 0; i < collector->runs_count; ++i) {
            const double tmp_factor = collector->runs[i].runtime - stats.avg;
            tmp_sum += tmp_factor * tmp_factor;
        }
        stats.standard_deviation = _tit_sqrt(tmp_sum / (collector->runs_count - 1));
    }

    return stats;
}

static inline void _tit_stringify_timespan(tit_timespan_us timespan) {
    if (timespan < 1) {
        printf("%.3f ns", timespan * 1000);
    } else if (timespan < 1000) {
        printf("%.3f us", timespan);
    } else if (timespan < 1000 * 1000) {
        printf("%.3f ms", timespan / 1000);
    } else {
        printf("%.3f s", timespan / (1000 * 1000));
    }
} 

static inline void _tit_execution_stats_dump(const tit_execution_stats_t *stats) {
    printf("TIMEIT\n");

    if (stats->execution_count < 1) {
        _tit_fatal("Cannot dump statistics about less than 1 run!");
    }

    printf("  Executions: %ld\n", stats->execution_count);

    printf("  Min [at %ld]:   ", stats->min_ts);
    _tit_stringify_timespan(stats->min);
    printf("\n");

    printf("  Max [at %ld]:   ", stats->max_ts);
    _tit_stringify_timespan(stats->max);
    printf("\n");

    printf("  Avg:             ");
    _tit_stringify_timespan(stats->avg);
    printf("\n");

    if (stats->execution_count > 1) {
        printf("  Std. deviation:  +/- ");
        _tit_stringify_timespan(stats->standard_deviation);
        printf("\n");
    }

    printf("  95%%-quantile:    ");
    _tit_stringify_timespan(stats->q95);
    printf("\n");

    printf("  99%%-quantile:    ");
    _tit_stringify_timespan(stats->q99);
    printf("\n");

    printf("  99.9%%-quantile:  ");
    _tit_stringify_timespan(stats->q99_9);
    printf("\n");

    printf("  99.99%%-quantile: ");
    _tit_stringify_timespan(stats->q99_99);
    printf("\n");

    printf("  Code: %s\n", stats->code);
}

#define _TIT_STR(s)                                                                 #s

#define TIMEIT(exec_code)                                                           \
({                                                                                  \
    const _tit_stats_collector_t _tit_collector = _TIT_COLLECT_STATS(exec_code);    \
    tit_execution_stats_t _tit_stats =                                              \
        _tit_executions_stats_compute(&_tit_collector);                             \
    _tit_stats.code = #exec_code;                                                   \
    _tit_execution_stats_dump(&_tit_stats);                                         \
    _tit_stats;                                                                     \
})

#endif
