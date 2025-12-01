#include "core/misc/progresscontroller.h"


namespace core
{

class ProgressTaskImpl
{
public:
    explicit ProgressTaskImpl(size_t taskId,
                              const std::shared_ptr<ProgressSequenceImpl>& progressImpl,
                              const std::string& taskName,
                              const std::optional<int>& totalStepsCount,
                              bool isCancellable);
    ~ProgressTaskImpl();

    bool isCancelled() const;
    void sendErrorMessage(const std::string& errorMessage);
    void sendProgressMessage(const std::string& progressMessage);

    void advanceBy(int stepsDoneIncrement);

private:
    DeadlockDetectionMutex m_mutex;

    const std::string m_name;
    const std::optional<int> m_totalStepsCount;
    const bool m_isCancellable;

    int m_stepsDoneSoFar {0};

    std::shared_ptr<ProgressSequenceImpl> m_progressImpl;
    size_t m_taskId {0};
};


class ProgressSequenceImpl
{
public:
    explicit ProgressSequenceImpl(const std::weak_ptr<ProgressNotifier>& progressNotifier);
    ~ProgressSequenceImpl();

    bool isCancelled() const;
    void setCancelled();

    std::shared_ptr<ProgressTaskImpl> createTaskImpl(const std::string& taskName, const std::optional<int>& taskTotalStepsCount, bool isCancellable,
                                                     const std::shared_ptr<ProgressSequenceImpl>& progressImpl);

    void sendErrorMessage(size_t taskId, const std::string& errorMessage);
    void sendResultMessage(size_t taskId, const std::string& resultMessage);
    void sendProgressMessage(size_t taskId, const std::string& progressMessage);

    void taskAdvancedTo(size_t taskId, int stepsDoneSoFar, const std::string& taskName, const std::optional<int>& totalStepsCount, bool isCancellable);
    void taskFinished(size_t taskId);

private:
    friend class ProgressController;
    void sendErrorMessage(const std::string& errorMessage);
    void sendResultMessage(const std::string& resultMessage);

    mutable DeadlockDetectionMutex m_mutex;

    std::weak_ptr<ProgressNotifier> m_progressNotifier;
    std::weak_ptr<ProgressTaskImpl> m_currentTaskImpl;
    std::atomic<size_t> m_currentTaskId {0};

    bool m_isCancelled {false};
};


ProgressTaskImpl::ProgressTaskImpl(size_t taskId,
                                   const std::shared_ptr<ProgressSequenceImpl>& progressImpl,
                                   const std::string& taskName,
                                   const std::optional<int>& totalStepsCount,
                                   bool isCancellable) :
    m_name(taskName),
    m_totalStepsCount(totalStepsCount),
    m_isCancellable(isCancellable),
    m_progressImpl(progressImpl),
    m_taskId(taskId)
{
}

ProgressTaskImpl::~ProgressTaskImpl()
{
    m_progressImpl->taskFinished(m_taskId);
}

bool ProgressTaskImpl::isCancelled() const
{
    return m_progressImpl->isCancelled();
}

void ProgressTaskImpl::sendErrorMessage(const std::string& errorMessage)
{
    m_progressImpl->sendErrorMessage(m_taskId, errorMessage);
}

void ProgressTaskImpl::sendProgressMessage(const std::string& progressMessage)
{
    m_progressImpl->sendProgressMessage(m_taskId, progressMessage);
}

void ProgressTaskImpl::advanceBy(int stepsDoneIncrement)
{
    const std::scoped_lock lock(m_mutex);

    m_stepsDoneSoFar += stepsDoneIncrement;

    m_progressImpl->taskAdvancedTo(m_taskId, m_stepsDoneSoFar, m_name, m_totalStepsCount, m_isCancellable);
}


ProgressSequenceImpl::ProgressSequenceImpl(const std::weak_ptr<ProgressNotifier>& progressNotifier) :
    m_progressNotifier(progressNotifier)
{
}

ProgressSequenceImpl::~ProgressSequenceImpl()
{
    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        progressNotifier->sequenceFinished();
    }
}

bool ProgressSequenceImpl::isCancelled() const
{
    const std::scoped_lock lock(m_mutex);

    return m_isCancelled;
}

void ProgressSequenceImpl::setCancelled()
{
    const std::scoped_lock lock(m_mutex);

    m_isCancelled = true;
}

std::shared_ptr<ProgressTaskImpl> ProgressSequenceImpl::createTaskImpl(const std::string& taskName, const std::optional<int>& taskTotalStepsCount, bool isCancellable,
                                                                       const std::shared_ptr<ProgressSequenceImpl>& progressImpl)
{
    std::shared_ptr<ProgressTaskImpl> newTaskImpl;
    bool finishPreviousTask {false};
    {
        const std::scoped_lock lock(m_mutex);

        if (m_currentTaskImpl.lock() != nullptr)
        {
            finishPreviousTask = true;
        }

        newTaskImpl = std::make_shared<ProgressTaskImpl>(++m_currentTaskId, progressImpl, taskName, taskTotalStepsCount, isCancellable);
        m_currentTaskImpl = newTaskImpl;
        m_isCancelled = false;
    }

    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        if (finishPreviousTask)
        {
            progressNotifier->taskFinished();
        }

        if (taskTotalStepsCount.has_value())
        {
            progressNotifier->taskStartedBound(taskName, taskTotalStepsCount.value(), isCancellable);
        }
        else
        {
            progressNotifier->taskStartedUnbound(taskName, isCancellable);
        }
    }

    return newTaskImpl;
}

void ProgressSequenceImpl::sendErrorMessage(size_t taskId, const std::string& errorMessage)
{
    if (m_currentTaskId != taskId)
    {
        assert(false && "Task already finished!");
        return;
    }

    sendErrorMessage(errorMessage);
}

void ProgressSequenceImpl::sendResultMessage(size_t taskId, const std::string& resultMessage)
{
    if (m_currentTaskId != taskId)
    {
        assert(false && "Task already finished!");
        return;
    }

    sendResultMessage(resultMessage);
}

void ProgressSequenceImpl::sendProgressMessage(size_t taskId, const std::string& progressMessage)
{
    if (m_currentTaskId != taskId)
    {
        assert(false && "Task already finished!");
        return;
    }

    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        progressNotifier->progressMessageSent(progressMessage);
    }
}

void ProgressSequenceImpl::taskAdvancedTo(size_t taskId, int stepsDoneSoFar, const std::string& taskName, const std::optional<int>& totalStepsCount, bool isCancellable)
{
    if (m_currentTaskId != taskId)
    {
        assert(false && "Task already finished!");
        return;
    }

    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        progressNotifier->taskAdvancedTo(stepsDoneSoFar, taskName, totalStepsCount.has_value() ? totalStepsCount.value() : 0, isCancellable);
    }
}

void ProgressSequenceImpl::taskFinished(size_t taskId)
{
    if (m_currentTaskId != taskId)
    {
        return;
    }

    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        progressNotifier->taskFinished();
    }
}

void ProgressSequenceImpl::sendErrorMessage(const std::string& errorMessage)
{
    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        progressNotifier->errorMessageSent(errorMessage);
    }
}

void ProgressSequenceImpl::sendResultMessage(const std::string& resultMessage)
{
    if (const auto progressNotifier = m_progressNotifier.lock())
    {
        progressNotifier->resultMessageSent(resultMessage);
    }
}


CancelToken::CancelToken()
{
}

CancelToken::CancelToken(const std::shared_ptr<ProgressTaskImpl>& taskImpl) :
    m_taskImpl(taskImpl)
{
}

bool CancelToken::isCancelled() const
{
    if (m_taskImpl == nullptr)
    {
        return false;
    }

    return m_taskImpl->isCancelled();
}


ProgressTask::ProgressTask()
{
}

ProgressTask::ProgressTask(const std::shared_ptr<ProgressTaskImpl>& taskImpl) :
    m_taskImpl(taskImpl)
{
}

bool ProgressTask::isCancelled() const
{
    if (m_taskImpl == nullptr)
    {
        return false;
    }

    return m_taskImpl->isCancelled();
}

void ProgressTask::sendErrorMessage(const std::string& errorMessage)
{
    if (m_taskImpl == nullptr)
    {
        return;
    }

    m_taskImpl->sendErrorMessage(errorMessage);
}

void ProgressTask::sendProgressMessage(const std::string& progressMessage)
{
    if (m_taskImpl == nullptr)
    {
        return;
    }

    m_taskImpl->sendProgressMessage(progressMessage);
}

bool ProgressTask::advanceByIsCancelled(int stepsDoneIncrement)
{
    advanceByIgnoreCancel(stepsDoneIncrement);

    return isCancelled();
}

void ProgressTask::advanceByIgnoreCancel(int stepsDoneIncrement)
{
    if (m_taskImpl == nullptr)
    {
        return;
    }

    m_taskImpl->advanceBy(stepsDoneIncrement);
}

CancelToken ProgressTask::getCancelToken() const
{
    return CancelToken(m_taskImpl);
}


ProgressController::ProgressController() :
    m_progressImpl(new ProgressSequenceImpl(std::weak_ptr<ProgressNotifier>()))
{
}

ProgressController::ProgressController(const std::shared_ptr<ProgressSequenceImpl>& progressImpl) :
    m_progressImpl(progressImpl)
{
}

bool ProgressController::isCancelled() const
{
    return m_progressImpl->isCancelled();
}

void ProgressController::sendErrorMessage(const std::string& errorMessage)
{
    m_progressImpl->sendErrorMessage(errorMessage);
}

void ProgressController::sendResultMessage(const std::string& resultMessage)
{
    m_progressImpl->sendResultMessage(resultMessage);
}

ProgressTask ProgressController::createTaskUnbound(const std::string& taskName, bool isCancellable)
{
    return ProgressTask(m_progressImpl->createTaskImpl(taskName, std::nullopt, isCancellable, m_progressImpl));
}

ProgressTask ProgressController::createTaskBound(const std::string& taskName, int taskTotalStepsCount, bool isCancellable)
{
    return ProgressTask(m_progressImpl->createTaskImpl(taskName, taskTotalStepsCount, isCancellable, m_progressImpl));
}


ProgressNotifier::ProgressNotifier()
{
}

ProgressController ProgressNotifier::getOrCreateProgressController()
{
    std::shared_ptr<ProgressSequenceImpl> newProgressImpl;
    {
        const std::scoped_lock lock(m_mutex);

        if (auto currentProgressImpl = m_currentProgressImpl.lock())
        {
            return ProgressController(currentProgressImpl);
        }

        newProgressImpl = std::make_shared<ProgressSequenceImpl>(m_weakThis);
        m_currentProgressImpl = newProgressImpl;
    }

    sequenceStarted();

    return ProgressController(newProgressImpl);
}

std::optional<ProgressController> ProgressNotifier::getProgressController()
{
    const std::scoped_lock lock(m_mutex);

    if (auto currentProgressImpl = m_currentProgressImpl.lock())
    {
        return ProgressController(currentProgressImpl);
    }

    return std::nullopt;
}

bool ProgressNotifier::isInProgress() const
{
    const std::scoped_lock lock(m_mutex);

    return !m_currentProgressImpl.expired();
}

void ProgressNotifier::cancelProgress()
{
    const std::scoped_lock lock(m_mutex);

    if (auto currentProgressImpl = m_currentProgressImpl.lock())
    {
        currentProgressImpl->setCancelled();
    }
}

std::shared_ptr<ProgressNotifier> ProgressNotifier::createProgressNotifier()
{
    const auto newProgressNotifier = std::shared_ptr<ProgressNotifier>(new ProgressNotifier());
    newProgressNotifier->m_weakThis = newProgressNotifier;

    return newProgressNotifier;
}

} // namespace core
