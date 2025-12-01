#ifndef CORE_PROPERTYDEPENDENCYVALIDATOR_H
#define CORE_PROPERTYDEPENDENCYVALIDATOR_H

#include "core/properties/propertyvalues.h"
#include "core/properties/rankedvalidationresult.h"


#include <any>


namespace core
{

class PropertyDependencyValidator
{

protected:
    using IgnoreDependencyValidationFunction = std::function<const bool&()>;
    PropertyDependencyValidator(const std::set<PropertyId>& propertyIds, const std::shared_ptr<PropertyValues>& propertyValues, IgnoreDependencyValidationFunction ignoreDependencyValidationFunction);

public:
    const std::set<PropertyId>& getPropertyIds() const;
    const RankedValidationResult& getValidationResult() const;

    template<class ValueType>
    RankedValidationResult validateWhatIf(PropertyId propertyId, const ValueType& value, PropertyValues::Transaction transaction);

    boost::signals2::signal<void(size_t)> validityChanged;

protected:
    [[nodiscard]] virtual RankedValidationResult validateImpl(PropertyValues::Transaction transaction) = 0;
    [[nodiscard]] virtual RankedValidationResult validateWhatIfImpl(PropertyId propertyId, const std::any& value, PropertyValues::Transaction transaction) = 0;

private:
    void setValidationResult(const RankedValidationResult& result);

    std::set<PropertyId> m_propertyIds;
    IgnoreDependencyValidationFunction m_ignoreDependencyValidationFunction;
    RankedValidationResult m_validationResult {RankedValidationResult::createOk()};
};

// Impl

template<class ValueType>
RankedValidationResult PropertyDependencyValidator::validateWhatIf(PropertyId propertyId, const ValueType& value, PropertyValues::Transaction transaction)
{
    return validateWhatIfImpl(propertyId, value, transaction);
}

} // namespace core

#endif // CORE_PROPERTYDEPENDENCYVALIDATOR_H
