#ifndef CORE_PROPERTYADAPTERVALUE_H
#define CORE_PROPERTYADAPTERVALUE_H

#include "core/properties/propertyadapterbase.h"
#include "core/connection/resultdeviceinfo.h"
#include "core/utils.h"


namespace core
{

template<class ValueType>
class PropertyAdapterValue : public PropertyAdapterBase
{
    using BaseClass = PropertyAdapterBase;

public:
    explicit PropertyAdapterValue(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction);

    virtual OptionalResult<ValueType> getValue(const PropertyValues::Transaction& transaction);
    virtual VoidResult setValue(const ValueType& newValue, const PropertyValues::Transaction& transaction) = 0;
    virtual RankedValidationResult validateValueForWrite(const ValueType& value, const PropertyValues::Transaction& transaction) const;

    virtual const std::type_info& getTypeInfo() override;

    virtual void touch(const PropertyValues::Transaction& transaction) override;
    virtual void invalidateValue(const PropertyValues::Transaction& transaction) override;
    virtual VoidResult setValueAccording(PropertyAdapterBase* sourceAdapter, const PropertyValues::Transaction& transaction) override;
    virtual RankedValidationResult validateSourcePropertyValueForWrite(PropertyId sourcePropertyId, const PropertyValues::Transaction& transaction) const override;

    static bool isRecoverableError(const OptionalResult<ValueType>& result);

protected:
    virtual VoidResult validateValue(const ValueType& value, const PropertyValues::Transaction& transaction) const;
    virtual void beforeValueUpdate(const OptionalResult<ValueType>& newValue, const PropertyValues::Transaction& transaction);

    void touchDependentProperties(const PropertyValues::Transaction& transaction) const;
};

// Impl

template<class ValueType>
PropertyAdapterValue<ValueType>::PropertyAdapterValue(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction) :
    BaseClass(propertyId, getStatusForDeviceFunction)
{
}

template<class ValueType>
OptionalResult<ValueType> PropertyAdapterValue<ValueType>::getValue(const PropertyValues::Transaction& transaction)
{
    touch(transaction);

    return transaction.getValue<ValueType>(getPropertyId());
}

template<class ValueType>
RankedValidationResult PropertyAdapterValue<ValueType>::validateValueForWrite(const ValueType& value, const PropertyValues::Transaction& transaction) const
{
    const VoidResult result = this->validateValue(value, transaction);
    if (!result.isOk())
    {
        return RankedValidationResult::createError(result);
    }

    this->touchDependentProperties(transaction);

    std::optional<RankedValidationResult> warning;
    for (const auto& validator : getDependencyValidators())
    {
        const auto result = validator->validateWhatIf(this->getPropertyId(), value, transaction);
        if (!result.isAcceptable())
        {
            return result;
        }

        if (!warning.has_value() && !result.getResult().isOk())
        {
            assert(result.getErrorRank() == RankedValidationResult::ErrorRank::WARNING);
            warning = result;
        }
    }

    if (warning.has_value())
    {
        return warning.value();
    }

    return RankedValidationResult::createOk();
}

template<class ValueType>
RankedValidationResult PropertyAdapterValue<ValueType>::validateSourcePropertyValueForWrite(PropertyId sourcePropertyId, const PropertyValues::Transaction& transaction) const
{
    const OptionalResult<ValueType> value = transaction.getValue<ValueType>(sourcePropertyId);
    if (!value.hasResult())
    {
        return RankedValidationResult::createError("Invalid value!", utils::format("property: {} value unknown",sourcePropertyId.getIdString()));
    }
    else if (value.containsError())
    {
        return RankedValidationResult::createError("Invalid value!", utils::format("property: {} error: {}",sourcePropertyId.getIdString(),value.getResult().getDetailErrorMessage()));
    }

    return validateValueForWrite(value.getValue(), transaction);
}

template<class ValueType>
void PropertyAdapterValue<ValueType>::touch(const PropertyValues::Transaction& transaction)
{
    const auto currentValue = transaction.getValue<ValueType>(getPropertyId());
    assert(isReadable(transaction) || isWritable(transaction) || !currentValue.hasResult());
    if (isReadable(transaction) && (!currentValue.hasResult() || isRecoverableError(currentValue)))
    {
        refreshValue(transaction);
    }
}

template<class ValueType>
void PropertyAdapterValue<ValueType>::invalidateValue(const PropertyValues::Transaction& transaction)
{
    const auto currentValue = transaction.getValue<ValueType>(getPropertyId());
    assert(isReadable(transaction) || !currentValue.hasResult());
    if (isReadable(transaction) && currentValue.hasResult())
    {
        refreshValue(transaction);
    }
}

template<class ValueType>
VoidResult PropertyAdapterValue<ValueType>::setValueAccording(PropertyAdapterBase* sourceAdapter, const PropertyValues::Transaction& transaction)
{
    if (auto* typedSourceAdapter = dynamic_cast<PropertyAdapterValue<ValueType>*>(sourceAdapter))
    {
        const auto currentValue = typedSourceAdapter->getValue(transaction);
        if (currentValue.containsValue())
        {
            return setValue(currentValue.getValue(), transaction);
        }
        else if (currentValue.containsError())
        {
            return currentValue.getResult().toVoidResult();
        }

        return VoidResult::createError("Unable to set value!", utils::format("unknown property {} value", getPropertyId().getIdString()));
    }

    assert(false && "PropertyAdapter for different data type!");
    return VoidResult::createOk();
}

template<class ValueType>
bool PropertyAdapterValue<ValueType>::isRecoverableError(const OptionalResult<ValueType>& result)
{
    if (!result.containsError())
    {
        return false;
    }

    const auto* resultDeviceInfo = dynamic_cast<const connection::ResultDeviceInfo*>(result.getResult().getSpecificInfo());
    if (resultDeviceInfo != nullptr)
    {
        return resultDeviceInfo->isRecoverableError();
    }

    return false;
}

template<class ValueType>
VoidResult PropertyAdapterValue<ValueType>::validateValue(const ValueType& value, const PropertyValues::Transaction& transaction) const
{
    return transaction.validateValue(this->getPropertyId(), value);
}

template<class ValueType>
void PropertyAdapterValue<ValueType>::beforeValueUpdate(const OptionalResult<ValueType>& , const PropertyValues::Transaction& )
{
}

template<class ValueType>
void PropertyAdapterValue<ValueType>::touchDependentProperties(const PropertyValues::Transaction& transaction) const
{
    for (const auto& validator : getDependencyValidators())
    {
        for (const auto propertyId : validator->getPropertyIds())
        {
            if (propertyId != this->getPropertyId() && !transaction.hasValueResult(propertyId))
            {
                touchDependentProperty(propertyId.getInternalId());
            }
        }
    }
}

template<class ValueType>
const std::type_info& PropertyAdapterValue<ValueType>::getTypeInfo()
{
    return typeid(ValueType);
}

} // namespace core

#endif // CORE_PROPERTYADAPTERVALUE_H
