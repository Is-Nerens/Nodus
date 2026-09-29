#pragma once

typedef struct NU_Timer {
    u32 intervalMs;
    u64 lastFiredTime;
    NU_Callback callback;
    void* args;
    bool willBeRemoved;
} NU_Timer;
