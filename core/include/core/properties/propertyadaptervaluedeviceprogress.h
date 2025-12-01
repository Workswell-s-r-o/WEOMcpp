#ifndef CORE_PROPERTYADAPTERVALUEDEVICEPROGRESS_H
#define CORE_PROPERTYADAPTERVALUEDEVICEPROGRESS_H

#include "core/properties/propertyadaptervaluedevice.h"
#include "core/properties/itaskmanager.h"


namespace core
{

template<class ValueType>
class PropertyAdapterValueDeviceProgress : public PropertyAdapterValueDevice<ValueType>
{
    using BaseClass = PropertyAdapterValueDevice<ValueType>;

public:
    using ValueReader = std::function<ValueResult<ValueType> (connection::IDeviceInterface* device, ProgressController progressController)>;
    using ValueWriter = std::function<VoidResult (connection::IDeviceInterface* device, const ValueType&, ProgressController progressController)>;

    explicit PropertyAdapterValueDeviceProgress(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                const Properties::AdapterTaskCreator& taskCreator,
                                                const connection::AddressRanges& addressRanges,
                                                const ValueReader& valueReader, const ValueWriter& valueWriter,
                                                const typename BaseClass::TransformFunction& transformFunction = nullptr);

protected:
    virtual void addReadTask() override;
    virtual void addWriteTask(const ValueType& value, const PropertyValues::Transaction& transaction) override;

private:
    ValueReader m_valueReader;
    ValueWriter m_valueWriter;
};

// Impl

template<class ValueType>
PropertyAdapterValueDeviceProgress<ValueType>::PropertyAdapterValueDeviceProgress(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                                  const Properties::AdapterTaskCreator& taskCreator,
                                                                                  const connection::AddressRanges& addressRanges,
                                                                                  const ValueReader& valueReader, const ValueWriter& valueWriter,
                                                                                  const typename BaseClass::TransformFunction& transformFunction) :
    BaseClass(propertyId, getStatusForDeviceFunction, taskCreator, addressRanges, transformFunction),
    m_valueReader(valueReader),
    m_valueWriter(valueWriter)
{
}

template<class ValueType>
void PropertyAdapterValueDeviceProgress<ValueType>::addReadTask()
{
    const auto task = [this](connection::IDeviceInterface* device, ProgressController progressController, const Properties::AdapterTaskCreator::GetTaskResultTransactionFunction& getTransactionFunction)
    {
        const auto newValue = m_valueReader(device, progressController);

        const auto transaction = getTransactionFunction();

        this->updateValueAfterRead(newValue, transaction.getValuesTransaction());

        return newValue.toVoidResult();
    };

    this->getTaskCreator().createTaskWithProgressRead(this->getAddressRanges(), task);
}

template<class ValueType>
void PropertyAdapterValueDeviceProgress<ValueType>::addWriteTask(const ValueType& value, const PropertyValues::Transaction& transaction)
{
    transaction.resetValue(this->getPropertyId());

    const auto task = [this, value](connection::IDeviceInterface* device, ProgressController progressController, const Properties::AdapterTaskCreator::GetTaskResultTransactionFunction& getTransactionFunction)
    {
        ValueResult<ValueType> newValue = value;

        const auto writeResult = m_valueWriter(device, value, progressController);
        if ((m_valueReader != nullptr) && (!writeResult.isOk() || this->alwaysRereadValueAfterWrite()))
        {
            newValue = m_valueReader(device, progressController);
        }

        const auto transaction = getTransactionFunction();

        this->updateValueAfterWrite(writeResult, newValue, transaction.getValuesTransaction());

        return writeResult;
    };

    this->getTaskCreator().createTaskWithProgressWrite(this->getAddressRanges(), task);
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDEVICEPROGRESS_H
