#ifndef CORE_PROPERTIES_H
#define CORE_PROPERTIES_H

#include "core/properties/propertyid.h"
#include "core/properties/propertyadapterbase.h"
#include "core/properties/transactionsummary.h"
#include "core/properties/itaskmanager.h"
#include "core/connection/addressrange.h"
#include "core/misc/imainthreadindicator.h"


#include <set>
#include <map>
#include <span>



namespace core
{

namespace connection
{
class IDataLinkInterface;
class IDeviceInterface;
}
class ProgressNotifier;
class ProgressController;
class PropertyDependencyValidator;

class Properties
{

public:
    /**
     * @brief Defines the operational modes for the Properties system, controlling its threading and execution behavior.
     *
     * The choice of mode is critical for application stability and responsiveness, especially in multi-threaded contexts.
     */
    enum class Mode
    {
        /**
         * @brief Synchronous and direct execution mode.
         * All property operations are executed synchronously on the calling thread. This mode is straightforward
         * and recommended for command-line applications or scenarios without a dedicated event loop.
         * It avoids the complexities of asynchronous task management but means that long-running operations
         * will block the calling thread.
         */
        SYNC_DIRECT,

        /**
         * @brief Asynchronous and queued execution mode.
         * Property operations are placed in a queue and processed by a worker thread. This mode is designed for
         * GUI applications to prevent long-running operations from blocking the UI thread.
         * @warning This mode requires a running event loop on the main thread to handle tasks that are
         * dispatched back to it. Using this mode in an application without a proper event loop can
         * lead to deadlocks.
         */
        ASYNC_QUEUED
    };

protected:
    explicit Properties(const std::shared_ptr<connection::IDeviceInterface>& deviceInterface,
                        Mode mode,
                        const std::shared_ptr<IMainThreadIndicator>& indicator);
    Properties(const Properties& ) = delete;
    Properties& operator=(const Properties& ) = delete;

public:
    ~Properties();

    class PropertiesTransaction;
    [[nodiscard]] PropertiesTransaction createPropertiesTransaction();
    [[nodiscard]] std::optional<PropertiesTransaction> tryCreatePropertiesTransaction(const std::chrono::steady_clock::duration& timeout);

    class ConnectionExclusiveTransaction;
    [[nodiscard]] ConnectionExclusiveTransaction createConnectionExclusiveTransaction(bool cancelRunningTasks);

    class TaskResultTransaction;
    [[nodiscard]] TaskResultTransaction getTaskResultTransaction();

    VoidResult tryLoadProperties(const std::set<PropertyId>& properties, const std::chrono::steady_clock::duration& timeout);

    virtual void refreshProperties(const std::set<PropertyId>& properties, std::optional<PropertiesTransaction>& transaction);

    const std::shared_ptr<ProgressNotifier>& getCommunicationProgressNotifier() const;

    const std::optional<DeviceType>& getCurrentDeviceType(const PropertiesTransaction& transaction) const;

    const std::vector<PropertyId>& getPropertiesToTouchAfterConnect(const PropertiesTransaction& transaction);
    void setPropertiesToTouchAfterConnect(const std::vector<PropertyId>& propertiesToTouchAfterConnect, const PropertiesTransaction& transaction);

    class AdapterTaskCreator;

    boost::signals2::signal<void(const core::TransactionSummary&)> transactionFinished;

protected:
    class TransactionData;
    class TaskResultTransactionData;
    class ConnectionStateTransactionData;
    class ConnectionExclusiveTransactionData;

    virtual void onCurrentDeviceTypeChanged();
    virtual void onTransactionFinished(const TransactionSummary& transactionSummary);

    const std::optional<DeviceType>& getDeviceType() const;

    const std::shared_ptr<PropertyValues>& getPropertyValues() const;

    std::set<PropertyId> getMappedPropertiesInConflict(const connection::AddressRange& addressRange, DeviceType deviceType) const;
    void addValueAdapter(const std::shared_ptr<PropertyValueBase>& value, const std::shared_ptr<PropertyAdapterBase>& adapter);
    void removeValueAdapter(PropertyId propertyId);
    void addPropertyDependencyValidator(const std::shared_ptr<PropertyDependencyValidator>& validator);

    [[nodiscard]] std::shared_ptr<ConnectionStateTransactionData> createConnectionStateTransactionData();
    [[nodiscard]] std::shared_ptr<ConnectionStateTransactionData> createConnectionStateTransactionDataFromConnectionExclusiveTransaction(const ConnectionExclusiveTransaction& connectionExclusiveTransaction);

    const std::map<PropertyId, std::shared_ptr<PropertyAdapterBase>>& getPropertyAdapters() const;

    const connection::IDeviceInterface* getDeviceInterface() const;
    connection::IDeviceInterface* getDeviceInterface();

    std::weak_ptr<Properties> m_weakThis;

private:
    class PropertiesTransactionData;

    [[nodiscard]] std::shared_ptr<PropertiesTransactionData> createPropertiesTransactionData();
    [[nodiscard]] std::shared_ptr<PropertiesTransactionData> createPropertiesTransactionDataImpl();

    void mapAdapterAddressRanges(const std::shared_ptr<PropertyAdapterBase>& adapter);
    void unmapAdapterAddressRanges(const std::shared_ptr<PropertyAdapterBase>& adapter);
    void setCurrentDeviceType(const std::optional<DeviceType>& deviceType, const PropertyValues::Transaction& transaction);
    void invalidateProperties(const connection::AddressRanges& addressRanges);

    Mode getMode(const ITaskManager* taskManager) const;
    [[nodiscard]] VoidResult setNonexclusiveMode(Mode mode);
    std::shared_ptr<ITaskManager> createNewTaskManager(Mode mode);

    const ITaskManager* getTaskManager() const;
    ITaskManager* getTaskManager();

    std::shared_ptr<ITaskManager> m_nonexclusiveTaskManager;
    std::shared_ptr<ITaskManager> m_exclusiveTaskManager;

    std::shared_ptr<connection::IDeviceInterface> m_deviceInterface;
    std::shared_ptr<IMainThreadIndicator> m_mainThreadIndicator;

    std::shared_ptr<PropertyValues> m_propertyValues;
    std::map<PropertyId, std::shared_ptr<PropertyAdapterBase>> m_propertyAdapters;

    std::weak_ptr<TransactionData> m_transactionData;
    std::weak_ptr<TaskResultTransactionData> m_taskResultTransactionData;
    std::weak_ptr<ConnectionStateTransactionData> m_connectionStateTransactionData;
    std::weak_ptr<ConnectionExclusiveTransactionData> m_connectionExclusiveTransactionData;

    DeadlockDetectionMutex m_outerTransactionMutex;

    std::optional<DeviceType> m_currentDeviceType;
    std::vector<PropertyId> m_propertiesToTouchAfterConnect;

    std::map<DeviceType, connection::AddressRangeMap<PropertyId>> m_adapterAddressRangeMaps;
};


class Properties::PropertiesTransaction
{
    explicit PropertiesTransaction(const std::shared_ptr<PropertiesTransactionData>& propertiesTransactionData);
    friend class Properties;
public:

    std::set<PropertyId> getAllProperyIds() const;

    // returns true, if adapter exists, and is not DISABLED (regardless of status constraints)
    [[nodiscard]] bool isPropertyActiveForCurrentDevice(PropertyId propertyId) const;

    // returns true, if adapter exists, and is not DISABLED (status constraints applied)
    [[nodiscard]] bool hasProperty(PropertyId propertyId) const;

    // returns true, if property is readable (adapter NUST exist, status constraints applied)
    [[nodiscard]] bool isPropertyReadable(PropertyId propertyId) const;

    // returns true, if property is writable (adapter NUST exist, status constraints applied)
    [[nodiscard]] bool isPropertyWritable(PropertyId propertyId) const;

    [[nodiscard]] const std::type_info& getPropertyTypeInfo(PropertyId propertyId) const;

    // if propertyValue == nullopt or propertyValue.isRecoverableError() => refreshValue()
    void touch(PropertyId propertyId) const;

    // set propertyValue = nullopt
    void resetValue(PropertyId propertyId) const;

    // if is readable, read new value
    void refreshValue(PropertyId propertyId) const;
    std::future<void> refreshValueAsync(PropertyId propertyId) const;

    // if propertyValue != nullopt => refreshValue()
    void invalidateValue(PropertyId propertyId) const;

    [[nodiscard]] bool hasValueResult(PropertyId propertyId) const;
    [[nodiscard]] VoidResult getPropertyValidationResult(PropertyId propertyId) const;
    [[nodiscard]] bool areValuesEqual(PropertyId propertyId1, PropertyId propertyId2) const;
    [[nodiscard]] VoidResult setValueAccording(PropertyId targetPropertyId, PropertyId sourcePropertyId) const;

    [[nodiscard]] std::vector<RankedValidationResult> getValueDependencyValidationResults(PropertyId propertyId) const;
    [[nodiscard]] RankedValidationResult validateSourcePropertyValueForWrite(PropertyId targetPropertyId, PropertyId sourcePropertyId) const;

    [[nodiscard]] const std::set<PropertyId>& getValidationDependencyPropertyIds(PropertyId propertyId) const;

    template<class ValueType>
    [[nodiscard]] RankedValidationResult validateValueForWrite(PropertyId propertyId, const ValueType& value) const;

    [[nodiscard]] std::string getValueAsString(PropertyId propertyId) const;

    template<class ValueType>
    [[nodiscard]] OptionalResult<ValueType> getValue(PropertyId propertyId) const;

    template<class ValueType>
    [[nodiscard]] std::map<ValueType, std::string> getValueToUserNameMap(PropertyId propertyId) const;

    template<class ValueType>
    [[nodiscard]] std::vector<ValueType> getMinAndMaxValidValues(PropertyId propertyId) const;

    template<class ValueType>
    [[nodiscard]] std::string convertToString(PropertyId propertyId, const ValueType& value) const;

    template<class ValueType>
    [[nodiscard]] VoidResult setValue(PropertyId propertyId, const ValueType& newValue) const;

    [[nodiscard]] VoidResult getLastWriteResult(PropertyId propertyId) const;

    template<class T>
    [[nodiscard]] std::future<ValueResult<std::vector<T>>> readDataSimple(uint32_t address, size_t dataCount) const;
    template<class T>
    [[nodiscard]] std::future<VoidResult> writeDataSimple(std::span<const T> data, uint32_t address) const;

    template<class T>
    [[nodiscard]] std::future<ValueResult<std::vector<T>>> readDataWithProgress(uint32_t address, size_t dataCount,
                                                                                const std::string& taskName, const std::string& errorMessage) const;
    template<class T>
    [[nodiscard]] std::future<ValueResult<std::vector<T>>> readDataWithProgress(uint32_t address, size_t dataCount,
                                                                                ProgressTask progressTask) const;

    template<class T>
    [[nodiscard]] std::future<VoidResult> writeDataWithProgress(std::span<const T> data, uint32_t address,
                                                                const std::string& taskName, const std::string& errorMessage) const;
    template<class T>
    [[nodiscard]] std::future<VoidResult> writeDataWithProgress(std::span<const T> data, uint32_t address,
                                                                ProgressTask progressTask) const;

    const std::shared_ptr<Properties>& getProperties() const;

    LifetimeChecker getLifetimeChecker() const;

private:
    PropertyAdapterBase* getPropertyAdapter(PropertyId propertyId) const;
    const PropertyValues::Transaction& getValuesTransaction() const;

    std::shared_ptr<PropertiesTransactionData> m_transactionData;
};


class Properties::ConnectionExclusiveTransaction
{
    using BaseClass = PropertiesTransaction;

    explicit ConnectionExclusiveTransaction(const std::shared_ptr<ConnectionExclusiveTransactionData>& transactionData,
                                            const PropertiesTransaction& propertiesTransaction);
    friend class Properties;
public:

    [[nodiscard]] VoidResult setNonexclusiveMode(Mode mode) const;

    template<class T>
    [[nodiscard]] ValueResult<std::vector<T>> readData(uint32_t address, size_t dataCount) const;

    template<class T>
    [[nodiscard]] VoidResult writeData(std::span<const T> data, uint32_t address) const;

    template<class T>
    [[nodiscard]] ValueResult<std::vector<T>> readDataWithProgress(uint32_t address, size_t dataCount, ProgressTask progressTask) const;

    template<class T>
    [[nodiscard]] VoidResult writeDataWithProgress(std::span<const T> data, uint32_t address, ProgressTask progressTask) const;

    template<class ValueType>
    [[nodiscard]] VoidResult setValueAndConfirm(PropertyId propertyId, const ValueType& newValue) const;

    const PropertiesTransaction& getPropertiesTransaction() const;

private:
    static constexpr std::chrono::milliseconds WRITE_TIMER = std::chrono::milliseconds(100);

    std::shared_ptr<ConnectionExclusiveTransactionData> m_transactionData;
    PropertiesTransaction m_propertiesTransaction;
};


class Properties::TaskResultTransaction
{
    explicit TaskResultTransaction(const std::shared_ptr<TransactionData>& transactionData);
    explicit TaskResultTransaction(const std::shared_ptr<TaskResultTransactionData>& taskResultTransactionData);
    friend class Properties;
public:

    const PropertyValues::Transaction& getValuesTransaction() const;

private:
    std::shared_ptr<TransactionData> m_transactionData;
    std::shared_ptr<TaskResultTransactionData> m_taskResultTransactionData;
};


class Properties::ConnectionStateTransactionData
{
    explicit ConnectionStateTransactionData(const std::shared_ptr<PropertiesTransactionData>& propertiesTransactionData);
    friend class Properties;
public:
    ~ConnectionStateTransactionData();

    const std::shared_ptr<Properties>& getProperties() const;

    const std::optional<DeviceType>& getCurrentDeviceType() const;
    void setCurrentDeviceType(const std::optional<DeviceType>& deviceType);

    connection::IDeviceInterface* getDeviceInterface();

    ConnectionExclusiveTransaction createConnectionExclusiveTransaction() const;

private:
    std::shared_ptr<PropertiesTransactionData> m_transactionData;

    std::optional<ITaskManager::StopAndBlockTasks> m_stopAndBlockTasks;

    std::optional<DeviceType> m_currentDeviceType;
};


class Properties::ConnectionExclusiveTransactionData
{
    explicit ConnectionExclusiveTransactionData(const std::shared_ptr<PropertiesTransactionData>& propertiesTransactionData,
                                                bool cancelRunningTasks);
    friend class Properties;
public:
    ~ConnectionExclusiveTransactionData();

    const std::shared_ptr<Properties>& getProperties() const;

private:
    std::shared_ptr<PropertiesTransactionData> m_transactionData;

    std::optional<ITaskManager::PauseTasks> m_pauseTasks;
};


class Properties::AdapterTaskCreator
{
public:
    explicit AdapterTaskCreator(const std::weak_ptr<Properties>& properties);

    using GetTaskResultTransactionFunction = std::function<Properties::TaskResultTransaction ()>;
    using TaskSimpleFunction = std::function<VoidResult (connection::IDeviceInterface*, const GetTaskResultTransactionFunction&)>;
    using TaskWithProgressFunction = std::function<VoidResult (connection::IDeviceInterface*, ProgressController, const GetTaskResultTransactionFunction&)>;

    void createTaskSimpleRead(const connection::AddressRanges& addressRanges, const TaskSimpleFunction& taskFunction) const;
    void createTaskSimpleWrite(const connection::AddressRanges& addressRanges, const TaskSimpleFunction& taskFunction) const;

    void createTaskWithProgressRead(const connection::AddressRanges& addressRanges, const TaskWithProgressFunction& taskFunction) const;
    void createTaskWithProgressWrite(const connection::AddressRanges& addressRanges, const TaskWithProgressFunction& taskFunction) const;

private:
    void createTaskSimple(const connection::AddressRanges& addressRanges, const TaskSimpleFunction& taskFunction, ITaskManager::TaskType taskType) const;
    void createTaskWithProgress(const connection::AddressRanges& addressRanges, const TaskWithProgressFunction& taskFunction, ITaskManager::TaskType taskType) const;

    std::weak_ptr<Properties> m_properties;
};

} // namespace core

#endif // CORE_PROPERTIES_H
