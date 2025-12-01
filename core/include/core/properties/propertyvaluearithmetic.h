#ifndef CORE_PROPERTYVALUEARITHMETIC_H
#define CORE_PROPERTYVALUEARITHMETIC_H

#include "core/properties/propertyvalue.h"
#include "core/utils.h"

namespace core
{

template<class ValueType>
class PropertyValueArithmetic : public PropertyValue<ValueType>
{
    using BaseClass = PropertyValue<ValueType>;
    static_assert(std::is_arithmetic_v<ValueType>);

public:
    using AllValuesMap = std::map<ValueType, std::string>;
    explicit PropertyValueArithmetic(PropertyId propertyId, ValueType minValidValue, ValueType maxValidValue, const typename BaseClass::ValidationFunction& validationFunction = nullptr);

    ValueType getMinValidValue() const;
    ValueType getMaxValidValue() const;

private:
    ValueType m_minValidValue;
    ValueType m_maxValidValue;
};

// Impl

template<class ValueType>
PropertyValueArithmetic<ValueType>::PropertyValueArithmetic(PropertyId propertyId, ValueType minValidValue, ValueType maxValidValue, const typename BaseClass::ValidationFunction& validationFunction) :
    BaseClass(propertyId, [this, validationFunction](const ValueType& value)
                        {
                            if (value < m_minValidValue || value > m_maxValidValue)
                            {
                                return VoidResult::createError("Value out of range!", utils::format("value: {} min: {} max: {}", value, m_minValidValue, m_maxValidValue));
                            }

                            if (validationFunction)
                            {
                                return validationFunction(value);
                            }

                            return VoidResult::createOk();
                        }),
    m_minValidValue(minValidValue),
    m_maxValidValue(maxValidValue)
{
    assert(m_minValidValue <= m_maxValidValue);
}

template<class ValueType>
ValueType PropertyValueArithmetic<ValueType>::getMinValidValue() const
{
    return m_minValidValue;
}

template<class ValueType>
ValueType PropertyValueArithmetic<ValueType>::getMaxValidValue() const
{
    return m_maxValidValue;
}

} // namespace core

#endif // CORE_PROPERTYVALUEARITHMETIC_H
