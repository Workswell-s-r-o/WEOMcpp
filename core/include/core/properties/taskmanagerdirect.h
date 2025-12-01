#ifndef CORE_TASKMANAGERDIRECT_H
#define CORE_TASKMANAGERDIRECT_H

#include "core/properties/itaskmanager.h"


namespace core
{

class TaskManagerDirect : public ITaskManager
{
    using BaseClass = ITaskManager;

    explicit TaskManagerDirect(const std::shared_ptr<connection::IDeviceInterface>& device);

public:
    ~TaskManagerDirect();

    static std::shared_ptr<TaskManagerDirect> createInstance(const std::shared_ptr<connection::IDeviceInterface>& device);

    virtual void addTaskSimple(const connection::AddressRanges& addressRanges, TaskType taskType,
                               const std::function<VoidResult ()>& taskFunction) override;
    virtual void addTaskWithProgress(const connection::AddressRanges& addressRanges, TaskType taskType,
                                     const std::function<VoidResult (ProgressController)>& taskFunction) override;

protected:
    virtual void blockAddingTasksAndWait() override;
    virtual void unblockAddingTasks() override;
    virtual void blockRunningTasksAndWait(bool cancelRunningTasks) override;
    virtual void unblockRunningTasks() override;
    virtual std::shared_ptr<ITaskManager> getThis() override;

private:
    bool m_blockTasks {false};

    std::weak_ptr<TaskManagerDirect> m_weakThis;
};

} // namespace core

#endif // CORE_TASKMANAGERDIRECT_H
