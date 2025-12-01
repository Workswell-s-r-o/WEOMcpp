#ifndef CORE_DEADLOCKDETECTIONMUTEX_H
#define CORE_DEADLOCKDETECTIONMUTEX_H

#include <mutex>
#include <atomic>
#include <thread>

namespace core
{

class DeadlockDetectionMutex
{
public:
    void lock();
    void unlock();
    bool try_lock();
    bool try_lock_for(const std::chrono::steady_clock::duration& timeout_duration);

private:
    std::timed_mutex m_mutex;

    std::atomic<std::thread::id> m_holder;
};

} // namespace core

#endif // CORE_DEADLOCKDETECTIONMUTEX_H
