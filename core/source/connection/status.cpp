#include "core/connection/status.h"


namespace core
{

namespace connection
{

void Status::incrementOperationsCount()
{
    std::scoped_lock lock(m_mutex);

    ++m_stats.operationsCount;
}

void Status::incrementFlashBurstWritesCount()
{
    std::scoped_lock lock(m_mutex);

    ++m_stats.flashBurstWritesCount;
}

void Status::addReadError(const VoidResult& result)
{
    std::scoped_lock lock(m_mutex);

    m_stats.readErrors.addResult(result);
}

void Status::addWriteError(const VoidResult& result)
{
    std::scoped_lock lock(m_mutex);

    m_stats.writeErrors.addResult(result);
}

void Status::addResponseError(const VoidResult& result)
{
    std::scoped_lock lock(m_mutex);

    m_stats.responseErrors.addResult(result);
}

void Status::resetStats()
{
    std::scoped_lock lock(m_mutex);

    m_stats = Stats{};
}

Stats Status::getStatsCopy() const
{
    std::scoped_lock lock(m_mutex);

    return m_stats;
}

} // namespace connection

} // namespace core
