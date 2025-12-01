#include "core/connection/stats.h"


namespace core
{

namespace connection
{

const std::deque<VoidResult>& ResultsList::getResults() const
{
    return m_results;
}

size_t ResultsList::getFirstResultOrdinal() const
{
    return m_firstResultOrdinal;
}

void ResultsList::addResult(const VoidResult& result)
{
    m_results.push_back(result);

    if (m_results.size() > MAX_RESULT_COUNT)
    {
        m_results.pop_front();
        ++m_firstResultOrdinal;
    }
}

} // namespace connection

} // namespace core
