#include "core/properties/propertydependencyvalidator.h"

namespace core
{

PropertyDependencyValidator::PropertyDependencyValidator(const std::set<PropertyId>& propertyIds, const std::shared_ptr<PropertyValues>& propertyValues, IgnoreDependencyValidationFunction ignoreDependencyValidationFunction) :
    m_propertyIds(propertyIds),
    m_ignoreDependencyValidationFunction(ignoreDependencyValidationFunction)
{
    propertyValues.get()->valueChanged.connect([this](size_t propertyInternalId, PropertyValues::Transaction transaction)
    {
        const auto propertyId = core::PropertyId::getPropertyIdByInternalId(propertyInternalId);
        assert(propertyId.has_value() && "Invalid id!");
        if (!m_ignoreDependencyValidationFunction && propertyId.has_value() && m_propertyIds.find(propertyId.value()) != m_propertyIds.end())
        {
            setValidationResult(validateImpl(transaction));
        }
    });
}

const std::set<PropertyId>& PropertyDependencyValidator::getPropertyIds() const
{
    return m_propertyIds;
}

const RankedValidationResult& PropertyDependencyValidator::getValidationResult() const
{
    return m_validationResult;
}

void PropertyDependencyValidator::setValidationResult(const RankedValidationResult& result)
{
    if (m_validationResult != result)
    {
        m_validationResult = result;

        for (const auto property : m_propertyIds)
        {
            validityChanged(property.getInternalId());
        }
    }
}

} // namespace core
