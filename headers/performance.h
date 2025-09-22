#pragma once

#include <x86intrin.h>  
#include <time.h>
#include <stdio.h>

unsigned long long _PERFORMANCE_start_cycles;
static clock_t _PERFORMANCE_start_time;

void start_measurement() {
    _PERFORMANCE_start_cycles = __rdtsc();
}

void end_measurement() {
    unsigned long long end_cycles = __rdtsc();
    printf("CPU cycles: %llu\n", end_cycles - _PERFORMANCE_start_cycles);
}

void timer_start() {
    _PERFORMANCE_start_time = clock();
}

void timer_stop() {
    clock_t end_time = clock();
    double duration_seconds = (double)(end_time - _PERFORMANCE_start_time) / CLOCKS_PER_SEC;
    printf("Elapsed time: %.6f seconds\n", duration_seconds);
}