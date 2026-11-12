#pragma once

typedef struct NU_Timer {
    uint32 intervalMs;
    uint64 lastFiredTime;
    NU_Callback callback;
    void* args;
    uint8 syncState;
    bool willBeRemoved;
} NU_Timer;
