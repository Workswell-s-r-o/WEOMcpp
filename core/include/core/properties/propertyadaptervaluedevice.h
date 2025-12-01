#ifndef CORE_PROPERTYADAPTERVALUEDEVICE_H
#define CORE_PROPERTYADAPTERVALUEDEVICE_H

#include "core/properties/propertyadaptervalue.h"
#include "core/properties/properties.h"
#include "core/misc/result.h"
#include "core/utils.h"


namespace core
{

template<class ValueType>
class PropertyAdapterValueDevice : public PropertyAdapterValue<ValueType>
{
    using BaseClass = PropertyAdapterValue<ValueType>;

public:
    using TransformFunction = std::function<ValueType (const ValueType&, const PropertyValues::Transaction&)>;

    explicit PropertyAdapterValueDevice(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                        const Properties::AdapterTaskCreator& taskCreator,
                                        const connection::AddressRanges& addressRanges,
                                        const TransformFunction& transformFunction = nullptr);

    virtual void refreshValue(const PropertyValues::Transaction& transaction) override;
    virtual VoidResult setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction) override;
    virtual VoidResult getLastWriteResult() const override;
    virtual connection::AddressRanges getAddressRanges() const override;

    void setAlwaysRereadValueAfterWrite(bool alwaysRereadValueAfterWrite);

protected:
    virtual void addReadTask() = 0;
    virtual void addWriteTask(const ValueType& value, const PropertyValues::Transaction& transaction) = 0;

    bool alwaysRereadValueAfterWrite() const;

    void updateValueAfterRead(const ValueResult<ValueType>& value, const PropertyValues::Transaction& transaction);
    void updateValueAfterWrite(const VoidResult& writeResult, const ValueResult<ValueType>& value, const PropertyValues::Transaction& transaction);

    const Properties::AdapterTaskCreator& getTaskCreator() const;

private:
    ValueResult<ValueType> getTransformedAndValidatedValue(const ValueType& value, const PropertyValues::Transaction& transaction);

    Properties::AdapterTaskCreator m_taskCreator;
    connection::AddressRanges m_addressRanges;

    TransformFunction m_transformFunction;

    bool m_alwaysRereadValueAfterWrite {false};

    VoidResult m_lastWriteResult {VoidResult::createOk()};
};

// Impl

template<class ValueType>
PropertyAdapterValueDevice<ValueType>::PropertyAdapterValueDevice(PropertyId propertyId, const typename BaseClass::GetStatusForDeviceFunction& getStatusForDeviceFunction,
                                                                  const Properties::AdapterTaskCreator& taskCreator,
                                                                  const connection::AddressRanges& addressRanges,
                                                                  const TransformFunction& transformFunction) :
    BaseClass(propertyId, getStatusForDeviceFunction),
    m_taskCreator(taskCreator),
    m_addressRanges(addressRanges),
    m_transformFunction(transformFunction)
{

    this->statusChanged.connect([this](size_t propertyInternalId, PropertyAdapterBase::Status newStatus)
    {
        assert(propertyInternalId == this->getPropertyId().getInternalId());
        if (!this->isWritableStatus(newStatus))
        {
            m_lastWriteResult = VoidResult::createOk();
        }

    });
}

template<class ValueType>
void PropertyAdapterValueDevice<ValueType>::refreshValue(const PropertyValues::Transaction& transaction)
{
    if (this->isReadable(transaction))
    {
        addReadTask();

        this->touchDependentProperties(transaction);
    }
}

template<class ValueType>
VoidResult PropertyAdapterValueDevice<ValueType>::setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction)
{
    if (!this->isWritable(transaction))
    {
        return VoidResult::createError("Unable to write!", utils::format("adapter in non-writable mode - property: {}", this->getPropertyId().getIdString()));
    }

    auto value = newValue;
    if (m_transformFunction)
    {
        value = m_transformFunction(value, transaction);
    }

    const RankedValidationResult result = this->validateValueForWrite(value, transaction);
    if (!result.isAcceptable())
    {
        assert(!result.getResult().isOk());
        return result.getResult();
    }

    const OptionalResult<ValueType> oldValue = transaction.getValue<ValueType>(this->getPropertyId());
    if (oldValue.containsValue() && value == oldValue.getValue())
    {
        return VoidResult::createOk();
    }

    addWriteTask(value, transaction);

    return VoidResult::createOk();
}

template<class ValueType>
VoidResult PropertyAdapterValueDevice<ValueType>::getLastWriteResult() const
{
    return m_lastWriteResult;
}

template<class ValueType>
void PropertyAdapterValueDevice<ValueType>::setAlwaysRereadValueAfterWrite(bool alwaysRereadValueAfterWrite)
{
    m_alwaysRereadValueAfterWrite = alwaysRereadValueAfterWrite;
}

template<class ValueType>
connection::AddressRanges PropertyAdapterValueDevice<ValueType>::getAddressRanges() const
{
    return m_addressRanges;
}

template<class ValueType>
bool PropertyAdapterValueDevice<ValueType>::alwaysRereadValueAfterWrite() const
{
    return m_alwaysRereadValueAfterWrite;
}

template<class ValueType>
void PropertyAdapterValueDevice<ValueType>::updateValueAfterRead(const ValueResult<ValueType>& value, const PropertyValues::Transaction& transaction)
{
    if (this->isReadable(transaction))
    {
        const auto newValue = value.isOk() ? getTransformedAndValidatedValue(value.getValue(), transaction) : value;
        this->beforeValueUpdate(newValue, transaction);

        transaction.setValue<ValueType>(this->getPropertyId(), newValue);
    }
}

template<class ValueType>
void PropertyAdapterValueDevice<ValueType>::updateValueAfterWrite(const VoidResult& writeResult, const ValueResult<ValueType>& value, const PropertyValues::Transaction& transaction)
{
    if (this->isReadable(transaction))
    {
        const auto newValue = (writeResult.isOk() || !value.isOk()) ? value : getTransformedAndValidatedValue(value.getValue(), transaction);

        transaction.setValue<ValueType>(this->getPropertyId(), newValue);
    }

    m_lastWriteResult = writeResult;
    this->valueWriteFinished(this->getPropertyId().getInternalId(), writeResult.getGeneralErrorMessage(), writeResult.getDetailErrorMessage());
}

template<class ValueType>
const Properties::AdapterTaskCreator& PropertyAdapterValueDevice<ValueType>::getTaskCreator() const
{
    return m_taskCreator;
}

template<class ValueType>
ValueResult<ValueType> PropertyAdapterValueDevice<ValueType>::getTransformedAndValidatedValue(const ValueType& value, const PropertyValues::Transaction& transaction)
{
    ValueResult<ValueType> validatedValue = value;

    if (m_transformFunction != nullptr)
    {
        validatedValue = m_transformFunction(value, transaction);
    }

    const VoidResult result = this->validateValue(validatedValue.getValue(), transaction);
    if (!result.isOk())
    {
        validatedValue = ValueResult<ValueType>::createFromError(result);
    }

    return validatedValue;
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUEDEVICE_H
