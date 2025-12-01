#ifndef CORE_LIFETIMECHECKER_H
#define CORE_LIFETIMECHECKER_H

#include <future>

namespace core
{

class LifetimeChecker
{
public:
    LifetimeChecker() = default;
    // LifetimeChecker(const LifetimeChecker& other) = default;
    // LifetimeChecker& operator=(const LifetimeChecker& other) = default;
    explicit LifetimeChecker(std::promise<bool>& future, size_t id);

    bool isFinished() const;

    void waitForFinished();

    size_t getId() const;

private:
    std::future<bool> m_future;
    size_t m_id {0};
};

} // namespace core

#endif // CORE_LIFETIMECHECKER_H
