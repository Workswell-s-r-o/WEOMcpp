#ifndef CORE_PROPERTYADAPTERVALUEDEVICESIMPLE_H
#define CORE_PROPERTYADAPTERVALUEDEVICESIMPLE_H

#include "core/properties/propertyadaptervaluedevice.h"


namespace core
{

template<class ValueType>
class PropertyAdapterValueDeviceSimple : public PropertyAdapterValueDevice<ValueType>
{
    using BaseClass = PropertyAdapterValueDevice<ValueType>;

public:
    using ValueReader = std::function<ValueResult<ValueType> (connection::IDeviceInterface*)>;
    using ValueWriter = std::function<VoidResult (connection::IDeviceInterface*, const ValueType&)>;

    explicit PropertyAdapterValueDeviceSimple(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
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
PropertyAdapterValueDeviceSimple<ValueType>::PropertyAdapterValueDeviceSimple(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
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
void PropertyAdapterValueDeviceSimple<ValueType>::addReadTask()
{
    const auto task = [this](connection::IDeviceInterface* device, const Properties::AdapterTaskCreator::GetTaskResultTransactionFunction& getTransactionFunction)
    {
        const auto newValue = m_valueReader(device);

        const auto transaction = getTransactionFunction();

        this->updateValueAfterRead(newValue, transaction.getValuesTransaction());

        return newValue.toVoidResult();
    };

    this->getTaskCreator().createTaskSimpleRead(this->getAddressRanges(), task);
}

template<class ValueType>
void PropertyAdapterValueDeviceSimple<ValueType>::addWriteTask(const ValueType& value, const PropertyValues::Transaction& )
{
    const auto task = [this, value](connection::IDeviceInterface* device, const Properties::AdapterTaskCreator::GetTaskResultTransactionFunction& getTransactionFunction)
    {
        ValueResult<ValueType> newValue = value;

        const auto writeResult = m_valueWriter(device, value);
        if ((m_valueReader != nullptr) && (!writeResult.isOk() || this->alwaysRereadValueAfterWrite()))
        {
            newValue = m_valueReader(device);
        }

        const auto transaction = getTransactionFunction();

        this->updateValueAfterWrite(writeResult, newValue, transaction.getValuesTransaction());

        return writeResult;
    };

    this->getTaskCreator().createTaskSimpleWrite(this->getAddressRanges(), task);
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDEVICESIMPLE_H
