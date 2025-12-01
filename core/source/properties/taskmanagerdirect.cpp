#include "core/properties/taskmanagerdirect.h"

#include "core/utils.h"
#include "core/logging.h"


namespace core
{

TaskManagerDirect::TaskManagerDirect(const std::shared_ptr<connection::IDeviceInterface>& device) :
    BaseClass(device)
{
}

TaskManagerDirect::~TaskManagerDirect()
{
}

std::shared_ptr<TaskManagerDirect> TaskManagerDirect::createInstance(const std::shared_ptr<connection::IDeviceInterface>& device)
{
    auto instance = std::shared_ptr<TaskManagerDirect>(new TaskManagerDirect(device));
    instance->m_weakThis = instance;

    return instance;
}

void TaskManagerDirect::addTaskSimple(const connection::AddressRanges& addressRanges, TaskType taskType,
                                      const std::function<VoidResult ()>& taskFunction)
{
    if (!m_blockTasks)
    {
        WW_LOG_PROPERTIES_INFO << utils::format("run task {}", taskInfoToString(addressRanges, taskType));

        taskFunction();

        if (taskType == TaskType::WRITE_WILD)
        {
            invalidateProperties(addressRanges);
        }
    }
}

void TaskManagerDirect::addTaskWithProgress(const connection::AddressRanges& addressRanges, TaskType taskType,
                                            const std::function<VoidResult (ProgressController)>& taskFunction)
{
    if (!m_blockTasks)
    {
        WW_LOG_PROPERTIES_INFO << utils::format("run task {}", taskInfoToString(addressRanges, taskType));

        taskFunction(getProgressNotifier()->getOrCreateProgressController());

        if (taskType == TaskType::WRITE_WILD)
        {
            invalidateProperties(addressRanges);
        }
    }
}

void TaskManagerDirect::blockAddingTasksAndWait()
{
    m_blockTasks = true;
}

void TaskManagerDirect::unblockAddingTasks()
{
    m_blockTasks = false;
}

void TaskManagerDirect::blockRunningTasksAndWait(bool cancelRunningTasks)
{
    m_blockTasks = true;
}

void TaskManagerDirect::unblockRunningTasks()
{
    m_blockTasks = false;
}

std::shared_ptr<ITaskManager> TaskManagerDirect::getThis()
{
    return m_weakThis.lock();
}

} // namespace core
