#ifndef CORE_PROGRESSCONTROLLER_H
#define CORE_PROGRESSCONTROLLER_H

#include "core/misc/deadlockdetectionmutex.h"

#include <boost/signals2.hpp>
#include <memory>
#include <optional>

namespace core
{

class ProgressTaskImpl;
class ProgressSequenceImpl;


class CancelToken
{
public:
    explicit CancelToken();
    explicit CancelToken(const std::shared_ptr<ProgressTaskImpl>& taskImpl);

    bool isCancelled() const;

private:
    std::shared_ptr<ProgressTaskImpl> m_taskImpl;
};


class ProgressTask
{
public:
    explicit ProgressTask();
    explicit ProgressTask(const std::shared_ptr<ProgressTaskImpl>& taskImpl);

    bool isCancelled() const;
    void sendErrorMessage(const std::string& errorMessage);
    void sendProgressMessage(const std::string& progressMessage);

    [[nodiscard]] bool advanceByIsCancelled(int stepsDoneIncrement);
    void advanceByIgnoreCancel(int stepsDoneIncrement);

    CancelToken getCancelToken() const;

private:
    std::shared_ptr<ProgressTaskImpl> m_taskImpl;
};


class ProgressController
{
public:
    explicit ProgressController();
    explicit ProgressController(const std::shared_ptr<ProgressSequenceImpl>& progressImpl);

    bool isCancelled() const;
    void sendErrorMessage(const std::string& errorMessage);
    void sendResultMessage(const std::string& resultMessage);

    ProgressTask createTaskUnbound(const std::string& taskName, bool isCancellable);
    ProgressTask createTaskBound(const std::string& taskName, int taskTotalStepsCount, bool isCancellable);

private:
    std::shared_ptr<ProgressSequenceImpl> m_progressImpl;
};


class ProgressNotifier
{
    explicit ProgressNotifier();

public:
    ProgressController getOrCreateProgressController();
    std::optional<ProgressController> getProgressController();

    bool isInProgress() const;
    void cancelProgress();

    static std::shared_ptr<ProgressNotifier> createProgressNotifier();

    boost::signals2::signal<void()> sequenceStarted;
    boost::signals2::signal<void()> sequenceFinished;

    boost::signals2::signal<void(const std::string&)> errorMessageSent;
    boost::signals2::signal<void(const std::string&)> resultMessageSent;
    boost::signals2::signal<void(const std::string&)> progressMessageSent;

    boost::signals2::signal<void(const std::string&, int, bool)> taskStartedBound;
    boost::signals2::signal<void(const std::string&, bool)> taskStartedUnbound;
    boost::signals2::signal<void(int, const std::string&, const int&, bool)> taskAdvancedTo;
    boost::signals2::signal<void()> taskFinished;

private:
    std::weak_ptr<ProgressNotifier> m_weakThis;
    std::weak_ptr<ProgressSequenceImpl> m_currentProgressImpl;

    mutable DeadlockDetectionMutex m_mutex;
};

} // namespace core

#endif // CORE_PROGRESSCONTROLLER_H
