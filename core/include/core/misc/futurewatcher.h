#ifndef CORE_FUTUREWATCHER_H
#define CORE_FUTUREWATCHER_H

#include "core/misc/result.h"
#include "core/misc/deadlockdetectionmutex.h"


#include <future>
#include <boost/signals2.hpp>

namespace core
{

/**
 * @brief An interface for watching a future.
 */
class IFutureWatcher
{
public:
    /**
     * @brief Default constructor.
     */
    explicit IFutureWatcher(){};

    /**
     * @brief A signal that is emitted when the future has started.
     */
    boost::signals2::signal<void()> started;
    /**
     * @brief A signal that is emitted when the future has finished.
     */
    boost::signals2::signal<void()> finished;
};


/**
 * @brief A class for watching a future with a result.
 * @tparam ResultType The type of the result.
 */
template<class ResultType>
class FutureResultWatcherImpl : public IFutureWatcher
{
    using BaseClass = IFutureWatcher;

protected:
    /**
     * @brief Default constructor.
     */
    explicit FutureResultWatcherImpl();

public:
    /**
     * @brief Default destructor.
     */
    virtual ~FutureResultWatcherImpl();

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

private:
    struct FutureData
    {
        ResultType result {ResultType::createError("No data!", "FutureResultWatcher - future not assigned")};
        std::shared_ptr<std::future<ResultType>> future;

        DeadlockDetectionMutex mutex;
    };

    std::shared_ptr<FutureData> m_futureData;
};


/**
 * @brief A class for watching a future with a value result.
 * @tparam TypeName The type of the value.
 */
template<class TypeName>
class FutureResultWatcher : public FutureResultWatcherImpl<ValueResult<TypeName>>
{
    using BaseClass = FutureResultWatcherImpl<ValueResult<TypeName>>;

public:
    /**
     * @brief Default constructor.
     */
    explicit FutureResultWatcher();
};


/**
 * @brief A class for watching a future with a void result.
 */
template<>
class FutureResultWatcher<void> : public FutureResultWatcherImpl<VoidResult>
{
    using BaseClass = FutureResultWatcherImpl<VoidResult>;

public:
    /**
     * @brief Default constructor.
     */
    explicit FutureResultWatcher();
};

// Impl

template<class ResultType>
FutureResultWatcherImpl<ResultType>::FutureResultWatcherImpl() :
    m_futureData(std::make_shared<FutureData>())
{
}

template<class ResultType>
FutureResultWatcherImpl<ResultType>::~FutureResultWatcherImpl()
{
    const std::scoped_lock lock(m_futureData->mutex);

    m_futureData->future = nullptr;
}

template<class ResultType>
void FutureResultWatcherImpl<ResultType>::setFuture(std::future<ResultType>&& future)
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
bool FutureResultWatcherImpl<ResultType>::isWaiting() const
{
    const std::scoped_lock lock(m_futureData->mutex);

    return m_futureData->future != nullptr;
}

template<class ResultType>
const ResultType& FutureResultWatcherImpl<ResultType>::getResult()
{
    const std::scoped_lock lock(m_futureData->mutex);

    return m_futureData->result;
}

template<class TypeName>
FutureResultWatcher<TypeName>::FutureResultWatcher()
{
}

inline FutureResultWatcher<void>::FutureResultWatcher()
{
}

} // namespace core

#endif // CORE_FUTUREWATCHER_H
