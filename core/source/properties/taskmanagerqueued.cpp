#include "core/properties/taskmanagerqueued.h"

#include "core/utils.h"
#include "core/misc/verify.h"
#include "core/logging.h"

#include <boost/scope_exit.hpp>


namespace core
{

TaskManagerQueued::TaskManagerQueued(const std::shared_ptr<connection::IDeviceInterface>& device, std::size_t maximumNumberOfThreads) :
    BaseClass(device),
    m_maximumNumberOfThreads(maximumNumberOfThreads)
{
}

TaskManagerQueued::~TaskManagerQueued()
{
    {
        const std::scoped_lock lock(m_mutex);

        m_blockAddingTasks = true;

        m_tasksWaitingQueue.clear();
    }

    finishTasks(true);
}

std::shared_ptr<TaskManagerQueued> TaskManagerQueued::createInstance(const std::shared_ptr<core::connection::IDeviceInterface>& device, std::size_t maximumNumberOfThreads)
{
    auto instance = std::shared_ptr<TaskManagerQueued>(new TaskManagerQueued(device, maximumNumberOfThreads));
    instance->m_weakThis = instance;

    return instance;
}

void TaskManagerQueued::addTaskSimple(const connection::AddressRanges& addressRanges, TaskType taskType,
                                      const std::function<VoidResult ()>& taskFunction)
{
    const std::scoped_lock lock(m_mutex);

    const TaskInfo taskInfo {addressRanges, taskType, false};

    const auto task = [this, taskFunction, taskInfo]()
    {
        BOOST_SCOPE_EXIT(this_, taskInfo)
        {
            this_->onTaskFinished(taskInfo);
        } BOOST_SCOPE_EXIT_END

        return taskFunction();
    };

    addTask(taskInfo, task);
}

void TaskManagerQueued::addTaskWithProgress(const connection::AddressRanges& addressRanges, TaskType taskType,
                                            const std::function<VoidResult (ProgressController)>& taskFunction)
{
    const std::scoped_lock lock(m_mutex);

    const TaskInfo taskInfo {addressRanges, taskType, true};

    const auto task = [this, taskFunction, taskInfo, progressController = getProgressNotifier()->getOrCreateProgressController()]()
    {
        BOOST_SCOPE_EXIT(this_, taskInfo)
        {
            this_->onTaskFinished(taskInfo);
        } BOOST_SCOPE_EXIT_END

        return taskFunction(progressController);
    };

    addTask(taskInfo, task);
}

TaskManagerQueued::TaskCount TaskManagerQueued::getTaskCount()
{
    const std::scoped_lock lock(m_mutex);

    TaskCount result;
    result.runningTaskCount = m_tasksInProgress.size();
    result.waitingTaskCount = m_tasksWaitingQueue.size();

    return result;
}

void TaskManagerQueued::blockAddingTasksAndWait()
{
    {
        const std::scoped_lock lock(m_mutex);

        m_blockAddingTasks = true;

        if (m_blockRunningTasks)
        {
            m_tasksWaitingQueue.clear();
        }
    }

    finishTasks(true);
}

void TaskManagerQueued::unblockAddingTasks()
{
    const std::scoped_lock lock(m_mutex);

    m_blockAddingTasks = false;
}

void TaskManagerQueued::blockRunningTasksAndWait(bool cancelRunningTasks)
{
    {
        const std::scoped_lock lock(m_mutex);

        m_blockRunningTasks = true;
    }

    finishTasks(cancelRunningTasks);
}

void TaskManagerQueued::unblockRunningTasks()
{
    const std::scoped_lock lock(m_mutex);

    m_blockRunningTasks = false;

    tryRunTasks();
}

std::shared_ptr<ITaskManager> TaskManagerQueued::getThis()
{
    return m_weakThis.lock();
}

void TaskManagerQueued::finishTasks(bool cancelProgress)
{
    while (true)
    {
        {
            const std::scoped_lock lock(m_mutex);

            if (m_tasksInProgress.empty())
            {
                break;
            }

            if (cancelProgress)
            {
                getProgressNotifier()->cancelProgress();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void TaskManagerQueued::addTask(const TaskInfo& taskInfo, const std::function<VoidResult ()>& task)
{
    assert(!taskInfo.addressRanges.getRanges().empty() && "may not work properly with empty AddressRanges");

    if (m_blockAddingTasks || (taskInfo.taskType == TaskType::READ_PROPERTY && hasPropertyTaskWaitingOrRunning(taskInfo.addressRanges)))
    {
        WW_LOG_PROPERTIES_DEBUG << utils::format("task ignored {}", taskInfo.toString());
        return;
    }

    m_tasksWaitingQueue.push_back(Task{taskInfo, task});
    WW_LOG_PROPERTIES_INFO << utils::format("task added {}", taskInfo.toString());

    tryRunTasks();
}

bool TaskManagerQueued::hasPropertyTaskWaitingOrRunning(connection::AddressRanges addressRanges) const
{
    for (const auto& info : m_tasksInProgress)
    {
        if (info.isPropertyTask() && info.addressRanges.contains(addressRanges))
        {
            return true;
        }
    }

    for (const auto& task : m_tasksWaitingQueue)
    {
        if (task.info.isPropertyTask() && task.info.addressRanges.contains(addressRanges))
        {
            return true;
        }
    }

    return false;
}

void TaskManagerQueued::onTaskFinished(const TaskInfo& taskInfo)
{
    // nesmi se delat az po odstraneni z m_tasksInProgress - reakce na invalidateProperties by se mohla dostat do ConnectionExclusiveTransaction
    if (taskInfo.taskType == TaskType::WRITE_WILD)
    {
        invalidateProperties(taskInfo.addressRanges);
    }

    {
        const std::scoped_lock lock(m_mutex);

        VERIFY(m_tasksInProgress.erase(taskInfo) > 0);
        WW_LOG_PROPERTIES_INFO << utils::format("task finished {}", taskInfo.toString());

        tryRunTasks();
    }
}

void TaskManagerQueued::tryRunTasks()
{
    if (m_blockRunningTasks)
    {
        return;
    }

    bool isRunningTaskWithProgress = false;
    connection::AddressRanges runningAddressRanges;
    for (const auto& taskInfo : m_tasksInProgress)
    {
        runningAddressRanges = connection::AddressRanges(runningAddressRanges, taskInfo.addressRanges.getRanges());
        if (taskInfo.isTaskWithProgress)
        {
            isRunningTaskWithProgress = true;
        }
    }

    for (size_t taskIndex = 0 ; taskIndex < m_tasksWaitingQueue.size(); )
    {
        if (m_tasksInProgress.size() >= m_maximumNumberOfThreads)
        {
            return;
        }

        const auto& task = m_tasksWaitingQueue.at(taskIndex);
        if (!canRunTask(task.info, runningAddressRanges, isRunningTaskWithProgress))
        {
            ++taskIndex;

            continue;
        }

        runningAddressRanges = connection::AddressRanges(runningAddressRanges, task.info.addressRanges.getRanges());
        if (task.info.isTaskWithProgress)
        {
            isRunningTaskWithProgress = true;
        }

        std::thread thread(task.taskFunction);
        VERIFY(m_tasksInProgress.insert(task.info).second);

        WW_LOG_PROPERTIES_INFO << utils::format("task started {}", task.info.toString());
        m_tasksWaitingQueue.erase(m_tasksWaitingQueue.begin() + taskIndex);

        thread.detach();
    }
}

bool TaskManagerQueued::canRunTask(const TaskInfo& taskInfo, connection::AddressRanges runningAddressRanges, bool isRunningTaskWithProgress)
{
    return (!taskInfo.isTaskWithProgress || !isRunningTaskWithProgress) &&
            !runningAddressRanges.overlaps(taskInfo.addressRanges);
}

bool TaskManagerQueued::TaskInfo::isWriteTask() const
{
    return taskType == TaskType::WRITE_PROPERTY || taskType == TaskType::WRITE_WILD;
}

bool TaskManagerQueued::TaskInfo::isPropertyTask() const
{
    return taskType == TaskType::READ_PROPERTY || taskType == TaskType::WRITE_PROPERTY;
}

std::string TaskManagerQueued::TaskInfo::toString() const
{
    return taskInfoToString(addressRanges, taskType);
}

bool TaskManagerQueued::TaskInfo::operator<(const TaskInfo& other) const
{
    return taskType < other.taskType ||
            (taskType == other.taskType && isTaskWithProgress < other.isTaskWithProgress) ||
            (taskType == other.taskType && isTaskWithProgress == other.isTaskWithProgress && addressRanges < other.addressRanges);
}

} // namespace core
