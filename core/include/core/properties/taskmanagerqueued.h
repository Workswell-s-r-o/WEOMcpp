#ifndef CORE_TASKMANAGERQUEUED_H
#define CORE_TASKMANAGERQUEUED_H

#include "core/properties/itaskmanager.h"
#include "core/connection/addressrange.h"
#include "core/misc/progresscontroller.h"
#include "core/misc/deadlockdetectionmutex.h"

#include <set>


namespace core
{

class TaskManagerQueued : public ITaskManager
{
    using BaseClass = ITaskManager;
    explicit TaskManagerQueued(const std::shared_ptr<connection::IDeviceInterface>& device, std::size_t maximumNumberOfThreads);

public:
    struct TaskCount
    {
        size_t runningTaskCount = 0;
        size_t waitingTaskCount = 0;
    };

    virtual ~TaskManagerQueued() override;

    static std::shared_ptr<TaskManagerQueued> createInstance(const std::shared_ptr<connection::IDeviceInterface>& device, std::size_t maximumNumberOfThreads = 8);

    virtual void addTaskSimple(const connection::AddressRanges& addressRanges, TaskType taskType,
                               const std::function<VoidResult ()>& taskFunction) override;
    virtual void addTaskWithProgress(const connection::AddressRanges& addressRanges, TaskType taskType,
                                     const std::function<VoidResult (ProgressController)>& taskFunction) override;

    TaskCount getTaskCount();

protected:
    virtual void blockAddingTasksAndWait() override;
    virtual void unblockAddingTasks() override;
    virtual void blockRunningTasksAndWait(bool cancelRunningTasks) override;
    virtual void unblockRunningTasks() override;
    virtual std::shared_ptr<ITaskManager> getThis() override;

private:
    struct TaskInfo
    {
        connection::AddressRanges addressRanges;
        TaskType taskType {TaskType::READ_PROPERTY};
        bool isTaskWithProgress {false};

        bool isWriteTask() const;
        bool isPropertyTask() const;

        std::string toString() const;

        bool operator<(const TaskInfo& other) const;
    };

    struct Task
    {
        TaskInfo info;
        std::function<VoidResult ()> taskFunction;
    };

    void finishTasks(bool cancelProgress);

    void addTask(const TaskInfo& taskInfo, const std::function<VoidResult ()>& task);
    bool hasPropertyTaskWaitingOrRunning(connection::AddressRanges addressRanges) const;
    void onTaskFinished(const TaskInfo& taskInfo);
    void tryRunTasks();

    static bool canRunTask(const TaskInfo& taskInfo, connection::AddressRanges runningAddressRanges, bool isRunningTaskWithProgress);

    bool m_blockAddingTasks {false};
    bool m_blockRunningTasks {false};

    std::set<TaskInfo> m_tasksInProgress;
    std::vector<Task> m_tasksWaitingQueue;

    std::weak_ptr<TaskManagerQueued> m_weakThis;

    DeadlockDetectionMutex m_mutex;

    std::size_t m_maximumNumberOfThreads;
};

} // namespace core

#endif // CORE_TASKMANAGERQUEUED_H
