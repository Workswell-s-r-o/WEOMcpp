#ifndef CORE_PROPERTIES_INL
#define CORE_PROPERTIES_INL

#include "properties.h"

#include "core/properties/propertyadaptervalue.h"
#include "core/properties/itaskmanager.h"
#include "core/connection/ideviceinterface.h"
#include "core/connection/addressrange.h"
#include "core/misc/elapsedtimer.h"

#include <chrono>


namespace core
{

template<class T>
[[nodiscard]] ValueResult<std::vector<T>> Properties::ConnectionExclusiveTransaction::readData(uint32_t address, size_t dataCount) const
{
    auto resultFuture = getPropertiesTransaction().readDataSimple<T>(address, dataCount);

    try
    {
        const auto result = resultFuture.get();
        assert(!result.isOk() || result.getValue().size() == dataCount);
        return result;
    }
    catch (...)
    {
        return ValueResult<std::vector<T>>::createError("Reading interrupted", "task terminated");
    }
}

template<class T>
[[nodiscard]] VoidResult Properties::ConnectionExclusiveTransaction::writeData(std::span<const T> data, uint32_t address) const
{
    auto resultFuture = getPropertiesTransaction().writeDataSimple(data, address);

    try
    {
        return resultFuture.get();
    }
    catch (...)
    {
        return VoidResult::createError("Writing interrupted", "task terminated");
    }
}

template<class T>
[[nodiscard]] ValueResult<std::vector<T>> Properties::ConnectionExclusiveTransaction::readDataWithProgress(uint32_t address, size_t dataCount, ProgressTask progressTask) const
{
    auto resultFuture = getPropertiesTransaction().readDataWithProgress<T>(address, dataCount, progressTask);

    try
    {
        const auto result = resultFuture.get();
        assert(!result.isOk() || result.getValue().size() == dataCount);
        return result;
    }
    catch (...)
    {
        return ValueResult<std::vector<T>>::createError("Reading interrupted", "task terminated");
    }
}

template<class T>
[[nodiscard]] VoidResult Properties::ConnectionExclusiveTransaction::writeDataWithProgress(std::span<const T> data, uint32_t address, ProgressTask progressTask) const
{
    auto resultFuture = getPropertiesTransaction().writeDataWithProgress(data, address, progressTask);

    try
    {
        return resultFuture.get();
    }
    catch (...)
    {
        return VoidResult::createError("Writing interrupted", "task terminated");
    }
}

template<class ValueType>
core::VoidResult core::Properties::ConnectionExclusiveTransaction::setValueAndConfirm(core::PropertyId propertyId, const ValueType& newValue) const
{
    const auto& transaction = getPropertiesTransaction();

    const auto setValueResult = transaction.setValue<ValueType>(propertyId, newValue);
    if (!setValueResult.isOk())
    {
        return setValueResult;
    }

    core::ElapsedTimer timer(WRITE_TIMER);
    while(!timer.timedOut())
    {
        transaction.resetValue(propertyId);
        auto writtenValue = transaction.getValue<ValueType>(propertyId);
        if(writtenValue.containsValue() && writtenValue.getValue() == newValue)
        {
            return core::VoidResult::createOk();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(WRITE_TIMER / 10));
    }

    return core::VoidResult::createError("setValueAndConfirm verification failed", "Timeout waiting for register value to be updated.");
}

template<class ValueType>
RankedValidationResult Properties::PropertiesTransaction::validateValueForWrite(PropertyId propertyId, const ValueType& value) const
{
    if (auto* adapter = dynamic_cast<PropertyAdapterValue<ValueType>*>(getPropertyAdapter(propertyId)))
    {
        return adapter->validateValueForWrite(value, getValuesTransaction());
    }

    assert(false && "PropertyAdapter for different data type!");
    return RankedValidationResult::createOk();
}

template<class ValueType>
OptionalResult<ValueType> Properties::PropertiesTransaction::getValue(PropertyId propertyId) const
{
    if (auto* adapter = dynamic_cast<PropertyAdapterValue<ValueType>*>(getPropertyAdapter(propertyId)))
    {
        return adapter->getValue(getValuesTransaction());
    }

    assert(false && "PropertyAdapter for different data type!");
    return std::nullopt;
}

template<class ValueType>
std::map<ValueType, std::string> Properties::PropertiesTransaction::getValueToUserNameMap(PropertyId propertyId) const
{
    return getValuesTransaction().getValueToUserNameMap<ValueType>(propertyId);
}

template<class ValueType>
std::vector<ValueType> Properties::PropertiesTransaction::getMinAndMaxValidValues(PropertyId propertyId) const
{
    return getValuesTransaction().getMinAndMaxValidValues<ValueType>(propertyId);
}

template<class ValueType>
std::string Properties::PropertiesTransaction::convertToString(PropertyId propertyId, const ValueType& value) const
{
    return getValuesTransaction().convertToString(propertyId, value);
}

template<class ValueType>
VoidResult Properties::PropertiesTransaction::setValue(PropertyId propertyId, const ValueType& newValue) const
{
    if (auto* adapter = dynamic_cast<PropertyAdapterValue<ValueType>*>(getPropertyAdapter(propertyId)))
    {
        return adapter->setValue(newValue, getValuesTransaction());
    }

    assert(false && "PropertyAdapter for different data type!");
    return VoidResult::createOk();
}

template<class T>
std::future<ValueResult<std::vector<T>>> Properties::PropertiesTransaction::readDataSimple(uint32_t address, size_t dataCount) const
{
    using ResultType = ValueResult<std::vector<T>>;
    auto promise = std::make_shared<std::promise<ResultType>>();
    auto result = promise->get_future();

    const auto addressRange = connection::AddressRange::firstAndSize(address, dataCount * sizeof(T));
    getProperties()->getTaskManager()->addTaskSimple(addressRange, ITaskManager::TaskType::READ_WILD, [=, properties = getProperties()]() // capture properties shared_ptr to keep properties alive till task ends
    {
        std::vector<T> data(dataCount, 0);
        const auto result = properties->getTaskManager()->getDevice()->readTypedData<T>(data, address, ProgressTask());
        if (!result.isOk())
        {
            promise->set_value(ResultType::createFromError(result));
        }
        else
        {
            promise->set_value(data);
        }

        return result;
    });

    return result;
}

template<class T>
std::future<VoidResult> Properties::PropertiesTransaction::writeDataSimple(std::span<const T> data, uint32_t address) const
{
    auto promise = std::make_shared<std::promise<VoidResult>>();
    auto result = promise->get_future();

    const auto addressRange = connection::AddressRange::firstAndSize(address, data.size() * sizeof(T));
    getProperties()->getTaskManager()->addTaskSimple(addressRange, ITaskManager::TaskType::WRITE_WILD, [=, byteData = getProperties()->getTaskManager()->getDevice()->toByteData<T>(data), properties = getProperties()]() // capture properties shared_ptr to keep properties alive till task ends
    {
        const auto result = properties->getTaskManager()->getDevice()->writeData(byteData, address, ProgressTask());
        promise->set_value(result);

        return result;
    });

    return result;
}

template<class T>
std::future<ValueResult<std::vector<T>>> Properties::PropertiesTransaction::readDataWithProgress(uint32_t address, size_t dataCount,
                                                                                                 const std::string& taskName, const std::string& errorMessage) const
{
    using ResultType = ValueResult<std::vector<T>>;
    auto promise = std::make_shared<std::promise<ResultType>>();
    auto result = promise->get_future();

    const auto addressRange = connection::AddressRange::firstAndSize(address, dataCount * sizeof(T));
    getProperties()->getTaskManager()->addTaskWithProgress(addressRange, ITaskManager::TaskType::READ_WILD, [=, properties = getProperties()](ProgressController progressController) // capture properties shared_ptr to keep properties alive till task ends
    {
        auto progressTask = progressController.createTaskBound(taskName, dataCount * sizeof(T), true);

        std::vector<T> data(dataCount, 0);
        const auto result = properties->getTaskManager()->getDevice()->readTypedData<T>(data, address, progressTask);
        if (!result.isOk())
        {
            progressTask.sendErrorMessage(errorMessage);

            promise->set_value(ResultType::createFromError(result));
        }
        else
        {
            promise->set_value(data);
        }

        return result;
    });

    return result;
}

template<class T>
std::future<ValueResult<std::vector<T>>> Properties::PropertiesTransaction::readDataWithProgress(uint32_t address, size_t dataCount,
                                                                                                 ProgressTask progressTask) const
{
    using ResultType = ValueResult<std::vector<T>>;
    auto promise = std::make_shared<std::promise<ResultType>>();
    auto result = promise->get_future();

    const auto addressRange = connection::AddressRange::firstAndSize(address, dataCount * sizeof(T));
    getProperties()->getTaskManager()->addTaskWithProgress(addressRange, ITaskManager::TaskType::READ_WILD, [=, properties = getProperties()](ProgressController progressController) // capture properties shared_ptr to keep properties alive till task ends
    {
        std::vector<T> data(dataCount, 0);
        const auto result = properties->getTaskManager()->getDevice()->readTypedData<T>(data, address, progressTask);
        if (!result.isOk())
        {
            promise->set_value(ResultType::createFromError(result));
        }
        else
        {
            promise->set_value(data);
        }

        return result;
    });

    return result;
}

template<class T>
std::future<VoidResult> Properties::PropertiesTransaction::writeDataWithProgress(std::span<const T> data, uint32_t address,
                                                                                 const std::string& taskName, const std::string& errorMessage) const
{
    auto promise = std::make_shared<std::promise<VoidResult>>();
    auto result = promise->get_future();

    const auto addressRange = connection::AddressRange::firstAndSize(address, data.size() * sizeof(T));
    getProperties()->getTaskManager()->addTaskWithProgress(addressRange, ITaskManager::TaskType::WRITE_WILD, [=, byteData = getProperties()->getTaskManager()->getDevice()->toByteData(data), properties = getProperties()](ProgressController progressController) // capture properties shared_ptr to keep properties alive till task ends
    {
        auto progressTask = progressController.createTaskBound(taskName, byteData.size(), false);

        const auto result = properties->getTaskManager()->getDevice()->writeData(byteData, address, progressTask);
        if (!result.isOk())
        {
            progressTask.sendErrorMessage(errorMessage);
        }

        promise->set_value(result);

        return result;
    });

    return result;
}

template<class T>
std::future<VoidResult> Properties::PropertiesTransaction::writeDataWithProgress(std::span<const T> data, uint32_t address,
                                                                                 ProgressTask progressTask) const
{
    auto promise = std::make_shared<std::promise<VoidResult>>();
    auto result = promise->get_future();

    const auto addressRange = connection::AddressRange::firstAndSize(address, data.size() * sizeof(T));
    getProperties()->getTaskManager()->addTaskSimple(addressRange, ITaskManager::TaskType::WRITE_WILD, [=, byteData = getProperties()->getTaskManager()->getDevice()->toByteData(data), properties = getProperties()]() // capture properties shared_ptr to keep properties alive till task ends
    {
        const auto result = properties->getTaskManager()->getDevice()->writeData(byteData, address, progressTask);
        promise->set_value(result);

        return result;
    });

    return result;
}

} // namespace core

#endif // CORE_PROPERTIES_INL

