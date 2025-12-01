#include "core/misc/lifetimechecker.h"

#include <chrono>
#include <future>

namespace core
{

LifetimeChecker::LifetimeChecker(std::promise<bool>& future, size_t id) :
    m_future(future.get_future()),
    m_id(id)
{
}

bool LifetimeChecker::isFinished() const
{
    if (!m_future.valid())
    {
        return true;
    }
    return m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void LifetimeChecker::waitForFinished()
{
    m_future.wait();
}

size_t LifetimeChecker::getId() const
{
    return m_id;
}

} // namespace core
