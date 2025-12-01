#include "core/properties/itaskmanager.h"

#include "core/connection/addressrange.h"

#include <ranges>

namespace core
{

class ITaskManager::StopAndBlockTasksData
{
public:
    explicit StopAndBlockTasksData(const std::shared_ptr<ITaskManager>& taskManager);
    ~StopAndBlockTasksData();

private:
    std::shared_ptr<ITaskManager> m_taskManager;
};


class ITaskManager::PauseTasksData
{
public:
    explicit PauseTasksData(const std::shared_ptr<ITaskManager>& taskManager,
                            bool cancelRunningTasks);
    ~PauseTasksData();

private:
    std::shared_ptr<ITaskManager> m_taskManager;
};


ITaskManager::ITaskManager(const std::shared_ptr<connection::IDeviceInterface>& device) :
    m_device(device),
    m_progressNotifier(ProgressNotifier::createProgressNotifier())
{
}

ITaskManager::~ITaskManager()
{
}

ITaskManager::StopAndBlockTasks ITaskManager::getOrCreateStopAndBlockTasksObject()
{
    auto stopAndBlockTasksData = m_stopAndBlockTasksData.lock();
    if (stopAndBlockTasksData == nullptr)
    {
        stopAndBlockTasksData.reset(new StopAndBlockTasksData(getThis()));
        m_stopAndBlockTasksData = stopAndBlockTasksData;
    }

    return StopAndBlockTasks(stopAndBlockTasksData);
}

ITaskManager::PauseTasks ITaskManager::getOrCreatePauseTasksObject(bool cancelRunningTasks)
{
    auto pauseTasksData = m_pauseTasksData.lock();
    if (pauseTasksData == nullptr)
    {
        pauseTasksData.reset(new PauseTasksData(getThis(), cancelRunningTasks));
        m_pauseTasksData = pauseTasksData;
    }

    return PauseTasks(pauseTasksData);
}

const std::shared_ptr<ProgressNotifier>& ITaskManager::getProgressNotifier() const
{
    return m_progressNotifier;
}

void ITaskManager::setProgressNotifier(const std::shared_ptr<ProgressNotifier>& progressNotifier)
{
    m_progressNotifier = progressNotifier;
}

const connection::IDeviceInterface* ITaskManager::getDevice() const
{
    return m_device.get();
}

connection::IDeviceInterface* ITaskManager::getDevice()
{
    return m_device.get();
}

std::string ITaskManager::taskInfoToString(const connection::AddressRanges& addressRanges, TaskType taskType)
{
    std::vector<std::string> list;

    switch (taskType)
    {
        case TaskType::READ_PROPERTY:
            list.push_back("READ_PROPERTY");
            break;

        case TaskType::WRITE_PROPERTY:
            list.push_back("WRITE_PROPERTY");
            break;

        case TaskType::READ_WILD:
            list.push_back("READ_WILD");
            break;

        case TaskType::WRITE_WILD:
            list.push_back("WRITE_WILD");
            break;

        default:
            assert(false);
    }

    for (const auto& addressRange : addressRanges.getRanges())
    {
        list.push_back(addressRange.toHexString());
    }

    std::string joined;
    for(const auto& item : list)
    {
        joined += item + " ";
    }

    return joined;
}

ITaskManager::StopAndBlockTasksData::StopAndBlockTasksData(const std::shared_ptr<ITaskManager>& taskManager) :
    m_taskManager(taskManager)
{
    m_taskManager->blockAddingTasksAndWait();
}

ITaskManager::StopAndBlockTasksData::~StopAndBlockTasksData()
{
    m_taskManager->unblockAddingTasks();
}

ITaskManager::StopAndBlockTasks::StopAndBlockTasks(const std::shared_ptr<StopAndBlockTasksData>& stopAndBlockTasksData) :
    m_stopAndBlockTasksData(stopAndBlockTasksData)
{
}

ITaskManager::PauseTasksData::PauseTasksData(const std::shared_ptr<ITaskManager>& taskManager,
                                             bool cancelRunningTasks) :
    m_taskManager(taskManager)
{
    m_taskManager->blockRunningTasksAndWait(cancelRunningTasks);
}

ITaskManager::PauseTasksData::~PauseTasksData()
{
    m_taskManager->unblockRunningTasks();
}

ITaskManager::PauseTasks::PauseTasks(const std::shared_ptr<PauseTasksData>& pauseTasksData) :
    m_pauseTasksData(pauseTasksData)
{
}

} // namespace core
