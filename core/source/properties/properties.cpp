#include "core/properties/properties.h"

#include "core/utils.h"
#include "core/properties/transactionchanges.h"
#include "core/properties/propertydependencyvalidator.h"
#include "core/properties/taskmanagerdirect.h"
#include "core/properties/taskmanagerqueued.h"
#include "core/logging.h"
#include "core/misc/elapsedtimer.h"
#include "core/misc/verify.h"
#include "core/prtutils.h"

#include <boost/polymorphic_cast.hpp>
#include <thread>


namespace core
{

class Properties::TransactionData
{
public:
    explicit TransactionData(const std::shared_ptr<Properties>& properties, const PropertyValues::Transaction& valuesTransaction);
    ~TransactionData();

    void setConnectionChanged();

    void addPropertiesValuesChanged(const std::set<PropertyId>& propertiesValuesChanged);
    void addPropertyStatusChanged(PropertyId propertyId);
    void addPropertyWriteFinished(PropertyId propertyId, const VoidResult& writeResult);
    void touchDependentProperty(PropertyId propertyId);
    void touchProperty(PropertyId propertyId);

    const std::shared_ptr<Properties>& getProperties() const;

    const std::optional<PropertyValues::Transaction>& getValuesTransaction() const;
    void setValuesTransaction(const std::optional<PropertyValues::Transaction>& valuesTransaction);

    const std::shared_ptr<std::promise<bool>>& getLifetimeObject() const;

private:
    std::shared_ptr<Properties> m_properties;

    std::optional<PropertyValues::Transaction> m_valuesTransaction;

    bool m_connectionChanged {false};

    std::set<PropertyId> m_propertiesValuesChanged;
    std::set<PropertyId> m_propertiesStatusChanged;
    std::set<PropertyId> m_propertiesValueWritten;
    std::map<PropertyId, VoidResult> m_propertiesLastWritteErrors;
    std::set<PropertyId> m_touchedDependentProperties;

    std::shared_ptr<std::promise<bool>> m_lifetimeObject;
};


class Properties::PropertiesTransactionData : public TransactionData
{
    using BaseClass = TransactionData;

public:
    explicit PropertiesTransactionData(const std::shared_ptr<Properties>& properties, const PropertyValues::Transaction& valuesTransaction);
    ~PropertiesTransactionData();
};


class Properties::TaskResultTransactionData
{
public:
    explicit TaskResultTransactionData(const std::shared_ptr<Properties>& properties, const PropertyValues::Transaction& valuesTransaction);
    ~TaskResultTransactionData();

    const PropertyValues::Transaction& getValuesTransaction() const;

private:
    std::shared_ptr<Properties> m_properties;

    PropertyValues::Transaction m_valuesTransaction;
};

Properties::Properties(const std::shared_ptr<connection::IDeviceInterface>& deviceInterface,
    Mode mode, const std::shared_ptr<IMainThreadIndicator>& indicator) :
    m_deviceInterface(deviceInterface),
    m_mainThreadIndicator(indicator),
    m_propertyValues(PropertyValues::createInstance())
{
    m_nonexclusiveTaskManager = createNewTaskManager(mode);
}

Properties::~Properties()
{
}

Properties::PropertiesTransaction Properties::createPropertiesTransaction()
{
    const auto data = createPropertiesTransactionData();

    return PropertiesTransaction(data);
}

std::optional<Properties::PropertiesTransaction> Properties::tryCreatePropertiesTransaction(const std::chrono::steady_clock::duration& timeout)
{
    if (!m_outerTransactionMutex.try_lock_for(timeout))
    {
        return std::nullopt;
    }
    assert(m_connectionStateTransactionData.lock() == nullptr && m_connectionExclusiveTransactionData.lock() == nullptr);

    return PropertiesTransaction(createPropertiesTransactionDataImpl());
}

Properties::ConnectionExclusiveTransaction Properties::createConnectionExclusiveTransaction(bool cancelRunningTasks)
{
    if (!m_outerTransactionMutex.try_lock())
    {
        m_outerTransactionMutex.lock();
    }

    assert(m_connectionStateTransactionData.lock() == nullptr && m_connectionExclusiveTransactionData.lock() == nullptr);

    const auto connectionExclusiveTransactionData = std::shared_ptr<ConnectionExclusiveTransactionData>(new ConnectionExclusiveTransactionData(createPropertiesTransactionDataImpl(), cancelRunningTasks));
    m_connectionExclusiveTransactionData = connectionExclusiveTransactionData;

    return ConnectionExclusiveTransaction(connectionExclusiveTransactionData, PropertiesTransaction(connectionExclusiveTransactionData->m_transactionData));
}

Properties::TaskResultTransaction Properties::getTaskResultTransaction()
{
    const auto currentMode = getMode(getTaskManager());
    if (currentMode == Mode::ASYNC_QUEUED)
    {
        const auto valuesTransaction = m_propertyValues->createTransaction();

        auto transactionData = m_transactionData.lock();
        if (transactionData != nullptr)
        {
            // we are in connection state or connection parameters transaction - it is trying to finish running tasks

            assert(m_taskResultTransactionData.lock() == nullptr);
            const auto taskResultTransactionData = std::make_shared<TaskResultTransactionData>(m_weakThis.lock(), valuesTransaction);
            m_taskResultTransactionData = taskResultTransactionData;

            return TaskResultTransaction(taskResultTransactionData);
        }
        else
        {
            transactionData = std::make_shared<TransactionData>(m_weakThis.lock(), valuesTransaction);
            m_transactionData = transactionData;

            return TaskResultTransaction(transactionData);
        }
    }
    else
    {
        assert(currentMode == Mode::SYNC_DIRECT);

        const auto transactionData = m_transactionData.lock();
        assert(transactionData != nullptr);

        return TaskResultTransaction(transactionData);
    }
}

const std::optional<DeviceType>& Properties::getDeviceType() const
{
    return m_currentDeviceType;
}

const std::shared_ptr<ProgressNotifier>& Properties::getCommunicationProgressNotifier() const
{
    return m_nonexclusiveTaskManager->getProgressNotifier();
}

const std::optional<DeviceType>& Properties::getCurrentDeviceType(const PropertiesTransaction& transaction) const
{
    assert(transaction.getProperties().get() == this);

    if (const auto connectionStateTransactionData = m_connectionStateTransactionData.lock())
    {
        return connectionStateTransactionData->getCurrentDeviceType();
    }

    return getDeviceType();
}

const std::vector<PropertyId>& Properties::getPropertiesToTouchAfterConnect(const PropertiesTransaction& transaction)
{
    assert(transaction.getProperties().get() == this);

    return m_propertiesToTouchAfterConnect;
}

void Properties::setPropertiesToTouchAfterConnect(const std::vector<PropertyId>& propertiesToTouchAfterConnect, const PropertiesTransaction& transaction)
{
    assert(transaction.getProperties().get() == this);

    m_propertiesToTouchAfterConnect = propertiesToTouchAfterConnect;
}

void Properties::onCurrentDeviceTypeChanged()
{
}

void Properties::onTransactionFinished(const TransactionSummary& transactionSummary)
{
    if (!transactionSummary.getTransactionChanges().isEmpty())
    {
        transactionFinished(transactionSummary);
    }
}

const ITaskManager* Properties::getTaskManager() const
{
    if (m_exclusiveTaskManager != nullptr)
    {
        return m_exclusiveTaskManager.get();
    }

    return m_nonexclusiveTaskManager.get();
}

ITaskManager* Properties::getTaskManager()
{
    if (m_exclusiveTaskManager != nullptr)
    {
        return m_exclusiveTaskManager.get();
    }

    return m_nonexclusiveTaskManager.get();
}

const std::shared_ptr<PropertyValues>& Properties::getPropertyValues() const
{
    return m_propertyValues;
}

std::set<PropertyId> Properties::getMappedPropertiesInConflict(const connection::AddressRange& addressRange, DeviceType deviceType) const
{
    return m_adapterAddressRangeMaps.at(deviceType).getOverlap(addressRange);
}

std::shared_ptr<Properties::PropertiesTransactionData> Properties::createPropertiesTransactionData()
{
    const ElapsedTimer timer;

    if (!m_outerTransactionMutex.try_lock())
    {
        m_outerTransactionMutex.lock();
    }
    assert(m_connectionStateTransactionData.lock() == nullptr && m_connectionExclusiveTransactionData.lock() == nullptr);

    if (timer.getElapsedMilliseconds() > 1)
    {
        WW_LOG_PROPERTIES_DEBUG << utils::format("transaction created - thread: {} waited: {}ms",
                                               m_mainThreadIndicator->isInGuiThread() ? "GUI" : "Other",
                                               timer.getElapsedMilliseconds());
    }

    return createPropertiesTransactionDataImpl();
}

std::shared_ptr<Properties::PropertiesTransactionData> Properties::createPropertiesTransactionDataImpl()
{
    const ElapsedTimer timer;

    const auto valuesTransaction = m_propertyValues->createTransaction();

    if (timer.getElapsedMilliseconds() > 1)
    {
        WW_LOG_PROPERTIES_WARNING << utils::format("lock adapters DELAY! - thread: {} waited: {}ms",
                                                   m_mainThreadIndicator->isInGuiThread() ? "GUI" : "Other",
                                                   timer.getElapsedMilliseconds());
    }

    auto propertiesTransactionData = std::make_shared<PropertiesTransactionData>(m_weakThis.lock(), valuesTransaction);

    assert(m_transactionData.lock() == nullptr);
    m_transactionData = propertiesTransactionData;

    return propertiesTransactionData;
}

void Properties::addValueAdapter(const std::shared_ptr<PropertyValueBase>& value, const std::shared_ptr<PropertyAdapterBase>& adapter)
{
    assert(value->getPropertyId() == adapter->getPropertyId());

    m_propertyValues->addProperty(value);

    mapAdapterAddressRanges(adapter);

    VERIFY(m_propertyAdapters.emplace(adapter->getPropertyId(), adapter).second);

    adapter.get()->statusChanged.connect([this](size_t propertyInternalId, PropertyAdapterBase::Status status)
    {
        const auto propertyId = core::PropertyId::getPropertyIdByInternalId(propertyInternalId);
        assert(propertyId.has_value() && "Invalid id!");
        if (propertyId.has_value())
        {
            m_transactionData.lock()->addPropertyStatusChanged(propertyId.value());
        }
    });

    adapter.get()->valueWriteFinished.connect([this](size_t propertyInternalId, const std::string& generalErrorMessage, const std::string& detailErrorMessage)
    {
        const auto propertyId = core::PropertyId::getPropertyIdByInternalId(propertyInternalId);
        assert(propertyId.has_value() && "Invalid id!");
        if (propertyId.has_value())
        {
            const auto writeResult = generalErrorMessage.empty() ? VoidResult::createOk() : VoidResult::createError(generalErrorMessage, detailErrorMessage);
            m_transactionData.lock()->addPropertyWriteFinished(propertyId.value(), writeResult);
        }
    });

    adapter.get()->touchDependentProperty.connect([this](size_t propertyInternalId)
    {
        const auto propertyId = core::PropertyId::getPropertyIdByInternalId(propertyInternalId);
        assert(propertyId.has_value() && "Invalid id!");
        if (propertyId.has_value())
        {
            m_transactionData.lock()->touchDependentProperty(propertyId.value());
        }
    });
}

void Properties::removeValueAdapter(PropertyId propertyId)
{
    const auto it = m_propertyAdapters.find(propertyId);
    if (it != m_propertyAdapters.end())
    {
        const auto adapter = it->second;

        assert(adapter->getSubsidiaryAdaptersPropertyIds().empty());
        assert(adapter->getValidationDependencyPropertyIds().empty());

        unmapAdapterAddressRanges(adapter);

        adapter.get()->statusChanged.disconnect_all_slots();
        adapter.get()->touchDependentProperty.disconnect_all_slots();
        adapter.get()->valueWriteFinished.disconnect_all_slots();

        m_propertyValues->removeProperty(propertyId);
        m_propertyAdapters.erase(it);
    }

    assert(m_propertyAdapters.size() == m_propertyValues->getPropertyIds().size());
}

void Properties::addPropertyDependencyValidator(const std::shared_ptr<PropertyDependencyValidator>& validator)
{
    for (const auto property : validator->getPropertyIds())
    {
        m_propertyAdapters.at(property)->addDependencyValidator(validator);
    }
}

void Properties::mapAdapterAddressRanges(const std::shared_ptr<PropertyAdapterBase>& adapter)
{
    if (!adapter->getAddressRanges().getRanges().empty())
    {
        for (const auto deviceType : DeviceType::getAllDeviceTypes())
        {
            if (adapter->isActiveForDeviceType(deviceType))
            {
                VERIFY(m_adapterAddressRangeMaps[deviceType].addRanges(adapter->getAddressRanges(), adapter->getPropertyId()));
            }
        }
    }
}

void Properties::unmapAdapterAddressRanges(const std::shared_ptr<PropertyAdapterBase>& adapter)
{
    for (auto& [deviceType, addressRangeMap] : m_adapterAddressRangeMaps)
    {
        addressRangeMap.removeRanges(adapter->getPropertyId());
    }
}

void Properties::setCurrentDeviceType(const std::optional<DeviceType>& deviceType, const PropertyValues::Transaction& transaction)
{
    if (m_currentDeviceType != deviceType)
    {
        m_currentDeviceType = deviceType;

        onCurrentDeviceTypeChanged();

        for (auto& [property, adapter] : m_propertyAdapters)
        {
            adapter->updateStatusDeviceChanged(m_currentDeviceType, transaction);
        }

        m_transactionData.lock()->setConnectionChanged();
    }
}

void Properties::invalidateProperties(const connection::AddressRanges& addressRanges)
{
    if (const auto deviceType = getDeviceType(); deviceType.has_value())
    {
        const auto invalidProperties = m_adapterAddressRangeMaps.at(deviceType.value()).getOverlap(addressRanges);
        if (!invalidProperties.empty())
        {
            const auto transaction = getTaskResultTransaction();

            for (const auto property : invalidProperties)
            {
                transaction.getValuesTransaction().resetValue(property);
            }
        }
    }
}

VoidResult Properties::tryLoadProperties(const std::set<PropertyId>& properties, const std::chrono::steady_clock::duration& timeout)
{
    {
        const auto transaction = createPropertiesTransaction();
        for (const auto& property : properties)
        {
            transaction.touch(property);
        }
    }

    const ElapsedTimer timer(timeout);
    while (true)
    {
        {
            const auto transaction = createPropertiesTransaction();
            if (std::all_of(properties.begin(), properties.end(), [transaction](PropertyId property){ return transaction.hasValueResult(property); }))
            {
                break;
            }
        }

        if (timer.timedOut())
        {
            return VoidResult::createError("Read parameters error!", utils::format("timedout: {}ms", timer.getElapsedMilliseconds()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return VoidResult::createOk();
}

void Properties::refreshProperties(const std::set<PropertyId>& properties, std::optional<PropertiesTransaction>& transaction)
{
    assert(transaction.has_value());
    if (!getCurrentDeviceType(transaction.value()).has_value())
    {
        return;
    }

    for (const auto& propertyToTouch : properties)
    {
        transaction->touch(propertyToTouch);
    }
}

Properties::Mode Properties::getMode(const ITaskManager* taskManager) const
{
    if (dynamic_cast<const TaskManagerQueued*>(taskManager) != nullptr)
    {
        return Mode::ASYNC_QUEUED;
    }
    else
    {
        assert(dynamic_cast<const TaskManagerDirect*>(taskManager) != nullptr);

        return Mode::SYNC_DIRECT;
    }
}

VoidResult Properties::setNonexclusiveMode(Mode mode)
{
    assert(m_nonexclusiveTaskManager->getDevice() == m_deviceInterface.get());

    const auto currentMode = getMode(m_nonexclusiveTaskManager.get());
    if (currentMode != mode)
    {
        if (currentMode == Mode::ASYNC_QUEUED)
        {
            auto* managerQueued = boost::polymorphic_downcast<TaskManagerQueued*>(m_nonexclusiveTaskManager.get());
            const auto taskCount = managerQueued->getTaskCount();
            assert(taskCount.runningTaskCount == 0);
            if (taskCount.waitingTaskCount > 0)
            {
                return VoidResult::createError("Unable to change mode!", utils::format("waiting tasks count: {}", taskCount.waitingTaskCount));
            }
        }

        if (m_nonexclusiveTaskManager != nullptr)
        {
            m_nonexclusiveTaskManager.get()->invalidateProperties.disconnect_all_slots();
        }
        m_nonexclusiveTaskManager = createNewTaskManager(mode);
    }

    return VoidResult::createOk();
}

std::shared_ptr<ITaskManager> Properties::createNewTaskManager(Mode mode)
{
    std::shared_ptr<ITaskManager> taskManager;

    std::shared_ptr<ProgressNotifier> communicationProgressNotifier;
    if (m_nonexclusiveTaskManager != nullptr)
    {
        communicationProgressNotifier = getCommunicationProgressNotifier();
    }

    if (mode == Mode::SYNC_DIRECT)
    {
        taskManager = TaskManagerDirect::createInstance(m_deviceInterface);
    }
    else
    {
        assert(mode == Mode::ASYNC_QUEUED);

        taskManager = TaskManagerQueued::createInstance(m_deviceInterface);
    }

    if (communicationProgressNotifier != nullptr)
    {
        taskManager->setProgressNotifier(communicationProgressNotifier); // Do not change progress notifier - someone might be connected!
    }

    taskManager.get()->invalidateProperties.connect([this](const connection::AddressRanges& addressRange){invalidateProperties(addressRange);});

    return taskManager;
}

std::shared_ptr<Properties::ConnectionStateTransactionData> Properties::createConnectionStateTransactionData()
{
    if (!m_outerTransactionMutex.try_lock())
    {
        m_outerTransactionMutex.lock();
    }
    assert(m_connectionStateTransactionData.lock() == nullptr && m_connectionExclusiveTransactionData.lock() == nullptr);

    const auto connectionStateTransactionData = std::shared_ptr<ConnectionStateTransactionData>(new ConnectionStateTransactionData(createPropertiesTransactionDataImpl()));
    m_connectionStateTransactionData = connectionStateTransactionData;

    return connectionStateTransactionData;
}

std::shared_ptr<Properties::ConnectionStateTransactionData> Properties::createConnectionStateTransactionDataFromConnectionExclusiveTransaction(const ConnectionExclusiveTransaction& connectionExclusiveTransaction)
{
    auto connectionStateTransactionData = m_connectionStateTransactionData.lock();
    if (connectionStateTransactionData == nullptr)
    {
        connectionStateTransactionData = std::shared_ptr<ConnectionStateTransactionData>(new ConnectionStateTransactionData(connectionExclusiveTransaction.m_transactionData->m_transactionData));
        m_connectionStateTransactionData = connectionStateTransactionData;
    }

    return connectionStateTransactionData;
}

const std::map<PropertyId, std::shared_ptr<PropertyAdapterBase>>& Properties::getPropertyAdapters() const
{
    return m_propertyAdapters;
}

const connection::IDeviceInterface* Properties::getDeviceInterface() const
{
    return m_deviceInterface.get();
}

connection::IDeviceInterface* Properties::getDeviceInterface()
{
    return m_deviceInterface.get();
}

Properties::TransactionData::TransactionData(const std::shared_ptr<Properties>& properties, const PropertyValues::Transaction& valuesTransaction) :
    m_properties(properties),
    m_valuesTransaction(valuesTransaction)
{
    assert(m_properties != nullptr);

    m_lifetimeObject = std::make_shared<std::promise<bool>>();
}

Properties::TransactionData::~TransactionData()
{
    auto propertiesValueChanged = m_valuesTransaction->getPropertiesChanged();
    propertiesValueChanged.insert(m_propertiesValuesChanged.begin(), m_propertiesValuesChanged.end());

    m_valuesTransaction.reset();

    m_properties->onTransactionFinished(TransactionSummary(
                                            std::make_shared<TransactionChanges>(m_propertiesStatusChanged,
                                                                                 propertiesValueChanged,
                                                                                 m_propertiesValueWritten,
                                                                                 m_propertiesLastWritteErrors,
                                                                                 m_connectionChanged),
                                            m_lifetimeObject));
}

void Properties::TransactionData::setConnectionChanged()
{
    m_connectionChanged = true;
}

void Properties::TransactionData::addPropertiesValuesChanged(const std::set<PropertyId>& propertiesValuesChanged)
{
    m_propertiesValuesChanged.insert(propertiesValuesChanged.begin(), propertiesValuesChanged.end());
}

void Properties::TransactionData::addPropertyStatusChanged(PropertyId propertyId)
{
    m_propertiesStatusChanged.insert(propertyId);
}

void Properties::TransactionData::addPropertyWriteFinished(PropertyId propertyId, const VoidResult& writeResult)
{
    if (writeResult.isOk())
    {
        m_propertiesValueWritten.insert(propertyId);
    }
    else
    {
        m_propertiesLastWritteErrors[propertyId] = writeResult;
    }
}

void Properties::TransactionData::touchDependentProperty(PropertyId propertyId)
{
    if (m_touchedDependentProperties.insert(propertyId).second)
    {
        touchProperty(propertyId);
    }
}

void Properties::TransactionData::touchProperty(PropertyId propertyId)
{
    if (getValuesTransaction().has_value())
    {
        getProperties()->m_propertyAdapters.at(propertyId)->touch(getValuesTransaction().value());
    }
    else
    {
        // we are in connection state or connection parameters transaction - it is trying to finish running tasks

        assert(getProperties()->m_taskResultTransactionData.lock() != nullptr);
        getProperties()->m_propertyAdapters.at(propertyId)->touch(getProperties()->m_taskResultTransactionData.lock()->getValuesTransaction());
    }
}

const std::shared_ptr<Properties>& Properties::TransactionData::getProperties() const
{
    return m_properties;
}

const std::optional<PropertyValues::Transaction>& Properties::TransactionData::getValuesTransaction() const
{
    return m_valuesTransaction;
}

void Properties::TransactionData::setValuesTransaction(const std::optional<PropertyValues::Transaction>& valuesTransaction)
{
    if (m_valuesTransaction.has_value() && !valuesTransaction.has_value())
    {
        addPropertiesValuesChanged(m_valuesTransaction->getPropertiesChanged());
    }

    m_valuesTransaction = valuesTransaction;
}

const std::shared_ptr<std::promise<bool>>& Properties::TransactionData::getLifetimeObject() const
{
    return m_lifetimeObject;
}

Properties::PropertiesTransactionData::PropertiesTransactionData(const std::shared_ptr<Properties>& properties, const PropertyValues::Transaction& valuesTransaction) :
    BaseClass(properties, valuesTransaction)
{
}

Properties::PropertiesTransactionData::~PropertiesTransactionData()
{
    assert(getProperties()->m_connectionStateTransactionData.lock() == nullptr && getProperties()->m_connectionExclusiveTransactionData.lock() == nullptr);

    getProperties()->m_outerTransactionMutex.unlock();
}

Properties::TaskResultTransactionData::TaskResultTransactionData(const std::shared_ptr<Properties>& properties, const PropertyValues::Transaction& valuesTransaction) :
    m_properties(properties),
    m_valuesTransaction(valuesTransaction)
{
}

Properties::TaskResultTransactionData::~TaskResultTransactionData()
{
    const auto transactionData = m_properties->m_transactionData.lock();
    assert(transactionData != nullptr);

    transactionData->addPropertiesValuesChanged(m_valuesTransaction.getPropertiesChanged());
}

const PropertyValues::Transaction& Properties::TaskResultTransactionData::getValuesTransaction() const
{
    return m_valuesTransaction;
}

Properties::PropertiesTransaction::PropertiesTransaction(const std::shared_ptr<PropertiesTransactionData>& propertiesTransactionData) :
    m_transactionData(propertiesTransactionData)
{
}

std::set<PropertyId> Properties::PropertiesTransaction::getAllProperyIds() const
{
    std::set<PropertyId> properyIds;

    for (const auto& [propertyId, adapter] : m_transactionData->getProperties()->m_propertyAdapters)
    {
        properyIds.insert(propertyId);
    }

    return properyIds;
}

bool Properties::PropertiesTransaction::isPropertyActiveForCurrentDevice(PropertyId propertyId) const
{
    const auto itAdapter = m_transactionData->getProperties()->m_propertyAdapters.find(propertyId);
    if (itAdapter == m_transactionData->getProperties()->m_propertyAdapters.end())
    {
        return false;
    }

    const auto* adapter = itAdapter->second.get();
    return adapter->isActiveForDeviceType(m_transactionData->getProperties()->getCurrentDeviceType(*this));
}

bool Properties::PropertiesTransaction::hasProperty(PropertyId propertyId) const
{
    const auto itAdapter = m_transactionData->getProperties()->m_propertyAdapters.find(propertyId);
    if (itAdapter == m_transactionData->getProperties()->m_propertyAdapters.end())
    {
        return false;
    }

    const auto* adapter = itAdapter->second.get();
    return adapter->isReadable(getValuesTransaction()) || adapter->isWritable(getValuesTransaction());
}

bool Properties::PropertiesTransaction::isPropertyReadable(PropertyId propertyId) const
{
    if (const auto* adapter = getPropertyAdapter(propertyId))
    {
        return adapter->isReadable(getValuesTransaction());
    }
    return false;
}

bool Properties::PropertiesTransaction::isPropertyWritable(PropertyId propertyId) const
{
    if (const auto* adapter = getPropertyAdapter(propertyId))
    {
        return adapter->isWritable(getValuesTransaction());
    }
    return false;
}

const std::type_info& Properties::PropertiesTransaction::getPropertyTypeInfo(PropertyId propertyId) const
{
    return derefPtr(getPropertyAdapter(propertyId)).getTypeInfo();
}

void Properties::PropertiesTransaction::touch(PropertyId propertyId) const
{
    if (auto* adapter = getPropertyAdapter(propertyId))
    {
        adapter->touch(getValuesTransaction());
    }
}

void Properties::PropertiesTransaction::resetValue(PropertyId propertyId) const
{
    getValuesTransaction().resetValue(propertyId);
}

void Properties::PropertiesTransaction::refreshValue(PropertyId propertyId) const
{
    derefPtr(getPropertyAdapter(propertyId)).refreshValue(getValuesTransaction());
}

std::future<void> Properties::PropertiesTransaction::refreshValueAsync(PropertyId propertyId) const
{
    auto promise = std::make_shared<std::promise<void>>();
    const auto& valueTransaction = getValuesTransaction();

    auto* propertyValue = valueTransaction.getPropertyValue(propertyId);
    auto connection = std::make_shared<boost::signals2::connection>();
    *connection = propertyValue->valueChanged.connect([connection, promise, propertyId](size_t propertyInternalId)
    {
        if (core::PropertyId::getPropertyIdByInternalId(propertyInternalId) == propertyId)
        {
            connection->disconnect();
            promise->set_value();
        }
    });
    derefPtr(getPropertyAdapter(propertyId)).refreshValue(valueTransaction);

    return promise->get_future();
}

void Properties::PropertiesTransaction::invalidateValue(PropertyId propertyId) const
{
    derefPtr(getPropertyAdapter(propertyId)).invalidateValue(getValuesTransaction());
}

bool Properties::PropertiesTransaction::hasValueResult(PropertyId propertyId) const
{
    return getValuesTransaction().hasValueResult(propertyId);
}

VoidResult Properties::PropertiesTransaction::getPropertyValidationResult(PropertyId propertyId) const
{
    return getValuesTransaction().getPropertyValidationResult(propertyId);
}

bool Properties::PropertiesTransaction::areValuesEqual(PropertyId propertyId1, PropertyId propertyId2) const
{
    return getValuesTransaction().areValuesEqual(propertyId1, propertyId2);
}

VoidResult Properties::PropertiesTransaction::setValueAccording(PropertyId targetPropertyId, PropertyId sourcePropertyId) const
{
    return derefPtr(getPropertyAdapter(targetPropertyId)).setValueAccording(getPropertyAdapter(sourcePropertyId), getValuesTransaction());
}

std::vector<RankedValidationResult> Properties::PropertiesTransaction::getValueDependencyValidationResults(PropertyId propertyId) const
{
    return derefPtr(getPropertyAdapter(propertyId)).getValueDependencyValidationResults();
}

RankedValidationResult Properties::PropertiesTransaction::validateSourcePropertyValueForWrite(PropertyId targetPropertyId, PropertyId sourcePropertyId) const
{
    return derefPtr(getPropertyAdapter(targetPropertyId)).validateSourcePropertyValueForWrite(sourcePropertyId, getValuesTransaction());
}

const std::set<PropertyId>& Properties::PropertiesTransaction::getValidationDependencyPropertyIds(PropertyId propertyId) const
{
    return derefPtr(getPropertyAdapter(propertyId)).getValidationDependencyPropertyIds();
}

std::string Properties::PropertiesTransaction::getValueAsString(PropertyId propertyId) const
{
    return derefPtr(getPropertyAdapter(propertyId)).getValueAsString(getValuesTransaction());
}

VoidResult Properties::PropertiesTransaction::getLastWriteResult(PropertyId propertyId) const
{
    return derefPtr(getPropertyAdapter(propertyId)).getLastWriteResult();
}

const std::shared_ptr<Properties>& Properties::PropertiesTransaction::getProperties() const
{
    return m_transactionData->getProperties();
}

LifetimeChecker Properties::PropertiesTransaction::getLifetimeChecker() const
{
    return LifetimeChecker(*m_transactionData->getLifetimeObject().get(), (size_t)m_transactionData->getLifetimeObject().get());
}

PropertyAdapterBase* Properties::PropertiesTransaction::getPropertyAdapter(PropertyId propertyId) const
{
    const auto it = m_transactionData->getProperties()->m_propertyAdapters.find(propertyId);
    return (it != m_transactionData->getProperties()->m_propertyAdapters.end()) ? it->second.get() : nullptr;
}

const PropertyValues::Transaction& Properties::PropertiesTransaction::getValuesTransaction() const
{
    return m_transactionData->getValuesTransaction().value();
}

Properties::ConnectionExclusiveTransaction::ConnectionExclusiveTransaction(const std::shared_ptr<ConnectionExclusiveTransactionData>& transactionData,
                                                                           const PropertiesTransaction& propertiesTransaction) :
    m_transactionData(transactionData),
    m_propertiesTransaction(propertiesTransaction)
{
}

VoidResult Properties::ConnectionExclusiveTransaction::setNonexclusiveMode(Mode mode) const
{
    return getPropertiesTransaction().getProperties()->setNonexclusiveMode(mode);
}

const Properties::PropertiesTransaction& Properties::ConnectionExclusiveTransaction::getPropertiesTransaction() const
{
    return m_propertiesTransaction;
}

Properties::TaskResultTransaction::TaskResultTransaction(const std::shared_ptr<TransactionData>& transactionData) :
    m_transactionData(transactionData)
{
}

Properties::TaskResultTransaction::TaskResultTransaction(const std::shared_ptr<TaskResultTransactionData>& taskResultTransactionData) :
    m_taskResultTransactionData(taskResultTransactionData)
{
}

const PropertyValues::Transaction& Properties::TaskResultTransaction::getValuesTransaction() const
{
    assert((m_transactionData != nullptr) != (m_taskResultTransactionData != nullptr));

    if (m_transactionData != nullptr)
    {
        assert(m_transactionData->getValuesTransaction().has_value());
        return m_transactionData->getValuesTransaction().value();
    }
    else
    {
        return m_taskResultTransactionData->getValuesTransaction();
    }
}

Properties::ConnectionStateTransactionData::ConnectionStateTransactionData(const std::shared_ptr<PropertiesTransactionData>& propertiesTransactionData) :
    m_transactionData(propertiesTransactionData),
    m_currentDeviceType(std::nullopt)
{
    m_transactionData->setValuesTransaction(std::nullopt);
    m_stopAndBlockTasks = m_transactionData->getProperties()->m_nonexclusiveTaskManager->getOrCreateStopAndBlockTasksObject();
    m_transactionData->setValuesTransaction(m_transactionData->getProperties()->m_propertyValues->createTransaction());

    m_transactionData->getProperties()->setCurrentDeviceType(getCurrentDeviceType(), m_transactionData->getValuesTransaction().value());
}

Properties::ConnectionStateTransactionData::~ConnectionStateTransactionData()
{
    m_transactionData->getProperties()->setCurrentDeviceType(getCurrentDeviceType(), m_transactionData->getValuesTransaction().value());

    const ConnectionExclusiveTransaction exclusiveTransaction = createConnectionExclusiveTransaction();
    for (const auto property : m_transactionData->getProperties()->m_propertiesToTouchAfterConnect)
    {
        exclusiveTransaction.getPropertiesTransaction().touch(property);
    }

    m_stopAndBlockTasks = std::nullopt;
}

const std::shared_ptr<Properties>& Properties::ConnectionStateTransactionData::getProperties() const
{
    return m_transactionData->getProperties();
}

const std::optional<DeviceType>& Properties::ConnectionStateTransactionData::getCurrentDeviceType() const
{
    return m_currentDeviceType;
}

void Properties::ConnectionStateTransactionData::setCurrentDeviceType(const std::optional<DeviceType>& deviceType)
{
    m_transactionData->setConnectionChanged();
    m_currentDeviceType = deviceType;
}

connection::IDeviceInterface* Properties::ConnectionStateTransactionData::getDeviceInterface()
{
    return m_transactionData->getProperties()->m_deviceInterface.get();
}

Properties::ConnectionExclusiveTransaction Properties::ConnectionStateTransactionData::createConnectionExclusiveTransaction() const
{
    auto connectionExclusiveTransactionData = m_transactionData->getProperties()->m_connectionExclusiveTransactionData.lock();
    if (connectionExclusiveTransactionData == nullptr)
    {
        connectionExclusiveTransactionData = std::shared_ptr<ConnectionExclusiveTransactionData>(new ConnectionExclusiveTransactionData(m_transactionData, false));
        m_transactionData->getProperties()->m_connectionExclusiveTransactionData = connectionExclusiveTransactionData;
    }

    return ConnectionExclusiveTransaction(connectionExclusiveTransactionData, PropertiesTransaction(connectionExclusiveTransactionData->m_transactionData));
}

Properties::ConnectionExclusiveTransactionData::ConnectionExclusiveTransactionData(const std::shared_ptr<PropertiesTransactionData>& propertiesTransactionData,
                                                                                   bool cancelRunningTasks) :
    m_transactionData(propertiesTransactionData)
{
    m_transactionData->setValuesTransaction(std::nullopt);
    m_pauseTasks = m_transactionData->getProperties()->m_nonexclusiveTaskManager->getOrCreatePauseTasksObject(cancelRunningTasks);
    m_transactionData->setValuesTransaction(m_transactionData->getProperties()->m_propertyValues->createTransaction());

    assert(getProperties()->m_exclusiveTaskManager == nullptr);
    getProperties()->m_exclusiveTaskManager = getProperties()->createNewTaskManager(Mode::SYNC_DIRECT);
}

Properties::ConnectionExclusiveTransactionData::~ConnectionExclusiveTransactionData()
{
    assert(getProperties()->m_exclusiveTaskManager != nullptr);
    if (getProperties()->m_exclusiveTaskManager != nullptr)
    {
        getProperties()->m_exclusiveTaskManager.get()->invalidateProperties.disconnect_all_slots();
        // Properties::disconnect(getProperties()->m_exclusiveTaskManager.get(), nullptr, getProperties().get(), nullptr);
    }
    getProperties()->m_exclusiveTaskManager = nullptr;

    m_pauseTasks = std::nullopt;
}

const std::shared_ptr<Properties>& Properties::ConnectionExclusiveTransactionData::getProperties() const
{
    return m_transactionData->getProperties();
}

Properties::AdapterTaskCreator::AdapterTaskCreator(const std::weak_ptr<Properties>& properties) :
    m_properties(properties)
{
}

void Properties::AdapterTaskCreator::createTaskSimpleRead(const connection::AddressRanges& addressRanges, const TaskSimpleFunction& taskFunction) const
{
    createTaskSimple(addressRanges, taskFunction, ITaskManager::TaskType::READ_PROPERTY);
}

void Properties::AdapterTaskCreator::createTaskSimpleWrite(const connection::AddressRanges& addressRanges, const TaskSimpleFunction& taskFunction) const
{
    createTaskSimple(addressRanges, taskFunction, ITaskManager::TaskType::WRITE_PROPERTY);
}

void Properties::AdapterTaskCreator::createTaskWithProgressRead(const connection::AddressRanges& addressRanges, const TaskWithProgressFunction& taskFunction) const
{
    createTaskWithProgress(addressRanges, taskFunction, ITaskManager::TaskType::READ_PROPERTY);
}

void Properties::AdapterTaskCreator::createTaskWithProgressWrite(const connection::AddressRanges& addressRanges, const TaskWithProgressFunction& taskFunction) const
{
    createTaskWithProgress(addressRanges, taskFunction, ITaskManager::TaskType::WRITE_PROPERTY);
}

void Properties::AdapterTaskCreator::createTaskSimple(const connection::AddressRanges& addressRanges, const TaskSimpleFunction& taskFunction, ITaskManager::TaskType taskType) const
{
    const auto properties = m_properties.lock();
    properties->getTaskManager()->addTaskSimple(addressRanges, taskType, [taskFunction, properties]() // capture properties shared_ptr to keep properties alive till task ends
    {
        return taskFunction(properties->getTaskManager()->getDevice(), [properties]()
        {
            return properties->getTaskResultTransaction();
        });
    });
}

void Properties::AdapterTaskCreator::createTaskWithProgress(const connection::AddressRanges& addressRanges, const TaskWithProgressFunction& taskFunction, ITaskManager::TaskType taskType) const
{
    const auto properties = m_properties.lock();
    properties->getTaskManager()->addTaskWithProgress(addressRanges, taskType, [taskFunction, properties](ProgressController progressController) // capture properties shared_ptr to keep properties alive till task ends
    {
        return taskFunction(properties->getTaskManager()->getDevice(), progressController, [properties]()
        {
            return properties->getTaskResultTransaction();
        });
    });
}

} // namespace core
