#ifndef CORE_CONNECTION_STATS_H
#define CORE_CONNECTION_STATS_H

#include "core/misc/result.h"

#include <deque>


namespace core
{

namespace connection
{

class ResultsList
{
public:

    const std::deque<VoidResult>& getResults() const;
    size_t getFirstResultOrdinal() const;

    void addResult(const VoidResult& result);

    static constexpr size_t MAX_RESULT_COUNT = 200;

private:
    std::deque<VoidResult> m_results;
    size_t m_firstResultOrdinal {0};
};


struct Stats
{
    size_t operationsCount {0};
    size_t flashBurstWritesCount {0};

    ResultsList readErrors;
    ResultsList writeErrors;
    ResultsList responseErrors;
};

} // namespace connection

} // namespace core

#endif // CORE_CONNECTION_STATS_H
