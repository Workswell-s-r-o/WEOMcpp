#ifndef CORE_FUTUREWATCHER_H
#define CORE_FUTUREWATCHER_H

#include "core/misc/result.h"
#include "core/misc/deadlockdetectionmutex.h"


#include <future>
#include <boost/signals2.hpp>

namespace core
{
/**
 * @brief A class for watching a future with a result.
 * @tparam ResultType The type of the result.
 */
template<class ResultType>
class FutureWatcher final
{
public:
    /**
     * @brief Default constructor.
     */
    explicit FutureWatcher() = default;

    /**
     * @brief Default destructor.
     */
    ~FutureWatcher();

    /**
     * @brief Sets the future to watch.
     * @param future The future to watch.
     */
    void setFuture(std::future<ResultType>&& future);

    /**
     * @brief Checks if the future is being waited for.
     * @return True if the future is being waited for, false otherwise.
     */
    bool isWaiting() const;

    /**
     * @brief Gets the result of the future.
     * @return The result of the future.
     */
    const ResultType& getResult();

public:
    /**
     * @brief A signal that is emitted when the future has started.
     */
    boost::signals2::signal<void()> started;

    /**
     * @brief A signal that is emitted when the future has finished.
     */
    boost::signals2::signal<void()> finished;

private:
    struct FutureData
    {
        ResultType result {ResultType::createError("No data!", "FutureResultWatcher - future not assigned")};
        std::shared_ptr<std::future<ResultType>> future;

        DeadlockDetectionMutex mutex;
    };

    std::shared_ptr<FutureData> m_futureData = std::make_shared<FutureData>();
};


/**
 * @brief A class for watching a future with a value result.
 * @tparam TypeName The type of the value.
 */
template<class TypeName>
using FutureResultWatcher = FutureWatcher<std::conditional_t<std::is_void_v<TypeName>, VoidResult, ValueResult<TypeName>>>;

// Impl

template<class ResultType>
FutureWatcher<ResultType>::~FutureWatcher()
{
    const std::scoped_lock lock(m_futureData->mutex);

    m_futureData->future = nullptr;
}

template<class ResultType>
void FutureWatcher<ResultType>::setFuture(std::future<ResultType>&& future)
{
    std::thread waitingThread;

    {
        const std::scoped_lock lock(m_futureData->mutex);

        if (m_futureData->future != nullptr)
        {
            assert(false && "Old future not finished!");
            return;
        }

        m_futureData->result = ResultType::createError("Data not finished!", "FutureResultWatcher - waiting for future");
        m_futureData->future = std::make_shared<std::future<ResultType>>(std::move(future));

        waitingThread = std::thread([futureData = m_futureData, this]()
        {
            futureData->future->wait();

            {
                const std::scoped_lock lock(futureData->mutex);

                if (futureData->future == nullptr)
                {
                    return; // FutureWatcher object deleted!
                }

                try
                {
                    futureData->result = futureData->future->get();
                }
                catch (...)
                {
                    futureData->result = ResultType::createError("Data source lost!", "FutureResultWatcher - broken promise");
                }

                futureData->future = nullptr;

                finished();
            }
        });
    }

    started();

    waitingThread.detach();
}

template<class ResultType>
bool FutureWatcher<ResultType>::isWaiting() const
{
    const std::scoped_lock lock(m_futureData->mutex);

    return m_futureData->future != nullptr;
}

template<class ResultType>
const ResultType& FutureWatcher<ResultType>::getResult()
{
    const std::scoped_lock lock(m_futureData->mutex);

    return m_futureData->result;
}
} // namespace core

#endif // CORE_FUTUREWATCHER_H
