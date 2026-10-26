#pragma once

#include <stdio.h>
#include <stdint.h>

// Platform detection
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
#endif
#ifdef PLATFORM_MACOS
    #include <mach/mach_time.h>
    #include <time.h>
#endif
#ifdef PLATFORM_LINUX
    #include <time.h>
#endif

// -----------------------------
// High precision wall clock timer (microseconds)
// -----------------------------

#ifdef PLATFORM_WINDOWS

static LARGE_INTEGER _PERFORMANCE_freq;
static LARGE_INTEGER _PERFORMANCE_start_time;
static int _PERFORMANCE_initialized = 0;

static inline void _PERFORMANCE_init() {
    if (!_PERFORMANCE_initialized) {
        QueryPerformanceFrequency(&_PERFORMANCE_freq);
        _PERFORMANCE_initialized = 1;
    }
}

static inline void timer_start() {
    _PERFORMANCE_init();
    QueryPerformanceCounter(&_PERFORMANCE_start_time);
}

static inline void timer_stop() {
    LARGE_INTEGER end_time;
    QueryPerformanceCounter(&end_time);
    double elapsed_us = (double)(end_time.QuadPart - _PERFORMANCE_start_time.QuadPart) * 1e6 /
                        (double)_PERFORMANCE_freq.QuadPart;
    printf("Elapsed time: %.3f us\n", elapsed_us);
}
#endif

#ifdef PLATFORM_MACOS

static uint64_t _PERFORMANCE_start_time = 0;
static mach_timebase_info_data_t _PERFORMANCE_timebase = {0};

static inline void _PERFORMANCE_init() {
    if (_PERFORMANCE_timebase.denom == 0) {
        mach_timebase_info(&_PERFORMANCE_timebase);
    }
}

static inline void timer_start() {
    _PERFORMANCE_init();
    _PERFORMANCE_start_time = mach_absolute_time();
}

static inline void timer_stop() {
    uint64_t end_time = mach_absolute_time();
    uint64_t elapsed_ns = (end_time - _PERFORMANCE_start_time) * _PERFORMANCE_timebase.numer / _PERFORMANCE_timebase.denom;
    double elapsed_us = (double)elapsed_ns / 1000.0;
    printf("Elapsed time: %.3f us\n", elapsed_us);
}
#endif

#ifdef PLATFORM_LINUX

static struct timespec _PERFORMANCE_start_time;
static int _PERFORMANCE_initialized = 0;

static inline void _PERFORMANCE_init() {
    if (!_PERFORMANCE_initialized) {
        _PERFORMANCE_initialized = 1;
    }
}

static inline void timer_start() {
    _PERFORMANCE_init();
    clock_gettime(CLOCK_MONOTONIC, &_PERFORMANCE_start_time);
}

static inline void timer_stop() {
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_us = (double)(end_time.tv_sec - _PERFORMANCE_start_time.tv_sec) * 1e6 +
                        (double)(end_time.tv_nsec - _PERFORMANCE_start_time.tv_nsec) / 1000.0;
    printf("Elapsed time: %.3f us\n", elapsed_us);
}

#endif
