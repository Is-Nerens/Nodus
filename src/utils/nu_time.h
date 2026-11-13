#pragma once

// Platform detection
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
#endif
#ifdef PLATFORM_MACOS
    #include <mach/mach_time.h>
#endif
#ifdef PLATFORM_LINUX
    #include <time.h>
#endif

// -----------------------------
// High precision wall clock timer (microseconds)
// Returns a monotonically increasing uint64 count of microseconds.
// -----------------------------

#ifdef PLATFORM_WINDOWS

static LARGE_INTEGER _TIME_freq;
static int _TIME_initialized = 0;

static inline void _TIME_init() {
    if (!_TIME_initialized) {
        QueryPerformanceFrequency(&_TIME_freq);
        _TIME_initialized = 1;
    }
}

// Returns elapsed microseconds since an arbitrary epoch.
static inline uint64 nu_time(void) {
    _TIME_init();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    // Convert to microseconds without overflowing: seconds first, then remainder.
    uint64 seconds = (uint64)now.QuadPart / (uint64)_TIME_freq.QuadPart;
    uint64 remainder = (uint64)now.QuadPart % (uint64)_TIME_freq.QuadPart;
    return seconds * 1000000ull + (remainder * 1000000ull) / (uint64)_TIME_freq.QuadPart;
}

#endif

#ifdef PLATFORM_MACOS

static mach_timebase_info_data_t _TIME_timebase = {0};

static inline void _TIME_init() {
    if (_TIME_timebase.denom == 0) {
        mach_timebase_info(&_TIME_timebase);
    }
}

// Returns elapsed microseconds since an arbitrary epoch.
static inline uint64 nu_time(void) {
    _TIME_init();
    uint64 ticks = mach_absolute_time();
    // nanos = ticks * numer / denom; microseconds = nanos / 1000
    return (ticks * (uint64)_TIME_timebase.numer / (uint64)_TIME_timebase.denom) / 1000ull;
}

#endif

#ifdef PLATFORM_LINUX

// Returns elapsed microseconds since CLOCK_MONOTONIC epoch.
static inline uint64 nu_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec * 1000000ull + (uint64)ts.tv_nsec / 1000ull;
}

#endif
