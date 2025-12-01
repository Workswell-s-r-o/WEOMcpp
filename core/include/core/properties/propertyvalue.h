#ifndef CORE_PROPERTYVALUE_H
#define CORE_PROPERTYVALUE_H

#include "core/properties/propertyvaluebase.h"
#include "core/properties/propertyvalueconversion.h"
#include "core/misc/result.h"


namespace core
{

template<class ValueType>
class PropertyValue : public PropertyValueBase
{
    using BaseClass = PropertyValueBase;

public:
    using ValidationFunction = std::function<VoidResult (const ValueType&)>;
    explicit PropertyValue(PropertyId propertyId, const ValidationFunction& validationFunction = nullptr);

    virtual void resetValue() override;

    virtual bool hasValueResult() const override;
    virtual VoidResult getValidationResult() const override;
    virtual std::string getValueAsString() const override;
    virtual bool valueEquals(const PropertyValueBase* other) const override;

    virtual std::string convertToString(const OptionalResult<ValueType>& value) const;

    using ConvertToStringFunction = std::function<std::string (const ValueType& )>;
    void setCustomConvertToStringFunction(const ConvertToStringFunction& convertToStringFunction);

    VoidResult validateValue(const ValueType& value) const;

    const OptionalResult<ValueType>& getCurrentValue() const;
    void setCurrentValue(const OptionalResult<ValueType>& newValue);

protected:
    const ConvertToStringFunction& getCustomConvertToStringFunction() const;

private:
    OptionalResult<ValueType> m_value;

    ValidationFunction m_validationFunction;
    ConvertToStringFunction m_convertToStringFunction;
};

// Impl

template<class ValueType>
PropertyValue<ValueType>::PropertyValue(PropertyId propertyId, const ValidationFunction& validationFunction) :
    BaseClass(propertyId),
    m_validationFunction(validationFunction)
{
}

template<class ValueType>
void PropertyValue<ValueType>::resetValue()
{
    setCurrentValue(std::nullopt);
}

template<class ValueType>
bool PropertyValue<ValueType>::hasValueResult() const
{
    return getCurrentValue().hasResult();
}

template<class ValueType>
VoidResult PropertyValue<ValueType>::getValidationResult() const
{
    return getCurrentValue().containsError() ? getCurrentValue().getResult().toVoidResult() : VoidResult::createOk();
}

template<class ValueType>
std::string PropertyValue<ValueType>::getValueAsString() const
{
    return convertToString(getCurrentValue());
}

template<class ValueType>
bool PropertyValue<ValueType>::valueEquals(const PropertyValueBase* other) const
{
    if (const auto* otherValue = dynamic_cast<const PropertyValue<ValueType>*>(other))
    {
        return getCurrentValue() == otherValue->getCurrentValue();
    }

    assert(false && "Invalid data type!");
    return false;
}

template<class ValueType>
std::string PropertyValue<ValueType>::convertToString(const OptionalResult<ValueType>& value) const
{
    if (!value.containsValue())
    {
        return std::string();
    }

    if (m_convertToStringFunction)
    {
        return m_convertToStringFunction(value.getValue());
    }

    return PropertyValueConversion<ValueType>::toString(value.getValue());
}

template<class ValueType>
void PropertyValue<ValueType>::setCustomConvertToStringFunction(const ConvertToStringFunction& convertToStringFunction)
{
    m_convertToStringFunction = convertToStringFunction;
}

template<class ValueType>
VoidResult PropertyValue<ValueType>::validateValue(const ValueType& value) const
{
    if (m_validationFunction != nullptr)
    {
        return m_validationFunction(value);
    }

    return VoidResult::createOk();
}

template<class ValueType>
const OptionalResult<ValueType>& PropertyValue<ValueType>::getCurrentValue() const
{
    return m_value;
}

template<class ValueType>
void PropertyValue<ValueType>::setCurrentValue(const OptionalResult<ValueType>& newValue)
{
    if (newValue != m_value)
    {
        m_value = newValue;

        valueChanged(this->getPropertyId().getInternalId());
    }
}

template<class ValueType>
const typename PropertyValue<ValueType>::ConvertToStringFunction& PropertyValue<ValueType>::getCustomConvertToStringFunction() const
{
    return m_convertToStringFunction;
}

} // namespace core


#endif // CORE_PROPERTYVALUE_H
