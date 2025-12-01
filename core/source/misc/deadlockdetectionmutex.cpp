#include "core/misc/deadlockdetectionmutex.h"
#include "core/logging.h"

#include <cassert>


namespace core
{

void DeadlockDetectionMutex::lock()
{
    assert(m_holder != std::this_thread::get_id() && "Deadlock is comming!");

    m_mutex.lock();

    m_holder = std::this_thread::get_id();
}

void DeadlockDetectionMutex::unlock()
{
    m_holder = std::thread::id();

    m_mutex.unlock();
}

bool DeadlockDetectionMutex::try_lock()
{
    assert(m_holder != std::this_thread::get_id() && "Deadlock is comming!");
    const bool result = m_mutex.try_lock();
    if (result)
    {
        m_holder = std::this_thread::get_id();
    }
    return result;
}

bool DeadlockDetectionMutex::try_lock_for(const std::chrono::steady_clock::duration& timeout_duration)
{
    assert(m_holder != std::this_thread::get_id() && "Deadlock is comming!");

    const bool result = m_mutex.try_lock_for(timeout_duration);
    if (result)
    {
        m_holder = std::this_thread::get_id();
    }

    return result;
}

} // namespace core
