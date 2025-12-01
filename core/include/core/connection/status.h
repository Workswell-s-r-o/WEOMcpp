#ifndef CORE_CONNECTION_STATUS_H
#define CORE_CONNECTION_STATUS_H

#include "core/connection/stats.h"
#include "core/misc/deadlockdetectionmutex.h"

#include <deque>


namespace core
{

namespace connection
{

class Status
{
public:
    void incrementOperationsCount();
    void incrementFlashBurstWritesCount();
    void addReadError(const VoidResult& result);
    void addWriteError(const VoidResult& result);
    void addResponseError(const VoidResult& result);

    void resetStats();
    Stats getStatsCopy() const;

private:
    Stats m_stats;

    mutable DeadlockDetectionMutex m_mutex;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_STATUS_H
