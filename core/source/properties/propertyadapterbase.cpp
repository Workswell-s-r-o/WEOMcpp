#include "core/properties/propertyadapterbase.h"

#include "core/properties/propertydependencyvalidator.h"


namespace core
{

PropertyAdapterBase::PropertyAdapterBase(PropertyId propertyId, const GetStatusForDeviceFunction& getStatusForDeviceFunction) :
    m_propertyId(propertyId),
    m_getStatusForDeviceFunction(getStatusForDeviceFunction)
{
}

PropertyId PropertyAdapterBase::getPropertyId() const
{
    return m_propertyId;
}

bool PropertyAdapterBase::isReadable(const PropertyValues::Transaction& transaction) const
{
    return isReadableStatus(getStatus(transaction));
}

bool PropertyAdapterBase::isWritable(const PropertyValues::Transaction& transaction) const
{
    return isWritableStatus(getStatus(transaction));
}

bool PropertyAdapterBase::isReadableStatus(Status status)
{
    return status == Status::ENABLED_READ_WRITE || status == Status::ENABLED_READ_ONLY;
}

bool PropertyAdapterBase::isWritableStatus(Status status)
{
    return status == Status::ENABLED_READ_WRITE || status == Status::ENABLED_WRITE_ONLY;
}

PropertyAdapterBase::Status PropertyAdapterBase::getStatus(const PropertyValues::Transaction& transaction) const
{
    for (const auto& adapter : m_statusConstraintAdapters)
    {
        adapter->touch(transaction);
    }

    return m_status;
}

void PropertyAdapterBase::updateStatusDeviceChanged(const std::optional<DeviceType>& currentDeviceType, const PropertyValues::Transaction& transaction)
{
    if (m_getStatusForDeviceFunction != nullptr)
    {
        m_statusForDevice = m_getStatusForDeviceFunction(currentDeviceType);

        updateStatusValueChanged(transaction);
    }
}

void PropertyAdapterBase::updateStatusValueChanged(const PropertyValues::Transaction& transaction)
{
    auto newStatus = m_statusForDevice;

    if (m_getStatusConstraintByValuesFunction != nullptr)
    {
        const auto statusConstraint = m_getStatusConstraintByValuesFunction(transaction);

        if ((!isReadableStatus(statusConstraint) && !isWritableStatus(statusConstraint)) ||
            (newStatus == Status::ENABLED_READ_ONLY && !isReadableStatus(statusConstraint)) ||
            (newStatus == Status::ENABLED_WRITE_ONLY && !isWritableStatus(statusConstraint)))
        {
            newStatus = Status::DISABLED;
        }
        else if (newStatus == Status::ENABLED_READ_WRITE)
        {
            assert(isReadableStatus(statusConstraint) || isWritableStatus(statusConstraint));

            if (!isReadableStatus(statusConstraint))
            {
                newStatus = Status::ENABLED_WRITE_ONLY;
            }
            else if (!isWritableStatus(statusConstraint))
            {
                newStatus = Status::ENABLED_READ_ONLY;
            }
        }
    }

    setStatus(newStatus, transaction);
}

void PropertyAdapterBase::setStatusConstraintByValuesFunction(const GetStatusConstraintByValuesFunction& getStatusConstraintByValuesFunction,
                                                              const std::vector<std::shared_ptr<PropertyAdapterBase>>& statusConstraintAdapters,
                                                              PropertyValues* propertyValues)
{
    assert(m_getStatusConstraintByValuesFunction == nullptr);

    m_getStatusConstraintByValuesFunction = getStatusConstraintByValuesFunction;
    m_statusConstraintAdapters = statusConstraintAdapters;

    std::set<size_t> constraintPropertyInternalIds;
    for (const auto& adapter : statusConstraintAdapters)
    {
        constraintPropertyInternalIds.insert(adapter->getPropertyId().getInternalId());
    }

    m_valueChangedConnection = propertyValues->valueChanged.connect([this, constraintPropertyInternalIds](size_t propertyInternalId, core::PropertyValues::Transaction transaction)
    {
        if (constraintPropertyInternalIds.contains(propertyInternalId))
        {
            updateStatusValueChanged(transaction);
        }
    });
}

bool PropertyAdapterBase::isActiveForDeviceType(const std::optional<DeviceType>& deviceType) const
{
    if (m_getStatusForDeviceFunction != nullptr)
    {
        return m_getStatusForDeviceFunction(deviceType) != PropertyAdapterBase::Status::DISABLED;
    }

    return false;
}

std::string PropertyAdapterBase::getValueAsString(const PropertyValues::Transaction& transaction)
{
    touch(transaction);

    return transaction.getValueAsString(getPropertyId());
}

void PropertyAdapterBase::addDependencyValidator(const std::shared_ptr<PropertyDependencyValidator>& validator)
{
    assert(validator->getPropertyIds().find(getPropertyId()) != validator->getPropertyIds().end());

    m_dependencyValidators.push_back(validator);

    for (const auto property : validator->getPropertyIds())
    {
        if (property != getPropertyId())
        {
            m_validationDependencyPropertyIds.insert(property);
        }
    }
}

std::vector<RankedValidationResult> PropertyAdapterBase::getValueDependencyValidationResults() const
{
    std::vector<RankedValidationResult> dependencyValidationResults;

    for (const auto& validator : m_dependencyValidators)
    {
        const auto result = validator->getValidationResult();
        if (!result.getResult().isOk())
        {
            dependencyValidationResults.push_back(result);
        }
    }

    return dependencyValidationResults;
}

const std::set<PropertyId>& PropertyAdapterBase::getValidationDependencyPropertyIds() const
{
    return m_validationDependencyPropertyIds;
}

connection::AddressRanges PropertyAdapterBase::getAddressRanges() const
{
    return {};
}

std::set<PropertyId> PropertyAdapterBase::getSourcePropertyIds() const
{
    return {};
}

const std::set<PropertyId>& PropertyAdapterBase::getSubsidiaryAdaptersPropertyIds() const
{
    return m_subsidiaryAdaptersPropertyIds;
}

void PropertyAdapterBase::addSubsidiaryAdaptersPropertyId(PropertyId propertyId)
{
    VERIFY(m_subsidiaryAdaptersPropertyIds.insert(propertyId).second);
}

void PropertyAdapterBase::removeSubsidiaryAdaptersPropertyId(PropertyId propertyId)
{
    VERIFY(m_subsidiaryAdaptersPropertyIds.erase(propertyId));
}

const std::vector<std::shared_ptr<PropertyDependencyValidator>>& PropertyAdapterBase::getDependencyValidators() const
{
    return m_dependencyValidators;
}

void PropertyAdapterBase::setStatus(Status status, const PropertyValues::Transaction& transaction)
{
    if (status != m_status)
    {
        m_status = status;
        if (!isReadableStatus(m_status))
        {
            transaction.resetValue(getPropertyId());
        }

        statusChanged(getPropertyId().getInternalId(), m_status);
    }
}

} // namespace core
