#ifndef CORE_PROPERTYDEPENDENCYVALIDATORFOR2_H
#define CORE_PROPERTYDEPENDENCYVALIDATORFOR2_H

#include "core/properties/propertydependencyvalidator.h"
#include "core/utils.h"

namespace core
{

template<class ValueType1, class ValueType2>
class PropertyDependencyValidatorFor2 : public PropertyDependencyValidator
{
    using BaseClass = PropertyDependencyValidator;

public:
    using IgnoreDependencyValidationFunction = std::function<const bool&()>;
    using DependencyValidationFunction = std::function<RankedValidationResult (const OptionalResult<ValueType1>& value1, const OptionalResult<ValueType2>& value2)>;
    PropertyDependencyValidatorFor2(PropertyId propertyId1, PropertyId propertyId2, const DependencyValidationFunction& dependencyValidationFunction,
                                    const std::shared_ptr<PropertyValues>& propertyValues, IgnoreDependencyValidationFunction ignoreDependencyValidationFunction);

protected:
    [[nodiscard]] virtual RankedValidationResult validateImpl(PropertyValues::Transaction transaction) override;
    [[nodiscard]] virtual RankedValidationResult validateWhatIfImpl(PropertyId propertyId, const std::any& value, PropertyValues::Transaction transaction) override;

private:
    PropertyId m_propertyId1;
    PropertyId m_propertyId2;
    DependencyValidationFunction m_dependencyValidationFunction;
    IgnoreDependencyValidationFunction m_ignoreDependencyValidationFunction;
};

// Impl

template<class ValueType1, class ValueType2>
PropertyDependencyValidatorFor2<ValueType1, ValueType2>::PropertyDependencyValidatorFor2(PropertyId propertyId1, PropertyId propertyId2, const DependencyValidationFunction& dependencyValidationFunction,
                                                                                         const std::shared_ptr<PropertyValues>& propertyValues, IgnoreDependencyValidationFunction ignoreDependencyValidationFunction) :
    BaseClass({propertyId1, propertyId2}, propertyValues, ignoreDependencyValidationFunction),
    m_propertyId1(propertyId1),
    m_propertyId2(propertyId2),
    m_dependencyValidationFunction(dependencyValidationFunction),
    m_ignoreDependencyValidationFunction(ignoreDependencyValidationFunction)
{
}

template<class ValueType1, class ValueType2>
RankedValidationResult PropertyDependencyValidatorFor2<ValueType1, ValueType2>::validateImpl(PropertyValues::Transaction transaction)
{
    if(m_ignoreDependencyValidationFunction)
    {
        return RankedValidationResult::createOk();
    }
    const auto value1 = transaction.getValue<ValueType1>(m_propertyId1);
    const auto value2 = transaction.getValue<ValueType2>(m_propertyId2);

    return m_dependencyValidationFunction(value1, value2);
}

template<class ValueType1, class ValueType2>
RankedValidationResult PropertyDependencyValidatorFor2<ValueType1, ValueType2>::validateWhatIfImpl(PropertyId propertyId, const std::any& value, PropertyValues::Transaction transaction)
{
    auto value1 = transaction.getValue<ValueType1>(m_propertyId1);
    auto value2 = transaction.getValue<ValueType2>(m_propertyId2);

    try
    {
        if (propertyId == m_propertyId1)
        {
            value1 = std::any_cast<ValueType1>(value);
        }
        else if (propertyId == m_propertyId2)
        {
            value2 = std::any_cast<ValueType2>(value);
        }
        else
        {
            assert(false && "Invalid property!");
            return RankedValidationResult::createError("Validation error!", utils::format("invalid property: {} expected: {} or {}", propertyId.getIdString(), m_propertyId1.getIdString(), m_propertyId2.getIdString()));
        }
    }
    catch (const std::bad_any_cast& e)
    {
        assert(false && "Invalid property type!");
        return RankedValidationResult::createError("Validation error!", utils::format("invalid property type: {}", e.what()));
    }

    return m_dependencyValidationFunction(value1, value2);
}

} // namespace core

#endif // CORE_PROPERTYDEPENDENCYVALIDATORFOR2_H
