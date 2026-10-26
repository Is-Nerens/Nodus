#pragma once

#define THREADPOOL_MAX_THREADS 64
#define THREADPOOL_DEFAULT_CAPACITY 64
#define THREADPOOL_SPIN_COUNT 4096

typedef int (*WorkerFunction)(void* data);

typedef struct ThreadJob {
    WorkerFunction function;
    void* data;
} ThreadJob;

typedef struct ThreadPool {
    int threadCount;
    SDL_Thread* threads[THREADPOOL_MAX_THREADS];
    SDL_Mutex* mutex;
    SDL_Condition* workerCondition;   // workers wait here when no jobs
    SDL_Condition* doneCondition;     // main thread waits here for completion
    ThreadJob* jobs;
    int jobCapacity;
    int jobCount;
    int nextJob;
    int activeJobs;
    int completedJobs;
    int stopping;
} ThreadPool;

// Portable CPU pause/yield for spin loops.
static inline void ThreadPool_SpinPause(void) {
#if defined(_MSC_VER)
    _mm_pause();
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#else
    // Fallback: no-op. The spin still works, just less power-friendly.
#endif
}

static int ThreadPool_Worker(void* arg)
{
    ThreadPool* pool = arg;

    while (1) {
        SDL_LockMutex(pool->mutex);

        // Spin briefly before parking. If a job arrives during the spin
        // (common when parallel phases are close together in a frame),
        // we avoid a syscall wake entirely.
        int spins = 0;
        while (pool->nextJob >= pool->jobCount && !pool->stopping) {
            if (spins < THREADPOOL_SPIN_COUNT) {
                spins++;
                SDL_UnlockMutex(pool->mutex);
                ThreadPool_SpinPause();
                SDL_LockMutex(pool->mutex);
            } else {
                // Spun long enough; park until signaled.
                SDL_WaitCondition(pool->workerCondition, pool->mutex);
                spins = 0;
            }
        }

        // Finish queued work before exiting.
        if (pool->nextJob >= pool->jobCount && pool->stopping) {
            SDL_UnlockMutex(pool->mutex);
            return 0;
        }

        // Claim another job.
        ThreadJob job = pool->jobs[pool->nextJob++];
        pool->activeJobs++;
        SDL_UnlockMutex(pool->mutex);

        // Do the job.
        job.function(job.data);

        SDL_LockMutex(pool->mutex);
        pool->activeJobs--;
        pool->completedJobs++;

        // Wake the main thread only when the whole batch is done.
        if (pool->completedJobs == pool->jobCount) {
            SDL_BroadcastCondition(pool->doneCondition);
        }
        SDL_UnlockMutex(pool->mutex);
    }
}

int ThreadPool_Init(ThreadPool* pool, float threadProportion)
{
    // Zero init.
    memset(pool, 0, sizeof(*pool));

    // Calculate the number of threads to create: (0.x -> 1) * number of system threads.
    int cores = SDL_GetNumLogicalCPUCores();
    if (threadProportion <= 0.0f) threadProportion = 1.0f;
    if (threadProportion > 1.0f) threadProportion = 1.0f;
    pool->threadCount = (int)(cores * threadProportion + 0.999f);

    // Clamp the thread count to a min/max.
    if (pool->threadCount < 1) pool->threadCount = 1;
    if (pool->threadCount > THREADPOOL_MAX_THREADS) pool->threadCount = THREADPOOL_MAX_THREADS;

    // Create sync primitives.
    pool->mutex = SDL_CreateMutex();
    pool->workerCondition = SDL_CreateCondition();
    pool->doneCondition = SDL_CreateCondition();

    // Failed to create sync primitives.
    if (!pool->mutex || !pool->workerCondition || !pool->doneCondition) {
        if (pool->mutex) SDL_DestroyMutex(pool->mutex);
        if (pool->workerCondition) SDL_DestroyCondition(pool->workerCondition);
        if (pool->doneCondition) SDL_DestroyCondition(pool->doneCondition);
        pool->mutex = NULL;
        pool->workerCondition = NULL;
        pool->doneCondition = NULL;
        pool->threadCount = 0;
        return 0;
    }

    // Pre-allocate the job queue. No realloc in the hot path.
    pool->jobCapacity = THREADPOOL_DEFAULT_CAPACITY;
    pool->jobs = malloc((size_t)pool->jobCapacity * sizeof(ThreadJob));
    if (!pool->jobs) {
        SDL_DestroyCondition(pool->doneCondition);
        SDL_DestroyCondition(pool->workerCondition);
        SDL_DestroyMutex(pool->mutex);
        pool->mutex = NULL;
        pool->workerCondition = NULL;
        pool->doneCondition = NULL;
        pool->threadCount = 0;
        return 0;
    }

    // Create threads.
    int requestedThreads = pool->threadCount;
    pool->threadCount = 0;
    for (int i = 0; i < requestedThreads; i++) {
        SDL_Thread* thread = SDL_CreateThread(ThreadPool_Worker, "ThreadPoolWorker", pool);
        if (!thread) break;
        pool->threads[pool->threadCount++] = thread;
    }

    // Success.
    return 1;
}

int ThreadPool_Submit(ThreadPool* pool, WorkerFunction function, void* data)
{
    if (!function) return 0;
    if (!pool->mutex || pool->threadCount == 0) return 0;

    SDL_LockMutex(pool->mutex);
    if (pool->stopping) {
        SDL_UnlockMutex(pool->mutex);
        return 0;
    }

    // Grow the job queue if needed. In steady state this never fires.
    if (pool->jobCount == pool->jobCapacity) {
        int newCapacity = pool->jobCapacity * 2;
        ThreadJob* newJobs = realloc(pool->jobs, (size_t)newCapacity * sizeof(ThreadJob));
        if (!newJobs) {
            SDL_UnlockMutex(pool->mutex);
            return 0;
        }
        pool->jobs = newJobs;
        pool->jobCapacity = newCapacity;
    }
    pool->jobs[pool->jobCount].function = function;
    pool->jobs[pool->jobCount].data = data;
    pool->jobCount++;

    // Wake exactly one worker per submitted job. If a worker is spinning,
    // this signal is a no-op and costs nothing. If a worker is parked, it
    // wakes one. Either way, we don't broadcast.
    SDL_SignalCondition(pool->workerCondition);
    SDL_UnlockMutex(pool->mutex);

    // Success.
    return 1;
}

void ThreadPool_Wait(ThreadPool* pool)
{
    if (!pool->mutex || pool->threadCount == 0) return;

    SDL_LockMutex(pool->mutex);

    // Spin briefly before parking. If workers finish quickly, we avoid
    // a syscall round-trip on the main thread.
    int spins = 0;
    while (pool->completedJobs < pool->jobCount) {
        if (spins < THREADPOOL_SPIN_COUNT) {
            spins++;
            SDL_UnlockMutex(pool->mutex);
            ThreadPool_SpinPause();
            SDL_LockMutex(pool->mutex);
        } else {
            SDL_WaitCondition(pool->doneCondition, pool->mutex);
            spins = 0;
        }
    }

    // All jobs have finished -> reset queue.
    pool->jobCount = 0;
    pool->nextJob = 0;
    pool->completedJobs = 0;
    SDL_UnlockMutex(pool->mutex);
}

void ThreadPool_Destroy(ThreadPool* pool)
{
    // Ensure all workers exit.
    if (pool->mutex && pool->threadCount > 0) {
        SDL_LockMutex(pool->mutex);
        pool->stopping = 1;
        SDL_BroadcastCondition(pool->workerCondition);
        SDL_UnlockMutex(pool->mutex);
        for (int i = 0; i < pool->threadCount; i++) {
            SDL_WaitThread(pool->threads[i], NULL);
        }
    }

    // Free job queue.
    free(pool->jobs);

    // Destroy sync primitives.
    if (pool->doneCondition) SDL_DestroyCondition(pool->doneCondition);
    if (pool->workerCondition) SDL_DestroyCondition(pool->workerCondition);
    if (pool->mutex) SDL_DestroyMutex(pool->mutex);
    memset(pool, 0, sizeof(*pool));
}
