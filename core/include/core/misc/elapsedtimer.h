#ifndef CORE_ELAPSEDTIMER_H
#define CORE_ELAPSEDTIMER_H

#include <chrono>


namespace core
{

class ElapsedTimer
{
public:
    using Clock = std::chrono::steady_clock;

    explicit ElapsedTimer();
    explicit ElapsedTimer(const Clock::duration& timeout);

    bool timedOut() const;

    Clock::duration getElapsedTime() const;
    Clock::rep getElapsedMilliseconds() const;

    Clock::duration getRestOfTimeout() const;

private:
    Clock::time_point m_start;
    Clock::time_point m_end;
};

} // namespace core

#endif // CORE_ELAPSEDTIMER_H
