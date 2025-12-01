#ifndef CORE_ITASKMANAGER_H
#define CORE_ITASKMANAGER_H

#include "core/misc/progresscontroller.h"
#include "core/misc/result.h"

#include <boost/signals2.hpp>
#include <memory>
#include <functional>


namespace core
{

namespace connection
{
class IDeviceInterface;
class AddressRanges;
}

class ITaskManager
{
protected:
    explicit ITaskManager(const std::shared_ptr<connection::IDeviceInterface>& device);

public:
    virtual ~ITaskManager();

    enum class TaskType
    {
        READ_PROPERTY,
        WRITE_PROPERTY,
        READ_WILD,
        WRITE_WILD,
    };

    virtual void addTaskSimple(const connection::AddressRanges& addressRanges, TaskType taskType,
                               const std::function<VoidResult ()>& taskFunction) = 0;

    virtual void addTaskWithProgress(const connection::AddressRanges& addressRanges, TaskType taskType,
                                     const std::function<VoidResult (ProgressController)>& taskFunction) = 0;

    class StopAndBlockTasks;
    [[nodiscard]] StopAndBlockTasks getOrCreateStopAndBlockTasksObject();

    class PauseTasks;
    [[nodiscard]] PauseTasks getOrCreatePauseTasksObject(bool cancelRunningTasks);

    const std::shared_ptr<ProgressNotifier>& getProgressNotifier() const;
    void setProgressNotifier(const std::shared_ptr<ProgressNotifier>& progressNotifier);

    const connection::IDeviceInterface* getDevice() const;
    connection::IDeviceInterface* getDevice();

    [[nodiscard]] static std::string taskInfoToString(const connection::AddressRanges& addressRanges, TaskType taskType);

    boost::signals2::signal<void(const connection::AddressRanges&)> invalidateProperties;

protected:
    virtual void blockAddingTasksAndWait() = 0;
    virtual void unblockAddingTasks() = 0;

    virtual void blockRunningTasksAndWait(bool cancelRunningTasks) = 0;
    virtual void unblockRunningTasks() = 0;

    virtual std::shared_ptr<ITaskManager> getThis() = 0;

private:
    class StopAndBlockTasksData;
    class PauseTasksData;

    std::shared_ptr<connection::IDeviceInterface> m_device;

    std::weak_ptr<StopAndBlockTasksData> m_stopAndBlockTasksData;
    std::weak_ptr<PauseTasksData> m_pauseTasksData;

    std::shared_ptr<ProgressNotifier> m_progressNotifier;
};


class ITaskManager::StopAndBlockTasks
{
private:
    explicit StopAndBlockTasks(const std::shared_ptr<StopAndBlockTasksData>& stopAndBlockTasksData);
    friend class ITaskManager;

    std::shared_ptr<StopAndBlockTasksData> m_stopAndBlockTasksData;
};


class ITaskManager::PauseTasks
{
private:
    explicit PauseTasks(const std::shared_ptr<PauseTasksData>& pauseTasksData);
    friend class ITaskManager;

    std::shared_ptr<PauseTasksData> m_pauseTasksData;
};

} // namespace core

#endif // CORE_ITASKMANAGER_H
